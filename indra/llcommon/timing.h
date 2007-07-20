/** 
 * @file timing.h
 * @brief Cross-platform routines for doing timing.
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_TIMING_H					
#define LL_TIMING_H

#include <time.h>

#if LL_LINUX || LL_DARWIN || LL_SOLARIS
#		include <sys/time.h>
#endif


const F32 SEC_TO_MICROSEC = 1000000.f;
const U64 SEC_TO_MICROSEC_U64 = 1000000;
const U32 SEC_PER_DAY = 86400;

// This is just a stub, implementation in lltimer.cpp.  This file will be deprecated in the future.
U64 totalTime();					// Returns current system time in microseconds

#endif
