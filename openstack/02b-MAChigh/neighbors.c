#include "opendefs.h"
#include "neighbors.h"
#include "openqueue.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "openserial.h"
#include "IEEE802154E.h"
#include "openrandom.h"
#include "openreport.h"

#define CLEARNEIGHBORS 20
#define BROADCAST_FACTOR 20
//=========================== variables =======================================

static neighbors_vars_t neighbors_vars;
uint8_t clearInactiveNeighborsCounter;
uint8_t prevDesync;

//=========================== prototypes ======================================

void registerNewNeighbor(
        open_addr_t* neighborID,
        int8_t       rssi,
        asn_t*       asnTimestamp,
        bool         joinPrioPresent,
        uint8_t      joinPrio,
        bool         insecure
     );
bool isNeighbor(open_addr_t* neighbor);
void removeNeighbor(uint8_t neighborIndex);
bool isThisRowMatching(
        open_addr_t* address,
        uint8_t      rowNumber
     );
uint8_t getMaxBroadcastRx(void);
uint8_t getMinBroadcastRx(void);
//=========================== public ==========================================

/**
\brief Initializes this module.
*/
void neighbors_init() {
   
   // clear module variables
   memset(&neighbors_vars,0,sizeof(neighbors_vars_t));
   // The .used fields get reset to FALSE by this memset.
   
   neighbors_vars.pdrWMEMA = 1.0;

}

//===== getters

/**
\brief Retrieve the number of neighbors this mote's currently knows of.

\returns The number of neighbors this mote's currently knows of.
*/
uint8_t neighbors_getNumNeighbors() {
   uint8_t i;
   uint8_t returnVal;
   
   returnVal=0;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (neighbors_vars.neighbors[i].used==TRUE) {
         returnVal++;
      }
   }
   return returnVal;
}

dagrank_t neighbors_getNeighborRank(uint8_t index) {
   return neighbors_vars.neighbors[index].DAGrank;
}

/**
\brief Find neighbor to which to send KA.

This function iterates through the neighbor table and identifies the neighbor
we need to send a KA to, if any. This neighbor satisfies the following
conditions:
- it is one of our preferred parents
- we haven't heard it for over kaPeriod

\param[in] kaPeriod The maximum number of slots I'm allowed not to have heard
   it.

\returns A pointer to the neighbor's address, or NULL if no KA is needed.
*/
open_addr_t* neighbors_getKANeighbor(uint16_t kaPeriod) {
   uint8_t         i;
   uint16_t        timeSinceHeard;
   
   // policy is not to KA to non-preferred parents so go strait to check if Preferred Parent is aging
   if (icmpv6rpl_getPreferredParentIndex(&i)) {      // we have a Parent
      if (neighbors_vars.neighbors[i].used==1) {     // that resolves to a neighbor in use (should always)
         timeSinceHeard = ieee154e_asnDiff(&neighbors_vars.neighbors[i].asn);
         if (timeSinceHeard>kaPeriod) {
            // this neighbor needs to be KA'ed to
            return &(neighbors_vars.neighbors[i].addr_64b);
         }
      }
   }
   return NULL;
}

/**
\brief Find neighbor which should act as a Join Proxy during the join process.

This function iterates through the neighbor table and identifies the neighbor
with lowest join priority metric to send join traffic through. 

\returns A pointer to the neighbor's address, or NULL if no join proxy is found.
*/
open_addr_t* neighbors_getJoinProxy() {
   uint8_t i;
   uint8_t joinPrioMinimum;
   open_addr_t* joinProxy;

   joinPrioMinimum = 0xff;
   joinProxy = NULL;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (neighbors_vars.neighbors[i].used==TRUE && 
              neighbors_vars.neighbors[i].stableNeighbor==TRUE &&
              neighbors_vars.neighbors[i].joinPrio <= joinPrioMinimum) {
          joinProxy = &(neighbors_vars.neighbors[i].addr_64b);
          joinPrioMinimum = neighbors_vars.neighbors[i].joinPrio;
      }
   }
   return joinProxy;
}

bool neighbors_getNeighborNoResource(uint8_t index){
    return neighbors_vars.neighbors[index].f6PNORES;
}

uint8_t neighbors_getGeneration(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            break;
        }
    }
    return neighbors_vars.neighbors[i].generation;
}

uint8_t neighbors_getSequenceNumber(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            break;
        }
    }
    return neighbors_vars.neighbors[i].sequenceNumber;

}

uint8_t neighbors_getpdrWMEMA() {
	return neighbors_vars.pdrWMEMA;
}

uint8_t	neighbors_getParentRSSI() {
	uint8_t parentIndex ;
	if(icmpv6rpl_getPreferredParentIndex(&parentIndex)) {
		return (-1)*neighbors_vars.neighbors[parentIndex].rssi;
	}
	return 0;
}


//===== interrogators

/**
\brief Indicate whether some neighbor is a stable neighbor

\param[in] address The address of the neighbor, a full 128-bit IPv6 addres.

\returns TRUE if that neighbor is stable, FALSE otherwise.
*/
bool neighbors_isStableNeighbor(open_addr_t* address) {
   uint8_t     i;
   open_addr_t temp_addr_64b;
   open_addr_t temp_prefix;
   bool        returnVal;
   
   // by default, not stable
   returnVal  = FALSE;
   
   // but neighbor's IPv6 address in prefix and EUI64
   switch (address->type) {
      case ADDR_128B:
         packetfunctions_ip128bToMac64b(address,&temp_prefix,&temp_addr_64b);
         break;
      default:
         openserial_printCritical(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)0);
         return returnVal;
   }
   
   // iterate through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(&temp_addr_64b,i) && neighbors_vars.neighbors[i].stableNeighbor==TRUE) {
         returnVal  = TRUE;
         break;
      }
   }
   
   return returnVal;
}

/**
\brief Indicate whether some neighbor is a stable neighbor

\param[in] index into the neighbor table.

\returns TRUE if that neighbor is in use and stable, FALSE otherwise.
*/
bool neighbors_isStableNeighborByIndex(uint8_t index) {
   return (neighbors_vars.neighbors[index].stableNeighbor &&
           neighbors_vars.neighbors[index].used);
}

/**
\brief Indicate whether some neighbor is an insecure neighbor

\param[in] address The address of the neighbor, a 64-bit address.

\returns TRUE if that neighbor is insecure, FALSE otherwise.
*/
bool neighbors_isInsecureNeighbor(open_addr_t* address) {
   uint8_t     i;
   bool        returnVal;
   
   // if not found, insecure
   returnVal  = TRUE;
   
   switch (address->type) {
      case ADDR_64B:
         break;
      default:
         openserial_printCritical(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)0);
         return returnVal;
   }
   
   // iterate through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(address,i)) {
         returnVal  = neighbors_vars.neighbors[i].insecure;
         break;
      }
   }
   
   return returnVal;
}

/**
\brief Indicate whether some neighbor has a lower DAG rank that me.

\param[in] index The index of that neighbor in the neighbor table.

\returns TRUE if that neighbor is in use and has a lower DAG rank than me, FALSE otherwise.
*/
bool neighbors_isNeighborWithLowerDAGrank(uint8_t index) {
   bool    returnVal;
   
   if (neighbors_vars.neighbors[index].used==TRUE &&
       neighbors_vars.neighbors[index].DAGrank < icmpv6rpl_getMyDAGrank()) { 
      returnVal = TRUE;
   } else {
      returnVal = FALSE;
   }
   
   return returnVal;
}


/**
\brief Indicate whether some neighbor has a lower DAG rank that me.

\param[in] index The index of that neighbor in the neighbor table.

\returns TRUE if that neighbor is in use and has a higher DAG rank than me, FALSE otherwise.
*/
bool neighbors_isNeighborWithHigherDAGrank(uint8_t index) {
   bool    returnVal;
   
   if (neighbors_vars.neighbors[index].used==TRUE &&
       neighbors_vars.neighbors[index].DAGrank >= icmpv6rpl_getMyDAGrank()) { 
      returnVal = TRUE;
   } else {
      returnVal = FALSE;
   }
   
   return returnVal;
}

bool neighbors_reachedMaxTransmission(uint8_t index){
	bool returnVal;
	uint8_t totalTx  = (uint8_t)schedule_getTotalTxToNeighbor(&neighbors_vars.neighbors[index].addr_64b);
	uint8_t attempt;

	#if defined(USERSSI) || defined(USEMINHOP)
		attempt = 1;
	#else
		attempt = 15;
	#endif

	if (prevDesync == 0) {prevDesync = ieee154e_getNumOfDesync();}
    
    if (totalTx  >=  attempt) {
        returnVal = TRUE;
    } else {
    	if(ieee154e_getNumOfDesync() - prevDesync >= DESYNCTHRESHOLD) {
			openserial_printError(COMPONENT_NEIGHBORS,ERR_NEIGHBORS_DESYNC,(errorparameter_t)ieee154e_getNumOfDesync(),(errorparameter_t)prevDesync);
    		prevDesync = ieee154e_getNumOfDesync();
    		returnVal = TRUE;
    	} else {
    		returnVal = FALSE;
    	}
    }
    
    return returnVal;
}

//===== updating neighbor information

/**
\brief Indicate some (non-ACK) packet was received from a neighbor.

This function should be called for each received (non-ACK) packet so neighbor
statistics in the neighbor table can be updated.

The fields which are updated are:
- numRx
- rssi
- asn
- stableNeighbor
- switchStabilityCounter

\param[in] l2_src MAC source address of the packet, i.e. the neighbor who sent
   the packet just received.
\param[in] rssi   RSSI with which this packet was received.
\param[in] asnTs  ASN at which this packet was received.
\param[in] joinPrioPresent Whether a join priority was present in the received
   packet.
\param[in] joinPrio The join priority present in the packet, if any.
*/
void neighbors_indicateRx(open_addr_t* l2_src,
                          int8_t       rssi,
                          asn_t*       asnTs,
                          bool         joinPrioPresent,
                          uint8_t      joinPrio,
                          bool         insecure) {
   uint8_t i;
   bool    newNeighbor;
   
   // update existing neighbor
   newNeighbor = TRUE;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(l2_src,i)) {
         
         // this is not a new neighbor
         newNeighbor = FALSE;
         
         // whether the neighbor is considered as secure or not
         neighbors_vars.neighbors[i].insecure = insecure;

         // update numRx, rssi, asn
         neighbors_vars.neighbors[i].numRx++;
         neighbors_vars.neighbors[i].rssi=rssi;
         memcpy(&neighbors_vars.neighbors[i].asn,asnTs,sizeof(asn_t));
         //update jp
         if (joinPrioPresent==TRUE){
            neighbors_vars.neighbors[i].joinPrio=joinPrio;
         }
         
         // update stableNeighbor, switchStabilityCounter
         if (neighbors_vars.neighbors[i].stableNeighbor==FALSE) {
            if (neighbors_vars.neighbors[i].rssi>BADNEIGHBORMAXRSSI) {
               neighbors_vars.neighbors[i].switchStabilityCounter++;
               if (neighbors_vars.neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  neighbors_vars.neighbors[i].switchStabilityCounter=0;
                  neighbors_vars.neighbors[i].stableNeighbor=TRUE;
               }
            } else {
               neighbors_vars.neighbors[i].switchStabilityCounter=0;
            }
         } else if (neighbors_vars.neighbors[i].stableNeighbor==TRUE) {
            if (neighbors_vars.neighbors[i].rssi<GOODNEIGHBORMINRSSI) {
               neighbors_vars.neighbors[i].switchStabilityCounter++;
               if (neighbors_vars.neighbors[i].switchStabilityCounter>=SWITCHSTABILITYTHRESHOLD) {
                  neighbors_vars.neighbors[i].switchStabilityCounter=0;
                   neighbors_vars.neighbors[i].stableNeighbor=FALSE;
               }
            } else {
               neighbors_vars.neighbors[i].switchStabilityCounter=0;
            }
         }
         
         // stop looping
         break;
      }
   }
   
   // register new neighbor
   if (newNeighbor==TRUE) {
      registerNewNeighbor(l2_src, rssi, asnTs, joinPrioPresent, joinPrio, insecure);
   }
}

void neighbors_indicateBroadcastReception(open_addr_t* address) {
	uint8_t i;
	for (i=0;i<MAXNUMNEIGHBORS;i++) {
		 if (isThisRowMatching(address,i)) {
			 neighbors_vars.neighbors[i].broadcast_rx++;
		 }
	}

	if(getMaxBroadcastRx() >= BROADCAST_FACTOR) {
		for (i=0;i<MAXNUMNEIGHBORS;i++) {
			if (neighbors_vars.neighbors[i].used == TRUE) {
				 neighbors_vars.neighbors[i].broadcast_rx /= 2;
			 }
		}
	}
}

/**
\brief Indicate some packet was sent to some neighbor.

This function should be called for each transmitted (non-ACK) packet so
neighbor statistics in the neighbor table can be updated.

The fields which are updated are:
- numTx
- numTxACK
- asn

\param[in] l2_dest MAC destination address of the packet, i.e. the neighbor
   who I just sent the packet to.
\param[in] numTxAttempts Number of transmission attempts to this neighbor.
\param[in] was_finally_acked TRUE iff the packet was ACK'ed by the neighbor
   on final transmission attempt.
\param[in] asnTs ASN of the last transmission attempt.
*/
void neighbors_indicateTx(open_addr_t* l2_dest,
                          uint8_t      numTxAttempts,
                          bool         was_finally_acked,
                          asn_t*       asnTs) {
   uint8_t i;
   // don't run through this function if packet was sent to broadcast address
   if (packetfunctions_isBroadcastMulticast(l2_dest)==TRUE) {
      return;
   }
   
   // loop through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(l2_dest,i)) {
         // handle roll-over case
        
          if (neighbors_vars.neighbors[i].numTx>(0xff-numTxAttempts)) {
              neighbors_vars.neighbors[i].numWraps++; //counting the number of times that tx wraps.
              neighbors_vars.neighbors[i].numTx/=2;
              neighbors_vars.neighbors[i].numTxACK/=2;
           }
         // update statistics
        neighbors_vars.neighbors[i].numTx += numTxAttempts; 
        
        if (was_finally_acked==TRUE) {
            neighbors_vars.neighbors[i].numTxACK++;
            memcpy(&neighbors_vars.neighbors[i].asn,asnTs,sizeof(asn_t));
        }
        // #TODO : investigate this TX wrap thing! @incorrect in the meantime
        // DB (Nov 2015) I believe this is correct. The ratio numTx/numTxAck is still a correct approximation
        // of ETX after scaling down by a factor 2. Obviously, each one of numTx and numTxAck is no longer an
        // accurate count of the related events, so don't rely of them to keep track of frames sent and ack received,
        // and don't use numTx as a frame sequence number!
        // The scaling means that older events have less weight when the scaling occurs. It is a way of progressively
        // forgetting about the ancient past and giving more importance to recent observations.
        break;
      }
   }
}

void neighbors_updateSequenceNumber(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            neighbors_vars.neighbors[i].sequenceNumber = (neighbors_vars.neighbors[i].sequenceNumber+1) & 0x0F;
            break;
        }
    }
}

void neighbors_updateGeneration(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            neighbors_vars.neighbors[i].generation = (neighbors_vars.neighbors[i].generation+1)%9;
            break;
        }
    }
}

void neighbors_resetGeneration(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            neighbors_vars.neighbors[i].generation = 0;
            break;
        }
    }
}

//===== write addresses

/**
\brief Write the 64-bit address of some neighbor to some location.
// Returns false if neighbor not in use or address type is not 64bits
*/
bool  neighbors_getNeighborEui64(open_addr_t* address, uint8_t addr_type, uint8_t index){
   bool ReturnVal = FALSE;
   switch(addr_type) {
      case ADDR_64B:
         memcpy(&(address->addr_64b),&(neighbors_vars.neighbors[index].addr_64b.addr_64b),LENGTH_ADDR64b);
         address->type=ADDR_64B;
         ReturnVal=neighbors_vars.neighbors[index].used;
         break;
      default:
         openserial_printCritical(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)addr_type,
                               (errorparameter_t)1);
         break; 
   }
   return ReturnVal;
}
// ==== update backoff
void neighbors_updateBackoff(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            // increase the backoffExponent
            if (neighbors_vars.neighbors[i].backoffExponenton<MAXBE) {
                neighbors_vars.neighbors[i].backoffExponenton++;
            }
            // set the backoff to a random value in [0..2^BE]
            neighbors_vars.neighbors[i].backoff = openrandom_get16b()%(1<<neighbors_vars.neighbors[i].backoffExponenton);
            break;
        }
    }
}
void neighbors_decreaseBackoff(open_addr_t* address){
    uint8_t i;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            if (neighbors_vars.neighbors[i].backoff>0) {
                neighbors_vars.neighbors[i].backoff--;
            }
            break;
        }
    }
}
bool neighbors_backoffHitZero(open_addr_t* address){
    uint8_t i;
    bool returnVal;
    
    returnVal = FALSE;
    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            returnVal = (neighbors_vars.neighbors[i].backoff==0);
            break;
        }
    }
    return returnVal;
}

void neighbors_resetBackoff(open_addr_t* address){
    uint8_t i;

    for (i=0;i<MAXNUMNEIGHBORS;i++){
        if (packetfunctions_sameAddress(address, &neighbors_vars.neighbors[i].addr_64b)){
            neighbors_vars.neighbors[i].backoffExponenton     = MINBE-1;
            neighbors_vars.neighbors[i].backoff               = 0;
            break;
        }
    }
}

//===== setters

void neighbors_setNeighborRank(uint8_t index, dagrank_t rank) {
   neighbors_vars.neighbors[index].DAGrank=rank;

}

void neighbors_setNeighborNoResource(open_addr_t* address){
   uint8_t i;
   
   // loop through neighbor table
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(address,i)) {
          neighbors_vars.neighbors[i].f6PNORES = TRUE;
          break;
      }
   }
}

void neighbors_setPreferredParent(uint8_t index, bool isPreferred){
    neighbors_vars.neighbors[index].parentPreference = isPreferred;
    neighbors_vars.pdrWMEMA = 1.0;
}

//===== managing routing info

/**
\brief return the link cost to a neighbor, expressed as a rank increase from this neighbor to this node

This really belongs to icmpv6rpl but it would require a much more complex interface to the neighbor table
*/

uint16_t neighbors_getLinkMetric(uint8_t index) {
	#ifdef USEMINHOP
		uint8_t parentIndex ;
		if(icmpv6rpl_getPreferredParentIndex(&parentIndex) && index == parentIndex) {
			uint16_t totalTx = schedule_getTotalTxToNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
			uint16_t totalAck = schedule_getTotalAckFromNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
			openreport_indicatePDR(&(neighbors_vars.neighbors[index].addr_64b),totalTx,totalAck,0);
		}

		return MINHOPRANKINCREASE;
	#endif

	#ifdef USERSSI
		uint8_t parentIndex ;
		if(icmpv6rpl_getPreferredParentIndex(&parentIndex) && index == parentIndex) {
			uint16_t totalTx = schedule_getTotalTxToNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
			uint16_t totalAck = schedule_getTotalAckFromNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
			openreport_indicatePDR(&(neighbors_vars.neighbors[index].addr_64b),totalTx,totalAck,0);
		}

		float min = -91.0f;
		float max = -80.0f;
		int8_t rssi = neighbors_vars.neighbors[index].rssi;

		//division by zero when transforming to etx
		if(rssi == -91) {
			rssi = -90;
		} else {
			if(rssi > -80) {
				rssi = -80;
			 }
		}

		float etxFromRSSI = 1/((rssi - min)/(max - min));
		return MINHOPRANKINCREASE*etxFromRSSI;
	#endif

	#ifdef USEETXN
		uint16_t  rankIncrease;

		uint16_t totalTx = schedule_getTotalTxToNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
		uint16_t totalAck = schedule_getTotalAckFromNeighbor(&(neighbors_vars.neighbors[index].addr_64b));

		if (totalAck == 0) {
			if (totalTx<=DEFAULTLINKCOST && ieee154e_getNumOfDesync() - prevDesync < DESYNCTHRESHOLD) {
				rankIncrease = 519; //70
			} else {
				rankIncrease = LARGESTLINKCOST*LARGESTLINKCOST*MINHOPRANKINCREASE;
			}

		} else {
			neighbors_vars.pdrWMEMA = 0.8f*neighbors_vars.pdrWMEMA + (0.2f)*(float)totalAck/totalTx;
			float etx2 = ((float)totalTx/totalAck) * ((float)totalTx/totalAck);
			rankIncrease = MINHOPRANKINCREASE*etx2;

			uint8_t parentIndex ;
			if(icmpv6rpl_getPreferredParentIndex(&parentIndex)) {
					openreport_indicatePDR(&(neighbors_vars.neighbors[parentIndex].addr_64b),totalTx,totalAck,(uint8_t)(neighbors_vars.pdrWMEMA*100));
			}
		}



		return rankIncrease;
	#endif

	#ifdef USEETX
		uint16_t  rankIncrease;

		uint16_t totalTx = schedule_getTotalTxToNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
		uint16_t totalAck = schedule_getTotalAckFromNeighbor(&(neighbors_vars.neighbors[index].addr_64b));

		if (totalAck == 0) {
			if (totalTx<=DEFAULTLINKCOST && ieee154e_getNumOfDesync() - prevDesync < DESYNCTHRESHOLD) {
				rankIncrease = 578; // (70% (3*1.42 -2)*256)
			} else {
				rankIncrease = (3*LARGESTLINKCOST-2)*MINHOPRANKINCREASE;
			}

		} else {
			neighbors_vars.pdrWMEMA = 0.8f*neighbors_vars.pdrWMEMA + (0.2f)*(float)totalAck/totalTx;
			float etx = ((float)totalTx/totalAck);
			rankIncrease = (3*etx - 2)*MINHOPRANKINCREASE;

			uint8_t parentIndex ;
			if(icmpv6rpl_getPreferredParentIndex(&parentIndex)) {
				openreport_indicatePDR(&(neighbors_vars.neighbors[parentIndex].addr_64b),totalTx,totalAck,(uint8_t)(neighbors_vars.pdrWMEMA*100));
			}
		}
		return rankIncrease;
	#endif

	#ifdef USEBROADCAST
		uint16_t  rankIncrease;

		uint8_t parentIndex ;
		if(icmpv6rpl_getPreferredParentIndex(&parentIndex) && index == parentIndex) {
			uint16_t totalTx = schedule_getTotalTxToNeighbor(&(neighbors_vars.neighbors[index].addr_64b));
			uint16_t totalAck = schedule_getTotalAckFromNeighbor(&(neighbors_vars.neighbors[index].addr_64b));

			if(totalAck == 0){
				rankIncrease = (3*LARGESTLINKCOST-2)*MINHOPRANKINCREASE;
			} else {
				neighbors_vars.pdrWMEMA = 0.8f*neighbors_vars.pdrWMEMA + (0.2f)*(float)totalAck/totalTx;
				float etx = ((float)totalTx/totalAck);
				rankIncrease = (3*etx - 2)*MINHOPRANKINCREASE;
				openreport_indicatePDR(&(neighbors_vars.neighbors[parentIndex].addr_64b),totalTx,totalAck,(uint8_t)(neighbors_vars.pdrWMEMA*100));

			}
		}else{
			uint8_t broadcastRx = neighbors_vars.neighbors[index].broadcast_rx;

			if(broadcastRx == 0) {
				rankIncrease = LARGESTLINKCOST*MINHOPRANKINCREASE;
			} else {
				uint8_t maxBroadcast = getMaxBroadcastRx();
				uint8_t minBroadcast = getMinBroadcastRx();

				if(maxBroadcast == minBroadcast) {
					rankIncrease = LARGESTLINKCOST*MINHOPRANKINCREASE;
				} else {
					float etxFromBroadcast = 1/(((float)broadcastRx - minBroadcast)/(maxBroadcast - minBroadcast));
					rankIncrease = etxFromBroadcast*MINHOPRANKINCREASE;
				}
			}
		}
		return rankIncrease;
	#endif

	return 0;
}

//===== maintenance

void  neighbors_removeOld() {
    uint8_t    i, j;
    bool       haveParent;
    PORT_TIMER_WIDTH timeSinceHeard;
    
    // remove old neighbor
    for (i=0;i<MAXNUMNEIGHBORS;i++) {
        if (neighbors_vars.neighbors[i].used==1) {
            timeSinceHeard = ieee154e_asnDiff(&neighbors_vars.neighbors[i].asn);
            if (timeSinceHeard>DESYNCTIMEOUT*2) {
                haveParent = icmpv6rpl_getPreferredParentIndex(&j);
                if (haveParent && (i==j)) { // this is our preferred parent, carefully!
                    icmpv6rpl_killPreferredParent();
                    removeNeighbor(i);
                    icmpv6rpl_updateMyDAGrankAndParentSelection();
                } else {
                    removeNeighbor(i);
                }
            }
        }
    }

    if (clearInactiveNeighborsCounter >= CLEARNEIGHBORS) {
    	prevDesync = 0;
		if (!idmanager_getIsDAGroot()) {
			//reset ack,tx counters for all other nodes execpt the parent (old information)
			haveParent = icmpv6rpl_getPreferredParentIndex(&j);
			for (i=0;i<MAXNUMNEIGHBORS;i++) {
				if (neighbors_vars.neighbors[i].used==1) {
					if (haveParent && (i!=j)){
						schedule_resetTxAck(&(neighbors_vars.neighbors[i].addr_64b));
					}
				}
			 }
		}
		clearInactiveNeighborsCounter = 0;
    } else {
    	clearInactiveNeighborsCounter++;
    }
}

void neighbors_resetBroadcastRx() {
	uint8_t    i;
	for (i=0;i<MAXNUMNEIGHBORS;i++) {
		if(neighbors_vars.neighbors[i].used == TRUE) {
			neighbors_vars.neighbors[i].broadcast_rx = 0;
		}
	}
}

//===== debug

/**
\brief Triggers this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_neighbors() {
    debugNeighborEntry_t temp;
    neighbors_vars.debugRow=(neighbors_vars.debugRow+1)%MAXNUMNEIGHBORS;
    temp.row=neighbors_vars.debugRow;
    temp.neighborEntry=neighbors_vars.neighbors[neighbors_vars.debugRow];
    openserial_printStatus(STATUS_NEIGHBORS,(uint8_t*)&temp,sizeof(debugNeighborEntry_t));
    return TRUE;
}

//=========================== private =========================================

void registerNewNeighbor(open_addr_t* address,
                         int8_t       rssi,
                         asn_t*       asnTimestamp,
                         bool         joinPrioPresent,
                         uint8_t      joinPrio,
                         bool         insecure) {
   uint8_t  i;
   // filter errors
   if (address->type!=ADDR_64B) {
      openserial_printCritical(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                            (errorparameter_t)address->type,
                            (errorparameter_t)2);
      return;
   }
   // add this neighbor
   if (isNeighbor(address)==FALSE) {
      i=0;
      while(i<MAXNUMNEIGHBORS) {
         if (neighbors_vars.neighbors[i].used==FALSE) {
            // add this neighbor
            neighbors_vars.neighbors[i].used                   = TRUE;
            neighbors_vars.neighbors[i].insecure               = insecure;
            // neighbors_vars.neighbors[i].stableNeighbor         = FALSE;
            // Note: all new neighbors are consider stable
            neighbors_vars.neighbors[i].stableNeighbor         = TRUE;
            neighbors_vars.neighbors[i].switchStabilityCounter = 0;
            memcpy(&neighbors_vars.neighbors[i].addr_64b,address,sizeof(open_addr_t));
            neighbors_vars.neighbors[i].DAGrank                = DEFAULTDAGRANK;
            // since we don't have a DAG rank at this point, no need to call for routing table update
            neighbors_vars.neighbors[i].rssi                   = rssi;
            neighbors_vars.neighbors[i].numRx                  = 1;
            neighbors_vars.neighbors[i].numTx                  = 0;
            neighbors_vars.neighbors[i].numTxACK               = 0;
            neighbors_vars.neighbors[i].generation             = 0;
            neighbors_vars.neighbors[i].broadcast_rx		   = 0;
            memcpy(&neighbors_vars.neighbors[i].asn,asnTimestamp,sizeof(asn_t));
            //update jp
            if (joinPrioPresent==TRUE){
               neighbors_vars.neighbors[i].joinPrio=joinPrio;
            }
            
            break;
         }
         i++;
      }
      if (i==MAXNUMNEIGHBORS) {
         /*openserial_printError(COMPONENT_NEIGHBORS,ERR_NEIGHBORS_FULL,
                               (errorparameter_t)MAXNUMNEIGHBORS,
                               (errorparameter_t)0);*/
         return;
      }
   }
}

bool isNeighbor(open_addr_t* neighbor) {
   uint8_t i=0;
   for (i=0;i<MAXNUMNEIGHBORS;i++) {
      if (isThisRowMatching(neighbor,i)) {
         return TRUE;
      }
   }
   return FALSE;
}

void removeNeighbor(uint8_t neighborIndex) {
   schedule_removeAllCells(0,&(neighbors_vars.neighbors[neighborIndex].addr_64b));
   neighbors_vars.neighbors[neighborIndex].used                      = FALSE;
   neighbors_vars.neighbors[neighborIndex].parentPreference          = 0;
   neighbors_vars.neighbors[neighborIndex].stableNeighbor            = FALSE;
   neighbors_vars.neighbors[neighborIndex].switchStabilityCounter    = 0;
   //neighbors_vars.neighbors[neighborIndex].addr_16b.type           = ADDR_NONE; // to save RAM
   neighbors_vars.neighbors[neighborIndex].addr_64b.type             = ADDR_NONE;
   //neighbors_vars.neighbors[neighborIndex].addr_128b.type          = ADDR_NONE; // to save RAM
   neighbors_vars.neighbors[neighborIndex].DAGrank                   = DEFAULTDAGRANK;
   neighbors_vars.neighbors[neighborIndex].rssi                      = 0;
   neighbors_vars.neighbors[neighborIndex].numRx                     = 0;
   neighbors_vars.neighbors[neighborIndex].numTx                     = 0;
   neighbors_vars.neighbors[neighborIndex].numTxACK                  = 0;
   neighbors_vars.neighbors[neighborIndex].asn.bytes0and1            = 0;
   neighbors_vars.neighbors[neighborIndex].asn.bytes2and3            = 0;
   neighbors_vars.neighbors[neighborIndex].asn.byte4                 = 0;
   neighbors_vars.neighbors[neighborIndex].f6PNORES                  = FALSE;
   neighbors_vars.neighbors[neighborIndex].sequenceNumber            = 0;
   neighbors_vars.neighbors[neighborIndex].generation   	         = 0;
   neighbors_vars.neighbors[neighborIndex].broadcast_rx   	         = 0;
}

uint8_t getMaxBroadcastRx(void) {
	uint8_t i=0;
	uint8_t maxBroadcastRx = 0;
	for (i=0;i<MAXNUMNEIGHBORS;i++) {
		if (neighbors_vars.neighbors[i].used==TRUE) {
			if(maxBroadcastRx < neighbors_vars.neighbors[i].broadcast_rx) {
				maxBroadcastRx = neighbors_vars.neighbors[i].broadcast_rx;
			}
		}
	}
	return maxBroadcastRx;
}

uint8_t getMinBroadcastRx(void) {
	uint8_t i=0;
	uint8_t minBroadcastRx = getMaxBroadcastRx();
	for (i=0;i<MAXNUMNEIGHBORS;i++) {
		if (neighbors_vars.neighbors[i].used==TRUE) {
			if(minBroadcastRx > neighbors_vars.neighbors[i].broadcast_rx) {
				minBroadcastRx = neighbors_vars.neighbors[i].broadcast_rx;
			}
		}
	}
	return minBroadcastRx;
}

//=========================== helpers =========================================

bool isThisRowMatching(open_addr_t* address, uint8_t rowNumber) {
   switch (address->type) {
      case ADDR_64B:
         return neighbors_vars.neighbors[rowNumber].used &&
                packetfunctions_sameAddress(address,&neighbors_vars.neighbors[rowNumber].addr_64b);
      default:
         openserial_printCritical(COMPONENT_NEIGHBORS,ERR_WRONG_ADDR_TYPE,
                               (errorparameter_t)address->type,
                               (errorparameter_t)3);
         return FALSE;
   }
}
