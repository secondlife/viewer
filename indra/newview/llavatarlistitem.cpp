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
#include "lltextutil.h"
#include "llbutton.h"

bool LLAvatarListItem::mStaticInitialized = false;
S32 LLAvatarListItem::mIconWidth = 0;
S32 LLAvatarListItem::mInfoBtnWidth = 0;
S32 LLAvatarListItem::mProfileBtnWidth = 0;
S32 LLAvatarListItem::mSpeakingIndicatorWidth = 0;

LLAvatarListItem::LLAvatarListItem(bool not_from_ui_factory/* = true*/)
:	LLPanel(),
	mAvatarIcon(NULL),
	mAvatarName(NULL),
	mLastInteractionTime(NULL),
	mSpeakingIndicator(NULL),
	mInfoBtn(NULL),
	mProfileBtn(NULL),
	mContextMenu(NULL),
	mOnlineStatus(E_UNKNOWN),
	mShowInfoBtn(true),
	mShowProfileBtn(true)
{
	if (not_from_ui_factory)
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_list_item.xml");
	}
	// *NOTE: mantipov: do not use any member here. They can be uninitialized here in case instance
	// is created from the UICtrlFactory
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

	// Remember avatar icon width including its padding from the name text box,
	// so that we can hide and show the icon again later.
	if (!mStaticInitialized)
	{
		mIconWidth = mAvatarName->getRect().mLeft - mAvatarIcon->getRect().mLeft;
		mInfoBtnWidth = mInfoBtn->getRect().mRight - mSpeakingIndicator->getRect().mRight;
		mProfileBtnWidth = mProfileBtn->getRect().mRight - mInfoBtn->getRect().mRight;
		mSpeakingIndicatorWidth = mSpeakingIndicator->getRect().mRight - mAvatarName->getRect().mRight;

		mStaticInitialized = true;
	}

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
	mInfoBtn->setVisible(mShowInfoBtn);
	mProfileBtn->setVisible(mShowProfileBtn);

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
	mAvatarNameStyle.color = online ? LLColor4::white : LLColor4::grey;
	setNameInternal(mAvatarName->getText(), mHighlihtSubstring);

	// Make the icon fade if the avatar goes offline.
	mAvatarIcon->setColor(online ? LLColor4::white : LLColor4::smoke);
}

void LLAvatarListItem::setName(const std::string& name)
{
	setNameInternal(name, mHighlihtSubstring);
}

void LLAvatarListItem::setHighlight(const std::string& highlight)
{
	setNameInternal(mAvatarName->getText(), mHighlihtSubstring = highlight);
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

void LLAvatarListItem::setLastInteractionTime(U32 secs_since)
{
	mLastInteractionTime->setValue(formatSeconds(secs_since));
}

void LLAvatarListItem::setShowInfoBtn(bool show)
{
	// Already done? Then do nothing.
	if(mShowInfoBtn == show)
		return;
	mShowInfoBtn = show;
	S32 width_delta = show ? - mInfoBtnWidth : mInfoBtnWidth;

	//Translating speaking indicator
	mSpeakingIndicator->translate(width_delta, 0);
	//Reshaping avatar name
	mAvatarName->reshape(mAvatarName->getRect().getWidth() + width_delta, mAvatarName->getRect().getHeight());
}

void LLAvatarListItem::setShowProfileBtn(bool show)
{
	// Already done? Then do nothing.
	if(mShowProfileBtn == show)
			return;
	mShowProfileBtn = show;
	S32 width_delta = show ? - mProfileBtnWidth : mProfileBtnWidth;

	//Translating speaking indicator
	mSpeakingIndicator->translate(width_delta, 0);
	//Reshaping avatar name
	mAvatarName->reshape(mAvatarName->getRect().getWidth() + width_delta, mAvatarName->getRect().getHeight());
}

void LLAvatarListItem::setSpeakingIndicatorVisible(bool visible)
{
	// Already done? Then do nothing.
	if (mSpeakingIndicator->getVisible() == (BOOL)visible)
		return;
	mSpeakingIndicator->setVisible(visible);
	S32 width_delta = visible ? - mSpeakingIndicatorWidth : mSpeakingIndicatorWidth;

	//Reshaping avatar name
	mAvatarName->reshape(mAvatarName->getRect().getWidth() + width_delta, mAvatarName->getRect().getHeight());
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
	name_rect.mLeft += visible ? mIconWidth : -mIconWidth;
	mAvatarName->setRect(name_rect);
}

void LLAvatarListItem::onInfoBtnClick()
{
	LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", mAvatarId));

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

//== PRIVATE SECITON ==========================================================

void LLAvatarListItem::setNameInternal(const std::string& name, const std::string& highlight)
{
	LLTextUtil::textboxSetHighlightedVal(mAvatarName, mAvatarNameStyle, name, highlight);
	mAvatarName->setToolTip(name);
}

void LLAvatarListItem::onNameCache(const std::string& first_name, const std::string& last_name)
{
	std::string name = first_name + " " + last_name;
	setName(name);
}

void LLAvatarListItem::reshapeAvatarName()
{
	S32 width_delta = 0;
	width_delta += mShowProfileBtn ? mProfileBtnWidth : 0;
	width_delta += mSpeakingIndicator->getVisible() ? mSpeakingIndicatorWidth : 0;
	width_delta += mAvatarIcon->getVisible() ? mIconWidth : 0;
	width_delta += mShowInfoBtn ? mInfoBtnWidth : 0;
	width_delta += mLastInteractionTime->getVisible() ? mLastInteractionTime->getRect().getWidth() : 0;

	S32 height = mAvatarName->getRect().getHeight();
	S32 width  = getRect().getWidth() - width_delta;

	mAvatarName->reshape(width, height);
}

// Convert given number of seconds to a string like "23 minutes", "15 hours" or "3 years",
// taking i18n into account. The format string to use is taken from the panel XML.
std::string LLAvatarListItem::formatSeconds(U32 secs)
{
	static const U32 LL_ALI_MIN		= 60;
	static const U32 LL_ALI_HOUR	= LL_ALI_MIN	* 60;
	static const U32 LL_ALI_DAY		= LL_ALI_HOUR	* 24;
	static const U32 LL_ALI_WEEK	= LL_ALI_DAY	* 7;
	static const U32 LL_ALI_MONTH	= LL_ALI_DAY	* 30;
	static const U32 LL_ALI_YEAR	= LL_ALI_DAY	* 365;

	std::string fmt; 
	U32 count = 0;

	if (secs >= LL_ALI_YEAR)
	{
		fmt = "FormatYears"; count = secs / LL_ALI_YEAR;
	}
	else if (secs >= LL_ALI_MONTH)
	{
		fmt = "FormatMonths"; count = secs / LL_ALI_MONTH;
	}
	else if (secs >= LL_ALI_WEEK)
	{
		fmt = "FormatWeeks"; count = secs / LL_ALI_WEEK;
	}
	else if (secs >= LL_ALI_DAY)
	{
		fmt = "FormatDays"; count = secs / LL_ALI_DAY;
	}
	else if (secs >= LL_ALI_HOUR)
	{
		fmt = "FormatHours"; count = secs / LL_ALI_HOUR;
	}
	else if (secs >= LL_ALI_MIN)
	{
		fmt = "FormatMinutes"; count = secs / LL_ALI_MIN;
	}
	else
	{
		fmt = "FormatSeconds"; count = secs;
	}

	LLStringUtil::format_map_t args;
	args["[COUNT]"] = llformat("%u", count);
	return getString(fmt, args);
}
