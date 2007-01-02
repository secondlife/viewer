/** 
 * @file lltracker.h
 * @brief Container for objects user is tracking.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// A singleton class for tracking stuff.
//
// TODO -- LLAvatarTracker functionality should probably be moved
// to the LLTracker class. 


#ifndef LL_LLTRACKER_H
#define LL_LLTRACKER_H

#include "lldarray.h"
#include "llmemory.h"
#include "llstring.h"
#include "lluuid.h"
#include "v3dmath.h"

class LLHUDText;


class LLTracker
{
public:
	enum ETrackingStatus
	{
		TRACKING_NOTHING = 0,
		TRACKING_AVATAR = 1,
		TRACKING_LANDMARK = 2,
		TRACKING_LOCATION = 3,
	};

	enum ETrackingLocationType
	{
		LOCATION_NOTHING,
		LOCATION_EVENT,
		LOCATION_ITEM,
	};

	static LLTracker* instance() 
	{ 
		if (!sTrackerp)
		{
			sTrackerp = new LLTracker();
		}
		return sTrackerp; 
	}

	static void cleanupInstance() { delete sTrackerp; sTrackerp = NULL; }

	//static void drawTrackerArrow(); 
	// these are static so that they can be used a callbacks
	static ETrackingStatus getTrackingStatus() { return instance()->mTrackingStatus; }
	static ETrackingLocationType getTrackedLocationType() { return instance()->mTrackingLocationType; }
	static BOOL isTracking(void*) { return (BOOL) instance()->mTrackingStatus; }
	static void stopTracking(void*);

	static const LLUUID& getTrackedLandmarkAssetID() { return instance()->mTrackedLandmarkAssetID; }
	static const LLUUID& getTrackedLandmarkItemID()	 { return instance()->mTrackedLandmarkItemID; }

	static void	trackAvatar( const LLUUID& avatar_id, const LLString& name );
	static void	trackLandmark( const LLUUID& landmark_asset_id, const LLUUID& landmark_item_id , const LLString& name);
	static void	trackLocation(const LLVector3d& pos, const LLString& full_name, const LLString& tooltip, ETrackingLocationType location_type = LOCATION_NOTHING);

	// returns global pos of tracked thing
	static LLVector3d 	getTrackedPositionGlobal();

	static BOOL 		hasLandmarkPosition();
	static const LLString& getTrackedLocationName();

	static void drawHUDArrow();

	// Draw in-world 3D tracking stuff
	static void	render3D();

	static BOOL handleMouseDown(S32 x, S32 y);

	static LLTracker* sTrackerp;
	static BOOL sCheesyBeacon;
	
	static const LLString& getLabel() { return instance()->mLabel; }
	static const LLString& getToolTip() { return instance()->mToolTip; }
protected:
	LLTracker();
	~LLTracker();

	static void renderBeacon( LLVector3d pos_global, 
							 const LLColor4& color, 
							 LLHUDText* hud_textp, 
							 const std::string& label );

	void stopTrackingAll(BOOL clear_ui = FALSE);
	void stopTrackingAvatar(BOOL clear_ui = FALSE);
	void stopTrackingLocation(BOOL clear_ui = FALSE);
	void stopTrackingLandmark(BOOL clear_ui = FALSE);

	void drawMarker(const LLVector3d& pos_global, const LLColor4& color);
	void setLandmarkVisited();
	void cacheLandmarkPosition();
	void purgeBeaconText();

protected:
	ETrackingStatus 		mTrackingStatus;
	ETrackingLocationType	mTrackingLocationType;
	LLPointer<LLHUDText>	mBeaconText;
	S32 mHUDArrowCenterX;
	S32 mHUDArrowCenterY;

	LLVector3d				mTrackedPositionGlobal;

	LLString				mLabel;
	LLString				mToolTip;

	LLString				mTrackedLandmarkName;
	LLUUID					mTrackedLandmarkAssetID;
	LLUUID					mTrackedLandmarkItemID;
	LLDynamicArray<LLUUID>	mLandmarkAssetIDList;
	LLDynamicArray<LLUUID>	mLandmarkItemIDList;
	BOOL					mHasReachedLandmark;
	BOOL 					mHasLandmarkPosition;
	BOOL					mLandmarkHasBeenVisited;

	LLString				mTrackedLocationName;
	BOOL					mIsTrackingLocation;
	BOOL					mHasReachedLocation;
};


#endif

