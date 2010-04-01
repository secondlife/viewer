/** 
 * @file llfloateravatartextures.cpp
 * @brief Debugging view showing underlying avatar textures and baked textures.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llfloateravatartextures.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "lltexturectrl.h"
#include "lluictrlfactory.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"

using namespace LLVOAvatarDefines;

LLFloaterAvatarTextures::LLFloaterAvatarTextures(const LLSD& id)
  : LLFloater(id),
	mID(id.asUUID())
{
// 	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_avatar_textures.xml");
}

LLFloaterAvatarTextures::~LLFloaterAvatarTextures()
{
}

BOOL LLFloaterAvatarTextures::postBuild()
{
	for (U32 i=0; i < TEX_NUM_INDICES; i++)
	{
		const std::string tex_name = LLVOAvatarDictionary::getInstance()->getTexture(ETextureIndex(i))->mName;
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
	const LLVOAvatarDictionary::TextureEntry* tex_entry = LLVOAvatarDictionary::getInstance()->getTexture(te);
	if (tex_entry->mIsLocalTexture)
	{
		const EWearableType wearable_type = tex_entry->mWearableType;
		LLWearable *wearable = gAgentWearables.getWearable(wearable_type, 0);
		if (wearable)
		{
			LLLocalTextureObject *lto = wearable->getLocalTextureObject(te);
			if (lto)
			{
				id = lto->getID();
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
		ctrl->setToolTip(std::string("IMG_DEFAULT_AVATAR"));
	}
	else
	{
		ctrl->setImageAssetID(id);
		ctrl->setToolTip(id.asString());
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
			std::string fullname;
			if (gCacheName->getFullName(avatarp->getID(), fullname))
			{
				setTitle(mTitle + ": " + fullname);
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
		LLFloaterAvatarTextures* self = (LLFloaterAvatarTextures*)data;
		LLVOAvatar* avatarp = find_avatar(self->mID);
		if (!avatarp) return;

		for (S32 i = 0; i < avatarp->getNumTEs(); i++)
		{
			const LLTextureEntry* te = avatarp->getTE(i);
			if (!te) continue;

			if (LLVOAvatar::isIndexLocalTexture((ETextureIndex)i))
			{
				LLUUID id = IMG_DEFAULT_AVATAR;
				EWearableType wearable_type = LLVOAvatarDictionary::getInstance()->getTEWearableType((ETextureIndex)i);
				LLWearable *wearable = gAgentWearables.getWearable(wearable_type, 0);
				if (wearable)
				{
					LLLocalTextureObject *lto = wearable->getLocalTextureObject(i);
					if (lto)
					{
						id = lto->getID();
					}
				}
				if (id != IMG_DEFAULT_AVATAR)
				{
					llinfos << "Avatar TE " << i << " id " << id << llendl;
				}
				else
				{
					llinfos << "Avatar TE " << i << " id " << "<DEFAULT>" << llendl;
				}
			}
			else
			{
				llinfos << "Avatar TE " << i << " id " << te->getID() << llendl;
			}
		}
	}
}
