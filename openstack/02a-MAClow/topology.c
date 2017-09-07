#include "opendefs.h"
#include "topology.h"
#include "idmanager.h"
//=========================== defines =========================================


//=========================== variables =======================================
open_addr_t node,sink;
//=========================== prototypes ======================================

//=========================== public ==========================================

/**
\brief Force a topology.

This function is used to force a certain topology, by hard-coding the list of
acceptable neighbors for a given mote. This function is invoked each time a
packet is received. If it returns FALSE, the packet is silently dropped, as if
it were never received. If it returns TRUE, the packet is accepted.

Typically, filtering packets is done by analyzing the IEEE802.15.4 header. An
example body for this function which forces a topology is:

   switch (idmanager_getMyID(ADDR_64B)->addr_64b[7]) {
      case TOPOLOGY_MOTE1:
         if (ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE2) {
            returnVal=TRUE;
         } else {
            returnVal=FALSE;
         }
         break;
      case TOPOLOGY_MOTE2:
         if (ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE1 ||
             ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE3) {
            returnVal=TRUE;
         } else {
            returnVal=FALSE;
         }
         break;
      default:
         returnVal=TRUE;
   }
   return returnVal;

By default, however, the function should return TRUE to *not* force any
topology.

\param[in] ieee802514_header The parsed IEEE802.15.4 MAC header.

\return TRUE if the packet can be received.
\return FALSE if the packet should be silently dropped.
*/
bool topology_isAcceptablePacket(ieee802154_header_iht* ieee802514_header) {
#ifdef FORCETOPOLOGY
   bool returnVal;
   returnVal=TRUE;
   
   node.type = ADDR_64B;
   memcpy(&(node.addr_64b),&addr_64b_node,8);

   sink.type = ADDR_64B;
   memcpy(&(sink.addr_64b),&addr_64b_sink,8);
   


   open_addr_t* my_address = idmanager_getMyID(ADDR_64B);

   if (packetfunctions_sameAddress(my_address,&node)) {
       if(ieee802514_header->src.addr_64b[6] == addr_64b_sink[6] && ieee802514_header->src.addr_64b[7] == addr_64b_sink[7]) {
         returnVal = FALSE;
       }
   } else {
      if (packetfunctions_sameAddress(my_address,&sink)) {
          if(ieee802514_header->src.addr_64b[6] == addr_64b_node[6] && ieee802514_header->src.addr_64b[7] == addr_64b_node[7]) {
            returnVal = FALSE;
          }
      } /*else {
        if ( (ieee802514_header->src.addr_64b[6] == addr_64b_node[6] && ieee802514_header->src.addr_64b[7] == addr_64b_node[7]) ||  (ieee802514_header->src.addr_64b[6] == addr_64b_sink[6] && ieee802514_header->src.addr_64b[7] == addr_64b_sink[7]) ) {
            returnVal = TRUE;
          } else {
          returnVal = FALSE;
        }
     }*/
  }

   return returnVal;
#else
   return TRUE;
#endif
}

//=========================== private =========================================