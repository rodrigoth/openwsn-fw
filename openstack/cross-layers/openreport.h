#include "opendefs.h"

//=========================== definition =====================================

//=========================== typedef =========================================

BEGIN_PACK
typedef struct {
	asn_t asn;
	open_addr_t newParent;
} debug_reportParentChangeEntry_t;
END_PACK

BEGIN_PACK
typedef struct {
	asn_t asn;
	uint8_t code;
} debug_report6pRequestEntry_t;
END_PACK


//=========================== variables =======================================



//=========================== prototypes ======================================

// admin
void openreport_init(void);
void openreport_indicateParentSwitch(open_addr_t *newParent);
void openreport_indicate6pRequest(uint8_t code);





