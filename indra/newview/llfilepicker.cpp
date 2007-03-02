/** 
 * @file llfilepicker.cpp
 * @brief OS-specific file picker
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfilepicker.h"
//#include "viewer.h"
//#include "llviewermessage.h"
#include "llworld.h"
#include "llviewerwindow.h"
#include "llkeyboard.h"
#include "lldir.h"
#include "llframetimer.h"

#if LL_SDL
#include "llwindowsdl.h" // for some X/GTK utils to help with filepickers
#endif // LL_SDL

//
// Globals
//

LLFilePicker LLFilePicker::sInstance;

#if LL_WINDOWS
#define SOUND_FILTER L"Sounds (*.wav)\0*.wav\0"
#define IMAGE_FILTER L"Images (*.tga; *.bmp; *.jpg; *.jpeg)\0*.tga;*.bmp;*.jpg;*.jpeg\0"
#define ANIM_FILTER L"Animations (*.bvh)\0*.bvh\0"
#ifdef _CORY_TESTING
#define GEOMETRY_FILTER L"SL Geometry (*.slg)\0*.slg\0"
#endif
#define XML_FILTER L"XML files (*.xml)\0*.xml\0"
#define SLOBJECT_FILTER L"Objects (*.slobject)\0*.slobject\0"
#define RAW_FILTER L"RAW files (*.raw)\0*.raw\0"
#endif

//
// Implementation
//
#if LL_WINDOWS

LLFilePicker::LLFilePicker() 
{
	reset();

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
}

LLFilePicker::~LLFilePicker()
{
	// nothing
}

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
	default:
		res = FALSE;
		break;
	}
	return res;
}

BOOL LLFilePicker::getOpenFile(ELoadFilter filter)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;
	mMultiFile = FALSE;

	// don't provide default file selection
	mFilesW[0] = '\0';

	mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
	mOFN.lpstrFile = mFilesW;
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR ;
	mOFN.nFilterIndex = 1;

	setupFilter(filter);
	
	// Modal, so pause agent
	send_agent_pause();
	// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
	success = GetOpenFileName(&mOFN);
	if (success)
	{
		LLString tstr = utf16str_to_utf8str(llutf16string(mFilesW));
		memcpy(mFiles, tstr.c_str(), tstr.size()+1); /*Flawfinder: ignore*/
		mCurrentFile = mFiles;
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

BOOL LLFilePicker::getMultipleOpenFiles(ELoadFilter filter)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;
	mMultiFile = FALSE;

	// don't provide default file selection
	mFilesW[0] = '\0';

	mOFN.hwndOwner = (HWND)gViewerWindow->getPlatformWindow();
	mOFN.lpstrFile = mFilesW;
	mOFN.nFilterIndex = 1;
	mOFN.nMaxFile = FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
		OFN_EXPLORER | OFN_ALLOWMULTISELECT;

	setupFilter(filter);
	
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
			mMultiFile = FALSE;
			mCurrentFile = mFiles;
			LLString tstr = utf16str_to_utf8str(llutf16string(mFilesW));
			memcpy(mFiles, tstr.c_str(), tstr.size()+1); /*Flawfinder: ignore*/
		}
		else
		{
			mMultiFile = TRUE;
			mCurrentFile = 0;
			mLocked = TRUE;
			WCHAR* tptrw = mFilesW;
			char* tptr = mFiles;
			memset( mFiles, 0, FILENAME_BUFFER_SIZE );
			while(1)
			{
				if (*tptrw == 0 && *(tptrw+1) == 0) // double '\0'
					break;
				if (*tptrw == 0 && !mCurrentFile)
					mCurrentFile = tptr+1;
				S32 tlen16,tlen8;
				tlen16 = utf16chars_to_utf8chars(tptrw, tptr, &tlen8);
				tptrw += tlen16;
				tptr += tlen8;
			}
		}
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

BOOL LLFilePicker::getSaveFile(ESaveFilter filter, const char* filename)
{
	if( mLocked )
	{
		return FALSE;
	}
	BOOL success = FALSE;
	mMultiFile = FALSE;

	mOFN.lpstrFile = mFilesW;
	if (filename)
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
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.wav", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"wav";
			L"WAV Sounds (*.wav)\0*.wav\0" \
			L"\0";
		break;
	case FFSAVE_TGA:
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.tga", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"tga";
		mOFN.lpstrFilter =
			L"Targa Images (*.tga)\0*.tga\0" \
			L"\0";
		break;
	case FFSAVE_BMP:
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.bmp", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"bmp";
		mOFN.lpstrFilter =
			L"Bitmap Images (*.bmp)\0*.bmp\0" \
			L"\0";
		break;
	case FFSAVE_AVI:
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.avi", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"avi";
		mOFN.lpstrFilter =
			L"AVI Movie File (*.avi)\0*.avi\0" \
			L"\0";
		break;
	case FFSAVE_ANIM:
		if (!filename)
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
		if (!filename)
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
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.xml", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}

		mOFN.lpstrDefExt = L"xml";
		mOFN.lpstrFilter =
			L"XML File (*.xml)\0*.xml\0" \
			L"\0";
		break;
	case FFSAVE_COLLADA:
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.collada", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"collada";
		mOFN.lpstrFilter =
			L"COLLADA File (*.collada)\0*.collada\0" \
			L"\0";
		break;
	case FFSAVE_RAW:
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.raw", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
		}
		mOFN.lpstrDefExt = L"raw";
		mOFN.lpstrFilter =	RAW_FILTER \
							L"\0";
		break;
	case FFSAVE_J2C:
		if (!filename)
		{
			wcsncpy( mFilesW,L"untitled.j2c", FILENAME_BUFFER_SIZE);
		}
		mOFN.lpstrDefExt = L"j2c";
		mOFN.lpstrFilter =
			L"Compressed Images (*.j2c)\0*.j2c\0" \
			L"\0";
		break;
	default:
		return FALSE;
	}

 
	mOFN.nMaxFile = SINGLE_FILENAME_BUFFER_SIZE;
	mOFN.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST;

	// Modal, so pause agent
	send_agent_pause();
	{
		// NOTA BENE: hitting the file dialog triggers a window focus event, destroying the selection manager!!
		success = GetSaveFileName(&mOFN);
		if (success)
		{
			LLString tstr = utf16str_to_utf8str(llutf16string(mFilesW));
			memcpy(mFiles, tstr.c_str(), tstr.size()+1);  /*Flawfinder: ignore*/
			mCurrentFile = mFiles;
		}
		gKeyboard->resetKeys();
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

const char* LLFilePicker::getFirstFile()
{
	if(mMultiFile)
	{
		buildFilename();
		return mFilename;
	}
	return mFiles;
}

const char* LLFilePicker::getNextFile()
{
	if(mMultiFile)
	{
		mCurrentFile += strlen(mCurrentFile) + 1;	/*Flawfinder: ignore*/
		if( '\0' != mCurrentFile[0] )
		{
			buildFilename();
			return mFilename;
		}
		else
		{
			mLocked = FALSE;
		}
	}
	return NULL;
}

const char* LLFilePicker::getDirname()
{
	if( '\0' != mCurrentFile[0] )
	{
		return mCurrentFile;
	}
	return NULL;
}

void LLFilePicker::reset()
{
	mLocked = FALSE;
	memset( mFiles, 0, FILENAME_BUFFER_SIZE );
	memset( mFilename, 0, LL_MAX_PATH );
	mCurrentFile = mFiles;
}

void LLFilePicker::buildFilename( void )
{
	strncpy( mFilename, mFiles, LL_MAX_PATH );	/*Flawfinder: ignore*/
	S32 len = strlen( mFilename );	/*Flawfinder: ignore*/

	strncat(mFilename,gDirUtilp->getDirDelimiter().c_str(), sizeof(mFilename)-len+1);		/*Flawfinder: ignore*/
	len += strlen(gDirUtilp->getDirDelimiter().c_str());	/*Flawfinder: ignore*/

//	mFilename[len++] = '\\';
	LLString::copy( mFilename + len, mCurrentFile, LL_MAX_PATH - len );
}

#elif LL_DARWIN

LLFilePicker::LLFilePicker() 
{
	reset();

	memset(&mNavOptions, 0, sizeof(mNavOptions));
	OSStatus	error = NavGetDefaultDialogCreationOptions(&mNavOptions);
	if (error == noErr)
	{
		mNavOptions.modality = kWindowModalityAppModal;
	}
	mFileIndex = 0;
}

LLFilePicker::~LLFilePicker()
{
	// nothing
}

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
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("jpeg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("jpg"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("bmp"), kCFCompareCaseInsensitive) != kCFCompareEqualTo &&
								CFStringCompare(fileInfo.extension, CFSTR("tga"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
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
								(fileInfo.extension && (CFStringCompare(fileInfo.extension, CFSTR("bvh"), kCFCompareCaseInsensitive) != kCFCompareEqualTo))
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

	memset(&navReply, 0, sizeof(navReply));
	mFiles[0] = '\0';
	mFileVector.clear();
	
	// NOTE: we are passing the address of a local variable here.  
	//   This is fine, because the object this call creates will exist for less than the lifetime of this function.
	//   (It is destroyed by NavDialogDispose() below.)
	error = NavCreateChooseFileDialog(&mNavOptions, NULL, NULL, NULL, navOpenFilterProc, (void*)(&filter), &navRef);

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
				mFileVector.push_back(LLString(path));
		}
	}
	
	return error;
}

OSStatus	LLFilePicker::doNavSaveDialog(ESaveFilter filter, const char* filename)
{
	OSStatus		error = noErr;
	NavDialogRef	navRef = NULL;
	NavReplyRecord	navReply;
	
	memset(&navReply, 0, sizeof(navReply));
	mFiles[0] = '\0';
	mFileVector.clear();
	
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
		if (filename)
			nameString = CFStringCreateWithCString(NULL, filename, kCFStringEncodingUTF8);
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
	
	gViewerWindow->mWindow->beforeDialog();

	// Run the dialog
	if (error == noErr)
		error = NavDialogRun(navRef);

	gViewerWindow->mWindow->afterDialog();

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
						mFileVector.push_back(LLString(path) + LLString("/") +  LLString(newFileName));
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

BOOL LLFilePicker::getOpenFile(ELoadFilter filter)
{
	if( mLocked ) return FALSE;
	mMultiFile = FALSE;
	BOOL success = FALSE;

	OSStatus	error = noErr;
	
	mFileVector.clear();
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;
	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavChooseDialog(filter);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (mFileVector.size())
			success = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

BOOL LLFilePicker::getMultipleOpenFiles(ELoadFilter filter)
{
	if( mLocked ) return FALSE;
	mMultiFile = TRUE;
	BOOL success = FALSE;

	OSStatus	error = noErr;
	
	mFileVector.clear();
	mNavOptions.optionFlags |= kNavAllowMultipleFiles;
	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavChooseDialog(filter);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (mFileVector.size())
			success = true;
		if (mFileVector.size() > 1)
			mLocked = TRUE;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

void LLFilePicker::getFilePath(SInt32 index)
{
	mFiles[0] = 0;
	if (mFileVector.size())
		strncpy(mFiles, mFileVector[index].c_str(), sizeof(mFiles));	/*Flawfinder: ignore*/ 	
}

void LLFilePicker::getFileName(SInt32 index)
{
	mFilename[0] = 0;
	if (mFileVector.size())
	{
		char	*start = strrchr(mFileVector[index].c_str(), '/');
		if (start && ((start + 1 - mFileVector[index].c_str()) < (mFileVector[index].size())))
			strncpy(mFilename, start + 1, sizeof(mFilename));		/*Flawfinder: ignore*/
	}
}

BOOL LLFilePicker::getSaveFile(ESaveFilter filter, const char* filename)
{
	if( mLocked ) return FALSE;
	BOOL success = FALSE;
	OSStatus	error = noErr;

	mFileVector.clear();
	mMultiFile = FALSE;
	mNavOptions.optionFlags &= ~kNavAllowMultipleFiles;

	// Modal, so pause agent
	send_agent_pause();
	{
		error = doNavSaveDialog(filter, filename);
	}
	send_agent_resume();
	if (error == noErr)
	{
		if (mFileVector.size())
			success = true;
	}

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

const char* LLFilePicker::getFirstFile()
{
	mFileIndex = 0;
	getFilePath(mFileIndex);
	return mFiles;
}

const char* LLFilePicker::getNextFile()
{
	if(mMultiFile)
	{
		mFileIndex++;
		if (mFileIndex < mFileVector.size())
		{
			getFilePath(mFileIndex);
			return mFiles;
		}
		else
		{
			mLocked = FALSE;
		}
	}
	return NULL;
}

const char* LLFilePicker::getDirname()
{
	if (mFileIndex < mFileVector.size())
	{
		getFileName(mFileIndex);
		return mFilename;
	}
	return NULL;
}

void LLFilePicker::reset()
{
	mLocked = FALSE;
	memset( mFiles, 0, FILENAME_BUFFER_SIZE );
	memset( mFilename, 0, LL_MAX_PATH );
	mCurrentFile = mFiles;

	mFileIndex = 0;
	mFileVector.clear();
}

#elif LL_LINUX

# if LL_GTK
LLFilePicker::LLFilePicker() 
{
	reset();
}

LLFilePicker::~LLFilePicker()
{
}

static void store_filenames(GtkWidget *widget, gpointer user_data) {
	StoreFilenamesStruct *sfs = (StoreFilenamesStruct*) user_data;
	GtkWidget *win = sfs->win;

	llinfos <<"store_filesnames: marker A" << llendl;

	// get NULL-terminated list of strings allocated for us by GTK
	gchar** string_list =
		gtk_file_selection_get_selections(GTK_FILE_SELECTION(win));

	llinfos <<"store_filesnames: marker B" << llendl;

	int idx = 0;
	while (string_list[idx])
	{
		// platform-string to utf8
		gchar* filename_utf8 = g_filename_from_utf8(string_list[idx],
							    -1, NULL,
							    NULL,
							    NULL);
		sfs->fileVector.push_back(LLString(filename_utf8));
		++idx;
	}

	llinfos <<"store_filesnames: marker C" << llendl;

	g_strfreev(string_list);

	llinfos <<"store_filesnames: marker D" << llendl;
	
	llinfos << sfs->fileVector.size() << " filename(s) selected:" << llendl;
	U32 x;
	for (x=0; x<sfs->fileVector.size(); ++x)
		llinfos << x << ":" << sfs->fileVector[x] << llendl;
}

GtkWindow* LLFilePicker::buildFilePicker(void)
{
	if (ll_try_gtk_init() &&
	    ! gViewerWindow->getWindow()->getFullscreen())
	{
		GtkWidget *win = NULL;

		win = gtk_file_selection_new(NULL);
		mStoreFilenames.win = win;

#  if LL_X11
		// Make GTK tell the window manager to associate this
		// dialog with our non-GTK raw X11 window, which should try
		// to keep it on top etc.
		Window XWindowID = get_SDL_XWindowID();
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

		g_signal_connect (G_OBJECT(win), "destroy",
				  G_CALLBACK(gtk_main_quit), NULL);

		// on 'ok', grab the file selection list
		g_signal_connect (GTK_FILE_SELECTION(win)->ok_button,
				  "clicked",
				  G_CALLBACK(store_filenames),
				  &mStoreFilenames);

		// both 'ok' and 'cancel' will end the dialog
		g_signal_connect_swapped (G_OBJECT(GTK_FILE_SELECTION(win)->
						   ok_button),
					  "clicked",
					  G_CALLBACK(gtk_widget_destroy),
					  G_OBJECT(win));
		g_signal_connect_swapped (G_OBJECT(GTK_FILE_SELECTION(win)->
						   cancel_button),
					  "clicked",
					  G_CALLBACK(gtk_widget_destroy),
					  G_OBJECT(win));

		gtk_file_selection_show_fileop_buttons(GTK_FILE_SELECTION(win));
		gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(win),
						       FALSE);

		gtk_window_set_modal(GTK_WINDOW(win), TRUE);

		return GTK_WINDOW(win);
	}
	else
	{
		return NULL;
	}
}

BOOL LLFilePicker::getSaveFile( ESaveFilter filter, const char* filename )
{
	BOOL rtn = FALSE;

	gViewerWindow->mWindow->beforeDialog();

	reset();
	GtkWindow* picker = buildFilePicker();

	if (picker)
	{
		std::string suggest_name = "untitled";
		std::string suggest_ext = "";
		std::string caption = "Save ";
		switch (filter)
		{
		case FFSAVE_WAV:
			caption += "Sounds (*.wav)";
			suggest_ext += ".wav";
			break;
		case FFSAVE_TGA:
			caption += "Targa Images (*.tga)";
			suggest_ext += ".tga";
			break;
		case FFSAVE_BMP:
			caption += "Bitmap Images (*.bmp)";
			suggest_ext += ".bmp";
			break;
		case FFSAVE_AVI:
			caption += "AVI Movie File (*.avi)";
			suggest_ext += ".avi";
			break;
		case FFSAVE_ANIM:
			caption += "XAF Anim File (*.xaf)";
			suggest_ext += ".xaf";
			break;
		case FFSAVE_XML:
			caption += "XML File (*.xml)";
			suggest_ext += ".xml";
			break;
		case FFSAVE_RAW:
			caption += "RAW File (*.raw)";
			suggest_ext += ".raw";
			break;
		case FFSAVE_J2C:
			caption += "Compressed Images (*.j2c)";
			suggest_ext += ".j2c";
			break;
		default:;
			break;
		}
		
		gtk_window_set_title(GTK_WINDOW(picker), caption.c_str());

		if (!filename)
		{
			suggest_name += suggest_ext;

			gtk_file_selection_set_filename
				(GTK_FILE_SELECTION(picker),
				 g_filename_from_utf8(suggest_name.c_str(),
						      -1, NULL,
						      NULL,
						      NULL));
			gtk_editable_select_region(GTK_EDITABLE(GTK_FILE_SELECTION(picker)->selection_entry), 0, suggest_name.length() - suggest_ext.length() );
		}
		else
		{
			gtk_file_selection_set_filename
				(GTK_FILE_SELECTION(picker),
				 g_filename_from_utf8(filename,
						      -1, NULL,
						      NULL,
						      NULL));
			gtk_editable_select_region(GTK_EDITABLE(GTK_FILE_SELECTION(picker)->selection_entry), 0, -1 );
		}

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();
		rtn = (mStoreFilenames.fileVector.size() == 1);
	}

	gViewerWindow->mWindow->afterDialog();

	return rtn;
}

BOOL LLFilePicker::getOpenFile( ELoadFilter filter )
{
	BOOL rtn = FALSE;

	gViewerWindow->mWindow->beforeDialog();

	reset();
	GtkWindow* picker = buildFilePicker();

	if (picker)
	{
		std::string caption = "Load ";
		switch (filter)
		{
		case FFLOAD_WAV:
			caption += "Sounds (*.wav)"; break;
		case FFLOAD_ANIM:
			caption += "Animations (*.bvh)"; break;
		case FFLOAD_IMAGE:
			caption += "Images (*.tga; *.bmp; *.jpg; *.jpeg)"; break;
		default:;
			break;
		}
		
		gtk_window_set_title(GTK_WINDOW(picker), caption.c_str());

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();
		rtn = (mStoreFilenames.fileVector.size() == 1);
	}

	gViewerWindow->mWindow->afterDialog();

	return rtn;
}

BOOL LLFilePicker::getMultipleOpenFiles( ELoadFilter filter )
{
	BOOL rtn = FALSE;

	gViewerWindow->mWindow->beforeDialog();

	reset();
	GtkWindow* picker = buildFilePicker();

	if (picker)
	{
		gtk_file_selection_set_select_multiple(GTK_FILE_SELECTION(picker),
						       TRUE);

		gtk_window_set_title(GTK_WINDOW(picker), "Load Files");

		gtk_widget_show_all(GTK_WIDGET(picker));
		gtk_main();
		rtn = !mStoreFilenames.fileVector.empty();
	}

	gViewerWindow->mWindow->afterDialog();

	return rtn;
}

const char* LLFilePicker::getFirstFile()
{
	mNextFileIndex = 0;
	return getNextFile();
}

const char* LLFilePicker::getNextFile()
{
	if (mStoreFilenames.fileVector.size() > mNextFileIndex)
		return mStoreFilenames.fileVector[mNextFileIndex++].c_str();
	else
		return NULL;
}

const char* LLFilePicker::getDirname()
{
	// getDirname is badly named... it really means getBasename.
	S32 index = mNextFileIndex - 1; // want index before the 'next' cursor
	if (index >= 0 && index < (S32)mStoreFilenames.fileVector.size())
	{
		// we do this using C strings so we don't have to
		// convert a LLString/std::string character offset into a
		// byte-offset for the return (which is a C string anyway).
		const char* dirsep = gDirUtilp->getDirDelimiter().c_str();
		const char* fullpath = mStoreFilenames.fileVector[index].c_str();
		const char* finalpart = NULL;
		const char* thispart = fullpath;
		// walk through the string looking for the final dirsep, i.e. /
		do
		{
			thispart = strstr(thispart, dirsep);
			if (NULL != thispart)
				finalpart = thispart = &thispart[1];
		}
		while (NULL != thispart);
		return finalpart;
	}
	else
		return NULL;
}

void LLFilePicker::reset()
{
	llinfos << "GTK LLFilePicker::reset()" << llendl;
	mNextFileIndex = 0;
	mStoreFilenames.win = NULL;
	mStoreFilenames.fileVector.clear();
}

# else // LL_GTK

// Hacky stubs designed to facilitate fake getSaveFile and getOpenFile with
// static results, when we don't have a real filepicker.

static LLString hackyfilename;

LLFilePicker::LLFilePicker() 
{
	reset();
}

LLFilePicker::~LLFilePicker()
{
}

BOOL LLFilePicker::getSaveFile( ESaveFilter filter, const char* filename )
{
	llinfos << "getSaveFile suggested filename is [" << filename
		<< "]" << llendl;
	if (filename && filename[0])
	{
		hackyfilename.assign(gDirUtilp->getLindenUserDir());
		hackyfilename += gDirUtilp->getDirDelimiter();
		hackyfilename += filename;
		return TRUE;
	}
	hackyfilename.clear();
	return FALSE;
}

BOOL LLFilePicker::getOpenFile( ELoadFilter filter )
{
	// HACK: Static filenames for 'open' until we implement filepicker
	hackyfilename.assign(gDirUtilp->getLindenUserDir());
	hackyfilename += gDirUtilp->getDirDelimiter();
	hackyfilename += "upload";
	switch (filter)
	{
	case FFLOAD_WAV: hackyfilename += ".wav"; break;
	case FFLOAD_IMAGE: hackyfilename += ".tga"; break;
	case FFLOAD_ANIM: hackyfilename += ".bvh"; break;
	default: break;
	}
	llinfos << "getOpenFile: Will try to open file: " << hackyfilename
		<< llendl;
	return TRUE;
}

BOOL LLFilePicker::getMultipleOpenFiles( ELoadFilter filter )
{
	hackyfilename.clear();
	return FALSE;
}

const char* LLFilePicker::getFirstFile()
{
	if (!hackyfilename.empty())
	{
		return hackyfilename.c_str();
	}
	return NULL;
}

const char* LLFilePicker::getNextFile()
{
	hackyfilename.clear();
	return NULL;
}

const char* LLFilePicker::getDirname()
{
	return NULL;
}

void LLFilePicker::reset()
{
}
#endif // LL_GTK

#else // not implemented

LLFilePicker::LLFilePicker() 
{
	reset();
}

LLFilePicker::~LLFilePicker()
{
}

BOOL LLFilePicker::getSaveFile( ESaveFilter filter, const char* filename )
{
	return FALSE;
}

BOOL LLFilePicker::getOpenFile( ELoadFilter filter )
{
	return FALSE;
}

BOOL LLFilePicker::getMultipleOpenFiles( ELoadFilter filter )
{
	return FALSE;
}

const char* LLFilePicker::getFirstFile()
{
	return NULL;
}

const char* LLFilePicker::getNextFile()
{
	return NULL;
}

const char* LLFilePicker::getDirname()
{
	return NULL;
}

void LLFilePicker::reset()
{
}

#endif
