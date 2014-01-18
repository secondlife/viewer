/** 
 * @file llpidlock.h
 * @brief System information debugging classes.
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

#ifndef LL_PIDLOCK_H
#define LL_PIDLOCK_H
#include "llnametable.h"

class LLSD;
class LLFrameTimer;

#if !LL_WINDOWS	//For non-windows platforms.
#include <signal.h>
#endif

namespace LLPidLock
{
    void initClass(); // { (void) LLPidLockFile::instance(); }

	bool requestLock( LLNameTable<void *> *name_table=NULL, bool autosave=TRUE,
					  bool force_immediate=FALSE, F32 timeout=300.0);
	bool checkLock(); 
	void releaseLock(); 
	bool isClean(); 

	//getters
	LLNameTable<void *> * getNameTable(); 
	bool getAutosave(); 
	bool getClean(); 
	std::string getSaveName(); 

	//setters
	void setClean(bool clean); 
	void setSaveName(std::string savename); 
};

#endif // LL_PIDLOCK_H
