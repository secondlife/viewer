/** 
 * @file llcommon.h
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_COMMON_H
#define LL_COMMON_H

#include "llmemory.h"
#include "llapr.h"
// #include "llframecallbackmanager.h"
#include "lltimer.h"
#include "llworkerthread.h"
#include "llfile.h"

class LLCommon
{
public:
	static void initClass();
	static void cleanupClass();
private:
	static BOOL sAprInitialized;
};

#endif

