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

static const uint8_t ipAddr_node_87[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd7, 0x92, 0x77};
static const uint8_t ipAddr_node_78[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd8, 0xa3, 0x82};
static const uint8_t ipAddr_node_76[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd8, 0x91, 0x77};
static const uint8_t ipAddr_node_74[]  =    {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd7, 0xa1, 0x71};
static const uint8_t ipAddr_node_71[]  =    {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xda, 0xa7, 0x79};
static const uint8_t ipAddr_node_70[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xd7, 0x90, 0x67};
static const uint8_t ipAddr_node_92[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x03, 0xdb, 0xb0, 0x78};
static const uint8_t ipAddr_node_94[]  =   {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x43, 0x32, 0xff, 0x02, 0xda, 0x29, 0x60};


uint8_t order = 1;
uint8_t order_eb = 1;

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
   open_addr_t node_86;
   node_86.type = ADDR_64B;
   node_86.addr_64b[0]=0x05;
   node_86.addr_64b[1]=0x43;
   node_86.addr_64b[2]=0x32;
   node_86.addr_64b[3]=0xff;
   node_86.addr_64b[4]=0x03;
   node_86.addr_64b[5]=0xd9;
   node_86.addr_64b[6]=0xa4;
   node_86.addr_64b[7]=0x71;
   
   if (packetfunctions_sameAddress(my_address,&node_86)) {	
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
	
	open_addr_t node_87,node_78,node_76,node_74,node_71,node_70,node_92,node_94;
   node_87.type = ADDR_64B;
   node_87.addr_64b[0]=0x05;
   node_87.addr_64b[1]=0x43;
   node_87.addr_64b[2]=0x32;
   node_87.addr_64b[3]=0xff;
   node_87.addr_64b[4]=0x03;
   node_87.addr_64b[5]=0xd7;
   node_87.addr_64b[6]=0x92;
   node_87.addr_64b[7]=0x77; //64

   node_78.type = ADDR_64B;
   node_78.addr_64b[0]=0x05;
   node_78.addr_64b[1]=0x43;
   node_78.addr_64b[2]=0x32;
   node_78.addr_64b[3]=0xff;
   node_78.addr_64b[4]=0x03;
   node_78.addr_64b[5]=0xd8;
   node_78.addr_64b[6]=0xa3;
   node_78.addr_64b[7]=0x82; //62

   node_76.type = ADDR_64B;
   node_76.addr_64b[0]=0x05;
   node_76.addr_64b[1]=0x43;
   node_76.addr_64b[2]=0x32;
   node_76.addr_64b[3]=0xff;
   node_76.addr_64b[4]=0x03;
   node_76.addr_64b[5]=0xd8;
   node_76.addr_64b[6]=0x91;
   node_76.addr_64b[7]=0x77;//61

   node_74.type = ADDR_64B;
   node_74.addr_64b[0]=0x05;
   node_74.addr_64b[1]=0x43;
   node_74.addr_64b[2]=0x32;
   node_74.addr_64b[3]=0xff;
   node_74.addr_64b[4]=0x03;
   node_74.addr_64b[5]=0xd7;
   node_74.addr_64b[6]=0xa1;
   node_74.addr_64b[7]=0x71;//6


   node_71.type = ADDR_64B;
   node_71.addr_64b[0]=0x05;
   node_71.addr_64b[1]=0x43;
   node_71.addr_64b[2]=0x32;
   node_71.addr_64b[3]=0xff;
   node_71.addr_64b[4]=0x03;
   node_71.addr_64b[5]=0xda;
   node_71.addr_64b[6]=0xa7;
   node_71.addr_64b[7]=0x79;

   node_70.type = ADDR_64B;
   node_70.addr_64b[0]=0x05;
   node_70.addr_64b[1]=0x43;
   node_70.addr_64b[2]=0x32;
   node_70.addr_64b[3]=0xff;
   node_70.addr_64b[4]=0x03;
   node_70.addr_64b[5]=0xd7;
   node_70.addr_64b[6]=0x90;
   node_70.addr_64b[7]=0x67;//47


   node_92.type = ADDR_64B;
   node_92.addr_64b[0]=0x05;
   node_92.addr_64b[1]=0x43;
   node_92.addr_64b[2]=0x32;
   node_92.addr_64b[3]=0xff;
   node_92.addr_64b[4]=0x03;
   node_92.addr_64b[5]=0xdb;
   node_92.addr_64b[6]=0xb0;
   node_92.addr_64b[7]=0x78;//32

   node_94.type = ADDR_64B;
   node_94.addr_64b[0]=0x05;
   node_94.addr_64b[1]=0x43;
   node_94.addr_64b[2]=0x32;
   node_94.addr_64b[3]=0xff;
   node_94.addr_64b[4]=0x02;
   node_94.addr_64b[5]=0xda;
   node_94.addr_64b[6]=0x29;
   node_94.addr_64b[7]=0x60;

	if(order_eb == 1) {
        neighbors_pushEbSerial(&node_87);
    }

    if(order_eb == 2) {
        neighbors_pushEbSerial(&node_78);
    }
    
    if(order_eb == 3) {
        neighbors_pushEbSerial(&node_74);
    }

    if (order_eb == 4) {
        neighbors_pushEbSerial(&node_71);
    }

    if (order_eb == 5) {
	    neighbors_pushEbSerial(&node_70);
    }

    if(order_eb == 6) {
        neighbors_pushEbSerial(&node_92);
    }

    if(order_eb == 7) {
	    neighbors_pushEbSerial(&node_94);
    }

    if(order_eb == 8) {
	    neighbors_pushEbSerial(&node_76);
        order_eb = 0;
    }
    order_eb++;
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
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_87,16);
   }

   if (order == 2) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_78,16);
   }

   if (order == 3) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_76,16);
   }

   if (order == 4) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_74,16);
   }

   if (order == 5) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_71,16);
   }

   if (order == 6) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_70,16);
   }

   if (order == 7) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_92,16);
   }

   if (order == 8) {
       memcpy(&pkt->l3_destinationAdd.addr_128b[0],ipAddr_node_94,16);
       order = 0;
   }
    
   order++;
   
    packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
    *((uint16_t*)&pkt->payload[0]) = uinject_vars.counter++;
   
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
}
