/**
 * @file llvoicechannel.h
 * @brief Voice channel related classes
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

#ifndef LL_VOICECHANNEL_H
#define LL_VOICECHANNEL_H

#include "llhandle.h"
#include "llvoiceclient.h"
#include "lleventcoro.h"
#include "llcoros.h"

class LLPanel;

class LLVoiceChannel : public LLVoiceClientStatusObserver
{
public:
    typedef enum e_voice_channel_state
    {
        STATE_NO_CHANNEL_INFO,
        STATE_ERROR,
        STATE_HUNG_UP,
        STATE_READY,
        STATE_CALL_STARTED,
        STATE_RINGING,
        STATE_CONNECTED
    } EState;

    typedef enum e_voice_channel_direction
    {
        INCOMING_CALL,
        OUTGOING_CALL
    } EDirection;

    typedef boost::signals2::signal<void(const EState& old_state, const EState& new_state, const EDirection& direction, bool ended_by_agent, const LLUUID& session_id)> state_changed_signal_t;

    // on current channel changed signal
    typedef boost::function<void(const LLUUID& session_id)> channel_changed_callback_t;
    typedef boost::signals2::signal<void(const LLUUID& session_id)> channel_changed_signal_t;
    static channel_changed_signal_t sCurrentVoiceChannelChangedSignal;
    static boost::signals2::connection setCurrentVoiceChannelChangedCallback(channel_changed_callback_t cb, bool at_front = false);


    LLVoiceChannel(const LLUUID& session_id, const std::string& session_name);
    virtual ~LLVoiceChannel();

    virtual void onChange(EStatusType status, const LLSD& channelInfo, bool proximal);

    virtual void handleStatusChange(EStatusType status);
    virtual void handleError(EStatusType status);
    virtual void deactivate();
    virtual void activate();
    virtual void setChannelInfo(const LLSD& channelInfo);
    virtual void resetChannelInfo();
    virtual void requestChannelInfo();
    virtual bool isActive() const;
    virtual bool callStarted() const;

    // Session name is a UI label used for feedback about which person,
    // group, or phone number you are talking to
    const std::string& getSessionName() const { return mSessionName; }

    boost::signals2::connection setStateChangedCallback(const state_changed_signal_t::slot_type& callback)
    { return mStateChangedCallback.connect(callback); }

    const LLUUID getSessionID() { return mSessionID; }
    EState getState() { return mState; }

    void updateSessionID(const LLUUID& new_session_id);
    const LLSD& getNotifyArgs() { return mNotifyArgs; }

    void setCallDirection(EDirection direction) {mCallDirection = direction;}
    EDirection getCallDirection() {return mCallDirection;}

    bool isThisVoiceChannel(const LLSD &voiceChannelInfo) { return LLVoiceClient::getInstance()->compareChannels(mChannelInfo, voiceChannelInfo); }

    static LLVoiceChannel* getChannelByID(const LLUUID& session_id);
    static LLVoiceChannel* getCurrentVoiceChannel();

    static void initClass();

    static void suspend();
    static void resume();

  protected:
    virtual void setState(EState state);
    /**
     * Use this method if you want mStateChangedCallback to be executed while state is changed
     */
    void doSetState(const EState& state);

    // there can be two directions INCOMING and OUTGOING
    EDirection mCallDirection;

    LLUUID      mSessionID;
    EState      mState;
    std::string mSessionName;
    LLSD        mNotifyArgs;
    LLSD        mChannelInfo;
    // true if call was ended by agent
    bool mCallEndedByAgent;
    bool        mIgnoreNextSessionLeave;
    LLHandle<LLPanel> mLoginNotificationHandle;

    typedef std::map<LLUUID, LLVoiceChannel*> voice_channel_map_t;
    static voice_channel_map_t sVoiceChannelMap;

    static LLVoiceChannel* sProximalVoiceChannel;
    static LLVoiceChannel* sCurrentVoiceChannel;
    static LLVoiceChannel* sSuspendedVoiceChannel;
    static bool sSuspended;

private:
    state_changed_signal_t mStateChangedCallback;
};

class LLVoiceChannelGroup : public LLVoiceChannel
{
public:
    LLVoiceChannelGroup(const LLUUID& session_id,
        const std::string& session_name,
        bool is_p2p);

    void handleStatusChange(EStatusType status) override;
    void handleError(EStatusType status) override;
    void activate() override;
    void deactivate() override;
    void setChannelInfo(const LLSD &channelInfo) override;
    void requestChannelInfo() override;

    bool isP2P() { return mIsP2P; }

protected:
    void setState(EState state) override;

private:
    void voiceCallCapCoro(std::string url);

    U32 mRetries;
    bool mIsRetrying;
    bool mIsP2P;
};

class LLVoiceChannelProximal : public LLVoiceChannel, public LLSingleton<LLVoiceChannelProximal>
{
    LLSINGLETON(LLVoiceChannelProximal);
  public:

    void onChange(EStatusType status, const LLSD &channelInfo, bool proximal) override;
    void handleStatusChange(EStatusType status) override;
    void handleError(EStatusType status) override;
    bool isActive() const override;
    void activate() override;
    void deactivate() override;
};

class LLVoiceChannelP2P : public LLVoiceChannelGroup
{
  public:
    LLVoiceChannelP2P(const LLUUID      &session_id,
                      const std::string &session_name,
                      const LLUUID      &other_user_id,
                      LLVoiceP2POutgoingCallInterface * outgoing_call_interface);

    void handleStatusChange(EStatusType status) override;
    void handleError(EStatusType status) override;
    void activate() override;
    void requestChannelInfo() override;
    void deactivate() override;
    void setChannelInfo(const LLSD& channel_info) override;
    void resetChannelInfo() override;

  protected:
    void setState(EState state) override;

private:

    /**
    * Add the caller to the list of people with which we've recently interacted
    *
    **/
    void addToTheRecentPeopleList();
    LLUUID      mOtherUserID;
    bool        mReceivedCall;
    LLVoiceP2POutgoingCallInterface *mOutgoingCallInterface;
    LLVoiceP2PIncomingCallInterfacePtr mIncomingCallInterface;
};

#endif  // LL_VOICECHANNEL_H
