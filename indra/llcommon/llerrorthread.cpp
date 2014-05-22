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
			LL_INFOS() << "get_child_status - Child exited cleanly with return of " << process_status << LL_ENDL;
		}
		return;
	}
	else if (WIFSIGNALED(waitpid_status))
	{
		process_status = WTERMSIG(waitpid_status);
		exited = true;
		if (do_logging)
		{
			LL_INFOS() << "get_child_status - Child died because of uncaught signal " << process_status << LL_ENDL;
#ifdef WCOREDUMP
			if (WCOREDUMP(waitpid_status))
			{
				LL_INFOS() << "get_child_status - Child dumped core" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "get_child_status - Child didn't dump core" << LL_ENDL;
			}
#endif
		}
		return;
	}
	else if (do_logging)
	{
		// This is weird.  I just dump the waitpid status into the status code,
		// not that there's any way of telling what it is...
		LL_INFOS() << "get_child_status - Got SIGCHILD but child didn't exit" << LL_ENDL;
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
	LL_INFOS() << "thread_error - Waiting for an error" << LL_ENDL;

	S32 counter = 0;
	while (! (LLApp::isError() || LLApp::isStopped()))
	{
		ms_sleep(10);
		counter++;
	}
	if (LLApp::isError())
	{
		// The app is in an error state, run the application's error handler.
		//LL_INFOS() << "thread_error - An error has occurred, running error callback!" << LL_ENDL;
		// Run the error handling callback
		LLApp::runErrorHandler();
	}
	else
	{
		// Everything is okay, a clean exit.
		//LL_INFOS() << "thread_error - Application exited cleanly" << LL_ENDL;
	}
	
	//LL_INFOS() << "thread_error - Exiting" << LL_ENDL;
	LLApp::sErrorThreadRunning = FALSE;
}

