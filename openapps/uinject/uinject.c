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
#include "neighbors.h"
#include "report.h"

#define PAYLOADLEN 10

//=========================== variables =======================================

uinject_vars_t uinject_vars;
open_addr_t node,sink;

//=========================== prototypes ======================================

void uinject_timer_cb(opentimer_id_t id);
void uinject_task_cb(void);
uint8_t order = 0;

uint32_t seqnuns[27] = {0};

//=========================== public ==========================================

void uinject_init() {
   
   // clear local variables
   memset(&uinject_vars,0,sizeof(uinject_vars_t));
   memset(&node,0,sizeof(open_addr_t));
   memset(&sink,0,sizeof(open_addr_t));

   uinject_vars.seqnum = ((uint32_t)openrandom_get16b() <<16) | ((uint32_t)openrandom_get16b());

   
   sink.type = ADDR_64B;
   node.type = ADDR_64B;
   memcpy(&(sink.addr_64b),&addr_64b_sink,8);
   memcpy(&(node.addr_64b),&addr_64b_node,8);
     
   open_addr_t* my_address = idmanager_getMyID(ADDR_64B);

   if (packetfunctions_sameAddress(my_address,&node)) {
       uinject_vars.period = UINJECT_PERIOD_MS;   
       uinject_vars.timerId = opentimers_start(uinject_vars.period,TIMER_PERIODIC,TIME_MS,uinject_timer_cb);
   }
}

void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

void uinject_receive(OpenQueueEntry_t* pkt) {
  /*OpenQueueEntry_t* new_pkt;

  new_pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);

  if (new_pkt==NULL) {
      openqueue_removeAllCreatedBy(COMPONENT_UINJECT);
      openserial_printError(COMPONENT_UINJECT,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
      return;
  }

  uint8_t ipAddr_node[16] =   {0x00};
  uint8_t prefix[8] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

   
  memset(&(ipAddr_node[0]), 0x00, sizeof(ipAddr_node));
  memcpy(&(ipAddr_node[0]),&(prefix[0]),8);
  memcpy(&(ipAddr_node[8]),&sink.addr_64b[0],8);
  

  new_pkt->owner                         = COMPONENT_UINJECT;
  new_pkt->creator                       = COMPONENT_UINJECT;
  new_pkt->l4_protocol                   = IANA_UDP;
  new_pkt->l4_destination_port           = WKP_UDP_INJECT;
  new_pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_INJECT;
  new_pkt->l3_destinationAdd.type        = ADDR_128B;
  memcpy(&new_pkt->l3_destinationAdd.addr_128b[0],&ipAddr_node[0],16);  

  packetfunctions_reserveHeaderSize(new_pkt,PAYLOADLEN);
  memcpy(&(new_pkt->payload[0]),&(pkt->payload[2]),PAYLOADLEN);

  if ((openudp_send(new_pkt))==E_FAIL) {
      openqueue_freePacketBuffer(new_pkt);
  }*/
  openqueue_freePacketBuffer(pkt); 
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
   
   uint8_t* ip_128;
   ip_128 = get_neighbor_128b_address(order);
   if(ip_128 != NULL) {
        pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);

        if (pkt==NULL) {
            openqueue_removeAllCreatedBy(COMPONENT_UINJECT);
            openserial_printError(COMPONENT_UINJECT,ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t)0,(errorparameter_t)0);
            return;
         }

         pkt->owner                         = COMPONENT_UINJECT;
         pkt->creator                       = COMPONENT_UINJECT;
         pkt->l4_protocol                   = IANA_UDP;
         pkt->l4_destination_port           = WKP_UDP_INJECT;
         pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_INJECT;
         pkt->l3_destinationAdd.type        = ADDR_128B;

         memcpy(&pkt->l3_destinationAdd.addr_128b[0],ip_128,16);
         //packetfunctions_reserveHeaderSize(pkt,sizeof(uint8_t));
         //*((uint8_t*)&pkt->payload[0]) = 5;


         (seqnuns[order])++;

          packetfunctions_reserveHeaderSize(pkt,PAYLOADLEN);
          ieee154e_getAsn(asnArray);
          pkt->payload[0] = asnArray[0];
          pkt->payload[1] = asnArray[1];
          pkt->payload[2] = asnArray[2];
          pkt->payload[3] = asnArray[3];
          pkt->payload[4] = asnArray[4];
          pkt->payload[5] = order;
          pkt->payload[6] = (seqnuns[order] & 0xff000000) >> 24;
          pkt->payload[7] = (seqnuns[order] & 0x00ff0000) >> 16;
          pkt->payload[8] = (seqnuns[order] & 0x0000ff00) >> 8;
          pkt->payload[9] = (seqnuns[order] & 0x000000ff);



          /*open_addr_t* my_address = idmanager_getMyID(ADDR_64B);
          memcpy(&(debug_reportEntry.node.addr_64b[0]),&(my_address->addr_64b[0]),8);
          
          open_addr_t dest_64b, prefix, src_64b;
          packetfunctions_ip128bToMac64b(&(pkt->l3_destinationAdd), &prefix, &dest_64b);
          memcpy(&(debug_reportEntry.destination.addr_64b[0]),&(dest_64b.addr_64b[0]),8);

          debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
          debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
          debug_reportEntry.asn.byte4 = asnArray[4];

          debug_reportEntry.asn_in.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
          debug_reportEntry.asn_in.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
          debug_reportEntry.asn_in.byte4 = asnArray[4];

          debug_reportEntry.track = order;
          debug_reportEntry.is_sent = 1;

          //openserial_printStatus(STATUS_UINJECT,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntryUinject_t));
          //openserial_printStatus(STATUS_UINJECT,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntryUinject_t));
          //openserial_printStatus(STATUS_UINJECT,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntryUinject_t));
          //openserial_printData((uint8_t*)&debug_reportEntry,sizeof(debug_reportEntryUinject_t));*/


         if ((openudp_send(pkt))==E_FAIL) {
            openqueue_freePacketBuffer(pkt);
         }
   }
   order++;

   if (order == MAXNUMNEIGHBORS) {
      order = 0;
   }
}
