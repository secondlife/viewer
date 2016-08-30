/** 
 * @file llcommon.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llcommon.h"

#include "llmemory.h"
#include "llthread.h"
#include "lltrace.h"
#include "lltracethreadrecorder.h"
#include "llcleanup.h"

//static
BOOL LLCommon::sAprInitialized = FALSE;

static LLTrace::ThreadRecorder* sMasterThreadRecorder = NULL;

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
	LLThreadSafeRefCount::initThreadSafeRefCount();
	assert_main_thread();		// Make sure we record the main thread
	if (!sMasterThreadRecorder)
	{
		sMasterThreadRecorder = new LLTrace::ThreadRecorder();
		LLTrace::set_master_thread_recorder(sMasterThreadRecorder);
	}
}

//static
void LLCommon::cleanupClass()
{
	delete sMasterThreadRecorder;
	sMasterThreadRecorder = NULL;
	LLTrace::set_master_thread_recorder(NULL);
	LLThreadSafeRefCount::cleanupThreadSafeRefCount();
	SUBSYSTEM_CLEANUP(LLTimer);
	if (sAprInitialized)
	{
		ll_cleanup_apr();
		sAprInitialized = FALSE;
	}
	SUBSYSTEM_CLEANUP(LLMemory);
}
