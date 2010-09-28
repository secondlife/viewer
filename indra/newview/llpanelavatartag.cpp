/** 
 * @file llpanelavatartag.cpp
 * @brief Avatar tag panel
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llpanelavatartag.h"

#include "lluictrlfactory.h"
#include "llavatariconctrl.h"
#include "lltextbox.h"

LLPanelAvatarTag::LLPanelAvatarTag(const LLUUID& key, const std::string im_time)
	: LLPanel()
	, mAvatarId(LLUUID::null)
//	, mFadeTimer()
{
	buildFromFile( "panel_avatar_tag.xml");
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
