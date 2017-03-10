/**
\brief An example CoAP application.
*/

#include "opendefs.h"
#include "cexample4.h"
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
//#include "schedule.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define CEXAMPLEPERIOD  13000
#define PAYLOADLEN      1

const uint8_t cexample4_path0[] = "exx";

//=========================== variables =======================================

cexample4_vars_t cexample4_vars;

//=========================== prototypes ======================================

owerror_t cexample4_receive(OpenQueueEntry_t* msg,
                    coap_header_iht*  coap_header,
                    coap_option_iht*  coap_options);
void    cexample4_timer_cb(opentimer_id_t id);
void    cexample4_task_cb(void);
void    cexample4_sendDone(OpenQueueEntry_t* msg,
                       owerror_t error);

void send_fake_packet4(uint8_t *address);
//=========================== public ==========================================

void cexample4_init() {
   
   // prepare the resource descriptor for the /ex path
   cexample4_vars.desc.path0len             = sizeof(cexample4_path0)-1;
   cexample4_vars.desc.path0val             = (uint8_t*)(&cexample4_path0);
   cexample4_vars.desc.path1len             = 0;
   cexample4_vars.desc.path1val             = NULL;
   cexample4_vars.desc.componentID          = COMPONENT_CEXAMPLE4;
   cexample4_vars.desc.discoverable         = TRUE;
   cexample4_vars.desc.callbackRx           = &cexample4_receive;
   cexample4_vars.desc.callbackSendDone     = &cexample4_sendDone;
   
   
   opencoap_register(&cexample4_vars.desc);
   cexample4_vars.timerId    = opentimers_start(CEXAMPLEPERIOD,
                                                TIMER_PERIODIC,TIME_MS,
                                                cexample4_timer_cb);
}

//=========================== private =========================================

owerror_t cexample4_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   return E_FAIL;
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void cexample4_timer_cb(opentimer_id_t id){
   scheduler_push_task(cexample4_task_cb,TASKPRIO_COAP);
}

void cexample4_task_cb() {
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

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(cexample4_vars.timerId);
      return;
   }
  
   //send_fake_packet(&ipAddr_iotlab);
   //send_fake_packet(&ipAddr_node_61);
   send_fake_packet4(&ipAddr_node_64);
   return;
}

void cexample4_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
   //ieee154e_reset_eb_stats();
}

void send_fake_packet4(uint8_t *address) {
    OpenQueueEntry_t*    pkt;
    owerror_t            outcome;

   pkt = openqueue_getFreePacketBuffer(COMPONENT_CEXAMPLE4);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_CEXAMPLE4,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      openqueue_freePacketBuffer(pkt);
      return;
   }

     // take ownership over that packet
   pkt->creator                   = COMPONENT_CEXAMPLE4;
   pkt->owner                     = COMPONENT_CEXAMPLE4;
   
   
   // CoAP payload
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = (uint8_t)1;
    
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = COAP_PAYLOAD_MARKER;
   
   // content-type option
   packetfunctions_reserveHeaderSize(pkt,2);
   pkt->payload[0]                = (COAP_OPTION_NUM_CONTENTFORMAT - COAP_OPTION_NUM_URIPATH) << 4
                                    | 1;
   pkt->payload[1]                = COAP_MEDTYPE_TEXTPLAIN;
   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(cexample4_path0)-1);
   memcpy(&pkt->payload[0],cexample4_path0,sizeof(cexample4_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0]                = ((COAP_OPTION_NUM_URIPATH) << 4) | (sizeof(cexample4_path0)-1);
   
   // metadata
   pkt->l4_destination_port       = 5683;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],address,16);
   
    // send
   outcome = opencoap_send(pkt,COAP_TYPE_NON,COAP_CODE_REQ_PUT,1,&cexample4_vars.desc);
   
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   
}


