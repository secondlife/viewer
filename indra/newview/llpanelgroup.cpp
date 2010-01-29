/** 
 * @file llpanelgroup.cpp
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

#include "llpanelgroup.h"

// Library includes
#include "llbutton.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"

// Viewer includes
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llnotificationsutil.h"
#include "llfloaterreg.h"
#include "llfloater.h"
#include "llgroupactions.h"

#include "llagent.h" 

#include "llsidetraypanelcontainer.h"

#include "llpanelgroupnotices.h"
#include "llpanelgroupgeneral.h"

#include "llsidetray.h"
#include "llaccordionctrltab.h"
#include "llaccordionctrl.h"

static LLRegisterPanelClassWrapper<LLPanelGroup> t_panel_group("panel_group_info_sidetray");



LLPanelGroupTab::LLPanelGroupTab()
	: LLPanel(),
	  mAllowEdit(TRUE),
	  mHasModal(FALSE)
{
	mGroupID = LLUUID::null;
}

LLPanelGroupTab::~LLPanelGroupTab()
{
}

BOOL LLPanelGroupTab::isVisibleByAgent(LLAgent* agentp)
{
	//default to being visible
	return TRUE;
}

BOOL LLPanelGroupTab::postBuild()
{
	return TRUE;
}

LLPanelGroup::LLPanelGroup()
:	LLPanel(),
	LLGroupMgrObserver( LLUUID() ),
	mSkipRefresh(FALSE),
	mShowingNotifyDialog(false)
{
	// Set up the factory callbacks.
	// Roles sub tabs
	LLGroupMgr::getInstance()->addObserver(this);
}


LLPanelGroup::~LLPanelGroup()
{
	LLGroupMgr::getInstance()->removeObserver(this);
	if(LLVoiceClient::getInstance())
		LLVoiceClient::getInstance()->removeObserver(this);
}

void LLPanelGroup::onOpen(const LLSD& key)
{
	if(!key.has("group_id"))
		return;
	
	LLUUID group_id = key["group_id"];
	if(!key.has("action"))
	{
		setGroupID(group_id);
		return;
	}

	std::string str_action = key["action"];

	if(str_action == "refresh")
	{
		if(mID == group_id || group_id == LLUUID::null)
			refreshData();
	}
	else if(str_action == "close")
	{
		onBackBtnClick();
	}
	else if(str_action == "create")
	{
		setGroupID(LLUUID::null);
	}
	else if(str_action == "refresh_notices")
	{
		LLPanelGroupNotices* panel_notices = findChild<LLPanelGroupNotices>("group_notices_tab_panel");
		if(panel_notices)
			panel_notices->refreshNotices();
	}

}

BOOL LLPanelGroup::postBuild()
{
	mDefaultNeedsApplyMesg = getString("default_needs_apply_text");
	mWantApplyMesg = getString("want_apply_text");

	LLButton* button;

	button = getChild<LLButton>("btn_apply");
	button->setClickedCallback(onBtnApply, this);
	button->setVisible(true);
	button->setEnabled(false);

	button = getChild<LLButton>("btn_call");
	button->setClickedCallback(onBtnGroupCallClicked, this);

	button = getChild<LLButton>("btn_chat");
	button->setClickedCallback(onBtnGroupChatClicked, this);

	button = getChild<LLButton>("btn_join");
	button->setVisible(false);
	button->setEnabled(true);

	button = getChild<LLButton>("btn_cancel");
	button->setVisible(false);	button->setEnabled(true);

	button = getChild<LLButton>("btn_refresh");
	button->setClickedCallback(onBtnRefresh, this);

	getChild<LLButton>("btn_create")->setVisible(false);

	childSetCommitCallback("back",boost::bind(&LLPanelGroup::onBackBtnClick,this),NULL);

	childSetCommitCallback("btn_create",boost::bind(&LLPanelGroup::onBtnCreate,this),NULL);
	childSetCommitCallback("btn_join",boost::bind(&LLPanelGroup::onBtnJoin,this),NULL);
	childSetCommitCallback("btn_cancel",boost::bind(&LLPanelGroup::onBtnCancel,this),NULL);

	LLPanelGroupTab* panel_general = findChild<LLPanelGroupTab>("group_general_tab_panel");
	LLPanelGroupTab* panel_roles = findChild<LLPanelGroupTab>("group_roles_tab_panel");
	LLPanelGroupTab* panel_notices = findChild<LLPanelGroupTab>("group_notices_tab_panel");
	LLPanelGroupTab* panel_land = findChild<LLPanelGroupTab>("group_land_tab_panel");

	if(panel_general)	mTabs.push_back(panel_general);
	if(panel_roles)		mTabs.push_back(panel_roles);
	if(panel_notices)	mTabs.push_back(panel_notices);
	if(panel_land)		mTabs.push_back(panel_land);

	if(panel_general)
		panel_general->setupCtrls(this);

	gVoiceClient->addObserver(this);
	
	return TRUE;
}

void LLPanelGroup::reposButton(const std::string& name)
{
	LLButton* button = findChild<LLButton>(name);
	if(!button)
		return;
	LLRect btn_rect = button->getRect();
	btn_rect.setLeftTopAndSize( btn_rect.mLeft, btn_rect.getHeight() + 2, btn_rect.getWidth(), btn_rect.getHeight());
	button->setRect(btn_rect);
}

void LLPanelGroup::reposButtons()
{
	LLButton* button_refresh = findChild<LLButton>("btn_refresh");
	LLButton* button_cancel = findChild<LLButton>("btn_cancel");

	if(button_refresh && button_cancel && button_refresh->getVisible() && button_cancel->getVisible())
	{
		LLRect btn_refresh_rect = button_refresh->getRect();
		LLRect btn_cancel_rect = button_cancel->getRect();
		btn_refresh_rect.setLeftTopAndSize( btn_cancel_rect.mLeft + btn_cancel_rect.getWidth() + 2, 
			btn_refresh_rect.getHeight() + 2, btn_refresh_rect.getWidth(), btn_refresh_rect.getHeight());
		button_refresh->setRect(btn_refresh_rect);
	}

	reposButton("btn_apply");
	reposButton("btn_create");
	reposButton("btn_refresh");
	reposButton("btn_cancel");
	reposButton("btn_chat");
	reposButton("btn_call");
}

void LLPanelGroup::reshape(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width, height, called_from_parent );

	reposButtons();
}

void LLPanelGroup::onBackBtnClick()
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if(parent)
	{
		parent->openPreviousPanel();
	}
}


void LLPanelGroup::onBtnCreate()
{
	LLPanelGroupGeneral* panel_general = findChild<LLPanelGroupGeneral>("group_general_tab_panel");
	if(!panel_general)
		return;
	std::string apply_mesg;
	if(panel_general->apply(apply_mesg))//yes yes you need to call apply to create...
		return;
	if ( !apply_mesg.empty() )
	{
		LLSD args;
		args["MESSAGE"] = apply_mesg;
		LLNotificationsUtil::add("GenericAlert", args);
	}
}

void LLPanelGroup::onBtnRefresh(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->refreshData();
}

void LLPanelGroup::onBtnApply(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->apply();
}

void LLPanelGroup::onBtnGroupCallClicked(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->callGroup();
}

void LLPanelGroup::onBtnGroupChatClicked(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->chatGroup();
}

void LLPanelGroup::onBtnJoin()
{
	lldebugs << "joining group: " << mID << llendl;
	LLGroupActions::join(mID);
}

void LLPanelGroup::onBtnCancel()
{
	onBackBtnClick();
}


void LLPanelGroup::changed(LLGroupChange gc)
{
	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		(*it)->update(gc);
	update(gc);
}

// virtual
void LLPanelGroup::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}

	childSetEnabled("btn_call", LLVoiceClient::voiceEnabled() && gVoiceClient->voiceWorking());
}

void LLPanelGroup::notifyObservers()
{
	changed(GC_ALL);
}

void LLPanelGroup::update(LLGroupChange gc)
{
	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
	if(gdatap)
	{
		childSetValue("group_name", gdatap->mName);
		childSetToolTip("group_name",gdatap->mName);

		LLButton* btn_join = getChild<LLButton>("btn_join");
		LLUICtrl* join_text = getChild<LLUICtrl>("join_cost_text");

		LLGroupData agent_gdatap;
		bool is_member = gAgent.getGroupData(mID,agent_gdatap);
		bool join_btn_visible = !is_member && gdatap->mOpenEnrollment;
		
		btn_join->setVisible(join_btn_visible);
		join_text->setVisible(join_btn_visible);

		if(join_btn_visible)
		{
			LLStringUtil::format_map_t string_args;
			std::string fee_buff;
			if(gdatap->mMembershipFee)
			{
				string_args["[AMOUNT]"] = llformat("%d", gdatap->mMembershipFee);
				fee_buff = getString("group_join_btn", string_args);
				
			}
			else
			{
				fee_buff = getString("group_join_free", string_args);
			}
			childSetValue("join_cost_text",fee_buff);
		}
	}
}

void LLPanelGroup::setGroupID(const LLUUID& group_id)
{
	std::string str_group_id;
	group_id.toString(str_group_id);

	bool is_same_id = group_id == mID;
	
	LLGroupMgr::getInstance()->removeObserver(this);
	mID = group_id;
	LLGroupMgr::getInstance()->addObserver(this);

	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		(*it)->setGroupID(group_id);

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
	if(gdatap)
	{
		childSetValue("group_name", gdatap->mName);
		childSetToolTip("group_name",gdatap->mName);
	}

	LLButton* button_apply = findChild<LLButton>("btn_apply");
	LLButton* button_refresh = findChild<LLButton>("btn_refresh");
	LLButton* button_create = findChild<LLButton>("btn_create");
	LLButton* button_join = findChild<LLButton>("btn_join");
	LLButton* button_cancel = findChild<LLButton>("btn_cancel");
	LLButton* button_call = findChild<LLButton>("btn_call");
	LLButton* button_chat = findChild<LLButton>("btn_chat");


	bool is_null_group_id = group_id == LLUUID::null;
	if(button_apply)
		button_apply->setVisible(!is_null_group_id);
	if(button_refresh)
		button_refresh->setVisible(!is_null_group_id);

	if(button_create)
		button_create->setVisible(is_null_group_id);
	if(button_cancel)
		button_cancel->setVisible(!is_null_group_id);

	if(button_call)
			button_call->setVisible(!is_null_group_id);
	if(button_chat)
			button_chat->setVisible(!is_null_group_id);

	getChild<LLUICtrl>("prepend_founded_by")->setVisible(!is_null_group_id);

	LLAccordionCtrl* tab_ctrl = findChild<LLAccordionCtrl>("group_accordion");
	if(tab_ctrl)
		tab_ctrl->reset();

	LLAccordionCtrlTab* tab_general = findChild<LLAccordionCtrlTab>("group_general_tab");
	LLAccordionCtrlTab* tab_roles = findChild<LLAccordionCtrlTab>("group_roles_tab");
	LLAccordionCtrlTab* tab_notices = findChild<LLAccordionCtrlTab>("group_notices_tab");
	LLAccordionCtrlTab* tab_land = findChild<LLAccordionCtrlTab>("group_land_tab");


	if(!tab_general || !tab_roles || !tab_notices || !tab_land)
		return;
	
	if(button_join)
		button_join->setVisible(false);


	if(is_null_group_id)//creating new group
	{
		if(!tab_general->getDisplayChildren())
			tab_general->changeOpenClose(tab_general->getDisplayChildren());
		
		if(tab_roles->getDisplayChildren())
			tab_roles->changeOpenClose(tab_roles->getDisplayChildren());
		if(tab_notices->getDisplayChildren())
			tab_notices->changeOpenClose(tab_notices->getDisplayChildren());
		if(tab_land->getDisplayChildren())
			tab_land->changeOpenClose(tab_land->getDisplayChildren());

		tab_roles->setVisible(false);
		tab_notices->setVisible(false);
		tab_land->setVisible(false);

		getChild<LLUICtrl>("group_name")->setVisible(false);
		getChild<LLUICtrl>("group_name_editor")->setVisible(true);

		if(button_call)
			button_call->setVisible(false);
		if(button_chat)
			button_chat->setVisible(false);
	}
	else 
	{
		if(!is_same_id)
		{
			if(!tab_general->getDisplayChildren())
				tab_general->changeOpenClose(tab_general->getDisplayChildren());
			if(tab_roles->getDisplayChildren())
				tab_roles->changeOpenClose(tab_roles->getDisplayChildren());
			if(tab_notices->getDisplayChildren())
				tab_notices->changeOpenClose(tab_notices->getDisplayChildren());
			if(tab_land->getDisplayChildren())
				tab_land->changeOpenClose(tab_land->getDisplayChildren());
		}

		LLGroupData agent_gdatap;
		bool is_member = gAgent.getGroupData(mID,agent_gdatap);
		
		tab_roles->setVisible(is_member);
		tab_notices->setVisible(is_member);
		tab_land->setVisible(is_member);

		getChild<LLUICtrl>("group_name")->setVisible(true);
		getChild<LLUICtrl>("group_name_editor")->setVisible(false);

		if(button_apply)
			button_apply->setVisible(is_member);
		if(button_call)
			button_call->setVisible(is_member);
		if(button_chat)
			button_chat->setVisible(is_member);
	}

	reposButtons();
}

bool LLPanelGroup::apply(LLPanelGroupTab* tab)
{
	if(!tab)
		return false;

	std::string mesg;
	if ( !tab->needsApply(mesg) )
		return true;
	
	std::string apply_mesg;
	if(tab->apply( apply_mesg ) )
	{
		//we skip refreshing group after ew manually apply changes since its very annoying
		//for those who are editing group
		mSkipRefresh = TRUE;
		return true;
	}
		
	if ( !apply_mesg.empty() )
	{
		LLSD args;
		args["MESSAGE"] = apply_mesg;
		LLNotificationsUtil::add("GenericAlert", args);
	}
	return false;
}

bool LLPanelGroup::apply()
{
	return apply(findChild<LLPanelGroupTab>("group_general_tab_panel")) 
		&& apply(findChild<LLPanelGroupTab>("group_roles_tab_panel"))
		&& apply(findChild<LLPanelGroupTab>("group_notices_tab_panel"))
		&& apply(findChild<LLPanelGroupTab>("group_land_tab_panel"))
		;
}


// virtual
void LLPanelGroup::draw()
{
	LLPanel::draw();

	if (mRefreshTimer.hasExpired())
	{
		mRefreshTimer.stop();
		childEnable("btn_refresh");
	}

	LLButton* button_apply = findChild<LLButton>("btn_apply");
	
	if(button_apply && button_apply->getVisible())
	{
		bool enable = false;
		std::string mesg;
		for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
			enable = enable || (*it)->needsApply(mesg);

		childSetEnabled("btn_apply", enable);
	}
}

void LLPanelGroup::refreshData()
{
	if(mSkipRefresh)
	{
		mSkipRefresh = FALSE;
		return;
	}
	LLGroupMgr::getInstance()->clearGroupData(getID());

	setGroupID(getID());
	
	// 5 second timeout
	childDisable("btn_refresh");
	mRefreshTimer.start();
	mRefreshTimer.setTimerExpirySec(5);
}

void LLPanelGroup::callGroup()
{
	LLGroupActions::startCall(getID());
}

void LLPanelGroup::chatGroup()
{
	LLGroupActions::startIM(getID());
}

void LLPanelGroup::showNotice(const std::string& subject,
			      const std::string& message,
			      const bool& has_inventory,
			      const std::string& inventory_name,
			      LLOfferInfo* inventory_offer)
{
	LLPanelGroupNotices* panel_notices = findChild<LLPanelGroupNotices>("group_notices_tab_panel");
	if(!panel_notices)
	{
		// We need to clean up that inventory offer.
		if (inventory_offer)
		{
			inventory_offer->forceResponse(IOR_DECLINE);
		}
		return;
	}
	panel_notices->showNotice(subject,message,has_inventory,inventory_name,inventory_offer);
}




//static
void LLPanelGroup::refreshCreatedGroup(const LLUUID& group_id)
{
	LLPanelGroup* panel = LLSideTray::getInstance()->findChild<LLPanelGroup>("panel_group_info_sidetray");
	if(!panel)
		return;
	panel->setGroupID(group_id);
}

//static

void LLPanelGroup::showNotice(const std::string& subject,
					   const std::string& message,
					   const LLUUID& group_id,
					   const bool& has_inventory,
					   const std::string& inventory_name,
					   LLOfferInfo* inventory_offer)
{
	LLPanelGroup* panel = LLSideTray::getInstance()->findChild<LLPanelGroup>("panel_group_info_sidetray");
	if(!panel)
		return;

	if(panel->getID() != group_id)//???? only for current group_id or switch panels? FIXME
		return;
	panel->showNotice(subject,message,has_inventory,inventory_name,inventory_offer);

}

bool	LLPanelGroup::canClose()
{
	if(getVisible() == false)
		return true;

	bool need_save = false;
	std::string mesg;
	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		if(need_save|=(*it)->needsApply(mesg))
			break;
	if(!need_save)
		return false;
	// If no message was provided, give a generic one.
	if (mesg.empty())
	{
		mesg = mDefaultNeedsApplyMesg;
	}
	// Create a notify box, telling the user about the unapplied tab.
	LLSD args;
	args["NEEDS_APPLY_MESSAGE"] = mesg;
	args["WANT_APPLY_MESSAGE"] = mWantApplyMesg;

	LLNotificationsUtil::add("SaveChanges", args, LLSD(), boost::bind(&LLPanelGroup::handleNotifyCallback,this, _1, _2));

	mShowingNotifyDialog = true;

	return false;
}

bool	LLPanelGroup::notifyChildren(const LLSD& info)
{
	if(info.has("request") && mID.isNull() )
	{
		std::string str_action = info["request"];

		if (str_action == "quit" )
		{
			canClose();
			return true;
		}
		if(str_action == "wait_quit")
			return mShowingNotifyDialog;
	}
	return false;
}
bool LLPanelGroup::handleNotifyCallback(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	mShowingNotifyDialog = false;
	switch (option)
	{
	case 0: // "Apply Changes"
		apply();
		break;
	case 1: // "Ignore Changes"
		break;
	case 2: // "Cancel"
	default:
		// Do nothing.  The user is canceling the action.
		// If we were quitting, we didn't really mean it.
		LLAppViewer::instance()->abortQuit();
		break;
	}
	return false;
}

