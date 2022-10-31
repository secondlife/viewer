/** 
 * @file llcrashlock.h
 * @brief Maintainence of disk locking files for crash reporting
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_CRASHLOCK_H
#define LL_CRASHLOCK_H

#include "llframetimer.h"

class LLSD;

#if !LL_WINDOWS //For non-windows platforms.
#include <signal.h>
#endif

//Crash reporter will now be kicked off by the viewer but otherwise
//run independent of the viewer.

class LLCrashLock
{
public:
    LLCrashLock();
    bool requestMaster( F32 timeout=300.0); //Wait until timeout for master lock.
    bool checkMaster();                     //True if available.  False if not.
    void releaseMaster( );           //Release master lockfile.
    bool isLockPresent(std::string filename); //Check if lockfile exists.
    bool isProcessAlive(U32 pid, const std::string& pname);               //Check if pid is alive.
    bool isWaiting();                           //Waiting for master lock to be released.
    LLSD getProcessList();                      //Get next process pid/dir pairs
    void cleanupProcess(std::string proc_dir);               //Remove from list, clean up working dir.
    bool putProcessList(const LLSD& processlist); //Write pid/dir pairs back to disk.
    static bool fileExists(std::string filename);
    

    //getters
    S32 getPID();

    //setters
    void setCleanUp(bool cleanup=true);
    void setSaveName(std::string savename);
private:
    LLSD getLockFile(std::string filename);
    bool putLockFile(std::string filename, const LLSD& data);
    bool mCleanUp;
    std::string mMaster;
    std::string mDumpTable;
    U32 mWaitingPID;            //The process we're waiting on if any.
    LLFrameTimer mTimer;
};

#endif // LL_CRASHLOCK_H
