/**
 * @file llfloaterworldmap.h
 * @brief LLFloaterWorldMap class definition
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

/*
 * Map of the entire world, with multiple background images,
 * avatar tracking, teleportation by double-click, etc.
 */

#ifndef LL_LLFLOATERWORLDMAP_H
#define LL_LLFLOATERWORLDMAP_H

#include "llfloater.h"
#include "llmapimagetype.h"
#include "llremoteparcelrequest.h"
#include "llslurl.h"
#include "lltracker.h"

class LLCtrlListInterface;
class LLFriendObserver;
class LLInventoryModel;
class LLInventoryObserver;
class LLItemInfo;
class LLLineEditor;
class LLTabContainer;
class LLWorldMapView;
class LLButton;
class LLCheckBoxCtrl;
class LLSliderCtrl;
class LLSpinCtrl;
class LLSearchEditor;

class LLWorldMapParcelInfoObserver : public LLRemoteParcelInfoObserver
{
public:
    LLWorldMapParcelInfoObserver(const LLVector3d& pos_global);
    ~LLWorldMapParcelInfoObserver();

    void processParcelInfo(const LLParcelData& parcel_data);
    void setParcelID(const LLUUID& parcel_id);
    void setErrorStatus(S32 status, const std::string& reason);

protected:
    LLVector3d  mPosGlobal;
    LLUUID      mParcelID;
};

class LLFloaterWorldMap : public LLFloater
{
public:
    LLFloaterWorldMap(const LLSD& key);
    virtual ~LLFloaterWorldMap();

    // Prefer this to gFloaterWorldMap
    static LLFloaterWorldMap* getInstance();

    static void *createWorldMapView(void* data);
    bool postBuild();

    /*virtual*/ void onOpen(const LLSD& key);
    /*virtual*/ void onClose(bool app_quitting);

    static void reloadIcons(void*);

    /*virtual*/ void reshape( S32 width, S32 height, bool called_from_parent = true );
    /*virtual*/ bool handleHover(S32 x, S32 y, MASK mask);
    /*virtual*/ bool handleScrollWheel(S32 x, S32 y, S32 clicks);
    /*virtual*/ void draw();

    /*virtual*/ void onFocusLost();

    // methods for dealing with inventory. The observe() method is
    // called during program startup. inventoryUpdated() will be
    // called by a helper object when an interesting change has
    // occurred.
    void observeInventory(LLInventoryModel* inventory);
    void inventoryChanged();

    // Calls for dealing with changes in friendship
    void observeFriends();
    void friendsChanged();

    // tracking methods
    void            trackAvatar( const LLUUID& avatar_id, const std::string& name );
    void            trackLandmark( const LLUUID& landmark_item_id );
    void            trackLocation(const LLVector3d& pos);
    void            trackEvent(const LLItemInfo &event_info);
    void            trackGenericItem(const LLItemInfo &item);
    void            trackURL(const std::string& region_name, S32 x_coord, S32 y_coord, S32 z_coord);

    static const LLUUID& getHomeID() { return sHomeID; }

    // A z_attenuation of 0.0f collapses the distance into the X-Y plane
    F32             getDistanceToDestination(const LLVector3d& pos_global, F32 z_attenuation = 0.5f) const;

    void            clearLocationSelection(bool clear_ui = false, bool dest_reached = false);
    void            clearAvatarSelection(bool clear_ui = false);
    void            clearLandmarkSelection(bool clear_ui = false);

    // Adjust the maximally zoomed out limit of the zoom slider so you can
    // see the whole world, plus a little.
    void            adjustZoomSliderBounds();

    // Catch changes in the sim list
    void            updateSims(bool found_null_sim);

    // teleport to the tracked item, if there is one
    void            teleport();
    void            onChangeMaturity();

    void            onClearBtn();

    //Slapp instigated avatar tracking
    void            avatarTrackFromSlapp( const LLUUID& id );

    void            processParcelInfo(const LLParcelData& parcel_data, const LLVector3d& pos_global) const;

protected:
    void            onGoHome();

    void            onLandmarkComboPrearrange();
    void            onLandmarkComboCommit();

    void            onAvatarComboPrearrange();
    void            onAvatarComboCommit();

    void            onComboTextEntry( );
    void            onSearchTextEntry( );

    void            onClickTeleportBtn();
    void            onShowTargetBtn();
    void            onShowAgentBtn();
    void            onCopySLURL();

    void            onExpandCollapseBtn();

    void            centerOnTarget(bool animate);
    void            updateLocation();

    // fly to the tracked item, if there is one
    void            fly();

    void            buildLandmarkIDLists();
    void            flyToLandmark();
    void            teleportToLandmark();

    void            buildAvatarIDList();
    void            flyToAvatar();
    void            teleportToAvatar();

    void            updateSearchEnabled();
    void            onLocationFocusChanged( LLFocusableElement* ctrl );
    void            onLocationCommit();
    void            onCoordinatesCommit();
    void            onCommitSearchResult();

    void            onTeleportFinished();

private:
    LLWorldMapView* mMapView; // Panel displaying the map

    // update display of teleport destination coordinates - pos is in global coordinates
    void updateTeleportCoordsDisplay( const LLVector3d& pos );

    // enable/disable teleport destination coordinates
    void enableTeleportCoordsDisplay( bool enabled );

    void            requestParcelInfo(const LLVector3d& pos_global, const LLVector3d& region_origin);
    LLVector3d      mRequestedGlobalPos;
    bool            mShowParcelInfo;
    LLWorldMapParcelInfoObserver* mParcelInfoObserver;

    uuid_vec_t      mLandmarkAssetIDList;
    uuid_vec_t      mLandmarkItemIDList;

    static const LLUUID sHomeID;

    LLInventoryModel* mInventory;
    LLInventoryObserver* mInventoryObserver;
    LLFriendObserver* mFriendObserver;

    std::string             mCompletingRegionName;
    // Local position from trackURL() request, used to select final
    // position once region lookup complete.
    LLVector3               mCompletingRegionPos;

    std::string             mLastRegionName;
    bool                    mWaitingForTracker;

    bool                    mIsClosing;
    bool                    mSetToUserPosition;

    LLVector3d              mTrackedLocation;
    LLTracker::ETrackingStatus mTrackedStatus;
    std::string             mTrackedSimName;
    LLUUID                  mTrackedAvatarID;
    LLSLURL                 mSLURL;

    LLCtrlListInterface *   mListFriendCombo;
    LLCtrlListInterface *   mListLandmarkCombo;
    LLCtrlListInterface *   mListSearchResults;

    LLButton*               mTeleportButton = nullptr;
    LLButton*               mShowDestinationButton = nullptr;
    LLButton*               mCopySlurlButton = nullptr;
    LLButton*               mGoHomeButton = nullptr;

    LLCheckBoxCtrl*         mPeopleCheck = nullptr;
    LLCheckBoxCtrl*         mInfohubCheck = nullptr;
    LLCheckBoxCtrl*         mLandSaleCheck = nullptr;
    LLCheckBoxCtrl*         mEventsCheck = nullptr;
    LLCheckBoxCtrl*         mEventsMatureCheck = nullptr;
    LLCheckBoxCtrl*         mEventsAdultCheck = nullptr;

    LLUICtrl*               mAvatarIcon = nullptr;
    LLUICtrl*               mLandmarkIcon = nullptr;
    LLUICtrl*               mLocationIcon = nullptr;

    LLSearchEditor*         mLocationEditor = nullptr;
    LLUICtrl*               mTeleportCoordSpinX = nullptr;
    LLUICtrl*               mTeleportCoordSpinY = nullptr;
    LLUICtrl*               mTeleportCoordSpinZ = nullptr;

    LLSliderCtrl*           mZoomSlider = nullptr;

    boost::signals2::connection mTeleportFinishConnection;
};

extern LLFloaterWorldMap* gFloaterWorldMap;


class LLPanelHideBeacon : public LLPanel
{
public:
    static LLPanelHideBeacon* getInstance();

    LLPanelHideBeacon();
    /*virtual*/ bool postBuild();
    /*virtual*/ void setVisible(bool visible);
    /*virtual*/ void draw();

private:
    static LLPanelHideBeacon* getPanelHideBeacon();
    void onHideButtonClick();
    void updatePosition();

    LLButton* mHideButton;

};

#endif

