/** 
 * @file lllivefile.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "lllivefile.h"
#include "llframetimer.h"
#include "lleventtimer.h"

const F32 DEFAULT_CONFIG_FILE_REFRESH = 5.0f;


class LLLiveFile::Impl
{
public:
	Impl(const std::string& filename, const F32 refresh_period);
	~Impl();
	
	bool check();
	void changed();
	
	bool mForceCheck;
	F32 mRefreshPeriod;
	LLFrameTimer mRefreshTimer;

	std::string mFilename;
	time_t mLastModTime;
	time_t mLastStatTime;
	bool mLastExists;
	
	LLEventTimer* mEventTimer;
};

LLLiveFile::Impl::Impl(const std::string& filename, const F32 refresh_period)
	:
	mForceCheck(true),
	mRefreshPeriod(refresh_period),
	mFilename(filename),
	mLastModTime(0),
	mLastStatTime(0),
	mLastExists(false),
	mEventTimer(NULL)
{
}

LLLiveFile::Impl::~Impl()
{
	delete mEventTimer;
}

LLLiveFile::LLLiveFile(const std::string& filename, const F32 refresh_period)
	: impl(* new Impl(filename, refresh_period))
{
}

LLLiveFile::~LLLiveFile()
{
	delete &impl;
}


bool LLLiveFile::Impl::check()
{
	if (!mForceCheck && mRefreshTimer.getElapsedTimeF32() < mRefreshPeriod)
	{
		// Skip the check if not enough time has elapsed and we're not
		// forcing a check of the file
		return false;
	}
	mForceCheck = false;
	mRefreshTimer.reset();

	// Stat the file to see if it exists and when it was last modified.
	llstat stat_data;
	int res = LLFile::stat(mFilename, &stat_data);

	if (res)
	{
		// Couldn't stat the file, that means it doesn't exist or is
		// broken somehow.  Clear flags and return.
		if (mLastExists)
		{
			mLastExists = false;
			return true;	// no longer existing is a change!
		}
		return false;
	}

	// The file exists, decide if we want to load it.
	if (mLastExists)
	{
		// The file existed last time, don't read it if it hasn't changed since
		// last time.
		if (stat_data.st_mtime <= mLastModTime)
		{
			return false;
		}
	}

	// We want to read the file.  Update status info for the file.
	mLastExists = true;
	mLastStatTime = stat_data.st_mtime;
	return true;
}

void LLLiveFile::Impl::changed()
{
	// we wanted to read this file, and we were successful.
	mLastModTime = mLastStatTime;
}

bool LLLiveFile::checkAndReload()
{
	bool changed = impl.check();
	if (changed)
	{
		if(loadFile())
		{
			impl.changed();
			this->changed();
		}
		else
		{
			changed = false;
		}
	}
	return changed;
}

std::string LLLiveFile::filename() const
{
	return impl.mFilename;
}

namespace
{
	class LiveFileEventTimer : public LLEventTimer
	{
	public:
		LiveFileEventTimer(LLLiveFile& f, F32 refresh)
			: LLEventTimer(refresh), mLiveFile(f)
			{ }
			
		BOOL tick()
		{ 
			mLiveFile.checkAndReload(); 
			return FALSE;
		}
	
	private:
		LLLiveFile& mLiveFile;
	};
	
}

void LLLiveFile::addToEventTimer()
{
	impl.mEventTimer = new LiveFileEventTimer(*this, impl.mRefreshPeriod);
}

void LLLiveFile::setRefreshPeriod(F32 seconds)
{
	if (seconds < 0.f)
	{
		seconds = -seconds;
	}
	impl.mRefreshPeriod = seconds;
}

