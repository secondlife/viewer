/** 
 * @file llfilepicker.h
 * @brief OS-specific file picker
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

// OS specific file selection dialog. This is implemented as a
// singleton class, so call the instance() method to get the working
// instance. When you call getMultipleOpenFile(), it locks the picker
// until you iterate to the end of the list of selected files with
// getNextFile() or call reset().

#ifndef LL_LLFILEPICKER_H
#define LL_LLFILEPICKER_H

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

// Need commdlg.h for OPENFILENAMEA
#ifdef LL_WINDOWS
#include <commdlg.h>
#endif

extern "C" {
// mostly for Linux, possible on others
#if LL_GTK
# include "gtk/gtk.h"
#endif // LL_GTK
}

class LLFilePicker
{
#ifdef LL_GTK
	friend class LLDirPicker;
	friend void chooser_responder(GtkWidget *, gint, gpointer);
#endif // LL_GTK
public:
	// calling this before main() is undefined
	static LLFilePicker& instance( void ) { return sInstance; }

	enum ELoadFilter
	{
		FFLOAD_ALL = 1,
		FFLOAD_WAV = 2,
		FFLOAD_IMAGE = 3,
		FFLOAD_ANIM = 4,
#ifdef _CORY_TESTING
		FFLOAD_GEOMETRY = 5,
#endif
		FFLOAD_XML = 6,
		FFLOAD_SLOBJECT = 7,
		FFLOAD_RAW = 8,
	};

	enum ESaveFilter
	{
		FFSAVE_ALL = 1,
		FFSAVE_WAV = 3,
		FFSAVE_TGA = 4,
		FFSAVE_BMP = 5,
		FFSAVE_AVI = 6,
		FFSAVE_ANIM = 7,
#ifdef _CORY_TESTING
		FFSAVE_GEOMETRY = 8,
#endif
		FFSAVE_XML = 9,
		FFSAVE_COLLADA = 10,
		FFSAVE_RAW = 11,
		FFSAVE_J2C = 12,
		FFSAVE_PNG = 13,
		FFSAVE_JPEG = 14,
	};

	// open the dialog. This is a modal operation
	BOOL getSaveFile( ESaveFilter filter = FFSAVE_ALL, const std::string& filename = LLStringUtil::null );
	BOOL getOpenFile( ELoadFilter filter = FFLOAD_ALL );
	BOOL getMultipleOpenFiles( ELoadFilter filter = FFLOAD_ALL );

	// Get the filename(s) found. getFirstFile() sets the pointer to
	// the start of the structure and allows the start of iteration.
	const std::string getFirstFile();

	// getNextFile() increments the internal representation and
	// returns the next file specified by the user. Returns NULL when
	// no more files are left. Further calls to getNextFile() are
	// undefined.
	const std::string getNextFile();

	// This utility function extracts the current file name without
	// doing any incrementing.
	const std::string getCurFile();

	// Returns the index of the current file.
	S32 getCurFileNum() const { return mCurrentFile; }

	S32 getFileCount() const { return (S32)mFiles.size(); }

	// See llvfs/lldir.h : getBaseFileName and getDirName to extract base or directory names
	
	// clear any lists of buffers or whatever, and make sure the file
	// picker isn't locked.
	void reset();

private:
	enum
	{
		SINGLE_FILENAME_BUFFER_SIZE = 1024,
		//FILENAME_BUFFER_SIZE = 65536
		FILENAME_BUFFER_SIZE = 65000
	};
	
#if LL_WINDOWS
	OPENFILENAMEW mOFN;				// for open and save dialogs
	WCHAR mFilesW[FILENAME_BUFFER_SIZE];

	BOOL setupFilter(ELoadFilter filter);
#endif

#if LL_DARWIN
	NavDialogCreationOptions mNavOptions;
	std::vector<std::string> mFileVector;
	UInt32 mFileIndex;
	
	OSStatus doNavChooseDialog(ELoadFilter filter);
	OSStatus doNavSaveDialog(ESaveFilter filter, const std::string& filename);
	void getFilePath(SInt32 index);
	void getFileName(SInt32 index);
	static Boolean navOpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode);
#endif

#if LL_GTK
	static void add_to_selectedfiles(gpointer data, gpointer user_data);
	static void chooser_responder(GtkWidget *widget, gint response, gpointer user_data);
	// we remember the last path that was accessed for a particular usage
	std::map <std::string, std::string> mContextToPathMap;
	std::string mCurContextName;
#endif

	std::vector<std::string> mFiles;
	S32 mCurrentFile;
	bool mLocked;

	static LLFilePicker sInstance;
	
protected:
#if LL_GTK
        GtkWindow* buildFilePicker(bool is_save, bool is_folder,
				   std::string context = "generic");
#endif

public:
	// don't call these directly please.
	LLFilePicker();
	~LLFilePicker();
};

#endif
