/**
 * @file llerrorcontrol.h
 * @date   December 2006
 * @brief error message system control
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

#ifndef LL_LLERRORCONTROL_H
#define LL_LLERRORCONTROL_H

#include "llerror.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "boost/function.hpp"
#include "boost/shared_ptr.hpp"
#include <string>

class LLSD;

/*
    This is the part of the LLError namespace that manages the messages
    produced by the logging.  The logging support is defined in llerror.h.
    Most files do not need to include this.

    These implementations are in llerror.cpp.
*/

// Line buffer interface
class LLLineBuffer
{
public:
    LLLineBuffer() {};
    virtual ~LLLineBuffer() {};

    virtual void clear() = 0; // Clear the buffer, and reset it.

    virtual void addLine(const std::string& utf8line) = 0;
};


namespace LLError
{
    LL_COMMON_API void initForApplication(const std::string& user_dir, const std::string& app_dir, bool log_to_stderr = true);
        // resets all logging settings to defaults needed by applicaitons
        // logs to stderr and windows debug log
        // sets up log configuration from the file logcontrol.xml in dir


    /*
        Settings that control what is logged.
        Setting a level means log messages at that level or above.
    */

    LL_COMMON_API void setPrintLocation(bool);
    LL_COMMON_API void setDefaultLevel(LLError::ELevel);
    LL_COMMON_API ELevel getDefaultLevel();
    LL_COMMON_API void setAlwaysFlush(bool flush);
    LL_COMMON_API bool getAlwaysFlush();
    LL_COMMON_API void setEnabledLogTypesMask(U32 mask);
    LL_COMMON_API U32 getEnabledLogTypesMask();
    LL_COMMON_API void setFunctionLevel(const std::string& function_name, LLError::ELevel);
    LL_COMMON_API void setClassLevel(const std::string& class_name, LLError::ELevel);
    LL_COMMON_API void setFileLevel(const std::string& file_name, LLError::ELevel);
    LL_COMMON_API void setTagLevel(const std::string& file_name, LLError::ELevel);

    LL_COMMON_API LLError::ELevel decodeLevel(std::string name);
    LL_COMMON_API void configure(const LLSD&);
        // the LLSD can configure all of the settings
        // usually read automatically from the live errorlog.xml file


    /*
        Control functions.
    */

    typedef boost::function<void(const std::string&)> FatalFunction;

    LL_COMMON_API void setFatalFunction(const FatalFunction&);
        // The fatal function will be called after an message of LEVEL_ERROR
        // is logged.  Note: supressing a LEVEL_ERROR message from being logged
        // (by, for example, setting a class level to LEVEL_NONE), will keep
        // that message from causing the fatal function to be invoked.
        // The passed FatalFunction will be the LAST log function called
        // before LL_ERRS crashes its caller. A FatalFunction can throw an
        // exception, or call exit(), to bypass the crash. It MUST disrupt the
        // flow of control because no caller expects LL_ERRS to return.

    LL_COMMON_API FatalFunction getFatalFunction();
        // Retrieve the previously-set FatalFunction

    LL_COMMON_API std::string getFatalMessage();
        // Retrieve the message last passed to FatalFunction, if any

    /// temporarily override the FatalFunction for the duration of a
    /// particular scope, e.g. for unit tests
    class LL_COMMON_API OverrideFatalFunction
    {
    public:
        OverrideFatalFunction(const FatalFunction& func):
            mPrev(getFatalFunction())
        {
            setFatalFunction(func);
        }
        ~OverrideFatalFunction()
        {
            setFatalFunction(mPrev);
        }

    private:
        FatalFunction mPrev;
    };

    typedef std::string (*TimeFunction)();
    LL_COMMON_API std::string utcTime();

    LL_COMMON_API void setTimeFunction(TimeFunction);
        // The function is use to return the current time, formatted for
        // display by those error recorders that want the time included.



    class LL_COMMON_API Recorder
    {
        // An object that handles the actual output or error messages.
    public:
        Recorder();
        virtual ~Recorder();

        virtual void recordMessage(LLError::ELevel, const std::string& message) = 0;
            // use the level for better display, not for filtering

        virtual bool enabled() { return true; }

        bool wantsTime();
        bool wantsTags();
        bool wantsLevel();
        bool wantsLocation(); 
        bool wantsFunctionName();
        bool wantsMultiline();

        void showTime(bool show);
        void showTags(bool show);
        void showLevel(bool show);
        void showLocation(bool show); 
        void showFunctionName(bool show);
        void showMultiline(bool show);

    protected:
        bool mWantsTime;
        bool mWantsTags;
        bool mWantsLevel;
        bool mWantsLocation;
        bool mWantsFunctionName;
        bool mWantsMultiline;
    };

    typedef boost::shared_ptr<Recorder> RecorderPtr;

    /**
     * Instantiate GenericRecorder with a callable(level, message) to get
     * control on every log message without having to code an explicit
     * Recorder subclass.
     */
    template <typename CALLABLE>
    class GenericRecorder: public Recorder
    {
    public:
        GenericRecorder(const CALLABLE& callable):
            mCallable(callable)
        {}
        void recordMessage(LLError::ELevel level, const std::string& message) override
        {
            LL_PROFILE_ZONE_SCOPED
            mCallable(level, message);
        }
    private:
        CALLABLE mCallable;
    };

    /**
     * @NOTE: addRecorder() and removeRecorder() uses the boost::shared_ptr to allow for shared ownership
     * while still ensuring that the allocated memory is eventually freed
     */
    LL_COMMON_API void addRecorder(RecorderPtr);
    LL_COMMON_API void removeRecorder(RecorderPtr);
        // each error message is passed to each recorder via recordMessage()
    /**
     * Call addGenericRecorder() with a callable(level, message) to get
     * control on every log message without having to code an explicit
     * Recorder subclass. Save the returned RecorderPtr if you later want to
     * call removeRecorder().
     */
    template <typename CALLABLE>
    RecorderPtr addGenericRecorder(const CALLABLE& callable)
    {
        RecorderPtr ptr{ new GenericRecorder<CALLABLE>(callable) };
        addRecorder(ptr);
        return ptr;
    }

    LL_COMMON_API void logToFile(const std::string& filename);
    LL_COMMON_API void logToStderr();
    LL_COMMON_API void logToFixedBuffer(LLLineBuffer*);
        // Utilities to add recorders for logging to a file or a fixed buffer
        // A second call to the same function will remove the logger added
        // with the first.
        // Passing the empty string or NULL to just removes any prior.
    LL_COMMON_API std::string logFileName();
        // returns name of current logging file, empty string if none


    /*
        Utilities for use by the unit tests of LLError itself.
    */

    typedef LLPointer<LLRefCount> SettingsStoragePtr;
    LL_COMMON_API SettingsStoragePtr saveAndResetSettings();
    LL_COMMON_API void restoreSettings(SettingsStoragePtr pSettingsStorage);

    LL_COMMON_API std::string abbreviateFile(const std::string& filePath);
    LL_COMMON_API int shouldLogCallCount();
};

#endif // LL_LLERRORCONTROL_H

