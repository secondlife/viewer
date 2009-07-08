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
#include "llavatarconstants.h"
#include "llcallingcard.h"
#include "llcombobox.h"
#include "llfriendactions.h"
#include "llimview.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltooldraganddrop.h"
#include "llscrollcontainer.h"
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

//////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// LLPanelProfileTab()
//-----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
LLPanelProfileTab::LLPanelProfileTab(const LLUUID& avatar_id) 
 : LLPanel()
 , mAvatarId(LLUUID::null)
 , mProfileType(PT_UNKNOWN)
{
	setAvatarId(avatar_id);
}

LLPanelProfileTab::LLPanelProfileTab(const Params& params )
 : LLPanel()
 , mAvatarId(LLUUID::null)
 , mProfileType(PT_UNKNOWN)
{

}

LLPanelProfileTab::~LLPanelProfileTab()
{
	if(mAvatarId.notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void LLPanelProfileTab::setAvatarId(const LLUUID& avatar_id)
{
	if(avatar_id.notNull())
	{
		if(mAvatarId.notNull())
		{
			LLAvatarPropertiesProcessor::getInstance()->removeObserver(mAvatarId,this);
		}
		mAvatarId = avatar_id;
		LLAvatarPropertiesProcessor::getInstance()->addObserver(getAvatarId(),this);
		setProfileType();
	}
}

void LLPanelProfileTab::setProfileType()
{
	mProfileType = (gAgentID == mAvatarId) ? PT_OWN : PT_OTHER;
}

void LLPanelProfileTab::onOpen(const LLSD& key)
{
	onActivate(key);
}

void LLPanelProfileTab::onActivate(const LLUUID& id)
{
	setAvatarId(id);
	scrollToTop();
	updateData();
}

void LLPanelProfileTab::onAddFriend()
{
	if (getAvatarId().notNull())
	{
		std::string name;
		gCacheName->getFullName(getAvatarId(),name);
		LLFriendActions::requestFriendshipDialog(getAvatarId(), name);
	}
}

void LLPanelProfileTab::onIM()
{
	if (getAvatarId().notNull())
	{
		std::string name;
		gCacheName->getFullName(getAvatarId(), name);
		gIMMgr->addSession(name, IM_NOTHING_SPECIAL, getAvatarId());
	}
}

void LLPanelProfileTab::onTeleport()
{
	if(getAvatarId().notNull())
	{
		LLFriendActions::offerTeleport(getAvatarId());
	}
}

void LLPanelProfileTab::scrollToTop()
{
	LLScrollContainer* scrollContainer = getChild<LLScrollContainer>("profile_scroll", FALSE, FALSE);
	if (NULL != scrollContainer)
	{
		scrollContainer->goToTop();
	}
}

//////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
// LLPanelAvatarProfile()
//-----------------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////
LLPanelAvatarProfile::LLPanelAvatarProfile(const LLUUID& avatar_id /* = LLUUID::null */)
 : LLPanelProfileTab(avatar_id), mUpdated(false), mEditMode(false), mStatusCombobox(NULL), mStatusMessage(NULL)
{
	updateData();
}

LLPanelAvatarProfile::LLPanelAvatarProfile(const Params& params )
 : LLPanelProfileTab(params), mUpdated(false), mEditMode(false), mStatusCombobox(NULL), mStatusMessage(NULL)
{
}

LLPanelAvatarProfile::~LLPanelAvatarProfile()
{
	if(getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void* LLPanelAvatarProfile::create(void* data /* = NULL */)
{
	LLSD* id = NULL;
	if(data)
	{
		id = static_cast<LLSD*>(data);
		return new LLPanelAvatarProfile(LLUUID(id->asUUID()));
	}
	return new LLPanelAvatarProfile();
}

void LLPanelAvatarProfile::updateData()
{
	if (getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->sendDataRequest(getAvatarId(),APT_PROPERTIES);
		LLAvatarPropertiesProcessor::getInstance()->sendDataRequest(getAvatarId(),APT_GROUPS);
	}
}

void LLPanelAvatarProfile::processProperties(void* data, EAvatarProcessorType type)
{
	if(APT_PROPERTIES == type)
	{
		const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
		if(avatar_data && getAvatarId() == avatar_data->avatar_id)
		{
			childSetValue("register_date", avatar_data->born_on);
			childSetValue("sl_description_edit", avatar_data->about_text);
			childSetValue("fl_description_edit",avatar_data->fl_about_text);
			childSetValue("2nd_life_pic", avatar_data->image_id);
			childSetValue("1st_life_pic", avatar_data->fl_image_id);
			childSetValue("homepage_edit", avatar_data->profile_url);

			if (!isEditMode())
			{
				setCaptionText(avatar_data);
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
			else
			{
				childSetValue("show_in_search_checkbox", (BOOL)(avatar_data->flags & AVATAR_ALLOW_PUBLISH));
			}


			bool online = avatar_data->flags & AVATAR_ONLINE;
			if(LLFriendActions::isFriend(avatar_data->avatar_id))
			{
				// Online status NO could be because they are hidden
				// If they are a friend, we may know the truth!
				online = LLAvatarTracker::instance().isBuddyOnline(avatar_data->avatar_id);
			}
			childSetValue("online_status", online ?
				"Online" : "Offline");
			childSetColor("online_status", online ? 
				LLColor4::green : LLColor4::red);

			LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
		}
		if (isOwnProfile() && NULL != mStatusCombobox)
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
		if (isOwnProfile())
		{
			std::string full_name;
			gCacheName->getFullName(mAvatarId, full_name);
			childSetValue("user_name", full_name);
		}
	}
	else if(APT_GROUPS == type)
	{
		LLAvatarGroups* avatar_groups = static_cast<LLAvatarGroups*>(data);
		if(avatar_groups)
		{
			std::string groups;
			LLAvatarGroups::group_list_t::const_iterator it = avatar_groups->group_list.begin();
			for(; avatar_groups->group_list.end() != it; ++it)
			{
				LLAvatarGroups::LLGroupData group_data = *it;
				groups += group_data.group_name;
				groups += ", ";
			}
			childSetValue("sl_groups",groups);
		}
	}
}

void LLPanelAvatarProfile::clear()
{
	clearControls();
}

void LLPanelAvatarProfile::clearControls()
{
	childSetValue("2nd_life_pic",LLUUID::null);
	childSetValue("1st_life_pic",LLUUID::null);
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

void LLPanelAvatarProfile::setCaptionText(const LLAvatarData* avatar_data)
{
	std::string caption_text = avatar_data->caption_text;
	if(caption_text.empty())
	{
		LLStringUtil::format_map_t args;
		caption_text = getString("CaptionTextAcctInfo");
		BOOL transacted = (avatar_data->flags & AVATAR_TRANSACTED);
		BOOL identified = (avatar_data->flags & AVATAR_IDENTIFIED);
		BOOL age_verified = (avatar_data->flags & AVATAR_AGEVERIFIED); // Not currently getting set in dataserver/lldataavatar.cpp for privacy considerations

		const char* ACCT_TYPE[] = {
			"AcctTypeResident",
			"AcctTypeTrial",
			"AcctTypeCharterMember",
			"AcctTypeEmployee"
		};
		U8 caption_index = llclamp(avatar_data->caption_index, (U8)0, (U8)(LL_ARRAY_SIZE(ACCT_TYPE)-1));
		args["[ACCTTYPE]"] = getString(ACCT_TYPE[caption_index]);

		std::string payment_text = " ";
		const S32 DEFAULT_CAPTION_LINDEN_INDEX = 3;
		if(caption_index != DEFAULT_CAPTION_LINDEN_INDEX)
		{			
			if(transacted)
			{
				payment_text = "PaymentInfoUsed";
			}
			else if (identified)
			{
				payment_text = "PaymentInfoOnFile";
			}
			else
			{
				payment_text = "NoPaymentInfoOnFile";
			}
			args["[PAYMENTINFO]"] = getString(payment_text);

			std::string age_text = age_verified ? "AgeVerified" : "NotAgeVerified";
			// Do not display age verification status at this time
			//args["[[AGEVERIFICATION]]"] = mPanelSecondLife->getString(age_text);
			args["[AGEVERIFICATION]"] = " ";
		}
		else
		{
			args["[PAYMENTINFO]"] = " ";
			args["[AGEVERIFICATION]"] = " ";
		}
		LLStringUtil::format(caption_text, args);
	}

	childSetValue("acc_status_text", caption_text);
}

void LLPanelAvatarProfile::onAddFriendButtonClick()
{
	onAddFriend();
}

void LLPanelAvatarProfile::onIMButtonClick()
{
	onIM();
}

void LLPanelAvatarProfile::onTeleportButtonClick()
{
	onTeleport();
}

void LLPanelAvatarProfile::onCallButtonClick()
{

}

void LLPanelAvatarProfile::onShareButtonClick()
{

}

/*virtual*/ BOOL LLPanelAvatarProfile::postBuild(void)
{
	mStatusCombobox = getChild<LLComboBox>("status_combo", TRUE, FALSE);
	if (NULL != mStatusCombobox)
	{
		mStatusCombobox->setCommitCallback(boost::bind(&LLPanelAvatarProfile::onStatusChanged, this));
	}
	mStatusMessage =  getChild<LLLineEditor>("status_me_message_edit", TRUE, FALSE);
	if (NULL != mStatusMessage)
	{
		mStatusMessage->setCommitCallback(boost::bind(&LLPanelAvatarProfile::onStatusMessageChanged, this));
	}

	if (!isEditMode())
	{
		childSetActionTextbox("homepage_edit", boost::bind(&LLPanelAvatarProfile::onHomepageTextboxClicked, this));
		childSetActionTextbox("payment_update_link", boost::bind(&LLPanelAvatarProfile::onUpdateAccountTextboxClicked, this));
		childSetActionTextbox("my_account_link", boost::bind(&LLPanelAvatarProfile::onMyAccountTextboxClicked, this));
		childSetActionTextbox("partner_edit_link", boost::bind(&LLPanelAvatarProfile::onPartnerEditTextboxClicked, this));
	}

	childSetCommitCallback("add_friend",(boost::bind(&LLPanelAvatarProfile::onAddFriendButtonClick,this)),NULL);
	childSetCommitCallback("im",(boost::bind(&LLPanelAvatarProfile::onIMButtonClick,this)),NULL);
	childSetCommitCallback("call",(boost::bind(&LLPanelAvatarProfile::onCallButtonClick,this)),NULL);
	childSetCommitCallback("teleport",(boost::bind(&LLPanelAvatarProfile::onTeleportButtonClick,this)),NULL);
	childSetCommitCallback("share",(boost::bind(&LLPanelAvatarProfile::onShareButtonClick,this)),NULL);

	LLTextureCtrl* pic = getChild<LLTextureCtrl>("2nd_life_pic",TRUE,FALSE);
	if(pic)
	{
		pic->setFallbackImageName("default_land_picture.j2c");
	}
	pic = getChild<LLTextureCtrl>("1st_life_pic",TRUE,FALSE);
	if(pic)
	{
		pic->setFallbackImageName("default_land_picture.j2c");
	}

	clearControls();
	updateChildrenList();

	return TRUE;
}

void LLPanelAvatarProfile::onOpen(const LLSD& key)
{
	onActivate(key);
	updateChildrenList();
}


void LLPanelAvatarProfile::updateChildrenList()
{
	if (mUpdated || isEditMode())
	{
		return;
	}
	switch (mProfileType)
	{
	case PT_OWN:
		childSetVisible("user_name", true);
		childSetVisible("status_panel", false);
		childSetVisible("profile_buttons_panel", false);
		childSetVisible("title_groups_text", false);
		childSetVisible("sl_groups", false);
		mUpdated = true;
		childSetVisible("status_me_panel", true);
		childSetVisible("profile_me_buttons_panel", true);

		break;
	case PT_OTHER:
		childSetVisible("user_name", false);
		childSetVisible("status_me_panel", false);
		childSetVisible("profile_me_buttons_panel", false);

		childSetVisible("status_panel", true);
		childSetVisible("profile_buttons_panel", true);
		childSetVisible("title_groups_text", true);
		childSetVisible("sl_groups", true);

		// account actions
		childSetVisible("account_actions_panel", false);
		childSetVisible("partner_edit_link", false);

		//hide for friends
		childSetEnabled("add_friend", !LLFriendActions::isFriend(getAvatarId()));

		//need to update profile view on every activate
		mUpdated = false;
		break;
	case PT_UNKNOWN: break;//do nothing 
	default:
		llassert(false);
	}
}
void LLPanelAvatarProfile::onStatusChanged()
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
	else
	{
	}

}

void LLPanelAvatarProfile::onStatusMessageChanged()
{
	updateData();
}

//static
void LLPanelAvatarProfile::onUrlTextboxClicked(std::string url)
{
	LLWeb::loadURL(url);
}

void LLPanelAvatarProfile::onHomepageTextboxClicked()
{
	onUrlTextboxClicked(childGetValue("homepage_edit").asString());
}

void LLPanelAvatarProfile::onUpdateAccountTextboxClicked()
{
	onUrlTextboxClicked(getString("payment_update_link_url"));
}

void LLPanelAvatarProfile::onMyAccountTextboxClicked()
{
	onUrlTextboxClicked(getString("my_account_link_url"));
}

void LLPanelAvatarProfile::onPartnerEditTextboxClicked()
{
	onUrlTextboxClicked(getString("partner_edit_link_url"));
}

//-----------------------------------------------------------------------------
// LLPanelAvatarNotes()
//-----------------------------------------------------------------------------
LLPanelAvatarNotes::LLPanelAvatarNotes(const LLUUID& id /* = LLUUID::null */)
:LLPanelProfileTab(id)
{
	updateData();
}

LLPanelAvatarNotes::LLPanelAvatarNotes(const Params& params)
: LLPanelProfileTab(params)
{

}

LLPanelAvatarNotes::~LLPanelAvatarNotes()
{
	if(getAvatarId().notNull())
	{
		LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
	}
}

void* LLPanelAvatarNotes::create(void* data)
{
	if(data)
	{
		LLSD* id = static_cast<LLSD*>(data);
		return new LLPanelAvatarNotes(LLUUID(id->asUUID()));
	}
	return new LLPanelAvatarNotes();
}

void LLPanelAvatarNotes::updateData()
{
	LLAvatarPropertiesProcessor::getInstance()->sendDataRequest(getAvatarId(),APT_NOTES);

	const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
	if(relation)
	{
		childSetEnabled("status_check",TRUE);
		childSetEnabled("map_check",TRUE);
		childSetEnabled("objects_check",TRUE);

		S32 rights = relation->getRightsGrantedTo();

		childSetValue("status_check",LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE);
		childSetValue("map_check",LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
		childSetValue("objects_check",LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);
	}
}

BOOL LLPanelAvatarNotes::postBuild()
{
	childSetCommitCallback("status_check",boost::bind(&LLPanelAvatarNotes::onCommitRights,this),NULL);
	childSetCommitCallback("map_check",boost::bind(&LLPanelAvatarNotes::onCommitRights,this),NULL);
	childSetCommitCallback("objects_check",boost::bind(&LLPanelAvatarNotes::onCommitRights,this),NULL);

	childSetCommitCallback("add_friend",(boost::bind(&LLPanelAvatarProfile::onAddFriend,this)),NULL);
	childSetCommitCallback("im",(boost::bind(&LLPanelAvatarProfile::onIM,this)),NULL);
//	childSetCommitCallback("call",(boost::bind(&LLPanelAvatarProfile::onCallButtonClick,this)));
	childSetCommitCallback("teleport",(boost::bind(&LLPanelAvatarProfile::onTeleport,this)),NULL);
//	childSetCommitCallback("share",(boost::bind(&LLPanelAvatarProfile::onShareButtonClick,this)));

	LLTextEditor* te = getChild<LLTextEditor>("notes_edit",TRUE,FALSE);
	if(te) 
	{
		te->setCommitCallback(boost::bind(&LLPanelAvatarNotes::onCommitNotes,this));
		te->setCommitOnFocusLost(TRUE);
	}

	return TRUE;
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

void LLPanelAvatarNotes::clear()
{
	childSetValue("notes_edit",LLStringUtil::null);

	childSetEnabled("status_check",FALSE);
	childSetEnabled("map_check",FALSE);
	childSetEnabled("objects_check",FALSE);
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

void LLPanelAvatarNotes::onActivate(const LLUUID& id)
{
	LLPanelProfileTab::onActivate(id);
	updateChildrenList();
}

void LLPanelAvatarNotes::updateChildrenList()
{
	//hide for friends
	childSetEnabled("add_friend", !LLFriendActions::isFriend(getAvatarId()));
}
