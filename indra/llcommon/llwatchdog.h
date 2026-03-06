/**
 * @file llthreadwatchdog.h
 * @brief The LLThreadWatchdog class declaration
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_LLTHREADWATCHDOG_H
#define LL_LLTHREADWATCHDOG_H

#ifndef LL_TIMER_H
    #include "lltimer.h"
#endif
#include "llmutex.h"
#include "llsingleton.h"

#include <functional>

// LLWatchdogEntry is the interface used by the tasks that
// need to be watched.
class LLWatchdogEntry
{
public:
    LLWatchdogEntry(const std::string &thread_name);
    virtual ~LLWatchdogEntry();

    // isAlive is accessed by the watchdog thread.
    // This may mean that resources used by
    // isAlive and other method may need synchronization.
    virtual bool isAlive() const = 0;
    virtual void reset() = 0;
    virtual void start();
    virtual void stop();
    virtual std::string getLastState() const { return std::string(); }
    typedef std::thread::id id_t;
    std::string getThreadName() const;

private:
    id_t mThreadID; // ID of the thread being watched
    std::string mThreadName;
};

class LLWatchdogTimeout : public LLWatchdogEntry
{
public:
    LLWatchdogTimeout(const std::string& thread_name);
    virtual ~LLWatchdogTimeout();

    bool isAlive() const override;
    void reset() override;
    void start() override { start(""); }
    void stop() override;

    void start(std::string_view state);
    void setTimeout(F32 d);
    void ping(std::string_view state);
    const std::string& getState() {return mPingState; }
    std::string getLastState() const override { return mPingState; }

private:
    LLTimer mTimer;
    F32 mTimeout;
    std::string mPingState;
};

class LLWatchdogTimerThread; // Defined in the cpp
class LLWatchdog : public LLSimpleton<LLWatchdog>
{
public:
    LLWatchdog();
    ~LLWatchdog();

    // Add an entry to the watchdog.
    void add(LLWatchdogEntry* e);
    void remove(LLWatchdogEntry* e);

    typedef std::function<void(bool)> create_marker_func_t;
    typedef std::function<void()> clear_marker_func_t;
    typedef std::function<bool(std::string&)> report_func_t;
    typedef std::function<void()> notify_func_t;
    void init(
        create_marker_func_t error_state_callback,
        clear_marker_func_t clear_marker_callback,
        report_func_t report_callback,
        notify_func_t notify_callback,
        bool crash_on_freeze);
    void run();
    void cleanup();


private:
    void lockThread();
    void unlockThread();

    typedef std::set<LLWatchdogEntry*> SuspectsRegistry;
    SuspectsRegistry mSuspects;
    SuspectsRegistry mFrozeList;
    LLMutex* mSuspectsAccessMutex;
    LLWatchdogTimerThread* mTimer;
    U64 mLastClockCount;
    bool mCrashOnFreeze;

    // At the moment watchdog expects app to set markers in mCreateMarkerFnc,
    // but technically can be used to set any error states or do some cleanup
    // or show warnings.
    create_marker_func_t mCreateMarkerFnc;
    clear_marker_func_t mClearMarkerFnc;
    report_func_t mCrashReportFnc;
    notify_func_t mNotifyFnc;
};

#endif // LL_LLTHREADWATCHDOG_H
