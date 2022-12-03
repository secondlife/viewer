/** 
 * @file llfloateravatar.h
 * @author Leyla Farazha
 * @brief floater for the avatar changer
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

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloateravatar.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"


LLFloaterAvatar::LLFloaterAvatar(const LLSD& key)
	:	LLFloater(key)
{
}

LLFloaterAvatar::~LLFloaterAvatar()
{
	if (mAvatarPicker)
	{
		mAvatarPicker->navigateStop();
        mAvatarPicker->clearCache();          //images are reloading each time already
        mAvatarPicker->unloadMediaSource();
	}
}

BOOL LLFloaterAvatar::postBuild()
{
    mAvatarPicker = findChild<LLMediaCtrl>("avatar_picker_contents");
    if (mAvatarPicker)
    {
        mAvatarPicker->clearCache();
    }
	enableResizeCtrls(true, true, false);
	return TRUE;
}


