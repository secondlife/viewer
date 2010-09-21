/** 
 * @file lldirpicker.cpp
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

#include "llviewerprecompiledheaders.h"

#include "lldirpicker.h"
//#include "llviewermessage.h"
#include "llworld.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "lldir.h"
#include "llframetimer.h"
#include "lltrans.h"
#include "llwindow.h"	// beforeDialog()

#if LL_LINUX || LL_SOLARIS
# include "llfilepicker.h"
#endif

//
// Globals
//

LLDirPicker LLDirPicker::sInstance;

#if LL_WINDOWS
#include <shlobj.h>
#endif

//
// Implementation
//
#if LL_WINDOWS

LLDirPicker::LLDirPicker() :
	mFileName(NULL),
	mLocked(false)
{
}

LLDirPicker::~LLDirPicker()
{
	// nothing
}

BOOL LLDirPicker::getDir(std::string* filename)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;

	// Modal, so pause agent
	send_agent_pause();

   BROWSEINFO bi;
   memset(&bi, 0, sizeof(bi));

   bi.ulFlags   = BIF_USENEWUI;
   bi.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
   bi.lpszTitle = NULL;

   ::OleInitialize(NULL);

   LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);

   if(pIDL != NULL)
   {
      WCHAR buffer[_MAX_PATH] = {'\0'};

      if(::SHGetPathFromIDList(pIDL, buffer) != 0)
      {
		  	// Set the string value.

   			mDir = utf16str_to_utf8str(llutf16string(buffer));
	         success = TRUE;
      }

      // free the item id list
      CoTaskMemFree(pIDL);
   }

   ::OleUninitialize();

	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

std::string LLDirPicker::getDirName()
{
	return mDir;
}

/////////////////////////////////////////////DARWIN
#elif LL_DARWIN

LLDirPicker::LLDirPicker() :
	mFileName(NULL),
	mLocked(false)
{
	reset();

	memset(&mNavOptions, 0, sizeof(mNavOptions));
	OSStatus	error = NavGetDefaultDialogCreationOptions(&mNavOptions);
	if (error == noErr)
	{
		mNavOptions.modality = kWindowModalityAppModal;
	}
}

LLDirPicker::~LLDirPicker()
{
	// nothing
}

//static
pascal void LLDirPicker::doNavCallbackEvent(NavEventCallbackMessage callBackSelector,
										 NavCBRecPtr callBackParms, void* callBackUD)
{
	switch(callBackSelector)
	{
		case kNavCBStart:
		{
			if (!sInstance.mFileName) break;
 
			OSStatus error = noErr; 
			AEDesc theLocation = {typeNull, NULL};
			FSSpec outFSSpec;
			
			//Convert string to a FSSpec
			FSRef myFSRef;
			
			const char* filename=sInstance.mFileName->c_str();
			
			error = FSPathMakeRef ((UInt8*)filename,	&myFSRef, 	NULL); 
			
			if (error != noErr) break;

			error = FSGetCatalogInfo (&myFSRef, kFSCatInfoNone, NULL, NULL, &outFSSpec, NULL);

			if (error != noErr) break;
	
			error = AECreateDesc(typeFSS, &outFSSpec, sizeof(FSSpec), &theLocation);

			if (error != noErr) break;

			error = NavCustomControl(callBackParms->context,
							kNavCtlSetLocation, (void*)&theLocation);

		}
	}
}

OSStatus	LLDirPicker::doNavChooseDialog()
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;

	memset(&navReply, 0, sizeof(navReply));
	
	// NOTE: we are passing the address of a local variable here.  
	//   This is fine, because the object this call creates will exist for less than the lifetime of this function.
	//   (It is destroyed by NavDialogDispose() below.)

	error = NavCreateChooseFolderDialog(&mNavOptions, &doNavCallbackEvent, NULL, NULL, &navRef);

	gViewerWindow->mWindow->beforeDialog();

	if (error == noErr)
		error = NavDialogRun(navRef);

	gViewerWindow->mWindow->afterDialog();

	if (error == noErr)
		error = NavDialogGetReply(navRef, &navReply);

	if (navRef)
		NavDialogDispose(navRef);

	if (error == noErr && navReply.validRecord)
	{	
		FSRef		fsRef;
		AEKeyword	theAEKeyword;
		DescType	typeCode;
		Size		actualSize = 0;
		char		path[LL_MAX_PATH];		 /*Flawfinder: ignore*/
		
		memset(&fsRef, 0, sizeof(fsRef));
		error = AEGetNthPtr(&navReply.selection, 1, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
		
		if (error == noErr)
			error = FSRefMakePath(&fsRef, (UInt8*) path, sizeof(path));
		
		if (error == noErr)
			mDir = path;
	}
	
	return error;
}

BOOL LLDirPicker::getDir(std::string* filename)
{
	if( mLocked ) return FALSE;
	BOOL success = FALSE;
	OSStatus	error = noErr;
	
	mFileName = filename;
	
//	mNavOptions.saveFileName 

	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavChooseDialog();
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (mDir.length() >  0)
			success = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

std::string LLDirPicker::getDirName()
{
	return mDir;
}

void LLDirPicker::reset()
{
	mLocked = false;
	mDir.clear();
}

#elif LL_LINUX || LL_SOLARIS

LLDirPicker::LLDirPicker() :
	mFileName(NULL),
	mLocked(false)
{
	mFilePicker = new LLFilePicker();
	reset();
}

LLDirPicker::~LLDirPicker()
{
	delete mFilePicker;
}


void LLDirPicker::reset()
{
	if (mFilePicker)
		mFilePicker->reset();
}

BOOL LLDirPicker::getDir(std::string* filename)
{
	reset();
	if (mFilePicker)
	{
		GtkWindow* picker = mFilePicker->buildFilePicker(false, true,
								 "dirpicker");

		if (picker)
		{		   
		   gtk_window_set_title(GTK_WINDOW(picker), LLTrans::getString("choose_the_directory").c_str());
		   gtk_widget_show_all(GTK_WIDGET(picker));
		   gtk_main();
		   return (!mFilePicker->getFirstFile().empty());
		}
	}
	return FALSE;
}

std::string LLDirPicker::getDirName()
{
	if (mFilePicker)
	{
		return mFilePicker->getFirstFile();
	}
	return "";
}

#else // not implemented

LLDirPicker::LLDirPicker() 
{
	reset();
}

LLDirPicker::~LLDirPicker()
{
}


void LLDirPicker::reset()
{
}

BOOL LLDirPicker::getDir(std::string* filename)
{
	return FALSE;
}

std::string LLDirPicker::getDirName()
{
	return "";
}

#endif
