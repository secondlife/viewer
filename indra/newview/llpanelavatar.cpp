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
#include "lldateutil.h"			// ageFromDate()
#include "llimview.h"
#include "llnotificationsutil.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "lltooldraganddrop.h"
#include "llscrollcontainer.h"
#include "llavatariconctrl.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llvoiceclient.h"
#include "llnamebox.h"

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
static LLRegisterPanelClassWrapper<LLPanelMyProfile> t_panel_my_profile("panel_my_profile");
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
	childSetCommitCallback("show_on_map_btn", (boost::bind(
				&LLPanelAvatarNotes::onMapButtonClick, this)), NULL);

	LLTextEditor* te = getChild<LLTextEditor>("notes_edit");
	te->setCommitCallback(boost::bind(&LLPanelAvatarNotes::onCommitNotes,this));
	te->setCommitOnFocusLost(TRUE);

	resetControls();
	resetData();

	gVoiceClient->addObserver((LLVoiceClientStatusObserver*)this);

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
	childSetValue("status_check", FALSE);
	childSetValue("map_check", FALSE);
	childSetValue("objects_check", FALSE);

	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	// If true - we are viewing friend's profile, enable check boxes and set values.
	if(relation)
	{
		S32 rights = relation->getRightsGrantedTo();

		childSetValue("status_check",LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE);
		childSetValue("map_check",LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
		childSetValue("objects_check",LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);

	}

	enableCheckboxes(NULL != relation);
}

void LLPanelAvatarNotes::onCommitNotes()
{
	std::string notes = childGetValue("notes_edit").asString();
	LLAvatarPropertiesProcessor::getInstance()-> sendNotes(getAvatarId(),notes);
}

void LLPanelAvatarNotes::rightsConfirmationCallback(const LLSD& notification,
		const LLSD& response, S32 rights)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if (option == 0)
	{
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(
				getAvatarId(), rights);
	}
	else
	{
		childSetValue("objects_check",
				childGetValue("objects_check").asBoolean() ? FALSE : TRUE);
	}
}

void LLPanelAvatarNotes::confirmModifyRights(bool grant, S32 rights)
{
	std::string first, last;
	LLSD args;
	if (gCacheName->getName(getAvatarId(), first, last))
	{
		args["FIRST_NAME"] = first;
		args["LAST_NAME"] = last;
	}

	if (grant)
	{
		LLNotificationsUtil::add("GrantModifyRights", args, LLSD(),
				boost::bind(&LLPanelAvatarNotes::rightsConfirmationCallback, this,
						_1, _2, rights));
	}
	else
	{
		LLNotificationsUtil::add("RevokeModifyRights", args, LLSD(),
				boost::bind(&LLPanelAvatarNotes::rightsConfirmationCallback, this,
						_1, _2, rights));
	}
}

void LLPanelAvatarNotes::onCommitRights()
{
	const LLRelationship* buddy_relationship =
		LLAvatarTracker::instance().getBuddyInfo(getAvatarId());

	if (NULL == buddy_relationship)
	{
		// Lets have a warning log message instead of having a crash. EXT-4947.
		llwarns << "Trying to modify rights for non-friend avatar. Skipped." << llendl;
		return;
	}


	S32 rights = 0;

	if(childGetValue("status_check").asBoolean())
		rights |= LLRelationship::GRANT_ONLINE_STATUS;
	if(childGetValue("map_check").asBoolean())
		rights |= LLRelationship::GRANT_MAP_LOCATION;
	if(childGetValue("objects_check").asBoolean())
		rights |= LLRelationship::GRANT_MODIFY_OBJECTS;

	bool allow_modify_objects = childGetValue("objects_check").asBoolean();

	// if modify objects checkbox clicked
	if (buddy_relationship->isRightGrantedTo(
			LLRelationship::GRANT_MODIFY_OBJECTS) != allow_modify_objects)
	{
		confirmModifyRights(allow_modify_objects, rights);
	}
	// only one checkbox can trigger commit, so store the rest of rights
	else
	{
		LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(
						getAvatarId(), rights);
	}
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

	enableCheckboxes(false);
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

void LLPanelAvatarNotes::enableCheckboxes(bool enable)
{
	childSetEnabled("status_check", enable);
	childSetEnabled("map_check", enable);
	childSetEnabled("objects_check", enable);
}

LLPanelAvatarNotes::~LLPanelAvatarNotes()
{
	if(getAvatarId().notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
		if(LLVoiceClient::instanceExists())
		{
			LLVoiceClient::getInstance()->removeObserver((LLVoiceClientStatusObserver*)this);
		}
	}
}

// virtual, called by LLAvatarTracker
void LLPanelAvatarNotes::changed(U32 mask)
{
	childSetEnabled("teleport", LLAvatarTracker::instance().isBuddyOnline(getAvatarId()));

	// update rights to avoid have checkboxes enabled when friendship is terminated. EXT-4947.
	fillRightsData();
}

// virtual
void LLPanelAvatarNotes::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}

	childSetEnabled("call", LLVoiceClient::voiceEnabled() && gVoiceClient->voiceWorking());
}

void LLPanelAvatarNotes::setAvatarId(const LLUUID& id)
{
	if(id.notNull())
	{
		if(getAvatarId().notNull())
		{
			LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
		}
		LLPanelProfileTab::setAvatarId(id);
		LLAvatarTracker::instance().addParticularFriendObserver(getAvatarId(), this);
	}
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
	updateButtons();
}

void LLPanelProfileTab::scrollToTop()
{
	LLScrollContainer* scrollContainer = findChild<LLScrollContainer>("profile_scroll");
	if (scrollContainer)
		scrollContainer->goToTop();
}

void LLPanelProfileTab::onMapButtonClick()
{
	LLAvatarActions::showOnMap(getAvatarId());
}

void LLPanelProfileTab::updateButtons()
{
	bool is_buddy_online = LLAvatarTracker::instance().isBuddyOnline(getAvatarId());
	
	if(LLAvatarActions::isFriend(getAvatarId()))
	{
		childSetEnabled("teleport", is_buddy_online);
	}
	else
	{
		childSetEnabled("teleport", true);
	}

	bool enable_map_btn = (is_buddy_online &&
			       is_agent_mappable(getAvatarId()))
		|| gAgent.isGodlike();
	childSetEnabled("show_on_map_btn", enable_map_btn);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool enable_god()
{
	return gAgent.isGodlike();
}

LLPanelAvatarProfile::LLPanelAvatarProfile()
: LLPanelProfileTab()
{
}

BOOL LLPanelAvatarProfile::postBuild()
{
	childSetCommitCallback("add_friend",(boost::bind(&LLPanelAvatarProfile::onAddFriendButtonClick,this)),NULL);
	childSetCommitCallback("im",(boost::bind(&LLPanelAvatarProfile::onIMButtonClick,this)),NULL);
	childSetCommitCallback("call",(boost::bind(&LLPanelAvatarProfile::onCallButtonClick,this)),NULL);
	childSetCommitCallback("teleport",(boost::bind(&LLPanelAvatarProfile::onTeleportButtonClick,this)),NULL);
	childSetCommitCallback("overflow_btn", boost::bind(&LLPanelAvatarProfile::onOverflowButtonClicked, this), NULL);
	childSetCommitCallback("share",(boost::bind(&LLPanelAvatarProfile::onShareButtonClick,this)),NULL);
	childSetCommitCallback("show_on_map_btn", (boost::bind(
			&LLPanelAvatarProfile::onMapButtonClick, this)), NULL);

	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Profile.ShowOnMap",  boost::bind(&LLPanelAvatarProfile::onMapButtonClick, this));
	registrar.add("Profile.Pay",  boost::bind(&LLPanelAvatarProfile::pay, this));
	registrar.add("Profile.Share", boost::bind(&LLPanelAvatarProfile::share, this));
	registrar.add("Profile.BlockUnblock", boost::bind(&LLPanelAvatarProfile::toggleBlock, this));
	registrar.add("Profile.Kick", boost::bind(&LLPanelAvatarProfile::kick, this));
	registrar.add("Profile.Freeze", boost::bind(&LLPanelAvatarProfile::freeze, this));
	registrar.add("Profile.Unfreeze", boost::bind(&LLPanelAvatarProfile::unfreeze, this));
	registrar.add("Profile.CSR", boost::bind(&LLPanelAvatarProfile::csr, this));

	LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable;
	enable.add("Profile.EnableShowOnMap", boost::bind(&LLPanelAvatarProfile::enableShowOnMap, this));
	enable.add("Profile.EnableGod", boost::bind(&enable_god));
	enable.add("Profile.EnableBlock", boost::bind(&LLPanelAvatarProfile::enableBlock, this));
	enable.add("Profile.EnableUnblock", boost::bind(&LLPanelAvatarProfile::enableUnblock, this));

	mProfileMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu>("menu_profile_overflow.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());

	LLTextureCtrl* pic = getChild<LLTextureCtrl>("2nd_life_pic");
	pic->setFallbackImageName("default_profile_picture.j2c");

	pic = getChild<LLTextureCtrl>("real_world_pic");
	pic->setFallbackImageName("default_profile_picture.j2c");

	gVoiceClient->addObserver((LLVoiceClientStatusObserver*)this);

	resetControls();
	resetData();

	return TRUE;
}

void LLPanelAvatarProfile::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);

	mGroups.clear();

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
	mGroups.clear();
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

	fillAccountStatus(avatar_data);
}

void LLPanelAvatarProfile::processGroupProperties(const LLAvatarGroups* avatar_groups)
{
	// *NOTE dzaporozhan
	// Group properties may arrive in two callbacks, we need to save them across
	// different calls. We can't do that in textbox as textbox may change the text.

	LLAvatarGroups::group_list_t::const_iterator it = avatar_groups->group_list.begin();
	const LLAvatarGroups::group_list_t::const_iterator it_end = avatar_groups->group_list.end();

	for(; it_end != it; ++it)
	{
		LLAvatarGroups::LLGroupData group_data = *it;
		mGroups[group_data.group_name] = group_data.group_id;
	}

	// Creating string, containing group list
	std::string groups = "";
	for (group_map_t::iterator it = mGroups.begin(); it != mGroups.end(); ++it)
	{
		if (it != mGroups.begin())
			groups += ", ";

		std::string group_name = LLURI::escape(it->first);
		std::string group_url= it->second.notNull()
				? "[secondlife:///app/group/" + it->second.asString() + "/about " + group_name + "]"
						: getString("no_group_text");

		groups += group_url;
	}

	childSetValue("sl_groups", groups);
}

void LLPanelAvatarProfile::fillCommonData(const LLAvatarData* avatar_data)
{
	//remove avatar id from cache to get fresh info
	LLAvatarIconIDCache::getInstance()->remove(avatar_data->avatar_id);

	LLStringUtil::format_map_t args;
	args["[REG_DATE]"] = avatar_data->born_on;
	args["[AGE]"] = LLDateUtil::ageFromDate( avatar_data->born_on, LLDate::now());
	std::string register_date = getString("RegisterDateFormat", args);
	childSetValue("register_date", register_date );
	childSetValue("sl_description_edit", avatar_data->about_text);
	childSetValue("fl_description_edit",avatar_data->fl_about_text);
	childSetValue("2nd_life_pic", avatar_data->image_id);
	childSetValue("real_world_pic", avatar_data->fl_image_id);
	childSetValue("homepage_edit", avatar_data->profile_url);

	// Hide home page textbox if no page was set to fix "homepage URL appears clickable without URL - EXT-4734"
	childSetVisible("homepage_edit", !avatar_data->profile_url.empty());
}

void LLPanelAvatarProfile::fillPartnerData(const LLAvatarData* avatar_data)
{
	LLNameBox* name_box = getChild<LLNameBox>("partner_text");
	if (avatar_data->partner_id.notNull())
	{
		name_box->setNameID(avatar_data->partner_id, FALSE);
	}
	else
	{
		name_box->setNameID(LLUUID::null, FALSE);
		name_box->setText(getString("no_partner_text"));
	}
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

void LLPanelAvatarProfile::toggleBlock()
{
	LLAvatarActions::toggleBlock(getAvatarId());
}

bool LLPanelAvatarProfile::enableShowOnMap()
{
	bool is_buddy_online = LLAvatarTracker::instance().isBuddyOnline(getAvatarId());

	bool enable_map_btn = (is_buddy_online && is_agent_mappable(getAvatarId()))
		|| gAgent.isGodlike();
	return enable_map_btn;
}

bool LLPanelAvatarProfile::enableBlock()
{
	return LLAvatarActions::canBlock(getAvatarId()) && !LLAvatarActions::isBlocked(getAvatarId());
}

bool LLPanelAvatarProfile::enableUnblock()
{
	return LLAvatarActions::isBlocked(getAvatarId());
}

void LLPanelAvatarProfile::kick()
{
	LLAvatarActions::kick(getAvatarId());
}

void LLPanelAvatarProfile::freeze()
{
	LLAvatarActions::freeze(getAvatarId());
}

void LLPanelAvatarProfile::unfreeze()
{
	LLAvatarActions::unfreeze(getAvatarId());
}

void LLPanelAvatarProfile::csr()
{
	std::string name;
	gCacheName->getFullName(getAvatarId(), name);
	LLAvatarActions::csr(getAvatarId(), name);
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

LLPanelAvatarProfile::~LLPanelAvatarProfile()
{
	if(getAvatarId().notNull())
	{
		LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
		if(LLVoiceClient::instanceExists())
		{
			LLVoiceClient::getInstance()->removeObserver((LLVoiceClientStatusObserver*)this);
		}
	}
}

// virtual, called by LLAvatarTracker
void LLPanelAvatarProfile::changed(U32 mask)
{
	childSetEnabled("teleport", LLAvatarTracker::instance().isBuddyOnline(getAvatarId()));
}

// virtual
void LLPanelAvatarProfile::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
	if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
	{
		return;
	}

	childSetEnabled("call", LLVoiceClient::voiceEnabled() && gVoiceClient->voiceWorking());
}

void LLPanelAvatarProfile::setAvatarId(const LLUUID& id)
{
	if(id.notNull())
	{
		if(getAvatarId().notNull())
		{
			LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
		}
		LLPanelProfileTab::setAvatarId(id);
		LLAvatarTracker::instance().addParticularFriendObserver(getAvatarId(), this);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelMyProfile::LLPanelMyProfile()
: LLPanelAvatarProfile()
{
}

BOOL LLPanelMyProfile::postBuild()
{
	LLPanelAvatarProfile::postBuild();

	mStatusCombobox = getChild<LLComboBox>("status_combo");

	childSetCommitCallback("status_combo", boost::bind(&LLPanelMyProfile::onStatusChanged, this), NULL);
	childSetCommitCallback("status_me_message_text", boost::bind(&LLPanelMyProfile::onStatusMessageChanged, this), NULL);

	resetControls();
	resetData();

	return TRUE;
}

void LLPanelMyProfile::onOpen(const LLSD& key)
{
	LLPanelProfileTab::onOpen(key);
}

void LLPanelMyProfile::processProfileProperties(const LLAvatarData* avatar_data)
{
	fillCommonData(avatar_data);

	fillPartnerData(avatar_data);

	fillStatusData(avatar_data);

	fillAccountStatus(avatar_data);
}

void LLPanelMyProfile::fillStatusData(const LLAvatarData* avatar_data)
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

void LLPanelMyProfile::resetControls()
{
	childSetVisible("status_panel", false);
	childSetVisible("profile_buttons_panel", false);
	childSetVisible("title_groups_text", false);
	childSetVisible("sl_groups", false);
	childSetVisible("status_me_panel", true);
	childSetVisible("profile_me_buttons_panel", true);
}

void LLPanelMyProfile::onStatusChanged()
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
		LLNotificationsUtil::add("BusyModeSet");
	}
}

void LLPanelMyProfile::onStatusMessageChanged()
{
	updateData();
}
