#include "openreport.h"
#include "openserial.h"
#include "IEEE802154E.h"
#include "icmpv6rpl.h"


//=========================== variables =======================================
uint8_t experiment_id;
//=========================== prototypes =======================================

//=========================== public ==========================================

//=== admin
void openreport_init(void) {
	#ifdef EXPERIMENT_ID_DEFINED
		experiment_id = EXPERIMENT_ID_DEFINED;
	#endif

}


void openreport_indicateParentSwitch(open_addr_t *newParent) {
	uint8_t asnArray[5];
	debug_reportParentChangeEntry_t debug_reportEntry;

    ieee154e_getAsn(asnArray);
    debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
    debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
    debug_reportEntry.asn.byte4 = asnArray[4];
    debug_reportEntry.experiment_id = experiment_id;

    memcpy(&(debug_reportEntry.newParent.addr_64b[0]),&(newParent->addr_64b[0]),8);

    //openserial_printStatus(STATUS_PARENTSWITCH,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicate6pRequest(uint8_t code,uint8_t requestedCells,open_addr_t *destination, uint8_t totalTx, uint8_t totalRx) {
	uint8_t asnArray[5];
	debug_report6pRequestEntry_t debug_reportEntry;

	ieee154e_getAsn(asnArray);
    debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
    debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
    debug_reportEntry.asn.byte4 = asnArray[4];

    debug_reportEntry.code = code;
    debug_reportEntry.requestedCells = requestedCells;

    memcpy(&(debug_reportEntry.destination.addr_64b[0]),&(destination->addr_64b[0]),8);
    debug_reportEntry.totalRx = totalRx;
    debug_reportEntry.totalTx = totalTx;

    debug_reportEntry.experiment_id = experiment_id;

    //openserial_printStatus(STATUS_6PREQUEST,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicate6pReceived(uint8_t code,uint8_t requestedCells,open_addr_t *sender, uint8_t totalTx, uint8_t totalRx, uint8_t state) {
	uint8_t asnArray[5];
	debug_report6pReceivedEntry_t debug_reportEntry;

	ieee154e_getAsn(asnArray);
	debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
	debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
	debug_reportEntry.asn.byte4 = asnArray[4];

	debug_reportEntry.code = code;
	debug_reportEntry.requestedCells = requestedCells;

	memcpy(&(debug_reportEntry.sender.addr_64b[0]),&(sender->addr_64b[0]),8);
	debug_reportEntry.totalRx = totalRx;
	debug_reportEntry.totalTx = totalTx;
	debug_reportEntry.state = state;

	debug_reportEntry.experiment_id = experiment_id;

	//openserial_printStatus(STATUS_6PREQUEST_RECEIVED,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicate6pResponse(uint8_t code,uint8_t requestedCells,open_addr_t *destination, uint8_t totalTx, uint8_t totalRx,uint8_t state){
	uint8_t asnArray[5];
	debug_report6pResponseEntry_t debug_reportEntry;

	ieee154e_getAsn(asnArray);
	debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
	debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
	debug_reportEntry.asn.byte4 = asnArray[4];

	debug_reportEntry.code = code;
	debug_reportEntry.requestedCells = requestedCells;

	memcpy(&(debug_reportEntry.destination.addr_64b[0]),&(destination->addr_64b[0]),8);
	debug_reportEntry.totalRx = totalRx;
	debug_reportEntry.totalTx = totalTx;
	debug_reportEntry.state = state;

	debug_reportEntry.experiment_id = experiment_id;

	//openserial_printStatus(STATUS_6PRESPONSE,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicatePDR(open_addr_t *destination, uint8_t totalTx, uint8_t totalAck) {
	uint8_t asnArray[5];
	debug_reportPDR_t debug_reportEntry;

	ieee154e_getAsn(asnArray);
	debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
	debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
	debug_reportEntry.asn.byte4 = asnArray[4];

	memcpy(&(debug_reportEntry.destination.addr_64b[0]),&(destination->addr_64b[0]),8);
	debug_reportEntry.totalAck = totalAck;
	debug_reportEntry.totalTx = totalTx;

	debug_reportEntry.experiment_id = experiment_id;

	//openserial_printStatus(STATUS_PDR,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicateTx(open_addr_t *sender, open_addr_t *destination, uint8_t ack, uint8_t tx,
	uint8_t channel, uint32_t seqnum,uint8_t component, uint8_t *asn) {
	debug_reportTxEntry_t debug_reportEntry;

	memset(&debug_reportEntry,0,sizeof(debug_reportTxEntry_t));

	debug_reportEntry.asn.bytes0and1 = ((uint16_t)asn[1] << 8) | asn[0];
	debug_reportEntry.asn.bytes2and3 = ((uint16_t)asn[3] << 8) | asn[2];
	debug_reportEntry.asn.byte4 = asn[4];

	memcpy(&(debug_reportEntry.sender.addr_64b[0]),&(sender->addr_64b[0]),8);

	memcpy(&(debug_reportEntry.destination.addr_64b[0]),&(destination->addr_64b[0]),8);



	debug_reportEntry.ack = ack;
	debug_reportEntry.tx = tx;
	debug_reportEntry.channel = channel;

	debug_reportEntry.experiment_id = experiment_id;

	debug_reportEntry.seqnum = seqnum;
	debug_reportEntry.component = component;
	debug_reportEntry.nodeRank = icmpv6rpl_getMyDAGrank();
	debug_reportEntry.destinationRank = icmpv6rpl_getPreferredParentRank();


	//openserial_printStatus(STATUS_TX,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicateTxReceived(open_addr_t *sender, uint32_t seqnum,uint8_t *asn) {
	debug_reportTxReceivedEntry_t debug_reportEntry;

	memset(&debug_reportEntry,0,sizeof(debug_reportTxReceivedEntry_t));

	debug_reportEntry.asn.bytes0and1 = ((uint16_t)asn[1] << 8) | asn[0];
	debug_reportEntry.asn.bytes2and3 = ((uint16_t)asn[3] << 8) | asn[2];
	debug_reportEntry.asn.byte4 = asn[4];

	debug_reportEntry.seqnum = seqnum;

	debug_reportEntry.experiment_id = experiment_id;

	memcpy(&(debug_reportEntry.sender.addr_64b[0]),&(sender->addr_64b[0]),8);

	//openserial_printStatus(STATUS_TX_RECEIVED,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}
