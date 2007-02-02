/** 
 * @file llpanelgroupgeneral.cpp
 * @brief General information about a group.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelgroupgeneral.h"

#include "llvieweruictrlfactory.h"
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
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"

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
	mEditName(NULL),
	mBtnJoinGroup(NULL),
	mListVisibleMembers(NULL),
	mCtrlShowInGroupList(NULL),
	mCtrlPublishOnWeb(NULL),
	mCtrlMature(NULL),
	mCtrlOpenEnrollment(NULL),
	mCtrlEnrollmentFee(NULL),
	mSpinEnrollmentFee(NULL),
	mCtrlReceiveNotices(NULL),
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
	mGroupNameEditor = (LLLineEditor*) getChildByName("group_name_editor", recurse);
	mGroupName = (LLTextBox*) getChildByName("group_name", recurse);
	
	mInsignia = (LLTextureCtrl*) getChildByName("insignia", recurse);
	if (mInsignia)
	{
		mInsignia->setCommitCallback(onCommitAny);
		mInsignia->setCallbackUserData(this);
		mDefaultIconID = mInsignia->getImageAssetID();
	}
	
	mEditCharter = (LLTextEditor*) getChildByName("charter", recurse);
	if(mEditCharter)
	{
		mEditCharter->setCommitCallback(onCommitAny);
		mEditCharter->setFocusReceivedCallback(onCommitAny);
		mEditCharter->setFocusChangedCallback(onCommitAny);
		mEditCharter->setCallbackUserData(this);
	}

	mBtnJoinGroup = (LLButton*) getChildByName("join_button", recurse);
	if ( mBtnJoinGroup )
	{
		mBtnJoinGroup->setClickedCallback(onClickJoin);
		mBtnJoinGroup->setCallbackUserData(this);
	}

	mBtnInfo = (LLButton*) getChildByName("info_button", recurse);
	if ( mBtnInfo )
	{
		mBtnInfo->setClickedCallback(onClickInfo);
		mBtnInfo->setCallbackUserData(this);
	}

	LLTextBox* founder = (LLTextBox*) getChildByName("founder_name");
	if (founder)
	{
		mFounderName = new LLNameBox(founder->getName(),founder->getRect(),LLUUID::null,FALSE,founder->getFont(),founder->getMouseOpaque());
		removeChild(founder);
		addChild(mFounderName);
	}

	mListVisibleMembers = (LLNameListCtrl*) getChildByName("visible_members", recurse);
	if (mListVisibleMembers)
	{
		mListVisibleMembers->setDoubleClickCallback(openProfile);
		mListVisibleMembers->setCallbackUserData(this);
	}

	// Options
	mCtrlShowInGroupList = (LLCheckBoxCtrl*) getChildByName("show_in_group_list", recurse);
	if (mCtrlShowInGroupList)
	{
		mCtrlShowInGroupList->setCommitCallback(onCommitAny);
		mCtrlShowInGroupList->setCallbackUserData(this);
	}

	mCtrlPublishOnWeb = (LLCheckBoxCtrl*) getChildByName("publish_on_web", recurse);
	if (mCtrlPublishOnWeb)
	{
		mCtrlPublishOnWeb->setCommitCallback(onCommitAny);
		mCtrlPublishOnWeb->setCallbackUserData(this);
	}

	mCtrlMature = (LLCheckBoxCtrl*) getChildByName("mature", recurse);
	if (mCtrlMature)
	{
		mCtrlMature->setCommitCallback(onCommitAny);
		mCtrlMature->setCallbackUserData(this);
	}

	mCtrlOpenEnrollment = (LLCheckBoxCtrl*) getChildByName("open_enrollement", recurse);
	if (mCtrlOpenEnrollment)
	{
		mCtrlOpenEnrollment->setCommitCallback(onCommitAny);
		mCtrlOpenEnrollment->setCallbackUserData(this);
	}

	mCtrlEnrollmentFee = (LLCheckBoxCtrl*) getChildByName("check_enrollment_fee", recurse);
	if (mCtrlEnrollmentFee)
	{
		mCtrlEnrollmentFee->setCommitCallback(onCommitEnrollment);
		mCtrlEnrollmentFee->setCallbackUserData(this);
	}

	mSpinEnrollmentFee = (LLSpinCtrl*) getChildByName("spin_enrollment_fee", recurse);
	if (mSpinEnrollmentFee)
	{
		mSpinEnrollmentFee->setCommitCallback(onCommitAny);
		mSpinEnrollmentFee->setCallbackUserData(this);
	}

	BOOL accept_notices = FALSE;
	LLGroupData data;
	if(gAgent.getGroupData(mGroupID,data))
	{
		accept_notices = data.mAcceptNotices;
	}
	mCtrlReceiveNotices = (LLCheckBoxCtrl*) getChildByName("receive_notices", recurse);
	if (mCtrlReceiveNotices)
	{
		mCtrlReceiveNotices->setCommitCallback(onReceiveNotices);
		mCtrlReceiveNotices->setCallbackUserData(this);
		mCtrlReceiveNotices->set(accept_notices);
		mCtrlReceiveNotices->setEnabled(data.mID.notNull());
	}
	
	mActiveTitleLabel = (LLTextBox*) getChildByName("active_title_label", recurse);
	
	mComboActiveTitle = (LLComboBox*) getChildByName("active_title", recurse);
	if (mComboActiveTitle)
	{
		mComboActiveTitle->setCommitCallback(onCommitTitle);
		mComboActiveTitle->setCallbackUserData(this);
	}

	// Extra data
	LLTextBox* txt;
	// Don't recurse for this, since we don't currently have a recursive removeChild()
	txt = (LLTextBox*)getChildByName("incomplete_member_data_str");
	if (txt)
	{
		mIncompleteMemberDataStr = txt->getText();
		removeChild(txt);
	}

	txt = (LLTextBox*)getChildByName("confirm_group_create_str");
	if (txt)
	{
		mConfirmGroupCreateStr = txt->getText();
		removeChild(txt);
	}

	// If the group_id is null, then we are creating a new group
	if (mGroupID.isNull())
	{
		mGroupNameEditor->setEnabled(TRUE);
		mEditCharter->setEnabled(TRUE);

		mCtrlShowInGroupList->setEnabled(TRUE);
		mCtrlPublishOnWeb->setEnabled(TRUE);
		mCtrlMature->setEnabled(TRUE);
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
void LLPanelGroupGeneral::onCommitAny(LLUICtrl* ctrl, void* data)
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
	gGroupMgr->sendGroupTitleUpdate(self->mGroupID,self->mComboActiveTitle->getCurrentID());
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

	LLGroupMgrGroupData* gdatap = gGroupMgr->getGroupData(self->mGroupID);

	S32 cost = gdatap->mMembershipFee;
	LLString::format_map_t args;
	args["[COST]"] = llformat("%d", cost);

	if (can_afford_transaction(cost))
	{
		gViewerWindow->alertXml("JoinGroupCanAfford", args,
								LLPanelGroupGeneral::joinDlgCB,
								self);
	}
	else
	{
		gViewerWindow->alertXml("JoinGroupCannotAfford", args);
	}
}

// static
void LLPanelGroupGeneral::joinDlgCB(S32 which, void *userdata)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*) userdata;

	if (which == 1 || !self)
	{
		// user clicked cancel
		return;
	}

	gGroupMgr->sendGroupMemberJoin(self->mGroupID);
}

// static
void LLPanelGroupGeneral::onReceiveNotices(LLUICtrl* ctrl, void* data)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)data;
	LLCheckBoxCtrl* check = (LLCheckBoxCtrl*)ctrl;

	if(!self) return;
	gAgent.setGroupAcceptNotices(self->mGroupID, check->get());
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

bool LLPanelGroupGeneral::needsApply(LLString& mesg)
{ 
	llinfos << "LLPanelGroupGeneral::needsApply(LLString& mesg)  " << mChanged << llendl;

	mesg = "General group information has changed."; 
	return mChanged || mGroupID.isNull();
}

void LLPanelGroupGeneral::activate()
{
	LLGroupMgrGroupData* gdatap = gGroupMgr->getGroupData(mGroupID);
	if (mGroupID.notNull()
		&& (!gdatap || mFirstUse))
	{
		gGroupMgr->sendGroupTitlesRequest(mGroupID);
		gGroupMgr->sendGroupPropertiesRequest(mGroupID);

		
		if (!gdatap || !gdatap->isMemberDataComplete() )
		{
			gGroupMgr->sendGroupMembersRequest(mGroupID);
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

bool LLPanelGroupGeneral::apply(LLString& mesg)
{
	if (!mAllowEdit)
	{
		llwarns << "LLPanelGroupGeneral::apply() called with false mAllowEdit"
				<< llendl;
		return true;
	}

	llinfos << "LLPanelGroupGeneral::apply" << llendl;
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

		LLString::format_map_t args;
		args["[MESSAGE]"] = mConfirmGroupCreateStr;
		gViewerWindow->alertXml("GenericAlertYesCancel", args,
								createGroupCallback,this);

        return false;
	}

	LLGroupMgrGroupData* gdatap = gGroupMgr->getGroupData(mGroupID);

	if (!gdatap)
	{
		mesg = "No group data found for group ";
		mesg.append(mGroupID.getString());
		return false;
	}

	bool can_change_ident = false;
	bool can_change_member_opts = false;
	can_change_ident = gAgent.hasPowerInGroup(mGroupID,GP_GROUP_CHANGE_IDENTITY);
	can_change_member_opts = gAgent.hasPowerInGroup(mGroupID,GP_MEMBER_OPTIONS);

	if (can_change_ident)
	{
		if (mCtrlPublishOnWeb) gdatap->mAllowPublish = mCtrlPublishOnWeb->get();
		if (mEditCharter) gdatap->mCharter = mEditCharter->getText();
		if (mInsignia) gdatap->mInsigniaID = mInsignia->getImageAssetID();
		if (mCtrlMature) gdatap->mMaturePublish = mCtrlMature->get();
		if (mCtrlShowInGroupList) gdatap->mShowInList = mCtrlShowInGroupList->get();
	}

	if (can_change_member_opts)
	{
		if (mCtrlOpenEnrollment) gdatap->mOpenEnrollment = mCtrlOpenEnrollment->get();
		if (mCtrlEnrollmentFee && mSpinEnrollmentFee)
		{
			gdatap->mMembershipFee = (mCtrlEnrollmentFee->get()) ? 
										 (S32) mSpinEnrollmentFee->get() : 0;
		}
	}

	if (can_change_ident || can_change_member_opts)
	{
		gGroupMgr->sendUpdateGroupInfo(mGroupID);
	}

	if (mCtrlReceiveNotices) gAgent.setGroupAcceptNotices(mGroupID, mCtrlReceiveNotices->get());

	mChanged = FALSE;
	notifyObservers();

	return true;
}

void LLPanelGroupGeneral::cancel()
{
	mChanged = FALSE;

	//cancel out all of the click changes to, although since we are
	//shifting tabs or closing the floater, this need not be done...yet
	notifyObservers();
}

// static
void LLPanelGroupGeneral::createGroupCallback(S32 option, void* userdata)
{
	LLPanelGroupGeneral* self = (LLPanelGroupGeneral*)userdata;
	if (!self) return;

	switch(option)
	{
	case 0:
		{
			// Yay!  We are making a new group!
			U32 enrollment_fee = (self->mCtrlEnrollmentFee->get() ? 
									(U32) self->mSpinEnrollmentFee->get() : 0);
		
			gGroupMgr->sendCreateGroupRequest(self->mGroupNameEditor->getText(),
												self->mEditCharter->getText(),
												self->mCtrlShowInGroupList->get(),
												self->mInsignia->getImageAssetID(),
												enrollment_fee,
												self->mCtrlOpenEnrollment->get(),
												self->mCtrlPublishOnWeb->get(),
												self->mCtrlMature->get());

		}
		break;
	case 1:
	default:
		break;
	}
}

static F32 sSDTime = 0.0f;
static F32 sElementTime = 0.0f;
static F32 sAllTime = 0.0f;

// virtual
void LLPanelGroupGeneral::update(LLGroupChange gc)
{
	if (mGroupID.isNull()) return;

	LLGroupMgrGroupData* gdatap = gGroupMgr->getGroupData(mGroupID);

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
	if (mCtrlPublishOnWeb) 
	{
		mCtrlPublishOnWeb->set(gdatap->mAllowPublish);
		mCtrlPublishOnWeb->setEnabled(mAllowEdit && can_change_ident);
	}
	if (mCtrlMature)
	{
		mCtrlMature->set(gdatap->mMaturePublish);
		mCtrlMature->setEnabled(mAllowEdit && can_change_ident);
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
		char fee_buff[20];		/*Flawfinder: ignore*/
		bool visible;

		visible = !is_member && gdatap->mOpenEnrollment;
		mBtnJoinGroup->setVisible(visible);

		if ( visible )
		{
			snprintf(fee_buff, sizeof(fee_buff), "Join (L$%d)", gdatap->mMembershipFee);		/*Flawfinder: ignore*/
			mBtnJoinGroup->setLabelSelected(std::string(fee_buff));
			mBtnJoinGroup->setLabelUnselected(std::string(fee_buff));
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
			mCtrlReceiveNotices->set(agent_gdatap.mAcceptNotices);
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
			LLScrollListItem* row = new LLScrollListItem( TRUE, NULL, LLUUID::null );
			std::stringstream pending;
			pending << "Retrieving member list (" << gdatap->mMembers.size() << "\\" << gdatap->mMemberCount  << ")";
			row->addColumn(pending.str(), LLFontGL::sSansSerif);
			mListVisibleMembers->setEnabled(FALSE);
			mListVisibleMembers->addItem(row);
		}
	}
}

void LLPanelGroupGeneral::updateMembers()
{
	mPendingMemberUpdate = FALSE;

	LLGroupMgrGroupData* gdatap = gGroupMgr->getGroupData(mGroupID);

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
	LLGroupMgrGroupData::member_iter end = gdatap->mMembers.end();

	for( ; mMemberProgress != end && i<UPDATE_MEMBERS_PER_FRAME; 
			++mMemberProgress, ++i)
	{
		//llinfos << "Adding " << iter->first << ", " << iter->second->getTitle() << llendl;
		LLGroupMemberData* member = mMemberProgress->second;
		if (!member)
		{
			continue;
		}
		// Owners show up in bold.
		LLString style = "NORMAL";
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
	if (mMemberProgress == end)
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
