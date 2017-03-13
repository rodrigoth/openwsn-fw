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
//#include "neighbors.h"

//=========================== variables =======================================

uinject_vars_t uinject_vars;

static const uint8_t node_60[]         =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xdb, 0xa6, 0x86};
static const uint8_t ipAddr_node_61[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xda, 0xa9, 0x88};
static const uint8_t ipAddr_node_64[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd2, 0x96, 0x87};

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
   
   if (packetfunctions_sameAddress(my_address,&node_62)) {	
       uinject_vars.period = UINJECT_PERIOD_MS;   

       uinject_vars.ebTimer = opentimers_start(360000,TIMER_PERIODIC,TIME_MS,uinject_timer_eb);
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

        open_addr_t node_64,node_61,node_6,node_8,node_32,node_47,node_22;

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

	node_6.type = ADDR_64B;
   node_6.addr_64b[0]=0x05;
   node_6.addr_64b[1]=0x43;
   node_6.addr_64b[2]=0x32;
   node_6.addr_64b[3]=0xff;
   node_6.addr_64b[4]=0x03;
   node_6.addr_64b[5]=0xd9;
   node_6.addr_64b[6]=0x92;
   node_6.addr_64b[7]=0x87;//6


   node_8.type = ADDR_64B;
   node_8.addr_64b[0]=0x05;
   node_8.addr_64b[1]=0x43;
   node_8.addr_64b[2]=0x32;
   node_8.addr_64b[3]=0xff;
   node_8.addr_64b[4]=0x03;
   node_8.addr_64b[5]=0xd8;
   node_8.addr_64b[6]=0xa8;
   node_8.addr_64b[7]=0x87;//8

   node_47.type = ADDR_64B;
   node_47.addr_64b[0]=0x05;
   node_47.addr_64b[1]=0x43;
   node_47.addr_64b[2]=0x32;
   node_47.addr_64b[3]=0xff;
   node_47.addr_64b[4]=0x03;
   node_47.addr_64b[5]=0xd6;
   node_47.addr_64b[6]=0xa4;
   node_47.addr_64b[7]=0x87;//47


   node_32.type = ADDR_64B;
   node_32.addr_64b[0]=0x05;
   node_32.addr_64b[1]=0x43;
   node_32.addr_64b[2]=0x32;
   node_32.addr_64b[3]=0xff;
   node_32.addr_64b[4]=0x03;
   node_32.addr_64b[5]=0xd7;
   node_32.addr_64b[6]=0xb2;
   node_32.addr_64b[7]=0x87;//32

   	node_22.type = ADDR_64B;
   	node_22.addr_64b[0]=0x05;
   	node_22.addr_64b[1]=0x43;
   	node_22.addr_64b[2]=0x32;
   	node_22.addr_64b[3]=0xff;
   	node_22.addr_64b[4]=0x03;
   	node_22.addr_64b[5]=0xdd;
   	node_22.addr_64b[6]=0xa4;
   	node_22.addr_64b[7]=0x88;

	/*neighbors_pushEbSerial(&node_60);
	neighbors_pushEbSerial(&node_64);
	neighbors_pushEbSerial(&node_61);
	neighbors_pushEbSerial(&node_6);
	neighbors_pushEbSerial(&node_8);
	neighbors_pushEbSerial(&node_47);
	neighbors_pushEbSerial(&node_32);
	neighbors_pushEbSerial(&node_22);*/

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
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],node_60,16);
   }

   if (order == 2) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_61,16);
   }

   if (order == 3) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_64,16);
       order = 0;
   }
    
   order++;
   
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    *((uint16_t*)&pkt->payload[0]) = uinject_vars.counter++;
   
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
}
