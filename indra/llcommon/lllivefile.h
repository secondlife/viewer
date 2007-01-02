/** 
 * @file lllivefile.h
 * @brief Automatically reloads a file whenever it changes or is removed.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLLIVEFILE_H
#define LL_LLLIVEFILE_H

#include "llframetimer.h"

class LLLiveFile
{
public:
	LLLiveFile(const std::string &filename, const F32 refresh_period = 5.f);
	virtual ~LLLiveFile();

	bool checkAndReload(); // Returns true if the file changed in any way

protected:
	virtual void loadFile() = 0; // Implement this to load your file if it changed

	bool mForceCheck;
	F32 mRefreshPeriod;
	LLFrameTimer mRefreshTimer;

	std::string mFilename;
	time_t mLastModTime;
	bool mLastExists;
};

#endif //LL_LLLIVEFILE_H
