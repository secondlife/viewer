/**
 * @file lltoastimpanel.cpp
 * @brief Panel for IM toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
#include "lltoastimpanel.h"

#include "llagent.h"
#include "llavatarnamecache.h"
#include "llfloaterreg.h"
#include "llgroupactions.h"
#include "llgroupiconctrl.h"
#include "llimview.h"
#include "llnotifications.h"
#include "llinstantmessage.h"
#include "lltooltip.h"

#include "llviewerchat.h"

const S32 LLToastIMPanel::DEFAULT_MESSAGE_MAX_LINE_COUNT	= 6;

//--------------------------------------------------------------------------
LLToastIMPanel::LLToastIMPanel(LLToastIMPanel::Params &p) :	LLToastPanel(p.notification),
								mAvatarIcon(NULL), mAvatarName(NULL),
								mTime(NULL), mMessage(NULL), mGroupIcon(NULL)
{
	buildFromFile( "panel_instant_message.xml");

	mGroupIcon = getChild<LLGroupIconCtrl>("group_icon");
	mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
	mAdhocIcon = getChild<LLAvatarIconCtrl>("adhoc_icon");
	mAvatarName = getChild<LLTextBox>("user_name");
	mTime = getChild<LLTextBox>("time_box");
	mMessage = getChild<LLTextBox>("message");
	mMessage->setContentTrusted(false);

	LLStyle::Params style_params;
	LLFontGL* fontp = LLViewerChat::getChatFont();
	std::string font_name = LLFontGL::nameFromFont(fontp);
	std::string font_size = LLFontGL::sizeFromFont(fontp);
	style_params.font.name(font_name);
	style_params.font.size(font_size);
	
	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(p.session_id);
	mIsGroupMsg = (im_session && im_session->mSessionType == LLIMModel::LLIMSession::GROUP_SESSION);
	std::string title = mIsGroupMsg ? im_session->mName : p.from;
	mAvatarName->setValue(title);

	//Handle IRC styled /me messages.
	std::string prefix = p.message.substr(0, 4);
	if (prefix == "/me " || prefix == "/me'")
	{
		//style_params.font.style = "UNDERLINE";
		mMessage->clear();
		
		style_params.font.style ="ITALIC";
		mMessage->appendText(p.from, false, style_params);

		style_params.font.style = "ITALIC";
		mMessage->appendText(p.message.substr(3), false, style_params);
	}
	else
	{
		if (mIsGroupMsg)
		{
			LLAvatarName avatar_name;
			LLAvatarNameCache::get(p.avatar_id, &avatar_name);
			p.message = "[From " + avatar_name.getDisplayName() + "]\n" + p.message;
		}
		style_params.font.style =  "NORMAL";
		mMessage->setText(p.message, style_params);
	}

	mTime->setValue(p.time);
	mSessionID = p.session_id;
	mAvatarID = p.avatar_id;
	mNotification = p.notification;



	initIcon();

	S32 maxLinesCount;
	std::istringstream ss( getString("message_max_lines_count") );
	if (!(ss >> maxLinesCount))
	{
		maxLinesCount = DEFAULT_MESSAGE_MAX_LINE_COUNT;
	}
	snapToMessageHeight(mMessage, maxLinesCount);
}

//--------------------------------------------------------------------------
LLToastIMPanel::~LLToastIMPanel()
{
}

//virtual
bool LLToastIMPanel::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if (LLPanel::handleMouseUp(x,y,mask) == false)
	{
		mNotification->respond(mNotification->getResponseTemplate());
	}

	return true;
}

//virtual
bool LLToastIMPanel::handleToolTip(S32 x, S32 y, MASK mask)
{
	// It's not our direct child, so parentPointInView() doesn't work.
	LLRect ctrl_rect;

	mAvatarName->localRectToOtherView(mAvatarName->getLocalRect(), &ctrl_rect, this);
	if (ctrl_rect.pointInRect(x, y))
	{
		spawnNameToolTip();
		return true;
	}

	mGroupIcon->localRectToOtherView(mGroupIcon->getLocalRect(), &ctrl_rect, this);
	if(mGroupIcon->getVisible() && ctrl_rect.pointInRect(x, y))
	{
		spawnGroupIconToolTip();
		return true;
	}

	return LLToastPanel::handleToolTip(x, y, mask);
}

void LLToastIMPanel::spawnNameToolTip()
{
	// Spawn at right side of the name textbox.
	LLRect sticky_rect = mAvatarName->calcScreenRect();
	S32 icon_x =
		llmin(sticky_rect.mLeft + mAvatarName->getTextPixelWidth() + 3, sticky_rect.mRight);
	LLCoordGL pos(icon_x, sticky_rect.mTop);

	LLToolTip::Params params;
	params.background_visible(false);
	if(!mIsGroupMsg)
	{
	params.click_callback(boost::bind(&LLFloaterReg::showInstance, "inspect_avatar", LLSD().with("avatar_id", mAvatarID), false));
	}
	else
	{
		params.click_callback(boost::bind(&LLFloaterReg::showInstance, "inspect_group", LLSD().with("group_id", mSessionID), false));
	}
	params.delay_time(0.0f);		// spawn instantly on hover
	params.image(LLUI::getUIImage("Info_Small"));
	params.message("");
	params.padding(0);
	params.pos(pos);
	params.sticky_rect(sticky_rect);

	LLToolTipMgr::getInstance()->show(params);
}

void LLToastIMPanel::spawnGroupIconToolTip()
{
	// Spawn at right bottom side of group icon.
	LLRect sticky_rect = mGroupIcon->calcScreenRect();
	LLCoordGL pos(sticky_rect.mRight, sticky_rect.mBottom);

	LLGroupData g_data;
	if(!gAgent.getGroupData(mSessionID, g_data))
	{
		LL_WARNS() << "Error getting group data" << LL_ENDL;
	}

	LLInspector::Params params;
	params.fillFrom(LLUICtrlFactory::instance().getDefaultParams<LLInspector>());
	params.click_callback(boost::bind(&LLFloaterReg::showInstance, "inspect_group", LLSD().with("group_id", mSessionID), false));
	params.delay_time(0.100f);
	params.image(LLUI::getUIImage("Info_Small"));
	params.message(g_data.mName);
	params.padding(3);
	params.pos(pos);
	params.max_width(300);

	LLToolTipMgr::getInstance()->show(params);
}

void LLToastIMPanel::initIcon()
{
	mAvatarIcon->setVisible(false);
	mGroupIcon->setVisible(false);
	mAdhocIcon->setVisible(false);

	if(mAvatarName->getValue().asString() == SYSTEM_FROM)
	{
		// "sys_msg_icon" was disabled by Erica in the changeset: 5109 (85181bc92cbe)
		// and "dummy widget" warnings appeared in log.
		// It does not make sense to have such image with empty name. Removed for EXT-5057.
	}
	else
	{
		LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(mSessionID);
		if(!im_session)
		{
			LL_WARNS() << "Invalid IM session" << LL_ENDL;
			return;
		}

		switch(im_session->mSessionType)
		{
		case LLIMModel::LLIMSession::P2P_SESSION:
			mAvatarIcon->setVisible(true);
			mAvatarIcon->setValue(mAvatarID);
			break;
		case LLIMModel::LLIMSession::GROUP_SESSION:
			mGroupIcon->setVisible(true);
			mGroupIcon->setValue(mSessionID);
			break;
		case LLIMModel::LLIMSession::ADHOC_SESSION:
			mAdhocIcon->setVisible(true);
			mAdhocIcon->setValue(im_session->mOtherParticipantID);
			mAdhocIcon->setToolTip(im_session->mName);
			break;
		default:
			LL_WARNS() << "Unknown IM session type" << LL_ENDL;
			break;
		}
	}
}

// EOF
