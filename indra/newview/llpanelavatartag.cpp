/** 
 * @file llpanelavatartag.cpp
 * @brief Avatar tag panel
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

#include "llpanelavatartag.h"

#include "lluictrlfactory.h"
#include "llavatariconctrl.h"
#include "lltextbox.h"

LLPanelAvatarTag::LLPanelAvatarTag(const LLUUID& key, const std::string im_time)
	: LLPanel()
	, mAvatarId(LLUUID::null)
//	, mFadeTimer()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_avatar_tag.xml");
	setLeftButtonClickCallback(boost::bind(&LLPanelAvatarTag::onClick, this));
	setAvatarId(key);
	setTime(im_time);
}

LLPanelAvatarTag::~LLPanelAvatarTag()
{
	// Name callbacks will be automatically disconnected since LLPanel is trackable
}

BOOL LLPanelAvatarTag::postBuild()
{
	mIcon = getChild<LLAvatarIconCtrl>("avatar_tag_icon");
	mName = getChild<LLTextBox>("sender_tag_name");
	mTime = getChild<LLTextBox>("tag_time");
	return TRUE;
}

void LLPanelAvatarTag::draw()
{

}
void LLPanelAvatarTag::setName(const std::string& name)
{
	if (mName)
		mName->setText(name);
}

void LLPanelAvatarTag::setTime(const std::string& time)
{
	if (mTime)
		mTime->setText(time);
}


void LLPanelAvatarTag::setAvatarId(const LLUUID& avatar_id)
{
	mAvatarId = avatar_id;
	if (mIcon)
	{
		mIcon->setValue(avatar_id);
	}
	setName(std::string(mIcon->getFullName()));
}

boost::signals2::connection LLPanelAvatarTag::setLeftButtonClickCallback(
																  const commit_callback_t& cb)
{
	return setCommitCallback(cb);
}

BOOL LLPanelAvatarTag::handleMouseDown(S32 x, S32 y, MASK mask)
{
	onCommit();
	return TRUE;
}

void LLPanelAvatarTag::onClick()
{
	// Do the on click stuff.
}
