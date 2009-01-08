/** 
 * @file llthreadwatchdog.h
 * @brief The LLThreadWatchdog class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#ifndef LL_LLTHREADWATCHDOG_H
#define LL_LLTHREADWATCHDOG_H

#include <boost/function.hpp>

#ifndef LL_TIMER_H					
	#include "lltimer.h"
#endif

// LLWatchdogEntry is the interface used by the tasks that 
// need to be watched.
class LLWatchdogEntry
{
public:
	LLWatchdogEntry();
	virtual ~LLWatchdogEntry();

	// isAlive is accessed by the watchdog thread.
	// This may mean that resources used by 
	// isAlive and other method may need synchronization.
	virtual bool isAlive() const = 0;
	virtual void reset() = 0;
	virtual void start();
	virtual void stop();
};

class LLWatchdogTimeout : public LLWatchdogEntry
{
public:
	LLWatchdogTimeout();
	virtual ~LLWatchdogTimeout();

	/* virtual */ bool isAlive() const;
	/* virtual */ void reset();
	/* virtual */ void start(const std::string& state); 
	/* virtual */ void stop();

	void setTimeout(F32 d);
	void ping(const std::string& state);
	const std::string& getState() {return mPingState; }

private:
	LLTimer mTimer;
	F32 mTimeout;
	std::string mPingState;
};

class LLWatchdogTimerThread; // Defined in the cpp
class LLWatchdog : public LLSingleton<LLWatchdog>
{
public:
	LLWatchdog();
	~LLWatchdog();

	// Add an entry to the watchdog.
	void add(LLWatchdogEntry* e);
	void remove(LLWatchdogEntry* e);

	typedef boost::function<void (void)> killer_event_callback;

	void init(killer_event_callback func = NULL);
	void run();
	void cleanup();
    
private:
	void lockThread();
	void unlockThread();

	typedef std::set<LLWatchdogEntry*> SuspectsRegistry;
	SuspectsRegistry mSuspects;
	LLMutex* mSuspectsAccessMutex;
	LLWatchdogTimerThread* mTimer;
	U64 mLastClockCount;

	killer_event_callback mKillerCallback;
};

#endif // LL_LLTHREADWATCHDOG_H
