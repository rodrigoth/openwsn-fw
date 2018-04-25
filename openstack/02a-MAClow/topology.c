#include "opendefs.h"
#include "topology.h"
#include "idmanager.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================
//=========================== public ==========================================

bool topology_isAcceptablePacket(ieee802154_header_iht* ieee802514_header,int8_t rssi) {
#ifdef FORCETOPOLOGY
	bool returnVal;
	returnVal=FALSE;

	if (idmanager_getIsDAGroot()) {
		returnVal = TRUE;
	} else {

		if(ieee802514_header->src.addr_64b[6] == 0x09  && ieee802514_header->src.addr_64b[7] == 0x62) {
				   returnVal = TRUE;
		 }
	}

   return returnVal;

#else
   return TRUE;
#endif
}
