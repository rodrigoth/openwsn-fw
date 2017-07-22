#include "report.h"
#include "opendefs.h"
#include "openserial.h"
#include "IEEE802154E.h"

//=========================== variables =======================================

//=========================== prototypes =======================================

//=========================== public ==========================================
//=== admin
void report_init(void) {}

void report_indicateTxAck(open_addr_t *neighbor, uint8_t tx, uint8_t ack, uint8_t channel) {
	debug_reportEntryAckTx_t debug_reportEntry;

	debug_reportEntry.ack = ack;
	debug_reportEntry.tx = tx;
	debug_reportEntry.channel = channel;
	memcpy(&(debug_reportEntry.neighbor.addr_64b[0]),&(neighbor->addr_64b[0]),8);

	uint8_t asnArray[5];
    ieee154e_getAsn(asnArray);
    debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
    debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
    debug_reportEntry.asn.byte4 = asnArray[4];

    openserial_printStatus(STATUS_ACK_TX,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntryAckTx_t));
}

void report_indicateEB(open_addr_t *neighbor, uint8_t channel,uint8_t iseb) {
	debug_reportEntryEB_t debug_reportEntry;
	debug_reportEntry.channel = channel;
	debug_reportEntry.iseb = iseb;
	memcpy(&(debug_reportEntry.neighbor.addr_64b[0]),&(neighbor->addr_64b[0]),8);

	uint8_t asnArray[5];
	ieee154e_getAsn(asnArray);
	debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
	debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
	debug_reportEntry.asn.byte4 = asnArray[4];

	openserial_printStatus(STATUS_EB,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntryEB_t));
}

