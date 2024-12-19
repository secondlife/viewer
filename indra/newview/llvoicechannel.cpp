/**
 * @file llvoicechannel.cpp
 * @brief Voice Channel related classes
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

#include "llagent.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanel.h"
#include "llrecentpeople.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llvoicechannel.h"
#include "llcorehttputil.h"

LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;
LLVoiceChannel* LLVoiceChannel::sSuspendedVoiceChannel = NULL;
LLVoiceChannel::channel_changed_signal_t LLVoiceChannel::sCurrentVoiceChannelChangedSignal;

bool LLVoiceChannel::sSuspended = false;

//
// Constants
//
const U32 DEFAULT_RETRIES_COUNT = 3;

//
// LLVoiceChannel
//
LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id, const std::string& session_name) :
    mSessionID(session_id),
    mState(STATE_NO_CHANNEL_INFO),
    mSessionName(session_name),
    mCallDirection(OUTGOING_CALL),
    mIgnoreNextSessionLeave(false),
    mCallEndedByAgent(false)
{
    mNotifyArgs["VOICE_CHANNEL_NAME"] = mSessionName;

    if (!sVoiceChannelMap.insert(std::make_pair(session_id, this)).second)
    {
        // a voice channel already exists for this session id, so this instance will be orphaned
        // the end result should simply be the failure to make voice calls
        LL_WARNS("Voice") << "Duplicate voice channels registered for session_id " << session_id << LL_ENDL;
    }
}

LLVoiceChannel::~LLVoiceChannel()
{
    if (sSuspendedVoiceChannel == this)
    {
        sSuspendedVoiceChannel = NULL;
    }
    if (sCurrentVoiceChannel == this)
    {
        sCurrentVoiceChannel = NULL;
    }
    LLVoiceClient::removeObserver(this);

    sVoiceChannelMap.erase(mSessionID);
}

void LLVoiceChannel::setChannelInfo(const LLSD &channelInfo)
{
    mChannelInfo     = channelInfo;

    if (mState == STATE_NO_CHANNEL_INFO)
    {
        if (mChannelInfo.isUndefined() || !mChannelInfo.isMap() || mChannelInfo.size() == 0)
        {
            LLNotificationsUtil::add("VoiceChannelJoinFailed", mNotifyArgs);
            LL_WARNS("Voice") << "Received empty channel info for channel " << mSessionName << LL_ENDL;
            deactivate();
        }
        else
        {
            setState(STATE_READY);

            // if we are supposed to be active, reconnect
            // this will happen on initial connect, as we request credentials on first use
            if (sCurrentVoiceChannel == this)
            {
                // just in case we got new channel info while active
                // should move over to new channel
                activate();
            }
        }
    }
}

void LLVoiceChannel::resetChannelInfo()
{
    mChannelInfo = LLSD();
    mState = STATE_NO_CHANNEL_INFO;
}

void LLVoiceChannel::onChange(EStatusType type, const LLSD& channelInfo, bool proximal)
{
    LL_DEBUGS("Voice") << "Incoming channel info: " << channelInfo << LL_ENDL;
    LL_DEBUGS("Voice") << "Current channel info: " << mChannelInfo << LL_ENDL;
    if (mChannelInfo.isUndefined() || (mChannelInfo.isMap() && mChannelInfo.size() == 0))
    {
        mChannelInfo = channelInfo;
    }
    if (!LLVoiceClient::getInstance()->compareChannels(mChannelInfo, channelInfo))
    {
        return;
    }

    if (type < BEGIN_ERROR_STATUS)
    {
        handleStatusChange(type);
    }
    else
    {
        handleError(type);
    }
}

void LLVoiceChannel::handleStatusChange(EStatusType type)
{
    // status updates
    switch(type)
    {
    case STATUS_LOGIN_RETRY:
        // no user notice
        break;
    case STATUS_LOGGED_IN:
        break;
    case STATUS_LEFT_CHANNEL:
        if (callStarted() && !sSuspended)
        {
            // if forceably removed from channel
            // update the UI and revert to default channel
            // deactivate will set the State to STATE_HUNG_UP
            // so when handleStatusChange is called again during
            // shutdown callStarted will return false and deactivate
            // won't be called again.
            deactivate();
        }
        break;
    case STATUS_JOINING:
        if (callStarted())
        {
            setState(STATE_RINGING);
        }
        break;
    case STATUS_JOINED:
        if (callStarted())
        {
            setState(STATE_CONNECTED);
        }
    default:
        break;
    }
}

// default behavior is to just deactivate channel
// derived classes provide specific error messages
void LLVoiceChannel::handleError(EStatusType type)
{
    deactivate();
    setState(STATE_ERROR);
}

bool LLVoiceChannel::isActive() const
{
    // only considered active when currently bound channel matches what our channel
    return callStarted() && LLVoiceClient::getInstance()->isCurrentChannel(mChannelInfo);
}

bool LLVoiceChannel::callStarted() const
{
    return mState >= STATE_CALL_STARTED;
}

void LLVoiceChannel::deactivate()
{
    if (mState >= STATE_RINGING)
    {
        // ignore session leave event
        mIgnoreNextSessionLeave = true;
    }

    if (callStarted())
    {
        setState(STATE_HUNG_UP);

        //Default mic is OFF when leaving voice calls
        if (gSavedSettings.getBOOL("AutoDisengageMic") &&
            sCurrentVoiceChannel == this &&
            LLVoiceClient::getInstance()->getUserPTTState())
        {
            gSavedSettings.setBOOL("PTTCurrentlyEnabled", true);
            LLVoiceClient::getInstance()->setUserPTTState(false);
        }
    }
    LLVoiceClient::removeObserver(this);

    if (sCurrentVoiceChannel == this)
    {
        // default channel is proximal channel
        sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
        sCurrentVoiceChannel->activate();
    }
}

void LLVoiceChannel::activate()
{
    if (callStarted())
    {
        return;
    }

    // deactivate old channel and mark ourselves as the active one
    if (sCurrentVoiceChannel != this)
    {
        // mark as current before deactivating the old channel to prevent
        // activating the proximal channel between IM calls
        LLVoiceChannel* old_channel = sCurrentVoiceChannel;
        sCurrentVoiceChannel = this;
        if (old_channel)
        {
            old_channel->deactivate();
        }
    }

    if (mState == STATE_NO_CHANNEL_INFO)
    {
        // responsible for setting status to active
        requestChannelInfo();
    }
    else
    {
        setState(STATE_CALL_STARTED);
    }

    LLVoiceClient::addObserver(this);

    //do not send earlier, channel should be initialized, should not be in STATE_NO_CHANNEL_INFO state
    sCurrentVoiceChannelChangedSignal(this->mSessionID);
}

void LLVoiceChannel::requestChannelInfo()
{
    // pretend we have everything we need
    if (sCurrentVoiceChannel == this)
    {
        setState(STATE_CALL_STARTED);
    }
}

//static
LLVoiceChannel* LLVoiceChannel::getChannelByID(const LLUUID& session_id)
{
    voice_channel_map_t::iterator found_it = sVoiceChannelMap.find(session_id);
    if (found_it == sVoiceChannelMap.end())
    {
        return NULL;
    }
    else
    {
        return found_it->second;
    }
}

LLVoiceChannel* LLVoiceChannel::getCurrentVoiceChannel()
{
    return sCurrentVoiceChannel;
}

void LLVoiceChannel::updateSessionID(const LLUUID& new_session_id)
{
    sVoiceChannelMap.erase(sVoiceChannelMap.find(mSessionID));
    mSessionID = new_session_id;
    sVoiceChannelMap.insert(std::make_pair(mSessionID, this));
}

void LLVoiceChannel::setState(EState state)
{
    switch(state)
    {
    case STATE_RINGING:
        //TODO: remove or redirect this call status notification
//      LLCallInfoDialog::show("ringing", mNotifyArgs);
        break;
    case STATE_CONNECTED:
        //TODO: remove or redirect this call status notification
//      LLCallInfoDialog::show("connected", mNotifyArgs);
        break;
    case STATE_HUNG_UP:
        //TODO: remove or redirect this call status notification
//      LLCallInfoDialog::show("hang_up", mNotifyArgs);
        break;
    default:
        break;
    }

    doSetState(state);
}

void LLVoiceChannel::doSetState(const EState& new_state)
{
    LL_DEBUGS("Voice") << "session '" << mSessionName << "' state " << mState << ", new_state " << new_state << ": "
        << (new_state == STATE_ERROR ? "ERROR" :
            new_state == STATE_HUNG_UP ? "HUNG_UP" :
            new_state == STATE_READY ? "READY" :
            new_state == STATE_CALL_STARTED ? "CALL_STARTED" :
            new_state == STATE_RINGING ? "RINGING" :
            new_state == STATE_CONNECTED ? "CONNECTED" :
            "NO_INFO")
        << LL_ENDL;

    EState old_state = mState;
    mState = new_state;

    if (!mStateChangedCallback.empty())
        mStateChangedCallback(old_state, mState, mCallDirection, mCallEndedByAgent, mSessionID);
}

//static
void LLVoiceChannel::initClass()
{
    sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
}

//static
void LLVoiceChannel::suspend()
{
    if (!sSuspended)
    {
        sSuspendedVoiceChannel = sCurrentVoiceChannel;
        sSuspended = true;
    }
}

//static
void LLVoiceChannel::resume()
{
    if (sSuspended)
    {
        if (LLVoiceClient::getInstance()->voiceEnabled())
        {
            if (sSuspendedVoiceChannel)
            {
                sSuspendedVoiceChannel->activate();
            }
            else
            {
                LLVoiceChannelProximal::getInstance()->activate();
            }
        }
        sSuspended = false;
    }
}

boost::signals2::connection LLVoiceChannel::setCurrentVoiceChannelChangedCallback(channel_changed_callback_t cb, bool at_front)
{
    if (at_front)
    {
        return sCurrentVoiceChannelChangedSignal.connect(cb,  boost::signals2::at_front);
    }
    else
    {
        return sCurrentVoiceChannelChangedSignal.connect(cb);
    }
}

//
// LLVoiceChannelGroup
//

LLVoiceChannelGroup::LLVoiceChannelGroup(const LLUUID      &session_id,
                                         const std::string &session_name,
                                         bool               is_p2p) :
                                         LLVoiceChannel(session_id, session_name),
                                         mIsP2P(is_p2p)
{
    mRetries = DEFAULT_RETRIES_COUNT;
    mIsRetrying = false;
}

void LLVoiceChannelGroup::deactivate()
{
    if (callStarted())
    {
        LLVoiceClient::getInstance()->leaveNonSpatialChannel();
    }
    LLVoiceChannel::deactivate();

    if (mIsP2P)
    {
        // void the channel info for p2p adhoc channels
        // so we request it again, hence throwing up the
        // connect dialogue on the other side.
        setState(STATE_NO_CHANNEL_INFO);
    }
 }

void LLVoiceChannelGroup::activate()
{
    if (callStarted()) return;

    LLVoiceChannel::activate();

    if (callStarted())
    {
        // we have the channel info, just need to use it now
        LLVoiceClient::getInstance()->setNonSpatialChannel(mChannelInfo,
                                                           mIsP2P && (mCallDirection == OUTGOING_CALL),
                                                           mIsP2P);

        if (mIsP2P)
        {
            LLIMModel::addSpeakersToRecent(mSessionID);
        }
        else
        {
            if (!gAgent.isInGroup(mSessionID))  // ad-hoc channel
            {
                LLIMModel::LLIMSession *session = LLIMModel::getInstance()->findIMSession(mSessionID);
                // Adding ad-hoc call participants to Recent People List.
                // If it's an outgoing ad-hoc, we can use mInitialTargetIDs that holds IDs of people we
                // called(both online and offline) as source to get people for recent (STORM-210).
                if (session && session->isOutgoingAdHoc())
                {
                    for (uuid_vec_t::iterator it = session->mInitialTargetIDs.begin(); it != session->mInitialTargetIDs.end(); ++it)
                    {
                        const LLUUID id = *it;
                        LLRecentPeople::instance().add(id);
                    }
                }
                // If this ad-hoc is incoming then trying to get ids of people from mInitialTargetIDs
                // would lead to EXT-8246. So in this case we get them from speakers list.
                else
                {
                    LLIMModel::addSpeakersToRecent(mSessionID);
                }
            }
        }

        // Mic default state is OFF on initiating/joining Ad-Hoc/Group calls.  It's on for P2P using the AdHoc infra.

        LLVoiceClient::getInstance()->setUserPTTState(mIsP2P);
    }
}

void LLVoiceChannelGroup::requestChannelInfo()
{
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        std::string url = region->getCapability("ChatSessionRequest");

        LLCoros::instance().launch("LLVoiceChannelGroup::voiceCallCapCoro",
            boost::bind(&LLVoiceChannelGroup::voiceCallCapCoro, this, url));
    }
}

void LLVoiceChannelGroup::setChannelInfo(const LLSD& channelInfo)
{
    mChannelInfo = channelInfo;

    if (mState == STATE_NO_CHANNEL_INFO)
    {
        if(mChannelInfo.isDefined() && mChannelInfo.isMap())
        {
            setState(STATE_READY);

            // if we are supposed to be active, reconnect
            // this will happen on initial connect, as we request credentials on first use
            if (sCurrentVoiceChannel == this)
            {
                // just in case we got new channel info while active
                // should move over to new channel
                activate();
            }
        }
        else
        {
            //*TODO: notify user
            LL_WARNS("Voice") << "Received invalid credentials for channel " << mSessionName << LL_ENDL;
            deactivate();
        }
    }
    else if ( mIsRetrying )
    {
        // we have the channel info, just need to use it now
        LLVoiceClient::getInstance()->setNonSpatialChannel(channelInfo,
                                                           mCallDirection == OUTGOING_CALL,
                                                           mIsP2P);
    }
}

void LLVoiceChannelGroup::handleStatusChange(EStatusType type)
{
    // status updates
    switch(type)
    {
    case STATUS_JOINED:
        mRetries = 3;
        mIsRetrying = false;
    default:
        break;
    }

    LLVoiceChannel::handleStatusChange(type);
}

void LLVoiceChannelGroup::handleError(EStatusType status)
{
    std::string notify;
    switch(status)
    {
    case ERROR_CHANNEL_LOCKED:
    case ERROR_CHANNEL_FULL:
        notify = "VoiceChannelFull";
        break;
    case ERROR_NOT_AVAILABLE:
        //clear URI and credentials
        //set the state to be no info
        //and activate
        if ( mRetries > 0 )
        {
            mRetries--;
            mIsRetrying = true;
            mIgnoreNextSessionLeave = true;

            requestChannelInfo();
            return;
        }
        else
        {
            notify = "VoiceChannelJoinFailed";
            mRetries = DEFAULT_RETRIES_COUNT;
            mIsRetrying = false;
        }

        break;

    case ERROR_UNKNOWN:
    default:
        break;
    }

    // notification
    if (!notify.empty())
    {
        LLNotificationPtr notification = LLNotificationsUtil::add(notify, mNotifyArgs);
        // echo to im window
        gIMMgr->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM, notification->getMessage());
    }

    LLVoiceChannel::handleError(status);
}

void LLVoiceChannelGroup::setState(EState state)
{
    switch(state)
    {
    case STATE_RINGING:
        if ( !mIsRetrying )
        {
            //TODO: remove or redirect this call status notification
//          LLCallInfoDialog::show("ringing", mNotifyArgs);
        }

        doSetState(state);
        break;
    default:
        LLVoiceChannel::setState(state);
    }
}

void LLVoiceChannelGroup::voiceCallCapCoro(std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("voiceCallCapCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD postData;
    postData["method"] = "call";
    postData["session-id"] = mSessionID;
    LLSD altParams;
    std::string  preferred_voice_server_type = gSavedSettings.getString("VoiceServerType");
    if (preferred_voice_server_type.empty())
    {
        // default to the server type associated with the region we're on.
        LLVoiceVersionInfo versionInfo = LLVoiceClient::getInstance()->getVersion();
        preferred_voice_server_type = versionInfo.internalVoiceServerType;
    }
    altParams["preferred_voice_server_type"] = preferred_voice_server_type;
    postData["alt_params"] = altParams;

    LL_INFOS("Voice", "voiceCallCapCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, postData);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(mSessionID);
    if (!channelp)
    {
        LL_WARNS("Voice") << "Unable to retrieve channel with Id = " << mSessionID << LL_ENDL;
        return;
    }

    if (!status)
    {
        if (status == LLCore::HttpStatus(HTTP_FORBIDDEN))
        {
            //403 == no ability
            LLNotificationsUtil::add(
                "VoiceNotAllowed",
                channelp->getNotifyArgs());
        }
        else
        {
            LLNotificationsUtil::add(
                "VoiceCallGenericError",
                channelp->getNotifyArgs());
        }
        channelp->deactivate();
        return;
    }

    result.erase(LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS);

    LLSD::map_const_iterator iter;
    for (iter = result.beginMap(); iter != result.endMap(); ++iter)
    {
        LL_DEBUGS("Voice") << "LLVoiceChannelGroup::voiceCallCapCoro got "
            << iter->first << LL_ENDL;
    }
    LL_INFOS("Voice") << "LLVoiceChannelGroup::voiceCallCapCoro got " << result << LL_ENDL;

    channelp->setChannelInfo(result["voice_credentials"]);
}


//
// LLVoiceChannelProximal
//
LLVoiceChannelProximal::LLVoiceChannelProximal() :
    LLVoiceChannel(LLUUID::null, LLStringUtil::null)
{
}

bool LLVoiceChannelProximal::isActive() const
{
    return callStarted() && LLVoiceClient::getInstance()->inProximalChannel();
}

void LLVoiceChannelProximal::activate()
{
    if (callStarted()) return;

    if((LLVoiceChannel::sCurrentVoiceChannel != this) && (LLVoiceChannel::getState() == STATE_CONNECTED))
    {
        // we're connected to a non-spatial channel, so disconnect.
        LLVoiceClient::getInstance()->leaveNonSpatialChannel();
    }

    LLVoiceClient::getInstance()->activateSpatialChannel(true);
    LLVoiceChannel::activate();
}

void LLVoiceChannelProximal::onChange(EStatusType type, const LLSD& channelInfo, bool proximal)
{
    if (!proximal)
    {
        return;
    }

    if (type < BEGIN_ERROR_STATUS)
    {
        handleStatusChange(type);
    }
    else
    {
        handleError(type);
    }
}

void LLVoiceChannelProximal::handleStatusChange(EStatusType status)
{
    // status updates
    switch(status)
    {
    case STATUS_LEFT_CHANNEL:
        // do not notify user when leaving proximal channel
        return;
    case STATUS_VOICE_DISABLED:
        LLVoiceClient::getInstance()->setUserPTTState(false);
        gAgent.setVoiceConnected(false);
        //skip showing "Voice not available at your current location" when agent voice is disabled (EXT-4749)
        if(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking())
        {
            //TODO: remove or redirect this call status notification
//          LLCallInfoDialog::show("unavailable", mNotifyArgs);
        }
        return;
    default:
        break;
    }
    LLVoiceChannel::handleStatusChange(status);
}


void LLVoiceChannelProximal::handleError(EStatusType status)
{
    std::string notify;
    switch(status)
    {
      case ERROR_CHANNEL_LOCKED:
      case ERROR_CHANNEL_FULL:
        notify = "ProximalVoiceChannelFull";
        break;
      default:
         break;
    }

    // notification
    if (!notify.empty())
    {
        LLNotificationsUtil::add(notify, mNotifyArgs);
    }

    // proximal voice remains up and the provider will try to reconnect.
}

void LLVoiceChannelProximal::deactivate()
{
    if (callStarted())
    {
        setState(STATE_HUNG_UP);
    }
    LLVoiceClient::removeObserver(this);
    LLVoiceClient::getInstance()->activateSpatialChannel(false);
}


//
// LLVoiceChannelP2P
//
LLVoiceChannelP2P::LLVoiceChannelP2P(const LLUUID      &session_id,
                                     const std::string &session_name,
                                     const LLUUID      &other_user_id,
                                    LLVoiceP2POutgoingCallInterface* outgoing_call_interface) :
    LLVoiceChannelGroup(session_id, session_name, true),
    mOtherUserID(other_user_id),
    mReceivedCall(false),
    mOutgoingCallInterface(outgoing_call_interface)
{
    mChannelInfo = LLVoiceClient::getInstance()->getP2PChannelInfoTemplate(other_user_id);
}

void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
    LL_INFOS("Voice") << "P2P CALL CHANNEL STATUS CHANGE: incoming=" << int(mReceivedCall) << " newstatus=" << LLVoiceClientStatusObserver::status2string(type) << " (mState=" << mState << ")" << LL_ENDL;

    // status updates
    switch(type)
    {
    case STATUS_LEFT_CHANNEL:
        if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
        {
            // *TODO: use it to show DECLINE voice notification
            if (mState == STATE_RINGING)
            {
                // other user declined call
                LLNotificationsUtil::add("P2PCallDeclined", mNotifyArgs);
            }
            else
            {
                // other user hung up, so we didn't end the call
                mCallEndedByAgent = false;
            }
            deactivate();
        }
        mIgnoreNextSessionLeave = false;
        return;
    case STATUS_JOINING:
        // because we join session we expect to process session leave event in the future. EXT-7371
        // may be this should be done in the LLVoiceChannel::handleStatusChange.
        mIgnoreNextSessionLeave = false;
        break;

    default:
        break;
    }

    LLVoiceChannel::handleStatusChange(type);
}

void LLVoiceChannelP2P::handleError(EStatusType type)
{
    switch(type)
    {
    case ERROR_NOT_AVAILABLE:
        LLNotificationsUtil::add("P2PCallNoAnswer", mNotifyArgs);
        break;
    default:
        break;
    }

    LLVoiceChannel::handleError(type);
}

void LLVoiceChannelP2P::activate()
{
    if (callStarted()) return;

    //call will be counted as ended by user unless this variable is changed in handleStatusChange()
    mCallEndedByAgent = true;

    LLVoiceChannel::activate();

    if (callStarted())
    {
        // no session handle yet, we're starting the call
        if (mIncomingCallInterface == nullptr)
        {
            mReceivedCall = false;
            mOutgoingCallInterface->callUser(mOtherUserID);
        }
        // otherwise answering the call
        else
        {
            if (!mIncomingCallInterface->answerInvite())
            {
                mCallEndedByAgent = false;
                mIncomingCallInterface.reset();
                handleError(ERROR_UNKNOWN);
                return;
            }
            // using the incoming call interface invalidates it.  Clear it out here so we can't reuse it by accident.
            mIncomingCallInterface.reset();
        }

        // Add the party to the list of people with which we've recently interacted.
        addToTheRecentPeopleList();

        //Default mic is ON on initiating/joining P2P calls
        if (!LLVoiceClient::getInstance()->getUserPTTState() && LLVoiceClient::getInstance()->getPTTIsToggle())
        {
            LLVoiceClient::getInstance()->inputUserControlState(true);
        }
    }
}

void LLVoiceChannelP2P::deactivate()
{
    if (callStarted())
    {
        mOutgoingCallInterface->hangup();
    }
    LLVoiceChannel::deactivate();
}


void LLVoiceChannelP2P::requestChannelInfo()
{
    // pretend we have everything we need, since P2P doesn't use channel info
    if (sCurrentVoiceChannel == this)
    {
        setState(STATE_CALL_STARTED);
    }
}

// receiving session from other user who initiated call
void LLVoiceChannelP2P::setChannelInfo(const LLSD& channel_info)
{
    mChannelInfo        = channel_info;
    bool needs_activate = false;
    if (callStarted())
    {
        // defer to lower agent id when already active
        if (mOtherUserID < gAgent.getID())
        {
            // pretend we haven't started the call yet, so we can connect to this session instead
            deactivate();
            needs_activate = true;
        }
        else
        {
            // we are active and have priority, invite the other user again
            // under the assumption they will join this new session
            mOutgoingCallInterface->callUser(mOtherUserID);
            return;
        }
    }

    mReceivedCall = true;
    if (channel_info.isDefined() && channel_info.isMap())
    {
        mIncomingCallInterface = LLVoiceClient::getInstance()->getIncomingCallInterface(channel_info);
    }
    if (needs_activate)
    {
        activate();
    }
}

void LLVoiceChannelP2P::resetChannelInfo()
{
    mChannelInfo = LLVoiceClient::getInstance()->getP2PChannelInfoTemplate(mOtherUserID);
    mState = STATE_NO_CHANNEL_INFO; // we have template, not full info
}

void LLVoiceChannelP2P::setState(EState state)
{
    LL_INFOS("Voice") << "P2P CALL STATE CHANGE: incoming=" << int(mReceivedCall) << " oldstate=" << mState << " newstate=" << state << LL_ENDL;

    if (mReceivedCall) // incoming call
    {
        // you only "answer" voice invites in p2p mode
        // so provide a special purpose message here
        if (mReceivedCall && state == STATE_RINGING)
        {
            //TODO: remove or redirect this call status notification
    //          LLCallInfoDialog::show("answering", mNotifyArgs);
            doSetState(state);
            return;
        }
    }

    LLVoiceChannel::setState(state);
}

void LLVoiceChannelP2P::addToTheRecentPeopleList()
{
    LLRecentPeople::instance().add(mOtherUserID);
}
