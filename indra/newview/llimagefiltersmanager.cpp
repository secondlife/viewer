/** 
 * @file llimagefiltersmanager.cpp
 * @brief Load and execute image filters. Mostly used for Flickr at the moment.
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


//---------------------------------------------------------------------------
// LLImageFilters
//---------------------------------------------------------------------------

LLImageFiltersManager::LLImageFiltersManager()
{
}

LLImageFiltersManager::~LLImageFiltersManager()
{
}

// virtual static
void LLImageFiltersManager::initSingleton()
{
	loadAllFilters();
}

// static
std::string LLImageFiltersManager::getSysDir()
{
	return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "filters", "");
}

void LLImageFiltersManager::loadAllFilters()
{
	// Load system (coming out of the box) filters
	loadFiltersFromDir(getSysDir());
}

void LLImageFiltersManager::loadFiltersFromDir(const std::string& dir)
{
	mFiltersList.clear();

	LLDirIterator dir_iter(dir, "*.xml");
	while (1)
	{
		std::string file;
		if (!dir_iter.next(file))
		{
			break; // no more files
		}
		
		// Get the ".xml" out of the file name to get the filter name
		std::string filter_name = file.substr(0,file.length()-4);
        mFiltersList.push_back(filter_name);
        
		std::string path = gDirUtilp->add(dir, file);

        // For the moment, just output the file found to the log
        llinfos << "Merov : loadFiltersFromDir, filter = " << file << ",path = " << path << llendl;
	}
}



//============================================================================
