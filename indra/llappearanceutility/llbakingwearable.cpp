/**
 * @file llbakingwearable.cpp
 * @brief Implementation of LLBakingWearable class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "llavatarappearancedefines.h"
#include "llbakingwearable.h"
#include "indra_constants.h"

using namespace LLAvatarAppearanceDefines;

LLBakingWearable::LLBakingWearable()
{
}

// virtual
LLBakingWearable::~LLBakingWearable()
{
}

// virtual
void LLBakingWearable::setUpdated() const
{
}

// virtual
void LLBakingWearable::addToBakedTextureHash(LLMD5& hash) const
{
}

// virtual
LLUUID LLBakingWearable::getDefaultTextureImageID(ETextureIndex index) const
{
    const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture(index);
    const std::string &default_image_name = texture_dict->mDefaultImageName;
    if (default_image_name == "")
    {
        return IMG_DEFAULT_AVATAR;
    }
    else
    {
        //return LLUUID(gSavedSettings.getString(default_image_name));
        // *TODO: Fix this.
        return LLUUID::null;
    }
}

void LLBakingWearable::asLLSD(LLSD& sd) const
{
    // stub implementation. TODO: fill in.
}
