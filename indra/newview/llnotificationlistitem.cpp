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

#include "llagent.h"
#include "llgroupactions.h"
#include "llinventoryicon.h"
#include "llwindow.h"
#include "v4color.h"
#include "lltrans.h"
#include "lluicolortable.h"
#include "message.h"
#include "llnotificationsutil.h"
#include <boost/regex.hpp>

LLNotificationListItem::LLNotificationListItem(const Params& p) : LLPanel(p),
    mParams(p),
    mTitleBox(NULL),
    mExpandBtn(NULL),
    mCondenseBtn(NULL),
    mCloseBtn(NULL),
    mCondensedViewPanel(NULL),
    mExpandedViewPanel(NULL),
    mCondensedHeight(0),
    mExpandedHeight(0),
    mExpandedHeightResize(0),
    mExpanded(false)
{
    mNotificationName = p.notification_name;
}

BOOL LLNotificationListItem::postBuild()
{
    BOOL rv = LLPanel::postBuild();
    mTitleBox = getChild<LLTextBox>("notification_title");
    mTitleBoxExp = getChild<LLTextBox>("notification_title_exp"); 
    mNoticeTextExp = getChild<LLChatEntry>("notification_text_exp");

    mTimeBox = getChild<LLTextBox>("notification_time");
    mTimeBoxExp = getChild<LLTextBox>("notification_time_exp");
    mExpandBtn = getChild<LLButton>("expand_btn");
    mCondenseBtn = getChild<LLButton>("condense_btn");
    mCloseBtn = getChild<LLButton>("close_btn");
    mCloseBtnExp = getChild<LLButton>("close_expanded_btn");

    mTitleBox->setValue(mParams.title);
    mTitleBoxExp->setValue(mParams.title);
    mNoticeTextExp->setValue(mParams.title);
    mNoticeTextExp->setEnabled(FALSE);
    mNoticeTextExp->setTextExpandedCallback(boost::bind(&LLNotificationListItem::reshapeNotification, this));

    mTitleBox->setContentTrusted(false);
    mTitleBoxExp->setContentTrusted(false);
    mNoticeTextExp->setContentTrusted(false);

    mTimeBox->setValue(buildNotificationDate(mParams.time_stamp));
    mTimeBoxExp->setValue(buildNotificationDate(mParams.time_stamp));

    mExpandBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickExpandBtn,this));
    mCondenseBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCondenseBtn,this));

    //mCloseBtn and mCloseExpandedBtn share the same callback
    mCloseBtn->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCloseBtn,this));
    mCloseBtnExp->setClickedCallback(boost::bind(&LLNotificationListItem::onClickCloseBtn,this));

    mCondensedViewPanel = getChild<LLPanel>("layout_panel_condensed_view");
    mExpandedViewPanel = getChild<LLPanel>("layout_panel_expanded_view");

    std::string expanded_height_str = getString("item_expanded_height");
    std::string condensed_height_str = getString("item_condensed_height");

    mExpandedHeight = (S32)atoi(expanded_height_str.c_str());
    mCondensedHeight = (S32)atoi(condensed_height_str.c_str());
    
    setExpanded(FALSE);

    return rv;
}

LLNotificationListItem::~LLNotificationListItem()
{
}

//static
std::string LLNotificationListItem::buildNotificationDate(const LLDate& time_stamp, ETimeType time_type)
{
    std::string timeStr;
	switch(time_type)
	{
		case Local:
			timeStr = "[" + LLTrans::getString("LTimeMthNum") + "]/["
        		+LLTrans::getString("LTimeDay")+"]/["
				+LLTrans::getString("LTimeYear")+"] ["
				+LLTrans::getString("LTimeHour")+"]:["
				+LLTrans::getString("LTimeMin")+ "]";
			break;
		case UTC:
			timeStr = "[" + LLTrans::getString("UTCTimeMth") + "]/["
		      	+LLTrans::getString("UTCTimeDay")+"]/["
				+LLTrans::getString("UTCTimeYr")+"] ["
				+LLTrans::getString("UTCTimeHr")+"]:["
				+LLTrans::getString("UTCTimeMin")+"] ["
				+LLTrans::getString("UTCTimeTimezone")+"]";
			break;
		case SLT:
		default:
			timeStr = "[" + LLTrans::getString("TimeMonth") + "]/["
			   	+LLTrans::getString("TimeDay")+"]/["
				+LLTrans::getString("TimeYear")+"] ["
				+LLTrans::getString("TimeHour")+"]:["
				+LLTrans::getString("TimeMin")+"] ["
				+LLTrans::getString("TimeTimezone")+"]";
			break;
	}
    LLSD substitution;
    substitution["datetime"] = time_stamp;
    LLStringUtil::format(timeStr, substitution);
    return timeStr;
}

void LLNotificationListItem::onClickCloseBtn()
{
    mOnItemClose(this);
    close();
}

BOOL LLNotificationListItem::handleMouseUp(S32 x, S32 y, MASK mask)
{
    BOOL res = LLPanel::handleMouseUp(x, y, mask);
    mOnItemClick(this);
    return res;
}

void LLNotificationListItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mCondensedViewPanel->setTransparentColor(LLUIColorTable::instance().getColor( "ScrollHoveredColor" ));
	mExpandedViewPanel->setTransparentColor(LLUIColorTable::instance().getColor( "ScrollHoveredColor" ));
}

void LLNotificationListItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mCondensedViewPanel->setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemUnselected" ));
	mExpandedViewPanel->setTransparentColor(LLUIColorTable::instance().getColor( "SysWellItemUnselected" ));
}

//static
LLNotificationListItem* LLNotificationListItem::create(const Params& p)
{
    if (LLNotificationListItem::getGroupInviteTypes().count(p.notification_name))
    {
        return new LLGroupInviteNotificationListItem(p);
    }
    else if (LLNotificationListItem::getGroupNoticeTypes().count(p.notification_name))
    {
        return new LLGroupNoticeNotificationListItem(p);
    }
    else if (LLNotificationListItem::getTransactionTypes().count(p.notification_name))
    {
        return new LLTransactionNotificationListItem(p);
    }
    return new LLSystemNotificationListItem(p);
}

//static
std::set<std::string> LLNotificationListItem::getGroupInviteTypes() 
{
    return LLGroupInviteNotificationListItem::getTypes();
}


std::set<std::string> LLNotificationListItem::getGroupNoticeTypes()
{
    return LLGroupNoticeNotificationListItem::getTypes();
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

void LLNotificationListItem::reshapeNotification()
{
    if(mExpanded)
    {
        S32 width = this->getRect().getWidth();
        this->reshape(width, mNoticeTextExp->getRect().getHeight() + mExpandedHeight, FALSE);
    }
}

void LLNotificationListItem::setExpanded(BOOL value)
{
    mCondensedViewPanel->setVisible(!value);
    mExpandedViewPanel->setVisible(value);
    S32 width = this->getRect().getWidth();

    if (value)
    {
       this->reshape(width, mNoticeTextExp->getRect().getHeight() + mExpandedHeight, FALSE);
    }
    else
    {
        this->reshape(width, mCondensedHeight, FALSE);
    }
    mExpanded = value;

}

std::set<std::string> LLGroupInviteNotificationListItem::getTypes()
{
    std::set<std::string> types;
    types.insert("JoinGroup");
    return types;
}

std::set<std::string> LLGroupNoticeNotificationListItem::getTypes()
{
    std::set<std::string> types;
    types.insert("GroupNotice");
    return types;
}

std::set<std::string> LLTransactionNotificationListItem::getTypes()
{
    std::set<std::string> types;
    types.insert("PaymentReceived");
    types.insert("PaymentSent");
    types.insert("UploadPayment");
    return types;
}

LLGroupNotificationListItem::LLGroupNotificationListItem(const Params& p)
    : LLNotificationListItem(p),
    mSenderOrFeeBox(NULL)
{
}

LLGroupInviteNotificationListItem::LLGroupInviteNotificationListItem(const Params& p)
    : LLGroupNotificationListItem(p)
{
    buildFromFile("panel_notification_list_item.xml");
}

BOOL LLGroupInviteNotificationListItem::postBuild()
{
    BOOL rv = LLGroupNotificationListItem::postBuild();
    setFee(mParams.fee);
    mInviteButtonPanel = getChild<LLPanel>("button_panel");
    mInviteButtonPanel->setVisible(TRUE);
    mJoinBtn = getChild<LLButton>("join_btn");
    mDeclineBtn = getChild<LLButton>("decline_btn");
    mInfoBtn = getChild<LLButton>("info_btn");

    //invitation with any non-default group role, doesn't have newline characters at the end unlike simple invitations
    std::string invitation_desc = mNoticeTextExp->getValue().asString();
    if (invitation_desc.substr(invitation_desc.size() - 2) != "\n\n")
    {
        invitation_desc += "\n\n";
        mNoticeTextExp->setValue(invitation_desc);
    }

    mJoinBtn->setClickedCallback(boost::bind(&LLGroupInviteNotificationListItem::onClickJoinBtn,this));
    mDeclineBtn->setClickedCallback(boost::bind(&LLGroupInviteNotificationListItem::onClickDeclineBtn,this));
    mInfoBtn->setClickedCallback(boost::bind(&LLGroupInviteNotificationListItem::onClickInfoBtn,this));

    std::string expanded_height_resize_str = getString("expanded_height_resize_for_attachment");
    mExpandedHeightResize = (S32)atoi(expanded_height_resize_str.c_str());

    return rv;
}

void LLGroupInviteNotificationListItem::onClickJoinBtn()
{
	if (!gAgent.canJoinGroups())
	{
		LLNotificationsUtil::add("JoinedTooManyGroups");
		return;
	}

	send_join_group_response(mParams.group_id, mParams.transaction_id, true, mParams.fee, mParams.use_offline_cap);

	LLNotificationListItem::onClickCloseBtn();
}

void LLGroupInviteNotificationListItem::onClickDeclineBtn()
{
	send_join_group_response(mParams.group_id, mParams.transaction_id, false, mParams.fee, mParams.use_offline_cap);

	LLNotificationListItem::onClickCloseBtn();
}

void LLGroupInviteNotificationListItem::onClickInfoBtn()
{
	LLGroupActions::show(mParams.group_id);
}

void LLGroupInviteNotificationListItem::setFee(S32 fee)
{
    LLStringUtil::format_map_t string_args;
    string_args["[GROUP_FEE]"] = llformat("%d", fee);
    std::string fee_text = getString("group_fee_text", string_args);
    mSenderOrFeeBox->setValue(fee_text);
    mSenderOrFeeBoxExp->setValue(fee_text);
    mSenderOrFeeBox->setVisible(TRUE);
    mSenderOrFeeBoxExp->setVisible(TRUE);
}

LLGroupNoticeNotificationListItem::LLGroupNoticeNotificationListItem(const Params& p)
    : LLGroupNotificationListItem(p),
    mAttachmentPanel(NULL),
    mAttachmentTextBox(NULL),
    mAttachmentIcon(NULL),
    mAttachmentIconExp(NULL),
    mInventoryOffer(NULL)
{
    if (mParams.inventory_offer.isDefined())
    {
        mInventoryOffer = new LLOfferInfo(mParams.inventory_offer);
    }

    buildFromFile("panel_notification_list_item.xml");
}

LLGroupNotificationListItem::~LLGroupNotificationListItem()
{
	LLGroupMgr::getInstance()->removeObserver(this);
}

BOOL LLGroupNoticeNotificationListItem::postBuild()
{
    BOOL rv = LLGroupNotificationListItem::postBuild();

    mAttachmentTextBox = getChild<LLTextBox>("attachment_text");
    mAttachmentIcon = getChild<LLIconCtrl>("attachment_icon");
    mAttachmentIconExp = getChild<LLIconCtrl>("attachment_icon_exp");
    mAttachmentPanel = getChild<LLPanel>("attachment_panel");
    mAttachmentPanel->setVisible(FALSE);


    mTitleBox->setValue(mParams.subject);
    mTitleBoxExp->setValue(mParams.subject);
    mNoticeTextExp->setValue(mParams.message);

    mTimeBox->setValue(buildNotificationDate(mParams.time_stamp));
    mTimeBoxExp->setValue(buildNotificationDate(mParams.time_stamp));
    //Workaround: in case server timestamp is 0 - we use the time when notification was actually received
    if (mParams.time_stamp.isNull())
    {
        mTimeBox->setValue(buildNotificationDate(mParams.received_time));
        mTimeBoxExp->setValue(buildNotificationDate(mParams.received_time));
    }
    setSender(mParams.sender);

    if (mInventoryOffer != NULL)
    {
        mAttachmentTextBox->setValue(mInventoryOffer->mDesc);
        mAttachmentTextBox->setVisible(TRUE);
        mAttachmentIcon->setVisible(TRUE);

        std::string icon_name = LLInventoryIcon::getIconName(mInventoryOffer->mType,
          LLInventoryType::IT_TEXTURE);
        mAttachmentIconExp->setValue(icon_name);
        mAttachmentIconExp->setVisible(TRUE);

        mAttachmentTextBox->setClickedCallback(boost::bind(
            &LLGroupNoticeNotificationListItem::onClickAttachment, this));

        std::string expanded_height_resize_str = getString("expanded_height_resize_for_attachment");
        mExpandedHeightResize = (S32)atoi(expanded_height_resize_str.c_str());

        mAttachmentPanel->setVisible(TRUE);
    }
    return rv;
}

BOOL LLGroupNotificationListItem::postBuild()
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

    mSenderOrFeeBox = getChild<LLTextBox>("sender_or_fee_box");
    mSenderOrFeeBoxExp = getChild<LLTextBox>("sender_or_fee_box_exp");

    LLSD value(mParams.group_id);
    setGroupId(value);

    return rv;
}

void LLGroupNotificationListItem::changed(LLGroupChange gc)
{
    if (GC_PROPERTIES == gc)
    {
    	updateFromCache();
        LLGroupMgr::getInstance()->removeObserver(this);
    }
}

bool LLGroupNotificationListItem::updateFromCache()
{
    LLGroupMgrGroupData* group_data = LLGroupMgr::getInstance()->getGroupData(mGroupId);
    if (!group_data) return false;
    setGroupName(group_data->mName);
    return true;
}

void LLGroupNotificationListItem::setGroupId(const LLUUID& value)
{
	LLGroupMgr* gm = LLGroupMgr::getInstance();
	if (mGroupId.notNull())
    {
        gm->removeObserver(this);


        mID = mGroupId;

        // Check if cache already contains image_id for that group
        if (!updateFromCache())
        {
        	gm->addObserver(this);
        	gm->sendGroupPropertiesRequest(mGroupId);
        }
    }
}

void LLGroupNotificationListItem::setGroupName(std::string name)
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

void LLGroupNoticeNotificationListItem::setSender(std::string sender)
{
    if (!sender.empty())
    {
        LLStringUtil::format_map_t string_args;
        string_args["[SENDER_RESIDENT]"] = llformat("%s", sender.c_str());
        std::string sender_text = getString("sender_resident_text", string_args);
        mSenderOrFeeBox->setValue(sender_text);
        mSenderOrFeeBoxExp->setValue(sender_text);
        mSenderOrFeeBox->setVisible(TRUE);
        mSenderOrFeeBoxExp->setVisible(TRUE);
    } else {
        mSenderOrFeeBox->setValue(LLStringUtil::null);
        mSenderOrFeeBoxExp->setValue(LLStringUtil::null);
        mSenderOrFeeBox->setVisible(FALSE);
        mSenderOrFeeBoxExp->setVisible(FALSE);
    }
}
void LLGroupNoticeNotificationListItem::close()
{
    // The group notice dialog may be an inventory offer.
    // If it has an inventory save button and that button is still enabled
    // Then we need to send the inventory declined message
    if (mInventoryOffer != NULL)
    {
        mInventoryOffer->forceResponse(IOR_DECLINE);
        mInventoryOffer = NULL;
    }
}

void LLGroupNoticeNotificationListItem::onClickAttachment()
{
    if (mInventoryOffer != NULL) {
        mInventoryOffer->forceResponse(IOR_ACCEPT);

        static const LLUIColor textColor = LLUIColorTable::instance().getColor(
            "GroupNotifyDimmedTextColor");
        mAttachmentTextBox->setColor(textColor);
        mAttachmentIconExp->setEnabled(FALSE);

        //if attachment isn't openable - notify about saving
        if (!isAttachmentOpenable(mInventoryOffer->mType)) {
            LLNotifications::instance().add("AttachmentSaved", LLSD(), LLSD());
        }

        mInventoryOffer = NULL;
    }
}

//static
bool LLGroupNoticeNotificationListItem::isAttachmentOpenable(LLAssetType::EType type)
{
    switch (type)
    {
    case LLAssetType::AT_LANDMARK:
    case LLAssetType::AT_NOTECARD:
    case LLAssetType::AT_IMAGE_JPEG:
    case LLAssetType::AT_IMAGE_TGA:
    case LLAssetType::AT_TEXTURE:
    case LLAssetType::AT_TEXTURE_TGA:
        return true;
    default:
        return false;
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
    mAvatarIcon->setValue("System_Notification");
    mAvatarIconExp->setValue("System_Notification");

    mAvatarIcon->setVisible(TRUE);
    mAvatarIconExp->setVisible(TRUE);
    if((GOVERNOR_LINDEN_ID == mParams.paid_to_id) ||
       (GOVERNOR_LINDEN_ID == mParams.paid_from_id))
    {
        return rv;
    }

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

    return rv;
}

LLSystemNotificationListItem::LLSystemNotificationListItem(const Params& p)
    : LLNotificationListItem(p),
    mSystemNotificationIcon(NULL),
    mIsCaution(false)
{
    buildFromFile("panel_notification_list_item.xml");
    mIsCaution = p.notification_priority >= NOTIFICATION_PRIORITY_HIGH;
    if (mIsCaution)
    {
        mTitleBox->setColor(LLUIColorTable::instance().getColor("NotifyCautionBoxColor"));
        mTitleBoxExp->setColor(LLUIColorTable::instance().getColor("NotifyCautionBoxColor"));
        mNoticeTextExp->setReadOnlyColor(LLUIColorTable::instance().getColor("NotifyCautionBoxColor"));
        mTimeBox->setColor(LLUIColorTable::instance().getColor("NotifyCautionBoxColor"));
        mTimeBoxExp->setColor(LLUIColorTable::instance().getColor("NotifyCautionBoxColor"));
    }
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
