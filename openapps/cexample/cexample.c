/**
\brief An example CoAP application.
*/

#include "opendefs.h"
#include "cexample.h"
#include "opencoap.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"
//#include "ADC_Channel.h"
#include "idmanager.h"
#include "IEEE802154E.h"
#include "neighbors.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define CEXAMPLEPERIOD  120000
#define PAYLOADLEN      1

const uint8_t cexample_path0[] = "ex";

//=========================== variables =======================================

cexample_vars_t cexample_vars;

//=========================== prototypes ======================================

owerror_t cexample_receive(OpenQueueEntry_t* msg,
                    coap_header_iht*  coap_header,
                    coap_option_iht*  coap_options);
void    cexample_timer_cb(opentimer_id_t id);
void    cexample_task_cb(void);
void    cexample_sendDone(OpenQueueEntry_t* msg,
                       owerror_t error);

//=========================== public ==========================================

void cexample_init() {
   
   // prepare the resource descriptor for the /ex path
   cexample_vars.desc.path0len             = sizeof(cexample_path0)-1;
   cexample_vars.desc.path0val             = (uint8_t*)(&cexample_path0);
   cexample_vars.desc.path1len             = 0;
   cexample_vars.desc.path1val             = NULL;
   cexample_vars.desc.componentID          = COMPONENT_CEXAMPLE;
   cexample_vars.desc.discoverable         = TRUE;
   cexample_vars.desc.callbackRx           = &cexample_receive;
   cexample_vars.desc.callbackSendDone     = &cexample_sendDone;
   
   
   opencoap_register(&cexample_vars.desc);
   cexample_vars.timerId    = opentimers_start(CEXAMPLEPERIOD,
                                                TIMER_PERIODIC,TIME_MS,
                                                cexample_timer_cb);
}

//=========================== private =========================================

owerror_t cexample_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   return E_FAIL;
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void cexample_timer_cb(opentimer_id_t id){
   scheduler_push_task(cexample_task_cb,TASKPRIO_COAP);
}

void cexample_task_cb() {
	open_addr_t* my_address = idmanager_getMyID(ADDR_64B);
	open_addr_t node_62;

	node_62.type = ADDR_64B;
	node_62.addr_64b[0]=0x05;
	node_62.addr_64b[1]=0x43;
	node_62.addr_64b[2]=0x32;
	node_62.addr_64b[3]=0xff;
	node_62.addr_64b[4]=0x03;
	node_62.addr_64b[5]=0xd9;
	node_62.addr_64b[6]=0xb3;
	node_62.addr_64b[7]=0x86; //62

	if (!packetfunctions_sameAddress(my_address,&node_62)) {
		return;
	}


	open_addr_t node_60;//sink
	node_60.type = ADDR_64B;
	node_60.addr_64b[0]=0x05;
	node_60.addr_64b[1]=0x43;
	node_60.addr_64b[2]=0x32;
	node_60.addr_64b[3]=0xff;
	node_60.addr_64b[4]=0x03;
	node_60.addr_64b[5]=0xdb;
	node_60.addr_64b[6]=0xa6;
	node_60.addr_64b[7]=0x86;


	open_addr_t node_64,node_61;
	node_64.type = ADDR_64B;
	node_64.addr_64b[0]=0x05;
	node_64.addr_64b[1]=0x43;
	node_64.addr_64b[2]=0x32;
	node_64.addr_64b[3]=0xff;
	node_64.addr_64b[4]=0x03;
	node_64.addr_64b[5]=0xd2;
	node_64.addr_64b[6]=0x96;
	node_64.addr_64b[7]=0x87; //64

	node_61.type = ADDR_64B;
	node_61.addr_64b[0]=0x05;
	node_61.addr_64b[1]=0x43;
	node_61.addr_64b[2]=0x32;
	node_61.addr_64b[3]=0xff;
	node_61.addr_64b[4]=0x03;
	node_61.addr_64b[5]=0xda;
	node_61.addr_64b[6]=0xa9;
	node_61.addr_64b[7]=0x88;//61

	neighbors_pushEbSerial(&node_60);
	neighbors_pushEbSerial(&node_64);
	neighbors_pushEbSerial(&node_61);
}

void cexample_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   //openqueue_freePacketBuffer(msg);
   //ieee154e_reset_eb_stats();
}
