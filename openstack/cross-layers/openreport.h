#include "opendefs.h"

//=========================== definition =====================================

//=========================== typedef =========================================

BEGIN_PACK
typedef struct {
	asn_t asn;
	open_addr_t newParent;
	uint8_t experiment_id;
	uint16_t previousRank;
	uint16_t newRank;
} debug_reportParentChangeEntry_t;
END_PACK

BEGIN_PACK
typedef struct {
	asn_t asn;
	uint8_t code;
	uint8_t requestedCells;
	open_addr_t destination;
	uint8_t totalTx;
	uint8_t totalRx;
	uint8_t experiment_id;
} debug_report6pRequestEntry_t;
END_PACK

BEGIN_PACK
typedef struct {
	asn_t asn;
	uint8_t code;
	uint8_t requestedCells;
	open_addr_t sender;
	uint8_t totalTx;
	uint8_t totalRx;
	uint8_t state;
	uint8_t experiment_id;
} debug_report6pReceivedEntry_t;
END_PACK


BEGIN_PACK
typedef struct {
	asn_t asn;
	uint8_t code;
	uint8_t requestedCells;
	open_addr_t destination;
	uint8_t totalTx;
	uint8_t totalRx;
	uint8_t state;
	uint8_t experiment_id;
} debug_report6pResponseEntry_t;
END_PACK


BEGIN_PACK
typedef struct {
	open_addr_t sender;
	open_addr_t destination;
	asn_t asn;
	uint8_t ack;
	uint8_t tx;
	uint8_t channel;
	uint32_t seqnum;
	uint8_t component;
	uint16_t nodeRank;
	uint16_t destinationRank;
	uint8_t experiment_id;
} debug_reportTxEntry_t;
END_PACK


BEGIN_PACK
typedef struct {
	asn_t asn;
	open_addr_t destination;
	uint8_t totalTx;
	uint8_t totalAck;
	uint8_t experiment_id;
	uint8_t pdrWMEMA;
} debug_reportPDR_t;
END_PACK


BEGIN_PACK
typedef struct {
	open_addr_t sender;
	asn_t asn;
	uint32_t seqnum;
	uint8_t experiment_id;
} debug_reportTxReceivedEntry_t;
END_PACK

//=========================== variables =======================================



//=========================== prototypes ======================================

// admin
void openreport_init(void);
void openreport_indicateParentSwitch(open_addr_t *newParent, uint16_t previousRank, uint16_t newRank);
void openreport_indicate6pRequest(uint8_t code,uint8_t requestedCells,open_addr_t *destination, uint8_t totalTx, uint8_t totalRx);
void openreport_indicate6pReceived(uint8_t code,uint8_t requestedCells,open_addr_t *sender, uint8_t totalTx, uint8_t totalRx,uint8_t state);
void openreport_indicate6pResponse(uint8_t code,uint8_t requestedCells,open_addr_t *destination, uint8_t totalTx, uint8_t totalRx,uint8_t state);
void openreport_indicateTx(open_addr_t *sender, open_addr_t *destination, uint8_t ack, uint8_t tx, uint8_t channel, uint32_t seqnum,uint8_t component,uint8_t *asn);
void openreport_indicatePDR(open_addr_t *destination, uint8_t totalTx, uint8_t totalAck, uint8_t pdrWMEMA);
void openreport_indicateTxReceived(open_addr_t *sender, uint32_t seqnum,uint8_t *asn);





