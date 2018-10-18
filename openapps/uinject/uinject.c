#include "opendefs.h"
#include "uinject.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "icmpv6rpl.h"
#include "openrandom.h"
#include "openreport.h"
#include "schedule.h"

#define PAYLOADLEN 17
#define MAX_DIFF_TX_RX 9
#define MAX_QUEUE_CAPACITY_TO_FORWARD 20

//=========================== variables =======================================

uinject_vars_t uinject_vars;

uint8_t ipAddr_node[16] = { 0x00 };
uint8_t prefix[8] = { 0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

uint32_t seqnum = 0;
uint32_t burst = 0;

uint32_t traffic_rates[6] = {2000, 12000, 15000, 20000, 30000, 60000 };
uint32_t current_traffic_rate;

//=========================== prototypes ======================================

void uinject_timer_cb(opentimers_id_t id);
void uinject_task_cb(void);

//=========================== public ==========================================

void uinject_init() {

	// clear local variables
	memset(&uinject_vars, 0, sizeof(uinject_vars_t));

	// register at UDP stack
	uinject_vars.desc.port = WKP_UDP_INJECT;
	uinject_vars.desc.callbackReceive = &uinject_receive;
	uinject_vars.desc.callbackSendDone = &uinject_sendDone;
	openudp_register(&uinject_vars.desc);

	//current_traffic_rate = traffic_rates[openrandom_get16b()%sizeof(traffic_rates)];
	//current_traffic_rate = 10000;

#ifdef VARIABLE_TRAFFIC_RATE
	current_traffic_rate = traffic_rates[VARIABLE_TRAFFIC_RATE];
#else
	current_traffic_rate = UINJECT_PERIOD_MS;
#endif

	memset(&(ipAddr_node[0]), 0x00, sizeof(ipAddr_node));
	memcpy(&(ipAddr_node[0]), &all_routers_multicast,sizeof(all_routers_multicast));

	//uinject_vars.period = current_traffic_rate;
	// start periodic timer
	//uinject_vars.timerId = opentimers_create();
	//opentimers_scheduleIn(uinject_vars.timerId, current_traffic_rate, TIME_MS,
	//		TIMER_PERIODIC, uinject_timer_cb);
}

void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
	openqueue_freePacketBuffer(msg);
}

//I suspect there is a problem in the forwarding module that makes the most demanding nodes to reboot occasionally
void uinject_receive(OpenQueueEntry_t* pkt) {
	open_addr_t sender;

	memcpy(&(sender.addr_64b[0]), &(pkt->payload[2]), 8);
	uint32_t uinject_seqnum = pkt->payload[18] | (pkt->payload[17] << 8)| (pkt->payload[16] << 16) | (pkt->payload[15] << 24);
	uint32_t uinject_burst = pkt->payload[22] | (pkt->payload[21] << 8)| (pkt->payload[20] << 16) | (pkt->payload[19] << 24);


	openreport_indicateAnycastTx(ieee154e_getLastFreq(), uinject_seqnum, 0,&sender,schedule_getCurrentScheduleEntry()->slotOffset,uinject_burst);
	openqueue_freePacketBuffer(pkt);
}

//=========================== private =========================================

/**
 \note timer fired, but we don't want to execute task in ISR mode instead, push
 task to scheduler with CoAP priority, and let scheduler take care of it.
 */
void uinject_timer_cb(opentimers_id_t id) {
	//scheduler_push_task(uinject_task_cb, TASKPRIO_COAP);
}

void uinject_task_cb() {}

void uinject_enqueuePackets(void) {
	OpenQueueEntry_t* pkt;
	uint8_t asnArray[5];
	uint8_t i;

	burst++;

	if (idmanager_getIsDAGroot()) {
		for (i = 0; i < 30; i++) {
			seqnum++;

			// get a free packet buffer
			pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
			if (pkt == NULL) {
				openserial_printError(COMPONENT_UINJECT,
						ERR_NO_FREE_PACKET_BUFFER, (errorparameter_t) 1,
						(errorparameter_t) 1);
				return;
			}

			pkt->owner = COMPONENT_UINJECT;
			pkt->creator = COMPONENT_UINJECT;
			pkt->l4_protocol = IANA_UDP;
			pkt->l4_destination_port = WKP_UDP_INJECT;
			pkt->l4_sourcePortORicmpv6Type = WKP_UDP_INJECT;
			pkt->l3_destinationAdd.type = ADDR_128B;
			memcpy(&pkt->l3_destinationAdd.addr_128b[0], &ipAddr_node[0], 16);

			packetfunctions_reserveHeaderSize(pkt, sizeof(uint32_t));
			pkt->payload[0] = (burst & 0xff000000) >> 24; //63
			pkt->payload[1] = (burst & 0x00ff0000) >> 16; //62
			pkt->payload[2] = (burst & 0x0000ff00) >> 8; //61
			pkt->payload[3] = (burst & 0x000000ff); //60


			packetfunctions_reserveHeaderSize(pkt, sizeof(uint32_t));
			pkt->payload[0] = (seqnum & 0xff000000) >> 24; //63
			pkt->payload[1] = (seqnum & 0x00ff0000) >> 16; //62
			pkt->payload[2] = (seqnum & 0x0000ff00) >> 8; //61
			pkt->payload[3] = (seqnum & 0x000000ff); //60

			packetfunctions_reserveHeaderSize(pkt, sizeof(asn_t));
			ieee154e_getAsn(asnArray);
			pkt->payload[0] = asnArray[0]; //59
			pkt->payload[1] = asnArray[1]; //58
			pkt->payload[2] = asnArray[2]; //57
			pkt->payload[3] = asnArray[3]; //56
			pkt->payload[4] = asnArray[4]; //55

			open_addr_t* my_address = idmanager_getMyID(ADDR_64B);
			packetfunctions_reserveHeaderSize(pkt, 8);
			pkt->payload[0] = my_address->addr_64b[0]; //54
			pkt->payload[1] = my_address->addr_64b[1]; //53
			pkt->payload[2] = my_address->addr_64b[2]; //52
			pkt->payload[3] = my_address->addr_64b[3]; //51
			pkt->payload[4] = my_address->addr_64b[4]; //50
			pkt->payload[5] = my_address->addr_64b[5]; //49
			pkt->payload[6] = my_address->addr_64b[6]; //48
			pkt->payload[7] = my_address->addr_64b[7]; //47

			if ((openudp_send(pkt)) == E_FAIL) {
				openqueue_freePacketBuffer(pkt);
			}
		}
	}
}

