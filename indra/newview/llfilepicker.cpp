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

#if LL_LINUX || LL_SOLARIS
#include "llhttpconstants.h"    // file picker uses some of thes constants on Linux
#endif

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

#ifdef LL_DARWIN
#include "llfilepicker_mac.h"
//#include <boost/algorithm/string/predicate.hpp>
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
    case FFLOAD_EXE:
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
	case FFSAVE_TGAPNG:
		if (filename.empty())
		{
			wcsncpy( mFilesW,L"untitled.png", FILENAME_BUFFER_SIZE);	/*Flawfinder: ignore*/
			//PNG by default
		}
		mOFN.lpstrDefExt = L"png";
		mOFN.lpstrFilter =
			L"PNG Images (*.png)\0*.png\0" \
			L"Targa Images (*.tga)\0*.tga\0" \
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
		try
		{
			success = GetSaveFileName(&mOFN);
			if (success)
			{
				std::string filename = utf16str_to_utf8str(llutf16string(mFilesW));
				mFiles.push_back(filename);
			}
		}
		catch (...)
		{
			LOG_UNHANDLED_EXCEPTION("");
		}
		gKeyboard->resetKeys();
	}
	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}

#elif LL_DARWIN

std::vector<std::string>* LLFilePicker::navOpenFilterProc(ELoadFilter filter) //(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode)
{
    std::vector<std::string> *allowedv = new std::vector< std::string >;
    switch(filter)
    {
        case FFLOAD_ALL:
            allowedv->push_back("wav");
            allowedv->push_back("bvh");
            allowedv->push_back("anim");
            allowedv->push_back("dae");
            allowedv->push_back("raw");
            allowedv->push_back("lsl");
            allowedv->push_back("dic");
            allowedv->push_back("xcu");
            allowedv->push_back("gif");
        case FFLOAD_IMAGE:
            allowedv->push_back("jpg");
            allowedv->push_back("jpeg");
            allowedv->push_back("bmp");
            allowedv->push_back("tga");
            allowedv->push_back("bmpf");
            allowedv->push_back("tpic");
            allowedv->push_back("png");
            break;
        case FFLOAD_EXE:
            allowedv->push_back("app");
            allowedv->push_back("exe");
            break;
        case FFLOAD_WAV:
            allowedv->push_back("wav");
            break;
        case FFLOAD_ANIM:
            allowedv->push_back("bvh");
            allowedv->push_back("anim");
            break;
        case FFLOAD_COLLADA:
            allowedv->push_back("dae");
            break;
#ifdef _CORY_TESTING
        case FFLOAD_GEOMETRY:
            allowedv->push_back("slg");
            break;
#endif
        case FFLOAD_XML:
            allowedv->push_back("xml");
            break;
        case FFLOAD_RAW:
            allowedv->push_back("raw");
            break;
        case FFLOAD_SCRIPT:
            allowedv->push_back("lsl");
            break;
        case FFLOAD_DICTIONARY:
            allowedv->push_back("dic");
            allowedv->push_back("xcu");
            break;
        case FFLOAD_DIRECTORY:
            break;
        default:
            LL_WARNS() << "Unsupported format." << LL_ENDL;
    }

	return allowedv;
}

bool	LLFilePicker::doNavChooseDialog(ELoadFilter filter)
{
	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return false;
	}
    
	gViewerWindow->getWindow()->beforeDialog();
    
    std::vector<std::string> *allowed_types=navOpenFilterProc(filter);
    
    std::vector<std::string> *filev  = doLoadDialog(allowed_types, 
                                                    mPickOptions);

	gViewerWindow->getWindow()->afterDialog();


    if (filev && filev->size() > 0)
    {
        mFiles.insert(mFiles.end(), filev->begin(), filev->end());
        return true;
    }
	
	return false;
}

bool	LLFilePicker::doNavSaveDialog(ESaveFilter filter, const std::string& filename)
{
	
	// Setup the type, creator, and extension
    std::string		extension, type, creator;
    
	switch (filter)
	{
		case FFSAVE_WAV:
			type = "WAVE";
			creator = "TVOD";
			extension = "wav";
			break;
		case FFSAVE_TGA:
			type = "TPIC";
			creator = "prvw";
			extension = "tga";
			break;
		case FFSAVE_TGAPNG:
			type = "PNG";
			creator = "prvw";
			extension = "png,tga";
			break;
		case FFSAVE_BMP:
			type = "BMPf";
			creator = "prvw";
			extension = "bmp";
			break;
		case FFSAVE_JPEG:
			type = "JPEG";
			creator = "prvw";
			extension = "jpeg";
			break;
		case FFSAVE_PNG:
			type = "PNG ";
			creator = "prvw";
			extension = "png";
			break;
		case FFSAVE_AVI:
			type = "\?\?\?\?";
			creator = "\?\?\?\?";
			extension = "mov";
			break;

		case FFSAVE_ANIM:
			type = "\?\?\?\?";
			creator = "\?\?\?\?";
			extension = "xaf";
			break;

#ifdef _CORY_TESTING
		case FFSAVE_GEOMETRY:
			type = "\?\?\?\?";
			creator = "\?\?\?\?";
			extension = "slg";
			break;
#endif	
			
		case FFSAVE_XML:
			type = "\?\?\?\?";
			creator = "\?\?\?\?";
			extension = "xml";
			break;
			
		case FFSAVE_RAW:
			type = "\?\?\?\?";
			creator = "\?\?\?\?";
			extension = "raw";
			break;

		case FFSAVE_J2C:
			type = "\?\?\?\?";
			creator = "prvw";
			extension = "j2c";
			break;
		
		case FFSAVE_SCRIPT:
			type = "LSL ";
			creator = "\?\?\?\?";
			extension = "lsl";
			break;
		
		case FFSAVE_ALL:
		default:
			type = "\?\?\?\?";
			creator = "\?\?\?\?";
			extension = "";
			break;
	}
	
    std::string namestring = filename;
    if (namestring.empty()) namestring="Untitled";
    
//    if (! boost::algorithm::ends_with(namestring, extension) )
//    {
//        namestring = namestring + "." + extension;
//        
//    }
    
	gViewerWindow->getWindow()->beforeDialog();

	// Run the dialog
    std::string* filev = doSaveDialog(&namestring, 
                 &type,
                 &creator,
                 &extension,
                 mPickOptions);

	gViewerWindow->getWindow()->afterDialog();

	if ( filev && !filev->empty() )
	{
        mFiles.push_back(*filev);
		return true;
    }
	
	return false;
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

	reset();
	
    mPickOptions &= ~F_MULTIPLE;
    mPickOptions |= F_FILE;
 
    if (filter == FFLOAD_DIRECTORY) //This should only be called from lldirpicker. 
    {

        mPickOptions |= ( F_NAV_SUPPORT | F_DIRECTORY );
        mPickOptions &= ~F_FILE;
    }

	if (filter == FFLOAD_ALL)	// allow application bundles etc. to be traversed; important for DEV-16869, but generally useful
	{
        mPickOptions |= F_NAV_SUPPORT;
	}
	
	if (blocking)
	{
		// Modal, so pause agent
		send_agent_pause();
	}


	success = doNavChooseDialog(filter);
		
	if (success)
	{
		if (!getFileCount())
			success = false;
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

	reset();
    
    mPickOptions |= F_FILE;

    mPickOptions |= F_MULTIPLE;
	// Modal, so pause agent
	send_agent_pause();
    
	success = doNavChooseDialog(filter);
    
    send_agent_resume();
    
	if (success)
	{
		if (!getFileCount())
			success = false;
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
		return false;
	BOOL success = false;

	// if local file browsing is turned off, return without opening dialog
	if ( check_local_file_access_enabled() == false )
	{
		return false;
	}

	reset();
	
    mPickOptions &= ~F_MULTIPLE;

	// Modal, so pause agent
	send_agent_pause();

    success = doNavSaveDialog(filter, filename);

    if (success)
	{
		if (!getFileCount())
			success = false;
	}

	send_agent_resume();

	// Account for the fact that the app has been stalled.
	LLFrameTimer::updateFrameTime();
	return success;
}
//END LL_DARWIN

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
		LL_WARNS() << "g_filename_to_utf8 failed on \"" << display_name << "\": " << error->message << LL_ENDL;
	}

	if (filename_utf8)
	{
		picker->mFiles.push_back(std::string(filename_utf8));
		LL_DEBUGS() << "ADDED FILE " << filename_utf8 << LL_ENDL;
		g_free(filename_utf8);
	}

	setlocale(LC_ALL, saved_locale.c_str());
}

// static
void LLFilePicker::chooser_responder(GtkWidget *widget, gint response, gpointer user_data)
{
	LLFilePicker* picker = (LLFilePicker*)user_data;

	LL_DEBUGS() << "GTK DIALOG RESPONSE " << response << LL_ENDL;

	if (response == GTK_RESPONSE_ACCEPT)
	{
		GSList *file_list = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(widget));
		g_slist_foreach(file_list, (GFunc)add_to_selectedfiles, user_data);
		g_slist_foreach(file_list, (GFunc)g_free, NULL);
		g_slist_free (file_list);
	}

	// let's save the extension of the last added file(considering current filter)
	GtkFileFilter *gfilter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(widget));
	if(gfilter)
	{
		std::string filter = gtk_file_filter_get_name(gfilter);

		if(filter == LLTrans::getString("png_image_files"))
		{
			picker->mCurrentExtension = ".png";
		}
		else if(filter == LLTrans::getString("targa_image_files"))
		{
			picker->mCurrentExtension = ".tga";
		}
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
			LL_WARNS() << "Hmm, couldn't get xwid to use for transient." << LL_ENDL;
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

static std::string add_xml_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_pattern_filter_to_gtkchooser(picker,  "*.xml",
												   LLTrans::getString("xml_files") + " (*.xml)");
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
	gtk_file_filter_add_mime_type(gfilter, HTTP_CONTENT_IMAGE_JPEG.c_str());
	gtk_file_filter_add_mime_type(gfilter, HTTP_CONTENT_IMAGE_PNG.c_str());
	gtk_file_filter_add_mime_type(gfilter, HTTP_CONTENT_IMAGE_BMP.c_str());
	std::string filtername = LLTrans::getString("image_files") + " (*.tga; *.bmp; *.jpg; *.png)";
	add_common_filters_to_gtkchooser(gfilter, picker, filtername);
	return filtername;
}
 
static std::string add_script_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_mime_filter_to_gtkchooser(picker,  HTTP_CONTENT_TEXT_PLAIN,
							LLTrans::getString("script_files") + " (*.lsl)");
}

static std::string add_dictionary_filter_to_gtkchooser(GtkWindow *picker)
{
	return add_simple_mime_filter_to_gtkchooser(picker, HTTP_CONTENT_TEXT_PLAIN,
							LLTrans::getString("dictionary_files") + " (*.dic; *.xcu)");
}

static std::string add_save_texture_filter_to_gtkchooser(GtkWindow *picker)
{
	GtkFileFilter *gfilter_tga = gtk_file_filter_new();
	GtkFileFilter *gfilter_png = gtk_file_filter_new();

	gtk_file_filter_add_pattern(gfilter_tga, "*.tga");
	gtk_file_filter_add_mime_type(gfilter_png, "image/png");
	std::string caption = LLTrans::getString("save_texture_image_files") + " (*.tga; *.png)";
	gtk_file_filter_set_name(gfilter_tga, LLTrans::getString("targa_image_files").c_str());
	gtk_file_filter_set_name(gfilter_png, LLTrans::getString("png_image_files").c_str());

	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker),
					gfilter_png);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(picker),
					gfilter_tga);
	return caption;
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
				(picker, HTTP_CONTENT_IMAGE_BMP, LLTrans::getString("bitmap_image_files") + " (*.bmp)");
			suggest_ext = ".bmp";
			break;
		case FFSAVE_PNG:
			caption += add_simple_mime_filter_to_gtkchooser
				(picker, "image/png", LLTrans::getString("png_image_files") + " (*.png)");
			suggest_ext = ".png";
			break;
		case FFSAVE_TGAPNG:
			caption += add_save_texture_filter_to_gtkchooser(picker);
			suggest_ext = ".png";
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
			// *TODO: Should this be 'image/j2c' ?
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

		if(rtn && filter == FFSAVE_TGAPNG)
		{
			std::string selected_file = mFiles.back();
			mFiles.pop_back();
			mFiles.push_back(selected_file + mCurrentExtension);
		}
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
		case FFLOAD_XML:
			filtername = add_xml_filter_to_gtkchooser(picker);
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
	
	LL_INFOS() << "getSaveFile suggested filename is [" << filename
		<< "]" << LL_ENDL;
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
	LL_INFOS() << "getOpenFile: Will try to open file: " << filename << LL_ENDL;
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

#endif // LL_LINUX || LL_SOLARIS
