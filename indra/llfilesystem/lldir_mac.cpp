/**
 * @file lldir_mac.cpp
 * @brief Implementation of directory utilities for macOS
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
#include "llstring.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>
#include <filesystem>
#include "lldir_utils_objc.h"

// --------------------------------------------------------------------------------

static bool CreateDirectories(const std::string &path)
{
    std::error_code error;
    std::filesystem::create_directories(path, error);
    return !error && std::filesystem::is_directory(path, error);
}

// --------------------------------------------------------------------------------

LLDir_Mac::LLDir_Mac()
{
    mDirDelimiter = "/";

    const std::string     secondLifeString = "SecondLife";

    std::string executablepathstr = getSystemExecutableFolder();

    //NOTE:  LLINFOS/LLERRS will not output to log here.  The streams are not initialized.

    if (!executablepathstr.empty())
    {
        // mExecutablePathAndName
        mExecutablePathAndName = executablepathstr;

        std::filesystem::path executablepath(executablepathstr);

        mExecutableFilename = executablepath.filename().string();
        mExecutableDir = executablepath.parent_path().string();

        // mAppRODataDir
        std::string resourcepath = getSystemResourceFolder();
        mAppRODataDir = resourcepath;

        // *NOTE: When running in a dev tree, use the copy of
        // skins in indra/newview/ rather than in the application bundle.  This
        // mirrors Windows dev environment behavior and allows direct checkin
        // of edited skins/xui files. JC

        // MBW -- This keeps the mac application from finding other things.
        // If this is really for skins, it should JUST apply to skins.

        std::string::size_type build_dir_pos = mExecutableDir.rfind("/build-darwin-");
        if (build_dir_pos != std::string::npos)
        {
            // ...we're in a dev checkout
            mSkinBaseDir = mExecutableDir.substr(0, build_dir_pos)
                + "/indra/newview/skins";
            LL_INFOS() << "Running in dev checkout with mSkinBaseDir "
                << mSkinBaseDir << LL_ENDL;
        }
        else
        {
            // ...normal installation running
            mSkinBaseDir = mAppRODataDir + mDirDelimiter + "skins";
        }

        auto app_home_env = LLStringUtil::getoptenv("SECONDLIFE_USER_DIR");
        if (app_home_env && !app_home_env->empty())
        {
            mOSUserDir = *app_home_env;
            mOSUserAppDir = *app_home_env;
            mOSCacheDir.clear();
        }
        else
        {
            const std::string appdir = getSystemApplicationSupportFolder();
            if (!appdir.empty())
            {
                mOSUserDir = (std::filesystem::path(appdir) / secondLifeString).string();
            }

            mOSCacheDir = getSystemCacheFolder();
            mOSUserAppDir = mOSUserDir;
        }

        // mTempDir
        //Aura 120920 std::filesystem::temp_directory_path() not yet implemented on mac. :(
        std::string tmpdir = getSystemTempFolder();
        if (!tmpdir.empty())
        {
            mTempDir = (std::filesystem::path(tmpdir) / secondLifeString).string();
        }

        mWorkingDir = getCurPath();

        mLLPluginDir = mAppRODataDir + mDirDelimiter + "SLPlugin.app" + mDirDelimiter + "Contents" + mDirDelimiter + "Frameworks";
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
        mSkinBaseDir = add(mAppRODataDir, "skins");
    }
    mAppName = app_name;
    CreateDirectories(mOSUserAppDir);
    CreateDirectories(add(mOSUserAppDir, "data"));
    CreateDirectories(add(mOSUserAppDir, "logs"));
    CreateDirectories(add(mOSUserAppDir, "user_settings"));
    CreateDirectories(add(mOSUserAppDir, "browser_profile"));
    CreateDirectories(buildSLOSCacheDir());
    CreateDirectories(mTempDir);
    mCAFile = add(mAppRODataDir, "ca-bundle.crt");
}

std::string LLDir_Mac::getCurPath()
{
    return std::filesystem::path( std::filesystem::current_path() ).string();
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
