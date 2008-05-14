/** 
 * @file llthreadwatchdog.cpp
 * @brief The LLThreadWatchdog class definitions
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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


#include "llviewerprecompiledheaders.h"
#include "llwatchdog.h"

// This class runs the watchdog timing thread.
class LLWatchdogTimerThread : public LLThread
{
public:
	LLWatchdogTimerThread() : 
		LLThread("Watchdog"),
		mSleepMsecs(0),
		mStopping(false)
	{
	}
            
	~LLWatchdogTimerThread() {}
    
	void setSleepTime(long ms) { mSleepMsecs = ms; }
	void stop() 
	{
		mStopping = true; 
		ms_sleep(1);
	}
    
	/* virtual */ void run()
	{
		while(!mStopping)
		{
			LLWatchdog::getInstance()->run();
			ms_sleep(mSleepMsecs);
		}
	}

private:
	long mSleepMsecs;
	bool mStopping;
};

// LLWatchdogEntry
LLWatchdogEntry::LLWatchdogEntry()
{
}

LLWatchdogEntry::~LLWatchdogEntry()
{
}

void LLWatchdogEntry::start()
{
	LLWatchdog::getInstance()->add(this);
}

void LLWatchdogEntry::stop()
{
	LLWatchdog::getInstance()->remove(this);
}

// LLWatchdogTimeout
LLWatchdogTimeout::LLWatchdogTimeout() : 
	mTimeout(0.0f) 
{
}

LLWatchdogTimeout::~LLWatchdogTimeout() 
{
}

bool LLWatchdogTimeout::isAlive() const 
{ 
	return (mTimer.getStarted() && !mTimer.hasExpired()); 
}

void LLWatchdogTimeout::setTimeout(F32 d) 
{
	mTimeout = d;
}

void LLWatchdogTimeout::start() 
{
	// Order of operation is very impmortant here.
	// After LLWatchdogEntry::start() is called
	// LLWatchdogTimeout::isAlive() will be called asynchronously. 
	mTimer.start(); 
	mTimer.setTimerExpirySec(mTimeout); 
	LLWatchdogEntry::start();
}
void LLWatchdogTimeout::stop() 
{
	LLWatchdogEntry::stop();
	mTimer.stop();
}

void LLWatchdogTimeout::ping() 
{ 
	mTimer.setTimerExpirySec(mTimeout); 
}

// LlWatchdog
LLWatchdog::LLWatchdog() :
	mSuspectsAccessMutex(NULL),
	mTimer(NULL)
{
}

LLWatchdog::~LLWatchdog()
{
}

void LLWatchdog::add(LLWatchdogEntry* e)
{
	mSuspectsAccessMutex->lock();
	mSuspects.insert(e);
	mSuspectsAccessMutex->unlock();
}

void LLWatchdog::remove(LLWatchdogEntry* e)
{
	mSuspectsAccessMutex->lock();
    mSuspects.erase(e);
	mSuspectsAccessMutex->unlock();
}

void LLWatchdog::init()
{
	mSuspectsAccessMutex = new LLMutex(NULL);
	mTimer = new LLWatchdogTimerThread();
	mTimer->setSleepTime(1000);
	mTimer->start();
}

void LLWatchdog::cleanup()
{
	mTimer->stop();
	delete mTimer;
	delete mSuspectsAccessMutex;
}

void LLWatchdog::run()
{
	mSuspectsAccessMutex->lock();
	
	SuspectsRegistry::iterator result = 
		std::find_if(mSuspects.begin(), 
			mSuspects.end(), 
			std::not1(std::mem_fun(&LLWatchdogEntry::isAlive))
			);

	if(result != mSuspects.end())
	{
		// error!!!
		mTimer->stop();

		llinfos << "Watchdog detected error:" << llendl;
#ifdef LL_WINDOWS
		RaiseException(0,0,0,0);
#else
		raise(SIGQUIT);
#endif
	}

	mSuspectsAccessMutex->unlock();
}
