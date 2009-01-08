/** 
 * @file llpanelgroupgeneral.cpp
 * @brief General information about a group.
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llpanelgroupgeneral.h"

#include "lluictrlfactory.h"
#include "llagent.h"
#include "roles_constants.h"
#include "llfloateravatarinfo.h"
#include "llfloatergroupinfo.h"

// UI elements
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldbstrings.h"
#include "lllineeditor.h"
#include "llnamebox.h"
#include "llnamelistctrl.h"
#include "llspinctrl.h"
#include "llstatusbar.h"	// can_afford_transaction()
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "llviewerwindow.h"

// consts
const S32 MATURE_CONTENT = 1;
const S32 NON_MATURE_CONTENT = 2;
const S32 DECLINE_TO_STATE = 0;

// static
void* LLPanelGroupGeneral::createTab(void* data)
{
	LLUUID* group_id = static_cast<LLUUID*>(data);
	return new LLPanelGroupGeneral("panel group general", *group_id);
}


LLPanelGroupGeneral::LLPanelGroupGeneral(const std::string& name, 
										 const LLUUID& group_id)
:	LLPanelGroupTab(name, group_id),
	mPendingMemberUpdate(FALSE),
	mChanged(FALSE),
	mFirstUse(TRUE),
	mGroupNameEditor(NULL),
	mGroupName(NULL),
	mFounderName(NULL),
	mInsignia(NULL),
	mEditCharter(NULL),
	mBtnJoinGroup(NULL),
	mListVisibleMembers(NULL),
	mCtrlShowInGroupList(NULL),
	mComboMature(NULL),
	mCtrlOpenEnrollment(NULL),
	mCtrlEnrollmentFee(NULL),
	mSpinEnrollmentFee(NULL),
	mCtrlReceiveNotices(NULL),
	mCtrlListGroup(NULL),
	mActiveTitleLabel(NULL),
	mComboActiveTitle(NULL)
{

}

LLPanelGroupGeneral::~LLPanelGroupGeneral()
{
}

BOOL LLPanelGroupGeneral::postBuild()
{
	llinfos << "LLPanelGroupGeneral::postBuild()" << llendl;

	bool recurse = true;

	// General info
	mGroupNameEditor = getChild<LLLineEditor>("group_name_editor", recurse);
	mGroupName = getChild<LLTextBox>("group_name", recurse);
	
	mInsignia = getChild<LLTextureCtrl>("insignia", recurse);
	if (mInsignia)
	{
		mInsignia->setCommitCallback(onCommitAny);
		mInsignia->setCallbackUserData(this);
		mDefaultIconID = mInsignia->getImageAssetID();
	}
	
	mEditCharter = getChild<LLTextEditor>("charter", recurse);
	if(mEditCharter)
	{
		mEditCharter->setCommitCallback(onCommitAny);
		mEditCharter->setFocusReceivedCallback(onFocusEdit, this);
		mEditCharter->setFocusChangedCallback(onFocusEdit, this);
		mEditCharter->setCallbackUserData(this);
	}

	mBtnJoinGroup = getChild<LLButton>("join_button", recurse);
	if ( mBtnJoinGroup )
	{
		mBtnJoinGroup->setClickedCallback(onClickJoin);
		mBtnJoinGroup->setCallbackUserData(this);
	}

	mBtnInfo = getChild<LLButton>("info_button", recurse);
	if ( mBtnInfo )
	{
		mBtnInfo->setClickedCallback(onClickInfo);
		mBtnInfo->setCallbackUserData(this);
	}

	LLTextBox* founder = getChild<LLTextBox>("founder_name");
	if (founder)
	{
		mFounderName = new LLNameBox(founder->getName(),founder->getRect(),LLUUID::null,FALSE,founder->getFont(),founder->getMouseOpaque());
		removeChild(founder, TRUE);
		addChild(mFounderName);
	}

	mListVisibleMembers = getChild<LLNameListCtrl>("visible_members", recurse);
	if (mListVisibleMembers)
	{
		mListVisibleMembers->setDoubleClickCallback(openProfile);
		mListVisibleMembers->setCallbackUserData(this);
	}

	// Options
	mCtrlShowInGroupList = getChild<LLCheckBoxCtrl>("show_in_group_list", recurse);
	if (mCtrlShowInGroupList)
	{
		mCtrlShowInGroupList->setCommitCallback(onCommitAny);
		mCtrlShowInGroupList->setCallbackUserData(this);
	}

	mComboMature = getChild<LLComboBox>("group_mature_check", recurse);	
	if(mComboMature)
	{
		mComboMature->setCurrentByIndex(0);
		mComboMature->setCommitCallback(onCommitAny);
		mComboMature->setCallbackUserData(this);
		if (gAgent.isTeen())
		{
			// Teens don't get to set mature flag. JC
			mComboMature->setVisible(FALSE);
			mComboMature->setCurrentByIndex(NON_MATURE_CONTENT);
		}
	}
	mCtrlOpenEnrollment = getChild<LLCheckBoxCtrl>("open_enrollement", recurse);
	if (mCtrlOpenEnrollment)
	{
		mCtrlOpenEnrollment->setCommitCallback(onCommitAny);
		mCtrlOpenEnrollment->setCallbackUserData(this);
	}

	mCtrlEnrollmentFee = getChild<LLCheckBoxCtrl>("check_enrollment_fee", recurse);
	if (mCtrlEnrollmentFee)
	{
		mCtrlEnrollmentFee->setCommitCallback(onCommitEnrollment);
		mCtrlEnrollmentFee->setCallbackUserData(this);
	}

	mSpinEnrollmentFee = getChild<LLSpinCtrl>("spin_enrollment_fee", recurse);
	if (mSpinEnrollmentFee)
	{
		mSpinEnrollmentFee->setCommitCallback(onCommitAny);
		mSpinEnrollmentFee->setCallbackUserData(this);
		mSpinEnrollmentFee->setPrecision(0);
	}

	BOOL accept_notices = FALSE;
	BOOL list_in_profile = FALSE;
	LLGroupData data;
	if(gAgent.getGroupData(mGroupID,data))
	{
		accept_notices = data.mAcceptNotices;
		list_in_profile = data.mListInProfile;
	}
	mCtrlReceiveNotices = getChild<LLCheckBoxCtrl>("receive_notices", recurse);
	if (mCtrlReceiveNotices)
	{
		mCtrlReceiveNotices->setCommitCallback(onCommitUserOnly);
		mCtrlReceiveNotices->setCallbackUserData(this);
		mCtrlReceiveNotices->set(accept_notices);
		mCtrlReceiveNotices->setEnabled(data.mID.notNull());
	}
	
	mCtrlListGroup = getChild<LLCheckBoxCtrl>("list_groups_in_profile", recurse);
	if (mCtrlListGroup)
	{
		mCtrlListGroup->setCommitCallback(onCommitUserOnly);
		mCtrlListGroup->setCallbackUserData(this);
		mCtrlListGroup->set(list_in_profile);
		mCtrlListGroup->setEnabled(data.mID.notNull());
	}

	mActiveTitleLabel = getChild<LLTextBox>("active_title_label", recurse);
	
	mComboActiveTitle = getChild<LLComboBox>("active_title", recurse);
	if (mComboActiveTitle)
	{
		mComboActiveTitle->setCommitCallback(onCommitTitle);
		mComboActiveTitle->setCallbackUserData(this);
	}

	mIncompleteMemberDataStr = getString("incomplete_member_data_str");
	mConfirmGroupCreateStr = getString("confirm_group_create_str");

	// If the group_id is null, then we are creating a new group
	if (mGroupID.isNull())
	{
		mGroupNameEditor->setEnabled(TRUE);
		mEditCharter->setEnabled(TRUE);

		mCtrlShowInGroupList->setEnabled(TRUE);
		mComboMature->setEnabled(TRUE);
		mCtrlOpenEnrollment->setEnabled(TRUE);
		mCtrlEnrollmentFee->setEnabled(TRUE);
		mSpinEnrollmentFee->setEnabled(TRUE);

		mBtnJoinGroup->setVisible(FALSE);
		mBtnInfo->setVisible(FALSE);
		mGroupName->setVisible(FALSE);
	}

	return LLPanelGroupTab::postBuild();
}

// static
void LLPanelGroupGeneral::onFocusEdit(LLFocusableElement* ctrl, void* data)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;
	self->mChanged = TRUE;
	self->notifyObservers();
}

// static
void LLPanelGroupGeneral::onCommitAny(LLUICtrl* ctrl, void* data)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;
	self->updateChanged();
	self->notifyObservers();
}

// static
void LLPanelGroupGeneral::onCommitUserOnly(LLUICtrl* ctrl, void* data)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;
	self->mChanged = TRUE;
	self->notifyObservers();
}


// static
void LLPanelGroupGeneral::onCommitEnrollment(LLUICtrl* ctrl, void* data)
{
	onCommitAny(ctrl, data);

	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;
	// Make sure both enrollment related widgets are there.
	if (!self->mCtrlEnrollmentFee || !self->mSpinEnrollmentFee)
	{
		return;
	}

	// Make sure the agent can change enrollment info.
	if (!gAgent.hasPowerInGroup(self->mGroupID,GP_MEMBER_OPTIONS)
		|| !self->mAllowEdit)
	{
		return;
	}

	if (self->mCtrlEnrollmentFee->get())
	{
		self->mSpinEnrollmentFee->setEnabled(TRUE);
	}
	else
	{
		self->mSpinEnrollmentFee->setEnabled(FALSE);
		self->mSpinEnrollmentFee->set(0);
	}
}

// static
void LLPanelGroupGeneral::onCommitTitle(LLUICtrl* ctrl, void* data)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;
	if (self->mGroupID.isNull() || !self->mAllowEdit) return;
	LLGroupMgr::getInstance()->sendGroupTitleUpdate(self->mGroupID,self->mComboActiveTitle->getCurrentID());
	self->update(GC_TITLES);
}

// static
void LLPanelGroupGeneral::onClickInfo(void *userdata)
{
	LLPanelGroupGeneral *self = (LLPanelGroupGeneral *)userdata;

	if ( !self ) return;

	lldebugs << "open group info: " << self->mGroupID << llendl;

	LLFloaterGroupInfo::showFromUUID(self->mGroupID);
}

// static
void LLPanelGroupGeneral::onClickJoin(void *userdata)
{
	LLPanelGroupGeneral *self = (LLPanelGroupGeneral *)userdata;

	if ( !self ) return;

	lldebugs << "joining group: " << self->mGroupID << llendl;

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(self->mGroupID);

	if (gdatap)
	{
		S32 cost = gdatap->mMembershipFee;
		LLSD args;
		args["COST"] = llformat("%d", cost);
		LLSD payload;
		payload["group_id"] = self->mGroupID;

		if (can_afford_transaction(cost))
		{
			LLNotifications::instance().add("JoinGroupCanAfford", args, payload, LLPanelGroupGeneral::joinDlgCB);
		}
		else
		{
			LLNotifications::instance().add("JoinGroupCannotAfford", args, payload);
		}
	}
	else
	{
		llwarns << "LLGroupMgr::getInstance()->getGroupData(" << self->mGroupID
			<< ") was NULL" << llendl;
	}
}

// static
bool LLPanelGroupGeneral::joinDlgCB(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if (option == 1)
	{
		// user clicked cancel
		return false;
	}

	LLGroupMgr::getInstance()->sendGroupMemberJoin(notification["payload"]["group_id"].asUUID());
	return false;
}

// static
void LLPanelGroupGeneral::openProfile(void* data)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;

	if (self && self->mListVisibleMembers)
	{
		LLScrollListItem* selected = self->mListVisibleMembers->getFirstSelected();
		if (selected)
		{
			LLFloaterAvatarInfo::showFromDirectory( selected->getUUID() );
		}
	}
}

bool LLPanelGroupGeneral::needsApply(std::string& mesg)
{ 
	mesg = getString("group_info_unchanged");
	return mChanged || mGroupID.isNull();
}

void LLPanelGroupGeneral::activate()
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mGroupID);
	if (mGroupID.notNull()
		&& (!gdatap || mFirstUse))
	{
		LLGroupMgr::getInstance()->sendGroupTitlesRequest(mGroupID);
		LLGroupMgr::getInstance()->sendGroupPropertiesRequest(mGroupID);

		
		if (!gdatap || !gdatap->isMemberDataComplete() )
		{
			LLGroupMgr::getInstance()->sendGroupMembersRequest(mGroupID);
		}

		mFirstUse = FALSE;
	}
	mChanged = FALSE;
	
	update(GC_ALL);
}

void LLPanelGroupGeneral::draw()
{
	LLPanelGroupTab::draw();

	if (mPendingMemberUpdate)
	{
		updateMembers();
	}
}

bool LLPanelGroupGeneral::apply(std::string& mesg)
{
	BOOL has_power_in_group = gAgent.hasPowerInGroup(mGroupID,GP_GROUP_CHANGE_IDENTITY);

	if (has_power_in_group || mGroupID.isNull())
	{
		llinfos << "LLPanelGroupGeneral::apply" << llendl;

		// Check to make sure mature has been set
		if(mComboMature &&
		   mComboMature->getCurrentIndex() == DECLINE_TO_STATE)
		{
			LLNotifications::instance().add("SetGroupMature", LLSD(), LLSD(), 
											boost::bind(&LLPanelGroupGeneral::confirmMatureApply, this, _1, _2));
			return false;
		}

		if (mGroupID.isNull())
		{
			// Validate the group name length.
			S32 group_name_len = mGroupNameEditor->getText().size();
			if ( group_name_len < DB_GROUP_NAME_MIN_LEN 
				|| group_name_len > DB_GROUP_NAME_STR_LEN)
			{
				std::ostringstream temp_error;
				temp_error << "A group name must be between " << DB_GROUP_NAME_MIN_LEN
					<< " and " << DB_GROUP_NAME_STR_LEN << " characters.";
				mesg = temp_error.str();
				return false;
			}

			LLSD args;
			args["MESSAGE"] = mConfirmGroupCreateStr;
			LLNotifications::instance().add("GenericAlertYesCancel", args, LLSD(), boost::bind(&LLPanelGroupGeneral::createGroupCallback, this, _1, _2));

			return false;
		}

		LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mGroupID);
		if (!gdatap)
		{
			// *TODO: Translate
			mesg = std::string("No group data found for group ");
			mesg.append(mGroupID.asString());
			return false;
		}
		bool can_change_ident = false;
		bool can_change_member_opts = false;
		can_change_ident = gAgent.hasPowerInGroup(mGroupID,GP_GROUP_CHANGE_IDENTITY);
		can_change_member_opts = gAgent.hasPowerInGroup(mGroupID,GP_MEMBER_OPTIONS);

		if (can_change_ident)
		{
			if (mEditCharter) gdatap->mCharter = mEditCharter->getText();
			if (mInsignia) gdatap->mInsigniaID = mInsignia->getImageAssetID();
			if (mComboMature)
			{
				if (!gAgent.isTeen())
				{
					gdatap->mMaturePublish = 
						mComboMature->getCurrentIndex() == MATURE_CONTENT;
				}
				else
				{
					gdatap->mMaturePublish = FALSE;
				}
			}
			if (mCtrlShowInGroupList) gdatap->mShowInList = mCtrlShowInGroupList->get();
		}

		if (can_change_member_opts)
		{
			if (mCtrlOpenEnrollment) gdatap->mOpenEnrollment = mCtrlOpenEnrollment->get();
			if (mCtrlEnrollmentFee && mSpinEnrollmentFee)
			{
				gdatap->mMembershipFee = (mCtrlEnrollmentFee->get()) ? 
					(S32) mSpinEnrollmentFee->get() : 0;
				// Set to the used value, and reset initial value used for isdirty check
				mSpinEnrollmentFee->set( (F32)gdatap->mMembershipFee );
			}
		}

		if (can_change_ident || can_change_member_opts)
		{
			LLGroupMgr::getInstance()->sendUpdateGroupInfo(mGroupID);
		}
	}

	BOOL receive_notices = false;
	BOOL list_in_profile = false;
	if (mCtrlReceiveNotices)
		receive_notices = mCtrlReceiveNotices->get();
	if (mCtrlListGroup) 
		list_in_profile = mCtrlListGroup->get();

	gAgent.setUserGroupFlags(mGroupID, receive_notices, list_in_profile);

	mChanged = FALSE;

	return true;
}

void LLPanelGroupGeneral::cancel()
{
	mChanged = FALSE;

	//cancel out all of the click changes to, although since we are
	//shifting tabs or closing the floater, this need not be done...yet
	notifyObservers();
}

// invoked from callbackConfirmMature
bool LLPanelGroupGeneral::confirmMatureApply(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	// 0 == Yes
	// 1 == No
	// 2 == Cancel
	switch(option)
	{
	case 0:
		mComboMature->setCurrentByIndex(MATURE_CONTENT);
		break;
	case 1:
		mComboMature->setCurrentByIndex(NON_MATURE_CONTENT);
		break;
	default:
		return false;
	}

	// If we got here it means they set a valid value
	std::string mesg = "";
	apply(mesg);
	return false;
}

// static
bool LLPanelGroupGeneral::createGroupCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	switch(option)
	{
	case 0:
		{
			// Yay!  We are making a new group!
			U32 enrollment_fee = (mCtrlEnrollmentFee->get() ? 
									(U32) mSpinEnrollmentFee->get() : 0);
		
			LLGroupMgr::getInstance()->sendCreateGroupRequest(mGroupNameEditor->getText(),
												mEditCharter->getText(),
												mCtrlShowInGroupList->get(),
												mInsignia->getImageAssetID(),
												enrollment_fee,
												mCtrlOpenEnrollment->get(),
												false,
												mComboMature->getCurrentIndex() == MATURE_CONTENT);

		}
		break;
	case 1:
	default:
		break;
	}
	return false;
}

static F32 sSDTime = 0.0f;
static F32 sElementTime = 0.0f;
static F32 sAllTime = 0.0f;

// virtual
void LLPanelGroupGeneral::update(LLGroupChange gc)
{
	if (mGroupID.isNull()) return;

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mGroupID);

	if (!gdatap) return;

	LLGroupData agent_gdatap;
	bool is_member = false;
	if (gAgent.getGroupData(mGroupID,agent_gdatap)) is_member = true;

	if (mComboActiveTitle)
	{
		mComboActiveTitle->setVisible(is_member);
		mComboActiveTitle->setEnabled(mAllowEdit);
		
		if ( mActiveTitleLabel) mActiveTitleLabel->setVisible(is_member);

		if (is_member)
		{
			LLUUID current_title_role;

			mComboActiveTitle->clear();
			mComboActiveTitle->removeall();
			bool has_selected_title = false;

			if (1 == gdatap->mTitles.size())
			{
				// Only the everyone title.  Don't bother letting them try changing this.
				mComboActiveTitle->setEnabled(FALSE);
			}
			else
			{
				mComboActiveTitle->setEnabled(TRUE);
			}

			std::vector<LLGroupTitle>::const_iterator citer = gdatap->mTitles.begin();
			std::vector<LLGroupTitle>::const_iterator end = gdatap->mTitles.end();
			
			for ( ; citer != end; ++citer)
			{
				mComboActiveTitle->add(citer->mTitle,citer->mRoleID, (citer->mSelected ? ADD_TOP : ADD_BOTTOM));
				if (citer->mSelected)
				{
					mComboActiveTitle->setCurrentByID(citer->mRoleID);
					has_selected_title = true;
				}
			}
			
			if (!has_selected_title)
			{
				mComboActiveTitle->setCurrentByID(LLUUID::null);
			}
		}
	}

	// If this was just a titles update, we are done.
	if (gc == GC_TITLES) return;

	bool can_change_ident = false;
	bool can_change_member_opts = false;
	can_change_ident = gAgent.hasPowerInGroup(mGroupID,GP_GROUP_CHANGE_IDENTITY);
	can_change_member_opts = gAgent.hasPowerInGroup(mGroupID,GP_MEMBER_OPTIONS);

	if (mCtrlShowInGroupList) 
	{
		mCtrlShowInGroupList->set(gdatap->mShowInList);
		mCtrlShowInGroupList->setEnabled(mAllowEdit && can_change_ident);
	}
	if (mComboMature)
	{
		if(gdatap->mMaturePublish)
		{
			mComboMature->setCurrentByIndex(MATURE_CONTENT);
		}
		else
		{
			mComboMature->setCurrentByIndex(NON_MATURE_CONTENT);
		}
		mComboMature->setEnabled(mAllowEdit && can_change_ident);
		mComboMature->setVisible( !gAgent.isTeen() );
	}
	if (mCtrlOpenEnrollment) 
	{
		mCtrlOpenEnrollment->set(gdatap->mOpenEnrollment);
		mCtrlOpenEnrollment->setEnabled(mAllowEdit && can_change_member_opts);
	}
	if (mCtrlEnrollmentFee) 
	{	
		mCtrlEnrollmentFee->set(gdatap->mMembershipFee > 0);
		mCtrlEnrollmentFee->setEnabled(mAllowEdit && can_change_member_opts);
	}
	
	if (mSpinEnrollmentFee)
	{
		S32 fee = gdatap->mMembershipFee;
		mSpinEnrollmentFee->set((F32)fee);
		mSpinEnrollmentFee->setEnabled( mAllowEdit 
								&& (fee > 0) && can_change_member_opts);
	}
	if ( mBtnJoinGroup )
	{
		std::string fee_buff;
		bool visible;

		visible = !is_member && gdatap->mOpenEnrollment;
		mBtnJoinGroup->setVisible(visible);

		if ( visible )
		{
			fee_buff = llformat( "Join (L$%d)", gdatap->mMembershipFee);
			mBtnJoinGroup->setLabelSelected(fee_buff);
			mBtnJoinGroup->setLabelUnselected(fee_buff);
		}
	}
	if ( mBtnInfo )
	{
		mBtnInfo->setVisible(is_member && !mAllowEdit);
	}

	if (mCtrlReceiveNotices)
	{
		mCtrlReceiveNotices->setVisible(is_member);
		if (is_member)
		{
			mCtrlReceiveNotices->setEnabled(mAllowEdit);
		}
	}
	

	if (mInsignia) mInsignia->setEnabled(mAllowEdit && can_change_ident);
	if (mEditCharter) mEditCharter->setEnabled(mAllowEdit && can_change_ident);
	
	if (mGroupName) mGroupName->setText(gdatap->mName);
	if (mGroupNameEditor) mGroupNameEditor->setVisible(FALSE);
	if (mFounderName) mFounderName->setNameID(gdatap->mFounderID,FALSE);
	if (mInsignia)
	{
		if (gdatap->mInsigniaID.notNull())
		{
			mInsignia->setImageAssetID(gdatap->mInsigniaID);
		}
		else
		{
			
			mInsignia->setImageAssetID(mDefaultIconID);
		}
	}
	if (mEditCharter) mEditCharter->setText(gdatap->mCharter);
	
	if (mListVisibleMembers)
	{
		mListVisibleMembers->deleteAllItems();

		if (gdatap->isMemberDataComplete())
		{
			mMemberProgress = gdatap->mMembers.begin();
			mPendingMemberUpdate = TRUE;

			sSDTime = 0.0f;
			sElementTime = 0.0f;
			sAllTime = 0.0f;
		}
		else
		{
			std::stringstream pending;
			pending << "Retrieving member list (" << gdatap->mMembers.size() << "\\" << gdatap->mMemberCount  << ")";

			LLSD row;
			row["columns"][0]["value"] = pending.str();

			mListVisibleMembers->setEnabled(FALSE);
			mListVisibleMembers->addElement(row);
		}
	}
}

void LLPanelGroupGeneral::updateMembers()
{
	mPendingMemberUpdate = FALSE;

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mGroupID);

	if (!mListVisibleMembers || !gdatap 
		|| !gdatap->isMemberDataComplete())
	{
		return;
	}

	static LLTimer all_timer;
	static LLTimer sd_timer;
	static LLTimer element_timer;

	all_timer.reset();
	S32 i = 0;

	for( ; mMemberProgress != gdatap->mMembers.end() && i<UPDATE_MEMBERS_PER_FRAME; 
			++mMemberProgress, ++i)
	{
		//llinfos << "Adding " << iter->first << ", " << iter->second->getTitle() << llendl;
		LLGroupMemberData* member = mMemberProgress->second;
		if (!member)
		{
			continue;
		}
		// Owners show up in bold.
		std::string style = "NORMAL";
		if ( member->isOwner() )
		{
			style = "BOLD";
		}
		
		sd_timer.reset();
		LLSD row;
		row["id"] = member->getID();

		row["columns"][0]["column"] = "name";
		row["columns"][0]["font-style"] = style;
		// value is filled in by name list control

		row["columns"][1]["column"] = "title";
		row["columns"][1]["value"] = member->getTitle();
		row["columns"][1]["font-style"] = style;
		
		row["columns"][2]["column"] = "online";
		row["columns"][2]["value"] = member->getOnlineStatus();
		row["columns"][2]["font-style"] = style;

		sSDTime += sd_timer.getElapsedTimeF32();

		element_timer.reset();
		mListVisibleMembers->addElement(row);//, ADD_SORTED);
		sElementTime += element_timer.getElapsedTimeF32();
	}
	sAllTime += all_timer.getElapsedTimeF32();

	llinfos << "Updated " << i << " of " << UPDATE_MEMBERS_PER_FRAME << "members in the list." << llendl;
	if (mMemberProgress == gdatap->mMembers.end())
	{
		llinfos << "   member list completed." << llendl;
		mListVisibleMembers->setEnabled(TRUE);

		llinfos << "All Time: " << sAllTime << llendl;
		llinfos << "SD Time: " << sSDTime << llendl;
		llinfos << "Element Time: " << sElementTime << llendl;
	}
	else
	{
		mPendingMemberUpdate = TRUE;
		mListVisibleMembers->setEnabled(FALSE);
	}
}

void LLPanelGroupGeneral::updateChanged()
{
	// List all the controls we want to check for changes...
	LLUICtrl *check_list[] =
	{
		mGroupNameEditor,
		mGroupName,
		mFounderName,
		mInsignia,
		mEditCharter,
		mCtrlShowInGroupList,
		mComboMature,
		mCtrlOpenEnrollment,
		mCtrlEnrollmentFee,
		mSpinEnrollmentFee,
		mCtrlReceiveNotices,
		mCtrlListGroup,
		mActiveTitleLabel,
		mComboActiveTitle
	};

	mChanged = FALSE;

	for( int i= 0; i< sizeof(check_list)/sizeof(*check_list); i++ )
	{
		if( check_list[i] && check_list[i]->isDirty() )
		{
			mChanged = TRUE;
			break;
		}
	}
}
