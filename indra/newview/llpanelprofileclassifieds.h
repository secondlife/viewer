/**
 * @file llpanelprofileclassifieds.h
 * @brief LLPanelProfileClassifieds and related class implementations
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

#ifndef LL_PANELPROFILECLASSIFIEDS_H
#define LL_PANELPROFILECLASSIFIEDS_H

#include "llavatarpropertiesprocessor.h"
#include "llclassifiedinfo.h"
#include "llfloater.h"
#include "llpanel.h"
#include "llpanelavatar.h"
#include "llrect.h"
#include "lluuid.h"
#include "v3dmath.h"
#include "llcoros.h"
#include "lleventcoro.h"

class LLCheckBoxCtrl;
class LLLineEditor;
class LLMediaCtrl;
class LLScrollContainer;
class LLTabContainer;
class LLTextEditor;
class LLTextureCtrl;
class LLUICtrl;


class LLPublishClassifiedFloater : public LLFloater
{
public:
    LLPublishClassifiedFloater(const LLSD& key);
    virtual ~LLPublishClassifiedFloater();

    bool postBuild() override;

    void setPrice(S32 price);
    S32 getPrice();

    void setPublishClickedCallback(const commit_signal_t::slot_type& cb);
    void setCancelClickedCallback(const commit_signal_t::slot_type& cb);
};


/**
* Panel for displaying Avatar's picks.
*/
class LLPanelProfileClassifieds
    : public LLPanelProfilePropertiesProcessorTab
{
public:
    LLPanelProfileClassifieds();
    /*virtual*/ ~LLPanelProfileClassifieds();

    bool postBuild() override;

    void onOpen(const LLSD& key) override;

    void selectClassified(const LLUUID& classified_id, bool edit);

    void createClassified();

    void processProperties(void* data, EAvatarProcessorType type) override;

    void resetData() override;

    void updateButtons();

    void updateData() override;

    bool hasNewClassifieds();
    bool hasUnsavedChanges() override;
    // commits changes to existing classifieds, but does not publish new classified!
    void commitUnsavedChanges() override;

private:
    void onClickNewBtn();
    void onClickDelete();
    void callbackDeleteClassified(const LLSD& notification, const LLSD& response);

    bool canAddNewClassified();
    bool canDeleteClassified();

    LLTabContainer* mTabContainer;
    LLUICtrl*       mNoItemsLabel;
    LLButton*       mNewButton;
    LLButton*       mDeleteButton;

    LLUUID          mClassifiedToSelectOnLoad;
    bool            mClassifiedEditOnLoad;
    bool            mSheduledClassifiedCreation;
};


class LLPanelProfileClassified
    : public LLPanelProfilePropertiesProcessorTab
{
public:

    static LLPanelProfileClassified* create();

    LLPanelProfileClassified();

    /*virtual*/ ~LLPanelProfileClassified();

    bool postBuild() override;

    void onOpen(const LLSD& key) override;

    void processProperties(void* data, EAvatarProcessorType type) override;

    void setSnapshotId(const LLUUID& id);

    LLUUID getSnapshotId();

    void setClassifiedId(const LLUUID& id) { mClassifiedId = id; }

    LLUUID& getClassifiedId() { return mClassifiedId; }

    void setClassifiedName(const std::string& name);

    std::string getClassifiedName();

    void setDescription(const std::string& desc);

    std::string getDescription();

    void setClassifiedLocation(const std::string& location);

    std::string getClassifiedLocation();

    void setPosGlobal(const LLVector3d& pos) { mPosGlobal = pos; }

    LLVector3d& getPosGlobal() { return mPosGlobal; }

    void setParcelId(const LLUUID& id) { mParcelId = id; }

    LLUUID getParcelId() const { return mParcelId; }

    void setSimName(const std::string& sim_name) { mSimName = sim_name; }

    std::string getSimName() const { return mSimName; }

    void setFromSearch(bool val) { mFromSearch = val; }

    bool fromSearch() const { return mFromSearch; }

    bool getInfoLoaded() const { return mInfoLoaded; }

    void setInfoLoaded(bool loaded) { mInfoLoaded = loaded; }

    bool isDirty() const override;

    void resetDirty() override;

    bool isNew() const { return mIsNew; }

    bool isNewWithErrors() const { return mIsNewWithErrors; }

    bool canClose();

    U32 getCategory();

    void setCategory(U32 category);

    U32 getContentType();

    void setContentType(bool mature);

    bool getAutoRenew();

    S32 getPriceForListing() const { return mPriceForListing; }

    void setEditMode(bool edit_mode);
    bool getEditMode() const { return mEditMode; }

    static void setClickThrough(
        const LLUUID& classified_id,
        S32 teleport,
        S32 map,
        S32 profile,
        bool from_new_table);

    static void sendClickMessage(
            const std::string& type,
            bool from_search,
            const LLUUID& classified_id,
            const LLUUID& parcel_id,
            const LLVector3d& global_pos,
            const std::string& sim_name);

    void doSave();

protected:

    void resetData() override;

    void resetControls();

    void updateButtons();
    void updateInfoRect();

    static std::string createLocationText(
        const std::string& original_name,
        const std::string& sim_name,
        const LLVector3d& pos_global);

    void sendClickMessage(const std::string& type);

    void scrollToTop();

    void onEditClick();
    void onCancelClick();
    void onSaveClick();
    void onMapClick();
    void onTeleportClick();

    void sendUpdate();

    void enableSave(bool enable);

    void enableEditing(bool enable);

    std::string makeClassifiedName();

    void setPriceForListing(S32 price) { mPriceForListing = price; }

    U8 getFlags();

    std::string getLocationNotice();

    bool isValidName();

    void notifyInvalidName();

    void onSetLocationClick();
    void onChange();
    void onTitleChange();

    void onPublishFloaterPublishClicked();

    void onTexturePickerMouseEnter();
    void onTexturePickerMouseLeave();

    void onTextureSelected();

    void updateTabLabel(const std::string& title);

private:

    LLTextureCtrl*      mSnapshotCtrl;
    LLUICtrl*           mEditIcon;
    LLUICtrl*           mClassifiedNameText;
    LLTextEditor*       mClassifiedDescText;
    LLLineEditor*       mClassifiedNameEdit;
    LLTextEditor*       mClassifiedDescEdit;
    LLUICtrl*           mLocationText;
    LLUICtrl*           mLocationEdit;
    LLUICtrl*           mCategoryText;
    LLComboBox*         mCategoryCombo;
    LLUICtrl*           mContentTypeText;
    LLIconCtrl*         mContentTypeM;
    LLIconCtrl*         mContentTypeG;
    LLComboBox*         mContentTypeCombo;
    LLUICtrl*           mPriceText;
    LLUICtrl*           mAutoRenewText;
    LLUICtrl*           mAutoRenewEdit;

    LLButton*           mMapButton;
    LLButton*           mTeleportButton;
    LLButton*           mEditButton;
    LLButton*           mSaveButton;
    LLButton*           mSetLocationButton;
    LLButton*           mCancelButton;

    LLPanel*            mUtilityBtnCnt;
    LLPanel*            mPublishBtnsCnt;
    LLPanel*            mSaveBtnCnt;
    LLPanel*            mCancelBtnCnt;

    LLScrollContainer*  mScrollContainer;
    LLView*             mInfoPanel;
    LLPanel*            mInfoScroll;
    LLPanel*            mEditPanel;


    LLUUID mClassifiedId;
    LLVector3d mPosGlobal;
    LLUUID mParcelId;
    std::string mSimName;
    bool mFromSearch;
    bool mInfoLoaded;
    bool mEditMode;

    // Needed for stat tracking
    S32 mTeleportClicksOld;
    S32 mMapClicksOld;
    S32 mProfileClicksOld;
    S32 mTeleportClicksNew;
    S32 mMapClicksNew;
    S32 mProfileClicksNew;

    S32 mPriceForListing;

    static void handleSearchStatResponse(LLUUID classifiedId, LLSD result);

    typedef std::list<LLPanelProfileClassified*> panel_list_t;
    static panel_list_t sAllPanels;


    bool mIsNew;
    bool mIsNewWithErrors;
    bool mCanClose;
    bool mEditOnLoad;

    LLPublishClassifiedFloater* mPublishFloater;
};

#endif // LL_PANELPROFILECLASSIFIEDS_H
