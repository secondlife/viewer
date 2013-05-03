/** 
 * @file lldirpicker.cpp
 * @brief OS-specific file picker
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
#include "llviewercontrol.h"

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

// utility function to check if access to local file system via file browser 
// is enabled and if not, tidy up and indicate we're not allowed to do this.
bool LLDirPicker::check_local_file_access_enabled()
{
	// if local file browsing is turned off, return without opening dialog
	bool local_file_system_browsing_enabled = gSavedSettings.getBOOL("LocalFileSystemBrowsingEnabled");
	if ( ! local_file_system_browsing_enabled )
	{
		mDir.clear();	// Windows
		mFileName = NULL; // Mac/Linux
		return false;
	}

	return true;
}

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

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
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

	gViewerWindow->getWindow()->beforeDialog();

	if (error == noErr)
		error = NavDialogRun(navRef);

	gViewerWindow->getWindow()->afterDialog();

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

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

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

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

#if !LL_MESA_HEADLESS

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
#endif // !LL_MESA_HEADLESS

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
