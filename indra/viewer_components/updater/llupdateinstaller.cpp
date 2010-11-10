/** 
 * @file llupdateinstaller.cpp
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

#include "linden_common.h"
#include <apr_file_io.h>
#include "llapr.h"
#include "llprocesslauncher.h"
#include "llupdateinstaller.h"
#include "lldir.h"


namespace {
	class RelocateError {};
	
	
	std::string copy_to_temp(std::string const & path)
	{
		std::string scriptFile = gDirUtilp->getBaseFileName(path);
		std::string newPath = gDirUtilp->getExpandedFilename(LL_PATH_TEMP, scriptFile);
		apr_status_t status = apr_file_copy(path.c_str(), newPath.c_str(), APR_FILE_SOURCE_PERMS, gAPRPoolp);
		if(status != APR_SUCCESS) throw RelocateError();
		
		return newPath;
	}
}


int ll_install_update(std::string const & script, std::string const & updatePath, LLInstallScriptMode mode)
{
	std::string finalPath;
	switch(mode) {
		case LL_COPY_INSTALL_SCRIPT_TO_TEMP:
			try {
				finalPath = copy_to_temp(updatePath);
			}
			catch (RelocateError &) {
				return -1;
			}
			break;
		case LL_RUN_INSTALL_SCRIPT_IN_PLACE:
			finalPath = updatePath;
			break;
		default:
			llassert(!"unpossible copy mode");
	}
	
	LLProcessLauncher launcher;
	launcher.setExecutable(script);
	launcher.addArgument(finalPath);
	int result = launcher.launch();
	launcher.orphan();
	
	return result;
}
