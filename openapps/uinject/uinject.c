#include "opendefs.h"
#include "uinject.h"
#include "openudp.h"
#include "openqueue.h"
#include "opentimers.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "icmpv6rpl.h"

//=========================== variables =======================================

uinject_vars_t uinject_vars;

static const uint8_t uinject_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
}; 

uint32_t seqnum = 0;
//=========================== prototypes ======================================

void uinject_timer_cb(opentimer_id_t id);
void uinject_task_cb(void);

//=========================== public ==========================================

void uinject_init() {
   
   // clear local variables
   memset(&uinject_vars,0,sizeof(uinject_vars_t));
   
   uinject_vars.period = UINJECT_PERIOD_MS;
   
   // start periodic timer
   uinject_vars.timerId                    = opentimers_start(
      uinject_vars.period,
      TIMER_PERIODIC,TIME_MS,
      uinject_timer_cb
   );
}

void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

void uinject_receive(OpenQueueEntry_t* pkt) {
   
   openqueue_freePacketBuffer(pkt);
   
   openserial_printError(
      COMPONENT_UINJECT,
      ERR_RCVD_ECHO_REPLY,
      (errorparameter_t)0,
      (errorparameter_t)0
   );
}

//=========================== private =========================================

/**
\note timer fired, but we don't want to execute task in ISR mode instead, push
   task to scheduler with CoAP priority, and let scheduler take care of it.
*/
void uinject_timer_cb(opentimer_id_t id){
   
   scheduler_push_task(uinject_task_cb,TASKPRIO_COAP);
}

void uinject_task_cb() {
    OpenQueueEntry_t*    pkt;
    uint8_t              asnArray[5];
   
   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(uinject_vars.timerId);
      return;
   }

   seqnum++;

   //uint8_t slots = schedule_getNumOfSlotsByType(CELLTYPE_TX);
   //if(slots == 0) return;

   //don't run if I dont have slots to my preferred parent
   open_addr_t neighbor;
   icmpv6rpl_getPreferredParentEui64(&neighbor);
   uint8_t slots = schedule_getNumberSlotToPreferredParent(&neighbor);
   if(slots == 0) return;
   
   // if you get here, send a packet
   
   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
   if (pkt==NULL) {
      openserial_printError(
         COMPONENT_UINJECT,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   pkt->owner                         = COMPONENT_UINJECT;
   pkt->creator                       = COMPONENT_UINJECT;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_INJECT;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_INJECT;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],uinject_dst_addr,16);
   
   

   packetfunctions_reserveHeaderSize(pkt,sizeof(uint32_t));
   pkt->payload[0] = (seqnum & 0xff000000) >> 24;
   pkt->payload[1] = (seqnum & 0x00ff0000) >> 16;
   pkt->payload[2] = (seqnum & 0x0000ff00) >> 8;
   pkt->payload[3] = (seqnum & 0x000000ff);

   
   packetfunctions_reserveHeaderSize(pkt,sizeof(asn_t));
   ieee154e_getAsn(asnArray);
   pkt->payload[0] = asnArray[0];
   pkt->payload[1] = asnArray[1];
   pkt->payload[2] = asnArray[2];
   pkt->payload[3] = asnArray[3];
   pkt->payload[4] = asnArray[4];


   open_addr_t* my_address = idmanager_getMyID(ADDR_64B);
   packetfunctions_reserveHeaderSize(pkt,8);
   pkt->payload[0] = my_address->addr_64b[0];
   pkt->payload[1] = my_address->addr_64b[1];
   pkt->payload[2] = my_address->addr_64b[2];
   pkt->payload[3] = my_address->addr_64b[3];
   pkt->payload[4] = my_address->addr_64b[4];
   pkt->payload[5] = my_address->addr_64b[5];
   pkt->payload[6] = my_address->addr_64b[6];
   pkt->payload[7] = my_address->addr_64b[7];
   
   
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }

   
   
}
