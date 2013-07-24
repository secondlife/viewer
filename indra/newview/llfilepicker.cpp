/** 
 * @file llfilepicker.cpp
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

#include "llfilepicker.h"
#include "llworld.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "lldir.h"
#include "llframetimer.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llwindow.h"	// beforeDialog()

#if LL_SDL
#include "llwindowsdl.h" // for some X/GTK utils to help with filepickers
#endif // LL_SDL

//
// Globals
//

LLFilePicker LLFilePicker::sInstance;

#if LL_WINDOWS
#define SOUND_FILTER L"Sounds (*.wav)\0*.wav\0"
#define IMAGE_FILTER L"Images (*.tga; *.bmp; *.jpg; *.jpeg; *.png)\0*.tga;*.bmp;*.jpg;*.jpeg;*.png\0"
#define ANIM_FILTER L"Animations (*.bvh; *.anim)\0*.bvh;*.anim\0"
#define COLLADA_FILTER L"Scene (*.dae)\0*.dae\0"
#ifdef _CORY_TESTING
#define GEOMETRY_FILTER L"SL Geometry (*.slg)\0*.slg\0"
#endif
#define XML_FILTER L"XML files (*.xml)\0*.xml\0"
#define SLOBJECT_FILTER L"Objects (*.slobject)\0*.slobject\0"
#define RAW_FILTER L"RAW files (*.raw)\0*.raw\0"
#define MODEL_FILTER L"Model files (*.dae)\0*.dae\0"
#define SCRIPT_FILTER L"Script files (*.lsl)\0*.lsl\0"
#define DICTIONARY_FILTER L"Dictionary files (*.dic; *.xcu)\0*.dic;*.xcu\0"
#endif

//
// Implementation
//
LLFilePicker::LLFilePicker()
	: mCurrentFile(0),
	  mLocked(false)

{
	reset();

#if LL_WINDOWS
	mOFN.lStructSize = sizeof(OPENFILENAMEW);
	mOFN.hwndOwner = NULL;  // Set later
	mOFN.hInstance = NULL;
	mOFN.lpstrCustomFilter = NULL;
	mOFN.nMaxCustFilter = 0;
	mOFN.lpstrFile = NULL;							// set in open and close
	mOFN.nMaxFile = LL_MAX_PATH;
	mOFN.lpstrFileTitle = NULL;
	mOFN.nMaxFileTitle = 0;
	mOFN.lpstrInitialDir = NULL;
	mOFN.lpstrTitle = NULL;
	mOFN.Flags = 0;									// set in open and close
	mOFN.nFileOffset = 0;
	mOFN.nFileExtension = 0;
	mOFN.lpstrDefExt = NULL;
	mOFN.lCustData = 0L;
	mOFN.lpfnHook = NULL;
	mOFN.lpTemplateName = NULL;
	mFilesW[0] = '\0';
#endif

#if LL_DARWIN
	memset(&mNavOptions, 0, sizeof(mNavOptions));
	OSStatus	error = NavGetDefaultDialogCreationOptions(&mNavOptions);
	if (error == noErr)
	{
		mNavOptions.modality = kWindowModalityAppModal;
	}
#endif
}

LLFilePicker::~LLFilePicker()
{
	// nothing
}

// utility function to check if access to local file system via file browser 
// is enabled and if not, tidy up and indicate we're not allowed to do this.
bool LLFilePicker::check_local_file_access_enabled()
{
	// if local file browsing is turned off, return without opening dialog
	bool local_file_system_browsing_enabled = gSavedSettings.getBOOL("LocalFileSystemBrowsingEnabled");
	if ( ! local_file_system_browsing_enabled )
	{
		mFiles.clear();
		return false;
	}

	return true;
}

const std::string LLFilePicker::getFirstFile()
{
	mCurrentFile = 0;
	return getNextFile();
}

const std::string LLFilePicker::getNextFile()
{
	if (mCurrentFile >= getFileCount())
	{
		mLocked = false;
		return std::string();
	}
	else
	{
		return mFiles[mCurrentFile++];
	}
}

const std::string LLFilePicker::getCurFile()
{
	if (mCurrentFile >= getFileCount())
	{
		mLocked = false;
		return std::string();
	}
	else
	{
		return mFiles[mCurrentFile];
	}
}

void LLFilePicker::reset()
{
	mLocked = false;
	mFiles.clear();
	mCurrentFile = 0;
}

#if LL_WINDOWS

BOOL LLFilePicker::setupFilter(ELoadFilter filter)
{
	BOOL res = TRUE;
	switch (filter)
	{
	case FFLOAD_ALL:
		mOFN.lpstrFilter = L"All Files (*.*)\0*.*\0" \
		SOUND_FILTER \
		IMAGE_FILTER \
		ANIM_FILTER \
		L"\0";
		break;
	case FFLOAD_WAV:
		mOFN.lpstrFilter = SOUND_FILTER \
			L"\0";
		break;
	case FFLOAD_IMAGE:
		mOFN.lpstrFilter = IMAGE_FILTER \
			L"\0";
		break;
	case FFLOAD_ANIM:
		mOFN.lpstrFilter = ANIM_FILTER \
			L"\0";
		break;
	case FFLOAD_COLLADA:
		mOFN.lpstrFilter = COLLADA_FILTER \
			L"\0";
		break;
#ifdef _CORY_TESTING
	case FFLOAD_GEOMETRY:
		mOFN.lpstrFilter = GEOMETRY_FILTER \
			L"\0";
		break;
#endif
	case FFLOAD_XML:
		mOFN.lpstrFilter = XML_FILTER \
			L"\0";
		break;
	case FFLOAD_SLOBJECT:
		mOFN.lpstrFilter = SLOBJECT_FILTER \
			L"\0";
		break;
	case FFLOAD_RAW:
		mOFN.lpstrFilter = RAW_FILTER \
			L"\0";
		break;
	case FFLOAD_MODEL:
		mOFN.lpstrFilter = MODEL_FILTER \
			L"\0";
		break;
	case FFLOAD_SCRIPT:
		mOFN.lpstrFilter = SCRIPT_FILTER \
			L"\0";
		break;
	case FFLOAD_DICTIONARY:
		mOFN.lpstrFilter = DICTIONARY_FILTER \
			L"\0";
		break;
	default:
		res = FALSE;
		break;
	}
	return res;
}

BOOL LLFilePicker::getOpenFile(ELoadFilter filter, bool blocking)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	// don't provide default file selection
	mFilesW[0] = '\0';

	mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
	mOFN.lpstrFile = mFilesW;
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;
	mOFN.nFilterIndex = 1;

	setupFilter(filter);
	
	if (blocking)
	{
		// Modal, so pause agent
		send_agent_pause();
	}

	reset();
	
	// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
	success = GetOpenFileName(&mOFN);
	if (success)
	{
		std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
		mFiles.push_back(filename);
	}

	if (blocking)
	{
		send_agent_resume();
		// Account for the fact that the app has been stalled.
		LLFrameTimer::updateFrameTime();
	}
	
	return success;
}

BOOL LLFilePicker::getMultipleOpenFiles(ELoadFilter filter)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	// don't provide default file selection
	mFilesW[0] = '\0';

	mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
	mOFN.lpstrFile = mFilesW;
	mOFN.nFilterIndex = 1;
	mOFN.nMaxFile = FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
		OFN_EXPLORER | OFN_ALLOWMULTISELECT;

	setupFilter(filter);

	reset();
	
	// Modal, so pause agent
	send_agent_pause();
	// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
	success = GetOpenFileName(&mOFN); // pauses until ok or cancel.
	if( success )
	{
		// The getopenfilename api doesn't tell us if we got more than
		// one file, so we have to test manually by checking string
		// lengths.
		if( wcslen(mOFN.lpstrFile) > mOFN.nFileOffset )	/*Flawfinder: ignore*/
		{
			std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
			mFiles.push_back(filename);
		}
		else
		{
			mLocked = true;
			WCHAR* tptrw = mFilesW;
			std::string dirname;
			while(1)
			{
				if (*tptrw == 0 && *(tptrw+1) == 0) // double '\0'
					break;
				if (*tptrw == 0)
					tptrw++; // shouldn't happen?
				std::string filename = utf16str_to_utf8str(llutf16string(tptrw));
				if (dirname.empty())
					dirname = filename + "\\";
				else
					mFiles.push_back(dirname + filename);
				tptrw += filename.size();
			}
		}
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

BOOL LLFilePicker::getSaveFile(ESaveFilter filter, const std::string& filename)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	mOFN.lpstrFile = mFilesW;
	if (!filename.empty())
	{
		llutf16string tstring = utf8str_to_utf16str(filename);
		wcsncpy(mFilesW, tstring.c_str(), FILENAME_BUFFER_SIZE);	}	/*Flawfinder: ignore*/
	else
	{
		mFilesW[0] = '\0';
	}
	mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();

	switch( filter )
	{
	case FFSAVE_ALL:
		mOFN.lpstrDefExt = NULL;
		mOFN.lpstrFilter =
			L"All Files (*.*)\0*.*\0" \
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"Targa, Bitmap Images (*.tga; *.bmp)\0*.tga;*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_WAV:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.wav", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"wav";
		mOFN.lpstrFilter =
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"\0";
		break;
	case FFSAVE_TGA:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.tga", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"tga";
		mOFN.lpstrFilter =
			L"Targa Images (*.tga)\0*.tga\0" \
			L"\0";
		break;
	case FFSAVE_BMP:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.bmp", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"bmp";
		mOFN.lpstrFilter =
			L"Bitmap Images (*.bmp)\0*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_PNG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.png", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"png";
		mOFN.lpstrFilter =
			L"PNG Images (*.png)\0*.png\0" \
			L"\0";
		break;
	case FFSAVE_JPEG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.jpeg", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"jpg";
		mOFN.lpstrFilter =
			L"JPEG Images (*.jpg *.jpeg)\0*.jpg;*.jpeg\0" \
			L"\0";
		break;
	case FFSAVE_AVI:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.avi", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"avi";
		mOFN.lpstrFilter =
			L"AVI Movie File (*.avi)\0*.avi\0" \
			L"\0";
		break;
	case FFSAVE_ANIM:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.xaf", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"xaf";
		mOFN.lpstrFilter =
			L"XAF Anim File (*.xaf)\0*.xaf\0" \
			L"\0";
		break;
#ifdef _CORY_TESTING
	case FFSAVE_GEOMETRY:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.slg", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"slg";
		mOFN.lpstrFilter =
			L"SLG SL Geometry File (*.slg)\0*.slg\0" \
			L"\0";
		break;
#endif
	case FFSAVE_XML:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.xml", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}

		mOFN.lpstrDefExt = L"xml";
		mOFN.lpstrFilter =
			L"XML File (*.xml)\0*.xml\0" \
			L"\0";
		break;
	case FFSAVE_COLLADA:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.collada", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"collada";
		mOFN.lpstrFilter =
			L"COLLADA File (*.collada)\0*.collada\0" \
			L"\0";
		break;
	case FFSAVE_RAW:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.raw", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"raw";
		mOFN.lpstrFilter =	RAW_FILTER \
							L"\0";
		break;
	case FFSAVE_J2C:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.j2c", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"j2c";
		mOFN.lpstrFilter =
			L"Compressed Images (*.j2c)\0*.j2c\0" \
			L"\0";
		break;
	case FFSAVE_SCRIPT:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.lsl", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"txt";
		mOFN.lpstrFilter = L"LSL Files (*.lsl)\0*.lsl\0" L"\0";
		break;
	default:
		return FALSE;
	}

 
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

	reset();

	// Modal, so pause agent
	send_agent_pause();
	{
		// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
		success = GetSaveFileName(&mOFN);
		if (success)
		{
			std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
			mFiles.push_back(filename);
		}
		gKeyboard->resetKeys();
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

#elif LL_DARWIN

Boolean LLFilePicker::navOpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode)
{
	Boolean		result = true;
	ELoadFilter filter = *((ELoadFilter*) callBackUD);
	OSStatus	error = noErr;
	
	if (filterMode == kNavFilteringBrowserList && filter != FFLOAD_ALL && (theItem->descriptorType == typeFSRef || theItem->descriptorType == typeFSS))
	{
		// navInfo is only valid for typeFSRef and typeFSS
		NavFileOrFolderInfo	*navInfo = (NavFileOrFolderInfo*) info;
		if (!navInfo->isFolder)
		{
			AEDesc	desc;
			error = AECoerceDesc(theItem, typeFSRef, &desc);
			if (error == noErr)
			{
				FSRef	fileRef;
				error = AEGetDescData(&desc, &fileRef, sizeof(fileRef));
				if (error == noErr)
				{
					LSItemInfoRecord	fileInfo;
					error = LSCopyItemInfoForRef(&fileRef, kLSRequestExtension | kLSRequestTypeCreator, &fileInfo);
					if (error == noErr)
					{
						if (filter == FFLOAD_IMAGE)
						{
							if (fileInfo.filetype != 'JPEG' && fileInfo.filetype != 'JPG ' && 
								fileInfo.filetype != 'BMP ' && fileInfo.filetype != 'TGA ' &&
								fileInfo.filetype != 'BMPf' && fileInfo.filetype != 'TPIC' &&
								fileInfo.filetype != 'PNG ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("jpeg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("jpg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("bmp"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("tga"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("png"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
								)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_WAV)
						{
							if (fileInfo.filetype != 'WAVE' && fileInfo.filetype != 'WAV ' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("wave"), kCFCompareCaseInsensitive) != kCFCompareEqualTo && 
								CFStringCompare(fileInfo.extension, CFSTR("wav"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_ANIM)
						{
							if (fileInfo.filetype != 'BVH ' &&
								fileInfo.filetype != 'ANIM' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("bvh"), kCFCompareCaseInsensitive) != kCFCompareEqualTo) &&
								fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("anim"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_COLLADA)
						{
							if (fileInfo.filetype != 'DAE ' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("dae"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
#ifdef _CORY_TESTING
						else if (filter == FFLOAD_GEOMETRY)
						{
							if (fileInfo.filetype != 'SLG ' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("slg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
#endif
						else if (filter == FFLOAD_SLOBJECT)
						{
							llwarns << "*** navOpenFilterProc: FFLOAD_SLOBJECT NOT IMPLEMENTED ***" << llendl;
						}
						else if (filter == FFLOAD_RAW)
						{
							if (fileInfo.filetype != '\?\?\?\?' && 
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("raw"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
							)
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_SCRIPT)
						{
							if (fileInfo.filetype != 'LSL ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("lsl"), kCFCompareCaseInsensitive) != kCFCompareEqualTo)) )
							{
								result = false;
							}
						}
						else if (filter == FFLOAD_DICTIONARY)
						{
							if (fileInfo.filetype != 'DIC ' &&
								fileInfo.filetype != 'XCU ' &&
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("dic"), kCFCompareCaseInsensitive) != kCFCompareEqualTo) &&
								 fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("xcu"), kCFCompareCaseInsensitive) != kCFCompareEqualTo)))
							{
								result = false;
							}
						}
						
						if (fileInfo.extension)
						{
							CFRelease(fileInfo.extension);
						}
					}
				}
				AEDisposeDesc(&desc);
			}
		}
	}
	return result;
}

OSStatus	LLFilePicker::doNavChooseDialog(ELoadFilter filter)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	memset(&navReply, 0, sizeof(navReply));
	
	// NOTE: we are passing the address of a local variable here.  
	//   This is fine, because the object this call creates will exist for less than the lifetime of this function.
	//   (It is destroyed by NavDialogDispose() below.)
	error = NavCreateChooseFileDialog(&mNavOptions, NULL, NULL, NULL, navOpenFilterProc, (void*)(&filter), &navRef);

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
		SInt32	count = 0;
		SInt32	index;
		
		// AE indexes are 1 based...
		error = AECountItems(&navReply.selection, &count);
		for (index = 1; index <= count; index++)
		{
			FSRef		fsRef;
			AEKeyword	theAEKeyword;
			DescType	typeCode;
			Size		actualSize = 0;
			char		path[MAX_PATH];	/*Flawfinder: ignore*/
			
			memset(&fsRef, 0, sizeof(fsRef));
			error = AEGetNthPtr(&navReply.selection, index, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
			
			if (error == noErr)
				error = FSRefMakePath(&fsRef, (UInt8*) path, sizeof(path));
			
			if (error == noErr)
				mFiles.push_back(std::string(path));
		}
	}
	
	return error;
}

OSStatus	LLFilePicker::doNavSaveDialog(ESaveFilter filter, const std::string& filename)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;
	
	memset(&navReply, 0, sizeof(navReply));
	
	// Setup the type, creator, and extension
	OSType		type, creator;
	CFStringRef	extension = NULL;
	switch (filter)
	{
		case FFSAVE_WAV:
			type = 'WAVE';
			creator = 'TVOD';
			extension = CFSTR(".wav");
			break;
		
		case FFSAVE_TGA:
			type = 'TPIC';
			creator = 'prvw';
			extension = CFSTR(".tga");
			break;
		
		case FFSAVE_BMP:
			type = 'BMPf';
			creator = 'prvw';
			extension = CFSTR(".bmp");
			break;
		case FFSAVE_JPEG:
			type = 'JPEG';
			creator = 'prvw';
			extension = CFSTR(".jpeg");
			break;
		case FFSAVE_PNG:
			type = 'PNG ';
			creator = 'prvw';
			extension = CFSTR(".png");
			break;
		case FFSAVE_AVI:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".mov");
			break;

		case FFSAVE_ANIM:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".xaf");
			break;

#ifdef _CORY_TESTING
		case FFSAVE_GEOMETRY:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".slg");
			break;
#endif		
		case FFSAVE_RAW:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR(".raw");
			break;

		case FFSAVE_J2C:
			type = '\?\?\?\?';
			creator = 'prvw';
			extension = CFSTR(".j2c");
			break;
		
		case FFSAVE_SCRIPT:
			type = 'LSL ';
			creator = '\?\?\?\?';
			extension = CFSTR(".lsl");
			break;
		
		case FFSAVE_ALL:
		default:
			type = '\?\?\?\?';
			creator = '\?\?\?\?';
			extension = CFSTR("");
			break;
	}
	
	// Create the dialog
	error = NavCreatePutFileDialog(&mNavOptions, type, creator, NULL, NULL, &navRef);
	if (error == noErr)
	{
		CFStringRef	nameString = NULL;
		bool		hasExtension = true;
		
		// Create a CFString of the initial file name
		if (!filename.empty())
			nameString = CFStringCreateWithCString(NULL, filename.c_str(), kCFStringEncodingUTF8);
		else
			nameString = CFSTR("Untitled");
			
		// Add the extension if one was not provided
		if (nameString && !CFStringHasSuffix(nameString, extension))
		{
			CFStringRef	tempString = nameString;
			hasExtension = false;
			nameString = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@%@"), tempString, extension);
			CFRelease(tempString);
		}
		
		// Set the name in the dialog
		if (nameString)
		{
			error = NavDialogSetSaveFileName(navRef, nameString);
			CFRelease(nameString);
		}
		else
		{
			error = paramErr;
		}
	}
	
	gViewerWindow->getWindow()->beforeDialog();

	// Run the dialog
	if (error == noErr)
		error = NavDialogRun(navRef);

	gViewerWindow->getWindow()->afterDialog();

	if (error == noErr)
		error = NavDialogGetReply(navRef, &navReply);
	
	if (navRef)
		NavDialogDispose(navRef);

	if (error == noErr && navReply.validRecord)
	{
		SInt32	count = 0;
		
		// AE indexes are 1 based...
		error = AECountItems(&navReply.selection, &count);
		if (count > 0)
		{
			// Get the FSRef to the containing folder
			FSRef		fsRef;
			AEKeyword	theAEKeyword;
			DescType	typeCode;
			Size		actualSize = 0;
			
			memset(&fsRef, 0, sizeof(fsRef));
			error = AEGetNthPtr(&navReply.selection, 1, typeFSRef, &theAEKeyword, &typeCode, &fsRef, sizeof(fsRef), &actualSize);
			
			if (error == noErr)
			{
				char	path[PATH_MAX];		/*Flawfinder: ignore*/
				char	newFileName[SINGLE_FILENAME_BUFFER_SIZE];	/*Flawfinder: ignore*/
				
				error = FSRefMakePath(&fsRef, (UInt8*)path, PATH_MAX);
				if (error == noErr)
				{
					if (CFStringGetCString(navReply.saveFileName, newFileName, sizeof(newFileName), kCFStringEncodingUTF8))
					{
						mFiles.push_back(std::string(path) + "/" +  std::string(newFileName));
					}
					else
					{
						error = paramErr;
					}
				}
				else
				{
					error = paramErr;
				}
			}
		}
	}
	
	return error;
}

BOOL LLFilePicker::getOpenFile(ELoadFilter filter, bool blocking)
{
	if( mLocked )
		return FALSE;

	BOOL success = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	OSStatus	error = noErr;
	
	reset();
	
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;

	if(filter == FFLOAD_ALL)	// allow application bundles etc. to be traversed; important for DEV-16869, but generally useful
	{
		// mNavOptions.optionFlags |= kNavAllowOpenPackages;
		mNavOptions.optionFlags |= kNavSupportPackages;
	}
	
	if (blocking)
	{
		// Modal, so pause agent
		send_agent_pause();
	}

	{
		error = doNavChooseDialog(filter);
	}
	
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
	}

	if (blocking)
	{
		send_agent_resume();
		// Account for the fact that the app has been stalled.
		LLFrameTimer::updateFrameTime();
	}

	return success;
}

BOOL LLFilePicker::getMultipleOpenFiles(ELoadFilter filter)
{
	if( mLocked )
		return FALSE;

	BOOL success = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	OSStatus	error = noErr;

	reset();
	
	mNavOptions.optionFlags |= kNavAllowMultipleFiles;
	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavChooseDialog(filter);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
		if (getFileCount() > 1)
			mLocked = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

BOOL LLFilePicker::getSaveFile(ESaveFilter filter, const std::string& filename)
{
	if( mLocked )
		return FALSE;
	BOOL success = FALSE;
	OSStatus	error = noErr;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	reset();
	
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;

	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavSaveDialog(filter, filename);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (getFileCount())
			success = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

#elif LL_LINUX || LL_SOLARIS

# if LL_GTK

// static
void LLFilePicker::add_to_selectedfiles(gpointer data, gpointer user_data)
{
	// We need to run g_filename_to_utf8 in the user's locale
	std::string saved_locale(setlocale(LC_ALL, NULL));
	setlocale(LC_ALL, "");

	LLFilePicker* picker = (LLFilePicker*) user_data;
	GError *error = NULL;
	gchar* filename_utf8 = g_filename_to_utf8((gchar*)data,
						  -1, NULL, NULL, &error);
	if (error)
	{
		// *FIXME.
		// This condition should really be notified to the user, e.g.
		// through a message box.  Just logging it is inappropriate.
		
		// g_filename_display_name is ideal, but >= glib 2.6, so:
		// a hand-rolled hacky makeASCII which disallows control chars
		std::string display_name;
		for (const gchar *str = (const gchar *)data; *str; str++)
		{
			display_name += (char)((*str >= 0x20 && *str <= 0x7E) ? *str : '?');
		}
		llwarns << "g_filename_to_utf8 failed on \"" << display_name << "\": " << error->message << llendl;
	}

	if (filename_utf8)
	{
		picker->mFiles.push_back(std::string(filename_utf8));
		lldebugs << "ADDED FILE " << filename_utf8 << llendl;
		g_free(filename_utf8);
	}

	setlocale(LC_ALL, saved_locale.c_str());
}

// static
void LLFilePicker::chooser_responder(GtkWidget *widget, gint response, gpointer user_data)
{
	LLFilePicker* picker = (LLFilePicker*)user_data;

	lldebugs << "GTK DIALOG RESPONSE " << response << llendl;

	if (response == GTK_RESPONSE_ACCEPT)
	{
		GSList *file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widget));
		g_slist_foreach(file_list, (GFunc)add_to_selectedfiles, user_data);
		g_slist_foreach(file_list, (GFunc)g_free, NULL);
		g_slist_free (file_list);
	}

	// set the default path for this usage context.
	const char* cur_folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(widget));
	if (cur_folder != NULL)
	{
		picker->mContextToPathMap[picker->mCurContextName] = cur_folder;
	}

	gtk_widget_destroy(widget);
	gtk_main_quit();
}


GtkWindow* LLFilePicker::buildFilePicker(bool is_save, bool is_folder, std::string context)
{
#ifndef LL_MESA_HEADLESS
	if (LLWindowSDL::ll_try_gtk_init())
	{
		GtkWidget *win = NULL;
		GtkFileChooserAction pickertype =
			is_save?
			(is_folder?
			 GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER :
			 GTK_FILE_CHOOSER_ACTION_SAVE) :
			(is_folder?
			 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER :
			 GTK_FILE_CHOOSER_ACTION_OPEN);

		win = gtk_file_chooser_dialog_new(NULL, NULL,
						  pickertype,
						  GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL,
						  is_folder ?
						  GTK_STOCK_APPLY :
						  (is_save ? 
						   GTK_STOCK_SAVE :
						   GTK_STOCK_OPEN),
						   GTK_RESPONSE_ACCEPT,
						  (gchar *)NULL);
		mCurContextName = context;

		// get the default path for this usage context if it's been
		// seen before.
		std::map<std::string,std::string>::iterator
			this_path = mContextToPathMap.find(context);
		if (this_path != mContextToPathMap.end())
		{
			gtk_file_chooser_set_current_folder
				(GTK_FILE_CHOOSER(win),
				 this_path->second.c_str());
		}

#  if LL_X11
		// Make GTK tell the window manager to associate this
		// dialog with our non-GTK raw X11 window, which should try
		// to keep it on top etc.
		Window XWindowID = LLWindowSDL::get_SDL_XWindowID();
		if (None != XWindowID)
		{
			gtk_widget_realize(GTK_WIDGET(win)); // so we can get its gdkwin
			GdkWindow *gdkwin = gdk_window_foreign_new(XWindowID);
			gdk_window_set_transient_for(GTK_WIDGET(win)->window,
						     gdkwin);
		}
		else
		{
			llwarns << "Hmm, couldn't get xwid to use for transient." << llendl;
		}
#  endif //LL_X11

		g_signal_connect (GTK_FILE_CHOOSER(win),
				  "response",
				  G_CALLBACK(LLFilePicker::chooser_responder),
				  this);

		gtk_window_set_modal(GTK_WINDOW(win), TRUE);

		/* GTK 2.6: if (is_folder)
			gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(win),
			TRUE); */

		return GTK_WINDOW(win);
	}
	else
	{
		return NULL;
	}
#else
	return NULL;
#endif //LL_MESA_HEADLESS
}

static void add_common_filters_to_gtkchooser(GtkFileFilter *gfilter,
					     GtkWindow *picker,
					     std::string filtername)
{	
	gtk_file_filter_set_name(gfilter, filtername.c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker),
				    gfilter);
	GtkFileFilter *allfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(allfilter, "*");
	gtk_file_filter_set_name(allfilter, LLTrans::getString("all_files").c_str());
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker), allfilter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(picker), gfilter);
}

static std::string add_simple_pattern_filter_to_gtkchooser(GtkWindow *picker,
							   std::string pattern,
							   std::string filtername)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gfilter, pattern.c_str());
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}

static std::string add_simple_mime_filter_to_gtkchooser(GtkWindow *picker,
							std::string mime,
							std::string filtername)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_mime_type(gfilter, mime.c_str());
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}

static std::string add_wav_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_mime_filter_to_gtkchooser(picker,  "audio/x-wav",
						    LLTrans::getString("sound_files") + " (*.wav)");
}

static std::string add_anim_filter_to_gtkchooser(GtkWindow *picker)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gfilter, "*.bvh");
	gtk_file_filter_add_pattern(gfilter, "*.anim");
	std::string filtername = LLTrans::getString("animation_files") + " (*.bvh; *.anim)";
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}

static std::string add_collada_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_pattern_filter_to_gtkchooser(picker,  "*.dae",
						       LLTrans::getString("scene_files") + " (*.dae)");
}

static std::string add_imageload_filter_to_gtkchooser(GtkWindow *picker)
{
	GtkFileFilter *gfilter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(gfilter, "*.tga");
	gtk_file_filter_add_mime_type(gfilter, "image/jpeg");
	gtk_file_filter_add_mime_type(gfilter, "image/png");
	gtk_file_filter_add_mime_type(gfilter, "image/bmp");
	std::string filtername = LLTrans::getString("image_files") + " (*.tga; *.bmp; *.jpg; *.png)";
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}
 
static std::string add_script_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_mime_filter_to_gtkchooser(picker,  "text/plain",
							LLTrans::getString("script_files") + " (*.lsl)");
}

static std::string add_dictionary_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_mime_filter_to_gtkchooser(picker,  "text/plain",
							LLTrans::getString("dictionary_files") + " (*.dic; *.xcu)");
}

BOOL LLFilePicker::getSaveFile( ESaveFilter filter, const std::string& filename )
{
	BOOL rtn = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	gViewerWindow->getWindow()->beforeDialog();

	reset();
	
	GtkWindow* picker = buildFilePicker(true, false, "savefile");

	if (picker)
	{
		std::string suggest_name = "untitled";
		std::string suggest_ext = "";
		std::string caption = LLTrans::getString("save_file_verb") + " ";
		switch (filter)
		{
		case FFSAVE_WAV:
			caption += add_wav_filter_to_gtkchooser(picker);
			suggest_ext = ".wav";
			break;
		case FFSAVE_TGA:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.tga", LLTrans::getString("targa_image_files") + " (*.tga)");
			suggest_ext = ".tga";
			break;
		case FFSAVE_BMP:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "image/bmp", LLTrans::getString("bitmap_image_files") + " (*.bmp)");
			suggest_ext = ".bmp";
			break;
		case FFSAVE_AVI:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "video/x-msvideo",
				 LLTrans::getString("avi_movie_file") + " (*.avi)");
			suggest_ext = ".avi";
			break;
		case FFSAVE_ANIM:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.xaf", LLTrans::getString("xaf_animation_file") + " (*.xaf)");
			suggest_ext = ".xaf";
			break;
		case FFSAVE_XML:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.xml", LLTrans::getString("xml_file") + " (*.xml)");
			suggest_ext = ".xml";
			break;
		case FFSAVE_RAW:
			caption += add_simple_pattern_filter_to_gtkchooser
				(picker, "*.raw", LLTrans::getString("raw_file") + " (*.raw)");
			suggest_ext = ".raw";
			break;
		case FFSAVE_J2C:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "images/jp2",
				 LLTrans::getString("compressed_image_files") + " (*.j2c)");
			suggest_ext = ".j2c";
			break;
		case FFSAVE_SCRIPT:
			caption += add_script_filter_to_gtkchooser(picker);
			suggest_ext = ".lsl";
			break;
		default:;
			break;
		}
		
		gtk_window_set_title(GTK_WINDOW(picker), caption.c_str());

		if (filename.empty())
		{
			suggest_name += suggest_ext;

			gtk_file_chooser_set_current_name
				(GTK_FILE_CHOOSER(picker),
				 suggest_name.c_str());
		}
		else
		{
			gtk_file_chooser_set_current_name
				(GTK_FILE_CHOOSER(picker), filename.c_str());
		}

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();

		rtn = (getFileCount() == 1);
	}

	gViewerWindow->getWindow()->afterDialog();

	return rtn;
}

BOOL LLFilePicker::getOpenFile( ELoadFilter filter, bool blocking )
{
	BOOL rtn = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	gViewerWindow->getWindow()->beforeDialog();

	reset();
	
	GtkWindow* picker = buildFilePicker(false, false, "openfile");

	if (picker)
	{
		std::string caption = LLTrans::getString("load_file_verb") + " ";
		std::string filtername = "";
		switch (filter)
		{
		case FFLOAD_WAV:
			filtername = add_wav_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_ANIM:
			filtername = add_anim_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_COLLADA:
			filtername = add_collada_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_IMAGE:
			filtername = add_imageload_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_SCRIPT:
			filtername = add_script_filter_to_gtkchooser(picker);
			break;
		case FFLOAD_DICTIONARY:
			filtername = add_dictionary_filter_to_gtkchooser(picker);
			break;
		default:;
			break;
		}

		caption += filtername;
		
		gtk_window_set_title(GTK_WINDOW(picker), caption.c_str());

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();

		rtn = (getFileCount() == 1);
	}

	gViewerWindow->getWindow()->afterDialog();

	return rtn;
}

BOOL LLFilePicker::getMultipleOpenFiles( ELoadFilter filter )
{
	BOOL rtn = FALSE;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	gViewerWindow->getWindow()->beforeDialog();

	reset();
	
	GtkWindow* picker = buildFilePicker(false, false, "openfile");

	if (picker)
	{
		gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER(picker),
						      TRUE);

		gtk_window_set_title(GTK_WINDOW(picker), LLTrans::getString("load_files").c_str());

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();
		rtn = !mFiles.empty();
	}

	gViewerWindow->getWindow()->afterDialog();

	return rtn;
}

# else // LL_GTK

// Hacky stubs designed to facilitate fake getSaveFile and getOpenFile with
// static results, when we don't have a real filepicker.

BOOL LLFilePicker::getSaveFile( ESaveFilter filter, const std::string& filename )
{
	// if local file browsing is turned off, return without opening dialog
	// (Even though this is a stub, I think we still should not return anything at all)
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	reset();
	
	llinfos << "getSaveFile suggested filename is [" << filename
		<< "]" << llendl;
	if (!filename.empty())
	{
		mFiles.push_back(gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + filename);
		return TRUE;
	}
	return FALSE;
}

BOOL LLFilePicker::getOpenFile( ELoadFilter filter, bool blocking )
{
	// if local file browsing is turned off, return without opening dialog
	// (Even though this is a stub, I think we still should not return anything at all)
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	reset();
	
	// HACK: Static filenames for 'open' until we implement filepicker
	std::string filename = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + "upload";
	switch (filter)
	{
	case FFLOAD_WAV: filename += ".wav"; break;
	case FFLOAD_IMAGE: filename += ".tga"; break;
	case FFLOAD_ANIM: filename += ".bvh"; break;
	default: break;
	}
	mFiles.push_back(filename);
	llinfos << "getOpenFile: Will try to open file: " << filename << llendl;
	return TRUE;
}

BOOL LLFilePicker::getMultipleOpenFiles( ELoadFilter filter )
{
	// if local file browsing is turned off, return without opening dialog
	// (Even though this is a stub, I think we still should not return anything at all)
	if ( check_local_file_access_enabled() == false )
	{
		return FALSE;
	}

	reset();
	return FALSE;
}

#endif // LL_GTK

#else // not implemented

BOOL LLFilePicker::getSaveFile( ESaveFilter filter, const std::string& filename )
{
	reset();	
	return FALSE;
}

BOOL LLFilePicker::getOpenFile( ELoadFilter filter )
{
	reset();
	return FALSE;
}

BOOL LLFilePicker::getMultipleOpenFiles( ELoadFilter filter )
{
	reset();
	return FALSE;
}

#endif
