/** 
 * @file lllivefile.cpp
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lllivefile.h"
#include "llframetimer.h"
#include "lltimer.h"

class LLLiveFile::Impl
{
public:
	Impl(const std::string &filename, const F32 refresh_period);
	~Impl();
	
	bool check();
	
	
	bool mForceCheck;
	F32 mRefreshPeriod;
	LLFrameTimer mRefreshTimer;

	std::string mFilename;
	time_t mLastModTime;
	bool mLastExists;
	
	LLEventTimer* mEventTimer;
};

LLLiveFile::Impl::Impl(const std::string &filename, const F32 refresh_period)
	: mForceCheck(true),
	mRefreshPeriod(refresh_period),
	mFilename(filename),
	mLastModTime(0),
	mLastExists(false),
	mEventTimer(NULL)
{
}

LLLiveFile::Impl::~Impl()
{
	delete mEventTimer;
}

LLLiveFile::LLLiveFile(const std::string &filename, const F32 refresh_period)
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
	int res = LLFile::stat(mFilename.c_str(), &stat_data);

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
	mLastModTime = stat_data.st_mtime;
	
	return true;
}

bool LLLiveFile::checkAndReload()
{
	bool changed = impl.check();
	if (changed)
	{
		loadFile();
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
			
		void tick()
			{ mLiveFile.checkAndReload(); }
	
	private:
		LLLiveFile& mLiveFile;
	};
	
}

void LLLiveFile::addToEventTimer()
{
	impl.mEventTimer = new LiveFileEventTimer(*this, impl.mRefreshPeriod);
}

