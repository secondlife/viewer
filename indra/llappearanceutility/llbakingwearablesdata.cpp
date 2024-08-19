/**
 * @file llbakingwearablesdata.cpp
 * @brief Implementation of LLBakingWearablesData class
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

#include "llbakingwearablesdata.h"
#include "llbakingwearable.h"

using namespace LLAvatarAppearanceDefines;

LLBakingWearablesData::LLBakingWearablesData()
{
}

// virtual
LLBakingWearablesData::~LLBakingWearablesData()
{
    std::for_each(mWearables.begin(), mWearables.end(), DeletePointer());
    mWearables.clear();
}

void LLBakingWearablesData::setWearableOutfit(LLSD& sd)
{
    LLSD::array_const_iterator type_iter = sd.beginArray();
    LLSD::array_const_iterator type_end = sd.endArray();
    S32 wearable_index = 0;
    for (; type_iter != type_end && wearable_index < (S32) LLWearableType::WT_COUNT; ++type_iter, ++wearable_index)
    {
        LLSD::array_const_iterator wearable_iter = type_iter->beginArray();
        LLSD::array_const_iterator wearable_end = type_iter->endArray();
        for (; wearable_iter != wearable_end; ++wearable_iter)
        {
            if ( wearable_iter->isDefined() )
            {
                LLBakingWearable* wearable = new LLBakingWearable();
                std::istringstream istr( (*wearable_iter)["contents"].asString() );
                wearable->importStream(istr, mAvatarAppearance);

                // Sanity check the wearable type.
                if (wearable->getType() != wearable_index)
                {
                    LL_WARNS() << "Unexpected wearable type!  Expected " << wearable_index
                            << ", processed " << (S32) wearable->getType() << LL_ENDL;
                    // *TODO: Throw exception?
                    delete wearable;
                    continue;
                }
                mWearables.push_back(wearable);

                const LLWearableType::EType type = wearable->getType();
                if (wearable->getAssetType() == LLAssetType::AT_BODYPART)
                {
                    // exactly one wearable per body part
                    setWearable(type,0,wearable);
                }
                else
                {
                    pushWearable(type,wearable);
                }
            }
        }
    }

    wearable_list_t::const_iterator wearable_iter = mWearables.begin();
    wearable_list_t::const_iterator wearable_end  = mWearables.end();
    for (; wearable_iter != wearable_end; ++wearable_iter)
    {
        LLBakingWearable* wearable = (*wearable_iter);
        const BOOL removed = FALSE;
        wearableUpdated(wearable, removed);
    }

    for(wearable_index = 0; wearable_index < (S32) LLWearableType::WT_COUNT; wearable_index++)
    {
        LLWearable* top_wearable = getTopWearable((LLWearableType::EType)wearable_index);
        if (top_wearable)
        {
            top_wearable->writeToAvatar(mAvatarAppearance);
        }
    }
}


void LLBakingWearablesData::asLLSD(LLSD& sd) const
{
    sd = LLSD::emptyMap();
    wearableentry_map_t::const_iterator type_iter = mWearableDatas.begin();
    wearableentry_map_t::const_iterator type_end  = mWearableDatas.end();
    for (; type_iter != type_end; ++type_iter)
    {
        LLSD wearable_type_sd = LLSD::emptyArray();
        LLWearableType::EType wearable_type = type_iter->first;
        wearableentry_vec_t::const_iterator wearable_iter = type_iter->second.begin();
        wearableentry_vec_t::const_iterator wearable_end  = type_iter->second.end();
        for (; wearable_iter != wearable_end; ++wearable_iter)
        {
            const LLBakingWearable* wearable = dynamic_cast<LLBakingWearable*>(*wearable_iter);
            LLSD wearable_sd;
            wearable->asLLSD(wearable_sd);
            wearable_type_sd.append(wearable_sd);
        }
        sd[LLWearableType::getInstance()->getTypeName(wearable_type)] = wearable_type_sd;
    }
}



