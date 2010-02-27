/** 
 * @file llapp.cpp
 * @brief Implementation of the LLApp class.
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "llapp.h"

#include "llcommon.h"
#include "llapr.h"
#include "llerrorcontrol.h"
#include "llerrorthread.h"
#include "llframetimer.h"
#include "lllivefile.h"
#include "llmemory.h"
#include "llstl.h" // for DeletePointer()
#include "lleventtimer.h"

//
// Signal handling
//
// Windows uses structured exceptions, so it's handled a bit differently.
//
#if LL_WINDOWS
LONG WINAPI default_windows_exception_handler(struct _EXCEPTION_POINTERS *exception_infop);
BOOL ConsoleCtrlHandler(DWORD fdwCtrlType);
#else
# include <signal.h>
# include <unistd.h> // for fork()
void setup_signals();
void default_unix_signal_handler(int signum, siginfo_t *info, void *);
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
#endif // LL_WINDOWS

// the static application instance
LLApp* LLApp::sApplication = NULL;

// Local flag for whether or not to do logging in signal handlers.
//static
BOOL LLApp::sLogInSignal = FALSE;

// static
LLApp::EAppStatus LLApp::sStatus = LLApp::APP_STATUS_STOPPED; // Keeps track of application status
LLAppErrorHandler LLApp::sErrorHandler = NULL;
LLAppErrorHandler LLApp::sSyncErrorHandler = NULL;
BOOL LLApp::sErrorThreadRunning = FALSE;
#if !LL_WINDOWS
LLApp::child_map LLApp::sChildMap;
LLAtomicU32* LLApp::sSigChildCount = NULL;
LLAppChildCallback LLApp::sDefaultChildCallback = NULL;
#endif


LLApp::LLApp() : mThreadErrorp(NULL)
{
	commonCtor();
}

void LLApp::commonCtor()
{
	// Set our status to running
	setStatus(APP_STATUS_RUNNING);

	LLCommon::initClass();

#if !LL_WINDOWS
	// This must be initialized before the error handler.
	sSigChildCount = new LLAtomicU32(0);
#endif

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

}

LLApp::LLApp(LLErrorThread *error_thread) :
	mThreadErrorp(error_thread)
{
	commonCtor();
}


LLApp::~LLApp()
{
#if !LL_WINDOWS
	delete sSigChildCount;
	sSigChildCount = NULL;
#endif

	// reclaim live file memory
	std::for_each(mLiveFiles.begin(), mLiveFiles.end(), DeletePointer());
	mLiveFiles.clear();

	setStopped();
	// HACK: wait for the error thread to clean itself
	ms_sleep(20);
	if (mThreadErrorp)
	{
		delete mThreadErrorp;
		mThreadErrorp = NULL;
	}

	LLCommon::cleanupClass();
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
			llinfos << "Did not find option identifier while parsing token: "
				<< argv[ii] << llendl;
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
	LLEventTimer::updateClass();
	mRunner.run();
}


void LLApp::setupErrorHandling()
{
	// Error handling is done by starting up an error handling thread, which just sleeps and
	// occasionally checks to see if the app is in an error state, and sees if it needs to be run.

#if LL_WINDOWS
	// Windows doesn't have the same signal handling mechanisms as UNIX, thus APR doesn't provide
	// a signal handling thread implementation.
	// What we do is install an unhandled exception handler, which will try to do the right thing
	// in the case of an error (generate a minidump)

	// Disable this until the viewer gets ported so server crashes can be JIT debugged.
	//LPTOP_LEVEL_EXCEPTION_FILTER prev_filter;
	//prev_filter = SetUnhandledExceptionFilter(default_windows_exception_handler);

	// This sets a callback to handle w32 signals to the console window.
	// The viewer shouldn't be affected, sicne its a windowed app.
	SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ConsoleCtrlHandler, TRUE);

#else
	//
	// Start up signal handling.
	//
	// There are two different classes of signals.  Synchronous signals are delivered to a specific
	// thread, asynchronous signals can be delivered to any thread (in theory)
	//

	setup_signals();

#endif

	startErrorThread();
}

void LLApp::startErrorThread()
{
	//
	// Start the error handling thread, which is responsible for taking action
	// when the app goes into the APP_STATUS_ERROR state
	//
	if(!mThreadErrorp)
	{
		llinfos << "Starting error thread" << llendl;
		mThreadErrorp = new LLErrorThread();
		mThreadErrorp->setUserData((void *) this);
		mThreadErrorp->start();
	}
}

void LLApp::setErrorHandler(LLAppErrorHandler handler)
{
	LLApp::sErrorHandler = handler;
}


void LLApp::setSyncErrorHandler(LLAppErrorHandler handler)
{
	LLApp::sSyncErrorHandler = handler;
}

// static
void LLApp::runSyncErrorHandler()
{
	if (LLApp::sSyncErrorHandler)
	{
		LLApp::sSyncErrorHandler();
	}
}

// static
void LLApp::runErrorHandler()
{
	if (LLApp::sErrorHandler)
	{
		LLApp::sErrorHandler();
	}

	//llinfos << "App status now STOPPED" << llendl;
	LLApp::setStopped();
}


// static
void LLApp::setStatus(EAppStatus status)
{
	sStatus = status;
}


// static
void LLApp::setError()
{
	if (!isError())
	{
		// perform any needed synchronous error-handling
		runSyncErrorHandler();
		// set app status to ERROR so that the LLErrorThread notices
		setStatus(APP_STATUS_ERROR);
	}
}


// static
void LLApp::setQuitting()
{
	if (!isExiting())
	{
		// If we're already exiting, we don't want to reset our state back to quitting.
		llinfos << "Setting app state to QUITTING" << llendl;
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
	return (APP_STATUS_STOPPED == sStatus);
}


// static
bool LLApp::isRunning()
{
	return (APP_STATUS_RUNNING == sStatus);
}


// static
bool LLApp::isError()
{
	return (APP_STATUS_ERROR == sStatus);
}


// static
bool LLApp::isQuitting()
{
	return (APP_STATUS_QUITTING == sStatus);
}

bool LLApp::isExiting()
{
	return isQuitting() || isError();
}

#if !LL_WINDOWS
// static
U32 LLApp::getSigChildCount()
{
	if (sSigChildCount)
	{
		return U32(*sSigChildCount);
	}
	return 0;
}

// static
void LLApp::incSigChildCount()
{
	if (sSigChildCount)
	{
		(*sSigChildCount)++;
	}
}

#endif


// static
int LLApp::getPid()
{
#if LL_WINDOWS
	return 0;
#else
	return getpid();
#endif
}

#if LL_WINDOWS
LONG WINAPI default_windows_exception_handler(struct _EXCEPTION_POINTERS *exception_infop)
{
	// Translate the signals/exceptions into cross-platform stuff
	// Windows implementation

	// Make sure the user sees something to indicate that the app crashed.
	LONG retval;

	if (LLApp::isError())
	{
		llwarns << "Got another fatal signal while in the error handler, die now!" << llendl;
		retval = EXCEPTION_EXECUTE_HANDLER;
		return retval;
	}

	// Flag status to error, so thread_error starts its work
	LLApp::setError();

	// Block in the exception handler until the app has stopped
	// This is pretty sketchy, but appears to work just fine
	while (!LLApp::isStopped())
	{
		ms_sleep(10);
	}

	//
	// Generate a minidump if we can.
	//
	// TODO: This needs to be ported over form the viewer-specific
	// LLWinDebug class

	//
	// At this point, we always want to exit the app.  There's no graceful
	// recovery for an unhandled exception.
	// 
	// Just kill the process.
	retval = EXCEPTION_EXECUTE_HANDLER;	
	return retval;
}

// Win32 doesn't support signals. This is used instead.
BOOL ConsoleCtrlHandler(DWORD fdwCtrlType) 
{ 
	switch (fdwCtrlType) 
	{ 
 		case CTRL_BREAK_EVENT: 
		case CTRL_LOGOFF_EVENT: 
		case CTRL_SHUTDOWN_EVENT: 
		case CTRL_CLOSE_EVENT: // From end task or the window close button.
		case CTRL_C_EVENT:  // from CTRL-C on the keyboard
			// Just set our state to quitting, not error
			if (LLApp::isQuitting() || LLApp::isError())
			{
				// We're already trying to die, just ignore this signal
				if (LLApp::sLogInSignal)
				{
					llinfos << "Signal handler - Already trying to quit, ignoring signal!" << llendl;
				}
				return TRUE;
			}
			LLApp::setQuitting();
			return TRUE; 
	
		default: 
			return FALSE; 
	} 
} 

#else //!LL_WINDOWS
void LLApp::setChildCallback(pid_t pid, LLAppChildCallback callback)
{
	LLChildInfo child_info;
	child_info.mCallback = callback;
	LLApp::sChildMap[pid] = child_info;
}

void LLApp::setDefaultChildCallback(LLAppChildCallback callback)
{
	LLApp::sDefaultChildCallback = callback;
}

pid_t LLApp::fork()
{
	fflush(NULL); // flush all buffers before the child inherits them
	pid_t pid = ::fork();
	if( pid < 0 )
	{
		int system_error = errno;
		llwarns << "Unable to fork! Operating system error code: "
				<< system_error << llendl;
	}
	else if (pid == 0)
	{
		// Sleep a bit to allow the parent to set up child callbacks.
		ms_sleep(10);

		// We need to disable signal handling, because we don't have a
		// signal handling thread anymore.
		setupErrorHandling();
	}
	else
	{
		llinfos << "Forked child process " << pid << llendl;
	}
	return pid;
}

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
	sigaction(SIGABRT, &act, NULL);
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
	sigaction(SIGABRT, &act, NULL);
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

	if (LLApp::sLogInSignal)
	{
		llinfos << "Signal handler - Got signal " << signum << " - " << apr_signal_description_get(signum) << llendl;
	}


	switch (signum)
	{
	case SIGCHLD:
		if (LLApp::sLogInSignal)
		{
			llinfos << "Signal handler - Got SIGCHLD from " << info->si_pid << llendl;
		}

		// Check result code for all child procs for which we've
		// registered callbacks THIS WILL NOT WORK IF SIGCHLD IS SENT
		// w/o killing the child (Go, launcher!)
		// TODO: Now that we're using SIGACTION, we can actually
		// implement the launcher behavior to determine who sent the
		// SIGCHLD even if it doesn't result in child termination
		if (LLApp::sChildMap.count(info->si_pid))
		{
			LLApp::sChildMap[info->si_pid].mGotSigChild = TRUE;
		}
		
		LLApp::incSigChildCount();

		return;
	case SIGABRT:
		// Abort just results in termination of the app, no funky error handling.
		if (LLApp::sLogInSignal)
		{
			llwarns << "Signal handler - Got SIGABRT, terminating" << llendl;
		}
		clear_signals();
		raise(signum);
		return;
	case SIGINT:
	case SIGHUP:
	case SIGTERM:
		if (LLApp::sLogInSignal)
		{
			llwarns << "Signal handler - Got SIGINT, HUP, or TERM, exiting gracefully" << llendl;
		}
		// Graceful exit
		// Just set our state to quitting, not error
		if (LLApp::isQuitting() || LLApp::isError())
		{
			// We're already trying to die, just ignore this signal
			if (LLApp::sLogInSignal)
			{
				llinfos << "Signal handler - Already trying to quit, ignoring signal!" << llendl;
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
					llwarns << "Signal handler - Handling smackdown signal!" << llendl;
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
				llwarns << "Signal handler - Handling fatal signal!" << llendl;
			}
			if (LLApp::isError())
			{
				// Received second fatal signal while handling first, just die right now
				// Set the signal handlers back to default before handling the signal - this makes the next signal wipe out the app.
				clear_signals();
				
				if (LLApp::sLogInSignal)
				{
					llwarns << "Signal handler - Got another fatal signal while in the error handler, die now!" << llendl;
				}
				raise(signum);
				return;
			}
			
			if (LLApp::sLogInSignal)
			{
				llwarns << "Signal handler - Flagging error status and waiting for shutdown" << llendl;
			}
			// Flag status to ERROR, so thread_error does its work.
			LLApp::setError();
			// Block in the signal handler until somebody says that we're done.
			while (LLApp::sErrorThreadRunning && !LLApp::isStopped())
			{
				ms_sleep(10);
			}
			
			if (LLApp::sLogInSignal)
			{
				llwarns << "Signal handler - App is stopped, reraising signal" << llendl;
			}
			clear_signals();
			raise(signum);
			return;
		} else {
			if (LLApp::sLogInSignal)
			{
				llinfos << "Signal handler - Unhandled signal " << signum << ", ignoring!" << llendl;
			}
		}
	}
}

#endif // !WINDOWS
