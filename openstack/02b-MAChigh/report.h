#include "opendefs.h"

//=========================== definition =====================================
#define CHANNELS 16

//=========================== typedef =========================================
typedef struct {
	open_addr_t neighbor;
	uint8_t eb_receptions[CHANNELS];
	uint8_t ack[CHANNELS];
	uint8_t tx[CHANNELS];
	void* next;
	bool used;
}reportEntry_t;

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

