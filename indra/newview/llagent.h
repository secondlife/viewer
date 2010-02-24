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

#ifndef LL_LLAGENT_H
#define LL_LLAGENT_H

#include "indra_constants.h"
#include "llevent.h" 				// LLObservable base class
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

extern const BOOL 	ANIMATE;
extern const U8 	AGENT_STATE_TYPING;  // Typing indication
extern const U8 	AGENT_STATE_EDITING; // Set when agent has objects selected

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

enum EAnimRequest
{
	ANIM_REQUEST_START,
	ANIM_REQUEST_STOP
};

struct LLGroupData
{
	LLUUID mID;
	LLUUID mInsigniaID;
	U64 mPowers;
	BOOL mAcceptNotices;
	BOOL mListInProfile;
	S32 mContribution;
	std::string mName;
};

class LLAgentListener;

//------------------------------------------------------------------------
// LLAgent
//------------------------------------------------------------------------
class LLAgent : public LLOldEvents::LLObservable
{
	LOG_CLASS(LLAgent);

public:
	friend class LLAgentDropGroupViewerNode;

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/

	//--------------------------------------------------------------------
	// Constructors / Destructors
	//--------------------------------------------------------------------
public:
	LLAgent();
	virtual 		~LLAgent();
	void			init();
	void			cleanup();
	void			setAvatarObject(LLVOAvatarSelf *avatar);

	//--------------------------------------------------------------------
	// Login
	//--------------------------------------------------------------------
public:
	void			onAppFocusGained();
	void			setFirstLogin(BOOL b) 	{ mFirstLogin = b; }
	// Return TRUE if the database reported this login as the first for this particular user.
	BOOL 			isFirstLogin() const 	{ return mFirstLogin; }
public:
	BOOL			mInitialized;
	BOOL			mFirstLogin;
	std::string		mMOTD; 					// Message of the day
private:
	boost::shared_ptr<LLAgentListener> mListener;

	//--------------------------------------------------------------------
	// Session
	//--------------------------------------------------------------------
public:
	const LLUUID&	getID() const				{ return gAgentID; }
	const LLUUID&	getSessionID() const		{ return gAgentSessionID; }
	// Note: NEVER send this value in the clear or over any weakly
	// encrypted channel (such as simple XOR masking).  If you are unsure
	// ask Aaron or MarkL.
	const LLUUID&	getSecureSessionID() const	{ return mSecureSessionID; }
public:
	LLUUID			mSecureSessionID; 			// Secure token for this login session
	
/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    IDENTITY
 **/

	//--------------------------------------------------------------------
	// Name
	//--------------------------------------------------------------------
public:
	//*TODO remove, is not used as of August 20, 2009
	void			buildFullnameAndTitle(std::string &name) const;

	//--------------------------------------------------------------------
	// Gender
	//--------------------------------------------------------------------
public:
	// On the very first login, gender isn't chosen until the user clicks
	// in a dialog.  We don't render the avatar until they choose.
	BOOL 			isGenderChosen() const 	{ return mGenderChosen; }
	void			setGenderChosen(BOOL b)	{ mGenderChosen = b; }
private:
	BOOL			mGenderChosen;

/**                    Identity
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    GENERAL ACCESSORS
 **/

public:
 	LLVOAvatarSelf* getAvatarObject() const		{ return mAvatarObject; }
private:
	LLPointer<LLVOAvatarSelf> mAvatarObject; 	// NULL until avatar object sent down from simulator

/**                    General Accessors
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    POSITION
 **/

  	//--------------------------------------------------------------------
 	// Position
	//--------------------------------------------------------------------
public:
	LLVector3		getPosAgentFromGlobal(const LLVector3d &pos_global) const;
	LLVector3d		getPosGlobalFromAgent(const LLVector3 &pos_agent) const;	
	const LLVector3d &getPositionGlobal() const;
	const LLVector3	&getPositionAgent();
	// Call once per frame to update position, angles (radians).
	void			updateAgentPosition(const F32 dt, const F32 yaw, const S32 mouse_x, const S32 mouse_y);	
	void			setPositionAgent(const LLVector3 &center);
protected:
	void			propagate(const F32 dt); // ! BUG ! Should roll into updateAgentPosition
private:
	mutable LLVector3d mPositionGlobal;

  	//--------------------------------------------------------------------
 	// Velocity
	//--------------------------------------------------------------------
public:
	LLVector3		getVelocity() const;
	F32				getVelocityZ() const 	{ return getVelocity().mV[VZ]; } // ! HACK !
	
  	//--------------------------------------------------------------------
	// Coordinate System
	//--------------------------------------------------------------------
public:
	LLCoordFrame	getFrameAgent()	const	{ return mFrameAgent; }
	void 			initOriginGlobal(const LLVector3d &origin_global); // Only to be used in ONE place
	void			resetAxes();
	void			resetAxes(const LLVector3 &look_at); // Makes reasonable left and up
	// The following three get*Axis functions return direction avatar is looking, not camera.
	const LLVector3& getAtAxis() const		{ return mFrameAgent.getAtAxis(); }
	const LLVector3& getUpAxis() const		{ return mFrameAgent.getUpAxis(); }
	const LLVector3& getLeftAxis() const	{ return mFrameAgent.getLeftAxis(); }
	LLQuaternion	getQuat() const; 		// Returns the quat that represents the rotation of the agent in the absolute frame
private:
	LLVector3d		mAgentOriginGlobal;		// Origin of agent coords from global coords
	LLCoordFrame	mFrameAgent; 			// Agent position and view, agent-region coordinates


	//--------------------------------------------------------------------
	// Home
	//--------------------------------------------------------------------
public:
	void			setStartPosition(U32 location_id); // Marks current location as start, sends information to servers
	void			setHomePosRegion(const U64& region_handle, const LLVector3& pos_region);
	BOOL			getHomePosGlobal(LLVector3d* pos_global);
private:
	BOOL 			mHaveHomePosition;
	U64				mHomeRegionHandle;
	LLVector3		mHomePosRegion;

	//--------------------------------------------------------------------
	// Region
	//--------------------------------------------------------------------
public:
	void			setRegion(LLViewerRegion *regionp);
	LLViewerRegion	*getRegion() const;
	LLHost			getRegionHost() const;
	BOOL			inPrelude();
private:
	LLViewerRegion	*mRegionp;

	//--------------------------------------------------------------------
	// History
	//--------------------------------------------------------------------
public:
	S32				getRegionsVisited() const;
	F64				getDistanceTraveled() const;	
private:
	std::set<U64>	mRegionsVisited;		// Stat - what distinct regions has the avatar been to?
	F64				mDistanceTraveled;		// Stat - how far has the avatar moved?
	LLVector3d		mLastPositionGlobal;	// Used to calculate travel distance
	
/**                    Position
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    ACTIONS
 **/

	//--------------------------------------------------------------------
	// Fidget
	//--------------------------------------------------------------------
	// Trigger random fidget animations
public:
	void			fidget();
	static void		stopFidget();
private:
	LLFrameTimer	mFidgetTimer;
	LLFrameTimer	mFocusObjectFadeTimer;
	F32				mNextFidgetTime;
	S32				mCurrentFidget;

	//--------------------------------------------------------------------
	// Fly
	//--------------------------------------------------------------------
public:
	BOOL			getFlying() const;
	void			setFlying(BOOL fly);
	static void		toggleFlying();
	static bool		enableFlying();
	BOOL			canFly(); 			// Does this parcel allow you to fly?
	
	//--------------------------------------------------------------------
	// Chat
	//--------------------------------------------------------------------
public:
	void			heardChat(const LLUUID& id);
	void			lookAtLastChat();
	F32				getTypingTime() 		{ return mTypingTimer.getElapsedTimeF32(); }
	LLUUID			getLastChatter() const 	{ return mLastChatterID; }
	F32				getNearChatRadius() 	{ return mNearChatRadius; }
protected:
	void 			ageChat(); 				// Helper function to prematurely age chat when agent is moving
private:
	LLFrameTimer	mChatTimer;
	LLUUID			mLastChatterID;
	F32				mNearChatRadius;
	
	//--------------------------------------------------------------------
	// Typing
	//--------------------------------------------------------------------
public:
	void			startTyping();
	void			stopTyping();
public:
	// When the agent hasn't typed anything for this duration, it leaves the 
	// typing state (for both chat and IM).
	static const F32 TYPING_TIMEOUT_SECS;
private:
	LLFrameTimer	mTypingTimer;

	//--------------------------------------------------------------------
	// AFK
	//--------------------------------------------------------------------
public:
	void			setAFK();
	void			clearAFK();
	BOOL			getAFK() const;

	//--------------------------------------------------------------------
	// Run
	//--------------------------------------------------------------------
public:
	enum EDoubleTapRunMode
	{
		DOUBLETAP_NONE,
		DOUBLETAP_FORWARD,
		DOUBLETAP_BACKWARD,
		DOUBLETAP_SLIDELEFT,
		DOUBLETAP_SLIDERIGHT
	};

	void			setAlwaysRun() 			{ mbAlwaysRun = true; }
	void			clearAlwaysRun() 		{ mbAlwaysRun = false; }
	void			setRunning() 			{ mbRunning = true; }
	void			clearRunning() 			{ mbRunning = false; }
	void 			sendWalkRun(bool running);
	bool			getAlwaysRun() const 	{ return mbAlwaysRun; }
	bool			getRunning() const 		{ return mbRunning; }
public:
	LLFrameTimer 	mDoubleTapRunTimer;
	EDoubleTapRunMode mDoubleTapRunMode;
private:
	bool 			mbAlwaysRun; 			// Should the avatar run by default rather than walk?
	bool 			mbRunning;				// Is the avatar trying to run right now?

	//--------------------------------------------------------------------
	// Sit and stand
	//--------------------------------------------------------------------
public:
	void			standUp();

	//--------------------------------------------------------------------
	// Busy
	//--------------------------------------------------------------------
public:
	void			setBusy();
	void			clearBusy();
	BOOL			getBusy() const;
private:
	BOOL			mIsBusy;

	//--------------------------------------------------------------------
	// Jump
	//--------------------------------------------------------------------
public:
	BOOL			getJump() const	{ return mbJump; }
private:
	BOOL 			mbJump;

	//--------------------------------------------------------------------
	// Grab
	//--------------------------------------------------------------------
public:
	BOOL 			leftButtonGrabbed() const;
	BOOL 			rotateGrabbed() const;
	BOOL 			forwardGrabbed() const;
	BOOL 			backwardGrabbed() const;
	BOOL 			upGrabbed() const;
	BOOL 			downGrabbed() const;

	//--------------------------------------------------------------------
	// Controls
	//--------------------------------------------------------------------
public:
	U32 			getControlFlags(); 
	void 			setControlFlags(U32 mask); 		// Performs bitwise mControlFlags |= mask
	void 			clearControlFlags(U32 mask); 	// Performs bitwise mControlFlags &= ~mask
	BOOL			controlFlagsDirty() const;
	void			enableControlFlagReset();
	void 			resetControlFlags();
	BOOL			anyControlGrabbed() const; 		// True iff a script has taken over a control
	BOOL			isControlGrabbed(S32 control_index) const;
	// Send message to simulator to force grabbed controls to be
	// released, in case of a poorly written script.
	void			forceReleaseControls();
private:
	S32				mControlsTakenCount[TOTAL_CONTROLS];
	S32				mControlsTakenPassedOnCount[TOTAL_CONTROLS];
	U32				mControlFlags;					// Replacement for the mFooKey's
	BOOL 			mbFlagsDirty;
	BOOL 			mbFlagsNeedReset;				// ! HACK ! For preventing incorrect flags sent when crossing region boundaries
	
	//--------------------------------------------------------------------
	// Animations
	//--------------------------------------------------------------------
public:
	void            stopCurrentAnimations();
	void			requestStopMotion(LLMotion* motion);
	void			onAnimStop(const LLUUID& id);
	void			sendAnimationRequests(LLDynamicArray<LLUUID> &anim_ids, EAnimRequest request);
	void			sendAnimationRequest(const LLUUID &anim_id, EAnimRequest request);
	void			endAnimationUpdateUI();
private:
	LLFrameTimer	mAnimationTimer; 	// Seconds that transition animation has been active
	F32				mAnimationDuration;	// In seconds
	BOOL            mCustomAnim; 		// Current animation is ANIM_AGENT_CUSTOMIZE ?
	LLAnimPauseRequest mPauseRequest;
	BOOL			mViewsPushed; 		// Keep track of whether or not we have pushed views
	
/**                    Animation
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    MOVEMENT
 **/
	
	//--------------------------------------------------------------------
	// Keys
	//--------------------------------------------------------------------
public:
	void			setKey(const S32 direction, S32 &key); // Sets key to +1 for +direction, -1 for -direction
private:
	S32 			mAtKey;				// Either 1, 0, or -1. Indicates that movement key is pressed
	S32				mWalkKey; 			// Like AtKey, but causes less forward thrust
	S32 			mLeftKey;
	S32				mUpKey;
	F32				mYawKey;
	F32				mPitchKey;

	//--------------------------------------------------------------------
	// Movement from user input
	//--------------------------------------------------------------------
	// All set the appropriate animation flags.
	// All turn off autopilot and make sure the camera is behind the avatar.
	// Direction is either positive, zero, or negative
public:
	void			moveAt(S32 direction, bool reset_view = true);
	void			moveAtNudge(S32 direction);
	void			moveLeft(S32 direction);
	void			moveLeftNudge(S32 direction);
	void			moveUp(S32 direction);
	void			moveYaw(F32 mag, bool reset_view = true);
	void			movePitch(F32 mag);

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
 	// Move the avatar's frame
	//--------------------------------------------------------------------
public:
	void			rotate(F32 angle, const LLVector3 &axis);
	void			rotate(F32 angle, F32 x, F32 y, F32 z);
	void			rotate(const LLMatrix3 &matrix);
	void			rotate(const LLQuaternion &quaternion);
	void			pitch(F32 angle);
	void			roll(F32 angle);
	void			yaw(F32 angle);
	LLVector3		getReferenceUpVector();
    F32             clampPitchToLimits(F32 angle);

	//--------------------------------------------------------------------
	// Autopilot
	//--------------------------------------------------------------------
public:
	BOOL			getAutoPilot() const				{ return mAutoPilot; }
	LLVector3d		getAutoPilotTargetGlobal() const 	{ return mAutoPilotTargetGlobal; }
	void			startAutoPilotGlobal(const LLVector3d &pos_global, 
										 const std::string& behavior_name = std::string(), 
										 const LLQuaternion *target_rotation = NULL, 
										 void (*finish_callback)(BOOL, void *) = NULL, void *callback_data = NULL, 
										 F32 stop_distance = 0.f, F32 rotation_threshold = 0.03f);
	void 			startFollowPilot(const LLUUID &leader_id);
	void			stopAutoPilot(BOOL user_cancel = FALSE);
	void 			setAutoPilotGlobal(const LLVector3d &pos_global);
	void			autoPilot(F32 *delta_yaw); 			// Autopilot walking action, angles in radians
	void			renderAutoPilotTarget();
private:
	BOOL			mAutoPilot;
	BOOL			mAutoPilotFlyOnStop;
	LLVector3d		mAutoPilotTargetGlobal;
	F32				mAutoPilotStopDistance;
	BOOL			mAutoPilotUseRotation;
	LLVector3		mAutoPilotTargetFacing;
	F32				mAutoPilotTargetDist;
	S32				mAutoPilotNoProgressFrameCount;
	F32				mAutoPilotRotationThreshold;
	std::string		mAutoPilotBehaviorName;
	void			(*mAutoPilotFinishedCallback)(BOOL, void *);
	void*			mAutoPilotCallbackData;
	LLUUID			mLeaderID;
	
/**                    Movement
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    TELEPORT
 **/

public:
	enum ETeleportState
	{
		TELEPORT_NONE = 0,			// No teleport in progress
		TELEPORT_START = 1,			// Transition to REQUESTED.  Viewer has sent a TeleportRequest to the source simulator
		TELEPORT_REQUESTED = 2,		// Waiting for source simulator to respond
		TELEPORT_MOVING = 3,		// Viewer has received destination location from source simulator
		TELEPORT_START_ARRIVAL = 4,	// Transition to ARRIVING.  Viewer has received avatar update, etc., from destination simulator
		TELEPORT_ARRIVING = 5		// Make the user wait while content "pre-caches"
	};

public:
	static void 	parseTeleportMessages(const std::string& xml_filename);
	const std::string getTeleportSourceSLURL() const { return mTeleportSourceSLURL; }
public:
	// ! TODO ! Define ERROR and PROGRESS enums here instead of exposing the mappings.
	static std::map<std::string, std::string> sTeleportErrorMessages;
	static std::map<std::string, std::string> sTeleportProgressMessages;
private:
	std::string		mTeleportSourceSLURL; 			// SLURL where last TP began

	//--------------------------------------------------------------------
	// Teleport Actions
	//--------------------------------------------------------------------
public:
	void 			teleportRequest(const U64& region_handle,
									const LLVector3& pos_local);			// Go to a named location home
	void 			teleportViaLandmark(const LLUUID& landmark_id);			// Teleport to a landmark
	void 			teleportHome()	{ teleportViaLandmark(LLUUID::null); }	// Go home
	void 			teleportViaLure(const LLUUID& lure_id, BOOL godlike);	// To an invited location
	void 			teleportViaLocation(const LLVector3d& pos_global);		// To a global location - this will probably need to be deprecated
	void 			teleportCancel();										// May or may not be allowed by server
protected:
	bool 			teleportCore(bool is_local = false); 					// Stuff for all teleports; returns true if the teleport can proceed

	//--------------------------------------------------------------------
	// Teleport State
	//--------------------------------------------------------------------
public:
	ETeleportState	getTeleportState() const 						{ return mTeleportState; }
	void			setTeleportState(ETeleportState state);
private:
	ETeleportState	mTeleportState;

	//--------------------------------------------------------------------
	// Teleport Message
	//--------------------------------------------------------------------
public:
	const std::string& getTeleportMessage() const 					{ return mTeleportMessage; }
	void 			setTeleportMessage(const std::string& message) 	{ mTeleportMessage = message; }
private:
	std::string		mTeleportMessage;
	
/**                    Teleport
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    CAMERA
 **/

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
	void			updateCamera();					// Call once per frame to update camera location/orientation
	void			resetCamera(); 					// Slam camera into its default position
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
	const LLColor4	&getEffectColor();
	void			setEffectColor(const LLColor4 &color);
public:
	F32				mHUDTargetZoom;	// Target zoom level for HUD objects (used when editing)
	F32				mHUDCurZoom; 	// Current animated zoom level for HUD objects
private:
	LLUIColor 		mEffectColor;

/**                    Camera
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    ACCESS
 **/

public:
	// Checks if agent can modify an object based on the permissions and the agent's proxy status.
	BOOL			isGrantedProxy(const LLPermissions& perm);
	BOOL			allowOperation(PermissionBit op,
								   const LLPermissions& perm,
								   U64 group_proxy_power = 0,
								   U8 god_minimum = GOD_MAINTENANCE);
	const LLAgentAccess& getAgentAccess();
	BOOL			canManageEstate() const;
	BOOL			getAdminOverride() const;
	// ! BACKWARDS COMPATIBILITY ! This function can go away after the AO transition (see llstartup.cpp).
	void 			setAOTransition();
private:
	LLAgentAccess 	mAgentAccess;
	
	//--------------------------------------------------------------------
	// God
	//--------------------------------------------------------------------
public:
	BOOL			isGodlike() const;
	U8				getGodLevel() const;
	void			setAdminOverride(BOOL b);
	void			setGodLevel(U8 god_level);
	void			requestEnterGodMode();
	void			requestLeaveGodMode();

	//--------------------------------------------------------------------
	// Maturity
	//--------------------------------------------------------------------
public:
	// Note: this is a prime candidate for pulling out into a Maturity class.
	// Rather than just expose the preference setting, we're going to actually
	// expose what the client code cares about -- what the user should see
	// based on a combination of the is* and prefers* flags, combined with god bit.
	bool 			wantsPGOnly() const;
	bool 			canAccessMature() const;
	bool 			canAccessAdult() const;
	bool 			canAccessMaturityInRegion( U64 region_handle ) const;
	bool 			canAccessMaturityAtGlobal( LLVector3d pos_global ) const;
	bool 			prefersPG() const;
	bool 			prefersMature() const;
	bool 			prefersAdult() const;
	bool 			isTeen() const;
	bool 			isMature() const;
	bool 			isAdult() const;
	void 			setTeen(bool teen);
	void 			setMaturity(char text);
	static int 		convertTextToMaturity(char text); 
	bool 			sendMaturityPreferenceToServer(int preferredMaturity); // ! "U8" instead of "int"?

	// Maturity callbacks for PreferredMaturity control variable
	void 			handleMaturity(const LLSD& newvalue);
	bool 			validateMaturity(const LLSD& newvalue);


/**                    Access
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    RENDERING
 **/

public:
	LLQuaternion	getHeadRotation();
	BOOL			needsRenderAvatar(); // TRUE when camera mode is such that your own avatar should draw
	BOOL			needsRenderHead();
public:
	F32				mDrawDistance;
private:
	BOOL			mShowAvatar; 		// Should we render the avatar?
	U32				mAppearanceSerialNum;
	
	//--------------------------------------------------------------------
	// Rendering state bitmap helpers
	//--------------------------------------------------------------------
public:
	void			setRenderState(U8 newstate);
	void			clearRenderState(U8 clearstate);
	U8				getRenderState();
private:
	U8				mRenderState; // Current behavior state of agent

/**                    Rendering
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    GROUPS
 **/

public:
	const LLUUID	&getGroupID() const			{ return mGroupID; }
	// Get group information by group_id, or FALSE if not in group.
	BOOL 			getGroupData(const LLUUID& group_id, LLGroupData& data) const;
	// Get just the agent's contribution to the given group.
	S32 			getGroupContribution(const LLUUID& group_id) const;
	// Update internal datastructures and update the server.
	BOOL 			setGroupContribution(const LLUUID& group_id, S32 contribution);
	BOOL 			setUserGroupFlags(const LLUUID& group_id, BOOL accept_notices, BOOL list_in_profile);
	const std::string &getGroupName() const 	{ return mGroupName; }
	BOOL			canJoinGroups() const;
private:
	std::string		mGroupName;
	LLUUID			mGroupID;

	//--------------------------------------------------------------------
	// Group Membership
	//--------------------------------------------------------------------
public:
	// Checks against all groups in the entire agent group list.
	BOOL 			isInGroup(const LLUUID& group_id, BOOL ingnore_God_mod = FALSE) const;
protected:
	// Only used for building titles.
	BOOL			isGroupMember() const 		{ return !mGroupID.isNull(); } 
public:
	LLDynamicArray<LLGroupData> mGroups;

	//--------------------------------------------------------------------
	// Group Title
	//--------------------------------------------------------------------
public:
	void			setHideGroupTitle(BOOL hide)	{ mHideGroupTitle = hide; }
	BOOL			isGroupTitleHidden() const 		{ return mHideGroupTitle; }
private:
	std::string		mGroupTitle; 					// Honorific, like "Sir"
	BOOL			mHideGroupTitle;

	//--------------------------------------------------------------------
	// Group Powers
	//--------------------------------------------------------------------
public:
	BOOL 			hasPowerInGroup(const LLUUID& group_id, U64 power) const;
	BOOL 			hasPowerInActiveGroup(const U64 power) const;
	U64  			getPowerInGroup(const LLUUID& group_id) const;
 	U64				mGroupPowers;

	//--------------------------------------------------------------------
	// Friends
	//--------------------------------------------------------------------
public:
	void 			observeFriends();
	void 			friendsChanged();
private:
	LLFriendObserver* mFriendObserver;
	std::set<LLUUID> mProxyForAgents;

/**                    Groups
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    MESSAGING
 **/

	//--------------------------------------------------------------------
	// Send
	//--------------------------------------------------------------------
public:
	void			sendMessage(); // Send message to this agent's region
	void			sendReliableMessage();
	void			sendAgentSetAppearance();
	void 			sendAgentDataUpdateRequest();
	void 			sendAgentUserInfoRequest();
	// IM to Email and Online visibility
	void			sendAgentUpdateUserInfo(bool im_to_email, const std::string& directory_visibility);

	//--------------------------------------------------------------------
	// Receive
	//--------------------------------------------------------------------
public:
	static void		processAgentDataUpdate(LLMessageSystem *msg, void **);
	static void		processAgentGroupDataUpdate(LLMessageSystem *msg, void **);
	static void		processAgentDropGroup(LLMessageSystem *msg, void **);
	static void		processScriptControlChange(LLMessageSystem *msg, void **);
	static void		processAgentCachedTextureResponse(LLMessageSystem *mesgsys, void **user_data);
	
/**                    Messaging
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    DEBUGGING
 **/

public:
	static void		dumpGroupInfo();
	static void		clearVisualParams(void *);
	friend std::ostream& operator<<(std::ostream &s, const LLAgent &sphere);

/**                    Debugging
 **                                                                            **
 *******************************************************************************/

};

extern LLAgent gAgent;

inline bool operator==(const LLGroupData &a, const LLGroupData &b)
{
	return (a.mID == b.mID);
}

class LLAgentQueryManager
{
	friend class LLAgent;
	friend class LLAgentWearables;
	
public:
	LLAgentQueryManager();
	virtual ~LLAgentQueryManager();
	
	BOOL 			hasNoPendingQueries() const 	{ return getNumPendingQueries() == 0; }
	S32 			getNumPendingQueries() const 	{ return mNumPendingQueries; }
private:
	S32				mNumPendingQueries;
	S32				mWearablesCacheQueryID;
	U32				mUpdateSerialNum;
	S32		    	mActiveCacheQueries[LLVOAvatarDefines::BAKED_NUM_INDICES];
};

extern LLAgentQueryManager gAgentQueryManager;

#endif
