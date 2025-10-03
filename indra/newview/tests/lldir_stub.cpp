/**
 * @file lldir_stub.cpp
 * @brief  stub class to allow unit testing
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

// Use me only if you need to stub out some helper functions, not if you e.g. need sane numbers from countFilesInDir

LLDir::LLDir() {}
LLDir::~LLDir() {}
BOOL LLDir::deleteFilesInDir(const std::string &dirname, const std::string &mask) { return true; }
void LLDir::setChatLogsDir(const std::string &path) {}
void LLDir::setPerAccountChatLogsDir(const std::string &) {}
void LLDir::updatePerAccountChatLogsDir() {}
void LLDir::setLindenUserDir(const std::string &) {}
void LLDir::setSkinFolder(const std::string &skin_folder, const std::string& language) {}
bool LLDir::setCacheDir(const std::string &path) { return true; }
void LLDir::dumpCurrentDirectories(LLError::ELevel) {}
std::string LLDir::getSkinFolder() const { return ""; }
std::string LLDir::getLanguage() const { return ""; }


class LLDir_stub : public LLDir
{
public:
    LLDir_stub() = default;
    ~LLDir_stub() = default;

    void initAppDirs(const std::string &app_name, const std::string &) override {}

    std::string getCurPath() override { return "CUR_PATH_FROM_LLDIR"; }
    bool fileExists(const std::string &filename) const override { return false; }

    std::string getLLPluginLauncher()  override { return ""; }
    std::string getLLPluginFilename(std::string base_name) override { return ""; }
};

LLDir_stub gDirUtil;

LLDir* gDirUtilp = &gDirUtil;

std::string LLDir::getExpandedFilename(ELLPath loc, const std::string& subdir, const std::string& filename) const
{
    return subdir + " --- " + filename + " --- expanded!";
}

std::string LLDir::getExpandedFilename(ELLPath location, const std::string &filename) const
{
    return filename + " --- expanded!";
}
