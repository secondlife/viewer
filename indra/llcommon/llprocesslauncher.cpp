/** 
 * @file llprocesslauncher.cpp
 * @brief Utility class for launching, terminating, and tracking the state of processes.
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llprocesslauncher.h"

#include <iostream>
#if LL_DARWIN || LL_LINUX
// not required or present on Win32
#include <sys/wait.h>
#endif

LLProcessLauncher::LLProcessLauncher()
{
#if LL_WINDOWS
	mProcessHandle = 0;
#else
	mProcessID = 0;
#endif
}

LLProcessLauncher::~LLProcessLauncher()
{
	kill();
}

void LLProcessLauncher::setExecutable(const std::string &executable)
{
	mExecutable = executable;
}

void LLProcessLauncher::setWorkingDirectory(const std::string &dir)
{
	mWorkingDir = dir;
}

const std::string& LLProcessLauncher::getExecutable() const
{
	return mExecutable;
}

void LLProcessLauncher::clearArguments()
{
	mLaunchArguments.clear();
}

void LLProcessLauncher::addArgument(const std::string &arg)
{
	mLaunchArguments.push_back(arg);
}

void LLProcessLauncher::addArgument(const char *arg)
{
	mLaunchArguments.push_back(std::string(arg));
}

#if LL_WINDOWS

int LLProcessLauncher::launch(void)
{
	// If there was already a process associated with this object, kill it.
	kill();
	orphan();

	int result = 0;
	
	PROCESS_INFORMATION pinfo;
	STARTUPINFOA sinfo;
	memset(&sinfo, 0, sizeof(sinfo));
	
	std::string args = mExecutable;
	for(int i = 0; i < (int)mLaunchArguments.size(); i++)
	{
		args += " ";
		args += mLaunchArguments[i];
	}
	
	// So retarded.  Windows requires that the second parameter to CreateProcessA be a writable (non-const) string...
	char *args2 = new char[args.size() + 1];
	strcpy(args2, args.c_str());

	const char * working_directory = 0;
	if(!mWorkingDir.empty()) working_directory = mWorkingDir.c_str();
	if( ! CreateProcessA( NULL, args2, NULL, NULL, FALSE, 0, NULL, working_directory, &sinfo, &pinfo ) )
	{
		result = GetLastError();

		LPTSTR error_str = 0;
		if(
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				result,
				0,
				(LPTSTR)&error_str,
				0,
				NULL) 
			!= 0) 
		{
			char message[256];
			wcstombs(message, error_str, 256);
			message[255] = 0;
			llwarns << "CreateProcessA failed: " << message << llendl;
			LocalFree(error_str);
		}

		if(result == 0)
		{
			// Make absolutely certain we return a non-zero value on failure.
			result = -1;
		}
	}
	else
	{
		// foo = pinfo.dwProcessId; // get your pid here if you want to use it later on
		// CloseHandle(pinfo.hProcess); // stops leaks - nothing else
		mProcessHandle = pinfo.hProcess;
		CloseHandle(pinfo.hThread); // stops leaks - nothing else
	}		
	
	delete[] args2;
	
	return result;
}

bool LLProcessLauncher::isRunning(void)
{
	if(mProcessHandle != 0)		
	{
		DWORD waitresult = WaitForSingleObject(mProcessHandle, 0);
		if(waitresult == WAIT_OBJECT_0)
		{
			// the process has completed.
			mProcessHandle = 0;
		}			
	}

	return (mProcessHandle != 0);
}
bool LLProcessLauncher::kill(void)
{
	bool result = true;
	
	if(mProcessHandle != 0)
	{
		TerminateProcess(mProcessHandle,0);

		if(isRunning())
		{
			result = false;
		}
	}
	
	return result;
}

void LLProcessLauncher::orphan(void)
{
	// Forget about the process
	mProcessHandle = 0;
}

// static 
void LLProcessLauncher::reap(void)
{
	// No actions necessary on Windows.
}

#else // Mac and linux

#include <signal.h>
#include <fcntl.h>
#include <errno.h>

static std::list<pid_t> sZombies;

// Attempt to reap a process ID -- returns true if the process has exited and been reaped, false otherwise.
static bool reap_pid(pid_t pid)
{
	bool result = false;
	
	pid_t wait_result = ::waitpid(pid, NULL, WNOHANG);
	if(wait_result == pid)
	{
		result = true;
	}
	else if(wait_result == -1)
	{
		if(errno == ECHILD)
		{
			// No such process -- this may mean we're ignoring SIGCHILD.
			result = true;
		}
	}
	
	return result;
}

int LLProcessLauncher::launch(void)
{
	// If there was already a process associated with this object, kill it.
	kill();
	orphan();
	
	int result = 0;
	int current_wd = -1;
	
	// create an argv vector for the child process
	const char ** fake_argv = new const char *[mLaunchArguments.size() + 2];  // 1 for the executable path, 1 for the NULL terminator

	int i = 0;
	
	// add the executable path
	fake_argv[i++] = mExecutable.c_str();
	
	// and any arguments
	for(int j=0; j < mLaunchArguments.size(); j++)
		fake_argv[i++] = mLaunchArguments[j].c_str();
	
	// terminate with a null pointer
	fake_argv[i] = NULL;
	
	if(!mWorkingDir.empty())
	{
		// save the current working directory
		current_wd = ::open(".", O_RDONLY);
	
		// and change to the one the child will be executed in
		if (::chdir(mWorkingDir.c_str()))
		{
			// chdir failed
		}
	}
		
 	// flush all buffers before the child inherits them
 	::fflush(NULL);

	pid_t id = vfork();
	if(id == 0)
	{
		// child process
		
		::execv(mExecutable.c_str(), (char * const *)fake_argv);
		
		// If we reach this point, the exec failed.
		// Use _exit() instead of exit() per the vfork man page.
		_exit(0);
	}

	// parent process
	
	if(current_wd >= 0)
	{
		// restore the previous working directory
		if (::fchdir(current_wd))
		{
			// chdir failed
		}
		::close(current_wd);
	}
	
	delete[] fake_argv;
	
	mProcessID = id;

	return result;
}

bool LLProcessLauncher::isRunning(void)
{
	if(mProcessID != 0)
	{
		// Check whether the process has exited, and reap it if it has.
		if(reap_pid(mProcessID))
		{
			// the process has exited.
			mProcessID = 0;
		}
	}
	
	return (mProcessID != 0);
}

bool LLProcessLauncher::kill(void)
{
	bool result = true;
	
	if(mProcessID != 0)
	{
		// Try to kill the process.  We'll do approximately the same thing whether the kill returns an error or not, so we ignore the result.
		(void)::kill(mProcessID, SIGTERM);
		
		// This will have the side-effect of reaping the zombie if the process has exited.
		if(isRunning())
		{
			result = false;
		}
	}
	
	return result;
}

void LLProcessLauncher::orphan(void)
{
	// Disassociate the process from this object
	if(mProcessID != 0)
	{	
		// We may still need to reap the process's zombie eventually
		sZombies.push_back(mProcessID);
	
		mProcessID = 0;
	}
}

// static 
void LLProcessLauncher::reap(void)
{
	// Attempt to real all saved process ID's.
	
	std::list<pid_t>::iterator iter = sZombies.begin();
	while(iter != sZombies.end())
	{
		if(reap_pid(*iter))
		{
			iter = sZombies.erase(iter);
		}
		else
		{
			iter++;
		}
	}
}

#endif
