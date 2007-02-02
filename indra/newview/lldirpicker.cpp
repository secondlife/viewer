/** 
 * @file lldirpicker.cpp
 * @brief OS-specific file picker
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldirpicker.h"
//#include "viewer.h"
//#include "llviewermessage.h"
#include "llworld.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "lldir.h"
#include "llframetimer.h"

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

LLDirPicker::LLDirPicker() 
{
}

LLDirPicker::~LLDirPicker()
{
	// nothing
}

BOOL LLDirPicker::getDir(LLString* filename)
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

LLString LLDirPicker::getDirName()
{
	return mDir;
}

/////////////////////////////////////////////DARWIN
#elif LL_DARWIN

LLDirPicker::LLDirPicker() 
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

BOOL LLDirPicker::getDir(LLString* filename)
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

LLString LLDirPicker::getDirName()
{
	return mDir;
}

void LLDirPicker::reset()
{
	mLocked = FALSE;
	mDir    = NULL;
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

BOOL LLDirPicker::getDir(LLString* filename)
{
	return FALSE;
}

LLString LLDirPicker::getDirName()
{
	return "";
}

#endif
