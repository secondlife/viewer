/** 
 * @file llpanelgroup.cpp
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llagent.h"
#include "llbutton.h"
#include "llpanelgroupgeneral.h"
#include "llpanelgrouproles.h"
#include "llpanelgroupvoting.h"
#include "llpanelgrouplandmoney.h"
#include "llpanelgroupnotices.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "llviewermessage.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llappviewer.h"

// static
void* LLPanelGroupTab::createTab(void* data)
{
	LLUUID* group_id = static_cast<LLUUID*>(data);
	return new LLPanelGroupTab("panel group tab", *group_id);
}

LLPanelGroupTab::~LLPanelGroupTab()
{
	mObservers.clear();
}

BOOL LLPanelGroupTab::isVisibleByAgent(LLAgent* agentp)
{
	//default to being visible
	return TRUE;
}

BOOL LLPanelGroupTab::postBuild()
{
	// Hook up the help button callback.
	LLButton* button = getChild<LLButton>("help_button");
	if (button)
	{
		button->setClickedCallback(onClickHelp);
		button->setCallbackUserData(this);
	}

	mHelpText = getString("help_text");
	return TRUE;
}

void LLPanelGroupTab::addObserver(LLPanelGroupTabObserver *obs)
{
	mObservers.insert(obs);
}

void LLPanelGroupTab::removeObserver(LLPanelGroupTabObserver *obs)
{
	mObservers.erase(obs);
}

void LLPanelGroupTab::notifyObservers()
{

	for (observer_list_t::iterator iter = mObservers.begin();
		 iter != mObservers.end(); )
	{
		LLPanelGroupTabObserver* observer = *iter;
		observer->tabChanged();

		// safe way to incrament since changed may delete entries! (@!##%@!@&*!)
		iter = mObservers.upper_bound(observer); 
	}
}

// static
void LLPanelGroupTab::onClickHelp(void* user_data)
{
	LLPanelGroupTab* self = static_cast<LLPanelGroupTab*>(user_data);
	self->handleClickHelp();
}

void LLPanelGroupTab::handleClickHelp()
{
	// Display the help text.
	std::string help_text( getHelpText() );
	if ( !help_text.empty() )
	{
		LLStringUtil::format_map_t args;
		args["[MESSAGE]"] = help_text;
		LLAlertDialog* dialogp = gViewerWindow->alertXml("GenericAlert", args);
		if (dialogp)
		{
			LLFloater* root_floater = gFloaterView->getParentFloater(this);;
			if (root_floater)
			{
				root_floater->addDependentFloater(dialogp);
			}
		}
	}
}

LLPanelGroup::LLPanelGroup(const std::string& filename,
						   const std::string& name,
						   const LLUUID& group_id,
						   const std::string& initial_tab_selected)
:	LLPanel(name, LLRect(), FALSE),
	LLGroupMgrObserver( group_id ),
	mCurrentTab( NULL ),
	mRequestedTab( NULL ),
	mTabContainer( NULL ),
	mIgnoreTransition( FALSE ),
	mForceClose( FALSE ),
	mInitialTab(initial_tab_selected),
	mAllowEdit( TRUE ),
	mShowingNotifyDialog( FALSE )
{
	// Set up the factory callbacks.
	mFactoryMap["general_tab"]	= LLCallbackMap(LLPanelGroupGeneral::createTab,
												&mID);
	mFactoryMap["roles_tab"]	= LLCallbackMap(LLPanelGroupRoles::createTab,
												&mID);
	mFactoryMap["notices_tab"]	= LLCallbackMap(LLPanelGroupNotices::createTab,
												&mID);
	mFactoryMap["voting_tab"]	= LLCallbackMap(LLPanelGroupVoting::createTab,
												&mID);
	mFactoryMap["land_money_tab"]= LLCallbackMap(LLPanelGroupLandMoney::createTab,
												 &mID);
	// Roles sub tabs
	mFactoryMap["members_sub_tab"] = LLCallbackMap(LLPanelGroupMembersSubTab::createTab, &mID);
	mFactoryMap["roles_sub_tab"] = LLCallbackMap(LLPanelGroupRolesSubTab::createTab, &mID);
	mFactoryMap["actions_sub_tab"] = LLCallbackMap(LLPanelGroupActionsSubTab::createTab, &mID);

	LLGroupMgr::getInstance()->addObserver(this);

	// Pass on construction of this panel to the control factory.
	LLUICtrlFactory::getInstance()->buildPanel(this, filename, &getFactoryMap());
	mFilename = filename;
}

LLPanelGroup::~LLPanelGroup()
{
	LLGroupMgr::getInstance()->removeObserver(this);

	int i;
	int tab_count = mTabContainer->getTabCount();

	for (i = tab_count - 1; i >=0; --i)
	{
		LLPanelGroupTab* panelp =
			(LLPanelGroupTab*) mTabContainer->getPanelByIndex(i);

		if ( panelp ) panelp->removeObserver(this);
	}
}

void LLPanelGroup::updateTabVisibility()
{
	S32 i;
	S32 tab_count = mTabContainer->getTabCount();

	for (i = tab_count - 1; i >=0; --i)
	{
		LLPanelGroupTab* panelp =
			(LLPanelGroupTab*) mTabContainer->getPanelByIndex(i);

		BOOL visible = panelp->isVisibleByAgent(&gAgent) || gAgent.isGodlike();
		mTabContainer->enableTabButton(i, visible);

		if ( !visible && mCurrentTab == panelp )
		{
			//we are disabling the currently selected tab
			//select the previous one
			mTabContainer->selectPrevTab();
			mCurrentTab = 
				(LLPanelGroupTab*) mTabContainer->getCurrentPanel();
		}
	}
}



BOOL LLPanelGroup::postBuild()
{
	mTabContainer = getChild<LLTabContainer>("group_tab_container");

	if (mTabContainer)
	{
		// Select the initial tab specified via constructor
		const BOOL recurse = TRUE;
		LLPanelGroupTab* tabp = 
			getChild<LLPanelGroupTab>(mInitialTab, recurse);

		if (!tabp)
		{
			//our initial tab selection was invalid, just select the
			//first tab then or default to selecting the initial
			//selected tab specified in the layout file
			tabp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();

			//no tab was initially selected through constructor
			//or the XML, select the first tab
			if (!tabp)
			{
				mTabContainer->selectFirstTab();
				tabp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
			}
		}
		else
		{
			mTabContainer->selectTabPanel(tabp);
		}

		mCurrentTab = tabp;

		// Add click callbacks.
		S32 i;
		S32 tab_count = mTabContainer->getTabCount();

		for (i = tab_count - 1; i >=0; --i)
		{
			LLPanel* tab_panel = mTabContainer->getPanelByIndex(i);
			LLPanelGroupTab* panelp =(LLPanelGroupTab*)tab_panel; // bit of a hack

			// Pass on whether or not to allow edit to tabs.
			panelp->setAllowEdit(mAllowEdit);
			panelp->addObserver(this);

			mTabContainer->setTabChangeCallback(panelp, onClickTab);
			mTabContainer->setTabUserData(panelp, this);
		}
		updateTabVisibility();

		// Act as though this tab was just activated.
		mCurrentTab->activate();
	}

	mDefaultNeedsApplyMesg = getString("default_needs_apply_text");
	mWantApplyMesg = getString("want_apply_text");

	LLButton* button = getChild<LLButton>("btn_ok");
	if (button)
	{
		button->setClickedCallback(onBtnOK);
		button->setCallbackUserData(this);
		button->setVisible(mAllowEdit);
	}
	
	button = getChild<LLButton>("btn_cancel");
	if (button)
	{
		button->setClickedCallback(onBtnCancel);
	   	button->setCallbackUserData(this);
		button->setVisible(mAllowEdit);
	}

	button = getChild<LLButton>("btn_apply");
	if (button)
	{
		button->setClickedCallback(onBtnApply);
		button->setVisible(mAllowEdit);
		button->setEnabled(FALSE);

		mApplyBtn = button;
	}

	button = getChild<LLButton>("btn_refresh");
	if (button)
	{
		button->setClickedCallback(onBtnRefresh);
		button->setCallbackUserData(this);
		button->setVisible(mAllowEdit);
	}

	return TRUE;
}

void LLPanelGroup::changed(LLGroupChange gc)
{
	updateTabVisibility();
	// Notify the currently active panel that group manager information has changed.
	LLPanelGroupTab* panelp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();

	if (panelp)
	{
		panelp->update(gc);
	}
}

// PanelGroupTab observer trigger
void LLPanelGroup::tabChanged()
{
	//some tab information has changed,....enable/disable the apply button
	//based on if they need an apply
	if ( mApplyBtn )
	{
		std::string mesg;
		mApplyBtn->setEnabled(mCurrentTab->needsApply(mesg));
	}
}

// static
void LLPanelGroup::onClickTab(void* user_data, bool from_click)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->handleClickTab();
}

void LLPanelGroup::handleClickTab()
{
	// If we are already handling a transition,
	// ignore this.
	if (mIgnoreTransition)
	{
		return;
	}

	mRequestedTab = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();

	// Make sure they aren't just clicking the same tab...
	if (mRequestedTab == mCurrentTab)
	{
		return;
	}

	// Try to switch from the current panel to the panel the user selected.
	attemptTransition();
}

void LLPanelGroup::setGroupID(const LLUUID& group_id)
{
	LLRect rect(getRect());

	LLGroupMgr::getInstance()->removeObserver(this);
	mID = group_id;
	LLGroupMgr::getInstance()->addObserver(this);
	//TODO:  this is really bad, we should add a method
	// where the panels can just update themselves
	// on a group id change.  Similar to update() but with a group
	// id change.

	// For now, rebuild panel
	//delete children and rebuild panel
	deleteAllChildren();
	LLUICtrlFactory::getInstance()->buildPanel(this, mFilename, &getFactoryMap());
}

void LLPanelGroup::selectTab(std::string tab_name)
{
	const BOOL recurse = TRUE;

	LLPanelGroupTab* tabp = 
		getChild<LLPanelGroupTab>(tab_name, recurse);

	if ( tabp && mTabContainer )
	{
		mTabContainer->selectTabPanel(tabp);
		onClickTab(this, false);
	}
}

BOOL LLPanelGroup::canClose()
{
	if (mShowingNotifyDialog) return FALSE;
	if (mCurrentTab && mCurrentTab->hasModal()) return FALSE;
	if (mForceClose || !mAllowEdit) return TRUE;

	// Try to switch from the current panel to nothing, indicating a close action.
	mRequestedTab = NULL;
	return attemptTransition();
}

BOOL LLPanelGroup::attemptTransition()
{
	// Check if the current tab needs to be applied.
	std::string mesg;
	if (mCurrentTab && mCurrentTab->needsApply(mesg))
	{
		// If no message was provided, give a generic one.
		if (mesg.empty())
		{
			mesg = mDefaultNeedsApplyMesg;
		}
		// Create a notify box, telling the user about the unapplied tab.
		LLStringUtil::format_map_t args;
		args["[NEEDS_APPLY_MESSAGE]"] = mesg;
		args["[WANT_APPLY_MESSAGE]"] = mWantApplyMesg;
		gViewerWindow->alertXml("PanelGroupApply", args,
								onNotifyCallback, (void*) this);
		mShowingNotifyDialog = TRUE;
		
		// We need to reselect the current tab, since it isn't finished.
		if (mTabContainer)
		{
			// selectTabPanel is going to trigger another
			// click event.  We want to ignore it so that
			// mRequestedTab is not updated.
			mIgnoreTransition = TRUE;
			mTabContainer->selectTabPanel( mCurrentTab );
			mIgnoreTransition = FALSE;
		}
		// Returning FALSE will block a close action from finishing until
		// we get a response back from the user.
		return FALSE;
	}
	else
	{
		// The current panel didn't have anything it needed to apply.
		if ( mRequestedTab )
		{
			transitionToTab();
		}
		// Returning TRUE will allow any close action to proceed.
		return TRUE;
	}
}

void LLPanelGroup::transitionToTab()
{
	// Tell the current panel that it is being deactivated.
	if (mCurrentTab)
	{
		mCurrentTab->deactivate();
	}
	
	// If the requested panel exists, activate it.
	if (mRequestedTab)
	{
		// This is now the current tab;
		mCurrentTab = mRequestedTab;
		mCurrentTab->activate();
	}
	else // NULL requested indicates a close action.
	{
		close();
	}
}

// static
void LLPanelGroup::onNotifyCallback(S32 option, void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	if (self)
	{
		self->handleNotifyCallback(option);
	}
}

void LLPanelGroup::handleNotifyCallback(S32 option)
{
	mShowingNotifyDialog = FALSE;
	switch (option)
	{
	case 0: // "Apply Changes"
		// Try to apply changes, and switch to the requested tab.
		if ( !apply() )
		{
			// There was a problem doing the apply.
			// Skip switching tabs.
			break;
		}

		// This panel's info successfully applied.
		// Switch to the next panel.
		mIgnoreTransition = TRUE;
		mTabContainer->selectTabPanel( mRequestedTab );
		mIgnoreTransition = FALSE;
		transitionToTab();
		break;
	case 1: // "Ignore Changes"
		// Switch to the requested panel without applying changes
		// (Changes may already have been applied in the previous block)
		mCurrentTab->cancel();
		mIgnoreTransition = TRUE;
		mTabContainer->selectTabPanel( mRequestedTab );
		mIgnoreTransition = FALSE;
		transitionToTab();
		break;
	case 2: // "Cancel"
	default:
		// Do nothing.  The user is canceling the action.
		// If we were quitting, we didn't really mean it.
		LLAppViewer::instance()->abortQuit();
		break;
	}
}

// static
void LLPanelGroup::onBtnOK(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	// If we are able to apply changes, then close.
	if(self->apply())
	{
		self->close();
	}
}

// static
void LLPanelGroup::onBtnCancel(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->close();
}

// static
void LLPanelGroup::onBtnApply(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->apply();
}

bool LLPanelGroup::apply()
{
	// Pass this along to the currently visible tab.
	if (!mTabContainer) return false;

	LLPanelGroupTab* panelp = (LLPanelGroupTab*) mTabContainer->getCurrentPanel();
	if (!panelp) return false;
	
	std::string mesg;
	if ( !panelp->needsApply(mesg) )
	{
		// We don't need to apply anything.
		// We're done.
		return true;
	}

	// Ignore the needs apply message.
	// Try to do the actual apply.
	std::string apply_mesg;
	if ( panelp->apply( apply_mesg ) )
	{
		// Everything worked.  We're done.
		return true;
	}

	// There was a problem doing the actual apply.
	// Inform the user.
	if ( !apply_mesg.empty() )
	{
		LLStringUtil::format_map_t args;
		args["[MESSAGE]"] = apply_mesg;
		gViewerWindow->alertXml("GenericAlert", args);
	}

	return false;
}

// static
void LLPanelGroup::onBtnRefresh(void* user_data)
{
	LLPanelGroup* self = static_cast<LLPanelGroup*>(user_data);
	self->refreshData();
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
	if (mCurrentTab)
	{
		std::string mesg;
		childSetEnabled("btn_apply", mCurrentTab->needsApply(mesg));
	}

}

void LLPanelGroup::refreshData()
{
	LLGroupMgr::getInstance()->clearGroupData(getID());
	mCurrentTab->activate();

	// 5 second timeout
	childDisable("btn_refresh");
	mRefreshTimer.start();
	mRefreshTimer.setTimerExpirySec(5);
}

void LLPanelGroup::close()
{
	// Pass this to the parent, if it is a floater.
	LLView* viewp = getParent();
	LLFloater* floaterp = dynamic_cast<LLFloater*>(viewp);
	if (floaterp)
	{
		// First, set the force close flag, since the floater
		// will be asking us whether it can close.
		mForceClose = TRUE;
		// Tell the parent floater to close.
		floaterp->close();
	}
}

void LLPanelGroup::showNotice(const std::string& subject,
							  const std::string& message,
							  const bool& has_inventory,
							  const std::string& inventory_name,
							  LLOfferInfo* inventory_offer)
{
	if (mCurrentTab->getName() != "notices_tab")
	{
		// We need to clean up that inventory offer.
		if (inventory_offer)
		{
			inventory_offer_callback( IOR_DECLINE , inventory_offer); 
		}
		return;
	}

	LLPanelGroupNotices* notices = static_cast<LLPanelGroupNotices*>(mCurrentTab);

	notices->showNotice(subject,message,has_inventory,inventory_name,inventory_offer);
}
