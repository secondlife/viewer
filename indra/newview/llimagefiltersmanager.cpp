/** 
 * @file llimagefiltersmanager.cpp
 * @brief Load image filters list and retrieve their path.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llimagefiltersmanager.h"

#include "lldiriterator.h"
#include "lltrans.h"

std::string get_sys_dir()
{
    return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "filters", "");
}

//---------------------------------------------------------------------------
// LLImageFiltersManager
//---------------------------------------------------------------------------

LLImageFiltersManager::LLImageFiltersManager()
{
}

LLImageFiltersManager::~LLImageFiltersManager()
{
}

// virtual
void LLImageFiltersManager::initSingleton()
{
    loadAllFilters();
}

void LLImageFiltersManager::loadAllFilters()
{
    // Load system (coming out of the box) filters
    loadFiltersFromDir(get_sys_dir());
}

void LLImageFiltersManager::loadFiltersFromDir(const std::string& dir)
{
    mFiltersList.clear();

    LLDirIterator dir_iter(dir, "*.xml");
    while (1)
    {
        std::string file_name;
        if (!dir_iter.next(file_name))
        {
            break; // no more files
        }
        
        // Get the ".xml" out of the file name to get the filter name. That's the one known in strings.xml
        std::string filter_name_untranslated = file_name.substr(0,file_name.length()-4);
        
        // Get the localized name for the filter
        std::string filter_name_translated;
        bool translated = LLTrans::findString(filter_name_translated, filter_name_untranslated);
        std::string filter_name = (translated ? filter_name_translated: filter_name_untranslated);
        
        // Store the filter in the list with its associated file name
        mFiltersList[filter_name] = file_name;
    }
}

// Note : That method is a bit heavy handed but the list of filters is always small (10 or so)
// and that method is typically called only once when building UI widgets.
const std::vector<std::string> LLImageFiltersManager::getFiltersList() const
{
    std::vector<std::string> filter_list;
    for (std::map<std::string,std::string>::const_iterator it = mFiltersList.begin(); it != mFiltersList.end(); ++it)
    {
        filter_list.push_back(it->first);
    }
    return filter_list;
}

std::string LLImageFiltersManager::getFilterPath(const std::string& filter_name)
{
    std::string path = "";
    std::map<std::string,std::string>::const_iterator it = mFiltersList.find(filter_name);
    if (it != mFiltersList.end())
    {
        // Get the file name for that filter and build the complete path
        std::string file = it->second;
        std::string dir = get_sys_dir();
        path = gDirUtilp->add(dir, file);
    }
    return path;
}

//============================================================================
