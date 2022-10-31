/** 
 * @file llliveappconfig.h
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

#ifndef LLLIVEAPPCONFIG_H
#define LLLIVEAPPCONFIG_H

#include "llapp.h"
#include "lllivefile.h"


/**
 * @class LLLiveAppConfig
 * @see LLLiveFile
 *
 * To use this, instantiate a LLLiveAppConfig object inside your main
 * loop.  The traditional name for it is live_config.  Be sure to call
 * <code>live_config.checkAndReload()</code> periodically.
 */
class LL_COMMON_API LLLiveAppConfig : public LLLiveFile
{
public:

    /**
     * @brief Constructor
     *
     * @param filename. The name of the file for periodically checking
     * configuration.
     * @param refresh_period How often the internal timer should
     * bother checking the filesystem.
     * @param The application priority level of that configuration file.
     */
    LLLiveAppConfig(
        const std::string& filename,
        F32 refresh_period,
        LLApp::OptionPriority priority);

    ~LLLiveAppConfig(); ///< Destructor

protected:
    /*virtual*/ bool loadFile();

private:
    LLApp::OptionPriority mPriority;
};

#endif
