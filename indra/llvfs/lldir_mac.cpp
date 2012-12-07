/** 
 * @file lldir_mac.cpp
 * @brief Implementation of directory utilities for Mac OS X
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#if LL_DARWIN

#include "linden_common.h"

#include "lldir_mac.h"
#include "llerror.h"
#include "llrand.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>

#include <Carbon/Carbon.h>

// --------------------------------------------------------------------------------

static OSStatus CFCreateDirectory(FSRef	*parentRef, CFStringRef name, FSRef *newRef)
{
	OSStatus		result = noErr;
	HFSUniStr255	uniStr;
	
	uniStr.length = CFStringGetLength(name);
	CFStringGetCharacters(name, CFRangeMake(0, uniStr.length), uniStr.unicode);
	result = FSMakeFSRefUnicode(parentRef, uniStr.length, uniStr.unicode, kTextEncodingMacRoman, newRef);
	if (result != noErr)
	{
		result = FSCreateDirectoryUnicode(parentRef, uniStr.length, uniStr.unicode, 0, NULL, newRef, NULL, NULL);
	}

	return result;
}

// --------------------------------------------------------------------------------

static void CFStringRefToLLString(CFStringRef stringRef, std::string &llString, bool releaseWhenDone)
{
	if (stringRef)
	{
		long stringSize = CFStringGetLength(stringRef) + 1;
		long bufferSize = CFStringGetMaximumSizeForEncoding(stringSize,kCFStringEncodingUTF8);
		char* buffer = new char[bufferSize];
		memset(buffer, 0, bufferSize);
		if (CFStringGetCString(stringRef, buffer, bufferSize, kCFStringEncodingUTF8))
			llString = buffer;
		delete[] buffer;
		if (releaseWhenDone)
			CFRelease(stringRef);
	}
}

// --------------------------------------------------------------------------------

static void CFURLRefToLLString(CFURLRef urlRef, std::string &llString, bool releaseWhenDone)
{
	if (urlRef)
	{
		CFURLRef	absoluteURLRef = CFURLCopyAbsoluteURL(urlRef);
		if (absoluteURLRef)
		{
			CFStringRef	stringRef = CFURLCopyFileSystemPath(absoluteURLRef, kCFURLPOSIXPathStyle);
			CFStringRefToLLString(stringRef, llString, true);
			CFRelease(absoluteURLRef);
		}
		if (releaseWhenDone)
			CFRelease(urlRef);
	}
}

// --------------------------------------------------------------------------------

static void FSRefToLLString(FSRef *fsRef, std::string &llString)
{
	OSStatus	error = noErr;
	char		path[MAX_PATH];
	
	error = FSRefMakePath(fsRef, (UInt8*) path, sizeof(path));
	if (error == noErr)
		llString = path;
}

// --------------------------------------------------------------------------------

LLDir_Mac::LLDir_Mac()
{
	mDirDelimiter = "/";
	mCurrentDirIndex = -1;
	mCurrentDirCount = -1;
	
	CFBundleRef		mainBundleRef = NULL;
	CFURLRef		executableURLRef = NULL;
	CFStringRef		stringRef = NULL;
	OSStatus		error = noErr;
	FSRef			fileRef;
	CFStringRef		secondLifeString = CFSTR("SecondLife");
	
	mainBundleRef = CFBundleGetMainBundle();
		
	executableURLRef = CFBundleCopyExecutableURL(mainBundleRef);
	
	if (executableURLRef != NULL)
	{
		// mExecutablePathAndName
		CFURLRefToLLString(executableURLRef, mExecutablePathAndName, false);
		
		// mExecutableFilename
		stringRef = CFURLCopyLastPathComponent(executableURLRef);
		CFStringRefToLLString(stringRef, mExecutableFilename, true);
		
		// mExecutableDir
		CFURLRef	executableParentURLRef = CFURLCreateCopyDeletingLastPathComponent(NULL, executableURLRef);
		CFURLRefToLLString(executableParentURLRef, mExecutableDir, true);
		
		// mAppRODataDir

		
		// *NOTE: When running in a dev tree, use the copy of
		// skins in indra/newview/ rather than in the application bundle.  This
		// mirrors Windows dev environment behavior and allows direct checkin
		// of edited skins/xui files. JC
		
		// MBW -- This keeps the mac application from finding other things.
		// If this is really for skins, it should JUST apply to skins.

		CFURLRef resourcesURLRef = CFBundleCopyResourcesDirectoryURL(mainBundleRef);
		CFURLRefToLLString(resourcesURLRef, mAppRODataDir, true);
		
		U32 build_dir_pos = mExecutableDir.rfind("/build-darwin-");
		if (build_dir_pos != std::string::npos)
		{
			// ...we're in a dev checkout
			mSkinBaseDir = mExecutableDir.substr(0, build_dir_pos)
				+ "/indra/newview/skins";
			llinfos << "Running in dev checkout with mSkinBaseDir "
				<< mSkinBaseDir << llendl;
		}
		else
		{
			// ...normal installation running
			mSkinBaseDir = mAppRODataDir + mDirDelimiter + "skins";
		}
		
		// mOSUserDir
		error = FSFindFolder(kUserDomain, kApplicationSupportFolderType, true, &fileRef);
		if (error == noErr)
		{
			FSRef	newFileRef;
			
			// Create the directory
			error = CFCreateDirectory(&fileRef, secondLifeString, &newFileRef);
			if (error == noErr)
			{
				// Save the full path to the folder
				FSRefToLLString(&newFileRef, mOSUserDir);
				
				// Create our sub-dirs
				(void) CFCreateDirectory(&newFileRef, CFSTR("data"), NULL);
				//(void) CFCreateDirectory(&newFileRef, CFSTR("cache"), NULL);
				(void) CFCreateDirectory(&newFileRef, CFSTR("logs"), NULL);
				(void) CFCreateDirectory(&newFileRef, CFSTR("user_settings"), NULL);
				(void) CFCreateDirectory(&newFileRef, CFSTR("browser_profile"), NULL);
			}
		}
		
		//mOSCacheDir
		FSRef cacheDirRef;
		error = FSFindFolder(kUserDomain, kCachedDataFolderType, true, &cacheDirRef);
		if (error == noErr)
		{
			FSRefToLLString(&cacheDirRef, mOSCacheDir);
			(void)CFCreateDirectory(&cacheDirRef, CFSTR("SecondLife"),NULL);
		}
		
		// mOSUserAppDir
		mOSUserAppDir = mOSUserDir;
		
		// mTempDir
		error = FSFindFolder(kOnAppropriateDisk, kTemporaryFolderType, true, &fileRef);
		if (error == noErr)
		{
			FSRef	tempRef;
			error = CFCreateDirectory(&fileRef, secondLifeString, &tempRef);
			if (error == noErr)
				FSRefToLLString(&tempRef, mTempDir);
		}
		
		mWorkingDir = getCurPath();

		mLLPluginDir = mAppRODataDir + mDirDelimiter + "llplugin";
				
		CFRelease(executableURLRef);
		executableURLRef = NULL;
	}
}

LLDir_Mac::~LLDir_Mac()
{
}

// Implementation


void LLDir_Mac::initAppDirs(const std::string &app_name,
							const std::string& app_read_only_data_dir)
{
	// Allow override so test apps can read newview directory
	if (!app_read_only_data_dir.empty())
	{
		mAppRODataDir = app_read_only_data_dir;
		mSkinBaseDir = mAppRODataDir + mDirDelimiter + "skins";
	}
	mCAFile = getExpandedFilename(LL_PATH_APP_SETTINGS, "CA.pem");

	//dumpCurrentDirectories();
}

U32 LLDir_Mac::countFilesInDir(const std::string &dirname, const std::string &mask)
{
	U32 file_count = 0;
	glob_t g;

	std::string tmp_str;
	tmp_str = dirname;
	tmp_str += mask;
	
	if(glob(tmp_str.c_str(), GLOB_NOSORT, NULL, &g) == 0)
	{
		file_count = g.gl_pathc;

		globfree(&g);
	}

	return (file_count);
}

std::string LLDir_Mac::getCurPath()
{
	char tmp_str[LL_MAX_PATH];	/* Flawfinder: ignore */ 
	getcwd(tmp_str, LL_MAX_PATH);
	return tmp_str;
}



BOOL LLDir_Mac::fileExists(const std::string &filename) const
{
	struct stat stat_data;
	// Check the age of the file
	// Now, we see if the files we've gathered are recent...
	int res = stat(filename.c_str(), &stat_data);
	if (!res)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


/*virtual*/ std::string LLDir_Mac::getLLPluginLauncher()
{
	return gDirUtilp->getAppRODataDir() + gDirUtilp->getDirDelimiter() +
		"SLPlugin.app/Contents/MacOS/SLPlugin";
}

/*virtual*/ std::string LLDir_Mac::getLLPluginFilename(std::string base_name)
{
	return gDirUtilp->getLLPluginDir() + gDirUtilp->getDirDelimiter() +
		base_name + ".dylib";
}


#endif // LL_DARWIN
