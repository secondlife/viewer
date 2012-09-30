/** 
 * @file lltracesampler.cpp
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

#include "lltracesampler.h"

namespace LLTrace
{

///////////////////////////////////////////////////////////////////////
// Sampler
///////////////////////////////////////////////////////////////////////

Sampler::Sampler(ThreadTrace* thread_trace) 
:	mElapsedSeconds(0),
	mIsStarted(false),
	mThreadTrace(thread_trace)
{
}

Sampler::~Sampler()
{
}

void Sampler::start()
{
	reset();
	resume();
}

void Sampler::reset()
{
	mF32Stats.reset();
	mS32Stats.reset();
	mStackTimers.reset();

	mElapsedSeconds = 0.0;
	mSamplingTimer.reset();
}

void Sampler::resume()
{
	if (!mIsStarted)
	{
		mSamplingTimer.reset();
		getThreadTrace()->activate(this);
		mIsStarted = true;
	}
}

void Sampler::stop()
{
	if (mIsStarted)
	{
		mElapsedSeconds += mSamplingTimer.getElapsedTimeF64();
		getThreadTrace()->deactivate(this);
		mIsStarted = false;
	}
}

ThreadTrace* Sampler::getThreadTrace()
{
	return mThreadTrace;
}

void Sampler::makePrimary()
{
	mF32Stats.makePrimary();
	mS32Stats.makePrimary();
	mStackTimers.makePrimary();
}

void Sampler::mergeFrom( const Sampler* other )
{
	mF32Stats.mergeFrom(other->mF32Stats);
	mS32Stats.mergeFrom(other->mS32Stats);
	mStackTimers.mergeFrom(other->mStackTimers);
}

}
