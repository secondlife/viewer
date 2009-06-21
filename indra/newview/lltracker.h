/** 
 * @file lltracker.h
 * @brief Container for objects user is tracking.
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

// A singleton class for tracking stuff.
//
// TODO -- LLAvatarTracker functionality should probably be moved
// to the LLTracker class. 


#ifndef LL_LLTRACKER_H
#define LL_LLTRACKER_H

#include "lldarray.h"
#include "llpointer.h"
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
	static void clearFocus();
	
	static const LLUUID& getTrackedLandmarkAssetID() { return instance()->mTrackedLandmarkAssetID; }
	static const LLUUID& getTrackedLandmarkItemID()	 { return instance()->mTrackedLandmarkItemID; }

	static void	trackAvatar( const LLUUID& avatar_id, const std::string& name );
	static void	trackLandmark( const LLUUID& landmark_asset_id, const LLUUID& landmark_item_id , const std::string& name);
	static void	trackLocation(const LLVector3d& pos, const std::string& full_name, const std::string& tooltip, ETrackingLocationType location_type = LOCATION_NOTHING);

	// returns global pos of tracked thing
	static LLVector3d 	getTrackedPositionGlobal();

	static BOOL 		hasLandmarkPosition();
	static const std::string& getTrackedLocationName();

	static void drawHUDArrow();

	// Draw in-world 3D tracking stuff
	static void	render3D();

	static BOOL handleMouseDown(S32 x, S32 y);

	static LLTracker* sTrackerp;
	static BOOL sCheesyBeacon;
	
	static const std::string& getLabel() { return instance()->mLabel; }
	static const std::string& getToolTip() { return instance()->mToolTip; }
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

	std::string				mLabel;
	std::string				mToolTip;

	std::string				mTrackedLandmarkName;
	LLUUID					mTrackedLandmarkAssetID;
	LLUUID					mTrackedLandmarkItemID;
	LLDynamicArray<LLUUID>	mLandmarkAssetIDList;
	LLDynamicArray<LLUUID>	mLandmarkItemIDList;
	BOOL					mHasReachedLandmark;
	BOOL 					mHasLandmarkPosition;
	BOOL					mLandmarkHasBeenVisited;

	std::string				mTrackedLocationName;
	BOOL					mIsTrackingLocation;
	BOOL					mHasReachedLocation;
};


#endif

