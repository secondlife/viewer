/** 
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpreviewanim.h"
#include "llbutton.h"
#include "llresmgr.h"
#include "llinventory.h"
#include "llinventoryview.h"
#include "llvoavatar.h"
#include "llagent.h"          // gAgent
#include "llkeyframemotion.h"
#include "llfilepicker.h"
#include "lllineeditor.h"
#include "lluictrlfactory.h"
#include "llvieweruictrlfactory.h"

extern LLAgent gAgent;

LLPreviewAnim::LLPreviewAnim(const std::string& name, const LLRect& rect, const std::string& title, const LLUUID& item_uuid, const S32& activate, const LLUUID& object_uuid )	:
	LLPreview( name, rect, title, item_uuid, object_uuid)
{
	gUICtrlFactory->buildFloater(this,"floater_preview_animation.xml");

	childSetAction("Anim play btn",playAnim,this);
	childSetAction("Anim audition btn",auditionAnim,this);

	LLInventoryItem* item = getItem();
	
	childSetCommitCallback("desc", LLPreview::onText, this);
	childSetText("desc", item->getDescription());
	childSetPrevalidate("desc", &LLLineEditor::prevalidatePrintableNotPipe);
	
	setTitle(title);

	if (!getHost())
	{
		LLRect curRect = getRect();
		translate(rect.mLeft - curRect.mLeft, rect.mTop - curRect.mTop);
	}

	// preload the animation
	if(item)
	{
		gAgent.getAvatarObject()->createMotion(item->getAssetUUID());
	}
	
	switch ( activate ) 
	{
		case 1:
		{
			playAnim( (void *) this );
			break;
		}
		case 2:
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
void LLPreviewAnim::endAnimCallback( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;

	self->childSetValue("Anim play btn", FALSE);
	self->childSetValue("Anim audition btn", FALSE);

}

// static
void LLPreviewAnim::playAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = LLUICtrlFactory::getButtonByName(self, "Anim play btn");
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
				motion->setDeactivateCallback(&endAnimCallback, (void *)self);
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
	LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		LLButton* btn = LLUICtrlFactory::getButtonByName(self, "Anim audition btn");
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
				motion->setDeactivateCallback(&endAnimCallback, (void *)self);
		}
		else
		{
			gAgent.getAvatarObject()->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);
		}
	}
}

void LLPreviewAnim::saveAnim( void *userdata )
{
	LLPreviewAnim* self = (LLPreviewAnim*) userdata;
	LLInventoryItem *item = self->getItem();

	if(item)
	{
		LLKeyframeMotion* motionp = (LLKeyframeMotion*)gAgent.getAvatarObject()->createMotion( item->getAssetUUID() );
		if (motionp && motionp->isLoaded())
		{
			LLFilePicker& picker = LLFilePicker::instance();
			LLString proposed_name = item->getName() + LLString(".xaf");
			if (picker.getSaveFile(LLFilePicker::FFSAVE_ANIM, proposed_name.c_str()))
			{
					apr_file_t* fp = ll_apr_file_open(picker.getFirstFile(), LL_APR_W);
					if (!fp)
					{
						llwarns << "Unable to open file " << picker.getFirstFile() << llendl;
						return;
					}
					motionp->writeCAL3D(fp);
					apr_file_close(fp);
			}
		}
	}
}

void LLPreviewAnim::onClose(bool app_quitting)
{
	LLInventoryItem *item = getItem();

	if(item)
	{
		gAgent.getAvatarObject()->stopMotion(item->getAssetUUID());
		gAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);
					
		LLVOAvatar* avatar = gAgent.getAvatarObject();
		LLMotion*   motion = avatar->findMotion(item->getAssetUUID());
		
		if (motion)
			motion->setDeactivateCallback(NULL, (void *)NULL);

	}
	destroy();
}
