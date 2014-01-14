/** 
 * @file llimagefiltersmanager.h
 * @brief Load and execute image filters. Mostly used for Flickr at the moment.
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
#include "llimage.h"
/*
typedef enum e_vignette_mode
{
	VIGNETTE_MODE_NONE  = 0,
	VIGNETTE_MODE_BLEND = 1,
	VIGNETTE_MODE_FADE  = 2
} EVignetteMode;

typedef enum e_screen_mode
{
	SCREEN_MODE_2DSINE   = 0,
	SCREEN_MODE_LINE     = 1
} EScreenMode;
*/
//============================================================================
// library initialization class

class LLImageFiltersManager : public LLSingleton<LLImageFiltersManager>
{
	LOG_CLASS(LLImageFiltersManager);
public:
    // getFilters(); get a vector of std::string containing the filter names
    //LLSD loadFilter(const std::string& filter_name);
    //void executeFilter(const LLSD& filter_data, LLPointer<LLImageRaw> raw_image);
    const std::vector<std::string> &getFiltersList() const { return mFiltersList; }
    std::string getFilterPath(const std::string& filter_name);
  
protected:
private:
	void loadAllFilters();
	void loadFiltersFromDir(const std::string& dir);
//	LLSD loadFilter(const std::string& path);
    
	static std::string getSysDir();
    
    friend class LLSingleton<LLImageFiltersManager>;
	/*virtual*/ void initSingleton();
	LLImageFiltersManager();
	~LLImageFiltersManager();
    
	// List of filters
    std::vector<std::string> mFiltersList;
};

#endif // LL_LLIMAGEFILTERSMANAGER_H
