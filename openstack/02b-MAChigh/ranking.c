#include "ranking.h"
#include "scheduler.h"
#include "neighbors.h"
#include "openserial.h"
#include "IEEE802154E.h"
#include "opendefs.h"
//=========================== variables =======================================

ranking_vars_t ranking_vars;


//=========================== prototypes ======================================

void ranking_timer_cb(opentimers_id_t id);
void ranking_task_cb(void);
void sort_arrays(uint8_t* receptions, uint8_t* indexes) ;

//=========================== public ==========================================
void ranking_init() {
	memset(&ranking_vars,0,sizeof(ranking_vars_t));
}


void ranking_startPeriodRanking() {
    if (ranking_vars.timeStarted == FALSE) {
		ranking_vars.timerId = opentimers_create();
		opentimers_scheduleIn(ranking_vars.timerId,RANKING_FIRST_OBSERVATION_PERIOD,TIME_MS,TIMER_PERIODIC,ranking_timer_cb);
		ranking_vars.timeStarted = TRUE;
	}
}


bool ranking_getPreferredParentEui64(open_addr_t* addressToWrite) {
	if(ranking_vars.haveParent) {
		bool isPreferred =  neighbors_isPreferredParent(ranking_vars.parentIndex);
		if(isPreferred) {
			return neighbors_getNeighborEui64(addressToWrite,ADDR_64B,ranking_vars.parentIndex);	
		}
		
	}
	return FALSE;
}



//=========================== helpers ==========================================

void ranking_timer_cb(opentimers_id_t id){
  	scheduler_push_task(ranking_task_cb,TASKPRIO_SF0);	
}

void ranking_task_cb() {
	uint8_t i;

	uint8_t broadcastReceptions[MAXNUMNEIGHBORS];
	uint8_t neighborsIndexes[MAXNUMNEIGHBORS];

	memset(&broadcastReceptions[0],0,sizeof(uint8_t)*MAXNUMNEIGHBORS);
	memset(&neighborsIndexes[0],0,sizeof(uint8_t)*MAXNUMNEIGHBORS);

	neighbors_getAllBroadcastReception(&broadcastReceptions[0],&neighborsIndexes[0]);

	sort_arrays(&broadcastReceptions[0],&neighborsIndexes[0]);
	
	if(!ranking_vars.haveParent) {
		ranking_vars.parentIndex = neighborsIndexes[0];
		ranking_vars.haveParent = TRUE;
		ranking_vars.lastParentIndex = neighborsIndexes[0];
		neighbors_setPreferredParent(neighborsIndexes[0], TRUE,TRUE);
	 	openserial_printError(COMPONENT_RANKING,ERR_FIRST_PARENT, (errorparameter_t)neighborsIndexes[0],(errorparameter_t)neighborsIndexes[0]);	
 	} else {
 		if (ranking_vars.lastParentIndex != neighborsIndexes[0]) {
 			neighbors_setPreferredParent(ranking_vars.lastParentIndex,FALSE,TRUE);
 			neighbors_setPreferredParent(neighborsIndexes[0], TRUE,TRUE);
 			ranking_vars.lastParentIndex = neighborsIndexes[0];
 			ranking_vars.parentIndex = neighborsIndexes[0];
 			openserial_printError(COMPONENT_RANKING,ERR_PARENT_CHANGED, (errorparameter_t)0,(errorparameter_t)0);	

 		} else {
 			openserial_printError(COMPONENT_RANKING,ERR_PARENT_NOT_CHANGED, (errorparameter_t)0,(errorparameter_t)0);	
 		}
 	}
	
	for(i = 0; i < MAXNUMNEIGHBORS; i++) {
		openserial_printError(COMPONENT_RANKING,ERR_SORT, (errorparameter_t)broadcastReceptions[i],(errorparameter_t)neighborsIndexes[i]);	
	}
	
	neighbors_resetBroadcastReception();
    opentimers_scheduleIn(ranking_vars.timerId,RANKING_OBSERVATION_WINDOW_PERIOD_MS,TIME_MS,TIMER_PERIODIC,ranking_timer_cb);
}

void sort_arrays(uint8_t* receptions, uint8_t* indexes) {
    uint8_t i, j;    
 	uint8_t current_nb,current_broadcast_count;

    for (i = 0; i < MAXNUMNEIGHBORS; i++) {
        for (j = i +1; j < MAXNUMNEIGHBORS; j++ ) {
            if (receptions[i] < receptions[j]) {
                current_broadcast_count = receptions[i];
                current_nb = indexes[i];

                receptions[i] = receptions[j];
                receptions[j] = current_broadcast_count;

                indexes[i] = indexes[j];
                indexes[j] = current_nb;
            }
        }
    }
}
