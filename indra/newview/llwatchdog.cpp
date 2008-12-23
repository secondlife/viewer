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

const U32 WATCHDOG_SLEEP_TIME_USEC = 1000000;

void default_killer_callback()
{
#ifdef LL_WINDOWS
	RaiseException(0,0,0,0);
#else
	raise(SIGQUIT);
#endif
}

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
		mSleepMsecs = 1;
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
	stop();
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
const std::string UNINIT_STRING = "uninitialized";

LLWatchdogTimeout::LLWatchdogTimeout() : 
	mTimeout(0.0f),
	mPingState(UNINIT_STRING)
{
}

LLWatchdogTimeout::~LLWatchdogTimeout() 
{
}

bool LLWatchdogTimeout::isAlive() const 
{ 
	return (mTimer.getStarted() && !mTimer.hasExpired()); 
}

void LLWatchdogTimeout::reset()
{
	mTimer.setTimerExpirySec(mTimeout); 
}

void LLWatchdogTimeout::setTimeout(F32 d) 
{
	mTimeout = d;
}

void LLWatchdogTimeout::start(const std::string& state) 
{
	// Order of operation is very impmortant here.
	// After LLWatchdogEntry::start() is called
	// LLWatchdogTimeout::isAlive() will be called asynchronously. 
	mTimer.start(); 
	ping(state);
	LLWatchdogEntry::start();
}

void LLWatchdogTimeout::stop() 
{
	LLWatchdogEntry::stop();
	mTimer.stop();
}

void LLWatchdogTimeout::ping(const std::string& state) 
{ 
	if(!state.empty())
	{
		mPingState = state;
	}
	reset();
}

// LLWatchdog
LLWatchdog::LLWatchdog() :
	mSuspectsAccessMutex(NULL),
	mTimer(NULL),
	mLastClockCount(0),
	mKillerCallback(&default_killer_callback)
{
}

LLWatchdog::~LLWatchdog()
{
}

void LLWatchdog::add(LLWatchdogEntry* e)
{
	lockThread();
	mSuspects.insert(e);
	unlockThread();
}

void LLWatchdog::remove(LLWatchdogEntry* e)
{
	lockThread();
    mSuspects.erase(e);
	unlockThread();
}

void LLWatchdog::init(killer_event_callback func)
{
	mKillerCallback = func;
	if(!mSuspectsAccessMutex && !mTimer)
	{
		mSuspectsAccessMutex = new LLMutex(NULL);
		mTimer = new LLWatchdogTimerThread();
		mTimer->setSleepTime(WATCHDOG_SLEEP_TIME_USEC / 1000);
		mLastClockCount = LLTimer::getTotalTime();

		// mTimer->start() kicks off the thread, any code after
		// start needs to use the mSuspectsAccessMutex
		mTimer->start();
	}
}

void LLWatchdog::cleanup()
{
	if(mTimer)
	{
		mTimer->stop();
		delete mTimer;
		mTimer = NULL;
	}

	if(mSuspectsAccessMutex)
	{
		delete mSuspectsAccessMutex;
		mSuspectsAccessMutex = NULL;
	}

	mLastClockCount = 0;
}

void LLWatchdog::run()
{
	lockThread();

	// Check the time since the last call to run...
	// If the time elapsed is two times greater than the regualr sleep time
	// reset the active timeouts.
	const U32 TIME_ELAPSED_MULTIPLIER = 2;
	U64 current_time = LLTimer::getTotalTime();
	U64 current_run_delta = current_time - mLastClockCount;
	mLastClockCount = current_time;
	
	if(current_run_delta > (WATCHDOG_SLEEP_TIME_USEC * TIME_ELAPSED_MULTIPLIER))
	{
		llinfos << "Watchdog thread delayed: resetting entries." << llendl;
		std::for_each(mSuspects.begin(), 
			mSuspects.end(), 
			std::mem_fun(&LLWatchdogEntry::reset)
			);
	}
	else
	{
		SuspectsRegistry::iterator result = 
			std::find_if(mSuspects.begin(), 
				mSuspects.end(), 
				std::not1(std::mem_fun(&LLWatchdogEntry::isAlive))
				);

		if(result != mSuspects.end())
		{
			// error!!!
			if(mTimer)
			{
				mTimer->stop();
			}

			llinfos << "Watchdog detected error:" << llendl;
			mKillerCallback();
		}
	}


	unlockThread();
}

void LLWatchdog::lockThread()
{
	if(mSuspectsAccessMutex != NULL)
	{
		mSuspectsAccessMutex->lock();
	}
}

void LLWatchdog::unlockThread()
{
	if(mSuspectsAccessMutex != NULL)
	{
		mSuspectsAccessMutex->unlock();
	}
}
