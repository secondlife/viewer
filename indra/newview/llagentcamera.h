/** 
 * @file llagent.h
 * @brief LLAgent class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLAGENTCAMERA_H
#define LL_LLAGENTCAMERA_H

#include "llfollowcam.h" 			// Ventrella
#include "llhudeffectlookat.h" 		// EPointAtType
#include "llhudeffectpointat.h" 	// ELookAtType

class LLPickInfo;
class LLVOAvatarSelf;
class LLControlVariable;

//--------------------------------------------------------------------
// Types
//--------------------------------------------------------------------
enum ECameraMode
{
	CAMERA_MODE_THIRD_PERSON,
	CAMERA_MODE_MOUSELOOK,
	CAMERA_MODE_CUSTOMIZE_AVATAR,
	CAMERA_MODE_FOLLOW
};

/** Camera Presets for CAMERA_MODE_THIRD_PERSON */
enum ECameraPreset 
{
	/** Default preset, what the Third Person Mode actually was */
	CAMERA_PRESET_REAR_VIEW,
	
	/** "Looking at the Avatar from the front" */
	CAMERA_PRESET_FRONT_VIEW, 

	/** "Above and to the left, over the shoulder, pulled back a little on the zoom" */
	CAMERA_PRESET_GROUP_VIEW,

	/** Current view when a preset is saved */
	CAMERA_PRESET_CUSTOM
};

//------------------------------------------------------------------------
// LLAgentCamera
//------------------------------------------------------------------------
class LLAgentCamera
{
	LOG_CLASS(LLAgentCamera);

public:
	//--------------------------------------------------------------------
	// Constructors / Destructors
	//--------------------------------------------------------------------
public:
	LLAgentCamera();
	virtual 		~LLAgentCamera();
	void			init();
	void			cleanup();
	void		    setAvatarObject(LLVOAvatarSelf* avatar);
	bool			isInitialized() { return mInitialized; }
private:
	bool			mInitialized;


	//--------------------------------------------------------------------
	// Mode
	//--------------------------------------------------------------------
public:
	void			changeCameraToDefault();
	void			changeCameraToMouselook(BOOL animate = TRUE);
	void			changeCameraToThirdPerson(BOOL animate = TRUE);
	void			changeCameraToCustomizeAvatar(); // Trigger transition animation
	void			changeCameraToFollow(BOOL animate = TRUE); 	// Ventrella
	BOOL			cameraThirdPerson() const		{ return (mCameraMode == CAMERA_MODE_THIRD_PERSON && mLastCameraMode == CAMERA_MODE_THIRD_PERSON); }
	BOOL			cameraMouselook() const			{ return (mCameraMode == CAMERA_MODE_MOUSELOOK && mLastCameraMode == CAMERA_MODE_MOUSELOOK); }
	BOOL			cameraCustomizeAvatar() const	{ return (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR /*&& !mCameraAnimating*/); }
	BOOL			cameraFollow() const			{ return (mCameraMode == CAMERA_MODE_FOLLOW && mLastCameraMode == CAMERA_MODE_FOLLOW); }
	ECameraMode		getCameraMode() const 			{ return mCameraMode; }
	ECameraMode		getLastCameraMode() const 		{ return mLastCameraMode; }
	void			updateCamera();					// Call once per frame to update camera location/orientation
	void			resetCamera(); 					// Slam camera into its default position
	void			updateLastCamera();				// Set last camera to current camera

private:
	ECameraMode		mCameraMode;					// Target mode after transition animation is done
	ECameraMode		mLastCameraMode;

	//--------------------------------------------------------------------
	// Preset
	//--------------------------------------------------------------------
public:
	void switchCameraPreset(ECameraPreset preset);
	/** Determines default camera offset depending on the current camera preset */
	LLVector3 getCameraOffsetInitial();
	/** Determines default focus offset depending on the current camera preset */
	LLVector3d getFocusOffsetInitial();

	LLVector3 getCurrentCameraOffset();
	LLVector3d getCurrentFocusOffset();
	LLQuaternion getCurrentAvatarRotation();
	bool isJoystickCameraUsed();
	void setInitSitRot(LLQuaternion sit_rot) { mInitSitRot = sit_rot; };
	void rotateToInitSitRot();

private:
	/** Determines maximum camera distance from target for mouselook, opposite to LAND_MIN_ZOOM */
	F32 getCameraMaxZoomDistance();

	/** Camera preset in Third Person Mode */
	ECameraPreset mCameraPreset; 

	LLQuaternion mInitSitRot;

	//--------------------------------------------------------------------
	// Position
	//--------------------------------------------------------------------
public:
	LLVector3d		getCameraPositionGlobal() const;
	const LLVector3 &getCameraPositionAgent() const;
	LLVector3d		calcCameraPositionTargetGlobal(BOOL *hit_limit = NULL); // Calculate the camera position target
	F32				getCameraMinOffGround(); 		// Minimum height off ground for this mode, meters
	void			setCameraCollidePlane(const LLVector4 &plane) { mCameraCollidePlane = plane; }
	BOOL			calcCameraMinDistance(F32 &obj_min_distance);
	F32				getCurrentCameraBuildOffset() 	{ return (F32)mCameraFocusOffset.length(); }
	void			clearCameraLag() { mCameraLag.clearVec(); }
private:
	LLVector3		getAvatarRootPosition();

	F32				mCurrentCameraDistance;	 		// Current camera offset from avatar
	F32				mTargetCameraDistance;			// Target camera offset from avatar
	F32				mCameraFOVZoomFactor;			// Amount of fov zoom applied to camera when zeroing in on an object
	F32				mCameraCurrentFOVZoomFactor;	// Interpolated fov zoom
	LLVector4		mCameraCollidePlane;			// Colliding plane for camera
	F32				mCameraZoomFraction;			// Mousewheel driven fraction of zoom
	LLVector3		mCameraPositionAgent;			// Camera position in agent coordinates
	LLVector3		mCameraVirtualPositionAgent;	// Camera virtual position (target) before performing FOV zoom
	LLVector3d      mCameraSmoothingLastPositionGlobal;    
	LLVector3d      mCameraSmoothingLastPositionAgent;
	bool            mCameraSmoothingStop;
	LLVector3		mCameraLag;						// Third person camera lag
	LLVector3		mCameraUpVector;				// Camera's up direction in world coordinates (determines the 'roll' of the view)

	//--------------------------------------------------------------------
	// Follow
	//--------------------------------------------------------------------
public:
	bool 			isfollowCamLocked();
private:
	LLFollowCam 	mFollowCam; 			// Ventrella

	//--------------------------------------------------------------------
	// Sit
	//--------------------------------------------------------------------
public:
	void			setupSitCamera();
	BOOL			sitCameraEnabled() 		{ return mSitCameraEnabled; }
	void			setSitCamera(const LLUUID &object_id, 
								 const LLVector3 &camera_pos = LLVector3::zero, const LLVector3 &camera_focus = LLVector3::zero);
private:
	LLPointer<LLViewerObject> mSitCameraReferenceObject; // Object to which camera is related when sitting
	BOOL			mSitCameraEnabled;		// Use provided camera information when sitting?
	LLVector3		mSitCameraPos;			// Root relative camera pos when sitting
	LLVector3		mSitCameraFocus;		// Root relative camera target when sitting

	//--------------------------------------------------------------------
	// Animation
	//--------------------------------------------------------------------
public:
	void			setCameraAnimating(BOOL b)			{ mCameraAnimating = b; }
	BOOL			getCameraAnimating()				{ return mCameraAnimating; }
	void			setAnimationDuration(F32 seconds);
	void			startCameraAnimation();
	void			stopCameraAnimation();
private:
	LLFrameTimer	mAnimationTimer; 	// Seconds that transition animation has been active
	F32				mAnimationDuration;	// In seconds
	BOOL			mCameraAnimating;					// Camera is transitioning from one mode to another
	LLVector3d		mAnimationCameraStartGlobal;		// Camera start position, global coords
	LLVector3d		mAnimationFocusStartGlobal;			// Camera focus point, global coords

	//--------------------------------------------------------------------
	// Focus
	//--------------------------------------------------------------------
public:
	LLVector3d		calcFocusPositionTargetGlobal();
	LLVector3		calcFocusOffset(LLViewerObject *object, LLVector3 pos_agent, S32 x, S32 y);
	BOOL			getFocusOnAvatar() const		{ return mFocusOnAvatar; }
	LLPointer<LLViewerObject>&	getFocusObject() 	{ return mFocusObject; }
	F32				getFocusObjectDist() const		{ return mFocusObjectDist; }
	void			updateFocusOffset();
	void			validateFocusObject();
	void			setFocusGlobal(const LLPickInfo& pick);
	void			setFocusGlobal(const LLVector3d &focus, const LLUUID &object_id = LLUUID::null);
	void			setFocusOnAvatar(BOOL focus, BOOL animate, BOOL reset_axes = TRUE);
	void			setCameraPosAndFocusGlobal(const LLVector3d& pos, const LLVector3d& focus, const LLUUID &object_id);
	void			clearFocusObject();
	void			setFocusObject(LLViewerObject* object);
	void			setAllowChangeToFollow(BOOL focus) 	{ mAllowChangeToFollow = focus; }
	void			setObjectTracking(BOOL track) 	{ mTrackFocusObject = track; }
	const LLVector3d &getFocusGlobal() const		{ return mFocusGlobal; }
	const LLVector3d &getFocusTargetGlobal() const	{ return mFocusTargetGlobal; }
private:
	LLVector3d		mCameraFocusOffset;				// Offset from focus point in build mode
	LLVector3d		mCameraFocusOffsetTarget;		// Target towards which we are lerping the camera's focus offset
	BOOL			mFocusOnAvatar;
	BOOL			mAllowChangeToFollow;
	LLVector3d		mFocusGlobal;
	LLVector3d		mFocusTargetGlobal;
	LLPointer<LLViewerObject> mFocusObject;
	F32				mFocusObjectDist;
	LLVector3		mFocusObjectOffset;
	BOOL			mTrackFocusObject;
	
	//--------------------------------------------------------------------
	// Lookat / Pointat
	//--------------------------------------------------------------------
public:
	void			updateLookAt(const S32 mouse_x, const S32 mouse_y);
	BOOL			setLookAt(ELookAtType target_type, LLViewerObject *object = NULL, LLVector3 position = LLVector3::zero);
	ELookAtType		getLookAtType();
	void			lookAtLastChat();
	void 			slamLookAt(const LLVector3 &look_at); // Set the physics data
	BOOL			setPointAt(EPointAtType target_type, LLViewerObject *object = NULL, LLVector3 position = LLVector3::zero);
	EPointAtType	getPointAtType();
public:
	LLPointer<LLHUDEffectLookAt> mLookAt;
	LLPointer<LLHUDEffectPointAt> mPointAt;

	//--------------------------------------------------------------------
	// Third person
	//--------------------------------------------------------------------
public:
	LLVector3d		calcThirdPersonFocusOffset();
	void			setThirdPersonHeadOffset(LLVector3 offset) 	{ mThirdPersonHeadOffset = offset; }	
private:
	LLVector3		mThirdPersonHeadOffset;						// Head offset for third person camera position

	//--------------------------------------------------------------------
	// Orbit
	//--------------------------------------------------------------------
public:
	void			cameraOrbitAround(const F32 radians);	// Rotate camera CCW radians about build focus point
	void			cameraOrbitOver(const F32 radians);		// Rotate camera forward radians over build focus point
	void			cameraOrbitIn(const F32 meters);		// Move camera in toward build focus point
	void			resetCameraOrbit();
	void			resetOrbitDiff();
	//--------------------------------------------------------------------
	// Zoom
	//--------------------------------------------------------------------
public:
	void			handleScrollWheel(S32 clicks); 							// Mousewheel driven zoom
	void			cameraZoomIn(const F32 factor);							// Zoom in by fraction of current distance
	F32				getCameraZoomFraction(bool get_third_person = false);	// Get camera zoom as fraction of minimum and maximum zoom
	void			setCameraZoomFraction(F32 fraction);					// Set camera zoom as fraction of minimum and maximum zoom
	F32				calcCameraFOVZoomFactor();
	F32				getAgentHUDTargetZoom();

	void			resetCameraZoomFraction();
	F32				getCurrentCameraZoomFraction() { return mCameraZoomFraction; }

	//--------------------------------------------------------------------
	// Pan
	//--------------------------------------------------------------------
public:
	void			cameraPanIn(const F32 meters);
	void			cameraPanLeft(const F32 meters);
	void			cameraPanUp(const F32 meters);	
	void			resetCameraPan();
	void			resetPanDiff();
	//--------------------------------------------------------------------
	// View
	//--------------------------------------------------------------------
public:
	// Called whenever the agent moves.  Puts camera back in default position, deselects items, etc.
	void			resetView(BOOL reset_camera = TRUE, BOOL change_camera = FALSE);
	// Called on camera movement.  Unlocks camera from the default position behind the avatar.
	void			unlockView();
public:
	F32				mDrawDistance;

	//--------------------------------------------------------------------
	// Mouselook
	//--------------------------------------------------------------------
public:
	BOOL			getForceMouselook() const 			{ return mForceMouselook; }
	void			setForceMouselook(BOOL mouselook) 	{ mForceMouselook = mouselook; }
private:
	BOOL			mForceMouselook;
	
	//--------------------------------------------------------------------
	// HUD
	//--------------------------------------------------------------------
public:
	F32				mHUDTargetZoom;	// Target zoom level for HUD objects (used when editing)
	F32				mHUDCurZoom; 	// Current animated zoom level for HUD objects


/********************************************************************************
 **                                                                            **
 **                    KEYS
 **/

public:
	S32				getAtKey() const		{ return mAtKey; }
	S32				getWalkKey() const		{ return mWalkKey; }
	S32				getLeftKey() const		{ return mLeftKey; }
	S32				getUpKey() const		{ return mUpKey; }
	F32				getYawKey() const		{ return mYawKey; }
	F32				getPitchKey() const		{ return mPitchKey; }

	void			setAtKey(S32 mag)		{ mAtKey = mag; }
	void			setWalkKey(S32 mag)		{ mWalkKey = mag; }
	void			setLeftKey(S32 mag)		{ mLeftKey = mag; }
	void			setUpKey(S32 mag)		{ mUpKey = mag; }
	void			setYawKey(F32 mag)		{ mYawKey = mag; }
	void			setPitchKey(F32 mag)	{ mPitchKey = mag; }

	void			clearGeneralKeys();
	static S32		directionToKey(S32 direction); // Changes direction to -1/0/1

private:
	S32 			mAtKey;				// Either 1, 0, or -1. Indicates that movement key is pressed
	S32				mWalkKey; 			// Like AtKey, but causes less forward thrust
	S32 			mLeftKey;
	S32				mUpKey;
	F32				mYawKey;
	F32				mPitchKey;

	//--------------------------------------------------------------------
	// Orbit
	//--------------------------------------------------------------------
public:
	F32				getOrbitLeftKey() const		{ return mOrbitLeftKey; }
	F32				getOrbitRightKey() const	{ return mOrbitRightKey; }
	F32				getOrbitUpKey() const		{ return mOrbitUpKey; }
	F32				getOrbitDownKey() const		{ return mOrbitDownKey; }
	F32				getOrbitInKey() const		{ return mOrbitInKey; }
	F32				getOrbitOutKey() const		{ return mOrbitOutKey; }

	void			setOrbitLeftKey(F32 mag)	{ mOrbitLeftKey = mag; }
	void			setOrbitRightKey(F32 mag)	{ mOrbitRightKey = mag; }
	void			setOrbitUpKey(F32 mag)		{ mOrbitUpKey = mag; }
	void			setOrbitDownKey(F32 mag)	{ mOrbitDownKey = mag; }
	void			setOrbitInKey(F32 mag)		{ mOrbitInKey = mag; }
	void			setOrbitOutKey(F32 mag)		{ mOrbitOutKey = mag; }

	void			clearOrbitKeys();
private:
	F32				mOrbitLeftKey;
	F32				mOrbitRightKey;
	F32				mOrbitUpKey;
	F32				mOrbitDownKey;
	F32				mOrbitInKey;
	F32				mOrbitOutKey;

	F32				mOrbitAroundRadians;
	F32				mOrbitOverAngle;

	//--------------------------------------------------------------------
	// Pan
	//--------------------------------------------------------------------
public:
	F32				getPanLeftKey() const		{ return mPanLeftKey; }
	F32				getPanRightKey() const	{ return mPanRightKey; }
	F32				getPanUpKey() const		{ return mPanUpKey; }
	F32				getPanDownKey() const		{ return mPanDownKey; }
	F32				getPanInKey() const		{ return mPanInKey; }
	F32				getPanOutKey() const		{ return mPanOutKey; }

	void			setPanLeftKey(F32 mag)		{ mPanLeftKey = mag; }
	void			setPanRightKey(F32 mag)		{ mPanRightKey = mag; }
	void			setPanUpKey(F32 mag)		{ mPanUpKey = mag; }
	void			setPanDownKey(F32 mag)		{ mPanDownKey = mag; }
	void			setPanInKey(F32 mag)		{ mPanInKey = mag; }
	void			setPanOutKey(F32 mag)		{ mPanOutKey = mag; }

	void			clearPanKeys();
private:
	F32				mPanUpKey;						
	F32				mPanDownKey;					
	F32				mPanLeftKey;					
	F32				mPanRightKey;					
	F32				mPanInKey;
	F32				mPanOutKey;

	LLVector3d		mPanFocusDiff;

/**                    Keys
 **                                                                            **
 *******************************************************************************/

};

extern LLAgentCamera gAgentCamera;

#endif
