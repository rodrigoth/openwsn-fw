#ifndef __UINJECT_H
#define __UINJECT_H

/**
\addtogroup AppUdp
\{
\addtogroup uinject
\{
*/

#include "opentimers.h"

//=========================== define ==========================================

#define UINJECT_PERIOD_MS 1000

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t              period;  ///< uinject packet sending period>
   uint32_t             seqnum;  //uniquely identifies this packet
} uinject_vars_t;

BEGIN_PACK
typedef struct {
	open_addr_t node;
	open_addr_t destination;
	asn_t asn;
	asn_t asn_in;
	uint8_t track;
	uint8_t is_sent;
} debug_reportEntryUinject_t;
END_PACK


//=========================== prototypes ======================================

void uinject_init(void);
void uinject_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void uinject_receive(OpenQueueEntry_t* msg);
/**
\}
\}
*/

#endif
