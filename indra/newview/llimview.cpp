/**
 * @file LLIMMgr.cpp
 * @brief Container for Instant Messaging
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llimview.h"

#include "llavatarnamecache.h"  // IDEVO
#include "llavataractions.h"
#include "llfloaterconversationlog.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llbutton.h"
#include "llsdutil_math.h"
#include "llstring.h"
#include "lltextutil.h"
#include "lltrans.h"
#include "lltranslate.h"
#include "lluictrlfactory.h"
#include "llfloaterimsessiontab.h"
#include "llagent.h"
#include "llagentui.h"
#include "llappviewer.h"
#include "llavatariconctrl.h"
#include "llcallingcard.h"
#include "llchat.h"
#include "llfloaterimsession.h"
#include "llfloaterimcontainer.h"
#include "llgroupiconctrl.h"
#include "llmd5.h"
#include "llmutelist.h"
#include "llrecentpeople.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llfloaterimnearbychat.h"
#include "llspeakers.h" //for LLIMSpeakerMgr
#include "lltextbox.h"
#include "lltoolbarview.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llconversationlog.h"
#include "message.h"
#include "llviewerregion.h"
#include "llcorehttputil.h"
#include "lluiusage.h"

#include <array>

const static std::string ADHOC_NAME_SUFFIX(" Conference");

const static std::string NEARBY_P2P_BY_OTHER("nearby_P2P_by_other");
const static std::string NEARBY_P2P_BY_AGENT("nearby_P2P_by_agent");

// Markers inserted around translated part of chat text
const static std::string XL8_START_TAG(" (");
const static std::string XL8_END_TAG(")");
const S32 XL8_PADDING = 3;  // XL8_START_TAG.size() + XL8_END_TAG.size()

/** Timeout of outgoing session initialization (in seconds) */
const static U32 SESSION_INITIALIZATION_TIMEOUT = 30;

// This enum corresponds to the sim's and adds P2P_CHAT_SESSION,
// as webrtc uses the multiagent chat mechanism for p2p calls,
// instead of relying on vivox calling.
// Don't change this without consulting a server developer.
enum EMultiAgentChatSessionType
{
    GROUP_CHAT_SESSION = 0,
    CONFERENCE_SESSION = 1,
    P2P_CHAT_SESSION   = 2,
    SESSION_TYPE_COUNT
};


void startConferenceCoro(std::string url, LLUUID tempSessionId, LLUUID creatorId, LLUUID otherParticipantId, LLSD agents);

void startP2PVoiceCoro(std::string url, LLUUID tempSessionId, LLUUID creatorId, LLUUID otherParticipantId);

void chatterBoxInvitationCoro(std::string url, LLUUID sessionId, LLIMMgr::EInvitationType invitationType, const LLSD& voiceChannelInfo);
void chatterBoxHistoryCoro(std::string url, LLUUID sessionId, std::string from, std::string message, U32 timestamp);
void start_deprecated_conference_chat(const LLUUID& temp_session_id, const LLUUID& creator_id, const LLUUID& other_participant_id, const LLSD& agents_to_invite);

const LLUUID LLOutgoingCallDialog::OCD_KEY = LLUUID("7CF78E11-0CFE-498D-ADB9-1417BF03DDB4");
//
// Globals
//
LLIMMgr* gIMMgr = NULL;


bool LLSessionTimeoutTimer::tick()
{
    if (mSessionId.isNull()) return true;

    LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(mSessionId);
    if (session && !session->mSessionInitialized)
    {
        gIMMgr->showSessionStartError("session_initialization_timed_out_error", mSessionId);
    }
    return true;
}


void notify_of_message(const LLSD& msg, bool is_dnd_msg);

void process_dnd_im(const LLSD& notification)
{
    LLSD data = notification["substitutions"];
    LLUUID sessionID = data["SESSION_ID"].asUUID();
    LLUUID fromID = data["FROM_ID"].asUUID();

    //re-create the IM session if needed
    //(when coming out of DND mode upon app restart)
    if(!gIMMgr->hasSession(sessionID))
    {
        //reconstruct session using data from the notification
        std::string name = data["FROM"];
        LLAvatarName av_name;
        if (LLAvatarNameCache::get(data["FROM_ID"], &av_name))
        {
            name = av_name.getDisplayName();
        }


        LLIMModel::getInstance()->newSession(sessionID,
            name,
            IM_NOTHING_SPECIAL,
            fromID,
            LLSD(),
            false); //will need slight refactor to retrieve whether offline message or not (assume online for now)
    }

    notify_of_message(data, true);
}


static void on_avatar_name_cache_toast(const LLUUID& agent_id,
                                       const LLAvatarName& av_name,
                                       LLSD msg)
{
    LLSD args;
    args["MESSAGE"] = msg["message"];
    args["TIME"] = msg["time"];
    // *TODO: Can this ever be an object name or group name?
    args["FROM"] = av_name.getCompleteName();
    args["FROM_ID"] = msg["from_id"];
    args["SESSION_ID"] = msg["session_id"];
    args["SESSION_TYPE"] = msg["session_type"];
    LLNotificationsUtil::add("IMToast", args, args, boost::bind(&LLFloaterIMContainer::showConversation, LLFloaterIMContainer::getInstance(), msg["session_id"].asUUID()));
}

void notify_of_message(const LLSD& msg, bool is_dnd_msg)
{
    std::string user_preferences;
    LLUUID participant_id = msg[is_dnd_msg ? "FROM_ID" : "from_id"].asUUID();
    LLUUID session_id = msg[is_dnd_msg ? "SESSION_ID" : "session_id"].asUUID();
    LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(session_id);

    // do not show notification which goes from agent
    if (gAgent.getID() == participant_id)
    {
        return;
    }

    // determine state of conversations floater
    enum {CLOSED, NOT_ON_TOP, ON_TOP, ON_TOP_AND_ITEM_IS_SELECTED} conversations_floater_status;


    LLFloaterIMContainer* im_box = LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container");
    LLFloaterIMSessionTab* session_floater = LLFloaterIMSessionTab::getConversation(session_id);
    bool store_dnd_message = false; // flag storage of a dnd message
    bool is_session_focused = session_floater->isTornOff() && session_floater->hasFocus();
    if (!LLFloater::isVisible(im_box) || im_box->isMinimized())
    {
        conversations_floater_status = CLOSED;
    }
    else if (!im_box->hasFocus() &&
                !(session_floater && LLFloater::isVisible(session_floater)
                && !session_floater->isMinimized() && session_floater->hasFocus()))
    {
        conversations_floater_status = NOT_ON_TOP;
    }
    else if (im_box->getSelectedSession() != session_id)
    {
        conversations_floater_status = ON_TOP;
    }
    else
    {
        conversations_floater_status = ON_TOP_AND_ITEM_IS_SELECTED;
    }

    //  determine user prefs for this session
    if (session_id.isNull())
    {
        if (msg["source_type"].asInteger() == CHAT_SOURCE_OBJECT)
        {
            user_preferences = gSavedSettings.getString("NotificationObjectIMOptions");
            if (!gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundObjectIM")))
            {
                make_ui_sound("UISndNewIncomingIMSession");
            }
        }
        else
        {
            user_preferences = gSavedSettings.getString("NotificationNearbyChatOptions");
            if (!gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundNearbyChatIM")))
            {
                make_ui_sound("UISndNewIncomingIMSession");
            }
        }
    }
    else if (session->isP2PSessionType())
    {
        if (LLAvatarTracker::instance().isBuddy(participant_id))
        {
            user_preferences = gSavedSettings.getString("NotificationFriendIMOptions");
            if (!gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundFriendIM")))
            {
                make_ui_sound("UISndNewIncomingIMSession");
            }
        }
        else
        {
            user_preferences = gSavedSettings.getString("NotificationNonFriendIMOptions");
            if (!gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundNonFriendIM")))
            {
                make_ui_sound("UISndNewIncomingIMSession");
            }
        }
    }
    else if (session->isAdHocSessionType())
    {
        user_preferences = gSavedSettings.getString("NotificationConferenceIMOptions");
        if (!gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundConferenceIM")))
        {
            make_ui_sound("UISndNewIncomingIMSession");
        }
    }
    else if(session->isGroupSessionType())
    {
        user_preferences = gSavedSettings.getString("NotificationGroupChatOptions");
        if (!gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundGroupChatIM")))
        {
            make_ui_sound("UISndNewIncomingIMSession");
        }
    }

    // actions:

    // 0. nothing - exit
    if (("noaction" == user_preferences ||
            ON_TOP_AND_ITEM_IS_SELECTED == conversations_floater_status)
            && session_floater->isMessagePaneExpanded())
    {
        return;
    }

    // 1. open floater and [optional] surface it
    if ("openconversations" == user_preferences &&
            (CLOSED == conversations_floater_status
                    || NOT_ON_TOP == conversations_floater_status))
    {
        if(!gAgent.isDoNotDisturb())
        {
            if(!LLAppViewer::instance()->quitRequested() && !LLFloater::isVisible(im_box))
            {
                // Open conversations floater
                LLFloaterReg::showInstance("im_container");
            }
            im_box->collapseMessagesPane(false);
            if (session_floater)
            {
                if (session_floater->getHost())
                {
                    if (NULL != im_box && im_box->isMinimized())
                    {
                        LLFloater::onClickMinimize(im_box);
                    }
                }
                else
                {
                    if (session_floater->isMinimized())
                    {
                        LLFloater::onClickMinimize(session_floater);
                    }
                }
            }
        }
        else
        {
            store_dnd_message = true;
        }
    }

    // 2. Flash line item
    if ("openconversations" == user_preferences
            || ON_TOP == conversations_floater_status
            || ("toast" == user_preferences && ON_TOP != conversations_floater_status)
        || ("flash" == user_preferences && (CLOSED == conversations_floater_status
                                        || NOT_ON_TOP == conversations_floater_status))
        || is_dnd_msg)
    {
        if(!LLMuteList::getInstance()->isMuted(participant_id))
        {
            if(gAgent.isDoNotDisturb())
            {
                store_dnd_message = true;
            }
            else
            {
                if (is_dnd_msg && (ON_TOP == conversations_floater_status ||
                                    NOT_ON_TOP == conversations_floater_status ||
                                    CLOSED == conversations_floater_status))
                {
                    im_box->highlightConversationItemWidget(session_id, true);
                }
                else
                {
            im_box->flashConversationItemWidget(session_id, true);
        }
    }
        }
    }

    // 3. Flash FUI button
    if (("toast" == user_preferences || "flash" == user_preferences) &&
            (CLOSED == conversations_floater_status
        || NOT_ON_TOP == conversations_floater_status)
        && !is_session_focused
        && !is_dnd_msg) //prevent flashing FUI button because the conversation floater will have already opened
    {
        if(!LLMuteList::getInstance()->isMuted(participant_id))
    {
            if(!gAgent.isDoNotDisturb())
        {
                gToolBarView->flashCommand(LLCommandId("chat"), true, im_box->isMinimized());
        }
            else
            {
                store_dnd_message = true;
            }
    }
    }

    // 4. Toast
    if ((("toast" == user_preferences) &&
        (ON_TOP_AND_ITEM_IS_SELECTED != conversations_floater_status) &&
        (!session_floater->isTornOff()
         || session_floater->isMinimized()
         || !LLFloater::isVisible(session_floater)))
         || !session_floater->isMessagePaneExpanded())

    {
        //Show IM toasts (upper right toasts)
        // Skip toasting for system messages and for nearby chat
        if(session_id.notNull() && participant_id.notNull())
        {
            if(!is_dnd_msg)
            {
                if(gAgent.isDoNotDisturb())
                {
                    store_dnd_message = true;
                }
                else
                {
                    LLAvatarNameCache::get(participant_id, boost::bind(&on_avatar_name_cache_toast, _1, _2, msg));
                }
            }
        }
    }
    if (store_dnd_message)
    {
        // If in DND mode, allow notification to be stored so upon DND exit
        // the user will be notified with some limitations (see 'is_dnd_msg' flag checks)
        if(session_id.notNull()
            && participant_id.notNull()
            && !session_floater->isShown())
        {
            LLAvatarNameCache::get(participant_id, boost::bind(&on_avatar_name_cache_toast, _1, _2, msg));
        }
    }
}

void on_new_message(const LLSD& msg)
{
    notify_of_message(msg, false);
}

void startConferenceCoro(std::string url,
    LLUUID tempSessionId, LLUUID creatorId, LLUUID otherParticipantId, LLSD agents)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ConferenceChatStart", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD postData;
    postData["method"] = "start conference";
    postData["session-id"] = tempSessionId;
    postData["params"] = agents;
    LLSD altParams;
    std::string voice_server_type = gSavedSettings.getString("VoiceServerType");
    if (voice_server_type.empty())
    {
        // default to the server type associated with the region we're on.
        LLVoiceVersionInfo versionInfo = LLVoiceClient::getInstance()->getVersion();
        voice_server_type              = versionInfo.internalVoiceServerType;
    }
    altParams["voice_server_type"] = voice_server_type;
    postData["alt_params"]         = altParams;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("LLIMModel") << "Failed to start conference" << LL_ENDL;
        //try an "old school" way.
        // *TODO: What about other error status codes?  4xx 5xx?
        if (status == LLCore::HttpStatus(HTTP_BAD_REQUEST))
        {
            start_deprecated_conference_chat(
                tempSessionId,
                creatorId,
                otherParticipantId,
                agents);
        }

        //else throw an error back to the client?
        //in theory we should have just have these error strings
        //etc. set up in this file as opposed to the IMMgr,
        //but the error string were unneeded here previously
        //and it is not worth the effort switching over all
        //the possible different language translations
    }
}

void startP2PVoiceCoro(std::string url, LLUUID sessionID, LLUUID creatorId, LLUUID otherParticipantId)
{
    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("StartP2PVoiceCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t                  httpRequest(new LLCore::HttpRequest);

    LLSD postData;
    postData["method"]     = "start p2p voice";
    postData["session-id"] = sessionID;
    postData["params"]     = otherParticipantId;
    LLSD altParams;
    std::string voice_server_type = gSavedSettings.getString("VoiceServerType");
    if (voice_server_type.empty())
    {
        // default to the server type associated with the region we're on.
        LLVoiceVersionInfo versionInfo = LLVoiceClient::getInstance()->getVersion();
        voice_server_type              = versionInfo.internalVoiceServerType;
    }
    altParams["voice_server_type"] = voice_server_type;
    postData["alt_params"]         = altParams;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD               httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status      = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("LLIMModel") << "Failed to start p2p session:" << postData << "->" << result << LL_ENDL;
        // try an "old school" way.
        //  *TODO: What about other error status codes?  4xx 5xx?
        if (status == LLCore::HttpStatus(HTTP_BAD_REQUEST))
        {
            static const std::string error_string("session_does_not_exist_error");
            gIMMgr->showSessionStartError(error_string, sessionID);
        }
    }
}

void chatterBoxInvitationCoro(std::string url, LLUUID sessionId, LLIMMgr::EInvitationType invitationType, const LLSD& voiceChannelInfo)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ConferenceInviteStart", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD postData;
    postData["method"] = "accept invitation";
    postData["session-id"] = sessionId;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!gIMMgr)
    {
        LL_WARNS("") << "Global IM Manager is NULL" << LL_ENDL;
        return;
    }

    if (!status)
    {
        LL_WARNS("LLIMModel") << "Bad HTTP response in chatterBoxInvitationCoro" << LL_ENDL;
        //throw something back to the viewer here?

        gIMMgr->clearPendingAgentListUpdates(sessionId);
        gIMMgr->clearPendingInvitation(sessionId);

        if (status == LLCore::HttpStatus(HTTP_NOT_FOUND))
        {
            static const std::string error_string("session_does_not_exist_error");
            gIMMgr->showSessionStartError(error_string, sessionId);
        }
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);

    LLIMSpeakerMgr* speakerMgr = LLIMModel::getInstance()->getSpeakerManager(sessionId);
    if (speakerMgr)
    {
        //we've accepted our invitation
        //and received a list of agents that were
        //currently in the session when the reply was sent
        //to us.  Now, it is possible that there were some agents
        //to slip in/out between when that message was sent to us
        //and now.

        //the agent list updates we've received have been
        //accurate from the time we were added to the session
        //but unfortunately, our base that we are receiving here
        //may not be the most up to date.  It was accurate at
        //some point in time though.
        speakerMgr->setSpeakers(result);

        //we now have our base of users in the session
        //that was accurate at some point, but maybe not now
        //so now we apply all of the updates we've received
        //in case of race conditions
        speakerMgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(sessionId));
    }

    if (LLIMMgr::INVITATION_TYPE_VOICE == invitationType)
    {
        gIMMgr->startCall(sessionId, LLVoiceChannel::INCOMING_CALL, voiceChannelInfo);
    }

    if ((invitationType == LLIMMgr::INVITATION_TYPE_VOICE
        || invitationType == LLIMMgr::INVITATION_TYPE_IMMEDIATE)
        && LLIMModel::getInstance()->findIMSession(sessionId))
    {
        // TODO remove in 2010, for voice calls we do not open an IM window
        //LLFloaterIMSession::show(mSessionID);
    }

    gIMMgr->clearPendingAgentListUpdates(sessionId);
    gIMMgr->clearPendingInvitation(sessionId);

}

void translateSuccess(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, const std::string& utf8_text,
                        U64 time_n_flags, std::string originalMsg, std::string expectLang, std::string translation, const std::string detected_language)
{
    std::string message_txt(utf8_text);
    // filter out non-interesting responses
    if (!translation.empty()
        && ((detected_language.empty()) || (expectLang != detected_language))
        && (LLStringUtil::compareInsensitive(translation, originalMsg) != 0))
    {   // Note - if this format changes, also fix code in addMessagesFromServerHistory()
        message_txt += XL8_START_TAG + LLTranslate::removeNoTranslateTags(translation) + XL8_END_TAG;
    }

    // Extract info packed in time_n_flags
    bool log2file =      (bool)(time_n_flags & (1LL << 32));
    bool is_region_msg = (bool)(time_n_flags & (1LL << 33));
    U32 time_stamp = (U32)(time_n_flags & 0x00000000ffffffff);

    LLIMModel::getInstance()->processAddingMessage(session_id, from, from_id, message_txt, log2file, is_region_msg, time_stamp);
}

void translateFailure(const LLUUID& session_id, const std::string& from, const LLUUID& from_id, const std::string& utf8_text,
                        U64 time_n_flags, int status, const std::string err_msg)
{
    std::string message_txt(utf8_text);
    std::string msg = LLTrans::getString("TranslationFailed", LLSD().with("[REASON]", err_msg));
    LLStringUtil::replaceString(msg, "\n", " "); // we want one-line error messages
    message_txt += XL8_START_TAG + msg + XL8_END_TAG;

    // Extract info packed in time_n_flags
    bool log2file = (bool)(time_n_flags & (1LL << 32));
    bool is_region_msg = (bool)(time_n_flags & (1LL << 33));
    U32 time_stamp = (U32)(time_n_flags & 0x00000000ffffffff);

    LLIMModel::getInstance()->processAddingMessage(session_id, from, from_id, message_txt, log2file, is_region_msg, time_stamp);
}

void chatterBoxHistoryCoro(std::string url, LLUUID sessionId, std::string from, std::string message, U32 timestamp)
{   // if parameters from, message and timestamp have values, they are a message that opened chat
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("ChatHistory", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD postData;
    postData["method"] = "fetch history";
    postData["session-id"] = sessionId;

    LL_DEBUGS("ChatHistory") << sessionId << ": Chat history posting " << postData << " to " << url
        << ", from " << from << ", message " << message << ", timestamp " << (S32)timestamp << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        LL_WARNS("ChatHistory") << sessionId << ": Bad HTTP response in chatterBoxHistoryCoro"
            << ", results: " << httpResults << LL_ENDL;
        return;
    }

    if (LLApp::isExiting() || gDisconnected)
    {
        LL_DEBUGS("ChatHistory") << "Ignoring chat history response, shutting down" << LL_ENDL;
        return;
    }

    // Add history to IM session
    LLSD history = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_CONTENT];

    LL_DEBUGS("ChatHistory") << sessionId << ": Chat server history fetch returned " << history << LL_ENDL;

    try
    {
        LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(sessionId);
        if (session && history.isArray())
        {   // Result array is sorted oldest to newest
            if (history.size() > 0)
            {   // History from the chat server has an integer 'time' value timestamp.   Create 'datetime' string which will match
                // what we have from the local history cache
                for (LLSD::array_iterator cur_server_hist = history.beginArray(), endLists = history.endArray();
                    cur_server_hist != endLists;
                    cur_server_hist++)
                {
                    if ((*cur_server_hist).isMap())
                    {   // Take the 'time' value from the server and make the date-time string that will be in local cache log files
                        //   {'from_id':u7aa8c222-8a81-450e-b3d1-9c28491ef717,'message':'Can you hear me now?','from':'Chat Tester','num':i86,'time':r1.66501e+09}
                        U32 timestamp = (U32)((*cur_server_hist)[LL_IM_TIME].asInteger());
                        (*cur_server_hist)[LL_IM_DATE_TIME] = LLLogChat::timestamp2LogString(timestamp, true);
                    }
                }

                session->addMessagesFromServerHistory(history, from, message, timestamp);

                // Display the newly added messages
                LLFloaterIMSession* floater = LLFloaterReg::findTypedInstance<LLFloaterIMSession>("impanel", sessionId);
                if (floater && floater->isInVisibleChain())
                {
                    floater->updateMessages();
                }
            }
            else
            {
                LL_DEBUGS("ChatHistory") << sessionId << ": Empty history from chat server, nothing to add" << LL_ENDL;
            }
        }
        else if (session && !history.isArray())
        {
            LL_WARNS("ChatHistory") << sessionId << ": Bad array data fetching chat history" << LL_ENDL;
        }
        else
        {
            LL_WARNS("ChatHistory") << sessionId << ": Unable to find session fetching chat history" << LL_ENDL;
        }
    }
    catch (...)
    {
        LOG_UNHANDLED_EXCEPTION("chatterBoxHistoryCoro");
        LL_WARNS("ChatHistory") << "chatterBoxHistoryCoro unhandled exception while processing data for session " << sessionId << LL_ENDL;
    }
}

LLIMModel::LLIMModel()
{
    addNewMsgCallback(boost::bind(&LLFloaterIMSession::newIMCallback, _1));
    addNewMsgCallback(boost::bind(&on_new_message, _1));
    LLCallDialogManager::instance();
}

LLIMModel::LLIMSession::LLIMSession(const LLUUID& session_id,
                                    const std::string& name,
                                    const EInstantMessage& type,
                                    const LLUUID& other_participant_id,
                                    const LLSD& voice_channel_info,
                                    const uuid_vec_t& ids,
                                    bool has_offline_msg)
:   mSessionID(session_id),
    mName(name),
    mType(type),
    mHasOfflineMessage(has_offline_msg),
    mParticipantUnreadMessageCount(0),
    mNumUnread(0),
    mOtherParticipantID(other_participant_id),
    mInitialTargetIDs(ids),
    mVoiceChannel(NULL),
    mP2PAsAdhocCall(false),
    mSpeakers(NULL),
    mSessionInitialized(false),
    mCallBackEnabled(true),
    mTextIMPossible(true),
    mStartCallOnInitialize(false),
    mStartedAsIMCall(!voice_channel_info.isUndefined()),
    mIsDNDsend(false),
    mAvatarNameCacheConnection()
{
    // set P2P type by default
    mSessionType        = P2P_SESSION;

    if (IM_NOTHING_SPECIAL == mType || IM_SESSION_P2P_INVITE == mType)
    {
        mP2PAsAdhocCall = (LLVoiceClient::getInstance()->getOutgoingCallInterface(voice_channel_info) == NULL);
    }
    else
    {
        // determine whether it is group or conference session
        mSessionType = gAgent.isInGroup(mSessionID) ? GROUP_SESSION : ADHOC_SESSION;
    }

    initVoiceChannel(voice_channel_info);

    // All participants will be added to the list of people we've recently interacted with.

    // we need to add only _active_ speakers...so comment this.
    // may delete this later on cleanup
    //mSpeakers->addListener(&LLRecentPeople::instance(), "add");

    //we need to wait for session initialization for outgoing ad-hoc and group chat session
    //correct session id for initiated ad-hoc chat will be received from the server
    if (!LLIMModel::getInstance()->sendStartSession(mSessionID, mOtherParticipantID, mInitialTargetIDs, mType, mP2PAsAdhocCall))
    {
        //we don't need to wait for any responses
        //so we're already initialized
        mSessionInitialized = true;
    }
    else
    {
        //tick returns true - timer will be deleted after the tick
        new LLSessionTimeoutTimer(mSessionID, SESSION_INITIALIZATION_TIMEOUT);
    }

    if (IM_NOTHING_SPECIAL == mType)
    {
        mCallBackEnabled = LLVoiceClient::getInstance()->isSessionCallBackPossible(mSessionID);
        mTextIMPossible = LLVoiceClient::getInstance()->isSessionTextIMPossible(mSessionID);
    }

    buildHistoryFileName();
    loadHistory();

    // Localizing name of ad-hoc session. STORM-153
    // Changing name should happen here- after the history file was created, so that
    // history files have consistent (English) names in different locales.
    if (isAdHocSessionType() && IM_SESSION_INVITE == mType)
    {
        mAvatarNameCacheConnection = LLAvatarNameCache::get(mOtherParticipantID,boost::bind(&LLIMModel::LLIMSession::onAdHocNameCache,this, _2));
    }
}

void LLIMModel::LLIMSession::initVoiceChannel(const LLSD& voiceChannelInfo)
{
    if (mVoiceChannel)
    {
        if (mVoiceChannel->isThisVoiceChannel(voiceChannelInfo))
        {
            return;
        }
        mVoiceChannelStateChangeConnection.disconnect();

        mVoiceChannel->deactivate();

        delete mVoiceChannel;
        mVoiceChannel = NULL;
    }
    mP2PAsAdhocCall = false;
    if (IM_NOTHING_SPECIAL == mType || IM_SESSION_P2P_INVITE == mType)
    {
        LLVoiceP2POutgoingCallInterface *outgoingInterface = LLVoiceClient::getInstance()->getOutgoingCallInterface(voiceChannelInfo);

        if (outgoingInterface)
        {
            // only use LLVoiceChannelP2P if the provider can handle the special P2P interface,
            // which uses the voice server to relay calls and invites.  Otherwise,
            // we use the group voice provider.
            mVoiceChannel = new LLVoiceChannelP2P(mSessionID, mName, mOtherParticipantID, outgoingInterface);
        }
        else
        {
            mP2PAsAdhocCall = true;
            mVoiceChannel  = new LLVoiceChannelGroup(mSessionID, mName, true);
        }
    }
    else
    {
        // determine whether it is group or conference session
        if (mSessionType == GROUP_SESSION)
        {
            mSessionType  = GROUP_SESSION;
            mVoiceChannel = new LLVoiceChannelGroup(mSessionID, mName, false);
        }
        else if (mSessionType == ADHOC_SESSION)
        {
            mSessionType  = ADHOC_SESSION;
            mVoiceChannel = new LLVoiceChannelGroup(mSessionID, mName, false);
        }
        else
        {
            LL_WARNS("Voice") << "Invalid Session Type when initializing voice channel: " << mSessionType << LL_ENDL;
            return;
        }
    }

    mVoiceChannelStateChangeConnection =
        mVoiceChannel->setStateChangedCallback(boost::bind(&LLIMSession::onVoiceChannelStateChanged, this, _1, _2, _3));

    if (!mSpeakers)
    {
        mSpeakers = new LLIMSpeakerMgr(mVoiceChannel);
    }
    else
    {
        mSpeakers->setVoiceChannel(mVoiceChannel);
    }
}

void LLIMModel::LLIMSession::onAdHocNameCache(const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();

    if (!av_name.isValidName())
    {
        auto separator_index = mName.rfind(" ");
        std::string name = mName.substr(0, separator_index);
        ++separator_index;
        std::string conference_word = mName.substr(separator_index, mName.length());

        // additional check that session name is what we expected
        if ("Conference" == conference_word)
        {
            LLStringUtil::format_map_t args;
            args["[AGENT_NAME]"] = name;
            LLTrans::findString(mName, "conference-title-incoming", args);
        }
    }
    else
    {
        LLStringUtil::format_map_t args;
        args["[AGENT_NAME]"] = av_name.getCompleteName();
        LLTrans::findString(mName, "conference-title-incoming", args);
    }
}

void LLIMModel::LLIMSession::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction)
{
    std::string you_joined_call = LLTrans::getString("you_joined_call");
    std::string you_started_call = LLTrans::getString("you_started_call");
    std::string other_avatar_name = "";
    LLAvatarName av_name;

    std::string message;

    switch(mSessionType)
    {
    case P2P_SESSION:
        LLAvatarNameCache::get(mOtherParticipantID, &av_name);
        other_avatar_name = av_name.getUserName();

        if(direction == LLVoiceChannel::INCOMING_CALL)
        {
            switch(new_state)
            {
            case LLVoiceChannel::STATE_CALL_STARTED :
                {
                    LLStringUtil::format_map_t string_args;
                    string_args["[NAME]"] = other_avatar_name;
                    message = LLTrans::getString("name_started_call", string_args);
                    LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, message);
                    break;
                }
            case LLVoiceChannel::STATE_CONNECTED :
                LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_joined_call);
            default:
                break;
            }
        }
        else // outgoing call
        {
            switch(new_state)
            {
            case LLVoiceChannel::STATE_CALL_STARTED :
                LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_started_call);
                break;
            case LLVoiceChannel::STATE_CONNECTED :
                message = LLTrans::getString("answered_call");
                LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, message);
            default:
                break;
            }
        }
        break;

    case GROUP_SESSION:
    case ADHOC_SESSION:
        if(direction == LLVoiceChannel::INCOMING_CALL)
        {
            switch(new_state)
            {
            case LLVoiceChannel::STATE_CONNECTED :
                LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_joined_call);
            default:
                break;
            }
        }
        else // outgoing call
        {
            switch(new_state)
            {
            case LLVoiceChannel::STATE_CALL_STARTED :
                LLIMModel::getInstance()->addMessage(mSessionID, SYSTEM_FROM, LLUUID::null, you_started_call);
                break;
            default:
                break;
            }
        }
    default:
        break;
    }
    // Update speakers list when connected
    if (mSpeakers && LLVoiceChannel::STATE_CONNECTED == new_state)
    {
        mSpeakers->update(true);
    }
}

LLIMModel::LLIMSession::~LLIMSession()
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }

    delete mSpeakers;
    mSpeakers = NULL;

    mVoiceChannelStateChangeConnection.disconnect();

    // HAVE to do this here -- if it happens in the LLVoiceChannel destructor it will call the wrong version (since the object's partially deconstructed at that point).
    mVoiceChannel->deactivate();

    delete mVoiceChannel;
    mVoiceChannel = NULL;
}

void LLIMModel::LLIMSession::sessionInitReplyReceived(const LLUUID& new_session_id)
{
    mSessionInitialized = true;

    if (new_session_id != mSessionID)
    {
        mSessionID = new_session_id;
        if (mVoiceChannel)
        {
            mVoiceChannel->updateSessionID(new_session_id);
        }
    }
}

void LLIMModel::LLIMSession::addMessage(const std::string& from,
                                        const LLUUID& from_id,
                                        const std::string& utf8_text,
                                        const std::string& time,
                                        const bool is_history,  // comes from a history file or chat server
                                        const bool is_region_msg,
                                        const U32 timestamp)   // may be zero
{
    LLSD message;
    message["from"] = from;
    message["from_id"] = from_id;
    message["message"] = utf8_text;
    message["time"] = time;         // string used in display, may be full data YYYY/MM/DD HH:MM or just HH:MM
    message["timestamp"] = (S32)timestamp;          // use string? LLLogChat::timestamp2LogString(timestamp, true);
    message["index"] = (LLSD::Integer)mMsgs.size();
    message["is_history"] = is_history;
    message["is_region_msg"] = is_region_msg;

    LL_DEBUGS("UIUsage") << "addMessage " << " from " << from << " from_id " << from_id << " utf8_text " << utf8_text << " time " << time << " is_history " << is_history << " session mType " << mType << LL_ENDL;
    if (from_id == gAgent.getID())
    {
        if (mType == IM_SESSION_GROUP_START)
        {
            LLUIUsage::instance().logCommand("Chat.SendGroup");
        }
        else if (mType == IM_NOTHING_SPECIAL)
        {
            LLUIUsage::instance().logCommand("Chat.SendIM");
        }
        else
        {
            LLUIUsage::instance().logCommand("Chat.SendOther");
        }
    }

    mMsgs.push_front(message);          // Add most recent messages to the front of mMsgs

    if (mSpeakers && from_id.notNull())
    {
        mSpeakers->speakerChatted(from_id);
        mSpeakers->setSpeakerTyping(from_id, false);
    }
}

void LLIMModel::LLIMSession::addMessagesFromHistoryCache(const chat_message_list_t& history)
{
    // Add the messages from the local cached chat history to the session window
    for (const auto& msg : history)
    {
        std::string from = msg[LL_IM_FROM];
        LLUUID from_id;
        if (msg[LL_IM_FROM_ID].isDefined())
        {
            from_id = msg[LL_IM_FROM_ID].asUUID();
        }
        else
        {   // convert it to a legacy name if we have a complete name
            std::string legacy_name = gCacheName->buildLegacyName(from);
            from_id = LLAvatarNameCache::getInstance()->findIdByName(legacy_name);
        }

        // Save the last minute of messages so we can merge with the chat server history.
        // Really would be nice to have a numeric timestamp in the local cached chat file
        const std::string & msg_time_str = msg[LL_IM_DATE_TIME].asString();
        if (mLastHistoryCacheDateTime != msg_time_str)
        {
            mLastHistoryCacheDateTime = msg_time_str;   // Reset to the new time
            mLastHistoryCacheMsgs.clear();
        }
        mLastHistoryCacheMsgs.push_front(msg);
        LL_DEBUGS("ChatHistory") << mSessionID << ": Adding history cache message: " << msg << LL_ENDL;

        // Add message from history cache to the display
        addMessage(from, from_id, msg[LL_IM_TEXT], msg[LL_IM_TIME], true, false, 0);   // from history data, not region message, no timestamp
    }
}

void LLIMModel::LLIMSession::addMessagesFromServerHistory(const LLSD& history,             // Array of chat messages from chat server
                                                        const std::string& target_from,    // Sender of message that opened chat
                                                        const std::string& target_message, // Message text that opened chat
                                                        U32 timestamp)                     // timestamp of message that opened chat
{   // Add messages from history returned by the chat server.

    // The session mMsgs may contain chat messages from the local history cache file, and possibly one or more newly
    // arrived chat messages.   If the chat window was manually opened, these will be empty and history can
    // more easily merged.    The history from the server, however, may overlap what is in the file and those must also be merged.

    // At this point, the session mMsgs can have
    //   no messages
    //   nothing from history file cache, but one or more very recently arrived messages,
    //   messages from history file cache, no recent chat
    //   messages from history file cache, one or more very recent messages
    //
    // The chat history from server can possibly contain:
    //   no messages
    //   messages that start back before anything in the local file (obscure case, but possible)
    //   messages that match messages from the history file cache
    //   messages from the last hour, new to the viewer
    //   one or more messages that match most recently received chat (the one that opened the window)
    // In other words:
    //   messages from chat server may or may not match what we already have in mMsgs
    //   We can drop anything that is during the time span covered by the local cache file
    //   To keep things simple, drop any chat data older than the local cache file

    if (!history.isArray())
    {
        LL_WARNS("ChatHistory") << mSessionID << ": Unexpected history data not array, type " << (S32)history.type() << LL_ENDL;
        return;
    }

    if (history.size() == 0)
    {   // If history is empty
        LL_DEBUGS("ChatHistory") << mSessionID << ": addMessagesFromServerHistory() has empty history, nothing to merge" << LL_ENDL;
        return;
    }

    if (history.size() == 1 &&          // Server chat history has one entry,
        target_from.length() > 0 &&     // and we have a chat message that just arrived
        mMsgs.size() > 0)               // and we have some data in the window - assume the history message is there.
    {   // This is the common case where a group chat is silent for a while, and then one message is sent.
        LL_DEBUGS("ChatHistory") << mSessionID << ": addMessagesFromServerHistory() only has chat message just received." << LL_ENDL;
        return;
    }

    LL_DEBUGS("ChatHistory") << mSessionID << ": addMessagesFromServerHistory() starting with mMsg.size() " << mMsgs.size()
        << " adding history with " << history.size() << " messages"
        << ", target_from: " << target_from
        << ", target_message: " << target_message
        << ", timestamp: " << (S32)timestamp << LL_ENDL;

    // At start of merging, mMsgs is either empty, has some chat messages read from a local cache file, and may have
    // one or more messages that just arrived from the server.
    U32 match_timestamp = 0;
    chat_message_list_t shift_msgs;
    if (mMsgs.size() > 0 &&
        target_from.length() > 0
        && target_message.length() > 0)
    {   // Find where to insert the history messages by popping off a few in the session.
        // The most common case is one duplciate message, the one that opens a chat session
        while (mMsgs.size() > 0)
        {
            // The "time" value from mMsgs is a string, either just time HH:MM or a full date and time
            LLSD cur_msg = mMsgs.front();       // Get most recent message from the chat display (front of mMsgs list)

            if (cur_msg.isMap())
            {
                LL_DEBUGS("ChatHistoryCompare") << mSessionID << ": Finding insertion point, looking at cur_msg: " << cur_msg << LL_ENDL;

                match_timestamp = cur_msg["timestamp"].asInteger();  // get timestamp of message in the session, may be zero
                if ((S32)timestamp > match_timestamp)
                {
                    LL_DEBUGS("ChatHistory") << mSessionID << ": found older chat message: " << cur_msg
                        << ", timestamp " << (S32)timestamp
                        << " vs. match_timestamp " << match_timestamp
                        << ", shift_msgs size is " << shift_msgs.size() << LL_ENDL;
                    break;
                }
                // Have the matching message or one more recent: these need to be at the end
                shift_msgs.push_front(cur_msg);     // Move chat message to temp list.
                mMsgs.pop_front();                  // Normally this is just one message
                LL_DEBUGS("ChatHistory") << mSessionID << ": shifting chat message " << cur_msg
                    << " to be inserted at end, shift_msgs size is " << shift_msgs.size()
                    << ", match_timestamp " << match_timestamp
                    << ", timestamp " << (S32)timestamp << LL_ENDL;
            }
            else
            {
                LL_DEBUGS("ChatHistory") << mSessionID << ": Unexpected non-map entry in session messages: " << cur_msg << LL_ENDL;
                return;
            }
        }
    }

    // Now merge messages from server history data into the session display.   The history data
    // from the local file may overlap with the chat messages from the server.
    // Drop any messages from the chat server history that are before the latest one from the local history file.
    // Unfortunately, messages from the local file don't have timestamps - just datetime strings
    LLSD::array_const_iterator cur_history_iter = history.beginArray();
    while (cur_history_iter != history.endArray())
    {
        const LLSD &cur_server_hist = *cur_history_iter;
        cur_history_iter++;

        if (cur_server_hist.isMap())
        {   // Each server history entry looks like
            //   { 'from':'Laggy Avatar', 'from_id' : u72345678 - 744f - 43b9 - 98af - b06f1c76ddda, 'index' : i24, 'is_history' : 1, 'message' : 'That was slow', 'time' : '02/13/2023 10:03', 'timestamp' : i1676311419 }

            // If we reach the message that opened our window, stop adding messages
            U32 history_msg_timestamp = (U32)cur_server_hist[LL_IM_TIME].asInteger();
            if ((match_timestamp > 0 && match_timestamp <= history_msg_timestamp) ||
                (timestamp > 0 && timestamp <= history_msg_timestamp))
            {   // we found the message we matched, so stop inserting from chat server history
                LL_DEBUGS("ChatHistoryCompare") << "Found end of chat history insertion with match_timestamp " << (S32)match_timestamp
                    << " vs. history_msg_timestamp " << (S32)history_msg_timestamp
                    << " vs. timestamp " << (S32)timestamp
                    << LL_ENDL;
                break;
            }
            LL_DEBUGS("ChatHistoryCompare") << "Compared match_timestamp " << (S32)match_timestamp
                << " vs. history_msg_timestamp " << (S32)history_msg_timestamp << LL_ENDL;

            bool add_chat_to_conversation = true;
            if (!mLastHistoryCacheDateTime.empty())
            {   // Skip past the any from server that are older than what we already read from the history file.
                std::string history_datetime = cur_server_hist[LL_IM_DATE_TIME].asString();
                if (history_datetime.empty())
                {
                    history_datetime = cur_server_hist[LL_IM_TIME].asString();
                }

                if (history_datetime < mLastHistoryCacheDateTime)
                {
                    LL_DEBUGS("ChatHistoryCompare") << "Skipping message from chat server history since it's older than messages the session already has."
                        << history_datetime << " vs  " << mLastHistoryCacheDateTime << LL_ENDL;
                    add_chat_to_conversation = false;
                }
                else if (history_datetime > mLastHistoryCacheDateTime)
                {   // The message from the chat server is more recent than the last one from the local cache file.   Add it
                    LL_DEBUGS("ChatHistoryCompare") << "Found message dated "
                        << history_datetime << " vs " << mLastHistoryCacheDateTime
                        << ", adding new message from chat server history " << cur_server_hist << LL_ENDL;
                }
                else   // (history_datetime == mLastHistoryCacheDateTime)
                {      // Messages are in the same minute as the last from the cache log file.
                    const std::string & history_msg_text = cur_server_hist[LL_IM_TEXT];

                    // Look in the saved messages from the history file that have the same time
                    for (const auto& scan_msg : mLastHistoryCacheMsgs)
                    {
                        LL_DEBUGS("ChatHistoryCompare") << "comparing messages " << scan_msg[LL_IM_TEXT]
                            << " with " << cur_server_hist << LL_ENDL;
                        if (scan_msg.size() > 0)
                        {   // Extra work ... the history_msg_text value may have been translated, i.e. "I am confused (je suis confus)"
                            //  while the server history will only have the first part "I am confused"
                            std::string target_compare(scan_msg[LL_IM_TEXT]);
                            if (target_compare.size() > history_msg_text.size() + XL8_PADDING &&
                                target_compare.substr(history_msg_text.size(), XL8_START_TAG.size()) == XL8_START_TAG &&
                                target_compare.substr(target_compare.size() - XL8_END_TAG.size()) == XL8_END_TAG)
                            {   // This really looks like a "translated string (cadena traducida)" so just compare the source part
                                LL_DEBUGS("ChatHistory") << mSessionID << ": Found translated chat " << target_compare
                                    << " when comparing to history " << history_msg_text
                                    << ", will truncate" << LL_ENDL;
                                target_compare = target_compare.substr(0, history_msg_text.size());
                            }
                            if (history_msg_text == target_compare)
                            {   // Found a match, so don't add a duplicate chat message to the window
                                LL_DEBUGS("ChatHistory") << mSessionID << ": Found duplicate message text " << history_msg_text
                                    << " : " << (S32)history_msg_timestamp << ", matching datetime " << history_datetime << LL_ENDL;
                                add_chat_to_conversation = false;
                                break;
                            }
                        }
                    }
                }
            }

            LLUUID sender_id = cur_server_hist[LL_IM_FROM_ID].asUUID();
            if (add_chat_to_conversation)
            {   // Check if they're muted
                if (LLMuteList::getInstance()->isMuted(sender_id, LLMute::flagTextChat))
                {
                    add_chat_to_conversation = false;
                    LL_DEBUGS("ChatHistory") << mSessionID << ": Skipped adding chat from " << sender_id
                        << " as muted, message: " << cur_server_hist
                        << LL_ENDL;
                }
            }

            if (add_chat_to_conversation)
            {   // Finally add message to the chat session
                std::string chat_time_str = LLConversation::createTimestamp((U64Seconds)history_msg_timestamp);
                std::string sender_name = cur_server_hist[LL_IM_FROM].asString();

                std::string history_msg_text = cur_server_hist[LL_IM_TEXT].asString();
                LLSD message;
                message["from"] = sender_name;
                message["from_id"] = sender_id;
                message["message"] = history_msg_text;
                message["time"] = chat_time_str;
                message["timestamp"] = (S32)history_msg_timestamp;
                message["index"] = (LLSD::Integer)mMsgs.size();
                message["is_history"] = true;
                mMsgs.push_front(message);

                LL_DEBUGS("ChatHistory") << mSessionID << ": push_front() adding group chat history message " << message << LL_ENDL;

                // Add chat history messages to the local cache file, only in the case where we opened the chat window
                // Need to solve the logic around messages that arrive and open chat - at this point, they've already been added to the
                //   local history cache file.   If we append messages here, it will be out of order.
                if (target_from.empty() && target_message.empty())
                {
                    LLIMModel::getInstance()->logToFile(LLIMModel::getInstance()->getHistoryFileName(mSessionID),
                        sender_name, sender_id, history_msg_text);
                }
            }
        }
    }

    auto shifted_size = shift_msgs.size();
    while (shift_msgs.size() > 0)
    {   // Finally add back any new messages, and tweak the index value to be correct.
        LLSD newer_message = shift_msgs.front();
        shift_msgs.pop_front();
        S32 old_index = newer_message["index"];
        newer_message["index"] = (LLSD::Integer)mMsgs.size();   // Update the index to match the new position in the conversation
        LL_DEBUGS("ChatHistory") << mSessionID << ": Re-adding newest group chat history messages from " << newer_message["from"]
            << ", text: " << newer_message["message"]
            << " old index " << old_index << ", new index " << newer_message["index"] << LL_ENDL;
        mMsgs.push_front(newer_message);
    }

    LL_DEBUGS("ChatHistory") << mSessionID << ": addMessagesFromServerHistory() exiting with mMsg.size() " << mMsgs.size()
        << ", shifted " << shifted_size << " messages" << LL_ENDL;

    mLastHistoryCacheDateTime.clear();  // Don't need this data
    mLastHistoryCacheMsgs.clear();
}


void LLIMModel::LLIMSession::chatFromLogFile(LLLogChat::ELogLineType type, const LLSD& msg, void* userdata)
{
    if (!userdata) return;

    LLIMSession* self = (LLIMSession*) userdata;

    if (type == LLLogChat::LOG_LINE)
    {
        LL_DEBUGS("ChatHistory") << "chatFromLogFile() adding LOG_LINE message from " << msg << LL_ENDL;
        self->addMessage("", LLSD(), msg["message"].asString(), "", true, false, 0);        // from history data, not region message, no timestamp
    }
    else if (type == LLLogChat::LOG_LLSD)
    {
        LL_DEBUGS("ChatHistory") << "chatFromLogFile() adding LOG_LLSD message from " << msg << LL_ENDL;
        self->addMessage(msg["from"].asString(), msg["from_id"].asUUID(), msg["message"].asString(), msg["time"].asString(), true, false, 0);  // from history data, not region message, no timestamp
    }
}

void LLIMModel::LLIMSession::loadHistory()
{
    mMsgs.clear();
    mLastHistoryCacheMsgs.clear();
    mLastHistoryCacheDateTime.clear();

    if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
    {
        // read and parse chat history from local file
        chat_message_list_t chat_history;
        LLLogChat::loadChatHistory(mHistoryFileName, chat_history, LLSD(), isGroupChat());
        addMessagesFromHistoryCache(chat_history);
    }
}

LLIMModel::LLIMSession* LLIMModel::findIMSession(const LLUUID& session_id) const
{
    return get_if_there(mId2SessionMap, session_id, (LLIMModel::LLIMSession*) NULL);
}

//*TODO consider switching to using std::set instead of std::list for holding LLUUIDs across the whole code
LLIMModel::LLIMSession* LLIMModel::findAdHocIMSession(const uuid_vec_t& ids)
{
    auto num = ids.size();
    if (!num) return NULL;

    if (mId2SessionMap.empty()) return NULL;

    std::map<LLUUID, LLIMSession*>::const_iterator it = mId2SessionMap.begin();
    for (; it != mId2SessionMap.end(); ++it)
    {
        LLIMSession* session = (*it).second;

        if (!session->isAdHoc()) continue;
        if (session->mInitialTargetIDs.size() != num) continue;

        std::list<LLUUID> tmp_list(session->mInitialTargetIDs.begin(), session->mInitialTargetIDs.end());

        uuid_vec_t::const_iterator iter = ids.begin();
        while (iter != ids.end())
        {
            tmp_list.remove(*iter);
            ++iter;

            if (tmp_list.empty())
            {
                break;
            }
        }

        if (tmp_list.empty() && iter == ids.end())
        {
            return session;
        }
    }

    return NULL;
}

bool LLIMModel::LLIMSession::isOutgoingAdHoc() const
{
    return IM_SESSION_CONFERENCE_START == mType;
}

bool LLIMModel::LLIMSession::isAdHoc()
{
    return IM_SESSION_CONFERENCE_START == mType || (IM_SESSION_INVITE == mType && !gAgent.isInGroup(mSessionID, true));
}

bool LLIMModel::LLIMSession::isP2P()
{
    return IM_NOTHING_SPECIAL == mType;
}

bool LLIMModel::LLIMSession::isGroupChat()
{
    return IM_SESSION_GROUP_START == mType || (IM_SESSION_INVITE == mType && gAgent.isInGroup(mSessionID, true));
}

LLUUID LLIMModel::LLIMSession::generateOutgoingAdHocHash() const
{
    LLUUID hash = LLUUID::null;

    if (mInitialTargetIDs.size())
    {
        std::set<LLUUID> sorted_uuids(mInitialTargetIDs.begin(), mInitialTargetIDs.end());
        hash = generateHash(sorted_uuids);
    }

    return hash;
}

void LLIMModel::LLIMSession::buildHistoryFileName()
{
    mHistoryFileName = mName;

    //ad-hoc requires sophisticated chat history saving schemes
    if (isAdHoc())
    {
        /* in case of outgoing ad-hoc sessions we need to make specilized names
        * if this naming system is ever changed then the filtering definitions in
        * lllogchat.cpp need to be change acordingly so that the filtering for the
        * date stamp code introduced in STORM-102 will work properly and not add
        * a date stamp to the Ad-hoc conferences.
        */
        if (mInitialTargetIDs.size())
        {
            std::set<LLUUID> sorted_uuids(mInitialTargetIDs.begin(), mInitialTargetIDs.end());
            mHistoryFileName = mName + " hash" + generateHash(sorted_uuids).asString();
        }
        else
        {
            //in case of incoming ad-hoc sessions
            mHistoryFileName = mName + " " + LLLogChat::timestamp2LogString(0, true) + " " + mSessionID.asString().substr(0, 4);
        }
    }
    else if (isP2P()) // look up username to use as the log name
    {
        LLAvatarName av_name;
        // For outgoing sessions we already have a cached name
        // so no need for a callback in LLAvatarNameCache::get()
        if (LLAvatarNameCache::get(mOtherParticipantID, &av_name))
        {
            mHistoryFileName = LLCacheName::buildUsername(av_name.getUserName());
        }
        else
        {
            // Incoming P2P sessions include a name that we can use to build a history file name
            mHistoryFileName = LLCacheName::buildUsername(mName);
        }

        // user's account name can change, but filenames and session names are account name based
        LLConversationLog::getInstance()->verifyFilename(mSessionID, mHistoryFileName, av_name.getCompleteName());
    }
    else if (isGroupChat())
    {
        mHistoryFileName = mName + GROUP_CHAT_SUFFIX;
    }
}

//static
LLUUID LLIMModel::LLIMSession::generateHash(const std::set<LLUUID>& sorted_uuids)
{
    LLMD5 md5_uuid;

    std::set<LLUUID>::const_iterator it = sorted_uuids.begin();
    while (it != sorted_uuids.end())
    {
        md5_uuid.update((unsigned char*)(*it).mData, 16);
        it++;
    }
    md5_uuid.finalize();

    LLUUID participants_md5_hash;
    md5_uuid.raw_digest((unsigned char*) participants_md5_hash.mData);
    return participants_md5_hash;
}

void LLIMModel::processSessionInitializedReply(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
    LLIMSession* session = findIMSession(old_session_id);
    if (session)
    {
        session->sessionInitReplyReceived(new_session_id);

        if (old_session_id != new_session_id)
        {
            mId2SessionMap.erase(old_session_id);
            mId2SessionMap[new_session_id] = session;
        }

        LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(old_session_id);
        if (im_floater)
        {
            im_floater->sessionInitReplyReceived(new_session_id);
        }

        if (old_session_id != new_session_id)
        {
            gIMMgr->notifyObserverSessionIDUpdated(old_session_id, new_session_id);
        }

        // auto-start the call on session initialization?
        if (session->mStartCallOnInitialize)
        {
            gIMMgr->startCall(new_session_id);
        }
    }
}

void LLIMModel::testMessages()
{
    LLUUID bot1_id("d0426ec6-6535-4c11-a5d9-526bb0c654d9");
    LLUUID bot1_session_id;
    std::string from = "IM Tester";

    bot1_session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, bot1_id);
    newSession(bot1_session_id, from, IM_NOTHING_SPECIAL, bot1_id);
    addMessage(bot1_session_id, from, bot1_id, "Test Message: Hi from testerbot land!");

    LLUUID bot2_id;
    std::string firstname[] = {"Roflcopter", "Joe"};
    std::string lastname[] = {"Linden", "Tester", "Resident", "Schmoe"};

    S32 rand1 = ll_rand(sizeof firstname)/(sizeof firstname[0]);
    S32 rand2 = ll_rand(sizeof lastname)/(sizeof lastname[0]);

    from = firstname[rand1] + " " + lastname[rand2];
    bot2_id.generate(from);
    LLUUID bot2_session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, bot2_id);
    newSession(bot2_session_id, from, IM_NOTHING_SPECIAL, bot2_id);
    addMessage(bot2_session_id, from, bot2_id, "Test Message: Hello there, I have a question. Can I bother you for a second? ");
    addMessage(bot2_session_id, from, bot2_id, "Test Message: OMGWTFBBQ.");
}

//session name should not be empty
bool LLIMModel::newSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type,
                           const LLUUID& other_participant_id, const uuid_vec_t& ids, const LLSD& voiceChannelInfo, bool has_offline_msg)
{
    if (name.empty())
    {
        LL_WARNS() << "Attempt to create a new session with empty name; id = " << session_id << LL_ENDL;
        return false;
    }

    if (findIMSession(session_id))
    {
        LL_WARNS() << "IM Session " << session_id << " already exists" << LL_ENDL;
        return false;
    }

    LLIMSession *session       = new LLIMSession(session_id, name, type, other_participant_id, voiceChannelInfo, ids, has_offline_msg);
    mId2SessionMap[session_id] = session;

    // When notifying observer, name of session is used instead of "name", because they may not be the
    // same if it is an adhoc session (in this case name is localized in LLIMSession constructor).
    std::string session_name = LLIMModel::getInstance()->getName(session_id);
    LLIMMgr::getInstance()->notifyObserverSessionAdded(session_id, session_name, other_participant_id,has_offline_msg);

    return true;

}

bool LLIMModel::newSession(const LLUUID& session_id, const std::string& name, const EInstantMessage& type, const LLUUID& other_participant_id, const LLSD& voiceChannelInfo, bool has_offline_msg)
{
    uuid_vec_t ids;
    ids.push_back(other_participant_id);
    return newSession(session_id, name, type, other_participant_id, ids, voiceChannelInfo, has_offline_msg);
}

bool LLIMModel::clearSession(const LLUUID& session_id)
{
    if (mId2SessionMap.find(session_id) == mId2SessionMap.end()) return false;
    delete (mId2SessionMap[session_id]);
    mId2SessionMap.erase(session_id);
    return true;
}

void LLIMModel::getMessages(const LLUUID& session_id, chat_message_list_t& messages, int start_index, const bool sendNoUnreadMsgs)
{
    getMessagesSilently(session_id, messages, start_index);

    if (sendNoUnreadMsgs)
    {
        sendNoUnreadMessages(session_id);
    }
}

void LLIMModel::getMessagesSilently(const LLUUID& session_id, chat_message_list_t& messages, int start_index)
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return;
    }

    int i = static_cast<int>(session->mMsgs.size()) - start_index;

    for (chat_message_list_t::iterator iter = session->mMsgs.begin();
        iter != session->mMsgs.end() && i > 0;
        iter++)
    {
        LLSD msg;
        msg = *iter;
        messages.push_back(*iter);
        i--;
    }
}

void LLIMModel::sendNoUnreadMessages(const LLUUID& session_id)
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return;
    }

    session->mNumUnread = 0;
    session->mParticipantUnreadMessageCount = 0;

    LLSD arg;
    arg["session_id"] = session_id;
    arg["num_unread"] = 0;
    arg["participant_unread"] = session->mParticipantUnreadMessageCount;
    mNoUnreadMsgsSignal(arg);
}

bool LLIMModel::addToHistory(const LLUUID& session_id,
                             const std::string& from,
                             const LLUUID& from_id,
                             const std::string& utf8_text,
                             bool is_region_msg,
                             U32 timestamp)
{
    LLIMSession* session = findIMSession(session_id);

    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return false;
    }

    // This is where a normal arriving message is added to the session.   Note that the time string created here is without the full date
    session->addMessage(from, from_id, utf8_text, LLLogChat::timestamp2LogString(timestamp, false), false, is_region_msg, timestamp);

    return true;
}

bool LLIMModel::logToFile(const std::string& file_name, const std::string& from, const LLUUID& from_id, const std::string& utf8_text)
{
    if (gSavedPerAccountSettings.getS32("KeepConversationLogTranscripts") > 1)
    {
        std::string from_name = from;

        LLAvatarName av_name;
        if (!from_id.isNull() &&
            LLAvatarNameCache::get(from_id, &av_name) &&
            !av_name.isDisplayNameDefault())
        {
            from_name = av_name.getCompleteName();
        }

        LLLogChat::saveHistory(file_name, from_name, from_id, utf8_text);
        LLConversationLog::instance().cache(); // update the conversation log too
        return true;
    }
    else
    {
        return false;
    }
}

void LLIMModel::proccessOnlineOfflineNotification(
    const LLUUID& session_id,
    const std::string& utf8_text)
{
    // Add system message to history
    addMessage(session_id, SYSTEM_FROM, LLUUID::null, utf8_text);
}

void LLIMModel::addMessage(const LLUUID& session_id, const std::string& from, const LLUUID& from_id,
                           const std::string& utf8_text, bool log2file /* = true */, bool is_region_msg, /* = false */ U32 time_stamp /* = 0 */)
{
    if (gSavedSettings.getBOOL("TranslateChat") && (from != SYSTEM_FROM))
    {
        const std::string from_lang = ""; // leave empty to trigger autodetect
        const std::string to_lang = LLTranslate::getTranslateLanguage();
        U64 time_n_flags = ((U64) time_stamp) | (log2file ? (1LL << 32) : 0) | (is_region_msg ? (1LL << 33) : 0);   // boost::bind has limited parameters
        LLTranslate::translateMessage(from_lang, to_lang, utf8_text,
            boost::bind(&translateSuccess, session_id, from, from_id, utf8_text, time_n_flags, utf8_text, from_lang, _1, _2),
            boost::bind(&translateFailure, session_id, from, from_id, utf8_text, time_n_flags, _1, _2));
    }
    else
    {
        processAddingMessage(session_id, from, from_id, utf8_text, log2file, is_region_msg, time_stamp);
    }
}

void LLIMModel::processAddingMessage(const LLUUID& session_id, const std::string& from, const LLUUID& from_id,
    const std::string& utf8_text, bool log2file, bool is_region_msg, U32 time_stamp)
{
    LLIMSession* session = addMessageSilently(session_id, from, from_id, utf8_text, log2file, is_region_msg, time_stamp);
    if (!session)
        return;

    //good place to add some1 to recent list
    //other places may be called from message history.
    if( !from_id.isNull() &&
        ( session->isP2PSessionType() || session->isAdHocSessionType() ) )
        LLRecentPeople::instance().add(from_id);

    // notify listeners
    LLSD arg;
    arg["session_id"] = session_id;
    arg["num_unread"] = session->mNumUnread;
    arg["participant_unread"] = session->mParticipantUnreadMessageCount;
    arg["message"] = utf8_text;
    arg["from"] = from;
    arg["from_id"] = from_id;
    arg["time"] = LLLogChat::timestamp2LogString(time_stamp, true);
    arg["session_type"] = session->mSessionType;
    arg["is_region_msg"] = is_region_msg;

    mNewMsgSignal(arg);
}

LLIMModel::LLIMSession* LLIMModel::addMessageSilently(const LLUUID& session_id, const std::string& from, const LLUUID& from_id,
                                                      const std::string& utf8_text, bool log2file /* = true */, bool is_region_msg, /* false */
                                                      U32 timestamp /* = 0 */)
{
    LLIMSession* session = findIMSession(session_id);

    if (!session)
    {
        return NULL;
    }

    // replace interactive system message marker with correct from string value
    std::string from_name = from;
    if (INTERACTIVE_SYSTEM_FROM == from)
    {
        from_name = SYSTEM_FROM;
    }

    addToHistory(session_id, from_name, from_id, utf8_text, is_region_msg, timestamp);
    if (log2file)
    {
        logToFile(getHistoryFileName(session_id), from_name, from_id, utf8_text);
    }

    session->mNumUnread++;

    //update count of unread messages from real participant
    if (!(from_id.isNull() || from_id == gAgentID || SYSTEM_FROM == from)
            // we should increment counter for interactive system messages()
            || INTERACTIVE_SYSTEM_FROM == from)
    {
        ++(session->mParticipantUnreadMessageCount);
    }

    return session;
}


const std::string LLIMModel::getName(const LLUUID& session_id) const
{
    LLIMSession* session = findIMSession(session_id);

    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return LLTrans::getString("no_session_message");
    }

    return session->mName;
}

const S32 LLIMModel::getNumUnread(const LLUUID& session_id) const
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return -1;
    }

    return session->mNumUnread;
}

const LLUUID& LLIMModel::getOtherParticipantID(const LLUUID& session_id) const
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << " does not exist " << LL_ENDL;
        return LLUUID::null;
    }

    return session->mOtherParticipantID;
}

EInstantMessage LLIMModel::getType(const LLUUID& session_id) const
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return IM_COUNT;
    }

    return session->mType;
}

LLVoiceChannel* LLIMModel::getVoiceChannel( const LLUUID& session_id) const
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << "does not exist " << LL_ENDL;
        return NULL;
    }

    return session->mVoiceChannel;
}

LLIMSpeakerMgr* LLIMModel::getSpeakerManager( const LLUUID& session_id ) const
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << " does not exist " << LL_ENDL;
        return NULL;
    }

    return session->mSpeakers;
}

const std::string& LLIMModel::getHistoryFileName(const LLUUID& session_id) const
{
    LLIMSession* session = findIMSession(session_id);
    if (!session)
    {
        LL_WARNS() << "session " << session_id << " does not exist " << LL_ENDL;
        return LLStringUtil::null;
    }

    return session->mHistoryFileName;
}


// TODO get rid of other participant ID
void LLIMModel::sendTypingState(LLUUID session_id, LLUUID other_participant_id, bool typing)
{
    std::string name;
    LLAgentUI::buildFullname(name);

    pack_instant_message(
        gMessageSystem,
        gAgent.getID(),
        false,
        gAgent.getSessionID(),
        other_participant_id,
        name,
        std::string("typing"),
        IM_ONLINE,
        (typing ? IM_TYPING_START : IM_TYPING_STOP),
        session_id);
    gAgent.sendReliableMessage();
}

void LLIMModel::sendLeaveSession(const LLUUID& session_id, const LLUUID& other_participant_id)
{
    if(session_id.notNull())
    {
        std::string name;
        LLAgentUI::buildFullname(name);
        pack_instant_message(
            gMessageSystem,
            gAgent.getID(),
            false,
            gAgent.getSessionID(),
            other_participant_id,
            name,
            LLStringUtil::null,
            IM_ONLINE,
            IM_SESSION_LEAVE,
            session_id);
        gAgent.sendReliableMessage();
    }
}

//*TODO this method is better be moved to the LLIMMgr
void LLIMModel::sendMessage(const std::string& utf8_text,
                     const LLUUID& im_session_id,
                     const LLUUID& other_participant_id,
                     EInstantMessage dialog)
{
    std::string name;
    bool sent = false;
    LLAgentUI::buildFullname(name);

    const LLRelationship* info = NULL;
    info = LLAvatarTracker::instance().getBuddyInfo(other_participant_id);

    U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;
    // Old call to send messages to SLim client,  no longer supported.
    //if((offline == IM_OFFLINE) && (LLVoiceClient::getInstance()->isOnlineSIP(other_participant_id)))
    //{
    //  // User is online through the OOW connector, but not with a regular viewer.  Try to send the message via SLVoice.
    //  sent = LLVoiceClient::getInstance()->sendTextMessage(other_participant_id, utf8_text);
    //}

    if(!sent)
    {
        // Send message normally.

        // default to IM_SESSION_SEND unless it's nothing special - in
        // which case it's probably an IM to everyone.
        U8 new_dialog = dialog;

        if ( dialog != IM_NOTHING_SPECIAL )
        {
            new_dialog = IM_SESSION_SEND;
        }
        pack_instant_message(
            gMessageSystem,
            gAgent.getID(),
            false,
            gAgent.getSessionID(),
            other_participant_id,
            name.c_str(),
            utf8_text.c_str(),
            offline,
            (EInstantMessage)new_dialog,
            im_session_id);
        gAgent.sendReliableMessage();
    }

    bool is_group_chat = false;
    LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(im_session_id);
    if(session)
    {
        is_group_chat = session->isGroupSessionType();
    }

    // If there is a mute list and this is not a group chat...
    if ( LLMuteList::getInstance() && !is_group_chat)
    {
        // ... the target should not be in our mute list for some message types.
        // Auto-remove them if present.
        switch( dialog )
        {
        case IM_NOTHING_SPECIAL:
        case IM_GROUP_INVITATION:
        case IM_INVENTORY_OFFERED:
        case IM_SESSION_INVITE:
        case IM_SESSION_P2P_INVITE:
        case IM_SESSION_CONFERENCE_START:
        case IM_SESSION_SEND: // This one is marginal - erring on the side of hearing.
        case IM_LURE_USER:
        case IM_GODLIKE_LURE_USER:
        case IM_FRIENDSHIP_OFFERED:
            LLMuteList::getInstance()->autoRemove(other_participant_id, LLMuteList::AR_IM);
            break;
        default: ; // do nothing
        }
    }

    if((dialog == IM_NOTHING_SPECIAL) &&
       (other_participant_id.notNull()))
    {
        // Do we have to replace the /me's here?
        std::string from;
        LLAgentUI::buildFullname(from);
        LLIMModel::getInstance()->addMessage(im_session_id, from, gAgentID, utf8_text);

        //local echo for the legacy communicate panel
        std::string history_echo;
        LLAgentUI::buildFullname(history_echo);

        history_echo += ": " + utf8_text;

        LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(im_session_id);
        if (speaker_mgr)
        {
            speaker_mgr->speakerChatted(gAgentID);
            speaker_mgr->setSpeakerTyping(gAgentID, false);
        }
    }

    // Add the recipient to the recent people list.
    bool is_not_group_id = LLGroupMgr::getInstance()->getGroupData(other_participant_id) == NULL;

    if (is_not_group_id)
    {
        if( session == 0)//??? shouldn't really happen
        {
            LLRecentPeople::instance().add(other_participant_id);
            return;
        }
        // IM_SESSION_INVITE means that this is an Ad-hoc incoming chat
        //      (it can be also Group chat but it is checked above)
        // In this case mInitialTargetIDs contains Ad-hoc session ID and it should not be added
        // to Recent People to prevent showing of an item with (?? ?)(?? ?), sans the spaces. See EXT-8246.
        // Concrete participants will be added into this list once they sent message in chat.
        if (IM_SESSION_INVITE == dialog) return;

        if (IM_SESSION_CONFERENCE_START == dialog) // outgoing ad-hoc session
        {
            // Add only online members of conference to recent list (EXT-8658)
            addSpeakersToRecent(im_session_id);
        }
        else // outgoing P2P session
        {
            // Add the recepient of the session.
            if (!session->mInitialTargetIDs.empty())
            {
                LLRecentPeople::instance().add(*(session->mInitialTargetIDs.begin()));
            }
        }
    }
}

void LLIMModel::addSpeakersToRecent(const LLUUID& im_session_id)
{
    LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(im_session_id);
    LLSpeakerMgr::speaker_list_t speaker_list;
    if(speaker_mgr != NULL)
    {
        speaker_mgr->getSpeakerList(&speaker_list, true);
    }
    for(LLSpeakerMgr::speaker_list_t::iterator it = speaker_list.begin(); it != speaker_list.end(); it++)
    {
        const LLPointer<LLSpeaker>& speakerp = *it;
        LLRecentPeople::instance().add(speakerp->mID);
    }
}

void session_starter_helper(
    const LLUUID& temp_session_id,
    const LLUUID& other_participant_id,
    EInstantMessage im_type)
{
    LLMessageSystem *msg = gMessageSystem;

    msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

    msg->nextBlockFast(_PREHASH_MessageBlock);
    msg->addBOOLFast(_PREHASH_FromGroup, false);
    msg->addUUIDFast(_PREHASH_ToAgentID, other_participant_id);
    msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
    msg->addU8Fast(_PREHASH_Dialog, im_type);
    msg->addUUIDFast(_PREHASH_ID, temp_session_id);
    msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

    std::string name;
    LLAgentUI::buildFullname(name);

    msg->addStringFast(_PREHASH_FromAgentName, name);
    msg->addStringFast(_PREHASH_Message, LLStringUtil::null);
    msg->addU32Fast(_PREHASH_ParentEstateID, 0);
    msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
    msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
}

void start_deprecated_conference_chat(
    const LLUUID& temp_session_id,
    const LLUUID& creator_id,
    const LLUUID& other_participant_id,
    const LLSD& agents_to_invite)
{
    U8* bucket;
    U8* pos;
    size_t count;
    S32 bucket_size;

    // *FIX: this could suffer from endian issues
    count = agents_to_invite.size();
    bucket_size = UUID_BYTES * static_cast<S32>(count);
    bucket = new U8[bucket_size];
    pos = bucket;

    for(S32 i = 0; i < count; ++i)
    {
        LLUUID agent_id = agents_to_invite[i].asUUID();

        memcpy(pos, &agent_id, UUID_BYTES);
        pos += UUID_BYTES;
    }

    session_starter_helper(
        temp_session_id,
        other_participant_id,
        IM_SESSION_CONFERENCE_START);

    gMessageSystem->addBinaryDataFast(
        _PREHASH_BinaryBucket,
        bucket,
        bucket_size);

    gAgent.sendReliableMessage();

    delete[] bucket;
}

// Returns true if any messages were sent, false otherwise.
// Is sort of equivalent to "does the server need to do anything?"
bool LLIMModel::sendStartSession(
    const LLUUID& temp_session_id,
    const LLUUID& other_participant_id,
    const uuid_vec_t& ids,
    EInstantMessage dialog,
    bool p2p_as_adhoc_call)
{
    if ( dialog == IM_SESSION_GROUP_START )
    {
        session_starter_helper(
            temp_session_id,
            other_participant_id,
            dialog);
        gMessageSystem->addBinaryDataFast(
                _PREHASH_BinaryBucket,
                EMPTY_BINARY_BUCKET,
                EMPTY_BINARY_BUCKET_SIZE);
        gAgent.sendReliableMessage();

        return true;
    }
    else if (dialog == IM_SESSION_CONFERENCE_START )
    {
        LLSD agents;
        for (int i = 0; i < (S32) ids.size(); i++)
        {
            agents.append(ids[i]);
        }

        //we have a new way of starting conference calls now
        LLViewerRegion* region = gAgent.getRegion();
        if (region)
        {
            std::string url = region->getCapability(
                "ChatSessionRequest");

            LLCoros::instance().launch("startConferenceCoro",
                boost::bind(&startConferenceCoro, url,
                temp_session_id, gAgent.getID(), other_participant_id, agents));
        }
        else
        {
            start_deprecated_conference_chat(
                temp_session_id,
                gAgent.getID(),
                other_participant_id,
                agents);
        }

        //we also need to wait for reply from the server in case of ad-hoc chat (we'll get new session id)
        return true;
    }
    else if (p2p_as_adhoc_call && ((dialog == IM_SESSION_P2P_INVITE) || (dialog == IM_NOTHING_SPECIAL)))
    {
        LLViewerRegion *region = gAgent.getRegion();
        if (region)
        {
            std::string url = region->getCapability("ChatSessionRequest");
            LLCoros::instance().launch("startP2PVoiceCoro", boost::bind(&startP2PVoiceCoro, url, temp_session_id, gAgent.getID(), other_participant_id));
        }
        return true;
    }
    return false;
}


// the other_participant_id is either an agent_id, a group_id, or an inventory
// folder item_id (collection of calling cards)

// static
LLUUID LLIMMgr::computeSessionID(
    EInstantMessage dialog,
    const LLUUID& other_participant_id)
{
    LLUUID session_id;
    if (IM_SESSION_GROUP_START == dialog)
    {
        // slam group session_id to the group_id (other_participant_id)
        session_id = other_participant_id;
    }
    else if (IM_SESSION_CONFERENCE_START == dialog)
    {
        session_id.generate();
    }
    else if (IM_SESSION_INVITE == dialog)
    {
        // use provided session id for invites
        session_id = other_participant_id;
    }
    else
    {
        LLUUID agent_id = gAgent.getID();
        if (other_participant_id == agent_id)
        {
            // if we try to send an IM to ourselves then the XOR would be null
            // so we just make the session_id the same as the agent_id
            session_id = agent_id;
        }
        else
        {
            // peer-to-peer or peer-to-asset session_id is the XOR
            session_id = other_participant_id ^ agent_id;
        }
    }

    if (gAgent.isInGroup(session_id, true) && (session_id != other_participant_id))
    {
        LL_WARNS() << "Group session id different from group id: IM type = " << dialog << ", session id = " << session_id << ", group id = " << other_participant_id << LL_ENDL;
    }
    return session_id;
}

void
LLIMMgr::showSessionStartError(
    const std::string& error_string,
    const LLUUID session_id)
{
    if (!hasSession(session_id)) return;

    LLSD args;
    args["REASON"] = LLTrans::getString(error_string);
    args["RECIPIENT"] = LLIMModel::getInstance()->getName(session_id);

    LLSD payload;
    payload["session_id"] = session_id;

    LLNotificationsUtil::add(
        "ChatterBoxSessionStartError",
        args,
        payload,
        LLIMMgr::onConfirmForceCloseError);
}

void
LLIMMgr::showSessionEventError(
    const std::string& event_string,
    const std::string& error_string,
    const LLUUID session_id)
{
    LLSD args;
    LLStringUtil::format_map_t event_args;

    event_args["RECIPIENT"] = LLIMModel::getInstance()->getName(session_id);

    args["REASON"] =
        LLTrans::getString(error_string);
    args["EVENT"] =
        LLTrans::getString(event_string, event_args);

    LLNotificationsUtil::add(
        "ChatterBoxSessionEventError",
        args);
}

void
LLIMMgr::showSessionForceClose(
    const std::string& reason_string,
    const LLUUID session_id)
{
    if (!hasSession(session_id)) return;

    LLSD args;

    args["NAME"] = LLIMModel::getInstance()->getName(session_id);
    args["REASON"] = LLTrans::getString(reason_string);

    LLSD payload;
    payload["session_id"] = session_id;

    LLNotificationsUtil::add(
        "ForceCloseChatterBoxSession",
        args,
        payload,
        LLIMMgr::onConfirmForceCloseError);
}

//static
bool
LLIMMgr::onConfirmForceCloseError(
    const LLSD& notification,
    const LLSD& response)
{
    //only 1 option really
    LLUUID session_id = notification["payload"]["session_id"];

    LLFloater* floater = LLFloaterIMSession::findInstance(session_id);
    if ( floater )
    {
        floater->closeFloater(false);
    }
    return false;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCallDialogManager
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

LLCallDialogManager::LLCallDialogManager():
mPreviousSessionlName(""),
mCurrentSessionlName(""),
mSession(NULL),
mOldState(LLVoiceChannel::STATE_READY)
{
}

LLCallDialogManager::~LLCallDialogManager()
{
}

void LLCallDialogManager::initSingleton()
{
    mVoiceChannelChanged = LLVoiceChannel::setCurrentVoiceChannelChangedCallback(LLCallDialogManager::onVoiceChannelChanged);
}

// static
void LLCallDialogManager::onVoiceChannelChanged(const LLUUID &session_id)
{
    LLCallDialogManager::getInstance()->onVoiceChannelChangedInt(session_id);
}

void LLCallDialogManager::onVoiceChannelChangedInt(const LLUUID &session_id)
{
    LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
    if(!session)
    {
        mPreviousSessionlName = mCurrentSessionlName;
        mCurrentSessionlName = ""; // Empty string results in "Nearby Voice Chat" after substitution
        return;
    }

    mSession = session;

    static boost::signals2::connection prev_channel_state_changed_connection;
    // disconnect previously connected callback to avoid have invalid sSession in onVoiceChannelStateChanged()
    prev_channel_state_changed_connection.disconnect();
    prev_channel_state_changed_connection =
        mSession->mVoiceChannel->setStateChangedCallback(boost::bind(LLCallDialogManager::onVoiceChannelStateChanged, _1, _2, _3, _4));

    if(mCurrentSessionlName != session->mName)
    {
        mPreviousSessionlName = mCurrentSessionlName;
        mCurrentSessionlName = session->mName;
    }

    if (LLVoiceChannel::getCurrentVoiceChannel()->getState() == LLVoiceChannel::STATE_CALL_STARTED &&
        LLVoiceChannel::getCurrentVoiceChannel()->getCallDirection() == LLVoiceChannel::OUTGOING_CALL)
    {

        //*TODO get rid of duplicated code
        LLSD mCallDialogPayload;
        mCallDialogPayload["session_id"] = mSession->mSessionID;
        mCallDialogPayload["session_name"] = mSession->mName;
        mCallDialogPayload["other_user_id"] = mSession->mOtherParticipantID;
        mCallDialogPayload["old_channel_name"] = mPreviousSessionlName;
        mCallDialogPayload["state"] = LLVoiceChannel::STATE_CALL_STARTED;
        mCallDialogPayload["disconnected_channel_name"] = mSession->mName;
        mCallDialogPayload["session_type"] = mSession->mSessionType;

        LLOutgoingCallDialog* ocd = LLFloaterReg::getTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
        if(ocd)
        {
            ocd->show(mCallDialogPayload);
        }
    }

}

// static
void LLCallDialogManager::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction, bool ended_by_agent)
{
    LLCallDialogManager::getInstance()->onVoiceChannelStateChangedInt(old_state, new_state, direction, ended_by_agent);
}

void LLCallDialogManager::onVoiceChannelStateChangedInt(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state, const LLVoiceChannel::EDirection& direction, bool ended_by_agent)
{
    LLSD mCallDialogPayload;
    LLOutgoingCallDialog* ocd = NULL;

    if(mOldState == new_state)
    {
        return;
    }

    mOldState = new_state;

    mCallDialogPayload["session_id"] = mSession->mSessionID;
    mCallDialogPayload["session_name"] = mSession->mName;
    mCallDialogPayload["other_user_id"] = mSession->mOtherParticipantID;
    mCallDialogPayload["old_channel_name"] = mPreviousSessionlName;
    mCallDialogPayload["state"] = new_state;
    mCallDialogPayload["disconnected_channel_name"] = mSession->mName;
    mCallDialogPayload["session_type"] = mSession->mSessionType;
    mCallDialogPayload["ended_by_agent"] = ended_by_agent;

    switch(new_state)
    {
    case LLVoiceChannel::STATE_CALL_STARTED :
        // do not show "Calling to..." if it is incoming call
        if(direction == LLVoiceChannel::INCOMING_CALL)
        {
            return;
        }
        break;

    case LLVoiceChannel::STATE_NO_CHANNEL_INFO :
        // This will happen in p2p calls using the adhoc
        // infrastructure, which marks the channel as no channel info
        // after the call is closed, which forces a dialogue.
        return;

    case LLVoiceChannel::STATE_HUNG_UP:
        // this state is coming before session is changed
        break;

    case LLVoiceChannel::STATE_CONNECTED :
        ocd = LLFloaterReg::findTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
        if (ocd)
        {
            ocd->closeFloater();
        }
        return;

    default:
        break;
    }

    ocd = LLFloaterReg::getTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
    if(ocd)
    {
        ocd->show(mCallDialogPayload);
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LLCallDialog::LLCallDialog(const LLSD& payload)
    : LLDockableFloater(NULL, false, payload),

      mPayload(payload),
      mLifetime(DEFAULT_LIFETIME)
{
    setAutoFocus(false);
    // force docked state since this floater doesn't save it between recreations
    setDocked(true);
}

LLCallDialog::~LLCallDialog()
{
    LLUI::getInstance()->removePopup(this);
}

bool LLCallDialog::postBuild()
{
    if (!LLDockableFloater::postBuild() || !gToolBarView)
        return false;

    dockToToolbarButton("speak");

    return true;
}

void LLCallDialog::dockToToolbarButton(const std::string& toolbarButtonName)
{
    LLDockControl::DocAt dock_pos = getDockControlPos(toolbarButtonName);
    LLView *anchor_panel = gToolBarView->findChildView(toolbarButtonName);

    setUseTongue(anchor_panel);

    setDockControl(new LLDockControl(anchor_panel, this, getDockTongue(dock_pos), dock_pos));
}

LLDockControl::DocAt LLCallDialog::getDockControlPos(const std::string& toolbarButtonName)
{
    LLCommandId command_id(toolbarButtonName);
    S32 toolbar_loc = gToolBarView->hasCommand(command_id);

    LLDockControl::DocAt doc_at = LLDockControl::TOP;

    switch (toolbar_loc)
    {
        case LLToolBarEnums::TOOLBAR_LEFT:
            doc_at = LLDockControl::RIGHT;
            break;

        case LLToolBarEnums::TOOLBAR_RIGHT:
            doc_at = LLDockControl::LEFT;
            break;
    }

    return doc_at;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLOutgoingCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LLOutgoingCallDialog::LLOutgoingCallDialog(const LLSD& payload) :
LLCallDialog(payload)
{
    LLOutgoingCallDialog* instance = LLFloaterReg::findTypedInstance<LLOutgoingCallDialog>("outgoing_call", LLOutgoingCallDialog::OCD_KEY);
    if(instance && instance->getVisible())
    {
        instance->onCancel(instance);
    }
}

void LLCallDialog::draw()
{
    if (lifetimeHasExpired())
    {
        onLifetimeExpired();
    }

    if (getDockControl() != NULL)
    {
        LLDockableFloater::draw();
    }
}

// virtual
void LLCallDialog::onOpen(const LLSD& key)
{
    LLDockableFloater::onOpen(key);

    // it should be over the all floaters. EXT-5116
    LLUI::getInstance()->addPopup(this);
}

void LLCallDialog::setIcon(const LLSD& session_id, const LLSD& participant_id)
{
    bool participant_is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(session_id);

    bool is_group = participant_is_avatar && gAgent.isInGroup(session_id, true);

    LLAvatarIconCtrl* avatar_icon = getChild<LLAvatarIconCtrl>("avatar_icon");
    LLGroupIconCtrl* group_icon = getChild<LLGroupIconCtrl>("group_icon");

    avatar_icon->setVisible(!is_group);
    group_icon->setVisible(is_group);

    if (is_group)
    {
        group_icon->setValue(session_id);
    }
    else if (participant_is_avatar)
    {
        avatar_icon->setValue(participant_id);
    }
    else
    {
        LL_WARNS() << "Participant neither avatar nor group" << LL_ENDL;
        group_icon->setValue(session_id);
    }
}

bool LLCallDialog::lifetimeHasExpired()
{
    if (mLifetimeTimer.getStarted())
    {
        F32 elapsed_time = mLifetimeTimer.getElapsedTimeF32();
        if (elapsed_time > mLifetime)
        {
            return true;
        }
    }
    return false;
}

void LLCallDialog::onLifetimeExpired()
{
    mLifetimeTimer.stop();
    closeFloater();
}

void LLOutgoingCallDialog::show(const LLSD& key)
{
    mPayload = key;

    //will be false only if voice in parcel is disabled and channel we leave is nearby(checked further)
    bool show_oldchannel = LLViewerParcelMgr::getInstance()->allowAgentVoice();

    // hide all text at first
    hideAllText();

    // init notification's lifetime
    std::istringstream ss( getString("lifetime") );
    if (!(ss >> mLifetime))
    {
        mLifetime = DEFAULT_LIFETIME;
    }

    // customize text strings
    // tell the user which voice channel they are leaving
    if (!mPayload["old_channel_name"].asString().empty())
    {
        std::string old_caller_name = mPayload["old_channel_name"].asString();

        getChild<LLUICtrl>("leaving")->setTextArg("[CURRENT_CHAT]", old_caller_name);
        show_oldchannel = true;
    }
    else
    {
        getChild<LLUICtrl>("leaving")->setTextArg("[CURRENT_CHAT]", getString("localchat"));
    }

    if (!mPayload["disconnected_channel_name"].asString().empty())
    {
        std::string channel_name = mPayload["disconnected_channel_name"].asString();
        getChild<LLUICtrl>("nearby")->setTextArg("[VOICE_CHANNEL_NAME]", channel_name);

        // skipping "You will now be reconnected to nearby" in notification when call is ended by disabling voice,
        // so no reconnection to nearby chat happens (EXT-4397)
        bool voice_works = LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();
        std::string reconnect_nearby = voice_works ? LLTrans::getString("reconnect_nearby") : std::string();
        getChild<LLUICtrl>("nearby")->setTextArg("[RECONNECT_NEARBY]", reconnect_nearby);

        const std::string& nearby_str = mPayload["ended_by_agent"] ? NEARBY_P2P_BY_AGENT : NEARBY_P2P_BY_OTHER;
        getChild<LLUICtrl>(nearby_str)->setTextArg("[RECONNECT_NEARBY]", reconnect_nearby);
    }

    std::string callee_name = mPayload["session_name"].asString();

    if (callee_name == "anonymous") // obsolete? Likely was part of avaline support
    {
        callee_name = getString("anonymous");
    }

    LLSD callee_id = mPayload["other_user_id"];
    // Beautification:  Since you know who you called, just show display name
    std::string title = callee_name;
    std::string final_callee_name = callee_name;
    if (mPayload["session_type"].asInteger() == LLIMModel::LLIMSession::P2P_SESSION)
    {
        LLAvatarName av_name;
        if (LLAvatarNameCache::get(callee_id, &av_name))
        {
            final_callee_name = av_name.getDisplayName();
            title = av_name.getCompleteName();
        }
    }
    getChild<LLUICtrl>("calling")->setTextArg("[CALLEE_NAME]", final_callee_name);
    getChild<LLUICtrl>("connecting")->setTextArg("[CALLEE_NAME]", final_callee_name);

    setTitle(title);

    // for outgoing group calls callee_id == group id == session id
    setIcon(callee_id, callee_id);

    // stop timer by default
    mLifetimeTimer.stop();

    // show only necessary strings and controls
    switch(mPayload["state"].asInteger())
    {
    case LLVoiceChannel::STATE_CALL_STARTED :
        getChild<LLTextBox>("calling")->setVisible(true);
        getChild<LLButton>("Cancel")->setVisible(true);
        if(show_oldchannel)
        {
            getChild<LLTextBox>("leaving")->setVisible(true);
        }
        break;
    // STATE_READY is here to show appropriate text for ad-hoc and group calls when floater is shown(EXT-6893)
    case LLVoiceChannel::STATE_READY :
    case LLVoiceChannel::STATE_RINGING :
        if(show_oldchannel)
        {
            getChild<LLTextBox>("leaving")->setVisible(true);
        }
        getChild<LLTextBox>("connecting")->setVisible(true);
        break;
    case LLVoiceChannel::STATE_ERROR :
        getChild<LLTextBox>("noanswer")->setVisible(true);
        getChild<LLButton>("Cancel")->setVisible(false);
        setCanClose(true);
        mLifetimeTimer.start();
        break;
    case LLVoiceChannel::STATE_HUNG_UP :
        if (mPayload["session_type"].asInteger() == LLIMModel::LLIMSession::P2P_SESSION)
        {
            const std::string& nearby_str = mPayload["ended_by_agent"] ? NEARBY_P2P_BY_AGENT : NEARBY_P2P_BY_OTHER;
            getChild<LLTextBox>(nearby_str)->setVisible(true);
        }
        else
        {
            getChild<LLTextBox>("nearby")->setVisible(true);
        }
        getChild<LLButton>("Cancel")->setVisible(false);
        setCanClose(true);
        mLifetimeTimer.start();
    }

    openFloater(LLOutgoingCallDialog::OCD_KEY);
}

void LLOutgoingCallDialog::hideAllText()
{
    getChild<LLTextBox>("calling")->setVisible(false);
    getChild<LLTextBox>("leaving")->setVisible(false);
    getChild<LLTextBox>("connecting")->setVisible(false);
    getChild<LLTextBox>("nearby_P2P_by_other")->setVisible(false);
    getChild<LLTextBox>("nearby_P2P_by_agent")->setVisible(false);
    getChild<LLTextBox>("nearby")->setVisible(false);
    getChild<LLTextBox>("noanswer")->setVisible(false);
}

//static
void LLOutgoingCallDialog::onCancel(void* user_data)
{
    LLOutgoingCallDialog* self = (LLOutgoingCallDialog*)user_data;

    if (!gIMMgr)
        return;

    LLUUID session_id = self->mPayload["session_id"].asUUID();
    gIMMgr->endCall(session_id);

    self->closeFloater();
}


bool LLOutgoingCallDialog::postBuild()
{
    bool success = LLCallDialog::postBuild();

    childSetAction("Cancel", onCancel, this);

    setCanDrag(false);

    return success;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIncomingCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const std::array<std::string, 4> voice_call_types =
{
    "VoiceInviteP2P",
    "VoiceInviteGroup",
    "VoiceInviteAdHoc",
    "InviteAdHoc"
};

bool is_voice_call_type(const std::string &value)
{
    return std::find(voice_call_types.begin(), voice_call_types.end(), value) != voice_call_types.end();
}

LLIncomingCallDialog::LLIncomingCallDialog(const LLSD& payload) :
    LLCallDialog(payload),
    mAvatarNameCacheConnection()
{
}

void LLIncomingCallDialog::onLifetimeExpired()
{
    LLVoiceP2PIncomingCallInterfacePtr call = LLVoiceClient::getInstance()->getIncomingCallInterface(mPayload["voice_channel_info"]);
    if (call)
    {
        // restart notification's timer if call is still valid
        mLifetimeTimer.start();
    }
    else
    {
        // close invitation if call is already not valid
        mLifetimeTimer.stop();
        LLUUID session_id = mPayload["session_id"].asUUID();
        gIMMgr->clearPendingAgentListUpdates(session_id);
        gIMMgr->clearPendingInvitation(session_id);
        LLIncomingCallDialog::onReject(this);
    }
}

bool LLIncomingCallDialog::postBuild()
{
    LLCallDialog::postBuild();

    if (!mPayload.isMap() || mPayload.size() == 0)
    {
        LL_INFOS("IMVIEW") << "IncomingCall: invalid argument" << LL_ENDL;
        return true;
    }

    LLUUID session_id = mPayload["session_id"].asUUID();
    LLSD caller_id = mPayload["caller_id"];
    std::string caller_name = mPayload["caller_name"].asString();

    if (session_id.isNull() && caller_id.asUUID().isNull())
    {
        LL_INFOS("IMVIEW") << "IncomingCall: invalid ids" << LL_ENDL;
        return true;
    }

    std::string notify_box_type = mPayload["notify_box_type"].asString();
    if (!is_voice_call_type(notify_box_type))
    {
        LL_INFOS("IMVIEW") << "IncomingCall: notify_box_type was not provided" << LL_ENDL;
        return true;
    }

    // init notification's lifetime
    std::istringstream ss( getString("lifetime") );
    if (!(ss >> mLifetime))
    {
        mLifetime = DEFAULT_LIFETIME;
    }

    std::string call_type;
    if (gAgent.isInGroup(session_id, true))
    {
        LLStringUtil::format_map_t args;
        LLGroupData data;
        if (gAgent.getGroupData(session_id, data))
        {
            args["[GROUP]"] = data.mName;
            call_type = getString(notify_box_type, args);
        }
    }
    else
    {
        call_type = getString(notify_box_type);
    }

    if (caller_name == "anonymous") // obsolete?  Likely was part of avaline support
    {
        caller_name = getString("anonymous");
        setCallerName(caller_name, caller_name, call_type);
    }
    else
    {
        // Get the full name information
        if (mAvatarNameCacheConnection.connected())
        {
            mAvatarNameCacheConnection.disconnect();
        }
        mAvatarNameCacheConnection = LLAvatarNameCache::get(caller_id, boost::bind(&LLIncomingCallDialog::onAvatarNameCache, this, _1, _2, call_type));
    }

    setIcon(session_id, caller_id);

    childSetAction("Accept", onAccept, this);
    childSetAction("Reject", onReject, this);
    childSetAction("Start IM", onStartIM, this);
    setDefaultBtn("Accept");

    if(notify_box_type != "VoiceInviteGroup" && notify_box_type != "VoiceInviteAdHoc")
    {
        // starting notification's timer for P2P invitations
        mLifetimeTimer.start();
    }
    else
    {
        mLifetimeTimer.stop();
    }

    //it's not possible to connect to existing Ad-Hoc/Group chat through incoming ad-hoc call
    bool is_avatar = LLVoiceClient::getInstance()->isParticipantAvatar(session_id);
    getChildView("Start IM")->setVisible( is_avatar && notify_box_type != "VoiceInviteAdHoc" && notify_box_type != "VoiceInviteGroup");

    setCanDrag(false);
    return true;
}

void LLIncomingCallDialog::setCallerName(const std::string& ui_title,
                                         const std::string& ui_label,
                                         const std::string& call_type)
{

    // call_type may be a string like " is calling."
    LLUICtrl* caller_name_widget = getChild<LLUICtrl>("caller name");
    caller_name_widget->setValue(ui_label + " " + call_type);
}

void LLIncomingCallDialog::onAvatarNameCache(const LLUUID& agent_id,
                                             const LLAvatarName& av_name,
                                             const std::string& call_type)
{
    mAvatarNameCacheConnection.disconnect();
    std::string title = av_name.getCompleteName();
    setCallerName(title, av_name.getCompleteName(), call_type);
}

void LLIncomingCallDialog::onOpen(const LLSD& key)
{
    LLCallDialog::onOpen(key);

    if (gSavedSettings.getBOOL("PlaySoundIncomingVoiceCall"))
    {
        // play a sound for incoming voice call if respective property is set
        make_ui_sound("UISndStartIM");
    }

    LLStringUtil::format_map_t args;
    LLGroupData data;
    // if it's a group call, retrieve group name to use it in question
    if (gAgent.getGroupData(key["session_id"].asUUID(), data))
    {
        args["[GROUP]"] = data.mName;
    }
}

//static
void LLIncomingCallDialog::onAccept(void* user_data)
{
    LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
    processCallResponse(0, self->mPayload);
    self->closeFloater();
}

//static
void LLIncomingCallDialog::onReject(void* user_data)
{
    LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
    processCallResponse(1, self->mPayload);
    self->closeFloater();
}

//static
void LLIncomingCallDialog::onStartIM(void* user_data)
{
    LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
    processCallResponse(2, self->mPayload);
    self->closeFloater();
}

// static
void LLIncomingCallDialog::processCallResponse(S32 response, const LLSD &payload)
{
    if (!gIMMgr || gDisconnected)
        return;

    LLUUID session_id = payload["session_id"].asUUID();
    LLUUID caller_id = payload["caller_id"].asUUID();
    std::string session_name = payload["session_name"].asString();
    EInstantMessage type = (EInstantMessage)payload["type"].asInteger();
    LLIMMgr::EInvitationType inv_type = (LLIMMgr::EInvitationType)payload["inv_type"].asInteger();
    bool voice = true;
    switch(response)
    {
    case 2: // start IM: just don't start the voice chat
    {
        voice = false;
        /* FALLTHROUGH */
    }
    case 0: // accept
    {
        if (type == IM_SESSION_P2P_INVITE)
        {
            if (session_name.empty())
            {
                session_name = payload["caller_name"].asString();
            }
            // create a normal IM session
            session_id = gIMMgr->addP2PSession(
                session_name, caller_id, payload["voice_channel_info"]);

            if (voice)
            {
                gIMMgr->startCall(session_id, LLVoiceChannel::INCOMING_CALL, payload["voice_channel_info"]);
            }
            else
            {
                LLAvatarActions::startIM(caller_id);
            }

            gIMMgr->clearPendingAgentListUpdates(session_id);
            gIMMgr->clearPendingInvitation(session_id);
        }
        else
        {
            //session name should not be empty, but it can contain spaces so we don't trim
            std::string correct_session_name = session_name;
            if (session_name.empty())
            {
                LL_WARNS() << "Received an empty session name from a server" << LL_ENDL;

                switch(type){
                case IM_SESSION_CONFERENCE_START:
                case IM_SESSION_GROUP_START:
                case IM_SESSION_INVITE:
                    if (gAgent.isInGroup(session_id, true))
                    {
                        LLGroupData data;
                        if (!gAgent.getGroupData(session_id, data)) break;
                        correct_session_name = data.mName;
                    }
                    else
                    {
                        // *NOTE: really should be using callbacks here
                        LLAvatarName av_name;
                        if (LLAvatarNameCache::get(caller_id, &av_name))
                        {
                            correct_session_name = av_name.getCompleteName();
                            correct_session_name.append(ADHOC_NAME_SUFFIX);
                        }
                    }
                    LL_INFOS("IMVIEW") << "Corrected session name is " << correct_session_name << LL_ENDL;
                    break;
                default:
                    LL_WARNS("IMVIEW") << "Received an empty session name from a server and failed to generate a new proper session name" << LL_ENDL;
                    break;
                }
            }

            gIMMgr->addSession(correct_session_name, type, session_id, payload["voice_channel_info"]);

            std::string url = gAgent.getRegionCapability(
                "ChatSessionRequest");

            if (voice)
            {
                if(!url.empty())
                {
                    LLCoros::instance().launch("chatterBoxInvitationCoro",
                        boost::bind(&chatterBoxInvitationCoro, url, session_id, inv_type, payload["voice_channel_info"]));
                }

                // send notification message to the corresponding chat
                if (payload["notify_box_type"].asString() == "VoiceInviteGroup" || payload["notify_box_type"].asString() == "VoiceInviteAdHoc")
                {
                    LLStringUtil::format_map_t string_args;
                    string_args["[NAME]"] = payload["caller_name"].asString();
                    std::string message = LLTrans::getString("name_started_call", string_args);
                    LLIMModel::getInstance()->addMessageSilently(session_id, SYSTEM_FROM, LLUUID::null, message);
                }
            }
        }
        if (voice)
        {
            break;
        }
    }
    case 1: // decline
    {
        if (type == IM_SESSION_P2P_INVITE)
        {
            // decline p2p voice, either via the vivox-style call mechanism
            // or via the webrtc-style "decline p2p" mechanism.
            LLVoiceP2PIncomingCallInterfacePtr call = LLVoiceClient::getInstance()->getIncomingCallInterface(payload["voice_channel_info"]);
            if (call)
            {
                call->declineInvite();
            }
            else
            {
                // webrtc-style decline.
                LLViewerRegion *region = gAgent.getRegion();
                if (region)
                {
                    std::string url = region->getCapability("ChatSessionRequest");

                    LLSD data;
                    data["method"]     = "decline p2p voice";
                    data["session-id"] = session_id;

                    LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, data, "P2P declined", "P2P decline failed.");
                }
            }
        }
        else
        {
            LLViewerRegion *region = gAgent.getRegion();
            if (region)
            {
                std::string url = region->getCapability("ChatSessionRequest");

                LLSD data;
                data["method"]     = "decline invitation";
                data["session-id"] = session_id;

                LLCoreHttpUtil::HttpCoroutineAdapter::messageHttpPost(url, data, "Invitation declined", "Invitation decline failed.");
            }
        }
    }

    gIMMgr->clearPendingAgentListUpdates(session_id);
    gIMMgr->clearPendingInvitation(session_id);
    }
}

//
// Member Functions
//

LLIMMgr::LLIMMgr()
{
    mPendingInvitations = LLSD::emptyMap();
    mPendingAgentListUpdates = LLSD::emptyMap();

    LLIMModel::getInstance()->addNewMsgCallback(boost::bind(&LLFloaterIMSession::sRemoveTypingIndicator, _1));

    gSavedPerAccountSettings.declareBOOL("FetchGroupChatHistory", true, "Fetch recent messages from group chat servers when a group window opens", LLControlVariable::PERSIST_ALWAYS);
}

// Add a message to a session.
void LLIMMgr::addMessage(
    const LLUUID& session_id,
    const LLUUID& target_id,
    const std::string& from,
    const std::string& msg,
    bool  is_offline_msg,
    const std::string& session_name,
    EInstantMessage dialog,
    U32 parent_estate_id,
    const LLUUID& region_id,
    const LLVector3& position,
    bool is_region_msg,
    U32 timestamp,  // May be zero
    LLUUID display_id,
    std::string_view display_name)
{
    LLUUID other_participant_id = target_id;
    std::string message_display_name = (display_name.empty()) ? from : std::string(display_name);
    if (display_id.isNull() && (display_name.empty()))
    {
        display_id = other_participant_id;
    }

    LLUUID new_session_id = session_id;
    if (new_session_id.isNull())
    {
        //no session ID...compute new one
        new_session_id = computeSessionID(dialog, other_participant_id);
    }

    //*NOTE session_name is empty in case of incoming P2P sessions
    std::string fixed_session_name = from;
    bool name_is_setted = false;
    if(!session_name.empty() && session_name.size()>1)
    {
        fixed_session_name = session_name;
        name_is_setted = true;
    }
    bool skip_message = false;
    bool from_linden = LLMuteList::isLinden(from);
    if (gSavedPerAccountSettings.getBOOL("VoiceCallsFriendsOnly") && !from_linden)
    {
        // Evaluate if we need to skip this message when that setting is true (default is false)
        skip_message = (LLAvatarTracker::instance().getBuddyInfo(other_participant_id) == NULL);    // Skip non friends...
        skip_message &= !(other_participant_id == gAgentID);    // You are your best friend... Don't skip yourself
    }

    bool new_session = !hasSession(new_session_id);
    if (new_session)
    {
        // Group chat session was initiated by muted resident, do not start this session viewerside
        // do not send leave msg either, so we are able to get group messages from other participants
        if ((IM_SESSION_INVITE == dialog) && gAgent.isInGroup(new_session_id) &&
            LLMuteList::getInstance()->isMuted(other_participant_id, LLMute::flagTextChat) && !from_linden)
        {
            return;
        }

        LLAvatarName av_name;
        if (LLAvatarNameCache::get(other_participant_id, &av_name) && !name_is_setted)
        {
            fixed_session_name = av_name.getDisplayName();
        }
        LLIMModel::getInstance()->newSession(new_session_id, fixed_session_name, dialog, other_participant_id, LLSD(), is_offline_msg);

        LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(new_session_id);
        if (session)
        {
            skip_message &= !session->isGroupSessionType();         // Do not skip group chats...
            if (skip_message)
            {
                gIMMgr->leaveSession(new_session_id);
            }
            // When we get a new IM, and if you are a god, display a bit
            // of information about the source. This is to help liaisons
            // when answering questions.
            if (gAgent.isGodlike())
            {
                // *TODO:translate (low priority, god ability)
                std::ostringstream bonus_info;
                bonus_info << LLTrans::getString("***") + " " + LLTrans::getString("IMParentEstate") + ":" + " "
                    << parent_estate_id
                    << ((parent_estate_id == 1) ? "," + LLTrans::getString("IMMainland") : "")
                    << ((parent_estate_id == 5) ? "," + LLTrans::getString("IMTeen") : "");

                // once we have web-services (or something) which returns
                // information about a region id, we can print this out
                // and even have it link to map-teleport or something.
                //<< "*** region_id: " << region_id << std::endl
                //<< "*** position: " << position << std::endl;

                LLIMModel::instance().addMessage(new_session_id, from, other_participant_id, bonus_info.str(), true, is_region_msg);
            }

            // Logically it would make more sense to reject the session sooner, in another area of the
            // code, but the session has to be established inside the server before it can be left.
            if (LLMuteList::getInstance()->isMuted(other_participant_id, LLMute::flagTextChat) && !from_linden)
            {
                LL_WARNS() << "Leaving IM session from initiating muted resident " << from << LL_ENDL;
                if (!gIMMgr->leaveSession(new_session_id))
                {
                    LL_INFOS("IMVIEW") << "Session " << new_session_id << " does not exist." << LL_ENDL;
                }
                return;
            }

            // Fetch group chat history, enabled by default.
            if (gSavedPerAccountSettings.getBOOL("FetchGroupChatHistory"))
            {
                std::string chat_url = gAgent.getRegionCapability("ChatSessionRequest");
                if (!chat_url.empty())
                {
                    LLCoros::instance().launch("chatterBoxHistoryCoro", boost::bind(&chatterBoxHistoryCoro, chat_url, session_id, from, msg, timestamp));
                }
            }

            //Play sound for new conversations
            if (!skip_message && !gAgent.isDoNotDisturb() && (gSavedSettings.getBOOL("PlaySoundNewConversation")))
            {
                make_ui_sound("UISndNewIncomingIMSession");
            }
        }
        else
        {
            // Failed to create a session, most likely due to empty name (name cache failed?)
            LL_WARNS() << "Failed to create IM session " << fixed_session_name << LL_ENDL;
        }
    }

    if (!LLMuteList::getInstance()->isMuted(other_participant_id, LLMute::flagTextChat) && !skip_message)
    {
        LLIMModel::instance().addMessage(new_session_id, message_display_name, display_id, msg, true, is_region_msg, timestamp);
    }

    // Open conversation floater if offline messages are present
    if (is_offline_msg && !skip_message)
    {
        LLFloaterReg::showInstance("im_container");
        LLFloaterReg::getTypedInstance<LLFloaterIMContainer>("im_container")->
                flashConversationItemWidget(new_session_id, true);
    }
}

void LLIMMgr::addSystemMessage(const LLUUID& session_id, const std::string& message_name, const LLSD& args)
{
    LLUIString message;

    // null session id means near me (chat history)
    if (session_id.isNull())
    {
        message = LLTrans::getString(message_name);
        message.setArgs(args);

        LLChat chat(message);
        chat.mSourceType = CHAT_SOURCE_SYSTEM;

        LLFloaterIMNearbyChat* nearby_chat = LLFloaterReg::findTypedInstance<LLFloaterIMNearbyChat>("nearby_chat");
        if (nearby_chat)
        {
            nearby_chat->addMessage(chat);
        }
    }
    else // going to IM session
    {
        message = LLTrans::getString(message_name + "-im");
        message.setArgs(args);
        if (hasSession(session_id))
        {
            gIMMgr->addMessage(session_id, LLUUID::null, SYSTEM_FROM, message.getString());
        }
        // log message to file

        else
        {
            LLAvatarName av_name;
            // since we select user to share item with - his name is already in cache
            LLAvatarNameCache::get(args["user_id"], &av_name);
            std::string session_name = LLCacheName::buildUsername(av_name.getUserName());
            LLIMModel::instance().logToFile(session_name, SYSTEM_FROM, LLUUID::null, message.getString());
        }
    }
}

S32 LLIMMgr::getNumberOfUnreadIM()
{
    std::map<LLUUID, LLIMModel::LLIMSession*>::iterator it;

    S32 num = 0;
    for(it = LLIMModel::getInstance()->mId2SessionMap.begin(); it != LLIMModel::getInstance()->mId2SessionMap.end(); ++it)
    {
        num += (*it).second->mNumUnread;
    }

    return num;
}

S32 LLIMMgr::getNumberOfUnreadParticipantMessages()
{
    std::map<LLUUID, LLIMModel::LLIMSession*>::iterator it;

    S32 num = 0;
    for(it = LLIMModel::getInstance()->mId2SessionMap.begin(); it != LLIMModel::getInstance()->mId2SessionMap.end(); ++it)
    {
        num += (*it).second->mParticipantUnreadMessageCount;
    }

    return num;
}

void LLIMMgr::autoStartCallOnStartup(const LLUUID& session_id)
{
    LLIMModel::LLIMSession *session = LLIMModel::getInstance()->findIMSession(session_id);
    if (!session) return;

    if (session->mSessionInitialized)
    {
        startCall(session_id);
    }
    else
    {
        session->mStartCallOnInitialize = true;
    }
}

LLUUID LLIMMgr::addP2PSession(const std::string& name,
                              const LLUUID& other_participant_id,
                              const LLSD& voice_channel_info)
{
    LL_DEBUGS("Voice") << "Add p2p voice channel info: " << voice_channel_info << LL_ENDL;
    return addSession(name, IM_NOTHING_SPECIAL, other_participant_id, voice_channel_info);
}

// This adds a session to the talk view. The name is the local name of
// the session, dialog specifies the type of session. If the session
// exists, it is brought forward.  Specifying id = NULL results in an
// im session to everyone. Returns the uuid of the session.
LLUUID LLIMMgr::addSession(
    const std::string& name,
    EInstantMessage dialog,
    const LLUUID& other_participant_id,
    const LLSD& voiceChannelInfo)
{
    std::vector<LLUUID> ids;
    ids.push_back(other_participant_id);
    LLUUID session_id = addSession(name, dialog, other_participant_id, ids, voiceChannelInfo);
    return session_id;
}

// Adds a session using the given session_id.  If the session already exists
// the dialog type is assumed correct. Returns the uuid of the session.
LLUUID LLIMMgr::addSession(
    const std::string& name,
    EInstantMessage dialog,
    const LLUUID& other_participant_id,
    const std::vector<LLUUID>& ids,
    const LLSD& voiceChannelInfo,
    const LLUUID& floater_id)
{
    if (ids.empty())
    {
        return LLUUID::null;
    }

    if (name.empty())
    {
        LL_WARNS() << "Session name cannot be null!" << LL_ENDL;
        return LLUUID::null;
    }

    LLUUID session_id = computeSessionID(dialog,other_participant_id);

    if (floater_id.notNull())
    {
        LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(floater_id);

        if (im_floater)
        {
            // The IM floater should be initialized with a new session_id
            // so that it is found by that id when creating a chiclet in LLFloaterIMSession::onIMChicletCreated,
            // and a new floater is not created.
            im_floater->initIMSession(session_id);
            im_floater->reloadMessages();
        }
    }
    LLIMModel::LLIMSession *session = LLIMModel::getInstance()->findIMSession(session_id);

    bool new_session = (session == NULL);

    //works only for outgoing ad-hoc sessions
    if (new_session &&
        ((IM_NOTHING_SPECIAL == dialog) || (IM_SESSION_P2P_INVITE == dialog) || (IM_SESSION_CONFERENCE_START == dialog)) &&
        ids.size())
    {
        session = LLIMModel::getInstance()->findAdHocIMSession(ids);
        if (session)
        {
            new_session = false;
            session_id = session->mSessionID;
        }
    }

    //Notify observers that a session was added
    if (new_session)
    {
        LLIMModel::getInstance()->newSession(session_id, name, dialog, other_participant_id, ids, voiceChannelInfo);
    }
    //Notifies observers that the session was already added
    else
    {
        session->initVoiceChannel(voiceChannelInfo);
        std::string session_name = LLIMModel::getInstance()->getName(session_id);
        LLIMMgr::getInstance()->notifyObserverSessionActivated(session_id, session_name, other_participant_id);
    }

    //we don't need to show notes about online/offline, mute/unmute users' statuses for existing sessions
    if (!new_session) return session_id;

    LL_INFOS("IMVIEW") << "LLIMMgr::addSession, new session added, name = " << name << ", session id = " << session_id << LL_ENDL;

    //Per Plan's suggestion commented "explicit offline status warning" out to make Dessie happier (see EXT-3609)
    //*TODO After February 2010 remove this commented out line if no one will be missing that warning
    //noteOfflineUsers(session_id, floater, ids);

    // Only warn for regular IMs - not group IMs
    if( dialog == IM_NOTHING_SPECIAL )
    {
        noteMutedUsers(session_id, ids);
    }

    notifyObserverSessionVoiceOrIMStarted(session_id);

    return session_id;
}

bool LLIMMgr::leaveSession(const LLUUID& session_id)
{
    LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
    if (!im_session) return false;

    LLIMModel::getInstance()->sendLeaveSession(session_id, im_session->mOtherParticipantID);
    gIMMgr->removeSession(session_id);
    return true;
}

// Removes data associated with a particular session specified by session_id
void LLIMMgr::removeSession(const LLUUID& session_id)
{
    llassert_always(hasSession(session_id));

    clearPendingInvitation(session_id);
    clearPendingAgentListUpdates(session_id);

    LLIMModel::getInstance()->clearSession(session_id);

    LL_INFOS("IMVIEW") << "LLIMMgr::removeSession, session removed, session id = " << session_id << LL_ENDL;

    notifyObserverSessionRemoved(session_id);
}

void LLIMMgr::inviteToSession(
    const LLUUID& session_id,
    const std::string& session_name,
    const LLUUID& caller_id,
    const std::string& caller_name,
    EInstantMessage type,
    EInvitationType inv_type,
    const LLSD& voice_channel_info)
{

    if (caller_id == gAgentID)
    {
        // ignore invites from ourself.
        return;
    }

    std::string notify_box_type;
    // voice invite question is different from default only for group call (EXT-7118)
    std::string question_type = "VoiceInviteQuestionDefault";

    bool voice_invite = false;
    bool is_linden = LLMuteList::isLinden(caller_name);


    if(type == IM_SESSION_P2P_INVITE)
    {
        //P2P is different...they only have voice invitations
        notify_box_type = "VoiceInviteP2P";
        voice_invite = true;
    }
    else if ( gAgent.isInGroup(session_id, true) )
    {
        //only really old school groups have voice invitations
        notify_box_type = "VoiceInviteGroup";
        question_type = "VoiceInviteQuestionGroup";
        voice_invite = true;
    }
    else if ( inv_type == INVITATION_TYPE_VOICE )
    {
        //else it's an ad-hoc
        //and a voice ad-hoc
        notify_box_type = "VoiceInviteAdHoc";
        voice_invite = true;
    }
    else if ( inv_type == INVITATION_TYPE_IMMEDIATE )
    {
        notify_box_type = "InviteAdHoc";
    }

    LLSD payload;
    payload["session_id"] = session_id;
    payload["session_name"] = session_name;
    payload["caller_id"] = caller_id;
    payload["caller_name"] = caller_name;
    payload["type"] = type;
    payload["inv_type"] = inv_type;
    payload["notify_box_type"] = notify_box_type;
    payload["question_type"] = question_type;

    LL_WARNS("Voice") << "INVITE PAYLOAD: " << payload << LL_ENDL;

    //ignore invites from muted residents
    if (!is_linden)
    {
        if (LLMuteList::getInstance()->isMuted(caller_id, LLMute::flagVoiceChat)
            && voice_invite && "VoiceInviteQuestionDefault" == question_type)
        {
            LL_INFOS("IMVIEW") << "Rejecting voice call from initiating muted resident " << caller_name << LL_ENDL;
            payload["voice_channel_info"] = voice_channel_info;
            LLIncomingCallDialog::processCallResponse(1, payload);
            return;
        }
        else if (LLMuteList::getInstance()->isMuted(caller_id, LLMute::flagAll & ~LLMute::flagVoiceChat) && !voice_invite)
        {
            LL_INFOS("IMVIEW") << "Rejecting session invite from initiating muted resident " << caller_name << LL_ENDL;
            return;
        }
    }

    LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(session_id);
    if (channelp && channelp->callStarted())
    {
        // you have already started a call to the other user, so just accept the invite
        LLIncomingCallDialog::processCallResponse(0, payload);
        return;
    }
    if (voice_invite)
    {
        bool isRejectGroupCall = (gSavedSettings.getBOOL("VoiceCallsRejectGroup") && (notify_box_type == "VoiceInviteGroup"));
        bool isRejectNonFriendCall = (gSavedPerAccountSettings.getBOOL("VoiceCallsFriendsOnly") && (LLAvatarTracker::instance().getBuddyInfo(caller_id) == NULL));
        if  (isRejectGroupCall || isRejectNonFriendCall || gAgent.isDoNotDisturb())
        {
            if (gAgent.isDoNotDisturb() && !isRejectGroupCall && !isRejectNonFriendCall)
            {
                if (!hasSession(session_id) && (type == IM_SESSION_P2P_INVITE))
                {
                    std::string fixed_session_name = caller_name;
                    if(!session_name.empty() && session_name.size()>1)
                    {
                        fixed_session_name = session_name;
                    }
                    else
                    {
                        LLAvatarName av_name;
                        if (LLAvatarNameCache::get(caller_id, &av_name))
                        {
                            fixed_session_name = av_name.getDisplayName();
                        }
                    }
                    LLIMModel::getInstance()->newSession(session_id, fixed_session_name, IM_NOTHING_SPECIAL, caller_id, LLSD(), false);
                }

                LLSD args;
                addSystemMessage(session_id, "you_auto_rejected_call", args);
                send_do_not_disturb_message(gMessageSystem, caller_id, session_id);
            }
            // silently decline the call
            payload["voice_channel_info"] = voice_channel_info;
            LLIncomingCallDialog::processCallResponse(1, payload);
            return;
        }
    }

    if ( !mPendingInvitations.has(session_id.asString()) )
    {
        // we're throwing up a dialogue, so we're using the voice channel passed to us,
        // save it in the payload.
        payload["voice_channel_info"] = voice_channel_info;
        if (caller_name.empty())
        {
            LLAvatarNameCache::get(caller_id,
                boost::bind(&LLIMMgr::onInviteNameLookup, payload, _1, _2));
        }
        else
        {
            LLFloaterReg::showInstance("incoming_call", payload, false);
        }

        // Add the caller to the Recent List here (at this point
        // "incoming_call" floater is shown and the recipient can
        // reject the call), because even if a recipient will reject
        // the call, the caller should be added to the recent list
        // anyway. STORM-507.
        if(type == IM_SESSION_P2P_INVITE)
            LLRecentPeople::instance().add(caller_id);

        mPendingInvitations[session_id.asString()] = LLSD();
    }
}

void LLIMMgr::onInviteNameLookup(LLSD payload, const LLUUID& id, const LLAvatarName& av_name)
{
    payload["caller_name"] = av_name.getUserName();
    payload["session_name"] = payload["caller_name"].asString();

    std::string notify_box_type = payload["notify_box_type"].asString();

    LLFloaterReg::showInstance("incoming_call", payload, false);
}

//*TODO disconnects all sessions
void LLIMMgr::disconnectAllSessions()
{
    //*TODO disconnects all IM sessions
}

bool LLIMMgr::hasSession(const LLUUID& session_id)
{
    return LLIMModel::getInstance()->findIMSession(session_id) != NULL;
}

void LLIMMgr::clearPendingInvitation(const LLUUID& session_id)
{
    if ( mPendingInvitations.has(session_id.asString()) )
    {
        mPendingInvitations.erase(session_id.asString());
    }
}

void LLIMMgr::processAgentListUpdates(const LLUUID& session_id, const LLSD& body)
{
    if (body.isMap() && body.has("agent_updates") && body["agent_updates"].isMap())
    {
        LLSD::map_const_iterator update_it;
        for (update_it = body["agent_updates"].beginMap(); update_it != body["agent_updates"].endMap(); ++update_it)
        {
            LLUUID agent_id   = LLUUID(update_it->first);
            LLSD   agent_data = update_it->second;
            if (agent_data.has("transition") && agent_data["transition"].asString() == "LEAVE")
            {
                // ignore actual leaves as those will be handled separately.
                continue;
            }

            if (agent_id != gAgentID && agent_data.isMap() && agent_data.has("info") && agent_data["info"].isMap())
            {
                // Is one of the participants leaving a P2P Chat?
                if (agent_data["info"].has("can_voice_chat") && !agent_data["info"]["can_voice_chat"].asBoolean())
                {
                    LLVoiceChannelGroup *channelp = dynamic_cast < LLVoiceChannelGroup*>(LLVoiceChannel::getChannelByID(session_id));
                    if (channelp && channelp->isP2P())
                    {
                        // it's an adhoc-style P2P channel, and the peer has declined voice.  notify the user
                        // and shut down the voice channel.
                        LLSD notifyArgs = LLSD::emptyMap();
                        notifyArgs["VOICE_CHANNEL_NAME"]  = channelp->getSessionName();
                        LLNotificationsUtil::add("P2PCallDeclined", notifyArgs);
                        endCall(session_id);
                        break;
                    }
                }
            }
        }
    }

    LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(session_id);
    if ( im_floater )
    {
        im_floater->processAgentListUpdates(body);
    }
    LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
    if (speaker_mgr)
    {
        speaker_mgr->updateSpeakers(body);

        // also the same call is added into LLVoiceClient::participantUpdatedEvent because
        // sometimes it is called AFTER LLViewerChatterBoxSessionAgentListUpdates::post()
        // when moderation state changed too late. See EXT-3544.
        speaker_mgr->update(true);
    }
    else
    {
        //we don't have a speaker manager yet..something went wrong
        //we are probably receiving an update here before
        //a start or an acceptance of an invitation.  Race condition.
        gIMMgr->addPendingAgentListUpdates(
            session_id,
            body);
    }
}

LLSD LLIMMgr::getPendingAgentListUpdates(const LLUUID& session_id)
{
    if ( mPendingAgentListUpdates.has(session_id.asString()) )
    {
        return mPendingAgentListUpdates[session_id.asString()];
    }
    else
    {
        return LLSD();
    }
}

void LLIMMgr::addPendingAgentListUpdates(
    const LLUUID& session_id,
    const LLSD& updates)
{
    LLSD::map_const_iterator iter;

    if ( !mPendingAgentListUpdates.has(session_id.asString()) )
    {
        //this is a new agent list update for this session
        mPendingAgentListUpdates[session_id.asString()] = LLSD::emptyMap();
    }

    if (
        updates.has("agent_updates") &&
        updates["agent_updates"].isMap() &&
        updates.has("updates") &&
        updates["updates"].isMap() )
    {
        //new school update
        LLSD update_types = LLSD::emptyArray();
        LLSD::array_iterator array_iter;

        update_types.append("agent_updates");
        update_types.append("updates");

        for (
            array_iter = update_types.beginArray();
            array_iter != update_types.endArray();
            ++array_iter)
        {
            //we only want to include the last update for a given agent
            for (
                iter = updates[array_iter->asString()].beginMap();
                iter != updates[array_iter->asString()].endMap();
                ++iter)
            {
                mPendingAgentListUpdates[session_id.asString()][array_iter->asString()][iter->first] =
                    iter->second;
            }
        }
    }
    else if (
        updates.has("updates") &&
        updates["updates"].isMap() )
    {
        //old school update where the SD contained just mappings
        //of agent_id -> "LEAVE"/"ENTER"

        //only want to keep last update for each agent
        for (
            iter = updates["updates"].beginMap();
            iter != updates["updates"].endMap();
            ++iter)
        {
            mPendingAgentListUpdates[session_id.asString()]["updates"][iter->first] =
                iter->second;
        }
    }
}

void LLIMMgr::clearPendingAgentListUpdates(const LLUUID& session_id)
{
    if ( mPendingAgentListUpdates.has(session_id.asString()) )
    {
        mPendingAgentListUpdates.erase(session_id.asString());
    }
}

void LLIMMgr::notifyObserverSessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, bool has_offline_msg)
{
    for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
    {
        (*it)->sessionAdded(session_id, name, other_participant_id, has_offline_msg);
    }
}

void LLIMMgr::notifyObserverSessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
    for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
    {
        (*it)->sessionActivated(session_id, name, other_participant_id);
    }
}

void LLIMMgr::notifyObserverSessionVoiceOrIMStarted(const LLUUID& session_id)
{
    for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
    {
        (*it)->sessionVoiceOrIMStarted(session_id);
    }
}

void LLIMMgr::notifyObserverSessionRemoved(const LLUUID& session_id)
{
    for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
    {
        (*it)->sessionRemoved(session_id);
    }
}

void LLIMMgr::notifyObserverSessionIDUpdated( const LLUUID& old_session_id, const LLUUID& new_session_id )
{
    for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
    {
        (*it)->sessionIDUpdated(old_session_id, new_session_id);
    }

}

void LLIMMgr::addSessionObserver(LLIMSessionObserver *observer)
{
    mSessionObservers.push_back(observer);
}

void LLIMMgr::removeSessionObserver(LLIMSessionObserver *observer)
{
    mSessionObservers.remove(observer);
}

bool LLIMMgr::startCall(const LLUUID& session_id, LLVoiceChannel::EDirection direction, const LLSD& voice_channel_info)
{
    LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(session_id);
    if (!voice_channel) return false;
    if (voice_channel_info.isDefined() && voice_channel_info.isMap() && voice_channel_info.size() > 0)
    {
        voice_channel->setChannelInfo(voice_channel_info);
    }
    else if (voice_channel->getState() < LLVoiceChannel::STATE_READY)
    {
        // restart if there wa an error or it was hang up
        voice_channel->resetChannelInfo();
    }
    voice_channel->setCallDirection(direction);
    voice_channel->activate();
    return true;
}

bool LLIMMgr::endCall(const LLUUID& session_id)
{
    LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(session_id);
    if (!voice_channel) return false;

    voice_channel->deactivate();
    LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
    if (im_session)
    {
        // need to update speakers' state
        im_session->mSpeakers->update(false);
    }
    return true;
}

bool LLIMMgr::isVoiceCall(const LLUUID& session_id)
{
    LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
    if (!im_session) return false;

    return im_session->mStartedAsIMCall;
}

void LLIMMgr::updateDNDMessageStatus()
{
    if (LLIMModel::getInstance()->mId2SessionMap.empty()) return;

    std::map<LLUUID, LLIMModel::LLIMSession*>::const_iterator it = LLIMModel::getInstance()->mId2SessionMap.begin();
    for (; it != LLIMModel::getInstance()->mId2SessionMap.end(); ++it)
    {
        LLIMModel::LLIMSession* session = (*it).second;

        if (session->isP2P())
        {
            setDNDMessageSent(session->mSessionID,false);
        }
    }
}

bool LLIMMgr::isDNDMessageSend(const LLUUID& session_id)
{
    LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
    if (!im_session) return false;

    return im_session->mIsDNDsend;
}

void LLIMMgr::setDNDMessageSent(const LLUUID& session_id, bool is_send)
{
    LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
    if (!im_session) return;

    im_session->mIsDNDsend = is_send;
}

void LLIMMgr::addNotifiedNonFriendSessionID(const LLUUID& session_id)
{
    mNotifiedNonFriendSessions.insert(session_id);
}

bool LLIMMgr::isNonFriendSessionNotified(const LLUUID& session_id)
{
    return mNotifiedNonFriendSessions.end() != mNotifiedNonFriendSessions.find(session_id);

}

void LLIMMgr::noteOfflineUsers(
    const LLUUID& session_id,
    const std::vector<LLUUID>& ids)
{
    auto count = ids.size();
    if(count == 0)
    {
        const std::string& only_user = LLTrans::getString("only_user_message");
        LLIMModel::getInstance()->addMessage(session_id, SYSTEM_FROM, LLUUID::null, only_user);
    }
    else
    {
        const LLRelationship* info = NULL;
        LLAvatarTracker& at = LLAvatarTracker::instance();
        LLIMModel& im_model = LLIMModel::instance();
        for(S32 i = 0; i < count; ++i)
        {
            info = at.getBuddyInfo(ids.at(i));
            LLAvatarName av_name;
            if (info
                && !info->isOnline()
                && LLAvatarNameCache::get(ids.at(i), &av_name))
            {
                LLUIString offline = LLTrans::getString("offline_message");
                // Use display name only because this user is your friend
                offline.setArg("[NAME]", av_name.getDisplayName());
                im_model.proccessOnlineOfflineNotification(session_id, offline);
            }
        }
    }
}

void LLIMMgr::noteMutedUsers(const LLUUID& session_id,
                                  const std::vector<LLUUID>& ids)
{
    // Don't do this if we don't have a mute list.
    LLMuteList *ml = LLMuteList::getInstance();
    if( !ml )
    {
        return;
    }

    auto count = ids.size();
    if(count > 0)
    {
        LLIMModel* im_model = LLIMModel::getInstance();

        for(S32 i = 0; i < count; ++i)
        {
            if( ml->isMuted(ids.at(i)) )
            {
                LLUIString muted = LLTrans::getString("muted_message");

                im_model->addMessage(session_id, SYSTEM_FROM, LLUUID::null, muted);
                break;
            }
        }
    }
}

void LLIMMgr::processIMTypingStart(const LLUUID& from_id, const EInstantMessage im_type)
{
    processIMTypingCore(from_id, im_type, true);
}

void LLIMMgr::processIMTypingStop(const LLUUID& from_id, const EInstantMessage im_type)
{
    processIMTypingCore(from_id, im_type, false);
}

void LLIMMgr::processIMTypingCore(const LLUUID& from_id, const EInstantMessage im_type, bool typing)
{
    LLUUID session_id = computeSessionID(im_type, from_id);
    LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(session_id);
    if ( im_floater )
    {
        im_floater->processIMTyping(from_id, typing);
    }
}

class LLViewerChatterBoxSessionStartReply : public LLHTTPNode
{
public:
    virtual void describe(Description& desc) const
    {
        desc.shortInfo("Used for receiving a reply to a request to initialize an ChatterBox session");
        desc.postAPI();
        desc.input(
            "{\"client_session_id\": UUID, \"session_id\": UUID, \"success\" boolean, \"reason\": string");
        desc.source(__FILE__, __LINE__);
    }

    virtual void post(ResponsePtr response,
                      const LLSD& context,
                      const LLSD& input) const
    {
        if (LLApp::isExiting() || gDisconnected)
        {
            LL_DEBUGS("ChatHistory") << "Ignoring ChatterBox session, Shutting down" << LL_ENDL;
            return;
        }

        LLSD body;
        LLUUID temp_session_id;
        LLUUID session_id;
        bool success;

        body = input["body"];
        success = body["success"].asBoolean();
        temp_session_id = body["temp_session_id"].asUUID();

        if ( success )
        {
            session_id = body["session_id"].asUUID();

            LLIMModel::getInstance()->processSessionInitializedReply(temp_session_id, session_id);

            LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
            if (speaker_mgr)
            {
                speaker_mgr->setSpeakers(body);
                speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(session_id));
            }

            LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(session_id);
            if ( im_floater )
            {
                if ( body.has("session_info") )
                {
                    im_floater->processSessionUpdate(body["session_info"]);

                    // Send request for chat history, if enabled.
                    if (gSavedPerAccountSettings.getBOOL("FetchGroupChatHistory"))
                    {
                        std::string url = gAgent.getRegionCapability("ChatSessionRequest");
                        if (!url.empty())
                        {
                            LLCoros::instance().launch("chatterBoxHistoryCoro",
                                boost::bind(&chatterBoxHistoryCoro, url, session_id, "", "", 0));
                        }
                    }
                }
            }

            gIMMgr->clearPendingAgentListUpdates(session_id);
        }
        else
        {
            //throw an error dialog and close the temp session's floater
            gIMMgr->showSessionStartError(body["error"].asString(), temp_session_id);
        }

        gIMMgr->clearPendingAgentListUpdates(session_id);
    }
};

class LLViewerChatterBoxSessionEventReply : public LLHTTPNode
{
public:
    virtual void describe(Description& desc) const
    {
        desc.shortInfo("Used for receiving a reply to a ChatterBox session event");
        desc.postAPI();
        desc.input(
            "{\"event\": string, \"reason\": string, \"success\": boolean, \"session_id\": UUID");
        desc.source(__FILE__, __LINE__);
    }

    virtual void post(ResponsePtr response,
                      const LLSD& context,
                      const LLSD& input) const
    {
        LLUUID session_id;
        bool success;

        LLSD body = input["body"];
        success = body["success"].asBoolean();
        session_id = body["session_id"].asUUID();

        if ( !success )
        {
            //throw an error dialog
            gIMMgr->showSessionEventError(
                body["event"].asString(),
                body["error"].asString(),
                session_id);
        }
    }
};

class LLViewerForceCloseChatterBoxSession: public LLHTTPNode
{
public:
    virtual void post(ResponsePtr response,
                      const LLSD& context,
                      const LLSD& input) const
    {
        LLUUID session_id;
        std::string reason;

        session_id = input["body"]["session_id"].asUUID();
        reason = input["body"]["reason"].asString();

        gIMMgr->showSessionForceClose(reason, session_id);
    }
};

class LLViewerChatterBoxSessionAgentListUpdates : public LLHTTPNode
{
public:
    virtual void post(
        ResponsePtr responder,
        const LLSD& context,
        const LLSD& input) const
    {
        const LLUUID& session_id = input["body"]["session_id"].asUUID();
        gIMMgr->processAgentListUpdates(session_id, input["body"]);
    }
};

class LLViewerChatterBoxSessionUpdate : public LLHTTPNode
{
public:
    virtual void post(
        ResponsePtr responder,
        const LLSD& context,
        const LLSD& input) const
    {
        LLUUID session_id = input["body"]["session_id"].asUUID();
        LLFloaterIMSession* im_floater = LLFloaterIMSession::findInstance(session_id);
        if ( im_floater )
        {
            im_floater->processSessionUpdate(input["body"]["info"]);
        }
        LLIMSpeakerMgr* im_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
        if (im_mgr)
        {
            im_mgr->processSessionUpdate(input["body"]["info"]);
        }
        if (input["body"]["info"].has("voice_channel_info"))
        {
            // new voice channel info incoming, update and re-activate call
            // if currently in a call.
            LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(session_id);
            if (session)
            {
                if (session->mVoiceChannel && session->mVoiceChannel->callStarted())
                {
                    session->initVoiceChannel(input["body"]["info"]["voice_channel_info"]);
                    session->mVoiceChannel->activate();
                }
            }
        }
    }
};


class LLViewerChatterBoxInvitation : public LLHTTPNode
{
public:

    virtual void post(
        ResponsePtr response,
        const LLSD& context,
        const LLSD& input) const
    {
        //for backwards compatiblity reasons...we need to still
        //check for 'text' or 'voice' invitations...bleh
        if ( input["body"].has("instantmessage") )
        {
            LLSD message_params =
                input["body"]["instantmessage"]["message_params"];

            //do something here to have the IM invite behave
            //just like a normal IM
            //this is just replicated code from process_improved_im
            //and should really go in it's own function -jwolk

            std::string message = message_params["message"].asString();
            std::string name = message_params["from_name"].asString();
            LLUUID from_id = message_params["from_id"].asUUID();
            LLUUID session_id = message_params["id"].asUUID();
            std::vector<U8> bin_bucket = message_params["data"]["binary_bucket"].asBinary();
            U8 offline = (U8)message_params["offline"].asInteger();

            time_t timestamp =
                (time_t) message_params["timestamp"].asInteger();

            bool is_do_not_disturb = gAgent.isDoNotDisturb();

            //don't return if user is muted b/c proper way to ignore a muted user who
            //initiated an adhoc/group conference is to create then leave the session (see STORM-1731)
            if (is_do_not_disturb)
            {
                return;
            }

            // standard message, not from system
            std::string saved;
            if(offline == IM_OFFLINE)
            {
                LLStringUtil::format_map_t args;
                args["[LONG_TIMESTAMP]"] = formatted_time(timestamp);
                saved = LLTrans::getString("Saved_message", args);
            }
            std::string buffer = saved + message;

            if(from_id == gAgentID)
            {
                return;
            }
            gIMMgr->addMessage(
                session_id,
                from_id,
                name,
                buffer,
                IM_OFFLINE == offline,
                std::string((char*)&bin_bucket[0]),
                IM_SESSION_INVITE,
                message_params["parent_estate_id"].asInteger(),
                message_params["region_id"].asUUID(),
                ll_vector3_from_sd(message_params["position"]),
                false,      // is_region_message
                (U32)timestamp);

            if (LLMuteList::getInstance()->isMuted(from_id, name, LLMute::flagTextChat))
            {
                return;
            }

            //K now we want to accept the invitation
            std::string url = gAgent.getRegionCapability("ChatSessionRequest");

            if ( url != "" )
            {
                LLCoros::instance().launch("chatterBoxInvitationCoro",
                    boost::bind(&chatterBoxInvitationCoro, url,
                    session_id, LLIMMgr::INVITATION_TYPE_INSTANT_MESSAGE, LLSD()));
            }
        } //end if invitation has instant message
        else if ( input["body"].has("voice") )
        {
            if(!LLVoiceClient::getInstance()->voiceEnabled() || !LLVoiceClient::getInstance()->isVoiceWorking())
            {
                // Don't display voice invites unless the user has voice enabled.
                return;
            }

            bool session_type_p2p = input["body"]["voice"].get("invitation_type").asInteger() == EMultiAgentChatSessionType::P2P_CHAT_SESSION;
            LL_DEBUGS("Voice") << "Received voice information from the server: " << input["body"]<< LL_ENDL;
            gIMMgr->inviteToSession(
                input["body"]["session_id"].asUUID(),
                input["body"]["session_name"].asString(),
                input["body"]["from_id"].asUUID(),
                input["body"]["from_name"].asString(),
                session_type_p2p ? IM_SESSION_P2P_INVITE : IM_SESSION_INVITE,
                LLIMMgr::INVITATION_TYPE_VOICE,
                input["body"]["voice"]);
        }
        else if ( input["body"].has("immediate") )
        {
            gIMMgr->inviteToSession(
                input["body"]["session_id"].asUUID(),
                input["body"]["session_name"].asString(),
                input["body"]["from_id"].asUUID(),
                input["body"]["from_name"].asString(),
                IM_SESSION_INVITE,
                LLIMMgr::INVITATION_TYPE_IMMEDIATE);
        }
    }
};

LLHTTPRegistration<LLViewerChatterBoxSessionStartReply>
   gHTTPRegistrationMessageChatterboxsessionstartreply(
       "/message/ChatterBoxSessionStartReply");

LLHTTPRegistration<LLViewerChatterBoxSessionEventReply>
   gHTTPRegistrationMessageChatterboxsessioneventreply(
       "/message/ChatterBoxSessionEventReply");

LLHTTPRegistration<LLViewerForceCloseChatterBoxSession>
    gHTTPRegistrationMessageForceclosechatterboxsession(
        "/message/ForceCloseChatterBoxSession");

LLHTTPRegistration<LLViewerChatterBoxSessionAgentListUpdates>
    gHTTPRegistrationMessageChatterboxsessionagentlistupdates(
        "/message/ChatterBoxSessionAgentListUpdates");

LLHTTPRegistration<LLViewerChatterBoxSessionUpdate>
    gHTTPRegistrationMessageChatterBoxSessionUpdate(
        "/message/ChatterBoxSessionUpdate");

LLHTTPRegistration<LLViewerChatterBoxInvitation>
    gHTTPRegistrationMessageChatterBoxInvitation(
        "/message/ChatterBoxInvitation");

