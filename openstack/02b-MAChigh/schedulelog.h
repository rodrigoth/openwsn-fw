#ifndef __SCHEDULELOG_H
#define __SCHEDULELOG_H

#include "opendefs.h"

typedef struct {
	open_addr_t neighbor;
	uint8_t lastOperation;
	uint16_t slots_offset[CELLLIST_MAX_LEN];
	uint8_t channels_offset[CELLLIST_MAX_LEN];
} ScheduleLogEntry_t;

typedef struct {
	ScheduleLogEntry_t neighbors[MAXNUMNEIGHBORS];
} schedulelog_vars_t;



//=========================== prototypes ======================================
void          schedulelog_init(void);


#endif
