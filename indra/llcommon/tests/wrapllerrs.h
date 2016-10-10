/**
 * @file   wrapllerrs.h
 * @author Nat Goodspeed
 * @date   2009-03-11
 * @brief  Define a class useful for unit tests that engage llerrs (LL_ERRS) functionality
 * 
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#if ! defined(LL_WRAPLLERRS_H)
#define LL_WRAPLLERRS_H

#if LL_WINDOWS
#pragma warning (disable : 4355) // 'this' used in initializer list: yes, intentionally
#endif

#include <tut/tut.hpp>
#include "llerrorcontrol.h"
#include "llexception.h"
#include "stringize.h"
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <string>

// statically reference the function in test.cpp... it's short, we could
// replicate, but better to reuse
extern void wouldHaveCrashed(const std::string& message);

struct WrapLLErrs
{
    WrapLLErrs():
        // Resetting Settings discards the default Recorder that writes to
        // stderr. Otherwise, expected llerrs (LL_ERRS) messages clutter the
        // console output of successful tests, potentially confusing things.
        mPriorErrorSettings(LLError::saveAndResetSettings()),
        // Save shutdown function called by LL_ERRS
        mPriorFatal(LLError::getFatalFunction())
    {
        // Make LL_ERRS call our own operator() method
        LLError::setFatalFunction(boost::bind(&WrapLLErrs::operator(), this, _1));
    }

    ~WrapLLErrs()
    {
        LLError::setFatalFunction(mPriorFatal);
        LLError::restoreSettings(mPriorErrorSettings);
    }

    struct FatalException: public LLException
    {
        FatalException(const std::string& what): LLException(what) {}
    };

    void operator()(const std::string& message)
    {
        // Save message for later in case consumer wants to sense the result directly
        error = message;
        // Also throw an appropriate exception since calling code is likely to
        // assume that control won't continue beyond LL_ERRS.
        LLTHROW(FatalException(message));
    }

    std::string error;
    LLError::SettingsStoragePtr mPriorErrorSettings;
    LLError::FatalFunction mPriorFatal;
};

/**
 * Capture log messages. This is adapted (simplified) from the one in
 * llerror_test.cpp.
 */
class CaptureLogRecorder : public LLError::Recorder, public boost::noncopyable
{
public:
    CaptureLogRecorder()
		: LLError::Recorder(),
		boost::noncopyable(),
		mMessages()
    {
    }

    virtual ~CaptureLogRecorder()
    {
    }

    virtual void recordMessage(LLError::ELevel level, const std::string& message)
    {
        mMessages.push_back(message);
    }

    /// Don't assume the message we want is necessarily the LAST log message
    /// emitted by the underlying code; search backwards through all messages
    /// for the sought string.
    std::string messageWith(const std::string& search, bool required)
    {
        for (MessageList::const_reverse_iterator rmi(mMessages.rbegin()), rmend(mMessages.rend());
             rmi != rmend; ++rmi)
        {
            if (rmi->find(search) != std::string::npos)
                return *rmi;
        }
        // failed to find any such message
        if (! required)
            return std::string();

        throw tut::failure(STRINGIZE("failed to find '" << search
                                     << "' in captured log messages:\n"
                                     << boost::ref(*this)));
    }

    std::ostream& streamto(std::ostream& out) const
    {
        MessageList::const_iterator mi(mMessages.begin()), mend(mMessages.end());
        if (mi != mend)
        {
            // handle first message separately: it doesn't get a newline
            out << *mi++;
            for ( ; mi != mend; ++mi)
            {
                // every subsequent message gets a newline
                out << '\n' << *mi;
            }
        }
        return out;
    }

private:
    typedef std::list<std::string> MessageList;
    MessageList mMessages;
};

/**
 * Capture log messages. This is adapted (simplified) from the one in
 * llerror_test.cpp.
 */
class CaptureLog : public boost::noncopyable
{
public:
    CaptureLog(LLError::ELevel level=LLError::LEVEL_DEBUG)
        // Mostly what we're trying to accomplish by saving and resetting
        // LLError::Settings is to bypass the default RecordToStderr and
        // RecordToWinDebug Recorders. As these are visible only inside
        // llerror.cpp, we can't just call LLError::removeRecorder() with
        // each. For certain tests we need to produce, capture and examine
        // DEBUG log messages -- but we don't want to spam the user's console
        // with that output. If it turns out that saveAndResetSettings() has
        // some bad effect, give up and just let the DEBUG level log messages
        // display.
		: boost::noncopyable(),
        mOldSettings(LLError::saveAndResetSettings()),
		mRecorder(new CaptureLogRecorder())
    {
        LLError::setFatalFunction(wouldHaveCrashed);
        LLError::setDefaultLevel(level);
        LLError::addRecorder(mRecorder);
    }

    ~CaptureLog()
    {
        LLError::removeRecorder(mRecorder);
        LLError::restoreSettings(mOldSettings);
    }

    /// Don't assume the message we want is necessarily the LAST log message
    /// emitted by the underlying code; search backwards through all messages
    /// for the sought string.
    std::string messageWith(const std::string& search, bool required=true)
    {
		return boost::dynamic_pointer_cast<CaptureLogRecorder>(mRecorder)->messageWith(search, required);
    }

    std::ostream& streamto(std::ostream& out) const
    {
		return boost::dynamic_pointer_cast<CaptureLogRecorder>(mRecorder)->streamto(out);
    }

private:
    LLError::SettingsStoragePtr mOldSettings;
	LLError::RecorderPtr mRecorder;
};

inline
std::ostream& operator<<(std::ostream& out, const CaptureLogRecorder& log)
{
    return log.streamto(out);
}

#endif /* ! defined(LL_WRAPLLERRS_H) */
