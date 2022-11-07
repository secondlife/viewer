/** 
 * @file llliveappconfig.cpp
 * @brief Configuration information for an LLApp that overrides indra.xml
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llliveappconfig.h"

#include "llapp.h"
#include "llsd.h"
#include "llsdserialize.h"

LLLiveAppConfig::LLLiveAppConfig(
    const std::string& filename,
    F32 refresh_period,
    LLApp::OptionPriority priority) :
    LLLiveFile(filename, refresh_period),
    mPriority(priority)
{ }


LLLiveAppConfig::~LLLiveAppConfig()
{ }

// virtual 
bool LLLiveAppConfig::loadFile()
{
    LL_INFOS() << "LLLiveAppConfig::loadFile(): reading from "
        << filename() << LL_ENDL;
    llifstream file(filename().c_str());
    LLSD config;
    if (file.is_open())
    {
        LLSDSerialize::fromXML(config, file);
        if(!config.isMap())
        {
            LL_WARNS() << "Live app config not an map in " << filename()
                << " Ignoring the data." << LL_ENDL;
            return false;
        }
        file.close();
    }
    else
    {
        LL_INFOS() << "Live file " << filename() << " does not exit." << LL_ENDL;
    }
    // *NOTE: we do not handle the else case here because we would not
    // have attempted to load the file unless LLLiveFile had
    // determined there was a reason to load it. This only happens
    // when either the file has been updated or it is either suddenly
    // in existence or has passed out of existence. Therefore, we want
    // to set the config to an empty config, and return that it
    // changed.

    LLApp* app = LLApp::instance();
    if(app) app->setOptionData(mPriority, config);
    return true;
}
