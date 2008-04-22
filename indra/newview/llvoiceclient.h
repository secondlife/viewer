/** 
 * @file llvoiceclient.h
 * @brief Declaration of LLVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#ifndef LL_VOICE_CLIENT_H
#define LL_VOICE_CLIENT_H

// This would create a circular reference -- just do a forward definition of necessary class names.
//#include "llvoavatar.h"
class LLVOAvatar;
class LLVivoxProtocolParser;

#include "lliopipe.h"
#include "llpumpio.h"
#include "llchainio.h"
#include "lliosocket.h"
#include "v3math.h"
#include "llframetimer.h"
#include "llviewerregion.h"

class LLVoiceClientParticipantObserver
{
public:
	virtual ~LLVoiceClientParticipantObserver() { }
	virtual void onChange() = 0;
};

class LLVoiceClientStatusObserver
{
public:
	typedef enum e_voice_status_type
	{
		// NOTE: when updating this enum, please also update the switch in
		//  LLVoiceClientStatusObserver::status2string().
		STATUS_LOGIN_RETRY,
		STATUS_LOGGED_IN,
		STATUS_JOINING,
		STATUS_JOINED,
		STATUS_LEFT_CHANNEL,
		STATUS_VOICE_DISABLED,
		BEGIN_ERROR_STATUS,
		ERROR_CHANNEL_FULL,
		ERROR_CHANNEL_LOCKED,
		ERROR_NOT_AVAILABLE,
		ERROR_UNKNOWN
	} EStatusType;

	virtual ~LLVoiceClientStatusObserver() { }
	virtual void onChange(EStatusType status, const std::string &channelURI, bool proximal) = 0;

	static const char *status2string(EStatusType inStatus);
};

class LLVoiceClient: public LLSingleton<LLVoiceClient>
{
	LOG_CLASS(LLVoiceClient);
	public:
		LLVoiceClient();	
		~LLVoiceClient();
		
	public:
		static void init(LLPumpIO *pump);	// Call this once at application startup (creates connector)
		static void terminate();	// Call this to clean up during shutdown
						
	protected:
		bool writeString(const std::string &str);

	public:
		
		enum serviceType
		{
			serviceTypeUnknown,	// Unknown, returned if no data on the avatar is available
			serviceTypeA,		// spatialized local chat
			serviceTypeB,		// remote multi-party chat
			serviceTypeC		// one-to-one and small group chat
		};
		static F32 OVERDRIVEN_POWER_LEVEL;
				
		/////////////////////////////
		// session control messages
		void connect();

		void connectorCreate();
		void connectorShutdown();

		void requestVoiceAccountProvision(S32 retries = 3);
		void userAuthorized(
			const std::string& firstName,
			const std::string& lastName,
			const LLUUID &agentID);
		void login(const std::string& accountName, const std::string &password);
		void loginSendMessage();
		void logout();
		void logoutSendMessage();
		
		void channelGetListSendMessage();
		void sessionCreateSendMessage();
		void sessionConnectSendMessage();
		void sessionTerminate();
		void sessionTerminateSendMessage();
		void sessionTerminateByHandle(std::string &sessionHandle);
		
		void getCaptureDevicesSendMessage();
		void getRenderDevicesSendMessage();
		
		void clearCaptureDevices();
		void addCaptureDevice(const std::string& name);
		void setCaptureDevice(const std::string& name);
		
		void clearRenderDevices();
		void addRenderDevice(const std::string& name);
		void setRenderDevice(const std::string& name);

		void tuningStart();
		void tuningStop();
		bool inTuningMode();
		bool inTuningStates();
		
		void tuningRenderStartSendMessage(const std::string& name, bool loop);
		void tuningRenderStopSendMessage();

		void tuningCaptureStartSendMessage(int duration);
		void tuningCaptureStopSendMessage();
		
		void tuningSetMicVolume(float volume);
		void tuningSetSpeakerVolume(float volume);
		float tuningGetEnergy(void);
				
		// This returns true when it's safe to bring up the "device settings" dialog in the prefs.
		// i.e. when the daemon is running and connected, and the device lists are populated.
		bool deviceSettingsAvailable();
		
		// Requery the vivox daemon for the current list of input/output devices.
		// If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
		// (use this if you want to know when it's done).
		// If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
		void refreshDeviceLists(bool clearCurrentList = true);
		
		// Call this if the connection to the daemon terminates unexpectedly.  It will attempt to reset everything and relaunch.
		void daemonDied();

		// Call this if we're just giving up on voice (can't provision an account, etc.).  It will clean up and go away.
		void giveUp();
		
		/////////////////////////////
		// Response/Event handlers
		void connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle);
		void loginResponse(int statusCode, std::string &statusString, std::string &accountHandle);
		void channelGetListResponse(int statusCode, std::string &statusString);
		void sessionCreateResponse(int statusCode, std::string &statusString, std::string &sessionHandle);
		void sessionConnectResponse(int statusCode, std::string &statusString);
		void sessionTerminateResponse(int statusCode, std::string &statusString);
		void logoutResponse(int statusCode, std::string &statusString);
		void connectorShutdownResponse(int statusCode, std::string &statusString);

		void loginStateChangeEvent(std::string &accountHandle, int statusCode, std::string &statusString, int state);
		void sessionNewEvent(std::string &accountHandle, std::string &eventSessionHandle, int state, std::string &nameString, std::string &uriString);
		void sessionStateChangeEvent(std::string &uriString, int statusCode, std::string &statusString, std::string &sessionHandle, int state,  bool isChannel, std::string &nameString);
		void participantStateChangeEvent(std::string &uriString, int statusCode, std::string &statusString, int state,  std::string &nameString, std::string &displayNameString, int participantType);
		void participantPropertiesEvent(std::string &uriString, int statusCode, std::string &statusString, bool isLocallyMuted, bool isModeratorMuted, bool isSpeaking, int volume, F32 energy);
		void auxAudioPropertiesEvent(F32 energy);
	
		void muteListChanged();
		
		/////////////////////////////
		// Sending updates of current state
		void setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot);
		void setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot);
		bool channelFromRegion(LLViewerRegion *region, std::string &name);
		void leaveChannel(void);		// call this on logout or teleport begin

		
		void setMuteMic(bool muted);		// Use this to mute the local mic (for when the client is minimized, etc), ignoring user PTT state.
		void setUserPTTState(bool ptt);
		bool getUserPTTState();
		void toggleUserPTTState(void);
		void setVoiceEnabled(bool enabled);
		static bool voiceEnabled();
		void setUsePTT(bool usePTT);
		void setPTTIsToggle(bool PTTIsToggle);
		void setPTTKey(std::string &key);
		void setEarLocation(S32 loc);
		void setVoiceVolume(F32 volume);
		void setMicGain(F32 volume);
		void setUserVolume(const LLUUID& id, F32 volume); // set's volume for specified agent, from 0-1 (where .5 is nominal)
		void setVivoxDebugServerName(std::string &serverName);
		void setLipSyncEnabled(U32 enabled);
		U32 lipSyncEnabled();

		// PTT key triggering
		void keyDown(KEY key, MASK mask);
		void keyUp(KEY key, MASK mask);
		void middleMouseState(bool down);
		
		/////////////////////////////
		// Accessors for data related to nearby speakers
		BOOL getVoiceEnabled(const LLUUID& id);		// true if we've received data for this avatar
		BOOL getIsSpeaking(const LLUUID& id);
		BOOL getIsModeratorMuted(const LLUUID& id);
		F32 getCurrentPower(const LLUUID& id);		// "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
		BOOL getPTTPressed(const LLUUID& id);			// This is the inverse of the "locally muted" property.
		BOOL getOnMuteList(const LLUUID& id);
		F32 getUserVolume(const LLUUID& id);
		LLString getDisplayName(const LLUUID& id);
		
		// MBW -- XXX -- Not sure how to get this data out of the TVC
		BOOL getUsingPTT(const LLUUID& id);
		serviceType getServiceType(const LLUUID& id);	// type of chat the user is involved in (see bHear scope doc for definitions of A/B/C)
		std::string getGroupID(const LLUUID& id);		// group ID if the user is in group chat (empty string if not applicable)

		/////////////////////////////
		BOOL getAreaVoiceDisabled();		// returns true if the area the avatar is in is speech-disabled.
											// Use this to determine whether to show a "no speech" icon in the menu bar.

		struct participantState
		{
		public:
			participantState(const std::string &uri);
			std::string mURI;
			std::string mName;
			std::string mDisplayName;
			bool mPTT;
			bool mIsSpeaking;
			bool mIsModeratorMuted;
			LLFrameTimer mSpeakingTimeout;
			F32	mLastSpokeTimestamp;
			F32 mPower;
			int mVolume;
			serviceType mServiceType;
			std::string mGroupID;
			bool mOnMuteList;		// true if this avatar is on the user's mute list (and should be muted)
			int mUserVolume;
			bool mVolumeDirty;		// true if this participant needs a volume command sent (either mOnMuteList or mUserVolume has changed)
			bool mAvatarIDValid;
			LLUUID mAvatarID;
		};
		typedef std::map<std::string, participantState*> participantMap;
		
		participantState *findParticipant(const std::string &uri);
		participantState *findParticipantByAvatar(LLVOAvatar *avatar);
		participantState *findParticipantByID(const LLUUID& id);
		
		participantMap *getParticipantList(void);

		void addObserver(LLVoiceClientParticipantObserver* observer);
		void removeObserver(LLVoiceClientParticipantObserver* observer);

		void addStatusObserver(LLVoiceClientStatusObserver* observer);
		void removeStatusObserver(LLVoiceClientStatusObserver* observer);
		
		static void onAvatarNameLookup(const LLUUID& id, const char* first, const char* last, BOOL is_group, void* user_data);
		typedef std::vector<std::string> deviceList;

		deviceList *getCaptureDevices();
		deviceList *getRenderDevices();
		
		void setNonSpatialChannel(
			const std::string &uri,
			const std::string &credentials);
		void setSpatialChannel(
			const std::string &uri,
			const std::string &credentials);
		void callUser(LLUUID &uuid);
		void answerInvite(std::string &sessionHandle, LLUUID& other_user_id);
		void declineInvite(std::string &sessionHandle);
		void leaveNonSpatialChannel();

		// Returns the URI of the current channel, or an empty string if not currently in a channel.
		// NOTE that it will return an empty string if it's in the process of joining a channel.
		std::string getCurrentChannel();
		
		// returns true iff the user is currently in a proximal (local spatial) channel.
		// Note that gestures should only fire if this returns true.
		bool inProximalChannel();

		std::string sipURIFromID(const LLUUID &id);

	private:

		// internal state for a simple state machine.  This is used to deal with the asynchronous nature of some of the messages.
		// Note: if you change this list, please make corresponding changes to LLVoiceClient::state2string().
		enum state
		{
			stateDisabled,				// Voice is turned off.
			stateStart,					// Class is initialized, socket is created
			stateDaemonLaunched,		// Daemon has been launched
			stateConnecting,			// connect() call has been issued
			stateIdle,					// socket is connected, ready for messaging
			stateConnectorStart,		// connector needs to be started
			stateConnectorStarting,		// waiting for connector handle
			stateConnectorStarted,		// connector handle received
			stateMicTuningNoLogin,		// mic tuning before login
			stateLoginRetry,			// need to retry login (failed due to changing password)
			stateLoginRetryWait,		// waiting for retry timer
			stateNeedsLogin,			// send login request
			stateLoggingIn,				// waiting for account handle
			stateLoggedIn,				// account handle received
			stateNoChannel,				// 
			stateMicTuningStart,
			stateMicTuningRunning,		
			stateMicTuningStop,
			stateSessionCreate,			// need to send Session.Create command
			stateSessionConnect,		// need to send Session.Connect command
			stateJoiningSession,		// waiting for session handle
			stateSessionJoined,			// session handle received
			stateRunning,				// in session, steady state
			stateLeavingSession,		// waiting for terminate session response
			stateSessionTerminated,		// waiting for terminate session response

			stateLoggingOut,			// waiting for logout response
			stateLoggedOut,				// logout response received
			stateConnectorStopping,		// waiting for connector stop
			stateConnectorStopped,		// connector stop received
			
			// We go to this state if the login fails because the account needs to be provisioned.
			
			// error states.  No way to recover from these yet.
			stateConnectorFailed,
			stateConnectorFailedWaiting,
			stateLoginFailed,
			stateLoginFailedWaiting,
			stateJoinSessionFailed,
			stateJoinSessionFailedWaiting,

			stateJail					// Go here when all else has failed.  Nothing will be retried, we're done.
		};
		
		state mState;
		bool mSessionTerminateRequested;
		bool mNonSpatialChannel;
		
		void setState(state inState);
		state getState(void)  { return mState; };
		static const char *state2string(state inState);
		
		void stateMachine();
		static void idle(void *user_data);
		
		LLHost mDaemonHost;
		LLSocket::ptr_t mSocket;
		bool mConnected;
		
		void closeSocket(void);
		
		LLPumpIO *mPump;
		friend class LLVivoxProtocolParser;
		
		std::string mAccountName;
		std::string mAccountPassword;
		std::string mAccountDisplayName;
		std::string mAccountFirstName;
		std::string mAccountLastName;
		
		std::string mNextP2PSessionURI;		// URI of the P2P session to join next
		std::string mNextSessionURI;		// URI of the session to join next
		std::string mNextSessionHandle;		// Session handle of the session to join next
		std::string mNextSessionHash;		// Password hash for the session to join next
		bool mNextSessionSpatial;			// Will next session be a spatial chat?
		bool mNextSessionNoReconnect;		// Next session should not auto-reconnect (i.e. user -> user chat)
		bool mNextSessionResetOnClose;		// If this is true, go back to spatial chat when the next session terminates.
		
		std::string mSessionStateEventHandle;	// session handle received in SessionStateChangeEvents
		std::string mSessionStateEventURI;		// session URI received in SessionStateChangeEvents
		
		bool mTuningMode;
		float mTuningEnergy;
		std::string mTuningAudioFile;
		int mTuningMicVolume;
		bool mTuningMicVolumeDirty;
		int mTuningSpeakerVolume;
		bool mTuningSpeakerVolumeDirty;
		state mTuningExitState;					// state to return to when we leave tuning mode.
		
		std::string mSpatialSessionURI;
		
		bool mSessionResetOnClose;
		
		int mVivoxErrorStatusCode;		
		std::string mVivoxErrorStatusString;
		
		std::string mChannelName;			// Name of the channel to be looked up 
		bool mAreaVoiceDisabled;
		std::string mSessionURI;			// URI of the session we're in.
		bool mSessionP2P;					// true if this session is a p2p call
		
		S32 mCurrentParcelLocalID;			// Used to detect parcel boundary crossings
		std::string mCurrentRegionName;		// Used to detect parcel boundary crossings
		
		std::string mConnectorHandle;	// returned by "Create Connector" message
		std::string mAccountHandle;		// returned by login message		
		std::string mSessionHandle;		// returned by ?
		U32 mCommandCookie;
	
		std::string mAccountServerName;
		std::string mAccountServerURI;
		
		int mLoginRetryCount;
		
		participantMap mParticipantMap;
		bool mParticipantMapChanged;
		
		deviceList mCaptureDevices;
		deviceList mRenderDevices;

		std::string mCaptureDevice;
		std::string mRenderDevice;
		bool mCaptureDeviceDirty;
		bool mRenderDeviceDirty;
		
		participantState *addParticipant(const std::string &uri);
		// Note: after removeParticipant returns, the participant* that was passed to it will have been deleted.
		// Take care not to use the pointer again after that.
		void removeParticipant(participantState *participant);
		void removeAllParticipants();

		void updateMuteState(participantState *participant);

		typedef std::map<std::string, std::string> channelMap;
		channelMap mChannelMap;
		
		// These are used by the parser when processing a channel list response.
		void clearChannelMap(void);
		void addChannelMapEntry(std::string &name, std::string &uri);
		std::string findChannelURI(std::string &name);
			
		// This should be called when the code detects we have changed parcels.
		// It initiates the call to the server that gets the parcel channel.
		void parcelChanged();
		
	void switchChannel(std::string uri = std::string(), bool spatial = true, bool noReconnect = false, std::string hash = "");
		void joinSession(std::string handle, std::string uri);
		
		std::string nameFromAvatar(LLVOAvatar *avatar);
		std::string nameFromID(const LLUUID &id);
		bool IDFromName(const std::string name, LLUUID &uuid);
		std::string displayNameFromAvatar(LLVOAvatar *avatar);
		std::string sipURIFromAvatar(LLVOAvatar *avatar);
		std::string sipURIFromName(std::string &name);
				
		void sendPositionalUpdate(void);
		
		void buildSetCaptureDevice(std::ostringstream &stream);
		void buildSetRenderDevice(std::ostringstream &stream);
		
		void enforceTether(void);
		
		bool		mSpatialCoordsDirty;
		
		LLVector3d	mCameraPosition;
		LLVector3d	mCameraRequestedPosition;
		LLVector3	mCameraVelocity;
		LLMatrix3	mCameraRot;

		LLVector3d	mAvatarPosition;
		LLVector3	mAvatarVelocity;
		LLMatrix3	mAvatarRot;
		
		bool		mPTTDirty;
		bool		mPTT;
		
		bool		mUsePTT;
		bool		mPTTIsMiddleMouse;
		KEY			mPTTKey;
		bool		mPTTIsToggle;
		bool		mUserPTTState;
		bool		mMuteMic;
		
		// Set to true when the mute state of someone in the participant list changes.
		// The code will have to walk the list to find the changed participant(s).
		bool		mVolumeDirty;
		
		enum
		{
			earLocCamera = 0,		// ear at camera
			earLocAvatar,			// ear at avatar
			earLocMixed				// ear at avatar location/camera direction
		};
		
		S32			mEarLocation;  
		
		bool		mSpeakerVolumeDirty;
		bool		mSpeakerMuteDirty;
		int			mSpeakerVolume;

		int			mMicVolume;
		bool		mMicVolumeDirty;
		
		bool		mVoiceEnabled;
		bool		mWriteInProgress;
		std::string mWriteString;
		size_t		mWriteOffset;
		
		LLTimer		mUpdateTimer;
		
		U32			mLipSyncEnabled;

		typedef std::set<LLVoiceClientParticipantObserver*> observer_set_t;
		observer_set_t mObservers;

		void notifyObservers();

		typedef std::set<LLVoiceClientStatusObserver*> status_observer_set_t;
		status_observer_set_t mStatusObservers;
		
		void notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status);
};

extern LLVoiceClient *gVoiceClient;

#endif //LL_VOICE_CLIENT_H



