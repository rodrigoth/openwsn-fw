#include "report.h"
#include "opendefs.h"
#include "openserial.h"

//=========================== variables =======================================
report_vars_t report_vars;

//=========================== prototypes =======================================
void report_resetEntry(reportEntry_t* entry);
reportEntry_t * report_getFirstAvailablePosition();
reportEntry_t * report_getLastUsedPosition();

//=========================== public ==========================================
//=== admin
void report_init(void) {
	uint8_t i;
	memset(&report_vars,0,sizeof(report_vars_t));
	for(i = 0; i< MAXNUMNEIGHBORS; i++) {
		report_resetEntry(&(report_vars.reportBuf[i]));
	}
}

owerror_t report_addNeighbor(open_addr_t *neighbor) {
	reportEntry_t *entry = report_getFirstAvailablePosition();
	reportEntry_t *lastUsedEntry = report_getLastUsedPosition();

	if (entry == NULL) {
		openserial_printError(COMPONENT_REPORT,ERR_REPORT_NO_ENTRY,(errorparameter_t)0,(errorparameter_t)0);
		return E_FAIL;
	}

	entry->used = TRUE;
	memcpy(&(entry->neighbor.addr_64b[0]),&(neighbor->addr_64b[0]),8);

	if(lastUsedEntry != entry) {
		lastUsedEntry->next = entry;
	} else {
		if(entry == &(report_vars.reportBuf[MAXNUMNEIGHBORS - 1])) {
			entry->next = &(report_vars.reportBuf[0]);
		}
	}

	return E_SUCCESS;
}

//TODO
owerror_t report_indicateTxAck(open_addr_t *neighbor, uint8_t tx, uint8_t ack, uint8_t channel) {
	reportEntry_t *entry;
	entry = &(report_vars.reportBuf[0]);

	while(entry != NULL) {
		openserial_printInfo(COMPONENT_REPORT,ERR_REPORT_ENTRY_ADDED,entry->neighbor.addr_64b[6],entry->neighbor.addr_64b[7]);
		entry = entry->next;
	}
	return E_SUCCESS;
}


//=========================== private ==========================================
reportEntry_t * report_getFirstAvailablePosition() {
	reportEntry_t *entry;

	entry = &(report_vars.reportBuf[0]);

	while( entry->used != FALSE &&  entry <= &(report_vars.reportBuf[MAXNUMNEIGHBORS - 1])) {
		entry++;
	}

	//overflow
	if(entry > &(report_vars.reportBuf[MAXNUMNEIGHBORS - 1])) {
		return NULL;
	}

	return entry;
}

reportEntry_t * report_getLastUsedPosition() {
	reportEntry_t *entry;
	reportEntry_t *last;

	entry = &(report_vars.reportBuf[0]);
	last = entry;

	while( entry->used != FALSE &&  entry <= &(report_vars.reportBuf[MAXNUMNEIGHBORS - 1])) {
		last = entry;
		entry++;
	}

	//overflow
	if(entry > &(report_vars.reportBuf[MAXNUMNEIGHBORS - 1])) {
		return NULL;
	}

	return last;
}


void report_resetEntry(reportEntry_t* entry) {
	entry->used = FALSE;
	entry->next = NULL;

	memset(&(entry->ack[0]),0,sizeof(uint8_t)*MAXNUMNEIGHBORS);
	memset(&(entry->eb_receptions[0]),0,sizeof(uint8_t)*MAXNUMNEIGHBORS);
	memset(&(entry->tx[0]),0,sizeof(uint8_t)*MAXNUMNEIGHBORS);

	entry->neighbor.type = ADDR_NONE;
	memset(&(entry->neighbor.addr_64b[0]), 0x00, 8);
}

uint8_t report_getNeighborIndex(open_addr_t *address) {
	uint8_t i;
	reportEntry_t *entry;

	for(i = 0; i < MAXNUMNEIGHBORS; i++) {

	}

}
