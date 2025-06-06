/**
 * @file llfloateravatarwelcomepack.cpp
 * @author Callum Prentice (callum@lindenlab.com)
 * @brief Floater container for the Avatar Welcome Pack we app
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloateravatarwelcomepack.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"

LLFloaterAvatarWelcomePack::LLFloaterAvatarWelcomePack(const LLSD& key)
    :   LLFloater(key)
{
}

LLFloaterAvatarWelcomePack::~LLFloaterAvatarWelcomePack()
{
    if (mAvatarPicker)
    {
        mAvatarPicker->navigateStop();
        mAvatarPicker->clearCache();          //images are reloading each time already
        mAvatarPicker->unloadMediaSource();
    }
}

bool LLFloaterAvatarWelcomePack::postBuild()
{
    center();
    mAvatarPicker = findChild<LLMediaCtrl>("avatar_picker_contents");
    if (mAvatarPicker)
    {
        mAvatarPicker->clearCache();
    }

    return true;
}
