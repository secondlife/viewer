/** 
 * @file llcommon.cpp
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llcommon.h"
#include "llthread.h"

//static
BOOL LLCommon::sAprInitialized = FALSE;

//static
void LLCommon::initClass()
{
	LLMemory::initClass();
	if (!sAprInitialized)
	{
		ll_init_apr();
		sAprInitialized = TRUE;
	}
	LLTimer::initClass();
	LLThreadSafeRefCount::initClass();
// 	LLWorkerThread::initClass();
// 	LLFrameCallbackManager::initClass();
}

//static
void LLCommon::cleanupClass()
{
// 	LLFrameCallbackManager::cleanupClass();
// 	LLWorkerThread::cleanupClass();
	LLThreadSafeRefCount::cleanupClass();
	LLTimer::cleanupClass();
	if (sAprInitialized)
	{
		ll_cleanup_apr();
		sAprInitialized = FALSE;
	}
	LLMemory::cleanupClass();
}
