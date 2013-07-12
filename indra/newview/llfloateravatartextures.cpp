/** 
 * @file llfloateravatartextures.cpp
 * @brief Debugging view showing underlying avatar textures and baked textures.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"
#include "llfloateravatartextures.h"

// library headers
#include "llavatarnamecache.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llviewerwearable.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"
#include "llviewerobjectlist.h"
#include "llvoavatarself.h"

using namespace LLAvatarAppearanceDefines;

LLFloaterAvatarTextures::LLFloaterAvatarTextures(const LLSD& id)
  : LLFloater(id),
	mID(id.asUUID())
{
}

LLFloaterAvatarTextures::~LLFloaterAvatarTextures()
{
}

BOOL LLFloaterAvatarTextures::postBuild()
{
	for (U32 i=0; i < TEX_NUM_INDICES; i++)
	{
		const std::string tex_name = LLAvatarAppearanceDictionary::getInstance()->getTexture(ETextureIndex(i))->mName;
		mTextures[i] = getChild<LLTextureCtrl>(tex_name);
	}
	mTitle = getTitle();

	childSetAction("Dump", onClickDump, this);

	refresh();
	return TRUE;
}

void LLFloaterAvatarTextures::draw()
{
	refresh();
	LLFloater::draw();
}

static void update_texture_ctrl(LLVOAvatar* avatarp,
								 LLTextureCtrl* ctrl,
								 ETextureIndex te)
{
	LLUUID id = IMG_DEFAULT_AVATAR;
	const LLAvatarAppearanceDictionary::TextureEntry* tex_entry = LLAvatarAppearanceDictionary::getInstance()->getTexture(te);
	if (tex_entry->mIsLocalTexture)
	{
		if (avatarp->isSelf())
		{
			const LLWearableType::EType wearable_type = tex_entry->mWearableType;
			LLViewerWearable *wearable = gAgentWearables.getViewerWearable(wearable_type, 0);
			if (wearable)
			{
				LLLocalTextureObject *lto = wearable->getLocalTextureObject(te);
				if (lto)
				{
					id = lto->getID();
				}
			}
		}
	}
	else
	{
		id = avatarp->getTE(te)->getID();
	}
	//id = avatarp->getTE(te)->getID();
	if (id == IMG_DEFAULT_AVATAR)
	{
		ctrl->setImageAssetID(LLUUID::null);
		ctrl->setToolTip(tex_entry->mName + " : " + std::string("IMG_DEFAULT_AVATAR"));
	}
	else
	{
		ctrl->setImageAssetID(id);
		ctrl->setToolTip(tex_entry->mName + " : " + id.asString());
	}
}

static LLVOAvatar* find_avatar(const LLUUID& id)
{
	LLViewerObject *obj = gObjectList.findObject(id);
	while (obj && obj->isAttachment())
	{
		obj = (LLViewerObject *)obj->getParent();
	}

	if (obj && obj->isAvatar())
	{
		return (LLVOAvatar*)obj;
	}
	else
	{
		return NULL;
	}
}

void LLFloaterAvatarTextures::refresh()
{
	if (gAgent.isGodlike())
	{
		LLVOAvatar *avatarp = find_avatar(mID);
		if (avatarp)
		{
			LLAvatarName av_name;
			if (LLAvatarNameCache::get(avatarp->getID(), &av_name))
			{
				setTitle(mTitle + ": " + av_name.getCompleteName());
			}
			for (U32 i=0; i < TEX_NUM_INDICES; i++)
			{
				update_texture_ctrl(avatarp, mTextures[i], ETextureIndex(i));
			}
		}
		else
		{
			setTitle(mTitle + ": " + getString("InvalidAvatar") + " (" + mID.asString() + ")");
		}
	}
}

// static
void LLFloaterAvatarTextures::onClickDump(void* data)
{
	if (gAgent.isGodlike())
	{
		const LLVOAvatarSelf* avatarp = gAgentAvatarp;
		if (!avatarp) return;
		for (S32 i = 0; i < avatarp->getNumTEs(); i++)
		{
			const LLTextureEntry* te = avatarp->getTE(i);
			if (!te) continue;

			const LLAvatarAppearanceDictionary::TextureEntry* tex_entry = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)(i));
			if (!tex_entry)
				continue;

			if (LLVOAvatar::isIndexLocalTexture((ETextureIndex)i))
			{
				LLUUID id = IMG_DEFAULT_AVATAR;
				LLWearableType::EType wearable_type = LLAvatarAppearanceDictionary::getInstance()->getTEWearableType((ETextureIndex)i);
				if (avatarp->isSelf())
				{
					LLViewerWearable *wearable = gAgentWearables.getViewerWearable(wearable_type, 0);
					if (wearable)
					{
						LLLocalTextureObject *lto = wearable->getLocalTextureObject(i);
						if (lto)
						{
							id = lto->getID();
						}
					}
				}
				if (id != IMG_DEFAULT_AVATAR)
				{
					llinfos << "TE " << i << " name:" << tex_entry->mName << " id:" << id << llendl;
				}
				else
				{
					llinfos << "TE " << i << " name:" << tex_entry->mName << " id:" << "<DEFAULT>" << llendl;
				}
			}
			else
			{
				llinfos << "TE " << i << " name:" << tex_entry->mName << " id:" << te->getID() << llendl;
			}
		}
	}
}
