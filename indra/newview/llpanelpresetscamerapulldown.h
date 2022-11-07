/** 
 * @file llpanelpresetscamerapulldown.h
 * @brief A panel showing a quick way to pick camera presets
 *
 * $LicenseInfo:firstyear=2017&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2017, Linden Research, Inc.
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

#ifndef LL_LLPANELPRESETSCAMERAPULLDOWN_H
#define LL_LLPANELPRESETSCAMERAPULLDOWN_H

#include "linden_common.h"

#include "llpanelpulldown.h"

class LLPanelPresetsCameraPulldown : public LLPanelPulldown
{
 public:
    LLPanelPresetsCameraPulldown();
    /*virtual*/ BOOL postBuild();
    void populatePanel();
    
 private:
    void onViewButtonClick(const LLSD& user_data);
    void onRowClick(const LLSD& user_data);

    std::list<std::string> mPresetNames;
    LOG_CLASS(LLPanelPresetsCameraPulldown);
};

#endif // LL_LLPANELPRESETSCAMERAPULLDOWN_H
