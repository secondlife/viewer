/** 
 * @file lldir_win32.cpp
 * @brief Implementation of directory utilities for windows
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#if LL_WINDOWS

#include "linden_common.h"

#include "lldir_win32.h"
#include "llerror.h"
#include "llrand.h"		// for gLindenLabRandomNumber
#include "shlobj.h"

#include <direct.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

// Utility stuff to get versions of the sh
#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD GetDllVersion(LPCTSTR lpszDllName);

LLDir_Win32::LLDir_Win32()
{
	mDirDelimiter = "\\";

	WCHAR w_str[MAX_PATH];

	// Application Data is where user settings go
	SHGetSpecialFolderPath(NULL, w_str, CSIDL_APPDATA, TRUE);

	mOSUserDir = utf16str_to_utf8str(llutf16string(w_str));

	// Local Settings\Application Data is where cache files should
	// go, they don't get copied to the server if the user moves his
	// profile around on the network. JC
	//
	// TODO: patch the installer to remove old cache files on update, then
	// enable this code.
	//SHGetSpecialFolderPath(NULL, w_str, CSIDL_LOCAL_APPDATA, TRUE);
	//mOSUserCacheDir = utf16str_to_utf8str(llutf16string(w_str));

	if (GetTempPath(MAX_PATH, w_str))
	{
		if (wcslen(w_str))	/* Flawfinder: ignore */ 
		{
			w_str[wcslen(w_str)-1] = '\0'; /* Flawfinder: ignore */ // remove trailing slash
		}
		mTempDir = utf16str_to_utf8str(llutf16string(w_str));
	}
	else
	{
		mTempDir = mOSUserDir;
	}

//	fprintf(stderr, "mTempDir = <%s>",mTempDir);

#if 1
	// Don't use the real app path for now, as we'll have to add parsing to detect if
	// we're in a developer tree, which has a different structure from the installed product.

	S32 size = GetModuleFileName(NULL, w_str, MAX_PATH);
	if (size)
	{
		w_str[size] = '\0';
		mExecutablePathAndName = utf16str_to_utf8str(llutf16string(w_str));
		S32 path_end = mExecutablePathAndName.find_last_of('\\');
		if (path_end != std::string::npos)
		{
			mExecutableDir = mExecutablePathAndName.substr(0, path_end);
			mExecutableFilename = mExecutablePathAndName.substr(path_end+1, std::string::npos);
		}
		else
		{
			mExecutableFilename = mExecutablePathAndName;
		}
		GetCurrentDirectory(MAX_PATH, w_str);
		mWorkingDir = utf16str_to_utf8str(llutf16string(w_str));

	}
	else
	{
		fprintf(stderr, "Couldn't get APP path, assuming current directory!");
		GetCurrentDirectory(MAX_PATH, w_str);
		mExecutableDir = utf16str_to_utf8str(llutf16string(w_str));
		// Assume it's the current directory
	}
#else
	GetCurrentDirectory(MAX_PATH, w_str);
	mExecutableDir = utf16str_to_utf8str(llutf16string(w_str));
#endif
	
	mAppRODataDir = getCurPath();
	// *FIX:Mani - The following is the old way we did things. I'm keeping this around
	// in case there is some really good reason to make mAppRODataDir == mExecutableDir
	/*
	if (strstr(mExecutableDir.c_str(), "indra\\newview"))
		mAppRODataDir = getCurPath();
	else
		mAppRODataDir = mExecutableDir;
	*/
}

LLDir_Win32::~LLDir_Win32()
{
}

// Implementation

void LLDir_Win32::initAppDirs(const std::string &app_name)
{
	mAppName = app_name;
	mOSUserAppDir = mOSUserDir;
	mOSUserAppDir += "\\";
	mOSUserAppDir += app_name;

	int res = LLFile::mkdir(mOSUserAppDir);
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create app user dir " << mOSUserAppDir << llendl;
			llwarns << "Default to base dir" << mOSUserDir << llendl;
			mOSUserAppDir = mOSUserDir;
		}
	}
	//dumpCurrentDirectories();

	res = LLFile::mkdir(getExpandedFilename(LL_PATH_LOGS,""));
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create LL_PATH_LOGS dir " << getExpandedFilename(LL_PATH_LOGS,"") << llendl;
		}
	}
	
	res = LLFile::mkdir(getExpandedFilename(LL_PATH_USER_SETTINGS,""));
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create LL_PATH_USER_SETTINGS dir " << getExpandedFilename(LL_PATH_USER_SETTINGS,"") << llendl;
		}
	}
	
	res = LLFile::mkdir(getExpandedFilename(LL_PATH_CACHE,""));
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create LL_PATH_CACHE dir " << getExpandedFilename(LL_PATH_CACHE,"") << llendl;
		}
	}
	
	res = LLFile::mkdir(getExpandedFilename(LL_PATH_MOZILLA_PROFILE,""));
	if (res == -1)
	{
		if (errno != EEXIST)
		{
			llwarns << "Couldn't create LL_PATH_MOZILLA_PROFILE dir " << getExpandedFilename(LL_PATH_MOZILLA_PROFILE,"") << llendl;
		}
	}
	
	mCAFile = getExpandedFilename(LL_PATH_APP_SETTINGS, "CA.pem");
}

U32 LLDir_Win32::countFilesInDir(const std::string &dirname, const std::string &mask)
{
	HANDLE count_search_h;
	U32 file_count;

	file_count = 0;

	WIN32_FIND_DATA FileData;

	llutf16string pathname = utf8str_to_utf16str(dirname);
	pathname += utf8str_to_utf16str(mask);
	
	if ((count_search_h = FindFirstFile(pathname.c_str(), &FileData)) != INVALID_HANDLE_VALUE)   
	{
		file_count++;

		while (FindNextFile(count_search_h, &FileData))
		{
			file_count++;
		}
		   
		FindClose(count_search_h);
	}

	return (file_count);
}


// get the next file in the directory
// automatically wrap if we've hit the end
BOOL LLDir_Win32::getNextFileInDir(const std::string &dirname, const std::string &mask, std::string &fname, BOOL wrap)
{
	llutf16string dirnamew = utf8str_to_utf16str(dirname);
	return getNextFileInDir(dirnamew, mask, fname, wrap);

}

BOOL LLDir_Win32::getNextFileInDir(const llutf16string &dirname, const std::string &mask, std::string &fname, BOOL wrap)
{
	WIN32_FIND_DATAW FileData;

	fname = "";
	llutf16string pathname = dirname;
	pathname += utf8str_to_utf16str(mask);

	if (pathname != mCurrentDir)
	{
		// different dir specified, close old search
		if (mCurrentDir[0])
		{
			FindClose(mDirSearch_h);
		}
		mCurrentDir = pathname;

		// and open new one
		// Check error opening Directory structure
		if ((mDirSearch_h = FindFirstFile(pathname.c_str(), &FileData)) == INVALID_HANDLE_VALUE)   
		{
//			llinfos << "Unable to locate first file" << llendl;
			return(FALSE);
		}
	}
	else // get next file in list
	{
		// Find next entry
		if (!FindNextFile(mDirSearch_h, &FileData))
		{
			if (GetLastError() == ERROR_NO_MORE_FILES)
			{
                // No more files, so reset to beginning of directory
				FindClose(mDirSearch_h);
				mCurrentDir[0] = NULL;

				if (wrap)
				{
					return(getNextFileInDir(pathname,"",fname,TRUE));
				}
				else
				{
					fname[0] = 0;
					return(FALSE);
				}
			}
			else
			{
				// Error
//				llinfos << "Unable to locate next file" << llendl;
				return(FALSE);
			}
		}
	}

	// convert from TCHAR to char
	fname = utf16str_to_utf8str(FileData.cFileName);
	
	// fname now first name in list
	return(TRUE);
}


// get a random file in the directory
// automatically wrap if we've hit the end
void LLDir_Win32::getRandomFileInDir(const std::string &dirname, const std::string &mask, std::string &fname)
{
	S32 num_files;
	S32 which_file;
	HANDLE random_search_h;

	fname = "";

	llutf16string pathname = utf8str_to_utf16str(dirname);
	pathname += utf8str_to_utf16str(mask);

	WIN32_FIND_DATA FileData;
	fname[0] = NULL;

	num_files = countFilesInDir(dirname,mask);
	if (!num_files)
	{
		return;
	}

	which_file = ll_rand(num_files);

//	llinfos << "Random select mp3 #" << which_file << llendl;

    // which_file now indicates the (zero-based) index to which file to play

	if ((random_search_h = FindFirstFile(pathname.c_str(), &FileData)) != INVALID_HANDLE_VALUE)   
	{
		while (which_file--)
		{
			if (!FindNextFile(random_search_h, &FileData))
			{
				return;
			}
		}		   
		FindClose(random_search_h);

		fname = utf16str_to_utf8str(llutf16string(FileData.cFileName));
	}
}

std::string LLDir_Win32::getCurPath()
{
	WCHAR w_str[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, w_str);

	return utf16str_to_utf8str(llutf16string(w_str));
}


BOOL LLDir_Win32::fileExists(const std::string &filename) const
{
	llstat stat_data;
	// Check the age of the file
	// Now, we see if the files we've gathered are recent...
	int res = LLFile::stat(filename, &stat_data);
	if (!res)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


#if 0
// Utility function to get version number of a DLL

#define PACKVERSION(major,minor) MAKELONG(minor,major)

DWORD GetDllVersion(LPCTSTR lpszDllName)
{

    HINSTANCE hinstDll;
    DWORD dwVersion = 0;

    hinstDll = LoadLibrary(lpszDllName);	/* Flawfinder: ignore */ 
	
    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;

        pDllGetVersion = (DLLGETVERSIONPROC) GetProcAddress(hinstDll, "DllGetVersion");

/*Because some DLLs might not implement this function, you
  must test for it explicitly. Depending on the particular 
  DLL, the lack of a DllGetVersion function can be a useful
  indicator of the version.
*/
        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
                dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }
        
        FreeLibrary(hinstDll);
    }
    return dwVersion;
}
#endif

#endif


