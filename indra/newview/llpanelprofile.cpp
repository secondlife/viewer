/**
* @file llpanelprofile.cpp
* @brief Profile panel implementation
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2010, Linden Research, Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation;
* version 2.1 of the License only.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*
* Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
* $/LicenseInfo$
*/

#include "llviewerprecompiledheaders.h"
#include "llpanelprofile.h"

// Common
#include "llavatarnamecache.h"
#include "llsdutil.h"
#include "llslurl.h"
#include "lldateutil.h" //ageFromDate

// UI
#include "llavatariconctrl.h"
#include "llclipboard.h"
#include "llcheckboxctrl.h"
#include "lllineeditor.h"
#include "llloadingindicator.h"
#include "llmenubutton.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "llgrouplist.h"

// Image
#include "llimagej2c.h"

// Newview
#include "llagent.h" //gAgent
#include "llagentpicksinfo.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llcommandhandler.h"
#include "llfloaterreg.h"
#include "llfilepicker.h"
#include "llfirstuse.h"
#include "llgroupactions.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llpanelblockedlist.h"
#include "llpanelprofileclassifieds.h"
#include "llpanelprofilepicks.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewermenu.h" //is_agent_mappable
#include "llviewermenufile.h"
#include "llviewertexturelist.h"
#include "llvoiceclient.h"
#include "llweb.h"


static LLPanelInjector<LLPanelProfileSecondLife> t_panel_profile_secondlife("panel_profile_secondlife");
static LLPanelInjector<LLPanelProfileWeb> t_panel_web("panel_profile_web");
static LLPanelInjector<LLPanelProfilePicks> t_panel_picks("panel_profile_picks");
static LLPanelInjector<LLPanelProfileFirstLife> t_panel_firstlife("panel_profile_firstlife");
static LLPanelInjector<LLPanelProfileNotes> t_panel_notes("panel_profile_notes");
static LLPanelInjector<LLPanelProfile>          t_panel_profile("panel_profile");

static const std::string PANEL_SECONDLIFE   = "panel_profile_secondlife";
static const std::string PANEL_WEB          = "panel_profile_web";
static const std::string PANEL_PICKS        = "panel_profile_picks";
static const std::string PANEL_CLASSIFIEDS  = "panel_profile_classifieds";
static const std::string PANEL_FIRSTLIFE    = "panel_profile_firstlife";
static const std::string PANEL_NOTES        = "panel_profile_notes";
static const std::string PANEL_PROFILE_VIEW = "panel_profile_view";

static const std::string PROFILE_PROPERTIES_CAP = "AgentProfile";
static const std::string PROFILE_IMAGE_UPLOAD_CAP = "UploadAgentProfileImage";


//////////////////////////////////////////////////////////////////////////

void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("request_avatar_properties_coro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    httpOpts->setFollowRedirects(true);

    std::string finalUrl = cap_url + "/" + agent_id.asString();

    LLSD result = httpAdapter->getAndSuspend(httpRequest, finalUrl, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status
        || !result.has("id")
        || agent_id != result["id"].asUUID())
    {
        LL_WARNS("AvatarProperties") << "Failed to get agent information for id " << agent_id << LL_ENDL;
        return;
    }

    LLFloater* floater_profile = LLFloaterReg::findInstance("profile", LLSD().with("id", agent_id));
    if (!floater_profile)
    {
        // floater is dead, so panels are dead as well
        return;
    }

    LLPanel *panel = floater_profile->findChild<LLPanel>(PANEL_PROFILE_VIEW, TRUE);
    LLPanelProfile *panel_profile = dynamic_cast<LLPanelProfile*>(panel);
    if (!panel_profile)
    {
        LL_WARNS() << PANEL_PROFILE_VIEW << " not found" << LL_ENDL;
        return;
    }


    // Avatar Data

    LLAvatarData *avatar_data = &panel_profile->mAvatarData;
    std::string birth_date;

    avatar_data->agent_id = agent_id;
    avatar_data->avatar_id = agent_id;
    avatar_data->image_id = result["sl_image_id"].asUUID();
    avatar_data->fl_image_id = result["fl_image_id"].asUUID();
    avatar_data->partner_id = result["partner_id"].asUUID();
    avatar_data->about_text = result["sl_about_text"].asString();
    // Todo: new descriptio size is 65536, check if it actually fits or has scroll
    avatar_data->fl_about_text = result["fl_about_text"].asString();
    avatar_data->born_on = result["member_since"].asDate();
    avatar_data->profile_url = getProfileURL(agent_id.asString());

    avatar_data->flags = 0;

    if (result["online"].asBoolean())
    {
        avatar_data->flags |= AVATAR_ONLINE;
    }
    if (result["allow_publish"].asBoolean())
    {
        avatar_data->flags |= AVATAR_ALLOW_PUBLISH;
    }

    avatar_data->caption_index = 0;
    if (result.has("charter_member")) // won't be present if "caption" is set
    {
        avatar_data->caption_index = result["charter_member"].asInteger();
    }
    else if (result.has("caption"))
    {
        avatar_data->caption_text = result["caption"].asString();
    }

    panel = floater_profile->findChild<LLPanel>(PANEL_SECONDLIFE, TRUE);
    LLPanelProfileSecondLife *panel_sl = dynamic_cast<LLPanelProfileSecondLife*>(panel);
    if (panel_sl)
    {
        panel_sl->processProfileProperties(avatar_data);
    }

    panel = floater_profile->findChild<LLPanel>(PANEL_WEB, TRUE);
    LLPanelProfileWeb *panel_web = dynamic_cast<LLPanelProfileWeb*>(panel);
    if (panel_web)
    {
        panel_web->updateButtons();
    }

    panel = floater_profile->findChild<LLPanel>(PANEL_FIRSTLIFE, TRUE);
    LLPanelProfileFirstLife *panel_first = dynamic_cast<LLPanelProfileFirstLife*>(panel);
    if (panel_first)
    {
        panel_first->mCurrentDescription = avatar_data->fl_about_text;
        panel_first->mDescriptionEdit->setValue(panel_first->mCurrentDescription);
        panel_first->mPicture->setValue(avatar_data->fl_image_id);
        panel_first->updateButtons();
    }

    // Picks

    LLSD picks_array = result["picks"];
    LLAvatarPicks avatar_picks;
    avatar_picks.agent_id = agent_id; // Not in use?
    avatar_picks.target_id = agent_id;

    for (LLSD::array_const_iterator it = picks_array.beginArray(); it != picks_array.endArray(); ++it)
    {
        const LLSD& pick_data = *it;
        avatar_picks.picks_list.emplace_back(pick_data["id"].asUUID(), pick_data["name"].asString());
    }

    panel = floater_profile->findChild<LLPanel>(PANEL_PICKS, TRUE);
    LLPanelProfilePicks *panel_picks = dynamic_cast<LLPanelProfilePicks*>(panel);
    if (panel_picks)
    {
        panel_picks->processProperties(&avatar_picks);
    }

    // Groups

    LLSD groups_array = result["groups"];
    LLAvatarGroups avatar_groups;
    avatar_groups.agent_id = agent_id; // Not in use?
    avatar_groups.avatar_id = agent_id; // target_id

    for (LLSD::array_const_iterator it = groups_array.beginArray(); it != groups_array.endArray(); ++it)
    {
        const LLSD& group_info = *it;
        LLAvatarGroups::LLGroupData group_data;
        group_data.group_powers = 0; // Not in use?
        group_data.group_title = group_info["name"].asString(); // Missing data, not in use?
        group_data.group_id = group_info["id"].asUUID();
        group_data.group_name = group_info["name"].asString();
        group_data.group_insignia_id = group_info["image_id"].asUUID();

        avatar_groups.group_list.push_back(group_data);
    }

    if (panel_sl)
    {
        panel_sl->processGroupProperties(&avatar_groups);
    }

    // Notes
    LLAvatarNotes avatar_notes;

    avatar_notes.agent_id = agent_id;
    avatar_notes.target_id = agent_id;
    // Todo: new notes size is 65536, check that field has a scroll
    avatar_notes.notes = result["notes"].asString();

    panel = floater_profile->findChild<LLPanel>(PANEL_NOTES, TRUE);
    LLPanelProfileNotes *panel_notes = dynamic_cast<LLPanelProfileNotes*>(panel);
    if (panel_notes)
    {
        panel_notes->processProperties(&avatar_notes);
    }
}

//TODO: changes take two minutes to propagate!
// Add some storage that holds updated data for two minutes
// for new instances to reuse the data
// Profile data is only relevant to won avatar, but notes
// are for everybody
void put_avatar_properties_coro(std::string cap_url, LLUUID agent_id, LLSD data)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("put_avatar_properties_coro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    httpOpts->setFollowRedirects(true);

    std::string finalUrl = cap_url + "/" + agent_id.asString();

    LLSD result = httpAdapter->putAndSuspend(httpRequest, finalUrl, data, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("AvatarProperties") << "Failed to put agent information for id " << agent_id << LL_ENDL;
        return;
    }
}

LLUUID post_profile_image(std::string cap_url, const LLSD &first_data, std::string path_to_image, LLHandle<LLPanel> *handle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("post_profile_image_coro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t httpHeaders;

    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);
    httpOpts->setFollowRedirects(true);
    
    LLSD result = httpAdapter->postAndSuspend(httpRequest, cap_url, first_data, httpOpts, httpHeaders);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        // todo: notification?
        LL_WARNS("AvatarProperties") << "Failed to get uploader cap " << status.toString() << LL_ENDL;
        return LLUUID::null;
    }
    if (!result.has("uploader"))
    {
        // todo: notification?
        LL_WARNS("AvatarProperties") << "Failed to get uploader cap, response contains no data." << LL_ENDL;
        return LLUUID::null;
    }
    std::string uploader_cap = result["uploader"].asString();
    if (uploader_cap.empty())
    {
        LL_WARNS("AvatarProperties") << "Failed to get uploader cap, cap invalid." << LL_ENDL;
        return LLUUID::null;
    }

    // Upload the image

    LLCore::HttpRequest::ptr_t uploaderhttpRequest(new LLCore::HttpRequest);
    LLCore::HttpHeaders::ptr_t uploaderhttpHeaders(new LLCore::HttpHeaders);
    LLCore::HttpOptions::ptr_t uploaderhttpOpts(new LLCore::HttpOptions);
    S64 length;

    {
        llifstream instream(path_to_image.c_str(), std::iostream::binary | std::iostream::ate);
        if (!instream.is_open())
        {
            LL_WARNS("AvatarProperties") << "Failed to open file " << path_to_image << LL_ENDL;
            return LLUUID::null;
        }
        length = instream.tellg();
    }

    uploaderhttpHeaders->append(HTTP_OUT_HEADER_CONTENT_TYPE, "application/jp2"); // optional
    uploaderhttpHeaders->append(HTTP_OUT_HEADER_CONTENT_LENGTH, llformat("%d", length)); // required!
    uploaderhttpOpts->setFollowRedirects(true);

    result = httpAdapter->postFileAndSuspend(uploaderhttpRequest, uploader_cap, path_to_image, uploaderhttpOpts, uploaderhttpHeaders);

    httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LL_WARNS("AvatarProperties") << result << LL_ENDL;

    if (!status)
    {
        LL_WARNS("AvatarProperties") << "Failed to upload image " << status.toString() << LL_ENDL;
        return LLUUID::null;
    }

    if (result["state"].asString() != "complete")
    {
        if (result.has("message"))
        {
            LL_WARNS("AvatarProperties") << "Failed to upload image, state " << result["state"] << " message: " << result["message"] << LL_ENDL;
        }
        else
        {
            LL_WARNS("AvatarProperties") << "Failed to upload image " << result << LL_ENDL;
        }
        return LLUUID::null;
    }

    return result["new_asset"].asUUID();
}

enum EProfileImageType
{
    PROFILE_IMAGE_SL,
    PROFILE_IMAGE_FL,
};

void post_profile_image_coro(std::string cap_url, EProfileImageType type, std::string path_to_image, LLHandle<LLPanel> *handle)
{
    LLSD data;
    switch (type)
    {
    case PROFILE_IMAGE_SL:
        data["profile-image-asset"] = "sl_image_id";
        break;
    case PROFILE_IMAGE_FL:
        data["profile-image-asset"] = "fl_image_id";
        break;
    }

    LLUUID result = post_profile_image(cap_url, data, path_to_image, handle);

    // reset loading indicator
    switch (type)
    {
    case PROFILE_IMAGE_SL:
        if (!handle->isDead())
        {
            LLPanelProfileSecondLife* panel = static_cast<LLPanelProfileSecondLife*>(handle->get());
            if (result.notNull())
            {
                panel->setProfileImageUploaded(result);
            }
            else
            {
                // failure, just stop progress indicator
                panel->setProfileImageUploading(false);
            }
        }
        break;
    case PROFILE_IMAGE_FL:
        // Todo: refresh the panel
        break;
    }

    // Cleanup
    LLFile::remove(path_to_image);
    delete handle;
}

void launch_profile_image_coro(EProfileImageType type, const std::string &file_path, LLHandle<LLPanel> *handle)
{
    std::string cap_url = gAgent.getRegionCapability(PROFILE_IMAGE_UPLOAD_CAP);
    if (!cap_url.empty())
    {
        // todo: createUploadFile needs to be done when user picks up a file,
        // not when user clicks 'ok', but coroutine should happen on 'ok'.
        // but this waits for a UI update, the main point is a functional coroutine
        std::string temp_file = gDirUtilp->getTempFilename();
        U32 codec = LLImageBase::getCodecFromExtension(gDirUtilp->getExtension(file_path));
        const S32 MAX_DIM = 256;

        if (LLViewerTextureList::createUploadFile(file_path, temp_file, codec, MAX_DIM))
        {
            LLCoros::instance().launch("postAgentUserImageCoro",
                boost::bind(post_profile_image_coro, cap_url, type, temp_file, handle));
        }
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to upload profile image of type " << (S32)PROFILE_IMAGE_SL << ", no cap found" << LL_ENDL;
    }
}

//////////////////////////////////////////////////////////////////////////
// LLProfileHandler

class LLProfileHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLProfileHandler() : LLCommandHandler("profile", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (params.size() < 1) return false;
		std::string agent_name = params[0];
		LL_INFOS() << "Profile, agent_name " << agent_name << LL_ENDL;
		std::string url = getProfileURL(agent_name);
		LLWeb::loadURLInternal(url);

		return true;
	}
};
LLProfileHandler gProfileHandler;


//////////////////////////////////////////////////////////////////////////
// LLAgentHandler

class LLAgentHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLAgentHandler() : LLCommandHandler("agent", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params, const LLSD& query_map,
		LLMediaCtrl* web)
	{
		if (params.size() < 2) return false;
		LLUUID avatar_id;
		if (!avatar_id.set(params[0], FALSE))
		{
			return false;
		}

		const std::string verb = params[1].asString();
		if (verb == "about")
		{
			LLAvatarActions::showProfile(avatar_id);
			return true;
		}

		if (verb == "inspect")
		{
			LLFloaterReg::showInstance("inspect_avatar", LLSD().with("avatar_id", avatar_id));
			return true;
		}

		if (verb == "im")
		{
			LLAvatarActions::startIM(avatar_id);
			return true;
		}

		if (verb == "pay")
		{
			if (!LLUI::getInstance()->mSettingGroups["config"]->getBOOL("EnableAvatarPay"))
			{
				LLNotificationsUtil::add("NoAvatarPay", LLSD(), LLSD(), std::string("SwitchToStandardSkinAndQuit"));
				return true;
			}

			LLAvatarActions::pay(avatar_id);
			return true;
		}

		if (verb == "offerteleport")
		{
			LLAvatarActions::offerTeleport(avatar_id);
			return true;
		}

		if (verb == "requestfriend")
		{
			LLAvatarActions::requestFriendshipDialog(avatar_id);
			return true;
		}

		if (verb == "removefriend")
		{
			LLAvatarActions::removeFriendDialog(avatar_id);
			return true;
		}

		if (verb == "mute")
		{
			if (! LLAvatarActions::isBlocked(avatar_id))
			{
				LLAvatarActions::toggleBlock(avatar_id);
			}
			return true;
		}

		if (verb == "unmute")
		{
			if (LLAvatarActions::isBlocked(avatar_id))
			{
				LLAvatarActions::toggleBlock(avatar_id);
			}
			return true;
		}

		if (verb == "block")
		{
			if (params.size() > 2)
			{
				const std::string object_name = LLURI::unescape(params[2].asString());
				LLMute mute(avatar_id, object_name, LLMute::OBJECT);
				LLMuteList::getInstance()->add(mute);
				LLPanelBlockedList::showPanelAndSelect(mute.mID);
			}
			return true;
		}

		if (verb == "unblock")
		{
			if (params.size() > 2)
			{
				const std::string object_name = params[2].asString();
				LLMute mute(avatar_id, object_name, LLMute::OBJECT);
				LLMuteList::getInstance()->remove(mute);
			}
			return true;
		}
		return false;
	}
};
LLAgentHandler gAgentHandler;



//////////////////////////////////////////////////////////////////////////
// LLPanelProfileSecondLife

LLPanelProfileSecondLife::LLPanelProfileSecondLife()
 : LLPanelProfileTab()
 , mAvatarNameCacheConnection()
 , mWaitingForImageUpload(false)
{
}

LLPanelProfileSecondLife::~LLPanelProfileSecondLife()
{
    if (getAvatarId().notNull())
    {
        LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
    }

    if (LLVoiceClient::instanceExists())
    {
        LLVoiceClient::getInstance()->removeObserver((LLVoiceClientStatusObserver*)this);
    }

    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
}

BOOL LLPanelProfileSecondLife::postBuild()
{
    mGroupList              = getChild<LLGroupList>("group_list");
    mShowInSearchCheckbox   = getChild<LLCheckBoxCtrl>("show_in_search_checkbox");
    mSecondLifePic          = getChild<LLIconCtrl>("2nd_life_pic");
    mSecondLifePicLayout    = getChild<LLPanel>("image_stack");
    mDescriptionEdit        = getChild<LLTextBase>("sl_description_edit");
    mAgentActionMenuButton  = getChild<LLMenuButton>("agent_actions_menu");
    mSaveDescriptionChanges = getChild<LLButton>("save_description_changes");
    mDiscardDescriptionChanges = getChild<LLButton>("discard_description_changes");

    mGroupList->setDoubleClickCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { LLPanelProfileSecondLife::openGroupProfile(); });
    mGroupList->setReturnCallback([this](LLUICtrl*, const LLSD&) { LLPanelProfileSecondLife::openGroupProfile(); });
    mSaveDescriptionChanges->setCommitCallback([this](LLUICtrl*, void*) { onSaveDescriptionChanges(); }, nullptr);
    mDiscardDescriptionChanges->setCommitCallback([this](LLUICtrl*, void*) { onDiscardDescriptionChanges(); }, nullptr);

    return TRUE;
}

void LLPanelProfileSecondLife::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();

    LLUUID avatar_id = getAvatarId();
    LLAvatarPropertiesProcessor::getInstance()->addObserver(avatar_id, this);

    BOOL own_profile = getSelfProfile();

    mGroupList->setShowNone(!own_profile);

    childSetVisible("notes_panel", !own_profile);
    childSetVisible("settings_panel", own_profile);
    childSetVisible("about_buttons_panel", own_profile);
    childSetVisible("permissions_panel", !own_profile);

    if (own_profile && !getEmbedded())
    {
        // Group list control cannot toggle ForAgent loading
        // Less than ideal, but viewing own profile via search is edge case
        mGroupList->enableForAgent(false);
    }

    // Init menu, menu needs to be created in scope of a registar to work correctly.
    LLUICtrl::CommitCallbackRegistry::ScopedRegistrar commit;
    commit.add("Profile.Commit", [this](LLUICtrl*, const LLSD& userdata) { onCommitMenu(userdata); });

    LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable;
    enable.add("Profile.EnableItem", [this](LLUICtrl*, const LLSD& userdata) { return onEnableMenu(userdata); });
    enable.add("Profile.CheckItem", [this](LLUICtrl*, const LLSD& userdata) { return onCheckMenu(userdata); });

    if (own_profile)
    {
        mAgentActionMenuButton->setMenu("menu_profile_self.xml", LLMenuButton::MP_BOTTOM_RIGHT);
    }
    else
    {
        // Todo: use PeopleContextMenu instead?
        mAgentActionMenuButton->setMenu("menu_profile_other.xml", LLMenuButton::MP_BOTTOM_RIGHT);
    }

    mDescriptionEdit->setParseHTML(!own_profile && !getEmbedded());

    LLProfileDropTarget* drop_target = getChild<LLProfileDropTarget>("drop_target");
    drop_target->setVisible(!own_profile);
    drop_target->setEnabled(!own_profile);

    if (!own_profile)
    {
        mVoiceStatus = LLAvatarActions::canCall() && (LLAvatarActions::isFriend(avatar_id) ? LLAvatarTracker::instance().isBuddyOnline(avatar_id) : TRUE);
        drop_target->setAgentID(avatar_id);
        updateOnlineStatus();
    }

    updateButtons();

    mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileSecondLife::onAvatarNameCache, this, _1, _2));
}

// todo:: remove apply
void LLPanelProfileSecondLife::apply(LLAvatarData* data)
{
    if (getIsLoaded() && getSelfProfile())
    {
        // Might be a better idea to accumulate changes in floater
        // instead of sending a request per tab

        LLSD params = LLSDMap();
        // we have an image, check if it is local. Server won't recognize local ids.
        if (data->image_id != mImageAssetId
            && !LLLocalBitmapMgr::getInstance()->isLocal(mImageAssetId))
        {
            params["sl_image_id"] = mImageAssetId;
        }
        if (data->about_text != mDescriptionEdit->getValue().asString())
        {
            params["sl_about_text"] = mDescriptionEdit->getValue().asString();
        }
        if ((bool)data->allow_publish != mShowInSearchCheckbox->getValue().asBoolean())
        {
            params["allow_publish"] = mShowInSearchCheckbox->getValue().asBoolean();
        }
        if (!params.emptyMap())
        {
            std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
            if (!cap_url.empty())
            {
                LLCoros::instance().launch("putAgentUserInfoCoro",
                    boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), params));
            }
            else
            {
                LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
            }
        }
    }
}

void LLPanelProfileSecondLife::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getIsLoading() && avatar_id.notNull() && !(getSelfProfile() && !getEmbedded()))
    {
        setIsLoading();

        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        if (!cap_url.empty())
        {
            LLCoros::instance().launch("requestAgentUserInfoCoro",
                boost::bind(request_avatar_properties_coro, cap_url, avatar_id));
        }
        else
        {
            LL_WARNS() << "Failed to update profile data, no cap found" << LL_ENDL;
        }
    }
}

void LLPanelProfileSecondLife::processProperties(void* data, EAvatarProcessorType type)
{

    if (APT_PROPERTIES == type)
    {
        const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
        if(avatar_data && getAvatarId() == avatar_data->avatar_id)
        {
            processProfileProperties(avatar_data);
        }
    }
}

void LLPanelProfileSecondLife::resetData()
{
    resetLoading();
    getChild<LLUICtrl>("complete_name")->setValue(LLStringUtil::null);
    getChild<LLUICtrl>("register_date")->setValue(LLStringUtil::null);
    getChild<LLUICtrl>("acc_status_text")->setValue(LLStringUtil::null);
    getChild<LLUICtrl>("partner_text")->setValue(LLStringUtil::null);

    // Set default image and 1:1 dimensions for it
    mSecondLifePic->setValue("Generic_Person_Large");
    LLRect imageRect = mSecondLifePicLayout->getRect();
    mSecondLifePicLayout->reshape(imageRect.getHeight(), imageRect.getHeight());

    mDescriptionEdit->setValue(LLStringUtil::null);
    mGroups.clear();
    mGroupList->setGroups(mGroups);
}

void LLPanelProfileSecondLife::processProfileProperties(const LLAvatarData* avatar_data)
{
    LLUUID avatar_id = getAvatarId();
    if (!LLAvatarActions::isFriend(avatar_id) && !getSelfProfile())
    {
        // this is non-friend avatar. Status will be updated from LLAvatarPropertiesProcessor.
        // in LLPanelProfileSecondLife::processOnlineStatus()

        // subscribe observer to get online status. Request will be sent by LLPanelProfileSecondLife itself.
        // do not subscribe for friend avatar because online status can be wrong overridden
        // via LLAvatarData::flags if Preferences: "Only Friends & Groups can see when I am online" is set.
        processOnlineStatus(avatar_data->flags & AVATAR_ONLINE);
    }

    fillCommonData(avatar_data);

    fillPartnerData(avatar_data);

    fillAccountStatus(avatar_data);

    updateButtons();
}

void LLPanelProfileSecondLife::processGroupProperties(const LLAvatarGroups* avatar_groups)
{
    //KC: the group_list ctrl can handle all this for us on our own profile
    if (getSelfProfile() && !getEmbedded())
    {
        return;
    }

    // *NOTE dzaporozhan
    // Group properties may arrive in two callbacks, we need to save them across
    // different calls. We can't do that in textbox as textbox may change the text.

    LLAvatarGroups::group_list_t::const_iterator it = avatar_groups->group_list.begin();
    const LLAvatarGroups::group_list_t::const_iterator it_end = avatar_groups->group_list.end();

    for (; it_end != it; ++it)
    {
        LLAvatarGroups::LLGroupData group_data = *it;
        mGroups[group_data.group_name] = group_data.group_id;
    }

    mGroupList->setGroups(mGroups);
}

void LLPanelProfileSecondLife::openGroupProfile()
{
    LLUUID group_id = mGroupList->getSelectedUUID();
    LLGroupActions::show(group_id);
}

void LLPanelProfileSecondLife::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();
    // Should be possible to get this from AgentProfile capability
    getChild<LLUICtrl>("display_name")->setValue( av_name.getDisplayName() );
    getChild<LLUICtrl>("user_name")->setValue(av_name.getUserName() );
}

void LLPanelProfileSecondLife::setProfileImageUploading(bool loading)
{
    // Todo: loading indicator here
    mWaitingForImageUpload = loading;
}

void LLPanelProfileSecondLife::setProfileImageUploaded(const LLUUID &image_asset_id)
{
    mSecondLifePic->setValue(image_asset_id);

    LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(image_asset_id);
    if (imagep->getFullHeight())
    {
        onImageLoaded(true, imagep);
    }
    else
    {
        imagep->setLoadedCallback(onImageLoaded,
            MAX_DISCARD_LEVEL,
            FALSE,
            FALSE,
            new LLHandle<LLPanel>(getHandle()),
            NULL,
            FALSE);
    }

    mWaitingForImageUpload = false;
    // Todo: reset loading indicator here
}

void LLPanelProfileSecondLife::fillCommonData(const LLAvatarData* avatar_data)
{
    // Refresh avatar id in cache with new info to prevent re-requests
    // and to make sure icons in text will be up to date
    LLAvatarIconIDCache::getInstance()->add(avatar_data->avatar_id, avatar_data->image_id);

    LLStringUtil::format_map_t args;
    args["[AGE]"] = LLDateUtil::ageFromDate( avatar_data->born_on, LLDate::now());
    std::string register_date = getString("AgeFormat", args);
    getChild<LLUICtrl>("user_age")->setValue(register_date );
    mDescriptionEdit->setValue(avatar_data->about_text);
    mImageAssetId = avatar_data->image_id;
    mSecondLifePic->setValue(mImageAssetId);

    // Will be loaded as a LLViewerFetchedTexture::BOOST_UI due to mSecondLifePic
    LLViewerFetchedTexture* imagep = LLViewerTextureManager::getFetchedTexture(avatar_data->image_id);
    if (imagep->getFullHeight())
    {
        onImageLoaded(true, imagep);
    }
    else
    {
        imagep->setLoadedCallback(onImageLoaded,
                                  MAX_DISCARD_LEVEL,
                                  FALSE,
                                  FALSE,
                                  new LLHandle<LLPanel>(getHandle()),
                                  NULL,
                                  FALSE);
    }

    if (getSelfProfile())
    {
        mShowInSearchCheckbox->setValue((BOOL)(avatar_data->flags & AVATAR_ALLOW_PUBLISH));
    }
}

void LLPanelProfileSecondLife::fillPartnerData(const LLAvatarData* avatar_data)
{
    LLTextBox* partner_text_ctrl = getChild<LLTextBox>("partner_link");
    if (avatar_data->partner_id.notNull())
    {
        LLStringUtil::format_map_t args;
        args["[LINK]"] = LLSLURL("agent", avatar_data->partner_id, "inspect").getSLURLString();
        std::string partner_text = getString("partner_text", args);
        partner_text_ctrl->setText(partner_text);
    }
    else
    {
        partner_text_ctrl->setText(getString("no_partner_text"));
    }
}

void LLPanelProfileSecondLife::fillAccountStatus(const LLAvatarData* avatar_data)
{
    LLStringUtil::format_map_t args;
    args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(avatar_data);
    args["[PAYMENTINFO]"] = LLAvatarPropertiesProcessor::paymentInfo(avatar_data);

    std::string caption_text = getString("CaptionTextAcctInfo", args);
    getChild<LLUICtrl>("account_info")->setValue(caption_text);
}

void LLPanelProfileSecondLife::onImageLoaded(BOOL success, LLViewerFetchedTexture *imagep)
{
    LLRect imageRect = mSecondLifePicLayout->getRect();
    if (!success || imagep->getFullWidth() == imagep->getFullHeight())
    {
        mSecondLifePicLayout->reshape(imageRect.getHeight(), imageRect.getHeight());
    }
    else
    {
        // assume 3:4, for sake of firestorm
        mSecondLifePicLayout->reshape(imageRect.getHeight() * 4 / 3, imageRect.getHeight());
    }
}

//static
void LLPanelProfileSecondLife::onImageLoaded(BOOL success,
                                             LLViewerFetchedTexture *src_vi,
                                             LLImageRaw* src,
                                             LLImageRaw* aux_src,
                                             S32 discard_level,
                                             BOOL final,
                                             void* userdata)
{
    if (!userdata) return;

    LLHandle<LLPanel>* handle = (LLHandle<LLPanel>*)userdata;

    if (!handle->isDead())
    {
        LLPanelProfileSecondLife* panel = static_cast<LLPanelProfileSecondLife*>(handle->get());
        if (panel)
        {
            panel->onImageLoaded(success, src_vi);
        }
    }

    if (final || !success)
    {
        delete handle;
    }
}

// virtual, called by LLAvatarTracker
void LLPanelProfileSecondLife::changed(U32 mask)
{
    updateOnlineStatus();
    updateButtons();
}

// virtual, called by LLVoiceClient
void LLPanelProfileSecondLife::onChange(EStatusType status, const std::string &channelURI, bool proximal)
{
    if(status == STATUS_JOINING || status == STATUS_LEFT_CHANNEL)
    {
        return;
    }

    mVoiceStatus = LLAvatarActions::canCall() && (LLAvatarActions::isFriend(getAvatarId()) ? LLAvatarTracker::instance().isBuddyOnline(getAvatarId()) : TRUE);
}

void LLPanelProfileSecondLife::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.notNull())
    {
        if (getAvatarId().notNull())
        {
            LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
        }

        LLPanelProfileTab::setAvatarId(avatar_id);

        if (LLAvatarActions::isFriend(getAvatarId()))
        {
            LLAvatarTracker::instance().addParticularFriendObserver(getAvatarId(), this);
        }
    }
}

bool LLPanelProfileSecondLife::isGrantedToSeeOnlineStatus()
{
    // set text box visible to show online status for non-friends who has not set in Preferences
    // "Only Friends & Groups can see when I am online"
    if (!LLAvatarActions::isFriend(getAvatarId()))
    {
        return true;
    }

    // *NOTE: GRANT_ONLINE_STATUS is always set to false while changing any other status.
    // When avatar disallow me to see her online status processOfflineNotification Message is received by the viewer
    // see comments for ChangeUserRights template message. EXT-453.
    // If GRANT_ONLINE_STATUS flag is changed it will be applied when viewer restarts. EXT-3880
    const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
    return relationship->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS);
}

// method was disabled according to EXT-2022. Re-enabled & improved according to EXT-3880
void LLPanelProfileSecondLife::updateOnlineStatus()
{
    if (!LLAvatarActions::isFriend(getAvatarId())) return;
    // For friend let check if he allowed me to see his status
    const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
    bool online = relationship->isOnline();
    processOnlineStatus(online);
}

void LLPanelProfileSecondLife::processOnlineStatus(bool online)
{
}

void LLPanelProfileSecondLife::updateButtons()
{
    LLPanelProfileTab::updateButtons();

    if (getSelfProfile() && !getEmbedded())
    {
        mShowInSearchCheckbox->setVisible(TRUE);
        mShowInSearchCheckbox->setEnabled(TRUE);
        mDescriptionEdit->setEnabled(TRUE);

        // todo: enable/disble buttons based on text changes
        //mSaveDescriptionChanges->
        //mDiscardDescriptionChanges->
    }
}



class LLProfileImagePicker : public LLFilePickerThread
{
public:
    LLProfileImagePicker(EProfileImageType type, LLHandle<LLPanel> *handle);
    ~LLProfileImagePicker();
    virtual void notify(const std::vector<std::string>& filenames);

private:
    LLHandle<LLPanel> *mHandle;
    EProfileImageType mType;
};

LLProfileImagePicker::LLProfileImagePicker(EProfileImageType type, LLHandle<LLPanel> *handle)
    : LLFilePickerThread(LLFilePicker::FFLOAD_IMAGE),
    mHandle(handle),
    mType(type)
{
}

LLProfileImagePicker::~LLProfileImagePicker()
{
    delete mHandle;
}

void LLProfileImagePicker::notify(const std::vector<std::string>& filenames)
{
    if (mHandle->isDead())
    {
        return;
    }
    std::string file_path = filenames[0];
    if (file_path.empty())
    {
        return;
    }

    // generate a temp texture file for coroutine
    std::string temp_file = gDirUtilp->getTempFilename();
    U32 codec = LLImageBase::getCodecFromExtension(gDirUtilp->getExtension(file_path));
    const S32 MAX_DIM = 256;
    if (!LLViewerTextureList::createUploadFile(file_path, temp_file, codec, MAX_DIM))
    {
        //todo: image not supported notification
        LL_WARNS("AvatarProperties") << "Failed to upload profile image of type " << (S32)PROFILE_IMAGE_SL << ", failed to open image" << LL_ENDL;
        return;
    }

    std::string cap_url = gAgent.getRegionCapability(PROFILE_IMAGE_UPLOAD_CAP);
    if (cap_url.empty())
    {
        LL_WARNS("AvatarProperties") << "Failed to upload profile image of type " << (S32)PROFILE_IMAGE_SL << ", no cap found" << LL_ENDL;
        return;
    }

    LLPanelProfileSecondLife* panel = static_cast<LLPanelProfileSecondLife*>(mHandle->get());
    panel->setProfileImageUploading(true);

    LLCoros::instance().launch("postAgentUserImageCoro",
        boost::bind(post_profile_image_coro, cap_url, mType, temp_file, mHandle));

    mHandle = nullptr; // transferred to post_profile_image_coro
}

void LLPanelProfileSecondLife::onCommitMenu(const LLSD& userdata)
{
    const std::string item_name = userdata.asString();
    const LLUUID agent_id = getAvatarId();
    // todo: consider moving this into LLAvatarActions::onCommit(name, id)
    // and making all other flaoters, like people menu do the same
    if (item_name == "im")
    {
        LLAvatarActions::startIM(agent_id);
    }
    else if (item_name == "offer_teleport")
    {
        LLAvatarActions::offerTeleport(agent_id);
    }
    else if (item_name == "request_teleport")
    {
        LLAvatarActions::teleportRequest(agent_id);
    }
    else if (item_name == "voice_call")
    {
        LLAvatarActions::startCall(agent_id);
    }
    else if (item_name == "callog")
    {
        LLAvatarActions::viewChatHistory(agent_id);
    }
    else if (item_name == "add_friend")
    {
        LLAvatarActions::requestFriendshipDialog(agent_id);
    }
    else if (item_name == "remove_friend")
    {
        LLAvatarActions::removeFriendDialog(agent_id);
    }
    else if (item_name == "invite_to_group")
    {
        LLAvatarActions::inviteToGroup(agent_id);
    }
    else if (item_name == "can_show_on_map")
    {
        LLAvatarActions::showOnMap(agent_id);
    }
    else if (item_name == "share")
    {
        LLAvatarActions::share(agent_id);
    }
    else if (item_name == "pay")
    {
        LLAvatarActions::pay(agent_id);
    }
    else if (item_name == "toggle_block_agent")
    {
        LLAvatarActions::toggleBlock(agent_id);
    }
    else if (item_name == "copy_user_id")
    {
        LLWString wstr = utf8str_to_wstring(getAvatarId().asString());
        LLClipboard::instance().copyToClipboard(wstr, 0, wstr.size());
    }
    else if (item_name == "copy_display_name"
        || item_name == "copy_username")
    {
        LLAvatarName av_name;
        if (!LLAvatarNameCache::get(getAvatarId(), &av_name))
        {
            // shouldn't happen, option is supposed to be invisible while name is fetching
            LL_WARNS() << "Failed to get agent data" << LL_ENDL;
            return;
        }
        LLWString wstr;
        if (item_name == "copy_display_name")
        {
            wstr = utf8str_to_wstring(av_name.getDisplayName(true));
        }
        else if (item_name == "copy_username")
        {
            wstr = utf8str_to_wstring(av_name.getUserName());
        }
        LLClipboard::instance().copyToClipboard(wstr, 0, wstr.size());
    }
    else if (item_name == "edit_display_name")
    {
        LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileSecondLife::onAvatarNameCacheSetName, this, _1, _2));
        LLFirstUse::setDisplayName(false);
    }
    else if (item_name == "edit_partner")
    {
        // todo: open https://secondlife.com/my/account/partners.php or whatever link is correct for the grid
    }
    else if (item_name == "change_photo")
    {
        (new LLProfileImagePicker(PROFILE_IMAGE_SL, new LLHandle<LLPanel>(getHandle())))->getFile();
    }
    else if (item_name == "remove_photo")
    {
        LLSD params;
        params["sl_image_id"] = LLUUID::null; // todo: verify that it works and matches Generic_Person_Large

        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        if (!cap_url.empty())
        {
            LLCoros::instance().launch("putAgentUserInfoCoro",
                boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), params));

            mSecondLifePic->setValue("Generic_Person_Large");
        }
        else
        {
            LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
        }
    }
}

bool LLPanelProfileSecondLife::onEnableMenu(const LLSD& userdata)
{
    const std::string item_name = userdata.asString();
    const LLUUID agent_id = getAvatarId();
    if (item_name == "offer_teleport" || item_name == "request_teleport")
    {
        return LLAvatarActions::canOfferTeleport(agent_id);
    }
    else if (item_name == "voice_call")
    {
        return mVoiceStatus;
    }
    else if (item_name == "callog")
    {
        return LLLogChat::isTranscriptExist(agent_id);
    }
    else if (item_name == "add_friend")
    {
        return !LLAvatarActions::isFriend(agent_id);
    }
    else if (item_name == "remove_friend")
    {
        return LLAvatarActions::isFriend(agent_id);
    }
    else if (item_name == "can_show_on_map")
    {
        return (LLAvatarTracker::instance().isBuddyOnline(agent_id) && is_agent_mappable(agent_id))
        || gAgent.isGodlike();
    }
    else if (item_name == "toggle_block_agent")
    {
        return LLAvatarActions::canBlock(agent_id);
    }
    else if (item_name == "copy_display_name"
        || item_name == "copy_username")
    {
        return !mAvatarNameCacheConnection.connected();
    }
    else if (item_name == "change_photo")
    {
        std::string cap_url = gAgent.getRegionCapability(PROFILE_IMAGE_UPLOAD_CAP);
        return !cap_url.empty() && !mWaitingForImageUpload;
    }
    else if (item_name == "remove_photo")
    {
        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        return !cap_url.empty() && !mWaitingForImageUpload;
    }

    return false;
}

bool LLPanelProfileSecondLife::onCheckMenu(const LLSD& userdata)
{
    const std::string item_name = userdata.asString();
    const LLUUID agent_id = getAvatarId();
    if (item_name == "toggle_block_agent")
    {
        return LLAvatarActions::isBlocked(agent_id);
    }
    return false;
}

void LLPanelProfileSecondLife::onAvatarNameCacheSetName(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    if (av_name.getDisplayName().empty())
    {
        // something is wrong, tell user to try again later
        LLNotificationsUtil::add("SetDisplayNameFailedGeneric");
        return;
    }

    LL_INFOS("LegacyProfile") << "name-change now " << LLDate::now() << " next_update "
        << LLDate(av_name.mNextUpdate) << LL_ENDL;
    F64 now_secs = LLDate::now().secondsSinceEpoch();

    if (now_secs < av_name.mNextUpdate)
    {
        // if the update time is more than a year in the future, it means updates have been blocked
        // show a more general message
        static const S32 YEAR = 60*60*24*365;
        if (now_secs + YEAR < av_name.mNextUpdate)
        {
            LLNotificationsUtil::add("SetDisplayNameBlocked");
            return;
        }
    }

    LLFloaterReg::showInstance("display_name");
}

void LLPanelProfileSecondLife::onSaveDescriptionChanges()
{
    // todo: force commit changes in mDescriptionEdit, reset dirty flags
    // todo: check if mDescriptionEdit can be made to not commit immediately

    LLSD params;
    params["sl_about_text"] = mDescriptionEdit->getValue().asString();

    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), params));
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }

    updateButtons();
}

void LLPanelProfileSecondLife::onDiscardDescriptionChanges()
{
    // todo: restore mDescriptionEdit

    updateButtons();
}

//////////////////////////////////////////////////////////////////////////
// LLPanelProfileWeb

LLPanelProfileWeb::LLPanelProfileWeb()
 : LLPanelProfileTab()
 , mWebBrowser(NULL)
 , mAvatarNameCacheConnection()
{
}

LLPanelProfileWeb::~LLPanelProfileWeb()
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
}

void LLPanelProfileWeb::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();

    mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileWeb::onAvatarNameCache, this, _1, _2));
}

BOOL LLPanelProfileWeb::postBuild()
{
    mWebBrowser = getChild<LLMediaCtrl>("profile_html");
    mWebBrowser->addObserver(this);
    mWebBrowser->setHomePageUrl("about:blank");

    return TRUE;
}

void LLPanelProfileWeb::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_PROPERTIES == type)
    {
        const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
        if (avatar_data && getAvatarId() == avatar_data->avatar_id)
        {
            updateButtons();
        }
    }
}

void LLPanelProfileWeb::resetData()
{
    mWebBrowser->navigateHome();
}

void LLPanelProfileWeb::apply(LLAvatarData* data)
{

}

void LLPanelProfileWeb::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getIsLoading() && avatar_id.notNull() && !mURLWebProfile.empty())
    {
        setIsLoading();

        mWebBrowser->setVisible(TRUE);
        mPerformanceTimer.start();
        mWebBrowser->navigateTo(mURLWebProfile, HTTP_CONTENT_TEXT_HTML);
    }
}

void LLPanelProfileWeb::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();

    std::string username = av_name.getAccountName();
    if (username.empty())
    {
        username = LLCacheName::buildUsername(av_name.getDisplayName());
    }
    else
    {
        LLStringUtil::replaceChar(username, ' ', '.');
    }

    mURLWebProfile = getProfileURL(username, true);
    if (mURLWebProfile.empty())
    {
        return;
    }

    //if the tab was opened before name was resolved, load the panel now
    updateData();
}

void LLPanelProfileWeb::onCommitLoad(LLUICtrl* ctrl)
{
    if (!mURLHome.empty())
    {
        LLSD::String valstr = ctrl->getValue().asString();
        if (valstr.empty())
        {
            mWebBrowser->setVisible(TRUE);
            mPerformanceTimer.start();
            mWebBrowser->navigateTo( mURLHome, HTTP_CONTENT_TEXT_HTML );
        }
        else if (valstr == "popout")
        {
            // open in viewer's browser, new window
            LLWeb::loadURLInternal(mURLHome);
        }
        else if (valstr == "external")
        {
            // open in external browser
            LLWeb::loadURLExternal(mURLHome);
        }
    }
}

void LLPanelProfileWeb::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
    switch(event)
    {
        case MEDIA_EVENT_STATUS_TEXT_CHANGED:
            childSetValue("status_text", LLSD( self->getStatusText() ) );
        break;

        case MEDIA_EVENT_NAVIGATE_BEGIN:
        {
            if (mFirstNavigate)
            {
                mFirstNavigate = false;
            }
            else
            {
                mPerformanceTimer.start();
            }
        }
        break;

        case MEDIA_EVENT_NAVIGATE_COMPLETE:
        {
            LLStringUtil::format_map_t args;
            args["[TIME]"] = llformat("%.2f", mPerformanceTimer.getElapsedTimeF32());
            childSetValue("status_text", LLSD( getString("LoadTime", args)) );
        }
        break;

        default:
            // Having a default case makes the compiler happy.
        break;
    }
}

void LLPanelProfileWeb::updateButtons()
{
    LLPanelProfileTab::updateButtons();
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelProfileFirstLife::LLPanelProfileFirstLife()
 : LLPanelProfileTab(),
 mIsEditing(false)
{
}

LLPanelProfileFirstLife::~LLPanelProfileFirstLife()
{
}

BOOL LLPanelProfileFirstLife::postBuild()
{
    mDescriptionEdit = getChild<LLTextEditor>("fl_description_edit");
    mPicture = getChild<LLTextureCtrl>("real_world_pic");

    mDescriptionEdit->setFocusReceivedCallback(boost::bind(&LLPanelProfileFirstLife::onDescriptionFocusReceived, this));

    return TRUE;
}

void LLPanelProfileFirstLife::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();
}


void LLPanelProfileFirstLife::onDescriptionFocusReceived()
{
    if (!mIsEditing && getSelfProfile())
    {
        mIsEditing = true;
        mDescriptionEdit->setParseHTML(false);
        mDescriptionEdit->setText(mCurrentDescription);
    }
}

void LLPanelProfileFirstLife::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_PROPERTIES == type)
    {
        const LLAvatarData* avatar_data = static_cast<const LLAvatarData*>(data);
        if (avatar_data && getAvatarId() == avatar_data->avatar_id)
        {
            mCurrentDescription = avatar_data->fl_about_text;
            mDescriptionEdit->setValue(mCurrentDescription);
            mPicture->setValue(avatar_data->fl_image_id);
            updateButtons();
        }
    }
}

void LLPanelProfileFirstLife::processProperties(const LLAvatarData* avatar_data)
{
    mCurrentDescription = avatar_data->fl_about_text;
    mDescriptionEdit->setValue(mCurrentDescription);
    mPicture->setValue(avatar_data->fl_image_id);
    updateButtons();
}

void LLPanelProfileFirstLife::resetData()
{
    mDescriptionEdit->setValue(LLStringUtil::null);
    mPicture->setValue(mPicture->getDefaultImageAssetID());
}

void LLPanelProfileFirstLife::apply(LLAvatarData* data)
{
    LLSD params = LLSDMap();
    if (data->fl_image_id != mPicture->getImageAssetID()
        && !LLLocalBitmapMgr::getInstance()->isLocal(mPicture->getImageAssetID()))
    {
        params["fl_image_id"] = mPicture->getImageAssetID();
    }
    if (data->fl_about_text != mDescriptionEdit->getValue().asString())
    {
        params["fl_about_text"] = mDescriptionEdit->getValue().asString();
    }
    if (!params.emptyMap())
    {
        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        if (getIsLoaded() && !cap_url.empty())
        {
            LLCoros::instance().launch("putAgentUserInfoCoro",
                boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), params));
        }
        else
        {
            LL_WARNS("AvatarProperties") << "Failed to upload profile data " << PROFILE_PROPERTIES_CAP << " cap not found" << LL_ENDL;
        }
    }

    if (data->fl_image_id != mPicture->getImageAssetID()
        && LLLocalBitmapMgr::getInstance()->isLocal(mPicture->getImageAssetID()))
    {
        // todo: temporary file, connect to UI
        std::string file_path = gDirUtilp->findSkinnedFilename("textures", "icons/Default_Outfit_Photo.png");
        launch_profile_image_coro(PROFILE_IMAGE_FL, file_path, new LLHandle<LLPanel>(getHandle()));
    }
}

void LLPanelProfileFirstLife::updateButtons()
{
    LLPanelProfileTab::updateButtons();

    if (getSelfProfile() && !getEmbedded())
    {
        mDescriptionEdit->setEnabled(TRUE);
        mPicture->setEnabled(TRUE);
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelProfileNotes::LLPanelProfileNotes()
: LLPanelProfileTab()
, mAvatarNameCacheConnection()
{

}

LLPanelProfileNotes::~LLPanelProfileNotes()
{
    if (getAvatarId().notNull())
    {
        LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
    }

    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
}

void LLPanelProfileNotes::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getIsLoading() && avatar_id.notNull())
    {
        setIsLoading();

        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        if (!cap_url.empty())
        {
            LLCoros::instance().launch("requestAgentUserInfoCoro",
                boost::bind(request_avatar_properties_coro, cap_url, avatar_id));
        }
    }
}

BOOL LLPanelProfileNotes::postBuild()
{
    mOnlineStatus = getChild<LLCheckBoxCtrl>("status_check");
    mMapRights = getChild<LLCheckBoxCtrl>("map_check");
    mEditObjectRights = getChild<LLCheckBoxCtrl>("objects_check");
    mNotesEditor = getChild<LLTextEditor>("notes_edit");

    mEditObjectRights->setCommitCallback(boost::bind(&LLPanelProfileNotes::onCommitRights, this));

    mNotesEditor->setCommitCallback(boost::bind(&LLPanelProfileNotes::onCommitNotes,this));

    return TRUE;
}

void LLPanelProfileNotes::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();

    fillRightsData();

    mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileNotes::onAvatarNameCache, this, _1, _2));
}

void LLPanelProfileNotes::apply()
{
    onCommitNotes();
    applyRights();
}

void LLPanelProfileNotes::fillRightsData()
{
    mOnlineStatus->setValue(FALSE);
    mMapRights->setValue(FALSE);
    mEditObjectRights->setValue(FALSE);

    const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
    // If true - we are viewing friend's profile, enable check boxes and set values.
    if(relation)
    {
        S32 rights = relation->getRightsGrantedTo();

        mOnlineStatus->setValue(LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE);
        mMapRights->setValue(LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
        mEditObjectRights->setValue(LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);
    }

    enableCheckboxes(NULL != relation);
}

void LLPanelProfileNotes::onCommitNotes()
{
    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (getIsLoaded())
    {
        if (!cap_url.empty())
        {
            std::string notes = mNotesEditor->getValue().asString();
            LLCoros::instance().launch("putAgentUserInfoCoro",
                boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), LLSD().with("notes", notes)));
        }
        else
        {
            LL_WARNS() << "Failed to update notes, no cap found" << LL_ENDL;
        }
    }
}

void LLPanelProfileNotes::rightsConfirmationCallback(const LLSD& notification,
        const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0)
    {
        mEditObjectRights->setValue(mEditObjectRights->getValue().asBoolean() ? FALSE : TRUE);
    }
}

void LLPanelProfileNotes::confirmModifyRights(bool grant)
{
    LLSD args;
    args["NAME"] = LLSLURL("agent", getAvatarId(), "completename").getSLURLString();


    LLNotificationsUtil::add(grant ? "GrantModifyRights" : "RevokeModifyRights", args, LLSD(),
        boost::bind(&LLPanelProfileNotes::rightsConfirmationCallback, this, _1, _2));

}

void LLPanelProfileNotes::onCommitRights()
{
	const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());

	if (!buddy_relationship)
	{
		LL_WARNS("LegacyProfile") << "Trying to modify rights for non-friend avatar. Skipped." << LL_ENDL;
		return;
	}

	bool allow_modify_objects = mEditObjectRights->getValue().asBoolean();

	// if modify objects checkbox clicked
	if (buddy_relationship->isRightGrantedTo(
		LLRelationship::GRANT_MODIFY_OBJECTS) != allow_modify_objects)
	{
		confirmModifyRights(allow_modify_objects);
	}
}

void LLPanelProfileNotes::applyRights()
{
    const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());

    if (!buddy_relationship)
    {
        // Lets have a warning log message instead of having a crash. EXT-4947.
        LL_WARNS("LegacyProfile") << "Trying to modify rights for non-friend avatar. Skipped." << LL_ENDL;
        return;
    }

    S32 rights = 0;

    if (mOnlineStatus->getValue().asBoolean())
    {
        rights |= LLRelationship::GRANT_ONLINE_STATUS;
    }
    if (mMapRights->getValue().asBoolean())
    {
        rights |= LLRelationship::GRANT_MAP_LOCATION;
    }
    if (mEditObjectRights->getValue().asBoolean())
    {
        rights |= LLRelationship::GRANT_MODIFY_OBJECTS;
    }

    LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(getAvatarId(), rights);
}

void LLPanelProfileNotes::processProperties(void* data, EAvatarProcessorType type)
{
    if (APT_NOTES == type)
    {
        LLAvatarNotes* avatar_notes = static_cast<LLAvatarNotes*>(data);
        if (avatar_notes && getAvatarId() == avatar_notes->target_id)
        {
            processProperties(avatar_notes);
            LLAvatarPropertiesProcessor::getInstance()->removeObserver(getAvatarId(),this);
        }
    }
}

void LLPanelProfileNotes::processProperties(LLAvatarNotes* avatar_notes)
{
    mNotesEditor->setValue(avatar_notes->notes);
    mNotesEditor->setEnabled(TRUE);
    updateButtons();
}

void LLPanelProfileNotes::resetData()
{
    resetLoading();
    mNotesEditor->setValue(LLStringUtil::null);
    mOnlineStatus->setValue(FALSE);
    mMapRights->setValue(FALSE);
    mEditObjectRights->setValue(FALSE);

    mURLWebProfile.clear();
}

void LLPanelProfileNotes::enableCheckboxes(bool enable)
{
    mOnlineStatus->setEnabled(enable);
    mMapRights->setEnabled(enable);
    mEditObjectRights->setEnabled(enable);
}

// virtual, called by LLAvatarTracker
void LLPanelProfileNotes::changed(U32 mask)
{
    // update rights to avoid have checkboxes enabled when friendship is terminated. EXT-4947.
    fillRightsData();
}

void LLPanelProfileNotes::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.notNull())
    {
        if (getAvatarId().notNull())
        {
            LLAvatarTracker::instance().removeParticularFriendObserver(getAvatarId(), this);
        }
        LLPanelProfileTab::setAvatarId(avatar_id);
        LLAvatarTracker::instance().addParticularFriendObserver(getAvatarId(), this);
    }
}

void LLPanelProfileNotes::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();

    std::string username = av_name.getAccountName();
    if (username.empty())
    {
        username = LLCacheName::buildUsername(av_name.getDisplayName());
    }
    else
    {
        LLStringUtil::replaceChar(username, ' ', '.');
    }

    mURLWebProfile = getProfileURL(username, false);
}


//////////////////////////////////////////////////////////////////////////
// LLPanelProfile

LLPanelProfile::LLPanelProfile()
 : LLPanelProfileTab()
{
}

LLPanelProfile::~LLPanelProfile()
{
}

BOOL LLPanelProfile::postBuild()
{
    return TRUE;
}

void LLPanelProfile::processProperties(void* data, EAvatarProcessorType type)
{
    //*TODO: figure out what this does
    mTabContainer->setCommitCallback(boost::bind(&LLPanelProfile::onTabChange, this));

    // Load data on currently opened tab as well
    onTabChange();
}

void LLPanelProfile::onTabChange()
{
    LLPanelProfileTab* active_panel = dynamic_cast<LLPanelProfileTab*>(mTabContainer->getCurrentPanel());
    if (active_panel)
    {
        active_panel->updateData();
    }
    updateBtnsVisibility();
}

void LLPanelProfile::updateBtnsVisibility()
{
}

void LLPanelProfile::onOpen(const LLSD& key)
{
    LLUUID avatar_id = key["id"].asUUID();

    // Don't reload the same profile
    if (getAvatarId() == avatar_id)
    {
        return;
    }

    LLPanelProfileTab::onOpen(avatar_id);

    mTabContainer       = getChild<LLTabContainer>("panel_profile_tabs");
    mPanelSecondlife    = findChild<LLPanelProfileSecondLife>(PANEL_SECONDLIFE);
    mPanelWeb           = findChild<LLPanelProfileWeb>(PANEL_WEB);
    mPanelPicks         = findChild<LLPanelProfilePicks>(PANEL_PICKS);
    mPanelClassifieds   = findChild<LLPanelProfileClassifieds>(PANEL_CLASSIFIEDS);
    mPanelFirstlife     = findChild<LLPanelProfileFirstLife>(PANEL_FIRSTLIFE);
    mPanelNotes         = findChild<LLPanelProfileNotes>(PANEL_NOTES);

    mPanelSecondlife->onOpen(avatar_id);
    mPanelWeb->onOpen(avatar_id);
    mPanelPicks->onOpen(avatar_id);
    mPanelClassifieds->onOpen(avatar_id);
    mPanelFirstlife->onOpen(avatar_id);
    mPanelNotes->onOpen(avatar_id);

    mPanelSecondlife->setEmbedded(getEmbedded());
    mPanelWeb->setEmbedded(getEmbedded());
    mPanelPicks->setEmbedded(getEmbedded());
    mPanelClassifieds->setEmbedded(getEmbedded());
    mPanelFirstlife->setEmbedded(getEmbedded());
    mPanelNotes->setEmbedded(getEmbedded());

    // Always request the base profile info
    resetLoading();
    updateData();

    updateBtnsVisibility();

    // KC - Not handling pick and classified opening thru onOpen
    // because this would make unique profile floaters per slurl
    // and result in multiple profile floaters for the same avatar
}

void LLPanelProfile::updateData()
{
    LLUUID avatar_id = getAvatarId();
    // Todo: getIsloading functionality needs to be expanded to
    // include 'inited' or 'data_provided' state to not rerequest
    if (!getIsLoading() && avatar_id.notNull())
    {
        setIsLoading();

        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        if (!cap_url.empty())
        {
            LLCoros::instance().launch("requestAgentUserInfoCoro",
                boost::bind(request_avatar_properties_coro, cap_url, avatar_id));
        }
    }
}

void LLPanelProfile::apply()
{
    if (getSelfProfile())
    {
        //KC - AvatarData is spread over 3 different panels
        // collect data from the last 2 and give to the first to save
        mPanelFirstlife->apply(&mAvatarData);
        mPanelWeb->apply(&mAvatarData);
        mPanelSecondlife->apply(&mAvatarData);

        mPanelPicks->apply();
        mPanelNotes->apply();
        mPanelClassifieds->apply();

        //KC - Classifieds handles this itself
    }
    else
    {
        mPanelNotes->apply();
    }
}

void LLPanelProfile::showPick(const LLUUID& pick_id)
{
    if (pick_id.notNull())
    {
        mPanelPicks->selectPick(pick_id);
    }
    mTabContainer->selectTabPanel(mPanelPicks);
}

bool LLPanelProfile::isPickTabSelected()
{
	return (mTabContainer->getCurrentPanel() == mPanelPicks);
}

bool LLPanelProfile::isNotesTabSelected()
{
	return (mTabContainer->getCurrentPanel() == mPanelNotes);
}

void LLPanelProfile::showClassified(const LLUUID& classified_id, bool edit)
{
    if (classified_id.notNull())
    {
        mPanelClassifieds->selectClassified(classified_id, edit);
    }
    mTabContainer->selectTabPanel(mPanelClassifieds);
}



