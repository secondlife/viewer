/** 
 * @file llinspectavatar.cpp
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

#include "llinspectavatar.h"

// viewer files
#include "llagent.h"
#include "llagentdata.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "lldateutil.h"		// ageFromDate()
#include "llfloaterreporter.h"
#include "llfloaterworldmap.h"
#include "llmutelist.h"
#include "llpanelblockedlist.h"
#include "llviewermenu.h"
#include "llvoiceclient.h"

// Linden libraries
#include "llcontrol.h"	// LLCachedControl
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lltooltip.h"	// positionViewNearMouse()
#include "lluictrl.h"

class LLFetchAvatarData;


//////////////////////////////////////////////////////////////////////////////
// LLInspectAvatar
//////////////////////////////////////////////////////////////////////////////

// Avatar Inspector, a small information window used when clicking
// on avatar names in the 2D UI and in the ambient inspector widget for
// the 3D world.
class LLInspectAvatar : public LLFloater
{
	friend class LLFloaterReg;
	
public:
	// avatar_id - Avatar ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLInspectAvatar(const LLSD& avatar_id);
	virtual ~LLInspectAvatar();
	
	/*virtual*/ BOOL postBuild(void);
	/*virtual*/ void draw();
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same avatar but in different position)
	/*virtual*/ void onOpen(const LLSD& avatar_id);
	
	// Inspectors close themselves when they lose focus
	/*virtual*/ void onFocusLost();
	
	// Update view based on information from avatar properties processor
	void processAvatarData(LLAvatarData* data);
	
private:
	// Make network requests for all the data to display in this view.
	// Used on construction and if avatar id changes.
	void requestUpdate();
	
	// Set the volume slider to this user's current client-side volume setting,
	// hiding/disabling if the user is not nearby.
	void updateVolumeSlider();
	
	// Button callbacks
	void onClickAddFriend();
	void onClickViewProfile();
	void onClickIM();
	void onClickTeleport();
	void onClickInviteToGroup();
	void onClickPay();
	void onClickBlock();
	void onClickReport();
	bool onVisibleFindOnMap();
	bool onVisibleGodMode();
	void onClickMuteVolume();
	void onFindOnMap();
	void onVolumeChange(const LLSD& data);
	
	// Callback for gCacheName to look up avatar name
	void nameUpdatedCallback(
							 const LLUUID& id,
							 const std::string& first,
							 const std::string& last,
							 BOOL is_group);
	
private:
	LLUUID				mAvatarID;
	// Need avatar name information to spawn friend add request
	std::string			mAvatarName;
	LLUUID				mPartnerID;
	// an in-flight request for avatar properties from LLAvatarPropertiesProcessor
	// is represented by this object
	LLFetchAvatarData*	mPropertiesRequest;
	LLFrameTimer		mCloseTimer;
	LLFrameTimer		mOpenTimer;
};

//////////////////////////////////////////////////////////////////////////////
// LLFetchAvatarData
//////////////////////////////////////////////////////////////////////////////

// This object represents a pending request for avatar properties information
class LLFetchAvatarData : public LLAvatarPropertiesObserver
{
public:
	// If the inspector closes it will delete the pending request object, so the
	// inspector pointer will be valid for the lifetime of this object
	LLFetchAvatarData(const LLUUID& avatar_id, LLInspectAvatar* inspector)
	:	mAvatarID(avatar_id),
		mInspector(inspector)
	{
		LLAvatarPropertiesProcessor* processor = 
			LLAvatarPropertiesProcessor::getInstance();
		// register ourselves as an observer
		processor->addObserver(mAvatarID, this);
		// send a request (duplicates will be suppressed inside the avatar
		// properties processor)
		processor->sendAvatarPropertiesRequest(mAvatarID);
	}
	
	~LLFetchAvatarData()
	{
		// remove ourselves as an observer
		LLAvatarPropertiesProcessor::getInstance()->
		removeObserver(mAvatarID, this);
	}
	
	void processProperties(void* data, EAvatarProcessorType type)
	{
		// route the data to the inspector
		if (data
			&& type == APT_PROPERTIES)
		{
			LLAvatarData* avatar_data = static_cast<LLAvatarData*>(data);
			mInspector->processAvatarData(avatar_data);
		}
	}
	
	// Store avatar ID so we can un-register the observer on destruction
	LLUUID mAvatarID;
	LLInspectAvatar* mInspector;
};

LLInspectAvatar::LLInspectAvatar(const LLSD& sd)
:	LLFloater( LLSD() ),	// single_instance, doesn't really need key
	mAvatarID(),			// set in onOpen()
	mPartnerID(),
	mAvatarName(),
	mPropertiesRequest(NULL),
	mCloseTimer()
{
	mCommitCallbackRegistrar.add("InspectAvatar.ViewProfile",	boost::bind(&LLInspectAvatar::onClickViewProfile, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.AddFriend",	boost::bind(&LLInspectAvatar::onClickAddFriend, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.IM",	boost::bind(&LLInspectAvatar::onClickIM, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Teleport",	boost::bind(&LLInspectAvatar::onClickTeleport, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.InviteToGroup",	boost::bind(&LLInspectAvatar::onClickInviteToGroup, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Pay",	boost::bind(&LLInspectAvatar::onClickPay, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Block",	boost::bind(&LLInspectAvatar::onClickBlock, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Report",	boost::bind(&LLInspectAvatar::onClickReport, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.FindOnMap",	boost::bind(&LLInspectAvatar::onFindOnMap, this));	
	mVisibleCallbackRegistrar.add("InspectAvatar.VisibleFindOnMap",	boost::bind(&LLInspectAvatar::onVisibleFindOnMap, this));	
	mVisibleCallbackRegistrar.add("InspectAvatar.VisibleGodMode",	boost::bind(&LLInspectAvatar::onVisibleGodMode, this));	


	// can't make the properties request until the widgets are constructed
	// as it might return immediately, so do it in postBuild.
}

LLInspectAvatar::~LLInspectAvatar()
{
	// clean up any pending requests so they don't call back into a deleted
	// view
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

/*virtual*/
BOOL LLInspectAvatar::postBuild(void)
{
	getChild<LLUICtrl>("add_friend_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickAddFriend, this) );

	getChild<LLUICtrl>("view_profile_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickViewProfile, this) );

	getChild<LLUICtrl>("mute_btn")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onClickMuteVolume, this) );

	getChild<LLUICtrl>("volume_slider")->setCommitCallback(
		boost::bind(&LLInspectAvatar::onVolumeChange, this, _2));

	return TRUE;
}

void LLInspectAvatar::draw()
{
	static LLCachedControl<F32> FADE_TIME(*LLUI::sSettingGroups["config"], "InspectorFadeTime", 1.f);
	if (mOpenTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mOpenTimer.getElapsedTimeF32(), 0.f, FADE_TIME, 0.f, 1.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (alpha == 1.f)
		{
			mOpenTimer.stop();
		}

	}
	else if (mCloseTimer.getStarted())
	{
		F32 alpha = clamp_rescale(mCloseTimer.getElapsedTimeF32(), 0.f, FADE_TIME, 1.f, 0.f);
		LLViewDrawContext context(alpha);
		LLFloater::draw();
		if (mCloseTimer.getElapsedTimeF32() > FADE_TIME)
		{
			closeFloater(false);
		}
	}
	else
	{
		LLFloater::draw();
	}
}


// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectAvatar::onOpen(const LLSD& data)
{
	mCloseTimer.stop();
	mOpenTimer.start();

	// Extract appropriate avatar id
	mAvatarID = data["avatar_id"];
	mPartnerID = LLUUID::null;

	// Position the inspector relative to the mouse cursor
	// Similar to how tooltips are positioned
	// See LLToolTipMgr::createToolTip
	if (data.has("pos"))
	{
		LLUI::positionViewNearMouse(this, data["pos"]["x"].asInteger(), data["pos"]["y"].asInteger());
	}
	else
	{
		LLUI::positionViewNearMouse(this);
	}

	// can't call from constructor as widgets are not built yet
	requestUpdate();

	updateVolumeSlider();
}

//virtual
void LLInspectAvatar::onFocusLost()
{
	// Start closing when we lose focus
	mCloseTimer.start();
	mOpenTimer.stop();
}

void LLInspectAvatar::requestUpdate()
{
	// Don't make network requests when spawning from the debug menu at the
	// login screen (which is useful to work on the layout).
	if (mAvatarID.isNull())
	{
		getChild<LLUICtrl>("user_subtitle")->
			setValue("Test subtitle");
		getChild<LLUICtrl>("user_details")->
			setValue("Test details");
		getChild<LLUICtrl>("user_partner")->
			setValue("Test partner");
		return;
	}

	// Clear out old data so it doesn't flash between old and new
	getChild<LLUICtrl>("user_name")->setValue("");
	getChild<LLUICtrl>("user_subtitle")->setValue("");
	getChild<LLUICtrl>("user_details")->setValue("");
	getChild<LLUICtrl>("user_partner")->setValue("");
	
	// Make a new request for properties
	delete mPropertiesRequest;
	mPropertiesRequest = new LLFetchAvatarData(mAvatarID, this);

	// You can't re-add someone as a friend if they are already your friend
	bool is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;
	bool is_self = (mAvatarID == gAgentID);
	childSetEnabled("add_friend_btn", !is_friend && !is_self);

	// Use an avatar_icon even though the image id will come down with the
	// avatar properties because the avatar_icon code maintains a cache of icons
	// and this may result in the image being visible sooner.
	// *NOTE: This may generate a duplicate avatar properties request, but that
	// will be suppressed internally in the avatar properties processor.
	childSetValue("avatar_icon", LLSD(mAvatarID) );

	gCacheName->get(mAvatarID, FALSE,
		boost::bind(&LLInspectAvatar::nameUpdatedCallback,
			this, _1, _2, _3, _4));
}

void LLInspectAvatar::processAvatarData(LLAvatarData* data)
{
	LLStringUtil::format_map_t args;
	args["[BORN_ON]"] = data->born_on;
	args["[AGE]"] = LLDateUtil::ageFromDate(data->born_on);
	args["[SL_PROFILE]"] = data->about_text;
	args["[RW_PROFILE"] = data->fl_about_text;
	args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(data);
	std::string payment_info = LLAvatarPropertiesProcessor::paymentInfo(data);
	args["[PAYMENTINFO]"] = payment_info;
	args["[COMMA]"] = (payment_info.empty() ? "" : ",");

	std::string subtitle = getString("Subtitle", args);
	getChild<LLUICtrl>("user_subtitle")->setValue( LLSD(subtitle) );
	std::string details = getString("Details", args);
	getChild<LLUICtrl>("user_details")->setValue( LLSD(details) );

	// Look up partner name, if there is one
	mPartnerID = data->partner_id;
	if (mPartnerID.notNull())
	{
		gCacheName->get(mPartnerID, FALSE,
			boost::bind(&LLInspectAvatar::nameUpdatedCallback,
			this, _1, _2, _3, _4));
	}

	// Delete the request object as it has been satisfied
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

void LLInspectAvatar::updateVolumeSlider()
{
	// By convention, we only display and toggle voice mutes, not all mutes
	bool is_muted = LLMuteList::getInstance()->
						isMuted(mAvatarID, LLMute::flagVoiceChat);
	bool voice_enabled = gVoiceClient->getVoiceEnabled(mAvatarID);

	LLUICtrl* mute_btn = getChild<LLUICtrl>("mute_btn");
	mute_btn->setEnabled( voice_enabled );
	mute_btn->setValue( is_muted );

	LLUICtrl* volume_slider = getChild<LLUICtrl>("volume_slider");
	volume_slider->setEnabled( voice_enabled && !is_muted );
	const F32 DEFAULT_VOLUME = 0.5f;
	F32 volume;
	if (is_muted)
	{
		// it's clearer to display their volume as zero
		volume = 0.f;
	}
	else if (!voice_enabled)
	{
		// use nominal value rather than 0
		volume = DEFAULT_VOLUME;
	}
	else
	{
		// actual volume
		volume = gVoiceClient->getUserVolume(mAvatarID);

		// *HACK: Voice client doesn't have any data until user actually
		// says something.
		if (volume == 0.f)
		{
			volume = DEFAULT_VOLUME;
		}
	}
	volume_slider->setValue( (F64)volume );
}

void LLInspectAvatar::onClickMuteVolume()
{
	// By convention, we only display and toggle voice mutes, not all mutes
	LLMuteList* mute_list = LLMuteList::getInstance();
	bool is_muted = mute_list->isMuted(mAvatarID, LLMute::flagVoiceChat);

	LLMute mute(mAvatarID, mAvatarName, LLMute::AGENT);
	if (!is_muted)
	{
		mute_list->add(mute, LLMute::flagVoiceChat);
	}
	else
	{
		mute_list->remove(mute, LLMute::flagVoiceChat);
	}

	updateVolumeSlider();
}

void LLInspectAvatar::onVolumeChange(const LLSD& data)
{
	F32 volume = (F32)data.asReal();
	gVoiceClient->setUserVolume(mAvatarID, volume);
}

void LLInspectAvatar::nameUpdatedCallback(
	const LLUUID& id,
	const std::string& first,
	const std::string& last,
	BOOL is_group)
{
	if (id == mAvatarID)
	{
		mAvatarName = first + " " + last;
		childSetValue("user_name", LLSD(mAvatarName) );
	}
	
	if (id == mPartnerID)
	{
		LLStringUtil::format_map_t args;
		args["[PARTNER]"] = first + " " + last;
		std::string partner = getString("Partner", args);
		getChild<LLUICtrl>("user_partner")->setValue(partner);
	}
	// Otherwise possibly a request for an older inspector, ignore it
}

void LLInspectAvatar::onClickAddFriend()
{
	LLAvatarActions::requestFriendshipDialog(mAvatarID, mAvatarName);
}

void LLInspectAvatar::onClickViewProfile()
{
	// hide inspector when showing profile
	setFocus(FALSE);
	LLAvatarActions::showProfile(mAvatarID);

}

bool LLInspectAvatar::onVisibleFindOnMap()
{
	return gAgent.isGodlike() || is_agent_mappable(mAvatarID);
}

bool LLInspectAvatar::onVisibleGodMode()
{
	return gAgent.isGodlike();
}

void LLInspectAvatar::onClickIM()
{ 
	LLAvatarActions::startIM(mAvatarID);
}

void LLInspectAvatar::onClickTeleport()
{
	LLAvatarActions::offerTeleport(mAvatarID);
}

void LLInspectAvatar::onClickInviteToGroup()
{
	LLAvatarActions::inviteToGroup(mAvatarID);
}

void LLInspectAvatar::onClickPay()
{
	LLAvatarActions::pay(mAvatarID);
}

void LLInspectAvatar::onClickBlock()
{
	LLMute mute(mAvatarID, mAvatarName, LLMute::AGENT);
	LLMuteList::getInstance()->add(mute);
	LLPanelBlockedList::showPanelAndSelect(mute.mID);
}

void LLInspectAvatar::onClickReport()
{
	LLFloaterReporter::showFromObject(mAvatarID);
}


void LLInspectAvatar::onFindOnMap()
{
	gFloaterWorldMap->trackAvatar(mAvatarID, mAvatarName);
	LLFloaterReg::showInstance("world_map");
}

//////////////////////////////////////////////////////////////////////////////
// LLInspectAvatarUtil
//////////////////////////////////////////////////////////////////////////////
void LLInspectAvatarUtil::registerFloater()
{
	LLFloaterReg::add("inspect_avatar", "inspect_avatar.xml",
					  &LLFloaterReg::build<LLInspectAvatar>);
}
