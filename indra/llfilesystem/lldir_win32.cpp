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
#include "llstring.h"
#include "stringize.h"
#include "llfile.h"
#include <shlobj.h>
#include <fstream>

#include <direct.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

// Utility stuff to get versions of the sh
#define PACKVERSION(major,minor) MAKELONG(minor,major)
DWORD GetDllVersion(LPCTSTR lpszDllName);

namespace
{ // anonymous
    enum class prst { INIT, OPEN, SKIP };
    prst state{ prst::INIT };

    // This is called so early that we can't count on static objects being
    // properly constructed yet, so declare a pointer instead of an instance.
    std::ofstream* prelogf = nullptr;

    void prelog(const std::string& message)
    {
        std::optional<std::string> prelog_name;

        switch (state)
        {
        case prst::INIT:
            // assume we failed, until we succeed
            state = prst::SKIP;

            prelog_name = LLStringUtil::getoptenv("PRELOG");
            if (! prelog_name)
                // no PRELOG variable set, carry on
                return;
            prelogf = new llofstream(*prelog_name, std::ios_base::app);
            if (! (prelogf && prelogf->is_open()))
                // can't complain to anybody; how?
                return;
            // got the log file open, cool!
            state = prst::OPEN;
            (*prelogf) << "========================================================================"
                       << std::endl;
            // fall through, don't break
            [[fallthrough]];

        case prst::OPEN:
            (*prelogf) << message << std::endl;
            break;

        case prst::SKIP:
            // either PRELOG isn't set, or we failed to open that pathname
            break;
        }
    }
} // anonymous namespace

#define PRELOG(expression) prelog(STRINGIZE(expression))

LLDir_Win32::LLDir_Win32()
{
	// set this first: used by append() and add() methods
	mDirDelimiter = "\\";

	WCHAR w_str[MAX_PATH];
	// Application Data is where user settings go. We rely on $APPDATA being
	// correct.
	auto APPDATA = LLStringUtil::getoptenv("APPDATA");
	if (APPDATA)
	{
		mOSUserDir = *APPDATA;
	}
	PRELOG("APPDATA='" << mOSUserDir << "'");
	// On Windows, we could have received a plain-ASCII pathname in which
	// non-ASCII characters have been munged to '?', or the pathname could
	// have been badly encoded and decoded such that we now have garbage
	// instead of a valid path. Check that mOSUserDir actually exists.
	if (mOSUserDir.empty() || ! fileExists(mOSUserDir))
	{
		PRELOG("APPDATA does not exist");
		//HRESULT okay = SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, w_str);
		wchar_t *pwstr = NULL;
		HRESULT okay = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &pwstr);
		PRELOG("SHGetKnownFolderPath(FOLDERID_RoamingAppData) returned " << okay);
		if (SUCCEEDED(okay) && pwstr)
		{
			// But of course, only update mOSUserDir if SHGetKnownFolderPath() works.
			mOSUserDir = ll_convert_wide_to_string(pwstr);
			// Not only that: update our environment so that child processes
			// will see a reasonable value as well.
			_wputenv_s(L"APPDATA", pwstr);
			// SHGetKnownFolderPath() contract requires us to free pwstr
			CoTaskMemFree(pwstr);
			PRELOG("mOSUserDir='" << mOSUserDir << "'");
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
	auto LOCALAPPDATA = LLStringUtil::getoptenv("LOCALAPPDATA");
	if (LOCALAPPDATA)
	{
		mOSCacheDir = *LOCALAPPDATA;
	}
	PRELOG("LOCALAPPDATA='" << mOSCacheDir << "'");
	// Windows really does not deal well with pathnames containing non-ASCII
	// characters. See above remarks about APPDATA.
	if (mOSCacheDir.empty() || ! fileExists(mOSCacheDir))
	{
		PRELOG("LOCALAPPDATA does not exist");
		//HRESULT okay = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, w_str);
		wchar_t *pwstr = NULL;
		HRESULT okay = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pwstr);
		PRELOG("SHGetKnownFolderPath(FOLDERID_LocalAppData) returned " << okay);
		if (SUCCEEDED(okay) && pwstr)
		{
			// But of course, only update mOSCacheDir if SHGetKnownFolderPath() works.
			mOSCacheDir = ll_convert_wide_to_string(pwstr);
			// Update our environment so that child processes will see a
			// reasonable value as well.
			_wputenv_s(L"LOCALAPPDATA", pwstr);
			// SHGetKnownFolderPath() contract requires us to free pwstr
			CoTaskMemFree(pwstr);
			PRELOG("mOSCacheDir='" << mOSCacheDir << "'");
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
		return true;
	}
	else
	{
		return false;
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


