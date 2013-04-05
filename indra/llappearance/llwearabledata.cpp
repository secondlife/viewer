/** 
 * @file llwearabledata.cpp
 * @brief LLWearableData class implementation
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#include "llwearabledata.h"

#include "llavatarappearance.h"
#include "llavatarappearancedefines.h"
#include "lldriverparam.h"
#include "llmd5.h"

LLWearableData::LLWearableData() :
	mAvatarAppearance(NULL)
{
}

// virtual
LLWearableData::~LLWearableData()
{
}

using namespace LLAvatarAppearanceDefines;

LLWearable* LLWearableData::getWearable(const LLWearableType::EType type, U32 index)
{
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return NULL;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
	{
		return NULL;
	}
	else
	{
		return wearable_vec[index];
	}
}

void LLWearableData::setWearable(const LLWearableType::EType type, U32 index, LLWearable *wearable)
{
	LLWearable *old_wearable = getWearable(type,index);
	if (!old_wearable)
	{
		pushWearable(type,wearable);
		return;
	}
	
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		llwarns << "invalid type, type " << type << " index " << index << llendl; 
		return;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
	{
		llwarns << "invalid index, type " << type << " index " << index << llendl; 
	}
	else
	{
		wearable_vec[index] = wearable;
		old_wearable->setUpdated();
		const BOOL removed = FALSE;
		wearableUpdated(wearable, removed);
	}
}

U32 LLWearableData::pushWearable(const LLWearableType::EType type, 
								   LLWearable *wearable,
								   bool trigger_updated /* = true */)
{
	if (wearable == NULL)
	{
		// no null wearables please!
		llwarns << "Null wearable sent for type " << type << llendl;
		return MAX_CLOTHING_PER_TYPE;
	}
	if (type < LLWearableType::WT_COUNT || mWearableDatas[type].size() < MAX_CLOTHING_PER_TYPE)
	{
		mWearableDatas[type].push_back(wearable);
		if (trigger_updated)
		{
			const BOOL removed = FALSE;
			wearableUpdated(wearable, removed);
		}
		return mWearableDatas[type].size()-1;
	}
	return MAX_CLOTHING_PER_TYPE;
}

// virtual
void LLWearableData::wearableUpdated(LLWearable *wearable, BOOL removed)
{
	wearable->setUpdated();
	// FIXME DRANO avoid updating params via wearables when rendering server-baked appearance.
#if 0
	if (mAvatarAppearance->isUsingServerBakes() && !mAvatarAppearance->isUsingLocalAppearance())
	{
		return;
	}
#endif
	if (!removed)
	{
		pullCrossWearableValues(wearable->getType());
	}
}

void LLWearableData::popWearable(LLWearable *wearable)
{
	if (wearable == NULL)
	{
		// nothing to do here. move along.
		return;
	}

	U32 index = getWearableIndex(wearable);
	const LLWearableType::EType type = wearable->getType();

	if (index < MAX_CLOTHING_PER_TYPE && index < getWearableCount(type))
	{
		popWearable(type, index);
	}
}

void LLWearableData::popWearable(const LLWearableType::EType type, U32 index)
{
	LLWearable *wearable = getWearable(type, index);
	if (wearable)
	{
		mWearableDatas[type].erase(mWearableDatas[type].begin() + index);
		const BOOL removed = TRUE;
		wearableUpdated(wearable, removed);
	}
}

void LLWearableData::clearWearableType(const LLWearableType::EType type)
{
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return;
	}
	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	wearable_vec.clear();
}

bool LLWearableData::swapWearables(const LLWearableType::EType type, U32 index_a, U32 index_b)
{
	wearableentry_map_t::iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return false;
	}

	wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (0 > index_a || index_a >= wearable_vec.size()) return false;
	if (0 > index_b || index_b >= wearable_vec.size()) return false;

	LLWearable* wearable = wearable_vec[index_a];
	wearable_vec[index_a] = wearable_vec[index_b];
	wearable_vec[index_b] = wearable;
	return true;
}

void LLWearableData::pullCrossWearableValues(const LLWearableType::EType type)
{
	llassert(mAvatarAppearance);
	// scan through all of the avatar's visual parameters
	for (LLViewerVisualParam* param = (LLViewerVisualParam*) mAvatarAppearance->getFirstVisualParam(); 
		 param;
		 param = (LLViewerVisualParam*) mAvatarAppearance->getNextVisualParam())
	{
		if( param )
		{
			LLDriverParam *driver_param = dynamic_cast<LLDriverParam*>(param);
			if(driver_param)
			{
				// parameter is a driver parameter, have it update its cross-driven params
				driver_param->updateCrossDrivenParams(type);
			}
		}
	}
}


U32	LLWearableData::getWearableIndex(const LLWearable *wearable) const
{
	if (wearable == NULL)
	{
		return MAX_CLOTHING_PER_TYPE;
	}

	const LLWearableType::EType type = wearable->getType();
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		llwarns << "tried to get wearable index with an invalid type!" << llendl;
		return MAX_CLOTHING_PER_TYPE;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	for(U32 index = 0; index < wearable_vec.size(); index++)
	{
		if (wearable_vec[index] == wearable)
		{
			return index;
		}
	}

	return MAX_CLOTHING_PER_TYPE;
}

BOOL LLWearableData::isOnTop(LLWearable* wearable) const
{
	if (!wearable) return FALSE;
	const LLWearableType::EType type = wearable->getType();
	return ( getTopWearable(type) == wearable );
}

const LLWearable* LLWearableData::getWearable(const LLWearableType::EType type, U32 index) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return NULL;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	if (index>=wearable_vec.size())
	{
		return NULL;
	}
	else
	{
		return wearable_vec[index];
	}
}

LLWearable* LLWearableData::getTopWearable(const LLWearableType::EType type)
{
	U32 count = getWearableCount(type);
	if ( count == 0)
	{
		return NULL;
	}

	return getWearable(type, count-1);
}

const LLWearable* LLWearableData::getTopWearable(const LLWearableType::EType type) const
{
	U32 count = getWearableCount(type);
	if ( count == 0)
	{
		return NULL;
	}

	return getWearable(type, count-1);
}

LLWearable* LLWearableData::getBottomWearable(const LLWearableType::EType type)
{
	if (getWearableCount(type) == 0)
	{
		return NULL;
	}

	return getWearable(type, 0);
}

const LLWearable* LLWearableData::getBottomWearable(const LLWearableType::EType type) const
{
	if (getWearableCount(type) == 0)
	{
		return NULL;
	}

	return getWearable(type, 0);
}

U32 LLWearableData::getWearableCount(const LLWearableType::EType type) const
{
	wearableentry_map_t::const_iterator wearable_iter = mWearableDatas.find(type);
	if (wearable_iter == mWearableDatas.end())
	{
		return 0;
	}
	const wearableentry_vec_t& wearable_vec = wearable_iter->second;
	return wearable_vec.size();
}

U32 LLWearableData::getWearableCount(const U32 tex_index) const
{
	const LLWearableType::EType wearable_type = LLAvatarAppearanceDictionary::getTEWearableType((LLAvatarAppearanceDefines::ETextureIndex)tex_index);
	return getWearableCount(wearable_type);
}

LLUUID LLWearableData::computeBakedTextureHash(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index,
												 BOOL generate_valid_hash) // Set to false if you want to upload the baked texture w/o putting it in the cache
{
	LLUUID hash_id;
	bool hash_computed = false;
	LLMD5 hash;
	const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = LLAvatarAppearanceDictionary::getInstance()->getBakedTexture(baked_index);

	for (U8 i=0; i < baked_dict->mWearables.size(); i++)
	{
		const LLWearableType::EType baked_type = baked_dict->mWearables[i];
		const U32 num_wearables = getWearableCount(baked_type);
		for (U32 index = 0; index < num_wearables; ++index)
		{
			const LLWearable* wearable = getWearable(baked_type,index);
			if (wearable)
			{
				wearable->addToBakedTextureHash(hash);
				hash_computed = true;
			}
		}
	}
	if (hash_computed)
	{
		hash.update((const unsigned char*)baked_dict->mWearablesHashID.mData, UUID_BYTES);

		if (!generate_valid_hash)
		{
			invalidateBakedTextureHash(hash);
		}
		hash.finalize();
		hash.raw_digest(hash_id.mData);
	}

	return hash_id;
}


