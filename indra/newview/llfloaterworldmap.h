/** 
 * @file llfloaterworldmap.h
 * @brief LLFloaterWorldMap class definition
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
class LLLineEditor;
class LLTabContainer;

class LLFloaterWorldMap : public LLFloater
{
public:
	LLFloaterWorldMap(const LLSD& key);
	virtual ~LLFloaterWorldMap();

	// Prefer this to gFloaterWorldMap
	static LLFloaterWorldMap* getInstance();

	static void *createWorldMapView(void* data);
	BOOL postBuild();

	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);

	static void reloadIcons(void*);

	/*virtual*/ void reshape( S32 width, S32 height, BOOL called_from_parent = TRUE );
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleScrollWheel(S32 x, S32 y, S32 clicks);
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
	void			trackAvatar( const LLUUID& avatar_id, const std::string& name );
	void			trackLandmark( const LLUUID& landmark_item_id ); 
	void			trackLocation(const LLVector3d& pos);
	void			trackEvent(const LLItemInfo &event_info);
	void			trackGenericItem(const LLItemInfo &item);
	void			trackURL(const std::string& region_name, S32 x_coord, S32 y_coord, S32 z_coord);

	static const LLUUID& getHomeID() { return sHomeID; }

	// A z_attenuation of 0.0f collapses the distance into the X-Y plane
	F32				getDistanceToDestination(const LLVector3d& pos_global, F32 z_attenuation = 0.5f) const;

	void			clearLocationSelection(BOOL clear_ui = FALSE);
	void			clearAvatarSelection(BOOL clear_ui = FALSE);
	void			clearLandmarkSelection(BOOL clear_ui = FALSE);

	// Adjust the maximally zoomed out limit of the zoom slider so you can
	// see the whole world, plus a little.
	void			adjustZoomSliderBounds();

	// Catch changes in the sim list
	void			updateSims(bool found_null_sim);

	// teleport to the tracked item, if there is one
	void			teleport();

protected:	
	void			onGoHome();

	void			onLandmarkComboPrearrange();
	void			onLandmarkComboCommit();

	void			onAvatarComboPrearrange();
	void		    onAvatarComboCommit();

	void			onComboTextEntry( );
	void			onSearchTextEntry( LLLineEditor* ctrl );

	void			onClearBtn();
	void			onClickTeleportBtn();
	void			onShowTargetBtn();
	void			onShowAgentBtn();
	void			onCopySLURL();

	void			centerOnTarget(BOOL animate);
	void			updateLocation();

	// fly to the tracked item, if there is one
	void			fly();

	void			buildLandmarkIDLists();
	void			flyToLandmark();
	void			teleportToLandmark();
	void			setLandmarkVisited();

	void			buildAvatarIDList();
	void			flyToAvatar();
	void			teleportToAvatar();

	void			updateSearchEnabled();
	void			onLocationFocusChanged( LLFocusableElement* ctrl );
	void		    onLocationCommit();
	void		    onCommitSearchResult();

	void			cacheLandmarkPosition();

private:
	LLPanel*			mPanel;		// Panel displaying the map

	// Ties to LLWorldMapView::sMapScale, in pixels per region
	F32						mCurZoomVal;
	LLFrameTimer			mZoomTimer;

	LLDynamicArray<LLUUID>	mLandmarkAssetIDList;
	LLDynamicArray<LLUUID>	mLandmarkItemIDList;

	static const LLUUID	sHomeID;

	LLInventoryModel* mInventory;
	LLInventoryObserver* mInventoryObserver;
	LLFriendObserver* mFriendObserver;

	std::string				mCompletingRegionName;
	// Local position from trackURL() request, used to select final
	// position once region lookup complete.
	LLVector3				mCompletingRegionPos;

	std::string				mLastRegionName;
	BOOL					mWaitingForTracker;

	BOOL					mIsClosing;
	BOOL					mSetToUserPosition;

	LLVector3d				mTrackedLocation;
	LLTracker::ETrackingStatus mTrackedStatus;
	std::string				mTrackedSimName;
	std::string				mTrackedAvatarName;
	std::string				mSLURL;
};

extern LLFloaterWorldMap* gFloaterWorldMap;

#endif

