/** 
 * @file llprocesslauncher.h
 * @brief Utility class for launching, terminating, and tracking the state of processes.
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2008, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
