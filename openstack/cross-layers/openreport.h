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

BEGIN_PACK
typedef struct {
	open_addr_t previousHop;
	open_addr_t destination;
	asn_t asn;
	uint8_t ack;
	uint8_t tx;
	uint8_t channel;
	uint32_t seqnum;
	uint8_t component;
} debug_reportTxEntry_t;
END_PACK


//=========================== variables =======================================



//=========================== prototypes ======================================

// admin
void openreport_init(void);
void openreport_indicateParentSwitch(open_addr_t *newParent);
void openreport_indicate6pRequest(uint8_t code);
void openreport_indicateTx(open_addr_t *previousHop, open_addr_t *destination, uint8_t ack, uint8_t tx, uint8_t channel, uint32_t seqnum,uint8_t component);





