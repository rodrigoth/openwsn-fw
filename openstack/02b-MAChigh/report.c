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

owerror_t report_indicateTxAck(open_addr_t *neighbor, uint8_t tx, uint8_t ack, uint8_t channel) {
	reportEntry_t *entry;
	entry = &(report_vars.reportBuf[0]);
	uint8_t channelIndex = channel - CHANNEL_OFFSET;

	while(entry != NULL) {
		if(memcmp(&(entry->neighbor.addr_64b[0]),&(neighbor->addr_64b[0]),8) == 0) {
			entry->tx[channelIndex] = entry->tx[channelIndex] + tx;
			entry->ack[channelIndex] = entry->ack[channelIndex] + ack;
			return E_SUCCESS;
		}
		entry = entry->next;
	}
	return E_FAIL;
}

owerror_t report_indicateEB(open_addr_t *neighbor, uint8_t channel) {
	reportEntry_t *entry;
	entry = &(report_vars.reportBuf[0]);
	uint8_t channelIndex = channel - CHANNEL_OFFSET;

	while(entry != NULL) {
		if(memcmp(&(entry->neighbor.addr_64b[0]),&(neighbor->addr_64b[0]),8) == 0) {
			entry->eb_receptions[channelIndex] = entry->eb_receptions[channelIndex]+1;
			return E_SUCCESS;
		}
		entry = entry->next;
	}
	return E_FAIL;
}

//fill the report only to the monitored node
void report_pushReportSerial(uint8_t from, uint8_t to,asn_t asn) {
	debug_reportEntry_t debug_reportEntry;
	reportEntry_t *entry;

	uint8_t i;
	for (i = from; i < to; i++) {
		entry = &(report_vars.reportBuf[i]);

		if(entry->used == FALSE) return;

		memset(&(debug_reportEntry.ack[0]),0,sizeof(uint8_t)*CHANNELS);
		memset(&(debug_reportEntry.eb_receptions[0]),0,sizeof(uint8_t)*CHANNELS);
		memset(&(debug_reportEntry.tx[0]),0,sizeof(uint8_t)*CHANNELS);
		memset(&(debug_reportEntry.neighbor.addr_64b[0]), 0x00, 8);
		debug_reportEntry.asn.bytes0and1 = 0;
		debug_reportEntry.asn.bytes2and3 = 0;
		debug_reportEntry.asn.byte4      = 0;


		memcpy(&(debug_reportEntry.neighbor.addr_64b[0]),&(entry->neighbor.addr_64b[0]),8);
		memcpy(&(debug_reportEntry.ack[0]),&(entry->ack[0]),sizeof(uint8_t)*CHANNELS);
		memcpy(&(debug_reportEntry.tx[0]),&(entry->tx[0]),sizeof(uint8_t)*CHANNELS);
		memcpy(&(debug_reportEntry.eb_receptions[0]),&(entry->eb_receptions[0]),sizeof(uint8_t)*CHANNELS);
		debug_reportEntry.asn.bytes0and1 = asn.bytes0and1;
        debug_reportEntry.asn.bytes2and3 = asn.bytes2and3;
        debug_reportEntry.asn.byte4      = asn.byte4;

		openserial_printStatus(STATUS_EB,(uint8_t*)&debug_reportEntry,sizeof(debug_reportEntry_t));

		//reset the statistics for the next observation window
		memset(&(entry->ack[0]),0,sizeof(uint8_t)*CHANNELS);
		memset(&(entry->eb_receptions[0]),0,sizeof(uint8_t)*CHANNELS);
		memset(&(entry->tx[0]),0,sizeof(uint8_t)*CHANNELS);
	}
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

	memset(&(entry->ack[0]),0,sizeof(uint8_t)*CHANNELS);
	memset(&(entry->eb_receptions[0]),0,sizeof(uint8_t)*CHANNELS);
	memset(&(entry->tx[0]),0,sizeof(uint8_t)*CHANNELS);

	entry->neighbor.type = ADDR_NONE;
	memset(&(entry->neighbor.addr_64b[0]), 0x00, 8);
}

