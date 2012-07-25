/** 
 * @file lldir_mac.h
 * @brief Definition of directory utilities class for Mac OS X
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#if !LL_DARWIN
#error This header must not be included when compiling for any target other than Mac OS. Consider including lldir.h instead.
#endif // !LL_DARWIN

#ifndef LL_LLDIR_MAC_H
#define LL_LLDIR_MAC_H

#include "lldir.h"

#include <dirent.h>

class LLDir_Mac : public LLDir
{
public:
	LLDir_Mac();
	virtual ~LLDir_Mac();

	/*virtual*/ void initAppDirs(const std::string &app_name,
		const std::string& app_read_only_data_dir);

	virtual std::string getCurPath();
	virtual U32 countFilesInDir(const std::string &dirname, const std::string &mask);
	virtual BOOL fileExists(const std::string &filename) const;

	/*virtual*/ std::string getLLPluginLauncher();
	/*virtual*/ std::string getLLPluginFilename(std::string base_name);

private:
	int mCurrentDirIndex;
	int mCurrentDirCount;
	std::string mCurrentDir;
};

#endif // LL_LLDIR_MAC_H


