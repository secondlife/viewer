/** 
 * @file llfloaterworldmap.h
 * @brief LLFloaterWorldMap class definition
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

/*
 * Map of the entire world, with multiple background images,
 * avatar tracking, teleportation by double-click, etc.
 */

#ifndef LL_LLFLOATERWORLDMAP_H
#define LL_LLFLOATERWORLDMAP_H

#include "lldarray.h"
#include "llfloater.h"
#include "llhudtext.h"
#include "llmapimagetype.h"
#include "lltracker.h"

class LLEventInfo;
class LLFriendObserver;
class LLInventoryModel;
class LLInventoryObserver;
class LLItemInfo;
class LLTabContainer;
class LLWorldMapView;

class LLFloaterWorldMap : public LLFloater
{
public:
	LLFloaterWorldMap();
	virtual ~LLFloaterWorldMap();

	static void *createWorldMapView(void* data);
	BOOL postBuild();

	/*virtual*/ void onClose(bool app_quitting);

	static void show(void*, BOOL center_on_target );
	static void reloadIcons(void*);
	static void toggle(void*);
	static void hide(void*); 

	/*virtual*/ void reshape( S32 width, S32 height, BOOL called_from_parent = TRUE );
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ void setVisible(BOOL visible);
	/*virtual*/ void draw();

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
	void			trackAvatar( const LLUUID& avatar_id, const LLString& name );
	void			trackLandmark( const LLUUID& landmark_item_id ); 
	void			trackLocation(const LLVector3d& pos);
	void			trackEvent(const LLItemInfo &event_info);
	void			trackGenericItem(const LLItemInfo &item);
	void 			trackURL(const LLString& region_name, S32 x_coord, S32 y_coord, S32 z_coord);

	static const LLUUID& getHomeID() { return sHomeID; }

	// A z_attenuation of 0.0f collapses the distance into the X-Y plane
	F32 			getDistanceToDestination(const LLVector3d& pos_global, F32 z_attenuation = 0.5f) const;

	void 			clearLocationSelection(BOOL clear_ui = FALSE);
	void 			clearAvatarSelection(BOOL clear_ui = FALSE);
	void 			clearLandmarkSelection(BOOL clear_ui = FALSE);

	// Adjust the maximally zoomed out limit of the zoom slider so you can
	// see the whole world, plus a little.
	void			adjustZoomSliderBounds();

	// Catch changes in the sim list
	void			updateSims(bool found_null_sim);

protected:
	static void		onPanBtn( void* userdata );

	static void		onGoHome(void* data);

	static void		onLandmarkComboPrearrange( LLUICtrl* ctrl, void* data );
	static void		onLandmarkComboCommit( LLUICtrl* ctrl, void* data );
	
	static void		onAvatarComboPrearrange( LLUICtrl* ctrl, void* data );
	static void		onAvatarComboCommit( LLUICtrl* ctrl, void* data );

	static void		onCommitBackground(void* data, bool from_click);

	static void		onClearBtn(void*);
	static void		onFlyBtn(void*);
	static void		onTeleportBtn(void*);
	static void		onShowTargetBtn(void*);
	static void		onShowAgentBtn(void*);
	static void		onCopySLURL(void*);

	static void onCheckEvents(LLUICtrl* ctrl, void*);

	void			centerOnTarget(BOOL animate);
	void			updateLocation();

	// fly to the tracked item, if there is one
	void			fly();

	// teleport to the tracked item, if there is one
	void			teleport();

	void			buildLandmarkIDLists();
	static void		onGoToLandmarkDialog(S32 option,void* userdata);
	void			flyToLandmark();
	void			teleportToLandmark();
	void 			setLandmarkVisited();

	void			buildAvatarIDList();
	void			flyToAvatar();
	void			teleportToAvatar();

	static void		updateSearchEnabled( LLUICtrl* ctrl, void* userdata );
	static void		onLocationCommit( void* userdata );
	static void		onCommitLocation( LLUICtrl* ctrl, void* userdata );
	static void		onCommitSearchResult( LLUICtrl* ctrl, void* userdata );

	void 			cacheLandmarkPosition();

protected:
	LLTabContainerCommon*	mTabs;

	// Sets gMapScale, in pixels per region
	F32						mCurZoomVal;
	LLFrameTimer			mZoomTimer;

	LLDynamicArray<LLUUID>	mLandmarkAssetIDList;
	LLDynamicArray<LLUUID>	mLandmarkItemIDList;
	BOOL 					mHasLandmarkPosition;

	static const LLUUID	sHomeID;

	LLInventoryModel* mInventory;
	LLInventoryObserver* mInventoryObserver;
	LLFriendObserver* mFriendObserver;

	LLString				mCompletingRegionName;
	LLString				mLastRegionName;
	BOOL					mWaitingForTracker;
	BOOL					mExactMatch;

	BOOL					mIsClosing;
	BOOL					mSetToUserPosition;

	LLVector3d				mTrackedLocation;
	LLTracker::ETrackingStatus mTrackedStatus;
	LLString				mTrackedSimName;
	LLString				mTrackedAvatarName;
	LLString				mSLURL;
};

extern LLFloaterWorldMap* gFloaterWorldMap;

#endif

