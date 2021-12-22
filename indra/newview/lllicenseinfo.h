/** 
 * @file llicenseinfo.h
 * @brief Routines to access library versions and license information
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

#ifndef LL_LLLICENSEINFO_H
#define LL_LLLICENSEINFO_H

#include "stdtypes.h"
#include "llsingleton.h"
#include <string>

///
/// This API provides license information for the viewer.
/// The singleton is initialized once (from package-info.txt), after which
/// it acts like a map of name => {version, copyrights} for each library.
///
class LLLicenseInfo: public LLSingleton<LLLicenseInfo>
{
	LLSINGLETON(LLLicenseInfo);
    
public:
    struct LibraryData
    {
        std::string version;
        std::string copyrights;
    };
    typedef std::map<std::string, LibraryData> LibraryMap;
    
	/// return the version as a string of the requested library, like "2.0.0.200030"
    const std::string& getVersion(const std::string& library_name) const { return mLibraries.at(library_name).version; }
    
    /// return an indication of whether any library data was found (e.g., false if packages-info.txt is missing)
    bool empty() const noexcept { return mLibraries.empty(); };

    LibraryMap::const_iterator begin() const noexcept { return mLibraries.begin(); };
    LibraryMap::const_iterator end() const noexcept { return mLibraries.end(); };

protected:
    virtual void initSingleton();;
private:
    LibraryMap mLibraries{};
};

#endif
