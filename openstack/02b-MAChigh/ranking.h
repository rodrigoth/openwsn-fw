#ifndef __RANKING_H
#define __RANKING_H


#include "opentimers.h"
#include "opendefs.h"


//=========================== define ==========================================
#define RANKING_OBSERVATION_WINDOW_PERIOD_MS 40000 // default 4 minutes
#define RANKING_FIRST_OBSERVATION_PERIOD 15000

//=========================== typedef =========================================


//=========================== variables =======================================

typedef struct {
   opentimers_id_t     timerId;  ///< periodic timer which triggers transmission
   bool 			   timeStarted;
   uint8_t 			   parentIndex;
   bool                haveParent;  
   uint8_t			   lastParentIndex;

} ranking_vars_t;

//=========================== prototypes ======================================

void ranking_init(void);
void ranking_startPeriodRanking(void);
bool ranking_getPreferredParentEui64(open_addr_t* addressToWrite);

#endif