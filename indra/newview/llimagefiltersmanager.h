/** 
 * @file llimagefiltersmanager.h
 * @brief Load image filters list and retrieve their path.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2014, Linden Research, Inc.
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

#ifndef LL_LLIMAGEFILTERSMANAGER_H
#define LL_LLIMAGEFILTERSMANAGER_H

#include "llsingleton.h"

//============================================================================
// LLImageFiltersManager class

class LLImageFiltersManager : public LLSingleton<LLImageFiltersManager>
{
    LLSINGLETON(LLImageFiltersManager);
    ~LLImageFiltersManager();
    LOG_CLASS(LLImageFiltersManager);
public:
    const std::vector<std::string> getFiltersList() const;
    std::string getFilterPath(const std::string& filter_name);
  
private:
    void loadAllFilters();
    void loadFiltersFromDir(const std::string& dir);
    
    /*virtual*/ void initSingleton();
    
    // List of filters : first is the user friendly localized name, second is the xml file name
    std::map<std::string,std::string> mFiltersList;
};

#endif // LL_LLIMAGEFILTERSMANAGER_H
