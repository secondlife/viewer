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

#include "llavataractions.h"
#include "llavatarlistitem.h"

#include "llfloaterreg.h"
#include "llagent.h"
#include "lloutputmonitorctrl.h"
#include "llavatariconctrl.h"
#include "llbutton.h"

S32 LLAvatarListItem::sIconWidth = 0;

LLAvatarListItem::LLAvatarListItem()
:	LLPanel(),
	mAvatarIcon(NULL),
	mAvatarName(NULL),
	mLastInteractionTime(NULL),
	mSpeakingIndicator(NULL),
	mInfoBtn(NULL),
	mProfileBtn(NULL),
	mContextMenu(NULL),
	mOnlineStatus(E_UNKNOWN)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_list_item.xml");
	// Remember avatar icon width including its padding from the name text box,
	// so that we can hide and show the icon again later.
	if (!sIconWidth)
	{
		sIconWidth = mAvatarName->getRect().mLeft - mAvatarIcon->getRect().mLeft;
	}
}

LLAvatarListItem::~LLAvatarListItem()
{
	if (mAvatarId.notNull())
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);
}

BOOL  LLAvatarListItem::postBuild()
{
	mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
	mAvatarName = getChild<LLTextBox>("avatar_name");
	mLastInteractionTime = getChild<LLTextBox>("last_interaction");
	
	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");
	mInfoBtn = getChild<LLButton>("info_btn");
	mProfileBtn = getChild<LLButton>("profile_btn");

	mInfoBtn->setVisible(false);
	mInfoBtn->setClickedCallback(boost::bind(&LLAvatarListItem::onInfoBtnClick, this));

	mProfileBtn->setVisible(false);
	mProfileBtn->setClickedCallback(boost::bind(&LLAvatarListItem::onProfileBtnClick, this));

/*
	if(!p.buttons.profile)
	{
		delete mProfile;
		mProfile = NULL;

		LLRect rect;

		rect.setLeftTopAndSize(mName->getRect().mLeft, mName->getRect().mTop, mName->getRect().getWidth() + 30, mName->getRect().getHeight());
		mName->setRect(rect);

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
	mProfileBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLAvatarListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	childSetVisible("hovered_icon", false);
	mInfoBtn->setVisible(false);
	mProfileBtn->setVisible(false);

	LLPanel::onMouseLeave(x, y, mask);
}

// virtual, called by LLAvatarTracker
void LLAvatarListItem::changed(U32 mask)
{
	// no need to check mAvatarId for null in this case
	setOnline(LLAvatarTracker::instance().isBuddyOnline(mAvatarId));
}

void LLAvatarListItem::setOnline(bool online)
{
	// *FIX: setName() overrides font style set by setOnline(). Not an issue ATM.
	// *TODO: Make the colors configurable via XUI.

	if (mOnlineStatus != E_UNKNOWN && (bool) mOnlineStatus == online)
		return;

	mOnlineStatus = (EOnlineStatus) online;

	// Change avatar name font style depending on the new online status.
	LLStyle::Params style_params;
	style_params.color = online ? LLColor4::white : LLColor4::grey;

	// Rebuild the text to change its style.
	std::string text = mAvatarName->getText();
	mAvatarName->setText(LLStringUtil::null);
	mAvatarName->appendText(text, false, style_params);

	// Make the icon fade if the avatar goes offline.
	mAvatarIcon->setColor(online ? LLColor4::white : LLColor4::smoke);
}

void LLAvatarListItem::setName(const std::string& name)
{
	mAvatarName->setValue(name);
	mAvatarName->setToolTip(name);
}

void LLAvatarListItem::setAvatarId(const LLUUID& id, bool ignore_status_changes)
{
	if (mAvatarId.notNull())
		LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarId, this);

	mAvatarId = id;
	mAvatarIcon->setValue(id);
	mSpeakingIndicator->setSpeakerId(id);

	// We'll be notified on avatar online status changes
	if (!ignore_status_changes && mAvatarId.notNull())
		LLAvatarTracker::instance().addParticularFriendObserver(mAvatarId, this);

	// Set avatar name.
	gCacheName->get(id, FALSE, boost::bind(&LLAvatarListItem::onNameCache, this, _2, _3));
}

void LLAvatarListItem::showLastInteractionTime(bool show)
{
	if (show)
		return;

	LLRect	name_rect	= mAvatarName->getRect();
	LLRect	time_rect	= mLastInteractionTime->getRect();

	mLastInteractionTime->setVisible(false);
	name_rect.mRight += (time_rect.mRight - name_rect.mRight);
	mAvatarName->setRect(name_rect);
}

void LLAvatarListItem::setLastInteractionTime(const std::string& val)
{
	mLastInteractionTime->setValue(val);
}

void LLAvatarListItem::setAvatarIconVisible(bool visible)
{
	// Already done? Then do nothing.
	if (mAvatarIcon->getVisible() == (BOOL)visible)
		return;

	// Show/hide avatar icon.
	mAvatarIcon->setVisible(visible);

	// Move the avatar name horizontally by icon size + its distance from the avatar name.
	LLRect name_rect = mAvatarName->getRect();
	name_rect.mLeft += visible ? sIconWidth : -sIconWidth;
	mAvatarName->setRect(name_rect);
}

void LLAvatarListItem::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().insert("avatar_id", mAvatarId));

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

void LLAvatarListItem::onProfileBtnClick()
{
	LLAvatarActions::showProfile(mAvatarId);
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
