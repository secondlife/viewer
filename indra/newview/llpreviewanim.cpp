/** 
 * @file llpreviewanim.cpp
 * @brief LLPreviewAnim class implementation
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
	mCommitCallbackRegistrar.add("PreviewAnim.Play", boost::bind(&LLPreviewAnim::play, this, _2));
}

// virtual
BOOL LLPreviewAnim::postBuild()
{
	const LLInventoryItem* item = getItem();
	if(item)
	{
		gAgentAvatarp->createMotion(item->getAssetUUID()); // preload the animation
		getChild<LLUICtrl>("desc")->setValue(item->getDescription());
	}

	childSetCommitCallback("desc", LLPreview::onText, this);
	getChild<LLLineEditor>("desc")->setPrevalidate(&LLTextValidate::validateASCIIPrintableNoPipe);

	return LLPreview::postBuild();
}

// static
// llinventorybridge also calls into here
void LLPreviewAnim::play(const LLSD& param)
{
	const LLInventoryItem *item = getItem();

	if(item)
	{
		LLUUID itemID=item->getAssetUUID();

		std::string btn_name = param.asString();
		LLButton* btn_inuse;
		LLButton* btn_other;

		if ("Inworld" == btn_name)
		{
			btn_inuse = getChild<LLButton>("Inworld");
			btn_other = getChild<LLButton>("Locally");
		}
		else if ("Locally" == btn_name)
		{
			btn_inuse = getChild<LLButton>("Locally");
			btn_other = getChild<LLButton>("Inworld");
		}
		else
		{
			return;
		}

		if (btn_inuse)
		{
			btn_inuse->toggleState();
		}

		if (btn_other)
		{
			btn_other->setEnabled(false);
		}
		
		if (getChild<LLUICtrl>(btn_name)->getValue().asBoolean() ) 
		{
			if("Inworld" == btn_name)
			{
				gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_START);
			}
			else
			{
				gAgentAvatarp->startMotion(item->getAssetUUID());
			}

			LLMotion* motion = gAgentAvatarp->findMotion(itemID);
			if (motion)
			{
				mItemID = itemID;
				mDidStart = false;
			}
		}
		else
		{
			gAgentAvatarp->stopMotion(itemID);
			gAgent.sendAnimationRequest(itemID, ANIM_REQUEST_STOP);

			if (btn_other)
			{
				btn_other->setEnabled(true);
			}
		}
	}
}

// virtual
void LLPreviewAnim::draw()
{
	LLPreview::draw();
	if (!this->mItemID.isNull())
	{
		LLMotion* motion = gAgentAvatarp->findMotion(this->mItemID);
		if (motion)
		{
			if (motion->isStopped() && this->mDidStart)
			{
				cleanup();
			}
			if(gAgentAvatarp->isMotionActive(this->mItemID) && !this->mDidStart)
			{
				this->mDidStart = true;
			}
		}
	}
}

// virtual
void LLPreviewAnim::cleanup()
{
	this->mItemID = LLUUID::null;
	this->mDidStart = false;
	getChild<LLUICtrl>("Inworld")->setValue(FALSE);
	getChild<LLUICtrl>("Locally")->setValue(FALSE);
	getChild<LLUICtrl>("Inworld")->setEnabled(true);
	getChild<LLUICtrl>("Locally")->setEnabled(true);
}

// virtual
void LLPreviewAnim::onClose(bool app_quitting)
{
	const LLInventoryItem *item = getItem();

	if(item)
	{
		gAgentAvatarp->stopMotion(item->getAssetUUID());
		gAgent.sendAnimationRequest(item->getAssetUUID(), ANIM_REQUEST_STOP);
	}
}
