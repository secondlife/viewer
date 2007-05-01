/** 
 * @file llagent.h
 * @brief LLAgent class header file
 *
 * Copyright (c) 2000-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLAGENT_H
#define LL_LLAGENT_H

#include <set>

#include "indra_constants.h"
#include "llmath.h"
#include "llcoordframe.h"
#include "llevent.h"
#include "llagentconstants.h"
#include "llanimationstates.h"
#include "lldbstrings.h"
#include "llhudeffectlookat.h"
#include "llhudeffectpointat.h"
#include "llmemory.h"
#include "llstring.h"
#include "lluuid.h"
#include "m3math.h"
#include "m4math.h"
#include "llquaternion.h"
#include "lltimer.h"
#include "v3dmath.h"
#include "v3math.h"
#include "v4color.h"
#include "v4math.h"
//#include "vmath.h"
#include "stdenums.h"
#include "llwearable.h"
#include "llcharacter.h"
#include "llinventory.h"
#include "llviewerinventory.h"
#include "llagentdata.h"

// Ventrella
#include "llfollowcam.h"
// end Ventrella

const U8 AGENT_STATE_TYPING =	0x04;			//  Typing indication
const U8 AGENT_STATE_EDITING =  0x10;			//  Set when agent has objects selected

const BOOL ANIMATE = TRUE;

typedef enum e_camera_modes
{
	CAMERA_MODE_THIRD_PERSON,
	CAMERA_MODE_MOUSELOOK,
	CAMERA_MODE_CUSTOMIZE_AVATAR,
	CAMERA_MODE_FOLLOW
} ECameraMode;

typedef enum e_anim_request
{
	ANIM_REQUEST_START,
	ANIM_REQUEST_STOP
} EAnimRequest;

class LLChat;
class LLVOAvatar;
class LLViewerRegion;
class LLMotion;
class LLToolset;
class LLMessageSystem;
class LLPermissions;
class LLHost;
class LLFriendObserver;

struct LLGroupData
{
	LLUUID mID;
	LLUUID mInsigniaID;
	U64 mPowers;
	BOOL mAcceptNotices;
	S32 mContribution;
	std::string mName;
};

inline bool operator==(const LLGroupData &a, const LLGroupData &b)
{
	return (a.mID == b.mID);
}

// forward declarations

//

class LLAgent : public LLObservable
{
public:
	// When the agent hasn't typed anything for this duration, it leaves the 
	// typing state (for both chat and IM).
	static const F32 TYPING_TIMEOUT_SECS;

	LLAgent();
	~LLAgent();

	void			init();
	void			cleanup();

	//
	// MANIPULATORS
	//
	// TODO: Put all non-const functions here.

	// Called whenever the agent moves.  Puts camera back in default position,
	// deselects items, etc.
	void			resetView(BOOL reset_camera = TRUE);

	// Called on camera movement, to allow the camera to be unlocked from the 
	// default position behind the avatar.
	void			unlockView();

	void			sendMessage();						// Send message to this agent's region.
	void			sendReliableMessage();

	LLVector3d		calcCameraPositionTargetGlobal(BOOL *hit_limit = NULL); // Calculate the camera position target
	LLVector3d		calcFocusPositionTargetGlobal();			// target for this mode
	LLVector3d		getCameraPositionGlobal() const;
	const LLVector3 &getCameraPositionAgent() const;
	F32				calcCameraFOVZoomFactor();
	F32				getCameraMinOffGround();			// minimum height off ground for this mode, meters
	void			endAnimationUpdateUI();
	void			setKey(const S32 direction, S32 &key);		// sets key to +1 for +direction, -1 for -direction
	void			handleScrollWheel(S32 clicks);				// mousewheel driven zoom
	
	void			setAvatarObject(LLVOAvatar *avatar);

	// rendering state bitmask helpers
	void			startTyping();
	void			stopTyping();
	void			setRenderState(U8 newstate);
	void			clearRenderState(U8 clearstate);
	U8				getRenderState();

	// Set the home data
	void			setRegion(LLViewerRegion *regionp);
	LLViewerRegion	*getRegion() const;
	const LLHost&	getRegionHost() const;

	void			updateAgentPosition(const F32 dt, const F32 yaw, const S32 mouse_x, const S32 mouse_y);		// call once per frame to update position, angles radians
	void			updateLookAt(const S32 mouse_x, const S32 mouse_y);


	void			updateCamera();			// call once per frame to update camera location/orientation
	void			resetCamera();						// slam camera into its default position
	void			setupSitCamera();
	void			setCameraCollidePlane(LLVector4 &plane) { mCameraCollidePlane = plane; }

	void			changeCameraToDefault();
	void			changeCameraToMouselook(BOOL animate = TRUE);
	void			changeCameraToThirdPerson(BOOL animate = TRUE);
	void			changeCameraToCustomizeAvatar(BOOL animate = TRUE);			// trigger transition animation
	// Ventrella
	void			changeCameraToFollow(BOOL animate = TRUE);
	//end Ventrella

	void			setFocusGlobal(const LLVector3d &focus, const LLUUID &object_id = LLUUID::null);
	void			setFocusOnAvatar(BOOL focus, BOOL animate);
	void			setCameraPosAndFocusGlobal(const LLVector3d& pos, const LLVector3d& focus, const LLUUID &object_id);
	void			setSitCamera(const LLUUID &object_id, const LLVector3 &camera_pos = LLVector3::zero, const LLVector3 &camera_focus = LLVector3::zero);
	void			clearFocusObject();
	void			setFocusObject(LLViewerObject* object);
	void			setObjectTracking(BOOL track) { mTrackFocusObject = track; }
//	void			setLookingAtAvatar(BOOL looking);

	void			heardChat(const LLChat& chat);
	void			lookAtLastChat();
	LLUUID			getLastChatter() { return mLastChatterID; }
	F32				getTypingTime() { return mTypingTimer.getElapsedTimeF32(); }

	void			setAFK();
	void			clearAFK();
	BOOL			getAFK() const;

	void			setAlwaysRun()	{ mbAlwaysRun = TRUE; }
	void			clearAlwaysRun() { mbAlwaysRun = FALSE; }
	BOOL			getAlwaysRun() const { return mbAlwaysRun; }

	void			setBusy();
	void			clearBusy();
	BOOL			getBusy() const;

	void			setAdminOverride(BOOL b)	{ mAdminOverride = b; }
	void			setGodLevel(U8 god_level)	{ mGodLevel = god_level; }
	void			setFirstLogin(BOOL b)		{ mFirstLogin = b; }
	void			setGenderChosen(BOOL b)		{ mGenderChosen = b; }
		
	BOOL			getAdminOverride() const	{ return mAdminOverride; }
	// update internal datastructures and update the server with the
	// new contribution level. Returns true if the group id was found
	// and contribution could be set.
	BOOL setGroupContribution(const LLUUID& group_id, S32 contribution);
	BOOL setGroupAcceptNotices(const LLUUID& group_id, BOOL accept_notices);
	void			setHideGroupTitle(BOOL hide)	{ mHideGroupTitle = hide; }

	//
	// ACCESSORS
	//
	// TODO: Put all read functions here, make them const

	const LLUUID&	getID() const				{ return gAgentID; }
	const LLUUID&	getSessionID() const		{ return gAgentSessionID; }
	
	const LLUUID&	getSecureSessionID() const	{ return mSecureSessionID; }
		// Note: NEVER send this value in the clear or over any weakly
		// encrypted channel (such as simple XOR masking).  If you are unsure
		// ask Aaron or MarkL.
		
	BOOL			isGodlike() const;
	U8				getGodLevel() const;
	BOOL			isGroupTitleHidden() const		{ return mHideGroupTitle; }
	BOOL			isGroupMember() const		{ return !mGroupID.isNull(); }		// This is only used for building titles!
	const LLUUID	&getGroupID() const			{ return mGroupID; }
	ECameraMode		getCameraMode() const		{ return mCameraMode; }
	BOOL			getFocusOnAvatar() const	{ return mFocusOnAvatar; }
	LLViewerObject*	getFocusObject() const		{ return mFocusObject; }
	F32				getFocusObjectDist() const	{ return mFocusObjectDist; }
	BOOL			inPrelude();
	BOOL			canManageEstate() const;

	const LLUUID&	getInventoryRootID() const 	{ return mInventoryRootID; }

	void			buildFullname(std::string &name) const;
	void			buildFullnameAndTitle(std::string &name) const;

	// Check against all groups in the entire agent group list.
	BOOL isInGroup(const LLUUID& group_id) const;
	BOOL hasPowerInGroup(const LLUUID& group_id, U64 power) const;
	// Check for power in just the active group.
	BOOL hasPowerInActiveGroup(const U64 power) const;
	U64  getPowerInGroup(const LLUUID& group_id) const;

	// Get group information by group_id. if not in group, data is
	// left unchanged and method returns FALSE. otherwise, values are
	// copied and returns TRUE.
	BOOL getGroupData(const LLUUID& group_id, LLGroupData& data) const;
	// Get just the agent's contribution to the given group.
	S32 getGroupContribution(const LLUUID& group_id) const;

	// return TRUE if the database reported this login as the first
	// for this particular user.
	BOOL isFirstLogin() const { return mFirstLogin; }

	// On the very first login, gender isn't chosen until the user clicks
	// in a dialog.  We don't render the avatar until they choose.
	BOOL isGenderChosen() const { return mGenderChosen; }

	// utility to build a location string
	void buildLocationString(std::string& str);

	LLQuaternion	getHeadRotation();
 	LLVOAvatar	   *getAvatarObject() const			{ return mAvatarObject; }

	BOOL			needsRenderAvatar();		// TRUE when camera mode is such that your own avatar should draw
												// Not const because timers can't be accessed in const-fashion.
	BOOL			needsRenderHead();
	BOOL			cameraThirdPerson() const		{ return (mCameraMode == CAMERA_MODE_THIRD_PERSON && mLastCameraMode == CAMERA_MODE_THIRD_PERSON); }
	BOOL			cameraMouselook() const			{ return (mCameraMode == CAMERA_MODE_MOUSELOOK && mLastCameraMode == CAMERA_MODE_MOUSELOOK); }
	BOOL			cameraCustomizeAvatar() const	{ return (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR /*&& !mCameraAnimating*/); }
	BOOL			cameraFollow() const			{ return (mCameraMode == CAMERA_MODE_FOLLOW && mLastCameraMode == CAMERA_MODE_FOLLOW); }

	LLVector3		getPosAgentFromGlobal(const LLVector3d &pos_global) const;
	LLVector3d		getPosGlobalFromAgent(const LLVector3 &pos_agent)	const;

	// Get the data members
	const LLVector3&	getAtAxis()		const	{ return mFrameAgent.getAtAxis(); }		// direction avatar is looking, not camera
	const LLVector3&	getUpAxis()		const	{ return mFrameAgent.getUpAxis(); }		// direction avatar is looking, not camera
	const LLVector3&	getLeftAxis()	const	{ return mFrameAgent.getLeftAxis(); }	// direction avatar is looking, not camera

	LLCoordFrame		getFrameAgent()	const	{ return mFrameAgent; }
	LLVector3			getVelocity()	const;
	F32					getVelocityZ()	const	{ return getVelocity().mV[VZ]; }	// a hack

	const LLVector3d	&getPositionGlobal();
	const LLVector3		&getPositionAgent();
	const S32			getRegionsVisited() const;
	const F64			getDistanceTraveled() const;

	const LLVector3d	&getFocusGlobal() const	{ return mFocusGlobal; }
	const LLVector3d	&getFocusTargetGlobal() const	{ return mFocusTargetGlobal; }

	BOOL				getJump() const			{ return mbJump; }
	BOOL				getAutoPilot() const	{ return mAutoPilot; }
	LLVector3d			getAutoPilotTargetGlobal() const	{ return mAutoPilotTargetGlobal; }

	LLQuaternion		getQuat() const;							// returns the quat that represents the rotation 
																	// of the agent in the absolute frame
//	BOOL				getLookingAtAvatar() const;

	void				getName(LLString& name);

	const LLColor4		&getEffectColor();
	void				setEffectColor(const LLColor4 &color);
	//
	// UTILITIES
	//

	// Set the physics data
	void 			slamLookAt(const LLVector3 &look_at);

	void			setPositionAgent(const LLVector3 &center);

	void			resetAxes();
	void			resetAxes(const LLVector3 &look_at);						// makes reasonable left and up

	// Move the avatar's frame
	void			rotate(F32 angle, const LLVector3 &axis);
	void			rotate(F32 angle, F32 x, F32 y, F32 z);
	void			rotate(const LLMatrix3 &matrix);
	void			rotate(const LLQuaternion &quaternion);
	void			pitch(F32 angle);
	void			roll(F32 angle);
	void			yaw(F32 angle);
	LLVector3		getReferenceUpVector();

	void			setThirdPersonHeadOffset(LLVector3 offset) { mThirdPersonHeadOffset = offset; }
	// Flight management
	BOOL			getFlying() const				{ return mControlFlags & AGENT_CONTROL_FLY; }
	void			setFlying(BOOL fly);
	void			toggleFlying();

	// Does this parcel allow you to fly?
	BOOL canFly();

	// Animation functions
	void			requestStopMotion( LLMotion* motion );
	void			onAnimStop(const LLUUID& id);

	void			sendAnimationRequests(LLDynamicArray<LLUUID> &anim_ids, EAnimRequest request);
	void			sendAnimationRequest(const LLUUID &anim_id, EAnimRequest request);

	LLVector3d		calcFocusOffset(LLViewerObject *object, S32 x, S32 y);
	BOOL			calcCameraMinDistance(F32 &obj_min_distance);

	void			startCameraAnimation();
	void			stopCameraAnimation();

	void			cameraZoomIn(const F32 factor);			// zoom in by fraction of current distance
	void			cameraOrbitAround(const F32 radians);	// rotate camera CCW radians about build focus point
	void			cameraOrbitOver(const F32 radians);		// rotate camera forward radians over build focus point
	void			cameraOrbitIn(const F32 meters);		// move camera in toward build focus point

	F32				getCameraZoomFraction();				// get camera zoom as fraction of minimum and maximum zoom
	void			setCameraZoomFraction(F32 fraction);	// set camera zoom as fraction of minimum and maximum zoom

	void			cameraPanIn(const F32 meters);
	void			cameraPanLeft(const F32 meters);
	void			cameraPanUp(const F32 meters);

	void			updateFocusOffset();
	void			validateFocusObject();

	void			setUsingFollowCam( bool using_follow_cam);
	
	F32				calcCustomizeAvatarUIOffset( const LLVector3d& camera_pos_global );

	// marks current location as start, sends information to servers
	void			setStartPosition(U32 location_id);

	// Movement from user input.  All set the appropriate animation flags.
	// All turn off autopilot and make sure the camera is behind the avatar.
	// direction is either positive, zero, or negative
	void			moveAt(S32 direction);
	void			moveAtNudge(S32 direction);
	void			moveLeft(S32 direction);
	void			moveLeftNudge(S32 direction);
	void			moveUp(S32 direction);
	void			moveYaw(F32 mag);
	void			movePitch(S32 direction);

	void			setOrbitLeftKey(F32 mag)				{ mOrbitLeftKey = mag; }
	void			setOrbitRightKey(F32 mag)				{ mOrbitRightKey = mag; }
	void			setOrbitUpKey(F32 mag)					{ mOrbitUpKey = mag; }
	void			setOrbitDownKey(F32 mag)				{ mOrbitDownKey = mag; }
	void			setOrbitInKey(F32 mag)					{ mOrbitInKey = mag; }
	void			setOrbitOutKey(F32 mag)					{ mOrbitOutKey = mag; }

	void			setPanLeftKey(F32 mag)					{ mPanLeftKey = mag; }
	void			setPanRightKey(F32 mag)					{ mPanRightKey = mag; }
	void			setPanUpKey(F32 mag)					{ mPanUpKey = mag; }
	void			setPanDownKey(F32 mag)					{ mPanDownKey = mag; }
	void			setPanInKey(F32 mag)					{ mPanInKey = mag; }
	void			setPanOutKey(F32 mag)					{ mPanOutKey = mag; }

	U32 			getControlFlags(); 
	void 			setControlFlags(U32 mask); 			// performs bitwise mControlFlags |= mask
	void 			clearControlFlags(U32 mask); 			// performs bitwise mControlFlags &= mask
	BOOL			controlFlagsDirty() const;
	void			enableControlFlagReset();
	void 			resetControlFlags();

	void			propagate(const F32 dt);									// BUG: should roll into updateAgentPosition

	void			startAutoPilotGlobal(const LLVector3d &pos_global, const std::string& behavior_name = "", const LLQuaternion *target_rotation = NULL, 
									void (*finish_callback)(BOOL, void *) = NULL, void *callback_data = NULL, F32 stop_distance = 0.f, F32 rotation_threshold = 0.03f);

	void 			startFollowPilot(const LLUUID &leader_id);
	void			stopAutoPilot(BOOL user_cancel = FALSE);
	void 			setAutoPilotGlobal(const LLVector3d &pos_global);
	void			autoPilot(F32 *delta_yaw);			// autopilot walking action, angles in radians
	void			renderAutoPilotTarget();

	//
	// teportation methods
	//

	// go to a named location home
	void teleportRequest(
		const U64& region_handle,
		const LLVector3& pos_local);

	// teleport to a landmark
	void teleportViaLandmark(const LLUUID& landmark_id);

	// go home
	void teleportHome()	{ teleportViaLandmark(LLUUID::null); }

	// to an invited location
	void teleportViaLure(const LLUUID& lure_id, BOOL godlike);

	// to a global location - this will probably need to be
	// deprecated.
	void teleportViaLocation(const LLVector3d& pos_global); 

	// cancel the teleport, may or may not be allowed by server
	void teleportCancel();

	void 			setTargetVelocity(const LLVector3 &vel);
	const LLVector3	&getTargetVelocity() const;


	// Setting the ability for this avatar to proxy for another avatar.
	//static void processAddModifyAbility(LLMessageSystem* msg, void**);
	//static void processGrantedProxies(LLMessageSystem* msg, void**);
	//static void processRemoveModifyAbility(LLMessageSystem* msg, void**);
	//BOOL isProxyFor(const LLUUID& agent_id);// *FIX should be const

	static void		processAgentDataUpdate(LLMessageSystem *msg, void **);
	static void		processAgentGroupDataUpdate(LLMessageSystem *msg, void **);
	static void		processAgentDropGroup(LLMessageSystem *msg, void **);
	static void		processScriptControlChange(LLMessageSystem *msg, void **);
	static void		processAgentCachedTextureResponse(LLMessageSystem *mesgsys, void **user_data);
	//static void		processControlTake(LLMessageSystem *msg, void **);
	//static void		processControlRelease(LLMessageSystem *msg, void **);

	// This method checks to see if this agent can modify an object
	// based on the permissions and the agent's proxy status.
	BOOL			isGrantedProxy(const LLPermissions& perm);

	BOOL			allowOperation(PermissionBit op,
								   const LLPermissions& perm,
								   U64 group_proxy_power = 0,
								   U8 god_minimum = GOD_MAINTENANCE);

	friend std::ostream& operator<<(std::ostream &s, const LLAgent &sphere);

	void			initOriginGlobal(const LLVector3d &origin_global); // Only to be used in ONE place! - djs 08/07/02

	BOOL leftButtonGrabbed() const	{ return (  (!cameraMouselook() && mControlsTakenCount[CONTROL_LBUTTON_DOWN_INDEX] > 0) 
											  ||(cameraMouselook() && mControlsTakenCount[CONTROL_ML_LBUTTON_DOWN_INDEX] > 0)
											  ||(!cameraMouselook() && mControlsTakenPassedOnCount[CONTROL_LBUTTON_DOWN_INDEX] > 0)
											  ||(cameraMouselook() && mControlsTakenPassedOnCount[CONTROL_ML_LBUTTON_DOWN_INDEX] > 0)); }
	BOOL rotateGrabbed() const		{ return (  (mControlsTakenCount[CONTROL_YAW_POS_INDEX] > 0)
											  ||(mControlsTakenCount[CONTROL_YAW_NEG_INDEX] > 0)); }
	BOOL forwardGrabbed() const		{ return (	(mControlsTakenCount[CONTROL_AT_POS_INDEX] > 0)); }
	BOOL backwardGrabbed() const		{ return (	(mControlsTakenCount[CONTROL_AT_NEG_INDEX] > 0)); }
	BOOL upGrabbed() const		{ return (	(mControlsTakenCount[CONTROL_UP_POS_INDEX] > 0)); }
	BOOL downGrabbed() const	{ return (	(mControlsTakenCount[CONTROL_UP_NEG_INDEX] > 0)); }

	// True iff a script has taken over a control.
	BOOL			anyControlGrabbed() const;

	BOOL isControlGrabbed(S32 control_index) const;

	// Send message to simulator to force grabbed controls to be
	// released, in case of a poorly written script.
	void			forceReleaseControls();

	BOOL			sitCameraEnabled() { return mSitCameraEnabled; }

	F32				getCurrentCameraBuildOffset() { return (F32)mCameraFocusOffset.magVec(); }

	// look at behavior
	BOOL			setLookAt(ELookAtType target_type, LLViewerObject *object = NULL, LLVector3 position = LLVector3::zero);
	ELookAtType		getLookAtType();

	// point at behavior
	BOOL			setPointAt(EPointAtType target_type, LLViewerObject *object = NULL, LLVector3 position = LLVector3::zero);
	EPointAtType	getPointAtType();

	void			setHomePosRegion( const U64& region_handle, const LLVector3& pos_region );
	BOOL			getHomePosGlobal( LLVector3d* pos_global );
	void			setCameraAnimating( BOOL b )	{ mCameraAnimating = b; }
	void			setAnimationDuration( F32 seconds ) { mAnimationDuration = seconds; }

	F32				getNearChatRadius() { return mNearChatRadius; }

	enum ETeleportState
	{
		TELEPORT_NONE = 0,			// No teleport in progress
		TELEPORT_START = 1,			// Transition to REQUESTED.  Viewer has sent a TeleportRequest to the source simulator
		TELEPORT_REQUESTED = 2,		// Waiting for source simulator to respond
		TELEPORT_MOVING = 3,		// Viewer has received destination location from source simulator
		TELEPORT_START_ARRIVAL = 4,	// Transition to ARRIVING.  Viewer has received avatar update, etc., from destination simulator
		TELEPORT_ARRIVING = 5,		// Make the user wait while content "pre-caches"
		TELEPORT_CANCELLING = 6		// used only if user clicks "cancel" button
	};

	ETeleportState	getTeleportState() const			{ return mTeleportState; }
	void			setTeleportState( ETeleportState state );
	const LLString& getTeleportMessage() const { return mTeleportMessage; }
	void setTeleportMessage(const LLString& message)
	{
		mTeleportMessage = message;
	}

	// trigger random fidget animations
	void			fidget();

	void			requestEnterGodMode();
	void			requestLeaveGodMode();

	void			sendAgentSetAppearance();

	void 			sendAgentDataUpdateRequest();

	// Ventrella
	LLFollowCam mFollowCam;
	// end Ventrella 

	//--------------------------------------------------------------------
	// Wearables
	//--------------------------------------------------------------------
	BOOL			getWearablesLoaded() const	{ return mWearablesLoaded; }

	void			setWearable( LLInventoryItem* new_item, LLWearable* wearable );
	static void		onSetWearableDialog( S32 option, void* userdata );
	void			setWearableFinal( LLInventoryItem* new_item, LLWearable* new_wearable );
	void			setWearableOutfit( 	const LLInventoryItem::item_array_t& items, const LLDynamicArray< LLWearable* >& wearables, BOOL remove );
	void			queryWearableCache();

	BOOL			isWearableModifiable(EWearableType type);
	BOOL			isWearableCopyable(EWearableType type);
	BOOL			needsReplacement(EWearableType wearableType, S32 remove);
	U32				getWearablePermMask(EWearableType type);

	LLInventoryItem* getWearableInventoryItem(EWearableType type);

	LLWearable*		getWearable( EWearableType type ) { return (type < WT_COUNT) ? mWearableEntry[ type ].mWearable : NULL; }
	BOOL			isWearingItem( const LLUUID& item_id );
	LLWearable*		getWearableFromWearableItem( const LLUUID& item_id );
	const LLUUID&	getWearableItem( EWearableType type ) { return (type < WT_COUNT) ? mWearableEntry[ type ].mItemID : LLUUID::null; }

	static EWearableType getTEWearableType( S32 te );
	static LLUUID	getDefaultTEImageID( S32 te );
	
	void			copyWearableToInventory( EWearableType type );

	void			makeNewOutfit(
						const std::string& new_folder_name,
						const LLDynamicArray<S32>& wearables_to_include,
						const LLDynamicArray<S32>& attachments_to_include,
						BOOL rename_clothing);
	void			makeNewOutfitDone(S32 index);

	void			removeWearable( EWearableType type );
	static void		onRemoveWearableDialog( S32 option, void* userdata );
	void			removeWearableFinal( EWearableType type );
	
	void			sendAgentWearablesUpdate();

	/**
	 * @brief Only public because of addWearableToAgentInventoryCallback.
	 *
	 * NOTE: Do not call this method unless you are the inventory callback.
	 * NOTE: This can suffer from race conditions when working on the
	 * same values for index.
	 * @param index The index in mWearableEntry.
	 * @param item_id The inventory item id of the new wearable to wear.
	 * @param wearable The actual wearable data.
	 */
	void addWearabletoAgentInventoryDone(
		S32 index,
		const LLUUID& item_id,
		LLWearable* wearable);
	
	void			saveWearableAs( EWearableType type, const std::string& new_name, BOOL save_in_lost_and_found );
	void			saveWearable( EWearableType type, BOOL send_update = TRUE );
	void			saveAllWearables();
	
	void			revertWearable( EWearableType type );
	void			revertAllWearables();

	void			setWearableName( const LLUUID& item_id, const std::string& new_name );
	void			createStandardWearables(BOOL female);
	void			createStandardWearablesDone(S32 index);
	void			createStandardWearablesAllDone();

	BOOL			areWearablesLoaded() { return mWearablesLoaded; }

	void observeFriends();
	void friendsChanged();

	// statics
	static void		stopFidget();
	static void		processAgentInitialWearablesUpdate(LLMessageSystem* mesgsys, void** user_data);
	static void		userRemoveWearable( void* userdata );	// userdata is EWearableType
	static void		userRemoveAllClothes( void* userdata );	// userdata is NULL
	static void		userRemoveAllClothesStep2(BOOL proceed, void* userdata ); // userdata is NULL
	static void		userRemoveAllAttachments( void* userdata);	// userdata is NULL
	static BOOL		selfHasWearable( void* userdata );			// userdata is EWearableType

	//debug methods
	static void		clearVisualParams(void *);

protected:
	// stuff to do for any sort of teleport. Returns true if the
	// teleport can proceed.
	bool teleportCore(bool is_local = false);

	// helper function to prematurely age chat when agent is moving
	void ageChat();

	// internal wearable functions
	void			sendAgentWearablesRequest();
	static void		onInitialWearableAssetArrived(LLWearable* wearable, void* userdata);
	void			recoverMissingWearable(EWearableType type);
	void			recoverMissingWearableDone();
	void			addWearableToAgentInventory(LLPointer<LLInventoryCallback> cb,
						LLWearable* wearable, const LLUUID& category_id = LLUUID::null,
						BOOL notify = TRUE);
public:
	// TODO: Make these private!
	U32				mViewerPort;				// Port this agent transmits on.
	LLUUID			mSecureSessionID;			// secure token for this login session

	F32				mDrawDistance;

	// Access or "maturity" level
	U8				mAccess;	// SIM_ACCESS_MATURE or SIM_ACCESS_PG
	U64				mGroupPowers;
	BOOL			mHideGroupTitle;
	char			mGroupTitle[DB_GROUP_TITLE_BUF_SIZE];	/*Flawfinder: ignore*/	// honorific, like "Sir"
	char			mGroupName[DB_GROUP_NAME_BUF_SIZE];	/*Flawfinder: ignore*/
	LLUUID			mGroupID;
	//LLUUID			mGroupInsigniaID;
	LLUUID			mInventoryRootID;
	LLUUID			mMapID;
	F64				mMapOriginX;	// Global x coord of mMapID's bottom left corner.
	F64				mMapOriginY;	// Global y coord of mMapID's bottom left corner.
	S32				mMapWidth;	// Width of map in meters
	S32				mMapHeight;	// Height of map in meters
	std::string		mMOTD;	// message of the day

	LLPointer<LLHUDEffectLookAt> mLookAt;
	LLPointer<LLHUDEffectPointAt> mPointAt;

	LLDynamicArray<LLGroupData> mGroups;

	BOOL			mInitialized;

	static BOOL		sDebugDisplayTarget;
	S32				mNumPendingQueries;
	S32*			mActiveCacheQueries;

	BOOL			mForceMouselook;

	static void parseTeleportMessages(const LLString& xml_filename);
	//we should really define ERROR and PROGRESS enums here
	//but I don't really feel like doing that, so I am just going
	//to expose the mappings....yup
	static std::map<LLString, LLString> sTeleportErrorMessages;
	static std::map<LLString, LLString> sTeleportProgressMessages;

private:
	ETeleportState	mTeleportState;
	LLString		mTeleportMessage;

	S32				mControlsTakenCount[TOTAL_CONTROLS];
	S32				mControlsTakenPassedOnCount[TOTAL_CONTROLS];

	LLViewerRegion	*mRegionp;
	LLVector3d		mAgentOriginGlobal;				// Origin of agent coords from global coords
	mutable LLVector3d mPositionGlobal;

	std::set<U64>	mRegionsVisited;				// stat - what distinct regions has the avatar been to?
	F64				mDistanceTraveled;				// stat - how far has the avatar moved?
	LLVector3d		mLastPositionGlobal;			// Used to calculate travel distance

	LLPointer<LLVOAvatar> mAvatarObject;			// NULL until avatar object sent down from simulator

	U8				mRenderState;					// Current behavior state of agent
	LLFrameTimer	mTypingTimer;

	ECameraMode		mCameraMode;					// target mode after transition animation is done
	ECameraMode		mLastCameraMode;
	BOOL			mViewsPushed;					// keep track of whether or not we have pushed views.

	BOOL			mbAlwaysRun;					// should the avatar run rather than walk
	BOOL			mShowAvatar;					// should we render the avatar?
	BOOL			mCameraAnimating;				// camera is transitioning from one mode to another
	LLVector3d		mAnimationCameraStartGlobal;	// camera start position, global coords
	LLVector3d		mAnimationFocusStartGlobal;		// camera focus point, global coords
	LLFrameTimer	mAnimationTimer;				// seconds that transition animation has been active
	F32				mAnimationDuration;				// seconds
	F32				mCameraFOVZoomFactor;			// amount of fov zoom applied to camera when zeroing in on an object
	F32				mCameraCurrentFOVZoomFactor;	// interpolated fov zoom
	F32				mCameraFOVDefault;				// default field of view that is basis for FOV zoom effect
	LLVector3d		mCameraFocusOffset;				// offset from focus point in build mode
	LLVector3d		mCameraFocusOffsetTarget;		// target towards which we are lerping the camera's focus offset
	LLVector3		mCameraOffsetDefault;			// default third-person camera offset
	LLVector4		mCameraCollidePlane;			// colliding plane for camera
	F32				mCurrentCameraDistance;			// current camera offset from avatar
	F32				mTargetCameraDistance;			// target camera offset from avatar
	F32				mCameraZoomFraction;			// mousewheel driven fraction of zoom
	LLVector3		mCameraLag;						// third person camera lag
	LLVector3		mThirdPersonHeadOffset;			// head offset for third person camera position
	LLVector3		mCameraPositionAgent;			// camera position in agent coordinates
	LLVector3		mCameraVirtualPositionAgent;	// camera virtual position (target) before performing FOV zoom
	BOOL			mSitCameraEnabled;				// use provided camera information when sitting?
	LLVector3		mSitCameraPos;					// root relative camera pos when sitting
	LLVector3		mSitCameraFocus;				// root relative camera target when sitting

	//Ventrella
	LLVector3		mCameraUpVector;				// camera's up direction in world coordinates (determines the 'roll' of the view)
	//End Ventrella

	LLPointer<LLViewerObject> mSitCameraReferenceObject;	// object to which camera is related when sitting

	BOOL			mFocusOnAvatar;					
	LLVector3d		mFocusGlobal;
	LLVector3d		mFocusTargetGlobal;
	LLPointer<LLViewerObject>	mFocusObject;
	F32				mFocusObjectDist;
	LLVector3		mFocusObjectOffset;
	F32				mFocusDotRadius;				// meters
	BOOL			mTrackFocusObject;
	F32				mUIOffset;

	LLCoordFrame	mFrameAgent;					// Agent position and view, agent-region coordinates

	BOOL			mCrouching;
	BOOL			mIsBusy;

	S32 			mAtKey;							// Either 1, 0, or -1... indicates that movement-key is pressed
	S32				mWalkKey;						// like AtKey, but causes less forward thrust
	S32 			mLeftKey;
	S32				mUpKey;
	F32				mYawKey;
	S32				mPitchKey;

	F32				mOrbitLeftKey;
	F32				mOrbitRightKey;
	F32				mOrbitUpKey;
	F32				mOrbitDownKey;
	F32				mOrbitInKey;
	F32				mOrbitOutKey;

	F32				mPanUpKey;						
	F32				mPanDownKey;					
	F32				mPanLeftKey;					
	F32				mPanRightKey;					
	F32				mPanInKey;
	F32				mPanOutKey;

	U32				mControlFlags;					// replacement for the mFooKey's
	BOOL 			mbFlagsDirty;
	BOOL 			mbFlagsNeedReset;				// HACK for preventing incorrect flags sent when crossing region boundaries

	BOOL 			mbJump;

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

	std::set<LLUUID> mProxyForAgents;

	LLColor4 mEffectColor;

	BOOL mHaveHomePosition;
	U64				mHomeRegionHandle;
	LLVector3		mHomePosRegion;
	LLFrameTimer	mChatTimer;
	LLUUID			mLastChatterID;
	F32				mNearChatRadius;
	BOOL			mAdminOverride;

	// See indra_constants.h for values.
	U8				mGodLevel;
	LLFrameTimer	mFidgetTimer;
	LLFrameTimer	mFocusObjectFadeTimer;
	std::set<LLPointer <LLViewerObject> > mFadeObjects;
	F32				mNextFidgetTime;
	S32				mCurrentFidget;
	BOOL			mFirstLogin;
	BOOL			mGenderChosen;
	
	//--------------------------------------------------------------------
	// Wearables
	//--------------------------------------------------------------------
	struct LLWearableEntry
	{
		LLWearableEntry() : mItemID( LLUUID::null ), mWearable( NULL ) {}

		LLUUID		mItemID;	// ID of the inventory item in the agent's inventory.
		LLWearable*	mWearable;
	};
	LLWearableEntry mWearableEntry[ WT_COUNT ];
	U32				mAgentWearablesUpdateSerialNum;
	BOOL			mWearablesLoaded;
	S32				mTextureCacheQueryID;
	U32				mAppearanceSerialNum;
	LLAnimPauseRequest mPauseRequest;

	class createStandardWearablesAllDoneCallback : public LLRefCount
	{
	public:
		~createStandardWearablesAllDoneCallback();
	};
	class sendAgentWearablesUpdateCallback : public LLRefCount
	{
	public:
		~sendAgentWearablesUpdateCallback();
	};

	class addWearableToAgentInventoryCallback : public LLInventoryCallback
	{
	public:
		enum {
			CALL_NONE = 0,
			CALL_UPDATE = 1,
			CALL_RECOVERDONE = 2,
			CALL_CREATESTANDARDDONE = 4,
			CALL_MAKENEWOUTFITDONE = 8
		} EType;

		/**
		 * @brief Construct a callback for dealing with the wearables.
		 *
		 * Would like to pass the agent in here, but we can't safely
		 * count on it being around later.  Just use gAgent directly.
		 * @param cb callback to execute on completion (??? unused ???)
		 * @param index Index for the wearable in the agent
		 * @param wearable The wearable data.
		 * @param todo Bitmask of actions to take on completion.
		 */
		addWearableToAgentInventoryCallback(
			LLPointer<LLRefCount> cb,
			S32 index,
			LLWearable* wearable,
			U32 todo = CALL_NONE);
		~addWearableToAgentInventoryCallback() {};
		virtual void fire(const LLUUID& inv_item);

	private:
		S32 mIndex;
		LLWearable* mWearable;
		U32 mTodo;
		LLPointer<LLRefCount> mCB;
	};

	//control listeners
	class LLHideGroupTitleListener: public LLSimpleListener
	{
	public:
		bool handleEvent(LLPointer<LLEvent> event, const LLSD &userdata);
	};

	class LLEffectColorListener: public LLSimpleListener
	{
	public:
		bool handleEvent(LLPointer<LLEvent> event, const LLSD &userdata);
	};

	LLHideGroupTitleListener mHideGroupTitleListener;
	LLEffectColorListener mEffectColorListener;
	LLFriendObserver* mFriendObserver;
};

extern LLAgent gAgent;

#endif
