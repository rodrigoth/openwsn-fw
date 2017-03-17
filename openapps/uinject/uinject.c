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

//=========================== variables =======================================

uinject_vars_t uinject_vars;

static const uint8_t ipAddr_node_231[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd8, 0x87, 0x79};
static const uint8_t ipAddr_node_237[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x02, 0xd6, 0x15, 0x62};
static const uint8_t ipAddr_node_247[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xda, 0xa9, 0x82};

uint8_t order = 1;

//=========================== prototypes ======================================

void uinject_timer_cb(opentimer_id_t id);
void uinject_timer_eb(opentimer_id_t id);
void uinject_task_cb(void);
void uinject_task_eb(void);

//=========================== public ==========================================

void uinject_init() {
   
   // clear local variables
   memset(&uinject_vars,0,sizeof(uinject_vars_t));
     
   open_addr_t* my_address = idmanager_getMyID(ADDR_64B);
   open_addr_t node_229;
   node_229.type = ADDR_64B;
   node_229.addr_64b[0]=0x05;
   node_229.addr_64b[1]=0x43;
   node_229.addr_64b[2]=0x32;
   node_229.addr_64b[3]=0xff;
   node_229.addr_64b[4]=0x03;
   node_229.addr_64b[5]=0xd7;
   node_229.addr_64b[6]=0xb0;
   node_229.addr_64b[7]=0x68;
   
   if (packetfunctions_sameAddress(my_address,&node_229)) {	
       uinject_vars.period = UINJECT_PERIOD_MS;   

       uinject_vars.ebTimer = opentimers_start(UINJECT_EB_PERIOD_MS,TIMER_PERIODIC,TIME_MS,uinject_timer_eb);
       uinject_vars.timerId = opentimers_start(uinject_vars.period,TIMER_PERIODIC,TIME_MS,uinject_timer_cb);
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
	// don't run if not synch
   	if (ieee154e_isSynch() == FALSE) return;	
	
   open_addr_t node_231,node_237,node_247;
   node_231.type = ADDR_64B;
   node_231.addr_64b[0]=0x05;
   node_231.addr_64b[1]=0x43;
   node_231.addr_64b[2]=0x32;
   node_231.addr_64b[3]=0xff;
   node_231.addr_64b[4]=0x03;
   node_231.addr_64b[5]=0xd8;
   node_231.addr_64b[6]=0x87;
   node_231.addr_64b[7]=0x79; 

   node_237.type = ADDR_64B;
   node_237.addr_64b[0]=0x05;
   node_237.addr_64b[1]=0x43;
   node_237.addr_64b[2]=0x32;
   node_237.addr_64b[3]=0xff;
   node_237.addr_64b[4]=0x02;
   node_237.addr_64b[5]=0xd6;
   node_237.addr_64b[6]=0x15;
   node_237.addr_64b[7]=0x62;

   node_247.type = ADDR_64B;
   node_247.addr_64b[0]=0x05;
   node_247.addr_64b[1]=0x43;
   node_247.addr_64b[2]=0x32;
   node_247.addr_64b[3]=0xff;
   node_247.addr_64b[4]=0x03;
   node_247.addr_64b[5]=0xda;
   node_247.addr_64b[6]=0xa9;
   node_247.addr_64b[7]=0x82;


	 neighbors_pushEbSerial(&node_231);
   neighbors_pushEbSerial(&node_237);
   neighbors_pushEbSerial(&node_247);
}


void uinject_task_cb() {
   OpenQueueEntry_t*    pkt;
   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) return;
   
  
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
    
   if (order == 1) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_231,16);
   }

   if (order == 2) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_237,16);
   }

   if (order == 3) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_247,16);
       order = 0;
   }
    
   order++;
   
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    *((uint16_t*)&pkt->payload[0]) = uinject_vars.counter++;
   
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
}
