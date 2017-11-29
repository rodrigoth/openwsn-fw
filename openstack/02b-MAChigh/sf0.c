#include "opendefs.h"
#include "sf0.h"
#include "neighbors.h"
#include "sixtop.h"
#include "scheduler.h"
#include "schedule.h"
#include "idmanager.h"
#include "openapps.h"
#include "openrandom.h"

//=========================== definition =====================================

#define SF0_ID            0
#define SF0THRESHOLD      2

//=========================== variables =======================================

sf0_vars_t sf0_vars;

//=========================== prototypes ======================================

void sf0_bandwidthEstimate_task(void);
// sixtop callback 
uint16_t sf0_getMetadata(void);
metadata_t sf0_translateMetadata(void);
void sf0_handleRCError(uint8_t code);

//=========================== public ==========================================

void sf0_init(void) {
    memset(&sf0_vars,0,sizeof(sf0_vars_t));
    sf0_vars.numAppPacketsPerSlotFrame = 0;
    sixtop_setSFcallback(sf0_getsfid,sf0_getMetadata,sf0_translateMetadata,sf0_handleRCError);
}

// this function is called once per slotframe. 
void sf0_notifyNewSlotframe(void) {
   scheduler_push_task(sf0_bandwidthEstimate_task,TASKPRIO_SF0);
}

void sf0_setBackoff(uint8_t value){
    sf0_vars.backoff = value;
}

//=========================== callback =========================================

uint8_t sf0_getsfid(void){
    return SF0_ID;
}

uint16_t sf0_getMetadata(void){
    return SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_HANDLE;
}

metadata_t sf0_translateMetadata(void){
    return METADATA_TYPE_FRAMEID;
}

void sf0_handleRCError(uint8_t code){
    if (code==IANA_6TOP_RC_BUSY || code==IANA_6TOP_RC_RESET ){
        // disable sf0 for [0...2^4] slotframe long time
    	sf0_setBackoff(openrandom_get16b()%(1<<4));
    }
    
    
    if (code==IANA_6TOP_RC_ERROR){
        // TBD: the neighbor can't statisfy the 6p request, call sf0 to make a decision
    }
    
    if (code==IANA_6TOP_RC_VER_ERR){
        // TBD: the 6p verion does not match
    }
    
    if (code==IANA_6TOP_RC_SFID_ERR){
        // TBD: the sfId does not match
    }
}

//=========================== private =========================================

void sf0_bandwidthEstimate_task(void){
    open_addr_t    neighbor;
    bool           foundNeighbor;
    int8_t         bw_outgoing;
    int8_t         bw_incoming;
    int8_t         bw_self;
    cellInfo_ht    celllist_add[CELLLIST_MAX_LEN];
    cellInfo_ht    celllist_delete[CELLLIST_MAX_LEN];
    
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
    bw_outgoing = schedule_getNumOfSlotsByType(CELLTYPE_TX);
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

    	openserial_printInfo(COMPONENT_SIXTOP,ERR_SIXTOP_LIST,(errorparameter_t)bw_outgoing,(errorparameter_t) bw_incoming+bw_self);

    	uint8_t requiredCells = bw_incoming+bw_self+1-bw_outgoing;
		if(requiredCells > CELLLIST_MAX_LEN) {
			requiredCells = CELLLIST_MAX_LEN;
		}


    	if (sf0_candidateAddCellList(celllist_add,requiredCells)==FALSE){
            // failed to get cell list to add
            return;
        }
        sixtop_request(
            IANA_6TOP_CMD_ADD,                  // code
            &neighbor,                          // neighbor
            requiredCells, 					 	// number cells
            LINKOPTIONS_TX,                     // cellOptions
            celllist_add,                       // celllist to add
            NULL,                               // celllist to delete (not used)
            SF0_ID,                             // sfid
            0,                                  // list command offset (not used)
            0                                   // list command maximum celllist (not used)
        );
    } else {
        // remove cell(s)
        if ( (bw_incoming+bw_self) < (bw_outgoing-SF0THRESHOLD)) {

        	uint8_t cellsToRemove = bw_outgoing - (bw_incoming+bw_self+1);

			if(cellsToRemove > CELLLIST_MAX_LEN ) {
				cellsToRemove = CELLLIST_MAX_LEN;
			}


        	if (sf0_candidateRemoveCellList(celllist_delete,&neighbor,cellsToRemove)==FALSE){
                // failed to get cell list to delete
                return;
            }
            sixtop_request(
                IANA_6TOP_CMD_DELETE,   // code
                &neighbor,              // neighbor
				cellsToRemove,           // number cells
                LINKOPTIONS_TX,         // cellOptions
                NULL,                   // celllist to add (not used)
                celllist_delete,        // celllist to delete
                SF0_ID,                 // sfid
                0,                      // list command offset (not used)
                0                       // list command maximum celllist (not used)
            );
        }
    }
}

void sf0_appPktPeriod(uint8_t numAppPacketsPerSlotFrame){
    sf0_vars.numAppPacketsPerSlotFrame = numAppPacketsPerSlotFrame;
}

bool sf0_candidateAddCellList(
      cellInfo_ht* cellList,
      uint8_t      requiredCells
   ){
    uint8_t i;
    frameLength_t slotoffset;
    uint8_t numCandCells;
    
    memset(cellList,0,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
    numCandCells=0;
    for(i=0;i<CELLLIST_MAX_LEN;i++){
        slotoffset = openrandom_get16b()%schedule_getFrameLength();
        if(schedule_isSlotOffsetAvailable(slotoffset)==TRUE){
            cellList[numCandCells].slotoffset       = slotoffset;
            cellList[numCandCells].channeloffset    = openrandom_get16b()%16;
            cellList[numCandCells].isUsed           = TRUE;
            numCandCells++;
        }
    }
   
    if (numCandCells<requiredCells || requiredCells==0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

bool sf0_candidateRemoveCellList(
      cellInfo_ht* cellList,
      open_addr_t* neighbor,
      uint8_t      requiredCells
   ){
   uint8_t              i;
   uint8_t              numCandCells;
   slotinfo_element_t   info;
   
   memset(cellList,0,CELLLIST_MAX_LEN*sizeof(cellInfo_ht));
   numCandCells    = 0;
   for(i=0;i<schedule_getFrameLength();i++){
      schedule_getSlotInfo(i,neighbor,&info);
      if(info.link_type == CELLTYPE_TX){
         cellList[numCandCells].slotoffset       = i;
         cellList[numCandCells].channeloffset    = info.channelOffset;
         cellList[numCandCells].isUsed           = TRUE;
         numCandCells++;
         if (numCandCells==CELLLIST_MAX_LEN){
            break;
         }
      }
   }
   
   if(numCandCells<requiredCells){
      return FALSE;
   }else{
      return TRUE;
   }
}
