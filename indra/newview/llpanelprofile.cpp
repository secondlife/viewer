/**
* @file llpanelprofile.cpp
* @brief Profile panel implementation
*
* $LicenseInfo:firstyear=2022&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2022, Linden Research, Inc.
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
#include "llcombobox.h"
#include "lllineeditor.h"
#include "llloadingindicator.h"
#include "llmenubutton.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "lltexturectrl.h"
#include "lltoggleablemenu.h"
#include "llgrouplist.h"
#include "llurlaction.h"

// Image
#include "llimagej2c.h"

// Newview
#include "llagent.h" //gAgent
#include "llagentpicksinfo.h"
#include "llavataractions.h"
#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llcommandhandler.h"
#include "llfloaterprofiletexture.h"
#include "llfloaterreg.h"
#include "llfloaterreporter.h"
#include "llfilepicker.h"
#include "llfirstuse.h"
#include "llgroupactions.h"
#include "lllogchat.h"
#include "llmutelist.h"
#include "llnotificationsutil.h"
#include "llpanelblockedlist.h"
#include "llpanelprofileclassifieds.h"
#include "llpanelprofilepicks.h"
#include "llthumbnailctrl.h"
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

    LL_DEBUGS("AvatarProperties") << "Agent id: " << agent_id << " Result: " << httpResults << LL_ENDL;

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
    avatar_data->fl_about_text = result["fl_about_text"].asString();
    avatar_data->born_on = result["member_since"].asDate();
    avatar_data->profile_url = getProfileURL(agent_id.asString());
    avatar_data->customer_type = result["customer_type"].asString();

    avatar_data->flags = 0;

    if (result["online"].asBoolean())
    {
        avatar_data->flags |= AVATAR_ONLINE;
    }
    if (result["allow_publish"].asBoolean())
    {
        avatar_data->flags |= AVATAR_ALLOW_PUBLISH;
    }
    if (result["identified"].asBoolean())
    {
        avatar_data->flags |= AVATAR_IDENTIFIED;
    }
    if (result["transacted"].asBoolean())
    {
        avatar_data->flags |= AVATAR_TRANSACTED;
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
        panel_web->setLoaded();
    }

    panel = floater_profile->findChild<LLPanel>(PANEL_FIRSTLIFE, TRUE);
    LLPanelProfileFirstLife *panel_first = dynamic_cast<LLPanelProfileFirstLife*>(panel);
    if (panel_first)
    {
        panel_first->processProperties(avatar_data);
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
        // Refresh pick limit before processing
        LLAgentPicksInfo::getInstance()->onServerRespond(&avatar_picks);
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
        LL_WARNS("AvatarProperties") << "Failed to put agent information " << data << " for id " << agent_id << LL_ENDL;
        return;
    }

    LL_DEBUGS("AvatarProperties") << "Agent id: " << agent_id << " Data: " << data << " Result: " << httpResults << LL_ENDL;
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

    LL_DEBUGS("AvatarProperties") << result << LL_ENDL;

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
    if (!handle->isDead())
    {
        switch (type)
        {
        case PROFILE_IMAGE_SL:
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
                break;
            }
        case PROFILE_IMAGE_FL:
            {
                LLPanelProfileFirstLife* panel = static_cast<LLPanelProfileFirstLife*>(handle->get());
                if (result.notNull())
                {
                    panel->setProfileImageUploaded(result);
                }
                else
                {
                    // failure, just stop progress indicator
                    panel->setProfileImageUploading(false);
                }
                break;
            }
        }
    }

    // Cleanup
    LLFile::remove(path_to_image);
    delete handle;
}

//////////////////////////////////////////////////////////////////////////
// LLProfileHandler

class LLProfileHandler : public LLCommandHandler
{
public:
	// requires trusted browser to trigger
	LLProfileHandler() : LLCommandHandler("profile", UNTRUSTED_THROTTLE) { }

	bool handle(const LLSD& params,
                const LLSD& query_map,
                const std::string& grid,
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

    virtual bool canHandleUntrusted(
        const LLSD& params,
        const LLSD& query_map,
        LLMediaCtrl* web,
        const std::string& nav_type)
    {
        if (params.size() < 2)
        {
            return true; // don't block, will fail later
        }

        if (nav_type == NAV_TYPE_CLICKED)
        {
            return true;
        }

        const std::string verb = params[1].asString();
        if (verb == "about" || verb == "inspect" || verb == "reportAbuse")
        {
            return true;
        }
        return false;
    }

	bool handle(const LLSD& params,
                const LLSD& query_map,
                const std::string& grid,
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

        // reportAbuse is here due to convoluted avatar handling
        // in LLScrollListCtrl and LLTextBase
        if (verb == "reportAbuse" && web == NULL) 
        {
            LLAvatarName av_name;
            if (LLAvatarNameCache::get(avatar_id, &av_name))
            {
                LLFloaterReporter::showFromAvatar(avatar_id, av_name.getCompleteName());
            }
            else
            {
                LLFloaterReporter::showFromAvatar(avatar_id, "not avaliable");
            }
            return true;
        }
		return false;
	}
};
LLAgentHandler gAgentHandler;


///----------------------------------------------------------------------------
/// LLFloaterProfilePermissions
///----------------------------------------------------------------------------

class LLFloaterProfilePermissions
    : public LLFloater
    , public LLFriendObserver
{
public:
    LLFloaterProfilePermissions(LLView * owner, const LLUUID &avatar_id);
    ~LLFloaterProfilePermissions();
    BOOL postBuild() override;
    void onOpen(const LLSD& key) override;
    void draw() override;
    void changed(U32 mask) override; // LLFriendObserver

    void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);
    bool hasUnsavedChanges() { return mHasUnsavedPermChanges; }

    void onApplyRights();

private:
    void fillRightsData();
    void rightsConfirmationCallback(const LLSD& notification, const LLSD& response);
    void confirmModifyRights(bool grant);
    void onCommitSeeOnlineRights();
    void onCommitEditRights();
    void onCancel();

    LLTextBase*         mDescription;
    LLCheckBoxCtrl*     mOnlineStatus;
    LLCheckBoxCtrl*     mMapRights;
    LLCheckBoxCtrl*     mEditObjectRights;
    LLButton*           mOkBtn;
    LLButton*           mCancelBtn;

    LLUUID              mAvatarID;
    F32                 mContextConeOpacity;
    bool                mHasUnsavedPermChanges;
    LLHandle<LLView>    mOwnerHandle;

    boost::signals2::connection	mAvatarNameCacheConnection;
};

LLFloaterProfilePermissions::LLFloaterProfilePermissions(LLView * owner, const LLUUID &avatar_id)
    : LLFloater(LLSD())
    , mAvatarID(avatar_id)
    , mContextConeOpacity(0.0f)
    , mHasUnsavedPermChanges(false)
    , mOwnerHandle(owner->getHandle())
{
    buildFromFile("floater_profile_permissions.xml");
}

LLFloaterProfilePermissions::~LLFloaterProfilePermissions()
{
    mAvatarNameCacheConnection.disconnect();
    if (mAvatarID.notNull())
    {
        LLAvatarTracker::instance().removeParticularFriendObserver(mAvatarID, this);
    }
}

BOOL LLFloaterProfilePermissions::postBuild()
{
    mDescription = getChild<LLTextBase>("perm_description");
    mOnlineStatus = getChild<LLCheckBoxCtrl>("online_check");
    mMapRights = getChild<LLCheckBoxCtrl>("map_check");
    mEditObjectRights = getChild<LLCheckBoxCtrl>("objects_check");
    mOkBtn = getChild<LLButton>("perms_btn_ok");
    mCancelBtn = getChild<LLButton>("perms_btn_cancel");

    mOnlineStatus->setCommitCallback([this](LLUICtrl*, void*) { onCommitSeeOnlineRights(); }, nullptr);
    mMapRights->setCommitCallback([this](LLUICtrl*, void*) { mHasUnsavedPermChanges = true; }, nullptr);
    mEditObjectRights->setCommitCallback([this](LLUICtrl*, void*) { onCommitEditRights(); }, nullptr);
    mOkBtn->setCommitCallback([this](LLUICtrl*, void*) { onApplyRights(); }, nullptr);
    mCancelBtn->setCommitCallback([this](LLUICtrl*, void*) { onCancel(); }, nullptr);

    return TRUE;
}

void LLFloaterProfilePermissions::onOpen(const LLSD& key)
{
    if (LLAvatarActions::isFriend(mAvatarID))
    {
        LLAvatarTracker::instance().addParticularFriendObserver(mAvatarID, this);
        fillRightsData();
    }

    mCancelBtn->setFocus(true);

    mAvatarNameCacheConnection = LLAvatarNameCache::get(mAvatarID, boost::bind(&LLFloaterProfilePermissions::onAvatarNameCache, this, _1, _2));
}

void LLFloaterProfilePermissions::draw()
{
    // drawFrustum
    LLView *owner = mOwnerHandle.get();
    static LLCachedControl<F32> max_opacity(gSavedSettings, "PickerContextOpacity", 0.4f);
    drawConeToOwner(mContextConeOpacity, max_opacity, owner);
    LLFloater::draw();
}

void LLFloaterProfilePermissions::changed(U32 mask)
{
    if (mask != LLFriendObserver::ONLINE)
    {
        fillRightsData();
    }
}

void LLFloaterProfilePermissions::onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();

    LLStringUtil::format_map_t args;
    args["[AGENT_NAME]"] = av_name.getDisplayName();
    std::string descritpion = getString("description_string", args);
    mDescription->setValue(descritpion);
}

void LLFloaterProfilePermissions::fillRightsData()
{
    const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(mAvatarID);
    // If true - we are viewing friend's profile, enable check boxes and set values.
    if (relation)
    {
        S32 rights = relation->getRightsGrantedTo();

        BOOL see_online = LLRelationship::GRANT_ONLINE_STATUS & rights ? TRUE : FALSE;
        mOnlineStatus->setValue(see_online);
        mMapRights->setEnabled(see_online);
        mMapRights->setValue(LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
        mEditObjectRights->setValue(LLRelationship::GRANT_MODIFY_OBJECTS & rights ? TRUE : FALSE);
    }
    else
    {
        closeFloater();
        LL_INFOS("ProfilePermissions") << "Floater closing since agent is no longer a friend" << LL_ENDL;
    }
}

void LLFloaterProfilePermissions::rightsConfirmationCallback(const LLSD& notification,
    const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option != 0) // canceled
    {
        mEditObjectRights->setValue(mEditObjectRights->getValue().asBoolean() ? FALSE : TRUE);
    }
    else
    {
        mHasUnsavedPermChanges = true;
    }
}

void LLFloaterProfilePermissions::confirmModifyRights(bool grant)
{
    LLSD args;
    args["NAME"] = LLSLURL("agent", mAvatarID, "completename").getSLURLString();
    LLNotificationsUtil::add(grant ? "GrantModifyRights" : "RevokeModifyRights", args, LLSD(),
        boost::bind(&LLFloaterProfilePermissions::rightsConfirmationCallback, this, _1, _2));
}

void LLFloaterProfilePermissions::onCommitSeeOnlineRights()
{
    bool see_online = mOnlineStatus->getValue().asBoolean();
    mMapRights->setEnabled(see_online);
    if (see_online)
    {
        const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(mAvatarID);
        if (relation)
        {
            S32 rights = relation->getRightsGrantedTo();
            mMapRights->setValue(LLRelationship::GRANT_MAP_LOCATION & rights ? TRUE : FALSE);
        }
        else
        {
            closeFloater();
            LL_INFOS("ProfilePermissions") << "Floater closing since agent is no longer a friend" << LL_ENDL;
        }
    }
    else
    {
        mMapRights->setValue(FALSE);
    }
    mHasUnsavedPermChanges = true;
}

void LLFloaterProfilePermissions::onCommitEditRights()
{
    const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(mAvatarID);

    if (!buddy_relationship)
    {
        LL_WARNS("ProfilePermissions") << "Trying to modify rights for non-friend avatar. Closing floater." << LL_ENDL;
        closeFloater();
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

void LLFloaterProfilePermissions::onApplyRights()
{
    const LLRelationship* buddy_relationship = LLAvatarTracker::instance().getBuddyInfo(mAvatarID);

    if (!buddy_relationship)
    {
        LL_WARNS("ProfilePermissions") << "Trying to modify rights for non-friend avatar. Skipped." << LL_ENDL;
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

    LLAvatarPropertiesProcessor::getInstance()->sendFriendRights(mAvatarID, rights);

    closeFloater();
}

void LLFloaterProfilePermissions::onCancel()
{
    closeFloater();
}

//////////////////////////////////////////////////////////////////////////
// LLPanelProfileSecondLife

LLPanelProfileSecondLife::LLPanelProfileSecondLife()
    : LLPanelProfileTab()
    , mAvatarNameCacheConnection()
    , mHasUnsavedDescriptionChanges(false)
    , mWaitingForImageUpload(false)
    , mAllowPublish(false)
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
    mShowInSearchCombo      = getChild<LLComboBox>("show_in_search");
    mSecondLifePic          = getChild<LLThumbnailCtrl>("2nd_life_pic");
    mSecondLifePicLayout    = getChild<LLPanel>("image_panel");
    mDescriptionEdit        = getChild<LLTextEditor>("sl_description_edit");
    mAgentActionMenuButton  = getChild<LLMenuButton>("agent_actions_menu");
    mSaveDescriptionChanges = getChild<LLButton>("save_description_changes");
    mDiscardDescriptionChanges = getChild<LLButton>("discard_description_changes");
    mCanSeeOnlineIcon       = getChild<LLIconCtrl>("can_see_online");
    mCantSeeOnlineIcon      = getChild<LLIconCtrl>("cant_see_online");
    mCanSeeOnMapIcon        = getChild<LLIconCtrl>("can_see_on_map");
    mCantSeeOnMapIcon       = getChild<LLIconCtrl>("cant_see_on_map");
    mCanEditObjectsIcon     = getChild<LLIconCtrl>("can_edit_objects");
    mCantEditObjectsIcon    = getChild<LLIconCtrl>("cant_edit_objects");

    mShowInSearchCombo->setCommitCallback([this](LLUICtrl*, void*) { onShowInSearchCallback(); }, nullptr);
    mGroupList->setDoubleClickCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { LLPanelProfileSecondLife::openGroupProfile(); });
    mGroupList->setReturnCallback([this](LLUICtrl*, const LLSD&) { LLPanelProfileSecondLife::openGroupProfile(); });
    mSaveDescriptionChanges->setCommitCallback([this](LLUICtrl*, void*) { onSaveDescriptionChanges(); }, nullptr);
    mDiscardDescriptionChanges->setCommitCallback([this](LLUICtrl*, void*) { onDiscardDescriptionChanges(); }, nullptr);
    mDescriptionEdit->setKeystrokeCallback([this](LLTextEditor* caller) { onSetDescriptionDirty(); });

    mCanSeeOnlineIcon->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentPermissionsDialog(); });
    mCantSeeOnlineIcon->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentPermissionsDialog(); });
    mCanSeeOnMapIcon->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentPermissionsDialog(); });
    mCantSeeOnMapIcon->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentPermissionsDialog(); });
    mCanEditObjectsIcon->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentPermissionsDialog(); });
    mCantEditObjectsIcon->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentPermissionsDialog(); });
    mSecondLifePic->setMouseUpCallback([this](LLUICtrl*, S32 x, S32 y, MASK mask) { onShowAgentProfileTexture(); });

    return TRUE;
}

void LLPanelProfileSecondLife::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();

    LLUUID avatar_id = getAvatarId();

    BOOL own_profile = getSelfProfile();

    mGroupList->setShowNone(!own_profile);

    childSetVisible("notes_panel", !own_profile);
    childSetVisible("settings_panel", own_profile);
    childSetVisible("about_buttons_panel", own_profile);

    if (own_profile)
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

    mDescriptionEdit->setParseHTML(!own_profile);

    if (!own_profile)
    {
        mVoiceStatus = LLAvatarActions::canCall() && (LLAvatarActions::isFriend(avatar_id) ? LLAvatarTracker::instance().isBuddyOnline(avatar_id) : TRUE);
        updateOnlineStatus();
        fillRightsData();
    }

    mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileSecondLife::onAvatarNameCache, this, _1, _2));
}

void LLPanelProfileSecondLife::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getStarted() && avatar_id.notNull())
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

void LLPanelProfileSecondLife::refreshName()
{
    if (!mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection = LLAvatarNameCache::get(getAvatarId(), boost::bind(&LLPanelProfileSecondLife::onAvatarNameCache, this, _1, _2));
    }
}

void LLPanelProfileSecondLife::resetData()
{
    resetLoading();

    // Set default image and 1:1 dimensions for it
    mSecondLifePic->setValue("Generic_Person_Large");
    mImageId = LLUUID::null;

    LLRect imageRect = mSecondLifePicLayout->getRect();
    mSecondLifePicLayout->reshape(imageRect.getHeight(), imageRect.getHeight());

    setDescriptionText(LLStringUtil::null);
    mGroups.clear();
    mGroupList->setGroups(mGroups);

    bool own_profile = getSelfProfile();
    mCanSeeOnlineIcon->setVisible(false);
    mCantSeeOnlineIcon->setVisible(!own_profile);
    mCanSeeOnMapIcon->setVisible(false);
    mCantSeeOnMapIcon->setVisible(!own_profile);
    mCanEditObjectsIcon->setVisible(false);
    mCantEditObjectsIcon->setVisible(!own_profile);

    mCanSeeOnlineIcon->setEnabled(false);
    mCantSeeOnlineIcon->setEnabled(false);
    mCanSeeOnMapIcon->setEnabled(false);
    mCantSeeOnMapIcon->setEnabled(false);
    mCanEditObjectsIcon->setEnabled(false);
    mCantEditObjectsIcon->setEnabled(false);

    childSetVisible("partner_layout", FALSE);
    childSetVisible("badge_layout", FALSE);
    childSetVisible("partner_spacer_layout", TRUE);
}

void LLPanelProfileSecondLife::processProfileProperties(const LLAvatarData* avatar_data)
{
    const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
    if ((relationship != NULL || gAgent.isGodlike()) && !getSelfProfile())
    {
        // Relies onto friend observer to get information about online status updates.
        // Once SL-17506 gets implemented, condition might need to become:
        // (gAgent.isGodlike() || isRightGrantedFrom || flags & AVATAR_ONLINE)
        processOnlineStatus(relationship != NULL,
                            gAgent.isGodlike() || relationship->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS),
                            (avatar_data->flags & AVATAR_ONLINE));
    }

    fillCommonData(avatar_data);

    fillPartnerData(avatar_data);

    fillAccountStatus(avatar_data);

    setLoaded();
}

void LLPanelProfileSecondLife::processGroupProperties(const LLAvatarGroups* avatar_groups)
{

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
    getChild<LLUICtrl>("display_name")->setValue(av_name.getDisplayName());
    getChild<LLUICtrl>("user_name")->setValue(av_name.getAccountName());
}

void LLPanelProfileSecondLife::setProfileImageUploading(bool loading)
{
    LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("image_upload_indicator");
    indicator->setVisible(loading);
    if (loading)
    {
        indicator->start();
    }
    else
    {
        indicator->stop();
    }
    mWaitingForImageUpload = loading;
}

void LLPanelProfileSecondLife::setProfileImageUploaded(const LLUUID &image_asset_id)
{
    mSecondLifePic->setValue(image_asset_id);
    mImageId = image_asset_id;

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

    LLFloater *floater = mFloaterProfileTextureHandle.get();
    if (floater)
    {
        LLFloaterProfileTexture * texture_view = dynamic_cast<LLFloaterProfileTexture*>(floater);
        if (mImageId.notNull())
        {
            texture_view->loadAsset(mImageId);
        }
        else
        {
            texture_view->resetAsset();
        }
    }

    setProfileImageUploading(false);
}

bool LLPanelProfileSecondLife::hasUnsavedChanges()
{
    LLFloater *floater = mFloaterPermissionsHandle.get();
    if (floater)
    {
        LLFloaterProfilePermissions* perm = dynamic_cast<LLFloaterProfilePermissions*>(floater);
        if (perm && perm->hasUnsavedChanges())
        {
            return true;
        }
    }
    // if floater
    return mHasUnsavedDescriptionChanges;
}

void LLPanelProfileSecondLife::commitUnsavedChanges()
{
    LLFloater *floater = mFloaterPermissionsHandle.get();
    if (floater)
    {
        LLFloaterProfilePermissions* perm = dynamic_cast<LLFloaterProfilePermissions*>(floater);
        if (perm && perm->hasUnsavedChanges())
        {
            perm->onApplyRights();
        }
    }
    if (mHasUnsavedDescriptionChanges)
    {
        onSaveDescriptionChanges();
    }
}

void LLPanelProfileSecondLife::fillCommonData(const LLAvatarData* avatar_data)
{
    // Refresh avatar id in cache with new info to prevent re-requests
    // and to make sure icons in text will be up to date
    LLAvatarIconIDCache::getInstance()->add(avatar_data->avatar_id, avatar_data->image_id);

    fillAgeData(avatar_data->born_on);

    setDescriptionText(avatar_data->about_text);

    if (avatar_data->image_id.notNull())
    {
        mSecondLifePic->setValue(avatar_data->image_id);
        mImageId = avatar_data->image_id;
    }
    else
    {
        mSecondLifePic->setValue("Generic_Person_Large");
        mImageId = LLUUID::null;
    }

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
        mAllowPublish = avatar_data->flags & AVATAR_ALLOW_PUBLISH;
        mShowInSearchCombo->setValue((BOOL)mAllowPublish);
    }
}

void LLPanelProfileSecondLife::fillPartnerData(const LLAvatarData* avatar_data)
{
    LLTextBox* partner_text_ctrl = getChild<LLTextBox>("partner_link");
    if (avatar_data->partner_id.notNull())
    {
        childSetVisible("partner_layout", TRUE);
        LLStringUtil::format_map_t args;
        args["[LINK]"] = LLSLURL("agent", avatar_data->partner_id, "inspect").getSLURLString();
        std::string partner_text = getString("partner_text", args);
        partner_text_ctrl->setText(partner_text);
    }
    else
    {
        childSetVisible("partner_layout", FALSE);
    }
}

void LLPanelProfileSecondLife::fillAccountStatus(const LLAvatarData* avatar_data)
{
    LLStringUtil::format_map_t args;
    args["[ACCTTYPE]"] = LLAvatarPropertiesProcessor::accountType(avatar_data);
    args["[PAYMENTINFO]"] = LLAvatarPropertiesProcessor::paymentInfo(avatar_data);

    std::string caption_text = getString("CaptionTextAcctInfo", args);
    getChild<LLUICtrl>("account_info")->setValue(caption_text);

    const S32 LINDEN_EMPLOYEE_INDEX = 3;
    LLDate sl_release;
    sl_release.fromYMDHMS(2003, 6, 23, 0, 0, 0);
    std::string customer_lower = avatar_data->customer_type;
    LLStringUtil::toLower(customer_lower);
    if (avatar_data->caption_index == LINDEN_EMPLOYEE_INDEX)
    {
        getChild<LLUICtrl>("badge_icon")->setValue("Profile_Badge_Linden");
        getChild<LLUICtrl>("badge_text")->setValue(getString("BadgeLinden"));
        childSetVisible("badge_layout", TRUE);
        childSetVisible("partner_spacer_layout", FALSE);
    }
    else if (avatar_data->born_on < sl_release)
    {
        getChild<LLUICtrl>("badge_icon")->setValue("Profile_Badge_Beta");
        getChild<LLUICtrl>("badge_text")->setValue(getString("BadgeBeta"));
        childSetVisible("badge_layout", TRUE);
        childSetVisible("partner_spacer_layout", FALSE);
    }
    else if (customer_lower == "beta_lifetime")
    {
        getChild<LLUICtrl>("badge_icon")->setValue("Profile_Badge_Beta_Lifetime");
        getChild<LLUICtrl>("badge_text")->setValue(getString("BadgeBetaLifetime"));
        childSetVisible("badge_layout", TRUE);
        childSetVisible("partner_spacer_layout", FALSE);
    }
    else if (customer_lower == "lifetime")
    {
        getChild<LLUICtrl>("badge_icon")->setValue("Profile_Badge_Lifetime");
        getChild<LLUICtrl>("badge_text")->setValue(getString("BadgeLifetime"));
        childSetVisible("badge_layout", TRUE);
        childSetVisible("partner_spacer_layout", FALSE);
    }
    else if (customer_lower == "secondlifetime_premium")
    {
        getChild<LLUICtrl>("badge_icon")->setValue("Profile_Badge_Premium_Lifetime");
        getChild<LLUICtrl>("badge_text")->setValue(getString("BadgePremiumLifetime"));
        childSetVisible("badge_layout", TRUE);
        childSetVisible("partner_spacer_layout", FALSE);
    }
    else if (customer_lower == "secondlifetime_premium_plus")
    {
        getChild<LLUICtrl>("badge_icon")->setValue("Profile_Badge_Pplus_Lifetime");
        getChild<LLUICtrl>("badge_text")->setValue(getString("BadgePremiumPlusLifetime"));
        childSetVisible("badge_layout", TRUE);
        childSetVisible("partner_spacer_layout", FALSE);
    }
    else
    {
        childSetVisible("badge_layout", FALSE);
        childSetVisible("partner_spacer_layout", TRUE);
    }
}

void LLPanelProfileSecondLife::fillRightsData()
{
    if (getSelfProfile())
    {
        return;
    }

    const LLRelationship* relation = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
    // If true - we are viewing friend's profile, enable check boxes and set values.
    if (relation)
    {
        S32 rights = relation->getRightsGrantedTo();
        bool can_see_online = LLRelationship::GRANT_ONLINE_STATUS & rights;
        bool can_see_on_map = LLRelationship::GRANT_MAP_LOCATION & rights;
        bool can_edit_objects = LLRelationship::GRANT_MODIFY_OBJECTS & rights;

        mCanSeeOnlineIcon->setVisible(can_see_online);
        mCantSeeOnlineIcon->setVisible(!can_see_online);
        mCanSeeOnMapIcon->setVisible(can_see_on_map);
        mCantSeeOnMapIcon->setVisible(!can_see_on_map);
        mCanEditObjectsIcon->setVisible(can_edit_objects);
        mCantEditObjectsIcon->setVisible(!can_edit_objects);

        mCanSeeOnlineIcon->setEnabled(true);
        mCantSeeOnlineIcon->setEnabled(true);
        mCanSeeOnMapIcon->setEnabled(true);
        mCantSeeOnMapIcon->setEnabled(true);
        mCanEditObjectsIcon->setEnabled(true);
        mCantEditObjectsIcon->setEnabled(true);
    }
    else
    {
        mCanSeeOnlineIcon->setVisible(false);
        mCantSeeOnlineIcon->setVisible(false);
        mCanSeeOnMapIcon->setVisible(false);
        mCantSeeOnMapIcon->setVisible(false);
        mCanEditObjectsIcon->setVisible(false);
        mCantEditObjectsIcon->setVisible(false);
    }
}

void LLPanelProfileSecondLife::fillAgeData(const LLDate &born_on)
{
    // Date from server comes already converted to stl timezone,
    // so display it as an UTC + 0
    std::string name_and_date = getString("date_format");
    LLSD args_name;
    args_name["datetime"] = (S32)born_on.secondsSinceEpoch();
    LLStringUtil::format(name_and_date, args_name);
    getChild<LLUICtrl>("sl_birth_date")->setValue(name_and_date);

    std::string register_date = getString("age_format");
    LLSD args_age;
    args_age["[AGE]"] = LLDateUtil::ageFromDate(born_on, LLDate::now());
    LLStringUtil::format(register_date, args_age);
    getChild<LLUICtrl>("user_age")->setValue(register_date);
}

void LLPanelProfileSecondLife::onImageLoaded(BOOL success, LLViewerFetchedTexture *imagep)
{
    LLRect imageRect = mSecondLifePicLayout->getRect();
    if (!success || imagep->getFullWidth() == imagep->getFullHeight())
    {
        mSecondLifePicLayout->reshape(imageRect.getWidth(), imageRect.getWidth());
    }
    else
    {
        // assume 3:4, for sake of firestorm
        mSecondLifePicLayout->reshape(imageRect.getWidth(), imageRect.getWidth() * 3 / 4);
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
    if (mask != LLFriendObserver::ONLINE)
    {
        fillRightsData();
    }
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

// method was disabled according to EXT-2022. Re-enabled & improved according to EXT-3880
void LLPanelProfileSecondLife::updateOnlineStatus()
{
    const LLRelationship* relationship = LLAvatarTracker::instance().getBuddyInfo(getAvatarId());
    if (relationship != NULL)
    {
        // For friend let check if he allowed me to see his status
        bool online = relationship->isOnline();
        bool perm_granted = relationship->isRightGrantedFrom(LLRelationship::GRANT_ONLINE_STATUS);
        processOnlineStatus(true, perm_granted, online);
    }
    else
    {
        childSetVisible("friend_layout", false);
        childSetVisible("online_layout", false);
        childSetVisible("offline_layout", false);
    }
}

void LLPanelProfileSecondLife::processOnlineStatus(bool is_friend, bool show_online, bool online)
{
    childSetVisible("friend_layout", is_friend);
    childSetVisible("online_layout", online && show_online);
    childSetVisible("offline_layout", !online && show_online);
}

void LLPanelProfileSecondLife::setLoaded()
{
    LLPanelProfileTab::setLoaded();

    if (getSelfProfile())
    {
        mShowInSearchCombo->setEnabled(TRUE);
        mDescriptionEdit->setEnabled(TRUE);
    }
}


class LLProfileImagePicker : public LLFilePickerThread
{
public:
    LLProfileImagePicker(EProfileImageType type, LLHandle<LLPanel> *handle);
    ~LLProfileImagePicker();
    void notify(const std::vector<std::string>& filenames) override;

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
    if (filenames.empty())
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
        LLSD notif_args;
        notif_args["REASON"] = LLImage::getLastError().c_str();
        LLNotificationsUtil::add("CannotUploadTexture", notif_args);
        LL_WARNS("AvatarProperties") << "Failed to upload profile image of type " << (S32)mType << ", " << notif_args["REASON"].asString() << LL_ENDL;
        return;
    }

    std::string cap_url = gAgent.getRegionCapability(PROFILE_IMAGE_UPLOAD_CAP);
    if (cap_url.empty())
    {
        LLSD args;
        args["CAPABILITY"] = PROFILE_IMAGE_UPLOAD_CAP;
        LLNotificationsUtil::add("RegionCapabilityRequestError", args);
        LL_WARNS("AvatarProperties") << "Failed to upload profile image of type " << (S32)mType << ", no cap found" << LL_ENDL;
        return;
    }

    switch (mType)
    {
    case PROFILE_IMAGE_SL:
        {
            LLPanelProfileSecondLife* panel = static_cast<LLPanelProfileSecondLife*>(mHandle->get());
            panel->setProfileImageUploading(true);
        }
        break;
    case PROFILE_IMAGE_FL:
        {
            LLPanelProfileFirstLife* panel = static_cast<LLPanelProfileFirstLife*>(mHandle->get());
            panel->setProfileImageUploading(true);
        }
        break;
    }

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
    else if (item_name == "chat_history")
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
    else if (item_name == "agent_permissions")
    {
        onShowAgentPermissionsDialog();
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
        std::string url = "https://[GRID]/my/account/partners.php";
        LLSD subs;
        url = LLWeb::expandURLSubstitutions(url, subs);
        LLUrlAction::openURL(url);
    }
    else if (item_name == "upload_photo")
    {
        (new LLProfileImagePicker(PROFILE_IMAGE_SL, new LLHandle<LLPanel>(getHandle())))->getFile();

        LLFloater* floaterp = mFloaterTexturePickerHandle.get();
        if (floaterp)
        {
            floaterp->closeFloater();
        }
    }
    else if (item_name == "change_photo")
    {
        onShowTexturePicker();
    }
    else if (item_name == "remove_photo")
    {
        onCommitProfileImage(LLUUID::null);

        LLFloater* floaterp = mFloaterTexturePickerHandle.get();
        if (floaterp)
        {
            floaterp->closeFloater();
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
    else if (item_name == "chat_history")
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
    else if (item_name == "agent_permissions")
    {
        return LLAvatarActions::isFriend(agent_id);
    }
    else if (item_name == "copy_display_name"
        || item_name == "copy_username")
    {
        return !mAvatarNameCacheConnection.connected();
    }
    else if (item_name == "upload_photo"
        || item_name == "change_photo")
    {
        std::string cap_url = gAgent.getRegionCapability(PROFILE_IMAGE_UPLOAD_CAP);
        return !cap_url.empty() && !mWaitingForImageUpload && getIsLoaded();
    }
    else if (item_name == "remove_photo")
    {
        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        return mImageId.notNull() && !cap_url.empty() && !mWaitingForImageUpload && getIsLoaded();
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

void LLPanelProfileSecondLife::setDescriptionText(const std::string &text)
{
    mSaveDescriptionChanges->setEnabled(FALSE);
    mDiscardDescriptionChanges->setEnabled(FALSE);
    mHasUnsavedDescriptionChanges = false;

    mDescriptionText = text;
    mDescriptionEdit->setValue(mDescriptionText);
}

void LLPanelProfileSecondLife::onSetDescriptionDirty()
{
    mSaveDescriptionChanges->setEnabled(TRUE);
    mDiscardDescriptionChanges->setEnabled(TRUE);
    mHasUnsavedDescriptionChanges = true;
}

void LLPanelProfileSecondLife::onShowInSearchCallback()
{
    S32 value = mShowInSearchCombo->getValue().asInteger();
    if (mAllowPublish == (bool)value)
    {
        return;
    }
    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        mAllowPublish = value;
        LLSD data;
        data["allow_publish"] = mAllowPublish;
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), data));
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }
}

void LLPanelProfileSecondLife::onSaveDescriptionChanges()
{
    mDescriptionText = mDescriptionEdit->getValue().asString();
    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), LLSD().with("sl_about_text", mDescriptionText)));
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }

    mSaveDescriptionChanges->setEnabled(FALSE);
    mDiscardDescriptionChanges->setEnabled(FALSE);
    mHasUnsavedDescriptionChanges = false;
}

void LLPanelProfileSecondLife::onDiscardDescriptionChanges()
{
    setDescriptionText(mDescriptionText);
}

void LLPanelProfileSecondLife::onShowAgentPermissionsDialog()
{
    LLFloater *floater = mFloaterPermissionsHandle.get();
    if (!floater)
    {
        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        if (parent_floater)
        {
            LLFloaterProfilePermissions * perms = new LLFloaterProfilePermissions(parent_floater, getAvatarId());
            mFloaterPermissionsHandle = perms->getHandle();
            perms->openFloater();
            perms->setVisibleAndFrontmost(TRUE);

            parent_floater->addDependentFloater(mFloaterPermissionsHandle);
        }
    }
    else // already open
    {
        floater->setMinimized(FALSE);
        floater->setVisibleAndFrontmost(TRUE);
    }
}

void LLPanelProfileSecondLife::onShowAgentProfileTexture()
{
    if (!getIsLoaded())
    {
        return;
    }

    LLFloater *floater = mFloaterProfileTextureHandle.get();
    if (!floater)
    {
        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        if (parent_floater)
        {
            LLFloaterProfileTexture * texture_view = new LLFloaterProfileTexture(parent_floater);
            mFloaterProfileTextureHandle = texture_view->getHandle();
            if (mImageId.notNull())
            {
                texture_view->loadAsset(mImageId);
            }
            else
            {
                texture_view->resetAsset();
            }
            texture_view->openFloater();
            texture_view->setVisibleAndFrontmost(TRUE);

            parent_floater->addDependentFloater(mFloaterProfileTextureHandle);
        }
    }
    else // already open
    {
        LLFloaterProfileTexture * texture_view = dynamic_cast<LLFloaterProfileTexture*>(floater);
        texture_view->setMinimized(FALSE);
        texture_view->setVisibleAndFrontmost(TRUE);
        if (mImageId.notNull())
        {
            texture_view->loadAsset(mImageId);
        }
        else
        {
            texture_view->resetAsset();
        }
    }
}

void LLPanelProfileSecondLife::onShowTexturePicker()
{
    LLFloater* floaterp = mFloaterTexturePickerHandle.get();

    // Show the dialog
    if (!floaterp)
    {
        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        if (parent_floater)
        {
            // because inventory construction is somewhat slow
            getWindow()->setCursor(UI_CURSOR_WAIT);
            LLFloaterTexturePicker* texture_floaterp = new LLFloaterTexturePicker(
                this,
                mImageId,
                LLUUID::null,
                mImageId,
                FALSE,
                FALSE,
                "SELECT PHOTO",
                PERM_NONE,
                PERM_NONE,
                FALSE,
                NULL);

            mFloaterTexturePickerHandle = texture_floaterp->getHandle();

            texture_floaterp->setOnFloaterCommitCallback([this](LLTextureCtrl::ETexturePickOp op, LLPickerSource source, const LLUUID& asset_id, const LLUUID&, const LLUUID&)
            {
                if (op == LLTextureCtrl::TEXTURE_SELECT)
                {
                    onCommitProfileImage(asset_id);
                }
            });
            texture_floaterp->setLocalTextureEnabled(FALSE);
            texture_floaterp->setBakeTextureEnabled(FALSE);
            texture_floaterp->setCanApply(false, true, false);

            parent_floater->addDependentFloater(mFloaterTexturePickerHandle);

            texture_floaterp->openFloater();
            texture_floaterp->setFocus(TRUE);
        }
    }
    else
    {
        floaterp->setMinimized(FALSE);
        floaterp->setVisibleAndFrontmost(TRUE);
    }
}

void LLPanelProfileSecondLife::onCommitProfileImage(const LLUUID& id)
{
    if (mImageId == id)
    {
        return;
    }

    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        LLSD params;
        params["sl_image_id"] = id;
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), params));

        mImageId = id;
        if (mImageId == LLUUID::null)
        {
            mSecondLifePic->setValue("Generic_Person_Large");
        }
        else
        {
            mSecondLifePic->setValue(mImageId);
        }

        LLFloater *floater = mFloaterProfileTextureHandle.get();
        if (floater)
        {
            LLFloaterProfileTexture * texture_view = dynamic_cast<LLFloaterProfileTexture*>(floater);
            if (mImageId == LLUUID::null)
            {
                texture_view->resetAsset();
            }
            else
            {
                texture_view->loadAsset(mImageId);
            }
        }
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }
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

void LLPanelProfileWeb::resetData()
{
    mWebBrowser->navigateHome();
}

void LLPanelProfileWeb::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getStarted() && avatar_id.notNull() && !mURLWebProfile.empty())
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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelProfileFirstLife::LLPanelProfileFirstLife()
 : LLPanelProfileTab()
 , mHasUnsavedChanges(false)
{
}

LLPanelProfileFirstLife::~LLPanelProfileFirstLife()
{
}

BOOL LLPanelProfileFirstLife::postBuild()
{
    mDescriptionEdit = getChild<LLTextEditor>("fl_description_edit");
    mPicture = getChild<LLThumbnailCtrl>("real_world_pic");

    mUploadPhoto = getChild<LLButton>("fl_upload_image");
    mChangePhoto = getChild<LLButton>("fl_change_image");
    mRemovePhoto = getChild<LLButton>("fl_remove_image");
    mSaveChanges = getChild<LLButton>("fl_save_changes");
    mDiscardChanges = getChild<LLButton>("fl_discard_changes");

    mUploadPhoto->setCommitCallback([this](LLUICtrl*, void*) { onUploadPhoto(); }, nullptr);
    mChangePhoto->setCommitCallback([this](LLUICtrl*, void*) { onChangePhoto(); }, nullptr);
    mRemovePhoto->setCommitCallback([this](LLUICtrl*, void*) { onRemovePhoto(); }, nullptr);
    mSaveChanges->setCommitCallback([this](LLUICtrl*, void*) { onSaveDescriptionChanges(); }, nullptr);
    mDiscardChanges->setCommitCallback([this](LLUICtrl*, void*) { onDiscardDescriptionChanges(); }, nullptr);
    mDescriptionEdit->setKeystrokeCallback([this](LLTextEditor* caller) { onSetDescriptionDirty(); });

    return TRUE;
}

void LLPanelProfileFirstLife::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    if (!getSelfProfile())
    {
        // Otherwise as the only focusable element it will be selected
        mDescriptionEdit->setTabStop(FALSE);
    }

    resetData();
}

void LLPanelProfileFirstLife::setProfileImageUploading(bool loading)
{
    mUploadPhoto->setEnabled(!loading);
    mChangePhoto->setEnabled(!loading);
    mRemovePhoto->setEnabled(!loading && mImageId.notNull());

    LLLoadingIndicator* indicator = getChild<LLLoadingIndicator>("image_upload_indicator");
    indicator->setVisible(loading);
    if (loading)
    {
        indicator->start();
    }
    else
    {
        indicator->stop();
    }
}

void LLPanelProfileFirstLife::setProfileImageUploaded(const LLUUID &image_asset_id)
{
    mPicture->setValue(image_asset_id);
    mImageId = image_asset_id;
    setProfileImageUploading(false);
}

void LLPanelProfileFirstLife::commitUnsavedChanges()
{
    if (mHasUnsavedChanges)
    {
        onSaveDescriptionChanges();
    }
}

void LLPanelProfileFirstLife::onUploadPhoto()
{
    (new LLProfileImagePicker(PROFILE_IMAGE_FL, new LLHandle<LLPanel>(getHandle())))->getFile();

    LLFloater* floaterp = mFloaterTexturePickerHandle.get();
    if (floaterp)
    {
        floaterp->closeFloater();
    }
}

void LLPanelProfileFirstLife::onChangePhoto()
{
    LLFloater* floaterp = mFloaterTexturePickerHandle.get();

    // Show the dialog
    if (!floaterp)
    {
        LLFloater* parent_floater = gFloaterView->getParentFloater(this);
        if (parent_floater)
        {
            // because inventory construction is somewhat slow
            getWindow()->setCursor(UI_CURSOR_WAIT);
            LLFloaterTexturePicker* texture_floaterp = new LLFloaterTexturePicker(
                this,
                mImageId,
                LLUUID::null,
                mImageId,
                FALSE,
                FALSE,
                "SELECT PHOTO",
                PERM_NONE,
                PERM_NONE,
                FALSE,
                NULL);

            mFloaterTexturePickerHandle = texture_floaterp->getHandle();

            texture_floaterp->setOnFloaterCommitCallback([this](LLTextureCtrl::ETexturePickOp op, LLPickerSource source, const LLUUID& asset_id, const LLUUID&, const LLUUID&)
            {
                if (op == LLTextureCtrl::TEXTURE_SELECT)
                {
                    onCommitPhoto(asset_id);
                }
            });
            texture_floaterp->setLocalTextureEnabled(FALSE);
            texture_floaterp->setCanApply(false, true, false);

            parent_floater->addDependentFloater(mFloaterTexturePickerHandle);

            texture_floaterp->openFloater();
            texture_floaterp->setFocus(TRUE);
        }
    }
    else
    {
        floaterp->setMinimized(FALSE);
        floaterp->setVisibleAndFrontmost(TRUE);
    }
}

void LLPanelProfileFirstLife::onRemovePhoto()
{
    onCommitPhoto(LLUUID::null);

    LLFloater* floaterp = mFloaterTexturePickerHandle.get();
    if (floaterp)
    {
        floaterp->closeFloater();
    }
}

void LLPanelProfileFirstLife::onCommitPhoto(const LLUUID& id)
{
    if (mImageId == id)
    {
        return;
    }

    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        LLSD params;
        params["fl_image_id"] = id;
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), params));

        mImageId = id;
        if (mImageId.notNull())
        {
            mPicture->setValue(mImageId);
        }
        else
        {
            mPicture->setValue("Generic_Person_Large");
        }

        mRemovePhoto->setEnabled(mImageId.notNull());
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }
}

void LLPanelProfileFirstLife::setDescriptionText(const std::string &text)
{
    mSaveChanges->setEnabled(FALSE);
    mDiscardChanges->setEnabled(FALSE);
    mHasUnsavedChanges = false;

    mCurrentDescription = text;
    mDescriptionEdit->setValue(mCurrentDescription);
}

void LLPanelProfileFirstLife::onSetDescriptionDirty()
{
    mSaveChanges->setEnabled(TRUE);
    mDiscardChanges->setEnabled(TRUE);
    mHasUnsavedChanges = true;
}

void LLPanelProfileFirstLife::onSaveDescriptionChanges()
{
    mCurrentDescription = mDescriptionEdit->getValue().asString();
    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), LLSD().with("fl_about_text", mCurrentDescription)));
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }

    mSaveChanges->setEnabled(FALSE);
    mDiscardChanges->setEnabled(FALSE);
    mHasUnsavedChanges = false;
}

void LLPanelProfileFirstLife::onDiscardDescriptionChanges()
{
    setDescriptionText(mCurrentDescription);
}

void LLPanelProfileFirstLife::processProperties(const LLAvatarData* avatar_data)
{
    setDescriptionText(avatar_data->fl_about_text);

    mImageId = avatar_data->fl_image_id;

    if (mImageId.notNull())
    {
        mPicture->setValue(mImageId);
    }
    else
    {
        mPicture->setValue("Generic_Person_Large");
    }

    setLoaded();
}

void LLPanelProfileFirstLife::resetData()
{
    setDescriptionText(std::string());
    mPicture->setValue("Generic_Person_Large");
    mImageId = LLUUID::null;

    mUploadPhoto->setVisible(getSelfProfile());
    mChangePhoto->setVisible(getSelfProfile());
    mRemovePhoto->setVisible(getSelfProfile());
    mSaveChanges->setVisible(getSelfProfile());
    mDiscardChanges->setVisible(getSelfProfile());
}

void LLPanelProfileFirstLife::setLoaded()
{
    LLPanelProfileTab::setLoaded();

    if (getSelfProfile())
    {
        mDescriptionEdit->setEnabled(TRUE);
        mPicture->setEnabled(TRUE);
        mRemovePhoto->setEnabled(mImageId.notNull());
    }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LLPanelProfileNotes::LLPanelProfileNotes()
: LLPanelProfileTab()
 , mHasUnsavedChanges(false)
{

}

LLPanelProfileNotes::~LLPanelProfileNotes()
{
}

void LLPanelProfileNotes::updateData()
{
    LLUUID avatar_id = getAvatarId();
    if (!getStarted() && avatar_id.notNull())
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

void LLPanelProfileNotes::commitUnsavedChanges()
{
    if (mHasUnsavedChanges)
    {
        onSaveNotesChanges();
    }
}

BOOL LLPanelProfileNotes::postBuild()
{
    mNotesEditor = getChild<LLTextEditor>("notes_edit");
    mSaveChanges = getChild<LLButton>("notes_save_changes");
    mDiscardChanges = getChild<LLButton>("notes_discard_changes");

    mSaveChanges->setCommitCallback([this](LLUICtrl*, void*) { onSaveNotesChanges(); }, nullptr);
    mDiscardChanges->setCommitCallback([this](LLUICtrl*, void*) { onDiscardNotesChanges(); }, nullptr);
    mNotesEditor->setKeystrokeCallback([this](LLTextEditor* caller) { onSetNotesDirty(); });

    return TRUE;
}

void LLPanelProfileNotes::onOpen(const LLSD& key)
{
    LLPanelProfileTab::onOpen(key);

    resetData();
}

void LLPanelProfileNotes::setNotesText(const std::string &text)
{
    mSaveChanges->setEnabled(FALSE);
    mDiscardChanges->setEnabled(FALSE);
    mHasUnsavedChanges = false;

    mCurrentNotes = text;
    mNotesEditor->setValue(mCurrentNotes);
}

void LLPanelProfileNotes::onSetNotesDirty()
{
    mSaveChanges->setEnabled(TRUE);
    mDiscardChanges->setEnabled(TRUE);
    mHasUnsavedChanges = true;
}

void LLPanelProfileNotes::onSaveNotesChanges()
{
    mCurrentNotes = mNotesEditor->getValue().asString();
    std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
    if (!cap_url.empty())
    {
        LLCoros::instance().launch("putAgentUserInfoCoro",
            boost::bind(put_avatar_properties_coro, cap_url, getAvatarId(), LLSD().with("notes", mCurrentNotes)));
    }
    else
    {
        LL_WARNS("AvatarProperties") << "Failed to update profile data, no cap found" << LL_ENDL;
    }

    mSaveChanges->setEnabled(FALSE);
    mDiscardChanges->setEnabled(FALSE);
    mHasUnsavedChanges = false;
}

void LLPanelProfileNotes::onDiscardNotesChanges()
{
    setNotesText(mCurrentNotes);
}

void LLPanelProfileNotes::processProperties(LLAvatarNotes* avatar_notes)
{
    setNotesText(avatar_notes->notes);
    mNotesEditor->setEnabled(TRUE);
    setLoaded();
}

void LLPanelProfileNotes::resetData()
{
    resetLoading();
    setNotesText(std::string());
}

void LLPanelProfileNotes::setAvatarId(const LLUUID& avatar_id)
{
    if (avatar_id.notNull())
    {
        LLPanelProfileTab::setAvatarId(avatar_id);
    }
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

void LLPanelProfile::onTabChange()
{
    LLPanelProfileTab* active_panel = dynamic_cast<LLPanelProfileTab*>(mTabContainer->getCurrentPanel());
    if (active_panel)
    {
        active_panel->updateData();
    }
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

    // Always request the base profile info
    resetLoading();
    updateData();

    // Some tabs only request data when opened
    mTabContainer->setCommitCallback(boost::bind(&LLPanelProfile::onTabChange, this));
}

void LLPanelProfile::updateData()
{
    LLUUID avatar_id = getAvatarId();
    // Todo: getIsloading functionality needs to be expanded to
    // include 'inited' or 'data_provided' state to not rerequest
    if (!getStarted() && avatar_id.notNull())
    {
        setIsLoading();

        mPanelSecondlife->setIsLoading();
        mPanelPicks->setIsLoading();
        mPanelFirstlife->setIsLoading();
        mPanelNotes->setIsLoading();

        std::string cap_url = gAgent.getRegionCapability(PROFILE_PROPERTIES_CAP);
        if (!cap_url.empty())
        {
            LLCoros::instance().launch("requestAgentUserInfoCoro",
                boost::bind(request_avatar_properties_coro, cap_url, avatar_id));
        }
    }
}

void LLPanelProfile::refreshName()
{
    mPanelSecondlife->refreshName();
}

void LLPanelProfile::createPick(const LLPickData &data)
{
    mTabContainer->selectTabPanel(mPanelPicks);
    mPanelPicks->createPick(data);
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

bool LLPanelProfile::hasUnsavedChanges()
{
    return mPanelSecondlife->hasUnsavedChanges()
        || mPanelPicks->hasUnsavedChanges()
        || mPanelClassifieds->hasUnsavedChanges()
        || mPanelFirstlife->hasUnsavedChanges()
        || mPanelNotes->hasUnsavedChanges();
}

bool LLPanelProfile::hasUnpublishedClassifieds()
{
    return mPanelClassifieds->hasNewClassifieds();
}

void LLPanelProfile::commitUnsavedChanges()
{
    mPanelSecondlife->commitUnsavedChanges();
    mPanelPicks->commitUnsavedChanges();
    mPanelClassifieds->commitUnsavedChanges();
    mPanelFirstlife->commitUnsavedChanges();
    mPanelNotes->commitUnsavedChanges();
}

void LLPanelProfile::showClassified(const LLUUID& classified_id, bool edit)
{
    if (classified_id.notNull())
    {
        mPanelClassifieds->selectClassified(classified_id, edit);
    }
    mTabContainer->selectTabPanel(mPanelClassifieds);
}

void LLPanelProfile::createClassified()
{
    mPanelClassifieds->createClassified();
    mTabContainer->selectTabPanel(mPanelClassifieds);
}

