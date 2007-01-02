/** 
 * @file lllivefile.cpp
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "lllivefile.h"


LLLiveFile::LLLiveFile(const std::string &filename, const F32 refresh_period) :
mForceCheck(true),
mRefreshPeriod(refresh_period),
mFilename(filename),
mLastModTime(0),
mLastExists(false)
{
}


LLLiveFile::~LLLiveFile()
{
}


bool LLLiveFile::checkAndReload()
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
			loadFile(); // Load the file, even though it's missing to allow it to clear state.
			mLastExists = false;
			return true;
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
	
	loadFile();
	return true;
}

