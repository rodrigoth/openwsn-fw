#include "opendefs.h"
#include "topology.h"
#include "idmanager.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================
//=========================== public ==========================================

bool topology_isAcceptablePacket(ieee802154_header_iht* ieee802514_header,int8_t rssi) {
#ifdef FORCETOPOLOGY
   if(rssi < -65) return FALSE;

   return TRUE;

#else
   return TRUE;
#endif
}
