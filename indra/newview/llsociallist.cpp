sDestroyImmediate
/** 
* @file llsociallist.cpp
* @brief Implementation of llsociallist
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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

#include "llsociallist.h"

#include "llavatariconctrl.h"
#include "lloutputmonitorctrl.h"
#include "lltextutil.h"
    
static LLDefaultChildRegistry::Register<LLSocialList> r("social_list");

LLSocialList::LLSocialList(const Params&p) : LLFlatListViewEx(p)
{

}

LLSocialList::~LLSocialList()
{

}

void LLSocialList::draw()
{
	LLFlatListView::draw();
}

void LLSocialList::refresh()
{

}

void LLSocialList::addNewItem(const LLUUID& id, const std::string& name, BOOL is_online, EAddPosition pos)
{
	LLSocialListItem * item = new LLSocialListItem();
	item->setName(name, mNameFilter);
	addItem(item, id, pos);
}

LLSocialListItem::LLSocialListItem()
{
	buildFromFile("panel_avatar_list_item.xml");
}

LLSocialListItem::~LLSocialListItem()
{

}

BOOL LLSocialListItem::postBuild()
{
	mIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
	mLabelTextBox = getChild<LLTextBox>("avatar_name");

	mLastInteractionTime = getChild<LLTextBox>("last_interaction");
	mIconPermissionOnline = getChild<LLIconCtrl>("permission_online_icon");
	mIconPermissionMap = getChild<LLIconCtrl>("permission_map_icon");
	mIconPermissionEditMine = getChild<LLIconCtrl>("permission_edit_mine_icon");
	mIconPermissionEditTheirs = getChild<LLIconCtrl>("permission_edit_theirs_icon");
	mSpeakingIndicator = getChild<LLOutputMonitorCtrl>("speaking_indicator");
	mInfoBtn = getChild<LLButton>("info_btn");
	mProfileBtn = getChild<LLButton>("profile_btn");

	mLastInteractionTime->setVisible(false);
	mIconPermissionOnline->setVisible(false);
	mIconPermissionMap->setVisible(false);
	mIconPermissionEditMine->setVisible(false);
	mIconPermissionEditTheirs->setVisible(false);
	mSpeakingIndicator->setVisible(false);
	mInfoBtn->setVisible(false);
	mProfileBtn->setVisible(false);

	return TRUE;
}

void LLSocialListItem::setName(const std::string& name, const std::string& highlight)
{
	mLabel = name;
	LLTextUtil::textboxSetHighlightedVal(mLabelTextBox, mLabelTextBoxStyle, name, highlight);
}

void LLSocialListItem::setValue(const LLSD& value)
{
	getChildView("selected_icon")->setVisible( value["selected"]);
}

void LLSocialListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( true);
	mInfoBtn->setVisible(true);
	mProfileBtn->setVisible(true);

	LLPanel::onMouseEnter(x, y, mask);
}

void LLSocialListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	getChildView("hovered_icon")->setVisible( false);
	mInfoBtn->setVisible(false);
	mProfileBtn->setVisible(false);

	LLPanel::onMouseLeave(x, y, mask);
}
