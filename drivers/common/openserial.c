/**
\brief Definition of the "openserial" driver.

\author Fabien Chraim <chraim@eecs.berkeley.edu>, March 2012.
\author Thomas Watteyne <thomas.watteyne@inria.fr>, August 2016.
*/

#include "opendefs.h"
#include "openserial.h"
#include "IEEE802154E.h"
#include "neighbors.h"
#include "sixtop.h"
#include "icmpv6echo.h"
#include "idmanager.h"
#include "openqueue.h"
#include "openbridge.h"
#include "leds.h"
#include "schedule.h"
#include "uart.h"
#include "opentimers.h"
#include "openhdlc.h"
#include "schedule.h"
#include "icmpv6rpl.h"
#include "icmpv6echo.h"
#include "sf0.h"

//=========================== variables =======================================

openserial_vars_t openserial_vars;

//=========================== prototypes ======================================

// printing
owerror_t openserial_printInfoErrorCritical(
    char             severity,
    uint8_t          calling_component,
    uint8_t          error_code,
    errorparameter_t arg1,
    errorparameter_t arg2
);

owerror_t openserial_push_output_buffer(char *buffer_out, uint8_t buffer_out_length);
int8_t openserial_get_output_buffer(char *buffer, uint8_t length);

// command handlers
void openserial_handleEcho(uint8_t* but, uint8_t bufLen);
void openserial_handleCommands(void);

// misc
void openserial_board_reset_cb(
    opentimer_id_t id
);

// HDLC output
void outputHdlcOpen(uint8_t bufindex);
void outputHdlcWrite(uint8_t bufindex, uint8_t b);
void outputHdlcClose(uint8_t bufindex);

// HDLC input
void inputHdlcOpen(void);
void inputHdlcWrite(uint8_t b);
void inputHdlcClose(void);

//=========================== public ==========================================

//===== admin

void openserial_init() {
    uint16_t crc;
    
    // reset variable
    memset(&openserial_vars,0,sizeof(openserial_vars_t));
    
    // admin
    openserial_vars.mode               = MODE_OFF;
    openserial_vars.debugPrintCounter  = 0;
    
    // input
    openserial_vars.reqFrame[0]        = HDLC_FLAG;
    openserial_vars.reqFrame[1]        = SERFRAME_MOTE2PC_REQUEST;
    crc = HDLC_CRCINIT;
    crc = crcIteration(crc,openserial_vars.reqFrame[1]);
    crc = ~crc;
    openserial_vars.reqFrame[2]        = (crc>>0)&0xff;
    openserial_vars.reqFrame[3]        = (crc>>8)&0xff;
    openserial_vars.reqFrame[4]        = HDLC_FLAG;
    openserial_vars.reqFrameIdx        = 0;
    openserial_vars.lastRxByte         = HDLC_FLAG;
    openserial_vars.busyReceiving      = FALSE;
    openserial_vars.inputEscaping      = FALSE;
    openserial_vars.inputBufFill       = 0;
    
    uint8_t i;
    openserial_vars.outputCurrentW  = 0;
	openserial_vars.outputCurrentR  = 0;
	for(i=0; i<OPENSERIAL_OUTPUT_NBBUFFERS; i++){
		openserial_vars.outputBufFilled[i] = FALSE;
		openserial_vars.outputBufIdxR[i]       = 0;
		openserial_vars.outputBufIdxW[i]       = 0;

		// set callbacks
		uart_setCallbacks(isr_openserial_tx,isr_openserial_rx);
    }
}

void openserial_register(openserial_rsvpt* rsvp) {
    // FIXME: register multiple commands (linked list)
    openserial_vars.registeredCmd = rsvp;
}

//===== printing

owerror_t openserial_printStatus(uint8_t statusElement, uint8_t* buffer_in, uint8_t buffer_in_length) {
	 char     buffer_out[256];     //the message to send
	 uint8_t  buffer_out_length = 0;   //its length

	 //prepare the headers  and the content
	 buffer_out[buffer_out_length++] = SERFRAME_MOTE2PC_STATUS;
	 buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[0];
	 buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[1];
	 buffer_out[buffer_out_length++] = statusElement;

	 //copy the "payload"
	 memcpy(&(buffer_out[buffer_out_length]), buffer_in, buffer_in_length);
	 buffer_out_length += buffer_in_length;

	 //push the buffer
	 return(openserial_push_output_buffer(buffer_out, buffer_out_length));
}

owerror_t openserial_printInfo(
    uint8_t             calling_component,
    uint8_t             error_code,
    errorparameter_t    arg1,
    errorparameter_t    arg2
) {
    return openserial_printInfoErrorCritical(
        SERFRAME_MOTE2PC_INFO,
        calling_component,
        error_code,
        arg1,
        arg2
    );
}

owerror_t openserial_printError(
    uint8_t             calling_component,
    uint8_t             error_code,
    errorparameter_t    arg1,
    errorparameter_t    arg2
) {
    // toggle error LED
    leds_error_toggle();
    
    return openserial_printInfoErrorCritical(
        SERFRAME_MOTE2PC_ERROR,
        calling_component,
        error_code,
        arg1,
        arg2
    );
}

owerror_t openserial_printCritical(
    uint8_t             calling_component,
    uint8_t             error_code,
    errorparameter_t    arg1,
    errorparameter_t    arg2
) {
    // blink error LED, this is serious
    leds_error_blink();
    
    // schedule for the mote to reboot in 10s
    opentimers_start(
        10000,
        TIMER_ONESHOT,
        TIME_MS,
        openserial_board_reset_cb
    );
    
    return openserial_printInfoErrorCritical(
        SERFRAME_MOTE2PC_CRITICAL,
        calling_component,
        error_code,
        arg1,
        arg2
    );
}

owerror_t openserial_printData(uint8_t* buffer_in, uint8_t buffer_in_length) {
	char     buffer_out[256];     //the message to send
	uint8_t  buffer_out_length = 0;   //its length
	uint8_t  asn[5];

	//prepare the headers  and the content
	ieee154e_getAsn(asn);// byte01,byte23,byte4
	buffer_out[buffer_out_length++] = SERFRAME_MOTE2PC_DATA;
	buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[0];
	buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[1];
	buffer_out[buffer_out_length++] = asn[0];
	buffer_out[buffer_out_length++] = asn[1];
	buffer_out[buffer_out_length++] = asn[2];
	buffer_out[buffer_out_length++] = asn[3];
	buffer_out[buffer_out_length++] = asn[4];

	//copy the "payload"
	memcpy(&(buffer_out[buffer_out_length]), buffer_in, buffer_in_length);
	buffer_out_length += buffer_in_length;

	//push the buffer
    return(openserial_push_output_buffer(buffer_out, buffer_out_length));
}

owerror_t openserial_printSniffedPacket(uint8_t* buffer_in, uint8_t buffer_in_length, uint8_t channel) {
	char     buffer_out[256];     //the message to send
	uint8_t  buffer_out_length = 0;   //its length

	//prepare the headers  and the content
	buffer_out[buffer_out_length++] = SERFRAME_MOTE2PC_SNIFFED_PACKET;
	buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[0];
	buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[1];

	//copy the "payload"
	memcpy(&(buffer_out[buffer_out_length]), buffer_in, buffer_in_length);
	buffer_out_length += buffer_in_length;
	buffer_out[buffer_out_length++] = channel;

	//push the buffer
    return(openserial_push_output_buffer(buffer_out, buffer_out_length));
}

//===== retrieving inputBuffer

uint8_t openserial_getInputBufferFilllevel() {
    uint8_t inputBufFill;
    INTERRUPT_DECLARATION();
    
    DISABLE_INTERRUPTS();
    inputBufFill = openserial_vars.inputBufFill;
    ENABLE_INTERRUPTS();
    
    return inputBufFill-1; // removing the command byte
}

uint8_t openserial_getInputBuffer(uint8_t* bufferToWrite, uint8_t maxNumBytes) {
    uint8_t numBytesWritten;
    uint8_t inputBufFill;
    INTERRUPT_DECLARATION();
    
    DISABLE_INTERRUPTS();
    inputBufFill = openserial_vars.inputBufFill;
    ENABLE_INTERRUPTS();
     
    if (maxNumBytes<inputBufFill-1) {
        openserial_printError(
            COMPONENT_OPENSERIAL,
            ERR_GETDATA_ASKS_TOO_FEW_BYTES,
            (errorparameter_t)maxNumBytes,
            (errorparameter_t)inputBufFill-1
        );
        numBytesWritten = 0;
    } else {
        numBytesWritten = inputBufFill-1;
        memcpy(bufferToWrite,&(openserial_vars.inputBuf[1]),numBytesWritten);
    }
    
    return numBytesWritten;
}

//===== scheduling

void openserial_startInput() {
    INTERRUPT_DECLARATION();
    
    if (openserial_vars.inputBufFill>0) {
        openserial_printError(
            COMPONENT_OPENSERIAL,
            ERR_INPUTBUFFER_LENGTH,
            (errorparameter_t)openserial_vars.inputBufFill,
            (errorparameter_t)0
        );
        DISABLE_INTERRUPTS();
        openserial_vars.inputBufFill=0;
        ENABLE_INTERRUPTS();
    }
    
    uart_clearTxInterrupts();
    uart_clearRxInterrupts();      // clear possible pending interrupts
    uart_enableInterrupts();       // Enable USCI_A1 TX & RX interrupt
    
    DISABLE_INTERRUPTS();
    openserial_vars.busyReceiving  = FALSE;
    openserial_vars.mode           = MODE_INPUT;
    openserial_vars.reqFrameIdx    = 0;
#ifdef FASTSIM
    uart_writeBufferByLen_FASTSIM(
        openserial_vars.reqFrame,
        sizeof(openserial_vars.reqFrame)
    );
    openserial_vars.reqFrameIdx = sizeof(openserial_vars.reqFrame);
#else
    uart_writeByte(openserial_vars.reqFrame[openserial_vars.reqFrameIdx]);
#endif
    ENABLE_INTERRUPTS();
}

void openserial_startOutput() {
    uint8_t debugPrintCounter;
    INTERRUPT_DECLARATION();
    
    //=== made modules print debug information
    
    DISABLE_INTERRUPTS();
    openserial_vars.debugPrintCounter = (openserial_vars.debugPrintCounter+1)%STATUS_MAX;
    debugPrintCounter = openserial_vars.debugPrintCounter;
    ENABLE_INTERRUPTS();
    
    switch (debugPrintCounter) {
        case STATUS_ISSYNC:
            if (debugPrint_isSync()==TRUE) {
                break;
            }
        case STATUS_ID:
            if (debugPrint_id()==TRUE) {
               break;
            }
        case STATUS_DAGRANK:
            if (debugPrint_myDAGrank()==TRUE) {
                break;
            }
        case STATUS_OUTBUFFERINDEXES:
            if (debugPrint_outBufferIndexes()==TRUE) {
                break;
            }
        case STATUS_ASN:
            if (debugPrint_asn()==TRUE) {
                break;
            }
        case STATUS_MACSTATS:
            if (debugPrint_macStats()==TRUE) {
                break;
            }
        case STATUS_SCHEDULE:
            if(debugPrint_schedule()==TRUE) {
                break;
            }
        case STATUS_BACKOFF:
            if(debugPrint_backoff()==TRUE) {
                break;
            }
        case STATUS_QUEUE:
            if(debugPrint_queue()==TRUE) {
                break;
            }
        case STATUS_NEIGHBORS:
            if (debugPrint_neighbors()==TRUE) {
                break;
            }
        case STATUS_KAPERIOD:
            if (debugPrint_kaPeriod()==TRUE) {
                break;
            }
        default:
            DISABLE_INTERRUPTS();
            openserial_vars.debugPrintCounter=0;
            ENABLE_INTERRUPTS();
    }
    
    //=== flush TX buffer
    
    uart_clearTxInterrupts();
    uart_clearRxInterrupts();          // clear possible pending interrupts
    uart_enableInterrupts();           // Enable USCI_A1 TX & RX interrupt
    
    uint8_t bufindex = openserial_vars.outputCurrentR;


    DISABLE_INTERRUPTS();

    openserial_vars.mode=MODE_OUTPUT;
    if (openserial_vars.outputBufFilled[bufindex]) {
#ifdef FASTSIM
    	 uart_writeCircularBuffer_FASTSIM(openserial_vars.outputBuf[bufindex],&openserial_vars.outputBufIdxR[bufindex],&openserial_vars.outputBufIdxW[bufindex]);
#else
    	 uart_writeByte(openserial_vars.outputBuf[bufindex][openserial_vars.outputBufIdxR[bufindex]++]);
#endif
    } else {
        openserial_stop();
    }
    ENABLE_INTERRUPTS();
}

void openserial_stop() {
    uint8_t inputBufFill;
    uint8_t cmdByte;
    bool    busyReceiving;
    INTERRUPT_DECLARATION();

    DISABLE_INTERRUPTS();
    busyReceiving = openserial_vars.busyReceiving;
    inputBufFill  = openserial_vars.inputBufFill;
    ENABLE_INTERRUPTS();

    // disable UART interrupts
    uart_disableInterrupts();

    DISABLE_INTERRUPTS();
    openserial_vars.mode=MODE_OFF;
    ENABLE_INTERRUPTS();
    
    // the inputBuffer has to be reset if it is not reset where the data is read.
    // or the function openserial_getInputBuffer is called (which resets the buffer)
    if (busyReceiving==TRUE) {
        openserial_printError(
            COMPONENT_OPENSERIAL,
            ERR_BUSY_RECEIVING,
            (errorparameter_t)0,
            (errorparameter_t)inputBufFill
        );
    }
    
    if (busyReceiving==FALSE && inputBufFill>0) {
        DISABLE_INTERRUPTS();
        cmdByte = openserial_vars.inputBuf[0];
        ENABLE_INTERRUPTS();
        // call hard-coded commands
        // FIXME: needs to be replaced by registered commands only
        switch (cmdByte) {
            case SERFRAME_PC2MOTE_SETROOT:
                idmanager_triggerAboutRoot();
                break;
            case SERFRAME_PC2MOTE_RESET:
                board_reset();
                break;
            case SERFRAME_PC2MOTE_DATA:
                openbridge_triggerData();
                break;
            case SERFRAME_PC2MOTE_TRIGGERSERIALECHO:
                openserial_handleEcho(&openserial_vars.inputBuf[1],inputBufFill-1);
                break;
            case SERFRAME_PC2MOTE_COMMAND:
                openserial_handleCommands();
                break;
        }
        // call registered commands
        if (openserial_vars.registeredCmd!=NULL && openserial_vars.registeredCmd->cmdId==cmdByte) {
            
            openserial_vars.registeredCmd->cb();
        }
    }
    
    DISABLE_INTERRUPTS();
    openserial_vars.inputBufFill  = 0;
    openserial_vars.busyReceiving = FALSE;
    ENABLE_INTERRUPTS();
}

//===== debugprint

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_outBufferIndexes() {
	uint16_t temp_buffer[4];
	INTERRUPT_DECLARATION();
	DISABLE_INTERRUPTS();
	temp_buffer[0] = openserial_vars.outputBufIdxW[openserial_vars.outputCurrentR];
	temp_buffer[1] = openserial_vars.outputBufIdxR[openserial_vars.outputCurrentR];
	temp_buffer[2] = openserial_vars.outputCurrentW;
	temp_buffer[3] = openserial_vars.outputCurrentR;
	ENABLE_INTERRUPTS();
	openserial_printStatus(STATUS_OUTBUFFERINDEXES,(uint8_t*)temp_buffer,sizeof(temp_buffer));
	return TRUE;
}

//=========================== private =========================================

//===== printing

owerror_t openserial_printInfoErrorCritical(
    char             severity,
    uint8_t          calling_component,
    uint8_t          error_code,
    errorparameter_t arg1,
    errorparameter_t arg2
) {
	 char     buffer_out[256];     //the message to send
	 uint8_t  buffer_out_length = 0;   //its length

	 //prepare the headers  and the content
	 buffer_out[buffer_out_length++] = severity;
	 buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[0];
	 buffer_out[buffer_out_length++] = idmanager_getMyID(ADDR_16B)->addr_16b[1];
	 buffer_out[buffer_out_length++] = calling_component;
	 buffer_out[buffer_out_length++] = error_code;
	 buffer_out[buffer_out_length++] = (uint8_t)((arg1 & 0xff00)>>8);
	 buffer_out[buffer_out_length++] = (uint8_t) (arg1 & 0x00ff);
	 buffer_out[buffer_out_length++] = (uint8_t)((arg2 & 0xff00)>>8);
	 buffer_out[buffer_out_length++] = (uint8_t) (arg2 & 0x00ff);

	 //push the buffer
	return(openserial_push_output_buffer(buffer_out, buffer_out_length));
}

//===== command handlers

void openserial_handleEcho(uint8_t* buf, uint8_t bufLen){
    
    // echo back what you received
    openserial_printData(
        buf,
        bufLen
    );
}

void openserial_handleCommands(void){
   uint8_t  input_buffer[10];
   uint8_t  numDataBytes;
   uint8_t  commandId;
   uint8_t  commandLen;
   uint8_t  comandParam_8;
   uint16_t comandParam_16;
   cellInfo_ht cellList[SCHEDULEIEMAXNUMCELLS];
   uint8_t  i;
   
   open_addr_t neighbor;
   bool        foundNeighbor;
   
   memset(cellList,0,sizeof(cellList));
   
   numDataBytes = openserial_getInputBufferFilllevel();
   //copying the buffer
   openserial_getInputBuffer(&(input_buffer[0]),numDataBytes);
   commandId  = openserial_vars.inputBuf[1];
   commandLen = openserial_vars.inputBuf[2];
   
   if (commandLen>3) {
       // the max command Len is 2, except ping commands
       return;
   } else {
       if (commandLen == 1) {
           comandParam_8 = openserial_vars.inputBuf[3];
       } else {
           // commandLen == 2
           comandParam_16 = (openserial_vars.inputBuf[3]      & 0x00ff) | \
                            ((openserial_vars.inputBuf[4]<<8) & 0xff00); 
       }
   }
   
   switch(commandId) {
       case COMMAND_SET_EBPERIOD:
           sixtop_setEBPeriod(comandParam_8); // one byte, in seconds
           break;
       case COMMAND_SET_CHANNEL:
           // set communication channel for protocol stack
           ieee154e_setSingleChannel(comandParam_8); // one byte
           // set listenning channel for sniffer
           sniffer_setListeningChannel(comandParam_8); // one byte
           break;
       case COMMAND_SET_KAPERIOD: // two bytes, in slots
           sixtop_setKaPeriod(comandParam_16);
           break;
       case COMMAND_SET_DIOPERIOD: // two bytes, in mili-seconds
           icmpv6rpl_setDIOPeriod(comandParam_16);
           break;
       case COMMAND_SET_DAOPERIOD: // two bytes, in mili-seconds
           icmpv6rpl_setDAOPeriod(comandParam_16);
           break;
       case COMMAND_SET_DAGRANK: // two bytes
           icmpv6rpl_setMyDAGrank(comandParam_16);
           break;
       case COMMAND_SET_SECURITY_STATUS: // one byte
           if (comandParam_8 ==1) {
               ieee154e_setIsSecurityEnabled(TRUE);
           } else {
               if (comandParam_8 == 0) {
                  ieee154e_setIsSecurityEnabled(FALSE);
               } else {
                   // security only can be 1 or 0 
                   break;
               }
           }
           break;
       case COMMAND_SET_SLOTFRAMELENGTH: // two bytes
           schedule_setFrameLength(comandParam_16);
           break;
       case COMMAND_SET_ACK_STATUS:
           if (comandParam_8 == 1) {
               ieee154e_setIsAckEnabled(TRUE);
           } else {
               if (comandParam_8 == 0) {
                   ieee154e_setIsAckEnabled(FALSE);
               } else {
                   // ack reply
                   break;
               }
           }
           break;
        case COMMAND_SET_6P_ADD:
        case COMMAND_SET_6P_DELETE:
        case COMMAND_SET_6P_COUNT:
        case COMMAND_SET_6P_LIST:
        case COMMAND_SET_6P_CLEAR:
            // get preferred parent
            foundNeighbor =icmpv6rpl_getPreferredParentEui64(&neighbor);
            if (foundNeighbor==FALSE) {
                break;
            }
            
            if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
                // one sixtop transcation is happening, only one instance at one time
                return;
            }
            if ( 
                (
                  commandId != COMMAND_SET_6P_ADD &&
                  commandId != COMMAND_SET_6P_DELETE
                ) ||
                (
                    ( 
                      commandId == COMMAND_SET_6P_ADD ||
                      commandId == COMMAND_SET_6P_DELETE
                    ) && 
                    commandLen == 0
                ) 
            ){
                // randomly select cell
                sixtop_request(commandId-8,&neighbor,1);
            } else {
                for (i=0;i<commandLen;i++){
                    cellList[i].tsNum           = openserial_vars.inputBuf[3+i];
                    cellList[i].choffset        = DEFAULT_CHANNEL_OFFSET;
                    cellList[i].linkoptions     = CELLTYPE_TX;
                }
                sixtop_addORremoveCellByInfo(commandId-8,&neighbor,cellList);
            }
            break;
       case COMMAND_SET_SLOTDURATION:
            ieee154e_setSlotDuration(comandParam_16);
            break;
       case COMMAND_SET_6PRESPONSE:
            if (comandParam_8 ==1) {
               sixtop_setIsResponseEnabled(TRUE);
            } else {
                if (comandParam_8 == 0) {
                    sixtop_setIsResponseEnabled(FALSE);
                } else {
                    // security only can be 1 or 0 
                    break;
                }
            }
            break;
       case COMMAND_SET_UINJECTPERIOD:
            sf0_appPktPeriod(comandParam_8);
            break;
       case COMMAND_SET_ECHO_REPLY_STATUS:
            if (comandParam_8 == 1) {
                icmpv6echo_setIsReplyEnabled(TRUE);
            } else {
                if (comandParam_8 == 0) {
                    icmpv6echo_setIsReplyEnabled(FALSE);
                } else {
                    // ack reply
                    break;
                }
            }
            break;
       default:
           // wrong command ID
           break;
   }
}

//===== misc

void openserial_board_reset_cb(opentimer_id_t id) {
    board_reset();
}

//===== hdlc (output)

/**
\brief Start an HDLC frame in the output buffer.
*/
port_INLINE void outputHdlcOpen(uint8_t bufindex) {
	// initialize the value of the CRC
	openserial_vars.outputCrc[bufindex] = HDLC_CRCINIT;

	// write the opening HDLC flag
	openserial_vars.outputBuf[bufindex][openserial_vars.outputBufIdxW[bufindex]++] = HDLC_FLAG;
}

/**
\brief Add a byte to the outgoing HDLC frame being built.
*/
port_INLINE void outputHdlcWrite(uint8_t bufindex, uint8_t b) {
	//buffer overflow: the last cell overwrites the first one!
	if (((uint8_t) openserial_vars.outputBufIdxW[bufindex] + 1) == openserial_vars.outputBufIdxR[bufindex]){
		openserial_printCritical(COMPONENT_OPENSERIAL, ERR_OPENSERIAL_BUFFER_OVERFLOW,(errorparameter_t)0,(errorparameter_t)255);
		return;
	}

	// iterate through CRC calculator
	openserial_vars.outputCrc[bufindex] = crcIteration(openserial_vars.outputCrc[bufindex],b);

	// add byte to buffer
	if (b==HDLC_FLAG || b==HDLC_ESCAPE) {
		openserial_vars.outputBuf[bufindex][openserial_vars.outputBufIdxW[bufindex]++]  = HDLC_ESCAPE;
		b                                               = b^HDLC_ESCAPE_MASK;
	}
	openserial_vars.outputBuf[bufindex][openserial_vars.outputBufIdxW[bufindex]++]     = b;
}
/**
\brief Finalize the outgoing HDLC frame.
*/
port_INLINE void outputHdlcClose(uint8_t bufindex) {
	uint16_t   finalCrc;

	// finalize the calculation of the CRC
	finalCrc   = ~openserial_vars.outputCrc[bufindex];

	// write the CRC value
	outputHdlcWrite(bufindex, (finalCrc>>0)&0xff);
	outputHdlcWrite(bufindex, (finalCrc>>8)&0xff);

	// write the closing HDLC flag
	openserial_vars.outputBuf[bufindex][openserial_vars.outputBufIdxW[bufindex]++]   = HDLC_FLAG;
}

//===== hdlc (input)

/**
\brief Start an HDLC frame in the input buffer.
*/
port_INLINE void inputHdlcOpen(void) {
    // reset the input buffer index
	openserial_vars.inputBufFill = 0;

    // initialize the value of the CRC
    openserial_vars.inputCrc = HDLC_CRCINIT;
}

/**
\brief Add a byte to the incoming HDLC frame.
*/
port_INLINE void inputHdlcWrite(uint8_t b) {
    if (b==HDLC_ESCAPE) {
        openserial_vars.inputEscaping = TRUE;
    } else {
        if (openserial_vars.inputEscaping==TRUE) {
            b                             = b^HDLC_ESCAPE_MASK;
            openserial_vars.inputEscaping = FALSE;
        }
        
        // add byte to input buffer
        openserial_vars.inputBuf[openserial_vars.inputBufFill] = b;
        openserial_vars.inputBufFill++;
        
        // iterate through CRC calculator
        openserial_vars.inputCrc = crcIteration(openserial_vars.inputCrc,b);
    }
}
/**
\brief Finalize the incoming HDLC frame.
*/
port_INLINE void inputHdlcClose() {
    
    // verify the validity of the frame
    if (openserial_vars.inputCrc==HDLC_CRCGOOD) {
        // the CRC is correct
        
        // remove the CRC from the input buffer
        openserial_vars.inputBufFill    -= 2;
    } else {
        // the CRC is incorrect
        
        // drop the incoming fram
        openserial_vars.inputBufFill     = 0;
    }
}

//=========================== interrupt handlers ==============================

// executed in ISR, called from scheduler.c
void isr_openserial_tx() {
	uint8_t bufindex;

	switch (openserial_vars.mode) {
		case MODE_INPUT:
			openserial_vars.reqFrameIdx++;
			if (openserial_vars.reqFrameIdx<sizeof(openserial_vars.reqFrame)) {
				uart_writeByte(openserial_vars.reqFrame[openserial_vars.reqFrameIdx]);
			}
			break;
		case MODE_OUTPUT:
				bufindex  = openserial_vars.outputCurrentR;

				//that's the end of this buffer
				if (openserial_vars.outputBufIdxW[bufindex]==openserial_vars.outputBufIdxR[bufindex]) {
				   openserial_vars.outputBufFilled[bufindex] = FALSE;

				   //considers the next buffer only if this one was entirely filled and the written one is the next one
				   if (openserial_vars.outputCurrentW != openserial_vars.outputCurrentR)
					  openserial_vars.outputCurrentR = (1 + openserial_vars.outputCurrentR) % OPENSERIAL_OUTPUT_NBBUFFERS;
				}

				//we push the next byte
				else if (openserial_vars.outputBufFilled[bufindex]) {
				   uart_writeByte(openserial_vars.outputBuf[bufindex][openserial_vars.outputBufIdxR[bufindex]++]);
				}

				break;
		case MODE_OFF:
	            default:
	            break;
	 }
}

// executed in ISR, called from scheduler.c
void isr_openserial_rx() {
    uint8_t rxbyte;
    uint8_t inputBufFill;

    // stop if I'm not in input mode
    if (openserial_vars.mode!=MODE_INPUT) {
        return;
    }

    // read byte just received
    rxbyte = uart_readByte();
    // keep length
    inputBufFill=openserial_vars.inputBufFill;
    
    if (
        openserial_vars.busyReceiving==FALSE  &&
        openserial_vars.lastRxByte==HDLC_FLAG &&
        rxbyte!=HDLC_FLAG
    ) {
        // start of frame

        // I'm now receiving
        openserial_vars.busyReceiving         = TRUE;

        // create the HDLC frame
        inputHdlcOpen();

        // add the byte just received
        inputHdlcWrite(rxbyte);
    } else if (
        openserial_vars.busyReceiving==TRUE   &&
        rxbyte!=HDLC_FLAG
    ) {
        // middle of frame

        // add the byte just received
        inputHdlcWrite(rxbyte);
        if (openserial_vars.inputBufFill+1>SERIAL_INPUT_BUFFER_SIZE){
            // input buffer overflow
            openserial_printError(
                COMPONENT_OPENSERIAL,
                ERR_INPUT_BUFFER_OVERFLOW,
                (errorparameter_t)0,
                (errorparameter_t)0
            );
            openserial_vars.inputBufFill       = 0;
            openserial_vars.busyReceiving      = FALSE;
            openserial_stop();
        }
    } else if (
        openserial_vars.busyReceiving==TRUE   &&
        rxbyte==HDLC_FLAG
    ) {
        // end of frame
        
        // finalize the HDLC frame
        inputHdlcClose();
        
        if (openserial_vars.inputBufFill==0){
            // invalid HDLC frame
            openserial_printError(
                COMPONENT_OPENSERIAL,
                ERR_WRONG_CRC_INPUT,
                (errorparameter_t)inputBufFill,
                (errorparameter_t)0
            );
        }
         
        openserial_vars.busyReceiving      = FALSE;
        openserial_stop();
    }
    
    openserial_vars.lastRxByte = rxbyte;
}


owerror_t openserial_push_output_buffer(char *buffer_out, uint8_t buffer_out_length){
   uint8_t  buf_id;              //the buffer to use
   uint8_t  buffer_out_index;    //to walk in the buffer to push

   INTERRUPT_DECLARATION();

    DISABLE_INTERRUPTS();
    buf_id = openserial_get_output_buffer(buffer_out, buffer_out_length);
    if (buf_id >= OPENSERIAL_OUTPUT_NBBUFFERS){
       //leds_error_toggle();
       ENABLE_INTERRUPTS();
       return(E_FAIL);
    }

    //write the buffer
    openserial_vars.outputBufFilled[buf_id] = TRUE;
    outputHdlcOpen(buf_id);
    for (buffer_out_index=0; buffer_out_index<buffer_out_length; buffer_out_index++){
       outputHdlcWrite(buf_id, buffer_out[buffer_out_index]);
    }
    outputHdlcClose(buf_id);

    ENABLE_INTERRUPTS();

    return E_SUCCESS;
}

int8_t openserial_get_output_buffer(char *buffer, uint8_t length){
   uint8_t  bufindex;
   uint8_t  i;
   uint8_t  length_escaped = 0;
   uint16_t _size_remain = 0;

   //compute the size of the buffer (with escape characters)
   for(i=0; i<length; i++)
      if (buffer[i]==HDLC_FLAG || buffer[i]==HDLC_ESCAPE)
         length_escaped += 2;
      else
         length_escaped += 1;
   length_escaped += 2;      // start frame delimiters (HDLC)
   length_escaped += 2;      // CRC (2B)

   //consider the current bufferindex
   bufindex = openserial_vars.outputCurrentW;

   //number of remainig cells from W to R
   //W=currently written by our module, R=Currently pushed (read by the serial line)
   if (openserial_vars.outputBufIdxW[bufindex] < openserial_vars.outputBufIdxR[bufindex])
      _size_remain = openserial_vars.outputBufIdxR[bufindex] - openserial_vars.outputBufIdxW[bufindex];
   else
      _size_remain = 255 - openserial_vars.outputBufIdxW[bufindex] + openserial_vars.outputBufIdxR[bufindex];

   //do we have enough space?
   if (_size_remain < length_escaped){

      //the next buffer is also filled -> not anymore space
      if ((openserial_vars.outputCurrentR == ((openserial_vars.outputCurrentW + 1) % SERIAL_OUTPUT_BUFFER_SIZE)) || (length_escaped > SERIAL_OUTPUT_BUFFER_SIZE))
         return(-1);

      //else, get the next buffer in the cycle
      if (openserial_vars.outputCurrentW == OPENSERIAL_OUTPUT_NBBUFFERS - 1)
         openserial_vars.outputCurrentW = 0;
      else
         (openserial_vars.outputCurrentW)++;
   }
   return(openserial_vars.outputCurrentW);
}
