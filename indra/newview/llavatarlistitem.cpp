/** 
 * @file llavatarlistitem.cpp
 * @avatar list item source file
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "llavatarlistitem.h"

#include "llfloaterreg.h"
#include "llagent.h"
#include "lloutputmonitorctrl.h"
#include "llavatariconctrl.h"
#include "llbutton.h"


LLAvatarListItem::LLAvatarListItem()
:	LLPanel(),
	mAvatarIcon(NULL),
	mAvatarName(NULL),
	mStatus(NULL),
	mSpeakingIndicator(NULL),
	mInfoBtn(NULL),
	mContextMenu(NULL),
	mAvatarId(LLUUID::null)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_list_item.xml");
}

BOOL  LLAvatarListItem::postBuild()
{
	mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
	mAvatarName = getChild<LLTextBox>("avatar_name");
	mStatus = getChild<LLTextBox>("avatar_status");

	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");
	mInfoBtn = getChild<LLButton>("info_btn");

	mInfoBtn->setVisible(false);
	mInfoBtn->setClickedCallback(boost::bind(&LLAvatarListItem::onInfoBtnClick, this));

/*
	if(!p.buttons.profile)
	{
		delete mProfile;
		mProfile = NULL;

		LLRect rect;

		rect.setLeftTopAndSize(mName->getRect().mLeft, mName->getRect().mTop, mName->getRect().getWidth() + 30, mName->getRect().getHeight());
		mName->setRect(rect);

		if(mStatus)
		{
			rect.setLeftTopAndSize(mStatus->getRect().mLeft + 30, mStatus->getRect().mTop, mStatus->getRect().getWidth(), mStatus->getRect().getHeight());
			mStatus->setRect(rect);
		}

		if(mLocator)
		{
			rect.setLeftTopAndSize(mLocator->getRect().mLeft + 30, mLocator->getRect().mTop, mLocator->getRect().getWidth(), mLocator->getRect().getHeight());
			mLocator->setRect(rect);
		}

		if(mInfo)
		{
			rect.setLeftTopAndSize(mInfo->getRect().mLeft + 30, mInfo->getRect().mTop, mInfo->getRect().getWidth(), mInfo->getRect().getHeight());
			mInfo->setRect(rect);
		}
	}
*/
	return TRUE;
}

void LLAvatarListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", true);
	mInfoBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLAvatarListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);
	mInfoBtn->setVisible(false);

	LLPanel::onMouseLeave(x, y, mask);
}

// virtual
BOOL LLAvatarListItem::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	if (mContextMenu)
		mContextMenu->show(this, const_cast<const LLUUID&>(mAvatarId), x, y);

	return LLPanel::handleRightMouseDown(x, y, mask);
}

void LLAvatarListItem::setStatus(const std::string& status)
{
	mStatus->setValue(status);
}

void LLAvatarListItem::setName(const std::string& name)
{
	mAvatarName->setValue(name);
	mAvatarName->setToolTip(name);
}

void LLAvatarListItem::setAvatarId(const LLUUID& id)
{
	mAvatarId = id;
	mAvatarIcon->setValue(id);
	mSpeakingIndicator->setSpeakerId(id);

	// Set avatar name.
	gCacheName->get(id, FALSE, boost::bind(&LLAvatarListItem::onNameCache, this, _2, _3));
}

void LLAvatarListItem::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", mAvatarId);

	/* TODO fix positioning of inspector
	localPointToScreen(mXPos, mYPos, &mXPos, &mYPos);
	
	
	LLRect rect;

	// *TODO Vadim: rewrite this. "+= -" looks weird.
	S32 delta = mYPos - inspector->getRect().getHeight();
	if(delta < 0)
	{
		mYPos += -delta;
	}
	
	rect.setLeftTopAndSize(mXPos, mYPos,
	inspector->getRect().getWidth(), inspector->getRect().getHeight()); 
	inspector->setRect(rect);
	inspector->setFrontmost(true);
	inspector->setVisible(true);
	*/
}

void LLAvatarListItem::showStatus(bool show_status)
{
	// *HACK: dirty hack until we can determine correct avatar status (EXT-1076).

	if (show_status)
		return;

	LLRect	name_rect	= mAvatarName->getRect();
	LLRect	status_rect	= mStatus->getRect();

	mStatus->setVisible(show_status);
	name_rect.mRight += (status_rect.mRight - name_rect.mRight);
	mAvatarName->setRect(name_rect);
}

void LLAvatarListItem::setValue( const LLSD& value )
{
	if (!value.isMap()) return;;
	if (!value.has("selected")) return;
	childSetVisible("selected_icon", value["selected"]);
}

const LLUUID& LLAvatarListItem::getAvatarId() const
{
	return mAvatarId;
}

const std::string LLAvatarListItem::getAvatarName() const
{
	return mAvatarName->getValue();
}

void LLAvatarListItem::onNameCache(const std::string& first_name, const std::string& last_name)
{
	std::string name = first_name + " " + last_name;
	mAvatarName->setValue(name);
	mAvatarName->setToolTip(name);
}
