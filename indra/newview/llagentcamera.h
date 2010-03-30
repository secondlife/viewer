/** 
 * @file llagent.h
 * @brief LLAgent class header file
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_LLAGENTCAMERA_H
#define LL_LLAGENTCAMERA_H

#include "indra_constants.h"
#include "llevent.h" 				// LLObservable base class
#include "llagent.h"
#include "llagentaccess.h"
#include "llagentconstants.h"
#include "llagentdata.h" 			// gAgentID, gAgentSessionID
#include "llcharacter.h" 			// LLAnimPauseRequest
#include "llfollowcam.h" 			// Ventrella
#include "llhudeffectlookat.h" 		// EPointAtType
#include "llhudeffectpointat.h" 	// ELookAtType
#include "llpointer.h"
#include "lluicolor.h"
#include "llvoavatardefines.h"

class LLChat;
class LLVOAvatarSelf;
class LLViewerRegion;
class LLMotion;
class LLToolset;
class LLMessageSystem;
class LLPermissions;
class LLHost;
class LLFriendObserver;
class LLPickInfo;
class LLViewerObject;
class LLAgentDropGroupViewerNode;

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
	CAMERA_PRESET_GROUP_VIEW
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
private:
	BOOL			mInitialized;


	//--------------------------------------------------------------------
	// Mode
	//--------------------------------------------------------------------
public:
	void			changeCameraToDefault();
	void			changeCameraToMouselook(BOOL animate = TRUE);
	void			changeCameraToThirdPerson(BOOL animate = TRUE);
	void			changeCameraToCustomizeAvatar(BOOL avatar_animate = TRUE, BOOL camera_animate = TRUE); // Trigger transition animation
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
private:
	/** Determines default camera offset depending on the current camera preset */
	LLVector3 getCameraOffsetInitial();

	/** Camera preset in Third Person Mode */
	ECameraPreset mCameraPreset; 

	/** Initial camera offsets */
	std::map<ECameraPreset, LLVector3> mCameraOffsetInitial;

	/** Initial focus offsets */
	std::map<ECameraPreset, LLVector3d> mFocusOffsetInitial;

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
	F32				calcCustomizeAvatarUIOffset(const LLVector3d& camera_pos_global);
	F32				getCurrentCameraBuildOffset() 	{ return (F32)mCameraFocusOffset.length(); }
	void			clearCameraLag() { mCameraLag.clearVec(); }
private:
	F32				mCurrentCameraDistance;	 		// Current camera offset from avatar
	F32				mTargetCameraDistance;			// Target camera offset from avatar
	F32				mCameraFOVZoomFactor;			// Amount of fov zoom applied to camera when zeroing in on an object
	F32				mCameraCurrentFOVZoomFactor;	// Interpolated fov zoom
	F32				mCameraFOVDefault;				// Default field of view that is basis for FOV zoom effect
	LLVector4		mCameraCollidePlane;			// Colliding plane for camera
	F32				mCameraZoomFraction;			// Mousewheel driven fraction of zoom
	LLVector3		mCameraPositionAgent;			// Camera position in agent coordinates
	LLVector3		mCameraVirtualPositionAgent;	// Camera virtual position (target) before performing FOV zoom
	LLVector3d      mCameraSmoothingLastPositionGlobal;    
	LLVector3d      mCameraSmoothingLastPositionAgent;
	BOOL            mCameraSmoothingStop;
	LLVector3		mCameraLag;						// Third person camera lag
	LLVector3		mCameraUpVector;				// Camera's up direction in world coordinates (determines the 'roll' of the view)

	//--------------------------------------------------------------------
	// Orbit
	//--------------------------------------------------------------------
public:
	void			setOrbitLeftKey(F32 mag)	{ mOrbitLeftKey = mag; }
	void			setOrbitRightKey(F32 mag)	{ mOrbitRightKey = mag; }
	void			setOrbitUpKey(F32 mag)		{ mOrbitUpKey = mag; }
	void			setOrbitDownKey(F32 mag)	{ mOrbitDownKey = mag; }
	void			setOrbitInKey(F32 mag)		{ mOrbitInKey = mag; }
	void			setOrbitOutKey(F32 mag)		{ mOrbitOutKey = mag; }
private:
	F32				mOrbitLeftKey;
	F32				mOrbitRightKey;
	F32				mOrbitUpKey;
	F32				mOrbitDownKey;
	F32				mOrbitInKey;
	F32				mOrbitOutKey;

	//--------------------------------------------------------------------
	// Pan
	//--------------------------------------------------------------------
public:
	void			setPanLeftKey(F32 mag)		{ mPanLeftKey = mag; }
	void			setPanRightKey(F32 mag)		{ mPanRightKey = mag; }
	void			setPanUpKey(F32 mag)		{ mPanUpKey = mag; }
	void			setPanDownKey(F32 mag)		{ mPanDownKey = mag; }
	void			setPanInKey(F32 mag)		{ mPanInKey = mag; }
	void			setPanOutKey(F32 mag)		{ mPanOutKey = mag; }
private:
	F32				mPanUpKey;						
	F32				mPanDownKey;					
	F32				mPanLeftKey;					
	F32				mPanRightKey;					
	F32				mPanInKey;
	F32				mPanOutKey;	
	
	//--------------------------------------------------------------------
	// Follow
	//--------------------------------------------------------------------
public:
	void			setUsingFollowCam(bool using_follow_cam);
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
	void			setAnimationDuration(F32 seconds) 	{ mAnimationDuration = seconds; }
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
	void			setFocusOnAvatar(BOOL focus, BOOL animate);
	void			setCameraPosAndFocusGlobal(const LLVector3d& pos, const LLVector3d& focus, const LLUUID &object_id);
	void			clearFocusObject();
	void			setFocusObject(LLViewerObject* object);
	void			setObjectTracking(BOOL track) 	{ mTrackFocusObject = track; }
	const LLVector3d &getFocusGlobal() const		{ return mFocusGlobal; }
	const LLVector3d &getFocusTargetGlobal() const	{ return mFocusTargetGlobal; }
private:
	LLVector3d		mCameraFocusOffset;				// Offset from focus point in build mode
	LLVector3d		mCameraFocusOffsetTarget;		// Target towards which we are lerping the camera's focus offset
	BOOL			mFocusOnAvatar;					
	LLVector3d		mFocusGlobal;
	LLVector3d		mFocusTargetGlobal;
	LLPointer<LLViewerObject> mFocusObject;
	F32				mFocusObjectDist;
	LLVector3		mFocusObjectOffset;
	F32				mFocusDotRadius; 				// Meters
	BOOL			mTrackFocusObject;
	F32				mUIOffset;	
	
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

	//--------------------------------------------------------------------
	// Zoom
	//--------------------------------------------------------------------
public:
	void			handleScrollWheel(S32 clicks); 			// Mousewheel driven zoom
	void			cameraZoomIn(const F32 factor);			// Zoom in by fraction of current distance
	F32				getCameraZoomFraction();				// Get camera zoom as fraction of minimum and maximum zoom
	void			setCameraZoomFraction(F32 fraction);	// Set camera zoom as fraction of minimum and maximum zoom
	F32				calcCameraFOVZoomFactor();

	//--------------------------------------------------------------------
	// Pan
	//--------------------------------------------------------------------
public:
	void			cameraPanIn(const F32 meters);
	void			cameraPanLeft(const F32 meters);
	void			cameraPanUp(const F32 meters);
	
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
};

extern LLAgentCamera gAgentCamera;

#endif
