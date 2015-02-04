/** 
 * @file llnotificationlistitem.cpp
 * @brief                                    // TODO
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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
    mTitleBox(NULL),
    mCloseBtn(NULL),
    mSenderBox(NULL)
{
    buildFromFile( "panel_notification_tabbed_item.xml");

    mTitleBox = getChild<LLTextBox>("GroupName_NoticeTitle");
    mTimeBox = getChild<LLTextBox>("Time_Box");
    mCloseBtn = getChild<LLButton>("close_btn");
    mSenderBox = getChild<LLTextBox>("Sender_Resident");

    mTitleBox->setValue(p.title);
    mTimeBox->setValue(buildNotificationDate(p.time_stamp));
    mSenderBox->setVisible(FALSE);

    mCloseBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCloseBtn,this));

    mID = p.notification_id;
    mNotificationName = p.notification_name;
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

//---------------------------------------------------------------------------------
//void LLNotificationTabbedItem::setTitle( std::string title )
//{
//    mTitleBox->setValue(title);
//}

void LLNotificationListItem::onClickCloseBtn()
{
    mOnItemClose(this);
}

BOOL LLNotificationListItem::handleMouseDown(S32 x, S32 y, MASK mask)
{
    BOOL res = LLPanel::handleMouseDown(x, y, mask);
    if(!mCloseBtn->getRect().pointInRect(x, y))
    //if(!mCloseBtn->getRect().pointInRect(x, y))
    //if(!mCloseBtn->getLocalRect().pointInRect(x, y))
        mOnItemClick(this);

    return res;
}

void LLNotificationListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
    //setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemSelected" ));
}

void LLNotificationListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
    //setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemUnselected" ));
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

std::set<std::string> LLSystemNotificationListItem::getTypes()
{
    std::set<std::string> types;
    types.insert("AddPrimitiveFailure");
    types.insert("AddToNavMeshNoCopy");
    types.insert("AssetServerTimeoutObjReturn");
    types.insert("AvatarEjected");
    types.insert("AutoUnmuteByIM");
    types.insert("AutoUnmuteByInventory");
    types.insert("AutoUnmuteByMoney");
    types.insert("BuyInventoryFailedNoMoney");
    types.insert("DeactivatedGesturesTrigger");
    types.insert("DeedFailedNoPermToDeedForGroup");
    types.insert("WhyAreYouTryingToWearShrubbery");
    types.insert("YouDiedAndGotTPHome");
    types.insert("YouFrozeAvatar");

    types.insert("OfferCallingCard");
    //ExpireExplanation
    return types;
}

LLInviteNotificationListItem::LLInviteNotificationListItem(const Params& p)
    : LLNotificationListItem(p)
{
    mGroupIcon = getChild<LLIconCtrl>("group_icon_small");
    mGroupIcon->setValue(p.group_id);
    mGroupID = p.group_id;
    if (!p.sender.empty())
    {
        LLStringUtil::format_map_t string_args;
        string_args["[SENDER_RESIDENT]"] = llformat("%s", p.sender.c_str());
        std::string sender_text = getString("sender_resident_text", string_args);
        mSenderBox->setValue(sender_text);
        mSenderBox->setVisible(TRUE);
    }
}

LLTransactionNotificationListItem::LLTransactionNotificationListItem(const Params& p)
    : LLNotificationListItem(p)
{}

LLSystemNotificationListItem::LLSystemNotificationListItem(const Params& p)
    :LLNotificationListItem(p)
{}

