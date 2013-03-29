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
#include "llagentcamera.h"
#include "llagentwearables.h"
#include "llanimationstates.h"
#include "llavatarnamecache.h"
#include "llavatarpropertiesprocessor.h"
#include "llphysicsmotion.h"
#include "llviewercontrol.h"
#include "llcallingcard.h"		// IDEVO for LLAvatarTracker
#include "lldrawpoolavatar.h"
#include "lldriverparam.h"
#include "llpolyskeletaldistortion.h"
#include "lleditingmotion.h"
#include "llemote.h"
//#include "llfirstuse.h"
#include "llfloatertools.h"
#include "llheadrotmotion.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llhudnametag.h"
#include "llhudtext.h"				// for mText/mDebugText
#include "llinitparam.h"
#include "llkeyframefallmotion.h"
#include "llkeyframestandmotion.h"
#include "llkeyframewalkmotion.h"
#include "llmanipscale.h"  // for get_default_max_prim_scale()
#include "llmeshrepository.h"
#include "llmutelist.h"
#include "llmoveview.h"
#include "llnotificationsutil.h"
#include "llquantize.h"
#include "llrand.h"
#include "llregionhandle.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsprite.h"
#include "lltargetingmotion.h"
#include "lltoolmorph.h"
#include "llviewercamera.h"
#include "llviewertexlayer.h"
#include "llviewertexturelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
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
#include "llsdserialize.h"

extern F32 SPEED_ADJUST_MAX;
extern F32 SPEED_ADJUST_MAX_SEC;
extern F32 ANIM_SPEED_MAX;
extern F32 ANIM_SPEED_MIN;


// #define OUTPUT_BREAST_DATA

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

const S32 MIN_PIXEL_AREA_FOR_COMPOSITE = 1024;
const F32 SHADOW_OFFSET_AMT = 0.03f;

const F32 DELTA_TIME_MIN = 0.01f;	// we clamp measured deltaTime to this
const F32 DELTA_TIME_MAX = 0.2f;	// range to insure stability of computations.

const F32 PELVIS_LAG_FLYING		= 0.22f;// pelvis follow half life while flying
const F32 PELVIS_LAG_WALKING	= 0.4f;	// ...while walking
const F32 PELVIS_LAG_MOUSELOOK = 0.15f;
const F32 MOUSELOOK_PELVIS_FOLLOW_FACTOR = 0.5f;
const F32 PELVIS_LAG_WHEN_FOLLOW_CAM_IS_ON = 0.0001f; // not zero! - something gets divided by this!
const F32 TORSO_NOISE_AMOUNT = 1.0f;	// Amount of deviation from up-axis, in degrees
const F32 TORSO_NOISE_SPEED = 0.2f;	// Time scale factor on torso noise.

const F32 BREATHE_ROT_MOTION_STRENGTH = 0.05f;
const F32 BREATHE_SCALE_MOTION_STRENGTH = 0.005f;

const F32 MIN_SHADOW_HEIGHT = 0.f;
const F32 MAX_SHADOW_HEIGHT = 0.3f;

const S32 MIN_REQUIRED_PIXEL_AREA_BODY_NOISE = 10000;
const S32 MIN_REQUIRED_PIXEL_AREA_BREATHE = 10000;
const S32 MIN_REQUIRED_PIXEL_AREA_PELVIS_FIX = 40;

const S32 TEX_IMAGE_SIZE_SELF = 512;
const S32 TEX_IMAGE_AREA_SELF = TEX_IMAGE_SIZE_SELF * TEX_IMAGE_SIZE_SELF;
const S32 TEX_IMAGE_SIZE_OTHER = 512 / 4;  // The size of local textures for other (!isSelf()) avatars

const F32 HEAD_MOVEMENT_AVG_TIME = 0.9f;

const S32 MORPH_MASK_REQUESTED_DISCARD = 0;

// Discard level at which to switch to baked textures
// Should probably be 4 or 3, but didn't want to change it while change other logic - SJB
const S32 SWITCH_TO_BAKED_DISCARD = 5;

const F32 FOOT_COLLIDE_FUDGE = 0.04f;

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

enum ERenderName
{
	RENDER_NAME_NEVER,
	RENDER_NAME_ALWAYS,	
	RENDER_NAME_FADE
};

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

//------------------------------------------------------------------------
// LLVOAvatarBoneInfo
// Trans/Scale/Rot etc. info about each avatar bone.  Used by LLVOAvatarSkeleton.
//------------------------------------------------------------------------
struct LLVOAvatarCollisionVolumeInfo : public LLInitParam::Block<LLVOAvatarCollisionVolumeInfo>
{
	LLVOAvatarCollisionVolumeInfo() 
	:	name("name"),
		pos("pos"),
		rot("rot"),
		scale("scale")
	{}

	Mandatory<std::string>	name;
	Mandatory<LLVector3>	pos,
							rot,
							scale;
};

struct LLAppearanceMessageContents
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
};

struct LLVOAvatarChildJoint : public LLInitParam::ChoiceBlock<LLVOAvatarChildJoint>
	{
	Alternative<Lazy<struct LLVOAvatarBoneInfo, IS_A_BLOCK> >	bone;
	Alternative<LLVOAvatarCollisionVolumeInfo>		collision_volume;
	
	LLVOAvatarChildJoint()
	:	bone("bone"),
		collision_volume("collision_volume")
	{}
};

	

struct LLVOAvatarBoneInfo : public LLInitParam::Block<LLVOAvatarBoneInfo, LLVOAvatarCollisionVolumeInfo>
{
	LLVOAvatarBoneInfo() 
	:	pivot("pivot")
	{}
	
	Mandatory<LLVector3>					pivot;
	Multiple<LLVOAvatarChildJoint>			children;
};

//------------------------------------------------------------------------
// LLVOAvatarSkeletonInfo
// Overall avatar skeleton
//------------------------------------------------------------------------
struct LLVOAvatarSkeletonInfo : public LLInitParam::Block<LLVOAvatarSkeletonInfo>
{
	LLVOAvatarSkeletonInfo()
	:	skeleton_root(""),
		num_bones("num_bones"),
		num_collision_volumes("num_collision_volumes"),
		version("version")
	{}
	
	Mandatory<std::string>			version;
	Mandatory<S32>					num_bones,
									num_collision_volumes;
	Mandatory<LLVOAvatarChildJoint>	skeleton_root;
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
	virtual BOOL getLoop() { return TRUE; }

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
	// must return TRUE to indicate success, or else
	// it will be deactivated
	virtual BOOL onActivate() { return TRUE; }

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	virtual BOOL onUpdate(F32 time, U8* joint_mask)
	{
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

		return TRUE;
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
	virtual BOOL getLoop() { return TRUE; }

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
		BOOL success = true;

		if ( !mChestState->setJoint( character->getJoint( "mChest" ) ) ) { success = false; }

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
	// must return TRUE to indicate success, or else
	// it will be deactivated
	virtual BOOL onActivate() { return TRUE; }

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	virtual BOOL onUpdate(F32 time, U8* joint_mask)
	{
		mBreatheRate = 1.f;

		F32 breathe_amt = (sinf(mBreatheRate * time) * BREATHE_ROT_MOTION_STRENGTH);

		mChestState->setRotation(LLQuaternion(breathe_amt, LLVector3(0.f, 1.f, 0.f)));

		return TRUE;
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
	virtual BOOL getLoop() { return TRUE; }

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
	// must return TRUE to indicate success, or else
	// it will be deactivated
	virtual BOOL onActivate() { return TRUE; }

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	virtual BOOL onUpdate(F32 time, U8* joint_mask)
	{
		mPelvisState->setPosition(LLVector3::zero);

		return TRUE;
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
LLAvatarAppearanceDictionary *LLVOAvatar::sAvatarDictionary = NULL;
S32 LLVOAvatar::sFreezeCounter = 0;
U32 LLVOAvatar::sMaxVisible = 12;
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
BOOL LLVOAvatar::sRenderGroupTitles = TRUE;
S32 LLVOAvatar::sNumVisibleChatBubbles = 0;
BOOL LLVOAvatar::sDebugInvisible = FALSE;
BOOL LLVOAvatar::sShowAttachmentPoints = FALSE;
BOOL LLVOAvatar::sShowAnimationDebug = FALSE;
BOOL LLVOAvatar::sShowFootPlane = FALSE;
BOOL LLVOAvatar::sVisibleInFirstPerson = FALSE;
F32 LLVOAvatar::sLODFactor = 1.f;
F32 LLVOAvatar::sPhysicsLODFactor = 1.f;
BOOL LLVOAvatar::sUseImpostors = FALSE;
BOOL LLVOAvatar::sJointDebug = FALSE;
F32 LLVOAvatar::sUnbakedTime = 0.f;
F32 LLVOAvatar::sUnbakedUpdateTime = 0.f;
F32 LLVOAvatar::sGreyTime = 0.f;
F32 LLVOAvatar::sGreyUpdateTime = 0.f;

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
	mAttachmentGeometryBytes(0),
	mAttachmentSurfaceArea(0.f),
	mTurning(FALSE),
	mLastSkeletonSerialNum( 0 ),
	mIsSitting(FALSE),
	mTimeVisible(),
	mTyping(FALSE),
	mMeshValid(FALSE),
	mVisible(FALSE),
	mWindFreq(0.f),
	mRipplePhase( 0.f ),
	mBelowWater(FALSE),
	mLastAppearanceBlendTime(0.f),
	mAppearanceAnimating(FALSE),
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
	mFirstTEMessageReceived( FALSE ),
	mFirstAppearanceMessageReceived( FALSE ),
	mCulled( FALSE ),
	mVisibilityRank(0),
	mNeedsSkin(FALSE),
	mLastSkinTime(0.f),
	mUpdatePeriod(1),
	mFirstFullyVisible(TRUE),
	mFullyLoaded(FALSE),
	mPreviousFullyLoaded(FALSE),
	mFullyLoadedInitialized(FALSE),
	mLoadedCallbacksPaused(FALSE),
	mHasPelvisOffset( FALSE ),
	mRenderUnloadedAvatar(LLCachedControl<bool>(gSavedSettings, "RenderUnloadedAvatar")),
	mLastRezzedStatus(-1),
	mIsEditingAppearance(FALSE),
	mUseLocalAppearance(FALSE),
	mUseServerBakes(FALSE), // FIXME DRANO consider using boost::optional, defaulting to unknown.
	mLastUpdateRequestCOFVersion(-1),
	mLastUpdateReceivedCOFVersion(-1)
{
	//VTResume();  // VTune
	
	// mVoiceVisualizer is created by the hud effects manager and uses the HUD Effects pipeline
	const BOOL needsSendToSim = false; // currently, this HUD effect doesn't need to pack and unpack data to do its job
	mVoiceVisualizer = ( LLVoiceVisualizer *)LLHUDManager::getInstance()->createViewerEffect( LLHUDObject::LL_HUD_EFFECT_VOICE_VISUALIZER, needsSendToSim );

	lldebugs << "LLVOAvatar Constructor (0x" << this << ") id:" << mID << llendl;

	mPelvisp = NULL;

	mDirtyMesh = 2;	// Dirty geometry, need to regenerate.
	mMeshTexturesDirty = FALSE;
	mHeadp = NULL;


	// set up animation variables
	mSpeed = 0.f;
	setAnimationData("Speed", &mSpeed);

	mNeedsImpostorUpdate = TRUE;
	mNeedsAnimUpdate = TRUE;

	mImpostorDistance = 0;
	mImpostorPixelArea = 0;

	setNumTEs(TEX_NUM_INDICES);

	mbCanSelect = TRUE;

	mSignaledAnimations.clear();
	mPlayingAnimations.clear();

	mWasOnGroundLeft = FALSE;
	mWasOnGroundRight = FALSE;

	mTimeLast = 0.0f;
	mSpeedAccum = 0.0f;

	mRippleTimeLast = 0.f;

	mInAir = FALSE;

	mStepOnLand = TRUE;
	mStepMaterial = 0;

	mLipSyncActive = false;
	mOohMorph      = NULL;
	mAahMorph      = NULL;

	mCurrentGesticulationLevel = 0;

	mRuthTimer.reset();
	mRuthDebugTimer.reset();
	mDebugExistenceTimer.reset();
	mPelvisOffset = LLVector3(0.0f,0.0f,0.0f);
	mLastPelvisToFoot = 0.0f;
	mPelvisFixup = 0.0f;
	mLastPelvisFixup = 0.0f;
}

std::string LLVOAvatar::avString() const
{
	std::string viz_string = LLVOAvatar::rezStatusToString(getRezzedStatus());
	return " Avatar '" + getFullname() + "' " + viz_string + " ";
}

void LLVOAvatar::debugAvatarRezTime(std::string notification_name, std::string comment)
{
	LL_INFOS("Avatar") << "REZTIME: [ " << (U32)mDebugExistenceTimer.getElapsedTimeF32()
					   << "sec ]"
					   << avString() 
					   << "RuthTimer " << (U32)mRuthDebugTimer.getElapsedTimeF32()
					   << " Notification " << notification_name
					   << " : " << comment
					   << llendl;

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

	logPendingPhases();
	
	lldebugs << "LLVOAvatar Destructor (0x" << this << ") id:" << mID << llendl;

	std::for_each(mAttachmentPoints.begin(), mAttachmentPoints.end(), DeletePairedPointer());
	mAttachmentPoints.clear();

	mDead = TRUE;
	
	mAnimationSources.clear();
	LLLoadedCallbackEntry::cleanUpCallbackList(&mCallbackTextureList) ;

	getPhases().clearPhases();
	
	lldebugs << "LLVOAvatar Destructor end" << llendl;
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


BOOL LLVOAvatar::isFullyBaked()
{
	if (mIsDummy) return TRUE;
	if (getNumTEs() == 0) return FALSE;

	for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
	{
		if (!isTextureDefined(mBakedTextureDatas[i].mTextureIndex)
			&& ( (i != BAKED_SKIRT) || isWearingWearableType(LLWearableType::WT_SKIRT) ) )
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLVOAvatar::isFullyTextured() const
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
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLVOAvatar::hasGray() const
{
	return !getIsCloud() && !isFullyTextured();
}

S32 LLVOAvatar::getRezzedStatus() const
{
	if (getIsCloud()) return 0;
	if (isFullyTextured() && allBakedTexturesCompletelyDownloaded()) return 3;
	if (isFullyTextured()) return 2;
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
			LLImageGL::deleteTextures(LLTexUnit::TT_TEXTURE, 0, -1, 1, (GLuint*)&(mBakedTextureDatas[i].mMaskTexName));
			mBakedTextureDatas[i].mMaskTexName = 0 ;
		}
	}
}

// static 
BOOL LLVOAvatar::areAllNearbyInstancesBaked(S32& grey_avatars)
{
	BOOL res = TRUE;
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
			res = FALSE;
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
		if (!inst)
			continue;
		S32 rez_status = inst->getRezzedStatus();
		counts[rez_status]++;
	}
}

// static
std::string LLVOAvatar::rezStatusToString(S32 rez_status)
{
	if (rez_status==0) return "cloud";
	if (rez_status==1) return "gray";
	if (rez_status==2) return "textured";
	if (rez_status==3) return "textured_and_downloaded";
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
		llinfos << "Avatar ";

		LLNameValue* firstname = inst->getNVPair("FirstName");
		LLNameValue* lastname = inst->getNVPair("LastName");

		if( firstname )
		{
			llcont << firstname->getString();
		}
		if( lastname )
		{
			llcont << " " << lastname->getString();
		}

		llcont << " " << inst->mID;

		if( inst->isDead() )
		{
			llcont << " DEAD ("<< inst->getNumRefs() << " refs)";
		}

		if( inst->isSelf() )
		{
			llcont << " (self)";
		}


		F64 dist_to_camera = (inst->getPositionGlobal() - camera_pos_global).length();
		llcont << " " << dist_to_camera << "m ";

		llcont << " " << inst->mPixelArea << " pixels";

		if( inst->isVisible() )
		{
			llcont << " (visible)";
		}
		else
		{
			llcont << " (not visible)";
		}

		if( inst->isFullyBaked() )
		{
			llcont << " Baked";
		}
		else
		{
			llcont << " Unbaked (";
			
			for (LLAvatarAppearanceDictionary::BakedTextures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().begin();
				 iter != LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().end();
				 ++iter)
			{
				const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = iter->second;
				const ETextureIndex index = baked_dict->mTextureIndex;
				if (!inst->isTextureDefined(index))
				{
					llcont << " " << LLAvatarAppearanceDictionary::getInstance()->getTexture(index)->mName;
				}
			}
			llcont << " ) " << inst->getUnbakedPixelAreaRank();
			if( inst->isCulled() )
			{
				llcont << " culled";
			}
		}
		llcont << llendl;
	}
}

//static
void LLVOAvatar::restoreGL()
{
	if (!isAgentAvatarValid()) return;

	gAgentAvatarp->setCompositeUpdatesEnabled(TRUE);
	for (U32 i = 0; i < gAgentAvatarp->mBakedTextureDatas.size(); i++)
	{
		gAgentAvatarp->invalidateComposite(gAgentAvatarp->getTexLayerSet(i), FALSE);
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
	}
}

// static
void LLVOAvatar::deleteCachedImages(bool clearAll)
{	
	if (LLViewerTexLayerSet::sHasCaches)
	{
		lldebugs << "Deleting layer set caches" << llendl;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			 iter != LLCharacter::sInstances.end(); ++iter)
		{
			LLVOAvatar* inst = (LLVOAvatar*) *iter;
			inst->deleteLayerSetCaches(clearAll);
		}
		LLViewerTexLayerSet::sHasCaches = FALSE;
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
}


void LLVOAvatar::cleanupClass()
{
}

// virtual
void LLVOAvatar::initInstance(void)
{
	//-------------------------------------------------------------------------
	// register motions
	//-------------------------------------------------------------------------
	if (LLCharacter::sInstances.size() == 1)
	{
		LLKeyframeMotion::setVFS(gStaticVFS);
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
		if ( !mHasPelvisOffset )
		{
			return mDrawable->getPositionAgent();
		}
		else
		{
			//Apply a pelvis fixup (as defined by the avs skin)
			LLVector3 pos = mDrawable->getPositionAgent();
			pos[VZ] += mPelvisFixup;
			return pos;
		}
	}
	else
	{
		return getPosition() * mDrawable->getParent()->getRenderMatrix();
	}
}

void LLVOAvatar::updateDrawable(BOOL force_damped)
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
	if (isImpostor() && !needsImpostorUpdate())
	{
		LLVector3 delta = getRenderPosition() -
			((LLVector3(mDrawable->getPositionGroup().getF32ptr())-mImpostorOffset));
		
		newMin.load3( (mLastAnimExtents[0] + delta).mV);
		newMax.load3( (mLastAnimExtents[1] + delta).mV);
	}
	else
	{
		getSpatialExtents(newMin,newMax);
		mLastAnimExtents[0].set(newMin.getF32ptr());
		mLastAnimExtents[1].set(newMax.getF32ptr());
		LLVector4a pos_group;
		pos_group.setAdd(newMin,newMax);
		pos_group.mul(0.5f);
		mImpostorOffset = LLVector3(pos_group.getF32ptr())-getRenderPosition();
		mDrawable->setPositionGroup(pos_group);
	}
}

void LLVOAvatar::getSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{
	LLVector4a buffer(0.25f);
	LLVector4a pos;
	pos.load3(getRenderPosition().mV);
	newMin.setSub(pos, buffer);
	newMax.setAdd(pos, buffer);

	float max_attachment_span = get_default_max_prim_scale() * 5.0f;
	
	//stretch bounding box by joint positions
	for (polymesh_map_t::iterator i = mPolyMeshes.begin(); i != mPolyMeshes.end(); ++i)
	{
		LLPolyMesh* mesh = i->second;
		for (S32 joint_num = 0; joint_num < mesh->mJointRenderData.count(); joint_num++)
		{
			LLVector4a trans;
			trans.load3( mesh->mJointRenderData[joint_num]->mWorldMatrix->getTranslation().mV);
			update_min_max(newMin, newMax, trans);
		}
	}

	LLVector4a center, size;
	center.setAdd(newMin, newMax);
	center.mul(0.5f);

	size.setSub(newMax,newMin);
	size.mul(0.5f);

	mPixelArea = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());

	//stretch bounding box by attachments
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;

		if (!attachment->getValid())
		{
			continue ;
		}

		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			const LLViewerObject* attached_object = (*attachment_iter);
			if (attached_object && !attached_object->isHUDAttachment())
			{
				LLDrawable* drawable = attached_object->mDrawable;
				if (drawable && !drawable->isState(LLDrawable::RIGGED))
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

	//pad bounding box	

	newMin.sub(buffer);
	newMax.add(buffer);
}

//-----------------------------------------------------------------------------
// renderCollisionVolumes()
//-----------------------------------------------------------------------------
void LLVOAvatar::renderCollisionVolumes()
{
	for (S32 i = 0; i < mNumCollisionVolumes; i++)
	{
		mCollisionVolumes[i].renderCollision();
	}

	if (mNameText.notNull())
	{
		LLVector3 unused;
		mNameText->lineSegmentIntersect(LLVector3(0,0,0), LLVector3(0,0,1), unused, TRUE);
	}
}

BOOL LLVOAvatar::lineSegmentIntersect(const LLVector3& start, const LLVector3& end,
									  S32 face,
									  BOOL pick_transparent,
									  S32* face_hit,
									  LLVector3* intersection,
									  LLVector2* tex_coord,
									  LLVector3* normal,
									  LLVector3* bi_normal)
{
	if ((isSelf() && !gAgent.needsRenderAvatar()) || !LLPipeline::sPickAvatar)
	{
		return FALSE;
	}

	if (lineSegmentBoundingBox(start, end))
	{
		for (S32 i = 0; i < mNumCollisionVolumes; ++i)
		{
			mCollisionVolumes[i].updateWorldMatrix();

			glh::matrix4f mat((F32*) mCollisionVolumes[i].getXform()->getWorldMatrix().mMatrix);
			glh::matrix4f inverse = mat.inverse();
			glh::matrix4f norm_mat = inverse.transpose();

			glh::vec3f p1(start.mV);
			glh::vec3f p2(end.mV);

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
					*intersection = LLVector3(res_pos.v);
				}

				if (normal)
				{
					*normal = LLVector3(res_norm.v);
				}

				return TRUE;
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
					LLViewerObject* attached_object = (*attachment_iter);
					
					if (attached_object && !attached_object->isDead() && attachment->getValid())
					{
						LLDrawable* drawable = attached_object->mDrawable;
						if (drawable->isState(LLDrawable::RIGGED))
						{ //regenerate octree for rigged attachment
							gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_RIGGED, TRUE);
						}
					}
				}
			}
		}
	}

	
	
	LLVector3 position;
	if (mNameText.notNull() && mNameText->lineSegmentIntersect(start, end, position))
	{
		if (intersection)
		{
			*intersection = position;
		}

		return TRUE;
	}

	return FALSE;
}

LLViewerObject* LLVOAvatar::lineSegmentIntersectRiggedAttachments(const LLVector3& start, const LLVector3& end,
									  S32 face,
									  BOOL pick_transparent,
									  S32* face_hit,
									  LLVector3* intersection,
									  LLVector2* tex_coord,
									  LLVector3* normal,
									  LLVector3* bi_normal)
{
	if (isSelf() && !gAgent.needsRenderAvatar())
	{
		return NULL;
	}

	LLViewerObject* hit = NULL;

	if (lineSegmentBoundingBox(start, end))
	{
		LLVector3 local_end = end;
		LLVector3 local_intersection;

		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			iter != mAttachmentPoints.end();
			++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;

			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					attachment_iter != attachment->mAttachedObjects.end();
					++attachment_iter)
			{
				LLViewerObject* attached_object = (*attachment_iter);
					
				if (attached_object->lineSegmentIntersect(start, local_end, face, pick_transparent, face_hit, &local_intersection, tex_coord, normal, bi_normal))
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
	mIsBuilt = FALSE;

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
		llwarns << "Missing 'Ooh' morph for lipsync, using fallback." << llendl;
		mOohMorph = getVisualParam( "Express_Kiss" );
	}

	// If we don't have the Aah morph, use the Open Mouth morph
	if (!mAahMorph)
	{
		llwarns << "Missing 'Aah' morph for lipsync, using fallback." << llendl;
		mAahMorph = getVisualParam( "Express_Open_Mouth" );
	}

	startDefaultMotions();

	//-------------------------------------------------------------------------
	// restart any currently active motions
	//-------------------------------------------------------------------------
	processAnimationStateChanges();

	mIsBuilt = TRUE;
	stop_glerror();

	mMeshValid = TRUE;
}


//-----------------------------------------------------------------------------
// releaseMeshData()
//-----------------------------------------------------------------------------
void LLVOAvatar::releaseMeshData()
{
	if (sInstances.size() < AVATAR_RELEASE_THRESHOLD || mIsDummy)
	{
		return;
	}

	//llinfos << "Releasing" << llendl;

	// cleanup mesh data
	for (avatar_joint_list_t::iterator iter = mMeshLOD.begin();
		 iter != mMeshLOD.end(); 
		 ++iter)
	{
		LLAvatarJoint* joint = (*iter);
		joint->setValid(FALSE, TRUE);
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
			attachment->setAttachmentVisibility(FALSE);
		}
	}
	mMeshValid = FALSE;
}

//-----------------------------------------------------------------------------
// restoreMeshData()
//-----------------------------------------------------------------------------
// virtual
void LLVOAvatar::restoreMeshData()
{
	llassert(!isSelf());
	
	//llinfos << "Restoring" << llendl;
	mMeshValid = TRUE;
	updateJointLODs();

	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (!attachment->getIsHUDAttachment())
		{
			attachment->setAttachmentVisibility(TRUE);
		}
	}

	// force mesh update as LOD might not have changed to trigger this
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_GEOMETRY, TRUE);
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
				buff = new LLVertexBufferAvatar();
				buff->allocateBuffer(num_vertices, num_indices, TRUE);
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
					buff->resizeBuffer(num_vertices, num_indices);
				}
			}
			
		
			// This is a hack! Avatars have their own pool, so we are detecting
			//   the case of more than one avatar in the pool (thus > 0 instead of >= 0)
			if (facep->getGeomIndex() > 0)
			{
				llerrs << "non-zero geom index: " << facep->getGeomIndex() << " in LLVOAvatar::restoreMeshData" << llendl;
			}

			for(S32 k = j ; k < part_index ; k++)
			{
				bool rigid = false;
				if (k == MESH_ID_EYEBALL_LEFT ||
					k == MESH_ID_EYEBALL_RIGHT)
				{ //eyeballs can't have terse updates since they're never rendered with
					//the hardware skinning shader
					rigid = true;
				}
				
				LLViewerJoint* mesh = getViewerJoint(k);
				if (mesh)
				{
					mesh->updateFaceData(facep, mAdjustedPixelArea, k == MESH_ID_HAIR, terse_update && !rigid);
				}
			}

			stop_glerror();
			buff->flush();

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
	const BOOL has_name = !getNVPair("FirstName");

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

	//llinfos << getRotation() << llendl;
	//llinfos << getPosition() << llendl;

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
		result = gTextureList.findImage(uuid);
	}

	if (!result)
	{
		const std::string url = getImageURL(te,uuid);
		if (!url.empty())
		{
			LL_DEBUGS("Avatar") << avString() << "from URL " << url << llendl;
			result = LLViewerTextureManager::getFetchedTextureFromUrl(
				url, FTT_SERVER_BAKE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, uuid);
		}
		else
		{
			LL_DEBUGS("Avatar") << avString() << "from host " << uuid << llendl;
			LLHost host = getObjectHost();
			result = LLViewerTextureManager::getFetchedTexture(
				uuid, FTT_HOST_BAKE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE, 0, 0, host);
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

static LLFastTimer::DeclareTimer FTM_AVATAR_UPDATE("Avatar Update");
static LLFastTimer::DeclareTimer FTM_JOINT_UPDATE("Update Joints");

//------------------------------------------------------------------------
// LLVOAvatar::dumpAnimationState()
//------------------------------------------------------------------------
void LLVOAvatar::dumpAnimationState()
{
	llinfos << "==============================================" << llendl;
	for (LLVOAvatar::AnimIterator it = mSignaledAnimations.begin(); it != mSignaledAnimations.end(); ++it)
	{
		LLUUID id = it->first;
		std::string playtag = "";
		if (mPlayingAnimations.find(id) != mPlayingAnimations.end())
		{
			playtag = "*";
		}
		llinfos << gAnimLibrary.animationName(id) << playtag << llendl;
	}
	for (LLVOAvatar::AnimIterator it = mPlayingAnimations.begin(); it != mPlayingAnimations.end(); ++it)
	{
		LLUUID id = it->first;
		bool is_signaled = mSignaledAnimations.find(id) != mSignaledAnimations.end();
		if (!is_signaled)
		{
			llinfos << gAnimLibrary.animationName(id) << "!S" << llendl;
		}
	}
}

//------------------------------------------------------------------------
// idleUpdate()
//------------------------------------------------------------------------
void LLVOAvatar::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	LLFastTimer t(FTM_AVATAR_UPDATE);

	if (isDead())
	{
		llinfos << "Warning!  Idle on dead avatar" << llendl;
		return;
	}	

	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_AVATAR))
		&& !(gSavedSettings.getBOOL("DisableAllRenderTypes")))
	{
		return;
	}

	checkTextureLoading() ;
	
	// force immediate pixel area update on avatars using last frames data (before drawable or camera updates)
	setPixelAreaAndAngle(gAgent);

	// force asynchronous drawable update
	if(mDrawable.notNull())
	{	
		LLFastTimer t(FTM_JOINT_UPDATE);
	
		if (mIsSitting && getParent())
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
		LLViewerObject::idleUpdate(agent, world, time);
		
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
		LLViewerObject::idleUpdate(agent, world, time);
		setRotation(rotation);
	}

	// attach objects that were waiting for a drawable
	lazyAttach();
	
	// animate the character
	// store off last frame's root position to be consistent with camera position
	LLVector3 root_pos_last = mRoot->getWorldPosition();
	BOOL detailed_update = updateCharacter(agent);

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
	
	idleUpdateNameTag( root_pos_last );
	idleUpdateRenderCost();
}

void LLVOAvatar::idleUpdateVoiceVisualizer(bool voice_enabled)
{
	bool render_visualizer = voice_enabled;
	
	// Don't render the user's own voice visualizer when in mouselook, or when opening the mic is disabled.
	if(isSelf())
	{
		if(gAgentCamera.cameraMouselook() || gSavedSettings.getBOOL("VoiceDisableMic"))
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
					else	{ llinfos << "oops - CurrentGesticulationLevel can be only 0, 1, or 2"  << llendl; }
					
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
					if( mOohMorph ) mOohMorph->setWeight(mOohMorph->getMinWeight(), FALSE);
					if( mAahMorph ) mAahMorph->setWeight(mAahMorph->getMinWeight(), FALSE);
					
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
		
		if ( mIsSitting )
		{
			LLVector3 headOffset = LLVector3( 0.0f, 0.0f, mHeadOffset.mV[2] );
			mVoiceVisualizer->setVoiceSourceWorldPosition( mRoot->getWorldPosition() + headOffset );
		}
		else 
		{
			LLVector3 tagPos = mRoot->getWorldPosition();
			tagPos[VZ] -= mPelvisToFoot;
			tagPos[VZ] += ( mBodySize[VZ] + mAvatarOffset[VZ] + 0.125f );
			mVoiceVisualizer->setVoiceSourceWorldPosition( tagPos );
		}
	}//if ( voiceEnabled )
}		

static LLFastTimer::DeclareTimer FTM_ATTACHMENT_UPDATE("Update Attachments");

void LLVOAvatar::idleUpdateMisc(bool detailed_update)
{
	if (LLVOAvatar::sJointDebug)
	{
		llinfos << getFullname() << ": joint touches: " << LLJoint::sNumTouches << " updates: " << LLJoint::sNumUpdates << llendl;
	}

	LLJoint::sNumUpdates = 0;
	LLJoint::sNumTouches = 0;

	BOOL visible = isVisible() || mNeedsAnimUpdate;

	// update attachments positions
	if (detailed_update || !sUseImpostors)
	{
		LLFastTimer t(FTM_ATTACHMENT_UPDATE);
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end();
			 ++iter)
		{
			LLViewerJointAttachment* attachment = iter->second;

			for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
				 attachment_iter != attachment->mAttachedObjects.end();
				 ++attachment_iter)
			{
				LLViewerObject* attached_object = (*attachment_iter);
				BOOL visibleAttachment = visible || (attached_object && 
													 !(attached_object->mDrawable->getSpatialBridge() &&
													   attached_object->mDrawable->getSpatialBridge()->getRadius() < 2.0));
				
				if (visibleAttachment && attached_object && !attached_object->isDead() && attachment->getValid())
				{
					// if selecting any attachments, update all of them as non-damped
					if (LLSelectMgr::getInstance()->getSelection()->getObjectCount() && LLSelectMgr::getInstance()->getSelection()->isAttachment())
					{
						gPipeline.updateMoveNormalAsync(attached_object->mDrawable);
					}
					else
					{
						gPipeline.updateMoveDampedAsync(attached_object->mDrawable);
					}
					
					LLSpatialBridge* bridge = attached_object->mDrawable->getSpatialBridge();
					if (bridge)
					{
						gPipeline.updateMoveNormalAsync(bridge);
					}
					attached_object->updateText();	
				}
			}
		}
	}

	mNeedsAnimUpdate = FALSE;

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
				mNeedsImpostorUpdate = TRUE;
			}
		}

		if (detailed_update && !mNeedsImpostorUpdate)
		{	//update impostor if view angle, distance, or bounding box change
			//significantly
			
			F32 dist_diff = fabsf(distance-mImpostorDistance);
			if (dist_diff/mImpostorDistance > 0.1f)
			{
				mNeedsImpostorUpdate = TRUE;
			}
			else
			{
				//VECTORIZE THIS
				getSpatialExtents(ext[0], ext[1]);
				LLVector4a diff;
				diff.setSub(ext[1], mImpostorExtents[1]);
				if (diff.getLength3().getF32() > 0.05f)
				{
					mNeedsImpostorUpdate = TRUE;
				}
				else
				{
					diff.setSub(ext[0], mImpostorExtents[0]);
					if (diff.getLength3().getF32() > 0.05f)
					{
						mNeedsImpostorUpdate = TRUE;
					}
				}
			}
		}
	}

	mDrawable->movePartition();
	
	//force a move if sitting on an active object
	if (getParent() && ((LLViewerObject*) getParent())->mDrawable->isActive())
	{
		gPipeline.markMoved(mDrawable, TRUE);
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
			mAppearanceAnimating = FALSE;
			for (LLVisualParam *param = getFirstVisualParam(); 
				 param;
				 param = getNextVisualParam())
			{
				if (param->isTweakable())
				{
					param->stopAnimating(FALSE);
				}
			}
			updateVisualParams();
			if (isSelf())
			{
				gAgent.sendAgentSetAppearance();
			}
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
						param->animate(morph_amt, FALSE);
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
	if ( voice_enabled && (LLVoiceClient::getInstance()->lipSyncEnabled()) && LLVoiceClient::getInstance()->getIsSpeaking( mID ) )
	{
		F32 ooh_morph_amount = 0.0f;
		F32 aah_morph_amount = 0.0f;

		mVoiceVisualizer->lipSyncOohAah( ooh_morph_amount, aah_morph_amount );

		if( mOohMorph )
		{
			F32 ooh_weight = mOohMorph->getMinWeight()
				+ ooh_morph_amount * (mOohMorph->getMaxWeight() - mOohMorph->getMinWeight());

			mOohMorph->setWeight( ooh_weight, FALSE );
		}

		if( mAahMorph )
		{
			F32 aah_weight = mAahMorph->getMinWeight()
				+ aah_morph_amount * (mAahMorph->getMaxWeight() - mAahMorph->getMinWeight());

			mAahMorph->setWeight( aah_weight, FALSE );
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
		if (isFullyLoaded() && mFirstFullyVisible && isSelf())
		{
			LL_INFOS("Avatar") << avString() << "self isFullyLoaded, mFirstFullyVisible" << LL_ENDL;
			mFirstFullyVisible = FALSE;
				LLAppearanceMgr::instance().onFirstFullyVisible();
			}
		if (isFullyLoaded() && mFirstFullyVisible && !isSelf())
		{
			LL_INFOS("Avatar") << avString() << "other isFullyLoaded, mFirstFullyVisible" << LL_ENDL;
			mFirstFullyVisible = FALSE;
		}
		if (isFullyLoaded())
		{
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
			LLViewerTexture* cloud = LLViewerTextureManager::getFetchedTextureFromFile("cloud-particle.j2c");
			particle_parameters.mPartImageID                 = cloud->getID();
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
			
			if (!isTooComplex()) // do not generate particles for overly-complex avatars
			{
				setParticleSource(particle_parameters, getID());
			}
		}
	}
}	

void LLVOAvatar::idleUpdateWindEffect()
{
	// update wind effect
	if ((LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) >= LLDrawPoolAvatar::SHADER_LEVEL_CLOTH))
	{
		F32 hover_strength = 0.f;
		F32 time_delta = mRippleTimer.getElapsedTimeF32() - mRippleTimeLast;
		mRippleTimeLast = mRippleTimer.getElapsedTimeF32();
		LLVector3 velocity = getVelocity();
		F32 speed = velocity.length();
		//RN: velocity varies too much frame to frame for this to work
		mRippleAccel.clearVec();//lerp(mRippleAccel, (velocity - mLastVel) * time_delta, LLCriticalDamp::getInterpolant(0.02f));
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
			interp = LLCriticalDamp::getInterpolant(0.2f);
		}
		else
		{
			interp = LLCriticalDamp::getInterpolant(0.4f);
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
	// update chat bubble
	//--------------------------------------------------------------------
	// draw text label over character's head
	//--------------------------------------------------------------------
	if (mChatTimer.getElapsedTimeF32() > BUBBLE_CHAT_TIME)
	{
		mChats.clear();
	}
	
	const F32 time_visible = mTimeVisible.getElapsedTimeF32();
	const F32 NAME_SHOW_TIME = gSavedSettings.getF32("RenderNameShowTime");	// seconds
	const F32 FADE_DURATION = gSavedSettings.getF32("RenderNameFadeDuration"); // seconds
	BOOL visible_avatar = isVisible() || mNeedsAnimUpdate;
	BOOL visible_chat = gSavedSettings.getBOOL("UseChatBubbles") && (mChats.size() || mTyping);
	BOOL render_name =	visible_chat ||
		(visible_avatar &&
		 ((sRenderName == RENDER_NAME_ALWAYS) ||
		  (sRenderName == RENDER_NAME_FADE && time_visible < NAME_SHOW_TIME)));
	// If it's your own avatar, don't draw in mouselook, and don't
	// draw if we're specifically hiding our own name.
	if (isSelf())
	{
		render_name = render_name
			&& !gAgentCamera.cameraMouselook()
			&& (visible_chat || (gSavedSettings.getBOOL("RenderNameShowSelf") 
								 && gSavedSettings.getS32("AvatarNameTagMode") ));
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

	BOOL new_name = FALSE;
	if (visible_chat != mVisibleChat)
	{
		mVisibleChat = visible_chat;
		new_name = TRUE;
	}

	if (sRenderGroupTitles != mRenderGroupTitles)
	{
		mRenderGroupTitles = sRenderGroupTitles;
		new_name = TRUE;
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
		mNameText->setVisibleOffScreen(TRUE);
		mNameText->setMaxLines(11);
		mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
		sNumVisibleChatBubbles++;
		new_name = TRUE;
    }
				
	idleUpdateNameTagPosition(root_pos_last);
	idleUpdateNameTagText(new_name);			
	idleUpdateNameTagAlpha(new_name, alpha);
}

void LLVOAvatar::idleUpdateNameTagText(BOOL new_name)
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
		is_muted = LLMuteList::getInstance()->isMuted(getID());
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
				LLFontGL::getFontSansSerifSmall());
		}

		static LLUICachedControl<bool> show_display_names("NameTagShowDisplayNames");
		static LLUICachedControl<bool> show_usernames("NameTagShowUsernames");

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
					LLFontGL::getFontSansSerif());
			}
			// Suppress SLID display if display name matches exactly (ugh)
			if (show_usernames && !av_name.isDisplayNameDefault())
			{
				// *HACK: Desaturate the color
				LLColor4 username_color = name_tag_color * 0.83f;
				addNameTagLine(av_name.getUserName(), username_color, LLFontGL::NORMAL,
					LLFontGL::getFontSansSerifSmall());
			}
		}
		else
		{
			const LLFontGL* font = LLFontGL::getFontSansSerif();
			std::string full_name = LLCacheName::buildFullName( firstname->getString(), lastname->getString() );
			addNameTagLine(full_name, name_tag_color, LLFontGL::NORMAL, font);
		}

		mNameAway = is_away;
		mNameDoNotDisturb = is_do_not_disturb;
		mNameMute = is_muted;
		mNameAppearance = is_appearance;
		mNameFriend = is_friend;
		mNameCloud = is_cloud;
		mTitle = title ? title->getString() : "";
		LLStringFn::replace_ascii_controlchars(mTitle,LL_UNKNOWN_CHAR);
		new_name = TRUE;
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
		mNameText->setVisibleOffScreen(TRUE);

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
		mNameText->setVisibleOffScreen(FALSE);
	}
}

void LLVOAvatar::addNameTagLine(const std::string& line, const LLColor4& color, S32 style, const LLFontGL* font)
{
	llassert(mNameText);
	if (mVisibleChat)
	{
		mNameText->addLabel(line);
	}
	else
	{
		mNameText->addLine(line, color, (LLFontGL::StyleFlags)style, font);
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
	
	mCurRootToHeadOffset = lerp(mCurRootToHeadOffset, mTargetRootToHeadOffset, LLCriticalDamp::getInterpolant(0.2f));

	LLVector3 name_position = mRoot->getLastWorldPosition() + (mCurRootToHeadOffset * root_rot);
	name_position += (local_camera_up * root_rot) - (projected_vec(local_camera_at * root_rot, camera_to_av));	
	name_position += pixel_up_vec * NAMETAG_VERTICAL_SCREEN_OFFSET;

	mNameText->setPositionAgent(name_position);				
}

void LLVOAvatar::idleUpdateNameTagAlpha(BOOL new_name, F32 alpha)
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
	static LLUICachedControl<bool> show_friends("NameTagShowFriends");
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
	mRoot->setWorldPosition(getPositionAgent()); // teleport
	setChanged(TRANSLATED);
	if (mDrawable.notNull())
	{
		gPipeline.updateMoveNormalAsync(mDrawable);
	}
	mRoot->updateWorldMatrixChildren();
}

bool LLVOAvatar::isVisuallyMuted() const
{
	static LLCachedControl<U32> max_attachment_bytes(gSavedSettings, "RenderAutoMuteByteLimit");
	static LLCachedControl<F32> max_attachment_area(gSavedSettings, "RenderAutoMuteSurfaceAreaLimit");
	
	return LLMuteList::getInstance()->isMuted(getID()) 
			|| (mAttachmentGeometryBytes > max_attachment_bytes && max_attachment_bytes > 0) 
			|| (mAttachmentSurfaceArea > max_attachment_area && max_attachment_area > 0.f);
}

//------------------------------------------------------------------------
// updateCharacter()
// called on both your avatar and other avatars
//------------------------------------------------------------------------
BOOL LLVOAvatar::updateCharacter(LLAgent &agent)
{
	// clear debug text
	mDebugText.clear();

	if (gSavedSettings.getBOOL("DebugAvatarAppearanceMessage"))
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
										  mUseServerBakes, central_bake_version);
		std::string origin_string = bakedTextureOriginInfo();
		debug_line += " [" + origin_string + "]";
		S32 curr_cof_version = LLAppearanceMgr::instance().getCOFVersion();
		S32 last_request_cof_version = mLastUpdateRequestCOFVersion;
		S32 last_received_cof_version = mLastUpdateReceivedCOFVersion;
		if (isSelf())
		{
			debug_line += llformat(" - cof: %d req: %d rcv:%d",
								   curr_cof_version, last_request_cof_version, last_received_cof_version);
			if (gSavedSettings.getBOOL("DebugForceAppearanceRequestFailure"))
			{
				debug_line += " FORCING ERRS";
			}
		}
		else
		{
			debug_line += llformat(" - cof rcv:%d", last_received_cof_version);
		}
		addDebugText(debug_line);
	}
	if (gSavedSettings.getBOOL("DebugAvatarCompositeBaked"))
	{
		if (!mBakedTextureDebugText.empty())
			addDebugText(mBakedTextureDebugText);
	}
				 
	if (LLVOAvatar::sShowAnimationDebug)
	{
		for (LLMotionController::motion_list_t::iterator iter = mMotionController.getActiveMotions().begin();
			 iter != mMotionController.getActiveMotions().end(); ++iter)
		{
			LLMotion* motionp = *iter;
			if (motionp->getMinPixelArea() < getPixelArea())
			{
				std::string output;
				if (motionp->getName().empty())
				{
					output = llformat("%s - %d",
							  gAgent.isGodlikeWithoutAdminMenuFakery() ?
							  motionp->getID().asString().c_str() :
							  LLUUID::null.asString().c_str(),
							  (U32)motionp->getPriority());
				}
				else
				{
					output = llformat("%s - %d",
							  motionp->getName().c_str(),
							  (U32)motionp->getPriority());
				}
				addDebugText(output);
			}
		}
	}

	if (!mIsBuilt)
	{
		return FALSE;
	}

	BOOL visible = isVisible();

	// For fading out the names above heads, only let the timer
	// run if we're visible.
	if (mDrawable.notNull() && !visible)
	{
		mTimeVisible.reset();
	}

	//--------------------------------------------------------------------
	// the rest should only be done occasionally for far away avatars
	//--------------------------------------------------------------------

	if (visible && (!isSelf() || isVisuallyMuted()) && !mIsDummy && sUseImpostors && !mNeedsAnimUpdate && !sFreezeCounter)
	{
		const LLVector4a* ext = mDrawable->getSpatialExtents();
		LLVector4a size;
		size.setSub(ext[1],ext[0]);
		F32 mag = size.getLength3().getF32()*0.5f;

		
		F32 impostor_area = 256.f*512.f*(8.125f - LLVOAvatar::sLODFactor*8.f);
		if (isVisuallyMuted())
		{ // muted avatars update at 16 hz
			mUpdatePeriod = 16;
		}
		else if (mVisibilityRank <= LLVOAvatar::sMaxVisible ||
			mDrawable->mDistanceWRTCamera < 1.f + mag)
		{ //first 25% of max visible avatars are not impostored
			//also, don't impostor avatars whose bounding box may be penetrating the 
			//impostor camera near clip plane
			mUpdatePeriod = 1;
		}
		else if (mVisibilityRank > LLVOAvatar::sMaxVisible * 4)
		{ //background avatars are REALLY slow updating impostors
			mUpdatePeriod = 16;
		}
		else if (mVisibilityRank > LLVOAvatar::sMaxVisible * 3)
		{ //back 25% of max visible avatars are slow updating impostors
			mUpdatePeriod = 8;
		}
		else if (mImpostorPixelArea <= impostor_area)
		{  // stuff in between gets an update period based on pixel area
			mUpdatePeriod = llclamp((S32) sqrtf(impostor_area*4.f/mImpostorPixelArea), 2, 8);
		}
		else
		{
			//nearby avatars, update the impostors more frequently.
			mUpdatePeriod = 4;
		}

		visible = (LLDrawable::getCurrentFrame()+mID.mData[0])%mUpdatePeriod == 0 ? TRUE : FALSE;
	}
	else
	{
		mUpdatePeriod = 1;
	}


	// don't early out for your own avatar, as we rely on your animations playing reliably
	// for example, the "turn around" animation when entering customize avatar needs to trigger
	// even when your avatar is offscreen
	if (!visible && !isSelf())
	{
		updateMotions(LLCharacter::HIDDEN_UPDATE);
		return FALSE;
	}

	// change animation time quanta based on avatar render load
	if (!isSelf() && !mIsDummy)
	{
		F32 time_quantum = clamp_rescale((F32)sInstances.size(), 10.f, 35.f, 0.f, 0.25f);
		F32 pixel_area_scale = clamp_rescale(mPixelArea, 100, 5000, 1.f, 0.f);
		F32 time_step = time_quantum * pixel_area_scale;
		if (time_step != 0.f)
		{
			// disable walk motion servo controller as it doesn't work with motion timesteps
			stopMotion(ANIM_AGENT_WALK_ADJUST);
			removeAnimationData("Walk Speed");
		}
		mMotionController.setTimeStep(time_step);
//		llinfos << "Setting timestep to " << time_quantum * pixel_area_scale << llendl;
	}

	if (getParent() && !mIsSitting)
	{
		sitOnObject((LLViewerObject*)getParent());
	}
	else if (!getParent() && mIsSitting && !isMotionActive(ANIM_AGENT_SIT_GROUND_CONSTRAINED))
	{
		getOffObject();
	}

	//--------------------------------------------------------------------
	// create local variables in world coords for region position values
	//--------------------------------------------------------------------
	F32 speed;
	LLVector3 normal;

	LLVector3 xyVel = getVelocity();
	xyVel.mV[VZ] = 0.0f;
	speed = xyVel.length();

	if (!(mIsSitting && getParent()))
	{
		//--------------------------------------------------------------------
		// get timing info
		// handle initial condition case
		//--------------------------------------------------------------------
		F32 animation_time = mAnimTimer.getElapsedTimeF32();
		if (mTimeLast == 0.0f)
		{
			mTimeLast = animation_time;

			// put the pelvis at slaved position/mRotation
			mRoot->setWorldPosition( getPositionAgent() ); // first frame
			mRoot->setWorldRotation( getRotation() );
		}
	
		//--------------------------------------------------------------------
		// dont' let dT get larger than 1/5th of a second
		//--------------------------------------------------------------------
		F32 deltaTime = animation_time - mTimeLast;

		deltaTime = llclamp( deltaTime, DELTA_TIME_MIN, DELTA_TIME_MAX );
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
		root_pos.mdV[VZ] += getVisualParamWeight(11001);


		resolveHeightGlobal(root_pos, ground_under_pelvis, normal);
		F32 foot_to_ground = (F32) (root_pos.mdV[VZ] - mPelvisToFoot - ground_under_pelvis.mdV[VZ]);				
		BOOL in_air = ((!LLWorld::getInstance()->getRegionFromPosGlobal(ground_under_pelvis)) || 
						foot_to_ground > FOOT_GROUND_COLLISION_TOLERANCE);

		if (in_air && !mInAir)
		{
			mTimeInAir.reset();
		}
		mInAir = in_air;

		// correct for the fact that the pelvis is not necessarily the center 
		// of the agent's physical representation
		root_pos.mdV[VZ] -= (0.5f * mBodySize.mV[VZ]) - mPelvisToFoot;
		
		LLVector3 newPosition = gAgent.getPosAgentFromGlobal(root_pos);

		if (newPosition != mRoot->getXform()->getWorldPosition())
		{		
			mRoot->touch();
			mRoot->setWorldPosition( newPosition ); // regular update				
		}


		//--------------------------------------------------------------------
		// Propagate viewer object rotation to root of avatar
		//--------------------------------------------------------------------
		if (!isAnyAnimationSignaled(AGENT_NO_ROTATE_ANIMS, NUM_AGENT_NO_ROTATE_ANIMS))
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
			// forward direction to allow it to hold it's position while the torso
			// and head turn.  Once in motion, it must conform however.
			BOOL self_in_mouselook = isSelf() && gAgentCamera.cameraMouselook();

			LLVector3 pelvisDir( mRoot->getWorldMatrix().getFwdRow4().mV );

			static LLCachedControl<F32> s_pelvis_rot_threshold_slow(gSavedSettings, "AvatarRotateThresholdSlow");
			static LLCachedControl<F32> s_pelvis_rot_threshold_fast(gSavedSettings, "AvatarRotateThresholdFast");

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
					mTurning = TRUE;
				}

				// use tighter threshold when turning
				if (mTurning)
				{
					pelvis_rot_threshold *= 0.4f;
				}

				// am I done turning?
				if (angle < pelvis_rot_threshold)
				{
					mTurning = FALSE;
				}

				LLVector3 correction_vector = (pelvisDir - fwdDir) * clamp_rescale(angle, pelvis_rot_threshold*0.75f, pelvis_rot_threshold, 1.0f, 0.0f);
				fwdDir += correction_vector;
			}
			else
			{
				mTurning = FALSE;
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
			//F32 u = LLCriticalDamp::getInterpolant(PELVIS_LAG);
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

			F32 u = llclamp((deltaTime / pelvis_lag_time), 0.0f, 1.0f);	

			mRoot->setWorldRotation( slerp(u, mRoot->getWorldRotation(), wQv) );
			
		}
	}
	else if (mDrawable.notNull())
	{
		mRoot->setPosition(mDrawable->getPosition());
		mRoot->setRotation(mDrawable->getRotation());
	}
	
	//-------------------------------------------------------------------------
	// Update character motions
	//-------------------------------------------------------------------------
	// store data relevant to motions
	mSpeed = speed;

	// update animations
	if (mSpecialRenderMode == 1) // Animation Preview
		updateMotions(LLCharacter::FORCE_UPDATE);
	else
		updateMotions(LLCharacter::NORMAL_UPDATE);

	// update head position
	updateHeadOffset();

	//-------------------------------------------------------------------------
	// Find the ground under each foot, these are used for a variety
	// of things that follow
	//-------------------------------------------------------------------------
	LLVector3 ankle_left_pos_agent = mFootLeftp->getWorldPosition();
	LLVector3 ankle_right_pos_agent = mFootRightp->getWorldPosition();

	LLVector3 ankle_left_ground_agent = ankle_left_pos_agent;
	LLVector3 ankle_right_ground_agent = ankle_right_pos_agent;
	resolveHeightAgent(ankle_left_pos_agent, ankle_left_ground_agent, normal);
	resolveHeightAgent(ankle_right_pos_agent, ankle_right_ground_agent, normal);

	F32 leftElev = llmax(-0.2f, ankle_left_pos_agent.mV[VZ] - ankle_left_ground_agent.mV[VZ]);
	F32 rightElev = llmax(-0.2f, ankle_right_pos_agent.mV[VZ] - ankle_right_ground_agent.mV[VZ]);

	if (!mIsSitting)
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
	
	//-------------------------------------------------------------------------
	// Generate footstep sounds when feet hit the ground
	//-------------------------------------------------------------------------
	const LLUUID AGENT_FOOTSTEP_ANIMS[] = {ANIM_AGENT_WALK, ANIM_AGENT_RUN, ANIM_AGENT_LAND};
	const S32 NUM_AGENT_FOOTSTEP_ANIMS = LL_ARRAY_SIZE(AGENT_FOOTSTEP_ANIMS);

	if ( gAudiop && isAnyAnimationSignaled(AGENT_FOOTSTEP_ANIMS, NUM_AGENT_FOOTSTEP_ANIMS) )
	{
		BOOL playSound = FALSE;
		LLVector3 foot_pos_agent;

		BOOL onGroundLeft = (leftElev <= 0.05f);
		BOOL onGroundRight = (rightElev <= 0.05f);

		// did left foot hit the ground?
		if ( onGroundLeft && !mWasOnGroundLeft )
		{
			foot_pos_agent = ankle_left_pos_agent;
			playSound = TRUE;
		}

		// did right foot hit the ground?
		if ( onGroundRight && !mWasOnGroundRight )
		{
			foot_pos_agent = ankle_right_pos_agent;
			playSound = TRUE;
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

	mRoot->updateWorldMatrixChildren();

	if (!mDebugText.size() && mText.notNull())
	{
		mText->markDead();
		mText = NULL;
	}
	else if (mDebugText.size())
	{
		setDebugText(mDebugText);
	}

	//mesh vertices need to be reskinned
	mNeedsSkin = TRUE;

	return TRUE;
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
	if (mIsSitting)
	{
		mHeadOffset = midEyePt;	
	}
	else
	{
		F32 u = llmax(0.f, HEAD_MOVEMENT_AVG_TIME - (1.f / gFPSClamped));
		mHeadOffset = lerp(midEyePt, mHeadOffset,  u);
	}
}
//------------------------------------------------------------------------
// setPelvisOffset
//------------------------------------------------------------------------
void LLVOAvatar::setPelvisOffset( bool hasOffset, const LLVector3& offsetAmount, F32 pelvisFixup ) 
{
	mHasPelvisOffset = hasOffset;
	if ( mHasPelvisOffset )
	{
		//Store off last pelvis to foot value
		mLastPelvisToFoot = mPelvisToFoot;
		mPelvisOffset	  = offsetAmount;
		mLastPelvisFixup  = mPelvisFixup;
		mPelvisFixup	  = pelvisFixup;
	}
}
//------------------------------------------------------------------------
// postPelvisSetRecalc
//------------------------------------------------------------------------
void LLVOAvatar::postPelvisSetRecalc( void )
{	
	computeBodySize(); 
	mRoot->touch();
	mRoot->updateWorldMatrixChildren();	
	dirtyMesh();
	updateHeadOffset();
}
//------------------------------------------------------------------------
// pelisPoke
//------------------------------------------------------------------------
void LLVOAvatar::setPelvisOffset( F32 pelvisFixupAmount )
{	
	mHasPelvisOffset  = true;
	mLastPelvisFixup  = mPelvisFixup;	
	mPelvisFixup	  = pelvisFixupAmount;	
}
//------------------------------------------------------------------------
// updateVisibility()
//------------------------------------------------------------------------
void LLVOAvatar::updateVisibility()
{
	BOOL visible = FALSE;

	if (mIsDummy)
	{
		visible = TRUE;
	}
	else if (mDrawable.isNull())
	{
		visible = FALSE;
	}
	else
	{
		if (!mDrawable->getSpatialGroup() || mDrawable->getSpatialGroup()->isVisible())
		{
			visible = TRUE;
		}
		else
		{
			visible = FALSE;
		}

		if(isSelf())
		{
			if (!gAgentWearables.areWearablesLoaded())
			{
				visible = FALSE;
			}
		}
		else if( !mFirstAppearanceMessageReceived )
		{
			visible = FALSE;
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
				llinfos << "Avatar " << this << " updating visiblity" << llendl;
			}

			if (visible)
			{
				llinfos << "Visible" << llendl;
			}
			else
			{
				llinfos << "Not visible" << llendl;
			}

			/*if (avatar_in_frustum)
			{
				llinfos << "Avatar in frustum" << llendl;
			}
			else
			{
				llinfos << "Avatar not in frustum" << llendl;
			}*/

			/*if (LLViewerCamera::getInstance()->sphereInFrustum(sel_pos_agent, 2.0f))
			{
				llinfos << "Sel pos visible" << llendl;
			}
			if (LLViewerCamera::getInstance()->sphereInFrustum(wrist_right_pos_agent, 0.2f))
			{
				llinfos << "Wrist pos visible" << llendl;
			}
			if (LLViewerCamera::getInstance()->sphereInFrustum(getPositionAgent(), getMaxScale()*2.f))
			{
				llinfos << "Agent visible" << llendl;
			}*/
			llinfos << "PA: " << getPositionAgent() << llendl;
			/*llinfos << "SPA: " << sel_pos_agent << llendl;
			llinfos << "WPA: " << wrist_right_pos_agent << llendl;*/
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
				 iter != mAttachmentPoints.end();
				 ++iter)
			{
				LLViewerJointAttachment* attachment = iter->second;

				for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
					 attachment_iter != attachment->mAttachedObjects.end();
					 ++attachment_iter)
				{
					if (LLViewerObject *attached_object = (*attachment_iter))
					{
						if(attached_object->mDrawable->isVisible())
						{
							llinfos << attachment->getName() << " visible" << llendl;
						}
						else
						{
							llinfos << attachment->getName() << " not visible at " << mDrawable->getWorldPosition() << " and radius " << mDrawable->getRadius() << llendl;
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
		if (mMeshValid && mMeshInvisibleTime.getElapsedTimeF32() > TIME_BEFORE_MESH_CLEANUP)
		{
			releaseMeshData();
		}
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
U32 LLVOAvatar::renderSkinned(EAvatarRenderPass pass)
{
	U32 num_indices = 0;

	if (!mIsBuilt)
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
		mNeedsSkin = TRUE;
		mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
	}
	}

	if (LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) <= 0)
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
			mNeedsSkin = FALSE;
			mLastSkinTime = gFrameTimeSeconds;

			LLFace * face = mDrawable->getFace(0);
			if (face)
			{
				LLVertexBuffer* vb = face->getVertexBuffer();
			if (vb)
			{
				vb->flush();
			}
		}
	}
	}
	else
	{
		mNeedsSkin = FALSE;
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
			llinfos << "Avatar " << this << " in render" << llendl;
		}
		if (!mIsBuilt)
		{
			llinfos << "Not built!" << llendl;
		}
		else if (!gAgent.needsRenderAvatar())
		{
			llinfos << "Doesn't need avatar render!" << llendl;
		}
		else
		{
			llinfos << "Rendering!" << llendl;
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

	// render collision normal
	// *NOTE: this is disabled (there is no UI for enabling sShowFootPlane) due
	// to DEV-14477.  the code is left here to aid in tracking down the cause
	// of the crash in the future. -brad
	if (sShowFootPlane && mDrawable.notNull())
	{
		LLVector3 slaved_pos = mDrawable->getPositionAgent();
		LLVector3 foot_plane_normal(mFootPlane.mV[VX], mFootPlane.mV[VY], mFootPlane.mV[VZ]);
		F32 dist_from_plane = (slaved_pos * foot_plane_normal) - mFootPlane.mV[VW];
		LLVector3 collide_point = slaved_pos;
		collide_point.mV[VZ] -= foot_plane_normal.mV[VZ] * (dist_from_plane + COLLISION_TOLERANCE - FOOT_COLLIDE_FUDGE);

		gGL.begin(LLRender::LINES);
		{
			F32 SQUARE_SIZE = 0.2f;
			gGL.color4f(1.f, 0.f, 0.f, 1.f);
			
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);

			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			
			gGL.vertex3f(collide_point.mV[VX] + SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] + SQUARE_SIZE, collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] - SQUARE_SIZE, collide_point.mV[VY] - SQUARE_SIZE, collide_point.mV[VZ]);
			
			gGL.vertex3f(collide_point.mV[VX], collide_point.mV[VY], collide_point.mV[VZ]);
			gGL.vertex3f(collide_point.mV[VX] + mFootPlane.mV[VX], collide_point.mV[VY] + mFootPlane.mV[VY], collide_point.mV[VZ] + mFootPlane.mV[VZ]);

		}
		gGL.end();
		gGL.flush();
	}
	//--------------------------------------------------------------------
	// render all geometry attached to the skeleton
	//--------------------------------------------------------------------
	static LLStat render_stat;

	LLViewerJointMesh::sRenderPass = pass;

	if (pass == AVATAR_RENDER_PASS_SINGLE)
	{

		bool should_alpha_mask = shouldAlphaMask();
		LLGLState test(GL_ALPHA_TEST, should_alpha_mask);
		
		if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		
		BOOL first_pass = TRUE;
		if (!LLDrawPoolAvatar::sSkipOpaque)
		{
			if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
				if (isTextureVisible(TEX_HEAD_BAKED) || mIsDummy)
				{
					LLViewerJoint* head_mesh = getViewerJoint(MESH_ID_HEAD);
					if (head_mesh)
					{
						num_indices += head_mesh->render(mAdjustedPixelArea, TRUE, mIsDummy);
					}
					first_pass = FALSE;
				}
			}
			if (isTextureVisible(TEX_UPPER_BAKED) || mIsDummy)
			{
				LLViewerJoint* upper_mesh = getViewerJoint(MESH_ID_UPPER_BODY);
				if (upper_mesh)
				{
					num_indices += upper_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = FALSE;
			}
			
			if (isTextureVisible(TEX_LOWER_BAKED) || mIsDummy)
			{
				LLViewerJoint* lower_mesh = getViewerJoint(MESH_ID_LOWER_BODY);
				if (lower_mesh)
				{
					num_indices += lower_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
				}
				first_pass = FALSE;
			}
		}

		if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
		}

		if (!LLDrawPoolAvatar::sSkipTransparent || LLPipeline::sImpostorRender)
		{
			LLGLState blend(GL_BLEND, !mIsDummy);
			LLGLState test(GL_ALPHA_TEST, !mIsDummy);
			num_indices += renderTransparent(first_pass);
		}
	}
	
	LLViewerJointMesh::sRenderPass = AVATAR_RENDER_PASS_SINGLE;
	
	//llinfos << "Avatar render: " << render_timer.getElapsedTimeF32() << llendl;

	//render_stat.addValue(render_timer.getElapsedTimeF32()*1000.f);

	return num_indices;
}

U32 LLVOAvatar::renderTransparent(BOOL first_pass)
{
	U32 num_indices = 0;
	if( isWearingWearableType( LLWearableType::WT_SKIRT ) && (mIsDummy || isTextureVisible(TEX_SKIRT_BAKED)) )
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.25f);
		LLViewerJoint* skirt_mesh = getViewerJoint(MESH_ID_SKIRT);
		if (skirt_mesh)
		{
			num_indices += skirt_mesh->render(mAdjustedPixelArea, FALSE);
		}
		first_pass = FALSE;
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}

	if (!isSelf() || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
	{
		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		
		if (isTextureVisible(TEX_HEAD_BAKED))
		{
			LLViewerJoint* eyelash_mesh = getViewerJoint(MESH_ID_EYELASH);
			if (eyelash_mesh)
			{
				num_indices += eyelash_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
			}
			first_pass = FALSE;
		}
		// Can't test for baked hair being defined, since that won't always be the case (not all viewers send baked hair)
		// TODO: 1.25 will be able to switch this logic back to calling isTextureVisible();
		if ( getImage(TEX_HAIR_BAKED, 0) && 
		     getImage(TEX_HAIR_BAKED, 0)->getID() != IMG_INVISIBLE || LLDrawPoolAlpha::sShowDebugAlpha)		
		{
			LLViewerJoint* hair_mesh = getViewerJoint(MESH_ID_HAIR);
			if (hair_mesh)
			{
				num_indices += hair_mesh->render(mAdjustedPixelArea, first_pass, mIsDummy);
			}
			first_pass = FALSE;
		}
		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
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
	
	if (!mIsBuilt)
	{
		return 0;
	}

	bool should_alpha_mask = shouldAlphaMask();
	LLGLState test(GL_ALPHA_TEST, should_alpha_mask);

	if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
	}

	if (isTextureVisible(TEX_EYES_BAKED)  || mIsDummy)
	{
		LLViewerJoint* eyeball_left = getViewerJoint(MESH_ID_EYEBALL_LEFT);
		LLViewerJoint* eyeball_right = getViewerJoint(MESH_ID_EYEBALL_RIGHT);
		if (eyeball_left)
		{
			num_indices += eyeball_left->render(mAdjustedPixelArea, TRUE, mIsDummy);
		}
		if(eyeball_right)
		{
			num_indices += eyeball_right->render(mAdjustedPixelArea, TRUE, mIsDummy);
		}
	}

	if (should_alpha_mask && !LLGLSLShader::sNoFixedFunction)
	{
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
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

	LLGLEnable test(GL_ALPHA_TEST);
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.f);

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

	return 6;
}

bool LLVOAvatar::allTexturesCompletelyDownloaded(std::set<LLUUID>& ids) const
{
	for (std::set<LLUUID>::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it);
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

void LLVOAvatar::bakedTextureOriginCounts(S32 &sb_count, // server-bake, has origin URL.
										  S32 &host_count, // host-based bake, has host.
										  S32 &both_count, // error - both host and URL set.
										  S32 &neither_count) // error - neither set.
{
	sb_count = host_count = both_count = neither_count = 0;
	
	std::set<LLUUID> baked_ids;
	collectBakedTextureUUIDs(baked_ids);
	for (std::set<LLUUID>::const_iterator it = baked_ids.begin(); it != baked_ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it);
		bool has_url = false, has_host = false;
		if (!imagep->getUrl().empty())
		{
			has_url = true;
		}
		if (imagep->getTargetHost().isOk())
		{
			has_host = true;
		}
		if (has_url && !has_host) sb_count++;
		else if (has_host && !has_url) host_count++;
		else if (has_host && has_url) both_count++;
		else if (!has_host && !has_url) neither_count++;
	}
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
			LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), TRUE);
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

S32 LLVOAvatar::totalTextureMemForUUIDS(std::set<LLUUID>& ids)
{
	S32 result = 0;
	for (std::set<LLUUID>::const_iterator it = ids.begin(); it != ids.end(); ++it)
	{
		LLViewerFetchedTexture *imagep = gTextureList.findImage(*it);
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
		LLWearableType::EType wearable_type = LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex)texture_index);
		U32 num_wearables = gAgentWearables.getWearableCount(wearable_type);

		LLViewerFetchedTexture *imagep = NULL;
		for (U32 wearable_index = 0; wearable_index < num_wearables; wearable_index++)
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index, wearable_index), TRUE);
			if (imagep)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)texture_index);
				if (texture_dict->mIsLocalTexture)
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
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), TRUE);
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
	S32 current_texture_mem = 0;
	
	// Any textures that we used to be using but are no longer using should no longer be flagged as "NO_DELETE"
	std::set<LLUUID> baked_texture_ids;
	collectBakedTextureUUIDs(baked_texture_ids);
	S32 new_baked_mem = totalTextureMemForUUIDS(baked_texture_ids);

	std::set<LLUUID> local_texture_ids;
	collectLocalTextureUUIDs(local_texture_ids);
	//S32 new_local_mem = totalTextureMemForUUIDS(local_texture_ids);

	std::set<LLUUID> new_texture_ids;
	new_texture_ids.insert(baked_texture_ids.begin(),baked_texture_ids.end());
	new_texture_ids.insert(local_texture_ids.begin(),local_texture_ids.end());
	S32 new_total_mem = totalTextureMemForUUIDS(new_texture_ids);

	//S32 old_total_mem = totalTextureMemForUUIDS(mTextureIDs);
	//LL_DEBUGS("Avatar") << getFullname() << " old_total_mem: " << old_total_mem << " new_total_mem (L/B): " << new_total_mem << " (" << new_local_mem <<", " << new_baked_mem << ")" << llendl;  
	if (!isSelf() && new_total_mem > new_baked_mem)
	{
			llwarns << "extra local textures stored for non-self av" << llendl;
	}
	for (std::set<LLUUID>::iterator it = mTextureIDs.begin(); it != mTextureIDs.end(); ++it)
	{
		if (new_texture_ids.find(*it) == new_texture_ids.end())
		{
			LLViewerFetchedTexture *imagep = gTextureList.findImage(*it);
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
	
	BOOL render_avatar = TRUE;

	if (mIsDummy)
	{
		return;
	}

	if( isSelf() )
	{
		render_avatar = TRUE;
	}
	else
	{
		if(!isVisible())
		{
			return ;//do not update for invisible avatar.
		}

		render_avatar = !mCulled; //visible and not culled.
	}

	std::vector<BOOL> layer_baked;
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
	mHasGrey = FALSE; // debug
	for (U32 texture_index = 0; texture_index < getNumTEs(); texture_index++)
	{
		LLWearableType::EType wearable_type = LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex)texture_index);
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
			llwarns << "getTE( " << texture_index << " ) returned 0" <<llendl;
		}

		LLViewerFetchedTexture *imagep = NULL;
		for (U32 wearable_index = 0; wearable_index < num_wearables; wearable_index++)
		{
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index, wearable_index), TRUE);
			if (imagep)
			{
				const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)texture_index);
				const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
				if (texture_dict->mIsLocalTexture)
				{
					addLocalTextureStats((ETextureIndex)texture_index, imagep, texel_area_ratio, render_avatar, mBakedTextureDatas[baked_index].mIsUsed);
				}
			}
		}
		if (isIndexBakedTexture((ETextureIndex) texture_index) && render_avatar)
		{
			const S32 boost_level = getAvatarBakedBoostLevel();
			imagep = LLViewerTextureManager::staticCastToFetchedTexture(getImage(texture_index,0), TRUE);
			// Spam if this is a baked texture, not set to default image, without valid host info
			if (isIndexBakedTexture((ETextureIndex)texture_index)
				&& imagep->getID() != IMG_DEFAULT_AVATAR
				&& imagep->getID() != IMG_INVISIBLE
				&& !isUsingServerBakes() 
				&& !imagep->getTargetHost().isOk())
			{
				LL_WARNS_ONCE("Texture") << "LLVOAvatar::updateTextures No host for texture "
										 << imagep->getID() << " for avatar "
										 << (isSelf() ? "<myself>" : getID().asString()) 
										 << " on host " << getRegion()->getHost() << llendl;
			}

			addBakedTextureStats( imagep, mPixelArea, texel_area_ratio, boost_level );			
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%4.0f:%4.0f", (F32) sqrt(mMinPixelArea),(F32) sqrt(mMaxPixelArea)));
	}	
}


void LLVOAvatar::addLocalTextureStats( ETextureIndex idx, LLViewerFetchedTexture* imagep,
									   F32 texel_area_ratio, BOOL render_avatar, BOOL covered_by_baked, U32 index )
{
	// No local texture stats for non-self avatars
	return;
}

const S32 MAX_TEXTURE_UPDATE_INTERVAL = 64 ; //need to call updateTextures() at least every 32 frames.	
const S32 MAX_TEXTURE_VIRTURE_SIZE_RESET_INTERVAL = S32_MAX ; //frames
void LLVOAvatar::checkTextureLoading()
{
	static const F32 MAX_INVISIBLE_WAITING_TIME = 15.f ; //seconds

	BOOL pause = !isVisible() ;
	if(!pause)
	{
		mInvisibleTimer.reset() ;
	}
	if(mLoadedCallbacksPaused == pause)
	{
		return ; 
	}
	
	if(mCallbackTextureList.empty()) //when is self or no callbacks. Note: this list for self is always empty.
	{
		mLoadedCallbacksPaused = pause ;
		return ; //nothing to check.
	}
	
	if(pause && mInvisibleTimer.getElapsedTimeF32() < MAX_INVISIBLE_WAITING_TIME)
	{
		return ; //have not been invisible for enough time.
	}
	
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
const F32  ADDITIONAL_PRI = 0.5f;
void LLVOAvatar::addBakedTextureStats( LLViewerFetchedTexture* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level)
{
	//Note:
	//if this function is not called for the last MAX_TEXTURE_VIRTURE_SIZE_RESET_INTERVAL frames, 
	//the texture pipeline will stop fetching this texture.

	imagep->resetTextureStats();
	// TODO: currently default to HTTP texture and fall back to UDP if cannot be found there.
	// Once server messaging is in place, we should call setCanUseHTTP(false) for old style
	// appearance requests
	imagep->setCanUseHTTP(true);
	imagep->setMaxVirtualSizeResetInterval(MAX_TEXTURE_VIRTURE_SIZE_RESET_INTERVAL);
	imagep->resetMaxVirtualSizeResetCounter() ;

	mMaxPixelArea = llmax(pixel_area, mMaxPixelArea);
	mMinPixelArea = llmin(pixel_area, mMinPixelArea);	
	imagep->addTextureStats(pixel_area / texel_area_ratio);
	imagep->setBoostLevel(boost_level);
	
	if(boost_level != LLGLTexture::BOOST_AVATAR_BAKED_SELF)
	{
		imagep->setAdditionalDecodePriority(ADDITIONAL_PRI) ;
	}
	else
	{
		imagep->setAdditionalDecodePriority(SELF_ADDITIONAL_PRI) ;
	}
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
	if (isUsingServerBakes())
	{
		const std::string& appearance_service_url = LLAppearanceMgr::instance().getAppearanceServiceURL();
		if (appearance_service_url.empty())
		{
			// Probably a server-side issue if we get here:
			llwarns << "AgentAppearanceServiceURL not set - Baked texture requests will fail" << llendl;
			return url;
		}
	
		const LLAvatarAppearanceDictionary::TextureEntry* texture_entry = LLAvatarAppearanceDictionary::getInstance()->getTexture((ETextureIndex)te);
		if (texture_entry != NULL)
		{
			url = appearance_service_url + "texture/" + getID().asString() + "/" + texture_entry->mDefaultImageName + "/" + uuid.asString();
			//llinfos << "baked texture url: " << url << llendl;
		}
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
		mStepOnLand = TRUE;
		mStepMaterial = 0;
		mStepObjectVelocity.setVec(0.0f, 0.0f, 0.0f);
	}
	else
	{
		mStepOnLand = FALSE;
		mStepMaterial = obj->getMaterial();

		// We want the primitive velocity, not our velocity... (which actually subtracts the
		// step object velocity)
		LLVector3 angularVelocity = obj->getAngularVelocity();
		LLVector3 relativePos = gAgent.getPosAgentFromGlobal(outPos) - obj->getPositionAgent();

		LLVector3 linearComponent = angularVelocity % relativePos;
//		llinfos << "Linear Component of Rotation Velocity " << linearComponent << llendl;
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
	else if (mInAir && !mIsSitting)
	{
		stopMotion(ANIM_AGENT_WALK_ADJUST);
		startMotion(ANIM_AGENT_FLY_ADJUST);
	}
	else
	{
		stopMotion(ANIM_AGENT_WALK_ADJUST);
		stopMotion(ANIM_AGENT_FLY_ADJUST);
	}

	if ( isAnyAnimationSignaled(AGENT_GUN_AIM_ANIMS, NUM_AGENT_GUN_AIM_ANIMS) )
	{
		startMotion(ANIM_AGENT_TARGET);
		stopMotion(ANIM_AGENT_BODY_NOISE);
	}
	else
	{
		stopMotion(ANIM_AGENT_TARGET);
		startMotion(ANIM_AGENT_BODY_NOISE);
	}
	
	// clear all current animations
	AnimIterator anim_it;
	for (anim_it = mPlayingAnimations.begin(); anim_it != mPlayingAnimations.end();)
	{
		AnimIterator found_anim = mSignaledAnimations.find(anim_it->first);

		// playing, but not signaled, so stop
		if (found_anim == mSignaledAnimations.end())
		{
			processSingleAnimationStateChange(anim_it->first, FALSE);
			mPlayingAnimations.erase(anim_it++);
			continue;
		}

		++anim_it;
	}

	// start up all new anims
	for (anim_it = mSignaledAnimations.begin(); anim_it != mSignaledAnimations.end();)
	{
		AnimIterator found_anim = mPlayingAnimations.find(anim_it->first);

		// signaled but not playing, or different sequence id, start motion
		if (found_anim == mPlayingAnimations.end() || found_anim->second != anim_it->second)
		{
			if (processSingleAnimationStateChange(anim_it->first, TRUE))
			{
				mPlayingAnimations[anim_it->first] = anim_it->second;
				++anim_it;
				continue;
			}
		}

		++anim_it;
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
BOOL LLVOAvatar::processSingleAnimationStateChange( const LLUUID& anim_id, BOOL start )
{
	BOOL result = FALSE;

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
						LLUUID sound_id = LLUUID(gSavedSettings.getString("UISndTyping"));
						gAudiop->triggerSound(sound_id, getID(), 1.0f, LLAudioEngine::AUDIO_TYPE_SFX, char_pos_global);
					}
				}
			}
		}
		else if (anim_id == ANIM_AGENT_SIT_GROUND_CONSTRAINED)
		{
			sitDown(TRUE);
		}


		if (startMotion(anim_id))
		{
			result = TRUE;
		}
		else
		{
			llwarns << "Failed to start motion!" << llendl;
		}
	}
	else //stop animation
	{
		if (anim_id == ANIM_AGENT_SIT_GROUND_CONSTRAINED)
		{
			sitDown(FALSE);
		}
		stopMotion(anim_id);
		result = TRUE;
	}

	return result;
}

//-----------------------------------------------------------------------------
// isAnyAnimationSignaled()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims) const
{
	for (S32 i = 0; i < num_anims; i++)
	{
		if(mSignaledAnimations.find(anim_array[i]) != mSignaledAnimations.end())
		{
			return TRUE;
		}
	}
	return FALSE;
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
	BOOL use_new_walk_run = gSavedSettings.getBOOL("UseNewWalkRun");
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
	
	}

	return result;

}

//-----------------------------------------------------------------------------
// startMotion()
// id is the asset if of the animation to start
// time_offset is the offset into the animation at which to start playing
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::startMotion(const LLUUID& id, F32 time_offset)
{
	lldebugs << "motion requested " << id.asString() << " " << gAnimLibrary.animationName(id) << llendl;

	LLUUID remap_id = remapMotionID(id);

	if (remap_id != id)
	{
		lldebugs << "motion resultant " << remap_id.asString() << " " << gAnimLibrary.animationName(remap_id) << llendl;
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
BOOL LLVOAvatar::stopMotion(const LLUUID& id, BOOL stop_immediate)
{
	lldebugs << "motion requested " << id.asString() << " " << gAnimLibrary.animationName(id) << llendl;

	LLUUID remap_id = remapMotionID(id);
	
	if (remap_id != id)
	{
		lldebugs << "motion resultant " << remap_id.asString() << " " << gAnimLibrary.animationName(remap_id) << llendl;
	}

	if (isSelf())
	{
		gAgent.onAnimStop(remap_id);
	}

	return LLCharacter::stopMotion(remap_id, stop_immediate);
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
	{ //search for joint and cache found joint in lookup table
		jointp = mRoot->findJoint(name);
		mJointMap[name] = jointp;
	}
	else
	{ //return cached pointer
		jointp = iter->second;
	}

	return jointp;
}

//-----------------------------------------------------------------------------
// resetJointPositions
//-----------------------------------------------------------------------------
void LLVOAvatar::resetJointPositions( void )
{
	avatar_joint_list_t::iterator iter = mSkeleton.begin();
	avatar_joint_list_t::iterator end  = mSkeleton.end();
	for (; iter != end; ++iter)
	{
		(*iter)->restoreOldXform();
		(*iter)->setId( LLUUID::null );
	}
	mHasPelvisOffset = false;
	mPelvisFixup	 = mLastPelvisFixup;
}
//-----------------------------------------------------------------------------
// resetSpecificJointPosition
//-----------------------------------------------------------------------------
void LLVOAvatar::resetSpecificJointPosition( const std::string& name )
{
	LLJoint* pJoint = mRoot->findJoint( name );
	
	if ( pJoint  && pJoint->doesJointNeedToBeReset() )
	{
		pJoint->restoreOldXform();
		pJoint->setId( LLUUID::null );
		//If we're reseting the pelvis position make sure not to apply offset
		if ( name == "mPelvis" )
		{
			mHasPelvisOffset = false;
		}
	}
	else
	{
		llinfos<<"Did not find "<< name.c_str()<<llendl;
	}
}
//-----------------------------------------------------------------------------
// resetJointPositionsToDefault
//-----------------------------------------------------------------------------
void LLVOAvatar::resetJointPositionsToDefault( void )
{
	//Subsequent joints are relative to pelvis
	avatar_joint_list_t::iterator iter = mSkeleton.begin();
	avatar_joint_list_t::iterator end  = mSkeleton.end();
	for (; iter != end; ++iter)
	{
		LLJoint* pJoint = (*iter);
		if ( pJoint->doesJointNeedToBeReset() )
		{
			pJoint->setId( LLUUID::null );
			//restore joints to default positions, however skip over the pelvis
			// *TODO: How does this pointer check skip over pelvis?
			if ( pJoint )
			{
				pJoint->restoreOldXform();
			}
		}
	}
	//make sure we don't apply the joint offset
	mHasPelvisOffset = false;
	mPelvisFixup	 = mLastPelvisFixup;
	postPelvisSetRecalc();
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

	if (mIsDummy)
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
	return mTimeDilation;
}


//-----------------------------------------------------------------------------
// LLVOAvatar::getPixelArea()
//-----------------------------------------------------------------------------
F32 LLVOAvatar::getPixelArea() const
{
	if (mIsDummy)
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
BOOL LLVOAvatar::loadSkeletonNode ()
{
	if (!LLAvatarAppearance::loadSkeletonNode())
	{
		return FALSE;
	}

	// ATTACHMENTS
	{
		LLAvatarXmlInfo::attachment_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mAttachmentInfoList.begin();
			 iter != sAvatarXmlInfo->mAttachmentInfoList.end(); 
			 ++iter)
		{
			LLAvatarXmlInfo::LLAvatarAttachmentInfo *info = *iter;
			if (!isSelf() && info->mJointName == "mScreen")
			{ //don't process screen joint for other avatars
				continue;
			}

			LLViewerJointAttachment* attachment = new LLViewerJointAttachment();

			attachment->setName(info->mName);
			LLJoint *parentJoint = getJoint(info->mJointName);
			if (!parentJoint)
			{
				llwarns << "No parent joint by name " << info->mJointName << " found for attachment point " << info->mName << llendl;
				delete attachment;
				continue;
			}

			if (info->mHasPosition)
			{
				attachment->setOriginalPosition(info->mPosition);
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
				if (group < 0 || group >= 9)
				{
					llwarns << "Invalid group number (" << group << ") for attachment point " << info->mName << llendl;
				}
				else
				{
					attachment->setGroup(group);
				}
			}

			S32 attachmentID = info->mAttachmentID;
			if (attachmentID < 1 || attachmentID > 255)
			{
				llwarns << "Attachment point out of range [1-255]: " << attachmentID << " on attachment point " << info->mName << llendl;
				delete attachment;
				continue;
			}
			if (mAttachmentPoints.find(attachmentID) != mAttachmentPoints.end())
			{
				llwarns << "Attachment point redefined with id " << attachmentID << " on attachment point " << info->mName << llendl;
				delete attachment;
				continue;
			}

			attachment->setPieSlice(info->mPieMenuSlice);
			attachment->setVisibleInFirstPerson(info->mVisibleFirstPerson);
			attachment->setIsHUDAttachment(info->mIsHUDAttachment);

			mAttachmentPoints[attachmentID] = attachment;

			// now add attachment joint
			parentJoint->addChild(attachment);
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// updateVisualParams()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateVisualParams()
{
	setSex( (getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE );

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
BOOL LLVOAvatar::isActive() const
{
	return TRUE;
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
BOOL LLVOAvatar::updateJointLODs()
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
		BOOL res = FALSE;
		if (root)
		{
			res = root->updateLOD(mAdjustedPixelArea, TRUE);
		}
 		if (res)
		{
			sNumLODChangesThisFrame++;
			dirtyMesh(2);
			return TRUE;
		}

	return FALSE;
}

//-----------------------------------------------------------------------------
// createDrawable()
//-----------------------------------------------------------------------------
LLDrawable *LLVOAvatar::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);

	LLDrawPoolAvatar *poolp = (LLDrawPoolAvatar*) gPipeline.getPool(LLDrawPool::POOL_AVATAR);

	// Only a single face (one per avatar)
	//this face will be splitted into several if its vertex buffer is too long.
	mDrawable->setState(LLDrawable::ACTIVE);
	mDrawable->addFace(poolp, NULL);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_AVATAR);
	
	mNumInitFaces = mDrawable->getNumFaces() ;

	dirtyMesh(2);
	return mDrawable;
}


void LLVOAvatar::updateGL()
{
	if (mMeshTexturesDirty)
	{
		updateMeshTextures();
		mMeshTexturesDirty = FALSE;
	}
}

//-----------------------------------------------------------------------------
// updateGeometry()
//-----------------------------------------------------------------------------
static LLFastTimer::DeclareTimer FTM_UPDATE_AVATAR("Update Avatar");
BOOL LLVOAvatar::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(FTM_UPDATE_AVATAR);
 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_AVATAR)))
	{
		return TRUE;
	}
	
	if (!mMeshValid)
	{
		return TRUE;
	}

	if (!drawable)
	{
		llerrs << "LLVOAvatar::updateGeometry() called with NULL drawable" << llendl;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// updateSexDependentLayerSets()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateSexDependentLayerSets( BOOL upload_bake )
{
	invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet, upload_bake );
	invalidateComposite( mBakedTextureDatas[BAKED_UPPER].mTexLayerSet, upload_bake );
	invalidateComposite( mBakedTextureDatas[BAKED_LOWER].mTexLayerSet, upload_bake );
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
// hideSkirt()
//-----------------------------------------------------------------------------
void LLVOAvatar::hideSkirt()
{
	mMeshLOD[MESH_ID_SKIRT]->setVisible(FALSE, TRUE);
}

BOOL LLVOAvatar::setParent(LLViewerObject* parent)
{
	BOOL ret ;
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
	LLViewerObject::addChild(childp);
	if (childp->mDrawable)
	{
		attachObject(childp);
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
		llwarns << "Calling detach on non-attached object " << llendl;
	}
}

LLViewerJointAttachment* LLVOAvatar::getTargetAttachmentPoint(LLViewerObject* viewer_object)
{
	S32 attachmentID = ATTACHMENT_ID_FROM_STATE(viewer_object->getState());

	// This should never happen unless the server didn't process the attachment point
	// correctly, but putting this check in here to be safe.
	if (attachmentID & ATTACHMENT_ADD)
	{
		llwarns << "Got an attachment with ATTACHMENT_ADD mask, removing ( attach pt:" << attachmentID << " )" << llendl;
		attachmentID &= ~ATTACHMENT_ADD;
	}
	
	LLViewerJointAttachment* attachment = get_if_there(mAttachmentPoints, attachmentID, (LLViewerJointAttachment*)NULL);

	if (!attachment)
	{
		llwarns << "Object attachment point invalid: " << attachmentID << llendl;
		attachment = get_if_there(mAttachmentPoints, 1, (LLViewerJointAttachment*)NULL); // Arbitrary using 1 (chest)
	}

	return attachment;
}

//-----------------------------------------------------------------------------
// attachObject()
//-----------------------------------------------------------------------------
const LLViewerJointAttachment *LLVOAvatar::attachObject(LLViewerObject *viewer_object)
{
	LLViewerJointAttachment* attachment = getTargetAttachmentPoint(viewer_object);

	if (!attachment || !attachment->addObject(viewer_object))
	{
		return 0;
	}

	if (viewer_object->isSelected())
	{
		LLSelectMgr::getInstance()->updateSelectionCenter();
		LLSelectMgr::getInstance()->updatePointAt();
	}

	return attachment;
}

//-----------------------------------------------------------------------------
// attachObject()
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
// canAttachMoreObjects()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::canAttachMoreObjects() const
{
	return (getNumAttachments() < MAX_AGENT_ATTACHMENTS);
}

//-----------------------------------------------------------------------------
// canAttachMoreObjects()
// Returns true if we can attach <n> more objects.
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::canAttachMoreObjects(U32 n) const
{
	return (getNumAttachments() + n) <= MAX_AGENT_ATTACHMENTS;
}

//-----------------------------------------------------------------------------
// lazyAttach()
//-----------------------------------------------------------------------------
void LLVOAvatar::lazyAttach()
{
	std::vector<LLPointer<LLViewerObject> > still_pending;
	
	for (U32 i = 0; i < mPendingAttachment.size(); i++)
	{
		if (mPendingAttachment[i]->mDrawable)
		{
			attachObject(mPendingAttachment[i]);
		}
		else
		{
			still_pending.push_back(mPendingAttachment[i]);
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
				const LLViewerObject* attached_object = (*attachment_iter);
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
	//If a VO has a skin that we'll reset the joint positions to their default
	if ( pVO && pVO->mDrawable )
	{
		LLVOVolume* pVObj = pVO->mDrawable->getVOVolume();
		if ( pVObj )
		{
			const LLMeshSkinInfo* pSkinData = gMeshRepo.getSkinInfo( pVObj->getVolume()->getParams().getSculptID(), pVObj );
			if (pSkinData 
				&& pSkinData->mJointNames.size() > 20				// full rig
				&& pSkinData->mAlternateBindMatrix.size() > 0)
					{
						LLVOAvatar::resetJointPositionsToDefault();
						//Need to handle the repositioning of the cam, updating rig data etc during outfit editing 
						//This handles the case where we detach a replacement rig.
						if ( gAgentCamera.cameraCustomizeAvatar() )
						{
							gAgent.unpauseAnimation();
							//Still want to refocus on head bone
							gAgentCamera.changeCameraToCustomizeAvatar();
						}
					}
				}
			}				
		}
//-----------------------------------------------------------------------------
// detachObject()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::detachObject(LLViewerObject *viewer_object)
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		
		if (attachment->isObjectAttached(viewer_object))
		{
			cleanupAttachedMesh( viewer_object );
			attachment->removeObject(viewer_object);
			lldebugs << "Detaching object " << viewer_object->mID << " from " << attachment->getName() << llendl;
			return TRUE;
		}
	}

	std::vector<LLPointer<LLViewerObject> >::iterator iter = std::find(mPendingAttachment.begin(), mPendingAttachment.end(), viewer_object);
	if (iter != mPendingAttachment.end())
	{
		mPendingAttachment.erase(iter);
		return TRUE;
	}
	
	return FALSE;
}

//-----------------------------------------------------------------------------
// sitDown()
//-----------------------------------------------------------------------------
void LLVOAvatar::sitDown(BOOL bSitting)
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

		gAgent.setFlying(FALSE);
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

	gPipeline.markMoved(mDrawable, TRUE);
	// Notice that removing sitDown() from here causes avatars sitting on
	// objects to be not rendered for new arrivals. See EXT-6835 and EXT-1655.
	sitDown(TRUE);
	mRoot->getXform()->setParent(&sit_object->mDrawable->mXform); // LLVOAvatar::sitOnObject
	mRoot->setPosition(getPosition());
	mRoot->updateWorldMatrixChildren();

	stopMotion(ANIM_AGENT_BODY_NOISE);

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
		LLFollowCamMgr::setCameraActive(sit_object->getID(), FALSE);

		LLViewerObject::const_child_list_t& child_list = sit_object->getChildren();
		for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
			 iter != child_list.end(); ++iter)
		{
			LLViewerObject* child_objectp = *iter;

			stopMotionFromSource(child_objectp->getID());
			LLFollowCamMgr::setCameraActive(child_objectp->getID(), FALSE);
		}
	}

	// assumes that transform will not be updated with drawable still having a parent
	LLVector3 cur_position_world = mDrawable->getWorldPosition();
	LLQuaternion cur_rotation_world = mDrawable->getWorldRotation();

	// set *local* position based on last *world* position, since we're unparenting the avatar
	mDrawable->mXform.setPosition(cur_position_world);
	mDrawable->mXform.setRotation(cur_rotation_world);	
	
	gPipeline.markMoved(mDrawable, TRUE);

	sitDown(FALSE);

	mRoot->getXform()->setParent(NULL); // LLVOAvatar::getOffObject
	mRoot->setPosition(cur_position_world);
	mRoot->setRotation(cur_rotation_world);
	mRoot->getXform()->update();

	startMotion(ANIM_AGENT_BODY_NOISE);

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

// warning: order(N) not order(1)
S32 LLVOAvatar::getAttachmentCount()
{
	S32 count = mAttachmentPoints.size();
	return count;
}

BOOL LLVOAvatar::isWearingWearableType(LLWearableType::EType type) const
{
	if (mIsDummy) return TRUE;

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
			return TRUE;  // everyone has all bodyparts
		default:
			break; // Do nothing
	}

	/* switch(type)
		case LLWearableType::WT_SHIRT:
			indicator_te = TEX_UPPER_SHIRT; */
	for (LLAvatarAppearanceDictionary::Textures::const_iterator tex_iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
		 tex_iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
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
				return isTextureDefined(LLAvatarAppearanceDictionary::getInstance()->getBakedTexture(baked_index)->mTextureIndex);
			}
			return FALSE;
		}
	}
	return FALSE;
}



// virtual
void LLVOAvatar::invalidateComposite( LLTexLayerSet* layerset, BOOL upload_result )
{
}

void LLVOAvatar::invalidateAll()
{
}

// virtual
void LLVOAvatar::onGlobalColorChanged(const LLTexGlobalColor* global_color, BOOL upload_bake )
{
	if (global_color == mTexSkinColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet, upload_bake );
		invalidateComposite( mBakedTextureDatas[BAKED_UPPER].mTexLayerSet, upload_bake );
		invalidateComposite( mBakedTextureDatas[BAKED_LOWER].mTexLayerSet, upload_bake );
	}
	else if (global_color == mTexHairColor)
	{
		invalidateComposite( mBakedTextureDatas[BAKED_HEAD].mTexLayerSet, upload_bake );
		invalidateComposite( mBakedTextureDatas[BAKED_HAIR].mTexLayerSet, upload_bake );
		
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
//		llinfos << "invalidateComposite cause: onGlobalColorChanged( eyecolor )" << llendl; 
		invalidateComposite( mBakedTextureDatas[BAKED_EYES].mTexLayerSet,  upload_bake );
	}
	updateMeshTextures();
}

BOOL LLVOAvatar::isVisible() const
{
	return mDrawable.notNull()
		&& (mDrawable->isVisible() || mIsDummy);
}

// Determine if we have enough avatar data to render
BOOL LLVOAvatar::getIsCloud() const
{
	// Do we have a shape?
	if ((const_cast<LLVOAvatar*>(this))->visualParamWeightsAreDefault())
	{
		return TRUE;
	}

	if (!isTextureDefined(TEX_LOWER_BAKED) || 
		!isTextureDefined(TEX_UPPER_BAKED) || 
		!isTextureDefined(TEX_HEAD_BAKED))
	{
		return TRUE;
	}

	if (isTooComplex())
	{
		return TRUE;
	}
	return FALSE;
}

void LLVOAvatar::updateRezzedStatusTimers()
{
	// State machine for rezzed status. Statuses are -1 on startup, 0
	// = cloud, 1 = gray, 2 = textured, 3 = textured_and_downloaded.
	// Purpose is to collect time data for each it takes avatar to reach
	// various loading landmarks: gray, textured (partial), textured fully.

	S32 rez_status = getRezzedStatus();
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
	F32 elapsed;
	bool completed;
	if (getPhases().getPhaseValues(phase_name, elapsed, completed))
	{
		if (!completed)
		{
			LL_DEBUGS("Avatar") << avString() << "no-op, start when started already for " << phase_name << llendl;
			return;
		}
	}
	LL_DEBUGS("Avatar") << "started phase " << phase_name << llendl;
	getPhases().startPhase(phase_name);
}

void LLVOAvatar::stopPhase(const std::string& phase_name, bool err_check)
{
	F32 elapsed;
	bool completed;
	if (getPhases().getPhaseValues(phase_name, elapsed, completed))
	{
		if (!completed)
		{
			getPhases().stopPhase(phase_name);
			completed = true;
			logMetricsTimerRecord(phase_name, elapsed, completed);
			LL_DEBUGS("Avatar") << avString() << "stopped phase " << phase_name << " elapsed " << elapsed << llendl;
		}
		else
		{
			if (err_check)
			{
				LL_DEBUGS("Avatar") << "no-op, stop when stopped already for " << phase_name << llendl;
			}
		}
	}
	else
	{
		if (err_check)
		{
			LL_DEBUGS("Avatar") << "no-op, stop when not started for " << phase_name << llendl;
		}
	}
}

void LLVOAvatar::logPendingPhases()
{
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
	LLSD record;
	record["timer_name"] = phase_name;
	record["avatar_id"] = getID();
	record["elapsed"] = elapsed;
	record["completed"] = completed;
	U32 grid_x(0), grid_y(0);
	if (getRegion())
	{
		record["central_bake_version"] = LLSD::Integer(getRegion()->getCentralBakeVersion());
		grid_from_region_handle(getRegion()->getHandle(), &grid_x, &grid_y);
	}
	record["grid_x"] = LLSD::Integer(grid_x);
	record["grid_y"] = LLSD::Integer(grid_y);
	record["is_using_server_bakes"] = ((bool) isUsingServerBakes());
	record["is_self"] = isSelf();
	

#if 0 // verbose logging
	std::ostringstream ostr;
	ostr << LLSDNotationStreamer(record);
	LL_DEBUGS("Avatar") << "record\n" << ostr.str() << llendl;
#endif

	if (isAgentAvatarValid())
	{
		gAgentAvatarp->addMetricsTimerRecord(record);
	}
}

// call periodically to keep isFullyLoaded up to date.
// returns true if the value has changed.
BOOL LLVOAvatar::updateIsFullyLoaded()
{
	const BOOL loading = getIsCloud();
	updateRezzedStatusTimers();
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

BOOL LLVOAvatar::processFullyLoadedChange(bool loading)
{
	// we wait a little bit before giving the all clear,
	// to let textures settle down
	const F32 PAUSE = 1.f;
	if (loading)
		mFullyLoadedTimer.reset();
	
	mFullyLoaded = (mFullyLoadedTimer.getElapsedTimeF32() > PAUSE);

		if (!mPreviousFullyLoaded && !loading && mFullyLoaded)
		{
		debugAvatarRezTime("AvatarRezNotification","fully loaded");
	}

	// did our loading state "change" from last call?
	// runway - why are we updating every 30 calls even if nothing has changed?
	const S32 UPDATE_RATE = 30;
	BOOL changed =
		((mFullyLoaded != mPreviousFullyLoaded) ||         // if the value is different from the previous call
		 (!mFullyLoadedInitialized) ||                     // if we've never been called before
		 (mFullyLoadedFrameCounter % UPDATE_RATE == 0));   // every now and then issue a change

	mPreviousFullyLoaded = mFullyLoaded;
	mFullyLoadedInitialized = TRUE;
	mFullyLoadedFrameCounter++;
	
	return changed;
}

BOOL LLVOAvatar::isFullyLoaded() const
{
	return (mRenderUnloadedAvatar || mFullyLoaded);
}

bool LLVOAvatar::isTooComplex() const
{
	if (gSavedSettings.getS32("RenderAvatarComplexityLimit") > 0 && mVisualComplexity >= gSavedSettings.getS32("RenderAvatarComplexityLimit"))
	{
		return true;
	}

	return false;
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
				{
					mesh->setColor(color);
				}
			}
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

	const BOOL other_culled = !isSelf() && mCulled;
	LLLoadedCallbackEntry::source_callback_list_t* src_callback_list = NULL ;
	BOOL paused = FALSE;
	if(!isSelf())
	{
		src_callback_list = &mCallbackTextureList ;
		paused = !isVisible();
	}

	std::vector<BOOL> is_layer_baked;
	is_layer_baked.resize(mBakedTextureDatas.size(), false);

	std::vector<BOOL> use_lkg_baked_layer; // lkg = "last known good"
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
				layerset->setUpdatesEnabled(TRUE);
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
			LLViewerFetchedTexture* baked_img = LLViewerTextureManager::getFetchedTexture(mBakedTextureDatas[i].mLastTextureID);
			mBakedTextureDatas[i].mIsUsed = TRUE;

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
			LLViewerFetchedTexture* baked_img =
				LLViewerTextureManager::staticCastToFetchedTexture(
					getImage( mBakedTextureDatas[i].mTextureIndex, 0 ), TRUE) ;
			if( baked_img->getID() == mBakedTextureDatas[i].mLastTextureID )
			{
				// Even though the file may not be finished loading,
				// we'll consider it loaded and use it (rather than
				// doing compositing).
				useBakedTexture( baked_img->getID() );
			}
			else
			{
				mBakedTextureDatas[i].mIsLoaded = FALSE;
				if ( (baked_img->getID() != IMG_INVISIBLE) &&
					 ((i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER)) )
				{			
					baked_img->setLoadedCallback(onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, TRUE, TRUE, new LLTextureMaskData( mID ), 
						src_callback_list, paused);	
				}
				baked_img->setLoadedCallback(onBakedTextureLoaded, SWITCH_TO_BAKED_DISCARD, FALSE, FALSE, new LLUUID( mID ), 
					src_callback_list, paused );
			}
		}
		else if (layerset && isUsingLocalAppearance())
		{
			debugColorizeSubMeshes(i,LLColor4::yellow );

			layerset->createComposite();
			layerset->setUpdatesEnabled( TRUE );
			mBakedTextureDatas[i].mIsUsed = FALSE;

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
	if (!is_layer_baked[BAKED_HAIR] || isEditingAppearance())
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
			 LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().begin();
		 baked_iter != LLAvatarAppearanceDictionary::getInstance()->getBakedTextures().end();
		 ++baked_iter)
	{
		const EBakedTextureIndex baked_index = baked_iter->first;
		const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = baked_iter->second;
		
		for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
			 local_tex_iter != baked_dict->mLocalTextures.end();
			 ++local_tex_iter)
		{
			const ETextureIndex texture_index = *local_tex_iter;
			const BOOL is_baked_ready = (is_layer_baked[baked_index] && mBakedTextureDatas[baked_index].mIsLoaded) || other_culled;
			if (isSelf())
			{
				setBakedReady(texture_index, is_baked_ready);
			}
		}
	}
	removeMissingBakedTextures();
}

// virtual
//-----------------------------------------------------------------------------
// setLocalTexture()
//-----------------------------------------------------------------------------
void LLVOAvatar::setLocalTexture( ETextureIndex type, LLViewerTexture* in_tex, BOOL baked_version_ready, U32 index )
{
	// invalid for anyone but self
	llassert(0);
}

//virtual 
void LLVOAvatar::setBakedReady(LLAvatarAppearanceDefines::ETextureIndex type, BOOL baked_version_exists, U32 index)
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


void LLVOAvatar::applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index)
{
	if (index >= BAKED_NUM_INDICES)
	{
		llwarns << "invalid baked texture index passed to applyMorphMask" << llendl;
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


// returns TRUE if morph masks are present and not valid for a given baked texture, FALSE otherwise
BOOL LLVOAvatar::morphMaskNeedsUpdate(LLAvatarAppearanceDefines::EBakedTextureIndex index)
{
	if (index >= BAKED_NUM_INDICES)
	{
		return FALSE;
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
			return FALSE;
		}
	}

	return FALSE;
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
		const LLAvatarAppearanceDictionary::BakedEntry * bakedDicEntry = LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)baked_index);
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
	for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
		 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
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

BOOL LLVOAvatar::hasHUDAttachment() const
{
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		if (attachment->getIsHUDAttachment() && attachment->getNumObjects() > 0)
		{
			return TRUE;
		}
	}
	return FALSE;
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
				const LLViewerObject* attached_object = (*attachment_iter);
				if (attached_object == NULL)
				{
					llwarns << "HUD attached object is NULL!" << llendl;
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
	LL_INFOS("Avatar") << avString() << LL_ENDL;
	if( !mFirstTEMessageReceived )
	{
		mFirstTEMessageReceived = TRUE;

		LLLoadedCallbackEntry::source_callback_list_t* src_callback_list = NULL ;
		BOOL paused = FALSE ;
		if(!isSelf())
		{
			src_callback_list = &mCallbackTextureList ;
			paused = !isVisible();
		}

		for (U32 i = 0; i < mBakedTextureDatas.size(); i++)
		{
			const BOOL layer_baked = isTextureDefined(mBakedTextureDatas[i].mTextureIndex);

			// Use any baked textures that we have even if they haven't downloaded yet.
			// (That is, don't do a transition from unbaked to baked.)
			if (layer_baked)
			{
				LLViewerFetchedTexture* image = LLViewerTextureManager::staticCastToFetchedTexture(getImage( mBakedTextureDatas[i].mTextureIndex, 0 ), TRUE) ;
				mBakedTextureDatas[i].mLastTextureID = image->getID();
				// If we have more than one texture for the other baked layers, we'll want to call this for them too.
				if ( (image->getID() != IMG_INVISIBLE) && ((i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER)) )
				{
					image->setLoadedCallback( onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, TRUE, TRUE, new LLTextureMaskData( mID ), 
						src_callback_list, paused);
				}
				LL_DEBUGS("Avatar") << avString() << "layer_baked, setting onInitialBakedTextureLoaded as callback" << LL_ENDL;
				image->setLoadedCallback( onInitialBakedTextureLoaded, MAX_DISCARD_LEVEL, FALSE, FALSE, new LLUUID( mID ), 
					src_callback_list, paused );
			}
		}

		mMeshTexturesDirty = TRUE;
		gPipeline.markGLRebuild(this);
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
				//llinfos << "param '" << param->getName() << "'=" << param->getWeight() << " which differs from default=" << param->getDefaultWeight() << llendl;
				rtn = false;
				break;
			}
		}
	}

	//llinfos << "params are default ? " << int(rtn) << llendl;

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
	apr_file_printf(file, "\t\t<param id=\"%d\" name=\"%s\" value=\"%.3f\" u8=\"%d\" type=\"%s\" wearable=\"%s\"/>\n",
					viewer_param->getID(), viewer_param->getName().c_str(), value, u8_value, type_string.c_str(),
					LLWearableType::getTypeName(LLWearableType::EType(wtype)).c_str()
//					param_location_name(vparam->getParamLocation()).c_str()
		);
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
		LL_DEBUGS("Avatar") << "dumping appearance message to " << fullpath << llendl;
	}

	apr_file_printf(file, "<header>\n");
	apr_file_printf(file, "\t\t<cof_version %i />\n", contents.mCOFVersion);
	apr_file_printf(file, "\t\t<appearance_version %i />\n", contents.mAppearanceVersion);
	apr_file_printf(file, "</header>\n");

	apr_file_printf(file, "\n<params>\n");
	LLVisualParam* param = getFirstVisualParam();
	for (S32 i = 0; i < params_for_dump.size(); i++)
	{
		while( param && (param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) ) // should not be any of group VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
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
		llinfos << "appversion set by AppearanceData field: " << contents.mAppearanceVersion << llendl;
		mesgsys->getS32Fast(_PREHASH_AppearanceData, _PREHASH_CofVersion, contents.mCOFVersion, 0);
		// For future use:
		//mesgsys->getU32Fast(_PREHASH_AppearanceData, _PREHASH_Flags, appearance_flags, 0);
	}

	// Parse visual params, if any.
	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_VisualParam);
	bool drop_visual_params_debug = gSavedSettings.getBOOL("BlockSomeAvatarAppearanceVisualParams") && (ll_rand(2) == 0); // pretend that ~12% of AvatarAppearance messages arrived without a VisualParam block, for testing
	if( num_blocks > 1 && !drop_visual_params_debug)
	{
		LL_DEBUGS("Avatar") << avString() << " handle visual params, num_blocks " << num_blocks << LL_ENDL;
		
		LLVisualParam* param = getFirstVisualParam();
		llassert(param); // if this ever fires, we should do the same as when num_blocks<=1
		if (!param)
		{
			llwarns << "No visual params!" << llendl;
		}
		else
		{
			for( S32 i = 0; i < num_blocks; i++ )
			{
				while( param && (param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) ) // should not be any of group VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
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

		const S32 expected_tweakable_count = getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TWEAKABLE); // don't worry about VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
		if (num_blocks != expected_tweakable_count)
		{
			llinfos << "Number of params in AvatarAppearance msg (" << num_blocks << ") does not match number of tweakable params in avatar xml file (" << expected_tweakable_count << ").  Processing what we can.  object: " << getID() << llendl;
		}
	}
	else
	{
		if (drop_visual_params_debug)
		{
			llinfos << "Debug-faked lack of parameters on AvatarAppearance for object: "  << getID() << llendl;
		}
		else
		{
			llinfos << "AvatarAppearance msg received without any parameters, object: " << getID() << llendl;
		}
	}

	LLVisualParam* appearance_version_param = getVisualParam(11000);
	if (appearance_version_param)
	{
		std::vector<LLVisualParam*>::iterator it = std::find(contents.mParams.begin(), contents.mParams.end(),appearance_version_param);
		if (it != contents.mParams.end())
		{
			S32 index = it - contents.mParams.begin();
			llinfos << "index: " << index << llendl;
			contents.mParamAppearanceVersion = llround(contents.mParamWeights[index]);
			LL_DEBUGS("Avatar") << "appversion req by appearance_version param: " << contents.mParamAppearanceVersion << llendl;
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
		llwarns << "inconsistent appearance_version settings - field: " <<
			contents.mAppearanceVersion << ", param: " <<  contents.mParamAppearanceVersion << llendl;
		return false;
	}
	if (contents.mParamAppearanceVersion >= 0) // use visual param if available.
	{
		appearance_version = contents.mParamAppearanceVersion;
	}
	if (contents.mAppearanceVersion >= 0)
	{
		appearance_version = contents.mAppearanceVersion;
	}
	if (appearance_version < 0) // still not set, go with 0.
	{
		appearance_version = 0;
	}
	LL_DEBUGS("Avatar") << "appearance version info - field " << contents.mAppearanceVersion
						<< " param: " << contents.mParamAppearanceVersion
						<< " final: " << appearance_version << llendl;
	return true;
}

//-----------------------------------------------------------------------------
// processAvatarAppearance()
//-----------------------------------------------------------------------------
void LLVOAvatar::processAvatarAppearance( LLMessageSystem* mesgsys )
{
	LL_DEBUGS("Avatar") << "starts" << llendl;
	
	bool enable_verbose_dumps = gSavedSettings.getBOOL("DebugAvatarAppearanceMessage");
	std::string dump_prefix = getFullname() + "_" + (isSelf()?"s":"o") + "_";
	//if (enable_verbose_dumps) { dumpArchetypeXML(dump_prefix + "process_start"); }
	if (gSavedSettings.getBOOL("BlockAvatarAppearanceMessages"))
	{
		llwarns << "Blocking AvatarAppearance message" << llendl;
		return;
	}

	ESex old_sex = getSex();

	LLAppearanceMessageContents contents;
	parseAppearanceMessage(mesgsys, contents);
	if (enable_verbose_dumps)
	{
		dumpAppearanceMsgParams(dump_prefix + "appearance_msg", contents);
	}

	S32 appearance_version;
	if (!resolve_appearance_version(contents, appearance_version))
	{
		llwarns << "bad appearance version info, discarding" << llendl;
		return;
	}
	S32 this_update_cof_version = contents.mCOFVersion;
	S32 last_update_request_cof_version = mLastUpdateRequestCOFVersion;

	// Only now that we have result of appearance_version can we decide whether to bail out.
	if( isSelf() )
	{
		LL_DEBUGS("Avatar") << "this_update_cof_version " << this_update_cof_version
				<< " last_update_request_cof_version " << last_update_request_cof_version
				<<  " my_cof_version " << LLAppearanceMgr::instance().getCOFVersion() << llendl;

		if (getRegion() && (getRegion()->getCentralBakeVersion()==0))
		{
			llwarns << avString() << "Received AvatarAppearance message for self in non-server-bake region" << llendl;
		}
		if( mFirstTEMessageReceived && (appearance_version == 0))
		{
			return;
		}
	}
	else
	{
		LL_DEBUGS("Avatar") << "appearance message received" << llendl;
	}

	// Check for stale update.
	if (isSelf()
		&& (appearance_version>0)
		&& (this_update_cof_version < last_update_request_cof_version))
	{
		llwarns << "Stale appearance update, wanted version " << last_update_request_cof_version
				<< ", got " << this_update_cof_version << llendl;
		return;
	}

	if (isSelf() && isEditingAppearance())
	{
		llinfos << "ignoring appearance message while in appearance edit" << llendl;
		return;
	}

	S32 num_params = contents.mParamWeights.size();
	if (num_params <= 1)
	{
		// In this case, we have no reliable basis for knowing
		// appearance version, which may cause us to look for baked
		// textures in the wrong place and flag them as missing
		// assets.
		llinfos << "ignoring appearance message due to lack of params" << llendl;
		return;
	}

	mLastUpdateReceivedCOFVersion = this_update_cof_version;
		
	setIsUsingServerBakes(appearance_version > 0);

	applyParsedTEMessage(contents.mTEContents);

	// prevent the overwriting of valid baked textures with invalid baked textures
	for (U8 baked_index = 0; baked_index < mBakedTextureDatas.size(); baked_index++)
	{
		if (!isTextureDefined(mBakedTextureDatas[baked_index].mTextureIndex) 
			&& mBakedTextureDatas[baked_index].mLastTextureID != IMG_DEFAULT
			&& baked_index != BAKED_SKIRT)
		{
			setTEImage(mBakedTextureDatas[baked_index].mTextureIndex, 
				LLViewerTextureManager::getFetchedTexture(mBakedTextureDatas[baked_index].mLastTextureID, FTT_DEFAULT, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
		}
	}

	// runway - was
	// if (!is_first_appearance_message )
	// which means it would be called on second appearance message - probably wrong.
	BOOL is_first_appearance_message = !mFirstAppearanceMessageReceived;
	mFirstAppearanceMessageReceived = TRUE;

	LL_INFOS("Avatar") << avString() << "processAvatarAppearance start " << mID
			<< " first? " << is_first_appearance_message << " self? " << isSelf() << LL_ENDL;

	if (is_first_appearance_message )
	{
		onFirstTEMessageReceived();
	}

	setCompositeUpdatesEnabled( FALSE );
	gPipeline.markGLRebuild(this);

	// Apply visual params
	if( num_params > 1)
	{
		LL_DEBUGS("Avatar") << avString() << " handle visual params, num_params " << num_params << LL_ENDL;
		BOOL params_changed = FALSE;
		BOOL interp_params = FALSE;
		
		for( S32 i = 0; i < num_params; i++ )
		{
			LLVisualParam* param = contents.mParams[i];
			F32 newWeight = contents.mParamWeights[i];

			if (is_first_appearance_message || (param->getWeight() != newWeight))
			{
				params_changed = TRUE;
				if(is_first_appearance_message)
				{
					param->setWeight(newWeight, FALSE);
				}
				else
				{
					interp_params = TRUE;
					param->setAnimationTarget(newWeight, FALSE);
				}
			}
		}
		const S32 expected_tweakable_count = getVisualParamCountInGroup(VISUAL_PARAM_GROUP_TWEAKABLE); // don't worry about VISUAL_PARAM_GROUP_TWEAKABLE_NO_TRANSMIT
		if (num_params != expected_tweakable_count)
		{
			llinfos << "Number of params in AvatarAppearance msg (" << num_params << ") does not match number of tweakable params in avatar xml file (" << expected_tweakable_count << ").  Processing what we can.  object: " << getID() << llendl;
		}

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
				updateSexDependentLayerSets( FALSE );
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
			llinfos << "Re-requesting AvatarAppearance for object: "  << getID() << llendl;
			LLAvatarPropertiesProcessor::getInstance()->sendAvatarTexturesRequest(getID());
			mRuthTimer.reset();
		}
		else
		{
			llinfos << "That's okay, we already have a non-default shape for object: "  << getID() << llendl;
			// we don't really care.
		}
	}

	setCompositeUpdatesEnabled( TRUE );

	// If all of the avatars are completely baked, release the global image caches to conserve memory.
	LLVOAvatar::cullAvatarsByPixelArea();

	if (isSelf())
	{
		mUseLocalAppearance = false;
	}

	updateMeshTextures();

	//if (enable_verbose_dumps) dumpArchetypeXML(dump_prefix + "process_end");
}

// static
void LLVOAvatar::getAnimLabels( LLDynamicArray<std::string>* labels )
{
	S32 i;
	for( i = 0; i < gUserAnimStatesCount; i++ )
	{
		labels->put( LLAnimStateLabels::getStateLabel( gUserAnimStates[i].mName ) );
	}

	// Special case to trigger away (AFK) state
	labels->put( "Away From Keyboard" );
}

// static 
void LLVOAvatar::getAnimNames( LLDynamicArray<std::string>* names )
{
	S32 i;

	for( i = 0; i < gUserAnimStatesCount; i++ )
	{
		names->put( std::string(gUserAnimStates[i].mName) );
	}

	// Special case to trigger away (AFK) state
	names->put( "enter_away_from_keyboard_state" );
}

// static
void LLVOAvatar::onBakedTextureMasksLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	if (!userdata) return;

	//llinfos << "onBakedTextureMasksLoaded: " << src_vi->getID() << llendl;
	const LLUUID id = src_vi->getID();
 
	LLTextureMaskData* maskData = (LLTextureMaskData*) userdata;
	LLVOAvatar* self = (LLVOAvatar*) gObjectList.findObject( maskData->mAvatarID );

	// if discard level is 2 less than last discard level we processed, or we hit 0,
	// then generate morph masks
	if(self && success && (discard_level < maskData->mLastDiscardLevel - 2 || discard_level == 0))
	{
		if(aux_src && aux_src->getComponents() == 1)
		{
			if (!aux_src->getData())
			{
				llerrs << "No auxiliary source (morph mask) data for image id " << id << llendl;
				return;
			}

			U32 gl_name;
			LLImageGL::generateTextures(LLTexUnit::TT_TEXTURE, GL_ALPHA8, 1, &gl_name );
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
				     //llinfos << "onBakedTextureMasksLoaded for head " << id << " discard = " << discard_level << llendl;
					 self->mBakedTextureDatas[BAKED_HEAD].mTexLayerSet->applyMorphMask(aux_src->getData(), aux_src->getWidth(), aux_src->getHeight(), 1);
					 maskData->mLastDiscardLevel = discard_level; */
			BOOL found_texture_id = false;
			for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
				 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
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
							LLImageGL::deleteTextures(LLTexUnit::TT_TEXTURE, 0, -1, 1, &(self->mBakedTextureDatas[baked_index].mMaskTexName));
						}
						self->mBakedTextureDatas[baked_index].mMaskTexName = gl_name;
						found_texture_id = true;
						break;
					}
				}
			}
			if (!found_texture_id)
			{
				llinfos << "unexpected image id: " << id << llendl;
			}
			self->dirtyMesh();
		}
		else
		{
            // this can happen when someone uses an old baked texture possibly provided by 
            // viewer-side baked texture caching
			llwarns << "Masks loaded callback but NO aux source, id " << id << llendl;
		}
	}

	if (final || !success)
	{
		delete maskData;
	}
}

// static
void LLVOAvatar::onInitialBakedTextureLoaded( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{

	
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = (LLVOAvatar *)gObjectList.findObject(*avatar_idp);

	if (selfp)
	{
		LL_DEBUGS("Avatar") << selfp->avString() << "discard_level " << discard_level << " success " << success << " final " << final << LL_ENDL;
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
void LLVOAvatar::onBakedTextureLoaded(BOOL success,
									  LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* aux_src,
									  S32 discard_level, BOOL final, void* userdata)
{
	LL_DEBUGS("Avatar") << "onBakedTextureLoaded: " << src_vi->getID() << LL_ENDL;

	LLUUID id = src_vi->getID();
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = (LLVOAvatar *)gObjectList.findObject(*avatar_idp);
	if (selfp)
	{	
		LL_DEBUGS("Avatar") << selfp->avString() << "discard_level " << discard_level << " success " << success << " final " << final << " id " << src_vi->getID() << LL_ENDL;
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
			LL_DEBUGS("Avatar") << avString() << " i " << i << " id " << id << LL_ENDL;
			mBakedTextureDatas[i].mIsLoaded = true;
			mBakedTextureDatas[i].mLastTextureID = id;
			mBakedTextureDatas[i].mIsUsed = true;

			if (isUsingLocalAppearance())
			{
				llinfos << "not changing to baked texture while isUsingLocalAppearance" << llendl;
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
				LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)i);
			for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
				 local_tex_iter != baked_dict->mLocalTextures.end();
				 ++local_tex_iter)
			{
				if (isSelf()) setBakedReady(*local_tex_iter, TRUE);
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
	std::string fullpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS,outfilename);
	outfile.open(fullpath, LL_APR_WB );
	apr_file_t* file = outfile.getFileHandle();
	if (!file)
	{
		return;
	}
	else
	{
		llinfos << "xmlfile write handle obtained : " << fullpath << llendl;
	}

	apr_file_printf( file, "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n" );
	apr_file_printf( file, "<linden_genepool version=\"1.0\">\n" );
	apr_file_printf( file, "\n\t<archetype name=\"???\">\n" );

	if (group_by_wearables)
	{
		for (S32 type = LLWearableType::WT_SHAPE; type < LLWearableType::WT_COUNT; type++)
		{
			const std::string& wearable_name = LLWearableType::getTypeName((LLWearableType::EType)type);
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
				if (LLAvatarAppearanceDictionary::getTEWearableType((ETextureIndex)te) == type)
				{
					// MULTIPLE_WEARABLES: extend to multiple wearables?
					LLViewerTexture* te_image = getImage((ETextureIndex)te, 0);
					if( te_image )
					{
						std::string uuid_str;
						te_image->getID().toString( uuid_str );
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
			{
				// MULTIPLE_WEARABLES: extend to multiple wearables?
				LLViewerTexture* te_image = getImage((ETextureIndex)te, 0);
				if( te_image )
				{
					std::string uuid_str;
					te_image->getID().toString( uuid_str );
					apr_file_printf( file, "\t\t<texture te=\"%i\" uuid=\"%s\"/>\n", te, uuid_str.c_str());
				}
			}
		}

	}
	apr_file_printf( file, "\t</archetype>\n" );
	apr_file_printf( file, "\n</linden_genepool>\n" );

	bool ultra_verbose = false;
	if (isSelf() && ultra_verbose)
	{
		// show the cloned params inside the wearables as well.
		gAgentAvatarp->dumpWearableInfo(outfile);
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
	BOOL operator()(const LLCharacter* const& lhs, const LLCharacter* const& rhs)
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
		BOOL culled;
		if (inst->isSelf() || inst->isFullyBaked())
		{
			culled = FALSE;
		}
		else 
		{
			culled = TRUE;
		}

		if (inst->mCulled != culled)
		{
			inst->mCulled = culled;
			lldebugs << "avatar " << inst->getID() << (culled ? " start culled" : " start not culled" ) << llendl;
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
			sUnbakedTime += gFrameIntervalSeconds;
		}
		if (grey_avatars > 0)
		{
			if (gFrameTimeSeconds != sGreyUpdateTime) // only update once per frame
			{
				sGreyUpdateTime = gFrameTimeSeconds;
				sGreyTime += gFrameIntervalSeconds;
			}
		}
	}
}

void LLVOAvatar::startAppearanceAnimation()
{
	if(!mAppearanceAnimating)
	{
		mAppearanceAnimating = TRUE;
		mAppearanceMorphTimer.reset();
		mLastAppearanceBlendTime = 0.f;
	}
}

//virtual
void LLVOAvatar::bodySizeChanged()
{
	if (isSelf() && !LLAppearanceMgr::instance().isInUpdateAppearanceFromCOF())
	{	// notify simulator of change in size
		// but not if we are in the middle of updating appearance
		gAgent.sendAgentSetAppearance();
	}
}

BOOL LLVOAvatar::isUsingServerBakes() const
{
#if 1
	// Sanity check - visual param for appearance version should match mUseServerBakes
	LLVisualParam* appearance_version_param = getVisualParam(11000);
	llassert(appearance_version_param);
	F32 wt = appearance_version_param->getWeight();
	F32 expect_wt = mUseServerBakes ? 1.0 : 0.0;
	if (!is_approx_equal(wt,expect_wt))
	{
		llwarns << "wt " << wt << " differs from expected " << expect_wt << llendl;
	}
#endif

	return mUseServerBakes;
}

void LLVOAvatar::setIsUsingServerBakes(BOOL newval)
{
	mUseServerBakes = newval;
	LLVisualParam* appearance_version_param = getVisualParam(11000);
	llassert(appearance_version_param);
	appearance_version_param->setWeight(newval ? 1.0 : 0.0, false);
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
		return LLHost::invalid;
	}
}

//static
void LLVOAvatar::updateFreezeCounter(S32 counter)
{
	if(counter)
	{
		sFreezeCounter = counter;
	}
	else if(sFreezeCounter > 0)
	{
		sFreezeCounter--;
	}
	else
	{
		sFreezeCounter = 0;
	}
}

BOOL LLVOAvatar::updateLOD()
{
	if (isImpostor())
	{
		return TRUE;
	}

	BOOL res = updateJointLODs();

	LLFace* facep = mDrawable->getFace(0);
	if (!facep || !facep->getVertexBuffer())
	{
		dirtyMesh(2);
	}

	if (mDirtyMesh >= 2 || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY))
	{	//LOD changed or new mesh created, allocate new vertex buffer if needed
		updateMeshData();
		mDirtyMesh = 0;
		mNeedsSkin = TRUE;
		mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
	}
	updateVisibility();

	return res;
}

void LLVOAvatar::updateLODRiggedAttachments( void )
{
	updateLOD();
	rebuildRiggedAttachments();
}
U32 LLVOAvatar::getPartitionType() const
{ 
	// Avatars merely exist as drawables in the bridge partition
	return LLViewerRegion::PARTITION_BRIDGE;
}

//static
void LLVOAvatar::updateImpostors() 
{
	LLCharacter::sAllowInstancesChange = FALSE ;

	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		 iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* avatar = (LLVOAvatar*) *iter;
		if (!avatar->isDead() && avatar->needsImpostorUpdate() && avatar->isVisible() && avatar->isImpostor())
		{
			gPipeline.generateImpostor(avatar);
		}
	}

	LLCharacter::sAllowInstancesChange = TRUE ;
}

BOOL LLVOAvatar::isImpostor() const
{
	return (isVisuallyMuted() || (sUseImpostors && mUpdatePeriod >= IMPOSTOR_PERIOD)) ? TRUE : FALSE;
}


BOOL LLVOAvatar::needsImpostorUpdate() const
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

void LLVOAvatar::idleUpdateRenderCost()
{
	static const U32 ARC_BODY_PART_COST = 200;
	static const U32 ARC_LIMIT = 20000;

	static std::set<LLUUID> all_textures;

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_ATTACHMENT_BYTES))
	{ //set debug text to attachment geometry bytes here so render cost will override
		setDebugText(llformat("%.1f KB, %.2f m^2", mAttachmentGeometryBytes/1024.f, mAttachmentSurfaceArea));
	}

	if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHAME))
	{
		return;
	}

	U32 cost = 0;
	LLVOVolume::texture_cost_t textures;

	for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
	{
		const LLAvatarAppearanceDictionary::BakedEntry *baked_dict = LLAvatarAppearanceDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)baked_index);
		ETextureIndex tex_index = baked_dict->mTextureIndex;
		if ((tex_index != TEX_SKIRT_BAKED) || (isWearingWearableType(LLWearableType::WT_SKIRT)))
		{
			if (isTextureVisible(tex_index))
			{
				cost +=ARC_BODY_PART_COST;
			}
		}
	}


	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end();
		 ++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator attachment_iter = attachment->mAttachedObjects.begin();
			 attachment_iter != attachment->mAttachedObjects.end();
			 ++attachment_iter)
		{
			const LLViewerObject* attached_object = (*attachment_iter);
			if (attached_object && !attached_object->isHUDAttachment())
			{
				textures.clear();
				const LLDrawable* drawable = attached_object->mDrawable;
				if (drawable)
				{
					const LLVOVolume* volume = drawable->getVOVolume();
					if (volume)
					{
						cost += volume->getRenderCost(textures);

						const_child_list_t children = volume->getChildren();
						for (const_child_list_t::const_iterator child_iter = children.begin();
							  child_iter != children.end();
							  ++child_iter)
						{
							LLViewerObject* child_obj = *child_iter;
							LLVOVolume *child = dynamic_cast<LLVOVolume*>( child_obj );
							if (child)
							{
								cost += child->getRenderCost(textures);
							}
						}

						for (LLVOVolume::texture_cost_t::iterator iter = textures.begin(); iter != textures.end(); ++iter)
						{
							// add the cost of each individual texture in the linkset
							cost += iter->second;
						}
					}
				}
			}
		}

	}



	// Diagnostic output to identify all avatar-related textures.
	// Does not affect rendering cost calculation.
	// Could be wrapped in a debug option if output becomes problematic.
	if (isSelf())
	{
		// print any attachment textures we didn't already know about.
		for (LLVOVolume::texture_cost_t::iterator it = textures.begin(); it != textures.end(); ++it)
		{
			LLUUID image_id = it->first;
			if( image_id.isNull() || image_id == IMG_DEFAULT || image_id == IMG_DEFAULT_AVATAR)
				continue;
			if (all_textures.find(image_id) == all_textures.end())
			{
				// attachment texture not previously seen.
				llinfos << "attachment_texture: " << image_id.asString() << llendl;
				all_textures.insert(image_id);
			}
		}

		// print any avatar textures we didn't already know about
		for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
			 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
			 ++iter)
		{
			const LLAvatarAppearanceDictionary::TextureEntry *texture_dict = iter->second;
			// TODO: MULTI-WEARABLE: handle multiple textures for self
			const LLViewerTexture* te_image = getImage(iter->first,0);
			if (!te_image)
				continue;
			LLUUID image_id = te_image->getID();
			if( image_id.isNull() || image_id == IMG_DEFAULT || image_id == IMG_DEFAULT_AVATAR)
				continue;
			if (all_textures.find(image_id) == all_textures.end())
			{
				llinfos << "local_texture: " << texture_dict->mName << ": " << image_id << llendl;
				all_textures.insert(image_id);
			}
		}
	}

	
	std::string viz_string = LLVOAvatar::rezStatusToString(getRezzedStatus());
	setDebugText(llformat("%s %d", viz_string.c_str(), cost));
	mVisualComplexity = cost;
	F32 green = 1.f-llclamp(((F32) cost-(F32)ARC_LIMIT)/(F32)ARC_LIMIT, 0.f, 1.f);
	F32 red = llmin((F32) cost/(F32)ARC_LIMIT, 1.f);
	mText->setColor(LLColor4(red,green,0,1));
}

// static
BOOL LLVOAvatar::isIndexLocalTexture(ETextureIndex index)
{
	if (index < 0 || index >= TEX_NUM_INDICES) return false;
	return LLAvatarAppearanceDictionary::getInstance()->getTexture(index)->mIsLocalTexture;
}

// static
BOOL LLVOAvatar::isIndexBakedTexture(ETextureIndex index)
{
	if (index < 0 || index >= TEX_NUM_INDICES) return false;
	return LLAvatarAppearanceDictionary::getInstance()->getTexture(index)->mIsBakedTexture;
}

const std::string LLVOAvatar::getBakedStatusForPrintout() const
{
	std::string line;

	for (LLAvatarAppearanceDictionary::Textures::const_iterator iter = LLAvatarAppearanceDictionary::getInstance()->getTextures().begin();
		 iter != LLAvatarAppearanceDictionary::getInstance()->getTextures().end();
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
BOOL LLVOAvatar::isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te, U32 index ) const
{
	if (isIndexLocalTexture(te)) 
	{
		return FALSE;
	}

	if( !getImage( te, index ) )
	{
		llwarns << "getImage( " << te << ", " << index << " ) returned 0" << llendl;
		return FALSE;
	}

	return (getImage(te, index)->getID() != IMG_DEFAULT_AVATAR && 
			getImage(te, index)->getID() != IMG_DEFAULT);
}

//virtual
BOOL LLVOAvatar::isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, U32 index) const
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
BOOL LLVOAvatar::isTextureVisible(LLAvatarAppearanceDefines::ETextureIndex type, LLViewerWearable *wearable) const
{
	// non-self avatars don't have wearables
	return FALSE;
}



