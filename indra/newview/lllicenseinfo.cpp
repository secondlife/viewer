/** 
 * @file lllicenseinfo.cpp
 * @brief Routines to access library version and license information
 * @author Aech Linden
 *
 * $LicenseInfo:firstyear=2021&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2021, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include "lllicenseinfo.h"
#include <boost/algorithm/string.hpp>
#include "lldir.h"

LLLicenseInfo::LLLicenseInfo()
{
    LL_DEBUGS("LicenseInfo") << "instantiating license info" << LL_ENDL;
}

void LLLicenseInfo::initSingleton()
{
    // Get the the map with name => {version, cpyrights}, from file created at build time
    std::string licenses_path = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "packages-info.txt");
    llifstream licenses_file;
    licenses_file.open(licenses_path.c_str());        /* Flawfinder: ignore */
    if (!licenses_file.is_open()) {
        LL_INFOS("LicenseInfo") << "Could not read licenses file at " << licenses_path << LL_ENDL;
        return;
    }

    LL_DEBUGS("LicenseInfo") << "Reading licenses file at " << licenses_path << LL_ENDL;
    std::string license_line;
    std::string name{}, version{}, copyright{};
    while ( std::getline(licenses_file, license_line) )
    {
        if (license_line.empty())  // blank line starts a new library/version/copyright
        {
            if (!name.empty()) {  // Add what we have accumulated.
                mLibraries.insert({name, {version, copyright}});
            }
            else
            {
                LL_WARNS("LicenseInfo") << "new line with no current data" << LL_ENDL;
            }
            name.clear();
            version.clear();
            copyright.clear();
        }
        else
        {
            if (name.empty()) { // No name yet. Parse this line into name and version.
                auto name_termination_index = license_line.find(':');
                if (name_termination_index == std::string::npos) // First line has no colon.
                {
                    name_termination_index = license_line.find_last_of(' ');
                 }
                name = license_line.substr(0, name_termination_index);
                version = license_line.substr(name_termination_index + 1);
                boost::algorithm::trim(version);
            } else {
                copyright += license_line;
            }
        }
    }
    licenses_file.close();
}
