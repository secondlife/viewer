/**
 * @File llvoavatar.cpp
 * @brief Implementation of LLVOAvatar class which is a derivation of LLViewerObject
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llvoavatar.h"

#include <stdio.h>
#include <ctype.h>
#include <sstream>

#include "llaudioengine.h"
#include "noise.h"
#include "sound_ids.h"
#include "raytrace.h"

#include "llagent.h" //  Get state values from here
#include "llagentbenefits.h"
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llanimationstates.h"
#include "llavatarnamecache.h"
#include "llavatarpropertiesprocessor.h"
#include "llavatarrendernotifier.h"
#include "llcontrolavatar.h"
#include "llexperiencecache.h"
#include "llphysicsmotion.h"
#include "llviewercontrol.h"
#include "llcallingcard.h"		// IDEVO for LLAvatarTracker
#include "lldrawpoolavatar.h"
#include "lldriverparam.h"
#include "llpolyskeletaldistortion.h"
#include "lleditingmotion.h"
#include "llemote.h"
#include "llfloatertools.h"
#include "llheadrotmotion.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llhudnametag.h"
#include "llhudtext.h"				// for mText/mDebugText
#include "llimview.h"
#include "llinitparam.h"
#include "llkeyframefallmotion.h"
#include "llkeyframestandmotion.h"
#include "llkeyframewalkmotion.h"
#include "llmanipscale.h"  // for get_default_max_prim_scale()
#include "llmeshrepository.h"
#include "llmutelist.h"
#include "llmoveview.h"
#include "llnotificationsutil.h"
#include "llphysicsshapebuilderutil.h"
#include "llquantize.h"
#include "llrand.h"
#include "llregionhandle.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsprite.h"
#include "lltargetingmotion.h"
#include "lltoolmgr.h"
#include "lltoolmorph.h"
#include "llviewercamera.h"
#include "llviewertexlayer.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewershadermgr.h"
#include "llviewerstats.h"
#include "llviewerwearable.h"
#include "llvoavatarself.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llsky.h"
#include "llanimstatelabels.h"
#include "lltrans.h"
#include "llappearancemgr.h"

#include "llgesturemgr.h" //needed to trigger the voice gesticulations
#include "llvoiceclient.h"
#include "llvoicevisualizer.h" // Ventrella

#include "lldebugmessagebox.h"
#include "llsdutil.h"
#include "llscenemonitor.h"
#include "llsdserialize.h"
#include "llcallstack.h"
#include "llrendersphere.h"
#include "llskinningutil.h"

#include "llperfstats.h"

#include <boost/lexical_cast.hpp>

extern F32 SPEED_ADJUST_MAX;
extern F32 SPEED_ADJUST_MAX_SEC;
extern F32 ANIM_SPEED_MAX;
extern F32 ANIM_SPEED_MIN;
extern U32 JOINT_COUNT_REQUIRED_FOR_FULLRIG;

const F32 MAX_HOVER_Z = 2.0;
const F32 MIN_HOVER_Z = -2.0;

const F32 MIN_ATTACHMENT_COMPLEXITY = 0.f;
const F32 DEFAULT_MAX_ATTACHMENT_COMPLEXITY = 1.0e6f;

// Unlike with 'self' avatar, server doesn't inform viewer about
// expected attachments so viewer has to wait to see if anything
// else will arrive
const F32 FIRST_APPEARANCE_CLOUD_MIN_DELAY = 3.f; // seconds
const F32 FIRST_APPEARANCE_CLOUD_MAX_DELAY = 45.f;

using namespace LLAvatarAppearanceDefines;

//-----------------------------------------------------------------------------
// Global constants
//-----------------------------------------------------------------------------
const LLUUID ANIM_AGENT_BODY_NOISE = LLUUID("9aa8b0a6-0c6f-9518-c7c3-4f41f2c001ad"); //"body_noise"
const LLUUID ANIM_AGENT_BREATHE_ROT	= LLUUID("4c5a103e-b830-2f1c-16bc-224aa0ad5bc8");  //"breathe_rot"
const LLUUID ANIM_AGENT_EDITING	= LLUUID("2a8eba1d-a7f8-5596-d44a-b4977bf8c8bb");  //"editing"
const LLUUID ANIM_AGENT_EYE	= LLUUID("5c780ea8-1cd1-c463-a128-48c023f6fbea");  //"eye"
const LLUUID ANIM_AGENT_FLY_ADJUST = LLUUID("db95561f-f1b0-9f9a-7224-b12f71af126e");  //"fly_adjust"
const LLUUID ANIM_AGENT_HAND_MOTION	= LLUUID("ce986325-0ba7-6e6e-cc24-b17c4b795578");  //"hand_motion"
const LLUUID ANIM_AGENT_HEAD_ROT = LLUUID("e6e8d1dd-e643-fff7-b238-c6b4b056a68d");  //"head_rot"
const LLUUID ANIM_AGENT_PELVIS_FIX = LLUUID("0c5dd2a2-514d-8893-d44d-05beffad208b");  //"pelvis_fix"
const LLUUID ANIM_AGENT_TARGET = LLUUID("0e4896cb-fba4-926c-f355-8720189d5b55");  //"target"
const LLUUID ANIM_AGENT_WALK_ADJUST	= LLUUID("829bc85b-02fc-ec41-be2e-74cc6dd7215d");  //"walk_adjust"
const LLUUID ANIM_AGENT_PHYSICS_MOTION = LLUUID("7360e029-3cb8-ebc4-863e-212df440d987");  //"physics_motion"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const F32 DELTA_TIME_MIN = 0.01f;	// we clamp measured delta_time to this
const F32 DELTA_TIME_MAX = 0.2f;	// range to insure stability of computations.

const F32 PELVIS_LAG_FLYING		= 0.22f;// pelvis follow half life while flying
const F32 PELVIS_LAG_WALKING	= 0.4f;	// ...while walking
const F32 PELVIS_LAG_MOUSELOOK = 0.15f;
const F32 MOUSELOOK_PELVIS_FOLLOW_FACTOR = 0.5f;
const F32 TORSO_NOISE_AMOUNT = 1.0f;	// Amount of deviation from up-axis, in degrees
const F32 TORSO_NOISE_SPEED = 0.2f;	// Time scale factor on torso noise.

const F32 BREATHE_ROT_MOTION_STRENGTH = 0.05f;

const S32 MIN_REQUIRED_PIXEL_AREA_BODY_NOISE = 10000;
const S32 MIN_REQUIRED_PIXEL_AREA_BREATHE = 10000;
const S32 MIN_REQUIRED_PIXEL_AREA_PELVIS_FIX = 40;

const S32 TEX_IMAGE_SIZE_OTHER = 512 / 4;  // The size of local textures for other (!isSelf()) avatars

const F32 HEAD_MOVEMENT_AVG_TIME = 0.9f;

const S32 MORPH_MASK_REQUESTED_DISCARD = 0;

const F32 MAX_STANDOFF_FROM_ORIGIN = 3;
const F32 MAX_STANDOFF_DISTANCE_CHANGE = 32;

// Discard level at which to switch to baked textures
// Should probably be 4 or 3, but didn't want to change it while change other logic - SJB
const S32 SWITCH_TO_BAKED_DISCARD = 5;

const F32 HOVER_EFFECT_MAX_SPEED = 3.f;
const F32 HOVER_EFFECT_STRENGTH = 0.f;
const F32 UNDERWATER_EFFECT_STRENGTH = 0.1f;
const F32 UNDERWATER_FREQUENCY_DAMP = 0.33f;
const F32 APPEARANCE_MORPH_TIME = 0.65f;
const F32 TIME_BEFORE_MESH_CLEANUP = 5.f; // seconds
const S32 AVATAR_RELEASE_THRESHOLD = 10; // number of avatar instances before releasing memory
const F32 FOOT_GROUND_COLLISION_TOLERANCE = 0.25f;
const F32 AVATAR_LOD_TWEAK_RANGE = 0.7f;
const S32 MAX_BUBBLE_CHAT_LENGTH = DB_CHAT_MSG_STR_LEN;
const S32 MAX_BUBBLE_CHAT_UTTERANCES = 12;
const F32 CHAT_FADE_TIME = 8.0;
const F32 BUBBLE_CHAT_TIME = CHAT_FADE_TIME * 3.f;
const F32 NAMETAG_UPDATE_THRESHOLD = 0.3f;
const F32 NAMETAG_VERTICAL_SCREEN_OFFSET = 25.f;
const F32 NAMETAG_VERT_OFFSET_WEIGHT = 0.17f;

const U32 LLVOAvatar::VISUAL_COMPLEXITY_UNKNOWN = 0;
const F64 HUD_OVERSIZED_TEXTURE_DATA_SIZE = 1024 * 1024;

const F32 MAX_TEXTURE_WAIT_TIME_SEC = 60;

const S32 MIN_NONTUNED_AVS = 5;

enum ERenderName
{
	RENDER_NAME_NEVER,
	RENDER_NAME_ALWAYS,	
	RENDER_NAME_FADE
};

#define JELLYDOLLS_SHOULD_IMPOSTOR

//-----------------------------------------------------------------------------
// Callback data
//-----------------------------------------------------------------------------

struct LLTextureMaskData
{
	LLTextureMaskData( const LLUUID& id ) :
		mAvatarID(id), 
		mLastDiscardLevel(S32_MAX) 
	{}
	LLUUID				mAvatarID;
	S32					mLastDiscardLevel;
};

/*********************************************************************************
 **                                                                             **
 ** Begin private LLVOAvatar Support classes
 **
 **/


struct LLAppearanceMessageContents: public LLRefCount
{
	LLAppearanceMessageContents():
		mAppearanceVersion(-1),
		mParamAppearanceVersion(-1),
		mCOFVersion(LLViewerInventoryCategory::VERSION_UNKNOWN)
	{
	}
	LLTEContents mTEContents;
	S32 mAppearanceVersion;
	S32 mParamAppearanceVersion;
	S32 mCOFVersion;
	// For future use:
	//U32 appearance_flags = 0;
	std::vector<F32> mParamWeights;
	std::vector<LLVisualParam*> mParams;
	LLVector3 mHoverOffset;
	bool mHoverOffsetWasSet;
};


//-----------------------------------------------------------------------------
// class LLBodyNoiseMotion
//-----------------------------------------------------------------------------
class LLBodyNoiseMotion :
	public LLMotion
{
public:
	// Constructor
	LLBodyNoiseMotion(const LLUUID &id)
		: LLMotion(id)
	{
		mName = "body_noise";
		mTorsoState = new LLJointState;
	}

	// Destructor
	virtual ~LLBodyNoiseMotion() { }

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------
	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLBodyNoiseMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual bool getLoop() { return true; }

	// motions must report their total duration
	virtual F32 getDuration() { return 0.0; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return 0.0; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return 0.0; }

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return LLJoint::HIGH_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return ADDITIVE_BLEND; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_BODY_NOISE; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	virtual LLMotionInitStatus onInitialize(LLCharacter *character)
	{
		if( !mTorsoState->setJoint( character->getJoint("mTorso") ))
		{
			return STATUS_FAILURE;
		}

		mTorsoState->setUsage(LLJointState::ROT);

		addJointState( mTorsoState );
		return STATUS_SUCCESS;
	}

	// called when a motion is activated
	// must return true to indicate success, or else
	// it will be deactivated
	virtual bool onActivate() { return true; }

	// called per time step
	// must return true while it is active, and
	// must return false when the motion is completed.
	virtual bool onUpdate(F32 time, U8* joint_mask)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
		F32 nx[2];
		nx[0]=time*TORSO_NOISE_SPEED;
		nx[1]=0.0f;
		F32 ny[2];
		ny[0]=0.0f;
		ny[1]=time*TORSO_NOISE_SPEED;
		F32 noiseX = noise2(nx);
		F32 noiseY = noise2(ny);

		F32 rx = TORSO_NOISE_AMOUNT * DEG_TO_RAD * noiseX / 0.42f;
		F32 ry = TORSO_NOISE_AMOUNT * DEG_TO_RAD * noiseY / 0.42f;
		LLQuaternion tQn;
		tQn.setQuat( rx, ry, 0.0f );
		mTorsoState->setRotation( tQn );

		return true;
	}

	// called when a motion is deactivated
	virtual void onDeactivate() {}

private:
	//-------------------------------------------------------------------------
	// joint states to be animated
	//-------------------------------------------------------------------------
	LLPointer<LLJointState> mTorsoState;
};

//-----------------------------------------------------------------------------
// class LLBreatheMotionRot
//-----------------------------------------------------------------------------
class LLBreatheMotionRot :
	public LLMotion
{
public:
	// Constructor
	LLBreatheMotionRot(const LLUUID &id) :
		LLMotion(id),
		mBreatheRate(1.f),
		mCharacter(NULL)
	{
		mName = "breathe_rot";
		mChestState = new LLJointState;
	}

	// Destructor
	virtual ~LLBreatheMotionRot() {}

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------
	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLBreatheMotionRot(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual bool getLoop() { return true; }

	// motions must report their total duration
	virtual F32 getDuration() { return 0.0; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return 0.0; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return 0.0; }

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_BREATHE; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	virtual LLMotionInitStatus onInitialize(LLCharacter *character)
	{		
		mCharacter = character;
		bool success = true;

		if ( !mChestState->setJoint( character->getJoint( "mChest" ) ) )
		{
			success = false;
		}

		if ( success )
		{
			mChestState->setUsage(LLJointState::ROT);
			addJointState( mChestState );
		}

		if ( success )
		{
			return STATUS_SUCCESS;
		}
		else
		{
			return STATUS_FAILURE;
		}
	}

	// called when a motion is activated
	// must return true to indicate success, or else
	// it will be deactivated
	virtual bool onActivate() { return true; }

	// called per time step
	// must return true while it is active, and
	// must return false when the motion is completed.
	virtual bool onUpdate(F32 time, U8* joint_mask)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
		mBreatheRate = 1.f;

		F32 breathe_amt = (sinf(mBreatheRate * time) * BREATHE_ROT_MOTION_STRENGTH);

		mChestState->setRotation(LLQuaternion(breathe_amt, LLVector3(0.f, 1.f, 0.f)));

		return true;
	}

	// called when a motion is deactivated
	virtual void onDeactivate() {}

private:
	//-------------------------------------------------------------------------
	// joint states to be animated
	//-------------------------------------------------------------------------
	LLPointer<LLJointState> mChestState;
	F32					mBreatheRate;
	LLCharacter*		mCharacter;
};

//-----------------------------------------------------------------------------
// class LLPelvisFixMotion
//-----------------------------------------------------------------------------
class LLPelvisFixMotion :
	public LLMotion
{
public:
	// Constructor
	LLPelvisFixMotion(const LLUUID &id)
		: LLMotion(id), mCharacter(NULL)
	{
		mName = "pelvis_fix";

		mPelvisState = new LLJointState;
	}

	// Destructor
	virtual ~LLPelvisFixMotion() { }

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------
	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID& id) { return new LLPelvisFixMotion(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual bool getLoop() { return true; }

	// motions must report their total duration
	virtual F32 getDuration() { return 0.0; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return 0.5f; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return 0.5f; }

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return LLJoint::LOW_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_PELVIS_FIX; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	virtual LLMotionInitStatus onInitialize(LLCharacter *character)
	{
		mCharacter = character;

		if (!mPelvisState->setJoint( character->getJoint("mPelvis")))
		{
			return STATUS_FAILURE;
		}

		mPelvisState->setUsage(LLJointState::POS);

		addJointState( mPelvisState );
		return STATUS_SUCCESS;
	}

	// called when a motion is activated
	// must return true to indicate success, or else
	// it will be deactivated
	virtual bool onActivate() { return true; }

	// called per time step
	// must return true while it is active, and
	// must return false when the motion is completed.
	virtual bool onUpdate(F32 time, U8* joint_mask)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
		mPelvisState->setPosition(LLVector3::zero);

		return true;
	}

	// called when a motion is deactivated
	virtual void onDeactivate() {}

private:
	//-------------------------------------------------------------------------
	// joint states to be animated
	//-------------------------------------------------------------------------
	LLPointer<LLJointState> mPelvisState;
	LLCharacter*		mCharacter;
};

/**
 **
 ** End LLVOAvatar Support classes
 **                                                                             **
 *********************************************************************************/


//-----------------------------------------------------------------------------
// Static Data
//-----------------------------------------------------------------------------
U32 LLVOAvatar::sMaxNonImpostors = 12; // Set from RenderAvatarMaxNonImpostors
bool LLVOAvatar::sLimitNonImpostors = false; // True unless RenderAvatarMaxNonImpostors is 0 (unlimited)
F32 LLVOAvatar::sRenderDistance = 256.f;
S32	LLVOAvatar::sNumVisibleAvatars = 0;
S32	LLVOAvatar::sNumLODChangesThisFrame = 0;

const LLUUID LLVOAvatar::sStepSoundOnLand("e8af4a28-aa83-4310-a7c4-c047e15ea0df");
const LLUUID LLVOAvatar::sStepSounds[LL_MCODE_END] =
{
	SND_STONE_RUBBER,
	SND_METAL_RUBBER,
	SND_GLASS_RUBBER,
	SND_WOOD_RUBBER,
	SND_FLESH_RUBBER,
	SND_RUBBER_PLASTIC,
	SND_RUBBER_RUBBER
};

S32 LLVOAvatar::sRenderName = RENDER_NAME_ALWAYS;
bool LLVOAvatar::sRenderGroupTitles = true;
S32 LLVOAvatar::sNumVisibleChatBubbles = 0;
bool LLVOAvatar::sDebugInvisible = false;
bool LLVOAvatar::sShowAttachmentPoints = false;
bool LLVOAvatar::sShowAnimationDebug = false;
bool LLVOAvatar::sVisibleInFirstPerson = false;
F32 LLVOAvatar::sLODFactor = 1.f;
F32 LLVOAvatar::sPhysicsLODFactor = 1.f;
bool LLVOAvatar::sJointDebug = false;
F32 LLVOAvatar::sUnbakedTime = 0.f;
F32 LLVOAvatar::sUnbakedUpdateTime = 0.f;
F32 LLVOAvatar::sGreyTime = 0.f;
F32 LLVOAvatar::sGreyUpdateTime = 0.f;
LLPointer<LLViewerTexture> LLVOAvatar::sCloudTexture = NULL;
std::vector<LLUUID> LLVOAvatar::sAVsIgnoringARTLimit;
S32 LLVOAvatar::sAvatarsNearby = 0;

//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------
static F32 calc_bouncy_animation(F32 x);

//-----------------------------------------------------------------------------
// LLVOAvatar()
//-----------------------------------------------------------------------------
LLVOAvatar::LLVOAvatar(const LLUUID& id,
					   const LLPCode pcode,
					   LLViewerRegion* regionp) :
	LLAvatarAppearance(&gAgentWearables),
	LLViewerObject(id, pcode, regionp),
	mSpecialRenderMode(0),
	mAttachmentSurfaceArea(0.f),
	mAttachmentVisibleTriangleCount(0),
	mAttachmentEstTriangleCount(0.f),
	mReportedVisualComplexity(VISUAL_COMPLEXITY_UNKNOWN),
	mTurning(false),
	mLastSkeletonSerialNum( 0 ),
	mIsSitting(false),
	mTimeVisible(),
	mTyping(false),
	mMeshValid(false),
	mVisible(false),
	mLastImpostorUpdateFrameTime(0.f),
	mLastImpostorUpdateReason(0),
	mWindFreq(0.f),
	mRipplePhase( 0.f ),
	mBelowWater(false),
	mLastAppearanceBlendTime(0.f),
	mAppearanceAnimating(false),
    mNameIsSet(false),
	mTitle(),
	mNameAway(false),
	mNameDoNotDisturb(false),
	mNameMute(false),
	mNameAppearance(false),
	mNameFriend(false),
	mNameAlpha(0.f),
	mRenderGroupTitles(sRenderGroupTitles),
	mNameCloud(false),
	mFirstTEMessageReceived( false ),
	mFirstAppearanceMessageReceived( false ),
	mCulled( false ),
	mVisibilityRank(0),
	mNeedsSkin(false),
	mLastSkinTime(0.f),
	mUpdatePeriod(1),
	mOverallAppearance(AOA_INVISIBLE),
	mVisualComplexityStale(true),
	mVisuallyMuteSetting(AV_RENDER_NORMALLY),
	mMutedAVColor(LLColor4::white /* used for "uninitialize" */),
	mFirstFullyVisible(true),
	mFirstUseDelaySeconds(FIRST_APPEARANCE_CLOUD_MIN_DELAY),
	mFullyLoaded(false),
	mPreviousFullyLoaded(false),
	mFullyLoadedInitialized(false),
	mVisualComplexity(VISUAL_COMPLEXITY_UNKNOWN),
	mLoadedCallbacksPaused(false),
	mLoadedCallbackTextures(0),
	mRenderUnloadedAvatar(LLCachedControl<bool>(gSavedSettings, "RenderUnloadedAvatar", false)),
	mLastRezzedStatus(-1),
	mIsEditingAppearance(false),
	mUseLocalAppearance(false),
	mLastUpdateRequestCOFVersion(-1),
	mLastUpdateReceivedCOFVersion(-1),
	mCachedMuteListUpdateTime(0),
	mCachedInMuteList(false),
    mIsControlAvatar(false),
    mIsUIAvatar(false),
    mEnableDefaultMotions(true)
{
	LL_DEBUGS("AvatarRender") << "LLVOAvatar Constructor (0x" << this << ") id:" << mID << LL_ENDL;

	//VTResume();  // VTune
	setHoverOffset(LLVector3(0.0, 0.0, 0.0));

	// mVoiceVisualizer is created by the hud effects manager and uses the HUD Effects pipeline
	const bool needsSendToSim = false; // currently, this HUD effect doesn't need to pack and unpack data to do its job
	mVoiceVisualizer = ( LLVoiceVisualizer *)LLHUDManager::getInstance()->createViewerEffect( LLHUDObject::LL_HUD_EFFECT_VOICE_VISUALIZER, needsSendToSim );

	LL_DEBUGS("Avatar","Message") << "LLVOAvatar Constructor (0x" << this << ") id:" << mID << LL_ENDL;
	mPelvisp = NULL;

	mDirtyMesh = 2;	// Dirty geometry, need to regenerate.
	mMeshTexturesDirty = false;
	mHeadp = NULL;


	// set up animation variables
	mSpeed = 0.f;
	setAnimationData("Speed", &mSpeed);

	mNeedsImpostorUpdate = true;
	mLastImpostorUpdateReason = 0;
	mNeedsAnimUpdate = true;

	mNeedsExtentUpdate = true;

	mImpostorDistance = 0;
	mImpostorPixelArea = 0;

	setNumTEs(TEX_NUM_INDICES);

	mbCanSelect = true;

	mSignaledAnimations.clear();
	mPlayingAnimations.clear();

	mWasOnGroundLeft = false;
	mWasOnGroundRight = false;

	mTimeLast = 0.0f;
	mSpeedAccum = 0.0f;

	mRippleTimeLast = 0.f;

	mInAir = false;

	mStepOnLand = true;
	mStepMaterial = 0;

	mLipSyncActive = false;
	mOohMorph      = NULL;
	mAahMorph      = NULL;

	mCurrentGesticulationLevel = 0;

    mFirstAppearanceMessageTimer.reset();
	mRuthTimer.reset();
	mRuthDebugTimer.reset();
	mDebugExistenceTimer.reset();
	mLastAppearanceMessageTimer.reset();

	if(LLSceneMonitor::getInstance()->isEnabled())
	{
	    LLSceneMonitor::getInstance()->freezeAvatar((LLCharacter*)this);
	}

	mVisuallyMuteSetting = LLVOAvatar::VisualMuteSettings(LLRenderMuteList::getInstance()->getSavedVisualMuteSetting(getID()));
}

std::string LLVOAvatar::avString() const
{
    if (isControlAvatar())
    {
        return getFullname();
    }
    else
    {
		std::string viz_string = LLVOAvatar::rezStatusToString(getRezzedStatus());
		return " Avatar '" + getFullname() + "' " + viz_string + " ";
    }
}

void LLVOAvatar::debugAvatarRezTime(std::string notification_name, std::string comment)
{
    if (gDisconnected)
    {
        // If we disconected, these values are likely to be invalid and
        // avString() might crash due to a dead sAvatarDictionary
        return;
    }

	LL_INFOS("Avatar") << "REZTIME: [ " << (U32)mDebugExistenceTimer.getElapsedTimeF32()
					   << "sec ]"
					   << avString() 
					   << "RuthTimer " << (U32)mRuthDebugTimer.getElapsedTimeF32()
					   << " Notification " << notification_name
					   << " : " << comment
					   << LL_ENDL;

	if (gSavedSettings.getBOOL("DebugAvatarRezTime"))
	{
		LLSD args;
		args["EXISTENCE"] = llformat("%d",(U32)mDebugExistenceTimer.getElapsedTimeF32());
		args["TIME"] = llformat("%d",(U32)mRuthDebugTimer.getElapsedTimeF32());
		args["NAME"] = getFullname();
		LLNotificationsUtil::add(notification_name,args);
	}
}

//------------------------------------------------------------------------
// LLVOAvatar::~LLVOAvatar()
//------------------------------------------------------------------------
LLVOAvatar::~LLVOAvatar()
{
	if (!mFullyLoaded)
	{
		debugAvatarRezTime("AvatarRezLeftCloudNotification","left after ruth seconds as cloud");
	}
	else
	{
		debugAvatarRezTime("AvatarRezLeftNotification","left sometime after declouding");
	}

    if(mTuned)
    {
        LLPerfStats::tunedAvatars--;
        mTuned = false;
    }
    sAVsIgnoringARTLimit.erase(std::remove(sAVsIgnoringARTLimit.begin(), sAVsIgnoringARTLimit.end(), mID), sAVsIgnoringARTLimit.end());


	logPendingPhases();
	
	LL_DEBUGS("Avatar") << "LLVOAvatar Destructor (0x" << this << ") id:" << mID << LL_ENDL;

	std::for_each(mAttachmentPoints.begin(), mAttachmentPoints.end(), DeletePairedPointer());
	mAttachmentPoints.clear();

	mDead = true;
	
	mAnimationSources.clear();
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;

	getPhases().clearPhases();
	
	LL_DEBUGS() << "LLVOAvatar Destructor end" << LL_ENDL;
}

void LLVOAvatar::markDead()
{
	if (mNameText)
	{
		mNameText->markDead();
		mNameText = NULL;
		sNumVisibleChatBubbles--;
	}
	mVoiceVisualizer->markDead();
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;
	LLViewerObject::markDead();
}


bool LLVOAvatar::isFullyBaked()
{
	if (mIsDummy) return true;
	if (getNumTEs() == 0) return false;

	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		if (!isTextureDefined(mBakedTextureDatas[i].mTextureIndex)
			&& ((i != BAKED_SKIRT) || isWearingWearableType(LLWearableType::WT_SKIRT))
			&& (i != BAKED_LEFT_ARM) && (i != BAKED_LEFT_LEG) && (i != BAKED_AUX1) && (i != BAKED_AUX2) && (i != BAKED_AUX3))
		{
			return false;
		}
	}
	return true;
}

bool LLVOAvatar::isFullyTextured() const
{
	for (S32 i = 0; i < mMeshLOD.size(); i++)
	{
		LLAvatarJoint* joint = mMeshLOD[i];
		if (i==MESH_ID_SKIRT && !isWearingWearableType(LLWearableType::WT_SKIRT))
		{
			continue; // don't care about skirt textures if we're not wearing one.
		}
		if (!joint)
		{
			continue; // nonexistent LOD OK.
		}
		avatar_joint_mesh_list_t::iterator meshIter = joint->mMeshParts.begin();
		if (meshIter != joint->mMeshParts.end())
		{
			LLAvatarJointMesh *mesh = (*meshIter);
			if (!mesh)
			{
				continue; // nonexistent mesh OK
			}
			if (mesh->hasGLTexture())
			{
				continue; // Mesh exists and has a baked texture.
			}
			if (mesh->hasComposite())
			{
				continue; // Mesh exists and has a composite texture.
			}
			// Fail
			return false;
		}
	}
	return true;
}

bool LLVOAvatar::hasGray() const
{
	return !getIsCloud() && !isFullyTextured();
}

S32 LLVOAvatar::getRezzedStatus() const
{
	if (getIsCloud()) return 0;
	bool textured = isFullyTextured();
	if (textured && allBakedTexturesCompletelyDownloaded()) return 3;
	if (textured) return 2;
	llassert(hasGray());
	return 1; // gray
}

void LLVOAvatar::deleteLayerSetCaches(bool clearAll)
{
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		if (mBakedTextureDatas[i].mTexLayerSet)
		{
			// ! BACKWARDS COMPATIBILITY !
			// Can be removed after hair baking is mandatory on the grid
			if ((i != BAKED_HAIR || isSelf()) && !clearAll)
			{
				mBakedTextureDatas[i].mTexLayerSet->deleteCaches();
			}
		}
		if (mBakedTextureDatas[i].mMaskTexName)
		{
			LLImageGL::deleteTextures(1, (GLuint*)&(mBakedTextureDatas[i].mMaskTexName));
			mBakedTextureDatas[i].mMaskTexName = 0 ;
		}
	}
}

// static 
bool LLVOAvatar::areAllNearbyInstancesBaked(S32& grey_avatars)
{
	bool res = true;
	grey_avatars = 0;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if( inst->isDead() )
		{
			continue;
		}
		else if( !inst->isFullyBaked() )
		{
			res = false;
			if (inst->mHasGrey)
			{
				++grey_avatars;
			}
		}
	}
	return res;
}

// static
void LLVOAvatar::getNearbyRezzedStats(std::vector<S32>& counts)
{
	counts.clear();
	counts.resize(4);
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if (inst)
		{
			S32 rez_status = inst->getRezzedStatus();
			counts[rez_status]++;
		}
	}
}

// static
std::string LLVOAvatar::rezStatusToString(S32 rez_status)
{
	if (rez_status==0) return "cloud";
	if (rez_status==1) return "gray";
	if (rez_status==2) return "downloading";
	if (rez_status==3) return "full";
	return "unknown";
}

// static
void LLVOAvatar::dumpBakedStatus()
{
	LLVector3d camera_pos_global = gAgentCamera.getCameraPositionGlobal();

	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		LL_INFOS() << "Avatar ";

		LLNameValue* firstname = inst->getNVPair("FirstName");
		LLNameValue* lastname = inst->getNVPair("LastName");

		if( firstname )
		{
			LL_CONT << firstname->getString();
		}
		if( lastname )
		{
			LL_CONT << " " << lastname->getString();
		}

		LL_CONT << " " << inst->mID;

		if( inst->isDead() )
		{
			LL_CONT << " DEAD ("<< inst->getNumRefs() << " refs)";
		}

		if( inst->isSelf() )
		{
			LL_CONT << " (self)";
		}


		F64 dist_to_camera = (inst->getPositionGlobal() - camera_pos_global).length();
		LL_CONT << " " << dist_to_camera << "m ";

		LL_CONT << " " << inst->mPixelArea << " pixels";

		if( inst->isVisible() )
		{
			LL_CONT << " (visible)";
		}
		else
		{
			LL_CONT << " (not visible)";
		}

		if( inst->isFullyBaked() )
		{
			LL_CONT << " Baked";
		}
		else
		{
			LL_CONT << " Unbaked (";
			
			for (LLAvatarAppearanceDictionary::BakedTextures::const_iterator iter = LLAvatarAppearance::getDictionary()->getBakedTextures().begin();
				 iter != LLAvatarAppearance::getDictionary()->getBakedTextures().end();
				 ++iter)
			{
				const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = iter->second;
				const ETextureIndex index = baked_dict->mTextureIndex;
				if (!inst->isTextureDefined(index))
				{
					LL_CONT << " " << (LLAvatarAppearance::getDictionary()->getTexture(index) ? LLAvatarAppearance::getDictionary()->getTexture(index)->mName : "");
				}
			}
			LL_CONT << " ) " << inst->getUnbakedPixelAreaRank();
			if( inst->isCulled() )
			{
				LL_CONT << " culled";
			}
		}
		LL_CONT << LL_ENDL;
	}
}

//static
void LLVOAvatar::restoreGL()
{
	if (!isAgentAvatarValid()) return;

	gAgentAvatarp->setCompositeUpdatesEnabled(true);
	for (U32 i = 0; i < gAgentAvatarp->mBakedTextureDatas.size(); i++)
	{
		gAgentAvatarp->invalidateComposite(gAgentAvatarp->getTexLayerSet(i));
	}
	gAgentAvatarp->updateMeshTextures();
}

//static
void LLVOAvatar::destroyGL()
{
	deleteCachedImages();

	resetImpostors();
}

//static
void LLVOAvatar::resetImpostors()
{
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* avatar = (LLVOAvatar*) *iter;
		avatar->mImpostor.release();
		avatar->mNeedsImpostorUpdate = true;
		avatar->mLastImpostorUpdateReason = 1;
	}
}

// static
void LLVOAvatar::deleteCachedImages(bool clearAll)
{	
	if (LLViewerTexLayerSet::sHasCaches)
	{
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			 iter != LLCharacter::sInstances.end(); ++iter)
		{
			LLVOAvatar* inst = (LLVOAvatar*) *iter;
			inst->deleteLayerSetCaches(clearAll);
		}
		LLViewerTexLayerSet::sHasCaches = false;
	}
	LLVOAvatarSelf::deleteScratchTextures();
	LLTexLayerStaticImageList::getInstance()->deleteCachedImages();
}


//------------------------------------------------------------------------
// static
// LLVOAvatar::initClass()
//------------------------------------------------------------------------
void LLVOAvatar::initClass()
{ 
	gAnimLibrary.animStateSetString(ANIM_AGENT_BODY_NOISE,"body_noise");
	gAnimLibrary.animStateSetString(ANIM_AGENT_BREATHE_ROT,"breathe_rot");
	gAnimLibrary.animStateSetString(ANIM_AGENT_PHYSICS_MOTION,"physics_motion");
	gAnimLibrary.animStateSetString(ANIM_AGENT_EDITING,"editing");
	gAnimLibrary.animStateSetString(ANIM_AGENT_EYE,"eye");
	gAnimLibrary.animStateSetString(ANIM_AGENT_FLY_ADJUST,"fly_adjust");
	gAnimLibrary.animStateSetString(ANIM_AGENT_HAND_MOTION,"hand_motion");
	gAnimLibrary.animStateSetString(ANIM_AGENT_HEAD_ROT,"head_rot");
	gAnimLibrary.animStateSetString(ANIM_AGENT_PELVIS_FIX,"pelvis_fix");
	gAnimLibrary.animStateSetString(ANIM_AGENT_TARGET,"target");
	gAnimLibrary.animStateSetString(ANIM_AGENT_WALK_ADJUST,"walk_adjust");

    // Where should this be set initially?
    LLJoint::setDebugJointNames(gSavedSettings.getString("DebugAvatarJoints"));

	LLControlAvatar::sRegionChangedSlot = gAgent.addRegionChangedCallback(&LLControlAvatar::onRegionChanged);

    sCloudTexture = LLViewerTextureManager::getFetchedTextureFromFile("cloud-particle.j2c");
}


void LLVOAvatar::cleanupClass()
{
}

// virtual
void LLVOAvatar::initInstance()
{
	//-------------------------------------------------------------------------
	// register motions
	//-------------------------------------------------------------------------
	if (LLCharacter::sInstances.size() == 1)
	{
		registerMotion( ANIM_AGENT_DO_NOT_DISTURB,					LLNullMotion::create );
		registerMotion( ANIM_AGENT_CROUCH,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_CROUCHWALK,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_EXPRESS_AFRAID,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_ANGER,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_BORED,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_CRY,				LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_DISDAIN,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_EMBARRASSED,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_FROWN,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_KISS,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_LAUGH,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_OPEN_MOUTH,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_REPULSED,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SAD,				LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SHRUG,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SMILE,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_SURPRISE,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_TONGUE_OUT,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_TOOTHSMILE,		LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_WINK,			LLEmote::create );
		registerMotion( ANIM_AGENT_EXPRESS_WORRY,			LLEmote::create );
		registerMotion( ANIM_AGENT_FEMALE_RUN_NEW,			LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_FEMALE_WALK,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_FEMALE_WALK_NEW,			LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_RUN,						LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_RUN_NEW,					LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_STAND,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_1,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_2,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_3,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_4,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STANDUP,					LLKeyframeFallMotion::create );
		registerMotion( ANIM_AGENT_TURNLEFT,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_TURNRIGHT,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_WALK,					LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_WALK_NEW,				LLKeyframeWalkMotion::create );
		
		// motions without a start/stop bit
		registerMotion( ANIM_AGENT_BODY_NOISE,				LLBodyNoiseMotion::create );
		registerMotion( ANIM_AGENT_BREATHE_ROT,				LLBreatheMotionRot::create );
		registerMotion( ANIM_AGENT_PHYSICS_MOTION,			LLPhysicsMotionController::create );
		registerMotion( ANIM_AGENT_EDITING,					LLEditingMotion::create	);
		registerMotion( ANIM_AGENT_EYE,						LLEyeMotion::create	);
		registerMotion( ANIM_AGENT_FEMALE_WALK,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_FLY_ADJUST,				LLFlyAdjustMotion::create );
		registerMotion( ANIM_AGENT_HAND_MOTION,				LLHandMotion::create );
		registerMotion( ANIM_AGENT_HEAD_ROT,				LLHeadRotMotion::create );
		registerMotion( ANIM_AGENT_PELVIS_FIX,				LLPelvisFixMotion::create );
		registerMotion( ANIM_AGENT_SIT_FEMALE,				LLKeyframeMotion::create );
		registerMotion( ANIM_AGENT_TARGET,					LLTargetingMotion::create );
		registerMotion( ANIM_AGENT_WALK_ADJUST,				LLWalkAdjustMotion::create );
	}
	
	LLAvatarAppearance::initInstance();
	
	// preload specific motions here
	createMotion( ANIM_AGENT_CUSTOMIZE);
	createMotion( ANIM_AGENT_CUSTOMIZE_DONE);
	
	//VTPause();  // VTune
	
	mVoiceVisualizer->setVoiceEnabled( LLVoiceClient::getInstance()->getVoiceEnabled( mID ) );

    mInitFlags |= 1<<1;
}

// virtual
LLAvatarJoint* LLVOAvatar::createAvatarJoint()
{
	return new LLViewerJoint();
}

// virtual
LLAvatarJoint* LLVOAvatar::createAvatarJoint(S32 joint_num)
{
	return new LLViewerJoint(joint_num);
}

// virtual
LLAvatarJointMesh* LLVOAvatar::createAvatarJointMesh()
{
	return new LLViewerJointMesh();
}

// virtual
LLTexLayerSet* LLVOAvatar::createTexLayerSet()
{
	return new LLViewerTexLayerSet(this);
}

const LLVector3 LLVOAvatar::getRenderPosition() const
{

	if (mDrawable.isNull() || mDrawable->getGeneration() < 0)
	{
		return getPositionAgent();
	}
	else if (isRoot())
	{
		F32 fixup;
		if ( hasPelvisFixup( fixup) )
		{
			//Apply a pelvis fixup (as defined by the avs skin)
			LLVector3 pos = mDrawable->getPositionAgent();
			pos[VZ] += fixup;
			return pos;
		}
		else
		{
			return mDrawable->getPositionAgent();
		}
	}
	else
	{
		return getPosition() * mDrawable->getParent()->getRenderMatrix();
	}
}

void LLVOAvatar::updateDrawable(bool force_damped)
{
	clearChanged(SHIFTED);
}

void LLVOAvatar::onShift(const LLVector4a& shift_vector)
{
	const LLVector3& shift = reinterpret_cast<const LLVector3&>(shift_vector);
	mLastAnimExtents[0] += shift;
	mLastAnimExtents[1] += shift;
}

void LLVOAvatar::updateSpatialExtents(LLVector4a& newMin, LLVector4a &newMax)
{
    if (mDrawable.isNull())
    {
        return;
    }

    if (mNeedsExtentUpdate)
    {
        calculateSpatialExtents(newMin,newMax);
        mLastAnimExtents[0].set(newMin.getF32ptr());
        mLastAnimExtents[1].set(newMax.getF32ptr());
		mLastAnimBasePos = mPelvisp->getWorldPosition();
        mNeedsExtentUpdate = false;
    }
	else
	{
		LLVector3 new_base_pos = mPelvisp->getWorldPosition();
		LLVector3 shift = new_base_pos-mLastAnimBasePos;
		mLastAnimExtents[0] += shift;
		mLastAnimExtents[1] += shift;
		mLastAnimBasePos = new_base_pos;

	}
          
	if (isImpostor() && !needsImpostorUpdate())
	{
		LLVector3 delta = getRenderPosition() -
			((LLVector3(mDrawable->getPositionGroup().getF32ptr())-mImpostorOffset));
		
		newMin.load3( (mLastAnimExtents[0] + delta).mV);
		newMax.load3( (mLastAnimExtents[1] + delta).mV);
	}
	else
	{
        newMin.load3(mLastAnimExtents[0].mV);
        newMax.load3(mLastAnimExtents[1].mV);
		LLVector4a pos_group;
		pos_group.setAdd(newMin,newMax);
		pos_group.mul(0.5f);
		mImpostorOffset = LLVector3(pos_group.getF32ptr())-getRenderPosition();
		mDrawable->setPositionGroup(pos_group);
	}
}


void LLVOAvatar::calculateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    static LLCachedControl<S32> box_detail_cache(gSavedSettings, "AvatarBoundingBoxComplexity");
    S32 box_detail = box_detail_cache;
    if (getOverallAppearance() != AOA_NORMAL)
    {
        if (isControlAvatar())
        {
            // Animated objects don't show system avatar but do need to include rigged meshes in their bounding box.
            box_detail = 3;
        }
        else
        {
            // Jellydolled avatars ignore attachments, etc, use only system avatar.
            box_detail = 1;
        }
    }

    // FIXME the update_min_max function used below assumes there is a
    // known starting point, but in general there isn't. Ideally the
    // box update logic should be modified to handle the no-point-yet
    // case. For most models, starting with the pelvis is safe though.
    LLVector3 zero_pos;
    LLVector4a pos;
    if (dist_vec(zero_pos, mPelvisp->getWorldPosition())<0.001)
    {
        // Don't use pelvis until av initialized
        pos.load3(getRenderPosition().mV);
    }
    else
    {
        pos.load3(mPelvisp->getWorldPosition().mV);
    }
    newMin = pos;
    newMax = pos;

    if (box_detail>=1 && !isControlAvatar())
    {
        //stretch bounding box by joint positions. Doing this for
        //control avs, where the polymeshes aren't maintained or
        //displayed, can give inaccurate boxes due to joints stuck at (0,0,0).
        for (polymesh_map_t::iterator i = mPolyMeshes.begin(); i != mPolyMeshes.end(); ++i)
        {
            LLPolyMesh* mesh = i->second;
            for (S32 joint_num = 0; joint_num < mesh->mJointRenderData.size(); joint_num++)
            {
                LLVector4a trans;
                trans.load3( mesh->mJointRenderData[joint_num]->mWorldMatrix->getTranslation().mV);
                update_min_max(newMin, newMax, trans);
            }
        }
    }

    // Pad bounding box for starting joint, plus polymesh if
    // applicable. Subsequent calcs should be accurate enough to not
    // need padding.
    LLVector4a padding(0.25);
    newMin.sub(padding);
    newMax.add(padding);


    //stretch bounding box by static attachments
    if (box_detail >= 2)
    {
        float max_attachment_span = get_default_max_prim_scale() * 5.0f;
    
        for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
             iter != mAttachmentPoints.end();
             ++iter)
        {
            LLViewerJointAttachment* attachment = iter->second;

            if (attachment->getValid())
            {
                for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                     attachment_iter != attachment->mAttachedObjects.end();
                     ++attachment_iter)
                {
                    // Don't we need to look at children of attached_object as well?
                    const LLViewerObject* attached_object = attachment_iter->get();
                    if (attached_object && !attached_object->isHUDAttachment())
                    {
                        const LLVOVolume *vol = dynamic_cast<const LLVOVolume*>(attached_object);
                        if (vol && vol->isAnimatedObject())
                        {
                            // Animated objects already have a bounding box in their control av, use that. 
                            // Could lag by a frame if there's no guarantee on order of processing for avatars.
                            LLControlAvatar *cav = vol->getControlAvatar();
                            if (cav)
                            {
                                LLVector4a cav_min;
                                cav_min.load3(cav->mLastAnimExtents[0].mV);
                                LLVector4a cav_max;
                                cav_max.load3(cav->mLastAnimExtents[1].mV);
                                update_min_max(newMin,newMax,cav_min);
                                update_min_max(newMin,newMax,cav_max);
                                continue;
                            }
                        }
                        if (vol && vol->isRiggedMeshFast())
                        {
                            continue;
                        }
                        LLDrawable* drawable = attached_object->mDrawable;
                        if (drawable && !drawable->isState(LLDrawable::RIGGED | LLDrawable::RIGGED_CHILD)) // <-- don't extend bounding box if any rigged objects are present
                        {
                            LLSpatialBridge* bridge = drawable->getSpatialBridge();
                            if (bridge)
                            {
                                const LLVector4a* ext = bridge->getSpatialExtents();
                                LLVector4a distance;
                                distance.setSub(ext[1], ext[0]);
                                LLVector4a max_span(max_attachment_span);

                                S32 lt = distance.lessThan(max_span).getGatheredBits() & 0x7;
                        
                                // Only add the prim to spatial extents calculations if it isn't a megaprim.
                                // max_attachment_span calculated at the start of the function 
                                // (currently 5 times our max prim size) 
                                if (lt == 0x7)
                                {
                                    update_min_max(newMin,newMax,ext[0]);
                                    update_min_max(newMin,newMax,ext[1]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Stretch bounding box by rigged mesh joint boxes
    if (box_detail>=3)
    {
        updateRiggingInfo();
        for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
        {
            LLJoint *joint = getJoint(joint_num);
            LLJointRiggingInfo *rig_info = NULL;
            if (joint_num < mJointRiggingInfoTab.size())
            {
                rig_info = &mJointRiggingInfoTab[joint_num];
            }

            if (joint && rig_info && rig_info->isRiggedTo())
            {
                LLViewerJointAttachment *as_joint_attach = dynamic_cast<LLViewerJointAttachment*>(joint);
                if (as_joint_attach && as_joint_attach->getIsHUDAttachment())
                {
                    // Ignore bounding box of HUD joints
                    continue;
                }
                LLMatrix4a mat;
                LLVector4a new_extents[2];
                mat.loadu(joint->getWorldMatrix());
                matMulBoundBox(mat, rig_info->getRiggedExtents(), new_extents);
                update_min_max(newMin, newMax, new_extents[0]);
                update_min_max(newMin, newMax, new_extents[1]);
                //if (isSelf())
                //{
                //    LL_INFOS() << joint->getName() << " extents " << new_extents[0] << "," << new_extents[1] << LL_ENDL;
                //    LL_INFOS() << joint->getName() << " av box is " << newMin << "," << newMax << LL_ENDL;
                //}
            }
        }
    }

    // Update pixel area
    LLVector4a center, size;
    center.setAdd(newMin, newMax);
    center.mul(0.5f);
    
    size.setSub(newMax,newMin);
    size.mul(0.5f);
    
    mPixelArea = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());
}

void render_sphere_and_line(const LLVector3& begin_pos, const LLVector3& end_pos, F32 sphere_scale, const LLVector3& occ_color, const LLVector3& visible_color)
{
    // Unoccluded bone portions
    LLGLDepthTest normal_depth(GL_TRUE);

    // Draw line segment for unoccluded joint
    gGL.diffuseColor3f(visible_color[0], visible_color[1], visible_color[2]);

    gGL.begin(LLRender::LINES);
    gGL.vertex3fv(begin_pos.mV); 
    gGL.vertex3fv(end_pos.mV);
    gGL.end();
        

    // Draw sphere representing joint pos
    gGL.pushMatrix();
    gGL.scalef(sphere_scale, sphere_scale, sphere_scale);
    gSphere.renderGGL();
    gGL.popMatrix();
        
    LLGLDepthTest depth_under(GL_TRUE, GL_FALSE, GL_GREATER);

    // Occluded bone portions
    gGL.diffuseColor3f(occ_color[0], occ_color[1], occ_color[2]);

    gGL.begin(LLRender::LINES);
    gGL.vertex3fv(begin_pos.mV); 
    gGL.vertex3fv(end_pos.mV);
    gGL.end();

    // Draw sphere representing joint pos
    gGL.pushMatrix();
    gGL.scalef(sphere_scale, sphere_scale, sphere_scale);
    gSphere.renderGGL();
    gGL.popMatrix();
}

//-----------------------------------------------------------------------------
// renderCollisionVolumes()
//-----------------------------------------------------------------------------
void LLVOAvatar::renderCollisionVolumes()
{
	std::ostringstream ostr;

	for (S32 i = 0; i < mNumCollisionVolumes; i++)
	{
		ostr << mCollisionVolumes[i].getName() << ", ";

        LLAvatarJointCollisionVolume& collision_volume = mCollisionVolumes[i];

		collision_volume.updateWorldMatrix();

		gGL.pushMatrix();
		gGL.multMatrix( &collision_volume.getXform()->getWorldMatrix().mMatrix[0][0] );

        LLVector3 begin_pos(0,0,0);
        LLVector3 end_pos(collision_volume.getEnd());
        static F32 sphere_scale = 1.0f;
        static F32 center_dot_scale = 0.05f;

        static LLVector3 BLUE(0.0f, 0.0f, 1.0f);
        static LLVector3 PASTEL_BLUE(0.5f, 0.5f, 1.0f);
        static LLVector3 RED(1.0f, 0.0f, 0.0f);
        static LLVector3 PASTEL_RED(1.0f, 0.5f, 0.5f);
        static LLVector3 WHITE(1.0f, 1.0f, 1.0f);
        

        LLVector3 cv_color_occluded;
        LLVector3 cv_color_visible;
        LLVector3 dot_color_occluded(WHITE);
        LLVector3 dot_color_visible(WHITE);
        if (isControlAvatar())
        {
            cv_color_occluded = RED;
            cv_color_visible = PASTEL_RED;
        }
        else
        {
            cv_color_occluded = BLUE;
            cv_color_visible = PASTEL_BLUE;
        }
        render_sphere_and_line(begin_pos, end_pos, sphere_scale, cv_color_occluded, cv_color_visible);
        render_sphere_and_line(begin_pos, end_pos, center_dot_scale, dot_color_occluded, dot_color_visible);

        gGL.popMatrix();
    }

    
	if (mNameText.notNull())
	{
		LLVector4a unused;
	
		mNameText->lineSegmentIntersect(unused, unused, unused, true);
	}
}

void LLVOAvatar::renderBones(const std::string &selected_joint)
{
    LLGLEnable blend(GL_BLEND);

	avatar_joint_list_t::iterator iter = mSkeleton.begin();
    avatar_joint_list_t::iterator end = mSkeleton.end();

    // For selected joints
    static LLVector3 SELECTED_COLOR_OCCLUDED(1.0f, 1.0f, 0.0f);
    static LLVector3 SELECTED_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    // For bones with position overrides defined
    static LLVector3 OVERRIDE_COLOR_OCCLUDED(1.0f, 0.0f, 0.0f);
    static LLVector3 OVERRIDE_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    // For bones which are rigged to by at least one attachment
    static LLVector3 RIGGED_COLOR_OCCLUDED(0.0f, 1.0f, 1.0f);
    static LLVector3 RIGGED_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    // For bones not otherwise colored
    static LLVector3 OTHER_COLOR_OCCLUDED(0.0f, 1.0f, 0.0f);
    static LLVector3 OTHER_COLOR_VISIBLE(0.5f, 0.5f, 0.5f);
    
    static F32 SPHERE_SCALEF = 0.001f;

	for (; iter != end; ++iter)
	{
		LLJoint* jointp = *iter;
		if (!jointp)
		{
			continue;
		}

		jointp->updateWorldMatrix();

        LLVector3 occ_color, visible_color;

        LLVector3 pos;
        LLUUID mesh_id;
        F32 sphere_scale = SPHERE_SCALEF;

        // We are in render, so it is preferable to implement selection
        // in a different way, but since this is for debug/preview, this
        // is low priority
        if (jointp->getName() == selected_joint)
        {
            sphere_scale *= 16;
            occ_color = SELECTED_COLOR_OCCLUDED;
            visible_color = SELECTED_COLOR_VISIBLE;
        }
        else if (jointp->hasAttachmentPosOverride(pos,mesh_id))
        {
            occ_color = OVERRIDE_COLOR_OCCLUDED;
            visible_color = OVERRIDE_COLOR_VISIBLE;
        }
        else
        {
            if (jointIsRiggedTo(jointp))
            {
                occ_color = RIGGED_COLOR_OCCLUDED;
                visible_color = RIGGED_COLOR_VISIBLE;
            }
            else
            {
                occ_color = OTHER_COLOR_OCCLUDED;
                visible_color = OTHER_COLOR_VISIBLE;
            }
        }
        LLVector3 begin_pos(0,0,0);
        LLVector3 end_pos(jointp->getEnd());

        
		gGL.pushMatrix();
		gGL.multMatrix( &jointp->getXform()->getWorldMatrix().mMatrix[0][0] );

        render_sphere_and_line(begin_pos, end_pos, sphere_scale, occ_color, visible_color);
        
		gGL.popMatrix();
	}
}


void LLVOAvatar::renderJoints()
{
	std::ostringstream ostr;
	std::ostringstream nullstr;

	for (joint_map_t::iterator iter = mJointMap.begin(); iter != mJointMap.end(); ++iter)
	{
		LLJoint* jointp = iter->second;
		if (!jointp)
		{
			nullstr << iter->first << " is NULL" << std::endl;
			continue;
		}

		ostr << jointp->getName() << ", ";

		jointp->updateWorldMatrix();
	
		gGL.pushMatrix();
		gGL.multMatrix( &jointp->getXform()->getWorldMatrix().mMatrix[0][0] );

		gGL.diffuseColor3f( 1.f, 0.f, 1.f );
	
		gGL.begin(LLRender::LINES);
	
		LLVector3 v[] = 
		{
			LLVector3(1,0,0),
			LLVector3(-1,0,0),
			LLVector3(0,1,0),
			LLVector3(0,-1,0),

			LLVector3(0,0,-1),
			LLVector3(0,0,1),
		};

		//sides
		gGL.vertex3fv(v[0].mV); 
		gGL.vertex3fv(v[2].mV);

		gGL.vertex3fv(v[0].mV); 
		gGL.vertex3fv(v[3].mV);

		gGL.vertex3fv(v[1].mV); 
		gGL.vertex3fv(v[2].mV);

		gGL.vertex3fv(v[1].mV); 
		gGL.vertex3fv(v[3].mV);


		//top
		gGL.vertex3fv(v[0].mV); 
		gGL.vertex3fv(v[4].mV);

		gGL.vertex3fv(v[1].mV); 
		gGL.vertex3fv(v[4].mV);

		gGL.vertex3fv(v[2].mV); 
		gGL.vertex3fv(v[4].mV);

		gGL.vertex3fv(v[3].mV); 
		gGL.vertex3fv(v[4].mV);


		//bottom
		gGL.vertex3fv(v[0].mV); 
		gGL.vertex3fv(v[5].mV);

		gGL.vertex3fv(v[1].mV); 
		gGL.vertex3fv(v[5].mV);

		gGL.vertex3fv(v[2].mV); 
		gGL.vertex3fv(v[5].mV);

		gGL.vertex3fv(v[3].mV); 
		gGL.vertex3fv(v[5].mV);

		gGL.end();

		gGL.popMatrix();
	}

	mDebugText.clear();
	addDebugText(ostr.str());
	addDebugText(nullstr.str());
}

bool LLVOAvatar::lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end,
									  S32 face,
									  bool pick_transparent,
									  bool pick_rigged,
                                      bool pick_unselectable,
									  S32* face_hit,
									  LLVector4a* intersection,
									  LLVector2* tex_coord,
									  LLVector4a* normal,
									  LLVector4a* tangent)
{
	if ((isSelf() && !gAgent.needsRenderAvatar()) || !LLPipeline::sPickAvatar)
	{
		return false;
	}

    if (isControlAvatar())
    {
        return false;
    }
    
	if (lineSegmentBoundingBox(start, end))
	{
		for (S32 i = 0; i < mNumCollisionVolumes; ++i)
		{
			mCollisionVolumes[i].updateWorldMatrix();
            
			glh::matrix4f mat((F32*) mCollisionVolumes[i].getXform()->getWorldMatrix().mMatrix);
			glh::matrix4f inverse = mat.inverse();
			glh::matrix4f norm_mat = inverse.transpose();

			glh::vec3f p1(start.getF32ptr());
			glh::vec3f p2(end.getF32ptr());

			inverse.mult_matrix_vec(p1);
			inverse.mult_matrix_vec(p2);

			LLVector3 position;
			LLVector3 norm;

			if (linesegment_sphere(LLVector3(p1.v), LLVector3(p2.v), LLVector3(0,0,0), 1.f, position, norm))
			{
				glh::vec3f res_pos(position.mV);
				mat.mult_matrix_vec(res_pos);
				
				norm.normalize();
				glh::vec3f res_norm(norm.mV);
				norm_mat.mult_matrix_dir(res_norm);

				if (intersection)
				{
					intersection->load3(res_pos.v);
				}

				if (normal)
				{
					normal->load3(res_norm.v);
				}

				return true;
			}
		}

		if (isSelf())
		{
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end();
			 ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;

				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject* attached_object = attachment_iter->get();
					
					if (attached_object && !attached_object->isDead() && attachment->getValid())
					{
						LLDrawable* drawable = attached_object->mDrawable;
						if (drawable->isState(LLDrawable::RIGGED))
						{ //regenerate octree for rigged attachment
							gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_RIGGED);
						}
					}
				}
			}
		}
	}

	
	
	LLVector4a position;
	if (mNameText.notNull() && mNameText->lineSegmentIntersect(start, end, position))
	{
		if (intersection)
		{
			*intersection = position;
		}

		return true;
	}

	return false;
}

// virtual
LLViewerObject* LLVOAvatar::lineSegmentIntersectRiggedAttachments(const LLVector4a& start, const LLVector4a& end,
									  S32 face,
									  bool pick_transparent,
									  bool pick_rigged,
                                      bool pick_unselectable,
									  S32* face_hit,
									  LLVector4a* intersection,
									  LLVector2* tex_coord,
									  LLVector4a* normal,
									  LLVector4a* tangent)
{
	if (isSelf() && !gAgent.needsRenderAvatar())
	{
		return NULL;
	}

	LLViewerObject* hit = NULL;

	if (lineSegmentBoundingBox(start, end))
	{
		LLVector4a local_end = end;
		LLVector4a local_intersection;

		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			iter != mAttachmentPoints.end();
			++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;

			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					attachment_iter != attachment->mAttachedObjects.end();
					++attachment_iter)
			{
				LLViewerObject* attached_object = attachment_iter->get();
					
				if (attached_object->lineSegmentIntersect(start, local_end, face, pick_transparent, pick_rigged, pick_unselectable, face_hit, &local_intersection, tex_coord, normal, tangent))
				{
					local_end = local_intersection;
					if (intersection)
					{
						*intersection = local_intersection;
					}
					
					hit = attached_object;
				}
			}
		}
	}
		
	return hit;
}


LLVOAvatar* LLVOAvatar::asAvatar()
{
	return this;
}

//-----------------------------------------------------------------------------
// LLVOAvatar::startDefaultMotions()
//-----------------------------------------------------------------------------
void LLVOAvatar::startDefaultMotions()
{
	//-------------------------------------------------------------------------
	// start default motions
	//-------------------------------------------------------------------------
	startMotion( ANIM_AGENT_HEAD_ROT );
	startMotion( ANIM_AGENT_EYE );
	startMotion( ANIM_AGENT_BODY_NOISE );
	startMotion( ANIM_AGENT_BREATHE_ROT );
	startMotion( ANIM_AGENT_PHYSICS_MOTION );
	startMotion( ANIM_AGENT_HAND_MOTION );
	startMotion( ANIM_AGENT_PELVIS_FIX );

	//-------------------------------------------------------------------------
	// restart any currently active motions
	//-------------------------------------------------------------------------
	processAnimationStateChanges();
}

//-----------------------------------------------------------------------------
// LLVOAvatar::buildCharacter()
// Deferred initialization and rebuild of the avatar.
//-----------------------------------------------------------------------------
// virtual
void LLVOAvatar::buildCharacter()
{
	LLAvatarAppearance::buildCharacter();

	// Not done building yet; more to do.
	mIsBuilt = false;

	//-------------------------------------------------------------------------
	// set head offset from pelvis
	//-------------------------------------------------------------------------
	updateHeadOffset();

	//-------------------------------------------------------------------------
	// initialize lip sync morph pointers
	//-------------------------------------------------------------------------
	mOohMorph     = getVisualParam( "Lipsync_Ooh" );
	mAahMorph     = getVisualParam( "Lipsync_Aah" );

	// If we don't have the Ooh morph, use the Kiss morph
	if (!mOohMorph)
	{
		LL_WARNS() << "Missing 'Ooh' morph for lipsync, using fallback." << LL_ENDL;
		mOohMorph = getVisualParam( "Express_Kiss" );
	}

	// If we don't have the Aah morph, use the Open Mouth morph
	if (!mAahMorph)
	{
		LL_WARNS() << "Missing 'Aah' morph for lipsync, using fallback." << LL_ENDL;
		mAahMorph = getVisualParam( "Express_Open_Mouth" );
	}

    // Currently disabled for control avatars (animated objects), enabled for all others.
    if (mEnableDefaultMotions)
    {
	startDefaultMotions();
    }

	//-------------------------------------------------------------------------
	// restart any currently active motions
	//-------------------------------------------------------------------------
	processAnimationStateChanges();

	mIsBuilt = true;
	stop_glerror();

	mMeshValid = true;
}

//-----------------------------------------------------------------------------
// resetVisualParams()
//-----------------------------------------------------------------------------
void LLVOAvatar::resetVisualParams()
{
	// Skeletal params
	{
		LLAvatarXmlInfo::skeletal_distortion_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mSkeletalDistortionInfoList.begin();
			 iter != sAvatarXmlInfo->mSkeletalDistortionInfoList.end(); 
			 ++iter)
		{
			LLPolySkeletalDistortionInfo *info = (LLPolySkeletalDistortionInfo*)*iter;
			LLPolySkeletalDistortion *param = dynamic_cast<LLPolySkeletalDistortion*>(getVisualParam(info->getID()));
            *param = LLPolySkeletalDistortion(this);
            llassert(param);
			if (!param->setInfo(info))
			{
				llassert(false);
			}			
		}
	}

	// Driver parameters
	for (LLAvatarXmlInfo::driver_info_list_t::iterator iter = sAvatarXmlInfo->mDriverInfoList.begin();
		 iter != sAvatarXmlInfo->mDriverInfoList.end(); 
		 ++iter)
	{
		LLDriverParamInfo *info = *iter;
        LLDriverParam *param = dynamic_cast<LLDriverParam*>(getVisualParam(info->getID()));
        LLDriverParam::entry_list_t driven_list = param->getDrivenList();
        *param = LLDriverParam(this);
        llassert(param);
        if (!param->setInfo(info))
        {
            llassert(false);
        }			
        param->setDrivenList(driven_list);
	}
}

void LLVOAvatar::applyDefaultParams()
{
	// These are params from avs with newly created copies of shape,
	// skin, hair, eyes, plus gender set as noted. Run arche_tool.py
	// to get params from some other xml appearance dump.
	std::map<S32, U8> male_params = {
		{1,33}, {2,61}, {4,85}, {5,23}, {6,58}, {7,127}, {8,63}, {10,85}, {11,63}, {12,42}, {13,0}, {14,85}, {15,63}, {16,36}, {17,85}, {18,95}, {19,153}, {20,63}, {21,34}, {22,0}, {23,63}, {24,109}, {25,88}, {27,132}, {31,63}, {33,136}, {34,81}, {35,85}, {36,103}, {37,136}, {38,127}, {80,255}, {93,203}, {98,0}, {99,0}, {105,127}, {108,0}, {110,0}, {111,127}, {112,0}, {113,0}, {114,127}, {115,0}, {116,0}, {117,0}, {119,127}, {130,114}, {131,127}, {132,99}, {133,63}, {134,127}, {135,140}, {136,127}, {137,127}, {140,0}, {141,0}, {142,0}, {143,191}, {150,0}, {155,104}, {157,0}, {162,0}, {163,0}, {165,0}, {166,0}, {167,0}, {168,0}, {169,0}, {177,0}, {181,145}, {182,216}, {183,133}, {184,0}, {185,127}, {192,0}, {193,127}, {196,170}, {198,0}, {503,0}, {505,127}, {506,127}, {507,109}, {508,85}, {513,127}, {514,127}, {515,63}, {517,85}, {518,42}, {603,100}, {604,216}, {605,214}, {606,204}, {607,204}, {608,204}, {609,51}, {616,25}, {617,89}, {619,76}, {624,204}, {625,0}, {629,127}, {637,0}, {638,0}, {646,144}, {647,85}, {649,127}, {650,132}, {652,127}, {653,85}, {654,0}, {656,127}, {659,127}, {662,127}, {663,127}, {664,127}, {665,127}, {674,59}, {675,127}, {676,85}, {678,127}, {682,127}, {683,106}, {684,47}, {685,79}, {690,127}, {692,127}, {693,204}, {700,63}, {701,0}, {702,0}, {703,0}, {704,0}, {705,127}, {706,127}, {707,0}, {708,0}, {709,0}, {710,0}, {711,127}, {712,0}, {713,159}, {714,0}, {715,0}, {750,178}, {752,127}, {753,36}, {754,85}, {755,131}, {756,127}, {757,127}, {758,127}, {759,153}, {760,95}, {762,0}, {763,140}, {764,74}, {765,27}, {769,127}, {773,127}, {775,0}, {779,214}, {780,204}, {781,198}, {785,0}, {789,0}, {795,63}, {796,30}, {799,127}, {800,226}, {801,255}, {802,198}, {803,255}, {804,255}, {805,255}, {806,255}, {807,255}, {808,255}, {812,255}, {813,255}, {814,255}, {815,204}, {816,0}, {817,255}, {818,255}, {819,255}, {820,255}, {821,255}, {822,255}, {823,255}, {824,255}, {825,255}, {826,255}, {827,255}, {828,0}, {829,255}, {830,255}, {834,255}, {835,255}, {836,255}, {840,0}, {841,127}, {842,127}, {844,255}, {848,25}, {858,100}, {859,255}, {860,255}, {861,255}, {862,255}, {863,84}, {868,0}, {869,0}, {877,0}, {879,51}, {880,132}, {921,255}, {922,255}, {923,255}, {10000,0}, {10001,0}, {10002,25}, {10003,0}, {10004,25}, {10005,23}, {10006,51}, {10007,0}, {10008,25}, {10009,23}, {10010,51}, {10011,0}, {10012,0}, {10013,25}, {10014,0}, {10015,25}, {10016,23}, {10017,51}, {10018,0}, {10019,0}, {10020,25}, {10021,0}, {10022,25}, {10023,23}, {10024,51}, {10025,0}, {10026,25}, {10027,23}, {10028,51}, {10029,0}, {10030,25}, {10031,23}, {10032,51}, {11000,1}, {11001,127}
	};
	std::map<S32, U8> female_params = {
		{1,33}, {2,61}, {4,85}, {5,23}, {6,58}, {7,127}, {8,63}, {10,85}, {11,63}, {12,42}, {13,0}, {14,85}, {15,63}, {16,36}, {17,85}, {18,95}, {19,153}, {20,63}, {21,34}, {22,0}, {23,63}, {24,109}, {25,88}, {27,132}, {31,63}, {33,136}, {34,81}, {35,85}, {36,103}, {37,136}, {38,127}, {80,0}, {93,203}, {98,0}, {99,0}, {105,127}, {108,0}, {110,0}, {111,127}, {112,0}, {113,0}, {114,127}, {115,0}, {116,0}, {117,0}, {119,127}, {130,114}, {131,127}, {132,99}, {133,63}, {134,127}, {135,140}, {136,127}, {137,127}, {140,0}, {141,0}, {142,0}, {143,191}, {150,0}, {155,104}, {157,0}, {162,0}, {163,0}, {165,0}, {166,0}, {167,0}, {168,0}, {169,0}, {177,0}, {181,145}, {182,216}, {183,133}, {184,0}, {185,127}, {192,0}, {193,127}, {196,170}, {198,0}, {503,0}, {505,127}, {506,127}, {507,109}, {508,85}, {513,127}, {514,127}, {515,63}, {517,85}, {518,42}, {603,100}, {604,216}, {605,214}, {606,204}, {607,204}, {608,204}, {609,51}, {616,25}, {617,89}, {619,76}, {624,204}, {625,0}, {629,127}, {637,0}, {638,0}, {646,144}, {647,85}, {649,127}, {650,132}, {652,127}, {653,85}, {654,0}, {656,127}, {659,127}, {662,127}, {663,127}, {664,127}, {665,127}, {674,59}, {675,127}, {676,85}, {678,127}, {682,127}, {683,106}, {684,47}, {685,79}, {690,127}, {692,127}, {693,204}, {700,63}, {701,0}, {702,0}, {703,0}, {704,0}, {705,127}, {706,127}, {707,0}, {708,0}, {709,0}, {710,0}, {711,127}, {712,0}, {713,159}, {714,0}, {715,0}, {750,178}, {752,127}, {753,36}, {754,85}, {755,131}, {756,127}, {757,127}, {758,127}, {759,153}, {760,95}, {762,0}, {763,140}, {764,74}, {765,27}, {769,127}, {773,127}, {775,0}, {779,214}, {780,204}, {781,198}, {785,0}, {789,0}, {795,63}, {796,30}, {799,127}, {800,226}, {801,255}, {802,198}, {803,255}, {804,255}, {805,255}, {806,255}, {807,255}, {808,255}, {812,255}, {813,255}, {814,255}, {815,204}, {816,0}, {817,255}, {818,255}, {819,255}, {820,255}, {821,255}, {822,255}, {823,255}, {824,255}, {825,255}, {826,255}, {827,255}, {828,0}, {829,255}, {830,255}, {834,255}, {835,255}, {836,255}, {840,0}, {841,127}, {842,127}, {844,255}, {848,25}, {858,100}, {859,255}, {860,255}, {861,255}, {862,255}, {863,84}, {868,0}, {869,0}, {877,0}, {879,51}, {880,132}, {921,255}, {922,255}, {923,255}, {10000,0}, {10001,0}, {10002,25}, {10003,0}, {10004,25}, {10005,23}, {10006,51}, {10007,0}, {10008,25}, {10009,23}, {10010,51}, {10011,0}, {10012,0}, {10013,25}, {10014,0}, {10015,25}, {10016,23}, {10017,51}, {10018,0}, {10019,0}, {10020,25}, {10021,0}, {10022,25}, {10023,23}, {10024,51}, {10025,0}, {10026,25}, {10027,23}, {10028,51}, {10029,0}, {10030,25}, {10031,23}, {10032,51}, {11000,1}, {11001,127}
	};
	std::map<S32, U8> *params = NULL;
	if (getSex() == SEX_MALE)
		params = &male_params;
	else
		params = &female_params;
			
	for( auto it = params->begin(); it != params->end(); ++it)
	{
		LLVisualParam* param = getVisualParam(it->first);
		if( !param )
		{
			// invalid id
			break;
		}

		U8 value = it->second;
		F32 newWeight = U8_to_F32(value, param->getMinWeight(), param->getMaxWeight());
		param->setWeight(newWeight);
	}
}

//-----------------------------------------------------------------------------
// resetSkeleton()
//-----------------------------------------------------------------------------
void LLVOAvatar::resetSkeleton(bool reset_animations)
{
    LL_DEBUGS("Avatar") << avString() << " reset starts" << LL_ENDL;
    if (!isControlAvatar() && !mLastProcessedAppearance)
    {
        LL_WARNS() << "Can't reset avatar; no appearance message has been received yet." << LL_ENDL;
        return;
    }

    // Save mPelvis state
    //LLVector3 pelvis_pos = getJoint("mPelvis")->getPosition();
    //LLQuaternion pelvis_rot = getJoint("mPelvis")->getRotation();

    // Clear all attachment pos and scale overrides
    clearAttachmentOverrides();

    // Note that we call buildSkeleton twice in this function. The first time is
    // just to get the right scale for the collision volumes, because
    // this will be used in setting the mJointScales for the
    // LLPolySkeletalDistortions of which the CVs are children.
	if( !buildSkeleton(sAvatarSkeletonInfo) )
    {
        LL_ERRS() << "Error resetting skeleton" << LL_ENDL;
	}

    // Reset some params to default state, without propagating changes downstream.
    resetVisualParams();

    // Now we have to reset the skeleton again, because its state
    // got clobbered by the resetVisualParams() calls
    // above.
	if( !buildSkeleton(sAvatarSkeletonInfo) )
    {
        LL_ERRS() << "Error resetting skeleton" << LL_ENDL;
	}

    // Reset attachment points
    // BuildSkeleton only does bones and CVs but we still need to reinit huds
    // since huds can be animated.
    bool ignore_hud_joints = !isSelf();
    initAttachmentPoints(ignore_hud_joints);

    // Fix up collision volumes
    for (LLVisualParam *param = getFirstVisualParam(); 
         param;
         param = getNextVisualParam())
    {
        LLPolyMorphTarget *poly_morph = dynamic_cast<LLPolyMorphTarget*>(param);
        if (poly_morph)
        {
            // This is a kludgy way to correct for the fact that the
            // collision volumes have been reset out from under the
            // poly morph sliders.
            F32 delta_weight = poly_morph->getLastWeight() - poly_morph->getDefaultWeight();
            poly_morph->applyVolumeChanges(delta_weight);
        }
    }

    // Reset tweakable params to preserved state
	if (getOverallAppearance() == AOA_NORMAL)
	{
		if (mLastProcessedAppearance)
		{
			bool slam_params = true;
			applyParsedAppearanceMessage(*mLastProcessedAppearance, slam_params);
		}
	}
	else
	{
		// Stripped down approximation of
		// applyParsedAppearanceMessage, but with alternative default
		// (jellydoll) params
		setCompositeUpdatesEnabled( false );
		gPipeline.markGLRebuild(this);
		applyDefaultParams();
		setCompositeUpdatesEnabled( true );
		updateMeshTextures();
		updateMeshVisibility();
	}
    updateVisualParams();

    // Restore attachment pos overrides
	updateAttachmentOverrides();

    // Animations
    if (reset_animations)
    {
        if (isSelf())
        {
            // This is equivalent to "Stop Animating Me". Will reset
            // all animations and propagate the changes to other
            // viewers.
            gAgent.stopCurrentAnimations();
        }
        else
        {
            // Local viewer-side reset for non-self avatars.
            resetAnimations();
        }
    }
    
    LL_DEBUGS("Avatar") << avString() << " reset ends" << LL_ENDL;
}

//-----------------------------------------------------------------------------
// releaseMeshData()
//-----------------------------------------------------------------------------
void LLVOAvatar::releaseMeshData()
{
	if (sInstances.size() < AVATAR_RELEASE_THRESHOLD || isUIAvatar())
	{
		return;
	}

	// cleanup mesh data
	for (avatar_joint_list_t::iterator iter = mMeshLOD.begin();
		 iter != mMeshLOD.end(); 
		 ++iter)
	{
		LLAvatarJoint* joint = (*iter);
		joint->setValid(false, true);
	}

	//cleanup data
	if (mDrawable.notNull())
	{
		LLFace* facep = mDrawable->getFace(0);
		if (facep)
		{
		facep->setSize(0, 0);
		for(S32 i = mNumInitFaces ; i < mDrawable->getNumFaces(); i++)
		{
			facep = mDrawable->getFace(i);
				if (facep)
				{
			facep->setSize(0, 0);
		}
	}
		}
	}
	
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (!attachment->getIsHUDAttachment())
		{
			attachment->setAttachmentVisibility(false);
		}
	}
	mMeshValid = false;
}

//-----------------------------------------------------------------------------
// restoreMeshData()
//-----------------------------------------------------------------------------
// virtual
void LLVOAvatar::restoreMeshData()
{
	llassert(!isSelf());
    if (mDrawable.isNull())
    {
        return;
    }
	
	//LL_INFOS() << "Restoring" << LL_ENDL;
	mMeshValid = true;
	updateJointLODs();

	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (!attachment->getIsHUDAttachment())
		{
			attachment->setAttachmentVisibility(true);
		}
	}

	// force mesh update as LOD might not have changed to trigger this
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY);
}

//-----------------------------------------------------------------------------
// updateMeshData()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateMeshData()
{
	if (mDrawable.notNull())
	{
		stop_glerror();

		S32 f_num = 0 ;
		const U32 VERTEX_NUMBER_THRESHOLD = 128 ;//small number of this means each part of an avatar has its own vertex buffer.
		const S32 num_parts = mMeshLOD.size();

		// this order is determined by number of LODS
		// if a mesh earlier in this list changed LODs while a later mesh doesn't,
		// the later mesh's index offset will be inaccurate
		for(S32 part_index = 0 ; part_index < num_parts ;)
		{
			S32 j = part_index ;
			U32 last_v_num = 0, num_vertices = 0 ;
			U32 last_i_num = 0, num_indices = 0 ;

			while(part_index < num_parts && num_vertices < VERTEX_NUMBER_THRESHOLD)
			{
				last_v_num = num_vertices ;
				last_i_num = num_indices ;

				LLViewerJoint* part_mesh = getViewerJoint(part_index++);
				if (part_mesh)
				{
					part_mesh->updateFaceSizes(num_vertices, num_indices, mAdjustedPixelArea);
				}
			}
			if(num_vertices < 1)//skip empty meshes
			{
				continue ;
			}
			if(last_v_num > 0)//put the last inserted part into next vertex buffer.
			{
				num_vertices = last_v_num ;
				num_indices = last_i_num ;	
				part_index-- ;
			}
		
			LLFace* facep = NULL;
			if(f_num < mDrawable->getNumFaces()) 
			{
				facep = mDrawable->getFace(f_num);
			}
			else
			{
				facep = mDrawable->getFace(0);
				if (facep)
				{
					facep = mDrawable->addFace(facep->getPool(), facep->getTexture()) ;
				}
			}
			if (!facep) continue;
			
			// resize immediately
			facep->setSize(num_vertices, num_indices);

			bool terse_update = false;

			facep->setGeomIndex(0);
			facep->setIndicesIndex(0);
		
			LLVertexBuffer* buff = facep->getVertexBuffer();
			if(!facep->getVertexBuffer())
			{
                buff = new LLVertexBuffer(LLDrawPoolAvatar::VERTEX_DATA_MASK);
				if (!buff->allocateBuffer(num_vertices, num_indices))
				{
					LL_WARNS() << "Failed to allocate Vertex Buffer for Mesh to "
						<< num_vertices << " vertices and "
						<< num_indices << " indices" << LL_ENDL;
					// Attempt to create a dummy triangle (one vertex, 3 indices, all 0)
					facep->setSize(1, 3);
					buff->allocateBuffer(1, 3);
					memset((U8*) buff->getMappedData(), 0, buff->getSize());
					memset((U8*) buff->getMappedIndices(), 0, buff->getIndicesSize());
				}
				facep->setVertexBuffer(buff);
			}
			else
			{
				if (buff->getNumIndices() == num_indices &&
					buff->getNumVerts() == num_vertices)
				{
					terse_update = true;
				}
				else
				{
                    buff = new LLVertexBuffer(buff->getTypeMask());
                    if (!buff->allocateBuffer(num_vertices, num_indices))
                    {
						LL_WARNS() << "Failed to allocate vertex buffer for Mesh, Substituting" << LL_ENDL;
						// Attempt to create a dummy triangle (one vertex, 3 indices, all 0)
						facep->setSize(1, 3);
						buff->allocateBuffer(1, 3);
						memset((U8*) buff->getMappedData(), 0, buff->getSize());
						memset((U8*) buff->getMappedIndices(), 0, buff->getIndicesSize());
					}
				}
			}
			
		
			// This is a hack! Avatars have their own pool, so we are detecting
			//   the case of more than one avatar in the pool (thus > 0 instead of >= 0)
			if (facep->getGeomIndex() > 0)
			{
				LL_ERRS() << "non-zero geom index: " << facep->getGeomIndex() << " in LLVOAvatar::restoreMeshData" << LL_ENDL;
			}

			if (num_vertices == buff->getNumVerts() && num_indices == buff->getNumIndices())
			{
				for(S32 k = j ; k < part_index ; k++)
				{
					bool rigid = false;
					if (k == MESH_ID_EYEBALL_LEFT ||
						k == MESH_ID_EYEBALL_RIGHT)
					{
						//eyeballs can't have terse updates since they're never rendered with
						//the hardware skinning shader
						rigid = true;
					}
				
					LLViewerJoint* mesh = getViewerJoint(k);
					if (mesh)
					{
						mesh->updateFaceData(facep, mAdjustedPixelArea, k == MESH_ID_HAIR, terse_update && !rigid);
					}
				}
			}

			stop_glerror();
			buff->unmapBuffer();

			if(!f_num)
			{
				f_num += mNumInitFaces ;
			}
			else
			{
				f_num++ ;
			}
		}
	}
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// LLVOAvatar::processUpdateMessage()
//------------------------------------------------------------------------
U32 LLVOAvatar::processUpdateMessage(LLMessageSystem *mesgsys,
									 void **user_data,
									 U32 block_num, const EObjectUpdateType update_type,
									 LLDataPacker *dp)
{
	const bool has_name = !getNVPair("FirstName");

	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	// Print out arrival information once we have name of avatar.
    if (has_name && getNVPair("FirstName"))
    {
        mDebugExistenceTimer.reset();
        debugAvatarRezTime("AvatarRezArrivedNotification","avatar arrived");
    }

	if(retval & LLViewerObject::INVALID_UPDATE)
	{
		if (isSelf())
		{
			//tell sim to cancel this update
			gAgent.teleportViaLocation(gAgent.getPositionGlobal());
		}
	}

	return retval;
}

LLViewerFetchedTexture *LLVOAvatar::getBakedTextureImage(const U8 te, const LLUUID& uuid)
{
	LLViewerFetchedTexture *result = NULL;

	if (uuid == IMG_DEFAULT_AVATAR ||
		uuid == IMG_DEFAULT ||
		uuid == IMG_INVISIBLE)
	{
		// Should already exist, don't need to find it on sim or baked-texture host.
		result = gTextureList.findImage(uuid, TEX_LIST_STANDARD);
	}
	if (!result)
	{
		const std::string url = getImageURL(te,uuid);

		if (url.empty())
		{
			LL_WARNS() << "unable to determine URL for te " << te << " uuid " << uuid << LL_ENDL;
			return NULL;
		}
		LL_DEBUGS("Avatar") << avString() << "get server-bake image from URL " << url << LL_ENDL;
		result = LLViewerTextureManager::getFetchedTextureFromUrl(
			url, FTT_SERVER_BAKE, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, uuid);
		if (result->isMissingAsset())
		{
			result->setIsMissingAsset(false);
		}
		
	}
	return result;
}

// virtual
S32 LLVOAvatar::setTETexture(const U8 te, const LLUUID& uuid)
{
	if (!isIndexBakedTexture((ETextureIndex)te))
	{
		// Sim still sends some uuids for non-baked slots sometimes - ignore.
		return LLViewerObject::setTETexture(te, LLUUID::null);
	}

	LLViewerFetchedTexture *image = getBakedTextureImage(te,uuid);
	llassert(image);
	return setTETextureCore(te, image);
}

//------------------------------------------------------------------------
// LLVOAvatar::dumpAnimationState()
//------------------------------------------------------------------------
void LLVOAvatar::dumpAnimationState()
{
	LL_INFOS() << "==============================================" << LL_ENDL;
	for (LLVOAvatar::AnimIterator it = mSignaledAnimations.begin(); it != mSignaledAnimations.end(); ++it)
	{
		LLUUID id = it->first;
		std::string playtag = "";
		if (mPlayingAnimations.find(id) != mPlayingAnimations.end())
		{
			playtag = "*";
		}
		LL_INFOS() << gAnimLibrary.animationName(id) << playtag << LL_ENDL;
	}
	for (LLVOAvatar::AnimIterator it = mPlayingAnimations.begin(); it != mPlayingAnimations.end(); ++it)
	{
		LLUUID id = it->first;
		bool is_signaled = mSignaledAnimations.find(id) != mSignaledAnimations.end();
		if (!is_signaled)
		{
			LL_INFOS() << gAnimLibrary.animationName(id) << "!S" << LL_ENDL;
		}
	}
}

//------------------------------------------------------------------------
// idleUpdate()
//------------------------------------------------------------------------
void LLVOAvatar::idleUpdate(LLAgent &agent, const F64 &time)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

	if (isDead())
	{
		LL_INFOS() << "Warning!  Idle on dead avatar" << LL_ENDL;
		return;
	}
    // record time and refresh "tooSlow" status
    updateTooSlow();

	static LLCachedControl<bool> disable_all_render_types(gSavedSettings, "DisableAllRenderTypes");
	if (!(gPipeline.hasRenderType(mIsControlAvatar ? LLPipeline::RENDER_TYPE_CONTROL_AV : LLPipeline::RENDER_TYPE_AVATAR))
		&& !disable_all_render_types && !isSelf())
	{
        if (!mIsControlAvatar)
        {
            idleUpdateNameTag( mLastRootPos );
        }
		return;
	}

    // Update should be happening max once per frame.
	if ((mLastAnimExtents[0]==LLVector3())||
		(mLastAnimExtents[1])==LLVector3())
	{
		mNeedsExtentUpdate = true;
	}
	else
	{
		const S32 upd_freq = 4; // force update every upd_freq frames.
		mNeedsExtentUpdate = ((LLDrawable::getCurrentFrame()+mID.mData[0])%upd_freq==0);
	}
    
    LLScopedContextString str("avatar_idle_update " + getFullname());
    
	checkTextureLoading() ;
	
	// force immediate pixel area update on avatars using last frames data (before drawable or camera updates)
	setPixelAreaAndAngle(gAgent);

	// force asynchronous drawable update
	if(mDrawable.notNull())
	{	
		if (isSitting() && getParent())
		{
			LLViewerObject *root_object = (LLViewerObject*)getRoot();
			LLDrawable* drawablep = root_object->mDrawable;
			// if this object hasn't already been updated by another avatar...
			if (drawablep) // && !drawablep->isState(LLDrawable::EARLY_MOVE))
			{
				if (root_object->isSelected())
				{
					gPipeline.updateMoveNormalAsync(drawablep);
				}
				else
				{
					gPipeline.updateMoveDampedAsync(drawablep);
				}
			}
		}
		else 
		{
			gPipeline.updateMoveDampedAsync(mDrawable);
		}
	}

	//--------------------------------------------------------------------
	// set alpha flag depending on state
	//--------------------------------------------------------------------

	if (isSelf())
	{
		LLViewerObject::idleUpdate(agent, time);
		
		// trigger fidget anims
		if (isAnyAnimationSignaled(AGENT_STAND_ANIMS, NUM_AGENT_STAND_ANIMS))
		{
			agent.fidget();
		}
	}
	else
	{
		// Should override the idleUpdate stuff and leave out the angular update part.
		LLQuaternion rotation = getRotation();
		LLViewerObject::idleUpdate(agent, time);
		setRotation(rotation);
	}

	// attach objects that were waiting for a drawable
	lazyAttach();
	
	// animate the character
	// store off last frame's root position to be consistent with camera position
	mLastRootPos = mRoot->getWorldPosition();
	bool detailed_update = updateCharacter(agent);

	static LLUICachedControl<bool> visualizers_in_calls("ShowVoiceVisualizersInCalls", false);
	bool voice_enabled = (visualizers_in_calls || LLVoiceClient::getInstance()->inProximalChannel()) &&
						 LLVoiceClient::getInstance()->getVoiceEnabled(mID);

	idleUpdateVoiceVisualizer( voice_enabled );
	idleUpdateMisc( detailed_update );
	idleUpdateAppearanceAnimation();
	if (detailed_update)
	{
		idleUpdateLipSync( voice_enabled );
		idleUpdateLoadingEffect();
		idleUpdateBelowWater();	// wind effect uses this
		idleUpdateWindEffect();
	}
		
	idleUpdateNameTag( mLastRootPos );

    // Complexity has stale mechanics, but updates still can be very rapid
    // so spread avatar complexity calculations over frames to lesen load from
    // rapid updates and to make sure all avatars are not calculated at once.
    S32 compl_upd_freq = 20;
    if (isControlAvatar())
    {
        // animeshes do not (or won't) have impostors nor change outfis,
        // no need for high frequency
        compl_upd_freq = 100;
    }
    else if (mLastRezzedStatus <= 0) //cloud or  init
    {
        compl_upd_freq = 60;
    }
    else if (isSelf())
    {
        compl_upd_freq = 5;
    }
    else if (mLastRezzedStatus == 1) //'grey', not fully loaded
    {
        compl_upd_freq = 40;
    }
    else if (isInMuteList()) //cheap, buffers value from search
    {
        compl_upd_freq = 100;
    }

    if ((LLFrameTimer::getFrameCount() + mID.mData[0]) % compl_upd_freq == 0)
    {
        // DEPRECATED 
        // replace with LLPipeline::profileAvatar?
        // Avatar profile takes ~ 0.5ms while idleUpdateRenderComplexity takes ~5ms
        // (both are unacceptably costly)
        idleUpdateRenderComplexity();
    }
    idleUpdateDebugInfo();
}

void LLVOAvatar::idleUpdateVoiceVisualizer(bool voice_enabled)
{
	bool render_visualizer = voice_enabled;
	
	// Don't render the user's own voice visualizer when in mouselook, or when opening the mic is disabled.
	if(isSelf())
	{
        static LLCachedControl<bool> voice_disable_mic(gSavedSettings, "VoiceDisableMic");
		if(gAgentCamera.cameraMouselook() || voice_disable_mic)
		{
			render_visualizer = false;
		}
	}
	
	mVoiceVisualizer->setVoiceEnabled(render_visualizer);
	
	if ( voice_enabled )
	{		
		//----------------------------------------------------------------
		// Only do gesture triggering for your own avatar, and only when you're in a proximal channel.
		//----------------------------------------------------------------
		if( isSelf() )
		{
			//----------------------------------------------------------------------------------------
			// The following takes the voice signal and uses that to trigger gesticulations. 
			//----------------------------------------------------------------------------------------
			int lastGesticulationLevel = mCurrentGesticulationLevel;
			mCurrentGesticulationLevel = mVoiceVisualizer->getCurrentGesticulationLevel();
			
			//---------------------------------------------------------------------------------------------------
			// If "current gesticulation level" changes, we catch this, and trigger the new gesture
			//---------------------------------------------------------------------------------------------------
			if ( lastGesticulationLevel != mCurrentGesticulationLevel )
			{
				if ( mCurrentGesticulationLevel != VOICE_GESTICULATION_LEVEL_OFF )
				{
					std::string gestureString = "unInitialized";
					if ( mCurrentGesticulationLevel == 0 )	{ gestureString = "/voicelevel1";	}
					else	if ( mCurrentGesticulationLevel == 1 )	{ gestureString = "/voicelevel2";	}
					else	if ( mCurrentGesticulationLevel == 2 )	{ gestureString = "/voicelevel3";	}
					else	{ LL_INFOS() << "oops - CurrentGesticulationLevel can be only 0, 1, or 2"  << LL_ENDL; }
					
					// this is the call that Karl S. created for triggering gestures from within the code.
					LLGestureMgr::instance().triggerAndReviseString( gestureString );
				}
			}
			
		} //if( isSelf() )
		
		//-----------------------------------------------------------------------------------------------------------------
		// If the avatar is speaking, then the voice amplitude signal is passed to the voice visualizer.
		// Also, here we trigger voice visualizer start and stop speaking, so it can animate the voice symbol.
		//
		// Notice the calls to "gAwayTimer.reset()". This resets the timer that determines how long the avatar has been
		// "away", so that the avatar doesn't lapse into away-mode (and slump over) while the user is still talking. 
		//-----------------------------------------------------------------------------------------------------------------
		if (LLVoiceClient::getInstance()->getIsSpeaking( mID ))
		{		
			if (!mVoiceVisualizer->getCurrentlySpeaking())
			{
				mVoiceVisualizer->setStartSpeaking();
				
				//printf( "gAwayTimer.reset();\n" );
			}
			
			mVoiceVisualizer->setSpeakingAmplitude( LLVoiceClient::getInstance()->getCurrentPower( mID ) );
			
			if( isSelf() )
			{
				gAgent.clearAFK();
			}
		}
		else
		{
			if ( mVoiceVisualizer->getCurrentlySpeaking() )
			{
				mVoiceVisualizer->setStopSpeaking();
				
				if ( mLipSyncActive )
				{
					if( mOohMorph ) mOohMorph->setWeight(mOohMorph->getMinWeight());
					if( mAahMorph ) mAahMorph->setWeight(mAahMorph->getMinWeight());
					
					mLipSyncActive = false;
					LLCharacter::updateVisualParams();
					dirtyMesh();
				}
			}
		}
		
		//--------------------------------------------------------------------------------------------
		// here we get the approximate head position and set as sound source for the voice symbol
		// (the following version uses a tweak of "mHeadOffset" which handle sitting vs. standing)
		//--------------------------------------------------------------------------------------------
		
		if ( isSitting() )
		{
			LLVector3 headOffset = LLVector3( 0.0f, 0.0f, mHeadOffset.mV[2] );
			mVoiceVisualizer->setVoiceSourceWorldPosition( mRoot->getWorldPosition() + headOffset );
		}
		else 
		{
			LLVector3 tagPos = mRoot->getWorldPosition();
			tagPos[VZ] -= mPelvisToFoot;
			tagPos[VZ] += ( mBodySize[VZ] + 0.125f ); // does not need mAvatarOffset -Nyx
			mVoiceVisualizer->setVoiceSourceWorldPosition( tagPos );
		}
	}//if ( voiceEnabled )
}		

static void override_bbox(LLDrawable* drawable, LLVector4a* extents)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SPATIAL;
    drawable->setSpatialExtents(extents[0], extents[1]);
    drawable->setPositionGroup(LLVector4a(0, 0, 0));
    drawable->movePartition();
}

void LLVOAvatar::idleUpdateMisc(bool detailed_update)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
	if (LLVOAvatar::sJointDebug)
	{
		LL_INFOS() << getFullname() << ": joint touches: " << LLJoint::sNumTouches << " updates: " << LLJoint::sNumUpdates << LL_ENDL;
	}

	LLJoint::sNumUpdates = 0;
	LLJoint::sNumTouches = 0;

	bool visible = isVisible() || mNeedsAnimUpdate;

	// update attachments positions
	if (detailed_update)
	{
        U32 draw_order = 0;
        S32 attachment_selected = LLSelectMgr::getInstance()->getSelection()->getObjectCount() && LLSelectMgr::getInstance()->getSelection()->isAttachment();
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end();
			 ++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;

			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
                LLViewerObject* attached_object = attachment_iter->get();
                if (!attached_object
                    || attached_object->isDead()
                    || !attachment->getValid()
                    || attached_object->mDrawable.isNull())
                {
                    continue;
                }

                LLSpatialBridge* bridge = attached_object->mDrawable->getSpatialBridge();
				
				if (visible || !(bridge && bridge->getRadius() < 2.0))
				{
                    //override rigged attachments' octree spatial extents with this avatar's bounding box
                    bool rigged = false;
                    if (bridge)
                    {
                        //transform avatar bounding box into attachment's coordinate frame
                        LLVector4a extents[2];
                        bridge->transformExtents(mDrawable->getSpatialExtents(), extents);

                        if (attached_object->mDrawable->isState(LLDrawable::RIGGED | LLDrawable::RIGGED_CHILD))
                        {
                            rigged = true;
                            override_bbox(attached_object->mDrawable, extents);
                        }
                    }

                    // if selecting any attachments, update all of them as non-damped
                    if (attachment_selected)
                    {
                        gPipeline.updateMoveNormalAsync(attached_object->mDrawable);
                    }
                    else
                    {
                        // Note: SL-17415; While most objects follow joints,
                        // some objects get position updates from server
                        gPipeline.updateMoveDampedAsync(attached_object->mDrawable);
                    }

                    // override_bbox calls movePartition() and getSpatialPartition(),
                    // so bridge might no longer be valid, get it again.
                    // ex: animesh stops being an animesh
                    bridge = attached_object->mDrawable->getSpatialBridge();
                    if (bridge)
                    {
                        if (!rigged)
                        {
                            gPipeline.updateMoveNormalAsync(bridge);
                        }
                        else
                        {
                            //specialized impl of updateMoveNormalAsync just for rigged attachment SpatialBridge
                            bridge->setState(LLDrawable::MOVE_UNDAMPED);
                            bridge->updateMove();
                            bridge->setState(LLDrawable::EARLY_MOVE);

                            LLSpatialGroup* group = attached_object->mDrawable->getSpatialGroup();
                            if (group)
                            { //set draw order of group
                                group->mAvatarp = this;
                                group->mRenderOrder = draw_order++;
                            }
                        }
                    }

					attached_object->updateText();	
				}
			}
		}
	}

	mNeedsAnimUpdate = false;

	if (isImpostor() && !mNeedsImpostorUpdate)
	{
		LL_ALIGN_16(LLVector4a ext[2]);
		F32 distance;
		LLVector3 angle;

		getImpostorValues(ext, angle, distance);

		for (U32 i = 0; i < 3 && !mNeedsImpostorUpdate; i++)
		{
			F32 cur_angle = angle.mV[i];
			F32 old_angle = mImpostorAngle.mV[i];
			F32 angle_diff = fabsf(cur_angle-old_angle);
		
			if (angle_diff > F_PI/512.f*distance*mUpdatePeriod)
			{
				mNeedsImpostorUpdate = true;
				mLastImpostorUpdateReason = 2;
			}
		}

		if (detailed_update && !mNeedsImpostorUpdate)
		{	//update impostor if view angle, distance, or bounding box change
			//significantly
			
			F32 dist_diff = fabsf(distance-mImpostorDistance);
			if (dist_diff/mImpostorDistance > 0.1f)
			{
				mNeedsImpostorUpdate = true;
				mLastImpostorUpdateReason = 3;
			}
			else
			{
				ext[0].load3(mLastAnimExtents[0].mV);
                ext[1].load3(mLastAnimExtents[1].mV);
                // Expensive. Just call this once per frame, in updateSpatialExtents();
                //calculateSpatialExtents(ext[0], ext[1]);
				LLVector4a diff;
				diff.setSub(ext[1], mImpostorExtents[1]);
				if (diff.getLength3().getF32() > 0.05f)
				{
					mNeedsImpostorUpdate = true;
					mLastImpostorUpdateReason = 4;
				}
				else
				{
					diff.setSub(ext[0], mImpostorExtents[0]);
					if (diff.getLength3().getF32() > 0.05f)
					{
						mNeedsImpostorUpdate = true;
						mLastImpostorUpdateReason = 5;
					}
				}
			}
		}
	}

    if (mDrawable.notNull())
    {
		mDrawable->movePartition();
	
		//force a move if sitting on an active object
		if (getParent() && ((LLViewerObject*) getParent())->mDrawable->isActive())
		{
			gPipeline.markMoved(mDrawable, true);
		}
    }
}

void LLVOAvatar::idleUpdateAppearanceAnimation()
{
	// update morphing params
	if (mAppearanceAnimating)
	{
		ESex avatar_sex = getSex();
		F32 appearance_anim_time = mAppearanceMorphTimer.getElapsedTimeF32();
		if (appearance_anim_time >= APPEARANCE_MORPH_TIME)
		{
			mAppearanceAnimating = false;
			for (LLVisualParam *param = getFirstVisualParam(); 
				 param;
				 param = getNextVisualParam())
			{
				if (param->isTweakable())
				{
					param->stopAnimating();
				}
			}
			updateVisualParams();
		}
		else
		{
			F32 morph_amt = calcMorphAmount();
			LLVisualParam *param;

			if (!isSelf())
			{
				// animate only top level params for non-self avatars
				for (param = getFirstVisualParam();
					 param;
					 param = getNextVisualParam())
				{
					if (param->isTweakable())
					{
						param->animate(morph_amt);
					}
				}
			}

			// apply all params
			for (param = getFirstVisualParam();
				 param;
				 param = getNextVisualParam())
			{
				param->apply(avatar_sex);
			}

			mLastAppearanceBlendTime = appearance_anim_time;
		}
		dirtyMesh();
	}
}

F32 LLVOAvatar::calcMorphAmount()
{
	F32 appearance_anim_time = mAppearanceMorphTimer.getElapsedTimeF32();
	F32 blend_frac = calc_bouncy_animation(appearance_anim_time / APPEARANCE_MORPH_TIME);
	F32 last_blend_frac = calc_bouncy_animation(mLastAppearanceBlendTime / APPEARANCE_MORPH_TIME);

	F32 morph_amt;
	if (last_blend_frac == 1.f)
	{
		morph_amt = 1.f;
	}
	else
	{
		morph_amt = (blend_frac - last_blend_frac) / (1.f - last_blend_frac);
	}

	return morph_amt;
}

void LLVOAvatar::idleUpdateLipSync(bool voice_enabled)
{
	// Use the Lipsync_Ooh and Lipsync_Aah morphs for lip sync
    if ( voice_enabled
        && mLastRezzedStatus > 0 // no point updating lip-sync for clouds
        && (LLVoiceClient::getInstance()->lipSyncEnabled())
        && LLVoiceClient::getInstance()->getIsSpeaking( mID ) )
	{
		F32 ooh_morph_amount = 0.0f;
		F32 aah_morph_amount = 0.0f;

		mVoiceVisualizer->lipSyncOohAah( ooh_morph_amount, aah_morph_amount );

		if( mOohMorph )
		{
			F32 ooh_weight = mOohMorph->getMinWeight()
				+ ooh_morph_amount * (mOohMorph->getMaxWeight() - mOohMorph->getMinWeight());

			mOohMorph->setWeight( ooh_weight);
		}

		if( mAahMorph )
		{
			F32 aah_weight = mAahMorph->getMinWeight()
				+ aah_morph_amount * (mAahMorph->getMaxWeight() - mAahMorph->getMinWeight());

			mAahMorph->setWeight( aah_weight);
		}

		mLipSyncActive = true;
		LLCharacter::updateVisualParams();
		dirtyMesh();
	}
}

void LLVOAvatar::idleUpdateLoadingEffect()
{
	// update visibility when avatar is partially loaded
	if (updateIsFullyLoaded()) // changed?
	{
		if (isFullyLoaded())
		{
			if (mFirstFullyVisible)
			{
				mFirstFullyVisible = false;
				if (isSelf())
				{
					LL_INFOS("Avatar") << avString() << "self isFullyLoaded, mFirstFullyVisible" << LL_ENDL;
					LLAppearanceMgr::instance().onFirstFullyVisible();
				}
				else
				{
					LL_INFOS("Avatar") << avString() << "other isFullyLoaded, mFirstFullyVisible" << LL_ENDL;
				}
			}

			deleteParticleSource();
			updateLOD();
		}
		else
		{
			LLPartSysData particle_parameters;

			// fancy particle cloud designed by Brent
			particle_parameters.mPartData.mMaxAge            = 4.f;
			particle_parameters.mPartData.mStartScale.mV[VX] = 0.8f;
			particle_parameters.mPartData.mStartScale.mV[VX] = 0.8f;
			particle_parameters.mPartData.mStartScale.mV[VY] = 1.0f;
			particle_parameters.mPartData.mEndScale.mV[VX]   = 0.02f;
			particle_parameters.mPartData.mEndScale.mV[VY]   = 0.02f;
			particle_parameters.mPartData.mStartColor        = LLColor4(1, 1, 1, 0.5f);
			particle_parameters.mPartData.mEndColor          = LLColor4(1, 1, 1, 0.0f);
			particle_parameters.mPartData.mStartScale.mV[VX] = 0.8f;
			particle_parameters.mPartImageID                 = sCloudTexture->getID();
			particle_parameters.mMaxAge                      = 0.f;
			particle_parameters.mPattern                     = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE;
			particle_parameters.mInnerAngle                  = F_PI;
			particle_parameters.mOuterAngle                  = 0.f;
			particle_parameters.mBurstRate                   = 0.02f;
			particle_parameters.mBurstRadius                 = 0.0f;
			particle_parameters.mBurstPartCount              = 1;
			particle_parameters.mBurstSpeedMin               = 0.1f;
			particle_parameters.mBurstSpeedMax               = 1.f;
			particle_parameters.mPartData.mFlags             = ( LLPartData::LL_PART_INTERP_COLOR_MASK | LLPartData::LL_PART_INTERP_SCALE_MASK |
																 LLPartData::LL_PART_EMISSIVE_MASK | // LLPartData::LL_PART_FOLLOW_SRC_MASK |
																 LLPartData::LL_PART_TARGET_POS_MASK );
			
			// do not generate particles for dummy or overly-complex avatars
			if (!mIsDummy && !isTooComplex() && !isTooSlow())
			{
				setParticleSource(particle_parameters, getID());
			}
		}
	}
}	

void LLVOAvatar::idleUpdateWindEffect()
{
	// update wind effect
	if ((LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) >= LLDrawPoolAvatar::SHADER_LEVEL_CLOTH))
	{
		F32 hover_strength = 0.f;
		F32 time_delta = mRippleTimer.getElapsedTimeF32() - mRippleTimeLast;
		mRippleTimeLast = mRippleTimer.getElapsedTimeF32();
		LLVector3 velocity = getVelocity();
		F32 speed = velocity.length();
		//RN: velocity varies too much frame to frame for this to work
		mRippleAccel.clearVec();//lerp(mRippleAccel, (velocity - mLastVel) * time_delta, LLSmoothInterpolation::getInterpolant(0.02f));
		mLastVel = velocity;
		LLVector4 wind;
		wind.setVec(getRegion()->mWind.getVelocityNoisy(getPositionAgent(), 4.f) - velocity);

		if (mInAir)
		{
			hover_strength = HOVER_EFFECT_STRENGTH * llmax(0.f, HOVER_EFFECT_MAX_SPEED - speed);
		}

		if (mBelowWater)
		{
			// TODO: make cloth flow more gracefully when underwater
			hover_strength += UNDERWATER_EFFECT_STRENGTH;
		}

		wind.mV[VZ] += hover_strength;
		wind.normalize();

		wind.mV[VW] = llmin(0.025f + (speed * 0.015f) + hover_strength, 0.5f);
		F32 interp;
		if (wind.mV[VW] > mWindVec.mV[VW])
		{
			interp = LLSmoothInterpolation::getInterpolant(0.2f);
		}
		else
		{
			interp = LLSmoothInterpolation::getInterpolant(0.4f);
		}
		mWindVec = lerp(mWindVec, wind, interp);
	
		F32 wind_freq = hover_strength + llclamp(8.f + (speed * 0.7f) + (noise1(mRipplePhase) * 4.f), 8.f, 25.f);
		mWindFreq = lerp(mWindFreq, wind_freq, interp); 

		if (mBelowWater)
		{
			mWindFreq *= UNDERWATER_FREQUENCY_DAMP;
		}

		mRipplePhase += (time_delta * mWindFreq);
		if (mRipplePhase > F_TWO_PI)
		{
			mRipplePhase = fmodf(mRipplePhase, F_TWO_PI);
		}
	}
}

void LLVOAvatar::idleUpdateNameTag(const LLVector3& root_pos_last)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

	// update chat bubble
	//--------------------------------------------------------------------
	// draw text label over character's head
	//--------------------------------------------------------------------
	if (mChatTimer.getElapsedTimeF32() > BUBBLE_CHAT_TIME)
	{
		mChats.clear();
	}
	
	const F32 time_visible = mTimeVisible.getElapsedTimeF32();

    static LLCachedControl<F32> NAME_SHOW_TIME(gSavedSettings, "RenderNameShowTime"); // seconds
    static LLCachedControl<F32> FADE_DURATION(gSavedSettings, "RenderNameFadeDuration"); // seconds
    static LLCachedControl<bool> use_chat_bubbles(gSavedSettings, "UseChatBubbles");

	bool visible_chat = use_chat_bubbles && (mChats.size() || mTyping);
	bool render_name =	visible_chat ||
		(((sRenderName == RENDER_NAME_ALWAYS) ||
		  (sRenderName == RENDER_NAME_FADE && time_visible < NAME_SHOW_TIME)));
	// If it's your own avatar, don't draw in mouselook, and don't
	// draw if we're specifically hiding our own name.
	if (isSelf())
	{
        static LLCachedControl<bool> render_name_show_self(gSavedSettings, "RenderNameShowSelf");
        static LLCachedControl<S32> name_tag_mode(gSavedSettings, "AvatarNameTagMode");
		render_name = render_name
			&& !gAgentCamera.cameraMouselook()
			&& (visible_chat || (render_name_show_self && name_tag_mode));
	}

	if ( !render_name )
	{
		if (mNameText)
		{
			// ...clean up old name tag
			mNameText->markDead();
			mNameText = NULL;
			sNumVisibleChatBubbles--;
		}
		return;
	}

	bool new_name = false;
	if (visible_chat != mVisibleChat)
	{
		mVisibleChat = visible_chat;
		new_name = true;
	}

	if (sRenderGroupTitles != mRenderGroupTitles)
	{
		mRenderGroupTitles = sRenderGroupTitles;
		new_name = true;
	}

	// First Calculate Alpha
	// If alpha > 0, create mNameText if necessary, otherwise delete it
	F32 alpha = 0.f;
	if (mAppAngle > 5.f)
	{
		const F32 START_FADE_TIME = NAME_SHOW_TIME - FADE_DURATION;
		if (!visible_chat && sRenderName == RENDER_NAME_FADE && time_visible > START_FADE_TIME)
		{
			alpha = 1.f - (time_visible - START_FADE_TIME) / FADE_DURATION;
		}
		else
		{
			// ...not fading, full alpha
			alpha = 1.f;
		}
	}
	else if (mAppAngle > 2.f)
	{
		// far away is faded out also
		alpha = (mAppAngle-2.f)/3.f;
	}

	if (alpha <= 0.f)
	{
		if (mNameText)
		{
			mNameText->markDead();
			mNameText = NULL;
			sNumVisibleChatBubbles--;
		}
		return;
	}

	if (!mNameText)
	{
		mNameText = static_cast<LLHUDNameTag*>( LLHUDObject::addHUDObject(
			LLHUDObject::LL_HUD_NAME_TAG) );
		//mNameText->setMass(10.f);
		mNameText->setSourceObject(this);
		mNameText->setVertAlignment(LLHUDNameTag::ALIGN_VERT_TOP);
		mNameText->setVisibleOffScreen(true);
		mNameText->setMaxLines(11);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
		sNumVisibleChatBubbles++;
		new_name = true;
    }
				
	idleUpdateNameTagPosition(root_pos_last);
	idleUpdateNameTagText(new_name);			
	idleUpdateNameTagAlpha(new_name, alpha);
}

void LLVOAvatar::idleUpdateNameTagText(bool new_name)
{
	LLNameValue *title = getNVPair("Title");
	LLNameValue* firstname = getNVPair("FirstName");
	LLNameValue* lastname = getNVPair("LastName");

	// Avatars must have a first and last name
	if (!firstname || !lastname) return;

	bool is_away = mSignaledAnimations.find(ANIM_AGENT_AWAY)  != mSignaledAnimations.end();
	bool is_do_not_disturb = mSignaledAnimations.find(ANIM_AGENT_DO_NOT_DISTURB) != mSignaledAnimations.end();
	bool is_appearance = mSignaledAnimations.find(ANIM_AGENT_CUSTOMIZE) != mSignaledAnimations.end();
	bool is_muted;
	if (isSelf())
	{
		is_muted = false;
	}
	else
	{
		is_muted = isInMuteList();
	}
	bool is_friend = LLAvatarTracker::instance().isBuddy(getID());
	bool is_cloud = getIsCloud();

	if (is_appearance != mNameAppearance)
	{
		if (is_appearance)
		{
			debugAvatarRezTime("AvatarRezEnteredAppearanceNotification","entered appearance mode");
		}
		else
		{
			debugAvatarRezTime("AvatarRezLeftAppearanceNotification","left appearance mode");
		}
	}

	// Rebuild name tag if state change detected
	if (!mNameIsSet
		|| new_name
		|| (!title && !mTitle.empty())
		|| (title && mTitle != title->getString())
		|| is_away != mNameAway 
		|| is_do_not_disturb != mNameDoNotDisturb 
		|| is_muted != mNameMute
		|| is_appearance != mNameAppearance 
		|| is_friend != mNameFriend
		|| is_cloud != mNameCloud)
	{
		LLColor4 name_tag_color = getNameTagColor(is_friend);

		clearNameTag();

		if (is_away || is_muted || is_do_not_disturb || is_appearance)
		{
			std::string line;
			if (is_away)
			{
				line += LLTrans::getString("AvatarAway");
				line += ", ";
			}
			if (is_do_not_disturb)
			{
				line += LLTrans::getString("AvatarDoNotDisturb");
				line += ", ";
			}
			if (is_muted)
			{
				line += LLTrans::getString("AvatarMuted");
				line += ", ";
			}
			if (is_appearance)
			{
				line += LLTrans::getString("AvatarEditingAppearance");
				line += ", ";
			}
			if (is_cloud)
			{
				line += LLTrans::getString("LoadingData");
				line += ", ";
			}
			// trim last ", "
			line.resize( line.length() - 2 );
			addNameTagLine(line, name_tag_color, LLFontGL::NORMAL,
				LLFontGL::getFontSansSerifSmall());
		}

		if (sRenderGroupTitles
			&& title && title->getString() && title->getString()[0] != '\0')
		{
			std::string title_str = title->getString();
			LLStringFn::replace_ascii_controlchars(title_str,LL_UNKNOWN_CHAR);
			addNameTagLine(title_str, name_tag_color, LLFontGL::NORMAL,
				LLFontGL::getFontSansSerifSmall(), true);
		}

		static LLUICachedControl<bool> show_display_names("NameTagShowDisplayNames", true);
		static LLUICachedControl<bool> show_usernames("NameTagShowUsernames", true);

		if (LLAvatarName::useDisplayNames())
		{
			LLAvatarName av_name;
			if (!LLAvatarNameCache::get(getID(), &av_name))
			{
				// Force a rebuild at next idle
				// Note: do not connect a callback on idle().
				clearNameTag();
			}

			// Might be blank if name not available yet, that's OK
			if (show_display_names)
			{
				addNameTagLine(av_name.getDisplayName(), name_tag_color, LLFontGL::NORMAL,
					LLFontGL::getFontSansSerif(), true);
			}
			// Suppress SLID display if display name matches exactly (ugh)
			if (show_usernames && !av_name.isDisplayNameDefault())
			{
				// *HACK: Desaturate the color
				LLColor4 username_color = name_tag_color * 0.83f;
				addNameTagLine(av_name.getUserName(), username_color, LLFontGL::NORMAL,
					LLFontGL::getFontSansSerifSmall(), true);
			}
		}
		else
		{
			const LLFontGL* font = LLFontGL::getFontSansSerif();
			std::string full_name = LLCacheName::buildFullName( firstname->getString(), lastname->getString() );
			addNameTagLine(full_name, name_tag_color, LLFontGL::NORMAL, font, true);
		}

		mNameAway = is_away;
		mNameDoNotDisturb = is_do_not_disturb;
		mNameMute = is_muted;
		mNameAppearance = is_appearance;
		mNameFriend = is_friend;
		mNameCloud = is_cloud;
		mTitle = title ? title->getString() : "";
		LLStringFn::replace_ascii_controlchars(mTitle,LL_UNKNOWN_CHAR);
		new_name = true;
	}

	if (mVisibleChat)
	{
		mNameText->setFont(LLFontGL::getFontSansSerif());
		mNameText->setTextAlignment(LLHUDNameTag::ALIGN_TEXT_LEFT);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS * 2.f, 5.f);

		std::deque<LLChat>::iterator chat_iter = mChats.begin();
		mNameText->clearString();

		LLColor4 new_chat = LLUIColorTable::instance().getColor( isSelf() ? "UserChatColor" : "AgentChatColor" );
		LLColor4 normal_chat = lerp(new_chat, LLColor4(0.8f, 0.8f, 0.8f, 1.f), 0.7f);
		LLColor4 old_chat = lerp(normal_chat, LLColor4(0.6f, 0.6f, 0.6f, 1.f), 0.7f);
		if (mTyping && mChats.size() >= MAX_BUBBLE_CHAT_UTTERANCES) 
		{
			++chat_iter;
		}

		for(; chat_iter != mChats.end(); ++chat_iter)
		{
			F32 chat_fade_amt = llclamp((F32)((LLFrameTimer::getElapsedSeconds() - chat_iter->mTime) / CHAT_FADE_TIME), 0.f, 4.f);
			LLFontGL::StyleFlags style;
			switch(chat_iter->mChatType)
			{
			case CHAT_TYPE_WHISPER:
				style = LLFontGL::ITALIC;
				break;
			case CHAT_TYPE_SHOUT:
				style = LLFontGL::BOLD;
				break;
			default:
				style = LLFontGL::NORMAL;
				break;
			}
			if (chat_fade_amt < 1.f)
			{
				F32 u = clamp_rescale(chat_fade_amt, 0.9f, 1.f, 0.f, 1.f);
				mNameText->addLine(chat_iter->mText, lerp(new_chat, normal_chat, u), style);
			}
			else if (chat_fade_amt < 2.f)
			{
				F32 u = clamp_rescale(chat_fade_amt, 1.9f, 2.f, 0.f, 1.f);
				mNameText->addLine(chat_iter->mText, lerp(normal_chat, old_chat, u), style);
			}
			else if (chat_fade_amt < 3.f)
			{
				// *NOTE: only remove lines down to minimum number
				mNameText->addLine(chat_iter->mText, old_chat, style);
			}
		}
		mNameText->setVisibleOffScreen(true);

		if (mTyping)
		{
			S32 dot_count = (llfloor(mTypingTimer.getElapsedTimeF32() * 3.f) + 2) % 3 + 1;
			switch(dot_count)
			{
			case 1:
				mNameText->addLine(".", new_chat);
				break;
			case 2:
				mNameText->addLine("..", new_chat);
				break;
			case 3:
				mNameText->addLine("...", new_chat);
				break;
			}

		}
	}
	else
	{
		// ...not using chat bubbles, just names
		mNameText->setTextAlignment(LLHUDNameTag::ALIGN_TEXT_CENTER);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
		mNameText->setVisibleOffScreen(false);
	}
}

void LLVOAvatar::addNameTagLine(const std::string& line, const LLColor4& color, S32 style, const LLFontGL* font, const bool use_ellipses)
{
    // extra width (NAMETAG_MAX_WIDTH) is for names only, not for chat
	llassert(mNameText);
	if (mVisibleChat)
	{
		mNameText->addLabel(line, LLHUDNameTag::NAMETAG_MAX_WIDTH);
	}
	else
	{
		mNameText->addLine(line, color, (LLFontGL::StyleFlags)style, font, use_ellipses, LLHUDNameTag::NAMETAG_MAX_WIDTH);
	}
    mNameIsSet |= !line.empty();
}

void LLVOAvatar::clearNameTag()
{
    mNameIsSet = false;
	if (mNameText)
	{
		mNameText->setLabel("");
		mNameText->setString("");
	}
	mTimeVisible.reset();
}

//static
void LLVOAvatar::invalidateNameTag(const LLUUID& agent_id)
{
	LLViewerObject* obj = gObjectList.findObject(agent_id);
	if (!obj) return;

	LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(obj);
	if (!avatar) return;

	avatar->clearNameTag();
}

//static
void LLVOAvatar::invalidateNameTags()
{
	std::vector<LLCharacter*>::iterator it = LLCharacter::sInstances.begin();
	for ( ; it != LLCharacter::sInstances.end(); ++it)
	{
		LLVOAvatar* avatar = dynamic_cast<LLVOAvatar*>(*it);
		if (!avatar) continue;
		if (avatar->isDead()) continue;

		avatar->clearNameTag();
	}
}

// Compute name tag position during idle update
void LLVOAvatar::idleUpdateNameTagPosition(const LLVector3& root_pos_last)
{
	LLQuaternion root_rot = mRoot->getWorldRotation();
	LLQuaternion inv_root_rot = ~root_rot;
	LLVector3 pixel_right_vec;
	LLVector3 pixel_up_vec;
	LLViewerCamera::getInstance()->getPixelVectors(root_pos_last, pixel_up_vec, pixel_right_vec);
	LLVector3 camera_to_av = root_pos_last - LLViewerCamera::getInstance()->getOrigin();
	camera_to_av.normalize();
	LLVector3 local_camera_at = camera_to_av * inv_root_rot;
	LLVector3 local_camera_up = camera_to_av % LLViewerCamera::getInstance()->getLeftAxis();
	local_camera_up.normalize();
	local_camera_up = local_camera_up * inv_root_rot;


	// position is based on head position, does not require mAvatarOffset here. - Nyx
	LLVector3 avatar_ellipsoid(mBodySize.mV[VX] * 0.4f,
								mBodySize.mV[VY] * 0.4f,
								mBodySize.mV[VZ] * NAMETAG_VERT_OFFSET_WEIGHT);

	local_camera_up.scaleVec(avatar_ellipsoid);
	local_camera_at.scaleVec(avatar_ellipsoid);

	LLVector3 head_offset = (mHeadp->getLastWorldPosition() - mRoot->getLastWorldPosition()) * inv_root_rot;

	if (dist_vec(head_offset, mTargetRootToHeadOffset) > NAMETAG_UPDATE_THRESHOLD)
	{
		mTargetRootToHeadOffset = head_offset;
	}
	
	mCurRootToHeadOffset = lerp(mCurRootToHeadOffset, mTargetRootToHeadOffset, LLSmoothInterpolation::getInterpolant(0.2f));

	LLVector3 name_position = mRoot->getLastWorldPosition() + (mCurRootToHeadOffset * root_rot);
	name_position += (local_camera_up * root_rot) - (projected_vec(local_camera_at * root_rot, camera_to_av));	
	name_position += pixel_up_vec * NAMETAG_VERTICAL_SCREEN_OFFSET;

	mNameText->setPositionAgent(name_position);				
}

void LLVOAvatar::idleUpdateNameTagAlpha(bool new_name, F32 alpha)
{
	llassert(mNameText);

	if (new_name
		|| alpha != mNameAlpha)
	{
		mNameText->setAlpha(alpha);
		mNameAlpha = alpha;
	}
}

LLColor4 LLVOAvatar::getNameTagColor(bool is_friend)
{
	static LLUICachedControl<bool> show_friends("NameTagShowFriends", false);
	const char* color_name;
	if (show_friends && is_friend)
	{
		color_name = "NameTagFriend";
	}
	else if (LLAvatarName::useDisplayNames())
	{
		// ...color based on whether username "matches" a computed display name
		LLAvatarName av_name;
		if (LLAvatarNameCache::get(getID(), &av_name) && av_name.isDisplayNameDefault())
		{
			color_name = "NameTagMatch";
		}
		else
		{
			color_name = "NameTagMismatch";
		}
	}
	else
	{
		// ...not using display names
		color_name = "NameTagLegacy";
	}
	return LLUIColorTable::getInstance()->getColor( color_name );
}

void LLVOAvatar::idleUpdateBelowWater()
{
	F32 avatar_height = (F32)(getPositionGlobal().mdV[VZ]);

	F32 water_height;
	water_height = getRegion()->getWaterHeight();

	mBelowWater =  avatar_height < water_height;
}

void LLVOAvatar::slamPosition()
{
	gAgent.setPositionAgent(getPositionAgent());
	// SL-315
	mRoot->setWorldPosition(getPositionAgent()); // teleport
	setChanged(TRANSLATED);
	if (mDrawable.notNull())
	{
		gPipeline.updateMoveNormalAsync(mDrawable);
	}
	mRoot->updateWorldMatrixChildren();
}

bool LLVOAvatar::isVisuallyMuted()
{
	bool muted = false;

	// Priority order (highest priority first)
	// * own avatar is never visually muted
	// * if on the "always draw normally" list, draw them normally
	// * if on the "always visually mute" list, mute them
	// * check against the render cost and attachment limits
	if (!isSelf())
	{
		if (mVisuallyMuteSetting == AV_ALWAYS_RENDER)
		{
			muted = false;
		}
		else if (mVisuallyMuteSetting == AV_DO_NOT_RENDER)
		{
#ifdef JELLYDOLLS_SHOULD_IMPOSTOR
			muted = true;
			// Always want to see this AV as an impostor
#else
			muted = false;
#endif
		}
        else if (isInMuteList())
        {
            muted = true;
        }
		else 
		{
			muted = isTooComplex() || isTooSlow();
		}
	}

	return muted;
}

bool LLVOAvatar::isInMuteList() const
{
	bool muted = false;
	F64 now = LLFrameTimer::getTotalSeconds();
	if (now < mCachedMuteListUpdateTime)
	{
		muted = mCachedInMuteList;
	}
	else
	{
		muted = LLMuteList::getInstance()->isMuted(getID());

		const F64 SECONDS_BETWEEN_MUTE_UPDATES = 1;
		mCachedMuteListUpdateTime = now + SECONDS_BETWEEN_MUTE_UPDATES;
		mCachedInMuteList = muted;
	}
	return muted;
}

void LLVOAvatar::updateAppearanceMessageDebugText()
{
		S32 central_bake_version = -1;
		if (getRegion())
		{
			central_bake_version = getRegion()->getCentralBakeVersion();
		}
		bool all_baked_downloaded = allBakedTexturesCompletelyDownloaded();
		bool all_local_downloaded = allLocalTexturesCompletelyDownloaded();
		std::string debug_line = llformat("%s%s - mLocal: %d, mEdit: %d, mUSB: %d, CBV: %d",
										  isSelf() ? (all_local_downloaded ? "L" : "l") : "-",
										  all_baked_downloaded ? "B" : "b",
										  mUseLocalAppearance, mIsEditingAppearance,
										  1, central_bake_version);
		std::string origin_string = bakedTextureOriginInfo();
		debug_line += " [" + origin_string + "]";
		S32 curr_cof_version = LLAppearanceMgr::instance().getCOFVersion();
		S32 last_request_cof_version = mLastUpdateRequestCOFVersion;
		S32 last_received_cof_version = mLastUpdateReceivedCOFVersion;
		if (isSelf())
		{
			debug_line += llformat(" - cof: %d req: %d rcv:%d",
								   curr_cof_version, last_request_cof_version, last_received_cof_version);
			static LLCachedControl<bool> debug_force_failure(gSavedSettings, "DebugForceAppearanceRequestFailure");
			if (debug_force_failure)
			{
				debug_line += " FORCING ERRS";
			}
		}
		else
		{
			debug_line += llformat(" - cof rcv:%d", last_received_cof_version);
		}
		debug_line += llformat(" bsz-z: %.3f", mBodySize[2]);
        if (mAvatarOffset[2] != 0.0f)
        {
            debug_line += llformat("avofs-z: %.3f", mAvatarOffset[2]);
        }
		bool hover_enabled = getRegion() && getRegion()->avatarHoverHeightEnabled();
		debug_line += hover_enabled ? " H" : " h";
		const LLVector3& hover_offset = getHoverOffset();
		if (hover_offset[2] != 0.0)
		{
			debug_line += llformat(" hov_z: %.3f", hover_offset[2]);
			debug_line += llformat(" %s", (isSitting() ? "S" : "T"));
			debug_line += llformat("%s", (isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED) ? "G" : "-"));
		}
		if (mInAir)
		{
			debug_line += " A";
			
		}

        LLVector3 ankle_right_pos_agent = mFootRightp->getWorldPosition();
		LLVector3 normal;
        LLVector3 ankle_right_ground_agent = ankle_right_pos_agent;
        resolveHeightAgent(ankle_right_pos_agent, ankle_right_ground_agent, normal);
        F32 rightElev = llmax(-0.2f, ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ]);
        debug_line += llformat(" relev %.3f", rightElev);

        LLVector3 root_pos = mRoot->getPosition();
        LLVector3 pelvis_pos = mPelvisp->getPosition();
        debug_line += llformat(" rp %.3f pp %.3f", root_pos[2], pelvis_pos[2]);

		const LLVector3& scale = getScale();
		debug_line += llformat(" scale-z %.3f", scale[2]);
		S32 is_visible = (S32) isVisible();
		S32 is_m_visible = (S32) mVisible;
		debug_line += llformat(" v %d/%d", is_visible, is_m_visible);

		AvatarOverallAppearance aoa = getOverallAppearance();
		if (aoa == AOA_NORMAL)
		{
			debug_line += " N";
		}
		else if (aoa == AOA_JELLYDOLL)
		{
			debug_line += " J";
		}
		else
		{
			debug_line += " I";
		}

		if (mMeshValid)
		{
			debug_line += "m";
		}
		else
		{
			debug_line += "-";
		}
		if (isImpostor())
		{
			debug_line += " Imp" + llformat("%d[%d]:%.1f", mUpdatePeriod, mLastImpostorUpdateReason, ((F32)(gFrameTimeSeconds-mLastImpostorUpdateFrameTime)));
		}

		addDebugText(debug_line);
}

LLViewerInventoryItem* getObjectInventoryItem(LLViewerObject *vobj, LLUUID asset_id)
{
    LLViewerInventoryItem *item = NULL;

    if (vobj)
    {
        if (vobj->getInventorySerial()<=0)
        {
            vobj->requestInventory(); 
	}
        item = vobj->getInventoryItemByAsset(asset_id);
    }
    return item;
}

LLViewerInventoryItem* recursiveGetObjectInventoryItem(LLViewerObject *vobj, LLUUID asset_id)
{
    LLViewerInventoryItem *item = getObjectInventoryItem(vobj, asset_id);
    if (!item)
    {
        LLViewerObject::const_child_list_t& children = vobj->getChildren();
        for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
             it != children.end(); ++it)
        {
            LLViewerObject *childp = *it;
            item = getObjectInventoryItem(childp, asset_id);
            if (item)
	{
                break;
            }
        }
	}
    return item;
}

void LLVOAvatar::updateAnimationDebugText()
{
	for (LLMotionController::motion_list_t::iterator iter = mMotionController.getActiveMotions().begin();
		 iter != mMotionController.getActiveMotions().end(); ++iter)
	{
		LLMotion* motionp = *iter;
		if (motionp->getMinPixelArea() < getPixelArea())
		{
			std::string output;
            std::string motion_name = motionp->getName();
            if (motion_name.empty())
            {
                if (isControlAvatar())
                {
                    LLControlAvatar *control_av = dynamic_cast<LLControlAvatar*>(this);
                    // Try to get name from inventory of associated object
                    LLVOVolume *volp = control_av->mRootVolp;
                    LLViewerInventoryItem *item = recursiveGetObjectInventoryItem(volp,motionp->getID());
                    if (item)
                    {
                        motion_name = item->getName();
                    }
                }
            }
            if (motion_name.empty())
			{
				std::string name;
				if (gAgent.isGodlikeWithoutAdminMenuFakery() || isSelf())
				{
					name = motionp->getID().asString();
					LLVOAvatar::AnimSourceIterator anim_it = mAnimationSources.begin();
					for (; anim_it != mAnimationSources.end(); ++anim_it)
					{
						if (anim_it->second == motionp->getID())
						{
							LLViewerObject* object = gObjectList.findObject(anim_it->first);
							if (!object)
							{
								break;
							}
							if (object->isAvatar())
							{
								if (mMotionController.mIsSelf)
								{
									// Searching inventory by asset id is really long
									// so just mark as inventory
									// Also item is likely to be named by LLPreviewAnim
									name += "(inventory)";
								}
							}
							else
							{
								LLViewerInventoryItem* item = NULL;
								if (!object->isInventoryDirty())
								{
									item = object->getInventoryItemByAsset(motionp->getID());
								}
								if (item)
								{
									name = item->getName();
								}
								else if (object->isAttachment())
								{
									name += "(att:" + getAttachmentItemName() + ")";
								}
								else
								{
									// in-world object, name or content unknown
									name += "(in-world)";
								}
							}
							break;
						}
					}
				}
				else
				{
					name = LLUUID::null.asString();
				}
				motion_name = name;
			}
			std::string motion_tag = "";
			if (mPlayingAnimations.find(motionp->getID()) != mPlayingAnimations.end())
			{
				motion_tag = "*";
			}
			output = llformat("%s%s - %d",
							  motion_name.c_str(),
							  motion_tag.c_str(),
							  (U32)motionp->getPriority());
			addDebugText(output);
		}
	}
}

void LLVOAvatar::updateDebugText()
{
    // Leave mDebugText uncleared here, in case a derived class has added some state first

	if (gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
	{
        updateAppearanceMessageDebugText();
	}

	if (gSavedSettings.getBOOL("DebugAvatarCompositeBaked"))
	{
		if (!mBakedTextureDebugText.empty())
			addDebugText(mBakedTextureDebugText);
	}

    // Develop -> Avatar -> Animation Info
	if (LLVOAvatar::sShowAnimationDebug)
	{
        updateAnimationDebugText();
	}

	if (!mDebugText.size() && mText.notNull())
	{
		mText->markDead();
		mText = NULL;
	}
	else if (mDebugText.size())
	{
		setDebugText(mDebugText);
	}
	mDebugText.clear();
}

//------------------------------------------------------------------------
// updateFootstepSounds
// Factored out from updateCharacter()
// Generate footstep sounds when feet hit the ground
//------------------------------------------------------------------------
void LLVOAvatar::updateFootstepSounds()
{
    if (mIsDummy)
    {
        return;
    }
	
	//-------------------------------------------------------------------------
	// Find the ground under each foot, these are used for a variety
	// of things that follow
	//-------------------------------------------------------------------------
	LLVector3 ankle_left_pos_agent = mFootLeftp->getWorldPosition();
	LLVector3 ankle_right_pos_agent = mFootRightp->getWorldPosition();

	LLVector3 ankle_left_ground_agent = ankle_left_pos_agent;
	LLVector3 ankle_right_ground_agent = ankle_right_pos_agent;
    LLVector3 normal;
	resolveHeightAgent(ankle_left_pos_agent, ankle_left_ground_agent, normal);
	resolveHeightAgent(ankle_right_pos_agent, ankle_right_ground_agent, normal);

	F32 leftElev = llmax(-0.2f, ankle_left_pos_agent.mV[VZ] - ankle_left_ground_agent.mV[VZ]);
	F32 rightElev = llmax(-0.2f, ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ]);

	if (!isSitting())
	{
		//-------------------------------------------------------------------------
		// Figure out which foot is on ground
		//-------------------------------------------------------------------------
		if (!mInAir)
		{
			if ((leftElev < 0.0f) || (rightElev < 0.0f))
	{
				ankle_left_pos_agent = mFootLeftp->getWorldPosition();
				ankle_right_pos_agent = mFootRightp->getWorldPosition();
				leftElev = ankle_left_pos_agent.mV[VZ] - ankle_left_ground_agent.mV[VZ];
				rightElev = ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ];
			}
		}
	}
	
	const LLUUID AGENT_FOOTSTEP_ANIMS[] = {ANIM_AGENT_WALK, ANIM_AGENT_RUN, ANIM_AGENT_LAND};
	const S32 NUM_AGENT_FOOTSTEP_ANIMS = LL_ARRAY_SIZE(AGENT_FOOTSTEP_ANIMS);

	if ( gAudiop && isAnyAnimationSignaled(AGENT_FOOTSTEP_ANIMS, NUM_AGENT_FOOTSTEP_ANIMS) )
	{
		bool playSound = false;
		LLVector3 foot_pos_agent;

		bool onGroundLeft = (leftElev <= 0.05f);
		bool onGroundRight = (rightElev <= 0.05f);

		// did left foot hit the ground?
		if ( onGroundLeft && !mWasOnGroundLeft )
		{
			foot_pos_agent = ankle_left_pos_agent;
			playSound = true;
		}

		// did right foot hit the ground?
		if ( onGroundRight && !mWasOnGroundRight )
	{
			foot_pos_agent = ankle_right_pos_agent;
			playSound = true;
	}

		mWasOnGroundLeft = onGroundLeft;
		mWasOnGroundRight = onGroundRight;

		if ( playSound )
		{
			const F32 STEP_VOLUME = 0.1f;
			const LLUUID& step_sound_id = getStepSound();

			LLVector3d foot_pos_global = gAgent.getPosGlobalFromAgent(foot_pos_agent);

			if (LLViewerParcelMgr::getInstance()->canHearSound(foot_pos_global)
				&& !LLMuteList::getInstance()->isMuted(getID(), LLMute::flagObjectSounds))
			{
				gAudiop->triggerSound(step_sound_id, getID(), STEP_VOLUME, LLAudioEngine::AUDIO_TYPE_AMBIENT, foot_pos_global);
			}
		}
	}
}

//------------------------------------------------------------------------
// computeUpdatePeriod()
// Factored out from updateCharacter()
// Set new value for mUpdatePeriod based on distance and various other factors.
//
// Note 10-2020: it turns out that none of these update period
// calculations have been having any effect, because
// mNeedsImpostorUpdate was not being set in updateCharacter(). So
// it's really open to question whether we want to enable time based updates, and if
// so, at what rate. Leaving the rates as given would lead to
// drastically more frequent impostor updates than we've been doing all these years.
// ------------------------------------------------------------------------
void LLVOAvatar::computeUpdatePeriod()
{
	bool visually_muted = isVisuallyMuted();
	if (mDrawable.notNull()
        && isVisible() 
        && (!isSelf() || visually_muted)
        && !isUIAvatar()
        && (sLimitNonImpostors || visually_muted)
        && !mNeedsAnimUpdate)
	{
		const LLVector4a* ext = mDrawable->getSpatialExtents();
		LLVector4a size;
		size.setSub(ext[1],ext[0]);
		F32 mag = size.getLength3().getF32()*0.5f;

		const S32 UPDATE_RATE_SLOW = 64;
		const S32 UPDATE_RATE_MED = 48;
		const S32 UPDATE_RATE_FAST = 32;
		
		if (visually_muted)
		{   // visually muted avatars update at lowest rate
			mUpdatePeriod = UPDATE_RATE_SLOW;
		}
		else if (! shouldImpostor()
				 || mDrawable->mDistanceWRTCamera < 1.f + mag)
		{   // first 25% of max visible avatars are not impostored
			// also, don't impostor avatars whose bounding box may be penetrating the 
			// impostor camera near clip plane
			mUpdatePeriod = 1;
		}
		else if ( shouldImpostor(4.0) )
		{ //background avatars are REALLY slow updating impostors
			mUpdatePeriod = UPDATE_RATE_SLOW;
		}
		else if (mLastRezzedStatus <= 0)
		{
			// Don't update cloud avatars too often
			mUpdatePeriod = UPDATE_RATE_SLOW;
		}
		else if ( shouldImpostor(3.0) )
		{ //back 25% of max visible avatars are slow updating impostors
			mUpdatePeriod = UPDATE_RATE_MED;
		}
		else 
		{
			//nearby avatars, update the impostors more frequently.
			mUpdatePeriod = UPDATE_RATE_FAST;
		}
	}
	else
	{
		mUpdatePeriod = 1;
	}
}

//------------------------------------------------------------------------
// updateOrientation()
// Factored out from updateCharacter()
// This is used by updateCharacter() to update the avatar's orientation:
// - updates mTurning state
// - updates rotation of the mRoot joint in the skeleton
// - for self, calls setControlFlags() to notify the simulator about any turns
//------------------------------------------------------------------------
void LLVOAvatar::updateOrientation(LLAgent& agent, F32 speed, F32 delta_time)
{
			LLQuaternion iQ;
			LLVector3 upDir( 0.0f, 0.0f, 1.0f );
			
			// Compute a forward direction vector derived from the primitive rotation
			// and the velocity vector.  When walking or jumping, don't let body deviate
			// more than 90 from the view, if necessary, flip the velocity vector.

			LLVector3 primDir;
			if (isSelf())
			{
				primDir = agent.getAtAxis() - projected_vec(agent.getAtAxis(), agent.getReferenceUpVector());
				primDir.normalize();
			}
			else
			{
				primDir = getRotation().getMatrix3().getFwdRow();
			}
			LLVector3 velDir = getVelocity();
			velDir.normalize();
			if ( mSignaledAnimations.find(ANIM_AGENT_WALK) != mSignaledAnimations.end())
			{
				F32 vpD = velDir * primDir;
				if (vpD < -0.5f)
				{
					velDir *= -1.0f;
				}
			}
			LLVector3 fwdDir = lerp(primDir, velDir, clamp_rescale(speed, 0.5f, 2.0f, 0.0f, 1.0f));
			if (isSelf() && gAgentCamera.cameraMouselook())
			{
				// make sure fwdDir stays in same general direction as primdir
				if (gAgent.getFlying())
				{
					fwdDir = LLViewerCamera::getInstance()->getAtAxis();
				}
				else
				{
					LLVector3 at_axis = LLViewerCamera::getInstance()->getAtAxis();
					LLVector3 up_vector = gAgent.getReferenceUpVector();
					at_axis -= up_vector * (at_axis * up_vector);
					at_axis.normalize();
					
					F32 dot = fwdDir * at_axis;
					if (dot < 0.f)
					{
						fwdDir -= 2.f * at_axis * dot;
						fwdDir.normalize();
					}
				}
			}

			LLQuaternion root_rotation = mRoot->getWorldMatrix().quaternion();
			F32 root_roll, root_pitch, root_yaw;
			root_rotation.getEulerAngles(&root_roll, &root_pitch, &root_yaw);

			// When moving very slow, the pelvis is allowed to deviate from the
    // forward direction to allow it to hold its position while the torso
			// and head turn.  Once in motion, it must conform however.
			bool self_in_mouselook = isSelf() && gAgentCamera.cameraMouselook();

			LLVector3 pelvisDir( mRoot->getWorldMatrix().getFwdRow4().mV );

			static LLCachedControl<F32> s_pelvis_rot_threshold_slow(gSavedSettings, "AvatarRotateThresholdSlow", 60.0);
			static LLCachedControl<F32> s_pelvis_rot_threshold_fast(gSavedSettings, "AvatarRotateThresholdFast", 2.0);

			F32 pelvis_rot_threshold = clamp_rescale(speed, 0.1f, 1.0f, s_pelvis_rot_threshold_slow, s_pelvis_rot_threshold_fast);
						
			if (self_in_mouselook)
			{
				pelvis_rot_threshold *= MOUSELOOK_PELVIS_FOLLOW_FACTOR;
			}
			pelvis_rot_threshold *= DEG_TO_RAD;

			F32 angle = angle_between( pelvisDir, fwdDir );

			// The avatar's root is allowed to have a yaw that deviates widely
			// from the forward direction, but if roll or pitch are off even
			// a little bit we need to correct the rotation.
			if(root_roll < 1.f * DEG_TO_RAD
			   && root_pitch < 5.f * DEG_TO_RAD)
			{
				// smaller correction vector means pelvis follows prim direction more closely
				if (!mTurning && angle > pelvis_rot_threshold*0.75f)
				{
					mTurning = true;
				}

				// use tighter threshold when turning
				if (mTurning)
				{
					pelvis_rot_threshold *= 0.4f;
				}

				// am I done turning?
				if (angle < pelvis_rot_threshold)
				{
					mTurning = false;
				}

				LLVector3 correction_vector = (pelvisDir - fwdDir) * clamp_rescale(angle, pelvis_rot_threshold*0.75f, pelvis_rot_threshold, 1.0f, 0.0f);
				fwdDir += correction_vector;
			}
			else
			{
				mTurning = false;
			}

			// Now compute the full world space rotation for the whole body (wQv)
			LLVector3 leftDir = upDir % fwdDir;
			leftDir.normalize();
			fwdDir = leftDir % upDir;
			LLQuaternion wQv( fwdDir, leftDir, upDir );

			if (isSelf() && mTurning)
			{
				if ((fwdDir % pelvisDir) * upDir > 0.f)
				{
					gAgent.setControlFlags(AGENT_CONTROL_TURN_RIGHT);
				}
				else
				{
					gAgent.setControlFlags(AGENT_CONTROL_TURN_LEFT);
				}
			}

			// Set the root rotation, but do so incrementally so that it
			// lags in time by some fixed amount.
			//F32 u = LLSmoothInterpolation::getInterpolant(PELVIS_LAG);
			F32 pelvis_lag_time = 0.f;
			if (self_in_mouselook)
			{
				pelvis_lag_time = PELVIS_LAG_MOUSELOOK;
			}
			else if (mInAir)
			{
				pelvis_lag_time = PELVIS_LAG_FLYING;
				// increase pelvis lag time when moving slowly
				pelvis_lag_time *= clamp_rescale(mSpeedAccum, 0.f, 15.f, 3.f, 1.f);
			}
			else
			{
				pelvis_lag_time = PELVIS_LAG_WALKING;
			}

    F32 u = llclamp((delta_time / pelvis_lag_time), 0.0f, 1.0f);	

			mRoot->setWorldRotation( slerp(u, mRoot->getWorldRotation(), wQv) );
}

//------------------------------------------------------------------------
// updateTimeStep()
// Factored out from updateCharacter().
//
// Updates the time step used by the motion controller, based on area
// and avatar count criteria.  This will also stop the
// ANIM_AGENT_WALK_ADJUST animation under some circumstances.
// ------------------------------------------------------------------------
void LLVOAvatar::updateTimeStep()
{
	if (!isSelf() && !isUIAvatar()) // ie, non-self avatars, and animated objects will be affected.
	{
        // Note that sInstances counts animated objects and
        // standard avatars in the same bucket. Is this desirable?
		F32 time_quantum = clamp_rescale((F32)sInstances.size(), 10.f, 35.f, 0.f, 0.25f);
		F32 pixel_area_scale = clamp_rescale(mPixelArea, 100, 5000, 1.f, 0.f);
		F32 time_step = time_quantum * pixel_area_scale;
        // Extrema:
        //   If number of avs is 10 or less, time_step is unmodified (flagged with 0.0).
        //   If area of av is 5000 or greater, time_step is unmodified (flagged with 0.0).
        //   If number of avs is 35 or greater, and area of av is 100 or less,
        //   time_step takes the maximum possible value of 0.25.
        //   Other situations will give values within the (0, 0.25) range.
		if (time_step != 0.f)
		{
			// disable walk motion servo controller as it doesn't work with motion timesteps
			stopMotion(ANIM_AGENT_WALK_ADJUST);
			removeAnimationData("Walk Speed");
		}
        // See SL-763 - playback with altered time step does not
        // appear to work correctly, odd behavior for distant avatars.
        // As of 11-2017, LLMotionController::updateMotions() will
        // ignore the value here. Need to re-enable if it's every
        // fixed.
		mMotionController.setTimeStep(time_step);
	}
}

void LLVOAvatar::updateRootPositionAndRotation(LLAgent& agent, F32 speed, bool was_sit_ground_constrained) 
{
	if (!(isSitting() && getParent()))
	{
		// This case includes all configurations except sitting on an
		// object, so does include ground sit.

		//--------------------------------------------------------------------
		// get timing info
		// handle initial condition case
		//--------------------------------------------------------------------
		F32 animation_time = mAnimTimer.getElapsedTimeF32();
		if (mTimeLast == 0.0f)
		{
			mTimeLast = animation_time;

			// Initially put the pelvis at slaved position/mRotation
			// SL-315
			mRoot->setWorldPosition( getPositionAgent() ); // first frame
			mRoot->setWorldRotation( getRotation() );
		}
			
		//--------------------------------------------------------------------
		// dont' let dT get larger than 1/5th of a second
		//--------------------------------------------------------------------
		F32 delta_time = animation_time - mTimeLast;

		delta_time = llclamp( delta_time, DELTA_TIME_MIN, DELTA_TIME_MAX );
		mTimeLast = animation_time;

		mSpeedAccum = (mSpeedAccum * 0.95f) + (speed * 0.05f);

		//--------------------------------------------------------------------
		// compute the position of the avatar's root
		//--------------------------------------------------------------------
		LLVector3d root_pos;
		LLVector3d ground_under_pelvis;

		if (isSelf())
		{
			gAgent.setPositionAgent(getRenderPosition());
		}

		root_pos = gAgent.getPosGlobalFromAgent(getRenderPosition());
		root_pos.mdV[VZ] += getVisualParamWeight(AVATAR_HOVER);

        LLVector3 normal;
		resolveHeightGlobal(root_pos, ground_under_pelvis, normal);
		F32 foot_to_ground = (F32) (root_pos.mdV[VZ] - mPelvisToFoot - ground_under_pelvis.mdV[VZ]);				
		bool in_air = ((!LLWorld::getInstance()->getRegionFromPosGlobal(ground_under_pelvis)) || 
						foot_to_ground > FOOT_GROUND_COLLISION_TOLERANCE);

		if (in_air && !mInAir)
		{
			mTimeInAir.reset();
		}
		mInAir = in_air;

        // SL-402: with the ability to animate the position of joints
        // that affect the body size calculation, computed body size
        // can get stale much more easily. Simplest fix is to update
        // it frequently.
        // SL-427: this appears to be too frequent, moving to only do on animation state change.
        //computeBodySize();
    
		// correct for the fact that the pelvis is not necessarily the center 
		// of the agent's physical representation
		root_pos.mdV[VZ] -= (0.5f * mBodySize.mV[VZ]) - mPelvisToFoot;
		if (!isSitting() && !was_sit_ground_constrained)
		{
			root_pos += LLVector3d(getHoverOffset());
			if (getOverallAppearance() == AOA_JELLYDOLL)
			{
				F32 offz = -0.5 * (getScale()[VZ] - mBodySize.mV[VZ]);
				root_pos[2] += offz;
				// if (!isSelf() && !isControlAvatar())
				// {
				// 	LL_DEBUGS("Avatar") << "av " << getFullname() 
				// 						<< " frame " << LLFrameTimer::getFrameCount()
				// 						<< " root adjust offz " << offz
				// 						<< " scalez " << getScale()[VZ]
				// 						<< " bsz " << mBodySize.mV[VZ]
				// 						<< LL_ENDL;
				// }
			}
		}
		// if (!isSelf() && !isControlAvatar())
		// {
		// 	LL_DEBUGS("Avatar") << "av " << getFullname() << " aoa " << (S32) getOverallAppearance()
		// 						<< " frame " << LLFrameTimer::getFrameCount()
		// 						<< " scalez " << getScale()[VZ]
		// 						<< " bsz " << mBodySize.mV[VZ]
		// 						<< " root pos " << root_pos[2]
		// 						<< " curr rootz " << mRoot->getPosition()[2] 
		// 						<< " pp-z " << mPelvisp->getPosition()[2]
		// 						<< " renderpos " << getRenderPosition()
		// 						<< LL_ENDL;
		// }

        LLControlAvatar *cav = dynamic_cast<LLControlAvatar*>(this);
        if (cav)
        {
            // SL-1350: Moved to LLDrawable::updateXform()
            cav->matchVolumeTransform();
        }
        else
        {
            LLVector3 newPosition = gAgent.getPosAgentFromGlobal(root_pos);
			// if (!isSelf() && !isControlAvatar())
			// {
			// 	LL_DEBUGS("Avatar") << "av " << getFullname() 
			// 						<< " frame " << LLFrameTimer::getFrameCount()
			// 						<< " newPosition " << newPosition
			// 						<< " renderpos " << getRenderPosition()
			// 						<< LL_ENDL;
			// }
            if (newPosition != mRoot->getXform()->getWorldPosition())
            {		
                mRoot->touch();
                // SL-315
                mRoot->setWorldPosition( newPosition ); // regular update				
            }
        }

		//--------------------------------------------------------------------
		// Propagate viewer object rotation to root of avatar
		//--------------------------------------------------------------------
		if (!isControlAvatar() && !isAnyAnimationSignaled(AGENT_NO_ROTATE_ANIMS, NUM_AGENT_NO_ROTATE_ANIMS))
		{
            // Rotation fixups for avatars in motion.
            // Skip for animated objects.
            updateOrientation(agent, speed, delta_time);
		}
	}
	else if (mDrawable.notNull())
	{
        // Sitting on an object - mRoot is slaved to mDrawable orientation.
		LLVector3 pos = mDrawable->getPosition();
		pos += getHoverOffset() * mDrawable->getRotation();
		// SL-315
		mRoot->setPosition(pos);
		mRoot->setRotation(mDrawable->getRotation());
	}
}

//------------------------------------------------------------------------
// LLVOAvatar::computeNeedsUpdate()
// 
// Most of the logic here is to figure out when to periodically update impostors.
// Non-impostors have mUpdatePeriod == 1 and will need update every frame.
//------------------------------------------------------------------------
bool LLVOAvatar::computeNeedsUpdate()
{
	const F32 MAX_IMPOSTOR_INTERVAL = 4.0f;
	computeUpdatePeriod();

	bool needs_update_by_frame_count = ((LLDrawable::getCurrentFrame()+mID.mData[0])%mUpdatePeriod == 0);

    bool needs_update_by_max_time = ((gFrameTimeSeconds-mLastImpostorUpdateFrameTime)> MAX_IMPOSTOR_INTERVAL);
	bool needs_update = needs_update_by_frame_count || needs_update_by_max_time;

	if (needs_update && !isSelf())
	{
		if (needs_update_by_max_time)
		{
			mNeedsImpostorUpdate = true;
			mLastImpostorUpdateReason = 11;
		}
		else
		{
			//mNeedsImpostorUpdate = true;
			//mLastImpostorUpdateReason = 10;
		}
	}
	return needs_update;
}

// updateCharacter()
//
// This is called for all avatars, so there are 4 possible situations:
//
// 1) Avatar is your own. In this case the class is LLVOAvatarSelf,
// isSelf() is true, and agent specifies the corresponding agent
// information for you. In all the other cases, agent is irrelevant
// and it would be less confusing if it were null or something.
//
// 2) Avatar is controlled by another resident. Class is LLVOAvatar,
// and isSelf() is false.
//
// 3) Avatar is the controller for an animated object. Class is
// LLControlAvatar and mIsDummy is true. Avatar is a purely
// viewer-side entity with no representation on the simulator.
//
// 4) Avatar is a UI avatar used in some areas of the UI, such as when
// previewing uploaded animations. Class is LLUIAvatar, and mIsDummy
// is true. Avatar is purely viewer-side with no representation on the
// simulator.
//
//------------------------------------------------------------------------
bool LLVOAvatar::updateCharacter(LLAgent &agent)
{	
	updateDebugText();
	
	if (!mIsBuilt)
	{
		return false;
	}

	bool visible = isVisible();
    bool is_control_avatar = isControlAvatar(); // capture state to simplify tracing
	bool is_attachment = false;

	if (is_control_avatar)
	{
        LLControlAvatar *cav = dynamic_cast<LLControlAvatar*>(this);
		is_attachment = cav && cav->mRootVolp && cav->mRootVolp->isAttachment(); // For attached animated objects
	}

    LLScopedContextString str("updateCharacter " + getFullname() + " is_control_avatar "
                              + boost::lexical_cast<std::string>(is_control_avatar) 
                              + " is_attachment " + boost::lexical_cast<std::string>(is_attachment));

	// For fading out the names above heads, only let the timer
	// run if we're visible.
	if (mDrawable.notNull() && !visible)
	{
		mTimeVisible.reset();
	}

	//--------------------------------------------------------------------
	// The rest should only be done occasionally for far away avatars.
    // Set mUpdatePeriod and visible based on distance and other criteria,
	// and flag for impostor update if needed.
	//--------------------------------------------------------------------
	bool needs_update = computeNeedsUpdate();
	
	//--------------------------------------------------------------------
	// Early out if does not need update and not self
	// don't early out for your own avatar, as we rely on your animations playing reliably
	// for example, the "turn around" animation when entering customize avatar needs to trigger
	// even when your avatar is offscreen
	//--------------------------------------------------------------------
	if (!needs_update && !isSelf())
	{
		updateMotions(LLCharacter::HIDDEN_UPDATE);
		return false;
	}

	//--------------------------------------------------------------------
	// Handle transitions between regular rendering, jellydoll, or invisible.
	// Can trigger skeleton reset or animation changes
	//--------------------------------------------------------------------
	updateOverallAppearance();
	
	//--------------------------------------------------------------------
	// change animation time quanta based on avatar render load
	//--------------------------------------------------------------------
    // SL-763 the time step quantization does not currently work.
    //updateTimeStep();
    
	//--------------------------------------------------------------------
    // Update sitting state based on parent and active animation info.
	//--------------------------------------------------------------------
	if (getParent() && !isSitting())
	{
		sitOnObject((LLViewerObject*)getParent());
	}
	else if (!getParent() && isSitting() && !isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED))
	{
        // If we are starting up, motion might be loading
        LLMotion *motionp = mMotionController.findMotion(ANIM_AGENT_SIT_GROUND_CONSTRAINED);
        if (!motionp || !mMotionController.isMotionLoading(motionp))
        {
            getOffObject();
        }
	}

	//--------------------------------------------------------------------
	// create local variables in world coords for region position values
	//--------------------------------------------------------------------
	LLVector3 xyVel = getVelocity();
	xyVel.mV[VZ] = 0.0f;
	F32 speed = xyVel.length();
	// remembering the value here prevents a display glitch if the
	// animation gets toggled during this update.
	bool was_sit_ground_constrained = isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED);

	//--------------------------------------------------------------------
    // This does a bunch of state updating, including figuring out
    // whether av is in the air, setting mRoot position and rotation
    // In some cases, calls updateOrientation() for a lot of the
    // work
    // --------------------------------------------------------------------
    updateRootPositionAndRotation(agent, speed, was_sit_ground_constrained);
	
	//-------------------------------------------------------------------------
	// Update character motions
	//-------------------------------------------------------------------------
	// store data relevant to motions
	mSpeed = speed;

	// update animations
	if (!visible)
	{
		updateMotions(LLCharacter::HIDDEN_UPDATE);
	}
	else if (mSpecialRenderMode == 1) // Animation Preview
	{
		updateMotions(LLCharacter::FORCE_UPDATE);
	}
	else
	{
		// Might be better to do HIDDEN_UPDATE if cloud
		updateMotions(LLCharacter::NORMAL_UPDATE);
	}

	// Special handling for sitting on ground.
	if (!getParent() && (isSitting() || was_sit_ground_constrained))
	{
		
		F32 off_z = LLVector3d(getHoverOffset()).mdV[VZ];
		if (off_z != 0.0)
		{
			LLVector3 pos = mRoot->getWorldPosition();
			pos.mV[VZ] += off_z;
			mRoot->touch();
			// SL-315
			mRoot->setWorldPosition(pos);
		}
	}

	// update head position
	updateHeadOffset();

	// Generate footstep sounds when feet hit the ground
    updateFootstepSounds();

	// Update child joints as needed.
	mRoot->updateWorldMatrixChildren();

    if (visible)
    {
		// System avatar mesh vertices need to be reskinned.
		mNeedsSkin = true;
    }

	return visible;
}

//-----------------------------------------------------------------------------
// updateHeadOffset()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateHeadOffset()
{
	// since we only care about Z, just grab one of the eyes
	LLVector3 midEyePt = mEyeLeftp->getWorldPosition();
	midEyePt -= mDrawable.notNull() ? mDrawable->getWorldPosition() : mRoot->getWorldPosition();
	midEyePt.mV[VZ] = llmax(-mPelvisToFoot + LLViewerCamera::getInstance()->getNear(), midEyePt.mV[VZ]);

	if (mDrawable.notNull())
	{
		midEyePt = midEyePt * ~mDrawable->getWorldRotation();
	}
	if (isSitting())
	{
		mHeadOffset = midEyePt;	
	}
	else
	{
		F32 u = llmax(0.f, HEAD_MOVEMENT_AVG_TIME - (1.f / gFPSClamped));
		mHeadOffset = lerp(midEyePt, mHeadOffset,  u);
	}
}

void LLVOAvatar::debugBodySize() const
{
	LLVector3 pelvis_scale = mPelvisp->getScale();

	// some of the joints have not been cached
	LLVector3 skull = mSkullp->getPosition();
    LL_DEBUGS("Avatar") << "skull pos " << skull << LL_ENDL;
	//LLVector3 skull_scale = mSkullp->getScale();

	LLVector3 neck = mNeckp->getPosition();
	LLVector3 neck_scale = mNeckp->getScale();
    LL_DEBUGS("Avatar") << "neck pos " << neck << " neck_scale " << neck_scale << LL_ENDL;

	LLVector3 chest = mChestp->getPosition();
	LLVector3 chest_scale = mChestp->getScale();
    LL_DEBUGS("Avatar") << "chest pos " << chest << " chest_scale " << chest_scale << LL_ENDL;

	// the rest of the joints have been cached
	LLVector3 head = mHeadp->getPosition();
	LLVector3 head_scale = mHeadp->getScale();
    LL_DEBUGS("Avatar") << "head pos " << head << " head_scale " << head_scale << LL_ENDL;

	LLVector3 torso = mTorsop->getPosition();
	LLVector3 torso_scale = mTorsop->getScale();
    LL_DEBUGS("Avatar") << "torso pos " << torso << " torso_scale " << torso_scale << LL_ENDL;

	LLVector3 hip = mHipLeftp->getPosition();
	LLVector3 hip_scale = mHipLeftp->getScale();
    LL_DEBUGS("Avatar") << "hip pos " << hip << " hip_scale " << hip_scale << LL_ENDL;

	LLVector3 knee = mKneeLeftp->getPosition();
	LLVector3 knee_scale = mKneeLeftp->getScale();
    LL_DEBUGS("Avatar") << "knee pos " << knee << " knee_scale " << knee_scale << LL_ENDL;

	LLVector3 ankle = mAnkleLeftp->getPosition();
	LLVector3 ankle_scale = mAnkleLeftp->getScale();
    LL_DEBUGS("Avatar") << "ankle pos " << ankle << " ankle_scale " << ankle_scale << LL_ENDL;

	LLVector3 foot  = mFootLeftp->getPosition();
    LL_DEBUGS("Avatar") << "foot pos " << foot << LL_ENDL;

	F32 new_offset = (const_cast<LLVOAvatar*>(this))->getVisualParamWeight(AVATAR_HOVER);
    LL_DEBUGS("Avatar") << "new_offset " << new_offset << LL_ENDL;

	F32 new_pelvis_to_foot = hip.mV[VZ] * pelvis_scale.mV[VZ] -
        knee.mV[VZ] * hip_scale.mV[VZ] -
        ankle.mV[VZ] * knee_scale.mV[VZ] -
        foot.mV[VZ] * ankle_scale.mV[VZ];
    LL_DEBUGS("Avatar") << "new_pelvis_to_foot " << new_pelvis_to_foot << LL_ENDL;

	LLVector3 new_body_size;
	new_body_size.mV[VZ] = new_pelvis_to_foot +
					   // the sqrt(2) correction below is an approximate
					   // correction to get to the top of the head
					   F_SQRT2 * (skull.mV[VZ] * head_scale.mV[VZ]) + 
					   head.mV[VZ] * neck_scale.mV[VZ] + 
					   neck.mV[VZ] * chest_scale.mV[VZ] + 
					   chest.mV[VZ] * torso_scale.mV[VZ] + 
					   torso.mV[VZ] * pelvis_scale.mV[VZ]; 

	// TODO -- measure the real depth and width
	new_body_size.mV[VX] = DEFAULT_AGENT_DEPTH;
	new_body_size.mV[VY] = DEFAULT_AGENT_WIDTH;

    LL_DEBUGS("Avatar") << "new_body_size " << new_body_size << LL_ENDL;
}
   
//------------------------------------------------------------------------
// postPelvisSetRecalc
//------------------------------------------------------------------------
void LLVOAvatar::postPelvisSetRecalc()
{		
	mRoot->updateWorldMatrixChildren();			
	computeBodySize();
	dirtyMesh(2);
}
//------------------------------------------------------------------------
// updateVisibility()
//------------------------------------------------------------------------
void LLVOAvatar::updateVisibility()
{
	bool visible = false;

	if (mIsDummy)
	{
		visible = false;
	}
	else if (mDrawable.isNull())
	{
		visible = false;
	}
	else
	{
		if (!mDrawable->getSpatialGroup() || mDrawable->getSpatialGroup()->isVisible())
		{
			visible = true;
		}
		else
		{
			visible = false;
		}

		if(isSelf())
		{
			if (!gAgentWearables.areWearablesLoaded())
			{
				visible = false;
			}
		}
		else if( !mFirstAppearanceMessageReceived )
		{
			visible = false;
		}

		if (sDebugInvisible)
		{
			LLNameValue* firstname = getNVPair("FirstName");
			if (firstname)
			{
				LL_DEBUGS("Avatar") << avString() << " updating visibility" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "Avatar " << this << " updating visiblity" << LL_ENDL;
			}

			if (visible)
			{
				LL_INFOS() << "Visible" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "Not visible" << LL_ENDL;
			}

			/*if (avatar_in_frustum)
			{
				LL_INFOS() << "Avatar in frustum" << LL_ENDL;
			}
			else
			{
				LL_INFOS() << "Avatar not in frustum" << LL_ENDL;
			}*/

			/*if (LLViewerCamera::getInstance()->sphereInFrustum(sel_pos_agent, 2.0f))
			{
				LL_INFOS() << "Sel pos visible" << LL_ENDL;
			}
			if (LLViewerCamera::getInstance()->sphereInFrustum(wrist_right_pos_agent, 0.2f))
			{
				LL_INFOS() << "Wrist pos visible" << LL_ENDL;
			}
			if (LLViewerCamera::getInstance()->sphereInFrustum(getPositionAgent(), getMaxScale()*2.f))
			{
				LL_INFOS() << "Agent visible" << LL_ENDL;
			}*/
			LL_INFOS() << "PA: " << getPositionAgent() << LL_ENDL;
			/*LL_INFOS() << "SPA: " << sel_pos_agent << LL_ENDL;
			LL_INFOS() << "WPA: " << wrist_right_pos_agent << LL_ENDL;*/
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
				 iter != mAttachmentPoints.end();
				 ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;

				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					if (LLViewerObject *attached_object = attachment_iter->get())
					{
						if(attached_object->mDrawable->isVisible())
						{
							LL_INFOS() << attachment->getName() << " visible" << LL_ENDL;
						}
						else
						{
							LL_INFOS() << attachment->getName() << " not visible at " << mDrawable->getWorldPosition() << " and radius " << mDrawable->getRadius() << LL_ENDL;
						}
					}
				}
			}
		}
	}

	if (!visible && mVisible)
	{
		mMeshInvisibleTime.reset();
	}

	if (visible)
	{
		if (!mMeshValid)
		{
			restoreMeshData();
		}
	}
	else
	{
		if (mMeshValid &&
            (isControlAvatar() || mMeshInvisibleTime.getElapsedTimeF32() > TIME_BEFORE_MESH_CLEANUP))
		{
			releaseMeshData();
		}
	}

    if ( visible != mVisible )
    {
        LL_DEBUGS("AvatarRender") << "visible was " << mVisible << " now " << visible << LL_ENDL;
    }
	mVisible = visible;
}

// private
bool LLVOAvatar::shouldAlphaMask()
{
	const bool should_alpha_mask = !LLDrawPoolAlpha::sShowDebugAlpha // Don't alpha mask if "Highlight Transparent" checked
							&& !LLDrawPoolAvatar::sSkipTransparent;

	return should_alpha_mask;

}

//-----------------------------------------------------------------------------
// renderSkinned()
//-----------------------------------------------------------------------------
U32 LLVOAvatar::renderSkinned()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

	U32 num_indices = 0;

	if (!mIsBuilt)
	{
		return num_indices;
	}

    if (mDrawable.isNull())
    {
		return num_indices;
    }

	LLFace* face = mDrawable->getFace(0);

	bool needs_rebuild = !face || !face->getVertexBuffer() || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY);

	if (needs_rebuild || mDirtyMesh)
	{	//LOD changed or new mesh created, allocate new vertex buffer if needed
		if (needs_rebuild || mDirtyMesh >= 2 || mVisibilityRank <= 4)
		{
			updateMeshData();
			mDirtyMesh = 0;
			mNeedsSkin = true;
			mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
		}
	}

	if (LLViewerShaderMgr::instance()->getShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) <= 0)
	{
		if (mNeedsSkin)
		{
			//generate animated mesh
			LLViewerJoint* lower_mesh = getViewerJoint(MESH_ID_LOWER_BODY);
			LLViewerJoint* upper_mesh = getViewerJoint(MESH_ID_UPPER_BODY);
			LLViewerJoint* skirt_mesh = getViewerJoint(MESH_ID_SKIRT);
			LLViewerJoint* eyelash_mesh = getViewerJoint(MESH_ID_EYELASH);
			LLViewerJoint* head_mesh = getViewerJoint(MESH_ID_HEAD);
			LLViewerJoint* hair_mesh = getViewerJoint(MESH_ID_HAIR);

			if(upper_mesh)
			{
				upper_mesh->updateJointGeometry();
			}
			if (lower_mesh)
			{
				lower_mesh->updateJointGeometry();
			}

			if( isWearingWearableType( LLWearableType::WT_SKIRT ) )
			{
				if(skirt_mesh)
				{
					skirt_mesh->updateJointGeometry();
				}
			}

			if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
				if(eyelash_mesh)
				{
					eyelash_mesh->updateJointGeometry();
				}
				if(head_mesh)
				{
					head_mesh->updateJointGeometry();
				}
				if(hair_mesh)
				{
					hair_mesh->updateJointGeometry();
				}
			}
			mNeedsSkin = false;
			mLastSkinTime = gFrameTimeSeconds;

			LLFace * face = mDrawable->getFace(0);
			if (face)
			{
				LLVertexBuffer* vb = face->getVertexBuffer();
				if (vb)
				{
					vb->unmapBuffer();
				}
			}
		}
	}
	else
	{
		mNeedsSkin = false;
	}

	if (sDebugInvisible)
	{
		LLNameValue* firstname = getNVPair("FirstName");
		if (firstname)
		{
			LL_DEBUGS("Avatar") << avString() << " in render" << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "Avatar " << this << " in render" << LL_ENDL;
		}
		if (!mIsBuilt)
		{
			LL_INFOS() << "Not built!" << LL_ENDL;
		}
		else if (!gAgent.needsRenderAvatar())
		{
			LL_INFOS() << "Doesn't need avatar render!" << LL_ENDL;
		}
		else
		{
			LL_INFOS() << "Rendering!" << LL_ENDL;
		}
	}

	if (!mIsBuilt)
	{
		return num_indices;
	}

	if (isSelf() && !gAgent.needsRenderAvatar())
	{
		return num_indices;
	}

	//--------------------------------------------------------------------
	// render all geometry attached to the skeleton
	//--------------------------------------------------------------------

		bool first_pass = true;
		if (!LLDrawPoolAvatar::sSkipOpaque)
		{
			if (isUIAvatar() && mIsDummy)
			{
				LLViewerJoint* hair_mesh = getViewerJoint(MESH_ID_HAIR);
				if (hair_mesh)
				{
					num_indices += hair_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = false;
			}
			if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
	
				if (isTextureVisible(TEX_HEAD_BAKED) || (getOverallAppearance() == AOA_JELLYDOLL && !isControlAvatar()) || isUIAvatar())
				{
					LLViewerJoint* head_mesh = getViewerJoint(MESH_ID_HEAD);
					if (head_mesh)
					{
						num_indices += head_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
					}
					first_pass = false;
				}
			}
			if (isTextureVisible(TEX_UPPER_BAKED) || (getOverallAppearance() == AOA_JELLYDOLL && !isControlAvatar()) || isUIAvatar())
			{
				LLViewerJoint* upper_mesh = getViewerJoint(MESH_ID_UPPER_BODY);
				if (upper_mesh)
				{
					num_indices += upper_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = false;
			}
			
			if (isTextureVisible(TEX_LOWER_BAKED) || (getOverallAppearance() == AOA_JELLYDOLL && !isControlAvatar()) || isUIAvatar())
			{
				LLViewerJoint* lower_mesh = getViewerJoint(MESH_ID_LOWER_BODY);
				if (lower_mesh)
				{
					num_indices += lower_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = false;
			}
		}

		if (!LLDrawPoolAvatar::sSkipTransparent || LLPipeline::sImpostorRender)
		{
			LLGLState blend(GL_BLEND, !mIsDummy);
			num_indices += renderTransparent(first_pass);
		}

	return num_indices;
}

U32 LLVOAvatar::renderTransparent(bool first_pass)
{
	U32 num_indices = 0;
	if( isWearingWearableType( LLWearableType::WT_SKIRT ) && (isUIAvatar() || isTextureVisible(TEX_SKIRT_BAKED)) )
	{
        gGL.flush();
		LLViewerJoint* skirt_mesh = getViewerJoint(MESH_ID_SKIRT);
		if (skirt_mesh)
		{
			num_indices += skirt_mesh->render(mAdjustedPixelArea, false);
		}
		first_pass = false;
        gGL.flush();
	}

	if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
	{
		if (LLPipeline::sImpostorRender)
		{
            gGL.flush();
		}
		
		if (isTextureVisible(TEX_HEAD_BAKED))
		{
			LLViewerJoint* eyelash_mesh = getViewerJoint(MESH_ID_EYELASH);
			if (eyelash_mesh)
			{
				num_indices += eyelash_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
			}
			first_pass = false;
		}
		if (isTextureVisible(TEX_HAIR_BAKED) && (getOverallAppearance() != AOA_JELLYDOLL))
		{
			LLViewerJoint* hair_mesh = getViewerJoint(MESH_ID_HAIR);
			if (hair_mesh)
			{
				num_indices += hair_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
			}
			first_pass = false;
		}
		if (LLPipeline::sImpostorRender)
		{
            gGL.flush();
		}
	}
	
	return num_indices;
}

//-----------------------------------------------------------------------------
// renderRigid()
//-----------------------------------------------------------------------------
U32 LLVOAvatar::renderRigid()
{
	U32 num_indices = 0;

	if (!mIsBuilt)
	{
		return 0;
	}

	if (isSelf() && (!gAgent.needsRenderAvatar() || !gAgent.needsRenderHead()))
	{
		return 0;
	}

	bool should_alpha_mask = shouldAlphaMask();
	LLGLState test(GL_ALPHA_TEST, should_alpha_mask);

	if (isTextureVisible(TEX_EYES_BAKED) || (getOverallAppearance() == AOA_JELLYDOLL && !isControlAvatar()) || isUIAvatar())
	{
		LLViewerJoint* eyeball_left = getViewerJoint(MESH_ID_EYEBALL_LEFT);
		LLViewerJoint* eyeball_right = getViewerJoint(MESH_ID_EYEBALL_RIGHT);
		if (eyeball_left)
		{
			num_indices += eyeball_left->render(mAdjustedPixelArea, true, mIsDummy);
		}
		if(eyeball_right)
		{
			num_indices += eyeball_right->render(mAdjustedPixelArea, true, mIsDummy);
		}
	}

	return num_indices;
}

U32 LLVOAvatar::renderImpostor(LLColor4U color, S32 diffuse_channel)
{
	if (!mImpostor.isComplete())
	{
		return 0;
	}

	LLVector3 pos(getRenderPosition()+mImpostorOffset);
	LLVector3 at = (pos - LLViewerCamera::getInstance()->getOrigin());
	at.normalize();
	LLVector3 left = LLViewerCamera::getInstance()->getUpAxis() % at;
	LLVector3 up = at%left;

	left *= mImpostorDim.mV[0];
	up *= mImpostorDim.mV[1];

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_IMPOSTORS))
	{
		LLGLEnable blend(GL_BLEND);
		gGL.setSceneBlendType(LLRender::BT_ADD);
		gGL.getTexUnit(diffuse_channel)->unbind(LLTexUnit::TT_TEXTURE);

		// gGL.begin(LLRender::QUADS);
		// gGL.vertex3fv((pos+left-up).mV);
		// gGL.vertex3fv((pos-left-up).mV);
		// gGL.vertex3fv((pos-left+up).mV);
		// gGL.vertex3fv((pos+left+up).mV);
		// gGL.end();


		gGL.begin(LLRender::LINES); 
		gGL.color4f(1.f,1.f,1.f,1.f);
		F32 thickness = llmax(F32(5.0f-5.0f*(gFrameTimeSeconds-mLastImpostorUpdateFrameTime)),1.0f);
		glLineWidth(thickness);
		gGL.vertex3fv((pos+left-up).mV);
		gGL.vertex3fv((pos-left-up).mV);
		gGL.vertex3fv((pos-left-up).mV);
		gGL.vertex3fv((pos-left+up).mV);
		gGL.vertex3fv((pos-left+up).mV);
		gGL.vertex3fv((pos+left+up).mV);
		gGL.vertex3fv((pos+left+up).mV);
		gGL.vertex3fv((pos+left-up).mV);
		gGL.end();
		gGL.flush();
	}
	{
	gGL.flush();

	gGL.color4ubv(color.mV);
	gGL.getTexUnit(diffuse_channel)->bind(&mImpostor);
	gGL.begin(LLRender::QUADS);
	gGL.texCoord2f(0,0);
	gGL.vertex3fv((pos+left-up).mV);
	gGL.texCoord2f(1,0);
	gGL.vertex3fv((pos-left-up).mV);
	gGL.texCoord2f(1,1);
	gGL.vertex3fv((pos-left+up).mV);
	gGL.texCoord2f(0,1);
	gGL.vertex3fv((pos+left+up).mV);
	gGL.end();
	gGL.flush();
	}

	return 6;
}

bool LLVOAvatar::allTexturesCompletelyDownloaded(std::set<LLUUID>& ids) const
{
	for (std::set<LLUUID>::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
		if (imagep && imagep->getDiscardLevel()!=0)
		{
			return false;
		}
	}
	return true;
}

bool LLVOAvatar::allLocalTexturesCompletelyDownloaded() const
{
	std::set<LLUUID> local_ids;
	collectLocalTextureUUIDs(local_ids);
	return allTexturesCompletelyDownloaded(local_ids);
}

bool LLVOAvatar::allBakedTexturesCompletelyDownloaded() const
{
	std::set<LLUUID> baked_ids;
	collectBakedTextureUUIDs(baked_ids);
	return allTexturesCompletelyDownloaded(baked_ids);
}

std::string LLVOAvatar::bakedTextureOriginInfo()
{
	std::string result;
	
	std::set<LLUUID> baked_ids;
	collectBakedTextureUUIDs(baked_ids);
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		ETextureIndex texture_index = mBakedTextureDatas[i].mTextureIndex;
		LLViewerFetchedTexture *imagep =
			LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), true);
		if (!imagep ||
			imagep->getID() == IMG_DEFAULT ||
			imagep->getID() == IMG_DEFAULT_AVATAR)
			
		{
			result += "-";
		}
		else
		{
			bool has_url = false, has_host = false;
			if (!imagep->getUrl().empty())
			{
				has_url = true;
			}
			if (imagep->getTargetHost().isOk())
			{
				has_host = true;
			}
			S32 discard = imagep->getDiscardLevel();
			if (has_url && !has_host) result += discard ? "u" : "U"; // server-bake texture with url 
			else if (has_host && !has_url) result += discard ? "h" : "H"; // old-style texture on sim
			else if (has_host && has_url) result += discard ? "x" : "X"; // both origins?
			else if (!has_host && !has_url) result += discard ? "n" : "N"; // no origin?
			if (discard != 0)
			{
				result += llformat("(%d/%d)",discard,imagep->getDesiredDiscardLevel());
			}
		}

	}
	return result;
}

S32Bytes LLVOAvatar::totalTextureMemForUUIDS(std::set<LLUUID>& ids)
{
	S32Bytes result(0);
	for (std::set<LLUUID>::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
		if (imagep)
		{
			result += imagep->getTextureMemory();
		}
	}
	return result;
}
	
void LLVOAvatar::collectLocalTextureUUIDs(std::set<LLUUID>& ids) const
{
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLWearableType::EType wearable_type = LLAvatarAppearance::getDictionary()->getTEWearableType((ETextureIndex)texture_index);
		U32 num_wearables = gAgentWearables.getWearableCount(wearable_type);

		LLViewerFetchedTexture *imagep = NULL;
		for (U32 wearable_index = 0; wearable_index < num_wearables; wearable_index++)
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index, wearable_index), true);
			if (imagep)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearance::getDictionary()->getTexture((ETextureIndex)texture_index);
				if (texture_dict && texture_dict->mIsLocalTexture)
				{
					ids.insert(imagep->getID());
				}
			}
		}
	}
	ids.erase(IMG_DEFAULT);
	ids.erase(IMG_DEFAULT_AVATAR);
	ids.erase(IMG_INVISIBLE);
}

void LLVOAvatar::collectBakedTextureUUIDs(std::set<LLUUID>& ids) const
{
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLViewerFetchedTexture *imagep = NULL;
		if (isIndexBakedTexture((ETextureIndex) texture_index))
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), true);
			if (imagep)
			{
				ids.insert(imagep->getID());
			}
		}
	}
	ids.erase(IMG_DEFAULT);
	ids.erase(IMG_DEFAULT_AVATAR);
	ids.erase(IMG_INVISIBLE);
}

void LLVOAvatar::collectTextureUUIDs(std::set<LLUUID>& ids)
{
	collectLocalTextureUUIDs(ids);
	collectBakedTextureUUIDs(ids);
}

void LLVOAvatar::releaseOldTextures()
{
	S32Bytes current_texture_mem;
	
	// Any textures that we used to be using but are no longer using should no longer be flagged as "NO_DELETE"
	std::set<LLUUID> baked_texture_ids;
	collectBakedTextureUUIDs(baked_texture_ids);
	S32Bytes new_baked_mem = totalTextureMemForUUIDS(baked_texture_ids);

	std::set<LLUUID> local_texture_ids;
	collectLocalTextureUUIDs(local_texture_ids);
	//S32 new_local_mem = totalTextureMemForUUIDS(local_texture_ids);

	std::set<LLUUID> new_texture_ids;
	new_texture_ids.insert(baked_texture_ids.begin(),baked_texture_ids.end());
	new_texture_ids.insert(local_texture_ids.begin(),local_texture_ids.end());
	S32Bytes new_total_mem = totalTextureMemForUUIDS(new_texture_ids);

	//S32 old_total_mem = totalTextureMemForUUIDS(mTextureIDs);
	//LL_DEBUGS("Avatar") << getFullname() << " old_total_mem: " << old_total_mem << " new_total_mem (L/B): " << new_total_mem << " (" << new_local_mem <<", " << new_baked_mem << ")" << LL_ENDL;  
	if (!isSelf() && new_total_mem > new_baked_mem)
	{
			LL_WARNS() << "extra local textures stored for non-self av" << LL_ENDL;
	}
	for (std::set<LLUUID>::iterator it = mTextureIDs.begin(); it != mTextureIDs.end(); ++it)
	{
		if (new_texture_ids.find(*it) == new_texture_ids.end())
		{
			LLViewerFetchedTexture *imagep = gTextureList.findImage(*it, TEX_LIST_STANDARD);
			if (imagep)
			{
				current_texture_mem += imagep->getTextureMemory();
				if (imagep->getTextureState() == LLGLTexture::NO_DELETE)
				{
					// This will allow the texture to be deleted if not in use.
					imagep->forceActive();

					// This resets the clock to texture being flagged
					// as unused, preventing the texture from being
					// deleted immediately. If other avatars or
					// objects are using it, it can still be flagged
					// no-delete by them.
					imagep->forceUpdateBindStats();
				}
			}
		}
	}
	mTextureIDs = new_texture_ids;
}

void LLVOAvatar::updateTextures()
{
	releaseOldTextures();
	
	bool render_avatar = true;

	if (mIsDummy)
	{
		return;
	}

	if( isSelf() )
	{
		render_avatar = true;
	}
	else
	{
		if(!isVisible())
		{
			return ;//do not update for invisible avatar.
		}

		render_avatar = !mCulled; //visible and not culled.
	}

	std::vector<bool> layer_baked;
	// GL NOT ACTIVE HERE - *TODO
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		layer_baked.push_back(isTextureDefined(mBakedTextureDatas[i].mTextureIndex));
		// bind the texture so that they'll be decoded slightly 
		// inefficient, we can short-circuit this if we have to
		if (render_avatar && !gGLManager.mIsDisabled)
		{
			if (layer_baked[i] && !mBakedTextureDatas[i].mIsLoaded)
			{
				gGL.getTexUnit(0)->bind(getImage( mBakedTextureDatas[i].mTextureIndex, 0 ));
			}
		}
	}

	mMaxPixelArea = 0.f;
	mMinPixelArea = 99999999.f;
	mHasGrey = false; // debug
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLWearableType::EType wearable_type = LLAvatarAppearance::getDictionary()->getTEWearableType((ETextureIndex)texture_index);
		U32 num_wearables = gAgentWearables.getWearableCount(wearable_type);
		const LLTextureEntry *te = getTE(texture_index);

		// getTE can return 0.
		// Not sure yet why it does, but of course it crashes when te->mScale? gets used.
		// Put safeguard in place so this corner case get better handling and does not result in a crash.
		F32 texel_area_ratio = 1.0f;
		if( te )
		{
			texel_area_ratio = fabs(te->mScaleS * te->mScaleT);
		}
		else
		{
			LL_WARNS() << "getTE( " << texture_index << " ) returned 0" <<LL_ENDL;
		}

		LLViewerFetchedTexture *imagep = NULL;
		for (U32 wearable_index = 0; wearable_index < num_wearables; wearable_index++)
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index, wearable_index), true);
			if (imagep)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearance::getDictionary()->getTexture((ETextureIndex)texture_index);
				const EBakedTextureIndex baked_index = texture_dict ? texture_dict->mBakedTextureIndex : EBakedTextureIndex::BAKED_NUM_INDICES;
				if (texture_dict && texture_dict->mIsLocalTexture)
				{
					addLocalTextureStats((ETextureIndex)texture_index, imagep, texel_area_ratio, render_avatar, mBakedTextureDatas[baked_index].mIsUsed);
				}
			}
		}
		if (isIndexBakedTexture((ETextureIndex) texture_index) && render_avatar)
		{
			const S32 boost_level = getAvatarBakedBoostLevel();
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), true);
			addBakedTextureStats( imagep, mPixelArea, texel_area_ratio, boost_level );			
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%4.0f:%4.0f", (F32) sqrt(mMinPixelArea),(F32) sqrt(mMaxPixelArea)));
	}	
}


void LLVOAvatar::addLocalTextureStats( ETextureIndex idx, LLViewerFetchedTexture* imagep,
									   F32 texel_area_ratio, bool render_avatar, bool covered_by_baked)
{
	// No local texture stats for non-self avatars
	return;
}

const S32 MAX_TEXTURE_UPDATE_INTERVAL = 64 ; //need to call updateTextures() at least every 32 frames.	
const S32 MAX_TEXTURE_VIRTUAL_SIZE_RESET_INTERVAL = S32_MAX ; //frames
void LLVOAvatar::checkTextureLoading()
{
	static const F32 MAX_INVISIBLE_WAITING_TIME = 15.f ; //seconds

	bool pause = !isVisible() ;
	if(!pause)
	{
		mInvisibleTimer.reset() ;
	}
	if(mLoadedCallbacksPaused == pause)
	{
        if (!pause && mFirstFullyVisible && mLoadedCallbackTextures < mCallbackTextureList.size())
        {
            // We still need to update 'loaded' textures count to decide on 'cloud' visibility
            // Alternatively this can be done on TextureLoaded callbacks, but is harder to properly track
            mLoadedCallbackTextures = 0;
            for (LLLoadedCallbackEntry::source_callback_list_t::iterator iter = mCallbackTextureList.begin();
                iter != mCallbackTextureList.end(); ++iter)
            {
                LLViewerFetchedTexture* tex = gTextureList.findImage(*iter);
                if (tex && (tex->getDiscardLevel() >= 0 || tex->isMissingAsset()))
                {
                    mLoadedCallbackTextures++;
                }
            }
        }
		return ; 
	}
	
	if(mCallbackTextureList.empty()) //when is self or no callbacks. Note: this list for self is always empty.
	{
		mLoadedCallbacksPaused = pause ;
		mLoadedCallbackTextures = 0;
		return ; //nothing to check.
	}
	
	if(pause && mInvisibleTimer.getElapsedTimeF32() < MAX_INVISIBLE_WAITING_TIME)
	{
		return ; //have not been invisible for enough time.
	}

	mLoadedCallbackTextures = pause ? mCallbackTextureList.size() : 0;

	for(LLLoadedCallbackEntry::source_callback_list_t::iterator iter = mCallbackTextureList.begin();
		iter != mCallbackTextureList.end(); ++iter)
	{
		LLViewerFetchedTexture* tex = gTextureList.findImage(*iter) ;
		if(tex)
		{
			if(pause)//pause texture fetching.
			{
				tex->pauseLoadedCallbacks(&mCallbackTextureList) ;

				//set to terminate texture fetching after MAX_TEXTURE_UPDATE_INTERVAL frames.
				tex->setMaxVirtualSizeResetInterval(MAX_TEXTURE_UPDATE_INTERVAL);
				tex->resetMaxVirtualSizeResetCounter() ;
			}
			else//unpause
			{
				static const F32 START_AREA = 100.f ;

				tex->unpauseLoadedCallbacks(&mCallbackTextureList) ;
				tex->addTextureStats(START_AREA); //jump start the fetching again

				// technically shouldn't need to account for missing, but callback might not have happened yet
				if (tex->getDiscardLevel() >= 0 || tex->isMissingAsset())
				{
					mLoadedCallbackTextures++; // consider it loaded (we have at least some data)
				}
			}
		}
	}
	
	if(!pause)
	{
		updateTextures() ; //refresh texture stats.
	}
	mLoadedCallbacksPaused = pause ;
	return ;
}

const F32  SELF_ADDITIONAL_PRI = 0.75f ;
void LLVOAvatar::addBakedTextureStats( LLViewerFetchedTexture* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level)
{
	//Note:
	//if this function is not called for the last MAX_TEXTURE_VIRTUAL_SIZE_RESET_INTERVAL frames, 
	//the texture pipeline will stop fetching this texture.

	imagep->resetTextureStats();
	imagep->setMaxVirtualSizeResetInterval(MAX_TEXTURE_VIRTUAL_SIZE_RESET_INTERVAL);
	imagep->resetMaxVirtualSizeResetCounter() ;

	mMaxPixelArea = llmax(pixel_area, mMaxPixelArea);
	mMinPixelArea = llmin(pixel_area, mMinPixelArea);	
	imagep->addTextureStats(pixel_area / texel_area_ratio);
	imagep->setBoostLevel(boost_level);
}

//virtual	
void LLVOAvatar::setImage(const U8 te, LLViewerTexture *imagep, const U32 index)
{
	setTEImage(te, imagep);
}

//virtual 
LLViewerTexture* LLVOAvatar::getImage(const U8 te, const U32 index) const
{
	return getTEImage(te);
}
//virtual 
const LLTextureEntry* LLVOAvatar::getTexEntry(const U8 te_num) const
{
	return getTE(te_num);
}

//virtual 
void LLVOAvatar::setTexEntry(const U8 index, const LLTextureEntry &te)
{
	setTE(index, te);
}

const std::string LLVOAvatar::getImageURL(const U8 te, const LLUUID &uuid)
{
	llassert(isIndexBakedTexture(ETextureIndex(te)));
	std::string url = "";
	const std::string& appearance_service_url = LLAppearanceMgr::instance().getAppearanceServiceURL();
	if (appearance_service_url.empty())
	{
		// Probably a server-side issue if we get here:
		LL_WARNS() << "AgentAppearanceServiceURL not set - Baked texture requests will fail" << LL_ENDL;
		return url;
	}
	
	const LLAvatarAppearanceDictionary::TextureEntry* texture_entry = LLAvatarAppearance::getDictionary()->getTexture((ETextureIndex)te);
	if (texture_entry != NULL)
	{
		url = appearance_service_url + "texture/" + getID().asString() + "/" + texture_entry->mDefaultImageName + "/" + uuid.asString();
		//LL_INFOS() << "baked texture url: " << url << LL_ENDL;
	}
	return url;
}

//-----------------------------------------------------------------------------
// resolveHeight()
//-----------------------------------------------------------------------------

void LLVOAvatar::resolveHeightAgent(const LLVector3 &in_pos_agent, LLVector3 &out_pos_agent, LLVector3 &out_norm)
{
	LLVector3d in_pos_global, out_pos_global;

	in_pos_global = gAgent.getPosGlobalFromAgent(in_pos_agent);
	resolveHeightGlobal(in_pos_global, out_pos_global, out_norm);
	out_pos_agent = gAgent.getPosAgentFromGlobal(out_pos_global);
}


void LLVOAvatar::resolveRayCollisionAgent(const LLVector3d start_pt, const LLVector3d end_pt, LLVector3d &out_pos, LLVector3 &out_norm)
{
	LLViewerObject *obj;
	LLWorld::getInstance()->resolveStepHeightGlobal(this, start_pt, end_pt, out_pos, out_norm, &obj);
}

void LLVOAvatar::resolveHeightGlobal(const LLVector3d &inPos, LLVector3d &outPos, LLVector3 &outNorm)
{
	LLVector3d zVec(0.0f, 0.0f, 0.5f);
	LLVector3d p0 = inPos + zVec;
	LLVector3d p1 = inPos - zVec;
	LLViewerObject *obj;
	LLWorld::getInstance()->resolveStepHeightGlobal(this, p0, p1, outPos, outNorm, &obj);
	if (!obj)
	{
		mStepOnLand = true;
		mStepMaterial = 0;
		mStepObjectVelocity.setVec(0.0f, 0.0f, 0.0f);
	}
	else
	{
		mStepOnLand = false;
		mStepMaterial = obj->getMaterial();

		// We want the primitive velocity, not our velocity... (which actually subtracts the
		// step object velocity)
		LLVector3 angularVelocity = obj->getAngularVelocity();
		LLVector3 relativePos = gAgent.getPosAgentFromGlobal(outPos) - obj->getPositionAgent();

		LLVector3 linearComponent = angularVelocity % relativePos;
//		LL_INFOS() << "Linear Component of Rotation Velocity " << linearComponent << LL_ENDL;
		mStepObjectVelocity = obj->getVelocity() + linearComponent;
	}
}


//-----------------------------------------------------------------------------
// getStepSound()
//-----------------------------------------------------------------------------
const LLUUID& LLVOAvatar::getStepSound() const
{
	if ( mStepOnLand )
	{
		return sStepSoundOnLand;
	}

	return sStepSounds[mStepMaterial];
}


//-----------------------------------------------------------------------------
// processAnimationStateChanges()
//-----------------------------------------------------------------------------
void LLVOAvatar::processAnimationStateChanges()
{
	if ( isAnyAnimationSignaled(AGENT_WALK_ANIMS, NUM_AGENT_WALK_ANIMS) )
	{
		startMotion(ANIM_AGENT_WALK_ADJUST);
		stopMotion(ANIM_AGENT_FLY_ADJUST);
	}
	else if (mInAir && !isSitting())
	{
		stopMotion(ANIM_AGENT_WALK_ADJUST);
        if (mEnableDefaultMotions)
        {
		startMotion(ANIM_AGENT_FLY_ADJUST);
	}
	}
	else
	{
		stopMotion(ANIM_AGENT_WALK_ADJUST);
		stopMotion(ANIM_AGENT_FLY_ADJUST);
	}

	if ( isAnyAnimationSignaled(AGENT_GUN_AIM_ANIMS, NUM_AGENT_GUN_AIM_ANIMS) )
	{
        if (mEnableDefaultMotions)
        {
		startMotion(ANIM_AGENT_TARGET);
        }
		stopMotion(ANIM_AGENT_BODY_NOISE);
	}
	else
	{
		stopMotion(ANIM_AGENT_TARGET);
        if (mEnableDefaultMotions)
        {
			startMotion(ANIM_AGENT_BODY_NOISE);
		}
	}
	
	// clear all current animations
	AnimIterator anim_it;
	for (anim_it = mPlayingAnimations.begin(); anim_it != mPlayingAnimations.end();)
	{
		AnimIterator found_anim = mSignaledAnimations.find(anim_it->first);

		// playing, but not signaled, so stop
		if (found_anim == mSignaledAnimations.end())
		{
			processSingleAnimationStateChange(anim_it->first, false);
			mPlayingAnimations.erase(anim_it++);
			continue;
		}

		++anim_it;
	}

	// if jellydolled, shelve all playing animations
	if (getOverallAppearance() != AOA_NORMAL)
	{
		mPlayingAnimations.clear();
	}
	
	// start up all new anims
	if (getOverallAppearance() == AOA_NORMAL)
	{
		for (anim_it = mSignaledAnimations.begin(); anim_it != mSignaledAnimations.end();)
		{
			AnimIterator found_anim = mPlayingAnimations.find(anim_it->first);

			// signaled but not playing, or different sequence id, start motion
			if (found_anim == mPlayingAnimations.end() || found_anim->second != anim_it->second)
			{
				if (processSingleAnimationStateChange(anim_it->first, true))
				{
					mPlayingAnimations[anim_it->first] = anim_it->second;
					++anim_it;
					continue;
				}
			}

			++anim_it;
		}
	}

	// clear source information for animations which have been stopped
	if (isSelf())
	{
		AnimSourceIterator source_it = mAnimationSources.begin();

		for (source_it = mAnimationSources.begin(); source_it != mAnimationSources.end();)
		{
			if (mSignaledAnimations.find(source_it->second) == mSignaledAnimations.end())
			{
				mAnimationSources.erase(source_it++);
			}
			else
			{
				++source_it;
			}
		}
	}

	stop_glerror();
}


//-----------------------------------------------------------------------------
// processSingleAnimationStateChange();
//-----------------------------------------------------------------------------
bool LLVOAvatar::processSingleAnimationStateChange( const LLUUID& anim_id, bool start )
{
    // SL-402, SL-427 - we need to update body size often enough to
    // keep appearances in sync, but not so often that animations
    // cause constant jiggling of the body or camera. Possible
    // compromise is to do it on animation changes:
    computeBodySize();
    
	bool result = false;

	if ( start ) // start animation
	{
		if (anim_id == ANIM_AGENT_TYPE)
		{
			if (gAudiop)
			{
				LLVector3d char_pos_global = gAgent.getPosGlobalFromAgent(getCharacterPosition());
				if (LLViewerParcelMgr::getInstance()->canHearSound(char_pos_global)
				    && !LLMuteList::getInstance()->isMuted(getID(), LLMute::flagObjectSounds))
				{
					// RN: uncomment this to play on typing sound at fixed volume once sound engine is fixed
					// to support both spatialized and non-spatialized instances of the same sound
					//if (isSelf())
					//{
					//	gAudiop->triggerSound(LLUUID(gSavedSettings.getString("UISndTyping")), 1.0f, LLAudioEngine::AUDIO_TYPE_UI);
					//}
					//else
					{
                        static LLCachedControl<std::string> ui_snd_string(gSavedSettings, "UISndTyping");
						LLUUID sound_id = LLUUID(ui_snd_string);
						gAudiop->triggerSound(sound_id, getID(), 1.0f, LLAudioEngine::AUDIO_TYPE_SFX, char_pos_global);
					}
				}
			}
		}
		else if (anim_id == ANIM_AGENT_SIT_GROUND_CONSTRAINED)
		{
			sitDown(true);
		}


		if (startMotion(anim_id))
		{
			result = true;
		}
		else
		{
			LL_WARNS("Motion") << "Failed to start motion!" << LL_ENDL;
		}
	}
	else //stop animation
	{
		if (anim_id == ANIM_AGENT_SIT_GROUND_CONSTRAINED)
		{
			sitDown(false);
		}
		if ((anim_id == ANIM_AGENT_DO_NOT_DISTURB) && gAgent.isDoNotDisturb())
		{
			// re-assert DND tag animation
			gAgent.sendAnimationRequest(ANIM_AGENT_DO_NOT_DISTURB, ANIM_REQUEST_START);
			return result;
		}
		stopMotion(anim_id);
		result = true;
	}

	return result;
}

//-----------------------------------------------------------------------------
// isAnyAnimationSignaled()
//-----------------------------------------------------------------------------
bool LLVOAvatar::isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims) const
{
	for (S32 i = 0; i < num_anims; i++)
	{
		if(mSignaledAnimations.find(anim_array[i]) != mSignaledAnimations.end())
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// resetAnimations()
//-----------------------------------------------------------------------------
void LLVOAvatar::resetAnimations()
{
	LLKeyframeMotion::flushKeyframeCache();
	flushAllMotions();
}

// Override selectively based on avatar sex and whether we're using new
// animations.
LLUUID LLVOAvatar::remapMotionID(const LLUUID& id)
{
    static LLCachedControl<bool> use_new_walk_run(gSavedSettings, "UseNewWalkRun");
	LLUUID result = id;

	// start special case female walk for female avatars
	if (getSex() == SEX_FEMALE)
	{
		if (id == ANIM_AGENT_WALK)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_FEMALE_WALK_NEW;
			else
				result = ANIM_AGENT_FEMALE_WALK;
		}
		else if (id == ANIM_AGENT_RUN)
		{
			// There is no old female run animation, so only override
			// in one case.
			if (use_new_walk_run)
				result = ANIM_AGENT_FEMALE_RUN_NEW;
		}
		else if (id == ANIM_AGENT_SIT)
		{
			result = ANIM_AGENT_SIT_FEMALE;
		}
	}
	else
	{
		// Male avatar.
		if (id == ANIM_AGENT_WALK)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_WALK_NEW;
		}
		else if (id == ANIM_AGENT_RUN)
		{
			if (use_new_walk_run)
				result = ANIM_AGENT_RUN_NEW;
		}
		// keeps in sync with setSex() related code (viewer controls sit's sex)
		else if (id == ANIM_AGENT_SIT_FEMALE)
		{
			result = ANIM_AGENT_SIT;
		}
	
	}

	return result;

}

//-----------------------------------------------------------------------------
// startMotion()
// id is the asset if of the animation to start
// time_offset is the offset into the animation at which to start playing
//-----------------------------------------------------------------------------
bool LLVOAvatar::startMotion(const LLUUID& id, F32 time_offset)
{
	LL_DEBUGS("Motion") << "motion requested " << id.asString() << " " << gAnimLibrary.animationName(id) << LL_ENDL;

	LLUUID remap_id = remapMotionID(id);

	if (remap_id != id)
	{
		LL_DEBUGS("Motion") << "motion resultant " << remap_id.asString() << " " << gAnimLibrary.animationName(remap_id) << LL_ENDL;
	}

	if (isSelf() && remap_id == ANIM_AGENT_AWAY)
	{
		gAgent.setAFK();
	}

	return LLCharacter::startMotion(remap_id, time_offset);
}

//-----------------------------------------------------------------------------
// stopMotion()
//-----------------------------------------------------------------------------
bool LLVOAvatar::stopMotion(const LLUUID& id, bool stop_immediate)
{
	LL_DEBUGS("Motion") << "Motion requested " << id.asString() << " " << gAnimLibrary.animationName(id) << LL_ENDL;

	LLUUID remap_id = remapMotionID(id);
	
	if (remap_id != id)
	{
		LL_DEBUGS("Motion") << "motion resultant " << remap_id.asString() << " " << gAnimLibrary.animationName(remap_id) << LL_ENDL;
	}

	if (isSelf())
	{
		gAgent.onAnimStop(remap_id);
	}

	return LLCharacter::stopMotion(remap_id, stop_immediate);
}

//-----------------------------------------------------------------------------
// hasMotionFromSource()
//-----------------------------------------------------------------------------
// virtual
bool LLVOAvatar::hasMotionFromSource(const LLUUID& source_id)
{
	return false;
}

//-----------------------------------------------------------------------------
// stopMotionFromSource()
//-----------------------------------------------------------------------------
// virtual
void LLVOAvatar::stopMotionFromSource(const LLUUID& source_id)
{
}

//-----------------------------------------------------------------------------
// addDebugText()
//-----------------------------------------------------------------------------
void LLVOAvatar::addDebugText(const std::string& text)
{
	mDebugText.append(1, '\n');
	mDebugText.append(text);
}

//-----------------------------------------------------------------------------
// getID()
//-----------------------------------------------------------------------------
const LLUUID& LLVOAvatar::getID() const
{
	return mID;
}

//-----------------------------------------------------------------------------
// getJoint()
//-----------------------------------------------------------------------------
// RN: avatar joints are multi-rooted to include screen-based attachments
LLJoint *LLVOAvatar::getJoint( const std::string &name )
{
	joint_map_t::iterator iter = mJointMap.find(name);

	LLJoint* jointp = NULL;

	if (iter == mJointMap.end() || iter->second == NULL)
	{   //search for joint and cache found joint in lookup table
		if (mJointAliasMap.empty())
		{
			getJointAliases();
		}
		joint_alias_map_t::const_iterator alias_iter = mJointAliasMap.find(name);
		std::string canonical_name;
		if (alias_iter != mJointAliasMap.end())
		{
			canonical_name = alias_iter->second;
		}
		else
		{
			canonical_name = name;
		}
		jointp = mRoot->findJoint(canonical_name);
		mJointMap[name] = jointp;
	}
	else
	{   //return cached pointer
		jointp = iter->second;
	}

#ifndef LL_RELEASE_FOR_DOWNLOAD
    if (jointp && jointp->getName()!="mScreen" && jointp->getName()!="mRoot")
    {
        llassert(getJoint(jointp->getJointNum())==jointp);
    }
#endif
	return jointp;
}

LLJoint *LLVOAvatar::getJoint( S32 joint_num )
{
    LLJoint *pJoint = NULL;
    if (joint_num >= 0)
    {
        if (joint_num < mNumBones)
        {
            pJoint = mSkeleton[joint_num];
        }
        else if (joint_num < mNumBones + mNumCollisionVolumes)
        {
            S32 collision_id = joint_num - mNumBones;
            pJoint = &mCollisionVolumes[collision_id];
        }
        else
        {
            // Attachment IDs start at 1
            S32 attachment_id = joint_num - (mNumBones + mNumCollisionVolumes) + 1;
            attachment_map_t::iterator iter = mAttachmentPoints.find(attachment_id);
            if (iter != mAttachmentPoints.end())
            {
                pJoint = iter->second;
            }
        }
    }
    
	llassert(!pJoint || pJoint->getJointNum() == joint_num);
    return pJoint;
}

//-----------------------------------------------------------------------------
// getRiggedMeshID
//
// If viewer object is a rigged mesh, set the mesh id and return true.
// Otherwise, null out the id and return false.
//-----------------------------------------------------------------------------
// static
bool LLVOAvatar::getRiggedMeshID(LLViewerObject* pVO, LLUUID& mesh_id)
{
	mesh_id.setNull();
	
	//If a VO has a skin that we'll reset the joint positions to their default
	if ( pVO && pVO->mDrawable )
	{
		LLVOVolume* pVObj = pVO->mDrawable->getVOVolume();
		if ( pVObj )
		{
			const LLMeshSkinInfo* pSkinData = pVObj->getSkinInfo();
			if (pSkinData 
				&& pSkinData->mJointNames.size() > JOINT_COUNT_REQUIRED_FOR_FULLRIG	// full rig
				&& pSkinData->mAlternateBindMatrix.size() > 0 )
					{				
						mesh_id = pSkinData->mMeshID;
						return true;
					}
		}
	}
	return false;
}

bool LLVOAvatar::jointIsRiggedTo(const LLJoint *joint) const
{
    if (joint)
	{
        const LLJointRiggingInfoTab& tab = mJointRiggingInfoTab;
        S32 joint_num = joint->getJointNum();
        if (joint_num < tab.size() && tab[joint_num].isRiggedTo())
            {
                return true;
            }
        }
    return false;
}

void LLVOAvatar::clearAttachmentOverrides()
{
    LLScopedContextString str("clearAttachmentOverrides " + getFullname());

    for (S32 i=0; i<LL_CHARACTER_MAX_ANIMATED_JOINTS; i++)
	{
        LLJoint *pJoint = getJoint(i);
        if (pJoint)
        {
			pJoint->clearAttachmentPosOverrides();
			pJoint->clearAttachmentScaleOverrides();
        }
	}

    if (mPelvisFixups.count()>0)
    {
        mPelvisFixups.clear();
        LLJoint* pJointPelvis = getJoint("mPelvis");
        if (pJointPelvis)
	{
			pJointPelvis->setPosition( LLVector3( 0.0f, 0.0f, 0.0f) );
        }
        postPelvisSetRecalc();	
	}

    mActiveOverrideMeshes.clear();
    onActiveOverrideMeshesChanged();
}

//-----------------------------------------------------------------------------
// rebuildAttachmentOverrides
//-----------------------------------------------------------------------------
void LLVOAvatar::rebuildAttachmentOverrides()
{
    LLScopedContextString str("rebuildAttachmentOverrides " + getFullname());

    LL_DEBUGS("AnimatedObjects") << "rebuilding" << LL_ENDL;
    dumpStack("AnimatedObjectsStack");
    
    clearAttachmentOverrides();

    // Handle the case that we're resetting the skeleton of an animated object.
    LLControlAvatar *control_av = dynamic_cast<LLControlAvatar*>(this);
    if (control_av)
	{
        LLVOVolume *volp = control_av->mRootVolp;
        if (volp)
        {
            LL_DEBUGS("Avatar") << volp->getID() << " adding attachment overrides for root vol, prim count " 
                                << (S32) (1+volp->numChildren()) << LL_ENDL;
            addAttachmentOverridesForObject(volp);
        }
    }

    // Attached objects
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment *attachment_pt = (*iter).second;
        if (attachment_pt)
        {
            for (LLViewerJointAttachment::attachedobjs_vec_t::iterator at_it = attachment_pt->mAttachedObjects.begin();
				 at_it != attachment_pt->mAttachedObjects.end(); ++at_it)
            {
                LLViewerObject *vo = at_it->get();
                // Attached animated objects affect joints in their control
                // avs, not the avs to which they are attached.
                if (vo && !vo->isAnimatedObject())
                {
                    addAttachmentOverridesForObject(vo);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// updateAttachmentOverrides
//
// This is intended to give the same results as
// rebuildAttachmentOverrides(), while avoiding redundant work.
// -----------------------------------------------------------------------------
void LLVOAvatar::updateAttachmentOverrides()
{
    LLScopedContextString str("updateAttachmentOverrides " + getFullname());

    LL_DEBUGS("AnimatedObjects") << "updating" << LL_ENDL;
    dumpStack("AnimatedObjectsStack");

    std::set<LLUUID> meshes_seen;
    
    // Handle the case that we're updating the skeleton of an animated object.
    LLControlAvatar *control_av = dynamic_cast<LLControlAvatar*>(this);
    if (control_av)
    {
        LLVOVolume *volp = control_av->mRootVolp;
        if (volp)
        {
            LL_DEBUGS("Avatar") << volp->getID() << " adding attachment overrides for root vol, prim count " 
                                << (S32) (1+volp->numChildren()) << LL_ENDL;
            addAttachmentOverridesForObject(volp, &meshes_seen);
        }
    }

    // Attached objects
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment *attachment_pt = (*iter).second;
        if (attachment_pt)
        {
            for (LLViewerJointAttachment::attachedobjs_vec_t::iterator at_it = attachment_pt->mAttachedObjects.begin();
				 at_it != attachment_pt->mAttachedObjects.end(); ++at_it)
            {
                LLViewerObject *vo = at_it->get();
                // Attached animated objects affect joints in their control
                // avs, not the avs to which they are attached.
                if (vo && !vo->isAnimatedObject())
                {
                    addAttachmentOverridesForObject(vo, &meshes_seen);
                }
            }
        }
    }
    // Remove meshes that are no longer present on the skeleton

	// have to work with a copy because removeAttachmentOverrides() will change mActiveOverrideMeshes.
    std::set<LLUUID> active_override_meshes = mActiveOverrideMeshes; 
    for (std::set<LLUUID>::iterator it = active_override_meshes.begin(); it != active_override_meshes.end(); ++it)
    {
        if (meshes_seen.find(*it) == meshes_seen.end())
        {
            removeAttachmentOverridesForObject(*it);
        }
    }


#ifdef ATTACHMENT_OVERRIDE_VALIDATION
    {
        std::vector<LLVector3OverrideMap> pos_overrides_by_joint;
        std::vector<LLVector3OverrideMap> scale_overrides_by_joint;
        LLVector3OverrideMap pelvis_fixups;

        // Capture snapshot of override state after update
        for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
        {
            LLVector3OverrideMap pos_overrides;
            LLJoint *joint = getJoint(joint_num);
            if (joint)
            {
                pos_overrides_by_joint.push_back(joint->m_attachmentPosOverrides);
                scale_overrides_by_joint.push_back(joint->m_attachmentScaleOverrides);
            }
            else
            {
                // No joint, use default constructed empty maps
                pos_overrides_by_joint.push_back(LLVector3OverrideMap());
                scale_overrides_by_joint.push_back(LLVector3OverrideMap());
            }
        }
        pelvis_fixups = mPelvisFixups;
        //dumpArchetypeXML(getFullname() + "_paranoid_updated");

        // Rebuild and compare
        rebuildAttachmentOverrides();
        //dumpArchetypeXML(getFullname() + "_paranoid_rebuilt");
        bool mismatched = false;
        for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
        {
            LLJoint *joint = getJoint(joint_num);
            if (joint)
            {
                if (pos_overrides_by_joint[joint_num] != joint->m_attachmentPosOverrides)
                {
                    mismatched = true;
                }
                if (scale_overrides_by_joint[joint_num] != joint->m_attachmentScaleOverrides)
                {
                    mismatched = true;
            }
        }
    }
        if (pelvis_fixups != mPelvisFixups)
        {
            mismatched = true;
        }
        if (mismatched)
        {
            LL_WARNS() << "MISMATCHED ATTACHMENT OVERRIDES" << LL_ENDL;
        }
    }
#endif
}

void LLVOAvatar::notifyAttachmentMeshLoaded()
{
    if (!isFullyLoaded())
    {
        // We just received mesh or skin info
        // Reset timer to wait for more potential meshes or changes
        mFullyLoadedTimer.reset();
    }
}

//-----------------------------------------------------------------------------
// addAttachmentOverridesForObject
//-----------------------------------------------------------------------------
void LLVOAvatar::addAttachmentOverridesForObject(LLViewerObject *vo, std::set<LLUUID>* meshes_seen, bool recursive)
{
    if (vo->getAvatar() != this && vo->getAvatarAncestor() != this)
	{
		LL_WARNS("Avatar") << "called with invalid avatar" << LL_ENDL;
        return;
	}

    LLScopedContextString str("addAttachmentOverridesForObject " + getFullname());

	if (getOverallAppearance() != AOA_NORMAL)
	{
		return;
	}
    
    LL_DEBUGS("AnimatedObjects") << "adding" << LL_ENDL;
    dumpStack("AnimatedObjectsStack");
    
	// Process all children
    if (recursive)
    {
	LLViewerObject::const_child_list_t& children = vo->getChildren();
	for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
		 it != children.end(); ++it)
	{
		LLViewerObject *childp = *it;
            addAttachmentOverridesForObject(childp, meshes_seen, true);
        }
	}

	LLVOVolume *vobj = dynamic_cast<LLVOVolume*>(vo);
	bool pelvisGotSet = false;

	if (!vobj)
	{
		return;
	}

	LLViewerObject *root_object = (LLViewerObject*)vobj->getRoot();
    LL_DEBUGS("AnimatedObjects") << "trying to add attachment overrides for root object " << root_object->getID() << " prim is " << vobj << LL_ENDL;
	if (vobj->isMesh() &&
		((vobj->getVolume() && !vobj->getVolume()->isMeshAssetLoaded()) || !gMeshRepo.meshRezEnabled()))
	{
        LL_DEBUGS("AnimatedObjects") << "failed to add attachment overrides for root object " << root_object->getID() << " mesh asset not loaded" << LL_ENDL;
		return;
	}
	const LLMeshSkinInfo*  pSkinData = vobj->getSkinInfo();

	if ( vobj && vobj->isMesh() && pSkinData )
	{
		const int bindCnt = pSkinData->mAlternateBindMatrix.size();								
        const int jointCnt = pSkinData->mJointNames.size();
        if ((bindCnt > 0) && (bindCnt != jointCnt))
        {
            LL_WARNS_ONCE() << "invalid mesh, bindCnt " << bindCnt << "!= jointCnt " << jointCnt << ", joint overrides will be ignored." << LL_ENDL;
        }
		if ((bindCnt > 0) && (bindCnt == jointCnt))
		{					
			const F32 pelvisZOffset = pSkinData->mPelvisOffset;
			const LLUUID& mesh_id = pSkinData->mMeshID;

            if (meshes_seen)
            {
                meshes_seen->insert(mesh_id);
            }
            bool mesh_overrides_loaded = (mActiveOverrideMeshes.find(mesh_id) != mActiveOverrideMeshes.end());
            if (mesh_overrides_loaded)
            {
                LL_DEBUGS("AnimatedObjects") << "skipping add attachment overrides for " << mesh_id 
                                             << " to root object " << root_object->getID()
                                             << ", already loaded"
                                             << LL_ENDL;
            }
            else
            {
                LL_DEBUGS("AnimatedObjects") << "adding attachment overrides for " << mesh_id 
                                             << " to root object " << root_object->getID() << LL_ENDL;
            }
			bool fullRig = (jointCnt>=JOINT_COUNT_REQUIRED_FOR_FULLRIG) ? true : false;								
			if ( fullRig && !mesh_overrides_loaded )
			{								
				for ( int i=0; i<jointCnt; ++i )
				{
					std::string lookingForJoint = pSkinData->mJointNames[i].c_str();
					LLJoint* pJoint = getJoint( lookingForJoint );
					if (pJoint)
					{   									
						const LLVector3& jointPos = LLVector3(pSkinData->mAlternateBindMatrix[i].getTranslation());
                        if (pJoint->aboveJointPosThreshold(jointPos))
                        {
                            bool override_changed;
                            pJoint->addAttachmentPosOverride( jointPos, mesh_id, avString(), override_changed );
                            
                            if (override_changed)
                            {
                                //If joint is a pelvis then handle old/new pelvis to foot values
                                if ( lookingForJoint == "mPelvis" )
                                {	
                                    pelvisGotSet = true;											
                                }										
                            }
                            if (pSkinData->mLockScaleIfJointPosition)
                            {
                                // Note that unlike positions, there's no threshold check here,
                                // just a lock at the default value.
                                pJoint->addAttachmentScaleOverride(pJoint->getDefaultScale(), mesh_id, avString());
                            }
                        }
					}										
				}																
				if (pelvisZOffset != 0.0F)
				{
                    F32 pelvis_fixup_before;
                    bool has_fixup_before =  hasPelvisFixup(pelvis_fixup_before);
					addPelvisFixup( pelvisZOffset, mesh_id );
					F32 pelvis_fixup_after;
                    hasPelvisFixup(pelvis_fixup_after); // Don't have to check bool here because we just added it...
                    if (!has_fixup_before || (pelvis_fixup_before != pelvis_fixup_after))
                    {
                        pelvisGotSet = true;											
                    }
                    
				}
                mActiveOverrideMeshes.insert(mesh_id);
                onActiveOverrideMeshesChanged();
			}							
		}
	}
    else
    {
        LL_DEBUGS("AnimatedObjects") << "failed to add attachment overrides for root object " << root_object->getID() << " not mesh or no pSkinData" << LL_ENDL;
    }
					
	//Rebuild body data if we altered joints/pelvis
	if ( pelvisGotSet ) 
	{
		postPelvisSetRecalc();
	}		
}

//-----------------------------------------------------------------------------
// getAttachmentOverrideNames
//-----------------------------------------------------------------------------
void LLVOAvatar::getAttachmentOverrideNames(std::set<std::string>& pos_names, std::set<std::string>& scale_names) const
{
    LLVector3 pos;
    LLVector3 scale;
    LLUUID mesh_id;

    // Bones
	for (avatar_joint_list_t::const_iterator iter = mSkeleton.begin();
         iter != mSkeleton.end(); ++iter)
	{
		const LLJoint* pJoint = (*iter);
		if (pJoint && pJoint->hasAttachmentPosOverride(pos,mesh_id))
		{
            pos_names.insert(pJoint->getName());
		}
		if (pJoint && pJoint->hasAttachmentScaleOverride(scale,mesh_id))
		{
            scale_names.insert(pJoint->getName());
		}
	}

    // Attachment points
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
        if (attachment_pt && attachment_pt->hasAttachmentPosOverride(pos,mesh_id))
        {
            pos_names.insert(attachment_pt->getName());
        }
        // Attachment points don't have scales.
    }

}

//-----------------------------------------------------------------------------
// showAttachmentOverrides
//-----------------------------------------------------------------------------
void LLVOAvatar::showAttachmentOverrides(bool verbose) const
{
    std::set<std::string> pos_names, scale_names;
    getAttachmentOverrideNames(pos_names, scale_names);
    if (pos_names.size())
    {
        std::stringstream ss;
        std::copy(pos_names.begin(), pos_names.end(), std::ostream_iterator<std::string>(ss, ","));
        LL_INFOS() << getFullname() << " attachment positions defined for joints: " << ss.str() << "\n" << LL_ENDL;
    }
    else
    {
        LL_DEBUGS("Avatar") << getFullname() << " no attachment positions defined for any joints" << "\n" << LL_ENDL;
    }
    if (scale_names.size())
    {
        std::stringstream ss;
        std::copy(scale_names.begin(), scale_names.end(), std::ostream_iterator<std::string>(ss, ","));
        LL_INFOS() << getFullname() << " attachment scales defined for joints: " << ss.str() << "\n" << LL_ENDL;
    }
    else
    {
        LL_INFOS() << getFullname() << " no attachment scales defined for any joints" << "\n" << LL_ENDL;
    }

    if (!verbose)
    {
        return;
    }

    LLVector3 pos, scale;
    LLUUID mesh_id;
    S32 count = 0;

    // Bones
	for (avatar_joint_list_t::const_iterator iter = mSkeleton.begin();
         iter != mSkeleton.end(); ++iter)
	{
		const LLJoint* pJoint = (*iter);
		if (pJoint && pJoint->hasAttachmentPosOverride(pos,mesh_id))
		{
			pJoint->showAttachmentPosOverrides(getFullname());
            count++;
		}
		if (pJoint && pJoint->hasAttachmentScaleOverride(scale,mesh_id))
		{
			pJoint->showAttachmentScaleOverrides(getFullname());
            count++;
        }
	}

    // Attachment points
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
        if (attachment_pt && attachment_pt->hasAttachmentPosOverride(pos,mesh_id))
        {
            attachment_pt->showAttachmentPosOverrides(getFullname());
            count++;
        }
    }

    if (count)
    {
        LL_DEBUGS("Avatar") << avString() << " end of pos, scale overrides" << LL_ENDL;
        LL_DEBUGS("Avatar") << "=================================" << LL_ENDL;
    }
}

//-----------------------------------------------------------------------------
// removeAttachmentOverridesForObject
//-----------------------------------------------------------------------------
void LLVOAvatar::removeAttachmentOverridesForObject(LLViewerObject *vo)
{
    if (vo->getAvatar() != this && vo->getAvatarAncestor() != this)
	{
		LL_WARNS("Avatar") << "called with invalid avatar" << LL_ENDL;
        return;
	}
		
	// Process all children
	LLViewerObject::const_child_list_t& children = vo->getChildren();
	for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
		 it != children.end(); ++it)
	{
		LLViewerObject *childp = *it;
		removeAttachmentOverridesForObject(childp);
	}

	// Process self.
	LLUUID mesh_id;
	if (getRiggedMeshID(vo,mesh_id))
	{
		removeAttachmentOverridesForObject(mesh_id);
	}
}

//-----------------------------------------------------------------------------
// removeAttachmentOverridesForObject
//-----------------------------------------------------------------------------
void LLVOAvatar::removeAttachmentOverridesForObject(const LLUUID& mesh_id)
{	
	LLJoint* pJointPelvis = getJoint("mPelvis");
    const std::string av_string = avString();
    for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
	{
        LLJoint *pJoint = getJoint(joint_num);
		if ( pJoint )
		{			
            bool dummy; // unused
			pJoint->removeAttachmentPosOverride(mesh_id, av_string, dummy);
			pJoint->removeAttachmentScaleOverride(mesh_id, av_string);
		}		
		if ( pJoint && pJoint == pJointPelvis)
		{
			removePelvisFixup( mesh_id );
			// SL-315
			pJoint->setPosition( LLVector3( 0.0f, 0.0f, 0.0f) );
		}		
	}	
		
	postPelvisSetRecalc();	

    mActiveOverrideMeshes.erase(mesh_id);
    onActiveOverrideMeshesChanged();
}
//-----------------------------------------------------------------------------
// getCharacterPosition()
//-----------------------------------------------------------------------------
LLVector3 LLVOAvatar::getCharacterPosition()
{
	if (mDrawable.notNull())
	{
		return mDrawable->getPositionAgent();
	}
	else
	{
		return getPositionAgent();
	}
}


//-----------------------------------------------------------------------------
// LLVOAvatar::getCharacterRotation()
//-----------------------------------------------------------------------------
LLQuaternion LLVOAvatar::getCharacterRotation()
{
	return getRotation();
}


//-----------------------------------------------------------------------------
// LLVOAvatar::getCharacterVelocity()
//-----------------------------------------------------------------------------
LLVector3 LLVOAvatar::getCharacterVelocity()
{
	return getVelocity() - mStepObjectVelocity;
}


//-----------------------------------------------------------------------------
// LLVOAvatar::getCharacterAngularVelocity()
//-----------------------------------------------------------------------------
LLVector3 LLVOAvatar::getCharacterAngularVelocity()
{
	return getAngularVelocity();
}

//-----------------------------------------------------------------------------
// LLVOAvatar::getGround()
//-----------------------------------------------------------------------------
void LLVOAvatar::getGround(const LLVector3 &in_pos_agent, LLVector3 &out_pos_agent, LLVector3 &outNorm)
{
	LLVector3d z_vec(0.0f, 0.0f, 1.0f);
	LLVector3d p0_global, p1_global;

	if (isUIAvatar())
	{
		outNorm.setVec(z_vec);
		out_pos_agent = in_pos_agent;
		return;
	}
	
	p0_global = gAgent.getPosGlobalFromAgent(in_pos_agent) + z_vec;
	p1_global = gAgent.getPosGlobalFromAgent(in_pos_agent) - z_vec;
	LLViewerObject *obj;
	LLVector3d out_pos_global;
	LLWorld::getInstance()->resolveStepHeightGlobal(this, p0_global, p1_global, out_pos_global, outNorm, &obj);
	out_pos_agent = gAgent.getPosAgentFromGlobal(out_pos_global);
}

//-----------------------------------------------------------------------------
// LLVOAvatar::getTimeDilation()
//-----------------------------------------------------------------------------
F32 LLVOAvatar::getTimeDilation()
{
	return mRegionp ? mRegionp->getTimeDilation() : 1.f;
}


//-----------------------------------------------------------------------------
// LLVOAvatar::getPixelArea()
//-----------------------------------------------------------------------------
F32 LLVOAvatar::getPixelArea() const
{
	if (isUIAvatar())
	{
		return 100000.f;
	}
	return mPixelArea;
}



//-----------------------------------------------------------------------------
// LLVOAvatar::getPosGlobalFromAgent()
//-----------------------------------------------------------------------------
LLVector3d	LLVOAvatar::getPosGlobalFromAgent(const LLVector3 &position)
{
	return gAgent.getPosGlobalFromAgent(position);
}

//-----------------------------------------------------------------------------
// getPosAgentFromGlobal()
//-----------------------------------------------------------------------------
LLVector3	LLVOAvatar::getPosAgentFromGlobal(const LLVector3d &position)
{
	return gAgent.getPosAgentFromGlobal(position);
}


//-----------------------------------------------------------------------------
// requestStopMotion()
//-----------------------------------------------------------------------------
// virtual
void LLVOAvatar::requestStopMotion( LLMotion* motion )
{
	// Only agent avatars should handle the stop motion notifications.
}

//-----------------------------------------------------------------------------
// loadSkeletonNode(): loads <skeleton> node from XML tree
//-----------------------------------------------------------------------------
//virtual
bool LLVOAvatar::loadSkeletonNode ()
{
	if (!LLAvatarAppearance::loadSkeletonNode())
	{
		return false;
	}
	
    bool ignore_hud_joints = false;
    initAttachmentPoints(ignore_hud_joints);

	return true;
}

//-----------------------------------------------------------------------------
// initAttachmentPoints(): creates attachment points if needed, sets state based on avatar_lad.xml. 
//-----------------------------------------------------------------------------
void LLVOAvatar::initAttachmentPoints(bool ignore_hud_joints)
{
    LLAvatarXmlInfo::attachment_info_list_t::iterator iter;
    for (iter = sAvatarXmlInfo->mAttachmentInfoList.begin();
         iter != sAvatarXmlInfo->mAttachmentInfoList.end(); 
         ++iter)
    {
        LLAvatarXmlInfo::LLAvatarAttachmentInfo *info = *iter;
        if (info->mIsHUDAttachment && (!isSelf() || ignore_hud_joints))
        {
		    //don't process hud joint for other avatars.
            continue;
        }

        S32 attachmentID = info->mAttachmentID;
        if (attachmentID < 1 || attachmentID > 255)
        {
            LL_WARNS() << "Attachment point out of range [1-255]: " << attachmentID << " on attachment point " << info->mName << LL_ENDL;
            continue;
        }

        LLViewerJointAttachment* attachment = NULL;
        bool newly_created = false;
        if (mAttachmentPoints.find(attachmentID) == mAttachmentPoints.end())
        {
            attachment = new LLViewerJointAttachment();
            newly_created = true;
        }
        else
        {
            attachment = mAttachmentPoints[attachmentID];
        }

        attachment->setName(info->mName);
        LLJoint *parent_joint = getJoint(info->mJointName);
        if (!parent_joint)
        {
            // If the intended parent for attachment point is unavailable, avatar_lad.xml is corrupt.
            LL_WARNS() << "No parent joint by name " << info->mJointName << " found for attachment point " << info->mName << LL_ENDL;
            LL_ERRS() << "Invalid avatar_lad.xml file" << LL_ENDL;
        }

        if (info->mHasPosition)
        {
            attachment->setOriginalPosition(info->mPosition);
            attachment->setDefaultPosition(info->mPosition);
        }
			
        if (info->mHasRotation)
        {
            LLQuaternion rotation;
            rotation.setQuat(info->mRotationEuler.mV[VX] * DEG_TO_RAD,
                             info->mRotationEuler.mV[VY] * DEG_TO_RAD,
                             info->mRotationEuler.mV[VZ] * DEG_TO_RAD);
            attachment->setRotation(rotation);
        }

        int group = info->mGroup;
        if (group >= 0)
        {
            if (group < 0 || group > 9)
            {
                LL_WARNS() << "Invalid group number (" << group << ") for attachment point " << info->mName << LL_ENDL;
            }
            else
            {
                attachment->setGroup(group);
            }
        }

        attachment->setPieSlice(info->mPieMenuSlice);
        attachment->setVisibleInFirstPerson(info->mVisibleFirstPerson);
        attachment->setIsHUDAttachment(info->mIsHUDAttachment);
        // attachment can potentially be animated, needs a number.
        attachment->setJointNum(mNumBones + mNumCollisionVolumes + attachmentID - 1);

        if (newly_created)
        {
            mAttachmentPoints[attachmentID] = attachment;
            
            // now add attachment joint
            parent_joint->addChild(attachment);
        }
    }
}

//-----------------------------------------------------------------------------
// updateVisualParams()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateVisualParams()
{
	ESex avatar_sex = (getVisualParamWeight("male") > 0.5f) ? SEX_MALE : SEX_FEMALE;
	if (getSex() != avatar_sex)
	{
		if (mIsSitting && findMotion(avatar_sex == SEX_MALE ? ANIM_AGENT_SIT_FEMALE : ANIM_AGENT_SIT) != NULL)
		{
			// In some cases of gender change server changes sit motion with motion message,
			// but in case of some avatars (legacy?) there is no update from server side,
			// likely because server doesn't know about difference between motions
			// (female and male sit ids are same server side, so it is likely unaware that it
			// need to send update)
			// Make sure motion is up to date
			stopMotion(ANIM_AGENT_SIT);
			setSex(avatar_sex);
			startMotion(ANIM_AGENT_SIT);
		}
		else
		{
			setSex(avatar_sex);
		}
	}

	LLCharacter::updateVisualParams();

	if (mLastSkeletonSerialNum != mSkeletonSerialNum)
	{
		computeBodySize();
		mLastSkeletonSerialNum = mSkeletonSerialNum;
		mRoot->updateWorldMatrixChildren();
	}

	dirtyMesh();
	updateHeadOffset();
}
//-----------------------------------------------------------------------------
// isActive()
//-----------------------------------------------------------------------------
bool LLVOAvatar::isActive() const
{
	return true;
}

//-----------------------------------------------------------------------------
// setPixelAreaAndAngle()
//-----------------------------------------------------------------------------
void LLVOAvatar::setPixelAreaAndAngle(LLAgent &agent)
{
	if (mDrawable.isNull())
	{
		return;
	}

	const LLVector4a* ext = mDrawable->getSpatialExtents();
	LLVector4a center;
	center.setAdd(ext[1], ext[0]);
	center.mul(0.5f);
	LLVector4a size;
	size.setSub(ext[1], ext[0]);
	size.mul(0.5f);

	mImpostorPixelArea = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());
    mPixelArea = mImpostorPixelArea;

	F32 range = mDrawable->mDistanceWRTCamera;

	if (range < 0.001f)		// range == zero
	{
		mAppAngle = 180.f;
	}
	else
	{
		F32 radius = size.getLength3().getF32();
		mAppAngle = (F32) atan2( radius, range) * RAD_TO_DEG;
	}

	// We always want to look good to ourselves
	if( isSelf() )
	{
		mPixelArea = llmax( mPixelArea, F32(getTexImageSize() / 16) );
	}
}

//-----------------------------------------------------------------------------
// updateJointLODs()
//-----------------------------------------------------------------------------
bool LLVOAvatar::updateJointLODs()
{
	const F32 MAX_PIXEL_AREA = 100000000.f;
	F32 lod_factor = (sLODFactor * AVATAR_LOD_TWEAK_RANGE + (1.f - AVATAR_LOD_TWEAK_RANGE));
	F32 avatar_num_min_factor = clamp_rescale(sLODFactor, 0.f, 1.f, 0.25f, 0.6f);
	F32 avatar_num_factor = clamp_rescale((F32)sNumVisibleAvatars, 8, 25, 1.f, avatar_num_min_factor);
	F32 area_scale = 0.16f;

		if (isSelf())
		{
			if(gAgentCamera.cameraCustomizeAvatar() || gAgentCamera.cameraMouselook())
			{
				mAdjustedPixelArea = MAX_PIXEL_AREA;
			}
			else
			{
				mAdjustedPixelArea = mPixelArea*area_scale;
			}
		}
		else if (mIsDummy)
		{
			mAdjustedPixelArea = MAX_PIXEL_AREA;
		}
		else
		{
			// reported avatar pixel area is dependent on avatar render load, based on number of visible avatars
			mAdjustedPixelArea = (F32)mPixelArea * area_scale * lod_factor * lod_factor * avatar_num_factor * avatar_num_factor;
		}

		// now select meshes to render based on adjusted pixel area
		LLViewerJoint* root = dynamic_cast<LLViewerJoint*>(mRoot);
		bool res = false;
		if (root)
		{
			res = root->updateLOD(mAdjustedPixelArea, true);
		}
 		if (res)
		{
			sNumLODChangesThisFrame++;
			dirtyMesh(2);
			return true;
		}

	return false;
}

//-----------------------------------------------------------------------------
// createDrawable()
//-----------------------------------------------------------------------------
LLDrawable *LLVOAvatar::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(false);

	LLDrawPoolAvatar *poolp = (LLDrawPoolAvatar*)gPipeline.getPool(mIsControlAvatar ? LLDrawPool::POOL_CONTROL_AV : LLDrawPool::POOL_AVATAR);

	// Only a single face (one per avatar)
	//this face will be splitted into several if its vertex buffer is too long.
	mDrawable->setState(LLDrawable::ACTIVE);
	mDrawable->addFace(poolp, NULL);
	mDrawable->setRenderType(mIsControlAvatar ? LLPipeline::RENDER_TYPE_CONTROL_AV : LLPipeline::RENDER_TYPE_AVATAR);
	
	mNumInitFaces = mDrawable->getNumFaces() ;

	dirtyMesh(2);
	return mDrawable;
}


void LLVOAvatar::updateGL()
{
	if (mMeshTexturesDirty)
	{
		LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR
		updateMeshTextures();
		mMeshTexturesDirty = false;
	}
}

//-----------------------------------------------------------------------------
// updateGeometry()
//-----------------------------------------------------------------------------
bool LLVOAvatar::updateGeometry(LLDrawable *drawable)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
	if (!(gPipeline.hasRenderType(mIsControlAvatar ? LLPipeline::RENDER_TYPE_CONTROL_AV : LLPipeline::RENDER_TYPE_AVATAR)))
	{
		return true;
	}
	
	if (!mMeshValid)
	{
		return true;
	}

	if (!drawable)
	{
		LL_ERRS() << "LLVOAvatar::updateGeometry() called with NULL drawable" << LL_ENDL;
	}

	return true;
}

//-----------------------------------------------------------------------------
// updateSexDependentLayerSets()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateSexDependentLayerSets()
{
	invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet);
	invalidateComposite( mBakedTextureDatas[BAKED_UPPER].mTexLayerSet);
	invalidateComposite( mBakedTextureDatas[BAKED_LOWER].mTexLayerSet);
}

//-----------------------------------------------------------------------------
// dirtyMesh()
//-----------------------------------------------------------------------------
void LLVOAvatar::dirtyMesh()
{
	dirtyMesh(1);
}
void LLVOAvatar::dirtyMesh(S32 priority)
{
	mDirtyMesh = llmax(mDirtyMesh, priority);
}

//-----------------------------------------------------------------------------
// getViewerJoint()
//-----------------------------------------------------------------------------
LLViewerJoint*	LLVOAvatar::getViewerJoint(S32 idx)
{
	return dynamic_cast<LLViewerJoint*>(mMeshLOD[idx]);
}

//-----------------------------------------------------------------------------
// hideHair()
//-----------------------------------------------------------------------------
void LLVOAvatar::hideHair()
{
    mMeshLOD[MESH_ID_HAIR]->setVisible(false, true);
}

//-----------------------------------------------------------------------------
// hideSkirt()
//-----------------------------------------------------------------------------
void LLVOAvatar::hideSkirt()
{
	mMeshLOD[MESH_ID_SKIRT]->setVisible(false, true);
}

bool LLVOAvatar::setParent(LLViewerObject* parent)
{
	bool ret ;
	if (parent == NULL)
	{
		getOffObject();
		ret = LLViewerObject::setParent(parent);
		if (isSelf())
		{
			gAgentCamera.resetCamera();
		}
	}
	else
	{
		ret = LLViewerObject::setParent(parent);
		if(ret)
		{
			sitOnObject(parent);
		}
	}
	return ret ;
}

void LLVOAvatar::addChild(LLViewerObject *childp)
{
	childp->extractAttachmentItemID(); // find the inventory item this object is associated with.
	if (isSelf())
	{
	    const LLUUID& item_id = childp->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << "ATT attachment child added " << (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;

	}

	LLViewerObject::addChild(childp);
	if (childp->mDrawable)
	{
		if (!attachObject(childp))
		{
			LL_WARNS() << "ATT addChild() failed for " 
					<< childp->getID()
					<< " item " << childp->getAttachmentItemID()
					<< LL_ENDL;
			// MAINT-3312 backout
			// mPendingAttachment.push_back(childp);
		}
	}
	else
	{
		mPendingAttachment.push_back(childp);
	}
}

void LLVOAvatar::removeChild(LLViewerObject *childp)
{
	LLViewerObject::removeChild(childp);
	if (!detachObject(childp))
	{
		LL_WARNS() << "Calling detach on non-attached object " << LL_ENDL;
	}
}

LLViewerJointAttachment* LLVOAvatar::getTargetAttachmentPoint(LLViewerObject* viewer_object)
{
	S32 attachmentID = ATTACHMENT_ID_FROM_STATE(viewer_object->getAttachmentState());

	// This should never happen unless the server didn't process the attachment point
	// correctly, but putting this check in here to be safe.
	if (attachmentID & ATTACHMENT_ADD)
	{
		LL_WARNS() << "Got an attachment with ATTACHMENT_ADD mask, removing ( attach pt:" << attachmentID << " )" << LL_ENDL;
		attachmentID &= ~ATTACHMENT_ADD;
	}
	
	LLViewerJointAttachment* attachment = get_if_there(mAttachmentPoints, attachmentID, (LLViewerJointAttachment*)NULL);

	if (!attachment)
	{
		LL_WARNS() << "Object attachment point invalid: " << attachmentID 
			<< " trying to use 1 (chest)"
			<< LL_ENDL;

		attachment = get_if_there(mAttachmentPoints, 1, (LLViewerJointAttachment*)NULL); // Arbitrary using 1 (chest)
		if (attachment)
		{
			LL_WARNS() << "Object attachment point invalid: " << attachmentID 
				<< " on object " << viewer_object->getID()
				<< " attachment item " << viewer_object->getAttachmentItemID()
				<< " falling back to 1 (chest)"
				<< LL_ENDL;
		}
		else
		{
			LL_WARNS() << "Object attachment point invalid: " << attachmentID 
				<< " on object " << viewer_object->getID()
				<< " attachment item " << viewer_object->getAttachmentItemID()
				<< "Unable to use fallback attachment point 1 (chest)"
				<< LL_ENDL;
		}
	}

	return attachment;
}

//-----------------------------------------------------------------------------
// attachObject()
//-----------------------------------------------------------------------------
const LLViewerJointAttachment *LLVOAvatar::attachObject(LLViewerObject *viewer_object)
{
	if (isSelf())
	{
		const LLUUID& item_id = viewer_object->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_DEBUGS("Avatar") << "ATT attaching object "
							<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;	
	}
	LLViewerJointAttachment* attachment = getTargetAttachmentPoint(viewer_object);

	if (!attachment || !attachment->addObject(viewer_object))
	{
		const LLUUID& item_id = viewer_object->getAttachmentItemID();
		LLViewerInventoryItem *item = gInventory.getItem(item_id);
		LL_WARNS("Avatar") << "ATT attach failed "
						   << (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;	
		return 0;
	}

    if (!viewer_object->isAnimatedObject())
    {
        updateAttachmentOverrides();
    }

	updateVisualComplexity();

	if (viewer_object->isSelected())
	{
		LLSelectMgr::getInstance()->updateSelectionCenter();
		LLSelectMgr::getInstance()->updatePointAt();
	}

	viewer_object->refreshBakeTexture();


	LLViewerObject::const_child_list_t& child_list = viewer_object->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		iter != child_list.end(); ++iter)
	{
		LLViewerObject* objectp = *iter;
		if (objectp)
		{
			objectp->refreshBakeTexture();
		}
	}

	updateMeshVisibility();

	return attachment;
}

//-----------------------------------------------------------------------------
// getNumAttachments()
//-----------------------------------------------------------------------------
U32 LLVOAvatar::getNumAttachments() const
{
	U32 num_attachments = 0;
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
		num_attachments += attachment_pt->getNumObjects();
	}
	return num_attachments;
}

//-----------------------------------------------------------------------------
// getMaxAttachments()
//-----------------------------------------------------------------------------
S32 LLVOAvatar::getMaxAttachments() const
{
	return LLAgentBenefitsMgr::current().getAttachmentLimit();
}

//-----------------------------------------------------------------------------
// canAttachMoreObjects()
// Returns true if we can attach <n> more objects.
//-----------------------------------------------------------------------------
bool LLVOAvatar::canAttachMoreObjects(U32 n) const
{
	return (getNumAttachments() + n) <= getMaxAttachments();
}

//-----------------------------------------------------------------------------
// getNumAnimatedObjectAttachments()
//-----------------------------------------------------------------------------
U32 LLVOAvatar::getNumAnimatedObjectAttachments() const
{
	U32 num_attachments = 0;
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin();
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		const LLViewerJointAttachment *attachment_pt = (*iter).second;
		num_attachments += attachment_pt->getNumAnimatedObjects();
	}
	return num_attachments;
}

//-----------------------------------------------------------------------------
// getMaxAnimatedObjectAttachments()
// Gets from simulator feature if available, otherwise 0.
//-----------------------------------------------------------------------------
S32 LLVOAvatar::getMaxAnimatedObjectAttachments() const
{
	return LLAgentBenefitsMgr::current().getAnimatedObjectLimit();
}

//-----------------------------------------------------------------------------
// canAttachMoreAnimatedObjects()
// Returns true if we can attach <n> more animated objects.
//-----------------------------------------------------------------------------
bool LLVOAvatar::canAttachMoreAnimatedObjects(U32 n) const
{
	return (getNumAnimatedObjectAttachments() + n) <= getMaxAnimatedObjectAttachments();
}

//-----------------------------------------------------------------------------
// lazyAttach()
//-----------------------------------------------------------------------------
void LLVOAvatar::lazyAttach()
{
	std::vector<LLPointer<LLViewerObject> > still_pending;
	
	for (U32 i = 0; i < mPendingAttachment.size(); i++)
	{
		LLPointer<LLViewerObject> cur_attachment = mPendingAttachment[i];
		// Object might have died while we were waiting for drawable
		if (!cur_attachment->isDead())
		{
			if (cur_attachment->mDrawable)
			{
				if (isSelf())
				{
					const LLUUID& item_id = cur_attachment->getAttachmentItemID();
					LLViewerInventoryItem *item = gInventory.getItem(item_id);
					LL_DEBUGS("Avatar") << "ATT attaching object "
						<< (item ? item->getName() : "UNKNOWN") << " id " << item_id << LL_ENDL;
				}
				if (!attachObject(cur_attachment))
				{	// Drop it
					LL_WARNS() << "attachObject() failed for "
						<< cur_attachment->getID()
						<< " item " << cur_attachment->getAttachmentItemID()
						<< LL_ENDL;
					// MAINT-3312 backout
					//still_pending.push_back(cur_attachment);
				}
			}
			else
			{
				still_pending.push_back(cur_attachment);
			}
		}
	}

	mPendingAttachment = still_pending;
}

void LLVOAvatar::resetHUDAttachments()
{

	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment())
		{
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				const LLViewerObject* attached_object = attachment_iter->get();
				if (attached_object && attached_object->mDrawable.notNull())
				{
					gPipeline.markMoved(attached_object->mDrawable);
				}
			}
		}
	}
}

void LLVOAvatar::rebuildRiggedAttachments( void )
{
	for ( attachment_map_t::iterator iter = mAttachmentPoints.begin(); iter != mAttachmentPoints.end(); ++iter )
	{
		LLViewerJointAttachment* pAttachment = iter->second;
		LLViewerJointAttachment::attachedobjs_vec_t::iterator attachmentIterEnd = pAttachment->mAttachedObjects.end();
		
		for ( LLViewerJointAttachment::attachedobjs_vec_t::iterator attachmentIter = pAttachment->mAttachedObjects.begin();
			 attachmentIter != attachmentIterEnd; ++attachmentIter)
		{
			const LLViewerObject* pAttachedObject =  *attachmentIter;
			if ( pAttachment && pAttachedObject->mDrawable.notNull() )
			{
				gPipeline.markRebuild(pAttachedObject->mDrawable);
			}
		}
	}
}
//-----------------------------------------------------------------------------
// cleanupAttachedMesh()
//-----------------------------------------------------------------------------
void LLVOAvatar::cleanupAttachedMesh( LLViewerObject* pVO )
{
	LLUUID mesh_id;
	if (getRiggedMeshID(pVO, mesh_id))
	{
        // FIXME this seems like an odd place for this code.
		if ( gAgentCamera.cameraCustomizeAvatar() )
		{
			gAgent.unpauseAnimation();
			//Still want to refocus on head bone
			gAgentCamera.changeCameraToCustomizeAvatar();
		}
	}
}

bool LLVOAvatar::hasPendingAttachedMeshes()
{
    for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
         iter != mAttachmentPoints.end();
         ++iter)
    {
        LLViewerJointAttachment* attachment = iter->second;
        if (attachment)
        {
            for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
                 attachment_iter != attachment->mAttachedObjects.end();
                 ++attachment_iter)
            {
                LLViewerObject* objectp = attachment_iter->get();
                if (objectp)
                {
                    LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
                    for (LLViewerObject::child_list_t::const_iterator iter1 = child_list.begin();
                         iter1 != child_list.end(); ++iter1)
                    {
                        LLViewerObject* objectchild = *iter1;
                        if (objectchild && objectchild->getVolume())
                        {
                            const LLUUID& mesh_id = objectchild->getVolume()->getParams().getSculptID();
                            if (mesh_id.isNull())
                            {
                                // No mesh nor skin info needed
                                continue;
                            }

                            if (objectchild->getVolume()->isMeshAssetUnavaliable())
                            {
                                // Mesh failed to load, do not expect it
                                continue;
                            }

                            if (objectchild->mDrawable)
                            {
                                LLVOVolume* pvobj = objectchild->mDrawable->getVOVolume();
                                if (pvobj)
                                {
                                    if (!pvobj->isMesh())
                                    {
                                        // Not a mesh
                                        continue;
                                    }

                                    if (!objectchild->getVolume()->isMeshAssetLoaded())
                                    {
                                        // Waiting for mesh
                                        return true;
                                    }

                                    const LLMeshSkinInfo* skin_data = pvobj->getSkinInfo();
                                    if (skin_data)
                                    {
                                        // Skin info present, done
                                        continue;
                                    }

                                    if (pvobj->isSkinInfoUnavaliable())
                                    {
                                        // Load failed or info not present, don't expect it
                                        continue;
                                    }
                                }

                                // objectchild is not ready
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
// detachObject()
//-----------------------------------------------------------------------------
bool LLVOAvatar::detachObject(LLViewerObject *viewer_object)
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		
		if (attachment->isObjectAttached(viewer_object))
		{
            updateVisualComplexity();
            bool is_animated_object = viewer_object->isAnimatedObject();
			cleanupAttachedMesh(viewer_object);

			attachment->removeObject(viewer_object);
            if (!is_animated_object)
            {
                updateAttachmentOverrides();
            }
			viewer_object->refreshBakeTexture();
		
			LLViewerObject::const_child_list_t& child_list = viewer_object->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter1 = child_list.begin();
				iter1 != child_list.end(); ++iter1)
			{
				LLViewerObject* objectp = *iter1;
				if (objectp)
            {
					objectp->refreshBakeTexture();
				}
            }

			updateMeshVisibility();

			LL_DEBUGS() << "Detaching object " << viewer_object->mID << " from " << attachment->getName() << LL_ENDL;
			return true;
		}
	}

	std::vector<LLPointer<LLViewerObject> >::iterator iter = std::find(mPendingAttachment.begin(), mPendingAttachment.end(), viewer_object);
	if (iter != mPendingAttachment.end())
	{
		mPendingAttachment.erase(iter);
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// sitDown()
//-----------------------------------------------------------------------------
void LLVOAvatar::sitDown(bool bSitting)
{
	mIsSitting = bSitting;
	if (isSelf())
	{
		// Update Movement Controls according to own Sitting mode
		LLFloaterMove::setSittingMode(bSitting);
	}
}

//-----------------------------------------------------------------------------
// sitOnObject()
//-----------------------------------------------------------------------------
void LLVOAvatar::sitOnObject(LLViewerObject *sit_object)
{
	if (isSelf())
	{
		// Might be first sit
		//LLFirstUse::useSit();

		gAgent.setFlying(false);
		gAgentCamera.setThirdPersonHeadOffset(LLVector3::zero);
		//interpolate to new camera position
		gAgentCamera.startCameraAnimation();
		// make sure we are not trying to autopilot
		gAgent.stopAutoPilot();
		gAgentCamera.setupSitCamera();
		if (gAgentCamera.getForceMouselook())
		{
			gAgentCamera.changeCameraToMouselook();
		}

        if (gAgentCamera.getFocusOnAvatar() && LLToolMgr::getInstance()->inEdit())
        {
            LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstRootNode();
            if (node && node->mValid)
            {
                LLViewerObject* root_object = node->getObject();
                if (root_object == sit_object)
                {
                    LLFloaterTools::sPreviousFocusOnAvatar = true;
                }
            }
        }
	}

	if (mDrawable.isNull())
	{
		return;
	}
	LLQuaternion inv_obj_rot = ~sit_object->getRenderRotation();
	LLVector3 obj_pos = sit_object->getRenderPosition();

	LLVector3 rel_pos = getRenderPosition() - obj_pos;
	rel_pos.rotVec(inv_obj_rot);

	mDrawable->mXform.setPosition(rel_pos);
	mDrawable->mXform.setRotation(mDrawable->getWorldRotation() * inv_obj_rot);

	gPipeline.markMoved(mDrawable, true);
	// Notice that removing sitDown() from here causes avatars sitting on
	// objects to be not rendered for new arrivals. See EXT-6835 and EXT-1655.
	sitDown(true);
	mRoot->getXform()->setParent(&sit_object->mDrawable->mXform); // LLVOAvatar::sitOnObject
	// SL-315
	mRoot->setPosition(getPosition());
	mRoot->updateWorldMatrixChildren();

	stopMotion(ANIM_AGENT_BODY_NOISE);
	
	gAgentCamera.setInitSitRot(gAgent.getFrameAgent().getQuaternion());
}

//-----------------------------------------------------------------------------
// getOffObject()
//-----------------------------------------------------------------------------
void LLVOAvatar::getOffObject()
{
	if (mDrawable.isNull())
	{
		return;
	}

	LLViewerObject* sit_object = (LLViewerObject*)getParent();

	if (sit_object)
	{
		stopMotionFromSource(sit_object->getID());
		LLFollowCamMgr::getInstance()->setCameraActive(sit_object->getID(), false);

		LLViewerObject::const_child_list_t& child_list = sit_object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); ++iter)
		{
			LLViewerObject* child_objectp = *iter;

			stopMotionFromSource(child_objectp->getID());
			LLFollowCamMgr::getInstance()->setCameraActive(child_objectp->getID(), false);
		}
	}

	// assumes that transform will not be updated with drawable still having a parent
	// or that drawable had no parent from the start
	LLVector3 cur_position_world = mDrawable->getWorldPosition();
	LLQuaternion cur_rotation_world = mDrawable->getWorldRotation();

	if (mLastRootPos.length() >= MAX_STANDOFF_FROM_ORIGIN
		&& (cur_position_world.length() < MAX_STANDOFF_FROM_ORIGIN
			|| dist_vec(cur_position_world, mLastRootPos) > MAX_STANDOFF_DISTANCE_CHANGE))
	{
		// Most likely drawable got updated too early or some updates were missed - we got relative position to non-existing parent
		// restore coordinates from cache
		cur_position_world = mLastRootPos;
	}

	// set *local* position based on last *world* position, since we're unparenting the avatar
	mDrawable->mXform.setPosition(cur_position_world);
	mDrawable->mXform.setRotation(cur_rotation_world);	
	
	gPipeline.markMoved(mDrawable, true);

	sitDown(false);

	mRoot->getXform()->setParent(NULL); // LLVOAvatar::getOffObject
	// SL-315
	mRoot->setPosition(cur_position_world);
	mRoot->setRotation(cur_rotation_world);
	mRoot->getXform()->update();

    if (mEnableDefaultMotions)
    {
	startMotion(ANIM_AGENT_BODY_NOISE);
    }

	if (isSelf())
	{
		LLQuaternion av_rot = gAgent.getFrameAgent().getQuaternion();
		LLQuaternion obj_rot = sit_object ? sit_object->getRenderRotation() : LLQuaternion::DEFAULT;
		av_rot = av_rot * obj_rot;
		LLVector3 at_axis = LLVector3::x_axis;
		at_axis = at_axis * av_rot;
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis);
		gAgentCamera.setThirdPersonHeadOffset(LLVector3(0.f, 0.f, 1.f));
		gAgentCamera.setSitCamera(LLUUID::null);
	}
}

//-----------------------------------------------------------------------------
// findAvatarFromAttachment()
//-----------------------------------------------------------------------------
// static 
LLVOAvatar* LLVOAvatar::findAvatarFromAttachment( LLViewerObject* obj )
{
	if( obj->isAttachment() )
	{
		do
		{
			obj = (LLViewerObject*) obj->getParent();
		}
		while( obj && !obj->isAvatar() );

		if( obj && !obj->isDead() )
		{
			return (LLVOAvatar*)obj;
		}
	}
	return NULL;
}

S32 LLVOAvatar::getAttachmentCount()
{
	S32 count = mAttachmentPoints.size();
	return count;
}

bool LLVOAvatar::isWearingWearableType(LLWearableType::EType type) const
{
	if (mIsDummy) return true;

	if (isSelf())
	{
		return LLAvatarAppearance::isWearingWearableType(type);
	}

	switch(type)
	{
		case LLWearableType::WT_SHAPE:
		case LLWearableType::WT_SKIN:
		case LLWearableType::WT_HAIR:
		case LLWearableType::WT_EYES:
			return true;  // everyone has all bodyparts
		default:
			break; // Do nothing
	}

	for (LLAvatarAppearanceDictionary::Textures::const_iterator tex_iter = LLAvatarAppearance::getDictionary()->getTextures().begin();
		 tex_iter != LLAvatarAppearance::getDictionary()->getTextures().end();
		 ++tex_iter)
	{
		const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = tex_iter->second;
		if (texture_dict->mWearableType == type)
		{
			// Thus, you must check to see if the corresponding baked texture is defined.
			// NOTE: this is a poor substitute if you actually want to know about individual pieces of clothing
			// this works for detecting a skirt (most important), but is ineffective at any piece of clothing that
			// gets baked into a texture that always exists (upper or lower).
			if (texture_dict->mIsUsedByBakedTexture)
			{
				const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
				return isTextureDefined(LLAvatarAppearance::getDictionary()->getBakedTexture(baked_index)->mTextureIndex);
			}
			return false;
		}
	}
	return false;
}

LLViewerObject *	LLVOAvatar::findAttachmentByID( const LLUUID & target_id ) const
{
	for(attachment_map_t::const_iterator attachment_points_iter = mAttachmentPoints.begin();
		attachment_points_iter != gAgentAvatarp->mAttachmentPoints.end();
		++attachment_points_iter)
	{
		LLViewerJointAttachment* attachment = attachment_points_iter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			LLViewerObject *attached_object = attachment_iter->get();
			if (attached_object &&
				attached_object->getID() == target_id)
			{
				return attached_object;
			}
		}
	}

	return NULL;
}

// virtual
void LLVOAvatar::invalidateComposite( LLTexLayerSet* layerset)
{
}

void LLVOAvatar::invalidateAll()
{
}

// virtual
void LLVOAvatar::onGlobalColorChanged(const LLTexGlobalColor* global_color)
{
	if (global_color == mTexSkinColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet);
		invalidateComposite( mBakedTextureDatas[BAKED_UPPER].mTexLayerSet);
		invalidateComposite( mBakedTextureDatas[BAKED_LOWER].mTexLayerSet);
	}
	else if (global_color == mTexHairColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet);
		invalidateComposite( mBakedTextureDatas[BAKED_HAIR].mTexLayerSet);
		
		// ! BACKWARDS COMPATIBILITY !
		// Fix for dealing with avatars from viewers that don't bake hair.
		if (!isTextureDefined(mBakedTextureDatas[BAKED_HAIR].mTextureIndex))
		{
			LLColor4 color = mTexHairColor->getColor();
			avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.begin();
			avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.end();
			for (; iter != end; ++iter)
			{
				LLAvatarJointMesh* mesh = (*iter);
				if (mesh)
			{
					mesh->setColor( color );
				}
			}
		}
	} 
	else if (global_color == mTexEyeColor)
	{
		// LL_INFOS() << "invalidateComposite cause: onGlobalColorChanged( eyecolor )" << LL_ENDL; 
		invalidateComposite( mBakedTextureDatas[BAKED_EYES].mTexLayerSet);
	}
	updateMeshTextures();
}

// virtual
// Do rigged mesh attachments display with this av?
bool LLVOAvatar::shouldRenderRigged() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

	if (getOverallAppearance() == AOA_NORMAL)
	{
		return true;
	}
	// TBD - render for AOA_JELLYDOLL?
	return false;
}

// FIXME: We have an mVisible member, set in updateVisibility(), but this
// function doesn't return it! isVisible() and mVisible are used
// different places for different purposes. mVisible seems to be more
// related to whether the actual avatar mesh is shown, and isVisible()
// to whether anything about the avatar is displayed in the scene.
// Maybe better naming could make this clearer?
bool LLVOAvatar::isVisible() const
{
	return mDrawable.notNull()
		&& (!mOrphaned || isSelf())
		&& (mDrawable->isVisible() || mIsDummy);
}

// Determine if we have enough avatar data to render
bool LLVOAvatar::getIsCloud() const
{
	if (mIsDummy)
	{
		return false;
	}

	return (   ((const_cast<LLVOAvatar*>(this))->visualParamWeightsAreDefault())// Do we have a shape?
			|| (   !isTextureDefined(TEX_LOWER_BAKED)
				|| !isTextureDefined(TEX_UPPER_BAKED)
				|| !isTextureDefined(TEX_HEAD_BAKED)
				)
			);
}

void LLVOAvatar::updateRezzedStatusTimers(S32 rez_status)
{
	// State machine for rezzed status. Statuses are -1 on startup, 0
	// = cloud, 1 = gray, 2 = downloading, 3 = full.
	// Purpose is to collect time data for each it takes avatar to reach
	// various loading landmarks: gray, textured (partial), textured fully.

	if (rez_status != mLastRezzedStatus)
	{
		LL_DEBUGS("Avatar") << avString() << "rez state change: " << mLastRezzedStatus << " -> " << rez_status << LL_ENDL;

		if (mLastRezzedStatus == -1 && rez_status != -1)
		{
			// First time initialization, start all timers.
			for (S32 i = 1; i < 4; i++)
			{
				startPhase("load_" + LLVOAvatar::rezStatusToString(i));
				startPhase("first_load_" + LLVOAvatar::rezStatusToString(i));
			}
		}
		if (rez_status < mLastRezzedStatus)
		{
			// load level has decreased. start phase timers for higher load levels.
			for (S32 i = rez_status+1; i <= mLastRezzedStatus; i++)
			{
				startPhase("load_" + LLVOAvatar::rezStatusToString(i));
			}
		}
		else if (rez_status > mLastRezzedStatus)
		{
			// load level has increased. stop phase timers for lower and equal load levels.
			for (S32 i = llmax(mLastRezzedStatus+1,1); i <= rez_status; i++)
			{
				stopPhase("load_" + LLVOAvatar::rezStatusToString(i));
				stopPhase("first_load_" + LLVOAvatar::rezStatusToString(i), false);
			}
			if (rez_status == 3)
			{
				// "fully loaded", mark any pending appearance change complete.
				selfStopPhase("update_appearance_from_cof");
				selfStopPhase("wear_inventory_category", false);
				selfStopPhase("process_initial_wearables_update", false);

                updateVisualComplexity();
			}
		}
		mLastRezzedStatus = rez_status;
	}
}

void LLVOAvatar::clearPhases()
{
	getPhases().clearPhases();
}

void LLVOAvatar::startPhase(const std::string& phase_name)
{
	F32 elapsed = 0.0;
	bool completed = false;
	bool found = getPhases().getPhaseValues(phase_name, elapsed, completed);
	//LL_DEBUGS("Avatar") << avString() << " phase state " << phase_name
	//					<< " found " << found << " elapsed " << elapsed << " completed " << completed << LL_ENDL;
	if (found)
	{
		if (!completed)
		{
			LL_DEBUGS("Avatar") << avString() << "no-op, start when started already for " << phase_name << LL_ENDL;
			return;
		}
	}
	LL_DEBUGS("Avatar") << "started phase " << phase_name << LL_ENDL;
	getPhases().startPhase(phase_name);
}

void LLVOAvatar::stopPhase(const std::string& phase_name, bool err_check)
{
	F32 elapsed = 0.0;
	bool completed = false;
	if (getPhases().getPhaseValues(phase_name, elapsed, completed))
	{
		if (!completed)
		{
			getPhases().stopPhase(phase_name);
			completed = true;
			logMetricsTimerRecord(phase_name, elapsed, completed);
			LL_DEBUGS("Avatar") << avString() << "stopped phase " << phase_name << " elapsed " << elapsed << LL_ENDL;
		}
		else
		{
			if (err_check)
			{
				LL_DEBUGS("Avatar") << "no-op, stop when stopped already for " << phase_name << LL_ENDL;
			}
		}
	}
	else
	{
		if (err_check)
		{
			LL_DEBUGS("Avatar") << "no-op, stop when not started for " << phase_name << LL_ENDL;
		}
	}
}

void LLVOAvatar::logPendingPhases()
{
	if (!isAgentAvatarValid())
	{
		return;
	}
	
	for (LLViewerStats::phase_map_t::iterator it = getPhases().begin();
		 it != getPhases().end();
		 ++it)
	{
		const std::string& phase_name = it->first;
		F32 elapsed;
		bool completed;
		if (getPhases().getPhaseValues(phase_name, elapsed, completed))
		{
			if (!completed)
			{
				logMetricsTimerRecord(phase_name, elapsed, completed);
			}
		}
	}
}

//static
void LLVOAvatar::logPendingPhasesAllAvatars()
{
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if( inst->isDead() )
		{
			continue;
		}
		inst->logPendingPhases();
	}
}

void LLVOAvatar::logMetricsTimerRecord(const std::string& phase_name, F32 elapsed, bool completed)
{
	if (!isAgentAvatarValid())
	{
		return;
	}
	
	LLSD record;
	record["timer_name"] = phase_name;
	record["avatar_id"] = getID();
	record["elapsed"] = elapsed;
	record["completed"] = completed;
	U32 grid_x(0), grid_y(0);
	if (getRegion() && LLWorld::instance().isRegionListed(getRegion()))
	{
		record["central_bake_version"] = LLSD::Integer(getRegion()->getCentralBakeVersion());
		grid_from_region_handle(getRegion()->getHandle(), &grid_x, &grid_y);
	}
	record["grid_x"] = LLSD::Integer(grid_x);
	record["grid_y"] = LLSD::Integer(grid_y);
	record["is_using_server_bakes"] = true;
	record["is_self"] = isSelf();
		
	if (isAgentAvatarValid())
	{
		gAgentAvatarp->addMetricsTimerRecord(record);
	}
}

// call periodically to keep isFullyLoaded up to date.
// returns true if the value has changed.
bool LLVOAvatar::updateIsFullyLoaded()
{
	S32 rez_status = getRezzedStatus();
	bool loading = getIsCloud();
	if (mFirstFullyVisible && !mIsControlAvatar)
	{
        loading = ((rez_status < 2)
                   // Wait at least 60s for unfinished textures to finish on first load,
                   // don't wait forever, it might fail. Even if it will eventually load by
                   // itself and update mLoadedCallbackTextures (or fail and clean the list),
                   // avatars are more time-sensitive than textures and can't wait that long.
                   || (mLoadedCallbackTextures < mCallbackTextureList.size() && mLastTexCallbackAddedTime.getElapsedTimeF32() < MAX_TEXTURE_WAIT_TIME_SEC)
                   || !mPendingAttachment.empty()
                   || (rez_status < 3 && !isFullyBaked())
                   || hasPendingAttachedMeshes()
                  );
	}
	updateRezzedStatusTimers(rez_status);
	updateRuthTimer(loading);
	return processFullyLoadedChange(loading);
}

void LLVOAvatar::updateRuthTimer(bool loading)
{
	if (isSelf() || !loading) 
	{
		return;
	}

	if (mPreviousFullyLoaded)
	{
		mRuthTimer.reset();
		debugAvatarRezTime("AvatarRezCloudNotification","became cloud");
	}
	
	const F32 LOADING_TIMEOUT__SECONDS = 120.f;
	if (mRuthTimer.getElapsedTimeF32() > LOADING_TIMEOUT__SECONDS)
	{
		LL_DEBUGS("Avatar") << avString()
				<< "Ruth Timer timeout: Missing texture data for '" << getFullname() << "' "
				<< "( Params loaded : " << !visualParamWeightsAreDefault() << " ) "
				<< "( Lower : " << isTextureDefined(TEX_LOWER_BAKED) << " ) "
				<< "( Upper : " << isTextureDefined(TEX_UPPER_BAKED) << " ) "
				<< "( Head : " << isTextureDefined(TEX_HEAD_BAKED) << " )."
				<< LL_ENDL;
		
		LLAvatarPropertiesProcessor::getInstance()->sendAvatarTexturesRequest(getID());
		mRuthTimer.reset();
	}
}

bool LLVOAvatar::processFullyLoadedChange(bool loading)
{
	// We wait a little bit before giving the 'all clear', to let things to
	// settle down (models to snap into place, textures to get first packets).
    // And if viewer isn't aware of some parts yet, this gives them a chance
    // to arrive.
	const F32 LOADED_DELAY = 1.f;

    if (loading)
    {
        mFullyLoadedTimer.reset();
    }

	if (mFirstFullyVisible)
	{
        if (!isSelf() && loading)
        {
                // Note that textures can causes 60s delay on thier own
                // so this delay might end up on top of textures' delay
                mFirstUseDelaySeconds = llclamp(
                    mFirstAppearanceMessageTimer.getElapsedTimeF32(),
                    FIRST_APPEARANCE_CLOUD_MIN_DELAY,
                    FIRST_APPEARANCE_CLOUD_MAX_DELAY);

                if (shouldImpostor())
                {
                    // Impostors are less of a priority,
                    // let them stay cloud longer
                    mFirstUseDelaySeconds *= 1.25;
                }
        }
		mFullyLoaded = (mFullyLoadedTimer.getElapsedTimeF32() > mFirstUseDelaySeconds);
	}
	else
	{
		mFullyLoaded = (mFullyLoadedTimer.getElapsedTimeF32() > LOADED_DELAY);
	}

	if (!mPreviousFullyLoaded && !loading && mFullyLoaded)
	{
		debugAvatarRezTime("AvatarRezNotification","fully loaded");
	}

	// did our loading state "change" from last call?
	// FIXME runway - why are we updating every 30 calls even if nothing has changed?
	// This causes updateLOD() to run every 30 frames, among other things.
	const S32 UPDATE_RATE = 30;
	bool changed =
		((mFullyLoaded != mPreviousFullyLoaded) ||         // if the value is different from the previous call
		 (!mFullyLoadedInitialized) ||                     // if we've never been called before
		 (mFullyLoadedFrameCounter % UPDATE_RATE == 0));   // every now and then issue a change
	bool fully_loaded_changed = (mFullyLoaded != mPreviousFullyLoaded);

	mPreviousFullyLoaded = mFullyLoaded;
	mFullyLoadedInitialized = true;
	mFullyLoadedFrameCounter++;

    if (changed && isSelf())
    {
        // to know about outfit switching
        LLAvatarRenderNotifier::getInstance()->updateNotificationState();
    }

	if (fully_loaded_changed && !isSelf() && mFullyLoaded && isImpostor())
	{
		// Fix for jellydoll initially invisible
		mNeedsImpostorUpdate = true;
		mLastImpostorUpdateReason = 6;
	}	
	return changed;
}

bool LLVOAvatar::isFullyLoaded() const
{
	return (mRenderUnloadedAvatar || mFullyLoaded);
}

bool LLVOAvatar::isTooComplex() const
{
	bool too_complex;
    static LLCachedControl<S32> compelxity_render_mode(gSavedSettings, "RenderAvatarComplexityMode");
	bool render_friend =  (LLAvatarTracker::instance().isBuddy(getID()) && compelxity_render_mode > AV_RENDER_LIMIT_BY_COMPLEXITY);

	if (isSelf() || render_friend || mVisuallyMuteSetting == AV_ALWAYS_RENDER)
	{
		too_complex = false;
	}
    else if (compelxity_render_mode == AV_RENDER_ONLY_SHOW_FRIENDS && !mIsControlAvatar)
    {
        too_complex = true;
    }
	else
	{
		// Determine if visually muted or not
		static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0U);
		static LLCachedControl<F32> max_attachment_area(gSavedSettings, "RenderAutoMuteSurfaceAreaLimit", 1000.0f);
		// If the user has chosen unlimited max complexity, we also disregard max attachment area
        // so that unlimited will completely disable the overly complex impostor rendering
        // yes, this leaves them vulnerable to griefing objects... their choice
        too_complex = (   max_render_cost > 0
                          && (mVisualComplexity > max_render_cost
                           || (max_attachment_area > 0.0f && mAttachmentSurfaceArea > max_attachment_area)
                           ));
	}

	return too_complex;
}

bool LLVOAvatar::isTooSlow() const
{
    static LLCachedControl<S32> compelxity_render_mode(gSavedSettings, "RenderAvatarComplexityMode");
    bool render_friend =  (LLAvatarTracker::instance().isBuddy(getID()) && compelxity_render_mode > AV_RENDER_LIMIT_BY_COMPLEXITY);

    if (render_friend || mVisuallyMuteSetting == AV_ALWAYS_RENDER)
    {
        return false;
    }
    else if (compelxity_render_mode == AV_RENDER_ONLY_SHOW_FRIENDS && !mIsControlAvatar)
    {
        return true;
    }
    return mTooSlow;
}

// Udpate Avatar state based on render time
void LLVOAvatar::updateTooSlow()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    static LLCachedControl<S32> compelxity_render_mode(gSavedSettings, "RenderAvatarComplexityMode");
    static LLCachedControl<bool> allowSelfImpostor(gSavedSettings, "AllowSelfImpostor");
    const auto id = getID();

    // mTooSlow - Is the avatar flagged as being slow (includes shadow time)
    // mTooSlowWithoutShadows - Is the avatar flagged as being slow even with shadows removed.
    
    // get max render time in ms
    F32 max_art_ms = (F32) (LLPerfStats::renderAvatarMaxART_ns / 1000000.0);

	bool autotune = LLPerfStats::tunables.userAutoTuneEnabled && !mIsControlAvatar && !isSelf();

	bool ignore_tune = false;
    if (autotune && sAVsIgnoringARTLimit.size() > 0)
    {
        auto it = std::find(sAVsIgnoringARTLimit.begin(), sAVsIgnoringARTLimit.end(), mID);
        if (it != sAVsIgnoringARTLimit.end())
        {
            S32 index = it - sAVsIgnoringARTLimit.begin();
            ignore_tune = (index < (MIN_NONTUNED_AVS - sAvatarsNearby + 1 + LLPerfStats::tunedAvatars));
        }
    }

	bool exceeds_max_ART =
        ((LLPerfStats::renderAvatarMaxART_ns > 0) && 
            (mGPURenderTime >= max_art_ms)); // NOTE: don't use getGPURenderTime accessor here to avoid "isTooSlow" feedback loop

    if (exceeds_max_ART && !ignore_tune)
    {
        mTooSlow = true;
        
        if(!mTooSlowWithoutShadows) // if we were not previously above the full impostor cap
        {
            bool always_render_friends = compelxity_render_mode > AV_RENDER_LIMIT_BY_COMPLEXITY;
            bool render_friend_or_exception =  	(always_render_friends && LLAvatarTracker::instance().isBuddy( id ) ) ||
                ( getVisualMuteSettings() == LLVOAvatar::AV_ALWAYS_RENDER ); 
            if( (!isSelf() || allowSelfImpostor) && !render_friend_or_exception)
            {
                // Note: slow rendering Friends still get their shadows zapped.
                mTooSlowWithoutShadows = (getGPURenderTime()*2.f >= max_art_ms)  // NOTE: assumes shadow rendering doubles render time
                    || (compelxity_render_mode == AV_RENDER_ONLY_SHOW_FRIENDS && !mIsControlAvatar);
            }
        }
    }
    else
    {
        mTooSlow = false;
        mTooSlowWithoutShadows = false;

		if (ignore_tune)
		{
            return;
		}
    }
    if(mTooSlow && !mTuned)
    {
        LLPerfStats::tunedAvatars++; // increment the number of avatars that have been tweaked.
        mTuned = true;
    }
    else if(!mTooSlow && mTuned)
    {
        LLPerfStats::tunedAvatars--;
        mTuned = false;
    }
}

//-----------------------------------------------------------------------------
// findMotion()
//-----------------------------------------------------------------------------
LLMotion* LLVOAvatar::findMotion(const LLUUID& id) const
{
	return mMotionController.findMotion(id);
}

// This is a semi-deprecated debugging tool - meshes will not show as
// colorized if using deferred rendering.
void LLVOAvatar::debugColorizeSubMeshes(U32 i, const LLColor4& color)
{
	if (gSavedSettings.getBOOL("DebugAvatarCompositeBaked"))
	{
		avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
		avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
		for (; iter != end; ++iter)
		{
			LLAvatarJointMesh* mesh = (*iter);
			if (mesh)
			{
				mesh->setColor(color);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// updateMeshVisibility()
// Hide the mesh joints if attachments are using baked textures
//-----------------------------------------------------------------------------
void LLVOAvatar::updateMeshVisibility()
{
	bool bake_flag[BAKED_NUM_INDICES];
	memset(bake_flag, 0, BAKED_NUM_INDICES*sizeof(bool));

	if (getOverallAppearance() == AOA_NORMAL)
	{
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
			 iter != mAttachmentPoints.end();
			 ++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;
			if (attachment)
			{
				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					LLViewerObject *objectp = attachment_iter->get();
					if (objectp)
					{
						for (int face_index = 0; face_index < objectp->getNumTEs(); face_index++)
						{
							LLTextureEntry* tex_entry = objectp->getTE(face_index);
							bake_flag[BAKED_HEAD] |= (tex_entry->getID() == IMG_USE_BAKED_HEAD);
							bake_flag[BAKED_EYES] |= (tex_entry->getID() == IMG_USE_BAKED_EYES);
							bake_flag[BAKED_HAIR] |= (tex_entry->getID() == IMG_USE_BAKED_HAIR);
							bake_flag[BAKED_LOWER] |= (tex_entry->getID() == IMG_USE_BAKED_LOWER);
							bake_flag[BAKED_UPPER] |= (tex_entry->getID() == IMG_USE_BAKED_UPPER);
							bake_flag[BAKED_SKIRT] |= (tex_entry->getID() == IMG_USE_BAKED_SKIRT);
							bake_flag[BAKED_LEFT_ARM] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTARM);
							bake_flag[BAKED_LEFT_LEG] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTLEG);
							bake_flag[BAKED_AUX1] |= (tex_entry->getID() == IMG_USE_BAKED_AUX1);
							bake_flag[BAKED_AUX2] |= (tex_entry->getID() == IMG_USE_BAKED_AUX2);
							bake_flag[BAKED_AUX3] |= (tex_entry->getID() == IMG_USE_BAKED_AUX3);
						}
					}

					LLViewerObject::const_child_list_t& child_list = objectp->getChildren();
					for (LLViewerObject::child_list_t::const_iterator iter1 = child_list.begin();
						 iter1 != child_list.end(); ++iter1)
					{
						LLViewerObject* objectchild = *iter1;
						if (objectchild)
						{
							for (int face_index = 0; face_index < objectchild->getNumTEs(); face_index++)
							{
								LLTextureEntry* tex_entry = objectchild->getTE(face_index);
								bake_flag[BAKED_HEAD] |= (tex_entry->getID() == IMG_USE_BAKED_HEAD);
								bake_flag[BAKED_EYES] |= (tex_entry->getID() == IMG_USE_BAKED_EYES);
								bake_flag[BAKED_HAIR] |= (tex_entry->getID() == IMG_USE_BAKED_HAIR);
								bake_flag[BAKED_LOWER] |= (tex_entry->getID() == IMG_USE_BAKED_LOWER);
								bake_flag[BAKED_UPPER] |= (tex_entry->getID() == IMG_USE_BAKED_UPPER);
								bake_flag[BAKED_SKIRT] |= (tex_entry->getID() == IMG_USE_BAKED_SKIRT);
								bake_flag[BAKED_LEFT_ARM] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTARM);
								bake_flag[BAKED_LEFT_LEG] |= (tex_entry->getID() == IMG_USE_BAKED_LEFTLEG);
								bake_flag[BAKED_AUX1] |= (tex_entry->getID() == IMG_USE_BAKED_AUX1);
								bake_flag[BAKED_AUX2] |= (tex_entry->getID() == IMG_USE_BAKED_AUX2);
								bake_flag[BAKED_AUX3] |= (tex_entry->getID() == IMG_USE_BAKED_AUX3);
							}
						}
					}
				}
			}
		}
	}

	//LL_INFOS() << "head " << bake_flag[BAKED_HEAD] << "eyes " << bake_flag[BAKED_EYES] << "hair " << bake_flag[BAKED_HAIR] << "lower " << bake_flag[BAKED_LOWER] << "upper " << bake_flag[BAKED_UPPER] << "skirt " << bake_flag[BAKED_SKIRT] << LL_ENDL;

	for (S32 i = 0; i < mMeshLOD.size(); i++)
	{
		LLAvatarJoint* joint = mMeshLOD[i];
		if (i == MESH_ID_HAIR)
		{
			joint->setVisible(!bake_flag[BAKED_HAIR], true);
		}
		else if (i == MESH_ID_HEAD)
		{
			joint->setVisible(!bake_flag[BAKED_HEAD], true);
		}
		else if (i == MESH_ID_SKIRT)
		{
			joint->setVisible(!bake_flag[BAKED_SKIRT], true);
		}
		else if (i == MESH_ID_UPPER_BODY)
		{
			joint->setVisible(!bake_flag[BAKED_UPPER], true);
		}
		else if (i == MESH_ID_LOWER_BODY)
		{
			joint->setVisible(!bake_flag[BAKED_LOWER], true);
		}
		else if (i == MESH_ID_EYEBALL_LEFT)
		{
			joint->setVisible(!bake_flag[BAKED_EYES], true);
		}
		else if (i == MESH_ID_EYEBALL_RIGHT)
		{
			joint->setVisible(!bake_flag[BAKED_EYES], true);
		}
		else if (i == MESH_ID_EYELASH)
		{
			joint->setVisible(!bake_flag[BAKED_HEAD], true);
		}
	}
}

//-----------------------------------------------------------------------------
// updateMeshTextures()
// Uses the current TE values to set the meshes' and layersets' textures.
//-----------------------------------------------------------------------------
// virtual
void LLVOAvatar::updateMeshTextures()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR
	static S32 update_counter = 0;
	mBakedTextureDebugText.clear();
	
	// if user has never specified a texture, assign the default
	for (U32 i=0; i < getNumTEs(); i++)
	{
		const LLViewerTexture* te_image = getImage(i, 0);
		if(!te_image || te_image->getID().isNull() || (te_image->getID() == IMG_DEFAULT))
		{
			// IMG_DEFAULT_AVATAR = a special texture that's never rendered.
			const LLUUID& image_id = (i == TEX_HAIR ? IMG_DEFAULT : IMG_DEFAULT_AVATAR);
			setImage(i, LLViewerTextureManager::getFetchedTexture(image_id), 0); 
		}
	}

	const bool other_culled = !isSelf() && mCulled;
	LLLoadedCallbackEntry::source_callback_list_t* src_callback_list = NULL ;
	bool paused = false;
	if(!isSelf())
	{
		src_callback_list = &mCallbackTextureList ;
		paused = !isVisible();
	}

	std::vector<bool> is_layer_baked;
	is_layer_baked.resize(mBakedTextureDatas.size(), false);

	std::vector<bool> use_lkg_baked_layer; // lkg = "last known good"
	use_lkg_baked_layer.resize(mBakedTextureDatas.size(), false);

	mBakedTextureDebugText += llformat("%06d\n",update_counter++);
	mBakedTextureDebugText += "indx layerset linvld ltda ilb ulkg ltid\n";
	for (U32 i=0; i < mBakedTextureDatas.size(); i++)
	{
		is_layer_baked[i] = isTextureDefined(mBakedTextureDatas[i].mTextureIndex);
		LLViewerTexLayerSet* layerset = NULL;
		bool layerset_invalid = false;
		if (!other_culled)
		{
			// When an avatar is changing clothes and not in Appearance mode,
			// use the last-known good baked texture until it finishes the first
			// render of the new layerset.
			layerset = getTexLayerSet(i);
			layerset_invalid = layerset && ( !layerset->getViewerComposite()->isInitialized()
											 || !layerset->isLocalTextureDataAvailable() );
			use_lkg_baked_layer[i] = (!is_layer_baked[i] 
									  && (mBakedTextureDatas[i].mLastTextureID != IMG_DEFAULT_AVATAR) 
									  && layerset_invalid);
			if (use_lkg_baked_layer[i])
			{
				layerset->setUpdatesEnabled(true);
			}
		}
		else
		{
			use_lkg_baked_layer[i] = (!is_layer_baked[i] 
									  && mBakedTextureDatas[i].mLastTextureID != IMG_DEFAULT_AVATAR);
		}

		std::string last_id_string;
		if (mBakedTextureDatas[i].mLastTextureID == IMG_DEFAULT_AVATAR)
			last_id_string = "A";
		else if (mBakedTextureDatas[i].mLastTextureID == IMG_DEFAULT)
			last_id_string = "D";
		else if (mBakedTextureDatas[i].mLastTextureID == IMG_INVISIBLE)
			last_id_string = "I";
		else
			last_id_string = "*";
		bool is_ltda = layerset
			&& layerset->getViewerComposite()->isInitialized()
			&& layerset->isLocalTextureDataAvailable();
		mBakedTextureDebugText += llformat("%4d   %4s     %4d %4d %4d %4d %4s\n",
										   i,
										   (layerset?"*":"0"),
										   layerset_invalid,
										   is_ltda,
										   is_layer_baked[i],
										   use_lkg_baked_layer[i],
										   last_id_string.c_str());
	}

	for (U32 i=0; i < mBakedTextureDatas.size(); i++)
	{
		debugColorizeSubMeshes(i, LLColor4::white);

		LLViewerTexLayerSet* layerset = getTexLayerSet(i);
		if (use_lkg_baked_layer[i] && !isUsingLocalAppearance() )
		{
			// use last known good layer (no new one)
			LLViewerFetchedTexture* baked_img = LLViewerTextureManager::getFetchedTexture(mBakedTextureDatas[i].mLastTextureID);
			mBakedTextureDatas[i].mIsUsed = true;

			debugColorizeSubMeshes(i,LLColor4::red);
	
			avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
			avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
			for (; iter != end; ++iter)
			{
				LLAvatarJointMesh* mesh = (*iter);
				if (mesh)
				{
					mesh->setTexture( baked_img );
				}
			}
		}
		else if (!isUsingLocalAppearance() && is_layer_baked[i])
		{
			// use new layer
			LLViewerFetchedTexture* baked_img =
				LLViewerTextureManager::staticCastToFetchedTexture(
					getImage( mBakedTextureDatas[i].mTextureIndex, 0 ), true) ;
			if( baked_img->getID() == mBakedTextureDatas[i].mLastTextureID )
			{
				// Even though the file may not be finished loading,
				// we'll consider it loaded and use it (rather than
				// doing compositing).
				useBakedTexture( baked_img->getID() );
                                mLoadedCallbacksPaused |= !isVisible();
                                checkTextureLoading();
			}
			else
			{
				mBakedTextureDatas[i].mIsLoaded = false;
				if ( (baked_img->getID() != IMG_INVISIBLE) &&
					 ((i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER)) )
				{			
					baked_img->setLoadedCallback(onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, true, true, new LLTextureMaskData( mID ), 
						src_callback_list, paused);
				}
				baked_img->setLoadedCallback(onBakedTextureLoaded, SWITCH_TO_BAKED_DISCARD, false, false, new LLUUID( mID ), 
					src_callback_list, paused );
				if (baked_img->getDiscardLevel() < 0 && !paused)
				{
					// mLoadedCallbackTextures will be updated by checkTextureLoading() below
					mLastTexCallbackAddedTime.reset();
				}

				// this could add paused texture callbacks
				mLoadedCallbacksPaused |= paused; 
				checkTextureLoading();
			}
		}
		else if (layerset && isUsingLocalAppearance())
		{
			debugColorizeSubMeshes(i,LLColor4::yellow );

			layerset->createComposite();
			layerset->setUpdatesEnabled( true );
			mBakedTextureDatas[i].mIsUsed = false;

			avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
			avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
			for (; iter != end; ++iter)
			{
				LLAvatarJointMesh* mesh = (*iter);
				if (mesh)
				{
					mesh->setLayerSet( layerset );
				}
			}
		}
		else
		{
			debugColorizeSubMeshes(i,LLColor4::blue);
		}
	}

	// set texture and color of hair manually if we are not using a baked image.
	// This can happen while loading hair for yourself, or for clients that did not
	// bake a hair texture. Still needed for yourself after 1.22 is depricated.
	if (!is_layer_baked[BAKED_HAIR])
	{
		const LLColor4 color = mTexHairColor ? mTexHairColor->getColor() : LLColor4(1,1,1,1);
		LLViewerTexture* hair_img = getImage( TEX_HAIR, 0 );
		avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.begin();
		avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[BAKED_HAIR].mJointMeshes.end();
		for (; iter != end; ++iter)
		{
			LLAvatarJointMesh* mesh = (*iter);
			if (mesh)
			{
				mesh->setColor( color );
				mesh->setTexture( hair_img );
			}
		}
	} 
	
	
	for (LLAvatarAppearanceDictionary::BakedTextures::const_iterator baked_iter =
			 LLAvatarAppearance::getDictionary()->getBakedTextures().begin();
		 baked_iter != LLAvatarAppearance::getDictionary()->getBakedTextures().end();
		 ++baked_iter)
	{
		const EBakedTextureIndex baked_index = baked_iter->first;
		const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = baked_iter->second;
		
		for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
			 local_tex_iter != baked_dict->mLocalTextures.end();
			 ++local_tex_iter)
		{
			const ETextureIndex texture_index = *local_tex_iter;
			const bool is_baked_ready = (is_layer_baked[baked_index] && mBakedTextureDatas[baked_index].mIsLoaded) || other_culled;
			if (isSelf())
			{
				setBakedReady(texture_index, is_baked_ready);
			}
		}
	}

	// removeMissingBakedTextures() will call back into this rountine if something is removed, and can blow up the stack
	static bool call_remove_missing = true;	
	if (call_remove_missing)
	{
		call_remove_missing = false;
		removeMissingBakedTextures();	// May call back into this function if anything is removed
		call_remove_missing = true;
	}

	//refresh bakes on any attached objects
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin();
		iter != mAttachmentPoints.end();
		++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;

		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			attachment_iter != attachment->mAttachedObjects.end();
			++attachment_iter)
		{
			LLViewerObject* attached_object = attachment_iter->get();
			if (attached_object && !attached_object->isDead())
			{
				attached_object->refreshBakeTexture();

				LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					iter != child_list.end(); ++iter)
				{
					LLViewerObject* objectp = *iter;
					if (objectp && !objectp->isDead())
					{
						objectp->refreshBakeTexture();
					}
				}
			}
		}
	}

	

}

// virtual
//-----------------------------------------------------------------------------
// setLocalTexture()
//-----------------------------------------------------------------------------
void LLVOAvatar::setLocalTexture( ETextureIndex type, LLViewerTexture* in_tex, bool baked_version_ready, U32 index )
{
	// invalid for anyone but self
	llassert(0);
}

//virtual 
void LLVOAvatar::setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, bool baked_version_exists, U32 index)
{
	// invalid for anyone but self
	llassert(0);
}

void LLVOAvatar::addChat(const LLChat& chat)
{
	std::deque<LLChat>::iterator chat_iter;

	mChats.push_back(chat);

	S32 chat_length = 0;
	for( chat_iter = mChats.begin(); chat_iter != mChats.end(); ++chat_iter)
	{
		chat_length += chat_iter->mText.size();
	}

	// remove any excess chat
	chat_iter = mChats.begin();
	while ((chat_length > MAX_BUBBLE_CHAT_LENGTH || mChats.size() > MAX_BUBBLE_CHAT_UTTERANCES) && chat_iter != mChats.end())
	{
		chat_length -= chat_iter->mText.size();
		mChats.pop_front();
		chat_iter = mChats.begin();
	}

	mChatTimer.reset();
}

void LLVOAvatar::clearChat()
{
	mChats.clear();
}


void LLVOAvatar::applyMorphMask(const U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index)
{
	if (index >= BAKED_NUM_INDICES)
	{
		LL_WARNS() << "invalid baked texture index passed to applyMorphMask" << LL_ENDL;
		return;
	}

	for (morph_list_t::const_iterator iter = mBakedTextureDatas[index].mMaskedMorphs.begin();
		 iter != mBakedTextureDatas[index].mMaskedMorphs.end(); ++iter)
	{
		const LLMaskedMorph* maskedMorph = (*iter);
		LLPolyMorphTarget* morph_target = dynamic_cast<LLPolyMorphTarget*>(maskedMorph->mMorphTarget);
		if (morph_target)
		{
			morph_target->applyMask(tex_data, width, height, num_components, maskedMorph->mInvert);
		}
	}
}

// returns true if morph masks are present and not valid for a given baked texture, false otherwise
bool LLVOAvatar::morphMaskNeedsUpdate(LLAvatarAppearanceDefines::EBakedTextureIndex index)
{
	if (index >= BAKED_NUM_INDICES)
	{
		return false;
	}

	if (!mBakedTextureDatas[index].mMaskedMorphs.empty())
	{
		if (isSelf())
		{
			LLViewerTexLayerSet *layer_set = getTexLayerSet(index);
			if (layer_set)
			{
				return !layer_set->isMorphValid();
			}
		}
		else
		{
			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// releaseComponentTextures()
// release any component texture UUIDs for which we have a baked texture
// ! BACKWARDS COMPATIBILITY !
// This is only called for non-self avatars, it can be taken out once component
// textures aren't communicated by non-self avatars.
//-----------------------------------------------------------------------------
void LLVOAvatar::releaseComponentTextures()
{
	// ! BACKWARDS COMPATIBILITY !
	// Detect if the baked hair texture actually wasn't sent, and if so set to default
	if (isTextureDefined(TEX_HAIR_BAKED) && getImage(TEX_HAIR_BAKED,0)->getID() == getImage(TEX_SKIRT_BAKED,0)->getID())
	{
		if (getImage(TEX_HAIR_BAKED,0)->getID() != IMG_INVISIBLE)
		{
			// Regression case of messaging system. Expected 21 textures, received 20. last texture is not valid so set to default
			setTETexture(TEX_HAIR_BAKED, IMG_DEFAULT_AVATAR);
		}
	}

	for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
	{
		const LLAvatarAppearanceDictionary::BakedEntry * bakedDicEntry = LLAvatarAppearance::getDictionary()->getBakedTexture((EBakedTextureIndex)baked_index);
		// skip if this is a skirt and av is not wearing one, or if we don't have a baked texture UUID
		if (!isTextureDefined(bakedDicEntry->mTextureIndex)
			&& ( (baked_index != BAKED_SKIRT) || isWearingWearableType(LLWearableType::WT_SKIRT) ))
		{
			continue;
		}

		for (U8 texture = 0; texture < bakedDicEntry->mLocalTextures.size(); texture++)
		{
			const U8 te = (ETextureIndex)bakedDicEntry->mLocalTextures[texture];
			setTETexture(te, IMG_DEFAULT_AVATAR);
		}
	}
}

void LLVOAvatar::dumpAvatarTEs( const std::string& context ) const
{	
	LL_DEBUGS("Avatar") << avString() << (isSelf() ? "Self: " : "Other: ") << context << LL_ENDL;
	for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearance::getDictionary()->getTextures().begin();
		 iter != LLAvatarAppearance::getDictionary()->getTextures().end();
		 ++iter)
	{
		const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
		// TODO: MULTI-WEARABLE: handle multiple textures for self
		const LLViewerTexture* te_image = getImage(iter->first,0);
		if( !te_image )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": null ptr" << LL_ENDL;
		}
		else if( te_image->getID().isNull() )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": null UUID" << LL_ENDL;
		}
		else if( te_image->getID() == IMG_DEFAULT )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": IMG_DEFAULT" << LL_ENDL;
		}
		else if( te_image->getID() == IMG_DEFAULT_AVATAR )
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": IMG_DEFAULT_AVATAR" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Avatar") << avString() << "       " << texture_dict->mName << ": " << te_image->getID() << LL_ENDL;
		}
	}
}

//-----------------------------------------------------------------------------
// clampAttachmentPositions()
//-----------------------------------------------------------------------------
void LLVOAvatar::clampAttachmentPositions()
{
	if (isDead())
	{
		return;
	}
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment)
		{
			attachment->clampObjectPosition();
		}
	}
}

bool LLVOAvatar::hasHUDAttachment() const
{
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment() && attachment->getNumObjects() > 0)
		{
			return true;
		}
	}
	return false;
}

LLBBox LLVOAvatar::getHUDBBox() const
{
	LLBBox bbox;
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment())
		{
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				const LLViewerObject* attached_object = attachment_iter->get();
				if (attached_object == NULL)
				{
					LL_WARNS() << "HUD attached object is NULL!" << LL_ENDL;
					continue;
				}
				// initialize bounding box to contain identity orientation and center point for attached object
				bbox.addPointLocal(attached_object->getPosition());
				// add rotated bounding box for attached object
				bbox.addBBoxAgent(attached_object->getBoundingBoxAgent());
				LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); 
					 ++iter)
				{
					const LLViewerObject* child_objectp = *iter;
					bbox.addBBoxAgent(child_objectp->getBoundingBoxAgent());
				}
			}
		}
	}

	return bbox;
}

//-----------------------------------------------------------------------------
// onFirstTEMessageReceived()
//-----------------------------------------------------------------------------
void LLVOAvatar::onFirstTEMessageReceived()
{
	LL_DEBUGS("Avatar") << avString() << LL_ENDL;
	if( !mFirstTEMessageReceived )
	{
		mFirstTEMessageReceived = true;

		LLLoadedCallbackEntry::source_callback_list_t* src_callback_list = NULL ;
		bool paused = false ;
		if(!isSelf())
		{
			src_callback_list = &mCallbackTextureList ;
			paused = !isVisible();
		}

		for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
		{
			const bool layer_baked = isTextureDefined(mBakedTextureDatas[i].mTextureIndex);

			// Use any baked textures that we have even if they haven't downloaded yet.
			// (That is, don't do a transition from unbaked to baked.)
			if (layer_baked)
			{
				LLViewerFetchedTexture* image = LLViewerTextureManager::staticCastToFetchedTexture(getImage( mBakedTextureDatas[i].mTextureIndex, 0 ), true) ;
				mBakedTextureDatas[i].mLastTextureID = image->getID();
				// If we have more than one texture for the other baked layers, we'll want to call this for them too.
				if ( (image->getID() != IMG_INVISIBLE) && ((i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER)) )
				{
					image->setLoadedCallback( onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, true, true, new LLTextureMaskData( mID ), 
						src_callback_list, paused);
				}
				LL_DEBUGS("Avatar") << avString() << "layer_baked, setting onInitialBakedTextureLoaded as callback" << LL_ENDL;
				image->setLoadedCallback( onInitialBakedTextureLoaded, MAX_DISCARD_LEVEL, false, false, new LLUUID( mID ), 
					src_callback_list, paused );
				if (image->getDiscardLevel() < 0 && !paused)
				{
					mLastTexCallbackAddedTime.reset();
				}
                               // this could add paused texture callbacks
                               mLoadedCallbacksPaused |= paused; 
			}
		}

        mMeshTexturesDirty = true;
		gPipeline.markGLRebuild(this);

        mFirstAppearanceMessageTimer.reset();
        mFullyLoadedTimer.reset();
	}
}

//-----------------------------------------------------------------------------
// bool visualParamWeightsAreDefault()
//-----------------------------------------------------------------------------
bool LLVOAvatar::visualParamWeightsAreDefault()
{
	bool rtn = true;

	bool is_wearing_skirt = isWearingWearableType(LLWearableType::WT_SKIRT);
	for (LLVisualParam *param = getFirstVisualParam(); 
	     param;
	     param = getNextVisualParam())
	{
		if (param->isTweakable())
		{
			LLViewerVisualParam* vparam = dynamic_cast<LLViewerVisualParam*>(param);
			llassert(vparam);
			bool is_skirt_param = vparam &&
				LLWearableType::WT_SKIRT == vparam->getWearableType();
			if (param->getWeight() != param->getDefaultWeight() &&
			    // we have to not care whether skirt weights are default, if we're not actually wearing a skirt
			    (is_wearing_skirt || !is_skirt_param))
			{
				//LL_INFOS() << "param '" << param->getName() << "'=" << param->getWeight() << " which differs from default=" << param->getDefaultWeight() << LL_ENDL;
				rtn = false;
				break;
			}
		}
	}

	//LL_INFOS() << "params are default ? " << int(rtn) << LL_ENDL;

	return rtn;
}

void dump_visual_param(apr_file_t* file, LLVisualParam* viewer_param, F32 value)
{
	std::string type_string = "unknown";
	if (dynamic_cast<LLTexLayerParamAlpha*>(viewer_param))
		type_string = "param_alpha";
	if (dynamic_cast<LLTexLayerParamColor*>(viewer_param))
		type_string = "param_color";
	if (dynamic_cast<LLDriverParam*>(viewer_param))
		type_string = "param_driver";
	if (dynamic_cast<LLPolyMorphTarget*>(viewer_param))
		type_string = "param_morph";
	if (dynamic_cast<LLPolySkeletalDistortion*>(viewer_param))
		type_string = "param_skeleton";
	S32 wtype = -1;
	LLViewerVisualParam *vparam = dynamic_cast<LLViewerVisualParam*>(viewer_param);
	if (vparam)
	{
		wtype = vparam->getWearableType();
	}
	S32 u8_value = F32_to_U8(value,viewer_param->getMinWeight(),viewer_param->getMaxWeight());
	apr_file_printf(file, "\t\t<param id=\"%d\" name=\"%s\" display=\"%s\" value=\"%.3f\" u8=\"%d\" type=\"%s\" wearable=\"%s\" group=\"%d\"/>\n",
					viewer_param->getID(), viewer_param->getName().c_str(), viewer_param->getDisplayName().c_str(), value, u8_value, type_string.c_str(),
					LLWearableType::getInstance()->getTypeName(LLWearableType::EType(wtype)).c_str(),
					viewer_param->getGroup());
	}
	

void LLVOAvatar::dumpAppearanceMsgParams( const std::string& dump_prefix,
	const LLAppearanceMessageContents& contents)
{
	std::string outfilename = get_sequential_numbered_file_name(dump_prefix,".xml");
	const std::vector<F32>& params_for_dump = contents.mParamWeights;
	const LLTEContents& tec = contents.mTEContents;

	LLAPRFile outfile;
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	outfile.open(fullpath, LL_APR_WB );
	apr_file_t* file = outfile.getFileHandle();
	if (!file)
	{
		return;
	}
	else
	{
		LL_DEBUGS("Avatar") << "dumping appearance message to " << fullpath << LL_ENDL;
	}

	apr_file_printf(file, "<header>\n");
	apr_file_printf(file, "\t\t<cof_version %i />\n", contents.mCOFVersion);
	apr_file_printf(file, "\t\t<appearance_version %i />\n", contents.mAppearanceVersion);
	apr_file_printf(file, "</header>\n");

	apr_file_printf(file, "\n<params>\n");
	LLVisualParam* param = getFirstVisualParam();
	for (S32 i = 0; i < params_for_dump.size(); i++)
	{
		while( param && ((param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) && 
						 (param->getGroup() != VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE)) ) // should not be any of group VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
		{
			param = getNextVisualParam();
		}
		LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
		F32 value = params_for_dump[i];
		dump_visual_param(file, viewer_param, value);
		param = getNextVisualParam();
	}
	apr_file_printf(file, "</params>\n");

	apr_file_printf(file, "\n<textures>\n");
	for (U32 i = 0; i < tec.face_count; i++)
	{
		std::string uuid_str;
		((LLUUID*)tec.image_data)[i].toString(uuid_str);
		apr_file_printf( file, "\t\t<texture te=\"%i\" uuid=\"%s\"/>\n", i, uuid_str.c_str());
	}
	apr_file_printf(file, "</textures>\n");
}

void LLVOAvatar::parseAppearanceMessage(LLMessageSystem* mesgsys, LLAppearanceMessageContents& contents)
{
	parseTEMessage(mesgsys, _PREHASH_ObjectData, -1, contents.mTEContents);

	// Parse the AppearanceData field, if any.
	if (mesgsys->has(_PREHASH_AppearanceData))
	{
		U8 av_u8;
		mesgsys->getU8Fast(_PREHASH_AppearanceData, _PREHASH_AppearanceVersion, av_u8, 0);
		contents.mAppearanceVersion = av_u8;
		//LL_DEBUGS("Avatar") << "appversion set by AppearanceData field: " << contents.mAppearanceVersion << LL_ENDL;
		mesgsys->getS32Fast(_PREHASH_AppearanceData, _PREHASH_CofVersion, contents.mCOFVersion, 0);
		// For future use:
		//mesgsys->getU32Fast(_PREHASH_AppearanceData, _PREHASH_Flags, appearance_flags, 0);
	}

	// Parse the AppearanceHover field, if any.
	contents.mHoverOffsetWasSet = false;
	if (mesgsys->has(_PREHASH_AppearanceHover))
	{
		LLVector3 hover;
		mesgsys->getVector3Fast(_PREHASH_AppearanceHover, _PREHASH_HoverHeight, hover);
		//LL_DEBUGS("Avatar") << avString() << " hover received " << hover.mV[ VX ] << "," << hover.mV[ VY ] << "," << hover.mV[ VZ ] << LL_ENDL;
		contents.mHoverOffset = hover;
		contents.mHoverOffsetWasSet = true;
	}

    // Get attachment info, if sent
    LLUUID attachment_id;
    U8     attach_point;
    S32    attach_count = mesgsys->getNumberOfBlocksFast(_PREHASH_AttachmentBlock);
    LL_DEBUGS("AVAppearanceAttachments") << "Agent " << getID() << " has "
                                         << attach_count << " attachments" << LL_ENDL;

    for (S32 attach_i = 0; attach_i < attach_count; attach_i++)
    {
        mesgsys->getUUIDFast(_PREHASH_AttachmentBlock, _PREHASH_ID, attachment_id, attach_i);
        mesgsys->getU8Fast(_PREHASH_AttachmentBlock, _PREHASH_AttachmentPoint, attach_point, attach_i);
        LL_DEBUGS("AVAppearanceAttachments") << "AV " << getID() << " has attachment " << attach_i << " "
            << (attachment_id.isNull() ? "pending" : attachment_id.asString())
            << " on point " << (S32)attach_point << LL_ENDL;
        // To do - store and use this information as needed
	}

	// Parse visual params, if any.
	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_VisualParam);
    static LLCachedControl<bool> block_some_avatars(gSavedSettings, "BlockSomeAvatarAppearanceVisualParams");
	bool drop_visual_params_debug = block_some_avatars && (ll_rand(2) == 0); // pretend that ~12% of AvatarAppearance messages arrived without a VisualParam block, for testing
	if( num_blocks > 1 && !drop_visual_params_debug)
	{
		//LL_DEBUGS("Avatar") << avString() << " handle visual params, num_blocks " << num_blocks << LL_ENDL;
		
		LLVisualParam* param = getFirstVisualParam();
		llassert(param); // if this ever fires, we should do the same as when num_blocks<=1
		if (!param)
		{
			LL_WARNS() << "No visual params!" << LL_ENDL;
		}
		else
		{
			for( S32 i = 0; i < num_blocks; i++ )
			{
				while( param && ((param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) && 
								 (param->getGroup() != VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE)) ) // should not be any of group VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
				{
					param = getNextVisualParam();
				}
						
				if( !param )
				{
					// more visual params supplied than expected - just process what we know about
					break;
				}

				U8 value;
				mesgsys->getU8Fast(_PREHASH_VisualParam, _PREHASH_ParamValue, value, i);
				F32 newWeight = U8_to_F32(value, param->getMinWeight(), param->getMaxWeight());
				contents.mParamWeights.push_back(newWeight);
				contents.mParams.push_back(param);

				param = getNextVisualParam();
			}
		}

		const S32 expected_tweakable_count = getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TWEAKABLE) +
											 getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE); // don't worry about VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
		if (num_blocks != expected_tweakable_count)
		{
			LL_DEBUGS("Avatar") << "Number of params in AvatarAppearance msg (" << num_blocks << ") does not match number of tweakable params in avatar xml file (" << expected_tweakable_count << ").  Processing what we can.  object: " << getID() << LL_ENDL;
		}
	}
	else
	{
		if (drop_visual_params_debug)
		{
			LL_INFOS() << "Debug-faked lack of parameters on AvatarAppearance for object: "  << getID() << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("Avatar") << "AvatarAppearance msg received without any parameters, object: " << getID() << LL_ENDL;
		}
	}

	LLVisualParam* appearance_version_param = getVisualParam(11000);
	if (appearance_version_param)
	{
		std::vector<LLVisualParam*>::iterator it = std::find(contents.mParams.begin(), contents.mParams.end(),appearance_version_param);
		if (it != contents.mParams.end())
		{
			S32 index = it - contents.mParams.begin();
			contents.mParamAppearanceVersion = ll_round(contents.mParamWeights[index]);
			//LL_DEBUGS("Avatar") << "appversion req by appearance_version param: " << contents.mParamAppearanceVersion << LL_ENDL;
		}
	}
}

bool resolve_appearance_version(const LLAppearanceMessageContents& contents, S32& appearance_version)
{
	appearance_version = -1;
	
	if ((contents.mAppearanceVersion) >= 0 &&
		(contents.mParamAppearanceVersion >= 0) &&
		(contents.mAppearanceVersion != contents.mParamAppearanceVersion))
	{
		LL_WARNS() << "inconsistent appearance_version settings - field: " <<
			contents.mAppearanceVersion << ", param: " <<  contents.mParamAppearanceVersion << LL_ENDL;
		return false;
	}
	if (contents.mParamAppearanceVersion >= 0) // use visual param if available.
	{
		appearance_version = contents.mParamAppearanceVersion;
	}
	else if (contents.mAppearanceVersion > 0)
	{
		appearance_version = contents.mAppearanceVersion;
	}
	else // still not set, go with 1.
	{
		appearance_version = 1;
	}
	//LL_DEBUGS("Avatar") << "appearance version info - field " << contents.mAppearanceVersion
	//					<< " param: " << contents.mParamAppearanceVersion
	//					<< " final: " << appearance_version << LL_ENDL;
	return true;
}

//-----------------------------------------------------------------------------
// processAvatarAppearance()
//-----------------------------------------------------------------------------
void LLVOAvatar::processAvatarAppearance( LLMessageSystem* mesgsys )
{
	LL_DEBUGS("Avatar") << "starts" << LL_ENDL;

    static LLCachedControl<bool> enable_verbose_dumps(gSavedSettings, "DebugAvatarAppearanceMessage");
    static LLCachedControl<bool> block_avatar_appearance_messages(gSavedSettings, "BlockAvatarAppearanceMessages");

	std::string dump_prefix = getFullname() + "_" + (isSelf()?"s":"o") + "_";
	if (block_avatar_appearance_messages)
	{
		LL_WARNS() << "Blocking AvatarAppearance message" << LL_ENDL;
		return;
	}

	mLastAppearanceMessageTimer.reset();

	LLPointer<LLAppearanceMessageContents> contents(new LLAppearanceMessageContents);
	parseAppearanceMessage(mesgsys, *contents);
	if (enable_verbose_dumps)
	{
		dumpAppearanceMsgParams(dump_prefix + "appearance_msg", *contents);
	}

	S32 appearance_version;
	if (!resolve_appearance_version(*contents, appearance_version))
	{
		LL_WARNS() << "bad appearance version info, discarding" << LL_ENDL;
		return;
	}
	llassert(appearance_version > 0);
	if (appearance_version > 1)
	{
		LL_WARNS() << "unsupported appearance version " << appearance_version << ", discarding appearance message" << LL_ENDL;
		return;
	}

    S32 thisAppearanceVersion(contents->mCOFVersion);
    if (isSelf())
    {   // In the past this was considered to be the canonical COF version, 
        // that is no longer the case.  The canonical version is maintained 
        // by the AIS code and should match the COF version there. Even so,
        // we must prevent rolling this one backwards backwards or processing 
        // stale versions.

        S32 aisCOFVersion(LLAppearanceMgr::instance().getCOFVersion());

        LL_DEBUGS("Avatar") << "handling self appearance message #" << thisAppearanceVersion <<
            " (highest seen #" << mLastUpdateReceivedCOFVersion <<
            ") (AISCOF=#" << aisCOFVersion << ")" << LL_ENDL;

        if (mLastUpdateReceivedCOFVersion >= thisAppearanceVersion)
        {
            LL_WARNS("Avatar") << "Stale appearance received #" << thisAppearanceVersion <<
                " attempt to roll back from #" << mLastUpdateReceivedCOFVersion <<
                "... dropping." << LL_ENDL;
            return;
        }
        if (isEditingAppearance())
        {
            LL_DEBUGS("Avatar") << "Editing appearance.  Dropping appearance update." << LL_ENDL;
            return;
        }

    }

	// SUNSHINE CLEANUP - is this case OK now?
	S32 num_params = contents->mParamWeights.size();
	if (num_params <= 1)
	{
		// In this case, we have no reliable basis for knowing
		// appearance version, which may cause us to look for baked
		// textures in the wrong place and flag them as missing
		// assets.
		LL_DEBUGS("Avatar") << "ignoring appearance message due to lack of params" << LL_ENDL;
		return;
	}

	// No backsies zone - if we get here, the message should be valid and usable, will be processed.
	// Note:
	// RequestAgentUpdateAppearanceResponder::onRequestRequested()
	// assumes that cof version is only updated with server-bake
	// appearance messages.
    if (isSelf())
    {
        LL_INFOS("Avatar") << "Processing appearance message version " << thisAppearanceVersion << LL_ENDL;
    }
    else
    {
        LL_INFOS("Avatar") << "Processing appearance message for " << getID() << ", version " << thisAppearanceVersion << LL_ENDL;
    }

    // Note:
    // locally the COF is maintained via LLInventoryModel::accountForUpdate
    // which is called from various places.  This should match the simhost's 
    // idea of what the COF version is.  AIS however maintains its own version
    // of the COF that should be considered canonical. 
    mLastUpdateReceivedCOFVersion = thisAppearanceVersion;

    mLastProcessedAppearance = contents;

    bool slam_params = false;
	applyParsedAppearanceMessage(*contents, slam_params);
	if (getOverallAppearance() != AOA_NORMAL)
	{
		resetSkeleton(false);
	}
}

void LLVOAvatar::applyParsedAppearanceMessage(LLAppearanceMessageContents& contents, bool slam_params)
{
	S32 num_params = contents.mParamWeights.size();
	ESex old_sex = getSex();

    if (applyParsedTEMessage(contents.mTEContents) > 0 && isChanged(TEXTURE))
    {
        updateVisualComplexity();
    }

	// prevent the overwriting of valid baked textures with invalid baked textures
	for (U8 baked_index = 0; baked_index < mBakedTextureDatas.size(); baked_index++)
	{
		if (!isTextureDefined(mBakedTextureDatas[baked_index].mTextureIndex) 
			&& mBakedTextureDatas[baked_index].mLastTextureID != IMG_DEFAULT
			&& baked_index != BAKED_SKIRT && baked_index != BAKED_LEFT_ARM && baked_index != BAKED_LEFT_LEG && baked_index != BAKED_AUX1 && baked_index != BAKED_AUX2 && baked_index != BAKED_AUX3)
		{
			LL_DEBUGS("Avatar") << avString() << " baked_index " << (S32) baked_index << " using mLastTextureID " << mBakedTextureDatas[baked_index].mLastTextureID << LL_ENDL;
			setTEImage(mBakedTextureDatas[baked_index].mTextureIndex, 
				LLViewerTextureManager::getFetchedTexture(mBakedTextureDatas[baked_index].mLastTextureID, FTT_DEFAULT, true, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
		}
		else
		{
			LL_DEBUGS("Avatar") << avString() << " baked_index " << (S32) baked_index << " using texture id "
								<< getTE(mBakedTextureDatas[baked_index].mTextureIndex)->getID() << LL_ENDL;
		}
	}

	// runway - was
	// if (!is_first_appearance_message )
	// which means it would be called on second appearance message - probably wrong.
	bool is_first_appearance_message = !mFirstAppearanceMessageReceived;
	mFirstAppearanceMessageReceived = true;

	//LL_DEBUGS("Avatar") << avString() << "processAvatarAppearance start " << mID
    //                    << " first? " << is_first_appearance_message << " self? " << isSelf() << LL_ENDL;

	if (is_first_appearance_message )
	{
		onFirstTEMessageReceived();
	}

	setCompositeUpdatesEnabled( false );
	gPipeline.markGLRebuild(this);

	// Apply visual params
	if( num_params > 1)
	{
		//LL_DEBUGS("Avatar") << avString() << " handle visual params, num_params " << num_params << LL_ENDL;
		bool params_changed = false;
		bool interp_params = false;
		S32 params_changed_count = 0;
		
		for( S32 i = 0; i < num_params; i++ )
		{
			LLVisualParam* param = contents.mParams[i];
			F32 newWeight = contents.mParamWeights[i];

			if (slam_params || is_first_appearance_message || (param->getWeight() != newWeight))
			{
				params_changed = true;
				params_changed_count++;

				if(is_first_appearance_message || slam_params)
				{
					//LL_DEBUGS("Avatar") << "param slam " << i << " " << newWeight << LL_ENDL;
					param->setWeight(newWeight);
				}
				else
				{
					interp_params = true;
					param->setAnimationTarget(newWeight);
				}
			}
		}
		const S32 expected_tweakable_count = getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TWEAKABLE) +
											 getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TRANSMIT_NOT_TWEAKABLE); // don't worry about VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
		if (num_params != expected_tweakable_count)
		{
			LL_DEBUGS("Avatar") << "Number of params in AvatarAppearance msg (" << num_params << ") does not match number of tweakable params in avatar xml file (" << expected_tweakable_count << ").  Processing what we can.  object: " << getID() << LL_ENDL;
		}

		LL_DEBUGS("Avatar") << "Changed " << params_changed_count << " params" << LL_ENDL;
		if (params_changed)
		{
			if (interp_params)
			{
				startAppearanceAnimation();
			}
			updateVisualParams();

			ESex new_sex = getSex();
			if( old_sex != new_sex )
			{
				updateSexDependentLayerSets();
			}	
		}

		llassert( getSex() == ((getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE) );
	}
	else
	{
		// AvatarAppearance message arrived without visual params
		LL_DEBUGS("Avatar") << avString() << "no visual params" << LL_ENDL;

		const F32 LOADING_TIMEOUT_SECONDS = 60.f;
		// this isn't really a problem if we already have a non-default shape
		if (visualParamWeightsAreDefault() && mRuthTimer.getElapsedTimeF32() > LOADING_TIMEOUT_SECONDS)
		{
			// re-request appearance, hoping that it comes back with a shape next time
			LL_INFOS() << "Re-requesting AvatarAppearance for object: "  << getID() << LL_ENDL;
			LLAvatarPropertiesProcessor::getInstance()->sendAvatarTexturesRequest(getID());
			mRuthTimer.reset();
		}
		else
		{
			LL_INFOS() << "That's okay, we already have a non-default shape for object: "  << getID() << LL_ENDL;
			// we don't really care.
		}
	}

	if (contents.mHoverOffsetWasSet && !isSelf())
	{
		// Got an update for some other avatar
		// Ignore updates for self, because we have a more authoritative value in the preferences.
		setHoverOffset(contents.mHoverOffset);
		LL_DEBUGS("Avatar") << avString() << "setting hover to " << contents.mHoverOffset[2] << LL_ENDL;
	}

	if (!contents.mHoverOffsetWasSet && !isSelf())
	{
		// If we don't get a value at all, we are presumably in a
		// region that does not support hover height.
		LL_WARNS() << avString() << "zeroing hover because not defined in appearance message" << LL_ENDL;
		setHoverOffset(LLVector3(0.0, 0.0, 0.0));
	}

	setCompositeUpdatesEnabled( true );

	// If all of the avatars are completely baked, release the global image caches to conserve memory.
	LLVOAvatar::cullAvatarsByPixelArea();

	if (isSelf())
	{
		mUseLocalAppearance = false;
	}

	updateMeshTextures();
	updateMeshVisibility();

}

LLViewerTexture* LLVOAvatar::getBakedTexture(const U8 te)
{
	if (te < 0 || te >= BAKED_NUM_INDICES)
	{
		return NULL;
	}

	bool is_layer_baked = isTextureDefined(mBakedTextureDatas[te].mTextureIndex);
	
	LLViewerTexLayerSet* layerset = NULL;
	layerset = getTexLayerSet(te);
	

	if (!isEditingAppearance() && is_layer_baked)
	{
		LLViewerFetchedTexture* baked_img = LLViewerTextureManager::staticCastToFetchedTexture(getImage(mBakedTextureDatas[te].mTextureIndex, 0), true);
		return baked_img;
	}
	else if (layerset && isEditingAppearance())
	{
		layerset->createComposite();
		layerset->setUpdatesEnabled(true);

		return layerset->getViewerComposite();
	}

	return NULL;

	
}

const LLVOAvatar::MatrixPaletteCache& LLVOAvatar::updateSkinInfoMatrixPalette(const LLMeshSkinInfo* skin)
{
    U64 hash = skin->mHash;
    MatrixPaletteCache& entry = mMatrixPaletteCache[hash];

    if (entry.mFrame != gFrameCount)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

        entry.mFrame = gFrameCount;

        //build matrix palette
        U32 count = LLSkinningUtil::getMeshJointCount(skin);
        entry.mMatrixPalette.resize(count);
        LLSkinningUtil::initSkinningMatrixPalette(&(entry.mMatrixPalette[0]), count, skin, this);

        const LLMatrix4a* mat = &(entry.mMatrixPalette[0]);

        entry.mGLMp.resize(count * 12);

        F32* mp = &(entry.mGLMp[0]);

        for (U32 i = 0; i < count; ++i)
        {
            F32* m = (F32*)mat[i].mMatrix[0].getF32ptr();

            U32 idx = i * 12;

            mp[idx + 0] = m[0];
            mp[idx + 1] = m[1];
            mp[idx + 2] = m[2];
            mp[idx + 3] = m[12];

            mp[idx + 4] = m[4];
            mp[idx + 5] = m[5];
            mp[idx + 6] = m[6];
            mp[idx + 7] = m[13];

            mp[idx + 8] = m[8];
            mp[idx + 9] = m[9];
            mp[idx + 10] = m[10];
            mp[idx + 11] = m[14];
        }
    }

    return entry;
}

// static
void LLVOAvatar::getAnimLabels( std::vector<std::string>* labels )
{
	S32 i;
	labels->reserve(gUserAnimStatesCount);
	for( i = 0; i < gUserAnimStatesCount; i++ )
	{
		labels->push_back( LLAnimStateLabels::getStateLabel( gUserAnimStates[i].mName ) );
	}

	// Special case to trigger away (AFK) state
	labels->push_back( "Away From Keyboard" );
}

// static 
void LLVOAvatar::getAnimNames( std::vector<std::string>* names )
{
	S32 i;

	names->reserve(gUserAnimStatesCount);
	for( i = 0; i < gUserAnimStatesCount; i++ )
	{
		names->push_back( std::string(gUserAnimStates[i].mName) );
	}

	// Special case to trigger away (AFK) state
	names->push_back( "enter_away_from_keyboard_state" );
}

// static
void LLVOAvatar::onBakedTextureMasksLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata )
{
	if (!userdata) return;

	//LL_INFOS() << "onBakedTextureMasksLoaded: " << src_vi->getID() << LL_ENDL;
	const LLUUID id = src_vi->getID();
 
	LLTextureMaskData* maskData = (LLTextureMaskData*) userdata;
	LLVOAvatar* self = (LLVOAvatar*) gObjectList.findObject( maskData->mAvatarID );

	// if discard level is 2 less than last discard level we processed, or we hit 0,
	// then generate morph masks
	if(self && success && (discard_level < maskData->mLastDiscardLevel - 2 || discard_level == 0))
	{
		if(aux_src && aux_src->getComponents() == 1)
		{
			LLImageDataSharedLock lock(aux_src);

			if (!aux_src->getData())
			{
				LL_ERRS() << "No auxiliary source (morph mask) data for image id " << id << LL_ENDL;
				return;
			}

			U32 gl_name;
			LLImageGL::generateTextures(1, &gl_name );
			stop_glerror();

			gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, gl_name);
			stop_glerror();

			LLImageGL::setManualImage(
				GL_TEXTURE_2D, 0, GL_ALPHA8, 
				aux_src->getWidth(), aux_src->getHeight(),
				GL_ALPHA, GL_UNSIGNED_BYTE, aux_src->getData());
			stop_glerror();

			gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);

			/* if( id == head_baked->getID() )
			     if (self->mBakedTextureDatas[BAKED_HEAD].mTexLayerSet)
				     //LL_INFOS() << "onBakedTextureMasksLoaded for head " << id << " discard = " << discard_level << LL_ENDL;
					 self->mBakedTextureDatas[BAKED_HEAD].mTexLayerSet->applyMorphMask(aux_src->getData(), aux_src->getWidth(), aux_src->getHeight(), 1);
					 maskData->mLastDiscardLevel = discard_level; */
			bool found_texture_id = false;
			for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearance::getDictionary()->getTextures().begin();
				 iter != LLAvatarAppearance::getDictionary()->getTextures().end();
				 ++iter)
			{

				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
				if (texture_dict->mIsUsedByBakedTexture)
				{
					const ETextureIndex texture_index = iter->first;
					const LLViewerTexture *baked_img = self->getImage(texture_index, 0);
					if (baked_img && id == baked_img->getID())
					{
						const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
						self->applyMorphMask(aux_src->getData(), aux_src->getWidth(), aux_src->getHeight(), 1, baked_index);
						maskData->mLastDiscardLevel = discard_level;
						if (self->mBakedTextureDatas[baked_index].mMaskTexName)
						{
							LLImageGL::deleteTextures(1, &(self->mBakedTextureDatas[baked_index].mMaskTexName));
						}
						self->mBakedTextureDatas[baked_index].mMaskTexName = gl_name;
						found_texture_id = true;
						break;
					}
				}
			}
			if (!found_texture_id)
			{
				LL_INFOS() << "unexpected image id: " << id << LL_ENDL;
			}
			self->dirtyMesh();
		}
		else
		{
            // this can happen when someone uses an old baked texture possibly provided by 
            // viewer-side baked texture caching
			LL_WARNS() << "Masks loaded callback but NO aux source, id " << id << LL_ENDL;
		}
	}

	if (final || !success)
	{
		delete maskData;
	}
}

// static
void LLVOAvatar::onInitialBakedTextureLoaded( bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, bool final, void* userdata )
{
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = (LLVOAvatar *)gObjectList.findObject(*avatar_idp);

	if (selfp)
	{
		//LL_DEBUGS("Avatar") << selfp->avString() << "discard_level " << discard_level << " success " << success << " final " << final << LL_ENDL;
	}

	if (!success && selfp)
	{
		selfp->removeMissingBakedTextures();
	}
	if (final || !success )
	{
		delete avatar_idp;
	}
}

// Static
void LLVOAvatar::onBakedTextureLoaded(bool success,
									  LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src,
									  S32 discard_level, bool final, void* userdata)
{
	//LL_DEBUGS("Avatar") << "onBakedTextureLoaded: " << src_vi->getID() << LL_ENDL;

	LLUUID id = src_vi->getID();
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = (LLVOAvatar *)gObjectList.findObject(*avatar_idp);
	if (selfp)
	{	
		//LL_DEBUGS("Avatar") << selfp->avString() << "discard_level " << discard_level << " success " << success << " final " << final << " id " << src_vi->getID() << LL_ENDL;
	}

	if (selfp && !success)
	{
		selfp->removeMissingBakedTextures();
	}

	if( final || !success )
	{
		delete avatar_idp;
	}

	if( selfp && success && final )
	{
		selfp->useBakedTexture( id );
	}
}


// Called when baked texture is loaded and also when we start up with a baked texture
void LLVOAvatar::useBakedTexture( const LLUUID& id )
{
	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		LLViewerTexture* image_baked = getImage( mBakedTextureDatas[i].mTextureIndex, 0 );
		if (id == image_baked->getID())
		{
			//LL_DEBUGS("Avatar") << avString() << " i " << i << " id " << id << LL_ENDL;
			mBakedTextureDatas[i].mIsLoaded = true;
			mBakedTextureDatas[i].mLastTextureID = id;
			mBakedTextureDatas[i].mIsUsed = true;

			if (isUsingLocalAppearance())
			{
				LL_INFOS() << "not changing to baked texture while isUsingLocalAppearance" << LL_ENDL;
			}
			else
			{
				debugColorizeSubMeshes(i,LLColor4::green);

				avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
				avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
				for (; iter != end; ++iter)
				{
					LLAvatarJointMesh* mesh = (*iter);
					if (mesh)
					{
						mesh->setTexture( image_baked );
					}
				}
			}
			
			const LLAvatarAppearanceDictionary::BakedEntry *baked_dict =
				LLAvatarAppearance::getDictionary()->getBakedTexture((EBakedTextureIndex)i);
			for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
				 local_tex_iter != baked_dict->mLocalTextures.end();
				 ++local_tex_iter)
			{
				if (isSelf()) setBakedReady(*local_tex_iter, true);
			}

			// ! BACKWARDS COMPATIBILITY !
			// Workaround for viewing avatars from old viewers that haven't baked hair textures.
			// This is paired with similar code in updateMeshTextures that sets hair mesh color.
			if (i == BAKED_HAIR)
			{
				avatar_joint_mesh_list_t::iterator iter = mBakedTextureDatas[i].mJointMeshes.begin();
				avatar_joint_mesh_list_t::iterator end  = mBakedTextureDatas[i].mJointMeshes.end();
				for (; iter != end; ++iter)
				{
					LLAvatarJointMesh* mesh = (*iter);
					if (mesh)
					{
						mesh->setColor( LLColor4::white );
					}
				}
			}
		}
	}

	dirtyMesh();
}

std::string get_sequential_numbered_file_name(const std::string& prefix,
											  const std::string& suffix)
{
	typedef std::map<std::string,S32> file_num_type;
	static  file_num_type file_nums;
	file_num_type::iterator it = file_nums.find(prefix);
	S32 num = 0;
	if (it != file_nums.end())
	{
		num = it->second;
	}
	file_nums[prefix] = num+1;
	std::string outfilename = prefix + " " + llformat("%04d",num) + ".xml";
	std::replace(outfilename.begin(),outfilename.end(),' ','_');
	return outfilename;
}

void dump_sequential_xml(const std::string outprefix, const LLSD& content)
{
	std::string outfilename = get_sequential_numbered_file_name(outprefix,".xml");
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	llofstream ofs(fullpath.c_str(), std::ios_base::out);
	ofs << LLSDOStreamer<LLSDXMLFormatter>(content, LLSDFormatter::OPTIONS_PRETTY);
	LL_DEBUGS("Avatar") << "results saved to: " << fullpath << LL_ENDL;
}

void LLVOAvatar::getSortedJointNames(S32 joint_type, std::vector<std::string>& result) const
{
    result.clear();
    if (joint_type==0)
    {
        avatar_joint_list_t::const_iterator iter = mSkeleton.begin();
        avatar_joint_list_t::const_iterator end  = mSkeleton.end();
		for (; iter != end; ++iter)
		{
			LLJoint* pJoint = (*iter);
            result.push_back(pJoint->getName());
        }
    }
    else if (joint_type==1)
    {
        for (S32 i = 0; i < mNumCollisionVolumes; i++)
        {
            LLAvatarJointCollisionVolume* pJoint = &mCollisionVolumes[i];
            result.push_back(pJoint->getName());
        }
    }
    else if (joint_type==2)
    {
		for (LLVOAvatar::attachment_map_t::const_iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end(); ++iter)
		{
			LLViewerJointAttachment* pJoint = iter->second;
			if (!pJoint) continue;
            result.push_back(pJoint->getName());
        }
    }
    std::sort(result.begin(), result.end());
}

void LLVOAvatar::dumpArchetypeXML(const std::string& prefix, bool group_by_wearables )
{
	std::string outprefix(prefix);
	if (outprefix.empty())
	{
		outprefix = getFullname() + (isSelf()?"_s":"_o");
	}
	if (outprefix.empty())
	{
		outprefix = std::string("new_archetype");
	}
	std::string outfilename = get_sequential_numbered_file_name(outprefix,".xml");
	
	LLAPRFile outfile;
    LLWearableType *wr_inst = LLWearableType::getInstance();
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	if (APR_SUCCESS == outfile.open(fullpath, LL_APR_WB ))
	{
		apr_file_t* file = outfile.getFileHandle();
		LL_INFOS() << "xmlfile write handle obtained : " << fullpath << LL_ENDL;

		apr_file_printf( file, "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n" );
		apr_file_printf( file, "<linden_genepool version=\"1.0\">\n" );
		apr_file_printf( file, "\n\t<archetype name=\"???\">\n" );

		bool agent_is_godlike = gAgent.isGodlikeWithoutAdminMenuFakery();

		if (group_by_wearables)
		{
			for (S32 type = LLWearableType::WT_SHAPE; type < LLWearableType::WT_COUNT; type++)
			{
				const std::string& wearable_name = wr_inst->getTypeName((LLWearableType::EType)type);
				apr_file_printf( file, "\n\t\t<!-- wearable: %s -->\n", wearable_name.c_str() );

				for (LLVisualParam* param = getFirstVisualParam(); param; param = getNextVisualParam())
				{
					LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
					if( (viewer_param->getWearableType() == type) && 
					   (viewer_param->isTweakable() ) )
					{
						dump_visual_param(file, viewer_param, viewer_param->getWeight());
					}
				}

				for (U8 te = 0; te < TEX_NUM_INDICES; te++)
				{
					if (LLAvatarAppearance::getDictionary()->getTEWearableType((ETextureIndex)te) == type)
					{
						// MULTIPLE_WEARABLES: extend to multiple wearables?
						LLViewerTexture* te_image = getImage((ETextureIndex)te, 0);
						if( te_image )
						{
							std::string uuid_str = LLUUID().asString();
							if (agent_is_godlike)
							{
								te_image->getID().toString(uuid_str);
							}
							apr_file_printf( file, "\t\t<texture te=\"%i\" uuid=\"%s\"/>\n", te, uuid_str.c_str());
						}
					}
				}
			}
		}
		else 
		{
			// Just dump all params sequentially.
			for (LLVisualParam* param = getFirstVisualParam(); param; param = getNextVisualParam())
			{
				LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
				dump_visual_param(file, viewer_param, viewer_param->getWeight());
			}

			for (U8 te = 0; te < TEX_NUM_INDICES; te++)
			{
				// MULTIPLE_WEARABLES: extend to multiple wearables?
				LLViewerTexture* te_image = getImage((ETextureIndex)te, 0);
				if( te_image )
				{
					std::string uuid_str = LLUUID().asString();
					if (agent_is_godlike)
					{
						te_image->getID().toString(uuid_str);
					}
					apr_file_printf( file, "\t\t<texture te=\"%i\" uuid=\"%s\"/>\n", te, uuid_str.c_str());
				}
			}
		}

        // Root joint
        const LLVector3& pos = mRoot->getPosition();
        const LLVector3& scale = mRoot->getScale();
        apr_file_printf( file, "\t\t<root name=\"%s\" position=\"%f %f %f\" scale=\"%f %f %f\"/>\n", 
                         mRoot->getName().c_str(), pos[0], pos[1], pos[2], scale[0], scale[1], scale[2]);

        // Bones
        std::vector<std::string> bone_names, cv_names, attach_names, all_names;
        getSortedJointNames(0, bone_names);
        getSortedJointNames(1, cv_names);
        getSortedJointNames(2, attach_names);
        all_names.insert(all_names.end(), bone_names.begin(), bone_names.end());
        all_names.insert(all_names.end(), cv_names.begin(), cv_names.end());
        all_names.insert(all_names.end(), attach_names.begin(), attach_names.end());

        for (std::vector<std::string>::iterator name_iter = bone_names.begin();
             name_iter != bone_names.end(); ++name_iter)
        {
            LLJoint *pJoint = getJoint(*name_iter);
			const LLVector3& pos = pJoint->getPosition();
			const LLVector3& scale = pJoint->getScale();
			apr_file_printf( file, "\t\t<bone name=\"%s\" position=\"%f %f %f\" scale=\"%f %f %f\"/>\n", 
							 pJoint->getName().c_str(), pos[0], pos[1], pos[2], scale[0], scale[1], scale[2]);
        }

        // Collision volumes
        for (std::vector<std::string>::iterator name_iter = cv_names.begin();
             name_iter != cv_names.end(); ++name_iter)
        {
            LLJoint *pJoint = getJoint(*name_iter);
			const LLVector3& pos = pJoint->getPosition();
			const LLVector3& scale = pJoint->getScale();
			apr_file_printf( file, "\t\t<collision_volume name=\"%s\" position=\"%f %f %f\" scale=\"%f %f %f\"/>\n", 
							 pJoint->getName().c_str(), pos[0], pos[1], pos[2], scale[0], scale[1], scale[2]);
        }

        // Attachment joints
        for (std::vector<std::string>::iterator name_iter = attach_names.begin();
             name_iter != attach_names.end(); ++name_iter)
        {
            LLJoint *pJoint = getJoint(*name_iter);
			if (!pJoint) continue;
			const LLVector3& pos = pJoint->getPosition();
			const LLVector3& scale = pJoint->getScale();
			apr_file_printf( file, "\t\t<attachment_point name=\"%s\" position=\"%f %f %f\" scale=\"%f %f %f\"/>\n", 
							 pJoint->getName().c_str(), pos[0], pos[1], pos[2], scale[0], scale[1], scale[2]);
        }
        
        // Joint pos overrides
        for (std::vector<std::string>::iterator name_iter = all_names.begin();
             name_iter != all_names.end(); ++name_iter)
        {
            LLJoint *pJoint = getJoint(*name_iter);
		
			LLVector3 pos;
			LLUUID mesh_id;

			if (pJoint && pJoint->hasAttachmentPosOverride(pos,mesh_id))
			{
                S32 num_pos_overrides;
                std::set<LLVector3> distinct_pos_overrides;
                pJoint->getAllAttachmentPosOverrides(num_pos_overrides, distinct_pos_overrides);
				apr_file_printf( file, "\t\t<joint_offset name=\"%s\" position=\"%f %f %f\" mesh_id=\"%s\" count=\"%d\" distinct=\"%d\"/>\n", 
								 pJoint->getName().c_str(), pos[0], pos[1], pos[2], mesh_id.asString().c_str(),
                                 num_pos_overrides, (S32) distinct_pos_overrides.size());
			}
		}
        // Joint scale overrides
        for (std::vector<std::string>::iterator name_iter = all_names.begin();
             name_iter != all_names.end(); ++name_iter)
        {
            LLJoint *pJoint = getJoint(*name_iter);
		
			LLVector3 scale;
			LLUUID mesh_id;

			if (pJoint && pJoint->hasAttachmentScaleOverride(scale,mesh_id))
			{
                S32 num_scale_overrides;
                std::set<LLVector3> distinct_scale_overrides;
                pJoint->getAllAttachmentPosOverrides(num_scale_overrides, distinct_scale_overrides);
				apr_file_printf( file, "\t\t<joint_scale name=\"%s\" scale=\"%f %f %f\" mesh_id=\"%s\" count=\"%d\" distinct=\"%d\"/>\n",
								 pJoint->getName().c_str(), scale[0], scale[1], scale[2], mesh_id.asString().c_str(),
                                 num_scale_overrides, (S32) distinct_scale_overrides.size());
			}
		}
		F32 pelvis_fixup;
		LLUUID mesh_id;
		if (hasPelvisFixup(pelvis_fixup, mesh_id))
		{
			apr_file_printf( file, "\t\t<pelvis_fixup z=\"%f\" mesh_id=\"%s\"/>\n", 
							 pelvis_fixup, mesh_id.asString().c_str());
		}

        LLVector3 rp = getRootJoint()->getWorldPosition();
        LLVector4a rpv;
        rpv.load3(rp.mV);
        
        for (S32 joint_num = 0; joint_num < LL_CHARACTER_MAX_ANIMATED_JOINTS; joint_num++)
        {
            LLJoint *joint = getJoint(joint_num);
            if (joint_num < mJointRiggingInfoTab.size())
            {
                LLJointRiggingInfo& rig_info = mJointRiggingInfoTab[joint_num];
                if (rig_info.isRiggedTo())
                {
                    LLMatrix4a mat;
                    LLVector4a new_extents[2];
                    mat.loadu(joint->getWorldMatrix());
                    matMulBoundBox(mat, rig_info.getRiggedExtents(), new_extents);
                    LLVector4a rrp[2];
                    rrp[0].setSub(new_extents[0],rpv);
                    rrp[1].setSub(new_extents[1],rpv);
                    apr_file_printf( file, "\t\t<joint_rig_info num=\"%d\" name=\"%s\" min=\"%f %f %f\" max=\"%f %f %f\" tmin=\"%f %f %f\" tmax=\"%f %f %f\"/>\n", 
                                     joint_num,
                                     joint->getName().c_str(),
                                     rig_info.getRiggedExtents()[0][0],
                                     rig_info.getRiggedExtents()[0][1],
                                     rig_info.getRiggedExtents()[0][2],
                                     rig_info.getRiggedExtents()[1][0],
                                     rig_info.getRiggedExtents()[1][1],
                                     rig_info.getRiggedExtents()[1][2],
                                     rrp[0][0],
                                     rrp[0][1],
                                     rrp[0][2],
                                     rrp[1][0],
                                     rrp[1][1],
                                     rrp[1][2] );
                }
            }
        }

		bool ultra_verbose = false;
		if (isSelf() && ultra_verbose)
		{
			// show the cloned params inside the wearables as well.
			gAgentAvatarp->dumpWearableInfo(outfile);
		}

		apr_file_printf( file, "\t</archetype>\n" );
		apr_file_printf( file, "\n</linden_genepool>\n" );

		LLSD args;
		args["PATH"] = fullpath;
		LLNotificationsUtil::add("AppearanceToXMLSaved", args);
	}
	else
	{
		LLNotificationsUtil::add("AppearanceToXMLFailed");
	}
	// File will close when handle goes out of scope
}


void LLVOAvatar::setVisibilityRank(U32 rank)
{
	if (mDrawable.isNull() || mDrawable->isDead())
	{
		// do nothing
		return;
	}
	mVisibilityRank = rank;
}

// Assumes LLVOAvatar::sInstances has already been sorted.
S32 LLVOAvatar::getUnbakedPixelAreaRank()
{
	S32 rank = 1;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		if (inst == this)
		{
			return rank;
		}
		else if (!inst->isDead() && !inst->isFullyBaked())
		{
			rank++;
		}
	}

	llassert(0);
	return 0;
}

struct CompareScreenAreaGreater
{
	bool operator()(const LLCharacter* const& lhs, const LLCharacter* const& rhs)
	{
		return lhs->getPixelArea() > rhs->getPixelArea();
	}
};

// static
void LLVOAvatar::cullAvatarsByPixelArea()
{
	std::sort(LLCharacter::sInstances.begin(), LLCharacter::sInstances.end(), CompareScreenAreaGreater());
	
	// Update the avatars that have changed status
	U32 rank = 2; //1 is reserved for self. 
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		bool culled;
		if (inst->isSelf() || inst->isFullyBaked())
		{
			culled = false;
		}
		else 
		{
			culled = true;
		}

		if (inst->mCulled != culled)
		{
			inst->mCulled = culled;
			LL_DEBUGS() << "avatar " << inst->getID() << (culled ? " start culled" : " start not culled" ) << LL_ENDL;
			inst->updateMeshTextures();
		}

		if (inst->isSelf())
		{
			inst->setVisibilityRank(1);
		}
		else if (inst->mDrawable.notNull() && inst->mDrawable->isVisible())
		{
			inst->setVisibilityRank(rank++);
		}
	}

	// runway - this doesn't really detect gray/grey state.
	S32 grey_avatars = 0;
	if (!LLVOAvatar::areAllNearbyInstancesBaked(grey_avatars))
	{
		if (gFrameTimeSeconds != sUnbakedUpdateTime) // only update once per frame
		{
			sUnbakedUpdateTime = gFrameTimeSeconds;
			sUnbakedTime += gFrameIntervalSeconds.value();
		}
		if (grey_avatars > 0)
		{
			if (gFrameTimeSeconds != sGreyUpdateTime) // only update once per frame
			{
				sGreyUpdateTime = gFrameTimeSeconds;
				sGreyTime += gFrameIntervalSeconds.value();
			}
		}
	}
}

void LLVOAvatar::startAppearanceAnimation()
{
	if(!mAppearanceAnimating)
	{
		mAppearanceAnimating = true;
		mAppearanceMorphTimer.reset();
		mLastAppearanceBlendTime = 0.f;
	}
}

// virtual
void LLVOAvatar::removeMissingBakedTextures()
{
}

//virtual
void LLVOAvatar::updateRegion(LLViewerRegion *regionp)
{
	LLViewerObject::updateRegion(regionp);
}

// virtual
std::string LLVOAvatar::getFullname() const
{
	std::string name;

	LLNameValue* first = getNVPair("FirstName"); 
	LLNameValue* last  = getNVPair("LastName"); 
	if (first && last)
	{
		name = LLCacheName::buildFullName( first->getString(), last->getString() );
	}

	return name;
}

LLHost LLVOAvatar::getObjectHost() const
{
	LLViewerRegion* region = getRegion();
	if (region && !isDead())
	{
		return region->getHost();
	}
	else
	{
		return LLHost();
	}
}

bool LLVOAvatar::updateLOD()
{
    if (mDrawable.isNull())
    {
        return false;
    }
    
	if (!LLPipeline::sImpostorRender && isImpostor() && 0 != mDrawable->getNumFaces() && mDrawable->getFace(0)->hasGeometry())
	{
		return true;
	}

	bool res = updateJointLODs();

	LLFace* facep = mDrawable->getFace(0);
	if (!facep || !facep->getVertexBuffer())
	{
		dirtyMesh(2);
	}

	if (mDirtyMesh >= 2 || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY))
	{	//LOD changed or new mesh created, allocate new vertex buffer if needed
		updateMeshData();
		mDirtyMesh = 0;
		mNeedsSkin = true;
		mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
	}
	updateVisibility();

	return res;
}

void LLVOAvatar::updateLODRiggedAttachments()
{
	updateLOD();
	rebuildRiggedAttachments();
}

void showRigInfoTabExtents(LLVOAvatar *avatar, LLJointRiggingInfoTab& tab, S32& count_rigged, S32& count_box)
{
    count_rigged = count_box = 0;
    LLVector4a zero_vec;
    zero_vec.clear();
    for (S32 i=0; i<tab.size(); i++)
    {
        if (tab[i].isRiggedTo())
        {
            count_rigged++;
            LLJoint *joint = avatar->getJoint(i);
            LL_DEBUGS("RigSpam") << "joint " << i << " name " << joint->getName() << " box " 
                                 << tab[i].getRiggedExtents()[0] << ", " << tab[i].getRiggedExtents()[1] << LL_ENDL;
            if ((!tab[i].getRiggedExtents()[0].equals3(zero_vec)) ||
                (!tab[i].getRiggedExtents()[1].equals3(zero_vec)))
            {
                count_box++;
            }
       }
    }
}

void LLVOAvatar::getAssociatedVolumes(std::vector<LLVOVolume*>& volumes)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
	for ( LLVOAvatar::attachment_map_t::iterator iter = mAttachmentPoints.begin(); iter != mAttachmentPoints.end(); ++iter )
	{
		LLViewerJointAttachment* attachment = iter->second;
		LLViewerJointAttachment::attachedobjs_vec_t::iterator attach_end = attachment->mAttachedObjects.end();
		
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attach_iter = attachment->mAttachedObjects.begin();
			 attach_iter != attach_end; ++attach_iter)
		{
			LLViewerObject* attached_object =  attach_iter->get();
            LLVOVolume *volume = dynamic_cast<LLVOVolume*>(attached_object);
            if (volume)
            {
                volumes.push_back(volume);
                if (volume->isAnimatedObject())
                {
                    // For animated object attachment, don't need
                    // the children. Will just get bounding box
                    // from the control avatar.
                    continue;
                }
            }
            LLViewerObject::const_child_list_t& children = attached_object->getChildren();
            for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
                 it != children.end(); ++it)
            {
                LLViewerObject *childp = *it;
                LLVOVolume *volume = dynamic_cast<LLVOVolume*>(childp);
                if (volume)
                {
                    volumes.push_back(volume);
                }
            }
        }
    }

    LLControlAvatar *control_av = dynamic_cast<LLControlAvatar*>(this);
    if (control_av)
    {
        LLVOVolume *volp = control_av->mRootVolp;
        if (volp)
        {
            volumes.push_back(volp);
            LLViewerObject::const_child_list_t& children = volp->getChildren();
            for (LLViewerObject::const_child_list_t::const_iterator it = children.begin();
                 it != children.end(); ++it)
            {
                LLViewerObject *childp = *it;
                LLVOVolume *volume = dynamic_cast<LLVOVolume*>(childp);
                if (volume)
                {
                    volumes.push_back(volume);
                }
            }
        }
    }
}

// virtual
void LLVOAvatar::updateRiggingInfo()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    LL_DEBUGS("RigSpammish") << getFullname() << " updating rig tab" << LL_ENDL;

    std::vector<LLVOVolume*> volumes;

	getAssociatedVolumes(volumes);

	std::map<LLUUID,S32> curr_rigging_info_key;
	{
		// Get current rigging info key
		for (std::vector<LLVOVolume*>::iterator it = volumes.begin(); it != volumes.end(); ++it)
		{
			LLVOVolume *vol = *it;
			if (vol->isMesh() && vol->getVolume())
			{
				const LLUUID& mesh_id = vol->getVolume()->getParams().getSculptID();
				S32 max_lod = llmax(vol->getLOD(), vol->mLastRiggingInfoLOD);
				curr_rigging_info_key[mesh_id] = max_lod;
			}
		}
		
		// Check for key change, which indicates some change in volume composition or LOD.
		if (curr_rigging_info_key == mLastRiggingInfoKey)
		{
			return;
		}
	}

	// Something changed. Update.
	mLastRiggingInfoKey = curr_rigging_info_key;
    mJointRiggingInfoTab.clear();
    for (std::vector<LLVOVolume*>::iterator it = volumes.begin(); it != volumes.end(); ++it)
    {
        LLVOVolume *vol = *it;
        vol->updateRiggingInfo();
        mJointRiggingInfoTab.merge(vol->mJointRiggingInfoTab);
    }

    //LL_INFOS() << "done update rig count is " << countRigInfoTab(mJointRiggingInfoTab) << LL_ENDL;
    LL_DEBUGS("RigSpammish") << getFullname() << " after update rig tab:" << LL_ENDL;
    S32 joint_count, box_count;
    showRigInfoTabExtents(this, mJointRiggingInfoTab, joint_count, box_count);
    LL_DEBUGS("RigSpammish") << "uses " << joint_count << " joints " << " nonzero boxes: " << box_count << LL_ENDL;
}

// virtual
void LLVOAvatar::onActiveOverrideMeshesChanged()
{
    mJointRiggingInfoTab.setNeedsUpdate(true);
}

U32 LLVOAvatar::getPartitionType() const
{ 
	// Avatars merely exist as drawables in the bridge partition
	return mIsControlAvatar ? LLViewerRegion::PARTITION_CONTROL_AV : LLViewerRegion::PARTITION_AVATAR;
}

//static
void LLVOAvatar::updateImpostors()
{
	LLViewerCamera::sCurCameraID = LLViewerCamera::CAMERA_WORLD;

    std::vector<LLCharacter*> instances_copy = LLCharacter::sInstances;
	for (std::vector<LLCharacter*>::iterator iter = instances_copy.begin();
		iter != instances_copy.end(); ++iter)
	{
		LLVOAvatar* avatar = (LLVOAvatar*) *iter;
		if (!avatar->isDead()
			&& avatar->isVisible()
			&& avatar->isImpostor()
			&& avatar->needsImpostorUpdate())
		{
            avatar->calcMutedAVColor();
			gPipeline.generateImpostor(avatar);
		}
	}

	LLCharacter::sAllowInstancesChange = true;
}

// virtual
bool LLVOAvatar::isImpostor()
{
	return isVisuallyMuted() || (sLimitNonImpostors && (mUpdatePeriod > 1));
}

bool LLVOAvatar::shouldImpostor(const F32 rank_factor)
{
	if (isSelf())
	{
		return false;
	}
	if (isVisuallyMuted())
	{
		return true;
	}
	return sLimitNonImpostors && (mVisibilityRank > sMaxNonImpostors * rank_factor);
}

bool LLVOAvatar::needsImpostorUpdate() const
{
	return mNeedsImpostorUpdate;
}

const LLVector3& LLVOAvatar::getImpostorOffset() const
{
	return mImpostorOffset;
}

const LLVector2& LLVOAvatar::getImpostorDim() const
{
	return mImpostorDim;
}

void LLVOAvatar::setImpostorDim(const LLVector2& dim)
{
	mImpostorDim = dim;
}

void LLVOAvatar::cacheImpostorValues()
{
	getImpostorValues(mImpostorExtents, mImpostorAngle, mImpostorDistance);
}

void LLVOAvatar::getImpostorValues(LLVector4a* extents, LLVector3& angle, F32& distance) const
{
	const LLVector4a* ext = mDrawable->getSpatialExtents();
	extents[0] = ext[0];
	extents[1] = ext[1];

	LLVector3 at = LLViewerCamera::getInstance()->getOrigin()-(getRenderPosition()+mImpostorOffset);
	distance = at.normalize();
	F32 da = 1.f - (at*LLViewerCamera::getInstance()->getAtAxis());
	angle.mV[0] = LLViewerCamera::getInstance()->getYaw()*da;
	angle.mV[1] = LLViewerCamera::getInstance()->getPitch()*da;
	angle.mV[2] = da;
}

// static
const U32 LLVOAvatar::NON_IMPOSTORS_MAX_SLIDER = 66; /* Must equal the maximum allowed the RenderAvatarMaxNonImpostors
										   * slider in panel_preferences_graphics1.xml */

// static
void LLVOAvatar::updateImpostorRendering(U32 newMaxNonImpostorsValue)
{
	U32  oldmax = sMaxNonImpostors;
	bool oldflg = sLimitNonImpostors;
	
	if (NON_IMPOSTORS_MAX_SLIDER <= newMaxNonImpostorsValue)
	{
		sMaxNonImpostors = 0;
	}
	else
	{
		sMaxNonImpostors = newMaxNonImpostorsValue;
	}
	// the sLimitNonImpostors flag depends on whether or not sMaxNonImpostors is set to the no-limit value (0)
	sLimitNonImpostors = (0 != sMaxNonImpostors);
    if ( oldflg != sLimitNonImpostors )
    {
        LL_DEBUGS("AvatarRender")
            << "was " << (oldflg ? "use" : "don't use" ) << " impostors (max " << oldmax << "); "
            << "now " << (sLimitNonImpostors ? "use" : "don't use" ) << " impostors (max " << sMaxNonImpostors << "); "
            << LL_ENDL;
    }
}


void LLVOAvatar::idleUpdateRenderComplexity()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    if (isControlAvatar())
    {
        LLControlAvatar *cav = dynamic_cast<LLControlAvatar*>(this);
        bool is_attachment = cav && cav->mRootVolp && cav->mRootVolp->isAttachment(); // For attached animated objects
        if (is_attachment)
        {
            // ARC for animated object attachments is accounted with the avatar they're attached to.
            return;
        }
    }

    // Render Complexity
    calculateUpdateRenderComplexity(); // Update mVisualComplexity if needed	

	bool autotune = LLPerfStats::tunables.userAutoTuneEnabled && !mIsControlAvatar && !isSelf();
    if (autotune && !isDead())
    {
        static LLCachedControl<F32> render_far_clip(gSavedSettings, "RenderFarClip", 64);
        F32 radius = render_far_clip * render_far_clip;

        bool is_nearby = true;
        if ((dist_vec_squared(getPositionGlobal(), gAgent.getPositionGlobal()) > radius) &&
            (dist_vec_squared(getPositionGlobal(), gAgentCamera.getCameraPositionGlobal()) > radius))
        {
            is_nearby = false;
        }

        if (is_nearby && (sAVsIgnoringARTLimit.size() < MIN_NONTUNED_AVS))
        {
            if (std::count(sAVsIgnoringARTLimit.begin(), sAVsIgnoringARTLimit.end(), mID) == 0)
            {
                sAVsIgnoringARTLimit.push_back(mID);
            }
        }
        else if (!is_nearby)
        {
            sAVsIgnoringARTLimit.erase(std::remove(sAVsIgnoringARTLimit.begin(), sAVsIgnoringARTLimit.end(), mID),
                                       sAVsIgnoringARTLimit.end());
        }
        updateNearbyAvatarCount();
    }
}

void LLVOAvatar::updateNearbyAvatarCount()
{
    static LLFrameTimer agent_update_timer;

	if (agent_update_timer.getElapsedTimeF32() > 1.0f)
    {
        S32 avs_nearby = 0;
        static LLCachedControl<F32> render_far_clip(gSavedSettings, "RenderFarClip", 64);
        F32 radius = render_far_clip * render_far_clip;
        std::vector<LLCharacter *>::iterator char_iter = LLCharacter::sInstances.begin();
        while (char_iter != LLCharacter::sInstances.end())
        {
            LLVOAvatar *avatar = dynamic_cast<LLVOAvatar *>(*char_iter);
            if (avatar && !avatar->isDead() && !avatar->isControlAvatar())
            {
                if ((dist_vec_squared(avatar->getPositionGlobal(), gAgent.getPositionGlobal()) > radius) &&
                    (dist_vec_squared(avatar->getPositionGlobal(), gAgentCamera.getCameraPositionGlobal()) > radius))
                {
                    char_iter++;
                    continue;
                }
                avs_nearby++;
            }
            char_iter++;
        }
        sAvatarsNearby = avs_nearby;
        agent_update_timer.reset();
    }
}

void LLVOAvatar::idleUpdateDebugInfo()
{
	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_AVATAR_DRAW_INFO))
	{
		std::string info_line;
		F32 red_level;
		F32 green_level;
		LLColor4 info_color;
		LLFontGL::StyleFlags info_style;
		
		if ( !mText )
		{
			initHudText();
			mText->setFadeDistance(20.0, 5.0); // limit clutter in large crowds
		}
		else
		{
			mText->clearString(); // clear debug text
		}

		/*
		 * NOTE: the logic for whether or not each of the values below
		 * controls muting MUST match that in the isVisuallyMuted and isTooComplex methods.
		 */

		static LLCachedControl<U32> max_render_cost(gSavedSettings, "RenderAvatarMaxComplexity", 0);
		info_line = llformat("%d Complexity", mVisualComplexity);

		if (max_render_cost != 0) // zero means don't care, so don't bother coloring based on this
		{
			green_level = 1.f-llclamp(((F32) mVisualComplexity-(F32)max_render_cost)/(F32)max_render_cost, 0.f, 1.f);
			red_level   = llmin((F32) mVisualComplexity/(F32)max_render_cost, 1.f);
			info_color.set(red_level, green_level, 0.0, 1.0);
			info_style = (  mVisualComplexity > max_render_cost
						  ? LLFontGL::BOLD : LLFontGL::NORMAL );
		}
		else
		{
			info_color.set(LLColor4::grey);
			info_style = LLFontGL::NORMAL;
		}
		mText->addLine(info_line, info_color, info_style);

		// Visual rank
		info_line = llformat("%d rank", mVisibilityRank);
		// Use grey for imposters, white for normal rendering or no impostors
		info_color.set(isImpostor() ? LLColor4::grey : (isControlAvatar() ? LLColor4::yellow : LLColor4::white));
		info_style = LLFontGL::NORMAL;
		mText->addLine(info_line, info_color, info_style);

        // Triangle count
        mText->addLine(std::string("VisTris ") + LLStringOps::getReadableNumber(mAttachmentVisibleTriangleCount), 
                       info_color, info_style);
        mText->addLine(std::string("EstMaxTris ") + LLStringOps::getReadableNumber(mAttachmentEstTriangleCount), 
                       info_color, info_style);

		// Attachment Surface Area
		static LLCachedControl<F32> max_attachment_area(gSavedSettings, "RenderAutoMuteSurfaceAreaLimit", 1000.0f);
		info_line = llformat("%.0f m^2", mAttachmentSurfaceArea);

		if (max_render_cost != 0 && max_attachment_area != 0) // zero means don't care, so don't bother coloring based on this
		{
			green_level = 1.f-llclamp((mAttachmentSurfaceArea-max_attachment_area)/max_attachment_area, 0.f, 1.f);
			red_level   = llmin(mAttachmentSurfaceArea/max_attachment_area, 1.f);
			info_color.set(red_level, green_level, 0.0, 1.0);
			info_style = (  mAttachmentSurfaceArea > max_attachment_area
						  ? LLFontGL::BOLD : LLFontGL::NORMAL );

		}
		else
		{
			info_color.set(LLColor4::grey);
			info_style = LLFontGL::NORMAL;
		}

		mText->addLine(info_line, info_color, info_style);

		updateText(); // corrects position
	}
}

void LLVOAvatar::updateVisualComplexity()
{
	LL_DEBUGS("AvatarRender") << "avatar " << getID() << " appearance changed" << LL_ENDL;
	// Set the cache time to in the past so it's updated ASAP
	mVisualComplexityStale = true;
}


// Account for the complexity of a single top-level object associated
// with an avatar. This will be either an attached object or an animated
// object.
void LLVOAvatar::accountRenderComplexityForObject(
    LLViewerObject *attached_object,
    const F32 max_attachment_complexity,
    LLVOVolume::texture_cost_t& textures,
    U32& cost,
    hud_complexity_list_t& hud_complexity_list,
    object_complexity_list_t& object_complexity_list)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
    if (attached_object && !attached_object->isHUDAttachment())
    {
        mAttachmentVisibleTriangleCount += attached_object->recursiveGetTriangleCount();
        mAttachmentEstTriangleCount += attached_object->recursiveGetEstTrianglesMax();
        mAttachmentSurfaceArea += attached_object->recursiveGetScaledSurfaceArea();

        textures.clear();
        const LLDrawable* drawable = attached_object->mDrawable;
        if (drawable)
        {
            const LLVOVolume* volume = drawable->getVOVolume();
            if (volume)
            {
                F32 attachment_total_cost = 0;
                F32 attachment_volume_cost = 0;
                F32 attachment_texture_cost = 0;
                F32 attachment_children_cost = 0;
                const F32 animated_object_attachment_surcharge = 1000;

                if (volume->isAnimatedObjectFast())
                {
                    attachment_volume_cost += animated_object_attachment_surcharge;
                }
                attachment_volume_cost += volume->getRenderCost(textures);

                const_child_list_t children = volume->getChildren();
                for (const_child_list_t::const_iterator child_iter = children.begin();
                    child_iter != children.end();
                    ++child_iter)
                {
                    LLViewerObject* child_obj = *child_iter;
                    LLVOVolume* child = dynamic_cast<LLVOVolume*>(child_obj);
                    if (child)
                    {
                        attachment_children_cost += child->getRenderCost(textures);
                    }
                }

                for (LLVOVolume::texture_cost_t::iterator volume_texture = textures.begin();
                    volume_texture != textures.end();
                    ++volume_texture)
                {
                    // add the cost of each individual texture in the linkset
                    attachment_texture_cost += LLVOVolume::getTextureCost(*volume_texture);
                }
                attachment_total_cost = attachment_volume_cost + attachment_texture_cost + attachment_children_cost;
                LL_DEBUGS("ARCdetail") << "Attachment costs " << attached_object->getAttachmentItemID()
                    << " total: " << attachment_total_cost
                    << ", volume: " << attachment_volume_cost
                    << ", " << textures.size()
                    << " textures: " << attachment_texture_cost
                    << ", " << volume->numChildren()
                    << " children: " << attachment_children_cost
                    << LL_ENDL;
                // Limit attachment complexity to avoid signed integer flipping of the wearer's ACI
                cost += (U32)llclamp(attachment_total_cost, MIN_ATTACHMENT_COMPLEXITY, max_attachment_complexity);

                if (isSelf())
                {
                    LLObjectComplexity object_complexity;
                    object_complexity.objectName = attached_object->getAttachmentItemName();
                    object_complexity.objectId = attached_object->getAttachmentItemID();
                    object_complexity.objectCost = attachment_total_cost;
                    object_complexity_list.push_back(object_complexity);
                }
            }
        }
    }
    if (isSelf()
        && attached_object
        && attached_object->isHUDAttachment()
        && !attached_object->isTempAttachment()
        && attached_object->mDrawable)
    {
        textures.clear();
        mAttachmentSurfaceArea += attached_object->recursiveGetScaledSurfaceArea();

        const LLVOVolume* volume = attached_object->mDrawable->getVOVolume();
        if (volume)
        {
            bool is_rigged_mesh = volume->isRiggedMeshFast();
            LLHUDComplexity hud_object_complexity;
            hud_object_complexity.objectName = attached_object->getAttachmentItemName();
            hud_object_complexity.objectId = attached_object->getAttachmentItemID();
            std::string joint_name;
            gAgentAvatarp->getAttachedPointName(attached_object->getAttachmentItemID(), joint_name);
            hud_object_complexity.jointName = joint_name;
            // get cost and individual textures
            hud_object_complexity.objectsCost += volume->getRenderCost(textures);
            hud_object_complexity.objectsCount++;

            LLViewerObject::const_child_list_t& child_list = attached_object->getChildren();
            for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
                iter != child_list.end(); ++iter)
            {
                LLViewerObject* childp = *iter;
                const LLVOVolume* chld_volume = dynamic_cast<LLVOVolume*>(childp);
                if (chld_volume)
                {
                    is_rigged_mesh = is_rigged_mesh || chld_volume->isRiggedMeshFast();
                    // get cost and individual textures
                    hud_object_complexity.objectsCost += chld_volume->getRenderCost(textures);
                    hud_object_complexity.objectsCount++;
                }
            }
            if (is_rigged_mesh && !attached_object->mRiggedAttachedWarned)
            {
                LLSD args;
                LLViewerInventoryItem* itemp = gInventory.getItem(attached_object->getAttachmentItemID());
                args["NAME"] = itemp ? itemp->getName() : LLTrans::getString("Unknown");
                args["POINT"] = LLTrans::getString(getTargetAttachmentPoint(attached_object)->getName());
                LLNotificationsUtil::add("RiggedMeshAttachedToHUD", args);

                attached_object->mRiggedAttachedWarned = true;
            }

            hud_object_complexity.texturesCount += textures.size();

            for (LLVOVolume::texture_cost_t::iterator volume_texture = textures.begin();
                volume_texture != textures.end();
                ++volume_texture)
            {
                // add the cost of each individual texture (ignores duplicates)
                hud_object_complexity.texturesCost += LLVOVolume::getTextureCost(*volume_texture);
                const LLViewerTexture* img = *volume_texture;
                if (img->getType() == LLViewerTexture::FETCHED_TEXTURE)
                {
                    LLViewerFetchedTexture* tex = (LLViewerFetchedTexture*)img;
                    // Note: Texture memory might be incorect since texture might be still loading.
                    hud_object_complexity.texturesMemoryTotal += tex->getTextureMemory();
                    if (tex->getOriginalHeight() * tex->getOriginalWidth() >= HUD_OVERSIZED_TEXTURE_DATA_SIZE)
                    {
                        hud_object_complexity.largeTexturesCount++;
                    }
                }
            }
            hud_complexity_list.push_back(hud_object_complexity);
        }
    }
}

// Calculations for mVisualComplexity value
void LLVOAvatar::calculateUpdateRenderComplexity()
{
    /*****************************************************************
     * This calculation should not be modified by third party viewers,
     * since it is used to limit rendering and should be uniform for
     * everyone. If you have suggested improvements, submit them to
     * the official viewer for consideration.
     *****************************************************************/
    if (mVisualComplexityStale)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

        static const U32 COMPLEXITY_BODY_PART_COST = 200;
        static LLCachedControl<F32> max_complexity_setting(gSavedSettings, "MaxAttachmentComplexity");
        F32 max_attachment_complexity = max_complexity_setting;
        max_attachment_complexity = llmax(max_attachment_complexity, DEFAULT_MAX_ATTACHMENT_COMPLEXITY);

        // Diagnostic list of all textures on our avatar
        static std::unordered_set<const LLViewerTexture*> all_textures;

		U32 cost = VISUAL_COMPLEXITY_UNKNOWN;
		LLVOVolume::texture_cost_t textures;
		hud_complexity_list_t hud_complexity_list;
        object_complexity_list_t object_complexity_list;

		for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
		{
		    const LLAvatarAppearanceDictionary::BakedEntry *baked_dict
				= LLAvatarAppearance::getDictionary()->getBakedTexture((EBakedTextureIndex)baked_index);
			ETextureIndex tex_index = baked_dict->mTextureIndex;
			if ((tex_index != TEX_SKIRT_BAKED) || (isWearingWearableType(LLWearableType::WT_SKIRT)))
			{
                // Same as isTextureVisible(), but doesn't account for isSelf to ensure identical numbers for all avatars
                if (isIndexLocalTexture(tex_index))
                {
                    if (isTextureDefined(tex_index, 0))
                    {
                        cost += COMPLEXITY_BODY_PART_COST;
                    }
                }
                else
                {
                    // baked textures can use TE images directly
                    if (isTextureDefined(tex_index)
                        && (getTEImage(tex_index)->getID() != IMG_INVISIBLE || LLDrawPoolAlpha::sShowDebugAlpha))
                    {
                        cost += COMPLEXITY_BODY_PART_COST;
                    }
                }
			}
		}
        LL_DEBUGS("ARCdetail") << "Avatar body parts complexity: " << cost << LL_ENDL;

        mAttachmentVisibleTriangleCount = 0;
        mAttachmentEstTriangleCount = 0.f;
        mAttachmentSurfaceArea = 0.f;
        
        // A standalone animated object needs to be accounted for
        // using its associated volume. Attached animated objects
        // will be covered by the subsequent loop over attachments.
        LLControlAvatar *control_av = dynamic_cast<LLControlAvatar*>(this);
        if (control_av)
        {
            LLVOVolume *volp = control_av->mRootVolp;
            if (volp && !volp->isAttachment())
            {
                accountRenderComplexityForObject(volp, max_attachment_complexity,
                                                 textures, cost, hud_complexity_list, object_complexity_list);
            }
        }

        // Account for complexity of all attachments.
		for (attachment_map_t::const_iterator attachment_point = mAttachmentPoints.begin(); 
			 attachment_point != mAttachmentPoints.end();
			 ++attachment_point)
		{
			LLViewerJointAttachment* attachment = attachment_point->second;
			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
                LLViewerObject* attached_object = attachment_iter->get();
                accountRenderComplexityForObject(attached_object, max_attachment_complexity,
                                                 textures, cost, hud_complexity_list, object_complexity_list);
			}
		}

        if ( cost != mVisualComplexity )
        {
            LL_DEBUGS("AvatarRender") << "Avatar "<< getID()
                                      << " complexity updated was " << mVisualComplexity << " now " << cost
                                      << " reported " << mReportedVisualComplexity
                                      << LL_ENDL;
        }
        else
        {
            LL_DEBUGS("AvatarRender") << "Avatar "<< getID()
                                      << " complexity updated no change " << mVisualComplexity
                                      << " reported " << mReportedVisualComplexity
                                      << LL_ENDL;
        }
		mVisualComplexity = cost;
		mVisualComplexityStale = false;

        static LLCachedControl<U32> show_my_complexity_changes(gSavedSettings, "ShowMyComplexityChanges", 20);

        if (isSelf() && show_my_complexity_changes)
        {
            // Avatar complexity
            LLAvatarRenderNotifier::getInstance()->updateNotificationAgent(mVisualComplexity);
            LLAvatarRenderNotifier::getInstance()->setObjectComplexityList(object_complexity_list);
            // HUD complexity
            LLHUDRenderNotifier::getInstance()->updateNotificationHUD(hud_complexity_list);
        }

        //schedule an update to ART next frame if needed
        if (LLPerfStats::tunables.userAutoTuneEnabled && 
            LLPerfStats::tunables.userFPSTuningStrategy != LLPerfStats::TUNE_SCENE_ONLY &&
            !isVisuallyMuted())
        {
            LLUUID id = getID(); // <== use id to make sure this avatar didn't get deleted between frames
            LL::WorkQueue::getInstance("mainloop")->post([this, id]()
                {
                    if (gObjectList.findObject(id) != nullptr)
                    {
                        gPipeline.profileAvatar(this);
                    }
                });
        }
    }
}

void LLVOAvatar::setVisualMuteSettings(VisualMuteSettings set)
{
    mVisuallyMuteSetting = set;
    mNeedsImpostorUpdate = true;
	mLastImpostorUpdateReason = 7;

    LLRenderMuteList::getInstance()->saveVisualMuteSetting(getID(), S32(set));
}


void LLVOAvatar::setOverallAppearanceNormal()
{
	if (isControlAvatar())
		return;

    LLVector3 pelvis_pos = getJoint("mPelvis")->getPosition();
	resetSkeleton(false);
    getJoint("mPelvis")->setPosition(pelvis_pos);

	for (auto it = mJellyAnims.begin(); it !=  mJellyAnims.end(); ++it)
	{
		bool is_playing = (mPlayingAnimations.find(*it) != mPlayingAnimations.end());
		LL_DEBUGS("Avatar") << "jelly anim " << *it << " " << is_playing << LL_ENDL;
		if (!is_playing)
		{
			// Anim was not requested for this av by sim, but may be playing locally
			stopMotion(*it);
		}
	}
	mJellyAnims.clear();

	processAnimationStateChanges();
}

void LLVOAvatar::setOverallAppearanceJellyDoll()
{
	if (isControlAvatar())
		return;

	// stop current animations
	{
		for ( LLVOAvatar::AnimIterator anim_it= mPlayingAnimations.begin();
			  anim_it != mPlayingAnimations.end();
			  ++anim_it)
		{
			{
				stopMotion(anim_it->first, true);
			}
		}
	}
	processAnimationStateChanges();

	// Start any needed anims for jellydoll
	updateOverallAppearanceAnimations();
	
    LLVector3 pelvis_pos = getJoint("mPelvis")->getPosition();
	resetSkeleton(false);
    getJoint("mPelvis")->setPosition(pelvis_pos);

}

void LLVOAvatar::setOverallAppearanceInvisible()
{
}

void LLVOAvatar::updateOverallAppearance()
{
	AvatarOverallAppearance new_overall = getOverallAppearance();
	if (new_overall != mOverallAppearance)
	{
		switch (new_overall)
		{
			case AOA_NORMAL:
				setOverallAppearanceNormal();
				break;
			case AOA_JELLYDOLL:
				setOverallAppearanceJellyDoll();
				break;
			case AOA_INVISIBLE:
				setOverallAppearanceInvisible();
				break;
		}
		mOverallAppearance = new_overall;
		if (!isSelf())
		{
			mNeedsImpostorUpdate = true;
			mLastImpostorUpdateReason = 8;
		}
		updateMeshVisibility();
	}

	// This needs to be done even if overall appearance has not
	// changed, since sit/stand status can be different.
	updateOverallAppearanceAnimations();
}

void LLVOAvatar::updateOverallAppearanceAnimations()
{
	if (isControlAvatar())
		return;

	if (getOverallAppearance() == AOA_JELLYDOLL)
	{
		LLUUID motion_id;
		if (isSitting() && getParent()) // sitting on object
		{
			motion_id = ANIM_AGENT_SIT_FEMALE; 
		}
		else if (isSitting()) // sitting on ground
		{
			motion_id = ANIM_AGENT_SIT_GROUND_CONSTRAINED;
		}
		else // standing
		{
			motion_id = ANIM_AGENT_STAND;
		}
		if (mJellyAnims.find(motion_id) == mJellyAnims.end())
		{
			for (auto it = mJellyAnims.begin(); it !=  mJellyAnims.end(); ++it)
			{
				bool is_playing = (mPlayingAnimations.find(*it) != mPlayingAnimations.end());
				LL_DEBUGS("Avatar") << "jelly anim " << *it << " " << is_playing << LL_ENDL;
				if (!is_playing)
				{
					// Anim was not requested for this av by sim, but may be playing locally
					stopMotion(*it, true);
				}
			}
			mJellyAnims.clear();

			startMotion(motion_id);
			mJellyAnims.insert(motion_id);

			processAnimationStateChanges();
		}
	}
}

// Based on isVisuallyMuted(), but has 3 possible results.
LLVOAvatar::AvatarOverallAppearance LLVOAvatar::getOverallAppearance() const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;
	AvatarOverallAppearance result = AOA_NORMAL;

	// Priority order (highest priority first)
	// * own avatar is always drawn normally
	// * if on the "always draw normally" list, draw them normally
	// * if on the "always visually mute" list, show as jellydoll
	// * if explicitly muted (blocked), show as invisible
	// * check against the render cost and attachment limits - if too complex, show as jellydoll
	if (isSelf())
	{
		result = AOA_NORMAL;
	}
	else // !isSelf()
	{
		if (isInMuteList())
		{
			result = AOA_INVISIBLE;
		}
		else if (mVisuallyMuteSetting == AV_ALWAYS_RENDER)
		{
			result = AOA_NORMAL;
		}
		else if (mVisuallyMuteSetting == AV_DO_NOT_RENDER)
		{	// Always want to see this AV as an impostor
			result = AOA_JELLYDOLL;
		}
		else if (isTooComplex() || isTooSlow())
		{
			result = AOA_JELLYDOLL;
		}
	}

	return result;
}

void LLVOAvatar::calcMutedAVColor()
{
    LLColor4 new_color(mMutedAVColor);
    std::string change_msg;
    LLUUID av_id(getID());

    if (getVisualMuteSettings() == AV_DO_NOT_RENDER)
    {
        // explicitly not-rendered avatars are light grey
        new_color = LLColor4::grey4;
        change_msg = " not rendered: color is grey4";
    }
    else if (LLMuteList::getInstance()->isMuted(av_id)) // the user blocked them
    {
        // blocked avatars are dark grey
        new_color = LLColor4::grey4;
        change_msg = " blocked: color is grey4";
    }
    else if (!isTooComplex() && !isTooSlow())
    {
        new_color = LLColor4::white;
        change_msg = " simple imposter ";
    }
#ifdef COLORIZE_JELLYDOLLS
    else if ( mMutedAVColor == LLColor4::white || mMutedAVColor == LLColor4::grey3 || mMutedAVColor == LLColor4::grey4 )
	{
        // select a color based on the first byte of the agents uuid so any muted agent is always the same color
        F32 color_value = (F32) (av_id.mData[0]);
        F32 spectrum = (color_value / 256.0);          // spectrum is between 0 and 1.f

        // Array of colors.  These are arranged so only one RGB color changes between each step,
        // and it loops back to red so there is an even distribution.  It is not a heat map
        const S32 NUM_SPECTRUM_COLORS = 7;
        static LLColor4 * spectrum_color[NUM_SPECTRUM_COLORS] = { &LLColor4::red, &LLColor4::magenta, &LLColor4::blue, &LLColor4::cyan, &LLColor4::green, &LLColor4::yellow, &LLColor4::red };

        spectrum = spectrum * (NUM_SPECTRUM_COLORS - 1);               // Scale to range of number of colors
        S32 spectrum_index_1  = floor(spectrum);                               // Desired color will be after this index
        S32 spectrum_index_2  = spectrum_index_1 + 1;                  //    and before this index (inclusive)
        F32 fractBetween = spectrum - (F32)(spectrum_index_1);  // distance between the two indexes (0-1)

        new_color = lerp(*spectrum_color[spectrum_index_1], *spectrum_color[spectrum_index_2], fractBetween);
		new_color.normalize();
        new_color *= 0.28f;            // Tone it down
	}
#endif
    else
    {
		new_color = LLColor4::grey4;
        change_msg = " over limit color ";
    }

    if (mMutedAVColor != new_color) 
    {
        LL_DEBUGS("AvatarRender") << "avatar "<< av_id << change_msg << std::setprecision(3) << new_color << LL_ENDL;
        mMutedAVColor = new_color;
    }
}

// static
bool LLVOAvatar::isIndexLocalTexture(ETextureIndex index)
{
	return (index < 0 || index >= TEX_NUM_INDICES)
		? false
		: LLAvatarAppearance::getDictionary()->getTexture(index)->mIsLocalTexture;
}

// static
bool LLVOAvatar::isIndexBakedTexture(ETextureIndex index)
{
	return (index < 0 || index >= TEX_NUM_INDICES)
		? false
		: LLAvatarAppearance::getDictionary()->getTexture(index)->mIsBakedTexture;
}

const std::string LLVOAvatar::getBakedStatusForPrintout() const
{
	std::string line;

	for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearance::getDictionary()->getTextures().begin();
		 iter != LLAvatarAppearance::getDictionary()->getTextures().end();
		 ++iter)
	{
		const ETextureIndex index = iter->first;
		const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
		if (texture_dict->mIsBakedTexture)
		{
			line += texture_dict->mName;
			if (isTextureDefined(index))
			{
				line += "_baked";
			}
			line += " ";
		}
	}
	return line;
}



//virtual
S32 LLVOAvatar::getTexImageSize() const
{
	return TEX_IMAGE_SIZE_OTHER;
}

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

F32 calc_bouncy_animation(F32 x)
{
	return -(cosf(x * F_PI * 2.5f - F_PI_BY_TWO))*(0.4f + x * -0.1f) + x * 1.3f;
}

//virtual
bool LLVOAvatar::isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te, U32 index ) const
{
	if (isIndexLocalTexture(te)) 
	{
		return false;
	}
	
	if( !getImage( te, index ) )
	{
		LL_WARNS() << "getImage( " << te << ", " << index << " ) returned 0" << LL_ENDL;
		return false;
	}

	return (getImage(te, index)->getID() != IMG_DEFAULT_AVATAR && 
			getImage(te, index)->getID() != IMG_DEFAULT);
}

//virtual
bool LLVOAvatar::isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const
{
	if (isIndexLocalTexture(type))
	{
		return isTextureDefined(type, index);
	}
	else
	{
		// baked textures can use TE images directly
		return ((isTextureDefined(type) || isSelf())
				&& (getTEImage(type)->getID() != IMG_INVISIBLE 
				|| LLDrawPoolAlpha::sShowDebugAlpha));
	}
}

//virtual
bool LLVOAvatar::isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const
{
	// non-self avatars don't have wearables
	return false;
}

void LLVOAvatar::placeProfileQuery()
{
    if (mGPUTimerQuery == 0)
    {
        glGenQueries(1, &mGPUTimerQuery);
    }

    glBeginQuery(GL_TIME_ELAPSED, mGPUTimerQuery);
}

void LLVOAvatar::readProfileQuery(S32 retries)
{
    if (!mGPUProfilePending)
    {
        glEndQuery(GL_TIME_ELAPSED);
        mGPUProfilePending = true;
    }

    GLuint64 result = 0;
    glGetQueryObjectui64v(mGPUTimerQuery, GL_QUERY_RESULT_AVAILABLE, &result);

    if (result == GL_TRUE || --retries <= 0)
    { // query available, readback result
        GLuint64 time_elapsed = 0;
        glGetQueryObjectui64v(mGPUTimerQuery, GL_QUERY_RESULT, &time_elapsed);
        mGPURenderTime = time_elapsed / 1000000.f;
        mGPUProfilePending = false;

        setDebugText(llformat("%d", (S32)(mGPURenderTime * 1000.f)));

    }
    else
    { // wait until next frame
        LLUUID id = getID();

        LL::WorkQueue::getInstance("mainloop")->post([id, retries] {
            LLVOAvatar* avatar = (LLVOAvatar*) gObjectList.findObject(id);
            if(avatar)
            {
                avatar->readProfileQuery(retries);
            }
            });
    }
}


F32 LLVOAvatar::getGPURenderTime()
{
    return isVisuallyMuted() ? 0.f : mGPURenderTime;
}

// static
F32 LLVOAvatar::getTotalGPURenderTime()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    F32 ret = 0.f;

    for (LLCharacter* iter : LLCharacter::sInstances)
    {
        LLVOAvatar* inst = (LLVOAvatar*) iter;
        ret += inst->getGPURenderTime();
    }

    return ret;
}

F32 LLVOAvatar::getMaxGPURenderTime()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    F32 ret = 0.f;

    for (LLCharacter* iter : LLCharacter::sInstances)
    {
        LLVOAvatar* inst = (LLVOAvatar*)iter;
        ret = llmax(inst->getGPURenderTime(), ret);
    }

    return ret;
}

F32 LLVOAvatar::getAverageGPURenderTime()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_AVATAR;

    F32 ret = 0.f;

    S32 count = 0;

    for (LLCharacter* iter : LLCharacter::sInstances)
    {
        LLVOAvatar* inst = (LLVOAvatar*)iter;
        if (!inst->isTooSlow())
        {
            ret += inst->getGPURenderTime();
            ++count;
        }
    }

    if (count > 0)
    {
        ret /= count;
    }

    return ret;
}


