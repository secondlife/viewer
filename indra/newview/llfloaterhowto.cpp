/** 
 * @file llfloaterhowto.cpp
 * @brief A variant of web floater meant to open guidebook
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

#include "llfloaterhowto.h"

#include "llfloaterreg.h"
#include "llviewercontrol.h"
#include "llweb.h"


LLFloaterHowTo::LLFloaterHowTo(const Params& key) :
    LLFloaterWebContent(key)
{
    mShowPageTitle = false;
}

BOOL LLFloaterHowTo::postBuild()
{
    LLFloaterWebContent::postBuild();

    return TRUE;
}

void LLFloaterHowTo::onOpen(const LLSD& key)
{
    LLFloaterWebContent::Params p(key);
    if (!p.url.isProvided() || p.url.getValue().empty())
    {
        std::string url = gSavedSettings.getString("GuidebookURL");
        p.url = LLWeb::expandURLSubstitutions(url, LLSD());
        p.show_chrome = false;
        p.preferred_media_size = LLRect(0, 500, 300, 0);
    }

    LLFloaterWebContent::onOpen(p);
}

LLFloaterHowTo* LLFloaterHowTo::getInstance()
{
    return LLFloaterReg::getTypedInstance<LLFloaterHowTo>("how_to");
}
