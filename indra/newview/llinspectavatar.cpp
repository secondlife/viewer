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
#include "llavatarnamecache.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "lldateutil.h"
#include "llfloaterreporter.h"
#include "llfloaterworldmap.h"
#include "llimview.h"
#include "llinspect.h"
#include "llmutelist.h"
#include "llpanelblockedlist.h"
#include "llstartup.h"
#include "llspeakers.h"
#include "llviewermenu.h"
#include "llvoiceclient.h"
#include "llviewerobjectlist.h"

// Linden libraries
#include "llfloater.h"
#include "llfloaterreg.h"
#include "llmenubutton.h"
#include "lltooltip.h"	// positionViewNearMouse()
#include "lluictrl.h"

#include "llavatariconctrl.h"

class LLFetchAvatarData;


//////////////////////////////////////////////////////////////////////////////
// LLInspectAvatar
//////////////////////////////////////////////////////////////////////////////

// Avatar Inspector, a small information window used when clicking
// on avatar names in the 2D UI and in the ambient inspector widget for
// the 3D world.
class LLInspectAvatar : public LLInspect
{
	friend class LLFloaterReg;
	
public:
	// avatar_id - Avatar ID for which to show information
	// Inspector will be positioned relative to current mouse position
	LLInspectAvatar(const LLSD& avatar_id);
	virtual ~LLInspectAvatar();
	
	/*virtual*/ BOOL postBuild(void);
	
	// Because floater is single instance, need to re-parse data on each spawn
	// (for example, inspector about same avatar but in different position)
	/*virtual*/ void onOpen(const LLSD& avatar_id);

	// When closing they should close their gear menu 
	/*virtual*/ void onClose(bool app_quitting);
	
	// Update view based on information from avatar properties processor
	void processAvatarData(LLAvatarData* data);
	
	// override the inspector mouse leave so timer is only paused if 
	// gear menu is not open
	/* virtual */ void onMouseLeave(S32 x, S32 y, MASK mask);
	
private:
	// Make network requests for all the data to display in this view.
	// Used on construction and if avatar id changes.
	void requestUpdate();
	
	// Set the volume slider to this user's current client-side volume setting,
	// hiding/disabling if the user is not nearby.
	void updateVolumeSlider();

	// Shows/hides moderator panel depending on voice state 
	void updateModeratorPanel();

	// Moderator ability to enable/disable voice chat for avatar
	void toggleSelectedVoice(bool enabled);
	
	// Button callbacks
	void onClickAddFriend();
	void onClickViewProfile();
	void onClickIM();
	void onClickCall();
	void onClickTeleport();
	void onClickInviteToGroup();
	void onClickPay();
	void onToggleMute();
	void onClickReport();
	void onClickFreeze();
	void onClickEject();
	void onClickZoomIn();  
	void onClickFindOnMap();
	bool onVisibleFindOnMap();
	bool onVisibleFreezeEject();
	bool onVisibleZoomIn();
	void onClickMuteVolume();
	void onVolumeChange(const LLSD& data);
	bool enableMute();
	bool enableUnmute();

	// Is used to determine if "Add friend" option should be enabled in gear menu
	bool isNotFriend();
	
	// Callback for gCacheName to look up avatar name
	void onNameCache(const LLUUID& id,
							 const std::string& name,
							 bool is_group);
	
private:
	LLUUID				mAvatarID;
	// Need avatar name information to spawn friend add request
	std::string			mAvatarName;
	// an in-flight request for avatar properties from LLAvatarPropertiesProcessor
	// is represented by this object
	LLFetchAvatarData*	mPropertiesRequest;
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
:	LLInspect( LLSD() ),	// single_instance, doesn't really need key
	mAvatarID(),			// set in onOpen()  *Note: we used to show partner's name but we dont anymore --angela 3rd Dec* 
	mAvatarName(),
	mPropertiesRequest(NULL)
{
	mCommitCallbackRegistrar.add("InspectAvatar.ViewProfile",	boost::bind(&LLInspectAvatar::onClickViewProfile, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.AddFriend",	boost::bind(&LLInspectAvatar::onClickAddFriend, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.IM",
		boost::bind(&LLInspectAvatar::onClickIM, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Call",		boost::bind(&LLInspectAvatar::onClickCall, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Teleport",	boost::bind(&LLInspectAvatar::onClickTeleport, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.InviteToGroup",	boost::bind(&LLInspectAvatar::onClickInviteToGroup, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Pay",	boost::bind(&LLInspectAvatar::onClickPay, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.ToggleMute",	boost::bind(&LLInspectAvatar::onToggleMute, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Freeze",
		boost::bind(&LLInspectAvatar::onClickFreeze, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Eject",
		boost::bind(&LLInspectAvatar::onClickEject, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.Report",	boost::bind(&LLInspectAvatar::onClickReport, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.FindOnMap",	boost::bind(&LLInspectAvatar::onClickFindOnMap, this));	
	mCommitCallbackRegistrar.add("InspectAvatar.ZoomIn", boost::bind(&LLInspectAvatar::onClickZoomIn, this));
	mCommitCallbackRegistrar.add("InspectAvatar.DisableVoice", boost::bind(&LLInspectAvatar::toggleSelectedVoice, this, false));
	mCommitCallbackRegistrar.add("InspectAvatar.EnableVoice", boost::bind(&LLInspectAvatar::toggleSelectedVoice, this, true));
	mEnableCallbackRegistrar.add("InspectAvatar.VisibleFindOnMap",	boost::bind(&LLInspectAvatar::onVisibleFindOnMap, this));	
	mEnableCallbackRegistrar.add("InspectAvatar.VisibleFreezeEject",	
		boost::bind(&LLInspectAvatar::onVisibleFreezeEject, this));	
	mEnableCallbackRegistrar.add("InspectAvatar.VisibleZoomIn", 
		boost::bind(&LLInspectAvatar::onVisibleZoomIn, this));
	mEnableCallbackRegistrar.add("InspectAvatar.Gear.Enable", boost::bind(&LLInspectAvatar::isNotFriend, this));
	mEnableCallbackRegistrar.add("InspectAvatar.Gear.EnableCall", boost::bind(&LLAvatarActions::canCall));
	mEnableCallbackRegistrar.add("InspectAvatar.EnableMute", boost::bind(&LLInspectAvatar::enableMute, this));
	mEnableCallbackRegistrar.add("InspectAvatar.EnableUnmute", boost::bind(&LLInspectAvatar::enableUnmute, this));

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


// Multiple calls to showInstance("inspect_avatar", foo) will provide different
// LLSD for foo, which we will catch here.
//virtual
void LLInspectAvatar::onOpen(const LLSD& data)
{
	// Start open animation
	LLInspect::onOpen(data);

	// Extract appropriate avatar id
	mAvatarID = data["avatar_id"];

	BOOL self = mAvatarID == gAgent.getID();
	
	getChild<LLUICtrl>("gear_self_btn")->setVisible(self);
	getChild<LLUICtrl>("gear_btn")->setVisible(!self);

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

	updateModeratorPanel();
}

// virtual
void LLInspectAvatar::onClose(bool app_quitting)
{  
  getChild<LLMenuButton>("gear_btn")->hideMenu();
}	

void LLInspectAvatar::requestUpdate()
{
	// Don't make network requests when spawning from the debug menu at the
	// login screen (which is useful to work on the layout).
	if (mAvatarID.isNull())
	{
		if (LLStartUp::getStartupState() >= STATE_STARTED)
		{
			// once we're running we don't want to show the test floater
			// for bogus LLUUID::null links
			closeFloater();
		}
		return;
	}

	// Clear out old data so it doesn't flash between old and new
	getChild<LLUICtrl>("user_name")->setValue("");
	getChild<LLUICtrl>("user_subtitle")->setValue("");
	getChild<LLUICtrl>("user_details")->setValue("");
	
	// Make a new request for properties
	delete mPropertiesRequest;
	mPropertiesRequest = new LLFetchAvatarData(mAvatarID, this);

	// You can't re-add someone as a friend if they are already your friend
	bool is_friend = LLAvatarTracker::instance().getBuddyInfo(mAvatarID) != NULL;
	bool is_self = (mAvatarID == gAgentID);
	if (is_self)
	{
		getChild<LLUICtrl>("add_friend_btn")->setVisible(false);
		getChild<LLUICtrl>("im_btn")->setVisible(false);
	}
	else if (is_friend)
	{
		getChild<LLUICtrl>("add_friend_btn")->setVisible(false);
		getChild<LLUICtrl>("im_btn")->setVisible(true);
	}
	else
	{
		getChild<LLUICtrl>("add_friend_btn")->setVisible(true);
		getChild<LLUICtrl>("im_btn")->setVisible(false);
	}

	// Use an avatar_icon even though the image id will come down with the
	// avatar properties because the avatar_icon code maintains a cache of icons
	// and this may result in the image being visible sooner.
	// *NOTE: This may generate a duplicate avatar properties request, but that
	// will be suppressed internally in the avatar properties processor.
	
	//remove avatar id from cache to get fresh info
	LLAvatarIconIDCache::getInstance()->remove(mAvatarID);

	childSetValue("avatar_icon", LLSD(mAvatarID) );

	gCacheName->get(mAvatarID, false,
		boost::bind(&LLInspectAvatar::onNameCache,
			this, _1, _2, _3));
}

void LLInspectAvatar::processAvatarData(LLAvatarData* data)
{
	LLStringUtil::format_map_t args;
	args["[BORN_ON]"] = data->born_on;
	args["[AGE]"] = LLDateUtil::ageFromDate(data->born_on, LLDate::now());
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

	// Delete the request object as it has been satisfied
	delete mPropertiesRequest;
	mPropertiesRequest = NULL;
}

// For the avatar inspector, we only want to unpause the fade timer 
// if neither the gear menu or self gear menu are open
void LLInspectAvatar::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLMenuGL* gear_menu = getChild<LLMenuButton>("gear_btn")->getMenu();
	LLMenuGL* gear_menu_self = getChild<LLMenuButton>("gear_self_btn")->getMenu();
	if ( gear_menu && gear_menu->getVisible() &&
		 gear_menu_self && gear_menu_self->getVisible() )
	{
		return;
	}

	if(childHasVisiblePopupMenu())
	{
		return;
	}

	mOpenTimer.unpause();
}

void LLInspectAvatar::updateModeratorPanel()
{
	bool enable_moderator_panel = false;

    if (LLVoiceChannel::getCurrentVoiceChannel() &&
		mAvatarID != gAgent.getID())
    {
		LLUUID session_id = LLVoiceChannel::getCurrentVoiceChannel()->getSessionID();

		if (session_id != LLUUID::null)
		{
			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);

			if (speaker_mgr)
			{
				LLPointer<LLSpeaker> self_speakerp = speaker_mgr->findSpeaker(gAgent.getID());
				LLPointer<LLSpeaker> selected_speakerp = speaker_mgr->findSpeaker(mAvatarID);
				
				if(speaker_mgr->isVoiceActive() && selected_speakerp && 
					selected_speakerp->isInVoiceChannel() &&
					((self_speakerp && self_speakerp->mIsModerator) || gAgent.isGodlike()))
				{
					getChild<LLUICtrl>("enable_voice")->setVisible(selected_speakerp->mModeratorMutedVoice);
					getChild<LLUICtrl>("disable_voice")->setVisible(!selected_speakerp->mModeratorMutedVoice);

					enable_moderator_panel = true;
				}
			}
		}
	}

	if (enable_moderator_panel)
	{
		if (!getChild<LLUICtrl>("moderator_panel")->getVisible())
		{
			getChild<LLUICtrl>("moderator_panel")->setVisible(true);
			// stretch the floater so it can accommodate the moderator panel
			reshape(getRect().getWidth(), getRect().getHeight() + getChild<LLUICtrl>("moderator_panel")->getRect().getHeight());
		}
	}
	else if (getChild<LLUICtrl>("moderator_panel")->getVisible())
	{
		getChild<LLUICtrl>("moderator_panel")->setVisible(false);
		// shrink the inspector floater back to original size
		reshape(getRect().getWidth(), getRect().getHeight() - getChild<LLUICtrl>("moderator_panel")->getRect().getHeight());					
	}
}

void LLInspectAvatar::toggleSelectedVoice(bool enabled)
{
	LLUUID session_id = LLVoiceChannel::getCurrentVoiceChannel()->getSessionID();
	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);

	if (speaker_mgr)
	{
		if (!gAgent.getRegion())
			return;

		std::string url = gAgent.getRegion()->getCapability("ChatSessionRequest");
		LLSD data;
		data["method"] = "mute update";
		data["session-id"] = session_id;
		data["params"] = LLSD::emptyMap();
		data["params"]["agent_id"] = mAvatarID;
		data["params"]["mute_info"] = LLSD::emptyMap();
		// ctrl value represents ability to type, so invert
		data["params"]["mute_info"]["voice"] = !enabled;

		class MuteVoiceResponder : public LLHTTPClient::Responder
		{
		public:
			MuteVoiceResponder(const LLUUID& session_id)
			{
				mSessionID = session_id;
			}

			virtual void error(U32 status, const std::string& reason)
			{
				llwarns << status << ": " << reason << llendl;

				if ( gIMMgr )
				{
					//403 == you're not a mod
					//should be disabled if you're not a moderator
					if ( 403 == status )
					{
						gIMMgr->showSessionEventError(
							"mute",
							"not_a_moderator",
							mSessionID);
					}
					else
					{
						gIMMgr->showSessionEventError(
							"mute",
							"generic",
							mSessionID);
					}
				}
			}

		private:
			LLUUID mSessionID;
		};

		LLHTTPClient::post(
			url,
			data,
			new MuteVoiceResponder(speaker_mgr->getSessionID()));
	}

	closeFloater();

}

void LLInspectAvatar::updateVolumeSlider()
{

	bool voice_enabled = gVoiceClient->getVoiceEnabled(mAvatarID);

	// Do not display volume slider and mute button if it 
	// is ourself or we are not in a voice channel together
	if (!voice_enabled || (mAvatarID == gAgent.getID()))
	{
		getChild<LLUICtrl>("mute_btn")->setVisible(false);
		getChild<LLUICtrl>("volume_slider")->setVisible(false);
	}

	else 
	{
		getChild<LLUICtrl>("mute_btn")->setVisible(true);
		getChild<LLUICtrl>("volume_slider")->setVisible(true);

		// By convention, we only display and toggle voice mutes, not all mutes
		bool is_muted = LLMuteList::getInstance()->
							isMuted(mAvatarID, LLMute::flagVoiceChat);

		LLUICtrl* mute_btn = getChild<LLUICtrl>("mute_btn");

		bool is_linden = LLStringUtil::endsWith(mAvatarName, " Linden");

		mute_btn->setEnabled( !is_linden);
		mute_btn->setValue( is_muted );

		LLUICtrl* volume_slider = getChild<LLUICtrl>("volume_slider");
		volume_slider->setEnabled( !is_muted );

		const F32 DEFAULT_VOLUME = 0.5f;
		F32 volume;
		if (is_muted)
		{
			// it's clearer to display their volume as zero
			volume = 0.f;
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

void LLInspectAvatar::onNameCache(
	const LLUUID& id,
	const std::string& full_name,
	bool is_group)
{
	if (id == mAvatarID)
	{
		mAvatarName = full_name;

		// IDEVO JAMESDEBUG - need to always display a display name
		LLAvatarName av_name;
		if (LLAvatarNameCache::useDisplayNames()
			&& LLAvatarNameCache::get(mAvatarID, &av_name))
		{
			getChild<LLUICtrl>("user_name")->setValue(av_name.mDisplayName);
			getChild<LLUICtrl>("user_slid")->setValue(av_name.mSLID);
		}
		else
		{
			getChild<LLUICtrl>("user_name")->setValue(full_name);
			getChild<LLUICtrl>("user_slid")->setValue(full_name);
		}
	}
}

void LLInspectAvatar::onClickAddFriend()
{
	LLAvatarActions::requestFriendshipDialog(mAvatarID, mAvatarName);
	closeFloater();
}

void LLInspectAvatar::onClickViewProfile()
{
	LLAvatarActions::showProfile(mAvatarID);
	closeFloater();
}

bool LLInspectAvatar::isNotFriend()
{
	return !LLAvatarActions::isFriend(mAvatarID);
}

bool LLInspectAvatar::onVisibleFindOnMap()
{
	return gAgent.isGodlike() || is_agent_mappable(mAvatarID);
}

bool LLInspectAvatar::onVisibleFreezeEject()
{
	return enable_freeze_eject( LLSD(mAvatarID) );
}

bool LLInspectAvatar::onVisibleZoomIn()
{
	return gObjectList.findObject(mAvatarID);
}

void LLInspectAvatar::onClickIM()
{ 
	LLAvatarActions::startIM(mAvatarID);
	closeFloater();
}

void LLInspectAvatar::onClickCall()
{ 
	LLAvatarActions::startCall(mAvatarID);
	closeFloater();
}

void LLInspectAvatar::onClickTeleport()
{
	LLAvatarActions::offerTeleport(mAvatarID);
	closeFloater();
}

void LLInspectAvatar::onClickInviteToGroup()
{
	LLAvatarActions::inviteToGroup(mAvatarID);
	closeFloater();
}

void LLInspectAvatar::onClickPay()
{
	LLAvatarActions::pay(mAvatarID);
	closeFloater();
}

void LLInspectAvatar::onToggleMute()
{
	LLMute mute(mAvatarID, mAvatarName, LLMute::AGENT);

	if (LLMuteList::getInstance()->isMuted(mute.mID, mute.mName))
	{
		LLMuteList::getInstance()->remove(mute);
	}
	else
	{
		LLMuteList::getInstance()->add(mute);
	}

	LLPanelBlockedList::showPanelAndSelect(mute.mID);
	closeFloater();
}

void LLInspectAvatar::onClickReport()
{
	LLFloaterReporter::showFromAvatar(mAvatarID, mAvatarName);
	closeFloater();
}

void LLInspectAvatar::onClickFreeze()
{
	handle_avatar_freeze( LLSD(mAvatarID) );
	closeFloater();
}

void LLInspectAvatar::onClickEject()
{
	handle_avatar_eject( LLSD(mAvatarID) );
	closeFloater();
}

void LLInspectAvatar::onClickZoomIn() 
{
	handle_zoom_to_object(mAvatarID);
	closeFloater();
}

void LLInspectAvatar::onClickFindOnMap()
{
	gFloaterWorldMap->trackAvatar(mAvatarID, mAvatarName);
	LLFloaterReg::showInstance("world_map");
}


bool LLInspectAvatar::enableMute()
{
		bool is_linden = LLStringUtil::endsWith(mAvatarName, " Linden");
		bool is_self = mAvatarID == gAgent.getID();

		if (!is_linden && !is_self && !LLMuteList::getInstance()->isMuted(mAvatarID, mAvatarName))
		{
			return true;
		}
		else
		{
			return false;
		}
}

bool LLInspectAvatar::enableUnmute()
{
		bool is_linden = LLStringUtil::endsWith(mAvatarName, " Linden");
		bool is_self = mAvatarID == gAgent.getID();

		if (!is_linden && !is_self && LLMuteList::getInstance()->isMuted(mAvatarID, mAvatarName))
		{
			return true;
		}
		else
		{
			return false;
		}
}

//////////////////////////////////////////////////////////////////////////////
// LLInspectAvatarUtil
//////////////////////////////////////////////////////////////////////////////
void LLInspectAvatarUtil::registerFloater()
{
	LLFloaterReg::add("inspect_avatar", "inspect_avatar.xml",
					  &LLFloaterReg::build<LLInspectAvatar>);
}
