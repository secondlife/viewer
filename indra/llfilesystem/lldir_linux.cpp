/** 
 * @file lldir_linux.cpp
 * @brief Implementation of directory utilities for linux
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

#include "linden_common.h"

#include "lldir_linux.h"
#include "llerror.h"
#include "llrand.h"
#include "llstring.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>
#include <pwd.h>


static std::string getCurrentUserHome(char* fallback)
{
	const uid_t uid = getuid();
	struct passwd *pw;

	pw = getpwuid(uid);
	if ((pw != NULL) && (pw->pw_dir != NULL))
	{
		return pw->pw_dir;
	}

	LL_INFOS() << "Couldn't detect home directory from passwd - trying $HOME" << LL_ENDL;
	auto home_env = LLStringUtil::getoptenv("HOME");
	if (home_env)
	{
		return *home_env;
	}
	else
	{
		LL_WARNS() << "Couldn't detect home directory!  Falling back to " << fallback << LL_ENDL;
		return fallback;
	}
}


LLDir_Linux::LLDir_Linux()
{
	mDirDelimiter = "/";
	mCurrentDirIndex = -1;
	mCurrentDirCount = -1;
	mDirp = NULL;

	char tmp_str[LL_MAX_PATH];	/* Flawfinder: ignore */ 
	if (getcwd(tmp_str, LL_MAX_PATH) == NULL)
	{
		strcpy(tmp_str, "/tmp");
		LL_WARNS() << "Could not get current directory; changing to "
				<< tmp_str << LL_ENDL;
		if (chdir(tmp_str) == -1)
		{
			LL_ERRS() << "Could not change directory to " << tmp_str << LL_ENDL;
		}
	}

	mExecutableFilename = "";
	mExecutablePathAndName = "";
	mExecutableDir = tmp_str;
	mWorkingDir = tmp_str;
#ifdef APP_RO_DATA_DIR
	mAppRODataDir = APP_RO_DATA_DIR;
#else
	mAppRODataDir = tmp_str;
#endif
    std::string::size_type build_dir_pos = mExecutableDir.rfind("/build-linux-");
    if (build_dir_pos != std::string::npos)
    {
		// ...we're in a dev checkout
		mSkinBaseDir = mExecutableDir.substr(0, build_dir_pos) + "/indra/newview/skins";
		LL_INFOS() << "Running in dev checkout with mSkinBaseDir "
		 << mSkinBaseDir << LL_ENDL;
    }
    else
    {
		// ...normal installation running
		mSkinBaseDir = mAppRODataDir + mDirDelimiter + "skins";
    }	

	mOSUserDir = getCurrentUserHome(tmp_str);
	mOSUserAppDir = "";
	mLindenUserDir = "";

	char path [32];	/* Flawfinder: ignore */ 

	// *NOTE: /proc/%d/exe doesn't work on FreeBSD. But that's ok,
	// because this is the linux implementation.

	snprintf (path, sizeof(path), "/proc/%d/exe", (int) getpid ()); 
	int rc = readlink (path, tmp_str, sizeof (tmp_str)-1);	/* Flawfinder: ignore */ 
	if ( (rc != -1) && (rc <= ((int) sizeof (tmp_str)-1)) )
	{
		tmp_str[rc] = '\0'; //readlink() doesn't 0-terminate the buffer
		mExecutablePathAndName = tmp_str;
		char *path_end;
		if ((path_end = strrchr(tmp_str,'/')))
		{
			*path_end = '\0';
			mExecutableDir = tmp_str;
			mWorkingDir = tmp_str;
			mExecutableFilename = path_end+1;
		}
		else
		{
			mExecutableFilename = tmp_str;
		}
	}

	mLLPluginDir = mExecutableDir + mDirDelimiter + "llplugin";

	// *TODO: don't use /tmp, use $HOME/.secondlife/tmp or something.
	mTempDir = "/tmp";
}

LLDir_Linux::~LLDir_Linux()
{
}

// Implementation


void LLDir_Linux::initAppDirs(const std::string &app_name,
							  const std::string& app_read_only_data_dir)
{
	// Allow override so test apps can read newview directory
	if (!app_read_only_data_dir.empty())
	{
		mAppRODataDir = app_read_only_data_dir;
		mSkinBaseDir = add(mAppRODataDir, "skins");
	}
	mAppName = app_name;

	std::string upper_app_name(app_name);
	LLStringUtil::toUpper(upper_app_name);

	auto app_home_env(LLStringUtil::getoptenv(upper_app_name + "_USER_DIR"));
	if (app_home_env)
	{
		// user has specified own userappdir i.e. $SECONDLIFE_USER_DIR
		mOSUserAppDir = *app_home_env;
	}
	else
	{
		// traditionally on unixoids, MyApp gets ~/.myapp dir for data
		mOSUserAppDir = mOSUserDir;
		mOSUserAppDir += "/";
		mOSUserAppDir += ".";
		std::string lower_app_name(app_name);
		LLStringUtil::toLower(lower_app_name);
		mOSUserAppDir += lower_app_name;
	}

	// create any directories we expect to write to.

	int res = LLFile::mkdir(mOSUserAppDir);
	if (res == -1)
	{
		LL_WARNS() << "Couldn't create app user dir " << mOSUserAppDir << LL_ENDL;
		LL_WARNS() << "Default to base dir" << mOSUserDir << LL_ENDL;
		mOSUserAppDir = mOSUserDir;
	}

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

	mCAFile = getExpandedFilename(LL_PATH_EXECUTABLE, "ca-bundle.crt");
}

U32 LLDir_Linux::countFilesInDir(const std::string &dirname, const std::string &mask)
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

std::string LLDir_Linux::getCurPath()
{
	char tmp_str[LL_MAX_PATH];	/* Flawfinder: ignore */ 
	if (getcwd(tmp_str, LL_MAX_PATH) == NULL)
	{
		LL_WARNS() << "Could not get current directory" << LL_ENDL;
		tmp_str[0] = '\0';
	}
	return tmp_str;
}


bool LLDir_Linux::fileExists(const std::string &filename) const
{
	struct stat stat_data;
	// Check the age of the file
	// Now, we see if the files we've gathered are recent...
	int res = stat(filename.c_str(), &stat_data);
	if (!res)
	{
		return true;
	}
	else
	{
		return false;
	}
}


/*virtual*/ std::string LLDir_Linux::getLLPluginLauncher()
{
	return gDirUtilp->getExecutableDir() + gDirUtilp->getDirDelimiter() +
		"SLPlugin";
}

/*virtual*/ std::string LLDir_Linux::getLLPluginFilename(std::string base_name)
{
	return gDirUtilp->getLLPluginDir() + gDirUtilp->getDirDelimiter() +
		"lib" + base_name + ".so";
}
