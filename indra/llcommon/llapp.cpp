/**
 * @file llapp.cpp
 * @brief Implementation of the LLApp class.
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

#include "linden_common.h"

#include "llapp.h"

#include <cstdlib>

#ifdef LL_DARWIN
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>
#endif

#include "llcommon.h"
#include "llapr.h"
#include "llerrorcontrol.h"
#include "llframetimer.h"
#include "lllivefile.h"
#include "llmemory.h"
#include "llstl.h" // for DeletePointer()
#include "llstring.h"
#include "lleventtimer.h"
#include "stringize.h"
#include "llcleanup.h"
#include "llevents.h"
#include "llsdutil.h"

//
// Signal handling
#ifndef LL_WINDOWS
# include <signal.h>
# include <unistd.h> // for fork()
void setup_signals();
void default_unix_signal_handler(int signum, siginfo_t *info, void *);

#if LL_LINUX
#else
// Called by breakpad exception handler after the minidump has been generated.
bool unix_post_minidump_callback(const char *dump_dir,
                      const char *minidump_id,
                      void *context, bool succeeded);
#endif

# if LL_DARWIN
/* OSX doesn't support SIGRT* */
S32 LL_SMACKDOWN_SIGNAL = SIGUSR1;
S32 LL_HEARTBEAT_SIGNAL = SIGUSR2;
# else // linux or (assumed) other similar unixoid
/* We want reliable delivery of our signals - SIGRT* is it. */
/* Old LinuxThreads versions eat SIGRTMIN+0 to SIGRTMIN+2, avoid those. */
/* Note that SIGRTMIN/SIGRTMAX may expand to a glibc function call with a
   nonconstant result so these are not consts and cannot be used in constant-
   expressions.  SIGRTMAX may return -1 on rare broken setups. */
S32 LL_SMACKDOWN_SIGNAL = (SIGRTMAX >= 0) ? (SIGRTMAX-1) : SIGUSR1;
S32 LL_HEARTBEAT_SIGNAL = (SIGRTMAX >= 0) ? (SIGRTMAX-0) : SIGUSR2;
# endif // LL_DARWIN
#endif // !LL_WINDOWS

// the static application instance
LLApp* LLApp::sApplication = NULL;

// Allows the generation of core files for post mortem under gdb
// and disables crashlogger
bool LLApp::sDisableCrashlogger = false;

// static
// Keeps track of application status
LLScalarCond<LLApp::EAppStatus> LLApp::sStatus{LLApp::APP_STATUS_STOPPED};
LLAppErrorHandler LLApp::sErrorHandler = NULL;


LLApp::LLApp()
{
    // Set our status to running
    setStatus(APP_STATUS_RUNNING);

    LLCommon::initClass();

    // initialize the options structure. We need to make this an array
    // because the structured data will not auto-allocate if we
    // reference an invalid location with the [] operator.
    mOptions = LLSD::emptyArray();
    LLSD sd;
    for(int i = 0; i < PRIORITY_COUNT; ++i)
    {
        mOptions.append(sd);
    }

    // Make sure we clean up APR when we exit
    // Don't need to do this if we're cleaning up APR in the destructor
    //atexit(ll_cleanup_apr);

    // Set the application to this instance.
    sApplication = this;

    // initialize the buffer to write the minidump filename to
    // (this is used to avoid allocating memory in the crash handler)
    memset(mMinidumpPath, 0, MAX_MINDUMP_PATH_LENGTH);
    mCrashReportPipeStr = L"\\\\.\\pipe\\LLCrashReporterPipe";
}


LLApp::~LLApp()
{

    // reclaim live file memory
    std::for_each(mLiveFiles.begin(), mLiveFiles.end(), DeletePointer());
    mLiveFiles.clear();

    setStopped();

    SUBSYSTEM_CLEANUP_DBG(LLCommon);
}

// static
LLApp* LLApp::instance()
{
    return sApplication;
}


LLSD LLApp::getOption(const std::string& name) const
{
    LLSD rv;
    LLSD::array_const_iterator iter = mOptions.beginArray();
    LLSD::array_const_iterator end = mOptions.endArray();
    for(; iter != end; ++iter)
    {
        rv = (*iter)[name];
        if(rv.isDefined()) break;
    }
    return rv;
}

bool LLApp::parseCommandOptions(int argc, char** argv)
{
    LLSD commands;
    std::string name;
    std::string value;
    for(int ii = 1; ii < argc; ++ii)
    {
        if(argv[ii][0] != '-')
        {
            LL_INFOS() << "Did not find option identifier while parsing token: "
                << argv[ii] << LL_ENDL;
            return false;
        }
        int offset = 1;
        if(argv[ii][1] == '-') ++offset;
        name.assign(&argv[ii][offset]);
        if(((ii+1) >= argc) || (argv[ii+1][0] == '-'))
        {
            // we found another option after this one or we have
            // reached the end. simply record that this option was
            // found and continue.
            int flag = name.compare("logfile");
            if (0 == flag)
            {
                commands[name] = "log";
            }
            else
            {
                commands[name] = true;
            }

            continue;
        }
        ++ii;
        value.assign(argv[ii]);

#if LL_WINDOWS
        //Windows changed command line parsing.  Deal with it.
        size_t slen = value.length() - 1;
        size_t start = 0;
        size_t end = slen;
        if (argv[ii][start]=='"')start++;
        if (argv[ii][end]=='"')end--;
        if (start!=0 || end!=slen)
        {
            value = value.substr (start,end);
        }
#endif

        commands[name] = value;
    }
    setOptionData(PRIORITY_COMMAND_LINE, commands);
    return true;
}

bool LLApp::parseCommandOptions(int argc, wchar_t** wargv)
{
    LLSD commands;
    std::string name;
    std::string value;
    for(int ii = 1; ii < argc; ++ii)
    {
        if(wargv[ii][0] != '-')
        {
            LL_INFOS() << "Did not find option identifier while parsing token: "
                << (intptr_t)wargv[ii] << LL_ENDL;
            return false;
        }
        int offset = 1;
        if(wargv[ii][1] == '-') ++offset;

#if LL_WINDOWS
    name.assign(utf16str_to_utf8str(&wargv[ii][offset]));
#else
    name.assign(wstring_to_utf8str(&wargv[ii][offset]));
#endif
        if(((ii+1) >= argc) || (wargv[ii+1][0] == '-'))
        {
            // we found another option after this one or we have
            // reached the end. simply record that this option was
            // found and continue.
            int flag = name.compare("logfile");
            if (0 == flag)
            {
                commands[name] = "log";
            }
            else
            {
                commands[name] = true;
            }

            continue;
        }
        ++ii;

#if LL_WINDOWS
    value.assign(utf16str_to_utf8str((wargv[ii])));
#else
    value.assign(wstring_to_utf8str((wargv[ii])));
#endif

#if LL_WINDOWS
        //Windows changed command line parsing.  Deal with it.
        size_t slen = value.length() - 1;
        size_t start = 0;
        size_t end = slen;
        if (wargv[ii][start]=='"')start++;
        if (wargv[ii][end]=='"')end--;
        if (start!=0 || end!=slen)
        {
            value = value.substr (start,end);
        }
#endif

        commands[name] = value;
    }
    setOptionData(PRIORITY_COMMAND_LINE, commands);
    return true;
}

void LLApp::manageLiveFile(LLLiveFile* livefile)
{
    if(!livefile) return;
    livefile->checkAndReload();
    livefile->addToEventTimer();
    mLiveFiles.push_back(livefile);
}

bool LLApp::setOptionData(OptionPriority level, LLSD data)
{
    if((level < 0)
       || (level >= PRIORITY_COUNT)
       || (data.type() != LLSD::TypeMap))
    {
        return false;
    }
    mOptions[level] = data;
    return true;
}

LLSD LLApp::getOptionData(OptionPriority level)
{
    if((level < 0) || (level >= PRIORITY_COUNT))
    {
        return LLSD();
    }
    return mOptions[level];
}

void LLApp::stepFrame()
{
    LLFrameTimer::updateFrameTime();
    LLFrameTimer::updateFrameCount();
    LLCallbackList::instance().callFunctions();
    mRunner.run();
}

void LLApp::setupErrorHandling(bool second_instance)
{
    // Error handling is done by starting up an error handling thread, which just sleeps and
    // occasionally checks to see if the app is in an error state, and sees if it needs to be run.

#if LL_WINDOWS

#else  // ! LL_WINDOWS

#if ! defined(LL_BUGSPLAT)
    //
    // Start up signal handling.
    //
    // There are two different classes of signals.  Synchronous signals are delivered to a specific
    // thread, asynchronous signals can be delivered to any thread (in theory)
    //
    setup_signals();
#endif // ! LL_BUGSPLAT

#endif // ! LL_WINDOWS
}

void LLApp::setErrorHandler(LLAppErrorHandler handler)
{
    LLApp::sErrorHandler = handler;
}

// static
void LLApp::runErrorHandler()
{
    if (LLApp::sErrorHandler)
    {
        LLApp::sErrorHandler();
    }

    //LL_INFOS() << "App status now STOPPED" << LL_ENDL;
    LLApp::setStopped();
}

namespace
{

static std::map<LLApp::EAppStatus, const char*> statusDesc
{
    { LLApp::APP_STATUS_RUNNING,  "running" },
    { LLApp::APP_STATUS_QUITTING, "quitting" },
    { LLApp::APP_STATUS_STOPPED,  "stopped" },
    { LLApp::APP_STATUS_ERROR,    "error" }
};

} // anonymous namespace

// static
void LLApp::setStatus(EAppStatus status)
{
    auto status_it = statusDesc.find(status);
    std::string status_text = status_it != statusDesc.end() ? std::string(status_it->second) : std::to_string(status);
    LL_INFOS() << "status: " << status_text << LL_ENDL;
    // notify everyone waiting on sStatus any time its value changes
    sStatus.set_all(status);

    // This can also happen very late in the application lifecycle -- don't
    // resurrect a deleted LLSingleton
    if (! LLEventPumps::wasDeleted())
    {
        // notify interested parties of status change
        LLEventPumps::instance().obtain("LLApp").post(llsd::map("status", status_text));
    }
}


// static
void LLApp::setError()
{
    // set app status to ERROR
    setStatus(APP_STATUS_ERROR);
}

void LLApp::setDebugFileNames(const std::string &path)
{
    mStaticDebugFileName = path + "static_debug_info.log";
    mDynamicDebugFileName = path + "dynamic_debug_info.log";
}

void LLApp::writeMiniDump()
{
}

// static
void LLApp::setQuitting()
{
    if (!isExiting())
    {
        // If we're already exiting, we don't want to reset our state back to quitting.
        LL_INFOS() << "Setting app state to QUITTING" << LL_ENDL;
        setStatus(APP_STATUS_QUITTING);
    }
}


// static
void LLApp::setStopped()
{
    setStatus(APP_STATUS_STOPPED);
}


// static
bool LLApp::isStopped()
{
    return (APP_STATUS_STOPPED == sStatus.get());
}


// static
bool LLApp::isRunning()
{
    return (APP_STATUS_RUNNING == sStatus.get());
}


// static
bool LLApp::isError()
{
    return (APP_STATUS_ERROR == sStatus.get());
}


// static
bool LLApp::isQuitting()
{
    return (APP_STATUS_QUITTING == sStatus.get());
}

// static
bool LLApp::isExiting()
{
    return isQuitting() || isError();
}

void LLApp::disableCrashlogger()
{
    sDisableCrashlogger = true;
}

// static
bool LLApp::isCrashloggerDisabled()
{
    return sDisableCrashlogger;
}

// static
int LLApp::getPid()
{
#if LL_WINDOWS
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

// static
void LLApp::notifyOutOfDiskSpace()
{
    static const U32Seconds min_interval = U32Seconds(60);
    static U32Seconds min_time_to_send = U32Seconds(0);
    U32Seconds now = LLTimer::getTotalTime();
    if (now < min_time_to_send)
        return;

    min_time_to_send = now + min_interval;

    if (LLApp* app = instance())
    {
        app->sendOutOfDiskSpaceNotification();
    }
    else
    {
        LL_WARNS() << "No app instance" << LL_ENDL;
    }
}

// virtual
void LLApp::sendOutOfDiskSpaceNotification()
{
    LL_WARNS() << "Should never be called" << LL_ENDL; // Should be overridden
}

#ifndef LL_WINDOWS
void setup_signals()
{
    //
    // Set up signal handlers that may result in program termination
    //
    struct sigaction act;
    act.sa_sigaction = default_unix_signal_handler;
    sigemptyset( &act.sa_mask );
    act.sa_flags = SA_SIGINFO;

    // Synchronous signals
#   ifndef LL_BUGSPLAT
    sigaction(SIGABRT, &act, NULL);
#   endif
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGILL, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGSYS, &act, NULL);

    sigaction(LL_HEARTBEAT_SIGNAL, &act, NULL);
    sigaction(LL_SMACKDOWN_SIGNAL, &act, NULL);

    // Asynchronous signals that are normally ignored
#ifndef LL_IGNORE_SIGCHLD
    sigaction(SIGCHLD, &act, NULL);
#endif // LL_IGNORE_SIGCHLD
    sigaction(SIGUSR2, &act, NULL);

    // Asynchronous signals that result in attempted graceful exit
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    // Asynchronous signals that result in core
    sigaction(SIGQUIT, &act, NULL);

}

void clear_signals()
{
    struct sigaction act;
    act.sa_handler = SIG_DFL;
    sigemptyset( &act.sa_mask );
    act.sa_flags = SA_SIGINFO;

    // Synchronous signals
#   ifndef LL_BUGSPLAT
    sigaction(SIGABRT, &act, NULL);
#   endif
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGILL, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGSYS, &act, NULL);

    sigaction(LL_HEARTBEAT_SIGNAL, &act, NULL);
    sigaction(LL_SMACKDOWN_SIGNAL, &act, NULL);

    // Asynchronous signals that are normally ignored
#ifndef LL_IGNORE_SIGCHLD
    sigaction(SIGCHLD, &act, NULL);
#endif // LL_IGNORE_SIGCHLD

    // Asynchronous signals that result in attempted graceful exit
    sigaction(SIGHUP, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

    // Asynchronous signals that result in core
    sigaction(SIGUSR2, &act, NULL);
    sigaction(SIGQUIT, &act, NULL);
}



void default_unix_signal_handler(int signum, siginfo_t *info, void *)
{
    // Unix implementation of synchronous signal handler
    // This runs in the thread that threw the signal.
    // We do the somewhat sketchy operation of blocking in here until the error handler
    // has gracefully stopped the app.

    // FIXME(brad) - we are using this handler for asynchronous signals as well, so sLogInSignal is currently
    // disabled for safety.  we need to find a way to selectively reenable it when it is safe.
    // see issue secondlife/viewer#2566

    if (LLApp::sLogInSignal)
    {
        LL_INFOS() << "Signal handler - Got signal " << signum << " - " << apr_signal_description_get(signum) << LL_ENDL;
    }


    switch (signum)
    {
    case SIGCHLD:
    case SIGHUP:
        if (LLApp::sLogInSignal)
        {
            LL_INFOS() << "Signal handler - Got SIGCHLD or SIGHUP from " << info->si_pid << LL_ENDL;
        }

        return;
    case SIGABRT:
        // Note that this handler is not set for SIGABRT when using Bugsplat
        // Abort just results in termination of the app, no funky error handling.
        if (LLApp::sLogInSignal)
        {
            LL_WARNS() << "Signal handler - Got SIGABRT, terminating" << LL_ENDL;
        }
        clear_signals();
        raise(signum);
        return;
    case SIGINT:
    case SIGTERM:
        if (LLApp::sLogInSignal)
        {
            LL_WARNS() << "Signal handler - Got SIGINT, or TERM, exiting gracefully" << LL_ENDL;
        }
        // Graceful exit
        // Just set our state to quitting, not error
        if (LLApp::isQuitting() || LLApp::isError())
        {
            // We're already trying to die, just ignore this signal
            if (LLApp::sLogInSignal)
            {
                LL_INFOS() << "Signal handler - Already trying to quit, ignoring signal!" << LL_ENDL;
            }
            return;
        }
        LLApp::setQuitting();
        return;
    case SIGALRM:
    case SIGPIPE:
    case SIGUSR2:
    default:
        if (signum == LL_SMACKDOWN_SIGNAL ||
            signum == SIGBUS ||
            signum == SIGILL ||
            signum == SIGFPE ||
            signum == SIGSEGV ||
            signum == SIGQUIT)
        {
            if (signum == LL_SMACKDOWN_SIGNAL)
            {
                // Smackdown treated just like any other app termination, for now
                if (LLApp::sLogInSignal)
                {
                    LL_WARNS() << "Signal handler - Handling smackdown signal!" << LL_ENDL;
                }
                else
                {
                    // Don't log anything, even errors - this is because this signal could happen anywhere.
                    LLError::setDefaultLevel(LLError::LEVEL_NONE);
                }

                // Change the signal that we reraise to SIGABRT, so we generate a core dump.
                signum = SIGABRT;
            }

            if (LLApp::sLogInSignal)
            {
                LL_WARNS() << "Signal handler - Handling fatal signal!" << LL_ENDL;
            }

            if (LLApp::isError())
            {
                // Received second fatal signal while handling first, just die right now
                // Set the signal handlers back to default before handling the signal - this makes the next signal wipe out the app.
                clear_signals();

                if (LLApp::sLogInSignal)
                {
                    LL_WARNS() << "Signal handler - Got another fatal signal while in the error handler, die now!" << LL_ENDL;
                }
                raise(signum);
                return;
            }

            if (LLApp::sLogInSignal)
            {
                LL_WARNS() << "Signal handler - Flagging error status and waiting for shutdown" << LL_ENDL;
            }

            if (LLApp::isCrashloggerDisabled()) // Don't gracefully handle any signal, crash and core for a gdb post mortem
            {
                clear_signals();
                LL_WARNS() << "Fatal signal received, not handling the crash here, passing back to operating system" << LL_ENDL;
                raise(signum);
                return;
            }

            // Flag status to ERROR
            LLApp::setError();

            if (LLApp::sLogInSignal)
            {
                LL_WARNS() << "Signal handler - App is stopped, reraising signal" << LL_ENDL;
            }
            clear_signals();
            raise(signum);
            return;
        }

        if (LLApp::sLogInSignal)
        {
            LL_INFOS() << "Signal handler - Unhandled signal " << signum << ", ignoring!" << LL_ENDL;
        }
    }
}

bool unix_post_minidump_callback(const char *dump_dir,
                      const char *minidump_id,
                      void *context, bool succeeded)
{
    // Copy minidump file path into fixed buffer in the app instance to avoid
    // heap allocations in a crash handler.

    // path format: <dump_dir>/<minidump_id>.dmp
    auto dirPathLength = strlen(dump_dir);
    auto idLength = strlen(minidump_id);

    // The path must not be truncated.
    llassert((dirPathLength + idLength + 5) <= LLApp::MAX_MINDUMP_PATH_LENGTH);

    char * path = LLApp::instance()->getMiniDumpFilename();
    auto remaining = LLApp::MAX_MINDUMP_PATH_LENGTH;
    strncpy(path, dump_dir, remaining);
    remaining -= dirPathLength;
    path += dirPathLength;
    if (remaining > 0 && dirPathLength > 0 && path[-1] != '/')
    {
        *path++ = '/';
        --remaining;
    }
    if (remaining > 0)
    {
        strncpy(path, minidump_id, remaining);
        remaining -= idLength;
        path += idLength;
        strncpy(path, ".dmp", remaining);
    }

    LL_INFOS("CRASHREPORT") << "generated minidump: " << LLApp::instance()->getMiniDumpFilename() << LL_ENDL;
    LLApp::runErrorHandler();

#ifndef LL_RELEASE_FOR_DOWNLOAD
    clear_signals();
    return false;
#else
    return true;
#endif
}
#endif // !WINDOWS

