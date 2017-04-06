#include "opendefs.h"

//=========================== definition =====================================
//=========================== typedef =========================================

BEGIN_PACK
typedef struct {
	open_addr_t neighbor;
	asn_t asn;
	uint8_t channel;
} debug_reportEntryEB_t;
END_PACK

BEGIN_PACK
typedef struct {
	open_addr_t neighbor;
	asn_t asn;
	uint8_t ack;
	uint8_t tx;
	uint8_t channel;
} debug_reportEntryAckTx_t;
END_PACK

//=========================== variables =======================================



//=========================== prototypes ======================================

// admin
void report_init(void);
void report_indicateTxAck(open_addr_t *neighbor, uint8_t tx, uint8_t ack, uint8_t channel);
void report_indicateEB(open_addr_t *neighbor, uint8_t channel);


