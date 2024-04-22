/** 
 * @dir lldirpicker.h
 * @brief OS-specific dir picker
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

// OS specific dir selection dialog. This is implemented as a
// singleton class, so call the instance() method to get the working
// instance. 

#ifndef LL_LLDIRPICKER_H
#define LL_LLDIRPICKER_H

#include "stdtypes.h"

#include "llthread.h"
#include <queue>

#if LL_WINDOWS
#include <shlobj.h>
#endif

#if LL_DARWIN

// AssertMacros.h does bad things.
#undef verify
#undef check
#undef require

#include <vector>
#include "llstring.h"

#endif

class LLFilePicker;

class LLDirPicker
{
public:
	// calling this before main() is undefined
	static LLDirPicker& instance( void ) { return sInstance; }

	BOOL getDir(std::string* filename, bool blocking = true);
	std::string getDirName();

	// clear any lists of buffers or whatever, and make sure the dir
	// picker isn't locked.
	void reset();

private:
	enum
	{
		SINGLE_DIRNAME_BUFFER_SIZE = 1024,
		//DIRNAME_BUFFER_SIZE = 65536
		DIRNAME_BUFFER_SIZE = 65000 
	};
	
	void buildDirname( void );
	bool check_local_file_access_enabled();

#if LL_LINUX || LL_DARWIN
	// On Linux we just implement LLDirPicker on top of LLFilePicker
#ifndef LL_FLTK
	LLFilePicker *mFilePicker;
#endif
#endif


	std::string* mFileName;
	std::string  mDir;
	bool mLocked;

	static LLDirPicker sInstance;
#if LL_WINDOWS
	BROWSEINFO bi;
#endif
	
public:
	// don't call these directly please.
	LLDirPicker();
	~LLDirPicker();
};

class LLDirPickerThread : public LLThread
{
public:

	static std::queue<LLDirPickerThread*> sDeadQ;
	static LLMutex* sMutex;

	static void initClass();
	static void cleanupClass();
	static void clearDead();

	std::vector<std::string> mResponses;
	std::string mProposedName;

	typedef boost::signals2::signal<void(const std::vector<std::string>& filenames, std::string proposed_name)> dir_picked_signal_t;

	LLDirPickerThread(const dir_picked_signal_t::slot_type& cb, const std::string &proposed_name);
	~LLDirPickerThread();

	void getFile();

	virtual void run();

	virtual void notify(const std::vector<std::string>& filenames);

private:
	dir_picked_signal_t*		mFilePickedSignal;
};

#endif
