#include "opendefs.h"

//=========================== definition =====================================
#define CHANNELS 16
#define CHANNEL_OFFSET 11

//=========================== typedef =========================================
typedef struct {
	open_addr_t neighbor;
	uint8_t eb_receptions[CHANNELS];
	uint8_t ack[CHANNELS];
	uint8_t tx[CHANNELS];
	void* next;
	bool used;
}reportEntry_t;


BEGIN_PACK
typedef struct {
	open_addr_t neighbor;
	asn_t asn;
	uint8_t eb_receptions[CHANNELS];
	uint8_t ack[CHANNELS];
	uint8_t tx[CHANNELS];
} debug_reportEntry_t;
END_PACK

//=========================== variables =======================================

typedef struct {
	reportEntry_t  reportBuf[MAXNUMNEIGHBORS];
	reportEntry_t *currentEntry;
} report_vars_t;

//=========================== prototypes ======================================

// admin
void report_init(void);

owerror_t report_addNeighbor(open_addr_t *neighbor);
owerror_t report_indicateTxAck(open_addr_t *neighbor, uint8_t tx, uint8_t ack, uint8_t channel);
owerror_t report_indicateEB(open_addr_t *neighbor, uint8_t channel);

void report_pushReportSerial(uint8_t from, uint8_t to,asn_t asn);
