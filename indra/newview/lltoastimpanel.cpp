/**
 * @file lltoastimpanel.cpp
 * @brief Panel for IM toasts.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
#include "lltoastimpanel.h"
#include "llimpanel.h"

const S32 LLToastIMPanel::MAX_MESSAGE_HEIGHT	= 50;
const S32 LLToastIMPanel::CAPTION_HEIGHT		= 30;
const S32 LLToastIMPanel::TOP_PAD 				= 5;

//--------------------------------------------------------------------------
LLToastIMPanel::LLToastIMPanel(LLToastIMPanel::Params &p) :	LLToastPanel(p.notification),
															mAvatar(NULL), mUserName(NULL),
															mTime(NULL), mMessage(NULL),
															mReplyBtn(NULL)
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_instant_message.xml");

	mAvatar = getChild<LLAvatarIconCtrl>("avatar");
	mUserName = getChild<LLTextBox>("user_name");
	mTime = getChild<LLTextBox>("time_box");
	mMessage = getChild<LLTextBox>("message");
	mReplyBtn = getChild<LLButton>("reply");	

	mMessage->setValue(p.message);
	mAvatar->setValue(p.avatar_id);
	mUserName->setValue(p.from);
	mTime->setValue(p.time);
	mSessionID = p.session_id;

	mReplyBtn->setClickedCallback(boost::bind(&LLToastIMPanel::onClickReplyBtn, this));

	snapToMessageHeight();
}

//--------------------------------------------------------------------------
LLToastIMPanel::~LLToastIMPanel()
{
}

//--------------------------------------------------------------------------
void LLToastIMPanel::snapToMessageHeight()
{
	S32 required_text_height = mMessage->getTextPixelHeight();
	S32 text_height = llmin(required_text_height, MAX_MESSAGE_HEIGHT);
	LLRect text_rect = mMessage->getRect();
	LLRect btn_rect = mReplyBtn->getRect();


	mMessage->reshape( text_rect.getWidth(), text_height, TRUE);
	mMessage->setValue(mMessage->getText());

	S32 panel_height = CAPTION_HEIGHT + text_height + btn_rect.getHeight() + TOP_PAD*5;
	reshape( getRect().getWidth(), panel_height, TRUE);
}

//--------------------------------------------------------------------------
void LLToastIMPanel::onClickReplyBtn()
{
	LLIMFloater::toggle(mSessionID);
}

//--------------------------------------------------------------------------

