#include "schedulelog.h"
#include "openserial.h"
//=========================== variables =======================================
schedulelog_vars_t schedule_var;
//=========================== prototypes ======================================

//=========================== public ==========================================

/**
\brief Initializes this module.
*/
void schedulelog_init() {
	uint8_t i;

	for(i = 0; i<MAXNUMNEIGHBORS; i++) {
		schedule_var.neighbors[i].neighbor.type = ADDR_NONE;
		schedule_var.neighbors[i].lastOperation = 0;
		memset(schedule_var.neighbors[i].channels_offset[0],0,sizeof(schedule_var.neighbors[i].channels_offset));
		memset(schedule_var.neighbors[i].slots_offset[0],0,sizeof(schedule_var.neighbors[i].slots_offset));
	}

	openserial_printError(COMPONENT_NEIGHBORS, error_code, arg1, arg2);


}
