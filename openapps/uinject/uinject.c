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



//=========================== variables =======================================

uinject_vars_t uinject_vars;
open_addr_t node;
uint8_t order = 0;
uint8_t from = 0;
uint8_t to = EB_PUSH_SERIAL_RANGE;

//=========================== prototypes ======================================

void uinject_timer_cb(opentimer_id_t id);
void uinject_timer_eb(opentimer_id_t id);
void uinject_task_cb(void);
void uinject_task_eb(void);

//=========================== public ==========================================

void uinject_init() {
   
   // clear local variables
   memset(&uinject_vars,0,sizeof(uinject_vars_t));
   memset(&node,0,sizeof(open_addr_t));

   node.type = ADDR_64B;
   memcpy(&(node.addr_64b),&addr_64b_node,8);
     
   open_addr_t* my_address = idmanager_getMyID(ADDR_64B);

   if (packetfunctions_sameAddress(my_address,&node)) {
       uinject_vars.period = UINJECT_PERIOD_MS;   
       uinject_vars.timerId = opentimers_start(uinject_vars.period,TIMER_PERIODIC,TIME_MS,uinject_timer_cb);
       uinject_vars.timer_eb_id = opentimers_start(UINJECT_EB_PERIOD_MS,TIMER_PERIODIC,TIME_MS,uinject_timer_eb);
   }
}

void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

void uinject_receive(OpenQueueEntry_t* pkt) {
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

void uinject_timer_eb(opentimer_id_t id){
   
   scheduler_push_task(uinject_task_eb,TASKPRIO_COAP);
}


void uinject_task_eb() {
   if (ieee154e_isSynch() == FALSE) return;  
   
   report_pushReportSerial(from,to);
   from = from + EB_PUSH_SERIAL_RANGE;
   to = to + EB_PUSH_SERIAL_RANGE;

   if(to >= MAXNUMNEIGHBORS+EB_PUSH_SERIAL_RANGE) {
      from = 0;
      to = EB_PUSH_SERIAL_RANGE;
      opentimers_setPeriod(uinject_vars.timer_eb_id,TIME_MS,UINJECT_EB_PERIOD_MS);
   } else {
	   opentimers_setPeriod(uinject_vars.timer_eb_id,TIME_MS,1000);
   }
}


void uinject_task_cb() {
   OpenQueueEntry_t*    pkt;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
   uint8_t* ip_128;
   ip_128 = get_neighbor_128b_address(order);
   if(ip_128 != NULL) {
         pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);

         if (pkt==NULL) {
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
         packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
         *((uint16_t*)&pkt->payload[0]) = uinject_vars.counter++;

         if ((openudp_send(pkt))==E_FAIL) {
            openqueue_freePacketBuffer(pkt);
         }
   }
   
   order++;

   if (order == MAXNUMNEIGHBORS) {
      order = 0;
   }
}
