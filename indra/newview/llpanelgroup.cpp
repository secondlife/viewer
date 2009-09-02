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

#include "llbutton.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "llviewermessage.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "llnotifications.h"
#include "llfloaterreg.h"
#include "llfloater.h"

#include "llsidetraypanelcontainer.h"

#include "llpanelgroupnotices.h"
#include "llpanelgroupgeneral.h"

#include "llsidetray.h"
#include "llaccordionctrltab.h"

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



void LLPanelGroupTab::handleClickHelp()
{
	// Display the help text.
	std::string help_text( getHelpText() );
	if ( !help_text.empty() )
	{
		LLSD args;
		args["MESSAGE"] = help_text;
		LLFloater* parent_floater = gFloaterView->getParentFloater(this);
		LLNotification::Params params(parent_floater->contextualNotification("GenericAlert"));
		params.substitutions(args);
		LLNotifications::instance().add(params);
	}
}

LLPanelGroup::LLPanelGroup()
:	LLPanel()
	,LLGroupMgrObserver( LLUUID() )
	,mAllowEdit(TRUE)
{
	// Set up the factory callbacks.
	// Roles sub tabs
	LLGroupMgr::getInstance()->addObserver(this);

}


LLPanelGroup::~LLPanelGroup()
{
	LLGroupMgr::getInstance()->removeObserver(this);
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


	button = getChild<LLButton>("btn_refresh");
	button->setClickedCallback(onBtnRefresh, this);
	button->setVisible(mAllowEdit);

	getChild<LLButton>("btn_create")->setVisible(false);

	childSetCommitCallback("btn_create",boost::bind(&LLPanelGroup::onBtnCreate,this),NULL);
	childSetCommitCallback("back",boost::bind(&LLPanelGroup::onBackBtnClick,this),NULL);

	LLPanelGroupTab* panel_general = findChild<LLPanelGroupTab>("group_general_tab_panel");
	LLPanelGroupTab* panel_roles = findChild<LLPanelGroupTab>("group_roles_tab_panel");
	LLPanelGroupTab* panel_notices = findChild<LLPanelGroupTab>("group_notices_tab_panel");
	LLPanelGroupTab* panel_land = findChild<LLPanelGroupTab>("group_land_tab_panel");

	if(panel_general)	mTabs.push_back(panel_general);
	if(panel_roles)		mTabs.push_back(panel_roles);
	if(panel_notices)	mTabs.push_back(panel_notices);
	if(panel_land)		mTabs.push_back(panel_land);

	panel_general->setupCtrls(this);
	
	return TRUE;
}

void LLPanelGroup::reshape(S32 width, S32 height, BOOL called_from_parent )
{
	LLPanel::reshape(width, height, called_from_parent );

	LLRect btn_rect;

	LLButton* button = findChild<LLButton>("btn_apply");
	if(button)
	{
		btn_rect = button->getRect();
		btn_rect.setLeftTopAndSize( btn_rect.mLeft, btn_rect.getHeight() + 2, btn_rect.getWidth(), btn_rect.getHeight());
		button->setRect(btn_rect);
	}

	button = findChild<LLButton>("btn_create");
	if(button)
	{
		btn_rect = button->getRect();
		btn_rect.setLeftTopAndSize( btn_rect.mLeft, btn_rect.getHeight() + 2, btn_rect.getWidth(), btn_rect.getHeight());
		button->setRect(btn_rect);
	}


	button = findChild<LLButton>("btn_refresh");
	if(button)
	{
		btn_rect = button->getRect();
		btn_rect.setLeftTopAndSize( btn_rect.mLeft, btn_rect.getHeight() + 2, btn_rect.getWidth(), btn_rect.getHeight());
		button->setRect(btn_rect);
	}
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
	panel_general->apply(apply_mesg);//yes yes you need to call apply to create...
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


void LLPanelGroup::changed(LLGroupChange gc)
{
	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		(*it)->update(gc);

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
	if(gdatap)
		childSetValue("group_name", gdatap->mName);
}

void LLPanelGroup::notifyObservers()
{
	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		(*it)->update(GC_ALL);

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
	if(gdatap)
		childSetValue("group_name", gdatap->mName);
	
}



void LLPanelGroup::setGroupID(const LLUUID& group_id)
{
	LLGroupMgr::getInstance()->removeObserver(this);
	mID = group_id;
	LLGroupMgr::getInstance()->addObserver(this);

	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		(*it)->setGroupID(group_id);

	LLGroupMgrGroupData* gdatap = LLGroupMgr::getInstance()->getGroupData(mID);
	if(gdatap)
		childSetValue("group_name", gdatap->mName);

	LLButton* button_apply = findChild<LLButton>("btn_apply");
	LLButton* button_refresh = findChild<LLButton>("btn_refresh");
	LLButton* button_create = findChild<LLButton>("btn_create");


	bool is_null_group_id = group_id == LLUUID::null;
	if(button_apply)
		button_apply->setVisible(!is_null_group_id);
	if(button_refresh)
		button_refresh->setVisible(!is_null_group_id);
	if(button_create)
		button_create->setVisible(is_null_group_id);

	getChild<LLUICtrl>("prepend_founded_by")->setVisible(!is_null_group_id);

	LLAccordionCtrlTab* tab_general = findChild<LLAccordionCtrlTab>("group_general_tab");
	LLAccordionCtrlTab* tab_roles = findChild<LLAccordionCtrlTab>("group_roles_tab");
	LLAccordionCtrlTab* tab_notices = findChild<LLAccordionCtrlTab>("group_notices_tab");
	LLAccordionCtrlTab* tab_land = findChild<LLAccordionCtrlTab>("group_land_tab");

	if(!tab_general || !tab_roles || !tab_notices || !tab_land)
		return;

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

		tab_roles->canOpenClose(false);
		tab_notices->canOpenClose(false);
		tab_land->canOpenClose(false);
	}
	else
	{
		if(!tab_general->getDisplayChildren())
			tab_general->changeOpenClose(tab_general->getDisplayChildren());
		if(!tab_roles->getDisplayChildren())
			tab_roles->changeOpenClose(tab_roles->getDisplayChildren());
		if(!tab_notices->getDisplayChildren())
			tab_notices->changeOpenClose(tab_notices->getDisplayChildren());
		if(!tab_land->getDisplayChildren())
			tab_land->changeOpenClose(tab_land->getDisplayChildren());
		
		tab_roles->canOpenClose(true);
		tab_notices->canOpenClose(true);
		tab_land->canOpenClose(true);
	}
}

bool	LLPanelGroup::apply(LLPanelGroupTab* tab)
{
	if(!tab)
		return false;

	std::string mesg;
	if ( !tab->needsApply(mesg) )
		return true;
	
	std::string apply_mesg;
	if(tab->apply( apply_mesg ) )
		return true;
		
	if ( !apply_mesg.empty() )
	{
		LLSD args;
		args["MESSAGE"] = apply_mesg;
		LLNotifications::instance().add("GenericAlert", args);
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

	bool enable = false;
	std::string mesg;
	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		enable = enable || (*it)->needsApply(mesg);

	childSetEnabled("btn_apply", enable);
}

void LLPanelGroup::refreshData()
{
	LLGroupMgr::getInstance()->clearGroupData(getID());
	
	for(std::vector<LLPanelGroupTab* >::iterator it = mTabs.begin();it!=mTabs.end();++it)
		(*it)->activate();
	

	// 5 second timeout
	childDisable("btn_refresh");
	mRefreshTimer.start();
	mRefreshTimer.setTimerExpirySec(5);
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

