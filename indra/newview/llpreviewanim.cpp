/** 
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#include "llpreviewanim.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llinventory.h"
#include "llvoavatarself.h"
#include "llagent.h"          // gAgent
#include "llkeyframemotion.h"
#include "llfilepicker.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "lluictrlfactory.h"

extern LLAgent gAgent;

LLPreviewAnim::LLPreviewAnim(const LLSD& key)
	: LLPreview( key )
{
	//Called from floater reg: LLUICtrlFactory::getInstance()->buildFloater(this,"floater_preview_animation.xml", FALSE);
}

// static
void LLPreviewAnim::endAnimCallback( void *userdata )
{
	LLHandle<LLFloater>* handlep = ((LLHandle<LLFloater>*)userdata);
	LLFloater* self = handlep->get();
	delete handlep; // done with the handle
	if (self)
	{
		self->childSetValue("Anim play btn", FALSE);
		self->childSetValue("Anim audition btn", FALSE);
	}
}

// virtual
BOOL LLPreviewAnim::postBuild()
{
	const LLInventoryItem* item = getItem();
	if(item)
	{
		gAgent.getAvatarObject()->createMotion(item->getAssetUUID()); // preload the animation
		childSetText("desc", item->getDescription());
	}

	childSetAction("Anim play btn",playAnim, this);
	childSetAction("Anim audition btn",auditionAnim, this);

	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetPrevalidate("desc", &LLTextValidate::validateASCIIPrintableNoPipe);
	
	return LLPreview::postBuild();
}

void LLPreviewAnim::activate(e_activation_type type)
{
	switch ( type ) 
	{
		case PLAY:
		{
			playAnim( (void *) this );
			break;
		}
		case AUDITION:
		{
			auditionAnim( (void *) this );
			break;
		}
		default:
		{
		//do nothing
		}
	}
}

// static
void LLPreviewAnim::playAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = self->getChild<LLButton>("Anim play btn");
		if (btn)
		{
			btn->toggleState();
		}
		
		if (self->childGetValue("Anim play btn").asBoolean() ) 
		{
			self->mPauseRequest = NULL;
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_START);
			
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			LLMotion*   motion = avatar->findMotion(itemID);
			
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback, (void *)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgent.getAvatarObject()->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

// static
void LLPreviewAnim::auditionAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	const LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = self->getChild<LLButton>("Anim audition btn");
		if (btn)
		{
			btn->toggleState();
		}
		
		if (self->childGetValue("Anim audition btn").asBoolean() ) 
		{
			self->mPauseRequest = NULL;
			gAgent.getAvatarObject()->startMotion(item->getAssetUUID());
			
			LLVOAvatar* avatar = gAgent.getAvatarObject();
			LLMotion*   motion = avatar->findMotion(itemID);
			
			if (motion)
			{
				motion->setDeactivateCallback(&endAnimCallback, (void *)(new LLHandle<LLFloater>(self->getHandle())));
			}
		}
		else
		{
			gAgent.getAvatarObject()->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

// virtual
void LLPreviewAnim::onClose(bool app_quitting)
{
	const LLInventoryItem *item = getItem();

	if(item)
	{
		gAgent.getAvatarObject()->stopMotion(item->getAssetUUID());
		gAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);
					
		LLVOAvatar* avatar = gAgent.getAvatarObject();
		LLMotion*   motion = avatar->findMotion(item->getAssetUUID());
		
		if (motion)
		{
			// *TODO: minor memory leak here, user data is never deleted (Use real callbacks)
			motion->setDeactivateCallback(NULL, (void *)NULL);
		}
	}
}
