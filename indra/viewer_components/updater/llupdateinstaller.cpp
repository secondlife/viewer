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
#include "llprocess.h"
#include "llupdateinstaller.h"
#include "lldir.h" 
#include "llsd.h"

#if defined(LL_WINDOWS)
#pragma warning(disable: 4702)      // disable 'unreachable code' so we can use lexical_cast (really!).
#endif
#include <boost/lexical_cast.hpp>


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


int ll_install_update(std::string const & script,
					  std::string const & updatePath,
					  bool required,
					  LLInstallScriptMode mode)
{
	std::string actualScriptPath;
	switch(mode) {
		case LL_COPY_INSTALL_SCRIPT_TO_TEMP:
			try {
				actualScriptPath = copy_to_temp(script);
			}
			catch (RelocateError &) {
				return -1;
			}
			break;
		case LL_RUN_INSTALL_SCRIPT_IN_PLACE:
			actualScriptPath = script;
			break;
		default:
			llassert(!"unpossible copy mode");
	}
	
	llinfos << "UpdateInstaller: installing " << updatePath << " using " <<
		actualScriptPath << LL_ENDL;
	
	LLProcess::Params params;
	params.executable = actualScriptPath;
	params.args.add(updatePath);
	params.args.add(ll_install_failed_marker_path());
	params.args.add(boost::lexical_cast<std::string>(required));
	params.autokill = false;
	return LLProcess::create(params)? 0 : -1;
}


std::string const & ll_install_failed_marker_path(void)
{
	static std::string path;
	if(path.empty()) {
		path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "SecondLifeInstallFailed.marker");
	}
	return path;
}
