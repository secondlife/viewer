/** 
 * @file llpanelavatar.cpp
 * @brief LLPanelAvatar and related class implementations
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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
#include "llpanelavatar.h"

#include "llagent.h"
#include "llavataractions.h"
#include "llavatarconstants.h"	// AVATAR_ONLINE
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llimview.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "lltooldraganddrop.h"
#include "llscrollcontainer.h"
#include "llavatariconctrl.h"
#include "llweb.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLDropTarget : public LLView
{
public:
	struct Params : public LLInitParam::Block<Params, LLView::Params>
	{
		Optional<LLUUID> agent_id;
		Params()
		:	agent_id("agent_id")
		{
			mouse_opaque(false);
			follows.flags(FOLLOWS_ALL);
		}
	};

	LLDropTarget(const Params&);
	~LLDropTarget();

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	void setAgentID(const LLUUID &agent_id)		{ mAgentID = agent_id; }
protected:
	LLUUID mAgentID;
};

LLDropTarget::LLDropTarget(const LLDropTarget::Params& p) 
:	LLView(p),
	mAgentID(p.agent_id)
{}

LLDropTarget::~LLDropTarget()
{}

void LLDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "LLDropTarget::doDrop()" << llendl;
}

BOOL LLDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	if(getParent())
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mAgentID, LLUUID::null, drop,
												 cargo_type, cargo_data, accept);

		return TRUE;
	}

	return FALSE;
}

static LLDefaultChildRegistry::Register<LLDropTarget> r("drop_target");

static LLRegisterPanelClassWrapper<LLPanelAvatarProfile> t_panel_profile("panel_profile");
static LLRegisterPanelClassWrapper<LLPanelAvatarMeProfile> t_panel_me_profile("panel_me_profile");
static LLRegisterPanelClassWrapper<LLPanelAvatarNotes> t_panel_notes("panel_notes");

//-----------------------------------------------------------------------------
// LLPanelAvatarNotes()
//-----------------------------------------------------------------------------
LLPanelAvatarNotes::LLPanelAvatarNotes()
: LLPanelProfileTab()
{

}

void LLPanelAvatarNotes::updateData()
{
	LLAvatarPropertiesProcessor::getInstance()->
		sendAvatarNotesRequest(getAvatarId());
}

BOOL LLPanelAvatarNotes::postBuild()
{
	childSetCommitCallback("status_check", boost::bind(&LLPanelAvatarNotes::onCommitRights, this), NULL);
	childSetCommitCallback("map_check", boost::bind(&LLPanelAvatarNotes::onCommitRights, this), NULL);
	childSetCommitCallback("objects_check", boost::bind(&LLPanelAvatarNotes::onCommitRights, this), NULL);

	childSetCommitCallback("add_friend", boost::bind(&LLPanelAvatarNotes::onAddFriendButtonClick, this),NULL);
	childSetCommitCallback("im", boost::bind(&LLPanelAvatarNotes::onIMButtonClick, this), NULL);
	childSetCommitCallback("call", boost::bind(&LLPanelAvatarNotes::onCallButtonClick, this), NULL);
	childSetCommitCallback("teleport", boost::bind(&LLPanelAvatarNotes::onTeleportButtonClick, this), NULL);
	childSetCommitCallback("share", boost::bind(&LLPanelAvatarNotes::onShareButtonClick, this), NULL);

	LLTextEditor* te = getChild<LLTextEditor>("notes_edit");
	te->setCommitCallback(boost::bind(&LLPanelAvatarNotes::onCommitNotes,this));
	te->setCommitOnFocusLost(TRUE);

	resetControls();
	resetData();

	return TRUE;
}

void LLPanelAvatarNotes::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);

	fillRightsData();

	//Disable "Add Friend" button for friends.
	childSetEnabled("add_friend", !LLAvatarActions::isFriend(getAvatarId()));
}

void LLPanelAvatarNotes::fillRightsData()
{
	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	// If true - we are viewing friend's profile, enable check boxes and set values.
	if(relation)
	{
		S32 rights = relation->getRightsGrantedTo();

		childSetValue("status_check",LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE);
		childSetValue("map_check",LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
		childSetValue("objects_check",LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);

		childSetEnabled("status_check",TRUE);
		childSetEnabled("map_check",TRUE);
		childSetEnabled("objects_check",TRUE);
	}
}

void LLPanelAvatarNotes::onCommitNotes()
{
	std::string notes = childGetValue("notes_edit").asString();
	LLAvatarPropertiesProcessor::getInstance()-> sendNotes(getAvatarId(),notes);
}

void LLPanelAvatarNotes::onCommitRights()
{
	S32 rights = 0;

	if(childGetValue("status_check").asBoolean())
		rights |= LLRelationship::GRANT_ONLINE_STATUS;
	if(childGetValue("map_check").asBoolean())
		rights |= LLRelationship::GRANT_MAP_LOCATION;
	if(childGetValue("objects_check").asBoolean())
		rights |= LLRelationship::GRANT_MODIFY_OBJECTS;

	LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(getAvatarId(),rights);
}

void LLPanelAvatarNotes::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_NOTES == type)
	{
		LLAvatarNotes* avatar_notes = static_cast<LLAvatarNotes*>(data);
		if(avatar_notes && getAvatarId() == avatar_notes->target_id)
		{
			childSetValue("notes_edit",avatar_notes->notes);
			childSetEnabled("notes edit", true);

			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
		}
	}
}

void LLPanelAvatarNotes::resetData()
{
	childSetValue("notes_edit",LLStringUtil::null);
	// Default value is TRUE
	childSetValue("status_check", TRUE);
}

void LLPanelAvatarNotes::resetControls()
{
	//Disable "Add Friend" button for friends.
	childSetEnabled("add_friend", TRUE);

	childSetEnabled("status_check",FALSE);
	childSetEnabled("map_check",FALSE);
	childSetEnabled("objects_check",FALSE);
}

void LLPanelAvatarNotes::onAddFriendButtonClick()
{
	LLAvatarActions::requestFriendshipDialog(getAvatarId());
}

void LLPanelAvatarNotes::onIMButtonClick()
{
	LLAvatarActions::startIM(getAvatarId());
}

void LLPanelAvatarNotes::onTeleportButtonClick()
{
	LLAvatarActions::offerTeleport(getAvatarId());
}

void LLPanelAvatarNotes::onCallButtonClick()
{
	LLAvatarActions::startCall(getAvatarId());
}

void LLPanelAvatarNotes::onShareButtonClick()
{
	//*TODO not implemented.
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelProfileTab::LLPanelProfileTab()
: LLPanel()
, mAvatarId(LLUUID::null)
{
}

LLPanelProfileTab::~LLPanelProfileTab()
{
	if(getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void LLPanelProfileTab::setAvatarId(const LLUUID& id)
{
	if(id.notNull())
	{
		if(getAvatarId().notNull())
		{
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId,this);
		}
		mAvatarId = id;
		LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(),this);
	}
}

void LLPanelProfileTab::onOpen(const LLSD& key)
{
	// Don't reset panel if we are opening it for same avatar.
	if(getAvatarId() != key.asUUID())
	{
		resetControls();
		resetData();

		scrollToTop();
	}

	// Update data even if we are viewing same avatar profile as some data might been changed.
	setAvatarId(key.asUUID());
	updateData();
}

void LLPanelProfileTab::scrollToTop()
{
	LLScrollContainer* scrollContainer = findChild<LLScrollContainer>("profile_scroll");
	if (scrollContainer)
		scrollContainer->goToTop();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelAvatarProfile::LLPanelAvatarProfile()
: LLPanelProfileTab()
{
}

BOOL LLPanelAvatarProfile::postBuild()
{
	childSetActionTextbox("homepage_edit", boost::bind(&LLPanelAvatarProfile::onHomepageTextboxClicked, this));
	childSetCommitCallback("add_friend",(boost::bind(&LLPanelAvatarProfile::onAddFriendButtonClick,this)),NULL);
	childSetCommitCallback("im",(boost::bind(&LLPanelAvatarProfile::onIMButtonClick,this)),NULL);
	childSetCommitCallback("call",(boost::bind(&LLPanelAvatarProfile::onCallButtonClick,this)),NULL);
	childSetCommitCallback("teleport",(boost::bind(&LLPanelAvatarProfile::onTeleportButtonClick,this)),NULL);
	childSetCommitCallback("overflow_btn", boost::bind(&LLPanelAvatarProfile::onOverflowButtonClicked, this), NULL);
	childSetCommitCallback("share",(boost::bind(&LLPanelAvatarProfile::onShareButtonClick,this)),NULL);

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Profile.Pay",  boost::bind(&LLPanelAvatarProfile::pay, this));
	registrar.add("Profile.Share", boost::bind(&LLPanelAvatarProfile::share, this));

	mProfileMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_profile_overflow.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	LLTextureCtrl* pic = getChild<LLTextureCtrl>("2nd_life_pic");
	pic->setFallbackImageName("default_profile_picture.j2c");

	pic = getChild<LLTextureCtrl>("real_world_pic");
	pic->setFallbackImageName("default_profile_picture.j2c");

	resetControls();
	resetData();

	return TRUE;
}

void LLPanelAvatarProfile::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);

	mGroups.erase();

	//Disable "Add Friend" button for friends.
	childSetEnabled("add_friend", !LLAvatarActions::isFriend(getAvatarId()));
}

void LLPanelAvatarProfile::updateData()
{
	if (getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->
			sendAvatarPropertiesRequest(getAvatarId());
		LLAvatarPropertiesProcessor::getInstance()->
			sendAvatarGroupsRequest(getAvatarId());
	}
}

void LLPanelAvatarProfile::resetControls()
{
	childSetVisible("status_panel", true);
	childSetVisible("profile_buttons_panel", true);
	childSetVisible("title_groups_text", true);
	childSetVisible("sl_groups", true);
	childSetEnabled("add_friend", true);

	childSetVisible("status_me_panel", false);
	childSetVisible("profile_me_buttons_panel", false);
	childSetVisible("account_actions_panel", false);
}

void LLPanelAvatarProfile::resetData()
{
	mGroups.erase();
	childSetValue("2nd_life_pic",LLUUID::null);
	childSetValue("real_world_pic",LLUUID::null);
	childSetValue("online_status",LLStringUtil::null);
	childSetValue("status_message",LLStringUtil::null);
	childSetValue("sl_description_edit",LLStringUtil::null);
	childSetValue("fl_description_edit",LLStringUtil::null);
	childSetValue("sl_groups",LLStringUtil::null);
	childSetValue("homepage_edit",LLStringUtil::null);
	childSetValue("register_date",LLStringUtil::null);
	childSetValue("acc_status_text",LLStringUtil::null);
	childSetTextArg("partner_text", "[FIRST]", LLStringUtil::null);
	childSetTextArg("partner_text", "[LAST]", LLStringUtil::null);
}

void LLPanelAvatarProfile::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_PROPERTIES == type)
	{
		const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
		if(avatar_data && getAvatarId() == avatar_data->avatar_id)
		{
			processProfileProperties(avatar_data);
		}
	}
	else if(APT_GROUPS == type)
	{
		LLAvatarGroups* avatar_groups = static_cast<LLAvatarGroups*>(data);
		if(avatar_groups && getAvatarId() == avatar_groups->avatar_id)
		{
			processGroupProperties(avatar_groups);
		}
	}
}

void LLPanelAvatarProfile::processProfileProperties(const LLAvatarData* avatar_data)
{
	fillCommonData(avatar_data);

	fillPartnerData(avatar_data);

	fillOnlineStatus(avatar_data);

	fillAccountStatus(avatar_data);
}

void LLPanelAvatarProfile::processGroupProperties(const LLAvatarGroups* avatar_groups)
{
	// *NOTE dzaporozhan
	// Group properties may arrive in two callbacks, we need to save them across
	// different calls. We can't do that in textbox as textbox may change the text.

	std::string groups = mGroups;
	LLAvatarGroups::group_list_t::const_iterator it = avatar_groups->group_list.begin();
	const LLAvatarGroups::group_list_t::const_iterator it_end = avatar_groups->group_list.end();

	if(groups.empty() && it_end != it)
	{
		groups = (*it).group_name;
		++it;
	}
	for(; it_end != it; ++it)
	{
		LLAvatarGroups::LLGroupData group_data = *it;
		groups += ", ";
		groups += group_data.group_name;
	}
	mGroups = groups;
	childSetValue("sl_groups",mGroups);
}

void LLPanelAvatarProfile::fillCommonData(const LLAvatarData* avatar_data)
{
	//remove avatar id from cache to get fresh info
	LLAvatarIconIDCache::getInstance()->remove(avatar_data->avatar_id);


	childSetValue("register_date", avatar_data->born_on );
	childSetValue("sl_description_edit", avatar_data->about_text);
	childSetValue("fl_description_edit",avatar_data->fl_about_text);
	childSetValue("2nd_life_pic", avatar_data->image_id);
	childSetValue("real_world_pic", avatar_data->fl_image_id);
	childSetValue("homepage_edit", avatar_data->profile_url);
}

void LLPanelAvatarProfile::fillPartnerData(const LLAvatarData* avatar_data)
{
	if (avatar_data->partner_id.notNull())
	{
		std::string first, last;
		BOOL found = gCacheName->getName(avatar_data->partner_id, first, last);
		if (found)
		{
			childSetTextArg("partner_text", "[FIRST]", first);
			childSetTextArg("partner_text", "[LAST]", last);
		}
	}
	else
	{
		childSetTextArg("partner_text", "[FIRST]", getString("no_partner_text"));
	}
}

void LLPanelAvatarProfile::fillOnlineStatus(const LLAvatarData* avatar_data)
{
	bool online = avatar_data->flags & AVATAR_ONLINE;
	if(LLAvatarActions::isFriend(avatar_data->avatar_id))
	{
		// Online status NO could be because they are hidden
		// If they are a friend, we may know the truth!
		online = LLAvatarTracker::instance().isBuddyOnline(avatar_data->avatar_id);
	}
	childSetValue("online_status", online ?
		"Online" : "Offline");
	childSetColor("online_status", online ? 
		LLColor4::green : LLColor4::red);
}

void LLPanelAvatarProfile::fillAccountStatus(const LLAvatarData* avatar_data)
{
	LLStringUtil::format_map_t args;
	args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(avatar_data);
	args["[PAYMENTINFO]"] = LLAvatarPropertiesProcessor::paymentInfo(avatar_data);
	// *NOTE: AVATAR_AGEVERIFIED not currently getting set in 
	// dataserver/lldataavatar.cpp for privacy considerations
	args["[AGEVERIFICATION]"] = "";
	std::string caption_text = getString("CaptionTextAcctInfo", args);
	childSetValue("acc_status_text", caption_text);
}

void LLPanelAvatarProfile::pay()
{
	LLAvatarActions::pay(getAvatarId());
}

void LLPanelAvatarProfile::share()
{
	LLAvatarActions::share(getAvatarId());
}

void LLPanelAvatarProfile::onUrlTextboxClicked(const std::string& url)
{
	LLWeb::loadURL(url);
}

void LLPanelAvatarProfile::onHomepageTextboxClicked()
{
	std::string url = childGetValue("homepage_edit").asString();
	if(!url.empty())
	{
		onUrlTextboxClicked(url);
	}
}

void LLPanelAvatarProfile::onAddFriendButtonClick()
{
	LLAvatarActions::requestFriendshipDialog(getAvatarId());
}

void LLPanelAvatarProfile::onIMButtonClick()
{
	LLAvatarActions::startIM(getAvatarId());
}

void LLPanelAvatarProfile::onTeleportButtonClick()
{
	LLAvatarActions::offerTeleport(getAvatarId());
}

void LLPanelAvatarProfile::onCallButtonClick()
{
	LLAvatarActions::startCall(getAvatarId());
}

void LLPanelAvatarProfile::onShareButtonClick()
{
	//*TODO not implemented
}

void LLPanelAvatarProfile::onOverflowButtonClicked()
{
	if (!mProfileMenu->toggleVisibility())
		return;

	LLView* btn = getChild<LLView>("overflow_btn");

	if (mProfileMenu->getButtonRect().isEmpty())
	{
		mProfileMenu->setButtonRect(btn);
	}
	mProfileMenu->updateParent(LLMenuGL::sMenuContainer);

	LLRect rect = btn->getRect();
	LLMenuGL::showPopup(this, mProfileMenu, rect.mRight, rect.mTop);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelAvatarMeProfile::LLPanelAvatarMeProfile()
: LLPanelAvatarProfile()
{
}

BOOL LLPanelAvatarMeProfile::postBuild()
{
	LLPanelAvatarProfile::postBuild();

	mStatusCombobox = getChild<LLComboBox>("status_combo");

	childSetCommitCallback("status_combo", boost::bind(&LLPanelAvatarMeProfile::onStatusChanged, this), NULL);
	childSetCommitCallback("status_me_message_text", boost::bind(&LLPanelAvatarMeProfile::onStatusMessageChanged, this), NULL);

	resetControls();
	resetData();

	return TRUE;
}

void LLPanelAvatarMeProfile::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);
}

void LLPanelAvatarMeProfile::processProfileProperties(const LLAvatarData* avatar_data)
{
	fillCommonData(avatar_data);

	fillPartnerData(avatar_data);

	fillStatusData(avatar_data);

	fillAccountStatus(avatar_data);
}

void LLPanelAvatarMeProfile::fillStatusData(const LLAvatarData* avatar_data)
{
	std::string status;
	if (gAgent.getAFK())
	{
		status = "away";
	}
	else if (gAgent.getBusy())
	{
		status = "busy";
	}
	else
	{
		status = "online";
	}

	mStatusCombobox->setValue(status);
}

void LLPanelAvatarMeProfile::resetControls()
{
	childSetVisible("status_panel", false);
	childSetVisible("profile_buttons_panel", false);
	childSetVisible("title_groups_text", false);
	childSetVisible("sl_groups", false);
	childSetVisible("status_me_panel", true);
	childSetVisible("profile_me_buttons_panel", true);
}

void LLPanelAvatarMeProfile::onStatusChanged()
{
	LLSD::String status = mStatusCombobox->getValue().asString();

	if ("online" == status)
	{
		gAgent.clearAFK();
		gAgent.clearBusy();
	}
	else if ("away" == status)
	{
		gAgent.clearBusy();
		gAgent.setAFK();
	}
	else if ("busy" == status)
	{
		gAgent.clearAFK();
		gAgent.setBusy();
		LLNotifications::instance().add("BusyModeSet");
	}
}

void LLPanelAvatarMeProfile::onStatusMessageChanged()
{
	updateData();
}
