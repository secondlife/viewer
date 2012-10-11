/** 
 * @file lltrace.cpp
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "lltrace.h"
#include "lltracerecording.h"
#include "lltracethreadrecorder.h"

namespace LLTrace
{

static MasterThreadRecorder* gMasterThreadRecorder = NULL;

void init()
{
	gMasterThreadRecorder = new MasterThreadRecorder();
}

void cleanup()
{
	delete gMasterThreadRecorder;
	gMasterThreadRecorder = NULL;
}

MasterThreadRecorder& getMasterThreadRecorder()
{
	llassert(gMasterThreadRecorder != NULL);
	return *gMasterThreadRecorder;
}

LLThreadLocalPointer<ThreadRecorder>& get_thread_recorder()
{
	static LLThreadLocalPointer<ThreadRecorder> s_thread_recorder;
	return s_thread_recorder;

}

BlockTimer::Recorder::StackEntry BlockTimer::sCurRecorder;

}
