#include "openreport.h"
#include "openserial.h"
#include "IEEE802154E.h"


//=========================== variables =======================================

//=========================== prototypes =======================================

//=========================== public ==========================================

//=== admin
void openreport_init(void) {}


void openreport_indicateParentSwitch() {
	uint8_t asnArray[5];
	debug_reportEntryParentSwitch_t debug_reportEntry;
	
    ieee154e_getAsn(asnArray);
    debug_reportEntry.asn.bytes0and1 = ((uint16_t)asnArray[1] << 8) | asnArray[0];
    debug_reportEntry.asn.bytes2and3 = ((uint16_t)asnArray[3] << 8) | asnArray[2];
    debug_reportEntry.asn.byte4 = asnArray[4];

    openserial_printStatus(STATUS_PARENTSWITCH,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry));
}