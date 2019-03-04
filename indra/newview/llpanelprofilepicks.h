/**
 * @file llpanelprofilepicks.h
 * @brief LLPanelProfilePicks and related class definitions
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELPICKS_H
#define LL_LLPANELPICKS_H

#include "llpanel.h"
#include "lluuid.h"
#include "llavatarpropertiesprocessor.h"
#include "llpanelavatar.h"
#include "llremoteparcelrequest.h"

class LLTabContainer;
class LLTextureCtrl;
class LLMediaCtrl;
class LLLineEditor;
class LLTextEditor;


/**
* Panel for displaying Avatar's picks.
*/
class LLPanelProfilePicks
    : public LLPanelProfileTab
{
public:
    LLPanelProfilePicks();
    /*virtual*/ ~LLPanelProfilePicks();

    /*virtual*/ BOOL postBuild();

    /*virtual*/ void onOpen(const LLSD& key);

    void selectPick(const LLUUID& pick_id);

    /*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

    /*virtual*/ void resetData();

    /*virtual*/ void updateButtons();

    /**
     * Saves changes.
     */
    virtual void apply();

    /**
     * Sends update data request to server.
     */
    /*virtual*/ void updateData();

private:
    void onClickNewBtn();
    void onClickDelete();
    void callbackDeletePick(const LLSD& notification, const LLSD& response);

    bool canAddNewPick();
    bool canDeletePick();

    LLTabContainer* mTabContainer;
    LLUICtrl*       mNoItemsLabel;
    LLButton*       mNewButton;
    LLButton*       mDeleteButton;

    LLUUID          mPickToSelectOnLoad;
};


class LLPanelProfilePick
    : public LLPanelProfileTab
    , public LLRemoteParcelInfoObserver
{
public:

    // Creates new panel
    static LLPanelProfilePick* create();

    LLPanelProfilePick();

    /*virtual*/ ~LLPanelProfilePick();

    /*virtual*/ BOOL postBuild();

    void setAvatarId(const LLUUID& avatar_id);

    void setPickId(const LLUUID& id) { mPickId = id; }
    virtual LLUUID& getPickId() { return mPickId; }

    virtual void setPickName(const std::string& name);
    const std::string getPickName();

    /*virtual*/ void processProperties(void* data, EAvatarProcessorType type);

    /**
     * Saves changes.
     */
    virtual void apply();

    void updateTabLabel(const std::string& title);

    //This stuff we got from LLRemoteParcelObserver, in the last one we intentionally do nothing
    /*virtual*/ void processParcelInfo(const LLParcelData& parcel_data);
    /*virtual*/ void setParcelID(const LLUUID& parcel_id) { mParcelId = parcel_id; }
    /*virtual*/ void setErrorStatus(S32 status, const std::string& reason) {};

protected:

    /**
     * Sends remote parcel info request to resolve parcel name from its ID.
     */
    void sendParcelInfoRequest();

    /**
    * "Location text" is actually the owner name, the original
    * name that owner gave the parcel, and the location.
    */
    static std::string createLocationText(
        const std::string& owner_name,
        const std::string& original_name,
        const std::string& sim_name,
        const LLVector3d& pos_global);

    /**
     * Sets snapshot id.
     *
     * Will mark snapshot control as valid if id is not null.
     * Will mark snapshot control as invalid if id is null. If null id is a valid value,
     * you have to manually mark snapshot is valid.
     */
    virtual void setSnapshotId(const LLUUID& id);
    virtual void setPickDesc(const std::string& desc);
    virtual void setPickLocation(const std::string& location);

    virtual void setPosGlobal(const LLVector3d& pos) { mPosGlobal = pos; }
    virtual LLVector3d& getPosGlobal() { return mPosGlobal; }

    /**
     * Callback for "Map" button, opens Map
     */
    void onClickMap();

    /**
     * Callback for "Teleport" button, teleports user to Pick location.
     */
    void onClickTeleport();

    /**
     * Enables/disables "Save" button
     */
    void enableSaveButton(BOOL enable);

    /**
     * Called when snapshot image changes.
     */
    void onSnapshotChanged();

    /**
     * Callback for Pick snapshot, name and description changed event.
     */
    void onPickChanged(LLUICtrl* ctrl);

    /**
     * Resets panel and all cantrols to unedited state
     */
    /*virtual*/ void resetDirty();

    /**
     * Returns true if any of Pick properties was changed by user.
     */
    /*virtual*/ BOOL isDirty() const;

    /**
     * Callback for "Set Location" button click
     */
    void onClickSetLocation();

    /**
     * Callback for "Save" button click
     */
    void onClickSave();

    std::string getLocationNotice();

    /**
     * Sends Pick properties to server.
     */
    void sendUpdate();

protected:

    LLTextureCtrl*      mSnapshotCtrl;
    LLLineEditor*       mPickName;
    LLTextEditor*       mPickDescription;
    LLButton*           mSetCurrentLocationButton;
    LLButton*           mSaveButton;

    LLVector3d mPosGlobal;
    LLUUID mParcelId;
    LLUUID mPickId;
    LLUUID mRequestedId;

    bool mLocationChanged;
    bool mNewPick;
    bool                mIsEditing;

    std::string mCurrentPickDescription;

    void onDescriptionFocusReceived();
};

#endif // LL_LLPANELPICKS_H
