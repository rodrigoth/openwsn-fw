#ifndef __CEXAMPLE_H
#define __CEXAMPLE_H

/**
\addtogroup AppUdp
\{
\addtogroup cexample
\{
*/
#include "opencoap.h"
//=========================== define ==========================================

//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t desc;
   opentimer_id_t       timerId;
} cexample3_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void cexample3_init(void);

/**
\}
\}
*/

#endif
