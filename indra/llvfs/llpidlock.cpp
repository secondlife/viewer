/** 
 * @file llformat.cpp
 * @date   January 2007
 * @brief string formatting utility
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llpidlock.h"
#include "lldir.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llnametable.h"
#include "llframetimer.h"

#if LL_WINDOWS   //For windows platform.

#include <windows.h>

namespace {
	inline DWORD getpid() {
		return GetCurrentProcessId();
	}
}

bool isProcessAlive(U32 pid)
{
	return (bool) GetProcessVersion((DWORD)pid);
}

#else   //Everyone Else
bool isProcessAlive(U32 pid)
{   
	return (bool) kill( (pid_t)pid, 0);
}
#endif //Everyone else.


	
class LLPidLockFile
{
	public:
		LLPidLockFile( ) :
			mAutosave(false),
			mSaving(false),
			mWaiting(false),
			mPID(getpid()),
			mNameTable(NULL),
			mClean(true)
		{
			mLockName = gDirUtilp->getTempDir() + gDirUtilp->getDirDelimiter() + "savelock";
		}
		bool requestLock(LLNameTable<void *> *name_table, bool autosave,
						bool force_immediate=FALSE, F32 timeout=300.0);
		bool checkLock();
		void releaseLock();

	private:
		void writeLockFile(LLSD pids);
	public:
		static LLPidLockFile& instance(); // return the singleton black list file
			 
		bool mAutosave;
		bool mSaving;
		bool mWaiting;
		LLFrameTimer mTimer;
		U32  mPID;
		std::string mLockName;
		std::string mSaveName;
		LLSD mPIDS_sd;
		LLNameTable<void*> *mNameTable;
		bool mClean;
};

LLPidLockFile& LLPidLockFile::instance()
{   
	static LLPidLockFile the_file;
	return the_file;
}

void LLPidLockFile::writeLockFile(LLSD pids)
{
	llofstream ofile(mLockName);

	if (!LLSDSerialize::toXML(pids,ofile))
	{
		llwarns << "Unable to write concurrent save lock file." << llendl;
	}
	ofile.close();
}

bool LLPidLockFile::requestLock(LLNameTable<void *> *name_table, bool autosave,
								bool force_immediate, F32 timeout)
{
	bool readyToSave = FALSE;

	if (mSaving) return FALSE;	//Bail out if we're currently saving.  Will not queue another save.
	
	if (!mWaiting){
		mNameTable=name_table;
		mAutosave = autosave;
	}

	LLSD out_pids;
	out_pids.append( (LLSD::Integer)mPID );

	llifstream ifile(mLockName);

	if (ifile.is_open()) 
	{									//If file exists, we need to decide whether or not to continue.
		if ( force_immediate
			|| mTimer.hasExpired() )	//Only deserialize if we REALLY need to.
		{

			LLSD in_pids;

			LLSDSerialize::fromXML(in_pids, ifile);	

			//Clean up any dead PIDS that might be in there.
			for (LLSD::array_iterator i=in_pids.beginArray();
				i !=in_pids.endArray();
				++i)
			{
				U32 stored_pid=(*i).asInteger();

				if (isProcessAlive(stored_pid))
				{
					out_pids.append( (*i) );
				}
			}

			readyToSave=TRUE;
		}
		ifile.close();
	}
	else
	{
		readyToSave=TRUE;
	}

	if (!mWaiting)				//Not presently waiting to save.  Queue up.
	{
		mTimer.resetWithExpiry(timeout);
		mWaiting=TRUE;
	}

	if (readyToSave)
	{	//Potential race condition won't kill us. Ignore it.
		writeLockFile(out_pids);
		mSaving=TRUE;
	}
	
	return readyToSave;
}

bool LLPidLockFile::checkLock()
{
	return mWaiting;
}

void LLPidLockFile::releaseLock()
{
	llifstream ifile(mLockName);
	LLSD in_pids;
	LLSD out_pids;
	bool write_file=FALSE;

	LLSDSerialize::fromXML(in_pids, ifile);	

	//Clean up this PID and any dead ones.
	for (LLSD::array_iterator i=in_pids.beginArray();
		i !=in_pids.endArray();
		++i)
	{
		U32 stored_pid=(*i).asInteger();

		if (stored_pid != mPID && isProcessAlive(stored_pid))
		{
			out_pids.append( (*i) );
			write_file=TRUE;
		}
	}
	ifile.close();

	if (write_file)
	{
		writeLockFile(out_pids);
	}
	else
	{
		unlink(mLockName.c_str());
	}

	mSaving=FALSE;
	mWaiting=FALSE;
}

//LLPidLock

void LLPidLock::initClass() { 
	(void) LLPidLockFile::instance(); 
}

bool LLPidLock::checkLock() 
{
	return LLPidLockFile::instance().checkLock();
}

bool LLPidLock::requestLock(LLNameTable<void *> *name_table, bool autosave,
								bool force_immediate, F32 timeout)
{
	return LLPidLockFile::instance().requestLock(name_table,autosave,force_immediate,timeout);
}

void LLPidLock::releaseLock() 
{ 
	return LLPidLockFile::instance().releaseLock(); 
}

bool LLPidLock::isClean() 
{ 
	return LLPidLockFile::instance().mClean; 
}

//getters
LLNameTable<void *> * LLPidLock::getNameTable() 
{ 
    return LLPidLockFile::instance().mNameTable; 
}

bool LLPidLock::getAutosave() 
{ 
	return LLPidLockFile::instance().mAutosave; 
}

bool LLPidLock::getClean() 
{ 
	return LLPidLockFile::instance().mClean; 
}

std::string LLPidLock::getSaveName() 
{ 
	return LLPidLockFile::instance().mSaveName; 
}

//setters
void LLPidLock::setClean(bool clean) 
{ 
	LLPidLockFile::instance().mClean=clean; 
}

void LLPidLock::setSaveName(std::string savename) 
{ 
	LLPidLockFile::instance().mSaveName=savename; 
}
