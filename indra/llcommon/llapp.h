/** 
 * @file llapp.h
 * @brief Declaration of the LLApp class.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLAPP_H
#define LL_LLAPP_H

#include <map>
#include "llcond.h"
#include "llrun.h"
#include "llsd.h"
#include <atomic>
#include <chrono>
// Forward declarations
class LLErrorThread;
class LLLiveFile;
#if LL_LINUX
#include <signal.h>
#endif

typedef void (*LLAppErrorHandler)();

#if !LL_WINDOWS
extern S32 LL_SMACKDOWN_SIGNAL;
extern S32 LL_HEARTBEAT_SIGNAL;

// Clear all of the signal handlers (which we want to do for the child process when we fork
void clear_signals();

#endif

class LL_COMMON_API LLApp
{
    friend class LLErrorThread;
public:
    typedef enum e_app_status
    {
        APP_STATUS_RUNNING,     // The application is currently running - the default status
        APP_STATUS_QUITTING,    // The application is currently quitting - threads should listen for this and clean up
        APP_STATUS_STOPPED,     // The application is no longer running - tells the error thread it can exit
        APP_STATUS_ERROR        // The application had a fatal error occur - tells the error thread to run
    } EAppStatus;


    LLApp();
    virtual ~LLApp();

protected:
    LLApp(LLErrorThread* error_thread);
    void commonCtor();
public:
    
    /** 
     * @brief Return the static app instance if one was created.
     */
    static LLApp* instance();

    /** @name Runtime options */
    //@{
    /** 
     * @brief Enumeration to specify option priorities in highest to
     * lowest order.
     */
    enum OptionPriority
    {
        PRIORITY_RUNTIME_OVERRIDE,
        PRIORITY_COMMAND_LINE,
        PRIORITY_SPECIFIC_CONFIGURATION,
        PRIORITY_GENERAL_CONFIGURATION,
        PRIORITY_DEFAULT,
        PRIORITY_COUNT
    };

    /**
     * @brief Get the application option at the highest priority.
     *
     * If the return value is undefined, the option does not exist.
     * @param name The name of the option.
     * @return Returns the option data.
     */
    LLSD getOption(const std::string& name) const;

    /** 
     * @brief Parse ASCII command line options and insert them into
     * application command line options.
     *
     * The name inserted into the option will have leading option
     * identifiers (a minus or double minus) stripped. All options
     * with values will be stored as a string, while all options
     * without values will be stored as true.
     * @param argc The argc passed into main().
     * @param argv The argv passed into main().
     * @return Returns true if the parse succeeded.
     */
    bool parseCommandOptions(int argc, char** argv);

    /** 
     * @brief Parse Unicode command line options and insert them into
     * application command line options.
     *
     * The name inserted into the option will have leading option
     * identifiers (a minus or double minus) stripped. All options
     * with values will be stored as a string, while all options
     * without values will be stored as true.
     * @param argc The argc passed into main().
     * @param wargv The wargv passed into main().
     * @return Returns true if the parse succeeded.
     */
    bool parseCommandOptions(int argc, wchar_t** wargv);

    /**
     * @brief Keep track of live files automatically.
     *
     * *TODO: it currently uses the <code>addToEventTimer()</code> API
     * instead of the runner. I should probalby use the runner.
     *
     * *NOTE: DO NOT add the livefile instance to any kind of check loop.
     *
     * @param livefile A valid instance of an LLLiveFile. This LLApp
     * instance will delete the livefile instance.
     */
    void manageLiveFile(LLLiveFile* livefile);

    /**
     * @brief Set the options at the specified priority.
     *
     * This function completely replaces the options at the priority
     * level with the data specified. This function will make sure
     * level and data might be valid before doing the replace.
     * @param level The priority level of the data.
     * @param data The data to set.
     * @return Returns true if the option was set.
     */
    bool setOptionData(OptionPriority level, LLSD data);

    /**
     * @brief Get the option data at the specified priority.
     *
     * This method is probably not so useful except when merging
     * information.
     * @param level The priority level of the data.
     * @return Returns The data (if any) at the level priority.
     */
    LLSD getOptionData(OptionPriority level);
    //@}



    //
    // Main application logic
    //
    virtual bool init() = 0;            // Override to do application initialization

    //
    // cleanup()
    //
    // It's currently assumed that the cleanup() method will only get
    // called from the main thread or the error handling thread, as it will
    // likely do thread shutdown, among other things.
    //
    virtual bool cleanup() = 0;         // Override to do application cleanup

    //
    // frame()
    //
    // Pass control to the application for a single frame. Returns 'done'
    // flag: if frame() returns false, it expects to be called again.
    //
    virtual bool frame() = 0; // Override for application body logic

    //
    // Crash logging
    //
    void disableCrashlogger();              // Let the OS handle the crashes
    static bool isCrashloggerDisabled();    // Get the here above set value

    //
    // Application status
    //
    static void setQuitting();  // Set status to QUITTING, the app is now shutting down
    static void setStopped();   // Set status to STOPPED, the app is done running and should exit
    static void setError();     // Set status to ERROR, the error handler should run
    static bool isStopped();
    static bool isRunning();
    static bool isQuitting();
    static bool isError();
    static bool isExiting(); // Either quitting or error (app is exiting, cleanly or not)
    static int getPid();

    //
    // Sleep for specified time while still running
    //
    // For use by a coroutine or thread that performs some maintenance on a
    // periodic basis. (See also LLEventTimer.) This method supports the
    // pattern of an "infinite" loop that sleeps for some time, performs some
    // action, then sleeps again. The trouble with literally sleeping a worker
    // thread is that it could potentially sleep right through attempted
    // application shutdown. This method avoids that by returning false as
    // soon as the application status changes away from APP_STATUS_RUNNING
    // (isRunning()).
    //
    // sleep() returns true if it sleeps undisturbed for the entire specified
    // duration. The idea is that you can code 'while sleep(duration) ...',
    // which will break the loop once shutdown begins.
    //
    // Since any time-based LLUnit should be implicitly convertible to
    // F32Milliseconds, accept that specific type as a proxy.
    static bool sleep(F32Milliseconds duration);
    // Allow any duration defined in terms of <chrono>.
    // One can imagine a wonderfully general bidirectional conversion system
    // between any type derived from LLUnits::LLUnit<T, LLUnits::Seconds> and
    // any std::chrono::duration -- but that doesn't yet exist.
    template <typename Rep, typename Period>
    bool sleep(const std::chrono::duration<Rep, Period>& duration)
    {
        // wait_for_unequal() has the opposite bool return convention
        return ! sStatus.wait_for_unequal(duration, APP_STATUS_RUNNING);
    }

    /** @name Error handling methods */
    //@{
    /**
     * @brief Do our generic platform-specific error-handling setup --
     * signals on unix, structured exceptions on windows.
     * 
     * DO call this method if your app will either spawn children or be
     * spawned by a launcher.
     * Call just after app object construction.
     * (Otherwise your app will crash when getting signals,
     * and will not core dump.)
     *
     * DO NOT call this method if your application has specialized
     * error handling code.
     */
    void setupErrorHandling(bool mSecondInstance=false);

    void setErrorHandler(LLAppErrorHandler handler);
    static void runErrorHandler(); // run shortly after we detect an error, ran in the relatively robust context of the LLErrorThread - preferred.
    //@}
    
    // the maximum length of the minidump filename returned by getMiniDumpFilename()
    static const U32 MAX_MINDUMP_PATH_LENGTH = 256;

    // change the directory where Breakpad minidump files are written to
    void setDebugFileNames(const std::string &path);

    // Return the Google Breakpad minidump filename after a crash.
    char *getMiniDumpFilename() { return mMinidumpPath; }
    std::string* getStaticDebugFile() { return &mStaticDebugFileName; }
    std::string* getDynamicDebugFile() { return &mDynamicDebugFileName; }

    // Write out a Google Breakpad minidump file.
    void writeMiniDump();


    /**
      * @brief Get a reference to the application runner
      *
      * Please use the runner with caution. Since the Runner usage
      * pattern is not yet clear, this method just gives access to it
      * to add and remove runnables.
      * @return Returns the application runner. Do not save the
      * pointer past the caller's stack frame.
      */
    LLRunner& getRunner() { return mRunner; }

#ifdef LL_WINDOWS
    virtual void reportCrashToBugsplat(void* pExcepInfo /*EXCEPTION_POINTERS*/) { }
#endif

public:
    typedef std::map<std::string, std::string> string_map;
    string_map mOptionMap;  // Contains all command-line options and arguments in a map

protected:

    static void setStatus(EAppStatus status);       // Use this to change the application status.
    static LLScalarCond<EAppStatus> sStatus; // Reflects current application status
    static BOOL sErrorThreadRunning; // Set while the error thread is running
    static BOOL sDisableCrashlogger; // Let the OS handle crashes for us.
    std::wstring mCrashReportPipeStr;  //Name of pipe to use for crash reporting.

    std::string mDumpPath;  //output path for google breakpad.  Dependency workaround.

    /**
      * @brief This method is called once a frame to do once a frame tasks.
      */
    void stepFrame();

private:
    void startErrorThread();
    
    // Contains the filename of the minidump file after a crash.
    char mMinidumpPath[MAX_MINDUMP_PATH_LENGTH];
    
    std::string mStaticDebugFileName;
    std::string mDynamicDebugFileName;

    // *NOTE: On Windows, we need a routine to reset the structured
    // exception handler when some evil driver has taken it over for
    // their own purposes
    typedef int(*signal_handler_func)(int signum);
    static LLAppErrorHandler sErrorHandler;

    // Default application threads
    LLErrorThread* mThreadErrorp;       // Waits for app to go to status ERROR, then runs the error callback

    // This is the application level runnable scheduler.
    LLRunner mRunner;

    /** @name Runtime option implementation */
    //@{

    // The application options.
    LLSD mOptions;

    // The live files for this application
    std::vector<LLLiveFile*> mLiveFiles;
    //@}

private:
    // the static application instance if it was created.
    static LLApp* sApplication;

#if !LL_WINDOWS
    friend void default_unix_signal_handler(int signum, siginfo_t *info, void *);
#endif

public:
    static BOOL sLogInSignal;
};

#endif // LL_LLAPP_H
