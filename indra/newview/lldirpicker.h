/** 
 * @dir lldirpicker.h
 * @brief OS-specific dir picker
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

// OS specific dir selection dialog. This is implemented as a
// singleton class, so call the instance() method to get the working
// instance. 

#ifndef LL_LLDIRPICKER_H
#define LL_LLDIRPICKER_H

#include "stdtypes.h"

#if LL_DARWIN
#include <Carbon/Carbon.h>

// AssertMacros.h does bad things.
#undef verify
#undef check
#undef require

#include <vector>
#include "llstring.h"

#endif

// Need commdlg.h for OPENDIRNAMEA
#ifdef LL_WINDOWS
#include <commdlg.h>
#endif

class LLFilePicker;

class LLDirPicker
{
public:
	// calling this before main() is undefined
	static LLDirPicker& instance( void ) { return sInstance; }

	BOOL getDir(std::string* filename);
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

#if LL_DARWIN
	NavDialogCreationOptions mNavOptions;
	static pascal void doNavCallbackEvent(NavEventCallbackMessage callBackSelector,
										 NavCBRecPtr callBackParms, void* callBackUD);
	OSStatus	doNavChooseDialog();
	
#endif

#if LL_LINUX || LL_SOLARIS
	// On Linux we just implement LLDirPicker on top of LLFilePicker
	LLFilePicker *mFilePicker;
#endif

	std::string* mFileName;
	std::string  mDir;
	bool mLocked;

	static LLDirPicker sInstance;
	
public:
	// don't call these directly please.
	LLDirPicker();
	~LLDirPicker();
};

#endif
