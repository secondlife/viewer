/** 
 * @file llagent.cpp
 * @brief LLAgent class implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "stdtypes.h"
#include "stdenums.h"

#include "llagent.h" 

#include "llcoordframe.h"
#include "indra_constants.h"
#include "llmath.h"
#include "llcriticaldamp.h"
#include "llfocusmgr.h"
#include "llglheaders.h"
#include "llparcel.h"
#include "llpermissions.h"
#include "llregionhandle.h"
#include "m3math.h"
#include "m4math.h"
#include "message.h"
#include "llquaternion.h"
#include "v3math.h"
#include "v4math.h"
#include "llsmoothstep.h"
#include "llsdutil.h"
//#include "vmath.h"

#include "imageids.h"
#include "llbox.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llchatbar.h"
#include "llconsole.h"
#include "lldrawable.h"
#include "llface.h"
#include "llfirstuse.h"
#include "llfloater.h"
#include "llfloateractivespeakers.h"
#include "llfloateravatarinfo.h"
#include "llfloaterbuildoptions.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfloaterdirectory.h"
#include "llfloatergroupinfo.h"
#include "llfloatergroups.h"
#include "llfloatermap.h"
#include "llfloatermute.h"
#include "llfloatersnapshot.h"
#include "llfloatertools.h"
#include "llfloaterworldmap.h"
#include "llgroupmgr.h"
#include "llhudeffectlookat.h"
#include "llhudmanager.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "lljoystickbutton.h"
#include "llmenugl.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llnotify.h"
#include "llquantize.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llrendersphere.h"
#include "llstatusbar.h"
#include "llimview.h"
#include "lltool.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolview.h"
#include "llui.h"			// for make_ui_sound
#include "llurldispatcher.h"
#include "llviewercamera.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llviewerdisplay.h"
#include "llvoavatar.h"
#include "llvoground.h"
#include "llvosky.h"
#include "llwearable.h"
#include "llwearablelist.h"
#include "llworld.h"
#include "llworldmap.h"
#include "pipeline.h"
#include "roles_constants.h"
#include "llviewercontrol.h"
#include "llappviewer.h"
#include "llvoiceclient.h"

// Ventrella
#include "llfollowcam.h"
// end Ventrella

extern LLMenuBarGL* gMenuBarView;
extern U8 gLastPickAlpha;

//drone wandering constants
const F32 MAX_WANDER_TIME = 20.f;						// seconds
const F32 MAX_HEADING_HALF_ERROR = 0.2f;				// radians
const F32 WANDER_MAX_SLEW_RATE = 2.f * DEG_TO_RAD;		// radians / frame
const F32 WANDER_TARGET_MIN_DISTANCE = 10.f;			// meters

// Autopilot constants
const F32 AUTOPILOT_HEADING_HALF_ERROR = 10.f * DEG_TO_RAD;	// radians
const F32 AUTOPILOT_MAX_SLEW_RATE = 1.f * DEG_TO_RAD;		// radians / frame
const F32 AUTOPILOT_STOP_DISTANCE = 2.f;					// meters
const F32 AUTOPILOT_HEIGHT_ADJUST_DISTANCE = 8.f;			// meters
const F32 AUTOPILOT_MIN_TARGET_HEIGHT_OFF_GROUND = 1.f;	// meters
const F32 AUTOPILOT_MAX_TIME_NO_PROGRESS = 1.5f;		// seconds

// face editing constants
const LLVector3d FACE_EDIT_CAMERA_OFFSET(0.4f, -0.05f, 0.07f);
const LLVector3d FACE_EDIT_TARGET_OFFSET(0.f, 0.f, 0.05f);

// Mousewheel camera zoom
const F32 MIN_ZOOM_FRACTION = 0.25f;
const F32 INITIAL_ZOOM_FRACTION = 1.f;
const F32 MAX_ZOOM_FRACTION = 8.f;
const F32 METERS_PER_WHEEL_CLICK = 1.f;

const F32 MAX_TIME_DELTA = 1.f;

const F32 CAMERA_ZOOM_HALF_LIFE = 0.07f;	// seconds
const F32 FOV_ZOOM_HALF_LIFE = 0.07f;	// seconds

const F32 CAMERA_FOCUS_HALF_LIFE = 0.f;//0.02f;
const F32 CAMERA_LAG_HALF_LIFE = 0.25f;
const F32 MIN_CAMERA_LAG = 0.5f;
const F32 MAX_CAMERA_LAG = 5.f;

const F32 CAMERA_COLLIDE_EPSILON = 0.1f;
const F32 MIN_CAMERA_DISTANCE = 0.1f;
const F32 AVATAR_ZOOM_MIN_X_FACTOR = 0.55f;
const F32 AVATAR_ZOOM_MIN_Y_FACTOR = 0.7f;
const F32 AVATAR_ZOOM_MIN_Z_FACTOR = 1.15f;

const F32 MAX_CAMERA_DISTANCE_FROM_AGENT = 50.f;

const F32 MAX_CAMERA_SMOOTH_DISTANCE = 20.0f;

const F32 HEAD_BUFFER_SIZE = 0.3f;
const F32 CUSTOMIZE_AVATAR_CAMERA_ANIM_SLOP = 0.2f;

const F32 LAND_MIN_ZOOM = 0.15f;
const F32 AVATAR_MIN_ZOOM = 0.5f;
const F32 OBJECT_MIN_ZOOM = 0.02f;

const F32 APPEARANCE_MIN_ZOOM = 0.39f;
const F32 APPEARANCE_MAX_ZOOM = 8.f;

// fidget constants
const F32 MIN_FIDGET_TIME = 8.f; // seconds
const F32 MAX_FIDGET_TIME = 20.f; // seconds

const S32 MAX_NUM_CHAT_POSITIONS = 10;
const F32 GROUND_TO_AIR_CAMERA_TRANSITION_TIME = 0.5f;
const F32 GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME = 0.5f;

const F32 MAX_VELOCITY_AUTO_LAND_SQUARED = 4.f * 4.f;

const F32 MAX_FOCUS_OFFSET = 20.f;

const F32 OBJECT_EXTENTS_PADDING = 0.5f;

const F32 MIN_RADIUS_ALPHA_SIZZLE = 0.5f;

const F64 CHAT_AGE_FAST_RATE = 3.0;

const S32 MAX_WEARABLES_PER_LAYERSET = 7;

const EWearableType WEARABLE_BAKE_TEXTURE_MAP[BAKED_TEXTURE_COUNT][MAX_WEARABLES_PER_LAYERSET] = 
{
	{ WT_SHAPE,	WT_SKIN,	WT_HAIR,	WT_INVALID,	WT_INVALID,	WT_INVALID,		WT_INVALID    },	// TEX_HEAD_BAKED
	{ WT_SHAPE, WT_SKIN,	WT_SHIRT,	WT_JACKET,	WT_GLOVES,	WT_UNDERSHIRT,	WT_INVALID	  },	// TEX_UPPER_BAKED
	{ WT_SHAPE, WT_SKIN,	WT_PANTS,	WT_SHOES,	WT_SOCKS,	WT_JACKET,		WT_UNDERPANTS },	// TEX_LOWER_BAKED
	{ WT_EYES,	WT_INVALID,	WT_INVALID,	WT_INVALID,	WT_INVALID,	WT_INVALID,		WT_INVALID    },	// TEX_EYES_BAKED
	{ WT_SKIRT,	WT_INVALID,	WT_INVALID,	WT_INVALID,	WT_INVALID,	WT_INVALID,		WT_INVALID    }		// TEX_SKIRT_BAKED
};

const LLUUID BAKED_TEXTURE_HASH[BAKED_TEXTURE_COUNT] = 
{
	LLUUID("18ded8d6-bcfc-e415-8539-944c0f5ea7a6"),
	LLUUID("338c29e3-3024-4dbb-998d-7c04cf4fa88f"),
	LLUUID("91b4a2c7-1b1a-ba16-9a16-1f8f8dcc1c3f"),
	LLUUID("b2cf28af-b840-1071-3c6a-78085d8128b5"),
	LLUUID("ea800387-ea1a-14e0-56cb-24f2022f969a")
};

// The agent instance.
LLAgent gAgent;

//
// Statics
//
BOOL LLAgent::sDebugDisplayTarget = FALSE;

const F32 LLAgent::TYPING_TIMEOUT_SECS = 5.f;

std::map<LLString, LLString> LLAgent::sTeleportErrorMessages;
std::map<LLString, LLString> LLAgent::sTeleportProgressMessages;

class LLAgentFriendObserver : public LLFriendObserver
{
public:
	LLAgentFriendObserver() {}
	virtual ~LLAgentFriendObserver() {}
	virtual void changed(U32 mask);
};

void LLAgentFriendObserver::changed(U32 mask)
{
	// if there's a change we're interested in.
	if((mask & (LLFriendObserver::POWERS)) != 0)
	{
		gAgent.friendsChanged();
	}
}

// ************************************************************
// Enabled this definition to compile a 'hacked' viewer that
// locally believes the end user has godlike powers.
// #define HACKED_GODLIKE_VIEWER
// For a toggled version, see viewer.h for the
// TOGGLE_HACKED_GODLIKE_VIEWER define, instead.
// ************************************************************

// Constructors and Destructors

// JC - Please try to make this order match the order in the header
// file.  Otherwise it's hard to find variables that aren't initialized.
//-----------------------------------------------------------------------------
// LLAgent()
//-----------------------------------------------------------------------------
LLAgent::LLAgent()
:	mDrawDistance( DEFAULT_FAR_PLANE ),

	mDoubleTapRunTimer(),
	mDoubleTapRunMode(DOUBLETAP_NONE),

	mbAlwaysRun(false),
	mbRunning(false),

	mAccess(SIM_ACCESS_PG),
	mGroupPowers(0),
	mGroupID(),
	//mGroupInsigniaID(),
	mMapOriginX(0),
	mMapOriginY(0),
	mMapWidth(0),
	mMapHeight(0),
	mLookAt(NULL),
	mPointAt(NULL),
	mInitialized(FALSE),
	mNumPendingQueries(0),
	mForceMouselook(FALSE),
	mTeleportState( TELEPORT_NONE ),
	mRegionp(NULL),

	mAgentOriginGlobal(),
	mPositionGlobal(),

	mDistanceTraveled(0),
	mLastPositionGlobal(LLVector3d::zero),

	mAvatarObject(NULL),

	mRenderState(0),
	mTypingTimer(),

	mCameraMode( CAMERA_MODE_THIRD_PERSON ),
	mLastCameraMode( CAMERA_MODE_THIRD_PERSON ),
	mViewsPushed(FALSE),

	mShowAvatar(TRUE),
	
	mCameraAnimating( FALSE ),
	mAnimationCameraStartGlobal(),
	mAnimationFocusStartGlobal(),
	mAnimationTimer(),
	mAnimationDuration(0.33f),
	mCameraFOVZoomFactor(0.f),
	mCameraCurrentFOVZoomFactor(0.f),
	mCameraFocusOffset(),
	mCameraOffsetDefault(),
//	mCameraOffsetNorm(),
	mCameraCollidePlane(),
	mCurrentCameraDistance(2.f),		// meters, set in init()
	mTargetCameraDistance(2.f),
	mCameraZoomFraction(1.f),			// deprecated
	mThirdPersonHeadOffset(0.f, 0.f, 1.f),
	mSitCameraEnabled(FALSE),
	mFocusOnAvatar(TRUE),
	mFocusGlobal(),
	mFocusTargetGlobal(),
	mFocusObject(NULL),
	mFocusObjectOffset(),
	mFocusDotRadius( 0.1f ),			// meters
	mTrackFocusObject(TRUE),
	mCameraSmoothingLastPositionGlobal(),
	mCameraSmoothingLastPositionAgent(),

	mFrameAgent(),

	mCrouching(FALSE),
	mIsBusy(FALSE),

	// movement keys below

	mControlFlags(0x00000000),
	mbFlagsDirty(FALSE),
	mbFlagsNeedReset(FALSE),

	mbJump(FALSE),

	mAutoPilot(FALSE),
	mAutoPilotFlyOnStop(FALSE),
	mAutoPilotTargetGlobal(),
	mAutoPilotStopDistance(1.f),
	mAutoPilotUseRotation(FALSE),
	mAutoPilotTargetFacing(LLVector3::zero),
	mAutoPilotTargetDist(0.f),
	mAutoPilotFinishedCallback(NULL),
	mAutoPilotCallbackData(NULL),
	

	mEffectColor(0.f, 1.f, 1.f, 1.f),
	mHaveHomePosition(FALSE),
	mHomeRegionHandle( 0 ),
	mNearChatRadius(CHAT_NORMAL_RADIUS / 2.f),
	mGodLevel( GOD_NOT ),


	mNextFidgetTime(0.f),
	mCurrentFidget(0),
	mFirstLogin(FALSE),
	mGenderChosen(FALSE),
	mAgentWearablesUpdateSerialNum(0),
	mWearablesLoaded(FALSE),
	mTextureCacheQueryID(0),
	mAppearanceSerialNum(0)
{

	U32 i;
	for (i = 0; i < TOTAL_CONTROLS; i++)
	{
		mControlsTakenCount[i] = 0;
		mControlsTakenPassedOnCount[i] = 0;
	}

	// Initialize movement keys
	mAtKey				= 0;	// Either 1, 0, or -1... indicates that movement-key is pressed
	mWalkKey			= 0;	// like AtKey, but causes less forward thrust
	mLeftKey			= 0;
	mUpKey				= 0;
	mYawKey				= 0.f;
	mPitchKey			= 0;

	mOrbitLeftKey		= 0.f;
	mOrbitRightKey		= 0.f;
	mOrbitUpKey			= 0.f;
	mOrbitDownKey		= 0.f;
	mOrbitInKey			= 0.f;
	mOrbitOutKey		= 0.f;

	mPanUpKey			= 0.f;
	mPanDownKey			= 0.f;
	mPanLeftKey			= 0.f;
	mPanRightKey		= 0.f;
	mPanInKey			= 0.f;
	mPanOutKey			= 0.f;

	mActiveCacheQueries = new S32[BAKED_TEXTURE_COUNT];
	for (i = 0; i < (U32)BAKED_TEXTURE_COUNT; i++)
	{
		mActiveCacheQueries[i] = 0;
	}

	//Ventrella
	mCameraUpVector = LLVector3::z_axis;// default is straight up 
	mFollowCam.setMaxCameraDistantFromSubject( MAX_CAMERA_DISTANCE_FROM_AGENT );
	//end ventrella

	mCustomAnim = FALSE ;
}

// Requires gSavedSettings to be initialized.
//-----------------------------------------------------------------------------
// init()
//-----------------------------------------------------------------------------
void LLAgent::init()
{
	mDrawDistance = gSavedSettings.getF32("RenderFarClip");

	// *Note: this is where LLViewerCamera::getInstance() used to be constructed.

	LLViewerCamera::getInstance()->setView(DEFAULT_FIELD_OF_VIEW);
	// Leave at 0.1 meters until we have real near clip management
	LLViewerCamera::getInstance()->setNear(0.1f);
	LLViewerCamera::getInstance()->setFar(mDrawDistance);			// if you want to change camera settings, do so in camera.h
	LLViewerCamera::getInstance()->setAspect( gViewerWindow->getDisplayAspectRatio() );		// default, overridden in LLViewerWindow::reshape
	LLViewerCamera::getInstance()->setViewHeightInPixels(768);			// default, overridden in LLViewerWindow::reshape

	setFlying( gSavedSettings.getBOOL("FlyingAtExit") );

	mCameraFocusOffsetTarget = LLVector4(gSavedSettings.getVector3("CameraOffsetBuild"));
	mCameraOffsetDefault = gSavedSettings.getVector3("CameraOffsetDefault");
//	mCameraOffsetNorm = mCameraOffsetDefault;
//	mCameraOffsetNorm.normVec();
	mCameraCollidePlane.clearVec();
	mCurrentCameraDistance = mCameraOffsetDefault.magVec();
	mTargetCameraDistance = mCurrentCameraDistance;
	mCameraZoomFraction = 1.f;
	mTrackFocusObject = gSavedSettings.getBOOL("TrackFocusObject");

//	LLDebugVarMessageBox::show("Camera Lag", &CAMERA_FOCUS_HALF_LIFE, 0.5f, 0.01f);

	mEffectColor = gSavedSettings.getColor4("EffectColor");
	
	mInitialized = TRUE;
}

//-----------------------------------------------------------------------------
// cleanup()
//-----------------------------------------------------------------------------
void LLAgent::cleanup()
{
	setSitCamera(LLUUID::null);
	mAvatarObject = NULL;
	mLookAt = NULL;
	mPointAt = NULL;
	mRegionp = NULL;
	setFocusObject(NULL);
}

//-----------------------------------------------------------------------------
// LLAgent()
//-----------------------------------------------------------------------------
LLAgent::~LLAgent()
{
	cleanup();

	delete [] mActiveCacheQueries;
	mActiveCacheQueries = NULL;

	// *Note: this is where LLViewerCamera::getInstance() used to be deleted.
}

// Change camera back to third person, stop the autopilot,
// deselect stuff, etc.
//-----------------------------------------------------------------------------
// resetView()
//-----------------------------------------------------------------------------
void LLAgent::resetView(BOOL reset_camera)
{
	if (mAutoPilot)
	{
		stopAutoPilot(TRUE);
	}

	if (!gNoRender)
	{
		LLSelectMgr::getInstance()->unhighlightAll();

		// By popular request, keep land selection while walking around. JC
		// LLViewerParcelMgr::getInstance()->deselectLand();

		// force deselect when walking and attachment is selected
		// this is so people don't wig out when their avatar moves without animating
		if (LLSelectMgr::getInstance()->getSelection()->isAttachment())
		{
			LLSelectMgr::getInstance()->deselectAll();
		}

		// Hide all popup menus
		gMenuHolder->hideMenus();
	}

	if (reset_camera && !gSavedSettings.getBOOL("FreezeTime"))
	{
		if (!gViewerWindow->getLeftMouseDown() && cameraThirdPerson())
		{
			// leaving mouse-steer mode
			LLVector3 agent_at_axis = getAtAxis();
			agent_at_axis -= projected_vec(agent_at_axis, getReferenceUpVector());
			agent_at_axis.normVec();
			gAgent.resetAxes(lerp(getAtAxis(), agent_at_axis, LLCriticalDamp::getInterpolant(0.3f)));
		}

		setFocusOnAvatar(TRUE, ANIMATE);
	}

	if (mAvatarObject.notNull())
	{
		mAvatarObject->mHUDTargetZoom = 1.f;
	}
}

// Handle any actions that need to be performed when the main app gains focus
// (such as through alt-tab).
//-----------------------------------------------------------------------------
// onAppFocusGained()
//-----------------------------------------------------------------------------
void LLAgent::onAppFocusGained()
{
	if (CAMERA_MODE_MOUSELOOK == mCameraMode)
	{
		changeCameraToDefault();
		LLToolMgr::getInstance()->clearSavedTool();
	}
}


void LLAgent::ageChat()
{
	if (mAvatarObject)
	{
		// get amount of time since I last chatted
		F64 elapsed_time = (F64)mAvatarObject->mChatTimer.getElapsedTimeF32();
		// add in frame time * 3 (so it ages 4x)
		mAvatarObject->mChatTimer.setAge(elapsed_time + (F64)gFrameDTClamped * (CHAT_AGE_FAST_RATE - 1.0));
	}
}

// Allow camera to be moved somewhere other than behind avatar.
//-----------------------------------------------------------------------------
// unlockView()
//-----------------------------------------------------------------------------
void LLAgent::unlockView()
{
	if (getFocusOnAvatar())
	{
		if (mAvatarObject)
		{
			setFocusGlobal( LLVector3d::zero, mAvatarObject->mID );
		}
		setFocusOnAvatar(FALSE, FALSE);	// no animation
	}
}


//-----------------------------------------------------------------------------
// moveAt()
//-----------------------------------------------------------------------------
void LLAgent::moveAt(S32 direction, bool reset)
{
	// age chat timer so it fades more quickly when you are intentionally moving
	ageChat();

	setKey(direction, mAtKey);

	if (direction > 0)
	{
		setControlFlags(AGENT_CONTROL_AT_POS | AGENT_CONTROL_FAST_AT);
	}
	else if (direction < 0)
	{
		setControlFlags(AGENT_CONTROL_AT_NEG | AGENT_CONTROL_FAST_AT);
	}

	if (reset)
	{
		resetView();
	}
}

//-----------------------------------------------------------------------------
// moveAtNudge()
//-----------------------------------------------------------------------------
void LLAgent::moveAtNudge(S32 direction)
{
	// age chat timer so it fades more quickly when you are intentionally moving
	ageChat();

	setKey(direction, mWalkKey);

	if (direction > 0)
	{
		setControlFlags(AGENT_CONTROL_NUDGE_AT_POS);
	}
	else if (direction < 0)
	{
		setControlFlags(AGENT_CONTROL_NUDGE_AT_NEG);
	}

	resetView();
}

//-----------------------------------------------------------------------------
// moveLeft()
//-----------------------------------------------------------------------------
void LLAgent::moveLeft(S32 direction)
{
	// age chat timer so it fades more quickly when you are intentionally moving
	ageChat();

	setKey(direction, mLeftKey);

	if (direction > 0)
	{
		setControlFlags(AGENT_CONTROL_LEFT_POS | AGENT_CONTROL_FAST_LEFT);
	}
	else if (direction < 0)
	{
		setControlFlags(AGENT_CONTROL_LEFT_NEG | AGENT_CONTROL_FAST_LEFT);
	}

	resetView();
}

//-----------------------------------------------------------------------------
// moveLeftNudge()
//-----------------------------------------------------------------------------
void LLAgent::moveLeftNudge(S32 direction)
{
	// age chat timer so it fades more quickly when you are intentionally moving
	ageChat();

	setKey(direction, mLeftKey);

	if (direction > 0)
	{
		setControlFlags(AGENT_CONTROL_NUDGE_LEFT_POS);
	}
	else if (direction < 0)
	{
		setControlFlags(AGENT_CONTROL_NUDGE_LEFT_NEG);
	}

	resetView();
}

//-----------------------------------------------------------------------------
// moveUp()
//-----------------------------------------------------------------------------
void LLAgent::moveUp(S32 direction)
{
	// age chat timer so it fades more quickly when you are intentionally moving
	ageChat();

	setKey(direction, mUpKey);

	if (direction > 0)
	{
		setControlFlags(AGENT_CONTROL_UP_POS | AGENT_CONTROL_FAST_UP);
	}
	else if (direction < 0)
	{
		setControlFlags(AGENT_CONTROL_UP_NEG | AGENT_CONTROL_FAST_UP);
	}

	resetView();
}

//-----------------------------------------------------------------------------
// moveYaw()
//-----------------------------------------------------------------------------
void LLAgent::moveYaw(F32 mag, bool reset_view)
{
	mYawKey = mag;

	if (mag > 0)
	{
		setControlFlags(AGENT_CONTROL_YAW_POS);
	}
	else if (mag < 0)
	{
		setControlFlags(AGENT_CONTROL_YAW_NEG);
	}

    if (reset_view)
	{
        resetView();
	}
}

//-----------------------------------------------------------------------------
// movePitch()
//-----------------------------------------------------------------------------
void LLAgent::movePitch(S32 direction)
{
	setKey(direction, mPitchKey);

	if (direction > 0)
	{
		setControlFlags(AGENT_CONTROL_PITCH_POS );
	}
	else if (direction < 0)
	{
		setControlFlags(AGENT_CONTROL_PITCH_NEG);
	}
}


// Does this parcel allow you to fly?
BOOL LLAgent::canFly()
{
	if (isGodlike()) return TRUE;

	LLViewerRegion* regionp = getRegion();
	if (regionp && regionp->getBlockFly()) return FALSE;
	
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	if (!parcel) return FALSE;

	// Allow owners to fly on their own land.
	if (LLViewerParcelMgr::isParcelOwnedByAgent(parcel, GP_LAND_ALLOW_FLY))
	{
		return TRUE;
	}

	return parcel->getAllowFly();
}


//-----------------------------------------------------------------------------
// setFlying()
//-----------------------------------------------------------------------------
void LLAgent::setFlying(BOOL fly)
{
	if (mAvatarObject.notNull())
	{
		if(mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_STANDUP) != mAvatarObject->mSignaledAnimations.end())
		{
			return;
		}

		// don't allow taking off while sitting
		if (fly && mAvatarObject->mIsSitting)
		{
			return;
		}
	}

	if (fly)
	{
		BOOL was_flying = getFlying();
		if (!canFly() && !was_flying)
		{
			// parcel doesn't let you start fly
			// gods can always fly
			// and it's OK if you're already flying
			make_ui_sound("UISndBadKeystroke");
			return;
		}
		if( !was_flying )
		{
			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_FLY_COUNT);
		}
		setControlFlags(AGENT_CONTROL_FLY);
		gSavedSettings.setBOOL("FlyBtnState", TRUE);
	}
	else
	{
		clearControlFlags(AGENT_CONTROL_FLY);
		gSavedSettings.setBOOL("FlyBtnState", FALSE);
	}
	mbFlagsDirty = TRUE;
}


// UI based mechanism of setting fly state
//-----------------------------------------------------------------------------
// toggleFlying()
//-----------------------------------------------------------------------------
void LLAgent::toggleFlying()
{
	BOOL fly = !(mControlFlags & AGENT_CONTROL_FLY);

	setFlying( fly );
	resetView();
}


//-----------------------------------------------------------------------------
// setRegion()
//-----------------------------------------------------------------------------
void LLAgent::setRegion(LLViewerRegion *regionp)
{
	llassert(regionp);
	if (mRegionp != regionp)
	{
		// JC - Avoid this, causes out-of-bounds array write deep within
		// Windows.
		// char host_name[MAX_STRING];
		// regionp->getHost().getHostName(host_name, MAX_STRING);

		char ip[MAX_STRING];		/*Flawfinder: ignore*/
		regionp->getHost().getString(ip, MAX_STRING);
		llinfos << "Moving agent into region: " << regionp->getName()
				<< " located at " << ip << llendl;
		if (mRegionp)
		{
			// We've changed regions, we're now going to change our agent coordinate frame.
			mAgentOriginGlobal = regionp->getOriginGlobal();
			LLVector3d agent_offset_global = mRegionp->getOriginGlobal();

			LLVector3 delta;
			delta.setVec(regionp->getOriginGlobal() - mRegionp->getOriginGlobal());

			setPositionAgent(getPositionAgent() - delta);

			LLVector3 camera_position_agent = LLViewerCamera::getInstance()->getOrigin();
			LLViewerCamera::getInstance()->setOrigin(camera_position_agent - delta);

			// Update all of the regions.
			LLWorld::getInstance()->updateAgentOffset(agent_offset_global);

			// Hack to keep sky in the agent's region, otherwise it may get deleted - DJS 08/02/02
			// *TODO: possibly refactor into gSky->setAgentRegion(regionp)? -Brad
			if (gSky.mVOSkyp)
			{
				gSky.mVOSkyp->setRegion(regionp);
			}
			if (gSky.mVOGroundp)
			{
				gSky.mVOGroundp->setRegion(regionp);
			}

		}
		else
		{
			// First time initialization.
			// We've changed regions, we're now going to change our agent coordinate frame.
			mAgentOriginGlobal = regionp->getOriginGlobal();

			LLVector3 delta;
			delta.setVec(regionp->getOriginGlobal());

			setPositionAgent(getPositionAgent() - delta);
			LLVector3 camera_position_agent = LLViewerCamera::getInstance()->getOrigin();
			LLViewerCamera::getInstance()->setOrigin(camera_position_agent - delta);

			// Update all of the regions.
			LLWorld::getInstance()->updateAgentOffset(mAgentOriginGlobal);
		}
	}
	mRegionp = regionp;

	// Must shift hole-covering water object locations because local
	// coordinate frame changed.
	LLWorld::getInstance()->updateWaterObjects();

	// keep a list of regions we've been too
	// this is just an interesting stat, logged at the dataserver
	// we could trake this at the dataserver side, but that's harder
	U64 handle = regionp->getHandle();
	mRegionsVisited.insert(handle);

	LLSelectMgr::getInstance()->updateSelectionCenter();
}


//-----------------------------------------------------------------------------
// getRegion()
//-----------------------------------------------------------------------------
LLViewerRegion *LLAgent::getRegion() const
{
	return mRegionp;
}


const LLHost& LLAgent::getRegionHost() const
{
	if (mRegionp)
	{
		return mRegionp->getHost();
	}
	else
	{
		return LLHost::invalid;
	}
}

//-----------------------------------------------------------------------------
// getSLURL()
// returns empty() if getRegion() == NULL
//-----------------------------------------------------------------------------
std::string LLAgent::getSLURL() const
{
	std::string slurl;
	LLViewerRegion *regionp = getRegion();
	if (regionp)
	{
		LLVector3d agentPos = getPositionGlobal();
		S32 x = llround( (F32)fmod( agentPos.mdV[VX], (F64)REGION_WIDTH_METERS ) );
		S32 y = llround( (F32)fmod( agentPos.mdV[VY], (F64)REGION_WIDTH_METERS ) );
		S32 z = llround( (F32)agentPos.mdV[VZ] );
		slurl = LLURLDispatcher::buildSLURL(regionp->getName(), x, y, z);
	}
	return slurl;
}

//-----------------------------------------------------------------------------
// inPrelude()
//-----------------------------------------------------------------------------
BOOL LLAgent::inPrelude()
{
	return mRegionp && mRegionp->isPrelude();
}


//-----------------------------------------------------------------------------
// canManageEstate()
//-----------------------------------------------------------------------------

BOOL LLAgent::canManageEstate() const
{
	return mRegionp && mRegionp->canManageEstate();
}

//-----------------------------------------------------------------------------
// sendMessage()
//-----------------------------------------------------------------------------
void LLAgent::sendMessage()
{
	if (gDisconnected)
	{
		llwarns << "Trying to send message when disconnected!" << llendl;
		return;
	}
	if (!mRegionp)
	{
		llerrs << "No region for agent yet!" << llendl;
	}
	gMessageSystem->sendMessage(mRegionp->getHost());
}


//-----------------------------------------------------------------------------
// sendReliableMessage()
//-----------------------------------------------------------------------------
void LLAgent::sendReliableMessage()
{
	if (gDisconnected)
	{
		lldebugs << "Trying to send message when disconnected!" << llendl;
		return;
	}
	if (!mRegionp)
	{
		lldebugs << "LLAgent::sendReliableMessage No region for agent yet, not sending message!" << llendl;
		return;
	}
	gMessageSystem->sendReliable(mRegionp->getHost());
}

//-----------------------------------------------------------------------------
// getVelocity()
//-----------------------------------------------------------------------------
LLVector3 LLAgent::getVelocity() const
{
	if (mAvatarObject)
	{
		return mAvatarObject->getVelocity();
	}
	else
	{
		return LLVector3::zero;
	}
}


//-----------------------------------------------------------------------------
// setPositionAgent()
//-----------------------------------------------------------------------------
void LLAgent::setPositionAgent(const LLVector3 &pos_agent)
{
	if (!pos_agent.isFinite())
	{
		llerrs << "setPositionAgent is not a number" << llendl;
	}

	if (!mAvatarObject.isNull() && mAvatarObject->getParent())
	{
		LLVector3 pos_agent_sitting;
		LLVector3d pos_agent_d;
		LLViewerObject *parent = (LLViewerObject*)mAvatarObject->getParent();

		pos_agent_sitting = mAvatarObject->getPosition() * parent->getRotation() + parent->getPositionAgent();
		pos_agent_d.setVec(pos_agent_sitting);

		mFrameAgent.setOrigin(pos_agent_sitting);
		mPositionGlobal = pos_agent_d + mAgentOriginGlobal;
	}
	else
	{
		mFrameAgent.setOrigin(pos_agent);

		LLVector3d pos_agent_d;
		pos_agent_d.setVec(pos_agent);
		mPositionGlobal = pos_agent_d + mAgentOriginGlobal;
	}
}

//-----------------------------------------------------------------------------
// slamLookAt()
//-----------------------------------------------------------------------------
void LLAgent::slamLookAt(const LLVector3 &look_at)
{
	LLVector3 look_at_norm = look_at;
	look_at_norm.mV[VZ] = 0.f;
	look_at_norm.normVec();
	resetAxes(look_at_norm);
}

//-----------------------------------------------------------------------------
// getPositionGlobal()
//-----------------------------------------------------------------------------
const LLVector3d &LLAgent::getPositionGlobal() const
{
	if (!mAvatarObject.isNull() && !mAvatarObject->mDrawable.isNull())
	{
		mPositionGlobal = getPosGlobalFromAgent(mAvatarObject->getRenderPosition());
	}
	else
	{
		mPositionGlobal = getPosGlobalFromAgent(mFrameAgent.getOrigin());
	}

	return mPositionGlobal;
}

//-----------------------------------------------------------------------------
// getPositionAgent()
//-----------------------------------------------------------------------------
const LLVector3 &LLAgent::getPositionAgent()
{
	if(!mAvatarObject.isNull() && !mAvatarObject->mDrawable.isNull())
	{
		mFrameAgent.setOrigin(mAvatarObject->getRenderPosition());	
	}

	return mFrameAgent.getOrigin();
}

//-----------------------------------------------------------------------------
// getRegionsVisited()
//-----------------------------------------------------------------------------
S32 LLAgent::getRegionsVisited() const
{
	return mRegionsVisited.size();
}

//-----------------------------------------------------------------------------
// getDistanceTraveled()
//-----------------------------------------------------------------------------
F64 LLAgent::getDistanceTraveled() const
{
	return mDistanceTraveled;
}


//-----------------------------------------------------------------------------
// getPosAgentFromGlobal()
//-----------------------------------------------------------------------------
LLVector3 LLAgent::getPosAgentFromGlobal(const LLVector3d &pos_global) const
{
	LLVector3 pos_agent;
	pos_agent.setVec(pos_global - mAgentOriginGlobal);
	return pos_agent;
}


//-----------------------------------------------------------------------------
// getPosGlobalFromAgent()
//-----------------------------------------------------------------------------
LLVector3d LLAgent::getPosGlobalFromAgent(const LLVector3 &pos_agent) const
{
	LLVector3d pos_agent_d;
	pos_agent_d.setVec(pos_agent);
	return pos_agent_d + mAgentOriginGlobal;
}


//-----------------------------------------------------------------------------
// resetAxes()
//-----------------------------------------------------------------------------
void LLAgent::resetAxes()
{
	mFrameAgent.resetAxes();
}


// Copied from LLCamera::setOriginAndLookAt
// Look_at must be unit vector
//-----------------------------------------------------------------------------
// resetAxes()
//-----------------------------------------------------------------------------
void LLAgent::resetAxes(const LLVector3 &look_at)
{
	LLVector3	skyward = getReferenceUpVector();

	// if look_at has zero length, fail
	// if look_at and skyward are parallel, fail
	//
	// Test both of these conditions with a cross product.
	LLVector3 cross(look_at % skyward);
	if (cross.isNull())
	{
		llinfos << "LLAgent::resetAxes cross-product is zero" << llendl;
		return;
	}

	// Make sure look_at and skyward are not parallel
	// and neither are zero length
	LLVector3 left(skyward % look_at);
	LLVector3 up(look_at % left);

	mFrameAgent.setAxes(look_at, left, up);
}


//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLAgent::rotate(F32 angle, const LLVector3 &axis) 
{ 
	mFrameAgent.rotate(angle, axis); 
}


//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLAgent::rotate(F32 angle, F32 x, F32 y, F32 z) 
{ 
	mFrameAgent.rotate(angle, x, y, z); 
}


//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLAgent::rotate(const LLMatrix3 &matrix) 
{ 
	mFrameAgent.rotate(matrix); 
}


//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLAgent::rotate(const LLQuaternion &quaternion) 
{ 
	mFrameAgent.rotate(quaternion); 
}


//-----------------------------------------------------------------------------
// getReferenceUpVector()
//-----------------------------------------------------------------------------
LLVector3 LLAgent::getReferenceUpVector()
{
	// this vector is in the coordinate frame of the avatar's parent object, or the world if none
	LLVector3 up_vector = LLVector3::z_axis;
	if (mAvatarObject.notNull() && 
		mAvatarObject->getParent() &&
		mAvatarObject->mDrawable.notNull())
	{
		U32 camera_mode = mCameraAnimating ? mLastCameraMode : mCameraMode;
		// and in third person...
		if (camera_mode == CAMERA_MODE_THIRD_PERSON)
		{
			// make the up vector point to the absolute +z axis
			up_vector = up_vector * ~((LLViewerObject*)mAvatarObject->getParent())->getRenderRotation();
		}
		else if (camera_mode == CAMERA_MODE_MOUSELOOK)
		{
			// make the up vector point to the avatar's +z axis
			up_vector = up_vector * mAvatarObject->mDrawable->getRotation();
		}
	}

	return up_vector;
}


// Radians, positive is forward into ground
//-----------------------------------------------------------------------------
// pitch()
//-----------------------------------------------------------------------------
void LLAgent::pitch(F32 angle)
{
	// don't let user pitch if pointed almost all the way down or up
	mFrameAgent.pitch(clampPitchToLimits(angle));
}


// Radians, positive is forward into ground
//-----------------------------------------------------------------------------
// clampPitchToLimits()
//-----------------------------------------------------------------------------
F32 LLAgent::clampPitchToLimits(F32 angle)
{
	// A dot B = mag(A) * mag(B) * cos(angle between A and B)
	// so... cos(angle between A and B) = A dot B / mag(A) / mag(B)
	//                                  = A dot B for unit vectors

	LLVector3 skyward = getReferenceUpVector();

	F32			look_down_limit;
	F32			look_up_limit = 10.f * DEG_TO_RAD;

	F32 angle_from_skyward = acos( mFrameAgent.getAtAxis() * skyward );

	if (mAvatarObject.notNull() && mAvatarObject->mIsSitting)
	{
		look_down_limit = 130.f * DEG_TO_RAD;
	}
	else
	{
		look_down_limit = 170.f * DEG_TO_RAD;
	}

	// clamp pitch to limits
	if ((angle >= 0.f) && (angle_from_skyward + angle > look_down_limit))
	{
		angle = look_down_limit - angle_from_skyward;
	}
	else if ((angle < 0.f) && (angle_from_skyward + angle < look_up_limit))
	{
		angle = look_up_limit - angle_from_skyward;
	}
   
    return angle;
}


//-----------------------------------------------------------------------------
// roll()
//-----------------------------------------------------------------------------
void LLAgent::roll(F32 angle)
{
	mFrameAgent.roll(angle);
}


//-----------------------------------------------------------------------------
// yaw()
//-----------------------------------------------------------------------------
void LLAgent::yaw(F32 angle)
{
	if (!rotateGrabbed())
	{
		mFrameAgent.rotate(angle, getReferenceUpVector());
	}
}


// Returns a quat that represents the rotation of the agent in the absolute frame
//-----------------------------------------------------------------------------
// getQuat()
//-----------------------------------------------------------------------------
LLQuaternion LLAgent::getQuat() const
{
	return mFrameAgent.getQuaternion();
}


//-----------------------------------------------------------------------------
// calcFocusOffset()
//-----------------------------------------------------------------------------
LLVector3d LLAgent::calcFocusOffset(LLViewerObject *object, S32 x, S32 y)
{
	// calculate offset based on view direction
	BOOL is_avatar = object->isAvatar();
	LLMatrix4 obj_matrix = is_avatar  ? ((LLVOAvatar*)object)->mPelvisp->getWorldMatrix() : object->getRenderMatrix();
	LLQuaternion obj_rot = is_avatar  ? ((LLVOAvatar*)object)->mPelvisp->getWorldRotation() : object->getRenderRotation();
	LLVector3 obj_pos = is_avatar ? ((LLVOAvatar*)object)->mPelvisp->getWorldPosition() : object->getRenderPosition();
	LLQuaternion inv_obj_rot = ~obj_rot;

	LLVector3 obj_dir_abs = obj_pos - LLViewerCamera::getInstance()->getOrigin();
	obj_dir_abs.rotVec(inv_obj_rot);
	obj_dir_abs.normVec();
	obj_dir_abs.abs();

	LLVector3 object_extents = object->getScale();
	// make sure they object extents are non-zero
	object_extents.clamp(0.001f, F32_MAX);
	LLVector3 object_half_extents = object_extents * 0.5f;

	obj_dir_abs.mV[VX] = obj_dir_abs.mV[VX] / object_extents.mV[VX];
	obj_dir_abs.mV[VY] = obj_dir_abs.mV[VY] / object_extents.mV[VY];
	obj_dir_abs.mV[VZ] = obj_dir_abs.mV[VZ] / object_extents.mV[VZ];

	LLVector3 normal;
	if (obj_dir_abs.mV[VX] > obj_dir_abs.mV[VY] && obj_dir_abs.mV[VX] > obj_dir_abs.mV[VZ])
	{
		normal.setVec(obj_matrix.getFwdRow4());
	}
	else if (obj_dir_abs.mV[VY] > obj_dir_abs.mV[VZ])
	{
		normal.setVec(obj_matrix.getLeftRow4());
	}
	else
	{
		normal.setVec(obj_matrix.getUpRow4());
	}
	normal.normVec();

	LLVector3d focus_pt_global;
	// RN: should we check return value for valid pick?
	gViewerWindow->mousePointOnPlaneGlobal(focus_pt_global, x, y, gAgent.getPosGlobalFromAgent(obj_pos), normal);
	LLVector3 focus_pt = gAgent.getPosAgentFromGlobal(focus_pt_global);
	// find vector from camera to focus point in object coordinates
	LLVector3 camera_focus_vec = focus_pt - LLViewerCamera::getInstance()->getOrigin();
	// convert to object-local space
	camera_focus_vec.rotVec(inv_obj_rot);

	// find vector from object origin to focus point in object coordinates
	LLVector3 focus_delta = focus_pt - obj_pos;
	// convert to object-local space
	focus_delta.rotVec(inv_obj_rot);

	// calculate clip percentage needed to get focus offset back in bounds along the camera_focus axis
	LLVector3 clip_fraction;

	for (U32 axis = VX; axis <= VZ; axis++)
	{
		F32 clip_amt;
		if (focus_delta.mV[axis] > 0.f)
		{
			clip_amt = llmax(0.f, focus_delta.mV[axis] - object_half_extents.mV[axis]);
		}
		else
		{
			clip_amt = llmin(0.f, focus_delta.mV[axis] + object_half_extents.mV[axis]);
		}

		// don't divide by very small nunber
		if (llabs(camera_focus_vec.mV[axis]) < 0.0001f)
		{
			clip_fraction.mV[axis] = 0.f;
		}
		else
		{
			clip_fraction.mV[axis] = clip_amt / camera_focus_vec.mV[axis];
		}
	}

	LLVector3 abs_clip_fraction = clip_fraction;
	abs_clip_fraction.abs();

	// find greatest shrinkage factor and
	// rescale focus offset to inside object extents
	if (abs_clip_fraction.mV[VX] > abs_clip_fraction.mV[VY] &&
		abs_clip_fraction.mV[VX] > abs_clip_fraction.mV[VZ])
	{
		focus_delta -= clip_fraction.mV[VX] * camera_focus_vec;
	}
	else if (abs_clip_fraction.mV[VY] > abs_clip_fraction.mV[VZ])
	{
		focus_delta -= clip_fraction.mV[VY] * camera_focus_vec;
	}
	else
	{
		focus_delta -= clip_fraction.mV[VZ] * camera_focus_vec;
	}

	// convert back to world space
	focus_delta.rotVec(obj_rot);
	
	if (!is_avatar) 
	{
		//unproject relative clicked coordinate from window coordinate using GL
		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];
		GLfloat winX, winY, winZ;
		GLdouble posX, posY, posZ;

		// convert our matrices to something that has a multiply that works
		glh::matrix4f newModel((F32*)LLViewerCamera::getInstance()->getModelview().mMatrix);
		glh::matrix4f tmpObjMat((F32*)obj_matrix.mMatrix);
		newModel *= tmpObjMat;

		for(U32 i = 0; i < 16; ++i)
		{
			modelview[i] = newModel.m[i];
			projection[i] = LLViewerCamera::getInstance()->getProjection().mMatrix[i/4][i%4];
		}
		glGetIntegerv( GL_VIEWPORT, viewport );

		winX = ((F32)x) * gViewerWindow->getDisplayScale().mV[VX];
		winY = ((F32)y) * gViewerWindow->getDisplayScale().mV[VY];
		glReadPixels( llfloor(winX), llfloor(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );

		gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);

		LLVector3 obj_rel((F32)posX, (F32)posY, (F32)posZ);
		obj_rel = obj_rel * object->getRenderMatrix();
		obj_rel -= object->getRenderPosition();
		
		LLVector3 obj_center = LLVector3(0, 0, 0) * object->getRenderMatrix();

		//now that we have the object relative position, we should bias toward the center of the object 
		//based on the distance of the camera to the focus point vs. the distance of the camera to the focus

		F32 relDist = llabs(obj_rel * LLViewerCamera::getInstance()->getAtAxis());
		F32 viewDist = dist_vec(obj_center + obj_rel, LLViewerCamera::getInstance()->getOrigin());


		LLBBox obj_bbox = object->getBoundingBoxAgent();
		F32 bias = 0.f;

		LLVector3 virtual_camera_pos = gAgent.getPosAgentFromGlobal(mFocusTargetGlobal + (getCameraPositionGlobal() - mFocusTargetGlobal) / (1.f + mCameraFOVZoomFactor));

		if(obj_bbox.containsPointAgent(virtual_camera_pos))
		{
			// if the camera is inside the object (large, hollow objects, for example)
			// force focus point all the way to destination depth, away from object center
			bias = 1.f;
		}
		else
		{
			// perform magic number biasing of focus point towards surface vs. planar center
			bias = clamp_rescale(relDist/viewDist, 0.1f, 0.7f, 0.0f, 1.0f);
		}
		
		obj_rel = lerp(focus_delta, obj_rel, bias);
		
		return LLVector3d(obj_rel);
	}

	return LLVector3d(focus_delta.mV[VX], focus_delta.mV[VY], focus_delta.mV[VZ]);
}

//-----------------------------------------------------------------------------
// calcCameraMinDistance()
//-----------------------------------------------------------------------------
BOOL LLAgent::calcCameraMinDistance(F32 &obj_min_distance)
{
	BOOL soft_limit = FALSE; // is the bounding box to be treated literally (volumes) or as an approximation (avatars)

	if (!mFocusObject || mFocusObject->isDead())
	{
		obj_min_distance = 0.f;
		return TRUE;
	}

	if (mFocusObject->mDrawable.isNull())
	{
#ifdef LL_RELEASE_FOR_DOWNLOAD
		llwarns << "Focus object with no drawable!" << llendl;
#else
		mFocusObject->dump();
		llerrs << "Focus object with no drawable!" << llendl;
#endif
		obj_min_distance = 0.f;
		return TRUE;
	}
	
	LLQuaternion inv_object_rot = ~mFocusObject->getRenderRotation();
	LLVector3 target_offset_origin = mFocusObjectOffset;
	LLVector3 camera_offset_target(getCameraPositionAgent() - getPosAgentFromGlobal(mFocusTargetGlobal));

	// convert offsets into object local space
	camera_offset_target.rotVec(inv_object_rot);
	target_offset_origin.rotVec(inv_object_rot);

	// push around object extents based on target offset
	LLVector3 object_extents = mFocusObject->getScale();
	if (mFocusObject->isAvatar())
	{
		// fudge factors that lets you zoom in on avatars a bit more (which don't do FOV zoom)
		object_extents.mV[VX] *= AVATAR_ZOOM_MIN_X_FACTOR;
		object_extents.mV[VY] *= AVATAR_ZOOM_MIN_Y_FACTOR;
		object_extents.mV[VZ] *= AVATAR_ZOOM_MIN_Z_FACTOR;
		soft_limit = TRUE;
	}
	LLVector3 abs_target_offset = target_offset_origin;
	abs_target_offset.abs();

	LLVector3 target_offset_dir = target_offset_origin;
	F32 object_radius = mFocusObject->getVObjRadius();

	BOOL target_outside_object_extents = FALSE;

	for (U32 i = VX; i <= VZ; i++)
	{
		if (abs_target_offset.mV[i] * 2.f > object_extents.mV[i] + OBJECT_EXTENTS_PADDING)
		{
			target_outside_object_extents = TRUE;
		}
		if (camera_offset_target.mV[i] > 0.f)
		{
			object_extents.mV[i] -= target_offset_origin.mV[i] * 2.f;
		}
		else
		{
			object_extents.mV[i] += target_offset_origin.mV[i] * 2.f;
		}
	}

	// don't shrink the object extents so far that the object inverts
	object_extents.clamp(0.001f, F32_MAX);

	// move into first octant
	LLVector3 camera_offset_target_abs_norm = camera_offset_target;
	camera_offset_target_abs_norm.abs();
	// make sure offset is non-zero
	camera_offset_target_abs_norm.clamp(0.001f, F32_MAX);
	camera_offset_target_abs_norm.normVec();

	// find camera position relative to normalized object extents
	LLVector3 camera_offset_target_scaled = camera_offset_target_abs_norm;
	camera_offset_target_scaled.mV[VX] /= object_extents.mV[VX];
	camera_offset_target_scaled.mV[VY] /= object_extents.mV[VY];
	camera_offset_target_scaled.mV[VZ] /= object_extents.mV[VZ];

	if (camera_offset_target_scaled.mV[VX] > camera_offset_target_scaled.mV[VY] && 
		camera_offset_target_scaled.mV[VX] > camera_offset_target_scaled.mV[VZ])
	{
		if (camera_offset_target_abs_norm.mV[VX] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VX] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VX] * 0.5f / camera_offset_target_abs_norm.mV[VX];
		}
	}
	else if (camera_offset_target_scaled.mV[VY] > camera_offset_target_scaled.mV[VZ])
	{
		if (camera_offset_target_abs_norm.mV[VY] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VY] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VY] * 0.5f / camera_offset_target_abs_norm.mV[VY];
		}
	}
	else
	{
		if (camera_offset_target_abs_norm.mV[VZ] < 0.001f)
		{
			obj_min_distance = object_extents.mV[VZ] * 0.5f;
		}
		else
		{
			obj_min_distance = object_extents.mV[VZ] * 0.5f / camera_offset_target_abs_norm.mV[VZ];
		}
	}

	LLVector3 object_split_axis;
	LLVector3 target_offset_scaled = target_offset_origin;
	target_offset_scaled.abs();
	target_offset_scaled.normVec();
	target_offset_scaled.mV[VX] /= object_extents.mV[VX];
	target_offset_scaled.mV[VY] /= object_extents.mV[VY];
	target_offset_scaled.mV[VZ] /= object_extents.mV[VZ];

	if (target_offset_scaled.mV[VX] > target_offset_scaled.mV[VY] && 
		target_offset_scaled.mV[VX] > target_offset_scaled.mV[VZ])
	{
		object_split_axis = LLVector3::x_axis;
	}
	else if (target_offset_scaled.mV[VY] > target_offset_scaled.mV[VZ])
	{
		object_split_axis = LLVector3::y_axis;
	}
	else
	{
		object_split_axis = LLVector3::z_axis;
	}

	LLVector3 camera_offset_object(getCameraPositionAgent() - mFocusObject->getPositionAgent());

	// length projected orthogonal to target offset
	F32 camera_offset_dist = (camera_offset_object - target_offset_dir * (camera_offset_object * target_offset_dir)).magVec();

	// calculate whether the target point would be "visible" if it were outside the bounding box
	// on the opposite of the splitting plane defined by object_split_axis;
	BOOL exterior_target_visible = FALSE;
	if (camera_offset_dist > object_radius)
	{
		// target is visible from camera, so turn off fov zoom
		exterior_target_visible = TRUE;
	}

	F32 camera_offset_clip = camera_offset_object * object_split_axis;
	F32 target_offset_clip = target_offset_dir * object_split_axis;

	// target has moved outside of object extents
	// check to see if camera and target are on same side 
	if (target_outside_object_extents)
	{
		if (camera_offset_clip > 0.f && target_offset_clip > 0.f)
		{
			return FALSE;
		}
		else if (camera_offset_clip < 0.f && target_offset_clip < 0.f)
		{
			return FALSE;
		}
	}

	// clamp obj distance to diagonal of 10 by 10 cube
	obj_min_distance = llmin(obj_min_distance, 10.f * F_SQRT3);

	obj_min_distance += LLViewerCamera::getInstance()->getNear() + (soft_limit ? 0.1f : 0.2f);
	
	return TRUE;
}

F32 LLAgent::getCameraZoomFraction()
{
	// 0.f -> camera zoomed all the way out
	// 1.f -> camera zoomed all the way in
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// already [0,1]
		return mAvatarObject->mHUDTargetZoom;
	}
	else if (mFocusOnAvatar && cameraThirdPerson())
	{
		return clamp_rescale(mCameraZoomFraction, MIN_ZOOM_FRACTION, MAX_ZOOM_FRACTION, 1.f, 0.f);
	}
	else if (cameraCustomizeAvatar())
	{
		F32 distance = (F32)mCameraFocusOffsetTarget.magVec();
		return clamp_rescale(distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM, 1.f, 0.f );
	}
	else
	{
		F32 min_zoom;
		const F32 DIST_FUDGE = 16.f; // meters
		F32 max_zoom = llmin(mDrawDistance - DIST_FUDGE, 
								LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE,
								MAX_CAMERA_DISTANCE_FROM_AGENT);

		F32 distance = (F32)mCameraFocusOffsetTarget.magVec();
		if (mFocusObject.notNull())
		{
			if (mFocusObject->isAvatar())
			{
				min_zoom = AVATAR_MIN_ZOOM;
			}
			else
			{
				min_zoom = OBJECT_MIN_ZOOM;
			}
		}
		else
		{
			min_zoom = LAND_MIN_ZOOM;
		}

		return clamp_rescale(distance, min_zoom, max_zoom, 1.f, 0.f);
	}
}

void LLAgent::setCameraZoomFraction(F32 fraction)
{
	// 0.f -> camera zoomed all the way out
	// 1.f -> camera zoomed all the way in
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		mAvatarObject->mHUDTargetZoom = fraction;
	}
	else if (mFocusOnAvatar && cameraThirdPerson())
	{
		mCameraZoomFraction = rescale(fraction, 0.f, 1.f, MAX_ZOOM_FRACTION, MIN_ZOOM_FRACTION);
	}
	else if (cameraCustomizeAvatar())
	{
		LLVector3d camera_offset_dir = mCameraFocusOffsetTarget;
		camera_offset_dir.normVec();
		mCameraFocusOffsetTarget = camera_offset_dir * rescale(fraction, 0.f, 1.f, APPEARANCE_MAX_ZOOM, APPEARANCE_MIN_ZOOM);
	}
	else
	{
		F32 min_zoom = LAND_MIN_ZOOM;
		const F32 DIST_FUDGE = 16.f; // meters
		F32 max_zoom = llmin(mDrawDistance - DIST_FUDGE, 
								LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE,
								MAX_CAMERA_DISTANCE_FROM_AGENT);

		if (mFocusObject.notNull())
		{
			if (mFocusObject.notNull())
			{
				if (mFocusObject->isAvatar())
				{
					min_zoom = AVATAR_MIN_ZOOM;
				}
				else
				{
					min_zoom = OBJECT_MIN_ZOOM;
				}
			}
		}

		LLVector3d camera_offset_dir = mCameraFocusOffsetTarget;
		camera_offset_dir.normVec();
		mCameraFocusOffsetTarget = camera_offset_dir * rescale(fraction, 0.f, 1.f, max_zoom, min_zoom);
	}
	startCameraAnimation();
}


//-----------------------------------------------------------------------------
// cameraOrbitAround()
//-----------------------------------------------------------------------------
void LLAgent::cameraOrbitAround(const F32 radians)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// do nothing for hud selection
	}
	else if (mFocusOnAvatar && (mCameraMode == CAMERA_MODE_THIRD_PERSON || mCameraMode == CAMERA_MODE_FOLLOW))
	{
		mFrameAgent.rotate(radians, getReferenceUpVector());
	}
	else
	{
		mCameraFocusOffsetTarget.rotVec(radians, 0.f, 0.f, 1.f);
		
		cameraZoomIn(1.f);
	}
}


//-----------------------------------------------------------------------------
// cameraOrbitOver()
//-----------------------------------------------------------------------------
void LLAgent::cameraOrbitOver(const F32 angle)
{
	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// do nothing for hud selection
	}
	else if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
	{
		pitch(angle);
	}
	else
	{
		LLVector3 camera_offset_unit(mCameraFocusOffsetTarget);
		camera_offset_unit.normVec();

		F32 angle_from_up = acos( camera_offset_unit * getReferenceUpVector() );

		LLVector3d left_axis;
		left_axis.setVec(LLViewerCamera::getInstance()->getLeftAxis());
		F32 new_angle = llclamp(angle_from_up - angle, 1.f * DEG_TO_RAD, 179.f * DEG_TO_RAD);
		mCameraFocusOffsetTarget.rotVec(angle_from_up - new_angle, left_axis);

		cameraZoomIn(1.f);
	}
}

//-----------------------------------------------------------------------------
// cameraZoomIn()
//-----------------------------------------------------------------------------
void LLAgent::cameraZoomIn(const F32 fraction)
{
	if (gDisconnected)
	{
		return;
	}

	LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
	if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
	{
		// just update hud zoom level
		mAvatarObject->mHUDTargetZoom /= fraction;
		return;
	}


	LLVector3d	camera_offset(mCameraFocusOffsetTarget);
	LLVector3d	camera_offset_unit(mCameraFocusOffsetTarget);
	F32 min_zoom = LAND_MIN_ZOOM;
	F32 current_distance = (F32)camera_offset_unit.normVec();
	F32 new_distance = current_distance * fraction;

	// Don't move through focus point
	if (mFocusObject)
	{
		LLVector3 camera_offset_dir((F32)camera_offset_unit.mdV[VX], (F32)camera_offset_unit.mdV[VY], (F32)camera_offset_unit.mdV[VZ]);

		if (mFocusObject->isAvatar())
		{
			calcCameraMinDistance(min_zoom);
		}
		else
		{
			min_zoom = OBJECT_MIN_ZOOM;
		}
	}

	new_distance = llmax(new_distance, min_zoom); 

	// Don't zoom too far back
	const F32 DIST_FUDGE = 16.f; // meters
	F32 max_distance = llmin(mDrawDistance - DIST_FUDGE, 
							 LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE );

	if (new_distance > max_distance)
	{
		new_distance = max_distance;

		/*
		// Unless camera is unlocked
		if (!LLViewerCamera::sDisableCameraConstraints)
		{
			return;
		}
		*/
	}

	if( cameraCustomizeAvatar() )
	{
		new_distance = llclamp( new_distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM );
	}

	mCameraFocusOffsetTarget = new_distance * camera_offset_unit;
}

//-----------------------------------------------------------------------------
// cameraOrbitIn()
//-----------------------------------------------------------------------------
void LLAgent::cameraOrbitIn(const F32 meters)
{
	if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
	{
		F32 camera_offset_dist = llmax(0.001f, mCameraOffsetDefault.magVec());
		
		mCameraZoomFraction = (mTargetCameraDistance - meters) / camera_offset_dist;

		if (!gSavedSettings.getBOOL("FreezeTime") && mCameraZoomFraction < MIN_ZOOM_FRACTION && meters > 0.f)
		{
			// No need to animate, camera is already there.
			changeCameraToMouselook(FALSE);
		}

		mCameraZoomFraction = llclamp(mCameraZoomFraction, MIN_ZOOM_FRACTION, MAX_ZOOM_FRACTION);
	}
	else
	{
		LLVector3d	camera_offset(mCameraFocusOffsetTarget);
		LLVector3d	camera_offset_unit(mCameraFocusOffsetTarget);
		F32 current_distance = (F32)camera_offset_unit.normVec();
		F32 new_distance = current_distance - meters;
		F32 min_zoom = LAND_MIN_ZOOM;
		
		// Don't move through focus point
		if (mFocusObject.notNull())
		{
			if (mFocusObject->isAvatar())
			{
				min_zoom = AVATAR_MIN_ZOOM;
			}
			else
			{
				min_zoom = OBJECT_MIN_ZOOM;
			}
		}

		new_distance = llmax(new_distance, min_zoom);

		// Don't zoom too far back
		const F32 DIST_FUDGE = 16.f; // meters
		F32 max_distance = llmin(mDrawDistance - DIST_FUDGE, 
								 LLWorld::getInstance()->getRegionWidthInMeters() - DIST_FUDGE );

		if (new_distance > max_distance)
		{
			// Unless camera is unlocked
			if (!gSavedSettings.getBOOL("DisableCameraConstraints"))
			{
				return;
			}
		}

		if( CAMERA_MODE_CUSTOMIZE_AVATAR == getCameraMode() )
		{
			new_distance = llclamp( new_distance, APPEARANCE_MIN_ZOOM, APPEARANCE_MAX_ZOOM );
		}

		// Compute new camera offset
		mCameraFocusOffsetTarget = new_distance * camera_offset_unit;
		cameraZoomIn(1.f);
	}
}


//-----------------------------------------------------------------------------
// cameraPanIn()
//-----------------------------------------------------------------------------
void LLAgent::cameraPanIn(F32 meters)
{
	LLVector3d at_axis;
	at_axis.setVec(LLViewerCamera::getInstance()->getAtAxis());

	mFocusTargetGlobal += meters * at_axis;
	mFocusGlobal = mFocusTargetGlobal;
	// don't enforce zoom constraints as this is the only way for users to get past them easily
	updateFocusOffset();
}

//-----------------------------------------------------------------------------
// cameraPanLeft()
//-----------------------------------------------------------------------------
void LLAgent::cameraPanLeft(F32 meters)
{
	LLVector3d left_axis;
	left_axis.setVec(LLViewerCamera::getInstance()->getLeftAxis());

	mFocusTargetGlobal += meters * left_axis;
	mFocusGlobal = mFocusTargetGlobal;
	cameraZoomIn(1.f);
	updateFocusOffset();
}

//-----------------------------------------------------------------------------
// cameraPanUp()
//-----------------------------------------------------------------------------
void LLAgent::cameraPanUp(F32 meters)
{
	LLVector3d up_axis;
	up_axis.setVec(LLViewerCamera::getInstance()->getUpAxis());

	mFocusTargetGlobal += meters * up_axis;
	mFocusGlobal = mFocusTargetGlobal;
	cameraZoomIn(1.f);
	updateFocusOffset();
}

//-----------------------------------------------------------------------------
// setKey()
//-----------------------------------------------------------------------------
void LLAgent::setKey(const S32 direction, S32 &key)
{
	if (direction > 0)
	{
		key = 1;
	}
	else if (direction < 0)
	{
		key = -1;
	}
	else
	{
		key = 0;
	}
}


//-----------------------------------------------------------------------------
// getControlFlags()
//-----------------------------------------------------------------------------
U32 LLAgent::getControlFlags()
{
/*
	// HACK -- avoids maintenance of control flags when camera mode is turned on or off,
	// only worries about it when the flags are measured
	if (mCameraMode == CAMERA_MODE_MOUSELOOK) 
	{
		if ( !(mControlFlags & AGENT_CONTROL_MOUSELOOK) )
		{
			mControlFlags |= AGENT_CONTROL_MOUSELOOK;
		}
	}
*/
	return mControlFlags;
}

//-----------------------------------------------------------------------------
// setControlFlags()
//-----------------------------------------------------------------------------
void LLAgent::setControlFlags(U32 mask)
{
	mControlFlags |= mask;
	mbFlagsDirty = TRUE;
}


//-----------------------------------------------------------------------------
// clearControlFlags()
//-----------------------------------------------------------------------------
void LLAgent::clearControlFlags(U32 mask)
{
	U32 old_flags = mControlFlags;
	mControlFlags &= ~mask;
	if (old_flags != mControlFlags)
	{
		mbFlagsDirty = TRUE;
	}
}

//-----------------------------------------------------------------------------
// controlFlagsDirty()
//-----------------------------------------------------------------------------
BOOL LLAgent::controlFlagsDirty() const
{
	return mbFlagsDirty;
}

//-----------------------------------------------------------------------------
// enableControlFlagReset()
//-----------------------------------------------------------------------------
void LLAgent::enableControlFlagReset()
{
	mbFlagsNeedReset = TRUE;
}

//-----------------------------------------------------------------------------
// resetControlFlags()
//-----------------------------------------------------------------------------
void LLAgent::resetControlFlags()
{
	if (mbFlagsNeedReset)
	{
		mbFlagsNeedReset = FALSE;
		mbFlagsDirty = FALSE;
		// reset all of the ephemeral flags
		// some flags are managed elsewhere
		mControlFlags &= AGENT_CONTROL_AWAY | AGENT_CONTROL_FLY | AGENT_CONTROL_MOUSELOOK;
	}
}

//-----------------------------------------------------------------------------
// setAFK()
//-----------------------------------------------------------------------------
void LLAgent::setAFK()
{
	// Drones can't go AFK
	if (gNoRender)
	{
		return;
	}

	if (!gAgent.getRegion())
	{
		// Don't set AFK if we're not talking to a region yet.
		return;
	}

	if (!(mControlFlags & AGENT_CONTROL_AWAY))
	{
		sendAnimationRequest(ANIM_AGENT_AWAY, ANIM_REQUEST_START);
		setControlFlags(AGENT_CONTROL_AWAY | AGENT_CONTROL_STOP);
		gAwayTimer.start();
		if (gAFKMenu)
		{
			//*TODO:Translate
			gAFKMenu->setLabel(LLString("Set Not Away"));
		}
	}
}

//-----------------------------------------------------------------------------
// clearAFK()
//-----------------------------------------------------------------------------
void LLAgent::clearAFK()
{
	gAwayTriggerTimer.reset();

	// Gods can sometimes get into away state (via gestures)
	// without setting the appropriate control flag. JC
	LLVOAvatar* av = mAvatarObject;
	if (mControlFlags & AGENT_CONTROL_AWAY
		|| (av
			&& (av->mSignaledAnimations.find(ANIM_AGENT_AWAY) != av->mSignaledAnimations.end())))
	{
		sendAnimationRequest(ANIM_AGENT_AWAY, ANIM_REQUEST_STOP);
		clearControlFlags(AGENT_CONTROL_AWAY);
		if (gAFKMenu)
		{
			//*TODO:Translate
			gAFKMenu->setLabel(LLString("Set Away"));
		}
	}
}

//-----------------------------------------------------------------------------
// getAFK()
//-----------------------------------------------------------------------------
BOOL LLAgent::getAFK() const
{
	return (mControlFlags & AGENT_CONTROL_AWAY) != 0;
}

//-----------------------------------------------------------------------------
// setBusy()
//-----------------------------------------------------------------------------
void LLAgent::setBusy()
{
	sendAnimationRequest(ANIM_AGENT_BUSY, ANIM_REQUEST_START);
	mIsBusy = TRUE;
	if (gBusyMenu)
	{
		//*TODO:Translate
		gBusyMenu->setLabel(LLString("Set Not Busy"));
	}
	LLFloaterMute::getInstance()->updateButtons();
}

//-----------------------------------------------------------------------------
// clearBusy()
//-----------------------------------------------------------------------------
void LLAgent::clearBusy()
{
	mIsBusy = FALSE;
	sendAnimationRequest(ANIM_AGENT_BUSY, ANIM_REQUEST_STOP);
	if (gBusyMenu)
	{
		//*TODO:Translate
		gBusyMenu->setLabel(LLString("Set Busy"));
	}
	LLFloaterMute::getInstance()->updateButtons();
}

//-----------------------------------------------------------------------------
// getBusy()
//-----------------------------------------------------------------------------
BOOL LLAgent::getBusy() const
{
	return mIsBusy;
}


//-----------------------------------------------------------------------------
// startAutoPilotGlobal()
//-----------------------------------------------------------------------------
void LLAgent::startAutoPilotGlobal(const LLVector3d &target_global, const std::string& behavior_name, const LLQuaternion *target_rotation, void (*finish_callback)(BOOL, void *),  void *callback_data, F32 stop_distance, F32 rot_threshold)
{
	if (!gAgent.getAvatarObject())
	{
		return;
	}
	
	mAutoPilotFinishedCallback = finish_callback;
	mAutoPilotCallbackData = callback_data;
	mAutoPilotRotationThreshold = rot_threshold;
	mAutoPilotBehaviorName = behavior_name;

	LLVector3d delta_pos( target_global );
	delta_pos -= getPositionGlobal();
	F64 distance = delta_pos.magVec();
	LLVector3d trace_target = target_global;

	trace_target.mdV[VZ] -= 10.f;

	LLVector3d intersection;
	LLVector3 normal;
	LLViewerObject *hit_obj;
	F32 heightDelta = LLWorld::getInstance()->resolveStepHeightGlobal(NULL, target_global, trace_target, intersection, normal, &hit_obj);

	if (stop_distance > 0.f)
	{
		mAutoPilotStopDistance = stop_distance;
	}
	else
	{
		// Guess at a reasonable stop distance.
		mAutoPilotStopDistance = fsqrtf( distance );
		if (mAutoPilotStopDistance < 0.5f) 
		{
			mAutoPilotStopDistance = 0.5f;
		}
	}

	mAutoPilotFlyOnStop = getFlying();

	if (distance > 30.0)
	{
		setFlying(TRUE);
	}

	if ( distance > 1.f && heightDelta > (sqrtf(mAutoPilotStopDistance) + 1.f))
	{
		setFlying(TRUE);
		mAutoPilotFlyOnStop = TRUE;
	}

	mAutoPilot = TRUE;
	mAutoPilotTargetGlobal = target_global;

	// trace ray down to find height of destination from ground
	LLVector3d traceEndPt = target_global;
	traceEndPt.mdV[VZ] -= 20.f;

	LLVector3d targetOnGround;
	LLVector3 groundNorm;
	LLViewerObject *obj;

	LLWorld::getInstance()->resolveStepHeightGlobal(NULL, target_global, traceEndPt, targetOnGround, groundNorm, &obj);
	F64 target_height = llmax((F64)gAgent.getAvatarObject()->getPelvisToFoot(), target_global.mdV[VZ] - targetOnGround.mdV[VZ]);

	// clamp z value of target to minimum height above ground
	mAutoPilotTargetGlobal.mdV[VZ] = targetOnGround.mdV[VZ] + target_height;
	mAutoPilotTargetDist = (F32)dist_vec(gAgent.getPositionGlobal(), mAutoPilotTargetGlobal);
	if (target_rotation)
	{
		mAutoPilotUseRotation = TRUE;
		mAutoPilotTargetFacing = LLVector3::x_axis * *target_rotation;
		mAutoPilotTargetFacing.mV[VZ] = 0.f;
		mAutoPilotTargetFacing.normVec();
	}
	else
	{
		mAutoPilotUseRotation = FALSE;
	}

	mAutoPilotNoProgressFrameCount = 0;
}


//-----------------------------------------------------------------------------
// startFollowPilot()
//-----------------------------------------------------------------------------
void LLAgent::startFollowPilot(const LLUUID &leader_id)
{
	if (!mAutoPilot) return;

	mLeaderID = leader_id;
	if ( mLeaderID.isNull() ) return;

	LLViewerObject* object = gObjectList.findObject(mLeaderID);
	if (!object) 
	{
		mLeaderID = LLUUID::null;
		return;
	}

	startAutoPilotGlobal(object->getPositionGlobal());
}


//-----------------------------------------------------------------------------
// stopAutoPilot()
//-----------------------------------------------------------------------------
void LLAgent::stopAutoPilot(BOOL user_cancel)
{
	if (mAutoPilot)
	{
		mAutoPilot = FALSE;
		if (mAutoPilotUseRotation && !user_cancel)
		{
			resetAxes(mAutoPilotTargetFacing);
		}
		//NB: auto pilot can terminate for a reason other than reaching the destination
		if (mAutoPilotFinishedCallback)
		{
			mAutoPilotFinishedCallback(!user_cancel && dist_vec(gAgent.getPositionGlobal(), mAutoPilotTargetGlobal) < mAutoPilotStopDistance, mAutoPilotCallbackData);
		}
		mLeaderID = LLUUID::null;

		// If the user cancelled, don't change the fly state
		if (!user_cancel)
		{
			setFlying(mAutoPilotFlyOnStop);
		}
		setControlFlags(AGENT_CONTROL_STOP);

		if (user_cancel && !mAutoPilotBehaviorName.empty())
		{
			if (mAutoPilotBehaviorName == "Sit")
				LLNotifyBox::showXml("CancelledSit");
			else if (mAutoPilotBehaviorName == "Attach")
				LLNotifyBox::showXml("CancelledAttach");
			else
				LLNotifyBox::showXml("Cancelled");
		}
	}
}


// Returns necessary agent pitch and yaw changes, radians.
//-----------------------------------------------------------------------------
// autoPilot()
//-----------------------------------------------------------------------------
void LLAgent::autoPilot(F32 *delta_yaw)
{
	if (mAutoPilot)
	{
		if (!mLeaderID.isNull())
		{
			LLViewerObject* object = gObjectList.findObject(mLeaderID);
			if (!object) 
			{
				stopAutoPilot();
				return;
			}
			mAutoPilotTargetGlobal = object->getPositionGlobal();
		}
		
		if (!mAvatarObject)
		{
			return;
		}

		if (mAvatarObject->mInAir)
		{
			setFlying(TRUE);
		}
	
		LLVector3 at;
		at.setVec(mFrameAgent.getAtAxis());
		LLVector3 target_agent = getPosAgentFromGlobal(mAutoPilotTargetGlobal);
		LLVector3 direction = target_agent - getPositionAgent();

		F32 target_dist = direction.magVec();

		if (target_dist >= mAutoPilotTargetDist)
		{
			mAutoPilotNoProgressFrameCount++;
			if (mAutoPilotNoProgressFrameCount > AUTOPILOT_MAX_TIME_NO_PROGRESS * gFPSClamped)
			{
				stopAutoPilot();
				return;
			}
		}

		mAutoPilotTargetDist = target_dist;

		// Make this a two-dimensional solution
		at.mV[VZ] = 0.f;
		direction.mV[VZ] = 0.f;

		at.normVec();
		F32 xy_distance = direction.normVec();

		F32 yaw = 0.f;
		if (mAutoPilotTargetDist > mAutoPilotStopDistance)
		{
			yaw = angle_between(mFrameAgent.getAtAxis(), direction);
		}
		else if (mAutoPilotUseRotation)
		{
			// we're close now just aim at target facing
			yaw = angle_between(at, mAutoPilotTargetFacing);
			direction = mAutoPilotTargetFacing;
		}

		yaw = 4.f * yaw / gFPSClamped;

		// figure out which direction to turn
		LLVector3 scratch(at % direction);

		if (scratch.mV[VZ] > 0.f)
		{
			setControlFlags(AGENT_CONTROL_YAW_POS);
		}
		else
		{
			yaw = -yaw;
			setControlFlags(AGENT_CONTROL_YAW_NEG);
		}

		*delta_yaw = yaw;

		// Compute when to start slowing down and when to stop
		F32 stop_distance = mAutoPilotStopDistance;
		F32 slow_distance;
		if (getFlying())
		{
			slow_distance = llmax(6.f, mAutoPilotStopDistance + 5.f);
			stop_distance = llmax(2.f, mAutoPilotStopDistance);
		}
		else
		{
			slow_distance = llmax(3.f, mAutoPilotStopDistance + 2.f);
		}

		// If we're flying, handle autopilot points above or below you.
		if (getFlying() && xy_distance < AUTOPILOT_HEIGHT_ADJUST_DISTANCE)
		{
			if (mAvatarObject)
			{
				F64 current_height = mAvatarObject->getPositionGlobal().mdV[VZ];
				F32 delta_z = (F32)(mAutoPilotTargetGlobal.mdV[VZ] - current_height);
				F32 slope = delta_z / xy_distance;
				if (slope > 0.45f && delta_z > 6.f)
				{
					setControlFlags(AGENT_CONTROL_FAST_UP | AGENT_CONTROL_UP_POS);
				}
				else if (slope > 0.002f && delta_z > 0.5f)
				{
					setControlFlags(AGENT_CONTROL_UP_POS);
				}
				else if (slope < -0.45f && delta_z < -6.f && current_height > AUTOPILOT_MIN_TARGET_HEIGHT_OFF_GROUND)
				{
					setControlFlags(AGENT_CONTROL_FAST_UP | AGENT_CONTROL_UP_NEG);
				}
				else if (slope < -0.002f && delta_z < -0.5f && current_height > AUTOPILOT_MIN_TARGET_HEIGHT_OFF_GROUND)
				{
					setControlFlags(AGENT_CONTROL_UP_NEG);
				}
			}
		}

		//  calculate delta rotation to target heading
		F32 delta_target_heading = angle_between(mFrameAgent.getAtAxis(), mAutoPilotTargetFacing);

		if (xy_distance > slow_distance && yaw < (F_PI / 10.f))
		{
			// walking/flying fast
			setControlFlags(AGENT_CONTROL_FAST_AT | AGENT_CONTROL_AT_POS);
		}
		else if (mAutoPilotTargetDist > mAutoPilotStopDistance)
		{
			// walking/flying slow
			if (at * direction > 0.9f)
			{
				setControlFlags(AGENT_CONTROL_AT_POS);
			}
			else if (at * direction < -0.9f)
			{
				setControlFlags(AGENT_CONTROL_AT_NEG);
			}
		}

		// check to see if we need to keep rotating to target orientation
		if (mAutoPilotTargetDist < mAutoPilotStopDistance)
		{
			setControlFlags(AGENT_CONTROL_STOP);
			if(!mAutoPilotUseRotation || (delta_target_heading < mAutoPilotRotationThreshold))
			{
				stopAutoPilot();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// propagate()
//-----------------------------------------------------------------------------
void LLAgent::propagate(const F32 dt)
{
	// Update UI based on agent motion
	LLFloaterMove *floater_move = LLFloaterMove::getInstance();
	if (floater_move)
	{
		floater_move->mForwardButton   ->setToggleState( mAtKey > 0 || mWalkKey > 0 );
		floater_move->mBackwardButton  ->setToggleState( mAtKey < 0 || mWalkKey < 0 );
		floater_move->mSlideLeftButton ->setToggleState( mLeftKey > 0 );
		floater_move->mSlideRightButton->setToggleState( mLeftKey < 0 );
		floater_move->mTurnLeftButton  ->setToggleState( mYawKey > 0.f );
		floater_move->mTurnRightButton ->setToggleState( mYawKey < 0.f );
		floater_move->mMoveUpButton    ->setToggleState( mUpKey > 0 );
		floater_move->mMoveDownButton  ->setToggleState( mUpKey < 0 );
	}

	// handle rotation based on keyboard levels
	const F32 YAW_RATE = 90.f * DEG_TO_RAD;				// radians per second
	yaw( YAW_RATE * mYawKey * dt );

	const F32 PITCH_RATE = 90.f * DEG_TO_RAD;			// radians per second
	pitch(PITCH_RATE * (F32) mPitchKey * dt);
	
	// handle auto-land behavior
	if (mAvatarObject)
	{
		BOOL in_air = mAvatarObject->mInAir;
		LLVector3 land_vel = getVelocity();
		land_vel.mV[VZ] = 0.f;

		if (!in_air 
			&& mUpKey < 0 
			&& land_vel.magVecSquared() < MAX_VELOCITY_AUTO_LAND_SQUARED
			&& gSavedSettings.getBOOL("AutomaticFly"))
		{
			// land automatically
			setFlying(FALSE);
		}
	}

	// clear keys
	mAtKey = 0;
	mWalkKey = 0;
	mLeftKey = 0;
	mUpKey = 0;
	mYawKey = 0.f;
	mPitchKey = 0;
}

//-----------------------------------------------------------------------------
// updateAgentPosition()
//-----------------------------------------------------------------------------
void LLAgent::updateAgentPosition(const F32 dt, const F32 yaw_radians, const S32 mouse_x, const S32 mouse_y)
{
	propagate(dt);

	// static S32 cameraUpdateCount = 0;

	rotate(yaw_radians, 0, 0, 1);
	
	//
	// Check for water and land collision, set underwater flag
	//

	updateLookAt(mouse_x, mouse_y);
}

//-----------------------------------------------------------------------------
// updateLookAt()
//-----------------------------------------------------------------------------
void LLAgent::updateLookAt(const S32 mouse_x, const S32 mouse_y)
{
	static LLVector3 last_at_axis;


	if ( mAvatarObject.isNull() )
	{
		return;
	}

	LLQuaternion av_inv_rot = ~mAvatarObject->mRoot.getWorldRotation();
	LLVector3 root_at = LLVector3::x_axis * mAvatarObject->mRoot.getWorldRotation();

	if 	((gViewerWindow->getMouseVelocityStat()->getCurrent() < 0.01f) &&
		(root_at * last_at_axis > 0.95f ))
	{
		LLVector3 vel = mAvatarObject->getVelocity();
		if (vel.magVecSquared() > 4.f)
		{
			setLookAt(LOOKAT_TARGET_IDLE, mAvatarObject, vel * av_inv_rot);
		}
		else
		{
			// *FIX: rotate mframeagent by sit object's rotation?
			LLQuaternion look_rotation = mAvatarObject->mIsSitting ? mAvatarObject->getRenderRotation() : mFrameAgent.getQuaternion(); // use camera's current rotation
			LLVector3 look_offset = LLVector3(2.f, 0.f, 0.f) * look_rotation * av_inv_rot;
			setLookAt(LOOKAT_TARGET_IDLE, mAvatarObject, look_offset);
		}
		last_at_axis = root_at;
		return;
	}

	last_at_axis = root_at;
	
	if (CAMERA_MODE_CUSTOMIZE_AVATAR == getCameraMode())
	{
		setLookAt(LOOKAT_TARGET_NONE, mAvatarObject, LLVector3(-2.f, 0.f, 0.f));	
	}
	else
	{
		// Move head based on cursor position
		ELookAtType lookAtType = LOOKAT_TARGET_NONE;
		LLVector3 headLookAxis;
		LLCoordFrame frameCamera = *((LLCoordFrame*)LLViewerCamera::getInstance());

		if (cameraMouselook())
		{
			lookAtType = LOOKAT_TARGET_MOUSELOOK;
		}
		else if (cameraThirdPerson())
		{
			// range from -.5 to .5
			F32 x_from_center = 
				((F32) mouse_x / (F32) gViewerWindow->getWindowWidth() ) - 0.5f;
			F32 y_from_center = 
				((F32) mouse_y / (F32) gViewerWindow->getWindowHeight() ) - 0.5f;

			frameCamera.yaw( - x_from_center * gSavedSettings.getF32("YawFromMousePosition") * DEG_TO_RAD);
			frameCamera.pitch( - y_from_center * gSavedSettings.getF32("PitchFromMousePosition") * DEG_TO_RAD);
			lookAtType = LOOKAT_TARGET_FREELOOK;
		}

		headLookAxis = frameCamera.getAtAxis();
		// RN: we use world-space offset for mouselook and freelook
		//headLookAxis = headLookAxis * av_inv_rot;
		setLookAt(lookAtType, mAvatarObject, headLookAxis);
	}
}

// friends and operators

std::ostream& operator<<(std::ostream &s, const LLAgent &agent)
{
	// This is unfinished, but might never be used. 
	// We'll just leave it for now; we can always delete it.
	s << " { "
	  << "  Frame = " << agent.mFrameAgent << "\n"
	  << " }";
	return s;
}


// ------------------- Beginning of legacy LLCamera hack ----------------------
// This section is included for legacy LLCamera support until
// it is no longer needed.  Some legacy code must exist in 
// non-legacy functions, and is labeled with "// legacy" comments.

//-----------------------------------------------------------------------------
// setAvatarObject()
//-----------------------------------------------------------------------------
void LLAgent::setAvatarObject(LLVOAvatar *avatar)			
{ 
	mAvatarObject = avatar;

	if (!avatar)
	{
		llinfos << "Setting LLAgent::mAvatarObject to NULL" << llendl;
		return;
	}

	if (!mLookAt)
	{
		mLookAt = (LLHUDEffectLookAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_LOOKAT);
	}
	if (!mPointAt)
	{
		mPointAt = (LLHUDEffectPointAt *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINTAT);
	}
	
	if (!mLookAt.isNull())
	{
		mLookAt->setSourceObject(avatar);
	}
	if (!mPointAt.isNull())
	{
		mPointAt->setSourceObject(avatar);
	}

	sendAgentWearablesRequest();
}

// TRUE if your own avatar needs to be rendered.  Usually only
// in third person and build.
//-----------------------------------------------------------------------------
// needsRenderAvatar()
//-----------------------------------------------------------------------------
BOOL LLAgent::needsRenderAvatar()
{
	if (cameraMouselook() && !LLVOAvatar::sVisibleInFirstPerson)
	{
		return FALSE;
	}

	return mShowAvatar && mGenderChosen;
}

// TRUE if we need to render your own avatar's head.
BOOL LLAgent::needsRenderHead()
{
	return mShowAvatar && !cameraMouselook();
}

//-----------------------------------------------------------------------------
// startTyping()
//-----------------------------------------------------------------------------
void LLAgent::startTyping()
{
	mTypingTimer.reset();

	if (getRenderState() & AGENT_STATE_TYPING)
	{
		// already typing, don't trigger a different animation
		return;
	}
	setRenderState(AGENT_STATE_TYPING);

	if (mChatTimer.getElapsedTimeF32() < 2.f)
	{
		LLViewerObject* chatter = gObjectList.findObject(mLastChatterID);
		if (chatter && chatter->isAvatar())
		{
			gAgent.setLookAt(LOOKAT_TARGET_RESPOND, chatter, LLVector3::zero);
		}
	}

	if (gSavedSettings.getBOOL("PlayTypingAnim"))
	{
		sendAnimationRequest(ANIM_AGENT_TYPE, ANIM_REQUEST_START);
	}
	gChatBar->sendChatFromViewer("", CHAT_TYPE_START, FALSE);
}

//-----------------------------------------------------------------------------
// stopTyping()
//-----------------------------------------------------------------------------
void LLAgent::stopTyping()
{
	if (mRenderState & AGENT_STATE_TYPING)
	{
		clearRenderState(AGENT_STATE_TYPING);
		sendAnimationRequest(ANIM_AGENT_TYPE, ANIM_REQUEST_STOP);
		gChatBar->sendChatFromViewer("", CHAT_TYPE_STOP, FALSE);
	}
}

//-----------------------------------------------------------------------------
// setRenderState()
//-----------------------------------------------------------------------------
void LLAgent::setRenderState(U8 newstate)
{
	mRenderState |= newstate;
}

//-----------------------------------------------------------------------------
// clearRenderState()
//-----------------------------------------------------------------------------
void LLAgent::clearRenderState(U8 clearstate)
{
	mRenderState &= ~clearstate;
}


//-----------------------------------------------------------------------------
// getRenderState()
//-----------------------------------------------------------------------------
U8 LLAgent::getRenderState()
{
	if (gNoRender || gKeyboard == NULL)
	{
		return 0;
	}

	// *FIX: don't do stuff in a getter!  This is infinite loop city!
	if ((mTypingTimer.getElapsedTimeF32() > TYPING_TIMEOUT_SECS) 
		&& (mRenderState & AGENT_STATE_TYPING))
	{
		stopTyping();
	}
	
	if ((!LLSelectMgr::getInstance()->getSelection()->isEmpty() && LLSelectMgr::getInstance()->shouldShowSelection())
		|| LLToolMgr::getInstance()->getCurrentTool()->isEditing() )
	{
		setRenderState(AGENT_STATE_EDITING);
	}
	else
	{
		clearRenderState(AGENT_STATE_EDITING);
	}

	return mRenderState;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static const LLFloaterView::skip_list_t& get_skip_list()
{
	static LLFloaterView::skip_list_t skip_list;
	skip_list.insert(gFloaterMap);
	return skip_list;
}

//-----------------------------------------------------------------------------
// endAnimationUpdateUI()
//-----------------------------------------------------------------------------
void LLAgent::endAnimationUpdateUI()
{
	if (mCameraMode == mLastCameraMode)
	{
		// We're already done endAnimationUpdateUI for this transition.
		return;
	}

	// clean up UI from mode we're leaving
	if ( mLastCameraMode == CAMERA_MODE_MOUSELOOK )
	{
		// show mouse cursor
		gViewerWindow->showCursor();
		// show menus
		gMenuBarView->setVisible(TRUE);
		gStatusBar->setVisibleForMouselook(true);

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);

		// Only pop if we have pushed...
		if (TRUE == mViewsPushed)
		{
			mViewsPushed = FALSE;
			gFloaterView->popVisibleAll(get_skip_list());
		}

		gAgent.setLookAt(LOOKAT_TARGET_CLEAR);
		if( gMorphView )
		{
			gMorphView->setVisible( FALSE );
		}

		// Disable mouselook-specific animations
		if (mAvatarObject)
		{
			if( mAvatarObject->isAnyAnimationSignaled(AGENT_GUN_AIM_ANIMS, NUM_AGENT_GUN_AIM_ANIMS) )
			{
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_AIM_RIFLE_R) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_AIM_RIFLE_R, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_HOLD_RIFLE_R, ANIM_REQUEST_START);
				}
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_AIM_HANDGUN_R) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_AIM_HANDGUN_R, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_HOLD_HANDGUN_R, ANIM_REQUEST_START);
				}
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_AIM_BAZOOKA_R) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_AIM_BAZOOKA_R, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_HOLD_BAZOOKA_R, ANIM_REQUEST_START);
				}
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_AIM_BOW_L) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_AIM_BOW_L, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_HOLD_BOW_L, ANIM_REQUEST_START);
				}
			}
		}
	}
	else
	if(	mLastCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR )
	{
		// make sure we ask to save changes

		LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);

		// HACK: If we're quitting, and we were in customize avatar, don't
		// let the mini-map go visible again. JC
        if (!LLAppViewer::instance()->quitRequested())
		{
			gFloaterMap->popVisible();
		}

		if( gMorphView )
		{
			gMorphView->setVisible( FALSE );
		}

		if (mAvatarObject)
		{
			if(mCustomAnim)
			{
				sendAnimationRequest(ANIM_AGENT_CUSTOMIZE, ANIM_REQUEST_STOP);
				sendAnimationRequest(ANIM_AGENT_CUSTOMIZE_DONE, ANIM_REQUEST_START);

				mCustomAnim = FALSE ;
			}
			
		}
		setLookAt(LOOKAT_TARGET_CLEAR);
	}

	//---------------------------------------------------------------------
	// Set up UI for mode we're entering
	//---------------------------------------------------------------------
	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		// hide menus
		gMenuBarView->setVisible(FALSE);
		gStatusBar->setVisibleForMouselook(false);

		// clear out camera lag effect
		mCameraLag.clearVec();

		// JC - Added for always chat in third person option
		gFocusMgr.setKeyboardFocus(NULL);

		LLToolMgr::getInstance()->setCurrentToolset(gMouselookToolset);

		mViewsPushed = TRUE;

		gFloaterView->pushVisibleAll(FALSE, get_skip_list());

		if( gMorphView )
		{
			gMorphView->setVisible(FALSE);
		}

		gIMMgr->setFloaterOpen( FALSE );
		gConsole->setVisible( TRUE );

		if (mAvatarObject)
		{
			// Trigger mouselook-specific animations
			if( mAvatarObject->isAnyAnimationSignaled(AGENT_GUN_HOLD_ANIMS, NUM_AGENT_GUN_HOLD_ANIMS) )
			{
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_HOLD_RIFLE_R) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_HOLD_RIFLE_R, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_AIM_RIFLE_R, ANIM_REQUEST_START);
				}
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_HOLD_HANDGUN_R) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_HOLD_HANDGUN_R, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_AIM_HANDGUN_R, ANIM_REQUEST_START);
				}
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_HOLD_BAZOOKA_R) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_HOLD_BAZOOKA_R, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_AIM_BAZOOKA_R, ANIM_REQUEST_START);
				}
				if (mAvatarObject->mSignaledAnimations.find(ANIM_AGENT_HOLD_BOW_L) != mAvatarObject->mSignaledAnimations.end())
				{
					sendAnimationRequest(ANIM_AGENT_HOLD_BOW_L, ANIM_REQUEST_STOP);
					sendAnimationRequest(ANIM_AGENT_AIM_BOW_L, ANIM_REQUEST_START);
				}
			}
			if (mAvatarObject->getParent())
			{
				LLVector3 at_axis = LLViewerCamera::getInstance()->getAtAxis();
				LLViewerObject* root_object = (LLViewerObject*)mAvatarObject->getRoot();
				if (root_object->flagCameraDecoupled())
				{
					resetAxes(at_axis);
				}
				else
				{
					resetAxes(at_axis * ~((LLViewerObject*)mAvatarObject->getParent())->getRenderRotation());
				}
			}
		}

	}
	else if (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		LLToolMgr::getInstance()->setCurrentToolset(gFaceEditToolset);

		gFloaterMap->pushVisible(FALSE);
		/*
		LLView *view;
		for (view = gFloaterView->getFirstChild(); view; view = gFloaterView->getNextChild())
		{
			view->pushVisible(FALSE);
		}
		*/

		if( gMorphView )
		{
			gMorphView->setVisible( TRUE );
		}

		// freeze avatar
		if (mAvatarObject)
		{
			mPauseRequest = mAvatarObject->requestPause();
		}
	}

	if (getAvatarObject())
	{
		getAvatarObject()->updateAttachmentVisibility(mCameraMode);
	}

	gFloaterTools->dirty();

	// Don't let this be called more than once if the camera
	// mode hasn't changed.  --JC
	mLastCameraMode = mCameraMode;

}


//-----------------------------------------------------------------------------
// updateCamera()
//-----------------------------------------------------------------------------
void LLAgent::updateCamera()
{
	//Ventrella - changed camera_skyward to the new global "mCameraUpVector"
	mCameraUpVector = LLVector3::z_axis;
	//LLVector3	camera_skyward(0.f, 0.f, 1.f);
	//end Ventrella

	U32 camera_mode = mCameraAnimating ? mLastCameraMode : mCameraMode;

	validateFocusObject();

	if (!mAvatarObject.isNull() && 
		mAvatarObject->mIsSitting &&
		camera_mode == CAMERA_MODE_MOUSELOOK)
	{
		//Ventrella
		//changed camera_skyward to the new global "mCameraUpVector"
		mCameraUpVector = mCameraUpVector * mAvatarObject->getRenderRotation();
		//end Ventrella
	}

	if (cameraThirdPerson() && mFocusOnAvatar && LLFollowCamMgr::getActiveFollowCamParams())
	{
		changeCameraToFollow();
	}

	//Ventrella
	//NOTE - this needs to be integrated into a general upVector system here within llAgent. 
	if ( camera_mode == CAMERA_MODE_FOLLOW && mFocusOnAvatar )
	{
		mCameraUpVector = mFollowCam.getUpVector();
	}
	//end Ventrella

	if (mSitCameraEnabled)
	{
		if (mSitCameraReferenceObject->isDead())
		{
			setSitCamera(LLUUID::null);
		}
	}

	// Update UI with our camera inputs
	LLFloaterCamera::getInstance()->mRotate->setToggleState(
		mOrbitRightKey > 0.f,	// left
		mOrbitUpKey > 0.f,		// top
		mOrbitLeftKey > 0.f,	// right
		mOrbitDownKey > 0.f);	// bottom

	LLFloaterCamera::getInstance()->mZoom->setToggleState( 
		mOrbitInKey > 0.f,		// top
		mOrbitOutKey > 0.f);	// bottom

	LLFloaterCamera::getInstance()->mTrack->setToggleState(
		mPanLeftKey > 0.f,		// left
		mPanUpKey > 0.f,		// top
		mPanRightKey > 0.f,		// right
		mPanDownKey > 0.f);		// bottom

	// Handle camera movement based on keyboard.
	const F32 ORBIT_OVER_RATE = 90.f * DEG_TO_RAD;			// radians per second
	const F32 ORBIT_AROUND_RATE = 90.f * DEG_TO_RAD;		// radians per second
	const F32 PAN_RATE = 5.f;								// meters per second

	if( mOrbitUpKey || mOrbitDownKey )
	{
		F32 input_rate = mOrbitUpKey - mOrbitDownKey;
		cameraOrbitOver( input_rate * ORBIT_OVER_RATE / gFPSClamped );
	}

	if( mOrbitLeftKey || mOrbitRightKey)
	{
		F32 input_rate = mOrbitLeftKey - mOrbitRightKey;
		cameraOrbitAround( input_rate * ORBIT_AROUND_RATE / gFPSClamped );
	}

	if( mOrbitInKey || mOrbitOutKey )
	{
		F32 input_rate = mOrbitInKey - mOrbitOutKey;
		
		LLVector3d to_focus = gAgent.getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin()) - calcFocusPositionTargetGlobal();
		F32 distance_to_focus = (F32)to_focus.magVec();
		// Move at distance (in meters) meters per second
		cameraOrbitIn( input_rate * distance_to_focus / gFPSClamped );
	}

	if( mPanInKey || mPanOutKey )
	{
		F32 input_rate = mPanInKey - mPanOutKey;
		cameraPanIn( input_rate * PAN_RATE / gFPSClamped );
	}

	if( mPanRightKey || mPanLeftKey )
	{
		F32 input_rate = mPanRightKey - mPanLeftKey;
		cameraPanLeft( input_rate * -PAN_RATE / gFPSClamped );
	}

	if( mPanUpKey || mPanDownKey )
	{
		F32 input_rate = mPanUpKey - mPanDownKey;
		cameraPanUp( input_rate * PAN_RATE / gFPSClamped );
	}

	// Clear camera keyboard keys.
	mOrbitLeftKey		= 0.f;
	mOrbitRightKey		= 0.f;
	mOrbitUpKey			= 0.f;
	mOrbitDownKey		= 0.f;
	mOrbitInKey			= 0.f;
	mOrbitOutKey		= 0.f;

	mPanRightKey		= 0.f;
	mPanLeftKey			= 0.f;
	mPanUpKey			= 0.f;
	mPanDownKey			= 0.f;
	mPanInKey			= 0.f;
	mPanOutKey			= 0.f;

	// lerp camera focus offset
	mCameraFocusOffset = lerp(mCameraFocusOffset, mCameraFocusOffsetTarget, LLCriticalDamp::getInterpolant(CAMERA_FOCUS_HALF_LIFE));

	//Ventrella
	if ( mCameraMode == CAMERA_MODE_FOLLOW )
	{
		if ( !mAvatarObject.isNull() )
		{
			//--------------------------------------------------------------------------------
			// this is where the avatar's position and rotation are given to followCam, and 
			// where it is updated. All three of its attributes are updated: (1) position, 
			// (2) focus, and (3) upvector. They can then be queried elsewhere in llAgent.
			//--------------------------------------------------------------------------------
			// *TODO: use combined rotation of frameagent and sit object
			LLQuaternion avatarRotationForFollowCam = mAvatarObject->mIsSitting ? mAvatarObject->getRenderRotation() : mFrameAgent.getQuaternion();

			LLFollowCamParams* current_cam = LLFollowCamMgr::getActiveFollowCamParams();
			if (current_cam)
			{
				mFollowCam.copyParams(*current_cam);
				mFollowCam.setSubjectPositionAndRotation( mAvatarObject->getRenderPosition(), avatarRotationForFollowCam );
				mFollowCam.update();
			}
			else
			{
				changeCameraToThirdPerson(TRUE);
			}
		}
	}
	// end Ventrella

	BOOL hit_limit;
	LLVector3d camera_pos_global;
	LLVector3d camera_target_global = calcCameraPositionTargetGlobal(&hit_limit);
	mCameraVirtualPositionAgent = getPosAgentFromGlobal(camera_target_global);
	LLVector3d focus_target_global = calcFocusPositionTargetGlobal();

	// perform field of view correction
	mCameraFOVZoomFactor = calcCameraFOVZoomFactor();
	camera_target_global = focus_target_global + (camera_target_global - focus_target_global) * (1.f + mCameraFOVZoomFactor);

	mShowAvatar = TRUE; // can see avatar by default

	// Adjust position for animation
	if (mCameraAnimating)
	{
		F32 time = mAnimationTimer.getElapsedTimeF32();

		// yet another instance of critically damped motion, hooray!
		// F32 fraction_of_animation = 1.f - pow(2.f, -time / CAMERA_ZOOM_HALF_LIFE);

		// linear interpolation
		F32 fraction_of_animation = time / mAnimationDuration;

		BOOL isfirstPerson = mCameraMode == CAMERA_MODE_MOUSELOOK;
		BOOL wasfirstPerson = mLastCameraMode == CAMERA_MODE_MOUSELOOK;
		F32 fraction_animation_to_skip;

		if (mAnimationCameraStartGlobal == camera_target_global)
		{
			fraction_animation_to_skip = 0.f;
		}
		else
		{
			LLVector3d cam_delta = mAnimationCameraStartGlobal - camera_target_global;
			fraction_animation_to_skip = HEAD_BUFFER_SIZE / (F32)cam_delta.magVec();
		}
		F32 animation_start_fraction = (wasfirstPerson) ? fraction_animation_to_skip : 0.f;
		F32 animation_finish_fraction =  (isfirstPerson) ? (1.f - fraction_animation_to_skip) : 1.f;
	
		if (fraction_of_animation < animation_finish_fraction)
		{
			if (fraction_of_animation < animation_start_fraction || fraction_of_animation > animation_finish_fraction )
			{
				mShowAvatar = FALSE;
			}

			// ...adjust position for animation
			F32 smooth_fraction_of_animation = llsmoothstep(0.0f, 1.0f, fraction_of_animation);
			camera_pos_global = lerp(mAnimationCameraStartGlobal, camera_target_global, smooth_fraction_of_animation);
			mFocusGlobal = lerp(mAnimationFocusStartGlobal, focus_target_global, smooth_fraction_of_animation);
		}
		else
		{
			// ...animation complete
			mCameraAnimating = FALSE;

			camera_pos_global = camera_target_global;
			mFocusGlobal = focus_target_global;

			endAnimationUpdateUI();
			mShowAvatar = TRUE;
		}

		if (getAvatarObject() && mCameraMode != CAMERA_MODE_MOUSELOOK)
		{
			getAvatarObject()->updateAttachmentVisibility(mCameraMode);
		}
	}
	else 
	{
		camera_pos_global = camera_target_global;
		mFocusGlobal = focus_target_global;
		mShowAvatar = TRUE;
	}

	// smoothing
	if (TRUE) 	
	{
		LLVector3d agent_pos = getPositionGlobal();
		LLVector3d camera_pos_agent = camera_pos_global - agent_pos;
		
		if (cameraThirdPerson()) // only smooth in third person mode
		{
			F32 smoothing = llclampf(1.f - pow(2.f, -4.f * gSavedSettings.getF32("CameraPositionSmoothing") / gFPSClamped));
			// we use average FPS instead of LLCriticalDamp b/c exact frame time is jittery

					
			if (!mFocusObject)  // we differentiate on avatar mode 
			{
				// for avatar-relative focus, we smooth in avatar space -
				// the avatar moves too jerkily w/r/t global space to smooth there.

				LLVector3d delta = camera_pos_agent - mCameraSmoothingLastPositionAgent;
				if (delta.magVec() < MAX_CAMERA_SMOOTH_DISTANCE)  // only smooth over short distances please
				{
					camera_pos_agent = lerp(camera_pos_agent, mCameraSmoothingLastPositionAgent, smoothing);
					camera_pos_global = camera_pos_agent + agent_pos;
				}
			}
			else
			{
				LLVector3d delta = camera_pos_global - mCameraSmoothingLastPositionGlobal;
				if (delta.magVec() < MAX_CAMERA_SMOOTH_DISTANCE) // only smooth over short distances please
				{
					camera_pos_global = lerp(camera_pos_global, mCameraSmoothingLastPositionGlobal, smoothing);
				}
			}
		}
								 
		mCameraSmoothingLastPositionGlobal = camera_pos_global;
		mCameraSmoothingLastPositionAgent = camera_pos_agent;
	}

	
	mCameraCurrentFOVZoomFactor = lerp(mCameraCurrentFOVZoomFactor, mCameraFOVZoomFactor, LLCriticalDamp::getInterpolant(FOV_ZOOM_HALF_LIFE));

//	llinfos << "Current FOV Zoom: " << mCameraCurrentFOVZoomFactor << " Target FOV Zoom: " << mCameraFOVZoomFactor << " Object penetration: " << mFocusObjectDist << llendl;

	F32 ui_offset = 0.f;
	if( CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode ) 
	{
		ui_offset = calcCustomizeAvatarUIOffset( camera_pos_global );
	}


	LLVector3 focus_agent = getPosAgentFromGlobal(mFocusGlobal);
	
	mCameraPositionAgent	= getPosAgentFromGlobal(camera_pos_global);

	// Move the camera

	//Ventrella
	LLViewerCamera::getInstance()->updateCameraLocation(mCameraPositionAgent, mCameraUpVector, focus_agent);
	//LLViewerCamera::getInstance()->updateCameraLocation(mCameraPositionAgent, camera_skyward, focus_agent);
	//end Ventrella

	//RN: translate UI offset after camera is oriented properly
	LLViewerCamera::getInstance()->translate(LLViewerCamera::getInstance()->getLeftAxis() * ui_offset);
	
	// Change FOV
	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / (1.f + mCameraCurrentFOVZoomFactor));

	// follow camera when in customize mode
	if (cameraCustomizeAvatar())	
	{
		setLookAt(LOOKAT_TARGET_FOCUS, NULL, mCameraPositionAgent);
	}

	// Send the camera position to the spatialized voice system.
	if(gVoiceClient && getRegion())
	{
		LLMatrix3 rot;
		rot.setRows(LLViewerCamera::getInstance()->getAtAxis(), LLViewerCamera::getInstance()->getLeftAxis (),  LLViewerCamera::getInstance()->getUpAxis());		

		// MBW -- XXX -- Setting velocity to 0 for now.  May figure it out later...
		gVoiceClient->setCameraPosition(
				getRegion()->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin()),// position
				LLVector3::zero, 			// velocity
				rot);						// rotation matrix
	}

	// update the travel distance stat
	// this isn't directly related to the camera
	// but this seemed like the best place to do this
	LLVector3d global_pos = getPositionGlobal(); 
	if (! mLastPositionGlobal.isExactlyZero())
	{
		LLVector3d delta = global_pos - mLastPositionGlobal;
		mDistanceTraveled += delta.magVec();
	}
	mLastPositionGlobal = global_pos;
	
	if (LLVOAvatar::sVisibleInFirstPerson && mAvatarObject.notNull() && !mAvatarObject->mIsSitting && cameraMouselook())
	{
		LLVector3 head_pos = mAvatarObject->mHeadp->getWorldPosition() + 
			LLVector3(0.08f, 0.f, 0.05f) * mAvatarObject->mHeadp->getWorldRotation() + 
			LLVector3(0.1f, 0.f, 0.f) * mAvatarObject->mPelvisp->getWorldRotation();
		LLVector3 diff = mCameraPositionAgent - head_pos;
		diff = diff * ~mAvatarObject->mRoot.getWorldRotation();

		LLJoint* torso_joint = mAvatarObject->mTorsop;
		LLJoint* chest_joint = mAvatarObject->mChestp;
		LLVector3 torso_scale = torso_joint->getScale();
		LLVector3 chest_scale = chest_joint->getScale();

		// shorten avatar skeleton to avoid foot interpenetration
		if (!mAvatarObject->mInAir)
		{
			LLVector3 chest_offset = LLVector3(0.f, 0.f, chest_joint->getPosition().mV[VZ]) * torso_joint->getWorldRotation();
			F32 z_compensate = llclamp(-diff.mV[VZ], -0.2f, 1.f);
			F32 scale_factor = llclamp(1.f - ((z_compensate * 0.5f) / chest_offset.mV[VZ]), 0.5f, 1.2f);
			torso_joint->setScale(LLVector3(1.f, 1.f, scale_factor));

			LLJoint* neck_joint = mAvatarObject->mNeckp;
			LLVector3 neck_offset = LLVector3(0.f, 0.f, neck_joint->getPosition().mV[VZ]) * chest_joint->getWorldRotation();
			scale_factor = llclamp(1.f - ((z_compensate * 0.5f) / neck_offset.mV[VZ]), 0.5f, 1.2f);
			chest_joint->setScale(LLVector3(1.f, 1.f, scale_factor));
			diff.mV[VZ] = 0.f;
		}

		mAvatarObject->mPelvisp->setPosition(mAvatarObject->mPelvisp->getPosition() + diff);

		mAvatarObject->mRoot.updateWorldMatrixChildren();

		for (LLVOAvatar::attachment_map_t::iterator iter = mAvatarObject->mAttachmentPoints.begin(); 
			 iter != mAvatarObject->mAttachmentPoints.end(); )
		{
			LLVOAvatar::attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			LLViewerObject *attached_object = attachment->getObject();
			if (attached_object && !attached_object->isDead() && attached_object->mDrawable.notNull())
			{
				// clear any existing "early" movements of attachment
				attached_object->mDrawable->clearState(LLDrawable::EARLY_MOVE);
				gPipeline.updateMoveNormalAsync(attached_object->mDrawable);
				attached_object->updateText();
			}
		}

		torso_joint->setScale(torso_scale);
		chest_joint->setScale(chest_scale);
	}
}

void LLAgent::updateFocusOffset()
{
	validateFocusObject();
	if (mFocusObject.notNull())
	{
		LLVector3d obj_pos = getPosGlobalFromAgent(mFocusObject->getRenderPosition());
		mFocusObjectOffset.setVec(mFocusTargetGlobal - obj_pos);
	}
}

void LLAgent::validateFocusObject()
{
	if (mFocusObject.notNull() && 
		(mFocusObject->isDead()))
	{
		mFocusObjectOffset.clearVec();
		clearFocusObject();
		mCameraFOVZoomFactor = 0.f;
	}
}

//-----------------------------------------------------------------------------
// calcCustomizeAvatarUIOffset()
//-----------------------------------------------------------------------------
F32 LLAgent::calcCustomizeAvatarUIOffset( const LLVector3d& camera_pos_global )
{
	F32 ui_offset = 0.f;

	if( gFloaterCustomize )
	{
		const LLRect& rect = gFloaterCustomize->getRect();

		// Move the camera so that the avatar isn't covered up by this floater.
		F32 fraction_of_fov = 0.5f - (0.5f * (1.f - llmin(1.f, ((F32)rect.getWidth() / (F32)gViewerWindow->getWindowWidth()))));
		F32 apparent_angle = fraction_of_fov * LLViewerCamera::getInstance()->getView() * LLViewerCamera::getInstance()->getAspect();  // radians
		F32 offset = tan(apparent_angle);

		if( rect.mLeft < (gViewerWindow->getWindowWidth() - rect.mRight) )
		{
			// Move the avatar to the right (camera to the left)
			ui_offset = offset;
		}
		else
		{
			// Move the avatar to the left (camera to the right)
			ui_offset = -offset;
		}
	}
	F32 range = (F32)dist_vec(camera_pos_global, gAgent.getFocusGlobal());
	mUIOffset = lerp(mUIOffset, ui_offset, LLCriticalDamp::getInterpolant(0.05f));
	return mUIOffset * range;
}

//-----------------------------------------------------------------------------
// calcFocusPositionTargetGlobal()
//-----------------------------------------------------------------------------
LLVector3d LLAgent::calcFocusPositionTargetGlobal()
{
	if (mFocusObject.notNull() && mFocusObject->isDead())
	{
		clearFocusObject();
	}

	// Ventrella
	if ( mCameraMode == CAMERA_MODE_FOLLOW && mFocusOnAvatar )
	{
		mFocusTargetGlobal = gAgent.getPosGlobalFromAgent(mFollowCam.getSimulatedFocus());
		return mFocusTargetGlobal;
	}// End Ventrella 
	else if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		LLVector3d at_axis(1.0, 0.0, 0.0);
		LLQuaternion agent_rot = mFrameAgent.getQuaternion();
		if (!mAvatarObject.isNull() && mAvatarObject->getParent())
		{
			LLViewerObject* root_object = (LLViewerObject*)mAvatarObject->getRoot();
			if (!root_object->flagCameraDecoupled())
			{
				agent_rot *= ((LLViewerObject*)(mAvatarObject->getParent()))->getRenderRotation();
			}
		}
		at_axis = at_axis * agent_rot;
		mFocusTargetGlobal = calcCameraPositionTargetGlobal() + at_axis;
		return mFocusTargetGlobal;
	}
	else if (mCameraMode == CAMERA_MODE_CUSTOMIZE_AVATAR)
	{
		return mFocusTargetGlobal;
	}
	else if (!mFocusOnAvatar)
	{
		if (mFocusObject.notNull() && !mFocusObject->isDead() && mFocusObject->mDrawable.notNull())
		{
			LLDrawable* drawablep = mFocusObject->mDrawable;
			
			if (mTrackFocusObject &&
				drawablep && 
				drawablep->isActive())
			{
				if (!mFocusObject->isAvatar())
				{
					if (mFocusObject->isSelected())
					{
						gPipeline.updateMoveNormalAsync(drawablep);
					}
					else
					{
						if (drawablep->isState(LLDrawable::MOVE_UNDAMPED))
						{
							gPipeline.updateMoveNormalAsync(drawablep);
						}
						else
						{
							gPipeline.updateMoveDampedAsync(drawablep);
						}
					}
				}
			}
			// if not tracking object, update offset based on new object position
			else
			{
				updateFocusOffset();
			}
			LLVector3 focus_agent = mFocusObject->getRenderPosition() + mFocusObjectOffset;
			mFocusTargetGlobal.setVec(getPosGlobalFromAgent(focus_agent));
		}
		return mFocusTargetGlobal;
	}
	else if (mSitCameraEnabled && mAvatarObject.notNull() && mAvatarObject->mIsSitting && mSitCameraReferenceObject.notNull())
	{
		// sit camera
		LLVector3 object_pos = mSitCameraReferenceObject->getRenderPosition();
		LLQuaternion object_rot = mSitCameraReferenceObject->getRenderRotation();

		LLVector3 target_pos = object_pos + (mSitCameraFocus * object_rot);
		return getPosGlobalFromAgent(target_pos);
	}
	else
	{
		// ...offset from avatar
		LLVector3d focus_offset;
		focus_offset.setVec(gSavedSettings.getVector3("FocusOffsetDefault"));

		LLQuaternion agent_rot = mFrameAgent.getQuaternion();
		if (!mAvatarObject.isNull() && mAvatarObject->getParent())
		{
			agent_rot *= ((LLViewerObject*)(mAvatarObject->getParent()))->getRenderRotation();
		}

		focus_offset = focus_offset * agent_rot;

		return getPositionGlobal() + focus_offset;
	}
}

void LLAgent::setupSitCamera()
{
	// agent frame entering this function is in world coordinates
	if (mAvatarObject.notNull() && mAvatarObject->getParent())
	{
		LLQuaternion parent_rot = ((LLViewerObject*)mAvatarObject->getParent())->getRenderRotation();
		// slam agent coordinate frame to proper parent local version
		LLVector3 at_axis = mFrameAgent.getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normVec();
		resetAxes(at_axis * ~parent_rot);
	}
}

//-----------------------------------------------------------------------------
// getCameraPositionAgent()
//-----------------------------------------------------------------------------
const LLVector3 &LLAgent::getCameraPositionAgent() const
{
	return LLViewerCamera::getInstance()->getOrigin();
}

//-----------------------------------------------------------------------------
// getCameraPositionGlobal()
//-----------------------------------------------------------------------------
LLVector3d LLAgent::getCameraPositionGlobal() const
{
	return getPosGlobalFromAgent(LLViewerCamera::getInstance()->getOrigin());
}

//-----------------------------------------------------------------------------
// calcCameraFOVZoomFactor()
//-----------------------------------------------------------------------------
F32	LLAgent::calcCameraFOVZoomFactor()
{
	LLVector3 camera_offset_dir;
	camera_offset_dir.setVec(mCameraFocusOffset);

	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		return 0.f;
	}
	else if (mFocusObject.notNull() && !mFocusObject->isAvatar())
	{
		// don't FOV zoom on mostly transparent objects
		LLVector3 focus_offset = mFocusObjectOffset;
		F32 obj_min_dist = 0.f;
		calcCameraMinDistance(obj_min_dist);
		F32 current_distance = llmax(0.001f, camera_offset_dir.magVec());

		mFocusObjectDist = obj_min_dist - current_distance;

		F32 new_fov_zoom = llclamp(mFocusObjectDist / current_distance, 0.f, 1000.f);
		return new_fov_zoom;
	}
	else // focusing on land or avatar
	{
		// keep old field of view until user changes focus explicitly
		return mCameraFOVZoomFactor;
		//return 0.f;
	}
}

//-----------------------------------------------------------------------------
// calcCameraPositionTargetGlobal()
//-----------------------------------------------------------------------------
LLVector3d LLAgent::calcCameraPositionTargetGlobal(BOOL *hit_limit)
{
	// Compute base camera position and look-at points.
	F32			camera_land_height;
	LLVector3d	frame_center_global = mAvatarObject.isNull() ? getPositionGlobal() 
															 : getPosGlobalFromAgent(mAvatarObject->mRoot.getWorldPosition());
		
	LLVector3   upAxis = getUpAxis();
	BOOL		isConstrained = FALSE;
	LLVector3d	head_offset;
	head_offset.setVec(mThirdPersonHeadOffset);

	LLVector3d camera_position_global;

	// Ventrella 
	if ( mCameraMode == CAMERA_MODE_FOLLOW && mFocusOnAvatar )
	{
		camera_position_global = gAgent.getPosGlobalFromAgent(mFollowCam.getSimulatedPosition());
	}// End Ventrella
	else if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		if (mAvatarObject.isNull() || mAvatarObject->mDrawable.isNull())
		{
			llwarns << "Null avatar drawable!" << llendl;
			return LLVector3d::zero;
		}
		head_offset.clearVec();
		if (mAvatarObject->mIsSitting && mAvatarObject->getParent())
		{
			mAvatarObject->updateHeadOffset();
			head_offset.mdV[VX] = mAvatarObject->mHeadOffset.mV[VX];
			head_offset.mdV[VY] = mAvatarObject->mHeadOffset.mV[VY];
			head_offset.mdV[VZ] = mAvatarObject->mHeadOffset.mV[VZ] + 0.1f;
			const LLMatrix4& mat = ((LLViewerObject*) mAvatarObject->getParent())->getRenderMatrix();
			camera_position_global = getPosGlobalFromAgent
								((mAvatarObject->getPosition()+
								 LLVector3(head_offset)*mAvatarObject->getRotation()) * mat);
		}
		else
		{
			head_offset.mdV[VZ] = mAvatarObject->mHeadOffset.mV[VZ];
			if (mAvatarObject->mIsSitting)
			{
				head_offset.mdV[VZ] += 0.1;
			}
			camera_position_global = getPosGlobalFromAgent(mAvatarObject->getRenderPosition());//frame_center_global;
			head_offset = head_offset * mAvatarObject->getRenderRotation();
			camera_position_global = camera_position_global + head_offset;
		}
	}
	else if (mCameraMode == CAMERA_MODE_THIRD_PERSON && mFocusOnAvatar)
	{
		LLVector3 local_camera_offset;
		F32 camera_distance = 0.f;

		if (mSitCameraEnabled 
			&& mAvatarObject.notNull() 
			&& mAvatarObject->mIsSitting 
			&& mSitCameraReferenceObject.notNull())
		{
			// sit camera
			LLVector3 object_pos = mSitCameraReferenceObject->getRenderPosition();
			LLQuaternion object_rot = mSitCameraReferenceObject->getRenderRotation();

			LLVector3 target_pos = object_pos + (mSitCameraPos * object_rot);

			camera_position_global = getPosGlobalFromAgent(target_pos);
		}
		else
		{
			local_camera_offset = mCameraZoomFraction * mCameraOffsetDefault;
			
			// are we sitting down?
			if (mAvatarObject.notNull() && mAvatarObject->getParent())
			{
				LLQuaternion parent_rot = ((LLViewerObject*)mAvatarObject->getParent())->getRenderRotation();
				// slam agent coordinate frame to proper parent local version
				LLVector3 at_axis = mFrameAgent.getAtAxis() * parent_rot;
				at_axis.mV[VZ] = 0.f;
				at_axis.normVec();
				resetAxes(at_axis * ~parent_rot);

				local_camera_offset = local_camera_offset * mFrameAgent.getQuaternion() * parent_rot;
			}
			else
			{
				local_camera_offset = mFrameAgent.rotateToAbsolute( local_camera_offset );
			}

			if (!mCameraCollidePlane.isExactlyZero() && (mAvatarObject.isNull() || !mAvatarObject->mIsSitting))
			{
				LLVector3 plane_normal;
				plane_normal.setVec(mCameraCollidePlane.mV);

				F32 offset_dot_norm = local_camera_offset * plane_normal;
				if (llabs(offset_dot_norm) < 0.001f)
				{
					offset_dot_norm = 0.001f;
				}
				
				camera_distance = local_camera_offset.normVec();

				F32 pos_dot_norm = getPosAgentFromGlobal(frame_center_global + head_offset) * plane_normal;
				
				// if agent is outside the colliding half-plane
				if (pos_dot_norm > mCameraCollidePlane.mV[VW])
				{
					// check to see if camera is on the opposite side (inside) the half-plane
					if (offset_dot_norm + pos_dot_norm < mCameraCollidePlane.mV[VW])
					{
						// diminish offset by factor to push it back outside the half-plane
						camera_distance *= (pos_dot_norm - mCameraCollidePlane.mV[VW] - CAMERA_COLLIDE_EPSILON) / -offset_dot_norm;
					}
				}
				else
				{
					if (offset_dot_norm + pos_dot_norm > mCameraCollidePlane.mV[VW])
					{
						camera_distance *= (mCameraCollidePlane.mV[VW] - pos_dot_norm - CAMERA_COLLIDE_EPSILON) / offset_dot_norm;
					}
				}
			}
			else
			{
				camera_distance = local_camera_offset.normVec();
			}

			mTargetCameraDistance = llmax(camera_distance, MIN_CAMERA_DISTANCE);

			if (mTargetCameraDistance != mCurrentCameraDistance)
			{
				F32 camera_lerp_amt = LLCriticalDamp::getInterpolant(CAMERA_ZOOM_HALF_LIFE);

				mCurrentCameraDistance = lerp(mCurrentCameraDistance, mTargetCameraDistance, camera_lerp_amt);
			}

			// Make the camera distance current
			local_camera_offset *= mCurrentCameraDistance;

			// set the global camera position
			LLVector3d camera_offset;
			
			LLVector3 av_pos = mAvatarObject.isNull() ? LLVector3::zero : mAvatarObject->getRenderPosition();
			camera_offset.setVec( local_camera_offset );
			camera_position_global = frame_center_global + head_offset + camera_offset;

			if (!mAvatarObject.isNull())
			{
				LLVector3d camera_lag_d;
				F32 lag_interp = LLCriticalDamp::getInterpolant(CAMERA_LAG_HALF_LIFE);
				LLVector3 target_lag;
				LLVector3 vel = getVelocity();

				// lag by appropriate amount for flying
				F32 time_in_air = mAvatarObject->mTimeInAir.getElapsedTimeF32();
				if(!mCameraAnimating && mAvatarObject->mInAir && time_in_air > GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME)
				{
					LLVector3 frame_at_axis = mFrameAgent.getAtAxis();
					frame_at_axis -= projected_vec(frame_at_axis, getReferenceUpVector());
					frame_at_axis.normVec();

					//transition smoothly in air mode, to avoid camera pop
					F32 u = (time_in_air - GROUND_TO_AIR_CAMERA_TRANSITION_START_TIME) / GROUND_TO_AIR_CAMERA_TRANSITION_TIME;
					u = llclamp(u, 0.f, 1.f);

					lag_interp *= u;

					if (gViewerWindow->getLeftMouseDown() && gLastHitObjectID == mAvatarObject->getID())
					{
						// disable camera lag when using mouse-directed steering
						target_lag.clearVec();
					}
					else
					{
						target_lag = vel * gSavedSettings.getF32("DynamicCameraStrength") / 30.f;
					}

					mCameraLag = lerp(mCameraLag, target_lag, lag_interp);

					F32 lag_dist = mCameraLag.magVec();
					if (lag_dist > MAX_CAMERA_LAG)
					{
						mCameraLag = mCameraLag * MAX_CAMERA_LAG / lag_dist;
					}

					// clamp camera lag so that avatar is always in front
					F32 dot = (mCameraLag - (frame_at_axis * (MIN_CAMERA_LAG * u))) * frame_at_axis;
					if (dot < -(MIN_CAMERA_LAG * u))
					{
						mCameraLag -= (dot + (MIN_CAMERA_LAG * u)) * frame_at_axis;
					}
				}
				else
				{
					mCameraLag = lerp(mCameraLag, LLVector3::zero, LLCriticalDamp::getInterpolant(0.15f));
				}

				camera_lag_d.setVec(mCameraLag);
				camera_position_global = camera_position_global - camera_lag_d;
			}
		}
	}
	else
	{
		LLVector3d focusPosGlobal = calcFocusPositionTargetGlobal();
		// camera gets pushed out later wrt mCameraFOVZoomFactor...this is "raw" value
		camera_position_global = focusPosGlobal + mCameraFocusOffset;
	}

	if (!gSavedSettings.getBOOL("DisableCameraConstraints") && !gAgent.isGodlike())
	{
		LLViewerRegion* regionp = LLWorld::getInstance()->getRegionFromPosGlobal(
			camera_position_global);
		bool constrain = true;
		if(regionp && regionp->canManageEstate())
		{
			constrain = false;
		}
		if(constrain)
		{
			F32 max_dist = ( CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode ) ?
				APPEARANCE_MAX_ZOOM : MAX_CAMERA_DISTANCE_FROM_AGENT;

			LLVector3d camera_offset = camera_position_global
				- gAgent.getPositionGlobal();
			F32 camera_distance = (F32)camera_offset.magVec();

			if(camera_distance > max_dist)
			{
				camera_position_global = gAgent.getPositionGlobal() + 
					(max_dist / camera_distance) * camera_offset;
				isConstrained = TRUE;
			}
		}

// JC - Could constrain camera based on parcel stuff here.
//			LLViewerRegion *regionp = LLWorld::getInstance()->getRegionFromPosGlobal(camera_position_global);
//			
//			if (regionp && !regionp->mParcelOverlay->isBuildCameraAllowed(regionp->getPosRegionFromGlobal(camera_position_global)))
//			{
//				camera_position_global = last_position_global;
//
//				isConstrained = TRUE;
//			}
	}

	// Don't let camera go underground
	F32 camera_min_off_ground = getCameraMinOffGround();

	camera_land_height = LLWorld::getInstance()->resolveLandHeightGlobal(camera_position_global);

	if (camera_position_global.mdV[VZ] < camera_land_height + camera_min_off_ground)
	{
		camera_position_global.mdV[VZ] = camera_land_height + camera_min_off_ground;
		isConstrained = TRUE;
	}


	if (hit_limit)
	{
		*hit_limit = isConstrained;
	}

	return camera_position_global;
}


//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
void LLAgent::handleScrollWheel(S32 clicks)
{
	if ( mCameraMode == CAMERA_MODE_FOLLOW && gAgent.getFocusOnAvatar())
	{
		if ( ! mFollowCam.getPositionLocked() ) // not if the followCam position is locked in place
		{
			mFollowCam.zoom( clicks ); 
			if ( mFollowCam.isZoomedToMinimumDistance() )
			{
				changeCameraToMouselook(FALSE);
			}
		}
	}
	else
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();
		const F32 ROOT_ROOT_TWO = sqrt(F_SQRT2);

		// Block if camera is animating
		if (mCameraAnimating)
		{
			return;
		}

		if (selection->getObjectCount() && selection->getSelectType() == SELECT_TYPE_HUD)
		{
			F32 zoom_factor = (F32)pow(0.8, -clicks);
			cameraZoomIn(zoom_factor);
		}
		else if (mFocusOnAvatar && mCameraMode == CAMERA_MODE_THIRD_PERSON)
		{
			F32 current_zoom_fraction = mTargetCameraDistance / mCameraOffsetDefault.magVec();
			current_zoom_fraction *= 1.f - pow(ROOT_ROOT_TWO, clicks);
			
			cameraOrbitIn(current_zoom_fraction * mCameraOffsetDefault.magVec());
		}
		else
		{
			F32 current_zoom_fraction = (F32)mCameraFocusOffsetTarget.magVec();
			cameraOrbitIn(current_zoom_fraction * (1.f - pow(ROOT_ROOT_TWO, clicks)));
		}
	}
}


//-----------------------------------------------------------------------------
// getCameraMinOffGround()
//-----------------------------------------------------------------------------
F32 LLAgent::getCameraMinOffGround()
{
	if (mCameraMode == CAMERA_MODE_MOUSELOOK)
	{
		return 0.f;
	}
	else
	{
		if (gSavedSettings.getBOOL("DisableCameraConstraints"))
		{
			return -1000.f;
		}
		else
		{
			return 0.5f;
		}
	}
}


//-----------------------------------------------------------------------------
// resetCamera()
//-----------------------------------------------------------------------------
void LLAgent::resetCamera()
{
	// Remove any pitch from the avatar
	LLVector3 at = mFrameAgent.getAtAxis();
	at.mV[VZ] = 0.f;
	at.normVec();
	gAgent.resetAxes(at);
	// have to explicitly clear field of view zoom now
	mCameraFOVZoomFactor = 0.f;

	updateCamera();
}

//-----------------------------------------------------------------------------
// changeCameraToMouselook()
//-----------------------------------------------------------------------------
void LLAgent::changeCameraToMouselook(BOOL animate)
{
	// visibility changes at end of animation
	gViewerWindow->getWindow()->resetBusyCount();

	// unpause avatar animation
	mPauseRequest = NULL;

	LLToolMgr::getInstance()->setCurrentToolset(gMouselookToolset);

	gSavedSettings.setBOOL("FirstPersonBtnState",	FALSE);
	gSavedSettings.setBOOL("MouselookBtnState",		TRUE);
	gSavedSettings.setBOOL("ThirdPersonBtnState",	FALSE);
	gSavedSettings.setBOOL("BuildBtnState",			FALSE);

	if (mAvatarObject)
	{
		mAvatarObject->stopMotion( ANIM_AGENT_BODY_NOISE );
		mAvatarObject->stopMotion( ANIM_AGENT_BREATHE_ROT );
	}

	//gViewerWindow->stopGrab();
	LLSelectMgr::getInstance()->deselectAll();
	gViewerWindow->hideCursor();
	gViewerWindow->moveCursorToCenter();

	if( mCameraMode != CAMERA_MODE_MOUSELOOK )
	{
		gViewerWindow->setKeyboardFocus( NULL );
		
		mLastCameraMode = mCameraMode;
		mCameraMode = CAMERA_MODE_MOUSELOOK;
		U32 old_flags = mControlFlags;
		setControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != mControlFlags)
		{
			mbFlagsDirty = TRUE;
		}

		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			mCameraAnimating = FALSE;
			endAnimationUpdateUI();
		}
	}
}


//-----------------------------------------------------------------------------
// changeCameraToDefault()
//-----------------------------------------------------------------------------
void LLAgent::changeCameraToDefault()
{
	if (LLFollowCamMgr::getActiveFollowCamParams())
	{
		changeCameraToFollow();
	}
	else
	{
		changeCameraToThirdPerson();
	}
}


// Ventrella
//-----------------------------------------------------------------------------
// changeCameraToFollow()
//-----------------------------------------------------------------------------
void LLAgent::changeCameraToFollow(BOOL animate)
{
	if( mCameraMode != CAMERA_MODE_FOLLOW )
	{
		if (mCameraMode == CAMERA_MODE_MOUSELOOK)
		{
			animate = FALSE;
		}
		startCameraAnimation();

		mLastCameraMode = mCameraMode;
		mCameraMode = CAMERA_MODE_FOLLOW;

		// bang-in the current focus, position, and up vector of the follow cam
		mFollowCam.reset( mCameraPositionAgent, LLViewerCamera::getInstance()->getPointOfInterest(), LLVector3::z_axis );
		
		if (gBasicToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}

		if (mAvatarObject)
		{
			mAvatarObject->mPelvisp->setPosition(LLVector3::zero);
			mAvatarObject->startMotion( ANIM_AGENT_BODY_NOISE );
			mAvatarObject->startMotion( ANIM_AGENT_BREATHE_ROT );
		}

		gSavedSettings.setBOOL("FirstPersonBtnState",	FALSE);
		gSavedSettings.setBOOL("MouselookBtnState",		FALSE);
		gSavedSettings.setBOOL("ThirdPersonBtnState",	TRUE);
		gSavedSettings.setBOOL("BuildBtnState",			FALSE);

		// unpause avatar animation
		mPauseRequest = NULL;

		U32 old_flags = mControlFlags;
		clearControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != mControlFlags)
		{
			mbFlagsDirty = TRUE;
		}

		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			mCameraAnimating = FALSE;
			endAnimationUpdateUI();
		}
	}
}

//-----------------------------------------------------------------------------
// changeCameraToThirdPerson()
//-----------------------------------------------------------------------------
void LLAgent::changeCameraToThirdPerson(BOOL animate)
{
//printf( "changeCameraToThirdPerson\n" );

	gViewerWindow->getWindow()->resetBusyCount();

	mCameraZoomFraction = INITIAL_ZOOM_FRACTION;

	if (mAvatarObject)
	{
		mAvatarObject->mPelvisp->setPosition(LLVector3::zero);
		mAvatarObject->startMotion( ANIM_AGENT_BODY_NOISE );
		mAvatarObject->startMotion( ANIM_AGENT_BREATHE_ROT );
	}

	gSavedSettings.setBOOL("FirstPersonBtnState",	FALSE);
	gSavedSettings.setBOOL("MouselookBtnState",		FALSE);
	gSavedSettings.setBOOL("ThirdPersonBtnState",	TRUE);
	gSavedSettings.setBOOL("BuildBtnState",			FALSE);

	LLVector3 at_axis;

	// unpause avatar animation
	mPauseRequest = NULL;

	if( mCameraMode != CAMERA_MODE_THIRD_PERSON )
	{
		if (gBasicToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(gBasicToolset);
		}

		mCameraLag.clearVec();
		if (mCameraMode == CAMERA_MODE_MOUSELOOK)
		{
			mCurrentCameraDistance = MIN_CAMERA_DISTANCE;
			mTargetCameraDistance = MIN_CAMERA_DISTANCE;
			animate = FALSE;
		}
		mLastCameraMode = mCameraMode;
		mCameraMode = CAMERA_MODE_THIRD_PERSON;
		U32 old_flags = mControlFlags;
		clearControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != mControlFlags)
		{
			mbFlagsDirty = TRUE;
		}

	}

	// Remove any pitch from the avatar
	if (!mAvatarObject.isNull() && mAvatarObject->getParent())
	{
		LLQuaternion obj_rot = ((LLViewerObject*)mAvatarObject->getParent())->getRenderRotation();
		at_axis = LLViewerCamera::getInstance()->getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normVec();
		resetAxes(at_axis * ~obj_rot);
	}
	else
	{
		at_axis = mFrameAgent.getAtAxis();
		at_axis.mV[VZ] = 0.f;
		at_axis.normVec();
		resetAxes(at_axis);
	}


	if (animate)
	{
		startCameraAnimation();
	}
	else
	{
		mCameraAnimating = FALSE;
		endAnimationUpdateUI();
	}
}

//-----------------------------------------------------------------------------
// changeCameraToCustomizeAvatar()
//-----------------------------------------------------------------------------
void LLAgent::changeCameraToCustomizeAvatar(BOOL avatar_animate, BOOL camera_animate)
{
	setControlFlags(AGENT_CONTROL_STAND_UP); // force stand up
	gViewerWindow->getWindow()->resetBusyCount();

	if (gFaceEditToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(gFaceEditToolset);
	}

	gSavedSettings.setBOOL("FirstPersonBtnState", FALSE);
	gSavedSettings.setBOOL("MouselookBtnState", FALSE);
	gSavedSettings.setBOOL("ThirdPersonBtnState", FALSE);
	gSavedSettings.setBOOL("BuildBtnState", FALSE);

	if (camera_animate)
	{
		startCameraAnimation();
	}

	// Remove any pitch from the avatar
	//LLVector3 at = mFrameAgent.getAtAxis();
	//at.mV[VZ] = 0.f;
	//at.normVec();
	//gAgent.resetAxes(at);

	if( mCameraMode != CAMERA_MODE_CUSTOMIZE_AVATAR )
	{
		mLastCameraMode = mCameraMode;
		mCameraMode = CAMERA_MODE_CUSTOMIZE_AVATAR;
		U32 old_flags = mControlFlags;
		clearControlFlags(AGENT_CONTROL_MOUSELOOK);
		if (old_flags != mControlFlags)
		{
			mbFlagsDirty = TRUE;
		}

		gViewerWindow->setKeyboardFocus( NULL );
		gViewerWindow->setMouseCapture( NULL );

		LLVOAvatar::onCustomizeStart();
	}

	if (!mAvatarObject.isNull())
	{
		if(avatar_animate)
		{
				// Remove any pitch from the avatar
			LLVector3 at = mFrameAgent.getAtAxis();
			at.mV[VZ] = 0.f;
			at.normVec();
			gAgent.resetAxes(at);

			sendAnimationRequest(ANIM_AGENT_CUSTOMIZE, ANIM_REQUEST_START);
			mCustomAnim = TRUE ;
			mAvatarObject->startMotion(ANIM_AGENT_CUSTOMIZE);
			LLMotion* turn_motion = mAvatarObject->findMotion(ANIM_AGENT_CUSTOMIZE);

			if (turn_motion)
			{
				mAnimationDuration = turn_motion->getDuration() + CUSTOMIZE_AVATAR_CAMERA_ANIM_SLOP;

			}
			else
			{
				mAnimationDuration = gSavedSettings.getF32("ZoomTime");
			}
		}



		gAgent.setFocusGlobal(LLVector3d::zero);
	}
	else
	{
		mCameraAnimating = FALSE;
		endAnimationUpdateUI();
	}

}


//
// Focus point management
//

//-----------------------------------------------------------------------------
// startCameraAnimation()
//-----------------------------------------------------------------------------
void LLAgent::startCameraAnimation()
{
	mAnimationCameraStartGlobal = getCameraPositionGlobal();
	mAnimationFocusStartGlobal = mFocusGlobal;
	mAnimationTimer.reset();
	mCameraAnimating = TRUE;
	mAnimationDuration = gSavedSettings.getF32("ZoomTime");
}

//-----------------------------------------------------------------------------
// stopCameraAnimation()
//-----------------------------------------------------------------------------
void LLAgent::stopCameraAnimation()
{
	mCameraAnimating = FALSE;
}

void LLAgent::clearFocusObject()
{
	if (mFocusObject.notNull())
	{
		startCameraAnimation();

		setFocusObject(NULL);
		mFocusObjectOffset.clearVec();
	}
}

void LLAgent::setFocusObject(LLViewerObject* object)
{
	mFocusObject = object;
}

// Focus on a point, but try to keep camera position stable.
//-----------------------------------------------------------------------------
// setFocusGlobal()
//-----------------------------------------------------------------------------
void LLAgent::setFocusGlobal(const LLVector3d& focus, const LLUUID &object_id)
{
	setFocusObject(gObjectList.findObject(object_id));
	LLVector3d old_focus = mFocusTargetGlobal;
	LLViewerObject *focus_obj = mFocusObject;

	// if focus has changed
	if (old_focus != focus)
	{
		if (focus.isExactlyZero())
		{
			if (!mAvatarObject.isNull())
			{
				mFocusTargetGlobal = getPosGlobalFromAgent(mAvatarObject->mHeadp->getWorldPosition());
			}
			else
			{
				mFocusTargetGlobal = getPositionGlobal();
			}
			mCameraFocusOffsetTarget = getCameraPositionGlobal() - mFocusTargetGlobal;
			mCameraFocusOffset = mCameraFocusOffsetTarget;
			setLookAt(LOOKAT_TARGET_CLEAR);
		}
		else
		{
			mFocusTargetGlobal = focus;
			if (!focus_obj)
			{
				mCameraFOVZoomFactor = 0.f;
			}

			mCameraFocusOffsetTarget = gAgent.getPosGlobalFromAgent(mCameraVirtualPositionAgent) - mFocusTargetGlobal;

			startCameraAnimation();

			if (focus_obj)
			{
				if (focus_obj->isAvatar())
				{
					setLookAt(LOOKAT_TARGET_FOCUS, focus_obj);
				}
				else
				{
					setLookAt(LOOKAT_TARGET_FOCUS, focus_obj, (getPosAgentFromGlobal(focus) - focus_obj->getRenderPosition()) * ~focus_obj->getRenderRotation());
				}
			}
			else
			{
				setLookAt(LOOKAT_TARGET_FOCUS, NULL, getPosAgentFromGlobal(mFocusTargetGlobal));
			}
		}
	}
	else // focus == mFocusTargetGlobal
	{
		if (focus.isExactlyZero())
		{
			if (!mAvatarObject.isNull())
			{
				mFocusTargetGlobal = getPosGlobalFromAgent(mAvatarObject->mHeadp->getWorldPosition());
			}
			else
			{
				mFocusTargetGlobal = getPositionGlobal();
			}
		}
		mCameraFocusOffsetTarget = (getCameraPositionGlobal() - mFocusTargetGlobal) / (1.f + mCameraFOVZoomFactor);;
		mCameraFocusOffset = mCameraFocusOffsetTarget;
	}

	if (mFocusObject.notNull())
	{
		// for attachments, make offset relative to avatar, not the attachment
		if (mFocusObject->isAttachment())
		{
			while (!mFocusObject->isAvatar())
			{
				mFocusObject = (LLViewerObject*) mFocusObject->getParent();
			}
			setFocusObject((LLViewerObject*)mFocusObject);
		}
		updateFocusOffset();
	}
}

// Used for avatar customization
//-----------------------------------------------------------------------------
// setCameraPosAndFocusGlobal()
//-----------------------------------------------------------------------------
void LLAgent::setCameraPosAndFocusGlobal(const LLVector3d& camera_pos, const LLVector3d& focus, const LLUUID &object_id)
{
	LLVector3d old_focus = mFocusTargetGlobal;

	F64 focus_delta_squared = (old_focus - focus).magVecSquared();
	const F64 ANIM_EPSILON_SQUARED = 0.0001;
	if( focus_delta_squared > ANIM_EPSILON_SQUARED )
	{
		startCameraAnimation();

		if( CAMERA_MODE_CUSTOMIZE_AVATAR == mCameraMode ) 
		{
			// Compensate for the fact that the camera has already been offset to make room for LLFloaterCustomize.
			mAnimationCameraStartGlobal -= LLVector3d(LLViewerCamera::getInstance()->getLeftAxis() * calcCustomizeAvatarUIOffset( mAnimationCameraStartGlobal ));
		}
	}
	
	//LLViewerCamera::getInstance()->setOrigin( gAgent.getPosAgentFromGlobal( camera_pos ) );
	setFocusObject(gObjectList.findObject(object_id));
	mFocusTargetGlobal = focus;
	mCameraFocusOffsetTarget = camera_pos - focus;
	mCameraFocusOffset = mCameraFocusOffsetTarget;

	if (mFocusObject)
	{
		if (mFocusObject->isAvatar())
		{
			setLookAt(LOOKAT_TARGET_FOCUS, mFocusObject);
		}
		else
		{
			setLookAt(LOOKAT_TARGET_FOCUS, mFocusObject, (getPosAgentFromGlobal(focus) - mFocusObject->getRenderPosition()) * ~mFocusObject->getRenderRotation());
		}
	}
	else
	{
		setLookAt(LOOKAT_TARGET_FOCUS, NULL, getPosAgentFromGlobal(mFocusTargetGlobal));
	}

	if( mCameraAnimating )
	{
		const F64 ANIM_METERS_PER_SECOND = 10.0;
		const F64 MIN_ANIM_SECONDS = 0.5;
		F64 anim_duration = llmax( MIN_ANIM_SECONDS, sqrt(focus_delta_squared) / ANIM_METERS_PER_SECOND );
		setAnimationDuration( (F32)anim_duration );
	}

	updateFocusOffset();
}

//-----------------------------------------------------------------------------
// setSitCamera()
//-----------------------------------------------------------------------------
void LLAgent::setSitCamera(const LLUUID &object_id, const LLVector3 &camera_pos, const LLVector3 &camera_focus)
{
	BOOL camera_enabled = !object_id.isNull();

	if (camera_enabled)
	{
		LLViewerObject *reference_object = gObjectList.findObject(object_id);
		if (reference_object)
		{
			//convert to root object relative?
			mSitCameraPos = camera_pos;
			mSitCameraFocus = camera_focus;
			mSitCameraReferenceObject = reference_object;
			mSitCameraEnabled = TRUE;
		}
	}
	else
	{
		mSitCameraPos.clearVec();
		mSitCameraFocus.clearVec();
		mSitCameraReferenceObject = NULL;
		mSitCameraEnabled = FALSE;
	}
}

//-----------------------------------------------------------------------------
// setFocusOnAvatar()
//-----------------------------------------------------------------------------
void LLAgent::setFocusOnAvatar(BOOL focus_on_avatar, BOOL animate)
{
	if (focus_on_avatar != mFocusOnAvatar)
	{
		if (animate)
		{
			startCameraAnimation();
		}
		else
		{
			stopCameraAnimation();
		}
	}
	
	//RN: when focused on the avatar, we're not "looking" at it
	// looking implies intent while focusing on avatar means
	// you're just walking around with a camera on you...eesh.
	if (focus_on_avatar && !mFocusOnAvatar)
	{
		setFocusGlobal(LLVector3d::zero);
		mCameraFOVZoomFactor = 0.f;
		if (mCameraMode == CAMERA_MODE_THIRD_PERSON)
		{
			LLVector3 at_axis;
			if (!mAvatarObject.isNull() && mAvatarObject->getParent())
			{
				LLQuaternion obj_rot = ((LLViewerObject*)mAvatarObject->getParent())->getRenderRotation();
				at_axis = LLViewerCamera::getInstance()->getAtAxis();
				at_axis.mV[VZ] = 0.f;
				at_axis.normVec();
				resetAxes(at_axis * ~obj_rot);
			}
			else
			{
				at_axis = LLViewerCamera::getInstance()->getAtAxis();
				at_axis.mV[VZ] = 0.f;
				at_axis.normVec();
				resetAxes(at_axis);
			}
		}
	}
	
	mFocusOnAvatar = focus_on_avatar;
}

//-----------------------------------------------------------------------------
// heardChat()
//-----------------------------------------------------------------------------
void LLAgent::heardChat(const LLUUID& id)
{
	// log text and voice chat to speaker mgr
	// for keeping track of active speakers, etc.
	LLLocalSpeakerMgr::getInstance()->speakerChatted(id);

	// don't respond to your own voice
	if (id == getID()) return;
	
	if (ll_rand(2) == 0) 
	{
		LLViewerObject *chatter = gObjectList.findObject(mLastChatterID);
		setLookAt(LOOKAT_TARGET_AUTO_LISTEN, chatter, LLVector3::zero);
	}			

	mLastChatterID = id;
	mChatTimer.reset();
}

//-----------------------------------------------------------------------------
// lookAtLastChat()
//-----------------------------------------------------------------------------
void LLAgent::lookAtLastChat()
{
	// Block if camera is animating or not in normal third person camera mode
	if (mCameraAnimating || !cameraThirdPerson())
	{
		return;
	}

	LLViewerObject *chatter = gObjectList.findObject(mLastChatterID);
	if (chatter)
	{
		LLVector3 delta_pos;
		if (chatter->isAvatar())
		{
			LLVOAvatar *chatter_av = (LLVOAvatar*)chatter;
			if (!mAvatarObject.isNull() && chatter_av->mHeadp)
			{
				delta_pos = chatter_av->mHeadp->getWorldPosition() - mAvatarObject->mHeadp->getWorldPosition();
			}
			else
			{
				delta_pos = chatter->getPositionAgent() - getPositionAgent();
			}
			delta_pos.normVec();

			setControlFlags(AGENT_CONTROL_STOP);

			changeCameraToThirdPerson();

			LLVector3 new_camera_pos = mAvatarObject->mHeadp->getWorldPosition();
			LLVector3 left = delta_pos % LLVector3::z_axis;
			left.normVec();
			LLVector3 up = left % delta_pos;
			up.normVec();
			new_camera_pos -= delta_pos * 0.4f;
			new_camera_pos += left * 0.3f;
			new_camera_pos += up * 0.2f;
			if (chatter_av->mHeadp)
			{
				setFocusGlobal(getPosGlobalFromAgent(chatter_av->mHeadp->getWorldPosition()), mLastChatterID);
				mCameraFocusOffsetTarget = getPosGlobalFromAgent(new_camera_pos) - gAgent.getPosGlobalFromAgent(chatter_av->mHeadp->getWorldPosition());
			}
			else
			{
				setFocusGlobal(chatter->getPositionGlobal(), mLastChatterID);
				mCameraFocusOffsetTarget = getPosGlobalFromAgent(new_camera_pos) - chatter->getPositionGlobal();
			}
			setFocusOnAvatar(FALSE, TRUE);
		}
		else
		{
			delta_pos = chatter->getRenderPosition() - getPositionAgent();
			delta_pos.normVec();

			setControlFlags(AGENT_CONTROL_STOP);

			changeCameraToThirdPerson();

			LLVector3 new_camera_pos = mAvatarObject->mHeadp->getWorldPosition();
			LLVector3 left = delta_pos % LLVector3::z_axis;
			left.normVec();
			LLVector3 up = left % delta_pos;
			up.normVec();
			new_camera_pos -= delta_pos * 0.4f;
			new_camera_pos += left * 0.3f;
			new_camera_pos += up * 0.2f;

			setFocusGlobal(chatter->getPositionGlobal(), mLastChatterID);
			mCameraFocusOffsetTarget = getPosGlobalFromAgent(new_camera_pos) - chatter->getPositionGlobal();
			setFocusOnAvatar(FALSE, TRUE);
		}
	}
}

const F32 SIT_POINT_EXTENTS = 0.2f;

// Grabs current position
void LLAgent::setStartPosition(U32 location_id)
{
	LLViewerObject		*object;

	if ( !(gAgentID == LLUUID::null) )
	{
		// we've got an ID for an agent viewerobject
		object = gObjectList.findObject(gAgentID);
		if (object)
		{
			// we've got the viewer object
			// Sometimes the agent can be velocity interpolated off of
			// this simulator.  Clamp it to the region the agent is
			// in, a little bit in on each side.
			const F32 INSET = 0.5f;	//meters
			const F32 REGION_WIDTH = LLWorld::getInstance()->getRegionWidthInMeters();

			LLVector3 agent_pos = getPositionAgent();

			if (mAvatarObject)
			{
				// the z height is at the agent's feet
				agent_pos.mV[VZ] -= 0.5f * mAvatarObject->mBodySize.mV[VZ];
			}

			agent_pos.mV[VX] = llclamp( agent_pos.mV[VX], INSET, REGION_WIDTH - INSET );
			agent_pos.mV[VY] = llclamp( agent_pos.mV[VY], INSET, REGION_WIDTH - INSET );

			// Don't let them go below ground, or too high.
			agent_pos.mV[VZ] = llclamp( agent_pos.mV[VZ], 
				mRegionp->getLandHeightRegion( agent_pos ), 
				LLWorld::getInstance()->getRegionMaxHeight() );

			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_SetStartLocationRequest);
			msg->nextBlockFast( _PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, getID());
			msg->addUUIDFast(_PREHASH_SessionID, getSessionID());
			msg->nextBlockFast( _PREHASH_StartLocationData);
			// corrected by sim
			msg->addStringFast(_PREHASH_SimName, "");
			msg->addU32Fast(_PREHASH_LocationID, location_id);
			msg->addVector3Fast(_PREHASH_LocationPos, agent_pos);
			msg->addVector3Fast(_PREHASH_LocationLookAt,mFrameAgent.getAtAxis());

			// Reliable only helps when setting home location.  Last
			// location is sent on quit, and we don't have time to ack
			// the packets.
			msg->sendReliable(mRegionp->getHost());

			const U32 HOME_INDEX = 1;
			if( HOME_INDEX == location_id )
			{
				setHomePosRegion( mRegionp->getHandle(), getPositionAgent() );
			}
		}
		else
		{
			llinfos << "setStartPosition - Can't find agent viewerobject id " << gAgentID << llendl;
		}
	}
}

void LLAgent::requestStopMotion( LLMotion* motion )
{
	// Notify all avatars that a motion has stopped.
	// This is needed to clear the animation state bits
	LLUUID anim_state = motion->getID();
	onAnimStop(motion->getID());

	// if motion is not looping, it could have stopped by running out of time
	// so we need to tell the server this
//	llinfos << "Sending stop for motion " << motion->getName() << llendl;
	sendAnimationRequest( anim_state, ANIM_REQUEST_STOP );
}

void LLAgent::onAnimStop(const LLUUID& id)
{
	// handle automatic state transitions (based on completion of animation playback)
	if (id == ANIM_AGENT_STAND)
	{
		stopFidget();
	}
	else if (id == ANIM_AGENT_AWAY)
	{
		clearAFK();
	}
	else if (id == ANIM_AGENT_STANDUP)
	{
		// send stand up command
		setControlFlags(AGENT_CONTROL_FINISH_ANIM);

		// now trigger dusting self off animation
		if (mAvatarObject.notNull() && !mAvatarObject->mBelowWater && rand() % 3 == 0)
			sendAnimationRequest( ANIM_AGENT_BRUSH, ANIM_REQUEST_START );
	}
	else if (id == ANIM_AGENT_PRE_JUMP || id == ANIM_AGENT_LAND || id == ANIM_AGENT_MEDIUM_LAND)
	{
		setControlFlags(AGENT_CONTROL_FINISH_ANIM);
	}
}

BOOL LLAgent::isGodlike() const
{
#ifdef HACKED_GODLIKE_VIEWER
	return TRUE;
#else
	if(mAdminOverride) return TRUE;
	return mGodLevel > GOD_NOT;
#endif
}

U8 LLAgent::getGodLevel() const
{
#ifdef HACKED_GODLIKE_VIEWER
	return GOD_MAINTENANCE;
#else
	if(mAdminOverride) return GOD_FULL;
	return mGodLevel;
#endif
}

bool LLAgent::isTeen() const
{
	return mAccess < SIM_ACCESS_MATURE;
}

void LLAgent::setTeen(bool teen)
{
	if (teen)
	{
		mAccess = SIM_ACCESS_PG;
	}
	else
	{
		mAccess = SIM_ACCESS_MATURE;
	}
}

void LLAgent::buildFullname(std::string& name) const
{
	if (mAvatarObject)
	{
		name = mAvatarObject->getFullname();
	}
}

void LLAgent::buildFullnameAndTitle(std::string& name) const
{
	if (isGroupMember())
	{
		name = mGroupTitle;
		name += ' ';
	}
	else
	{
		name.erase(0, name.length());
	}

	if (mAvatarObject)
	{
		name += mAvatarObject->getFullname();
	}
}

BOOL LLAgent::isInGroup(const LLUUID& group_id) const
{
	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			return TRUE;
		}
	}
	return FALSE;
}

// This implementation should mirror LLAgentInfo::hasPowerInGroup
BOOL LLAgent::hasPowerInGroup(const LLUUID& group_id, U64 power) const
{
	// GP_NO_POWERS can also mean no power is enough to grant an ability.
	if (GP_NO_POWERS == power) return FALSE;

	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			return (BOOL)((mGroups.get(i).mPowers & power) > 0);
		}
	}
	return FALSE;
}

BOOL LLAgent::hasPowerInActiveGroup(U64 power) const
{
	return (mGroupID.notNull() && (hasPowerInGroup(mGroupID, power)));
}

U64 LLAgent::getPowerInGroup(const LLUUID& group_id) const
{
	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			return (mGroups.get(i).mPowers);
		}
	}

	return GP_NO_POWERS;
}

BOOL LLAgent::getGroupData(const LLUUID& group_id, LLGroupData& data) const
{
	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			data = mGroups.get(i);
			return TRUE;
		}
	}
	return FALSE;
}

S32 LLAgent::getGroupContribution(const LLUUID& group_id) const
{
	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			S32 contribution = mGroups.get(i).mContribution;
			return contribution;
		}
	}
	return 0;
}

BOOL LLAgent::setGroupContribution(const LLUUID& group_id, S32 contribution)
{
	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			mGroups.get(i).mContribution = contribution;
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessage("SetGroupContribution");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgentSessionID);
			msg->nextBlock("Data");
			msg->addUUID("GroupID", group_id);
			msg->addS32("Contribution", contribution);
			sendReliableMessage();
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLAgent::setUserGroupFlags(const LLUUID& group_id, BOOL accept_notices, BOOL list_in_profile)
{
	S32 count = mGroups.count();
	for(S32 i = 0; i < count; ++i)
	{
		if(mGroups.get(i).mID == group_id)
		{
			mGroups.get(i).mAcceptNotices = accept_notices;
			mGroups.get(i).mListInProfile = list_in_profile;
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessage("SetGroupAcceptNotices");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgentID);
			msg->addUUID("SessionID", gAgentSessionID);
			msg->nextBlock("Data");
			msg->addUUID("GroupID", group_id);
			msg->addBOOL("AcceptNotices", accept_notices);
			msg->nextBlock("NewData");
			msg->addBOOL("ListInProfile", list_in_profile);
			sendReliableMessage();
			return TRUE;
		}
	}
	return FALSE;
}

// utility to build a location string
void LLAgent::buildLocationString(std::string& str)
{
	const LLVector3& agent_pos_region = getPositionAgent();
	S32 pos_x = S32(agent_pos_region.mV[VX]);
	S32 pos_y = S32(agent_pos_region.mV[VY]);
	S32 pos_z = S32(agent_pos_region.mV[VZ]);

	// Round the numbers based on the velocity
	LLVector3 agent_velocity = getVelocity();
	F32 velocity_mag_sq = agent_velocity.magVecSquared();

	const F32 FLY_CUTOFF = 6.f;		// meters/sec
	const F32 FLY_CUTOFF_SQ = FLY_CUTOFF * FLY_CUTOFF;
	const F32 WALK_CUTOFF = 1.5f;	// meters/sec
	const F32 WALK_CUTOFF_SQ = WALK_CUTOFF * WALK_CUTOFF;

	if (velocity_mag_sq > FLY_CUTOFF_SQ)
	{
		pos_x -= pos_x % 4;
		pos_y -= pos_y % 4;
	}
	else if (velocity_mag_sq > WALK_CUTOFF_SQ)
	{
		pos_x -= pos_x % 2;
		pos_y -= pos_y % 2;
	}

	// create a defult name and description for the landmark
	std::string buffer;
	if( LLViewerParcelMgr::getInstance()->getAgentParcelName().empty() )
	{
		// the parcel doesn't have a name
		buffer = llformat("%.32s (%d, %d, %d)",
						  getRegion()->getName().c_str(),
						  pos_x, pos_y, pos_z);
	}
	else
	{
		// the parcel has a name, so include it in the landmark name
		buffer = llformat("%.32s, %.32s (%d, %d, %d)",
						  LLViewerParcelMgr::getInstance()->getAgentParcelName().c_str(),
						  getRegion()->getName().c_str(),
						  pos_x, pos_y, pos_z);
	}
	str = buffer;
}

LLQuaternion LLAgent::getHeadRotation()
{
	if (mAvatarObject.isNull() || !mAvatarObject->mPelvisp || !mAvatarObject->mHeadp)
	{
		return LLQuaternion::DEFAULT;
	}

	if (!gAgent.cameraMouselook())
	{
		return mAvatarObject->getRotation();
	}

	// We must be in mouselook
	LLVector3 look_dir( LLViewerCamera::getInstance()->getAtAxis() );
	LLVector3 up = look_dir % mFrameAgent.getLeftAxis();
	LLVector3 left = up % look_dir;

	LLQuaternion rot(look_dir, left, up);
	if (mAvatarObject->getParent())
	{
		rot = rot * ~mAvatarObject->getParent()->getRotation();
	}

	return rot;
}

void LLAgent::sendAnimationRequests(LLDynamicArray<LLUUID> &anim_ids, EAnimRequest request)
{
	if (gAgentID.isNull())
	{
		return;
	}

	S32 num_valid_anims = 0;

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_AgentAnimation);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, getID());
	msg->addUUIDFast(_PREHASH_SessionID, getSessionID());

	for (S32 i = 0; i < anim_ids.count(); i++)
	{
		if (anim_ids[i].isNull())
		{
			continue;
		}
		msg->nextBlockFast(_PREHASH_AnimationList);
		msg->addUUIDFast(_PREHASH_AnimID, (anim_ids[i]) );
		msg->addBOOLFast(_PREHASH_StartAnim, (request == ANIM_REQUEST_START) ? TRUE : FALSE);
		num_valid_anims++;
	}

	msg->nextBlockFast(_PREHASH_PhysicalAvatarEventList);
	msg->addBinaryDataFast(_PREHASH_TypeData, NULL, 0);
	if (num_valid_anims)
	{
		sendReliableMessage();
	}
}

void LLAgent::sendAnimationRequest(const LLUUID &anim_id, EAnimRequest request)
{
	if (gAgentID.isNull() || anim_id.isNull() || !mRegionp)
	{
		return;
	}

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_AgentAnimation);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, getID());
	msg->addUUIDFast(_PREHASH_SessionID, getSessionID());

	msg->nextBlockFast(_PREHASH_AnimationList);
	msg->addUUIDFast(_PREHASH_AnimID, (anim_id) );
	msg->addBOOLFast(_PREHASH_StartAnim, (request == ANIM_REQUEST_START) ? TRUE : FALSE);

	msg->nextBlockFast(_PREHASH_PhysicalAvatarEventList);
	msg->addBinaryDataFast(_PREHASH_TypeData, NULL, 0);
	sendReliableMessage();
}

void LLAgent::sendWalkRun(bool running)
{
	LLMessageSystem* msgsys = gMessageSystem;
	if (msgsys)
	{
		msgsys->newMessageFast(_PREHASH_SetAlwaysRun);
		msgsys->nextBlockFast(_PREHASH_AgentData);
		msgsys->addUUIDFast(_PREHASH_AgentID, getID());
		msgsys->addUUIDFast(_PREHASH_SessionID, getSessionID());
		msgsys->addBOOLFast(_PREHASH_AlwaysRun, BOOL(running) );
		sendReliableMessage();
	}
}

void LLAgent::friendsChanged()
{
	LLCollectProxyBuddies collector;
	LLAvatarTracker::instance().applyFunctor(collector);
	mProxyForAgents = collector.mProxy;
}

BOOL LLAgent::isGrantedProxy(const LLPermissions& perm)
{
	return (mProxyForAgents.count(perm.getOwner()) > 0);
}

BOOL LLAgent::allowOperation(PermissionBit op,
							 const LLPermissions& perm,
							 U64 group_proxy_power,
							 U8 god_minimum)
{
	// Check god level.
	if (getGodLevel() >= god_minimum) return TRUE;

	if (!perm.isOwned()) return FALSE;

	// A group member with group_proxy_power can act as owner.
	BOOL is_group_owned;
	LLUUID owner_id;
	perm.getOwnership(owner_id, is_group_owned);
	LLUUID group_id(perm.getGroup());
	LLUUID agent_proxy(getID());

	if (is_group_owned)
	{
		if (hasPowerInGroup(group_id, group_proxy_power))
		{
			// Let the member assume the group's id for permission requests.
			agent_proxy = owner_id;
		}
	}
	else
	{
		// Check for granted mod permissions.
		if ((PERM_OWNER != op) && isGrantedProxy(perm))
		{
			agent_proxy = owner_id;
		}
	}

	// This is the group id to use for permission requests.
	// Only group members may use this field.
	LLUUID group_proxy = LLUUID::null;
	if (group_id.notNull() && isInGroup(group_id))
	{
		group_proxy = group_id;
	}

	// We now have max ownership information.
	if (PERM_OWNER == op)
	{
		// This this was just a check for ownership, we can now return the answer.
		return (agent_proxy == owner_id);
	}

	return perm.allowOperationBy(op, agent_proxy, group_proxy);
}


void LLAgent::getName(LLString& name)
{
	// Note: assumes that name points to a buffer of at least DB_FULL_NAME_BUF_SIZE bytes.
	name.clear();

	if (mAvatarObject)
	{
		LLNameValue *first_nv = mAvatarObject->getNVPair("FirstName");
		LLNameValue *last_nv = mAvatarObject->getNVPair("LastName");
		if (first_nv && last_nv)
		{
			name = first_nv->printData() + " " + last_nv->printData();
		}
		else
		{
			llwarns << "Agent is missing FirstName and/or LastName nv pair." << llendl;
		}
	}
	else
	{
		name = gSavedSettings.getString("FirstName") + " " + gSavedSettings.getString("LastName");
	}
}

const LLColor4 &LLAgent::getEffectColor()
{
	return mEffectColor;
}

void LLAgent::setEffectColor(const LLColor4 &color)
{
	mEffectColor = color;
}

void LLAgent::initOriginGlobal(const LLVector3d &origin_global)
{
	mAgentOriginGlobal = origin_global;
}

void update_group_floaters(const LLUUID& group_id)
{
	LLFloaterGroupInfo::refreshGroup(group_id);

	// update avatar info
	LLFloaterAvatarInfo* fa = LLFloaterAvatarInfo::getInstance(gAgent.getID());
	if(fa)
	{
		fa->resetGroupList();
	}

	if (gIMMgr)
	{
		// update the talk view
		gIMMgr->refresh();
	}

	gAgent.fireEvent(new LLEvent(&gAgent, "new group"), "");
}

// static
void LLAgent::processAgentDropGroup(LLMessageSystem *msg, void **)
{
	LLUUID	agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );

	if (agent_id != gAgentID)
	{
		llwarns << "processAgentDropGroup for agent other than me" << llendl;
		return;
	}

	LLUUID	group_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_GroupID, group_id );

	// Remove the group if it already exists remove it and add the new data to pick up changes.
	LLGroupData gd;
	gd.mID = group_id;
	S32 index = gAgent.mGroups.find(gd);
	if (index != -1)
	{
		gAgent.mGroups.remove(index);
		if (gAgent.getGroupID() == group_id)
		{
			gAgent.mGroupID.setNull();
			gAgent.mGroupPowers = 0;
			gAgent.mGroupName[0] = '\0';
			gAgent.mGroupTitle[0] = '\0';
		}
		
		// refresh all group information
		gAgent.sendAgentDataUpdateRequest();

		LLGroupMgr::getInstance()->clearGroupData(group_id);
		// close the floater for this group, if any.
		LLFloaterGroupInfo::closeGroup(group_id);
		// refresh the group panel of the search window, if necessary.
		LLFloaterDirectory::refreshGroup(group_id);
	}
	else
	{
		llwarns << "processAgentDropGroup, agent is not part of group " << group_id << llendl;
	}
}

class LLAgentDropGroupViewerNode : public LLHTTPNode
{
	virtual void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{

		if (
			!input.isMap() ||
			!input.has("body") )
		{
			//what to do with badly formed message?
			response->status(400);
			response->result(LLSD("Invalid message parameters"));
		}

		LLSD body = input["body"];
		if ( body.has("body") ) 
		{
			//stupid message system doubles up the "body"s
			body = body["body"];
		}

		if (
			body.has("AgentData") &&
			body["AgentData"].isArray() &&
			body["AgentData"][0].isMap() )
		{
			llinfos << "VALID DROP GROUP" << llendl;

			//there is only one set of data in the AgentData block
			LLSD agent_data = body["AgentData"][0];
			LLUUID agent_id;
			LLUUID group_id;

			agent_id = agent_data["AgentID"].asUUID();
			group_id = agent_data["GroupID"].asUUID();

			if (agent_id != gAgentID)
			{
				llwarns
					<< "AgentDropGroup for agent other than me" << llendl;

				response->notFound();
				return;
			}

			// Remove the group if it already exists remove it
			// and add the new data to pick up changes.
			LLGroupData gd;
			gd.mID = group_id;
			S32 index = gAgent.mGroups.find(gd);
			if (index != -1)
			{
				gAgent.mGroups.remove(index);
				if (gAgent.getGroupID() == group_id)
				{
					gAgent.mGroupID.setNull();
					gAgent.mGroupPowers = 0;
					gAgent.mGroupName[0] = '\0';
					gAgent.mGroupTitle[0] = '\0';
				}
		
				// refresh all group information
				gAgent.sendAgentDataUpdateRequest();

				LLGroupMgr::getInstance()->clearGroupData(group_id);
				// close the floater for this group, if any.
				LLFloaterGroupInfo::closeGroup(group_id);
				// refresh the group panel of the search window,
				//if necessary.
				LLFloaterDirectory::refreshGroup(group_id);
			}
			else
			{
				llwarns
					<< "AgentDropGroup, agent is not part of group "
					<< group_id << llendl;
			}

			response->result(LLSD());
		}
		else
		{
			//what to do with badly formed message?
			response->status(400);
			response->result(LLSD("Invalid message parameters"));
		}
	}
};

LLHTTPRegistration<LLAgentDropGroupViewerNode>
	gHTTPRegistrationAgentDropGroupViewerNode(
		"/message/AgentDropGroup");

// static
void LLAgent::processAgentGroupDataUpdate(LLMessageSystem *msg, void **)
{
	LLUUID	agent_id;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );

	if (agent_id != gAgentID)
	{
		llwarns << "processAgentGroupDataUpdate for agent other than me" << llendl;
		return;
	}	
	
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_GroupData);
	LLGroupData group;
	S32 index = -1;
	bool need_floater_update = false;
	char group_name[DB_GROUP_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
	for(S32 i = 0; i < count; ++i)
	{
		msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupID, group.mID, i);
		msg->getUUIDFast(_PREHASH_GroupData, _PREHASH_GroupInsigniaID, group.mInsigniaID, i);
		msg->getU64(_PREHASH_GroupData, "GroupPowers", group.mPowers, i);
		msg->getBOOL(_PREHASH_GroupData, "AcceptNotices", group.mAcceptNotices, i);
		msg->getS32(_PREHASH_GroupData, "Contribution", group.mContribution, i);
		msg->getStringFast(_PREHASH_GroupData, _PREHASH_GroupName, DB_GROUP_NAME_BUF_SIZE, group_name, i);
		group.mName.assign(group_name);
		
		if(group.mID.notNull())
		{
			need_floater_update = true;
			// Remove the group if it already exists remove it and add the new data to pick up changes.
			index = gAgent.mGroups.find(group);
			if (index != -1)
			{
				gAgent.mGroups.remove(index);
			}
			gAgent.mGroups.put(group);
		}
		if (need_floater_update)
		{
			update_group_floaters(group.mID);
		}
	}

}

class LLAgentGroupDataUpdateViewerNode : public LLHTTPNode
{
	virtual void post(
		LLHTTPNode::ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		LLSD body = input["body"];
		if(body.has("body"))
			body = body["body"];
		LLUUID agent_id = body["AgentData"][0]["AgentID"].asUUID();

		if (agent_id != gAgentID)
		{
			llwarns << "processAgentGroupDataUpdate for agent other than me" << llendl;
			return;
		}	

		LLSD group_data = body["GroupData"];

		LLSD::array_iterator iter_group =
			group_data.beginArray();
		LLSD::array_iterator end_group =
			group_data.endArray();
		int group_index = 0;
		for(; iter_group != end_group; ++iter_group)
		{

			LLGroupData group;
			S32 index = -1;
			bool need_floater_update = false;

			group.mID = (*iter_group)["GroupID"].asUUID();
			group.mPowers = ll_U64_from_sd((*iter_group)["GroupPowers"]);
			group.mAcceptNotices = (*iter_group)["AcceptNotices"].asBoolean();
			group.mListInProfile = body["NewGroupData"][group_index]["ListInProfile"].asBoolean();
			group.mInsigniaID = (*iter_group)["GroupInsigniaID"].asUUID();
			group.mName = (*iter_group)["GroupName"].asString();
			group.mContribution = (*iter_group)["Contribution"].asInteger();

			group_index++;

			if(group.mID.notNull())
			{
				need_floater_update = true;
				// Remove the group if it already exists remove it and add the new data to pick up changes.
				index = gAgent.mGroups.find(group);
				if (index != -1)
				{
					gAgent.mGroups.remove(index);
				}
				gAgent.mGroups.put(group);
			}
			if (need_floater_update)
			{
				update_group_floaters(group.mID);
			}
		}
	}
};

LLHTTPRegistration<LLAgentGroupDataUpdateViewerNode >
	gHTTPRegistrationAgentGroupDataUpdateViewerNode ("/message/AgentGroupDataUpdate"); 

// static
void LLAgent::processAgentDataUpdate(LLMessageSystem *msg, void **)
{
	LLUUID	agent_id;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );

	if (agent_id != gAgentID)
	{
		llwarns << "processAgentDataUpdate for agent other than me" << llendl;
		return;
	}

	msg->getStringFast(_PREHASH_AgentData, _PREHASH_GroupTitle, DB_GROUP_TITLE_BUF_SIZE, gAgent.mGroupTitle);
	LLUUID active_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_ActiveGroupID, active_id);


	if(active_id.notNull())
	{
		gAgent.mGroupID = active_id;
		msg->getU64(_PREHASH_AgentData, "GroupPowers", gAgent.mGroupPowers);
		msg->getString(_PREHASH_AgentData, _PREHASH_GroupName, DB_GROUP_NAME_BUF_SIZE, gAgent.mGroupName);
	}
	else
	{
		gAgent.mGroupID.setNull();
		gAgent.mGroupPowers = 0;
		gAgent.mGroupName[0] = '\0';
	}		

	update_group_floaters(active_id);
}

// static
void LLAgent::processScriptControlChange(LLMessageSystem *msg, void **)
{
	S32 block_count = msg->getNumberOfBlocks("Data");
	for (S32 block_index = 0; block_index < block_count; block_index++)
	{
		BOOL take_controls;
		U32	controls;
		BOOL passon;
		U32 i;
		msg->getBOOL("Data", "TakeControls", take_controls, block_index);
		if (take_controls)
		{
			// take controls
			msg->getU32("Data", "Controls", controls, block_index );
			msg->getBOOL("Data", "PassToAgent", passon, block_index );
			U32 total_count = 0;
			for (i = 0; i < TOTAL_CONTROLS; i++)
			{
				if (controls & ( 1 << i))
				{
					if (passon)
					{
						gAgent.mControlsTakenPassedOnCount[i]++;
					}
					else
					{
						gAgent.mControlsTakenCount[i]++;
					}
					total_count++;
				}
			}
		
			// Any control taken?  If so, might be first time.
			if (total_count > 0)
			{
				LLFirstUse::useOverrideKeys();
			}
		}
		else
		{
			// release controls
			msg->getU32("Data", "Controls", controls, block_index );
			msg->getBOOL("Data", "PassToAgent", passon, block_index );
			for (i = 0; i < TOTAL_CONTROLS; i++)
			{
				if (controls & ( 1 << i))
				{
					if (passon)
					{
						gAgent.mControlsTakenPassedOnCount[i]--;
						if (gAgent.mControlsTakenPassedOnCount[i] < 0)
						{
							gAgent.mControlsTakenPassedOnCount[i] = 0;
						}
					}
					else
					{
						gAgent.mControlsTakenCount[i]--;
						if (gAgent.mControlsTakenCount[i] < 0)
						{
							gAgent.mControlsTakenCount[i] = 0;
						}
					}
				}
			}
		}
	}
}

/*
// static
void LLAgent::processControlTake(LLMessageSystem *msg, void **)
{
	U32	controls;
	msg->getU32("Data", "Controls", controls );
	U32 passon;
	msg->getBOOL("Data", "PassToAgent", passon );

	S32 i;
	S32 total_count = 0;
	for (i = 0; i < TOTAL_CONTROLS; i++)
	{
		if (controls & ( 1 << i))
		{
			if (passon)
			{
				gAgent.mControlsTakenPassedOnCount[i]++;
			}
			else
			{
				gAgent.mControlsTakenCount[i]++;
			}
			total_count++;
		}
	}

	// Any control taken?  If so, might be first time.
	if (total_count > 0)
	{
		LLFirstUse::useOverrideKeys();
	}
}

// static
void LLAgent::processControlRelease(LLMessageSystem *msg, void **)
{
	U32	controls;
	msg->getU32("Data", "Controls", controls );
	U32 passon;
	msg->getBOOL("Data", "PassToAgent", passon );

	S32 i;
	for (i = 0; i < TOTAL_CONTROLS; i++)
	{
		if (controls & ( 1 << i))
		{
			if (passon)
			{
				gAgent.mControlsTakenPassedOnCount[i]--;
				if (gAgent.mControlsTakenPassedOnCount[i] < 0)
				{
					gAgent.mControlsTakenPassedOnCount[i] = 0;
				}
			}
			else
			{
				gAgent.mControlsTakenCount[i]--;
				if (gAgent.mControlsTakenCount[i] < 0)
				{
					gAgent.mControlsTakenCount[i] = 0;
				}
			}
		}
	}
}
*/

//static
void LLAgent::processAgentCachedTextureResponse(LLMessageSystem *mesgsys, void **user_data)
{
	gAgent.mNumPendingQueries--;

	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (!avatarp || avatarp->isDead())
	{
		llwarns << "No avatar for user in cached texture update!" << llendl;
		return;
	}

	if (gAgent.cameraCustomizeAvatar())
	{
		// ignore baked textures when in customize mode
		return;
	}

	S32 query_id;
	mesgsys->getS32Fast(_PREHASH_AgentData, _PREHASH_SerialNum, query_id);

	S32 num_texture_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_WearableData);


	S32 num_results = 0;
	for (S32 texture_block = 0; texture_block < num_texture_blocks; texture_block++)
	{
		LLUUID texture_id;
		U8 texture_index;

		mesgsys->getUUIDFast(_PREHASH_WearableData, _PREHASH_TextureID, texture_id, texture_block);
		mesgsys->getU8Fast(_PREHASH_WearableData, _PREHASH_TextureIndex, texture_index, texture_block);

		if (texture_id.notNull() 
			&& (S32)texture_index < BAKED_TEXTURE_COUNT 
			&& gAgent.mActiveCacheQueries[ texture_index ] == query_id)
		{
			//llinfos << "Received cached texture " << (U32)texture_index << ": " << texture_id << llendl;
			avatarp->setCachedBakedTexture((LLVOAvatar::ETextureIndex)LLVOAvatar::sBakedTextureIndices[texture_index], texture_id);
			//avatarp->setTETexture( LLVOAvatar::sBakedTextureIndices[texture_index], texture_id );
			gAgent.mActiveCacheQueries[ texture_index ] = 0;
			num_results++;
		}
	}

	llinfos << "Received cached texture response for " << num_results << " textures." << llendl;

	avatarp->updateMeshTextures();

	if (gAgent.mNumPendingQueries == 0)
	{
		// RN: not sure why composites are disabled at this point
		avatarp->setCompositeUpdatesEnabled(TRUE);
		gAgent.sendAgentSetAppearance();
	}
}

BOOL LLAgent::anyControlGrabbed() const
{
	U32 i;
	for (i = 0; i < TOTAL_CONTROLS; i++)
	{
		if (gAgent.mControlsTakenCount[i] > 0)
			return TRUE;
		if (gAgent.mControlsTakenPassedOnCount[i] > 0)
			return TRUE;
	}
	return FALSE;
}

BOOL LLAgent::isControlGrabbed(S32 control_index) const
{
	return mControlsTakenCount[control_index] > 0;
}

void LLAgent::forceReleaseControls()
{
	gMessageSystem->newMessage("ForceScriptControlRelease");
	gMessageSystem->nextBlock("AgentData");
	gMessageSystem->addUUID("AgentID", getID());
	gMessageSystem->addUUID("SessionID", getSessionID());
	sendReliableMessage();
}

void LLAgent::setHomePosRegion( const U64& region_handle, const LLVector3& pos_region)
{
	mHaveHomePosition = TRUE;
	mHomeRegionHandle = region_handle;
	mHomePosRegion = pos_region;
}

BOOL LLAgent::getHomePosGlobal( LLVector3d* pos_global )
{
	if(!mHaveHomePosition)
	{
		return FALSE;
	}
	F32 x = 0;
	F32 y = 0;
	from_region_handle( mHomeRegionHandle, &x, &y);
	pos_global->setVec( x + mHomePosRegion.mV[VX], y + mHomePosRegion.mV[VY], mHomePosRegion.mV[VZ] );
	return TRUE;
}

void LLAgent::clearVisualParams(void *data)
{
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		avatarp->clearVisualParamWeights();
		avatarp->updateVisualParams();
	}
}

//---------------------------------------------------------------------------
// Teleport
//---------------------------------------------------------------------------

// teleportCore() - stuff to do on any teleport
// protected
bool LLAgent::teleportCore(bool is_local)
{
	if(TELEPORT_NONE != mTeleportState)
	{
		llwarns << "Attempt to teleport when already teleporting." << llendl;
		return false;
	}

	// Stop all animation before actual teleporting 
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		for ( LLVOAvatar::AnimIterator anim_it= avatarp->mPlayingAnimations.begin()
			; anim_it != avatarp->mPlayingAnimations.end()
			; anim_it++)
		{
			avatarp->stopMotion(anim_it->first);
		}
		avatarp->processAnimationStateChanges();
	}

	// Don't call LLFirstUse::useTeleport because we don't know
	// yet if the teleport will succeed.  Look in 
	// process_teleport_location_reply

	// close the map and find panels so we can see our destination
	LLFloaterWorldMap::hide(NULL);
	LLFloaterDirectory::hide(NULL);

	LLViewerParcelMgr::getInstance()->deselectLand();

	// Close all pie menus, deselect land, etc.
	// Don't change the camera until we know teleport succeeded. JC
	resetView(FALSE);

	// local logic
	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TELEPORT_COUNT);
	if (!is_local)
	{
		gTeleportDisplay = TRUE;
		gAgent.setTeleportState( LLAgent::TELEPORT_START );

		//release geometry from old location
		gPipeline.resetVertexBuffers();
	}
	make_ui_sound("UISndTeleportOut");
	
	// MBW -- Let the voice client know a teleport has begun so it can leave the existing channel.
	// This was breaking the case of teleporting within a single sim.  Backing it out for now.
//	gVoiceClient->leaveChannel();
	
	return true;
}

void LLAgent::teleportRequest(
	const U64& region_handle,
	const LLVector3& pos_local)
{
	LLViewerRegion* regionp = getRegion();
	if(regionp && teleportCore())
	{
		llinfos << "TeleportRequest: '" << region_handle << "':" << pos_local
				<< llendl;
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("TeleportLocationRequest");
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, getID());
		msg->addUUIDFast(_PREHASH_SessionID, getSessionID());
		msg->nextBlockFast(_PREHASH_Info);
		msg->addU64("RegionHandle", region_handle);
		msg->addVector3("Position", pos_local);
		LLVector3 look_at(0,1,0);
		msg->addVector3("LookAt", look_at);
		sendReliableMessage();
	}
}

// Landmark ID = LLUUID::null means teleport home
void LLAgent::teleportViaLandmark(const LLUUID& landmark_asset_id)
{
	LLViewerRegion *regionp = getRegion();
	if(regionp && teleportCore())
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_TeleportLandmarkRequest);
		msg->nextBlockFast(_PREHASH_Info);
		msg->addUUIDFast(_PREHASH_AgentID, getID());
		msg->addUUIDFast(_PREHASH_SessionID, getSessionID());
		msg->addUUIDFast(_PREHASH_LandmarkID, landmark_asset_id);
		sendReliableMessage();
	}
}

void LLAgent::teleportViaLure(const LLUUID& lure_id, BOOL godlike)
{
	LLViewerRegion* regionp = getRegion();
	if(regionp && teleportCore())
	{
		U32 teleport_flags = 0x0;
		if (godlike)
		{
			teleport_flags |= TELEPORT_FLAGS_VIA_GODLIKE_LURE;
			teleport_flags |= TELEPORT_FLAGS_DISABLE_CANCEL;
		}
		else
		{
			teleport_flags |= TELEPORT_FLAGS_VIA_LURE;
		}

		// send the message
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_TeleportLureRequest);
		msg->nextBlockFast(_PREHASH_Info);
		msg->addUUIDFast(_PREHASH_AgentID, getID());
		msg->addUUIDFast(_PREHASH_SessionID, getSessionID());
		msg->addUUIDFast(_PREHASH_LureID, lure_id);
		// teleport_flags is a legacy field, now derived sim-side:
		msg->addU32("TeleportFlags", teleport_flags);
		sendReliableMessage();
	}	
}


// James Cook, July 28, 2005
void LLAgent::teleportCancel()
{
	LLViewerRegion* regionp = getRegion();
	if(regionp)
	{
		// send the message
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessage("TeleportCancel");
		msg->nextBlockFast(_PREHASH_Info);
		msg->addUUIDFast(_PREHASH_AgentID, getID());
		msg->addUUIDFast(_PREHASH_SessionID, getSessionID());
		sendReliableMessage();
	}	
	gTeleportDisplay = FALSE;
	gAgent.setTeleportState( LLAgent::TELEPORT_NONE );
}


void LLAgent::teleportViaLocation(const LLVector3d& pos_global)
{
	LLViewerRegion* regionp = getRegion();
	LLSimInfo* info = LLWorldMap::getInstance()->simInfoFromPosGlobal(pos_global);
	if(regionp && info)
	{
		U32 x_pos;
		U32 y_pos;
		from_region_handle(info->mHandle, &x_pos, &y_pos);
		LLVector3 pos_local(
			(F32)(pos_global.mdV[VX] - x_pos),
			(F32)(pos_global.mdV[VY] - y_pos),
			(F32)(pos_global.mdV[VZ]));
		teleportRequest(info->mHandle, pos_local);
	}
	else if(regionp && 
		teleportCore(regionp->getHandle() == to_region_handle_global((F32)pos_global.mdV[VX], (F32)pos_global.mdV[VY])))
	{
		llwarns << "Using deprecated teleportlocationrequest." << llendl; 
		// send the message
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_TeleportLocationRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, getID());
		msg->addUUIDFast(_PREHASH_SessionID, getSessionID());

		msg->nextBlockFast(_PREHASH_Info);
		F32 width = regionp->getWidth();
		LLVector3 pos(fmod((F32)pos_global.mdV[VX], width),
					  fmod((F32)pos_global.mdV[VY], width),
					  (F32)pos_global.mdV[VZ]);
		F32 region_x = (F32)(pos_global.mdV[VX]);
		F32 region_y = (F32)(pos_global.mdV[VY]);
		U64 region_handle = to_region_handle_global(region_x, region_y);
		msg->addU64Fast(_PREHASH_RegionHandle, region_handle);
		msg->addVector3Fast(_PREHASH_Position, pos);
		pos.mV[VX] += 1;
		msg->addVector3Fast(_PREHASH_LookAt, pos);
		sendReliableMessage();
	}
}

void LLAgent::setTeleportState(ETeleportState state)
{
	mTeleportState = state;
	if (mTeleportState > TELEPORT_NONE && gSavedSettings.getBOOL("FreezeTime"))
	{
		LLFloaterSnapshot::hide(0);
	}
	if (mTeleportState == TELEPORT_MOVING)
	{
		// We're outa here. Save "back" slurl.
		mTeleportSourceSLURL = getSLURL();
	}
}

void LLAgent::fidget()
{
	if (!getAFK())
	{
		F32 curTime = mFidgetTimer.getElapsedTimeF32();
		if (curTime > mNextFidgetTime)
		{
			// pick a random fidget anim here
			S32 oldFidget = mCurrentFidget;

			mCurrentFidget = ll_rand(NUM_AGENT_STAND_ANIMS);

			if (mCurrentFidget != oldFidget)
			{
				LLAgent::stopFidget();

				
				switch(mCurrentFidget)
				{
				case 0:
					mCurrentFidget = 0;
					break;
				case 1:
					sendAnimationRequest(ANIM_AGENT_STAND_1, ANIM_REQUEST_START);
					mCurrentFidget = 1;
					break;
				case 2:
					sendAnimationRequest(ANIM_AGENT_STAND_2, ANIM_REQUEST_START);
					mCurrentFidget = 2;
					break;
				case 3:
					sendAnimationRequest(ANIM_AGENT_STAND_3, ANIM_REQUEST_START);
					mCurrentFidget = 3;
					break;
				case 4:
					sendAnimationRequest(ANIM_AGENT_STAND_4, ANIM_REQUEST_START);
					mCurrentFidget = 4;
					break;
				}
			}

			// calculate next fidget time
			mNextFidgetTime = curTime + ll_frand(MAX_FIDGET_TIME - MIN_FIDGET_TIME) + MIN_FIDGET_TIME;
		}
	}
}

void LLAgent::stopFidget()
{
	LLDynamicArray<LLUUID> anims;
	anims.put(ANIM_AGENT_STAND_1);
	anims.put(ANIM_AGENT_STAND_2);
	anims.put(ANIM_AGENT_STAND_3);
	anims.put(ANIM_AGENT_STAND_4);

	gAgent.sendAnimationRequests(anims, ANIM_REQUEST_STOP);
}


void LLAgent::requestEnterGodMode()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RequestGodlikePowers);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_RequestBlock);
	msg->addBOOLFast(_PREHASH_Godlike, TRUE);
	msg->addUUIDFast(_PREHASH_Token, LLUUID::null);

	// simulators need to know about your request
	sendReliableMessage();
}

void LLAgent::requestLeaveGodMode()
{
	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_RequestGodlikePowers);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_RequestBlock);
	msg->addBOOLFast(_PREHASH_Godlike, FALSE);
	msg->addUUIDFast(_PREHASH_Token, LLUUID::null);

	// simulator needs to know about your request
	sendReliableMessage();
}

// wearables
LLAgent::createStandardWearablesAllDoneCallback::~createStandardWearablesAllDoneCallback()
{
	gAgent.createStandardWearablesAllDone();
}

LLAgent::sendAgentWearablesUpdateCallback::~sendAgentWearablesUpdateCallback()
{
	gAgent.sendAgentWearablesUpdate();
}

LLAgent::addWearableToAgentInventoryCallback::addWearableToAgentInventoryCallback(
	LLPointer<LLRefCount> cb, S32 index, LLWearable* wearable, U32 todo) :
	mIndex(index),
	mWearable(wearable),
	mTodo(todo),
	mCB(cb)
{
}

void LLAgent::addWearableToAgentInventoryCallback::fire(const LLUUID& inv_item)
{
	if (inv_item.isNull())
		return;

	gAgent.addWearabletoAgentInventoryDone(mIndex, inv_item, mWearable);

	if (mTodo & CALL_UPDATE)
	{
		gAgent.sendAgentWearablesUpdate();
	}
	if (mTodo & CALL_RECOVERDONE)
	{
		gAgent.recoverMissingWearableDone();
	}
	/*
	 * Do this for every one in the loop
	 */
	if (mTodo & CALL_CREATESTANDARDDONE)
	{
		gAgent.createStandardWearablesDone(mIndex);
	}
	if (mTodo & CALL_MAKENEWOUTFITDONE)
	{
		gAgent.makeNewOutfitDone(mIndex);
	}
}

void LLAgent::addWearabletoAgentInventoryDone(
	S32 index,
	const LLUUID& item_id,
	LLWearable* wearable)
{
	if (item_id.isNull())
		return;

	LLUUID old_item_id = mWearableEntry[index].mItemID;
	mWearableEntry[index].mItemID = item_id;
	mWearableEntry[index].mWearable = wearable;
	if (old_item_id.notNull())
		gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);
	gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
	LLViewerInventoryItem* item = gInventory.getItem(item_id);
	if(item && wearable)
	{
		// We're changing the asset id, so we both need to set it
		// locally via setAssetUUID() and via setTransactionID() which
		// will be decoded on the server. JC
		item->setAssetUUID(wearable->getID());
		item->setTransactionID(wearable->getTransactionID());
		gInventory.addChangedMask(LLInventoryObserver::INTERNAL, item_id);
		item->updateServer(FALSE);
	}
	gInventory.notifyObservers();
}

void LLAgent::sendAgentWearablesUpdate()
{
	// First make sure that we have inventory items for each wearable
	S32 i;
	for(i=0; i < WT_COUNT; ++i)
	{
		LLWearable* wearable = mWearableEntry[ i ].mWearable;
		if (wearable)
		{
			if( mWearableEntry[ i ].mItemID.isNull() )
			{
				LLPointer<LLInventoryCallback> cb =
					new addWearableToAgentInventoryCallback(
						LLPointer<LLRefCount>(NULL),
						i,
						wearable,
						addWearableToAgentInventoryCallback::CALL_NONE);
				addWearableToAgentInventory(cb, wearable);
			}
			else
			{
				gInventory.addChangedMask( LLInventoryObserver::LABEL,
						mWearableEntry[i].mItemID );
			}
		}
	}

	// Then make sure the inventory is in sync with the avatar.
	gInventory.notifyObservers();

	// Send the AgentIsNowWearing 
	gMessageSystem->newMessageFast(_PREHASH_AgentIsNowWearing);

	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, getSessionID());

	lldebugs << "sendAgentWearablesUpdate()" << llendl;
	for(i=0; i < WT_COUNT; ++i)
	{
		gMessageSystem->nextBlockFast(_PREHASH_WearableData);

		U8 type_u8 = (U8)i;
		gMessageSystem->addU8Fast(_PREHASH_WearableType, type_u8 );

		LLWearable* wearable = mWearableEntry[ i ].mWearable;
		if( wearable )
		{
			//llinfos << "Sending wearable " << wearable->getName() << llendl;
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, mWearableEntry[ i ].mItemID );
		}
		else
		{
			//llinfos << "Not wearing wearable type " << LLWearable::typeToTypeName((EWearableType)i) << llendl;
			gMessageSystem->addUUIDFast(_PREHASH_ItemID, LLUUID::null );
		}

		lldebugs << "       " << LLWearable::typeToTypeLabel((EWearableType)i) << ": " << (wearable ? wearable->getID() : LLUUID::null) << llendl;
	}
	gAgent.sendReliableMessage();
}

void LLAgent::saveWearable( EWearableType type, BOOL send_update )
{
	LLWearable* old_wearable = mWearableEntry[(S32)type].mWearable;
	if( old_wearable && (old_wearable->isDirty() || old_wearable->isOldVersion()) )
	{
		LLWearable* new_wearable = gWearableList.createCopyFromAvatar( old_wearable );
		mWearableEntry[(S32)type].mWearable = new_wearable;

		LLInventoryItem* item = gInventory.getItem(mWearableEntry[(S32)type].mItemID);
		if( item )
		{
			// Update existing inventory item
			LLPointer<LLViewerInventoryItem> template_item =
				new LLViewerInventoryItem(item->getUUID(),
										  item->getParentUUID(),
										  item->getPermissions(),
										  new_wearable->getID(),
										  new_wearable->getAssetType(),
										  item->getInventoryType(),
										  item->getName(),
										  item->getDescription(),
										  item->getSaleInfo(),
										  item->getFlags(),
										  item->getCreationDate());
			template_item->setTransactionID(new_wearable->getTransactionID());
			template_item->updateServer(FALSE);
			gInventory.updateItem(template_item);
		}
		else
		{
			// Add a new inventory item (shouldn't ever happen here)
			U32 todo = addWearableToAgentInventoryCallback::CALL_NONE;
			if (send_update)
			{
				todo |= addWearableToAgentInventoryCallback::CALL_UPDATE;
			}
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(
					LLPointer<LLRefCount>(NULL),
					(S32)type,
					new_wearable,
					todo);
			addWearableToAgentInventory(cb, new_wearable);
			return;
		}

		if( send_update )
		{
			sendAgentWearablesUpdate();
		}
	}
}

void LLAgent::saveWearableAs(
	EWearableType type,
	const std::string& new_name,
	BOOL save_in_lost_and_found)
{
	if(!isWearableCopyable(type))
	{
		llwarns << "LLAgent::saveWearableAs() not copyable." << llendl;
		return;
	}
	LLWearable* old_wearable = getWearable(type);
	if(!old_wearable)
	{
		llwarns << "LLAgent::saveWearableAs() no old wearable." << llendl;
		return;
	}
	LLInventoryItem* item = gInventory.getItem(mWearableEntry[type].mItemID);
	if(!item)
	{
		llwarns << "LLAgent::saveWearableAs() no inventory item." << llendl;
		return;
	}
	std::string trunc_name(new_name);
	LLString::truncate(trunc_name, DB_INV_ITEM_NAME_STR_LEN);
	LLWearable* new_wearable = gWearableList.createCopyFromAvatar(
		old_wearable,
		trunc_name);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type,
			new_wearable,
			addWearableToAgentInventoryCallback::CALL_UPDATE);
	LLUUID category_id;
	if (save_in_lost_and_found)
	{
		category_id = gInventory.findCategoryUUIDForType(
			LLAssetType::AT_LOST_AND_FOUND);
	}
	else
	{
		// put in same folder as original
		category_id = item->getParentUUID();
	}

	copy_inventory_item(
		gAgent.getID(),
		item->getPermissions().getOwner(),
		item->getUUID(),
		category_id,
		new_name,
		cb);

/*
	LLWearable* old_wearable = getWearable( type );
	if( old_wearable )
	{
		LLString old_name = old_wearable->getName();
		old_wearable->setName( new_name );
		LLWearable* new_wearable = gWearableList.createCopyFromAvatar( old_wearable );
		old_wearable->setName( old_name );
			
		LLUUID category_id;
		LLInventoryItem* item = gInventory.getItem( mWearableEntry[ type ].mItemID );
		if( item )
		{
			new_wearable->setPermissions(item->getPermissions());
			if (save_in_lost_and_found)
			{
				category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
			}
			else
			{
				// put in same folder as original
				category_id = item->getParentUUID();
			}
			LLInventoryView* view = LLInventoryView::getActiveInventory();
			if(view)
			{
				view->getPanel()->setSelection(item->getUUID(), TAKE_FOCUS_NO);
			}
		}

		mWearableEntry[ type ].mWearable = new_wearable;
		LLPointer<LLInventoryCallback> cb =
			new addWearableToAgentInventoryCallback(
				LLPointer<LLRefCount>(NULL),
				type,
				addWearableToAgentInventoryCallback::CALL_UPDATE);
		addWearableToAgentInventory(cb, new_wearable, category_id);
	}
*/
}

void LLAgent::revertWearable( EWearableType type )
{
	LLWearable* wearable = mWearableEntry[(S32)type].mWearable;
	if( wearable )
	{
		wearable->writeToAvatar( TRUE );
	}
	sendAgentSetAppearance();
}

void LLAgent::revertAllWearables()
{
	for( S32 i=0; i < WT_COUNT; i++ )
	{
		revertWearable( (EWearableType)i );
	}
}

void LLAgent::saveAllWearables()
{
	//if(!gInventory.isLoaded())
	//{
	//	return;
	//}

	for( S32 i=0; i < WT_COUNT; i++ )
	{
		saveWearable( (EWearableType)i, FALSE );
	}
	sendAgentWearablesUpdate();
}

// Called when the user changes the name of a wearable inventory item that is currenlty being worn.
void LLAgent::setWearableName( const LLUUID& item_id, const std::string& new_name )
{
	for( S32 i=0; i < WT_COUNT; i++ )
	{
		if( mWearableEntry[i].mItemID == item_id )
		{
			LLWearable* old_wearable = mWearableEntry[i].mWearable;
			llassert( old_wearable );

			LLString old_name = old_wearable->getName();
			old_wearable->setName( new_name );
			LLWearable* new_wearable = gWearableList.createCopy( old_wearable );
			LLInventoryItem* item = gInventory.getItem(item_id);
			if(item)
			{
				new_wearable->setPermissions(item->getPermissions());
			}
			old_wearable->setName( old_name );

			mWearableEntry[i].mWearable = new_wearable;
			sendAgentWearablesUpdate();
			break;
		}
	}
}


BOOL LLAgent::isWearableModifiable(EWearableType type)
{
	LLUUID item_id = getWearableItem(type);
	if(!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if(item && item->getPermissions().allowModifyBy(gAgent.getID(),
														gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLAgent::isWearableCopyable(EWearableType type)
{
	LLUUID item_id = getWearableItem(type);
	if(!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if(item && item->getPermissions().allowCopyBy(gAgent.getID(),
													  gAgent.getGroupID()))
		{
			return TRUE;
		}
	}
	return FALSE;
}

U32 LLAgent::getWearablePermMask(EWearableType type)
{
	LLUUID item_id = getWearableItem(type);
	if(!item_id.isNull())
	{
		LLInventoryItem* item = gInventory.getItem(item_id);
		if(item)
		{
			return item->getPermissions().getMaskOwner();
		}
	}
	return PERM_NONE;
}

LLInventoryItem* LLAgent::getWearableInventoryItem(EWearableType type)
{
	LLUUID item_id = getWearableItem(type);
	LLInventoryItem* item = NULL;
	if(item_id.notNull())
	{
		 item = gInventory.getItem(item_id);
	}
	return item;
}

LLWearable* LLAgent::getWearableFromWearableItem( const LLUUID& item_id )
{
	for( S32 i=0; i < WT_COUNT; i++ )
	{
		if( mWearableEntry[i].mItemID == item_id )
		{
			return mWearableEntry[i].mWearable;
		}
	}
	return NULL;
}


void LLAgent::sendAgentWearablesRequest()
{
	gMessageSystem->newMessageFast(_PREHASH_AgentWearablesRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID() );
	sendReliableMessage();
}

// Used to enable/disable menu items.
// static
BOOL LLAgent::selfHasWearable( void* userdata )
{
	EWearableType type = (EWearableType)(intptr_t)userdata;
	return gAgent.getWearable( type ) != NULL;
}

BOOL LLAgent::isWearingItem( const LLUUID& item_id )
{
	return (getWearableFromWearableItem( item_id ) != NULL);
}


// static
void LLAgent::processAgentInitialWearablesUpdate( LLMessageSystem* mesgsys, void** user_data )
{
	// We should only receive this message a single time.  Ignore subsequent AgentWearablesUpdates
	// that may result from AgentWearablesRequest having been sent more than once. 
	static BOOL first = TRUE;
	if( first )
	{
		first = FALSE;
	}
	else
	{
		return;
	}
	
	if (gNoRender)
	{
		return;
	}

	LLUUID agent_id;
	gMessageSystem->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id );

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( avatar && (agent_id == avatar->getID()) )
	{
		gMessageSystem->getU32Fast(_PREHASH_AgentData, _PREHASH_SerialNum, gAgent.mAgentWearablesUpdateSerialNum );
		
		S32 num_wearables = gMessageSystem->getNumberOfBlocksFast(_PREHASH_WearableData);
		if( num_wearables < 4 )
		{
			// Transitional state.  Avatars should always have at least their body parts (hair, eyes, shape and skin).
			// The fact that they don't have any here (only a dummy is sent) implies that this account existed
			// before we had wearables, or that the database has gotten messed up.
			// Deal with this by creating new body parts.
			//avatar->createStandardWearables();

			// no, deal with it by noting that we need to choose a
			// gender.
			gAgent.setGenderChosen(FALSE);
			return;
		}

		//lldebugs << "processAgentInitialWearablesUpdate()" << llendl;
		// Add wearables
		LLUUID asset_id_array[ WT_COUNT ];
		S32 i;
		for( i=0; i < num_wearables; i++ )
		{
			U8 type_u8 = 0;
			gMessageSystem->getU8Fast(_PREHASH_WearableData, _PREHASH_WearableType, type_u8, i );
			if( type_u8 >= WT_COUNT )
			{
				continue;
			}
			EWearableType type = (EWearableType) type_u8;

			LLUUID item_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_ItemID, item_id, i );

			LLUUID asset_id;
			gMessageSystem->getUUIDFast(_PREHASH_WearableData, _PREHASH_AssetID, asset_id, i );
			if( asset_id.isNull() )
			{
				LLWearable::removeFromAvatar( type, FALSE );
			}
			else
			{
				LLAssetType::EType asset_type = LLWearable::typeToAssetType( type );
				if( asset_type == LLAssetType::AT_NONE )
				{
					continue;
				}

				gAgent.mWearableEntry[type].mItemID = item_id;
				asset_id_array[type] = asset_id;
			}

			lldebugs << "       " << LLWearable::typeToTypeLabel(type) << llendl;
		}

		// now that we have the asset ids...request the wearable assets
		for( i = 0; i < WT_COUNT; i++ )
		{
			if( !gAgent.mWearableEntry[i].mItemID.isNull() )
			{
				gWearableList.getAsset( 
					asset_id_array[i],
					LLString::null,
					LLWearable::typeToAssetType( (EWearableType) i ), 
					LLAgent::onInitialWearableAssetArrived, (void*)(intptr_t)i );
			}
		}
	}
}

// A single wearable that the avatar was wearing on start-up has arrived from the database.
// static
void LLAgent::onInitialWearableAssetArrived( LLWearable* wearable, void* userdata )
{
	EWearableType type = (EWearableType)(intptr_t)userdata;

	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( !avatar )
	{
		return;
	}

	if( wearable )
	{
		llassert( type == wearable->getType() );
		gAgent.mWearableEntry[ type ].mWearable = wearable;

		// disable composites if initial textures are baked
		avatar->setupComposites();
		gAgent.queryWearableCache();

		wearable->writeToAvatar( FALSE );
		avatar->setCompositeUpdatesEnabled(TRUE);
		gInventory.addChangedMask( LLInventoryObserver::LABEL, gAgent.mWearableEntry[type].mItemID );
	}
	else
	{
		// Somehow the asset doesn't exist in the database.
		gAgent.recoverMissingWearable( type );
	}

	gInventory.notifyObservers();

	// Have all the wearables that the avatar was wearing at log-in arrived?
	if( !gAgent.mWearablesLoaded )
	{
		gAgent.mWearablesLoaded = TRUE;
		for( S32 i = 0; i < WT_COUNT; i++ )
		{
			if( !gAgent.mWearableEntry[i].mItemID.isNull() && !gAgent.mWearableEntry[i].mWearable )
			{
				gAgent.mWearablesLoaded = FALSE;
				break;
			}
		}
	}

	if( gAgent.mWearablesLoaded )
	{
		// Make sure that the server's idea of the avatar's wearables actually match the wearables.
		gAgent.sendAgentSetAppearance();

		// Check to see if there are any baked textures that we hadn't uploaded before we logged off last time.
		// If there are any, schedule them to be uploaded as soon as the layer textures they depend on arrive.
		if( !gAgent.cameraCustomizeAvatar() )
		{
			avatar->requestLayerSetUploads();
		}
	}
}

// Normally, all wearables referred to "AgentWearablesUpdate" will correspond to actual assets in the
// database.  If for some reason, we can't load one of those assets, we can try to reconstruct it so that
// the user isn't left without a shape, for example.  (We can do that only after the inventory has loaded.)
void LLAgent::recoverMissingWearable( EWearableType type )
{
	// Try to recover by replacing missing wearable with a new one.
	LLNotifyBox::showXml("ReplacedMissingWearable");
	lldebugs << "Wearable " << LLWearable::typeToTypeLabel( type ) << " could not be downloaded.  Replaced inventory item with default wearable." << llendl;
	LLWearable* new_wearable = gWearableList.createNewWearable(type);

	S32 type_s32 = (S32) type;
	mWearableEntry[type_s32].mWearable = new_wearable;
	new_wearable->writeToAvatar( TRUE );

	// Add a new one in the lost and found folder.
	// (We used to overwrite the "not found" one, but that could potentially
	// destory content.) JC
	LLUUID lost_and_found_id = 
		gInventory.findCategoryUUIDForType(LLAssetType::AT_LOST_AND_FOUND);
	LLPointer<LLInventoryCallback> cb =
		new addWearableToAgentInventoryCallback(
			LLPointer<LLRefCount>(NULL),
			type_s32,
			new_wearable,
			addWearableToAgentInventoryCallback::CALL_RECOVERDONE);
	addWearableToAgentInventory( cb, new_wearable, lost_and_found_id, TRUE);
}

void LLAgent::recoverMissingWearableDone()
{
	// Have all the wearables that the avatar was wearing at log-in arrived or been fabricated?
	mWearablesLoaded = TRUE;
	for( S32 i = 0; i < WT_COUNT; i++ )
	{
		if( !mWearableEntry[i].mItemID.isNull() && !mWearableEntry[i].mWearable )
		{
			mWearablesLoaded = FALSE;
			break;
		}
	}

	if( mWearablesLoaded )
	{
		// Make sure that the server's idea of the avatar's wearables actually match the wearables.
		sendAgentSetAppearance();
	}
	else
	{
		gInventory.addChangedMask( LLInventoryObserver::LABEL, LLUUID::null );
		gInventory.notifyObservers();
	}
}

void LLAgent::createStandardWearables(BOOL female)
{
	llwarns << "Creating Standard " << (female ? "female" : "male" )
			<< " Wearables" << llendl;

	if (mAvatarObject.isNull())
	{
		return;
	}

	if(female) mAvatarObject->setSex(SEX_FEMALE);
	else mAvatarObject->setSex(SEX_MALE);

	BOOL create[WT_COUNT] = 
	{
		TRUE,  //WT_SHAPE
		TRUE,  //WT_SKIN
		TRUE,  //WT_HAIR
		TRUE,  //WT_EYES
		TRUE,  //WT_SHIRT
		TRUE,  //WT_PANTS
		TRUE,  //WT_SHOES
		TRUE,  //WT_SOCKS
		FALSE, //WT_JACKET
		FALSE, //WT_GLOVES
		TRUE,  //WT_UNDERSHIRT
		TRUE,  //WT_UNDERPANTS
		FALSE  //WT_SKIRT
	};

	for( S32 i=0; i < WT_COUNT; i++ )
	{
		bool once = false;
		LLPointer<LLRefCount> donecb = NULL;
		if( create[i] )
		{
			if (!once)
			{
				once = true;
				donecb = new createStandardWearablesAllDoneCallback;
			}
			llassert( mWearableEntry[i].mWearable == NULL );
			LLWearable* wearable = gWearableList.createNewWearable((EWearableType)i);
			mWearableEntry[i].mWearable = wearable;
			// no need to update here...
			LLPointer<LLInventoryCallback> cb =
				new addWearableToAgentInventoryCallback(
					donecb,
					i,
					wearable,
					addWearableToAgentInventoryCallback::CALL_CREATESTANDARDDONE);
			addWearableToAgentInventory(cb, wearable, LLUUID::null, FALSE);
		}
	}
}
void LLAgent::createStandardWearablesDone(S32 index)
{
	LLWearable* wearable = mWearableEntry[index].mWearable;

	if (wearable)
	{
		wearable->writeToAvatar(TRUE);
	}
}

void LLAgent::createStandardWearablesAllDone()
{
	// ... because sendAgentWearablesUpdate will notify inventory
	// observers.
	mWearablesLoaded = TRUE; 
	sendAgentWearablesUpdate();
	sendAgentSetAppearance();

	// Treat this as the first texture entry message, if none received yet
	mAvatarObject->onFirstTEMessageReceived();
}

void LLAgent::makeNewOutfit( 
	const std::string& new_folder_name,
	const LLDynamicArray<S32>& wearables_to_include,
	const LLDynamicArray<S32>& attachments_to_include,
	BOOL rename_clothing)
{
	if (mAvatarObject.isNull())
	{
		return;
	}

	// First, make a folder in the Clothes directory.
	LLUUID folder_id = gInventory.createNewCategory(
		gInventory.findCategoryUUIDForType(LLAssetType::AT_CLOTHING),
		LLAssetType::AT_NONE,
		new_folder_name);

	bool found_first_item = false;

	///////////////////
	// Wearables

	if( wearables_to_include.count() )
	{
		// Then, iterate though each of the wearables and save copies of them in the folder.
		S32 i;
		S32 count = wearables_to_include.count();
		LLDynamicArray<LLUUID> delete_items;
		LLPointer<LLRefCount> cbdone = NULL;
		for( i = 0; i < count; ++i )
		{
			S32 index = wearables_to_include[i];
			LLWearable* old_wearable = mWearableEntry[ index ].mWearable;
			if( old_wearable )
			{
				std::string new_name;
				LLWearable* new_wearable;
				new_wearable = gWearableList.createCopy(old_wearable);
				if (rename_clothing)
				{
					new_name = new_folder_name;
					new_name.append(" ");
					new_name.append(old_wearable->getTypeLabel());
					LLString::truncate(new_name, DB_INV_ITEM_NAME_STR_LEN);
					new_wearable->setName(new_name);
				}

				LLViewerInventoryItem* item = gInventory.getItem(mWearableEntry[index].mItemID);
				S32 todo = addWearableToAgentInventoryCallback::CALL_NONE;
				if (!found_first_item)
				{
					found_first_item = true;
					/* set the focus to the first item */
					todo |= addWearableToAgentInventoryCallback::CALL_MAKENEWOUTFITDONE;
					/* send the agent wearables update when done */
					cbdone = new sendAgentWearablesUpdateCallback;
				}
				LLPointer<LLInventoryCallback> cb =
					new addWearableToAgentInventoryCallback(
						cbdone,
						index,
						new_wearable,
						todo);
				if (isWearableCopyable((EWearableType)index))
				{
					copy_inventory_item(
						gAgent.getID(),
						item->getPermissions().getOwner(),
						item->getUUID(),
						folder_id,
						new_name,
						cb);
				}
				else
				{
					move_inventory_item(
						gAgent.getID(),
						gAgent.getSessionID(),
						item->getUUID(),
						folder_id,
						new_name,
						cb);
				}
			}
		}
		gInventory.notifyObservers();
	}


	///////////////////
	// Attachments

	if( attachments_to_include.count() )
	{
		BOOL msg_started = FALSE;
		LLMessageSystem* msg = gMessageSystem;
		for( S32 i = 0; i < attachments_to_include.count(); i++ )
		{
			S32 attachment_pt = attachments_to_include[i];
			LLViewerJointAttachment* attachment = get_if_there(mAvatarObject->mAttachmentPoints, attachment_pt, (LLViewerJointAttachment*)NULL );
			if(!attachment) continue;
			LLViewerObject* attached_object = attachment->getObject();
			if(!attached_object) continue;
			const LLUUID& item_id = attachment->getItemID();
			if(item_id.isNull()) continue;
			LLInventoryItem* item = gInventory.getItem(item_id);
			if(!item) continue;
			if(!msg_started)
			{
				msg_started = TRUE;
				msg->newMessage("CreateNewOutfitAttachments");
				msg->nextBlock("AgentData");
				msg->addUUID("AgentID", getID());
				msg->addUUID("SessionID", getSessionID());
				msg->nextBlock("HeaderData");
				msg->addUUID("NewFolderID", folder_id);
			}
			msg->nextBlock("ObjectData");
			msg->addUUID("OldItemID", item_id);
			msg->addUUID("OldFolderID", item->getParentUUID());
		}

		if( msg_started )
		{
			sendReliableMessage();
		}

	} 
}

void LLAgent::makeNewOutfitDone(S32 index)
{
	LLUUID first_item_id = mWearableEntry[index].mItemID;
	// Open the inventory and select the first item we added.
	if( first_item_id.notNull() )
	{
		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if(view)
		{
			view->getPanel()->setSelection(first_item_id, TAKE_FOCUS_NO);
		}
	}
}


void LLAgent::addWearableToAgentInventory(
	LLPointer<LLInventoryCallback> cb,
	LLWearable* wearable,
	const LLUUID& category_id,
	BOOL notify)
{
	create_inventory_item(
		gAgent.getID(),
		gAgent.getSessionID(),
		category_id,
		wearable->getTransactionID(),
		wearable->getName(),
		wearable->getDescription(),
		wearable->getAssetType(),
		LLInventoryType::IT_WEARABLE,
		wearable->getType(),
		wearable->getPermissions().getMaskNextOwner(),
		cb);
}

//-----------------------------------------------------------------------------
// sendAgentSetAppearance()
//-----------------------------------------------------------------------------
void LLAgent::sendAgentSetAppearance()
{
	if (mAvatarObject.isNull()) return;

	if (mNumPendingQueries > 0 && !gAgent.cameraCustomizeAvatar()) 
	{
		return;
	}

	llinfos << "TAT: Sent AgentSetAppearance: " <<
		(( mAvatarObject->getTEImage( LLVOAvatar::TEX_HEAD_BAKED )->getID() != IMG_DEFAULT_AVATAR )  ? "HEAD " : "head " ) <<
		(( mAvatarObject->getTEImage( LLVOAvatar::TEX_UPPER_BAKED )->getID() != IMG_DEFAULT_AVATAR ) ? "UPPER " : "upper " ) <<
		(( mAvatarObject->getTEImage( LLVOAvatar::TEX_LOWER_BAKED )->getID() != IMG_DEFAULT_AVATAR ) ? "LOWER " : "lower " ) <<
		(( mAvatarObject->getTEImage( LLVOAvatar::TEX_EYES_BAKED )->getID() != IMG_DEFAULT_AVATAR )  ? "EYES" : "eyes" ) << llendl;
	//dumpAvatarTEs( "sendAgentSetAppearance()" );

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_AgentSetAppearance);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, getID());
	msg->addUUIDFast(_PREHASH_SessionID, getSessionID());

	// correct for the collision tolerance (to make it look like the 
	// agent is actually walking on the ground/object)
	// NOTE -- when we start correcting all of the other Havok geometry 
	// to compensate for the COLLISION_TOLERANCE ugliness we will have 
	// to tweak this number again
	LLVector3 body_size = mAvatarObject->mBodySize;
	msg->addVector3Fast(_PREHASH_Size, body_size);	

	// To guard against out of order packets
	// Note: always start by sending 1.  This resets the server's count. 0 on the server means "uninitialized"
	mAppearanceSerialNum++;
	msg->addU32Fast(_PREHASH_SerialNum, mAppearanceSerialNum );

	// is texture data current relative to wearables?
	// KLW - TAT this will probably need to check the local queue.
	BOOL textures_current = !mAvatarObject->hasPendingBakedUploads() && mWearablesLoaded;

	S32 baked_texture_index;
	for( baked_texture_index = 0; baked_texture_index < BAKED_TEXTURE_COUNT; baked_texture_index++ )
	{
		S32 tex_index = LLVOAvatar::sBakedTextureIndices[baked_texture_index];

		// if we're not wearing a skirt, we don't need the texture to be baked
		if (tex_index == LLVOAvatar::TEX_SKIRT_BAKED && !mAvatarObject->isWearingWearableType(WT_SKIRT))
		{
			continue;
		}

		// IMG_DEFAULT_AVATAR means not baked
		if (mAvatarObject->getTEImage( tex_index)->getID() == IMG_DEFAULT_AVATAR)
		{
			textures_current = FALSE;
			break;
		}
	}

	// only update cache entries if we have all our baked textures
	if (textures_current)
	{
		llinfos << "TAT: Sending cached texture data" << llendl;
		for (baked_texture_index = 0; baked_texture_index < BAKED_TEXTURE_COUNT; baked_texture_index++)
		{
			LLUUID hash;

			for( S32 wearable_num = 0; wearable_num < MAX_WEARABLES_PER_LAYERSET; wearable_num++ )
			{
				EWearableType wearable_type = WEARABLE_BAKE_TEXTURE_MAP[baked_texture_index][wearable_num];

				LLWearable* wearable = getWearable( wearable_type );
				if (wearable)
				{
					hash ^= wearable->getID();
				}
			}

			if (hash.notNull())
			{
				hash ^= BAKED_TEXTURE_HASH[baked_texture_index];
			}

			S32 tex_index = LLVOAvatar::sBakedTextureIndices[baked_texture_index];

			msg->nextBlockFast(_PREHASH_WearableData);
			msg->addUUIDFast(_PREHASH_CacheID, hash);
			msg->addU8Fast(_PREHASH_TextureIndex, (U8)tex_index);
		}
	}

	msg->nextBlockFast(_PREHASH_ObjectData);
	mAvatarObject->packTEMessage( gMessageSystem );

	S32 transmitted_params = 0;
	for (LLViewerVisualParam* param = (LLViewerVisualParam*)mAvatarObject->getFirstVisualParam();
		 param;
		 param = (LLViewerVisualParam*)mAvatarObject->getNextVisualParam())
	{
		F32 param_value = param->getWeight();
	
		if (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)
		{
			msg->nextBlockFast(_PREHASH_VisualParam );
			
			// We don't send the param ids.  Instead, we assume that the receiver has the same params in the same sequence.
			U8 new_weight = F32_to_U8(param_value, param->getMinWeight(), param->getMaxWeight());
			msg->addU8Fast(_PREHASH_ParamValue, new_weight );
			transmitted_params++;
		}
	}

//	llinfos << "Avatar XML num VisualParams transmitted = " << transmitted_params << llendl;
	sendReliableMessage();
}

void LLAgent::sendAgentDataUpdateRequest()
{
	gMessageSystem->newMessageFast(_PREHASH_AgentDataUpdateRequest);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	sendReliableMessage();
}

void LLAgent::removeWearable( EWearableType type )
{
	LLWearable* old_wearable = mWearableEntry[ type ].mWearable;

	if ( (gAgent.isTeen())
		 && (type == WT_UNDERSHIRT || type == WT_UNDERPANTS))
	{
		// Can't take off underclothing in simple UI mode or on PG accounts
		return;
	}

	if( old_wearable )
	{
		if( old_wearable->isDirty() )
		{
			// Bring up view-modal dialog: Save changes? Yes, No, Cancel
			gViewerWindow->alertXml("WearableSave", LLAgent::onRemoveWearableDialog, (void*)type );
			return;
		}
		else
		{
			removeWearableFinal( type );
		}
	}
}

// static 
void LLAgent::onRemoveWearableDialog( S32 option, void* userdata )
{
	EWearableType type = (EWearableType)(intptr_t)userdata;
	switch( option )
	{
	case 0:  // "Save"
		gAgent.saveWearable( type );
		gAgent.removeWearableFinal( type );
		break;

	case 1:  // "Don't Save"
		gAgent.removeWearableFinal( type );
		break;

	case 2: // "Cancel"
		break;

	default:
		llassert(0);
		break;
	}
}

// Called by removeWearable() and onRemoveWearableDialog() to actually do the removal.
void LLAgent::removeWearableFinal( EWearableType type )
{
	LLWearable* old_wearable = mWearableEntry[ type ].mWearable;

	gInventory.addChangedMask( LLInventoryObserver::LABEL, mWearableEntry[type].mItemID );

	mWearableEntry[ type ].mWearable = NULL;
	mWearableEntry[ type ].mItemID.setNull();

	queryWearableCache();

	if( old_wearable )
	{
		old_wearable->removeFromAvatar( TRUE );
	}

	// Update the server
	sendAgentWearablesUpdate(); 
	sendAgentSetAppearance();
	gInventory.notifyObservers();
}

void LLAgent::copyWearableToInventory( EWearableType type )
{
	LLWearable* wearable = mWearableEntry[ type ].mWearable;
	if( wearable )
	{
		// Save the old wearable if it has changed.
		if( wearable->isDirty() )
		{
			wearable = gWearableList.createCopyFromAvatar( wearable );
			mWearableEntry[ type ].mWearable = wearable;
		}

		// Make a new entry in the inventory.  (Put it in the same folder as the original item if possible.)
		LLUUID category_id;
		LLInventoryItem* item = gInventory.getItem( mWearableEntry[ type ].mItemID );
		if( item )
		{
			category_id = item->getParentUUID();
			wearable->setPermissions(item->getPermissions());
		}
		LLPointer<LLInventoryCallback> cb =
			new addWearableToAgentInventoryCallback(
				LLPointer<LLRefCount>(NULL),
				type,
				wearable);
		addWearableToAgentInventory(cb, wearable, category_id);
	}
}


// A little struct to let setWearable() communicate more than one value with onSetWearableDialog().
struct LLSetWearableData
{
	LLSetWearableData( const LLUUID& new_item_id, LLWearable* new_wearable ) :
		mNewItemID( new_item_id ), mNewWearable( new_wearable ) {}
	LLUUID				mNewItemID;
	LLWearable*			mNewWearable;
};

BOOL LLAgent::needsReplacement(EWearableType  wearableType, S32 remove)
{
	return TRUE;
	/*if (remove) return TRUE;
	
	return getWearable(wearableType) ? TRUE : FALSE;*/
}

// Assumes existing wearables are not dirty.
void LLAgent::setWearableOutfit( 
	const LLInventoryItem::item_array_t& items,
	const LLDynamicArray< LLWearable* >& wearables,
	BOOL remove )
{
	lldebugs << "setWearableOutfit() start" << llendl;

	BOOL wearables_to_remove[WT_COUNT];
	wearables_to_remove[WT_SHAPE]		= FALSE;
	wearables_to_remove[WT_SKIN]		= FALSE;
	wearables_to_remove[WT_HAIR]		= FALSE;
	wearables_to_remove[WT_EYES]		= FALSE;
	wearables_to_remove[WT_SHIRT]		= remove;
	wearables_to_remove[WT_PANTS]		= remove;
	wearables_to_remove[WT_SHOES]		= remove;
	wearables_to_remove[WT_SOCKS]		= remove;
	wearables_to_remove[WT_JACKET]		= remove;
	wearables_to_remove[WT_GLOVES]		= remove;
	wearables_to_remove[WT_UNDERSHIRT]	= (!gAgent.isTeen()) & remove;
	wearables_to_remove[WT_UNDERPANTS]	= (!gAgent.isTeen()) & remove;
	wearables_to_remove[WT_SKIRT]		= remove;

	S32 count = wearables.count();
	llassert( items.count() == count );

	S32 i;
	for( i = 0; i < count; i++ )
	{
		LLWearable* new_wearable = wearables[i];
		LLPointer<LLInventoryItem> new_item = items[i];

		EWearableType type = new_wearable->getType();
		wearables_to_remove[type] = FALSE;

		LLWearable* old_wearable = mWearableEntry[ type ].mWearable;
		if( old_wearable )
		{
			const LLUUID& old_item_id = mWearableEntry[ type ].mItemID;
			if( (old_wearable->getID() == new_wearable->getID()) &&
				(old_item_id == new_item->getUUID()) )
			{
				lldebugs << "No change to wearable asset and item: " << LLWearable::typeToTypeName( type ) << llendl;
				continue;
			}

			gInventory.addChangedMask(LLInventoryObserver::LABEL, old_item_id);

			// Assumes existing wearables are not dirty.
			if( old_wearable->isDirty() )
			{
				llassert(0);
				continue;
			}
		}

		mWearableEntry[ type ].mItemID = new_item->getUUID();
		mWearableEntry[ type ].mWearable = new_wearable;
	}

	std::vector<LLWearable*> wearables_being_removed;

	for( i = 0; i < WT_COUNT; i++ )
	{
		if( wearables_to_remove[i] )
		{
			wearables_being_removed.push_back(mWearableEntry[ i ].mWearable);
			mWearableEntry[ i ].mWearable = NULL;
			
			gInventory.addChangedMask(LLInventoryObserver::LABEL, mWearableEntry[ i ].mItemID);
			mWearableEntry[ i ].mItemID.setNull();
		}
	}

	gInventory.notifyObservers();

	queryWearableCache();

	std::vector<LLWearable*>::iterator wearable_iter;

	for( wearable_iter = wearables_being_removed.begin(); 
		wearable_iter != wearables_being_removed.end();
		++wearable_iter)
	{
		LLWearable* wearablep = *wearable_iter;
		if (wearablep)
		{
			wearablep->removeFromAvatar( TRUE );
		}
	}

	for( i = 0; i < count; i++ )
	{
		wearables[i]->writeToAvatar( TRUE );
	}

	LLFloaterCustomize::setCurrentWearableType( WT_SHAPE );

	// Start rendering & update the server
	mWearablesLoaded = TRUE; 
	sendAgentWearablesUpdate();
	sendAgentSetAppearance();

	lldebugs << "setWearableOutfit() end" << llendl;
}


// User has picked "wear on avatar" from a menu.
void LLAgent::setWearable( LLInventoryItem* new_item, LLWearable* new_wearable )
{
	EWearableType type = new_wearable->getType();

	LLWearable* old_wearable = mWearableEntry[ type ].mWearable;
	if( old_wearable )
	{
		const LLUUID& old_item_id = mWearableEntry[ type ].mItemID;
		if( (old_wearable->getID() == new_wearable->getID()) &&
			(old_item_id == new_item->getUUID()) )
		{
			lldebugs << "No change to wearable asset and item: " << LLWearable::typeToTypeName( type ) << llendl;
			return;
		}

		if( old_wearable->isDirty() )
		{
			// Bring up modal dialog: Save changes? Yes, No, Cancel
			gViewerWindow->alertXml( "WearableSave", LLAgent::onSetWearableDialog,
				new LLSetWearableData( new_item->getUUID(), new_wearable ));
			return;
		}
	}

	setWearableFinal( new_item, new_wearable );
}

// static 
void LLAgent::onSetWearableDialog( S32 option, void* userdata )
{
	LLSetWearableData* data = (LLSetWearableData*)userdata;
	LLInventoryItem* new_item = gInventory.getItem( data->mNewItemID );
	if( !new_item )
	{
		delete data;
		return;
	}

	switch( option )
	{
	case 0:  // "Save"
		gAgent.saveWearable( data->mNewWearable->getType() );
		gAgent.setWearableFinal( new_item, data->mNewWearable );
		break;

	case 1:  // "Don't Save"
		gAgent.setWearableFinal( new_item, data->mNewWearable );
		break;

	case 2: // "Cancel"
		break;

	default:
		llassert(0);
		break;
	}

	delete data;
}

// Called from setWearable() and onSetWearableDialog() to actually set the wearable.
void LLAgent::setWearableFinal( LLInventoryItem* new_item, LLWearable* new_wearable )
{
	EWearableType type = new_wearable->getType();

	// Replace the old wearable with a new one.
	llassert( new_item->getAssetUUID() == new_wearable->getID() );
	LLUUID old_item_id = mWearableEntry[ type ].mItemID;
	mWearableEntry[ type ].mItemID = new_item->getUUID();
	mWearableEntry[ type ].mWearable = new_wearable;

	if (old_item_id.notNull())
	{
		gInventory.addChangedMask( LLInventoryObserver::LABEL, old_item_id );
		gInventory.notifyObservers();
	}

	//llinfos << "LLVOAvatar::setWearable()" << llendl;
	queryWearableCache();
	new_wearable->writeToAvatar( TRUE );

	// Update the server
	sendAgentWearablesUpdate();
	sendAgentSetAppearance();
}

void LLAgent::queryWearableCache()
{
	if (!mWearablesLoaded)
	{
		return;
	}

	// Look up affected baked textures.
	// If they exist:
	//		disallow updates for affected layersets (until dataserver responds with cache request.)
	//		If cache miss, turn updates back on and invalidate composite.
	//		If cache hit, modify baked texture entries.
	//
	// Cache requests contain list of hashes for each baked texture entry.
	// Response is list of valid baked texture assets. (same message)

	gMessageSystem->newMessageFast(_PREHASH_AgentCachedTexture);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, getSessionID());
	gMessageSystem->addS32Fast(_PREHASH_SerialNum, mTextureCacheQueryID);

	S32 num_queries = 0;
	for (S32 baked_texture_index = 0; baked_texture_index < BAKED_TEXTURE_COUNT; baked_texture_index++)
	{
		LLUUID hash;
		for (S32 wearable_num = 0; wearable_num < MAX_WEARABLES_PER_LAYERSET; wearable_num++)
		{
			EWearableType wearable_type = WEARABLE_BAKE_TEXTURE_MAP[baked_texture_index][wearable_num];
				
			LLWearable* wearable = getWearable( wearable_type );
			if (wearable)
			{
				hash ^= wearable->getID();
			}
		}
		if (hash.notNull())
		{
			hash ^= BAKED_TEXTURE_HASH[baked_texture_index];
			num_queries++;
			// *NOTE: make sure at least one request gets packed

			//llinfos << "Requesting texture for hash " << hash << " in baked texture slot " << baked_texture_index << llendl;
			gMessageSystem->nextBlockFast(_PREHASH_WearableData);
			gMessageSystem->addUUIDFast(_PREHASH_ID, hash);
			gMessageSystem->addU8Fast(_PREHASH_TextureIndex, (U8)baked_texture_index);
		}

		mActiveCacheQueries[ baked_texture_index ] = mTextureCacheQueryID;
	}

	llinfos << "Requesting texture cache entry for " << num_queries << " baked textures" << llendl;
	gMessageSystem->sendReliable(getRegion()->getHost());
	mNumPendingQueries++;
	mTextureCacheQueryID++;
}

// User has picked "remove from avatar" from a menu.
// static
void LLAgent::userRemoveWearable( void* userdata )
{
	EWearableType type = (EWearableType)(intptr_t)userdata;
	
	if( !(type==WT_SHAPE || type==WT_SKIN || type==WT_HAIR ) ) //&&
		//!((!gAgent.isTeen()) && ( type==WT_UNDERPANTS || type==WT_UNDERSHIRT )) )
	{
		gAgent.removeWearable( type );
	}
}

void LLAgent::userRemoveAllClothes( void* userdata )
{
	// We have to do this up front to avoid having to deal with the case of multiple wearables being dirty.
	if( gFloaterCustomize )
	{
		gFloaterCustomize->askToSaveAllIfDirty( LLAgent::userRemoveAllClothesStep2, NULL );
	}
	else
	{
		LLAgent::userRemoveAllClothesStep2( TRUE, NULL );
	}
}

void LLAgent::userRemoveAllClothesStep2( BOOL proceed, void* userdata )
{
	if( proceed )
	{
		gAgent.removeWearable( WT_SHIRT );
		gAgent.removeWearable( WT_PANTS );
		gAgent.removeWearable( WT_SHOES );
		gAgent.removeWearable( WT_SOCKS );
		gAgent.removeWearable( WT_JACKET );
		gAgent.removeWearable( WT_GLOVES );
		gAgent.removeWearable( WT_UNDERSHIRT );
		gAgent.removeWearable( WT_UNDERPANTS );
		gAgent.removeWearable( WT_SKIRT );
	}
}

void LLAgent::userRemoveAllAttachments( void* userdata )
{
	LLVOAvatar* avatarp = gAgent.getAvatarObject();
	if(!avatarp)
	{
		llwarns << "No avatar found." << llendl;
		return;
	}

	gMessageSystem->newMessage("ObjectDetach");
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
		 iter != avatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		LLViewerObject* objectp = attachment->getObject();
		if (objectp)
		{
			gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
			gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, objectp->getLocalID());
		}
	}
	gMessageSystem->sendReliable( gAgent.getRegionHost() );
}

void LLAgent::observeFriends()
{
	if(!mFriendObserver)
	{
		mFriendObserver = new LLAgentFriendObserver;
		LLAvatarTracker::instance().addObserver(mFriendObserver);
		friendsChanged();
	}
}

void LLAgent::parseTeleportMessages(const LLString& xml_filename)
{
	LLXMLNodePtr root;
	BOOL success = LLUICtrlFactory::getLayeredXMLNode(xml_filename, root);

	if (!success || !root || !root->hasName( "teleport_messages" ))
	{
		llerrs << "Problem reading teleport string XML file: " 
			   << xml_filename << llendl;
		return;
	}

	for (LLXMLNode* message_set = root->getFirstChild();
		 message_set != NULL;
		 message_set = message_set->getNextSibling())
	{
		if ( !message_set->hasName("message_set") ) continue;

		std::map<LLString, LLString> *teleport_msg_map = NULL;
		LLString message_set_name;

		if ( message_set->getAttributeString("name", message_set_name) )
		{
			//now we loop over all the string in the set and add them
			//to the appropriate set
			if ( message_set_name == "errors" )
			{
				teleport_msg_map = &sTeleportErrorMessages;
			}
			else if ( message_set_name == "progress" )
			{
				teleport_msg_map = &sTeleportProgressMessages;
			}
		}

		if ( !teleport_msg_map ) continue;

		LLString message_name;
		for (LLXMLNode* message_node = message_set->getFirstChild();
			 message_node != NULL;
			 message_node = message_node->getNextSibling())
		{
			if ( message_node->hasName("message") && 
				 message_node->getAttributeString("name", message_name) )
			{
				(*teleport_msg_map)[message_name] =
					message_node->getTextContents();
			} //end if ( message exists and has a name)
		} //end for (all message in set)
	}//end for (all message sets in xml file)
}

// EOF
