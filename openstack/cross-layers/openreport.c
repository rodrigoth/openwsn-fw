#include "openreport.h"
#include "openserial.h"
#include "IEEE802154E.h"


//=========================== variables =======================================

//=========================== prototypes =======================================

//=========================== public ==========================================

//=== admin
void openreport_init(void) {}


void openreport_indicateParentSwitch(open_addr_t *newParent) {
	uint8_t asnArray[5];
	debug_reportParentChangeEntry_t debug_reportEntry;
	
    ieee154e_getAsn(asnArray);
    debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
    debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
    debug_reportEntry.asn.byte4 = asnArray[4];

    memcpy(&(debug_reportEntry.newParent.addr_64b[0]),&(newParent->addr_64b[0]),8);

    openserial_printStatus(STATUS_PARENTSWITCH,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}

void openreport_indicate6pRequest(uint8_t code) {
	uint8_t asnArray[5];
	debug_report6pRequestEntry_t debug_reportEntry;




	ieee154e_getAsn(asnArray);
    debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
    debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
    debug_reportEntry.asn.byte4 = asnArray[4];

    debug_reportEntry.code = code;

    openserial_printStatus(STATUS_6PREQUEST,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}
