/** 
 * @file lldir_win32.cpp
 * @brief Implementation of directory utilities for windows
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

#if LL_WINDOWS

#include "linden_common.h"

#include "lldir_win32.h"
#include "llerror.h"
#include "llrand.h"		// for gLindenLabRandomNumber
#include <shlobj.h>
#include <fstream>

#include <direct.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

// Utility stuff to get versions of the sh
#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD GetDllVersion(LPCTSTR lpszDllName);

LLDir_Win32::LLDir_Win32()
{
	// set this first: used by append() and add() methods
	mDirDelimiter = "\\";

	WCHAR w_str[MAX_PATH];
	// Application Data is where user settings go. We rely on $APPDATA being
	// correct; in fact the VMP makes a point of setting it properly, since
	// Windows itself botches the job for non-ASCII usernames (MAINT-8087).
	mOSUserDir = ll_safe_string(getenv("APPDATA"));
	// On Windows, it's a Bad Thing if a pathname contains ASCII question
	// marks. In our experience, it means that the original pathname contained
	// non-ASCII characters that were munged to '?' somewhere along the way.
	// Convert to LLWString first, though, in case one of the bytes in a
	// non-ASCII UTF-8 string accidentally resembles '?'.
	if (utf8string_to_wstring(mOSUserDir).find(llwchar('?')) != LLWString::npos)
	{
		// It is really unclear what we should do if the following call fails.
		// We use it, among other things, to find where to put our log file!
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, w_str)))
		{
			// But of course, only update mOSUserDir if SHGetFolderPathW() works.
			mOSUserDir = utf16str_to_utf8str(llutf16string(w_str));
			// Not only that: update our environment so that child processes
			// will see a reasonable value as well. Use _putenv_s() rather
			// than _wputenv_s() because WE want to control the encoding with
			// which APPDATA is passed to child processes, instead of letting
			// somebody else pick it.
			_putenv_s("APPDATA", mOSUserDir.c_str());
			// SL-10153: It is really tempting to make the above _putenv_s()
			// call unconditional, since we've observed cases in which the
			// parent viewer receives a valid non-ASCII APPDATA value while
			// the child SLVersionChecker process receives one containing
			// question marks. But if what we see is already valid, what do we
			// gain by storing it again?
		}
	}

	// We want cache files to go on the local disk, even if the
	// user is on a network with a "roaming profile".
	//
	// On Vista this is:
	//   C:\Users\James\AppData\Local
	//
	// We used to store the cache in AppData\Roaming, and the installer
	// cleans up that version on upgrade.  JC
	mOSCacheDir = ll_safe_string(getenv("LOCALAPPDATA"));
	// Windows really does not deal well with pathnames containing non-ASCII
	// characters. See above remarks about APPDATA.
	if (utf8string_to_wstring(mOSCacheDir).find(llwchar('?')) != LLWString::npos)
	{
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, w_str)))
		{
			// But of course, only update mOSCacheDir if SHGetFolderPathW() works.
			mOSCacheDir = utf16str_to_utf8str(llutf16string(w_str));
			// Update our environment so that child processes will see a
			// reasonable value as well.
			_putenv_s("LOCALAPPDATA", mOSCacheDir.c_str());
		}
	}

	if (GetTempPath(MAX_PATH, w_str))
	{
		if (wcslen(w_str))	/* Flawfinder: ignore */ 
		{
			w_str[wcslen(w_str)-1] = '\0'; /* Flawfinder: ignore */ // remove trailing slash
		}
		mTempDir = utf16str_to_utf8str(llutf16string(w_str));

		if (mOSUserDir.empty())
		{
			mOSUserDir = mTempDir;
		}

		if (mOSCacheDir.empty())
		{
			mOSCacheDir = mTempDir;
		}
	}
	else
	{
		mTempDir = mOSUserDir;
	}

/*==========================================================================*|
	// Now that we've got mOSUserDir, one way or another, let's see how we did
	// with our environment variables.
	{
		auto report = [this](std::ostream& out){
			out << "mOSUserDir  = '" << mOSUserDir  << "'\n"
				<< "mOSCacheDir = '" << mOSCacheDir << "'\n"
				<< "mTempDir    = '" << mTempDir    << "'" << std::endl;
		};
		int res = LLFile::mkdir(mOSUserDir);
		if (res == -1)
		{
			// If we couldn't even create the directory, just blurt to stderr
			report(std::cerr);
		}
		else
		{
			// successfully created logdir, plunk a log file there
			std::string logfilename(add(mOSUserDir, "lldir.log"));
			std::ofstream logfile(logfilename.c_str());
			if (! logfile.is_open())
			{
				report(std::cerr);
			}
			else
			{
				report(logfile);
			}
		}
	}
|*==========================================================================*/

//	fprintf(stderr, "mTempDir = <%s>",mTempDir);

	// Set working directory, for LLDir::getWorkingDir()
	GetCurrentDirectory(MAX_PATH, w_str);
	mWorkingDir = utf16str_to_utf8str(llutf16string(w_str));

	// Set the executable directory
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

	}
	else
	{
		fprintf(stderr, "Couldn't get APP path, assuming current directory!");
		mExecutableDir = mWorkingDir;
		// Assume it's the current directory
	}

	// mAppRODataDir = ".";	

	// Determine the location of the App-Read-Only-Data
	// Try the working directory then the exe's dir.
	mAppRODataDir = mWorkingDir;	


//	if (mExecutableDir.find("indra") == std::string::npos)
	
	// *NOTE:Mani - It is a mistake to put viewer specific code in
	// the LLDir implementation. The references to 'skins' and 
	// 'llplugin' need to go somewhere else.
	// alas... this also gets called during static initialization 
	// time due to the construction of gDirUtil in lldir.cpp.
	if(! LLFile::isdir(add(mAppRODataDir, "skins")))
	{
		// What? No skins in the working dir?
		// Try the executable's directory.
		mAppRODataDir = mExecutableDir;
	}

//	LL_INFOS() << "mAppRODataDir = " << mAppRODataDir << LL_ENDL;

	mSkinBaseDir = add(mAppRODataDir, "skins");

	// Build the default cache directory
	mDefaultCacheDir = buildSLOSCacheDir();
	
	// Make sure it exists
	int res = LLFile::mkdir(mDefaultCacheDir);
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create LL_PATH_CACHE dir " << mDefaultCacheDir << LL_ENDL;
	}

	mLLPluginDir = add(mExecutableDir, "llplugin");
}

LLDir_Win32::~LLDir_Win32()
{
}

// Implementation

void LLDir_Win32::initAppDirs(const std::string &app_name,
							  const std::string& app_read_only_data_dir)
{
	// Allow override so test apps can read newview directory
	if (!app_read_only_data_dir.empty())
	{
		mAppRODataDir = app_read_only_data_dir;
		mSkinBaseDir = add(mAppRODataDir, "skins");
	}
	mAppName = app_name;
	mOSUserAppDir = add(mOSUserDir, app_name);

	int res = LLFile::mkdir(mOSUserAppDir);
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create app user dir " << mOSUserAppDir << LL_ENDL;
		LL_WARNS() << "Default to base dir" << mOSUserDir << LL_ENDL;
		mOSUserAppDir = mOSUserDir;
	}
	//dumpCurrentDirectories();

	res = LLFile::mkdir(getExpandedFilename(LL_PATH_LOGS,""));
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create LL_PATH_LOGS dir " << getExpandedFilename(LL_PATH_LOGS,"") << LL_ENDL;
	}

	res = LLFile::mkdir(getExpandedFilename(LL_PATH_USER_SETTINGS,""));
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create LL_PATH_USER_SETTINGS dir " << getExpandedFilename(LL_PATH_USER_SETTINGS,"") << LL_ENDL;
	}

	res = LLFile::mkdir(getExpandedFilename(LL_PATH_CACHE,""));
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create LL_PATH_CACHE dir " << getExpandedFilename(LL_PATH_CACHE,"") << LL_ENDL;
	}

	mCAFile = getExpandedFilename( LL_PATH_EXECUTABLE, "ca-bundle.crt" );
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

std::string LLDir_Win32::getCurPath()
{
	WCHAR w_str[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, w_str);

	return utf16str_to_utf8str(llutf16string(w_str));
}


bool LLDir_Win32::fileExists(const std::string &filename) const
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


/*virtual*/ std::string LLDir_Win32::getLLPluginLauncher()
{
	return gDirUtilp->getExecutableDir() + gDirUtilp->getDirDelimiter() +
		"SLPlugin.exe";
}

/*virtual*/ std::string LLDir_Win32::getLLPluginFilename(std::string base_name)
{
	return gDirUtilp->getLLPluginDir() + gDirUtilp->getDirDelimiter() +
		base_name + ".dll";
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


