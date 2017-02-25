/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2017-02-23 11:25:38.725566.
*/
#ifndef __USERIALBRIDGE_H
#define __USERIALBRIDGE_H

/**
\addtogroup AppUdp
\{
\addtogroup userialbridge
\{
*/

#include "Python.h"

#include "opentimers_obj.h"
#include "openserial_obj.h"

//=========================== define ==========================================

//=========================== typedef =========================================

#define USERIALBRIDGE_MAXPAYLEN 32

//=========================== variables =======================================

typedef struct {
   uint8_t              txbuf[USERIALBRIDGE_MAXPAYLEN];
   uint8_t              txbufLen;
   openserial_rsvpt     openserial_rsvp;
} userialbridge_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void userialbridge_init(OpenMote* self);
void userialbridge_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
void userialbridge_receive(OpenMote* self, OpenQueueEntry_t* msg);
void userialbridge_triggerData(OpenMote* self);
/**
\}
\}
*/

#endif
