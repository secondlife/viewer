/** 
 * @file llapp.h
 * @brief Declaration of the LLApp class.
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

#ifndef LL_LLAPP_H
#define LL_LLAPP_H

#include <map>
#include "llrun.h"
#include "llsd.h"
#include "lloptioninterface.h"

// Forward declarations
template <typename Type> class LLAtomic32;
typedef LLAtomic32<U32> LLAtomicU32;
class LLErrorThread;
class LLLiveFile;
#if LL_LINUX
typedef struct siginfo siginfo_t;
#endif

typedef void (*LLAppErrorHandler)();
typedef void (*LLAppChildCallback)(int pid, bool exited, int status);

#if !LL_WINDOWS
extern S32 LL_SMACKDOWN_SIGNAL;
extern S32 LL_HEARTBEAT_SIGNAL;

// Clear all of the signal handlers (which we want to do for the child process when we fork
void clear_signals();

class LLChildInfo
{
public:
	LLChildInfo() : mGotSigChild(FALSE), mCallback(NULL) {}
	BOOL mGotSigChild;
	LLAppChildCallback mCallback;
};
#endif

class LL_COMMON_API LLApp : public LLOptionInterface
{
	friend class LLErrorThread;
public:
	typedef enum e_app_status
	{
		APP_STATUS_RUNNING,		// The application is currently running - the default status
		APP_STATUS_QUITTING,	// The application is currently quitting - threads should listen for this and clean up
		APP_STATUS_STOPPED,		// The application is no longer running - tells the error thread it can exit
		APP_STATUS_ERROR		// The application had a fatal error occur - tells the error thread to run
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
	virtual LLSD getOption(const std::string& name) const;

	/** 
	 * @brief Parse command line options and insert them into
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
	virtual bool init() = 0;			// Override to do application initialization

	//
	// cleanup()
	//
	// It's currently assumed that the cleanup() method will only get
	// called from the main thread or the error handling thread, as it will
	// likely do thread shutdown, among other things.
	//
	virtual bool cleanup() = 0;			// Override to do application cleanup

	//
	// mainLoop()
	//
	// Runs the application main loop.  It's assumed that when you exit
	// this method, the application is in one of the cleanup states, either QUITTING or ERROR
	//
	virtual bool mainLoop() = 0; // Override for the application main loop.  Needs to at least gracefully notice the QUITTING state and exit.


	//
	// Application status
	//
	static void setQuitting();	// Set status to QUITTING, the app is now shutting down
	static void setStopped();	// Set status to STOPPED, the app is done running and should exit
	static void setError();		// Set status to ERROR, the error handler should run
	static bool isStopped();
	static bool isRunning();
	static bool isQuitting();
	static bool isError();
	static bool isExiting(); // Either quitting or error (app is exiting, cleanly or not)
#if !LL_WINDOWS
	static U32  getSigChildCount();
	static void incSigChildCount();
#endif
	static int getPid();

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
	void setupErrorHandling();

	void setErrorHandler(LLAppErrorHandler handler);
	void setSyncErrorHandler(LLAppErrorHandler handler);
	//@}

#if !LL_WINDOWS
	//
	// Child process handling (Unix only for now)
	//
	// Set a callback to be run on exit of a child process
	// WARNING!  This callback is run from the signal handler due to
	// Linux threading requiring waitpid() to be called from the thread that spawned the process.
	// At some point I will make this more behaved, but I'm not going to fix this right now - djs
	void setChildCallback(pid_t pid, LLAppChildCallback callback);

    // The child callback to run if no specific handler is set
	void setDefaultChildCallback(LLAppChildCallback callback); 
	
    // Fork and do the proper signal handling/error handling mojo
	// *NOTE: You need to make sure your signal handling callback is
	// correct after you fork, because not all threads are duplicated
	// when you fork!
	pid_t fork(); 
#endif

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

public:
	typedef std::map<std::string, std::string> string_map;
	string_map mOptionMap;	// Contains all command-line options and arguments in a map

protected:

	static void setStatus(EAppStatus status);		// Use this to change the application status.
	static EAppStatus sStatus; // Reflects current application status
	static BOOL sErrorThreadRunning; // Set while the error thread is running

#if !LL_WINDOWS
	static LLAtomicU32* sSigChildCount; // Number of SIGCHLDs received.
	typedef std::map<pid_t, LLChildInfo> child_map; // Map key is a PID
	static child_map sChildMap;
	static LLAppChildCallback sDefaultChildCallback;
#endif

	/**
	  * @brief This method is called once a frame to do once a frame tasks.
	  */
	void stepFrame();

private:
	void startErrorThread();
	
	static void runErrorHandler(); // run shortly after we detect an error, ran in the relatively robust context of the LLErrorThread - preferred.
	static void runSyncErrorHandler(); // run IMMEDIATELY when we get an error, ran in the context of the faulting thread.

	// *NOTE: On Windows, we need a routine to reset the structured
	// exception handler when some evil driver has taken it over for
	// their own purposes
	typedef int(*signal_handler_func)(int signum);
	static LLAppErrorHandler sErrorHandler;
	static LLAppErrorHandler sSyncErrorHandler;

	// Default application threads
	LLErrorThread* mThreadErrorp;		// Waits for app to go to status ERROR, then runs the error callback

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
