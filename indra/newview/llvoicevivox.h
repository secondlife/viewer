/**
 * @file llvoicevivox.h
 * @brief Declaration of LLDiamondwareVoiceClient class which is the interface to the voice client process.
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
#ifndef LL_VOICE_VIVOX_H
#define LL_VOICE_VIVOX_H

class LLVOAvatar;
class LLVivoxProtocolParser;

#include "lliopipe.h"
#include "llpumpio.h"
#include "llchainio.h"
#include "lliosocket.h"
#include "v3math.h"
#include "llframetimer.h"
#include "llviewerregion.h"
#include "llcallingcard.h"   // for LLFriendObserver
#include "lleventcoro.h"
#include "llcoros.h"
#include <queue>

#ifdef LL_USESYSTEMLIBS
# include "expat.h"
#else
# include "expat/expat.h"
#endif
#include "llvoiceclient.h"

class LLAvatarName;
class LLVivoxVoiceClientMuteListObserver;

extern const std::string VIVOX_VOICE_SERVER_TYPE;

class LLVivoxVoiceP2PIncomingCall : public LLVoiceP2PIncomingCallInterface
{
  public:
    LLVivoxVoiceP2PIncomingCall(const LLSD& call_info) : mCallInfo(call_info) {}
    ~LLVivoxVoiceP2PIncomingCall() override {}

    bool answerInvite() override;
    void declineInvite() override;

  protected:
    LLSD mCallInfo;
};

class LLVivoxVoiceClient :  public LLSingleton<LLVivoxVoiceClient>,
                            virtual public LLVoiceModuleInterface,
                            virtual public LLVoiceEffectInterface,
                            virtual public LLVoiceP2POutgoingCallInterface
{
    LLSINGLETON(LLVivoxVoiceClient);
    LOG_CLASS(LLVivoxVoiceClient);
    virtual ~LLVivoxVoiceClient();

public:
    /// @name LLVoiceModuleInterface virtual implementations
    ///  @see LLVoiceModuleInterface
    //@{
    void init(LLPumpIO *pump) override; // Call this once at application startup (creates connector)
    void terminate() override;  // Call this to clean up during shutdown

    const LLVoiceVersionInfo& getVersion() override;

    void updateSettings() override; // call after loading settings and whenever they change

    // Returns true if vivox has successfully logged in and is not in error state
    bool isVoiceWorking() const override;

    void setHidden(bool hidden) override;  // virtual

    /////////////////////
    /// @name Tuning
    //@{
    void tuningStart() override;
    void tuningStop() override;
    bool inTuningMode() override;

    void tuningSetMicVolume(float volume) override;
    void tuningSetSpeakerVolume(float volume) override;
    float tuningGetEnergy(void) override;

    //@}

    /////////////////////
    /// @name Devices
    //@{
    // This returns true when it's safe to bring up the "device settings" dialog in the prefs.
    // i.e. when the daemon is running and connected, and the device lists are populated.
    bool deviceSettingsAvailable() override;
    bool deviceSettingsUpdated() override;  //return if the list has been updated and never fetched,  only to be called from the voicepanel.

    // Requery the vivox daemon for the current list of input/output devices.
    // If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
    // (use this if you want to know when it's done).
    // If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
    void refreshDeviceLists(bool clearCurrentList = true) override;

    void setCaptureDevice(const std::string& name) override;
    void setRenderDevice(const std::string& name) override;

    bool isCaptureNoDevice() override { return false; };
    bool isRenderNoDevice() override { return false; };

    LLVoiceDeviceList& getCaptureDevices() override;
    LLVoiceDeviceList& getRenderDevices() override;
    //@}

    void getParticipantList(std::set<LLUUID> &participants) override;
    bool isParticipant(const LLUUID& speaker_id) override;

    // Send a text message to the specified user, initiating the session if necessary.
    // virtual bool sendTextMessage(const LLUUID& participant_id, const std::string& message) const {return false;};

    // Returns true if calling back the session URI after the session has closed is possible.
    // Currently this will be false only for PSTN P2P calls.
    // NOTE: this will return true if the session can't be found.
    bool isSessionCallBackPossible(const LLUUID &session_id) override;

    // Returns true if the session can accepte text IM's.
    // Currently this will be false only for PSTN P2P calls.
    // NOTE: this will return true if the session can't be found.
    bool isSessionTextIMPossible(const LLUUID &session_id) override;

    ////////////////////////////
    /// @name Channel stuff
    //@{
    // returns true iff the user is currently in a proximal (local spatial) channel.
    // Note that gestures should only fire if this returns true.
    bool inProximalChannel() override;

    void setNonSpatialChannel(const LLSD& channelInfo,
                              bool notify_on_first_join,
                              bool hangup_on_last_leave) override;

    bool setSpatialChannel(const LLSD& channelInfo) override;

    void leaveNonSpatialChannel() override;

    void processChannels(bool process) override;

    void leaveChannel(void);

    bool isCurrentChannel(const LLSD &channelInfo) override;
    bool compareChannels(const LLSD &channelInfo1, const LLSD &channelInfo2) override;

    //@}


    //////////////////////////
    /// @name LLVoiceP2POutgoingCallInterface
    //@{
    // start a voice channel with the specified user
    void callUser(const LLUUID &uuid) override;
    void hangup() override;

    //@}

    LLVoiceP2POutgoingCallInterface *getOutgoingCallInterface() override { return this; }

    LLVoiceP2PIncomingCallInterfacePtr getIncomingCallInterface(const LLSD &voice_call_info) override;

    bool answerInvite(const std::string &sessionHandle);
    void declineInvite(const std::string &sessionHandle);

    /////////////////////////
    /// @name Volume/gain
    //@{
    void setVoiceVolume(F32 volume) override;
    void setMicGain(F32 volume) override;
    //@}

    /////////////////////////
    /// @name enable disable voice and features
    //@{
    void setVoiceEnabled(bool enabled) override;
    void setMuteMic(bool muted) override;       // Set the mute state of the local mic.
    //@}

    //////////////////////////
    /// @name nearby speaker accessors
    //@{
    std::string getDisplayName(const LLUUID& id) override;
    bool isParticipantAvatar(const LLUUID &id) override;
    bool getIsSpeaking(const LLUUID& id) override;
    bool getIsModeratorMuted(const LLUUID& id) override;
    F32 getCurrentPower(const LLUUID& id) override;     // "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
    F32 getUserVolume(const LLUUID& id) override;
    void setUserVolume(const LLUUID& id, F32 volume) override; // set's volume for specified agent, from 0-1 (where .5 is nominal)
    //@}

    // authorize the user
    void userAuthorized(const std::string& user_id,
                        const LLUUID &agentID) override;

    //////////////////////////////
    /// @name Status notification
    //@{
    void addObserver(LLVoiceClientStatusObserver* observer) override;
    void removeObserver(LLVoiceClientStatusObserver* observer) override;
    void addObserver(LLFriendObserver* observer) override;
    void removeObserver(LLFriendObserver* observer) override;
    void addObserver(LLVoiceClientParticipantObserver* observer) override;
    void removeObserver(LLVoiceClientParticipantObserver* observer) override;
    //@}

    std::string sipURIFromID(const LLUUID &id) const override;
    LLSD getP2PChannelInfoTemplate(const LLUUID& id) const override;
    //@}

    /// @name LLVoiceEffectInterface virtual implementations
    ///  @see LLVoiceEffectInterface
    //@{

    //////////////////////////
    /// @name Accessors
    //@{
    bool setVoiceEffect(const LLUUID& id) override;
    const LLUUID getVoiceEffect() override;
    LLSD getVoiceEffectProperties(const LLUUID& id) override;

    void refreshVoiceEffectLists(bool clear_lists) override;
    const voice_effect_list_t& getVoiceEffectList() const override;
    const voice_effect_list_t& getVoiceEffectTemplateList() const override;
    //@}

    //////////////////////////////
    /// @name Status notification
    //@{
    void addObserver(LLVoiceEffectObserver* observer) override;
    void removeObserver(LLVoiceEffectObserver* observer) override;
    //@}

    //////////////////////////////
    /// @name Effect preview buffer
    //@{
    void enablePreviewBuffer(bool enable) override;
    void recordPreviewBuffer() override;
    void playPreviewBuffer(const LLUUID& effect_id = LLUUID::null) override;
    void stopPreviewBuffer() override;

    bool isPreviewRecording() override;
    bool isPreviewPlaying() override;
    //@}

    //@}

    bool onCheckVoiceEffect(const std::string& voice_effect_name);
    void onClickVoiceEffect(const std::string& voice_effect_name);

protected:
    //////////////////////
    // Vivox Specific definitions

    friend class LLVivoxVoiceClientMuteListObserver;
    friend class LLVivoxVoiceClientFriendsObserver;


    enum streamState
    {
        streamStateUnknown = 0,
        streamStateIdle = 1,
        streamStateConnected = 2,
        streamStateRinging = 3,
        streamStateConnecting = 6,  // same as Vivox session_media_connecting enum
        streamStateDisconnecting = 7,  //Same as Vivox session_media_disconnecting enum
    };

    struct participantState
    {
    public:
        participantState(const std::string &uri);

            bool updateMuteState(); // true if mute state has changed
        bool isAvatar();

        std::string mURI;
        LLUUID mAvatarID;
        std::string mAccountName;
        std::string mDisplayName;
        LLFrameTimer mSpeakingTimeout;
        F32 mLastSpokeTimestamp;
        F32 mPower;
        F32 mVolume;
        std::string mGroupID;
        int mUserVolume;
        bool mPTT;
        bool mIsSpeaking;
        bool mIsModeratorMuted;
        bool mOnMuteList;       // true if this avatar is on the user's mute list (and should be muted)
        bool mVolumeSet;        // true if incoming volume messages should not change the volume
        bool mVolumeDirty;      // true if this participant needs a volume command sent (either mOnMuteList or mUserVolume has changed)
        bool mAvatarIDValid;
        bool mIsSelf;
    };
    typedef std::shared_ptr<participantState> participantStatePtr_t;
    typedef std::weak_ptr<participantState> participantStateWptr_t;

    typedef std::map<const std::string, participantStatePtr_t> participantMap;
    typedef std::map<const LLUUID, participantStatePtr_t> participantUUIDMap;

    struct sessionState
    {
    public:
        typedef std::shared_ptr<sessionState> ptr_t;
        typedef std::weak_ptr<sessionState> wptr_t;

        typedef std::function<void(const ptr_t &)> sessionFunc_t;

        static ptr_t createSession();
        ~sessionState();

        LLSD getVoiceChannelInfo();

        participantStatePtr_t addParticipant(const std::string &uri);
        void removeParticipant(const participantStatePtr_t &participant);
        void removeAllParticipants();

        participantStatePtr_t findParticipant(const std::string &uri);
        participantStatePtr_t findParticipantByID(const LLUUID& id);

        static ptr_t matchSessionByHandle(const std::string &handle);
        static ptr_t matchCreatingSessionByURI(const std::string &uri);
        static ptr_t matchSessionByURI(const std::string &uri);
        static ptr_t matchSessionByParticipant(const LLUUID &participant_id);

        bool isCallBackPossible();
        bool isTextIMPossible();
        bool isSpatial() { return mIsSpatial; }

        static void for_each(sessionFunc_t func);

        std::string mHandle;
        std::string mGroupHandle;
        std::string mSIPURI;
        std::string mAlias;
        std::string mName;
        std::string mAlternateSIPURI;
        std::string mHash;          // Channel password
        std::string mErrorStatusString;
        std::queue<std::string> mTextMsgQueue;

        LLUUID      mIMSessionID;
        LLUUID      mCallerID;
        int         mErrorStatusCode;
        int         mMediaStreamState;
        bool        mCreateInProgress;  // True if a Session.Create has been sent for this session and no response has been received yet.
        bool        mMediaConnectInProgress;    // True if a Session.MediaConnect has been sent for this session and no response has been received yet.
        bool        mVoiceInvitePending;    // True if a voice invite is pending for this session (usually waiting on a name lookup)
        bool        mTextInvitePending;     // True if a text invite is pending for this session (usually waiting on a name lookup)
        bool        mSynthesizedCallerID;   // True if the caller ID is a hash of the SIP URI -- this means we shouldn't do a name lookup.
        bool        mIsChannel; // True for both group and spatial channels (false for p2p, PSTN)
        bool        mIsSpatial; // True for spatial channels
        bool        mIsP2P;
        bool        mIncoming;
        bool        mVoiceActive;
        bool        mReconnect; // Whether we should try to reconnect to this session if it's dropped

        // Set to true when the volume/mute state of someone in the participant list changes.
        // The code will have to walk the list to find the changed participant(s).
        bool        mVolumeDirty;
        bool        mMuteDirty;

        bool        mParticipantsChanged;
        participantMap mParticipantsByURI;
        participantUUIDMap mParticipantsByUUID;

        LLUUID      mVoiceFontID;

        static void VerifySessions();

    private:
        sessionState();

        static std::set<wptr_t, std::owner_less<wptr_t>> mSession;   // canonical list of outstanding sessions.
        std::set<wptr_t>::iterator  mMyIterator;    // used for delete

        static void for_eachPredicate(const wptr_t &a, sessionFunc_t func);

        static bool testByHandle(const LLVivoxVoiceClient::sessionState::wptr_t &a, std::string handle);
        static bool testByCreatingURI(const LLVivoxVoiceClient::sessionState::wptr_t &a, std::string uri);
        static bool testBySIPOrAlterateURI(const LLVivoxVoiceClient::sessionState::wptr_t &a, std::string uri);
        static bool testByCallerId(const LLVivoxVoiceClient::sessionState::wptr_t &a, LLUUID participantId);

    };
    typedef std::shared_ptr<sessionState> sessionStatePtr_t;

    typedef std::map<std::string, sessionStatePtr_t> sessionMap;

    ///////////////////////////////////////////////////////
    // Private Member Functions
    //////////////////////////////////////////////////////



    //////////////////////////////
    /// @name TVC/Server management and communication
    //@{
    // Call this if the connection to the daemon terminates unexpectedly.  It will attempt to reset everything and relaunch.
    void daemonDied();

    // Call this if we're just giving up on voice (can't provision an account, etc.).  It will clean up and go away.
    void giveUp();

    // write to the tvc
    bool writeString(const std::string &str);

    void connectorCreate();
    void connectorShutdown();
    void closeSocket(void);

//  void requestVoiceAccountProvision(S32 retries = 3);
    void setLoginInfo(
               const std::string& account_name,
               const std::string& password,
               const std::string& voice_sip_uri_hostname,
               const std::string& voice_account_server_uri);
    void loginSendMessage();
    void logout();
    void logoutSendMessage();


    //@}

    //------------------------------------
    // tuning

    void tuningRenderStartSendMessage(const std::string& name, bool loop);
    void tuningRenderStopSendMessage();

    void tuningCaptureStartSendMessage(int duration);
    void tuningCaptureStopSendMessage();

    //----------------------------------
    // devices
    void clearCaptureDevices();
    void addCaptureDevice(const LLVoiceDevice& device);
    void clearRenderDevices();
    void setDevicesListUpdated(bool state);
    void addRenderDevice(const LLVoiceDevice& device);
    void buildSetAudioDevices(std::ostringstream &stream);

    void getCaptureDevicesSendMessage();
    void getRenderDevicesSendMessage();

    // local audio updates, mic mute, speaker mute, mic volume and speaker volumes
    void sendLocalAudioUpdates();

    /////////////////////////////
    // Response/Event handlers
    void connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID);
    void loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases);
    void sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle);
    void sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle);
    void sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString);
    void logoutResponse(int statusCode, std::string &statusString);
    void connectorShutdownResponse(int statusCode, std::string &statusString);

    void accountLoginStateChangeEvent(std::string &accountHandle, int statusCode, std::string &statusString, int state);
    void mediaCompletionEvent(std::string &sessionGroupHandle, std::string &mediaCompletionType);
    void mediaStreamUpdatedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, int statusCode, std::string &statusString, int state, bool incoming);
    void sessionAddedEvent(std::string &uriString, std::string &alias, std::string &sessionHandle, std::string &sessionGroupHandle, bool isChannel, bool incoming, std::string &nameString, std::string &applicationString);
    void sessionGroupAddedEvent(std::string &sessionGroupHandle);
    void sessionRemovedEvent(std::string &sessionHandle, std::string &sessionGroupHandle);
    void participantAddedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, std::string &uriString, std::string &alias, std::string &nameString, std::string &displayNameString, int participantType);
    void participantRemovedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, std::string &uriString, std::string &alias, std::string &nameString);
    void participantUpdatedEvent(std::string &sessionHandle, std::string &sessionGroupHandle, std::string &uriString, std::string &alias, bool isModeratorMuted, bool isSpeaking, int volume, F32 energy);
    void voiceServiceConnectionStateChangedEvent(int statusCode, std::string &statusString, std::string &build_id);
    void auxAudioPropertiesEvent(F32 energy);
    void messageEvent(std::string &sessionHandle, std::string &uriString, std::string &alias, std::string &messageHeader, std::string &messageBody, std::string &applicationString);
    void sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType);

    void muteListChanged();

    /////////////////////////////
    // VAD changes
    // disable auto-VAD and configure VAD parameters explicitly
    void setupVADParams(unsigned int vad_auto, unsigned int vad_hangover, unsigned int vad_noise_floor, unsigned int vad_sensitivity);
    void onVADSettingsChange();

    /////////////////////////////
    // Sending updates of current state
    void updatePosition(void);
    void setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot);
    void setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot);
    bool channelFromRegion(LLViewerRegion *region, std::string &name);

    void setEarLocation(S32 loc);


    /////////////////////////////
    // Accessors for data related to nearby speakers

    // MBW -- XXX -- Not sure how to get this data out of the TVC
    bool getUsingPTT(const LLUUID& id);
    std::string getGroupID(const LLUUID& id);       // group ID if the user is in group chat (empty string if not applicable)

    /////////////////////////////
    // Recording controls
    void recordingLoopStart(int seconds = 3600, int deltaFramesPerControlFrame = 200);
    void recordingLoopSave(const std::string& filename);
    void recordingStop();

    // Playback controls
    void filePlaybackStart(const std::string& filename);
    void filePlaybackStop();
    void filePlaybackSetPaused(bool paused);
    void filePlaybackSetMode(bool vox = false, float speed = 1.0f);

    participantStatePtr_t findParticipantByID(const LLUUID& id);


#if 0
    ////////////////////////////////////////
    // voice sessions.
    typedef std::set<sessionStatePtr_t> sessionSet;

    typedef sessionSet::iterator sessionIterator;
    sessionIterator sessionsBegin(void);
    sessionIterator sessionsEnd(void);
#endif

    sessionStatePtr_t findSession(const std::string &handle);
    sessionStatePtr_t findSessionBeingCreatedByURI(const std::string &uri);
    sessionStatePtr_t findSession(const LLUUID &participant_id);

    sessionStatePtr_t addSession(const std::string &uri, const std::string &handle = std::string());
    void clearSessionHandle(const sessionStatePtr_t &session);
    void setSessionHandle(const sessionStatePtr_t &session, const std::string &handle);
    void setSessionURI(const sessionStatePtr_t &session, const std::string &uri);
    void deleteSession(const sessionStatePtr_t &session);
    void deleteAllSessions(void);

    void verifySessionState(void);

    void joinedAudioSession(const sessionStatePtr_t &session);
    void leftAudioSession(const sessionStatePtr_t &session);

    // This is called in several places where the session _may_ need to be deleted.
    // It contains logic for whether to delete the session or keep it around.
    void reapSession(const sessionStatePtr_t &session);

    // Returns true if the session seems to indicate we've moved to a region on a different voice server
    bool sessionNeedsRelog(const sessionStatePtr_t &session);


    //////////////////////////////////////
    // buddy list stuff, needed for SLIM later
    struct buddyListEntry
    {
        buddyListEntry(const std::string &uri);
        std::string mURI;
        std::string mDisplayName;
        LLUUID  mUUID;
        bool mOnlineSL;
        bool mOnlineSLim;
        bool mCanSeeMeOnline;
        bool mHasBlockListEntry;
        bool mHasAutoAcceptListEntry;
        bool mNameResolved;
        bool mInSLFriends;
        bool mInVivoxBuddies;
    };

    typedef std::map<std::string, buddyListEntry*> buddyListMap;

    /////////////////////////////
    // session control messages

    void accountListBlockRulesSendMessage();
    void accountListAutoAcceptRulesSendMessage();

    void sessionGroupCreateSendMessage();
    void sessionCreateSendMessage(const sessionStatePtr_t &session, bool startAudio = true, bool startText = false);
    void sessionGroupAddSessionSendMessage(const sessionStatePtr_t &session, bool startAudio = true, bool startText = false);
    void sessionMediaConnectSendMessage(const sessionStatePtr_t &session);      // just joins the audio session
    void sessionTextConnectSendMessage(const sessionStatePtr_t &session);       // just joins the text session
    void sessionTerminateSendMessage(const sessionStatePtr_t &session);
    void sessionGroupTerminateSendMessage(const sessionStatePtr_t &session);
    void sessionMediaDisconnectSendMessage(const sessionStatePtr_t &session);
    // void sessionTextDisconnectSendMessage(sessionState *session);



    // Pokes the state machine to leave the audio session next time around.
    void sessionTerminate();

    // Pokes the state machine to shut down the connector and restart it.
    void requestRelog();

    // Does the actual work to get out of the audio session
    void leaveAudioSession();

    friend class LLVivoxVoiceClientCapResponder;


    void lookupName(const LLUUID &id);
    void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
    void avatarNameResolved(const LLUUID &id, const std::string &name);
    static void predAvatarNameResolution(const LLVivoxVoiceClient::sessionStatePtr_t &session, LLUUID id, std::string name);

    boost::signals2::connection mAvatarNameCacheConnection;

    /////////////////////////////
    // Voice fonts

    void addVoiceFont(const S32 id,
                      const std::string &name,
                      const std::string &description,
                      const LLDate &expiration_date,
                      bool  has_expired,
                      const S32 font_type,
                      const S32 font_status,
                      const bool template_font = false);
    void accountGetSessionFontsResponse(int statusCode, const std::string &statusString);
    void accountGetTemplateFontsResponse(int statusCode, const std::string &statusString);

private:

    LLVoiceVersionInfo mVoiceVersion;

    // Coroutine support methods
    //---
    void voiceControlCoro();
    void voiceControlStateMachine(S32 &coro_state);

    bool endAndDisconnectSession();

    bool callbackEndDaemon(const LLSD& data);
    bool startAndLaunchDaemon();
    bool provisionVoiceAccount();
    bool establishVoiceConnection();
    bool breakVoiceConnection(bool wait);
    bool loginToVivox();
    void logoutOfVivox(bool wait);
    bool retrieveVoiceFonts();

    bool requestParcelVoiceInfo();

    bool addAndJoinSession(const sessionStatePtr_t &nextSession);
    bool terminateAudioSession(bool wait);

    bool waitForChannel();
    bool runSession(const sessionStatePtr_t &session);

    void recordingAndPlaybackMode();
    int voiceRecordBuffer();
    int voicePlaybackBuffer();

    bool performMicTuning();
    //---
    /// Clean up objects created during a voice session.
    void cleanUp();

    bool mSessionTerminateRequested;
    bool mRelogRequested;
    // Number of times (in a row) "stateJoiningSession" case for spatial channel is reached in stateMachine().
    // The larger it is the greater is possibility there is a problem with connection to voice server.
    // Introduced while fixing EXT-4313.
    int mSpatialJoiningNum;

    static void idle(void *user_data);

    LLHost mDaemonHost;
    LLSocket::ptr_t mSocket;

    // We should kill the voice daemon in case of connection alert
    bool mTerminateDaemon;

    friend class LLVivoxProtocolParser;

    std::string mAccountName;
    std::string mAccountPassword;
    std::string mAccountDisplayName;

    bool mTuningMode;
    float mTuningEnergy;
    std::string mTuningAudioFile;
    int mTuningMicVolume;
    bool mTuningMicVolumeDirty;
    int mTuningSpeakerVolume;
    bool mTuningSpeakerVolumeDirty;
    bool mDevicesListUpdated;           // set to true when the device list has been updated
                                        // and false when the panelvoicedevicesettings has queried for an update status.

    std::string mSpatialSessionURI;
    std::string mSpatialSessionCredentials;

    std::string mMainSessionGroupHandle; // handle of the "main" session group.

    std::string mChannelName;           // Name of the channel to be looked up
    sessionStatePtr_t mAudioSession;        // Session state for the current audio session

    sessionStatePtr_t mNextAudioSession;    // Session state for the audio session we're trying to join

    S32 mCurrentParcelLocalID;          // Used to detect parcel boundary crossings
    std::string mCurrentRegionName;     // Used to detect parcel boundary crossings

    bool mConnectorEstablished; // set by "Create Connector" response
    bool mAccountLoggedIn;      // set by login message
    int  mNumberOfAliases;
    U32  mCommandCookie;

    std::string mVoiceAccountServerURI;
    std::string mVoiceSIPURIHostName;

    int mLoginRetryCount;

    sessionMap mSessionsByHandle;               // Active sessions, indexed by session handle.  Sessions which are being initiated may not be in this map.
#if 0
    sessionSet mSessions;                       // All sessions, not indexed.  This is the canonical session list.
#endif

    bool mBuddyListMapPopulated;
    bool mBlockRulesListReceived;
    bool mAutoAcceptRulesListReceived;
    buddyListMap mBuddyListMap;

    LLVoiceDeviceList mCaptureDevices;
    LLVoiceDeviceList mRenderDevices;

    std::string mCaptureDevice;
    std::string mRenderDevice;
    bool mCaptureDeviceDirty;
    bool mRenderDeviceDirty;

    bool mIsInitialized;
    bool mShutdownComplete;

    bool checkParcelChanged(bool update = false);
    bool switchChannel(std::string uri = std::string(), bool spatial = true, bool no_reconnect = false, bool is_p2p = false, std::string hash = "");
    void joinSession(const sessionStatePtr_t &session);

    std::string nameFromID(const LLUUID &id) const;
    bool IDFromName(const std::string name, LLUUID &uuid);
    std::string sipURIFromAvatar(LLVOAvatar *avatar);
    std::string sipURIFromName(std::string &name);

    // Returns the name portion of the SIP URI if the string looks vaguely like a SIP URI, or an empty string if not.
    std::string nameFromsipURI(const std::string &uri);

    bool inSpatialChannel(void);
    LLSD getAudioSessionChannelInfo();
    std::string getAudioSessionHandle();

    void sendPositionAndVolumeUpdate(void);

    void sendCaptureAndRenderDevices();
    void buildSetCaptureDevice(std::ostringstream &stream);
    void buildSetRenderDevice(std::ostringstream &stream);


    void sendFriendsListUpdates();

#if 0
    // start a text IM session with the specified user
    // This will be asynchronous, the session may be established at a future time.
    sessionStatePtr_t startUserIMSession(const LLUUID& uuid);
#endif

    void enforceTether(void);

    bool        mSpatialCoordsDirty;

    LLVector3d  mCameraPosition;
    LLVector3d  mCameraRequestedPosition;
    LLVector3   mCameraVelocity;
    LLMatrix3   mCameraRot;

    LLVector3d  mAvatarPosition;
    LLVector3   mAvatarVelocity;
    LLQuaternion mAvatarRot;

    bool        mMuteMic;
    bool        mMuteMicDirty;
    bool        mHidden;       //Set to true during teleport to hide the agent's position.

    // Set to true when the friends list is known to have changed.
    bool        mFriendsListDirty;

    enum
    {
        earLocCamera = 0,       // ear at camera
        earLocAvatar,           // ear at avatar
        earLocMixed             // ear at avatar location/camera direction
    };

    S32         mEarLocation;

    bool        mSpeakerVolumeDirty;
    bool        mSpeakerMuteDirty;
    int         mSpeakerVolume;

    int         mMicVolume;
    bool        mMicVolumeDirty;

    bool        mVoiceEnabled;
    bool        mProcessChannels;
    bool        mWriteInProgress;
    std::string mWriteString;
    size_t      mWriteOffset;

    typedef std::set<LLVoiceClientParticipantObserver*> observer_set_t;
    observer_set_t mParticipantObservers;

    void notifyParticipantObservers();

    typedef std::set<LLVoiceClientStatusObserver*> status_observer_set_t;
    status_observer_set_t mStatusObservers;

    void notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status);

    typedef std::set<LLFriendObserver*> friend_observer_set_t;
    friend_observer_set_t mFriendObservers;
    void notifyFriendObservers();

    // Voice Fonts

    void expireVoiceFonts();
    void deleteVoiceFont(const LLUUID& id);
    void deleteAllVoiceFonts();
    void deleteVoiceFontTemplates();

    S32 getVoiceFontIndex(const LLUUID& id) const;
    S32 getVoiceFontTemplateIndex(const LLUUID& id) const;

    void accountGetSessionFontsSendMessage();
    void accountGetTemplateFontsSendMessage();
    void sessionSetVoiceFontSendMessage(const sessionStatePtr_t &session);

    void updateVoiceMorphingMenu();
    void notifyVoiceFontObservers();

    typedef enum e_voice_font_type
    {
        VOICE_FONT_TYPE_NONE = 0,
        VOICE_FONT_TYPE_ROOT = 1,
        VOICE_FONT_TYPE_USER = 2,
        VOICE_FONT_TYPE_UNKNOWN
    } EVoiceFontType;

    typedef enum e_voice_font_status
    {
        VOICE_FONT_STATUS_NONE = 0,
        VOICE_FONT_STATUS_FREE = 1,
        VOICE_FONT_STATUS_NOT_FREE = 2,
        VOICE_FONT_STATUS_UNKNOWN
    } EVoiceFontStatus;

    struct voiceFontEntry
    {
        voiceFontEntry(LLUUID& id);
        ~voiceFontEntry();

        LLUUID      mID;
        S32         mFontIndex;
        std::string mName;
        LLDate      mExpirationDate;
        S32         mFontType;
        S32         mFontStatus;
        bool        mIsNew;

        LLFrameTimer    mExpiryTimer;
    };

    bool mVoiceFontsReceived;
    bool mVoiceFontsNew;
    bool mVoiceFontListDirty;
    voice_effect_list_t mVoiceFontList;
    voice_effect_list_t mVoiceFontTemplateList;

    typedef std::map<const LLUUID, voiceFontEntry*> voice_font_map_t;
    voice_font_map_t    mVoiceFontMap;
    voice_font_map_t    mVoiceFontTemplateMap;

    typedef std::set<LLVoiceEffectObserver*> voice_font_observer_set_t;
    voice_font_observer_set_t mVoiceFontObservers;

    LLFrameTimer    mVoiceFontExpiryTimer;


    // Audio capture buffer

    void captureBufferRecordStartSendMessage();
    void captureBufferRecordStopSendMessage();
    void captureBufferPlayStartSendMessage(const LLUUID& voice_font_id = LLUUID::null);
    void captureBufferPlayStopSendMessage();

    bool mCaptureBufferMode;        // Disconnected from voice channels while using the capture buffer.
    bool mCaptureBufferRecording;   // A voice sample is being captured.
    bool mCaptureBufferRecorded;    // A voice sample is captured in the buffer ready to play.
    bool mCaptureBufferPlaying;     // A voice sample is being played.

    LLTimer mCaptureTimer;
    LLUUID  mPreviewVoiceFont;
    LLUUID  mPreviewVoiceFontLast;
    S32     mPlayRequestCount;
    bool    mIsInTuningMode;
    bool    mIsInChannel;
    bool    mIsJoiningSession;
    bool    mIsWaitingForFonts;
    bool    mIsLoggingIn;
    bool    mIsLoggedIn;
    bool    mIsProcessingChannels;
    bool    mIsCoroutineActive;

    // This variables can last longer than vivox in coroutines so we need them as static
    static bool sShuttingDown;
    static bool sConnected;
    static LLPumpIO* sPump;

    LLEventMailDrop mVivoxPump;
};


/**
 * @class LLVivoxProtocolParser
 * @brief This class helps construct new LLIOPipe specializations
 * @see LLIOPipe
 *
 * THOROUGH_DESCRIPTION
 */
class LLVivoxProtocolParser : public LLIOPipe
{
    LOG_CLASS(LLVivoxProtocolParser);
public:
    LLVivoxProtocolParser();
    virtual ~LLVivoxProtocolParser();

protected:
    /* @name LLIOPipe virtual implementations
     */
    //@{
    /**
     * @brief Process the data in buffer
     */
    virtual EStatus process_impl(
                                 const LLChannelDescriptors& channels,
                                 buffer_ptr_t& buffer,
                                 bool& eos,
                                 LLSD& context,
                                 LLPumpIO* pump);
    //@}

    std::string     mInput;

    // Expat control members
    XML_Parser      parser;
    int             responseDepth;
    bool            ignoringTags;
    bool            isEvent;
    int             ignoreDepth;

    // Members for processing responses. The values are transient and only valid within a call to processResponse().
    int             returnCode;
    int             statusCode;
    std::string     statusString;
    std::string     requestId;
    std::string     actionString;
    std::string     connectorHandle;
    std::string     versionID;
    std::string     mBuildID;
    std::string     accountHandle;
    std::string     sessionHandle;
    std::string     sessionGroupHandle;
    std::string     alias;
    std::string     applicationString;

    // Members for processing events. The values are transient and only valid within a call to processResponse().
    std::string     eventTypeString;
    int             state;
    std::string     uriString;
    bool            isChannel;
    bool            incoming;
    bool            enabled;
    std::string     nameString;
    std::string     audioMediaString;
    std::string     deviceString;
    std::string     displayNameString;
    int             participantType;
    bool            isLocallyMuted;
    bool            isModeratorMuted;
    bool            isSpeaking;
    int             volume;
    F32             energy;
    std::string     messageHeader;
    std::string     messageBody;
    std::string     notificationType;
    bool            hasText;
    bool            hasAudio;
    bool            hasVideo;
    bool            terminated;
    std::string     blockMask;
    std::string     presenceOnly;
    std::string     autoAcceptMask;
    std::string     autoAddAsBuddy;
    int             numberOfAliases;
    std::string     subscriptionHandle;
    std::string     subscriptionType;
    S32             id;
    std::string     descriptionString;
    LLDate          expirationDate;
    bool            hasExpired;
    S32             fontType;
    S32             fontStatus;
    std::string     mediaCompletionType;

    // Members for processing text between tags
    std::string     textBuffer;
    bool            accumulateText;

    void            reset();

    void            processResponse(std::string tag);

    static void XMLCALL ExpatStartTag(void *data, const char *el, const char **attr);
    static void XMLCALL ExpatEndTag(void *data, const char *el);
    static void XMLCALL ExpatCharHandler(void *data, const XML_Char *s, int len);

    void            StartTag(const char *tag, const char **attr);
    void            EndTag(const char *tag);
    void            CharData(const char *buffer, int length);
    LLDate          expiryTimeStampToLLDate(const std::string& vivox_ts);

};

class LLVivoxSecurity : public LLSingleton<LLVivoxSecurity>
{
    LLSINGLETON(LLVivoxSecurity);
    virtual ~LLVivoxSecurity();

  public:
    std::string     connectorHandle() { return mConnectorHandle; };
    std::string     accountHandle()    { return mAccountHandle;    };

  private:
    std::string     mConnectorHandle;
    std::string     mAccountHandle;
};

class LLVoiceVivoxStats : public LLSingleton<LLVoiceVivoxStats>
{
    LLSINGLETON(LLVoiceVivoxStats);
    LOG_CLASS(LLVoiceVivoxStats);
    virtual ~LLVoiceVivoxStats();

  private:
    F64SecondsImplicit mStartTime;

    U32 mConnectCycles;

    F64 mConnectTime;
    U32 mConnectAttempts;

    F64 mProvisionTime;
    U32 mProvisionAttempts;

    F64 mEstablishTime;
    U32 mEstablishAttempts;

  public:

    void reset();
    void connectionAttemptStart();
    void connectionAttemptEnd(bool success);
    void provisionAttemptStart();
    void provisionAttemptEnd(bool success);
    void establishAttemptStart();
    void establishAttemptEnd(bool success);
    LLSD read();
};

#endif //LL_VIVOX_VOICE_CLIENT_H

