/**
* @file llpanelprofile.h
* @brief Profile panel
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

#ifndef LL_LLPANELPROFILE_H
#define LL_LLPANELPROFILE_H

#include "llavatarpropertiesprocessor.h"
#include "llcallingcard.h"
#include "llfloater.h"
#include "llpanel.h"
#include "llpanelavatar.h"
#include "llmediactrl.h"
#include "llvoiceclient.h"

// class LLPanelProfileClassifieds;
// class LLTabContainer;

// class LLPanelProfileSecondLife;
// class LLPanelProfileWeb;
// class LLPanelProfilePicks;
// class LLPanelProfileFirstLife;
// class LLPanelProfileNotes;

class LLAvatarName;
class LLButton;
class LLCheckBoxCtrl;
class LLComboBox;
class LLIconCtrl;
class LLTabContainer;
class LLTextBox;
class LLTextureCtrl;
class LLMediaCtrl;
class LLGroupList;
class LLTextBase;
class LLMenuButton;
class LLLineEditor;
class LLTextEditor;
class LLThumbnailCtrl;
class LLPanelProfileClassifieds;
class LLPanelProfilePicks;
class LLViewerFetchedTexture;


/**
* Panel for displaying Avatar's second life related info.
*/
class LLPanelProfileSecondLife
	: public LLPanelProfileTab
	, public LLFriendObserver
	, public LLVoiceClientStatusObserver
{
public:
	LLPanelProfileSecondLife();
	/*virtual*/ ~LLPanelProfileSecondLife();

	void onOpen(const LLSD& key) override;

    bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
                                   EDragAndDropType cargo_type,
                                   void* cargo_data,
                                   EAcceptance* accept,
                                   std::string& tooltip_msg) override;

	/**
	 * LLFriendObserver trigger
	 */
	void changed(U32 mask) override;

	// Implements LLVoiceClientStatusObserver::onChange() to enable the call
	// button when voice is available
	void onChange(EStatusType status, const std::string &channelURI, bool proximal) override;

	void setAvatarId(const LLUUID& avatar_id) override;

	bool postBuild() override;

	void resetData() override;

	/**
	 * Sends update data request to server.
	 */
	void updateData() override;
    void refreshName();

	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

    void setProfileImageUploading(bool loading);
    void setProfileImageUploaded(const LLUUID &image_asset_id);

    bool hasUnsavedChanges() override;
    void commitUnsavedChanges() override;

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

protected:
	/**
	 * Process profile related data received from server.
	 */
	void processProfileProperties(const LLAvatarData* avatar_data);

	/**
	 * Processes group related data received from server.
	 */
	void processGroupProperties(const LLAvatarGroups* avatar_groups);

	/**
	 * Fills common for Avatar profile and My Profile fields.
	 */
	void fillCommonData(const LLAvatarData* avatar_data);

	/**
	 * Fills partner data.
	 */
	void fillPartnerData(const LLAvatarData* avatar_data);

	/**
	 * Fills account status.
	 */
	void fillAccountStatus(const LLAvatarData* avatar_data);

    /**
     * Sets permissions specific icon
     */
    void fillRightsData();

    /**
     * Fills user name, display name, age.
     */
    void fillAgeData(const LLDate &born_on);

    void onImageLoaded(bool success, LLViewerFetchedTexture *imagep);
    static void onImageLoaded(bool success,
                              LLViewerFetchedTexture *src_vi,
                              LLImageRaw* src,
                              LLImageRaw* aux_src,
                              S32 discard_level,
                              bool final,
                              void* userdata);

	/**
	 * Displays avatar's online status if possible.
	 *
	 * Requirements from EXT-3880:
	 * For friends:
	 * - Online when online and privacy settings allow to show
	 * - Offline when offline and privacy settings allow to show
	 * - Else: nothing
	 * For other avatars:
	 *	- Online when online and was not set in Preferences/"Only Friends & Groups can see when I am online"
	 *	- Else: Offline
	 */
	void updateOnlineStatus();
	void processOnlineStatus(bool is_friend, bool show_online, bool online);

private:
    void setLoaded() override;
    void onCommitMenu(const LLSD& userdata);
    bool onEnableMenu(const LLSD& userdata);
    bool onCheckMenu(const LLSD& userdata);
	void onAvatarNameCacheSetName(const LLUUID& id, const LLAvatarName& av_name);

    void setDescriptionText(const std::string &text);
    void onSetDescriptionDirty();
    void onShowInSearchCallback();
    void onSaveDescriptionChanges();
    void onDiscardDescriptionChanges();
    void onShowAgentPermissionsDialog();
    void onShowAgentProfileTexture();
    void onShowTexturePicker();
    void onCommitProfileImage(const LLUUID& id);

private:
	typedef std::map<std::string, LLUUID> group_map_t;
	group_map_t				mGroups;
	void					openGroupProfile();

	LLGroupList*		mGroupList;
    LLComboBox*			mShowInSearchCombo;
    LLThumbnailCtrl*	mSecondLifePic;
	LLPanel*			mSecondLifePicLayout;
    LLTextEditor*		mDescriptionEdit;
    LLMenuButton*		mAgentActionMenuButton;
    LLButton*			mSaveDescriptionChanges;
    LLButton*			mDiscardDescriptionChanges;
    LLIconCtrl*			mCanSeeOnlineIcon;
    LLIconCtrl*			mCantSeeOnlineIcon;
    LLIconCtrl*			mCanSeeOnMapIcon;
    LLIconCtrl*			mCantSeeOnMapIcon;
    LLIconCtrl*			mCanEditObjectsIcon;
    LLIconCtrl*			mCantEditObjectsIcon;

    LLHandle<LLFloater>	mFloaterPermissionsHandle;
    LLHandle<LLFloater>	mFloaterProfileTextureHandle;
    LLHandle<LLFloater>	mFloaterTexturePickerHandle;

    bool				mHasUnsavedDescriptionChanges;
	bool				mVoiceStatus;
    bool				mWaitingForImageUpload;
    bool				mAllowPublish;
    std::string			mDescriptionText;
    LLUUID				mImageId;

	boost::signals2::connection	mAvatarNameCacheConnection;
};


/**
* Panel for displaying Avatar's web profile and home page.
*/
class LLPanelProfileWeb
	: public LLPanelProfileTab
	, public LLViewerMediaObserver
{
public:
	LLPanelProfileWeb();
	/*virtual*/ ~LLPanelProfileWeb();

	void onOpen(const LLSD& key) override;

	bool postBuild() override;

	void resetData() override;

	/**
	 * Loads web profile.
	 */
	void updateData() override;

	void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event) override;

	void onAvatarNameCache(const LLUUID& agent_id, const LLAvatarName& av_name);

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

protected:
	void onCommitLoad(LLUICtrl* ctrl);

private:
	std::string			mURLHome;
	std::string			mURLWebProfile;
	LLMediaCtrl*		mWebBrowser;

	LLFrameTimer		mPerformanceTimer;
	bool				mFirstNavigate;

	boost::signals2::connection	mAvatarNameCacheConnection;
};

/**
* Panel for displaying Avatar's first life related info.
*/
class LLPanelProfileFirstLife
	: public LLPanelProfileTab
{
public:
	LLPanelProfileFirstLife();
	/*virtual*/ ~LLPanelProfileFirstLife();

	void onOpen(const LLSD& key) override;

	bool postBuild() override;

    void processProperties(const LLAvatarData* avatar_data);

	void resetData() override;

    void setProfileImageUploading(bool loading);
    void setProfileImageUploaded(const LLUUID &image_asset_id);

    bool hasUnsavedChanges() override { return mHasUnsavedChanges; }
    void commitUnsavedChanges() override;

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

protected:
	void setLoaded() override;

    void onUploadPhoto();
    void onChangePhoto();
    void onRemovePhoto();
    void onCommitPhoto(const LLUUID& id);
    void setDescriptionText(const std::string &text);
    void onSetDescriptionDirty();
    void onSaveDescriptionChanges();
    void onDiscardDescriptionChanges();

	LLTextEditor*	mDescriptionEdit;
    LLThumbnailCtrl* mPicture;
    LLButton* mUploadPhoto;
    LLButton* mChangePhoto;
    LLButton* mRemovePhoto;
    LLButton* mSaveChanges;
    LLButton* mDiscardChanges;

    LLHandle<LLFloater>	mFloaterTexturePickerHandle;

    std::string		mCurrentDescription;
    LLUUID			mImageId;
    bool			mHasUnsavedChanges;
};

/**
 * Panel for displaying Avatar's notes and modifying friend's rights.
 */
class LLPanelProfileNotes
	: public LLPanelProfileTab
{
public:
	LLPanelProfileNotes();
	/*virtual*/ ~LLPanelProfileNotes();

	void setAvatarId(const LLUUID& avatar_id) override;

	void onOpen(const LLSD& key) override;

	bool postBuild() override;

    void processProperties(LLAvatarNotes* avatar_notes);

	void resetData() override;

	void updateData() override;

    bool hasUnsavedChanges() override { return mHasUnsavedChanges; }
    void commitUnsavedChanges() override;

protected:
    void setNotesText(const std::string &text);
    void onSetNotesDirty();
    void onSaveNotesChanges();
    void onDiscardNotesChanges();

	LLTextEditor*       mNotesEditor;
    LLButton* mSaveChanges;
    LLButton* mDiscardChanges;

    std::string		mCurrentNotes;
    bool			mHasUnsavedChanges;
};


/**
* Container panel for the profile tabs
*/
class LLPanelProfile
    : public LLPanelProfileTab
{
public:
    LLPanelProfile();
    /*virtual*/ ~LLPanelProfile();

    bool postBuild() override;

    void updateData() override;
    void refreshName();

    void onOpen(const LLSD& key) override;

    void createPick(const LLPickData &data);
    void showPick(const LLUUID& pick_id = LLUUID::null);
    bool isPickTabSelected();
    bool isNotesTabSelected();
    bool hasUnsavedChanges() override;
    bool hasUnpublishedClassifieds();
    void commitUnsavedChanges() override;

    void showClassified(const LLUUID& classified_id = LLUUID::null, bool edit = false);
    void createClassified();

    LLAvatarData getAvatarData() { return mAvatarData; };

    friend void request_avatar_properties_coro(std::string cap_url, LLUUID agent_id);

private:
    void onTabChange();

    LLPanelProfileSecondLife*   mPanelSecondlife;
    LLPanelProfileWeb*          mPanelWeb;
    LLPanelProfilePicks*        mPanelPicks;
    LLPanelProfileClassifieds*  mPanelClassifieds;
    LLPanelProfileFirstLife*    mPanelFirstlife;
    LLPanelProfileNotes*        mPanelNotes;
    LLTabContainer*             mTabContainer;

    // Todo: due to server taking minutes to update this needs a more long term storage
    // to reuse recently saved values if user opens floater again
    // Storage implementation depends onto how a cap will be implemented, if cap will be
    // enought to fully update LLAvatarPropertiesProcessor, then this storage can be
    // implemented there.
    LLAvatarData mAvatarData;
};

#endif //LL_LLPANELPROFILE_H
