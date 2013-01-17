/** 
 * @file mac_updater.h
 * @brief 
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include <iostream>
#include <pthread.h>
#include <boost/filesystem.hpp>

#ifndef LL_MAC_UPDATER_H
#define LL_MAC_UPDATER_H
extern bool gCancelled;
extern bool gFailure;

void *updatethreadproc(void*);
std::string* walkParents( signed int depth, std::string* childpath );
std::string* getUserTrashFolder();

void setProgress(int cur, int max);
void setProgressText(const std::string& str);
void sendProgress(int cur, int max, std::string str);
void sendDone();
void sendStopAlert();

bool isFSRefViewerBundle(const std::string& targetURL);
bool isDirWritable(const std::string& dir_name);
bool mkTempDir(boost::filesystem::path& temp_dir);
bool copyDir(const std::string& src_dir, const std::string& dest_dir);

int oldmain();

class LLMacUpdater
{
public:
    LLMacUpdater();
    void doUpdate();
    const std::string walkParents( signed int depth, const std::string& childpath );
    bool isApplication(const std::string& app_str);
    void filterFile(const char* filename);

    bool findAppBundleOnDiskImage(const boost::filesystem::path& dir_path,
                                  boost::filesystem::path& path_found);

    bool verifyDirectory(const boost::filesystem::path* directory, bool isParent=false);
    bool getViewerDir(boost::filesystem::path &app_dir);
    bool downloadDMG(const std::string& dmgName, boost::filesystem::path* temp_dir);
    bool doMount(const std::string& dmgName, char* deviceNode, const boost::filesystem::path& temp_dir);
    bool moveApplication (const boost::filesystem::path& app_dir, 
                          const boost::filesystem::path& temp_dir, 
                          boost::filesystem::path& aside_dir);
    bool doInstall(const boost::filesystem::path& app_dir, 
                   const boost::filesystem::path& temp_dir,
                   boost::filesystem::path& mount_dir,
                   bool replacingTarget);
    void* updatethreadproc(void*);
    static void* sUpdatethreadproc(void*);

public:
    std::string *mUpdateURL;
    std::string *mProductName;
    std::string *mBundleID;
    std::string *mDmgFile;
    std::string *mMarkerPath;
    std::string *mApplicationPath;
    static LLMacUpdater *sInstance;

};
#endif


