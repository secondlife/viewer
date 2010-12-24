/** 
 * @file llupdateinstaller.h
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#ifndef LL_UPDATE_INSTALLER_H
#define LL_UPDATE_INSTALLER_H


#include <string>


enum LLInstallScriptMode {
	LL_RUN_INSTALL_SCRIPT_IN_PLACE,
	LL_COPY_INSTALL_SCRIPT_TO_TEMP
};

//
// Launch the installation script.
// 
// The updater will overwrite the current installation, so it is highly recommended
// that the current application terminate once this function is called.
//
int ll_install_update(
					  std::string const & script, // Script to execute.
					  std::string const & updatePath, // Path to update file.
					  bool required, // Is the update required.
					  LLInstallScriptMode mode=LL_COPY_INSTALL_SCRIPT_TO_TEMP); // Run in place or copy to temp?


//
// Returns the path which points to the failed install marker file, should it
// exist.
//
std::string const & ll_install_failed_marker_path(void);


#endif
