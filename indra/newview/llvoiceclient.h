/** 
 * @file llvoiceclient.h
 * @brief Declaration of LLVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

class LLVOAvatar;

#include "lliopipe.h"
#include "llpumpio.h"
#include "llchainio.h"
#include "lliosocket.h"
#include "v3math.h"
#include "llframetimer.h"
#include "llviewerregion.h"
#include "llcallingcard.h"   // for LLFriendObserver
#include "llsecapi.h"

// devices

typedef std::vector<std::string> LLVoiceDeviceList;	


class LLVoiceClientParticipantObserver
{
public:
	virtual ~LLVoiceClientParticipantObserver() { }
	virtual void onChange() = 0;
};


///////////////////////////////////
/// @class LLVoiceClientStatusObserver
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
		STATUS_VOICE_ENABLED,
		BEGIN_ERROR_STATUS,
		ERROR_CHANNEL_FULL,
		ERROR_CHANNEL_LOCKED,
		ERROR_NOT_AVAILABLE,
		ERROR_UNKNOWN
	} EStatusType;

	virtual ~LLVoiceClientStatusObserver() { }
	virtual void onChange(EStatusType status, const std::string &channelURI, bool proximal) = 0;

	static std::string status2string(EStatusType inStatus);
};

struct LLVoiceVersionInfo
{
	std::string serverType;
	std::string serverVersion;
};

//////////////////////////////////
/// @class LLVoiceModuleInterface
/// @brief Voice module interface
///
/// Voice modules should provide an implementation for this interface.
/////////////////////////////////

class LLVoiceModuleInterface
{
public:
	LLVoiceModuleInterface() {}
	virtual ~LLVoiceModuleInterface() {}
	
	virtual void init(LLPumpIO *pump)=0;	// Call this once at application startup (creates connector)
	virtual void terminate()=0;	// Call this to clean up during shutdown
	
	virtual void updateSettings()=0; // call after loading settings and whenever they change
	
	virtual bool isVoiceWorking()=0; // connected to a voice server and voice channel

	virtual const LLVoiceVersionInfo& getVersion()=0;
	
	/////////////////////
	/// @name Tuning
	//@{
	virtual void tuningStart()=0;
	virtual void tuningStop()=0;
	virtual bool inTuningMode()=0;
	
	virtual void tuningSetMicVolume(float volume)=0;
	virtual void tuningSetSpeakerVolume(float volume)=0;
	virtual float tuningGetEnergy(void)=0;
	//@}
	
	/////////////////////
	/// @name Devices
	//@{
	// This returns true when it's safe to bring up the "device settings" dialog in the prefs.
	// i.e. when the daemon is running and connected, and the device lists are populated.
	virtual bool deviceSettingsAvailable()=0;
	
	// Requery the vivox daemon for the current list of input/output devices.
	// If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
	// (use this if you want to know when it's done).
	// If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
	virtual void refreshDeviceLists(bool clearCurrentList = true)=0;
	
	virtual void setCaptureDevice(const std::string& name)=0;
	virtual void setRenderDevice(const std::string& name)=0;
	
	virtual LLVoiceDeviceList& getCaptureDevices()=0;
	virtual LLVoiceDeviceList& getRenderDevices()=0;
	
	virtual void getParticipantList(std::set<LLUUID> &participants)=0;
	virtual bool isParticipant(const LLUUID& speaker_id)=0;
	//@}
	
	////////////////////////////
	/// @ name Channel stuff
	//@{
	// returns true iff the user is currently in a proximal (local spatial) channel.
	// Note that gestures should only fire if this returns true.
	virtual bool inProximalChannel()=0;
	
	virtual void setNonSpatialChannel(const std::string &uri,
									  const std::string &credentials)=0;
	
	virtual void setSpatialChannel(const std::string &uri,
								   const std::string &credentials)=0;
	
	virtual void leaveNonSpatialChannel()=0;
	
	virtual void leaveChannel(void)=0;	
	
	// Returns the URI of the current channel, or an empty string if not currently in a channel.
	// NOTE that it will return an empty string if it's in the process of joining a channel.
	virtual std::string getCurrentChannel()=0;
	//@}
	
	
	//////////////////////////
	/// @name invitations
	//@{
	// start a voice channel with the specified user
	virtual void callUser(const LLUUID &uuid)=0;
	virtual bool isValidChannel(std::string& channelHandle)=0;
	virtual bool answerInvite(std::string &channelHandle)=0;
	virtual void declineInvite(std::string &channelHandle)=0;
	//@}
	
	/////////////////////////
	/// @name Volume/gain
	//@{
	virtual void setVoiceVolume(F32 volume)=0;
	virtual void setMicGain(F32 volume)=0;
	//@}
	
	/////////////////////////
	/// @name enable disable voice and features
	//@{
	virtual bool voiceEnabled()=0;
	virtual void setVoiceEnabled(bool enabled)=0;
	virtual void setLipSyncEnabled(BOOL enabled)=0;
	virtual BOOL lipSyncEnabled()=0;	
	virtual void setMuteMic(bool muted)=0;		// Use this to mute the local mic (for when the client is minimized, etc), ignoring user PTT state.
	//@}
	
	////////////////////////
	/// @name PTT
	//@{
	virtual void setUserPTTState(bool ptt)=0;
	virtual bool getUserPTTState()=0;
	virtual void setUsePTT(bool usePTT)=0;
	virtual void setPTTIsToggle(bool PTTIsToggle)=0;
	virtual bool getPTTIsToggle()=0;	
	virtual void toggleUserPTTState(void)=0;
	virtual void inputUserControlState(bool down)=0;  // interpret any sort of up-down mic-open control input according to ptt-toggle prefs
	
	virtual void keyDown(KEY key, MASK mask)=0;
	virtual void keyUp(KEY key, MASK mask)=0;
	virtual void middleMouseState(bool down)=0;
	//@}
	
	//////////////////////////
	/// @name nearby speaker accessors
	//@{


	virtual BOOL getVoiceEnabled(const LLUUID& id)=0;		// true if we've received data for this avatar
	virtual std::string getDisplayName(const LLUUID& id)=0;
	virtual BOOL isOnlineSIP(const LLUUID &id)=0;	
	virtual BOOL isParticipantAvatar(const LLUUID &id)=0;
	virtual BOOL getIsSpeaking(const LLUUID& id)=0;
	virtual BOOL getIsModeratorMuted(const LLUUID& id)=0;
	virtual F32 getCurrentPower(const LLUUID& id)=0;		// "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
	virtual BOOL getOnMuteList(const LLUUID& id)=0;
	virtual F32 getUserVolume(const LLUUID& id)=0;
	virtual void setUserVolume(const LLUUID& id, F32 volume)=0; // set's volume for specified agent, from 0-1 (where .5 is nominal)	
	//@}
	
	//////////////////////////
	/// @name text chat
	//@{
	virtual BOOL isSessionTextIMPossible(const LLUUID& id)=0;
	virtual BOOL isSessionCallBackPossible(const LLUUID& id)=0;
	virtual BOOL sendTextMessage(const LLUUID& participant_id, const std::string& message)=0;
	virtual void endUserIMSession(const LLUUID &uuid)=0;	
	//@}
	
	// authorize the user
	virtual void userAuthorized(const std::string& user_id,
								const LLUUID &agentID)=0;
	
	//////////////////////////////
	/// @name Status notification
	//@{
	virtual void addObserver(LLVoiceClientStatusObserver* observer)=0;
	virtual void removeObserver(LLVoiceClientStatusObserver* observer)=0;
	virtual void addObserver(LLFriendObserver* observer)=0;
	virtual void removeObserver(LLFriendObserver* observer)=0;	
	virtual void addObserver(LLVoiceClientParticipantObserver* observer)=0;
	virtual void removeObserver(LLVoiceClientParticipantObserver* observer)=0;	
	//@}
	
	virtual std::string sipURIFromID(const LLUUID &id)=0;
	//@}
	
};


class LLVoiceClient: public LLSingleton<LLVoiceClient>
{
	LOG_CLASS(LLVoiceClient);
public:
	LLVoiceClient();	
	~LLVoiceClient();

	void init(LLPumpIO *pump);	// Call this once at application startup (creates connector)
	void terminate();	// Call this to clean up during shutdown
	
	const LLVoiceVersionInfo getVersion();
	
	static const F32 OVERDRIVEN_POWER_LEVEL;

	static const F32 VOLUME_MIN;
	static const F32 VOLUME_DEFAULT;
	static const F32 VOLUME_MAX;

	void updateSettings(); // call after loading settings and whenever they change

	bool isVoiceWorking(); // connected to a voice server and voice channel

	// tuning
	void tuningStart();
	void tuningStop();
	bool inTuningMode();
		
	void tuningSetMicVolume(float volume);
	void tuningSetSpeakerVolume(float volume);
	float tuningGetEnergy(void);
				
	// devices
	
	// This returns true when it's safe to bring up the "device settings" dialog in the prefs.
	// i.e. when the daemon is running and connected, and the device lists are populated.
	bool deviceSettingsAvailable();
		
	// Requery the vivox daemon for the current list of input/output devices.
	// If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
	// (use this if you want to know when it's done).
	// If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
	void refreshDeviceLists(bool clearCurrentList = true);

	void setCaptureDevice(const std::string& name);
	void setRenderDevice(const std::string& name);

	const LLVoiceDeviceList& getCaptureDevices();
	const LLVoiceDeviceList& getRenderDevices();

	////////////////////////////
	// Channel stuff
	//
	
	// returns true iff the user is currently in a proximal (local spatial) channel.
	// Note that gestures should only fire if this returns true.
	bool inProximalChannel();
	void setNonSpatialChannel(
							  const std::string &uri,
							  const std::string &credentials);
	void setSpatialChannel(
						   const std::string &uri,
						   const std::string &credentials);
	void leaveNonSpatialChannel();
	
	// Returns the URI of the current channel, or an empty string if not currently in a channel.
	// NOTE that it will return an empty string if it's in the process of joining a channel.
	std::string getCurrentChannel();
	// start a voice channel with the specified user
	void callUser(const LLUUID &uuid);
	bool isValidChannel(std::string& channelHandle);
	bool answerInvite(std::string &channelHandle);
	void declineInvite(std::string &channelHandle);	
	void leaveChannel(void);		// call this on logout or teleport begin
	
	
	/////////////////////////////
	// Sending updates of current state
	

	void setVoiceVolume(F32 volume);
	void setMicGain(F32 volume);
	void setUserVolume(const LLUUID& id, F32 volume); // set's volume for specified agent, from 0-1 (where .5 is nominal)		
	bool voiceEnabled();
	void setLipSyncEnabled(BOOL enabled);
	void setMuteMic(bool muted);		// Use this to mute the local mic (for when the client is minimized, etc), ignoring user PTT state.
	void setUserPTTState(bool ptt);
	bool getUserPTTState();
	void toggleUserPTTState(void);
	void inputUserControlState(bool down);  // interpret any sort of up-down mic-open control input according to ptt-toggle prefs	
	void setVoiceEnabled(bool enabled);

	void setUsePTT(bool usePTT);
	void setPTTIsToggle(bool PTTIsToggle);
	bool getPTTIsToggle();	
	
	BOOL lipSyncEnabled();
	
	// PTT key triggering
	void keyDown(KEY key, MASK mask);
	void keyUp(KEY key, MASK mask);
	void middleMouseState(bool down);
	
	
	/////////////////////////////
	// Accessors for data related to nearby speakers
	BOOL getVoiceEnabled(const LLUUID& id);		// true if we've received data for this avatar
	std::string getDisplayName(const LLUUID& id);	
	BOOL isOnlineSIP(const LLUUID &id);
	BOOL isParticipantAvatar(const LLUUID &id);
	BOOL getIsSpeaking(const LLUUID& id);
	BOOL getIsModeratorMuted(const LLUUID& id);
	F32 getCurrentPower(const LLUUID& id);		// "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
	BOOL getOnMuteList(const LLUUID& id);
	F32 getUserVolume(const LLUUID& id);

	/////////////////////////////
	BOOL getAreaVoiceDisabled();		// returns true if the area the avatar is in is speech-disabled.
													  // Use this to determine whether to show a "no speech" icon in the menu bar.
	void getParticipantList(std::set<LLUUID> &participants);
	bool isParticipant(const LLUUID& speaker_id);
	
	//////////////////////////
	/// @name text chat
	//@{
	BOOL isSessionTextIMPossible(const LLUUID& id);
	BOOL isSessionCallBackPossible(const LLUUID& id);
	BOOL sendTextMessage(const LLUUID& participant_id, const std::string& message);
	void endUserIMSession(const LLUUID &uuid);	
	//@}
	

	void userAuthorized(const std::string& user_id,
			const LLUUID &agentID);
	
	void addObserver(LLVoiceClientStatusObserver* observer);
	void removeObserver(LLVoiceClientStatusObserver* observer);
	void addObserver(LLFriendObserver* observer);
	void removeObserver(LLFriendObserver* observer);
	void addObserver(LLVoiceClientParticipantObserver* observer);
	void removeObserver(LLVoiceClientParticipantObserver* observer);
	
	std::string sipURIFromID(const LLUUID &id);	
		
protected:
	LLVoiceModuleInterface* mVoiceModule;
	LLPumpIO *m_servicePump;
};

/**
 * Speaker volume storage helper class
 **/
class LLSpeakerVolumeStorage : public LLSingleton<LLSpeakerVolumeStorage>
{
	LOG_CLASS(LLSpeakerVolumeStorage);
public:

	/**
	 * Stores volume level for specified user.
	 *
	 * @param[in] speaker_id - LLUUID of user to store volume level for.
	 * @param[in] volume - volume level to be stored for user.
	 */
	void storeSpeakerVolume(const LLUUID& speaker_id, F32 volume);

	/**
	 * Gets stored volume level for specified speaker
	 *
	 * @param[in] speaker_id - LLUUID of user to retrieve volume level for.
	 * @param[out] volume - set to stored volume if found, otherwise unmodified.
	 * @return - true if a stored volume is found.
	 */
	bool getSpeakerVolume(const LLUUID& speaker_id, F32& volume);

	/**
	 * Removes stored volume level for specified user.
	 *
	 * @param[in] speaker_id - LLUUID of user to remove.
	 */
	void removeSpeakerVolume(const LLUUID& speaker_id);

private:
	friend class LLSingleton<LLSpeakerVolumeStorage>;
	LLSpeakerVolumeStorage();
	~LLSpeakerVolumeStorage();

	const static std::string SETTINGS_FILE_NAME;

	void load();
	void save();

	static F32 LLSpeakerVolumeStorage::transformFromLegacyVolume(F32 volume_in);
	static F32 LLSpeakerVolumeStorage::transformToLegacyVolume(F32 volume_in);

	typedef std::map<LLUUID, F32> speaker_data_map_t;
	speaker_data_map_t mSpeakersData;
};

#endif //LL_VOICE_CLIENT_H



