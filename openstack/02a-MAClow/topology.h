#ifndef __TOPOLOGY_H
#define __TOPOLOGY_H

/**
\addtogroup MAClow
\{
\addtogroup topology
\{
*/

#include "opendefs.h"
#include "IEEE802154.h"

//=========================== define ==========================================
#define NUMBER_OF_NODES_PER_HOP	5
//=========================== typedef =========================================
typedef struct {
   uint8_t hop;
   uint8_t bytes6[NUMBER_OF_NODES_PER_HOP];
   uint8_t bytes7[NUMBER_OF_NODES_PER_HOP];
} topologyEntry_t;
//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== prototypes ======================================

bool topology_isAcceptablePacket(ieee802154_header_iht* ieee802514_header);

/**
\}
\}
*/

#endif
