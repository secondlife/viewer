/**
 * @file llvoiceclient.h
 * @brief Declaration of LLVoiceClient class which is the interface to the voice client process.
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
#ifndef LL_VOICE_CLIENT_H
#define LL_VOICE_CLIENT_H

class LLVOAvatar;

#include "lliopipe.h"
#include "llpumpio.h"
#include "llchainio.h"
#include "lliosocket.h"
#include "v3math.h"
#include "llframetimer.h"
#include "llsingleton.h"
#include "llcallingcard.h"   // for LLFriendObserver
#include "llsecapi.h"
#include "llcontrol.h"

// devices

class LLVoiceDevice
{
  public:
    std::string display_name; // friendly value for the user
    std::string full_name;  // internal value for selection

    LLVoiceDevice(const std::string& display_name, const std::string& full_name)
        :display_name(display_name)
        ,full_name(full_name)
    {
    };
};
typedef std::vector<LLVoiceDevice> LLVoiceDeviceList;

class LLVoiceClientParticipantObserver
{
public:
    virtual ~LLVoiceClientParticipantObserver() { }
    virtual void onParticipantsChanged() = 0;
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
    virtual void onChange(EStatusType status, const LLSD& channelInfo, bool proximal) = 0;

    static std::string status2string(EStatusType inStatus);
};

struct LLVoiceVersionInfo
{
    std::string voiceServerType;
    std::string internalVoiceServerType;
    int         majorVersion;
    int         minorVersion;
    std::string serverVersion;
    std::string mBuildVersion;
};

//////////////////////////////////
/// @class LLVoiceP2POutgoingCallInterface
/// @brief Outgoing call interface
///
/// For providers that support P2P signaling (vivox)
/////////////////////////////////

class LLVoiceP2POutgoingCallInterface
{
  public:
    // initiate an outgoing call to a user
    virtual void callUser(const LLUUID &agentID) = 0;
    virtual void hangup() = 0;
};

//////////////////////////////////
/// @class LLVoiceP2PIncomingCallInterface
/// @brief Incoming call interface
///
/// For providers that support P2P signaling (vivox)
/////////////////////////////////
class LLVoiceP2PIncomingCallInterface
{
  public:
    virtual ~LLVoiceP2PIncomingCallInterface() {}

    virtual bool answerInvite()  = 0;
    virtual void declineInvite() = 0;
};

typedef std::shared_ptr<LLVoiceP2PIncomingCallInterface> LLVoiceP2PIncomingCallInterfacePtr;

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

    virtual void init(LLPumpIO *pump)=0;    // Call this once at application startup (creates connector)
    virtual void terminate()=0; // Call this to clean up during shutdown

    virtual void updateSettings()=0; // call after loading settings and whenever they change

    virtual bool isVoiceWorking() const = 0; // connected to a voice server and voice channel

    virtual void setHidden(bool hidden)=0;  //  Hides the user from voice.

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
    virtual bool deviceSettingsUpdated() = 0;

    // Requery the vivox daemon for the current list of input/output devices.
    // If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
    // (use this if you want to know when it's done).
    // If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
    virtual void refreshDeviceLists(bool clearCurrentList = true)=0;

    virtual void setCaptureDevice(const std::string& name)=0;
    virtual void setRenderDevice(const std::string& name)=0;

    virtual LLVoiceDeviceList& getCaptureDevices()=0;
    virtual LLVoiceDeviceList& getRenderDevices()=0;

    virtual bool isCaptureNoDevice() = 0;
    virtual bool isRenderNoDevice() = 0;

    virtual void getParticipantList(std::set<LLUUID> &participants)=0;
    virtual bool isParticipant(const LLUUID& speaker_id)=0;
    //@}

    ////////////////////////////
    /// @ name Channel stuff
    //@{
    // returns true iff the user is currently in a proximal (local spatial) channel.
    // Note that gestures should only fire if this returns true.
    virtual bool inProximalChannel()=0;

    virtual void setNonSpatialChannel(const LLSD& channelInfo,
                                      bool notify_on_first_join,
                                      bool hangup_on_last_leave)=0;

    virtual bool setSpatialChannel(const LLSD& channelInfo)=0;

    virtual void leaveNonSpatialChannel() = 0;
    virtual void processChannels(bool process) = 0;

    virtual bool isCurrentChannel(const LLSD &channelInfo) = 0;
    virtual bool compareChannels(const LLSD &channelInfo1, const LLSD &channelInfo2) = 0;

    //@}


    //////////////////////////
    /// @name p2p
    //@{

    // initiate a call with a peer using the P2P interface, which only applies to some
    // voice server types.  Otherwise, a group call should be used for P2P
    virtual LLVoiceP2POutgoingCallInterface* getOutgoingCallInterface() = 0;

    // an incoming call was received, and the incoming call dialogue is asking for an interface to
    // answer or decline.
    virtual LLVoiceP2PIncomingCallInterfacePtr getIncomingCallInterface(const LLSD &voice_call_info) = 0;
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
    virtual void setVoiceEnabled(bool enabled)=0;
    virtual void setMuteMic(bool muted)=0;      // Set the mute state of the local mic.
    //@}

    //////////////////////////
    /// @name nearby speaker accessors
    //@{
    virtual std::string getDisplayName(const LLUUID& id)=0;
    virtual bool isParticipantAvatar(const LLUUID &id)=0;
    virtual bool getIsSpeaking(const LLUUID& id)=0;
    virtual bool getIsModeratorMuted(const LLUUID& id)=0;
    virtual F32 getCurrentPower(const LLUUID& id)=0;        // "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
    virtual F32 getUserVolume(const LLUUID& id)=0;
    virtual void setUserVolume(const LLUUID& id, F32 volume)=0; // set's volume for specified agent, from 0-1 (where .5 is nominal)
    //@}

    //////////////////////////
    /// @name text chat
    //@{
    virtual bool isSessionTextIMPossible(const LLUUID& id)=0;
    virtual bool isSessionCallBackPossible(const LLUUID& id)=0;
    //virtual bool sendTextMessage(const LLUUID& participant_id, const std::string& message)=0;
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

    virtual std::string sipURIFromID(const LLUUID &id) const=0;
    virtual LLSD getP2PChannelInfoTemplate(const LLUUID& id) const=0;
    //@}

};


//////////////////////////////////
/// @class LLVoiceEffectObserver
class LLVoiceEffectObserver
{
public:
    virtual ~LLVoiceEffectObserver() { }
    virtual void onVoiceEffectChanged(bool effect_list_updated) = 0;
};

typedef std::multimap<const std::string, const LLUUID, LLDictionaryLess> voice_effect_list_t;

//////////////////////////////////
/// @class LLVoiceEffectInterface
/// @brief Voice effect module interface
///
/// Voice effect modules should provide an implementation for this interface.
/////////////////////////////////

class LLVoiceEffectInterface
{
public:
    LLVoiceEffectInterface() {}
    virtual ~LLVoiceEffectInterface() {}

    //////////////////////////
    /// @name Accessors
    //@{
    virtual bool setVoiceEffect(const LLUUID& id) = 0;
    virtual const LLUUID getVoiceEffect() = 0;
    virtual LLSD getVoiceEffectProperties(const LLUUID& id) = 0;

    virtual void refreshVoiceEffectLists(bool clear_lists) = 0;
    virtual const voice_effect_list_t &getVoiceEffectList() const = 0;
    virtual const voice_effect_list_t &getVoiceEffectTemplateList() const = 0;
    //@}

    //////////////////////////////
    /// @name Status notification
    //@{
    virtual void addObserver(LLVoiceEffectObserver* observer) = 0;
    virtual void removeObserver(LLVoiceEffectObserver* observer) = 0;
    //@}

    //////////////////////////////
    /// @name Preview buffer
    //@{
    virtual void enablePreviewBuffer(bool enable) = 0;
    virtual void recordPreviewBuffer() = 0;
    virtual void playPreviewBuffer(const LLUUID& effect_id = LLUUID::null) = 0;
    virtual void stopPreviewBuffer() = 0;

    virtual bool isPreviewRecording() = 0;
    virtual bool isPreviewPlaying() = 0;
    //@}
};


class LLVoiceClient: public LLParamSingleton<LLVoiceClient>
{
    LLSINGLETON(LLVoiceClient, LLPumpIO *pump);
    LOG_CLASS(LLVoiceClient);
    ~LLVoiceClient();

public:
    typedef boost::signals2::signal<void(void)> micro_changed_signal_t;
    micro_changed_signal_t mMicroChangedSignal;

    void terminate();   // Call this to clean up during shutdown

    const LLVoiceVersionInfo getVersion();

    static const F32 OVERDRIVEN_POWER_LEVEL;

    static const F32 VOLUME_MIN;
    static const F32 VOLUME_DEFAULT;
    static const F32 VOLUME_MAX;

    void updateSettings(); // call after loading settings and whenever they change

    bool isVoiceWorking() const; // connected to a voice server and voice channel

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
    bool deviceSettingsUpdated();   // returns true when the device list has been updated recently.

    // Requery the vivox daemon for the current list of input/output devices.
    // If you pass true for clearCurrentList, deviceSettingsAvailable() will be false until the query has completed
    // (use this if you want to know when it's done).
    // If you pass false, you'll have no way to know when the query finishes, but the device lists will not appear empty in the interim.
    void refreshDeviceLists(bool clearCurrentList = true);

    void setCaptureDevice(const std::string& name);
    void setRenderDevice(const std::string& name);
    bool isCaptureNoDevice();
    bool isRenderNoDevice();
    void setHidden(bool hidden);

    const LLVoiceDeviceList& getCaptureDevices();
    const LLVoiceDeviceList& getRenderDevices();

    ////////////////////////////
    // Channel stuff
    //

    // returns true iff the user is currently in a proximal (local spatial) channel.
    // Note that gestures should only fire if this returns true.
    bool inProximalChannel();

    void setNonSpatialChannel(const LLSD& channelInfo,
                              bool notify_on_first_join,
                              bool hangup_on_last_leave);

    void setSpatialChannel(const LLSD &channelInfo);

    void activateSpatialChannel(bool activate);

    void leaveNonSpatialChannel();

    bool isCurrentChannel(const LLSD& channelInfo);

    bool compareChannels(const LLSD& channelInfo1, const LLSD& channelInfo2);

    // initiate a call with a peer using the P2P interface, which only applies to some
    // voice server types.  Otherwise, a group call should be used for P2P
    LLVoiceP2POutgoingCallInterface* getOutgoingCallInterface(const LLSD& voiceChannelInfo = LLSD());

    LLVoiceP2PIncomingCallInterfacePtr getIncomingCallInterface(const LLSD &voiceCallInfo);

    /////////////////////////////
    // Sending updates of current state


    void setVoiceVolume(F32 volume);
    void setMicGain(F32 volume);
    void setUserVolume(const LLUUID& id, F32 volume); // set's volume for specified agent, from 0-1 (where .5 is nominal)
    bool voiceEnabled();
    void setMuteMic(bool muted);        // Use this to mute the local mic (for when the client is minimized, etc), ignoring user PTT state.
    void setUserPTTState(bool ptt);
    bool getUserPTTState();
    void toggleUserPTTState(void);
    void inputUserControlState(bool down);  // interpret any sort of up-down mic-open control input according to ptt-toggle prefs
    static void setVoiceEnabled(bool enabled);

    void setUsePTT(bool usePTT);
    void setPTTIsToggle(bool PTTIsToggle);
    bool getPTTIsToggle();

    void updateMicMuteLogic();

    boost::signals2::connection MicroChangedCallback(const micro_changed_signal_t::slot_type& cb ) { return mMicroChangedSignal.connect(cb); }


    /////////////////////////////
    // Accessors for data related to nearby speakers
    bool getVoiceEnabled(const LLUUID& id) const;     // true if we've received data for this avatar
    std::string getDisplayName(const LLUUID& id) const;
    bool isOnlineSIP(const LLUUID &id);
    bool isParticipantAvatar(const LLUUID &id);
    bool getIsSpeaking(const LLUUID& id);
    bool getIsModeratorMuted(const LLUUID& id);
    F32 getCurrentPower(const LLUUID& id);      // "power" is related to "amplitude" in a defined way.  I'm just not sure what the formula is...
    bool getOnMuteList(const LLUUID& id);
    F32 getUserVolume(const LLUUID& id);

    /////////////////////////////
    void getParticipantList(std::set<LLUUID> &participants) const;
    bool isParticipant(const LLUUID& speaker_id) const;

    //////////////////////////
    /// @name text chat
    //@{
    bool isSessionTextIMPossible(const LLUUID& id);
    bool isSessionCallBackPossible(const LLUUID& id);
    //bool sendTextMessage(const LLUUID& participant_id, const std::string& message) const {return true;} ;
    //@}

    void setSpatialVoiceModule(const std::string& voice_server_type);
    void setNonSpatialVoiceModule(const std::string &voice_server_type);

    void userAuthorized(const std::string& user_id,
                        const LLUUID &agentID);

    void onRegionChanged();

    static void addObserver(LLVoiceClientStatusObserver* observer);
    static void removeObserver(LLVoiceClientStatusObserver* observer);
    static void addObserver(LLFriendObserver* observer);
    static void removeObserver(LLFriendObserver* observer);
    static void addObserver(LLVoiceClientParticipantObserver* observer);
    static void removeObserver(LLVoiceClientParticipantObserver* observer);

    std::string sipURIFromID(const LLUUID &id) const;
    LLSD getP2PChannelInfoTemplate(const LLUUID& id) const;

    //////////////////////////
    /// @name Voice effects
    //@{
    bool getVoiceEffectEnabled() const { return mVoiceEffectEnabled; };
    LLUUID getVoiceEffectDefault() const { return LLUUID(mVoiceEffectDefault); };

    // Returns NULL if voice effects are not supported, or not enabled.
    LLVoiceEffectInterface* getVoiceEffectInterface() const;
    //@}

    void handleSimulatorFeaturesReceived(const LLSD &simulatorFeatures);

  private:

    void init(LLPumpIO *pump);

protected:

    static bool onVoiceEffectsNotSupported(const LLSD &notification, const LLSD &response);

    LLVoiceModuleInterface* mSpatialVoiceModule;
    LLVoiceModuleInterface* mNonSpatialVoiceModule;
    LLSD                    mSpatialCredentials;  // used to store spatial credentials for vivox
                                                  // so they're available when the region voice
                                                  // server is retrieved.
    LLPumpIO *m_servicePump;

    boost::signals2::connection  mSimulatorFeaturesReceivedSlot;
    boost::signals2::connection  mRegionChangedCallbackSlot;

    LLCachedControl<bool> mVoiceEffectEnabled;
    LLCachedControl<std::string> mVoiceEffectDefault;
    bool        mVoiceEffectSupportNotified;

    bool        mPTTDirty;
    bool        mPTT;

    bool        mUsePTT;
    S32         mPTTMouseButton;
    KEY         mPTTKey;
    bool        mPTTIsToggle;
    bool        mUserPTTState;
    bool        mMuteMic;
    bool        mDisableMic;
};

/**
 * Speaker volume storage helper class
 **/
class LLSpeakerVolumeStorage : public LLSingleton<LLSpeakerVolumeStorage>
{
    LLSINGLETON(LLSpeakerVolumeStorage);
    ~LLSpeakerVolumeStorage();
    LOG_CLASS(LLSpeakerVolumeStorage);

protected:
    virtual void cleanupSingleton() override;

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
    const static std::string SETTINGS_FILE_NAME;

    void load();
    void save();

    static F32 transformFromLegacyVolume(F32 volume_in);
    static F32 transformToLegacyVolume(F32 volume_in);

    typedef std::map<LLUUID, F32> speaker_data_map_t;
    speaker_data_map_t mSpeakersData;
};

#endif //LL_VOICE_CLIENT_H



