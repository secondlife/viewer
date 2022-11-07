/** 
 * @file llstatview.cpp
 * @brief Container for all statistics info.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llstatview.h"

#include "llerror.h"
#include "llstatbar.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llui.h"

#include "llstatbar.h"

LLStatView::LLStatView(const LLStatView::Params& p)
:   LLContainerView(p),
    mSetting(p.setting)
{
    BOOL isopen = getDisplayChildren();
    if (mSetting.length() > 0)
    {
        isopen = LLUI::getInstance()->mSettingGroups["config"]->getBOOL(mSetting);
    }
    setDisplayChildren(isopen);
}

LLStatView::~LLStatView()
{
    // Children all cleaned up by default view destructor.
    if (mSetting.length() > 0)
    {
        BOOL isopen = getDisplayChildren();
        LLUI::getInstance()->mSettingGroups["config"]->setBOOL(mSetting, isopen);
    }
}


static StatViewRegistry::Register<LLStatBar> r1("stat_bar");
static StatViewRegistry::Register<LLStatView> r2("stat_view");
// stat_view can be a child of panels/etc.
static LLDefaultChildRegistry::Register<LLStatView> r3("stat_view");


