/** 
 * @file llvoicewebrtc.h
 * @brief Declaration of LLWebRTCVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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
#ifndef LL_VOICE_WEBRTC_H
#define LL_VOICE_WEBRTC_H

class LLVOAvatar;
class LLWebRTCProtocolParser;

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
#include "llparcel.h"
#include <queue>
#include "json/reader.h"

#ifdef LL_USESYSTEMLIBS
# include "expat.h"
#else
# include "expat/expat.h"
#endif
#include "llvoiceclient.h"

// WebRTC Includes
#include <llwebrtc.h>

class LLAvatarName;
class LLVoiceWebRTCConnection;
typedef boost::shared_ptr<LLVoiceWebRTCConnection> connectionPtr_t;

class LLWebRTCVoiceClient :	public LLSingleton<LLWebRTCVoiceClient>,
							virtual public LLVoiceModuleInterface,
							virtual public LLVoiceEffectInterface,
                            public llwebrtc::LLWebRTCDevicesObserver
{
    LLSINGLETON_C11(LLWebRTCVoiceClient);
	LOG_CLASS(LLWebRTCVoiceClient);
	virtual ~LLWebRTCVoiceClient();

public:
	/// @name LLVoiceModuleInterface virtual implementations
	///  @see LLVoiceModuleInterface
	//@{
	void init(LLPumpIO *pump) override;	// Call this once at application startup (creates connector)
	void terminate() override;	// Call this to clean up during shutdown

	static bool isShuttingDown() { return sShuttingDown; }
	
	const LLVoiceVersionInfo& getVersion() override;
	
	void updateSettings() override; // call after loading settings and whenever they change

	// Returns true if WebRTC has successfully logged in and is not in error state	
	bool isVoiceWorking() const override;

	std::string sipURIFromID(const LLUUID &id) override;

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
	
	// Requery the WebRTC daemon for the current list of input/output devices.
	// If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
	// (use this if you want to know when it's done).
	// If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
	void refreshDeviceLists(bool clearCurrentList = true) override;
	
	void setCaptureDevice(const std::string& name) override;
	void setRenderDevice(const std::string& name) override;
	
	LLVoiceDeviceList& getCaptureDevices() override;
	LLVoiceDeviceList& getRenderDevices() override;
	//@}	
	
	void getParticipantList(std::set<LLUUID> &participants) override;
	bool isParticipant(const LLUUID& speaker_id) override;

	// Send a text message to the specified user, initiating the session if necessary.
	// virtual BOOL sendTextMessage(const LLUUID& participant_id, const std::string& message) const {return false;};
	
	// close any existing text IM session with the specified user
	void endUserIMSession(const LLUUID &uuid) override;

	// Returns true if calling back the session URI after the session has closed is possible.
	// Currently this will be false only for PSTN P2P calls.		
	// NOTE: this will return true if the session can't be found. 
	BOOL isSessionCallBackPossible(const LLUUID &session_id) override;
	
	// Returns true if the session can accepte text IM's.
	// Currently this will be false only for PSTN P2P calls.
	// NOTE: this will return true if the session can't be found. 
	BOOL isSessionTextIMPossible(const LLUUID &session_id) override;
	
	
	////////////////////////////
	/// @name Channel stuff
	//@{
	// returns true iff the user is currently in a proximal (local spatial) channel.
	// Note that gestures should only fire if this returns true.
	bool inProximalChannel() override;
	
	void setNonSpatialChannel(const std::string &uri,
									  const std::string &credentials) override;
	
	bool setSpatialChannel(const std::string &uri, const std::string &credentials) override 
	{
        return setSpatialChannel(uri, credentials, INVALID_PARCEL_ID);
	}

    bool setSpatialChannel(const std::string &uri, const std::string &credentials, S32 localParcelID);
	
	void leaveNonSpatialChannel() override;
	
	void leaveChannel(void) override;
	
	// Returns the URI of the current channel, or an empty string if not currently in a channel.
	// NOTE that it will return an empty string if it's in the process of joining a channel.
	std::string getCurrentChannel() override;
	//@}
	
	
	//////////////////////////
	/// @name invitations
	//@{
	// start a voice channel with the specified user
	void callUser(const LLUUID &uuid) override;
	bool isValidChannel(std::string &channelID) override;
    bool answerInvite(std::string &channelID) override;
    void declineInvite(std::string &channelID) override;
	//@}
	
	/////////////////////////
	/// @name Volume/gain
	//@{
	void setVoiceVolume(F32 volume) override;
	void setMicGain(F32 volume) override;
	//@}
	
	/////////////////////////
	/// @name enable disable voice and features
	//@{
	bool voiceEnabled() override;
	void setVoiceEnabled(bool enabled) override;
	BOOL lipSyncEnabled() override;
	void setLipSyncEnabled(BOOL enabled) override;
	void setMuteMic(bool muted) override;		// Set the mute state of the local mic.
	//@}
		
	//////////////////////////
	/// @name nearby speaker accessors
	//@{
	BOOL getVoiceEnabled(const LLUUID& id) override;		// true if we've received data for this avatar
	std::string getDisplayName(const LLUUID& id) override;
	BOOL isParticipantAvatar(const LLUUID &id) override;
	BOOL getIsSpeaking(const LLUUID& id) override;
	BOOL getIsModeratorMuted(const LLUUID& id) override;
	F32 getCurrentPower(const LLUUID& id) override;		// "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
	BOOL getOnMuteList(const LLUUID& id) override;
	F32 getUserVolume(const LLUUID& id) override;
	void setUserVolume(const LLUUID& id, F32 volume) override; // set's volume for specified agent, from 0-1 (where .5 is nominal)
	//@}

	//////////////////////////
    /// @name Effect Accessors
    //@{
    bool         setVoiceEffect(const LLUUID &id) override { return false; }
    const LLUUID getVoiceEffect() override { return LLUUID(); }
    LLSD         getVoiceEffectProperties(const LLUUID &id) override { return LLSD(); }

    void                       refreshVoiceEffectLists(bool clear_lists) override {};
    const voice_effect_list_t &getVoiceEffectList() const override { return mVoiceEffectList; }
    const voice_effect_list_t &getVoiceEffectTemplateList() const override { return mVoiceEffectList; }

	voice_effect_list_t mVoiceEffectList;
    //@}

    //////////////////////////////
    /// @name Status notification
    //@{
    void addObserver(LLVoiceEffectObserver *observer) override {}
    void removeObserver(LLVoiceEffectObserver *observer) override {}
    //@}

	//////////////////////////////
    /// @name Preview buffer
    //@{
    void enablePreviewBuffer(bool enable) override {}
    void recordPreviewBuffer() override {}
    void playPreviewBuffer(const LLUUID &effect_id = LLUUID::null) override {}
    void stopPreviewBuffer() override {}

    bool isPreviewRecording() override { return false;  }
    bool isPreviewPlaying() override { return false; }
    //@}

	// authorize the user
    void userAuthorized(const std::string &user_id, const LLUUID &agentID) override {};


	void OnConnectionEstablished(const std::string& channelID);
    void OnConnectionFailure(const std::string &channelID);
    void sendPositionAndVolumeUpdate(bool force);
    void updateOwnVolume();
	
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

	//////////////////////////////
    /// @name Devices change notification
	//  LLWebRTCDevicesObserver
    //@{
    void OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList &render_devices,
                          const llwebrtc::LLWebRTCVoiceDeviceList &capture_devices) override;
    //@}


	struct participantState
	{
	public:
		participantState(const LLUUID& agent_id);
		
	    bool updateMuteState();	// true if mute state has changed
		bool isAvatar();
		
		std::string mURI;
		LLUUID mAvatarID;
		std::string mDisplayName;
		LLFrameTimer mSpeakingTimeout;
		F32	mLastSpokeTimestamp;
		F32 mPower;
		F32 mVolume;
		std::string mGroupID;
		int mUserVolume;
		bool mPTT;
		bool mIsSpeaking;
		bool mIsModeratorMuted;
		bool mOnMuteList;		// true if this avatar is on the user's mute list (and should be muted)
		bool mVolumeSet;		// true if incoming volume messages should not change the volume
		bool mVolumeDirty;		// true if this participant needs a volume command sent (either mOnMuteList or mUserVolume has changed)
		bool mAvatarIDValid;
		bool mIsSelf;
	};
    typedef boost::shared_ptr<participantState> participantStatePtr_t;
    typedef boost::weak_ptr<participantState> participantStateWptr_t;

    participantStatePtr_t                       findParticipantByID(const std::string &channelID, const LLUUID &id);
    participantStatePtr_t                       addParticipantByID(const std::string& channelID, const LLUUID &id);
    void                                        removeParticipantByID(const std::string& channelID, const LLUUID &id);

  protected:

    typedef std::map<const std::string, participantStatePtr_t> participantMap;
    typedef std::map<const LLUUID, participantStatePtr_t> participantUUIDMap;
	
	struct sessionState
	{
    public:
        typedef boost::shared_ptr<sessionState> ptr_t;
        typedef boost::weak_ptr<sessionState> wptr_t;

        typedef boost::function<void(const ptr_t &)> sessionFunc_t;

        static ptr_t createSession(const std::string& channelID, S32 parcel_local_id);
		~sessionState();
		
        participantStatePtr_t addParticipant(const LLUUID& agent_id);
        void removeParticipant(const participantStatePtr_t &participant);
		void removeAllParticipants();

        participantStatePtr_t findParticipant(const std::string &uri);
        participantStatePtr_t findParticipantByID(const LLUUID& id);

        static ptr_t matchSessionByChannelID(const std::string& channel_id);

		void shutdownAllConnections();

		bool isCallBackPossible();
		bool isTextIMPossible();

        void processSessionStates();

		void OnConnectionEstablished(const std::string &channelID);
        void OnConnectionFailure(const std::string &channelID);

		void sendData(const std::string &data);
		
        static void for_each(sessionFunc_t func);

		static void reapEmptySessions();

		bool isEmpty() { return mWebRTCConnections.empty(); }

		std::string mHandle;
		std::string mGroupHandle;
		std::string mChannelID;
		std::string mAlias;
		std::string mName;
		std::string mErrorStatusString;
		std::queue<std::string> mTextMsgQueue;
		
		LLUUID		mIMSessionID;
		LLUUID		mCallerID;
		int			mErrorStatusCode;
		bool		mIsChannel;	// True for both group and spatial channels (false for p2p, PSTN)
		bool		mIsSpatial;	// True for spatial channels
		bool		mIsP2P;
		bool		mIncoming;
		bool		mVoiceActive;
		bool		mReconnect;	// Whether we should try to reconnect to this session if it's dropped

		// Set to true when the volume/mute state of someone in the participant list changes.
		// The code will have to walk the list to find the changed participant(s).
		bool		mVolumeDirty;
		bool		mMuteDirty;

		bool		mParticipantsChanged;
		participantMap mParticipantsByURI;
		participantUUIDMap mParticipantsByUUID;

		LLUUID		mVoiceFontID;

        static void VerifySessions();
        static void deleteAllSessions();

    private:

		std::map<std::string, connectionPtr_t> mWebRTCConnections;
        std::string   					       mPrimaryConnectionID;

        sessionState();

        static std::map<std::string, ptr_t> mSessions;  // canonical list of outstanding sessions.

        static void for_eachPredicate(const std::pair<std::string, LLWebRTCVoiceClient::sessionState::wptr_t> &a, sessionFunc_t func);

        static bool testByCreatingURI(const LLWebRTCVoiceClient::sessionState::wptr_t &a, std::string uri);
        static bool testByCallerId(const LLWebRTCVoiceClient::sessionState::wptr_t &a, LLUUID participantId);

	};
    typedef boost::shared_ptr<sessionState> sessionStatePtr_t;

    typedef std::map<std::string, sessionStatePtr_t> sessionMap;
	
	///////////////////////////////////////////////////////
	// Private Member Functions
	//////////////////////////////////////////////////////

    static void predProcessSessionStates(const LLWebRTCVoiceClient::sessionStatePtr_t &session);
    static void predOnConnectionEstablished(const LLWebRTCVoiceClient::sessionStatePtr_t &session, std::string channelID);
    static void predOnConnectionFailure(const LLWebRTCVoiceClient::sessionStatePtr_t &session, std::string channelID);
    static void predSendData(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const std::string& spatial_data, const std::string& volume_data);
    static void predUpdateOwnVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 audio_level);
    static void predSetMuteMic(const LLWebRTCVoiceClient::sessionStatePtr_t &session, bool mute);
	//////////////////////////////
	/// @name TVC/Server management and communication
	//@{
	
	// Call this if we're just giving up on voice (can't provision an account, etc.).  It will clean up and go away.
	void giveUp();	
	
//	void requestVoiceAccountProvision(S32 retries = 3);
	
	
	//@}

	//----------------------------------
	// devices
	void clearCaptureDevices();
	void addCaptureDevice(const LLVoiceDevice& device);

    void clearRenderDevices();
	void addRenderDevice(const LLVoiceDevice& device);	
	void setDevicesListUpdated(bool state);
	void buildSetAudioDevices(std::ostringstream &stream);
	
	// local audio updates, mic mute, speaker mute, mic volume and speaker volumes
	void sendLocalAudioUpdates();

	/////////////////////////////
	// Event handlers

	void muteListChanged();

	/////////////////////////////
	// Sending updates of current state
	void updatePosition(void);
	void setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot);
	void setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot);
	bool channelFromRegion(LLViewerRegion *region, std::string &name);

	void setEarLocation(S32 loc);

	
	/////////////////////////////
	// Accessors for data related to nearby speakers

	// MBW -- XXX -- Not sure how to get this data out of the TVC
	BOOL getUsingPTT(const LLUUID& id);
	std::string getGroupID(const LLUUID& id);		// group ID if the user is in group chat (empty string if not applicable)

	/////////////////////////////
	BOOL getAreaVoiceDisabled();		// returns true if the area the avatar is in is speech-disabled.
										// Use this to determine whether to show a "no speech" icon in the menu bar.
    
    void              sessionEstablished(const LLUUID& region_id);
    sessionStatePtr_t findP2PSession(const LLUUID &agent_id);
	
    sessionStatePtr_t addSession(const std::string& channel_id, S32 parcel_local_id);
    void deleteSession(const sessionStatePtr_t &session);

	void verifySessionState(void);

    void joinedAudioSession(const sessionStatePtr_t &session);
    void leftAudioSession(const sessionStatePtr_t &session);

	// This is called in several places where the session _may_ need to be deleted.
	// It contains logic for whether to delete the session or keep it around.
    void reapSession(const sessionStatePtr_t &session);	
	
	//////////////////////////////////////
	// buddy list stuff, needed for SLIM later
	struct buddyListEntry
	{
		buddyListEntry(const std::string &uri);
		std::string mURI;
		std::string mDisplayName;
		LLUUID	mUUID;
		bool mOnlineSL;
		bool mOnlineSLim;
		bool mCanSeeMeOnline;
		bool mHasBlockListEntry;
		bool mHasAutoAcceptListEntry;
		bool mNameResolved;
		bool mInSLFriends;
		bool mInWebRTCBuddies;
	};

	typedef std::map<std::string, buddyListEntry*> buddyListMap;
	
	// Pokes the state machine to leave the audio session next time around.
	void sessionTerminate();	
	
	// Pokes the state machine to shut down the connector and restart it.
	void requestRelog();
	
	// Does the actual work to get out of the audio session
	void leaveAudioSession();
	
	friend class LLWebRTCVoiceClientCapResponder;
	
	
	void lookupName(const LLUUID &id);
	void onAvatarNameCache(const LLUUID& id, const LLAvatarName& av_name);
	void avatarNameResolved(const LLUUID &id, const std::string &name);
    static void predAvatarNameResolution(const LLWebRTCVoiceClient::sessionStatePtr_t &session, LLUUID id, std::string name);

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
    void voiceConnectionCoro();

    void voiceConnectionStateMachine();

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
			
	bool mTuningMode;
	float mTuningEnergy;
	int mTuningMicVolume;
	bool mTuningMicVolumeDirty;
	int mTuningSpeakerVolume;
	bool mTuningSpeakerVolumeDirty;
	bool mDevicesListUpdated;			// set to true when the device list has been updated
										// and false when the panelvoicedevicesettings has queried for an update status.
	std::string mSpatialSessionCredentials;

	std::string mMainSessionGroupHandle; // handle of the "main" session group.
	
	std::string mChannelName;			// Name of the channel to be looked up 
	bool mAreaVoiceDisabled;
    sessionStatePtr_t mAudioSession;    // Session state for the current audio session
	bool mAudioSessionChanged;			// set to true when the above pointer gets changed, so observers can be notified.

    sessionStatePtr_t mNextAudioSession;	// Session state for the audio session we're trying to join

	S32 mCurrentParcelLocalID;			// Used to detect parcel boundary crossings
	std::string mCurrentRegionName;		// Used to detect parcel boundary crossings
		
	bool mBuddyListMapPopulated;
	bool mBlockRulesListReceived;
	bool mAutoAcceptRulesListReceived;
	buddyListMap mBuddyListMap;
	
	llwebrtc::LLWebRTCDeviceInterface *mWebRTCDeviceInterface;

	LLVoiceDeviceList mCaptureDevices;
	LLVoiceDeviceList mRenderDevices;

	uint32_t mAudioLevel;

	bool mIsInitialized;
	bool mShutdownComplete;
	
	bool checkParcelChanged(bool update = false);
    bool switchChannel(const std::string channelID,
                       bool         spatial      = true,
                       bool         no_reconnect = false,
                       bool         is_p2p       = false,
                       std::string  hash         = "",
		               S32          parcel_local_id = INVALID_PARCEL_ID);
    void joinSession(const sessionStatePtr_t &session);
	
	std::string nameFromAvatar(LLVOAvatar *avatar);
	std::string nameFromID(const LLUUID &id);
	bool IDFromName(const std::string name, LLUUID &uuid);
	std::string displayNameFromAvatar(LLVOAvatar *avatar);
	

	bool inSpatialChannel(void);
	std::string getAudioSessionURI();
			
    void setHidden(bool hidden) override; //virtual

	void enforceTether(void);
	
	bool		mSpatialCoordsDirty;
	
	LLVector3d	mCameraPosition;
	LLVector3d	mCameraRequestedPosition;
	LLVector3	mCameraVelocity;
	LLQuaternion mCameraRot;

	LLVector3d	mAvatarPosition;
	LLVector3	mAvatarVelocity;
	LLQuaternion mAvatarRot;
	
	bool		mMuteMic;
	bool		mMuteMicDirty;
    bool        mHidden;       //Set to true during teleport to hide the agent's position.
			
	// Set to true when the friends list is known to have changed.
	bool		mFriendsListDirty;
	
	enum
	{
		earLocCamera = 0,		// ear at camera
		earLocAvatar,			// ear at avatar
		earLocMixed				// ear at avatar location/camera direction
	};
	
	S32			mEarLocation;  
	
	bool		mSpeakerVolumeDirty;
	bool		mSpeakerMuteDirty;
	float		mSpeakerVolume;

	int			mMicVolume;
	bool		mMicVolumeDirty;
	
	bool		mVoiceEnabled;
	
	BOOL		mLipSyncEnabled;

	typedef std::set<LLVoiceClientParticipantObserver*> observer_set_t;
	observer_set_t mParticipantObservers;

	void notifyParticipantObservers();

	typedef std::set<LLVoiceClientStatusObserver*> status_observer_set_t;
	status_observer_set_t mStatusObservers;
	
	void notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status);

	typedef std::set<LLFriendObserver*> friend_observer_set_t;
	friend_observer_set_t mFriendObservers;
	void notifyFriendObservers();

	S32     mPlayRequestCount;
    bool    mIsInTuningMode;
    bool    mIsJoiningSession;
    bool    mIsWaitingForFonts;
    bool    mIsLoggingIn;
    bool    mIsProcessingChannels;
    bool    mIsCoroutineActive;

    // These variables can last longer than WebRTC in coroutines so we need them as static
    static bool sShuttingDown;
    static bool sConnected;
    static LLPumpIO* sPump;

    LLEventMailDrop mWebRTCPump;
};


class LLVoiceWebRTCStats : public LLSingleton<LLVoiceWebRTCStats>
{
    LLSINGLETON(LLVoiceWebRTCStats);
    LOG_CLASS(LLVoiceWebRTCStats);
    virtual ~LLVoiceWebRTCStats();
    
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

class LLVoiceWebRTCConnection : 
	public llwebrtc::LLWebRTCSignalingObserver, 
	public llwebrtc::LLWebRTCDataObserver
{
  public:
    LLVoiceWebRTCConnection(const LLUUID& regionID, S32 parcelLocalID, const std::string& channelID);

    virtual ~LLVoiceWebRTCConnection();

	//////////////////////////////
    /// @name Signaling notification
    //  LLWebRTCSignalingObserver
    //@{
    void OnIceGatheringState(IceGatheringState state) override;
    void OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate &candidate) override;
    void OnOfferAvailable(const std::string &sdp) override;
    void OnRenegotiationNeeded() override;
    void OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface *audio_interface) override;
    //@}

    /////////////////////////
    /// @name Data Notification
    /// LLWebRTCDataObserver
    //@{
    void OnDataReceived(const std::string &data, bool binary) override;
    void OnDataChannelReady(llwebrtc::LLWebRTCDataInterface *data_interface) override;
    //@}

	void processIceUpdates();
    void onIceUpdateComplete(bool ice_completed, const LLSD &result);
    void onIceUpdateError(int retries, std::string url, LLSD body, bool ice_completed, const LLSD &result);

    bool requestVoiceConnection();
    void OnVoiceConnectionRequestSuccess(const LLSD &body);
    void OnVoiceConnectionRequestFailure(std::string url, int retries, LLSD body, const LLSD &result);

	bool connectionStateMachine();

	void sendData(const std::string &data);

	void shutDown()
	{ 
		LLMutexLock lock(&mVoiceStateMutex);
		mShutDown = true;
	}

protected:
    typedef enum e_voice_connection_state
    {
        VOICE_STATE_ERROR   = 0x0,
		VOICE_STATE_START_SESSION = 0x1,
		VOICE_STATE_WAIT_FOR_SESSION_START = 0x2,
		VOICE_STATE_REQUEST_CONNECTION = 0x4,
		VOICE_STATE_CONNECTION_WAIT = 0x8,
        VOICE_STATE_SESSION_ESTABLISHED = 0x10,
        VOICE_STATE_SESSION_UP = 0x20,
        VOICE_STATE_SESSION_RETRY = 0x40,
        VOICE_STATE_DISCONNECT = 0x80,
        VOICE_STATE_WAIT_FOR_EXIT = 0x100,
		VOICE_STATE_SESSION_EXIT = 0x200,
		VOICE_STATE_SESSION_STOPPING = 0x3c0,
		VOICE_STATE_SESSION_WAITING = 0x10a
    } EVoiceConnectionState;

    EVoiceConnectionState mVoiceConnectionState;
    LLMutex mVoiceStateMutex;
    void setVoiceConnectionState(EVoiceConnectionState new_voice_connection_state)
    {
        LLMutexLock lock(&mVoiceStateMutex);

		if (new_voice_connection_state & VOICE_STATE_SESSION_STOPPING)
		{
			// the new state is shutdown or restart.
            mVoiceConnectionState = new_voice_connection_state;
            return;
		}
        if (mVoiceConnectionState & VOICE_STATE_SESSION_STOPPING)
		{
			// we're currently shutting down or restarting, so ignore any
			// state changes.
            return;
		}

        mVoiceConnectionState = new_voice_connection_state;
    }
    EVoiceConnectionState getVoiceConnectionState()
    {
        LLMutexLock lock(&mVoiceStateMutex);
        return mVoiceConnectionState;
    }

    bool breakVoiceConnection(bool wait);
    void OnVoiceDisconnectionRequestSuccess(const LLSD &body);
    void OnVoiceDisconnectionRequestFailure(std::string url, int retries, LLSD body, const LLSD &result);

    std::string mChannelSDP;
    std::string mRemoteChannelSDP;

    std::string mChannelID;
    LLUUID mRegionID;
    S32    mParcelLocalID;

    bool   mShutDown;

    std::vector<llwebrtc::LLWebRTCIceCandidate> mIceCandidates;
    bool                                        mIceCompleted;
    bool                                        mTrickling;

    llwebrtc::LLWebRTCPeerConnection *mWebRTCPeerConnection;
    llwebrtc::LLWebRTCAudioInterface *mWebRTCAudioInterface;
    llwebrtc::LLWebRTCDataInterface  *mWebRTCDataInterface;
};


#endif //LL_WebRTC_VOICE_CLIENT_H

