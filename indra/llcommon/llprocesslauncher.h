/** 
 * @file llprocesslauncher.h
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
 
#ifndef LL_LLPROCESSLAUNCHER_H
#define LL_LLPROCESSLAUNCHER_H

#if LL_WINDOWS
#include <windows.h>
#endif


/*
	LLProcessLauncher handles launching external processes with specified command line arguments.
	It also keeps track of whether the process is still running, and can kill it if required.
*/

class LL_COMMON_API LLProcessLauncher
{
	LOG_CLASS(LLProcessLauncher);
public:
	LLProcessLauncher();
	virtual ~LLProcessLauncher();
	
	void setExecutable(const std::string &executable);
	void setWorkingDirectory(const std::string &dir);

	const std::string& getExecutable() const;

	void clearArguments();
	void addArgument(const std::string &arg);
	void addArgument(const char *arg);
		
	int launch(void);
	bool isRunning(void);
	
	// Attempt to kill the process -- returns true if the process is no longer running when it returns.
	// Note that even if this returns false, the process may exit some time after it's called.
	bool kill(void);
	
	// Use this if you want the external process to continue execution after the LLProcessLauncher instance controlling it is deleted.
	// Normally, the destructor will attempt to kill the process and wait for termination.
	// This should only be used if the viewer is about to exit -- otherwise, the child process will become a zombie after it exits.
	void orphan(void);	
	
	// This needs to be called periodically on Mac/Linux to clean up zombie processes.
	static void reap(void);
	
	// Accessors for platform-specific process ID
#if LL_WINDOWS
	HANDLE getProcessHandle() { return mProcessHandle; };
#else
	pid_t getProcessID() { return mProcessID; };
#endif	
	
private:
	std::string mExecutable;
	std::string mWorkingDir;
	std::vector<std::string> mLaunchArguments;
	
#if LL_WINDOWS
	HANDLE mProcessHandle;
#else
	pid_t mProcessID;
#endif
};

#endif // LL_LLPROCESSLAUNCHER_H
