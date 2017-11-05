#include "opendefs.h"
#include "sf0.h"
#include "neighbors.h"
#include "sixtop.h"
#include "scheduler.h"
#include "schedule.h"
#include "idmanager.h"
#include "openapps.h"
#include "processIE.h"

//=========================== definition =====================================

#define SF0THRESHOLD      2

//=========================== variables =======================================

sf0_vars_t sf0_vars;

//=========================== prototypes ======================================

void sf0_bandwidthEstimate_task(void);

//=========================== public ==========================================

void sf0_init(void) {
    memset(&sf0_vars,0,sizeof(sf0_vars_t));
    sf0_vars.numAppPacketsPerSlotFrame = 0;
}


// this function is called once per slotframe. 
void sf0_notifyNewSlotframe(void) {
   scheduler_push_task(sf0_bandwidthEstimate_task,TASKPRIO_SF0);
}

void sf0_setBackoff(uint8_t value){
    sf0_vars.backoff = value;
}

//=========================== private =========================================



void sf0_bandwidthEstimate_task(void){
    open_addr_t    neighbor;
    bool           foundNeighbor;
    int8_t         bw_outgoing;
    int8_t         bw_incoming;
    int8_t         bw_self;
    
    // do not reserve cells if I'm a DAGroot
    if (idmanager_getIsDAGroot()){
        return;
    }
    
    if (sf0_vars.backoff>0){
        sf0_vars.backoff -= 1;
        return;
    }
    
    // get preferred parent
    foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
    if (foundNeighbor==FALSE) {
        return;
    }
    
    // get bandwidth of outgoing, incoming and self.
    // Here we just calculate the estimated bandwidth for 
    // the application sending on dedicate cells(TX or Rx).
    //bw_outgoing = schedule_getNumOfSlotsByType(CELLTYPE_TX);
    bw_outgoing = schedule_getNumberSlotToPreferredParent(&neighbor);
    bw_incoming = schedule_getNumOfSlotsByType(CELLTYPE_RX);
    
    // get self required bandwith, you can design your
    // application and assign bw_self accordingly. 
    // for example:
    //    bw_self = application_getBandwdith(app_name);
    // By default, it's set to zero.
    // bw_self = openapps_getBandwidth(COMPONENT_UINJECT);
    bw_self = sf0_vars.numAppPacketsPerSlotFrame;
    
    // In SF0, scheduledCells = bw_outgoing
    //         requiredCells  = bw_incoming + bw_self
    // when scheduledCells<requiredCells, add one or more cell
    
    if (bw_outgoing == 0 || bw_outgoing < bw_incoming+bw_self) {
        if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
            // one sixtop transcation is happening, only one instance at one time
            return;
        }

        uint8_t requiredCells = bw_incoming+bw_self+1-bw_outgoing;
        if(requiredCells > SCHEDULEIEMAXNUMCELLS) {
        	requiredCells = SCHEDULEIEMAXNUMCELLS;
        }

        sixtop_request(IANA_6TOP_CMD_ADD,&neighbor,requiredCells);
    } else {
		// remove cell(s)
		if ((bw_incoming+bw_self) < (bw_outgoing-SF0THRESHOLD)) {
			if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
				// one sixtop transcation is happening, only one instance at one time
				   return;
			}

			uint8_t cellsToRemove = bw_outgoing - (bw_incoming+bw_self+1);

			if(cellsToRemove > SCHEDULEIEMAXNUMCELLS ) {
				cellsToRemove = SCHEDULEIEMAXNUMCELLS;
			}
			sixtop_request(IANA_6TOP_CMD_DELETE,&neighbor,cellsToRemove);
		}
	}

}

void sf0_appPktPeriod(uint8_t numAppPacketsPerSlotFrame){
    sf0_vars.numAppPacketsPerSlotFrame = numAppPacketsPerSlotFrame;
}
