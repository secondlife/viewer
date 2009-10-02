/** 
* @file llpanelprofileview.cpp
* @brief Side tray "Profile View" panel
*
* $LicenseInfo:firstyear=2009&license=viewergpl$
* 
* Copyright (c) 2009, Linden Research, Inc.
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

#include "lluserrelations.h"

#include "llpanelprofileview.h"

#include "llcallingcard.h"
#include "llpanelavatar.h"
#include "llpanelpicks.h"
#include "llpanelprofile.h"
#include "llsidetraypanelcontainer.h"

static LLRegisterPanelClassWrapper<LLPanelProfileView> t_panel_target_profile("panel_profile_view");

static std::string PANEL_NOTES = "panel_notes";
static const std::string PANEL_PROFILE = "panel_profile";

LLPanelProfileView::LLPanelProfileView()
:	LLPanelProfile()
,	mStatusText(NULL)
{
}

LLPanelProfileView::~LLPanelProfileView(void)
{
}

/*virtual*/ 
void LLPanelProfileView::onOpen(const LLSD& key)
{
	LLUUID id;
	if(key.has("id"))
	{
		id = key["id"];
	}
	if(id.notNull() && getAvatarId() != id)
	{
		setAvatarId(id);
	}

	// Update the avatar name.
	gCacheName->get(getAvatarId(), FALSE,
		boost::bind(&LLPanelProfileView::onAvatarNameCached, this, _1, _2, _3, _4));

	// status should only show if viewer has permission to view online/offline. EXT-453 
	mStatusText->setVisible(isGrantedToSeeOnlineStatus());
	updateOnlineStatus();

	LLPanelProfile::onOpen(key);
}

BOOL LLPanelProfileView::postBuild()
{
	LLPanelProfile::postBuild();

	getTabContainer()[PANEL_NOTES] = getChild<LLPanelAvatarNotes>(PANEL_NOTES);
	
	//*TODO remove this, according to style guide we don't use status combobox
	getTabContainer()[PANEL_PROFILE]->childSetVisible("online_me_status_text", FALSE);
	getTabContainer()[PANEL_PROFILE]->childSetVisible("status_combo", FALSE);

	mStatusText = getChild<LLTextBox>("status");

	childSetCommitCallback("back",boost::bind(&LLPanelProfileView::onBackBtnClick,this),NULL);
	
	return TRUE;
}


//private

void LLPanelProfileView::onBackBtnClick()
{
	LLSideTrayPanelContainer* parent = dynamic_cast<LLSideTrayPanelContainer*>(getParent());
	if(parent)
	{
		parent->openPreviousPanel();
	}
}

bool LLPanelProfileView::isGrantedToSeeOnlineStatus()
{
	const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if (NULL == relationship)
		return false;

	// *NOTE: GRANT_ONLINE_STATUS is always set to false while changing any other status.
	// When avatar disallow me to see her online status processOfflineNotification Message is received by the viewer
	// see comments for ChangeUserRights template message. EXT-453.
//	return relationship->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS);
	return true;
}

void LLPanelProfileView::updateOnlineStatus()
{
	const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if (NULL == relationship)
		return;

	bool online = relationship->isOnline();
//	std::string statusName();

	std::string status = getString(online ? "status_online" : "status_offline");

	mStatusText->setValue(status);
}

void LLPanelProfileView::onAvatarNameCached(const LLUUID& id, const std::string& first_name, const std::string& last_name, BOOL is_group)
{
	llassert(getAvatarId() == id);
	getChild<LLTextBox>("user_name", FALSE)->setValue(first_name + " " + last_name);
}

void LLPanelProfileView::togglePanel(LLPanel* panel)
{
	LLPanelProfile::togglePanel(panel);
	if(FALSE == panel->getVisible())
	{
		// LLPanelProfile::togglePanel shows/hides all children,
		// we don't want to display online status for non friends, so re-hide it here
		mStatusText->setVisible(isGrantedToSeeOnlineStatus());
	}
}

// EOF
