/** 
 * @file llformat.cpp
 * @date   January 2007
 * @brief string formatting utility
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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
