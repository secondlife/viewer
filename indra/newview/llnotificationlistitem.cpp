/** 
 * @file llnotificationlistitem.cpp
 * @brief                                   
 *
 * $LicenseInfo:firstyear=2015&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2015, Linden Research, Inc.
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


#include "llviewerprecompiledheaders.h" // must be first include

#include "llnotificationlistitem.h"

#include "llwindow.h"
#include "v4color.h"
#include "lltrans.h"
#include "lluicolortable.h"

LLNotificationListItem::LLNotificationListItem(const Params& p) : LLPanel(p),
    mParams(p),
    mTitleBox(NULL),
    mExpandBtn(NULL),
    mCondenseBtn(NULL),
    mCloseBtn(NULL),
    mCondensedViewPanel(NULL),
    mExpandedViewPanel(NULL),
    mMainPanel(NULL)
{
    mNotificationName = p.notification_name;
}

BOOL LLNotificationListItem::postBuild()
{
    BOOL rv = LLPanel::postBuild();
    mTitleBox = getChild<LLTextBox>("notification_title");
    mTitleBoxExp = getChild<LLTextBox>("notification_title_exp"); 
    mNoticeTextExp = getChild<LLTextBox>("notification_text_exp"); 

    mTimeBox = getChild<LLTextBox>("notification_time");
    mTimeBoxExp = getChild<LLTextBox>("notification_time_exp");
    mExpandBtn = getChild<LLButton>("expand_btn");
    mCondenseBtn = getChild<LLButton>("condense_btn");
    mCloseBtn = getChild<LLButton>("close_btn");
    mCloseBtnExp = getChild<LLButton>("close_expanded_btn");
    mVerticalStack = getChild<LLLayoutStack>("item_vertical_stack");

    mTitleBox->setValue(mParams.title);
    mTitleBoxExp->setValue(mParams.title);
    mNoticeTextExp->setValue(mParams.title);

    mTimeBox->setValue(buildNotificationDate(mParams.time_stamp));
    mTimeBoxExp->setValue(buildNotificationDate(mParams.time_stamp));

    mExpandBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickExpandBtn,this));
    mCondenseBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCondenseBtn,this));

    //mCloseBtn and mCloseExpandedBtn share the same callback
    mCloseBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCloseBtn,this));
    mCloseBtnExp->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCloseBtn,this));

    mCondensedViewPanel = getChild<LLPanel>("layout_panel_condensed_view");
    mExpandedViewPanel = getChild<LLPanel>("layout_panel_expanded_view");
    mMainPanel = getChild<LLPanel>("main_panel");

    std::string expanded_heigt_str = getString("item_expanded_height");
    std::string condensed_heigt_str = getString("item_condensed_height");

    mExpandedHeight = (S32)atoi(expanded_heigt_str.c_str());
    mCondensedHeight = (S32)atoi(condensed_heigt_str.c_str());

    setExpanded(FALSE);
    return rv;
}

LLNotificationListItem::~LLNotificationListItem()
{
}

//static
std::string LLNotificationListItem::buildNotificationDate(const LLDate& time_stamp)
{
    std::string timeStr = "[" + LLTrans::getString("LTimeMthNum") + "]/["
        +LLTrans::getString("LTimeDay")+"]/["
        +LLTrans::getString("LTimeYear")+"] ["
        +LLTrans::getString("LTimeHour")+"]:["
        +LLTrans::getString("LTimeMin")+"]";

    LLSD substitution;
    substitution["datetime"] = time_stamp;
    LLStringUtil::format(timeStr, substitution);
    return timeStr;
}

void LLNotificationListItem::onClickCloseBtn()
{
    mOnItemClose(this);
}

BOOL LLNotificationListItem::handleMouseUp(S32 x, S32 y, MASK mask)
{
    BOOL res = LLPanel::handleMouseUp(x, y, mask);
    mOnItemClick(this);
    return res;
}

//static
LLNotificationListItem* LLNotificationListItem::create(const Params& p)
{
    if (LLNotificationListItem::getInviteTypes().count(p.notification_name))
    {
        return new LLInviteNotificationListItem(p);
    }
    else if (LLNotificationListItem::getTransactionTypes().count(p.notification_name))
    {
        return new LLTransactionNotificationListItem(p);
    }
    return new LLSystemNotificationListItem(p);
}

//static
std::set<std::string> LLNotificationListItem::getInviteTypes() 
{
    return LLInviteNotificationListItem::getTypes();
}

//static
std::set<std::string> LLNotificationListItem::getTransactionTypes()
{
    return LLTransactionNotificationListItem::getTypes();
}

void LLNotificationListItem::onClickExpandBtn()
{
    setExpanded(TRUE);
}

void LLNotificationListItem::onClickCondenseBtn()
{
    setExpanded(FALSE);
}

void LLNotificationListItem::setExpanded(BOOL value)
{
    mCondensedViewPanel->setVisible(!value);
    mExpandedViewPanel->setVisible(value);
    S32 width = this->getRect().getWidth();
    if (value)
    {
        this->reshape(width, mExpandedHeight, FALSE);
    }
    else
    {
        this->reshape(width, mCondensedHeight, FALSE);
    }
}

std::set<std::string> LLInviteNotificationListItem::getTypes()
{
    std::set<std::string> types;
    types.insert("JoinGroup");
    return types;
}

std::set<std::string> LLTransactionNotificationListItem::getTypes()
{
    std::set<std::string> types;
    types.insert("PaymentReceived");
    types.insert("PaymentSent");
    return types;
}

LLInviteNotificationListItem::LLInviteNotificationListItem(const Params& p)
    : LLNotificationListItem(p),
    mSenderBox(NULL)
{
    buildFromFile("panel_notification_list_item.xml");
}

BOOL LLInviteNotificationListItem::postBuild()
{
    BOOL rv = LLNotificationListItem::postBuild();

    mGroupIcon = getChild<LLGroupIconCtrl>("group_icon");
    mGroupIconExp = getChild<LLGroupIconCtrl>("group_icon_exp");
    mGroupNameBoxExp = getChild<LLTextBox>("group_name_exp");

    mGroupIcon->setValue(mParams.group_id);
    mGroupIconExp->setValue(mParams.group_id);

    mGroupIcon->setVisible(TRUE);
    mGroupIconExp->setVisible(TRUE);

    mGroupId = mParams.group_id;

    mSenderBox = getChild<LLTextBox>("sender_resident");
    mSenderBoxExp = getChild<LLTextBox>("sender_resident_exp");

    setSender(mParams.sender);

    LLSD value(mParams.group_id);
    setGroupId(value);

    return rv;
}

void LLInviteNotificationListItem::changed(LLGroupChange gc)
{
    if (GC_PROPERTIES == gc)
    {
        updateFromCache();
    }
}

bool LLInviteNotificationListItem::updateFromCache()
{
    LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mGroupId);
    if (!group_data) return false;
    setGroupName(group_data->mName);
    return true;
}

void LLInviteNotificationListItem::setGroupId(const LLUUID& value)
{
    LLGroupMgr* gm = LLGroupMgr::getInstance();
    if (mGroupId.notNull())
    {
        gm->removeObserver(this);
    }

    mID = mGroupId;

    // Check if cache already contains image_id for that group
    if (!updateFromCache())
    {
        gm->addObserver(this);
        gm->sendGroupPropertiesRequest(mGroupId);
    }
}

void LLInviteNotificationListItem::setGroupName(std::string name)
{
    if (!name.empty())
    {
        LLStringUtil::format_map_t string_args;
        string_args["[GROUP_NAME]"] = llformat("%s", name.c_str());
        std::string group_box_str = getString("group_name_text", string_args);
        mGroupNameBoxExp->setValue(group_box_str);
        mGroupNameBoxExp->setVisible(TRUE);
    }
    else
    {
        mGroupNameBoxExp->setValue(LLStringUtil::null);
        mGroupNameBoxExp->setVisible(FALSE);
    }
}

void LLInviteNotificationListItem::setSender(std::string sender)
{
    if (!sender.empty())
    {
        LLStringUtil::format_map_t string_args;
        string_args["[SENDER_RESIDENT]"] = llformat("%s", sender.c_str());
        std::string sender_text = getString("sender_resident_text", string_args);
        mSenderBox->setValue(sender_text);
        mSenderBox->setVisible(TRUE);
        mSenderBoxExp->setValue(sender_text);
        mSenderBoxExp->setVisible(TRUE);
    } else {
        mSenderBox->setValue(LLStringUtil::null);
        mSenderBoxExp->setValue(LLStringUtil::null);
        mSenderBox->setVisible(FALSE);
        mSenderBoxExp->setVisible(FALSE);
    }
}

LLTransactionNotificationListItem::LLTransactionNotificationListItem(const Params& p)
    : LLNotificationListItem(p),
    mAvatarIcon(NULL)
{
    buildFromFile("panel_notification_list_item.xml");
}

BOOL LLTransactionNotificationListItem::postBuild()
{
    BOOL rv = LLNotificationListItem::postBuild();
    mAvatarIcon = getChild<LLAvatarIconCtrl>("avatar_icon");
    mAvatarIconExp = getChild<LLAvatarIconCtrl>("avatar_icon_exp");
    if (mParams.notification_name == "PaymentReceived")
    {
        mAvatarIcon->setValue(mParams.paid_from_id);
        mAvatarIconExp->setValue(mParams.paid_from_id);
    }
    else if (mParams.notification_name == "PaymentSent")
    {
        mAvatarIcon->setValue(mParams.paid_to_id);
        mAvatarIconExp->setValue(mParams.paid_to_id);
    }
    mAvatarIcon->setVisible(TRUE);
    mAvatarIconExp->setVisible(TRUE);
    return rv;
}

LLSystemNotificationListItem::LLSystemNotificationListItem(const Params& p)
    : LLNotificationListItem(p),
    mSystemNotificationIcon(NULL)
{
    buildFromFile("panel_notification_list_item.xml");
}

BOOL LLSystemNotificationListItem::postBuild()
{
    BOOL rv = LLNotificationListItem::postBuild();
    mSystemNotificationIcon = getChild<LLIconCtrl>("system_notification_icon");
    mSystemNotificationIconExp = getChild<LLIconCtrl>("system_notification_icon_exp");
    if (mSystemNotificationIcon)
        mSystemNotificationIcon->setVisible(TRUE);
    if (mSystemNotificationIconExp)
        mSystemNotificationIconExp->setVisible(TRUE);
    return rv;
}
