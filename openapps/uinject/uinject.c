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

#define PAYLOADLEN 17

//=========================== variables =======================================

uinject_vars_t uinject_vars;

uint8_t ipAddr_node[16] = { 0x00 };
uint8_t prefix[8] = { 0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

const uint16_t uinject_on_times[] =
		{ 12144, 5365, 2738, 2387, 1666, 10926, 4195, 2503, 1806, 1228, 205,
				302, 4722, 16822, 4525, 2750, 25769, 4362, 7236, 1309, 5084,
				505, 7912, 14702, 8586, 14631, 3596, 5878, 8067, 4059 };

const uint16_t uinject_off_times[] = { 18340, 3839, 23052, 24416, 147, 35511,
		18006, 9194, 32545, 30869, 5887, 9941, 17271, 8141, 64533, 1922, 553,
		28516, 2466, 1395, 52762, 5409, 12316, 6398, 20374, 7738, 24773, 15043,
		2324, 8737 };

//=========================== prototypes ======================================

void uinject_timer_cb(opentimers_id_t id);
void uinject_off_timer_cb(opentimers_id_t id);
void uinject_on_timer_cb(opentimers_id_t id);

void uinject_task_off_expired(void);
void uinject_task_on_expired(void);
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

	uinject_vars.timerIdOff = opentimers_create();
	uinject_vars.timerIdOn = opentimers_create();
	uinject_vars.timerBurst = opentimers_create();

	if (!idmanager_getIsDAGroot()) {
		uint16_t off_period = uinject_off_times[openrandom_get16b() % 30];
		opentimers_scheduleIn(uinject_vars.timerIdOff, off_period, TIME_MS,
				TIMER_ONESHOT, uinject_off_timer_cb);
	}
}

void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
	openqueue_freePacketBuffer(msg);
}

//I suspect there is a problem in the forwarding module that makes the most demanding nodes to reboot occasionally
void uinject_receive(OpenQueueEntry_t* pkt) {
	openqueue_freePacketBuffer(pkt);
}

//=========================== private =========================================

/**
 \note timer fired, but we don't want to execute task in ISR mode instead, push
 task to scheduler with CoAP priority, and let scheduler take care of it.
 */
void uinject_timer_cb(opentimers_id_t id) {

	scheduler_push_task(uinject_task_cb, TASKPRIO_COAP);
}

void uinject_off_timer_cb(opentimers_id_t id) {
	scheduler_push_task(uinject_task_off_expired, TASKPRIO_COAP);
}

void uinject_on_timer_cb(opentimers_id_t id) {
	scheduler_push_task(uinject_task_on_expired, TASKPRIO_COAP);
}

void uinject_task_on_expired() {
	opentimers_cancel(uinject_vars.timerBurst);

	uint16_t off_period = uinject_off_times[openrandom_get16b() % 30];
	opentimers_scheduleIn(uinject_vars.timerIdOff, off_period, TIME_MS,TIMER_ONESHOT, uinject_off_timer_cb);

}

void uinject_task_off_expired() {

	//how long ON will be active
	uint16_t on_period = uinject_on_times[openrandom_get16b() % 30];
	opentimers_scheduleIn(uinject_vars.timerIdOn, on_period, TIME_MS,TIMER_ONESHOT, uinject_on_timer_cb);

	//start burst traffic
	opentimers_scheduleIn(uinject_vars.timerBurst, 500, TIME_MS, TIMER_PERIODIC,uinject_timer_cb);
}

void uinject_task_cb() {
	OpenQueueEntry_t* pkt;
	bool foundNeighbor;

	// don't run if not synch
	if (ieee154e_isSynch() == FALSE)
		return;

	//don't run if I dont have slots to my preferred parent
	open_addr_t neighbor;
	foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
	if (!foundNeighbor
			|| schedule_getNumberSlotToPreferredParent(&neighbor) == 0) {
		return;
	}

	// if you get here, send a packet

	memset(&(ipAddr_node[0]), 0x00, sizeof(ipAddr_node));
	memcpy(&(ipAddr_node[0]), &(prefix[0]), 8);
	memcpy(&(ipAddr_node[8]), &neighbor.addr_64b[0], 8);

	// get a free packet buffer
	pkt = openqueue_getFreePacketBuffer(COMPONENT_UINJECT);
	if (pkt == NULL) {
		openserial_printError(COMPONENT_UINJECT, ERR_NO_FREE_PACKET_BUFFER,(errorparameter_t) 1, (errorparameter_t) 1);
		return;
	}

	pkt->owner = COMPONENT_UINJECT;
	pkt->creator = COMPONENT_UINJECT;
	pkt->l4_protocol = IANA_UDP;
	pkt->l4_destination_port = WKP_UDP_INJECT;
	pkt->l4_sourcePortORicmpv6Type = WKP_UDP_INJECT;
	pkt->l3_destinationAdd.type = ADDR_128B;
	memcpy(&pkt->l3_destinationAdd.addr_128b[0], &ipAddr_node[0], 16);

	packetfunctions_reserveHeaderSize(pkt, sizeof(uint8_t));
	pkt->payload[0] = 10;


	if ((openudp_send(pkt)) == E_FAIL) {
		openqueue_freePacketBuffer(pkt);
	}
}

