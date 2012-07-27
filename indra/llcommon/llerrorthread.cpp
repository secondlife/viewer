/** 
 * @file llerrorthread.cpp
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
#include "llerrorthread.h"

#include "llapp.h"
#include "lltimer.h"	// ms_sleep()

LLErrorThread::LLErrorThread()
	: LLThread("Error"),
	  mUserDatap(NULL)
{
}

LLErrorThread::~LLErrorThread()
{
}

void LLErrorThread::setUserData(void* user_data)
{
	mUserDatap = user_data;
}


void* LLErrorThread::getUserData() const
{
	return mUserDatap;
}

#if !LL_WINDOWS
//
// Various signal/error handling functions that can't be put into the class
//
void get_child_status(const int waitpid_status, int &process_status, bool &exited, bool do_logging)
{
	exited = false;
	process_status = -1;
	// The child process exited.  Call its callback, and then clean it up
	if (WIFEXITED(waitpid_status))
	{
		process_status = WEXITSTATUS(waitpid_status);
		exited = true;
		if (do_logging)
		{
			llinfos << "get_child_status - Child exited cleanly with return of " << process_status << llendl;
		}
		return;
	}
	else if (WIFSIGNALED(waitpid_status))
	{
		process_status = WTERMSIG(waitpid_status);
		exited = true;
		if (do_logging)
		{
			llinfos << "get_child_status - Child died because of uncaught signal " << process_status << llendl;
#ifdef WCOREDUMP
			if (WCOREDUMP(waitpid_status))
			{
				llinfos << "get_child_status - Child dumped core" << llendl;
			}
			else
			{
				llinfos << "get_child_status - Child didn't dump core" << llendl;
			}
#endif
		}
		return;
	}
	else if (do_logging)
	{
		// This is weird.  I just dump the waitpid status into the status code,
		// not that there's any way of telling what it is...
		llinfos << "get_child_status - Got SIGCHILD but child didn't exit" << llendl;
		process_status = waitpid_status;
	}

}
#endif

void LLErrorThread::run()
{
	LLApp::sErrorThreadRunning = TRUE;
	// This thread sits and waits for the sole purpose
	// of waiting for the signal/exception handlers to flag the
	// application state as APP_STATUS_ERROR.
	llinfos << "thread_error - Waiting for an error" << llendl;

	S32 counter = 0;
#if !LL_WINDOWS
	U32 last_sig_child_count = 0;
#endif
	while (! (LLApp::isError() || LLApp::isStopped()))
	{
#if !LL_WINDOWS
		// Check whether or not the main thread had a sig child we haven't handled.
		U32 current_sig_child_count = LLApp::getSigChildCount();
		if (last_sig_child_count != current_sig_child_count)
		{
			int status = 0;
			pid_t child_pid = 0;
			last_sig_child_count = current_sig_child_count;
			if (LLApp::sLogInSignal)
			{
				llinfos << "thread_error handling SIGCHLD #" << current_sig_child_count << llendl;
			}
			for (LLApp::child_map::iterator iter = LLApp::sChildMap.begin(); iter != LLApp::sChildMap.end();)
			{
				child_pid = iter->first;
				LLChildInfo &child_info = iter->second;
				// check the status of *all* children, in case we missed a signal
				if (0 != waitpid(child_pid, &status, WNOHANG))
				{
					bool exited = false;
					int exit_status = -1;
					get_child_status(status, exit_status, exited, LLApp::sLogInSignal);

					if (child_info.mCallback)
					{
						if (LLApp::sLogInSignal)
						{
							llinfos << "Signal handler - Running child callback" << llendl;
						}
						child_info.mCallback(child_pid, exited, status);
					}
					LLApp::sChildMap.erase(iter++);
				}
				else
				{
					// Child didn't terminate, yet we got a sigchild somewhere...
					if (child_info.mGotSigChild && child_info.mCallback)
					{
						child_info.mCallback(child_pid, false, 0);
					}
					child_info.mGotSigChild = FALSE;
					iter++;
				}
			}

			// check the status of *all* children, in case we missed a signal
			// Same as above, but use the default child callback
			while(0 < (child_pid = waitpid( -1, &status, WNOHANG )))
			{
				if (0 != waitpid(child_pid, &status, WNOHANG))
				{
					bool exited = false;
					int exit_status = -1;
					get_child_status(status, exit_status, exited, LLApp::sLogInSignal);
					if (LLApp::sDefaultChildCallback)
					{
						if (LLApp::sLogInSignal)
						{
							llinfos << "Signal handler - Running default child callback" << llendl;
						}
						LLApp::sDefaultChildCallback(child_pid, true, status);
					}
				}
			}
		}


#endif
		ms_sleep(10);
		counter++;
	}
	if (LLApp::isError())
	{
		// The app is in an error state, run the application's error handler.
		//llinfos << "thread_error - An error has occurred, running error callback!" << llendl;
		// Run the error handling callback
		LLApp::runErrorHandler();
	}
	else
	{
		// Everything is okay, a clean exit.
		//llinfos << "thread_error - Application exited cleanly" << llendl;
	}
	
	//llinfos << "thread_error - Exiting" << llendl;
	LLApp::sErrorThreadRunning = FALSE;
}

