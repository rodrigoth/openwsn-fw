#include "opendefs.h"
#include "linkestimator.h"
#include "opencoap.h"
#include "opentimers.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "scheduler.h"
#include "idmanager.h"
#include "IEEE802154E.h"
#include "neighbors.h"
#include "schedule.h"

//=========================== defines =========================================

/// inter-packet period (in ms)
#define PERIOD  60000

const uint8_t linkestimator_path0[] = "li";

//=========================== variables =======================================

linkestimator_vars_t linkestimator_vars;

//=========================== prototypes ======================================

owerror_t linkestimator_receive(OpenQueueEntry_t* msg,coap_header_iht*  coap_header, coap_option_iht*  coap_options);
void    linkestimator_timer_cb(opentimer_id_t id);
void    linkestimator_task_cb(void);
void    linkestimator_sendDone(OpenQueueEntry_t* msg, owerror_t error);

//=========================== public ==========================================

void linkestimator_init() {
   // prepare the resource descriptor for the /ex path
   linkestimator_vars.desc.path0len             = sizeof(linkestimator_path0)-1;
   linkestimator_vars.desc.path0val             = (uint8_t*)(&linkestimator_path0);
   linkestimator_vars.desc.path1len             = 0;
   linkestimator_vars.desc.path1val             = NULL;
   linkestimator_vars.desc.componentID          = COMPONENT_LINKESTIMATOR;
   linkestimator_vars.desc.discoverable         = TRUE;
   linkestimator_vars.desc.callbackRx           = &linkestimator_receive;
   linkestimator_vars.desc.callbackSendDone     = &linkestimator_sendDone;
   
   opencoap_register(&linkestimator_vars.desc);
   linkestimator_vars.timerId    = opentimers_start(PERIOD, TIMER_PERIODIC, TIME_MS, linkestimator_timer_cb);
}

//=========================== private =========================================

owerror_t linkestimator_receive(OpenQueueEntry_t* msg,coap_header_iht* coap_header,coap_option_iht* coap_options) {
   return E_FAIL;
}

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void linkestimator_timer_cb(opentimer_id_t id){
   scheduler_push_task(linkestimator_task_cb,TASKPRIO_COAP);
}

void linkestimator_task_cb() {
   OpenQueueEntry_t*    pkt;
   owerror_t            outcome;
   uint8_t              asnArray[5];
   uint8_t              top,i,reserved_payload_size,offset;
   ieee154e_top_nb_eb_count_t  ieee154e_top_nb_eb_count;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(linkestimator_vars.timerId);
      return;
   }
  
   // create a CoAP RD packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_LINKESTIMATOR);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_LINKESTIMATOR,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      openqueue_freePacketBuffer(pkt);
      return;
   }
   // take ownership over that packet
   pkt->creator                   = COMPONENT_LINKESTIMATOR;
   pkt->owner                     = COMPONENT_LINKESTIMATOR;
   
   // CoAP payload
    
    ieee154e_getAsn(asnArray);
   
    reserved_payload_size = 9;

    packetfunctions_reserveHeaderSize(pkt,reserved_payload_size);
    pkt->payload[0] = asnArray[0];
    pkt->payload[1] = asnArray[1];
    pkt->payload[2] = asnArray[2];
    pkt->payload[3] = asnArray[3];
    pkt->payload[4] = asnArray[4];

    //open_addr_t neighbor;
    //icmpv6rpl_getPreferredParentEui64(&neighbor);

    //neighbors_tx_ack_t stat = get_pdr_by_neighbor(&neighbor);

    pkt->payload[5] = 0;
    pkt->payload[6] = 0;

    top = 1;
    offset = 7;
    //ieee154e_top_nb_eb_count = ieee154e_get_top_nb_eb_stats(top);
   
    //uint16_t *nb = ieee154e_top_nb_eb_count.top_nb;
    //uint16_t *nb_count = ieee154e_top_nb_eb_count.top_nb_count;

    //for (i = offset; i < reserved_payload_size; i=i+4) {
        //pkt->payload[i]   = (uint8_t)(*nb & 0x00FF);
        //pkt->payload[i+1] = (uint8_t)((*nb & 0xFF00)>>8);
        pkt->payload[7] = 0;//(uint8_t)(*nb_count & 0x00FF);
        pkt->payload[8] = 0;//(uint8_t)((*nb_count & 0xFF00)>>8);

        //nb++;
      //  nb_count++;
    //}

   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = COAP_PAYLOAD_MARKER;
   
   // content-type option
   packetfunctions_reserveHeaderSize(pkt,2);
   pkt->payload[0]                = (COAP_OPTION_NUM_CONTENTFORMAT - COAP_OPTION_NUM_URIPATH) << 4
                                    | 1;
   pkt->payload[1]                = COAP_MEDTYPE_TEXTPLAIN;
   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(linkestimator_path0)-1);
   memcpy(&pkt->payload[0],linkestimator_path0,sizeof(linkestimator_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0]                = ((COAP_OPTION_NUM_URIPATH) << 4) | (sizeof(linkestimator_path0)-1);
   
   // metadata
   pkt->l4_destination_port       = 5683;
   pkt->l3_destinationAdd.type    = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ipAddr_iotlab,16);
   
   // send
   outcome = opencoap_send(
      pkt,
      COAP_TYPE_NON,
      COAP_CODE_REQ_POST,
      1,
      &linkestimator_vars.desc
   );
   
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
   
   
   return;
}

void linkestimator_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
   //ieee154e_reset_eb_stats();
}
