#ifndef __LINKESTIMATOR_H
#define __LINKESTIMATOR_H


#include "opencoap.h"
//=========================== define ==========================================

//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t desc;
   opentimer_id_t       timerId;
} linkestimator_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void linkestimator_init(void);

/**
\}
\}
*/

#endif
