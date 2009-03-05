/** 
 * @file llvoavatar.cpp
 * @brief Implementation of LLVOAvatar class which is a derivation fo LLViewerObject
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llvoavatar.h"

#include <stdio.h>
#include <ctype.h>

#include "audioengine.h"
#include "noise.h"

#include "llagent.h" //  Get state values from here
#include "llviewercontrol.h"
#include "lldrawpoolavatar.h"
#include "lldriverparam.h"
#include "lleditingmotion.h"
#include "llemote.h"
#include "llfirstuse.h"
#include "llheadrotmotion.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llinventoryview.h"
#include "llkeyframefallmotion.h"
#include "llkeyframestandmotion.h"
#include "llkeyframewalkmotion.h"
#include "llmutelist.h"
#include "llselectmgr.h"
#include "llsprite.h"
#include "lltargetingmotion.h"
#include "lltexlayer.h"
#include "lltoolgrab.h"	// for needsRenderBeam
#include "lltoolmgr.h" // for needsRenderBeam
#include "lltoolmorph.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerstats.h"
#include "llvovolume.h"
#include "llworld.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llsky.h"
#include "llanimstatelabels.h"
#include "llgesturemgr.h" //needed to trigger the voice gesticulations
#include "llvoiceclient.h"
#include "llvoicevisualizer.h" // Ventrella

#include "boost/lexical_cast.hpp"

using namespace LLVOAvatarDefines;

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


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
const std::string AVATAR_DEFAULT_CHAR = "avatar";

const S32 MIN_PIXEL_AREA_FOR_COMPOSITE = 1024;
const F32 SHADOW_OFFSET_AMT = 0.03f;

const F32 DELTA_TIME_MIN = 0.01f;	// we clamp measured deltaTime to this
const F32 DELTA_TIME_MAX = 0.2f;	// range to insure stability of computations.

const F32 PELVIS_LAG_FLYING		= 0.22f;// pelvis follow half life while flying
const F32 PELVIS_LAG_WALKING	= 0.4f;	// ...while walking
const F32 PELVIS_LAG_MOUSELOOK = 0.15f;
const F32 MOUSELOOK_PELVIS_FOLLOW_FACTOR = 0.5f;
const F32 PELVIS_LAG_WHEN_FOLLOW_CAM_IS_ON = 0.0001f; // not zero! - something gets divided by this!

const F32 PELVIS_ROT_THRESHOLD_SLOW = 60.0f;	// amount of deviation allowed between
const F32 PELVIS_ROT_THRESHOLD_FAST = 2.0f;	// the pelvis and the view direction
											// when moving fast & slow
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
const S32 TEX_IMAGE_SIZE_OTHER = TEX_IMAGE_SIZE_SELF / 4;  // The size of local textures for other (!mIsSelf) avatars
const S32 TEX_IMAGE_AREA_OTHER = TEX_IMAGE_SIZE_OTHER * TEX_IMAGE_SIZE_OTHER;

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
const S32 MAX_BUBBLE_CHAT_LENGTH = 1023;
const S32 MAX_BUBBLE_CHAT_UTTERANCES = 12;
const F32 CHAT_FADE_TIME = 8.0;
const F32 BUBBLE_CHAT_TIME = CHAT_FADE_TIME * 3.f;

enum ERenderName
{
	RENDER_NAME_NEVER,
	RENDER_NAME_FADE,
	RENDER_NAME_ALWAYS
};

//-----------------------------------------------------------------------------
// Callback data
//-----------------------------------------------------------------------------
struct LLAvatarTexData
{
	LLAvatarTexData( const LLUUID& id, ETextureIndex index )
		: mAvatarID(id), mIndex(index) {}
	LLUUID				mAvatarID;
	ETextureIndex	mIndex;
};

struct LLTextureMaskData
{
	LLTextureMaskData( const LLUUID& id )
		: mAvatarID(id), mLastDiscardLevel(S32_MAX) {}
	LLUUID				mAvatarID;
	S32					mLastDiscardLevel;
};

/*********************************************************************************
 **                                                                             **
 ** Begin LLVOAvatar Support classes
 **
 **/

//------------------------------------------------------------------------
// LLVOBoneInfo
// Trans/Scale/Rot etc. info about each avatar bone.  Used by LLVOAvatarSkeleton.
//------------------------------------------------------------------------
class LLVOAvatarBoneInfo
{
	friend class LLVOAvatar;
	friend class LLVOAvatarSkeletonInfo;
public:
	LLVOAvatarBoneInfo() : mIsJoint(FALSE) {}
	~LLVOAvatarBoneInfo()
	{
		std::for_each(mChildList.begin(), mChildList.end(), DeletePointer());
	}
	BOOL parseXml(LLXmlTreeNode* node);
	
private:
	std::string mName;
	BOOL mIsJoint;
	LLVector3 mPos;
	LLVector3 mRot;
	LLVector3 mScale;
	LLVector3 mPivot;
	typedef std::vector<LLVOAvatarBoneInfo*> child_list_t;
	child_list_t mChildList;
};

//------------------------------------------------------------------------
// LLVOAvatarSkeletonInfo
// Overall avatar skeleton
//------------------------------------------------------------------------
class LLVOAvatarSkeletonInfo
{
	friend class LLVOAvatar;
public:
	LLVOAvatarSkeletonInfo() :
		mNumBones(0), mNumCollisionVolumes(0) {}
	~LLVOAvatarSkeletonInfo()
	{
		std::for_each(mBoneInfoList.begin(), mBoneInfoList.end(), DeletePointer());
	}
	BOOL parseXml(LLXmlTreeNode* node);
	S32 getNumBones() const { return mNumBones; }
	S32 getNumCollisionVolumes() const { return mNumCollisionVolumes; }
	
private:
	S32 mNumBones;
	S32 mNumCollisionVolumes;
	typedef std::vector<LLVOAvatarBoneInfo*> bone_info_list_t;
	bone_info_list_t mBoneInfoList;
};


//------------------------------------------------------------------------
// LLVOAvatarXmlInfo
// One instance (in LLVOAvatar) with common data parsed from the XML files
//------------------------------------------------------------------------
class LLVOAvatarXmlInfo
{
	friend class LLVOAvatar;
public:
	LLVOAvatarXmlInfo();
	~LLVOAvatarXmlInfo();
	
private:
	BOOL 	parseXmlSkeletonNode(LLXmlTreeNode* root);
	BOOL 	parseXmlMeshNodes(LLXmlTreeNode* root);
	BOOL 	parseXmlColorNodes(LLXmlTreeNode* root);
	BOOL 	parseXmlLayerNodes(LLXmlTreeNode* root);
	BOOL 	parseXmlDriverNodes(LLXmlTreeNode* root);
	
	struct LLVOAvatarMeshInfo
	{
		typedef std::pair<LLPolyMorphTargetInfo*,BOOL> morph_info_pair_t;
		typedef std::vector<morph_info_pair_t> morph_info_list_t;

		LLVOAvatarMeshInfo() : mLOD(0), mMinPixelArea(.1f) {}
		~LLVOAvatarMeshInfo()
		{
			morph_info_list_t::iterator iter;
			for (iter = mPolyMorphTargetInfoList.begin(); iter != mPolyMorphTargetInfoList.end(); iter++)
			{
				delete iter->first;
			}
			mPolyMorphTargetInfoList.clear();
		}

		std::string mType;
		S32			mLOD;
		std::string	mMeshFileName;
		std::string	mReferenceMeshName;
		F32			mMinPixelArea;
		morph_info_list_t mPolyMorphTargetInfoList;
	};
	typedef std::vector<LLVOAvatarMeshInfo*> mesh_info_list_t;
	mesh_info_list_t mMeshInfoList;

	typedef std::vector<LLPolySkeletalDistortionInfo*> skeletal_distortion_info_list_t;
	skeletal_distortion_info_list_t mSkeletalDistortionInfoList;
	
	struct LLVOAvatarAttachmentInfo
	{
		LLVOAvatarAttachmentInfo()
			: mGroup(-1), mAttachmentID(-1), mPieMenuSlice(-1), mVisibleFirstPerson(FALSE),
			  mIsHUDAttachment(FALSE), mHasPosition(FALSE), mHasRotation(FALSE) {}
		std::string mName;
		std::string mJointName;
		LLVector3 mPosition;
		LLVector3 mRotationEuler;
		S32 mGroup;
		S32 mAttachmentID;
		S32 mPieMenuSlice;
		BOOL mVisibleFirstPerson;
		BOOL mIsHUDAttachment;
		BOOL mHasPosition;
		BOOL mHasRotation;
	};
	typedef std::vector<LLVOAvatarAttachmentInfo*> attachment_info_list_t;
	attachment_info_list_t mAttachmentInfoList;
	
	LLTexGlobalColorInfo *mTexSkinColorInfo;
	LLTexGlobalColorInfo *mTexHairColorInfo;
	LLTexGlobalColorInfo *mTexEyeColorInfo;

	typedef std::vector<LLTexLayerSetInfo*> layer_info_list_t;
	layer_info_list_t mLayerInfoList;

	typedef std::vector<LLDriverParamInfo*> driver_info_list_t;
	driver_info_list_t mDriverInfoList;
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
		bool success = true;

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
LLXmlTree LLVOAvatar::sXMLTree;
LLXmlTree LLVOAvatar::sSkeletonXMLTree;
BOOL LLVOAvatar::sDebugAvatarRotation = FALSE;
LLVOAvatarSkeletonInfo* LLVOAvatar::sAvatarSkeletonInfo = NULL;
LLVOAvatarXmlInfo* LLVOAvatar::sAvatarXmlInfo = NULL;
LLVOAvatarDictionary *LLVOAvatar::sAvatarDictionary = NULL;
S32 LLVOAvatar::sFreezeCounter = 0;
S32 LLVOAvatar::sMaxVisible = 50;
LLMap< LLGLenum, LLGLuint*> LLVOAvatar::sScratchTexNames;
LLMap< LLGLenum, F32*> LLVOAvatar::sScratchTexLastBindTime;
S32 LLVOAvatar::sScratchTexBytes = 0;
F32 LLVOAvatar::sRenderDistance = 256.f;
S32	LLVOAvatar::sNumVisibleAvatars = 0;
S32	LLVOAvatar::sNumLODChangesThisFrame = 0;

const LLUUID LLVOAvatar::sStepSoundOnLand = LLUUID("e8af4a28-aa83-4310-a7c4-c047e15ea0df");
const LLUUID LLVOAvatar::sStepSounds[LL_MCODE_END] =
{
	LLUUID(SND_STONE_RUBBER),
	LLUUID(SND_METAL_RUBBER),
	LLUUID(SND_GLASS_RUBBER),
	LLUUID(SND_WOOD_RUBBER),
	LLUUID(SND_FLESH_RUBBER),
	LLUUID(SND_RUBBER_PLASTIC),
	LLUUID(SND_RUBBER_RUBBER)
};

S32 LLVOAvatar::sRenderName = RENDER_NAME_ALWAYS;
BOOL LLVOAvatar::sRenderGroupTitles = TRUE;
S32 LLVOAvatar::sNumVisibleChatBubbles = 0;
BOOL LLVOAvatar::sDebugInvisible = FALSE;
BOOL LLVOAvatar::sShowAttachmentPoints = FALSE;
BOOL LLVOAvatar::sShowAnimationDebug = FALSE;
BOOL LLVOAvatar::sShowFootPlane = FALSE;
BOOL LLVOAvatar::sShowCollisionVolumes = FALSE;
BOOL LLVOAvatar::sVisibleInFirstPerson = FALSE;
F32 LLVOAvatar::sLODFactor = 1.f;
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
static U32 calc_shame(LLVOVolume* volume, std::set<LLUUID> &textures);

//-----------------------------------------------------------------------------
// LLVOAvatar()
//-----------------------------------------------------------------------------
LLVOAvatar::LLVOAvatar(const LLUUID& id,
					   const LLPCode pcode,
					   LLViewerRegion* regionp) :
	LLViewerObject(id, pcode, regionp),
	mIsDummy(FALSE),
	mSpecialRenderMode(0),
	mTurning(FALSE),
	mPelvisToFoot(0.f),
	mLastSkeletonSerialNum( 0 ),
	mHeadOffset(),
	mIsSitting(FALSE),
	mTimeVisible(),
	mTyping(FALSE),
	mMeshValid(FALSE),
	mVisible(FALSE),
	mWindFreq(0.f),
	mRipplePhase( 0.f ),
	mBelowWater(FALSE),
	mAppearanceAnimSetByUser(FALSE),
	mLastAppearanceBlendTime(0.f),
	mAppearanceAnimating(FALSE),
	mNameString(),
	mTitle(),
	mNameAway(FALSE),
	mNameBusy(FALSE),
	mNameMute(FALSE),
	mRenderGroupTitles(sRenderGroupTitles),
	mNameAppearance(FALSE),
	mLastRegionHandle(0),
	mRegionCrossingCount(0),
	mFirstTEMessageReceived( FALSE ),
	mFirstAppearanceMessageReceived( FALSE ),
	mCulled( FALSE ),
	mVisibilityRank(0),
	mTexSkinColor( NULL ),
	mTexHairColor( NULL ),
	mTexEyeColor( NULL ),
	mNeedsSkin(FALSE),
	mUpdatePeriod(1),
	mFullyLoadedInitialized(FALSE),
	mHasBakedHair( FALSE )
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	//VTResume();  // VTune
	
	// mVoiceVisualizer is created by the hud effects manager and uses the HUD Effects pipeline
	const bool needsSendToSim = false; // currently, this HUD effect doesn't need to pack and unpack data to do its job
	mVoiceVisualizer = ( LLVoiceVisualizer *)LLHUDManager::getInstance()->createViewerEffect( LLHUDObject::LL_HUD_EFFECT_VOICE_VISUALIZER, needsSendToSim );

	lldebugs << "LLVOAvatar Constructor (0x" << this << ") id:" << mID << llendl;

	mPelvisp = NULL;

	for( S32 i=0; i<TEX_NUM_INDICES; i++ )
	{
		if (isIndexLocalTexture((ETextureIndex)i))
		{
			mLocalTextureData[(ETextureIndex)i] = LocalTextureData();
		}
	}

	mBakedTextureData.resize(BAKED_NUM_INDICES);
	for (U32 i = 0; i < mBakedTextureData.size(); i++ )
	{
		mBakedTextureData[i].mLastTextureIndex = IMG_DEFAULT_AVATAR;
		mBakedTextureData[i].mTexLayerSet = NULL;
		mBakedTextureData[i].mIsLoaded = false;
		mBakedTextureData[i].mMaskTexName = 0;
		mBakedTextureData[i].mTextureIndex = getTextureIndex((EBakedTextureIndex)i);
	}

	mDirtyMesh = TRUE;	// Dirty geometry, need to regenerate.
	mShadow0Facep = NULL;
	mShadow1Facep = NULL;
	mHeadp = NULL;

	mIsBuilt = FALSE;

	mNumJoints = 0;
	mSkeleton = NULL;
	mScreenp = NULL;

	mNumCollisionVolumes = 0;
	mCollisionVolumes = NULL;

	// set up animation variables
	mSpeed = 0.f;
	setAnimationData("Speed", &mSpeed);

	if (id == gAgentID)
	{
		mIsSelf = TRUE;
		gAgent.setAvatarObject(this);
		lldebugs << "Marking avatar as self " << id << llendl;
	}
	else
	{
		mIsSelf = FALSE;
	}

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

	mShadowImagep = gImageList.getImageFromFile("foot_shadow.j2c");
	gGL.getTexUnit(0)->bind(mShadowImagep.get());
	mShadowImagep->setAddressMode(LLTexUnit::TAM_CLAMP);
	
	mInAir = FALSE;

	mStepOnLand = TRUE;
	mStepMaterial = 0;

	mLipSyncActive = false;
	mOohMorph      = NULL;
	mAahMorph      = NULL;

	//-------------------------------------------------------------------------
	// initialize joint, mesh and shape members
	//-------------------------------------------------------------------------
	mRoot.setName( "mRoot" );

	for (LLVOAvatarDictionary::mesh_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getMeshes().begin();
		 iter != LLVOAvatarDictionary::getInstance()->getMeshes().end();
		 iter++)
	{
		const EMeshIndex mesh_index = iter->first;
		const LLVOAvatarDictionary::MeshDictionaryEntry *mesh_dict = iter->second;

		LLViewerJoint* joint = new LLViewerJoint();
		joint->setName(mesh_dict->mName);
		joint->setMeshID(mesh_index);
		mMeshLOD.push_back(joint);
		
		/* mHairLOD.setName("mHairLOD");
		   mHairMesh0.setName("mHairMesh0");
		   mHairMesh0.setMeshID(MESH_ID_HAIR);
		   mHairMesh1.setName("mHairMesh1"); */
		for (U32 lod = 0; lod < mesh_dict->mLOD; lod++)
		{
			LLViewerJointMesh* mesh = new LLViewerJointMesh();
			std::string mesh_name = "m" + mesh_dict->mName + boost::lexical_cast<std::string>(lod);
			// We pre-pended an m - need to capitalize first character for camelCase
			mesh_name[1] = toupper(mesh_name[1]);
			mesh->setName(mesh_name);
			mesh->setMeshID(mesh_index);
			mesh->setPickName(mesh_dict->mPickName);
			switch((int)mesh_index)
			{
				case MESH_ID_HAIR:
					mesh->setIsTransparent(TRUE);
					break;
				case MESH_ID_SKIRT:
					mesh->setIsTransparent(TRUE);
					break;
				case MESH_ID_EYEBALL_LEFT:
				case MESH_ID_EYEBALL_RIGHT:
					mesh->setSpecular( LLColor4( 1.0f, 1.0f, 1.0f, 1.0f ), 1.f );
					break;
			}

			joint->mMeshParts.push_back(mesh);
		}
	}

	//-------------------------------------------------------------------------
	// associate baked textures with meshes
	//-------------------------------------------------------------------------
	for (LLVOAvatarDictionary::mesh_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getMeshes().begin();
		 iter != LLVOAvatarDictionary::getInstance()->getMeshes().end();
		 iter++)
	{
		const EMeshIndex mesh_index = iter->first;
		const LLVOAvatarDictionary::MeshDictionaryEntry *mesh_dict = iter->second;
		const EBakedTextureIndex baked_texture_index = mesh_dict->mBakedID;

		// Skip it if there's no associated baked texture.
		if (baked_texture_index == BAKED_NUM_INDICES) continue;

		for (std::vector<LLViewerJointMesh* >::iterator iter = mMeshLOD[mesh_index]->mMeshParts.begin();
			 iter != mMeshLOD[mesh_index]->mMeshParts.end(); iter++)
		{
			LLViewerJointMesh* mesh = (LLViewerJointMesh*) *iter;
			mBakedTextureData[(int)baked_texture_index].mMeshes.push_back(mesh);
		}
	}


	//-------------------------------------------------------------------------
	// register motions
	//-------------------------------------------------------------------------
	if (LLCharacter::sInstances.size() == 1)
	{
		LLKeyframeMotion::setVFS(gStaticVFS);
		registerMotion( ANIM_AGENT_BUSY,					LLNullMotion::create );
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
		registerMotion( ANIM_AGENT_RUN,						LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_STAND,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_1,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_2,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_3,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STAND_4,					LLKeyframeStandMotion::create );
		registerMotion( ANIM_AGENT_STANDUP,					LLKeyframeFallMotion::create );
		registerMotion( ANIM_AGENT_TURNLEFT,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_TURNRIGHT,				LLKeyframeWalkMotion::create );
		registerMotion( ANIM_AGENT_WALK,					LLKeyframeWalkMotion::create );
	
		// motions without a start/stop bit
		registerMotion( ANIM_AGENT_BODY_NOISE,				LLBodyNoiseMotion::create );
		registerMotion( ANIM_AGENT_BREATHE_ROT,				LLBreatheMotionRot::create );
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

	if (gNoRender)
	{
		return;
	}
	buildCharacter();

	// preload specific motions here
	createMotion( ANIM_AGENT_CUSTOMIZE);
	createMotion( ANIM_AGENT_CUSTOMIZE_DONE);

	//VTPause();  // VTune
	
	mVoiceVisualizer->setVoiceEnabled( gVoiceClient->getVoiceEnabled( mID ) );
	mCurrentGesticulationLevel = 0;		
}

//------------------------------------------------------------------------
// LLVOAvatar::~LLVOAvatar()
//------------------------------------------------------------------------
LLVOAvatar::~LLVOAvatar()
{
	lldebugs << "LLVOAvatar Destructor (0x" << this << ") id:" << mID << llendl;

	if (mIsSelf)
	{
		gAgent.setAvatarObject(NULL);
	}

	mRoot.removeAllChildren();

	delete [] mSkeleton;
	mSkeleton = NULL;

	delete mScreenp;
	mScreenp = NULL;
	
	delete [] mCollisionVolumes;
	mCollisionVolumes = NULL;


	mNumJoints = 0;

	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		delete mBakedTextureData[i].mTexLayerSet;
		mBakedTextureData[i].mTexLayerSet = NULL;
	}

	std::for_each(mAttachmentPoints.begin(), mAttachmentPoints.end(), DeletePairedPointer());
	mAttachmentPoints.clear();

	delete mTexSkinColor;
	mTexSkinColor = NULL;
	delete mTexHairColor;
	mTexHairColor = NULL;
	delete mTexEyeColor;
	mTexEyeColor = NULL;

	std::for_each(mMeshes.begin(), mMeshes.end(), DeletePairedPointer());
	mMeshes.clear();

	for (std::vector<LLViewerJoint*>::iterator jointIter = mMeshLOD.begin();
		jointIter != mMeshLOD.end(); jointIter++)
	{
		LLViewerJoint* joint = (LLViewerJoint *) *jointIter;
		std::for_each(joint->mMeshParts.begin(), joint->mMeshParts.end(), DeletePointer());
		joint->mMeshParts.clear();
	}
	std::for_each(mMeshLOD.begin(), mMeshLOD.end(), DeletePointer());
	mMeshLOD.clear();
	
	mDead = TRUE;
	
	// Clean up class data
	LLVOAvatar::cullAvatarsByPixelArea();

	mAnimationSources.clear();

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

	mBeam = NULL;
	LLViewerObject::markDead();
}


BOOL LLVOAvatar::isFullyBaked()
{
	if (mIsDummy) return TRUE;
	if (getNumTEs() == 0) return FALSE;

	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if (!isTextureDefined(mBakedTextureData[i].mTextureIndex)
			&& ( (i != BAKED_SKIRT) || isWearingWearableType(WT_SKIRT) ) )
		{
			return FALSE;
		}
	}
	return TRUE;
}

void LLVOAvatar::deleteLayerSetCaches(bool clearAll)
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if (mBakedTextureData[i].mTexLayerSet)
		{
			if ((i != BAKED_HAIR || mIsSelf) && !clearAll)		// Backwards compatibility - can be removed after hair baking is mandatory on the grid
			{
				mBakedTextureData[i].mTexLayerSet->deleteCaches();
			}
		}
		if (mBakedTextureData[i].mMaskTexName)
		{
			glDeleteTextures(1, (GLuint*)&(mBakedTextureData[i].mMaskTexName));
			mBakedTextureData[i].mMaskTexName = 0 ;
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
// 		else
// 		if( inst->getPixelArea() < MIN_PIXEL_AREA_FOR_COMPOSITE )
// 		{
// 			return res;  // Assumes sInstances is sorted by pixel area.
// 		}
		else
		if( !inst->isFullyBaked() )
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
void LLVOAvatar::dumpScratchTextureByteCount()
{
	llinfos << "Scratch Texture GL: " << (sScratchTexBytes/1024) << "KB" << llendl;
}

// static
void LLVOAvatar::dumpBakedStatus()
{
	LLVector3d camera_pos_global = gAgent.getCameraPositionGlobal();

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
			
			for (LLVOAvatarDictionary::baked_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getBakedTextures().begin();
				 iter != LLVOAvatarDictionary::getInstance()->getBakedTextures().end();
				 iter++)
			{
				const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = iter->second;
				const ETextureIndex index = baked_dict->mTextureIndex;
				if (!inst->isTextureDefined(index))
				{
					llcont << " " << LLVOAvatarDictionary::getInstance()->getTexture(index)->mName;
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
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* inst = (LLVOAvatar*) *iter;
		inst->setCompositeUpdatesEnabled( TRUE );
		for (U32 i = 0; i < inst->mBakedTextureData.size(); i++)
		{
			inst->invalidateComposite( inst->mBakedTextureData[i].mTexLayerSet, FALSE );
		}
		inst->updateMeshTextures();
	}
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
	if (LLTexLayerSet::sHasCaches)
	{
		lldebugs << "Deleting layer set caches" << llendl;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
		{
			LLVOAvatar* inst = (LLVOAvatar*) *iter;
			inst->deleteLayerSetCaches(clearAll);
		}
		LLTexLayerSet::sHasCaches = FALSE;
	}
	
	for( LLGLuint* namep = sScratchTexNames.getFirstData(); 
		 namep; 
		 namep = sScratchTexNames.getNextData() )
	{
		LLImageGL::deleteTextures(1, (U32 *)namep );
		stop_glerror();
	}

	if( sScratchTexBytes )
	{
		lldebugs << "Clearing Scratch Textures " << (sScratchTexBytes/1024) << "KB" << llendl;

		sScratchTexNames.deleteAllData();
		LLVOAvatar::sScratchTexLastBindTime.deleteAllData();
		LLImageGL::sGlobalTextureMemory -= sScratchTexBytes;
		sScratchTexBytes = 0;
	}

	gTexStaticImageList.deleteCachedImages();
}


//------------------------------------------------------------------------
// static
// LLVOAvatar::initClass()
//------------------------------------------------------------------------
void LLVOAvatar::initClass()
{ 
	std::string xmlFile;

	xmlFile = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,AVATAR_DEFAULT_CHAR) + "_lad.xml";
	BOOL success = sXMLTree.parseFile( xmlFile, FALSE );
	if (!success)
	{
		llerrs << "Problem reading avatar configuration file:" << xmlFile << llendl;
	}

	// now sanity check xml file
	LLXmlTreeNode* root = sXMLTree.getRoot();
	if (!root) 
	{
		llerrs << "No root node found in avatar configuration file: " << xmlFile << llendl;
		return;
	}

	//-------------------------------------------------------------------------
	// <linden_avatar version="1.0"> (root)
	//-------------------------------------------------------------------------
	if( !root->hasName( "linden_avatar" ) )
	{
		llerrs << "Invalid avatar file header: " << xmlFile << llendl;
	}
	
	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || (version != "1.0") )
	{
		llerrs << "Invalid avatar file version: " << version << " in file: " << xmlFile << llendl;
	}

	S32 wearable_def_version = 1;
	static LLStdStringHandle wearable_definition_version_string = LLXmlTree::addAttributeString("wearable_definition_version");
	root->getFastAttributeS32( wearable_definition_version_string, wearable_def_version );
	LLWearable::setCurrentDefinitionVersion( wearable_def_version );

	std::string mesh_file_name;

	LLXmlTreeNode* skeleton_node = root->getChildByName( "skeleton" );
	if (!skeleton_node)
	{
		llerrs << "No skeleton in avatar configuration file: " << xmlFile << llendl;
		return;
	}
	
	std::string skeleton_file_name;
	static LLStdStringHandle file_name_string = LLXmlTree::addAttributeString("file_name");
	if (!skeleton_node->getFastAttributeString(file_name_string, skeleton_file_name))
	{
		llerrs << "No file name in skeleton node in avatar config file: " << xmlFile << llendl;
	}
	
	std::string skeleton_path;
	skeleton_path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,skeleton_file_name);
	if (!parseSkeletonFile(skeleton_path))
	{
		llerrs << "Error parsing skeleton file: " << skeleton_path << llendl;
	}

	// Process XML data

	// avatar_skeleton.xml
	llassert(!sAvatarSkeletonInfo);
	sAvatarSkeletonInfo = new LLVOAvatarSkeletonInfo;
	if (!sAvatarSkeletonInfo->parseXml(sSkeletonXMLTree.getRoot()))
	{
		llerrs << "Error parsing skeleton XML file: " << skeleton_path << llendl;
	}
	// parse avatar_lad.xml
	llassert(!sAvatarXmlInfo);
	sAvatarXmlInfo = new LLVOAvatarXmlInfo;
	if (!sAvatarXmlInfo->parseXmlSkeletonNode(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlMeshNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlColorNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlLayerNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}
	if (!sAvatarXmlInfo->parseXmlDriverNodes(root))
	{
		llerrs << "Error parsing skeleton node in avatar XML file: " << skeleton_path << llendl;
	}

}


void LLVOAvatar::cleanupClass()
{
	delete sAvatarXmlInfo;
	sAvatarXmlInfo = NULL;
	delete sAvatarSkeletonInfo;
	sAvatarSkeletonInfo = NULL;
	sSkeletonXMLTree.cleanup();
	sXMLTree.cleanup();
}

const LLVector3 LLVOAvatar::getRenderPosition() const
{
	if (mDrawable.isNull() || mDrawable->getGeneration() < 0)
	{
		return getPositionAgent();
	}
	else if (isRoot())
	{
		return mDrawable->getPositionAgent();
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

void LLVOAvatar::onShift(const LLVector3& shift_vector)
{
	mLastAnimExtents[0] += shift_vector;
	mLastAnimExtents[1] += shift_vector;
	mNeedsImpostorUpdate = TRUE;
	mNeedsAnimUpdate = TRUE;
}

void LLVOAvatar::updateSpatialExtents(LLVector3& newMin, LLVector3 &newMax)
{
	if (isImpostor() && !needsImpostorUpdate())
	{
		LLVector3 delta = getRenderPosition() -
			((LLVector3(mDrawable->getPositionGroup())-mImpostorOffset));
		
		newMin = mLastAnimExtents[0] + delta;
		newMax = mLastAnimExtents[1] + delta;
	}
	else
	{
		getSpatialExtents(newMin,newMax);
		mLastAnimExtents[0] = newMin;
		mLastAnimExtents[1] = newMax;
		LLVector3 pos_group = (newMin+newMax)*0.5f;
		mImpostorOffset = pos_group-getRenderPosition();
		mDrawable->setPositionGroup(pos_group);
	}
}

void LLVOAvatar::getSpatialExtents(LLVector3& newMin, LLVector3& newMax)
{
	LLVector3 buffer(0.25f, 0.25f, 0.25f);
	LLVector3 pos = getRenderPosition();
	newMin = pos - buffer;
	newMax = pos + buffer;
	
	//stretch bounding box by joint positions
	for (polymesh_map_t::iterator i = mMeshes.begin(); i != mMeshes.end(); ++i)
	{
		LLPolyMesh* mesh = i->second;
		for (S32 joint_num = 0; joint_num < mesh->mJointRenderData.count(); joint_num++)
		{
			update_min_max(newMin, newMax, 
							mesh->mJointRenderData[joint_num]->mWorldMatrix->getTranslation());
		}
	}

	mPixelArea = LLPipeline::calcPixelArea((newMin+newMax)*0.5f, (newMax-newMin)*0.5f, *LLViewerCamera::getInstance());

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

		LLViewerObject* object = attachment->getObject();
		if (object && !object->isHUDAttachment())
		{
			LLDrawable* drawable = object->mDrawable;
			if (drawable)
			{
				LLSpatialBridge* bridge = drawable->getSpatialBridge();
				if (bridge)
				{
					const LLVector3* ext = bridge->getSpatialExtents();
					update_min_max(newMin,newMax,ext[0]);
					update_min_max(newMin,newMax,ext[1]);
				}
			}
		}
	}

	//pad bounding box	
	newMin -= buffer;
	newMax += buffer;
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
									  LLVector3* bi_normal
		)
{

	if (mIsSelf && !gAgent.needsRenderAvatar() || !LLPipeline::sPickAvatar)
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

//-----------------------------------------------------------------------------
// parseSkeletonFile()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::parseSkeletonFile(const std::string& filename)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	//-------------------------------------------------------------------------
	// parse the file
	//-------------------------------------------------------------------------
	BOOL success = sSkeletonXMLTree.parseFile( filename, FALSE );

	if (!success)
	{
		llerrs << "Can't parse skeleton file: " << filename << llendl;
		return FALSE;
	}

	// now sanity check xml file
	LLXmlTreeNode* root = sSkeletonXMLTree.getRoot();
	if (!root) 
	{
		llerrs << "No root node found in avatar skeleton file: " << filename << llendl;
	}

	if( !root->hasName( "linden_skeleton" ) )
	{
		llerrs << "Invalid avatar skeleton file header: " << filename << llendl;
	}

	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || (version != "1.0") )
	{
		llerrs << "Invalid avatar skeleton file version: " << version << " in file: " << filename << llendl;
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// setupBone()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::setupBone(const LLVOAvatarBoneInfo* info, LLViewerJoint* parent, S32 &volume_num, S32 &joint_num)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	LLViewerJoint* joint = NULL;

	if (info->mIsJoint)
	{
		joint = (LLViewerJoint*)getCharacterJoint(joint_num);
		if (!joint)
		{
			llwarns << "Too many bones" << llendl;
			return FALSE;
		}
		joint->setName( info->mName );
	}
	else // collision volume
	{
		if (volume_num >= (S32)mNumCollisionVolumes)
		{
			llwarns << "Too many bones" << llendl;
			return FALSE;
		}
		joint = (LLViewerJoint*)(&mCollisionVolumes[volume_num]);

		joint->setName( info->mName );
	}

	// add to parent
	if (parent)
	{
		parent->addChild( joint );
	}

	joint->setPosition(info->mPos);

	joint->setRotation(mayaQ(info->mRot.mV[VX], info->mRot.mV[VY],
							 info->mRot.mV[VZ], LLQuaternion::XYZ));

	joint->setScale(info->mScale);


	if (info->mIsJoint)
	{
		joint->setSkinOffset( info->mPivot );
		joint_num++;
	}
	else // collision volume
	{
		volume_num++;
	}

	// setup children
	LLVOAvatarBoneInfo::child_list_t::const_iterator iter;
	for (iter = info->mChildList.begin(); iter != info->mChildList.end(); iter++)
	{
		LLVOAvatarBoneInfo *child_info = *iter;
		if (!setupBone(child_info, joint, volume_num, joint_num))
		{
			return FALSE;
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// buildSkeleton()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::buildSkeleton(const LLVOAvatarSkeletonInfo *info)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	//-------------------------------------------------------------------------
	// allocate joints
	//-------------------------------------------------------------------------
	if (!allocateCharacterJoints(info->mNumBones))
	{
		llerrs << "Can't allocate " << info->mNumBones << " joints" << llendl;
		return FALSE;
	}
	
	//-------------------------------------------------------------------------
	// allocate volumes
	//-------------------------------------------------------------------------
	if (info->mNumCollisionVolumes)
	{
		if (!allocateCollisionVolumes(info->mNumCollisionVolumes))
		{
			llerrs << "Can't allocate " << info->mNumCollisionVolumes << " collision volumes" << llendl;
			return FALSE;
		}
	}

	S32 current_joint_num = 0;
	S32 current_volume_num = 0;
	LLVOAvatarSkeletonInfo::bone_info_list_t::const_iterator iter;
	for (iter = info->mBoneInfoList.begin(); iter != info->mBoneInfoList.end(); iter++)
	{
		LLVOAvatarBoneInfo *info = *iter;
		if (!setupBone(info, NULL, current_volume_num, current_joint_num))
		{
			llerrs << "Error parsing bone in skeleton file" << llendl;
			return FALSE;
		}
	}

	// add special-purpose "screen" joint
	if (mIsSelf)
	{
		mScreenp = new LLViewerJoint("mScreen", NULL);
		// for now, put screen at origin, as it is only used during special
		// HUD rendering mode
		F32 aspect = LLViewerCamera::getInstance()->getAspect();
		LLVector3 scale(1.f, aspect, 1.f);
		mScreenp->setScale(scale);
		mScreenp->setWorldPosition(LLVector3::zero);
	}

	return TRUE;
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
void LLVOAvatar::buildCharacter()
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	//-------------------------------------------------------------------------
	// remove all references to our existing skeleton
	// so we can rebuild it
	//-------------------------------------------------------------------------
	flushAllMotions();

	//-------------------------------------------------------------------------
	// remove all of mRoot's children
	//-------------------------------------------------------------------------
	mRoot.removeAllChildren();
	mIsBuilt = FALSE;

	//-------------------------------------------------------------------------
	// clear mesh data
	//-------------------------------------------------------------------------
	for (std::vector<LLViewerJoint*>::iterator jointIter = mMeshLOD.begin();
		jointIter != mMeshLOD.end(); jointIter++)
	{
		LLViewerJoint* joint = (LLViewerJoint*) *jointIter;
		for (std::vector<LLViewerJointMesh*>::iterator meshIter = joint->mMeshParts.begin();
			meshIter != joint->mMeshParts.end(); meshIter++)
		{
			LLViewerJointMesh * mesh = (LLViewerJointMesh *) *meshIter;
			mesh->setMesh(NULL);
		}
	}

	//-------------------------------------------------------------------------
	// (re)load our skeleton and meshes
	//-------------------------------------------------------------------------
	LLTimer timer;

	BOOL status = loadAvatar();
	stop_glerror();

	if (gNoRender)
	{
		// Still want to load the avatar skeleton so visual parameters work.
		return;
	}

// 	gPrintMessagesThisFrame = TRUE;
	lldebugs << "Avatar load took " << timer.getElapsedTimeF32() << " seconds." << llendl;

	if ( ! status )
	{
		if ( mIsSelf )
		{
			llerrs << "Unable to load user's avatar" << llendl;
		}
		else
		{
			llwarns << "Unable to load other's avatar" << llendl;
		}
		return;
	}

	//-------------------------------------------------------------------------
	// initialize "well known" joint pointers
	//-------------------------------------------------------------------------
	mPelvisp		= (LLViewerJoint*)mRoot.findJoint("mPelvis");
	mTorsop			= (LLViewerJoint*)mRoot.findJoint("mTorso");
	mChestp			= (LLViewerJoint*)mRoot.findJoint("mChest");
	mNeckp			= (LLViewerJoint*)mRoot.findJoint("mNeck");
	mHeadp			= (LLViewerJoint*)mRoot.findJoint("mHead");
	mSkullp			= (LLViewerJoint*)mRoot.findJoint("mSkull");
	mHipLeftp		= (LLViewerJoint*)mRoot.findJoint("mHipLeft");
	mHipRightp		= (LLViewerJoint*)mRoot.findJoint("mHipRight");
	mKneeLeftp		= (LLViewerJoint*)mRoot.findJoint("mKneeLeft");
	mKneeRightp		= (LLViewerJoint*)mRoot.findJoint("mKneeRight");
	mAnkleLeftp		= (LLViewerJoint*)mRoot.findJoint("mAnkleLeft");
	mAnkleRightp	= (LLViewerJoint*)mRoot.findJoint("mAnkleRight");
	mFootLeftp		= (LLViewerJoint*)mRoot.findJoint("mFootLeft");
	mFootRightp		= (LLViewerJoint*)mRoot.findJoint("mFootRight");
	mWristLeftp		= (LLViewerJoint*)mRoot.findJoint("mWristLeft");
	mWristRightp	= (LLViewerJoint*)mRoot.findJoint("mWristRight");
	mEyeLeftp		= (LLViewerJoint*)mRoot.findJoint("mEyeLeft");
	mEyeRightp		= (LLViewerJoint*)mRoot.findJoint("mEyeRight");

	//-------------------------------------------------------------------------
	// Make sure "well known" pointers exist
	//-------------------------------------------------------------------------
	if (!(mPelvisp && 
		mTorsop &&
		mChestp &&
		mNeckp &&
		mHeadp &&
		mSkullp &&
		mHipLeftp &&
		mHipRightp &&
		mKneeLeftp &&
		mKneeRightp &&
		mAnkleLeftp &&
		mAnkleRightp &&
		mFootLeftp &&
		mFootRightp &&
		mWristLeftp &&
		mWristRightp &&
		mEyeLeftp &&
		mEyeRightp))
	{
		llerrs << "Failed to create avatar." << llendl;
		return;
	}

	//-------------------------------------------------------------------------
	// initialize the pelvis
	//-------------------------------------------------------------------------
	mPelvisp->setPosition( LLVector3(0.0f, 0.0f, 0.0f) );
	
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

	//-------------------------------------------------------------------------
	// build the attach and detach menus
	//-------------------------------------------------------------------------
	if (mIsSelf)
	{
		// *TODO: Translate
		gAttachBodyPartPieMenus[0] = NULL;
		gAttachBodyPartPieMenus[1] = new LLPieMenu(std::string("Right Arm >"));
		gAttachBodyPartPieMenus[2] = new LLPieMenu(std::string("Head >"));
		gAttachBodyPartPieMenus[3] = new LLPieMenu(std::string("Left Arm >"));
		gAttachBodyPartPieMenus[4] = NULL;
		gAttachBodyPartPieMenus[5] = new LLPieMenu(std::string("Left Leg >"));
		gAttachBodyPartPieMenus[6] = new LLPieMenu(std::string("Torso >"));
		gAttachBodyPartPieMenus[7] = new LLPieMenu(std::string("Right Leg >"));

		gDetachBodyPartPieMenus[0] = NULL;
		gDetachBodyPartPieMenus[1] = new LLPieMenu(std::string("Right Arm >"));
		gDetachBodyPartPieMenus[2] = new LLPieMenu(std::string("Head >"));
		gDetachBodyPartPieMenus[3] = new LLPieMenu(std::string("Left Arm >"));
		gDetachBodyPartPieMenus[4] = NULL;
		gDetachBodyPartPieMenus[5] = new LLPieMenu(std::string("Left Leg >"));
		gDetachBodyPartPieMenus[6] = new LLPieMenu(std::string("Torso >"));
		gDetachBodyPartPieMenus[7] = new LLPieMenu(std::string("Right Leg >"));

		for (S32 i = 0; i < 8; i++)
		{
			if (gAttachBodyPartPieMenus[i])
			{
				gAttachPieMenu->appendPieMenu( gAttachBodyPartPieMenus[i] );
			}
			else
			{
				BOOL attachment_found = FALSE;
				for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
					 iter != mAttachmentPoints.end(); )
				{
					attachment_map_t::iterator curiter = iter++;
					LLViewerJointAttachment* attachment = curiter->second;
					if (attachment->getGroup() == i)
					{
						LLMenuItemCallGL* item;
						item = new LLMenuItemCallGL(attachment->getName(), 
													NULL, 
													object_selected_and_point_valid);
						item->addListener(gMenuHolder->getListenerByName("Object.AttachToAvatar"), "on_click", curiter->first);
						
						gAttachPieMenu->append(item);

						attachment_found = TRUE;
						break;

					}
				}

				if (!attachment_found)
				{
					gAttachPieMenu->appendSeparator();
				}
			}

			if (gDetachBodyPartPieMenus[i])
			{
				gDetachPieMenu->appendPieMenu( gDetachBodyPartPieMenus[i] );
			}
			else
			{
				BOOL attachment_found = FALSE;
				for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
					 iter != mAttachmentPoints.end(); )
				{
					attachment_map_t::iterator curiter = iter++;
					LLViewerJointAttachment* attachment = curiter->second;
					if (attachment->getGroup() == i)
					{
						gDetachPieMenu->append(new LLMenuItemCallGL(attachment->getName(), 
							&handle_detach_from_avatar, object_attached, attachment));

						attachment_found = TRUE;
						break;
					}
				}

				if (!attachment_found)
				{
					gDetachPieMenu->appendSeparator();
				}
			}
		}

		// add screen attachments
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end(); )
		{
			attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			if (attachment->getGroup() == 8)
			{
				LLMenuItemCallGL* item;
				item = new LLMenuItemCallGL(attachment->getName(), 
											NULL, 
											object_selected_and_point_valid);
				item->addListener(gMenuHolder->getListenerByName("Object.AttachToAvatar"), "on_click", curiter->first);
				gAttachScreenPieMenu->append(item);
				gDetachScreenPieMenu->append(new LLMenuItemCallGL(attachment->getName(), 
							&handle_detach_from_avatar, object_attached, attachment));
			}
		}

		for (S32 pass = 0; pass < 2; pass++)
		{
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
				 iter != mAttachmentPoints.end(); )
			{
				attachment_map_t::iterator curiter = iter++;
				LLViewerJointAttachment* attachment = curiter->second;
				if (attachment->getIsHUDAttachment() != (pass == 1))
				{
					continue;
				}
				LLMenuItemCallGL* item = new LLMenuItemCallGL(attachment->getName(), 
															  NULL, &object_selected_and_point_valid,
															  &attach_label, attachment);
				item->addListener(gMenuHolder->getListenerByName("Object.AttachToAvatar"), "on_click", curiter->first);
				gAttachSubMenu->append(item);

				gDetachSubMenu->append(new LLMenuItemCallGL(attachment->getName(), 
					&handle_detach_from_avatar, object_attached, &detach_label, attachment));
				
			}
			if (pass == 0)
			{
				// put separator between non-hud and hud attachments
				gAttachSubMenu->appendSeparator();
				gDetachSubMenu->appendSeparator();
			}
		}

		for (S32 group = 0; group < 8; group++)
		{
			// skip over groups that don't have sub menus
			if (!gAttachBodyPartPieMenus[group] || !gDetachBodyPartPieMenus[group])
			{
				continue;
			}

			std::multimap<S32, S32> attachment_pie_menu_map;

			// gather up all attachment points assigned to this group, and throw into map sorted by pie slice number
			for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
				 iter != mAttachmentPoints.end(); )
			{
				attachment_map_t::iterator curiter = iter++;
				LLViewerJointAttachment* attachment = curiter->second;
				if(attachment->getGroup() == group)
				{
					// use multimap to provide a partial order off of the pie slice key
					S32 pie_index = attachment->getPieSlice();
					attachment_pie_menu_map.insert(std::make_pair(pie_index, curiter->first));
				}
			}

			// add in requested order to pie menu, inserting separators as necessary
			S32 cur_pie_slice = 0;
			for (std::multimap<S32, S32>::iterator attach_it = attachment_pie_menu_map.begin();
				 attach_it != attachment_pie_menu_map.end(); ++attach_it)
			{
				S32 requested_pie_slice = attach_it->first;
				S32 attach_index = attach_it->second;
				while (cur_pie_slice < requested_pie_slice)
				{
					gAttachBodyPartPieMenus[group]->appendSeparator();
					gDetachBodyPartPieMenus[group]->appendSeparator();
					cur_pie_slice++;
				}

				LLViewerJointAttachment* attachment = get_if_there(mAttachmentPoints, attach_index, (LLViewerJointAttachment*)NULL);
				if (attachment)
				{
					LLMenuItemCallGL* item = new LLMenuItemCallGL(attachment->getName(), 
																  NULL, object_selected_and_point_valid);
					gAttachBodyPartPieMenus[group]->append(item);
					item->addListener(gMenuHolder->getListenerByName("Object.AttachToAvatar"), "on_click", attach_index);
					gDetachBodyPartPieMenus[group]->append(new LLMenuItemCallGL(attachment->getName(), 
																				&handle_detach_from_avatar,
																				object_attached, attachment));
					cur_pie_slice++;
				}
			}
		}
	}

	mMeshValid = TRUE;
}


//-----------------------------------------------------------------------------
// releaseMeshData()
//-----------------------------------------------------------------------------
void LLVOAvatar::releaseMeshData()
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	if (sInstances.size() < AVATAR_RELEASE_THRESHOLD || mIsDummy)
	{
		return;
	}

	//llinfos << "Releasing" << llendl;

	// cleanup mesh data
	for (std::vector<LLViewerJoint*>::iterator iter = mMeshLOD.begin();
		iter != mMeshLOD.end(); iter++)
	{
		LLViewerJoint* joint = (LLViewerJoint*) *iter;
		joint->setValid(FALSE, TRUE);
	}

	//cleanup data
	if (mDrawable.notNull())
	{
		LLFace* facep = mDrawable->getFace(0);
		facep->setSize(0, 0);
		for(S32 i = mNumInitFaces ; i < mDrawable->getNumFaces(); i++)
		{
			facep = mDrawable->getFace(i);
			facep->setSize(0, 0);
		}
	}
	
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
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
void LLVOAvatar::restoreMeshData()
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	//llinfos << "Restoring" << llendl;
	mMeshValid = TRUE;
	updateJointLODs();

	if (mIsSelf)
	{
		updateAttachmentVisibility(gAgent.getCameraMode());
	}
	else
	{
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end(); )
		{
			attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			if (!attachment->getIsHUDAttachment())
			{
				attachment->setAttachmentVisibility(TRUE);
			}
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

				mMeshLOD[part_index++]->updateFaceSizes(num_vertices, num_indices, mAdjustedPixelArea);
			}
			if(num_vertices < 1)//skip empty meshes
			{
				break ;
			}
			if(last_v_num > 0)//put the last inserted part into next vertex buffer.
			{
				num_vertices = last_v_num ;
				num_indices = last_i_num ;	
				part_index-- ;
			}
		
			LLFace* facep ;
			if(f_num < mDrawable->getNumFaces()) 
			{
				facep = mDrawable->getFace(f_num);
			}
			else
			{
				facep = mDrawable->addFace(mDrawable->getFace(0)->getPool(), mDrawable->getFace(0)->getTexture()) ;
			}
			
			// resize immediately
			facep->setSize(num_vertices, num_indices);

			if(facep->mVertexBuffer.isNull())
			{
				facep->mVertexBuffer = new LLVertexBufferAvatar();
				facep->mVertexBuffer->allocateBuffer(num_vertices, num_indices, TRUE);
			}
			else
			{
				facep->mVertexBuffer->resizeBuffer(num_vertices, num_indices) ;
			}
		
			facep->setGeomIndex(0);
			facep->setIndicesIndex(0);
		
			// This is a hack! Avatars have their own pool, so we are detecting
			//   the case of more than one avatar in the pool (thus > 0 instead of >= 0)
			if (facep->getGeomIndex() > 0)
			{
				llerrs << "non-zero geom index: " << facep->getGeomIndex() << " in LLVOAvatar::restoreMeshData" << llendl;
			}

			for(S32 k = j ; k < part_index ; k++)
			{
				mMeshLOD[k]->updateFaceData(facep, mAdjustedPixelArea, k == MESH_ID_HAIR);
			}

			stop_glerror();
			facep->mVertexBuffer->setBuffer(0);

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
// The viewer can only suggest a good size for the agent,
// the simulator will keep it inside a reasonable range.
void LLVOAvatar::computeBodySize() 
{
	LLVector3 pelvis_scale = mPelvisp->getScale();

	// some of the joints have not been cached
	LLVector3 skull = mSkullp->getPosition();
	LLVector3 skull_scale = mSkullp->getScale();

	LLVector3 neck = mNeckp->getPosition();
	LLVector3 neck_scale = mNeckp->getScale();

	LLVector3 chest = mChestp->getPosition();
	LLVector3 chest_scale = mChestp->getScale();

	// the rest of the joints have been cached
	LLVector3 head = mHeadp->getPosition();
	LLVector3 head_scale = mHeadp->getScale();

	LLVector3 torso = mTorsop->getPosition();
	LLVector3 torso_scale = mTorsop->getScale();

	LLVector3 hip = mHipLeftp->getPosition();
	LLVector3 hip_scale = mHipLeftp->getScale();

	LLVector3 knee = mKneeLeftp->getPosition();
	LLVector3 knee_scale = mKneeLeftp->getScale();

	LLVector3 ankle = mAnkleLeftp->getPosition();
	LLVector3 ankle_scale = mAnkleLeftp->getScale();

	LLVector3 foot  = mFootLeftp->getPosition();

	mPelvisToFoot = hip.mV[VZ] * pelvis_scale.mV[VZ] -
				 	knee.mV[VZ] * hip_scale.mV[VZ] -
				 	ankle.mV[VZ] * knee_scale.mV[VZ] -
				 	foot.mV[VZ] * ankle_scale.mV[VZ];

	mBodySize.mV[VZ] = mPelvisToFoot +
					   // the sqrt(2) correction below is an approximate
					   // correction to get to the top of the head
					   F_SQRT2 * (skull.mV[VZ] * head_scale.mV[VZ]) + 
					   head.mV[VZ] * neck_scale.mV[VZ] + 
					   neck.mV[VZ] * chest_scale.mV[VZ] + 
					   chest.mV[VZ] * torso_scale.mV[VZ] + 
					   torso.mV[VZ] * pelvis_scale.mV[VZ]; 

	// TODO -- measure the real depth and width
	mBodySize.mV[VX] = DEFAULT_AGENT_DEPTH;
	mBodySize.mV[VY] = DEFAULT_AGENT_WIDTH;

/* debug spam
	std::cout << "skull = " << skull << std::endl;				// adebug
	std::cout << "head = " << head << std::endl;				// adebug
	std::cout << "head_scale = " << head_scale << std::endl;	// adebug
	std::cout << "neck = " << neck << std::endl;				// adebug
	std::cout << "neck_scale = " << neck_scale << std::endl;	// adebug
	std::cout << "chest = " << chest << std::endl;				// adebug
	std::cout << "chest_scale = " << chest_scale << std::endl;	// adebug
	std::cout << "torso = " << torso << std::endl;				// adebug
	std::cout << "torso_scale = " << torso_scale << std::endl;	// adebug
	std::cout << std::endl;	// adebug

	std::cout << "pelvis_scale = " << pelvis_scale << std::endl;// adebug
	std::cout << std::endl;	// adebug

	std::cout << "hip = " << hip << std::endl;					// adebug
	std::cout << "hip_scale = " << hip_scale << std::endl;		// adebug
	std::cout << "ankle = " << ankle << std::endl;				// adebug
	std::cout << "ankle_scale = " << ankle_scale << std::endl;	// adebug
	std::cout << "foot = " << foot << std::endl;				// adebug
	std::cout << "mBodySize = " << mBodySize << std::endl;		// adebug
	std::cout << "mPelvisToFoot = " << mPelvisToFoot << std::endl;	// adebug
	std::cout << std::endl;		// adebug
*/
}

//------------------------------------------------------------------------
// LLVOAvatar::processUpdateMessage()
//------------------------------------------------------------------------
U32 LLVOAvatar::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num, const EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	LLVector3 old_vel = getVelocity();
	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	if(retval & LLViewerObject::INVALID_UPDATE)
	{
		if(this == gAgent.getAvatarObject())
		{
			//tell sim to cancel this update
			gAgent.teleportViaLocation(gAgent.getPositionGlobal());
		}
	}

	//llinfos << getRotation() << llendl;
	//llinfos << getPosition() << llendl;

	return retval;
}

// virtual
S32 LLVOAvatar::setTETexture(const U8 te, const LLUUID& uuid)
{
	// The core setTETexture() method requests images, so we need
	// to redirect certain avatar texture requests to different sims.
	if (isIndexBakedTexture((ETextureIndex)te))
	{
		LLHost target_host = getObjectHost();
		return setTETextureCore(te, uuid, target_host);
	}
	else
	{
		return setTETextureCore(te, uuid, LLHost::invalid);
	}
}


// setTEImage

//------------------------------------------------------------------------
// idleUpdate()
//------------------------------------------------------------------------
BOOL LLVOAvatar::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	LLFastTimer t(LLFastTimer::FTM_AVATAR_UPDATE);

	if (isDead())
	{
		llinfos << "Warning!  Idle on dead avatar" << llendl;
		return TRUE;
	}

 	if (!(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_AVATAR)))
	{
		return TRUE;
	}
	
	// force immediate pixel area update on avatars using last frames data (before drawable or camera updates)
	setPixelAreaAndAngle(gAgent);

	// force asynchronous drawable update
	if(mDrawable.notNull() && !gNoRender)
	{	
		LLFastTimer t(LLFastTimer::FTM_JOINT_UPDATE);
	
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

	if (mIsSelf)
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
	LLVector3 root_pos_last = mRoot.getWorldPosition();
	bool detailed_update = updateCharacter(agent);
	bool voice_enabled = gVoiceClient->getVoiceEnabled( mID ) && gVoiceClient->inProximalChannel();

	if (gNoRender)
	{
		return TRUE;
	}

	idleUpdateVoiceVisualizer( voice_enabled );
	idleUpdateMisc( detailed_update );
	idleUpdateAppearanceAnimation();
	idleUpdateLipSync( voice_enabled );
	idleUpdateLoadingEffect();
	idleUpdateBelowWater();	// wind effect uses this
	idleUpdateWindEffect();
	idleUpdateNameTag( root_pos_last );
	idleUpdateRenderCost();
	idleUpdateTractorBeam();
	return TRUE;
}

void LLVOAvatar::idleUpdateVoiceVisualizer(bool voice_enabled)
{
	// disable voice visualizer when in mouselook
	mVoiceVisualizer->setVoiceEnabled( voice_enabled && !(mIsSelf && gAgent.cameraMouselook()) );
	if ( voice_enabled )
	{		
		//----------------------------------------------------------------
		// Only do gesture triggering for your own avatar, and only when you're in a proximal channel.
		//----------------------------------------------------------------
		if( mIsSelf )
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
					gGestureManager.triggerAndReviseString( gestureString );
				}
			}
			
		} //if( mIsSelf )
		
		//-----------------------------------------------------------------------------------------------------------------
		// If the avatar is speaking, then the voice amplitude signal is passed to the voice visualizer.
		// Also, here we trigger voice visualizer start and stop speaking, so it can animate the voice symbol.
		//
		// Notice the calls to "gAwayTimer.reset()". This resets the timer that determines how long the avatar has been
		// "away", so that the avatar doesn't lapse into away-mode (and slump over) while the user is still talking. 
		//-----------------------------------------------------------------------------------------------------------------
		if ( gVoiceClient->getIsSpeaking( mID ) )
		{		
			if ( ! mVoiceVisualizer->getCurrentlySpeaking() )
			{
				mVoiceVisualizer->setStartSpeaking();
				
				//printf( "gAwayTimer.reset();\n" );
			}
			
			mVoiceVisualizer->setSpeakingAmplitude( gVoiceClient->getCurrentPower( mID ) );
			
			if( mIsSelf )
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
		LLVector3 headOffset = LLVector3( 0.0f, 0.0f, mHeadOffset.mV[2] );
		mVoiceVisualizer->setVoiceSourceWorldPosition( mRoot.getWorldPosition() + headOffset );
		
	}//if ( voiceEnabled )
}		

void LLVOAvatar::idleUpdateMisc(bool detailed_update)
{
	if (LLVOAvatar::sJointDebug)
	{
		llinfos << getFullname() << ": joint touches: " << LLJoint::sNumTouches << " updates: " << LLJoint::sNumUpdates << llendl;
	}

	LLJoint::sNumUpdates = 0;
	LLJoint::sNumTouches = 0;

	// *NOTE: this is necessary for the floating name text above your head.
	if (mDrawable.notNull())
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_SHADOW, TRUE);
	}

	BOOL visible = isVisible() || mNeedsAnimUpdate;

	// update attachments positions
	if (detailed_update || !sUseImpostors)
	{
		LLFastTimer t(LLFastTimer::FTM_ATTACHMENT_UPDATE);
		for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
			 iter != mAttachmentPoints.end(); )
		{
			attachment_map_t::iterator curiter = iter++;
			LLViewerJointAttachment* attachment = curiter->second;
			LLViewerObject *attached_object = attachment->getObject();

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

	mNeedsAnimUpdate = FALSE;

	if (isImpostor() && !mNeedsImpostorUpdate)
	{
		LLVector3 ext[2];
		F32 distance;
		LLVector3 angle;

		getImpostorValues(ext, angle, distance);

		for (U32 i = 0; i < 3 && !mNeedsImpostorUpdate; i++)
		{
			F32 cur_angle = angle.mV[i];
			F32 old_angle = mImpostorAngle.mV[i];
			F32 angle_diff = fabsf(cur_angle-old_angle);
		
			if (angle_diff > 3.14159f/512.f*distance*mUpdatePeriod)
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
				getSpatialExtents(ext[0], ext[1]);
				if ((ext[1]-mImpostorExtents[1]).length() > 0.05f ||
					(ext[0]-mImpostorExtents[0]).length() > 0.05f)
				{
					mNeedsImpostorUpdate = TRUE;
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
				if (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)
				{
					param->stopAnimating(mAppearanceAnimSetByUser);
				}
			}
			updateVisualParams();
			if (mIsSelf)
			{
				gAgent.sendAgentSetAppearance();
			}
		}
		else
		{
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

			LLVisualParam *param;

			// animate only top level params
			for (param = getFirstVisualParam();
				 param;
				 param = getNextVisualParam())
			{
				if (param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE)
				{
					param->animate(morph_amt, mAppearanceAnimSetByUser);
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

void LLVOAvatar::idleUpdateLipSync(bool voice_enabled)
{
	// Use the Lipsync_Ooh and Lipsync_Aah morphs for lip sync
	if ( voice_enabled && (gVoiceClient->lipSyncEnabled()) && gVoiceClient->getIsSpeaking( mID ) )
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
		if (isFullyLoaded())
		{
			deleteParticleSource();
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
			LLViewerImage* cloud = gImageList.getImageFromFile("cloud-particle.j2c");
			particle_parameters.mPartImageID                 = cloud->getID();
			particle_parameters.mMaxAge                      = 0.f;
			particle_parameters.mPattern                     = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE;
			particle_parameters.mInnerAngle                  = 3.14159f;
			particle_parameters.mOuterAngle                  = 0.f;
			particle_parameters.mBurstRate                   = 0.02f;
			particle_parameters.mBurstRadius                 = 0.0f;
			particle_parameters.mBurstPartCount              = 1;
			particle_parameters.mBurstSpeedMin               = 0.1f;
			particle_parameters.mBurstSpeedMax               = 1.f;
			particle_parameters.mPartData.mFlags             = ( LLPartData::LL_PART_INTERP_COLOR_MASK | LLPartData::LL_PART_INTERP_SCALE_MASK |
																 LLPartData::LL_PART_EMISSIVE_MASK | // LLPartData::LL_PART_FOLLOW_SRC_MASK |
																 LLPartData::LL_PART_TARGET_POS_MASK );
			
			setParticleSource(particle_parameters, getID());
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
	// draw text label over characters head
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
	if (mIsSelf)
	{
		render_name = render_name
						&& !gAgent.cameraMouselook()
						&& (visible_chat || !gSavedSettings.getBOOL("RenderNameHideSelf"));
	}

	if ( render_name )
	{
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
		{
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

			if (alpha > 0.f)
			{
				if (!mNameText)
				{
					mNameText = (LLHUDText *)LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
					mNameText->setMass(10.f);
					mNameText->setSourceObject(this);
					mNameText->setVertAlignment(LLHUDText::ALIGN_VERT_TOP);
					mNameText->setVisibleOffScreen(TRUE);
					mNameText->setMaxLines(11);
					mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
					mNameText->setUseBubble(TRUE);
					sNumVisibleChatBubbles++;
					new_name = TRUE;
				}
				
				LLColor4 avatar_name_color = gColors.getColor( "AvatarNameColor" );
				avatar_name_color.setAlpha(alpha);
				mNameText->setColor(avatar_name_color);
				
				LLQuaternion root_rot = mRoot.getWorldRotation();
				mNameText->setUsePixelSize(TRUE);
				LLVector3 pixel_right_vec;
				LLVector3 pixel_up_vec;
				LLViewerCamera::getInstance()->getPixelVectors(root_pos_last, pixel_up_vec, pixel_right_vec);
				LLVector3 camera_to_av = root_pos_last - LLViewerCamera::getInstance()->getOrigin();
				camera_to_av.normalize();
				LLVector3 local_camera_at = camera_to_av * ~root_rot;
				LLVector3 local_camera_up = camera_to_av % LLViewerCamera::getInstance()->getLeftAxis();
				local_camera_up.normalize();
				local_camera_up = local_camera_up * ~root_rot;
			
				local_camera_up.scaleVec(mBodySize * 0.5f);
				local_camera_at.scaleVec(mBodySize * 0.5f);

				LLVector3 name_position = mRoot.getWorldPosition() + 
					(local_camera_up * root_rot) -
					(projected_vec(local_camera_at * root_rot, camera_to_av));
				name_position += pixel_up_vec * 15.f;
				mNameText->setPositionAgent(name_position);
			}
			else if (mNameText)
			{
				mNameText->markDead();
				mNameText = NULL;
				sNumVisibleChatBubbles--;
			}
		}
		
		LLNameValue *title = getNVPair("Title");
		LLNameValue* firstname = getNVPair("FirstName");
		LLNameValue* lastname = getNVPair("LastName");

		if (mNameText.notNull() && firstname && lastname)
		{
			BOOL is_away = mSignaledAnimations.find(ANIM_AGENT_AWAY)  != mSignaledAnimations.end();
			BOOL is_busy = mSignaledAnimations.find(ANIM_AGENT_BUSY) != mSignaledAnimations.end();
			BOOL is_appearance = mSignaledAnimations.find(ANIM_AGENT_CUSTOMIZE) != mSignaledAnimations.end();
			BOOL is_muted;
			if (mIsSelf)
			{
				is_muted = FALSE;
			}
			else
			{
				is_muted = LLMuteList::getInstance()->isMuted(getID());
			}

			if (mNameString.empty() ||
				new_name ||
				(!title && !mTitle.empty()) ||
				(title && mTitle != title->getString()) ||
				(is_away != mNameAway || is_busy != mNameBusy || is_muted != mNameMute)
				|| is_appearance != mNameAppearance)
			{
				std::string line;
				if (!sRenderGroupTitles)
				{
					// If all group titles are turned off, stack first name
					// on a line above last name
					line += firstname->getString();
					line += "\n";
				}
				else if (title && title->getString() && title->getString()[0] != '\0')
				{
					line += title->getString();
					LLStringFn::replace_ascii_controlchars(line,LL_UNKNOWN_CHAR);
					line += "\n";
					line += firstname->getString();
				}
				else
				{
					line += firstname->getString();
				}

				line += " ";
				line += lastname->getString();
				BOOL need_comma = FALSE;

				if (is_away || is_muted || is_busy)
				{
					line += " (";
					if (is_away)
					{
						line += "Away";
						need_comma = TRUE;
					}
					if (is_busy)
					{
						if (need_comma)
						{
							line += ", ";
						}
						line += "Busy";
						need_comma = TRUE;
					}
					if (is_muted)
					{
						if (need_comma)
						{
							line += ", ";
						}
						line += "Muted";
						need_comma = TRUE;
					}
					line += ")";
				}
				if (is_appearance)
				{
					line += "\n";
					line += "(Editing Appearance)";
				}
				mNameAway = is_away;
				mNameBusy = is_busy;
				mNameMute = is_muted;
				mNameAppearance = is_appearance;
				mTitle = title ? title->getString() : "";
				LLStringFn::replace_ascii_controlchars(mTitle,LL_UNKNOWN_CHAR);
				mNameString = utf8str_to_wstring(line);
				new_name = TRUE;
			}

			if (visible_chat)
			{
				mNameText->setDropShadow(TRUE);
				mNameText->setFont(LLFontGL::sSansSerif);
				mNameText->setTextAlignment(LLHUDText::ALIGN_TEXT_LEFT);
				mNameText->setFadeDistance(CHAT_NORMAL_RADIUS * 2.f, 5.f);
				if (new_name)
				{
					mNameText->setLabel(mNameString);
				}
			
				char line[MAX_STRING];		/* Flawfinder: ignore */
				line[0] = '\0';
				std::deque<LLChat>::iterator chat_iter = mChats.begin();
				mNameText->clearString();

				LLColor4 new_chat = gColors.getColor( "AvatarNameColor" );
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
						mNameText->addLine(utf8str_to_wstring(chat_iter->mText), lerp(new_chat, normal_chat, u), style);
					}
					else if (chat_fade_amt < 2.f)
					{
						F32 u = clamp_rescale(chat_fade_amt, 1.9f, 2.f, 0.f, 1.f);
						mNameText->addLine(utf8str_to_wstring(chat_iter->mText), lerp(normal_chat, old_chat, u), style);
					}
					else if (chat_fade_amt < 3.f)
					{
						// *NOTE: only remove lines down to minimum number
						mNameText->addLine(utf8str_to_wstring(chat_iter->mText), old_chat, style);
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
				if (gSavedSettings.getBOOL("SmallAvatarNames"))
				{
					mNameText->setFont(LLFontGL::sSansSerif);
				}
				else
				{
					mNameText->setFont(LLFontGL::sSansSerifBig);
				}
				mNameText->setTextAlignment(LLHUDText::ALIGN_TEXT_CENTER);
				mNameText->setFadeDistance(CHAT_NORMAL_RADIUS, 5.f);
				mNameText->setVisibleOffScreen(FALSE);
				if (new_name)
				{
					mNameText->setLabel("");
					mNameText->setString(mNameString);
				}
			}
		}
	}
	else if (mNameText)
	{
		mNameText->markDead();
		mNameText = NULL;
		sNumVisibleChatBubbles--;
	}
}

void LLVOAvatar::idleUpdateTractorBeam()
{
	//--------------------------------------------------------------------
	// draw tractor beam when editing objects
	//--------------------------------------------------------------------
	if (!mIsSelf)
	{
		return;
	}

	// This is only done for yourself (maybe it should be in the agent?)
	if (!needsRenderBeam() || !mIsBuilt)
	{
		mBeam = NULL;
	}
	else if (!mBeam || mBeam->isDead())
	{
		// VEFFECT: Tractor Beam
		mBeam = (LLHUDEffectSpiral *)LLHUDManager::getInstance()->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_BEAM);
		mBeam->setColor(LLColor4U(gAgent.getEffectColor()));
		mBeam->setSourceObject(this);
		mBeamTimer.reset();
	}

	if (!mBeam.isNull())
	{
		LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection();

		if (gAgent.mPointAt.notNull())
		{
			// get point from pointat effect
			mBeam->setPositionGlobal(gAgent.mPointAt->getPointAtPosGlobal());
			mBeam->triggerLocal();
		}
		else if (selection->getFirstRootObject() && 
				selection->getSelectType() != SELECT_TYPE_HUD)
		{
			LLViewerObject* objectp = selection->getFirstRootObject();
			mBeam->setTargetObject(objectp);
		}
		else
		{
			mBeam->setTargetObject(NULL);
			LLTool *tool = LLToolMgr::getInstance()->getCurrentTool();
			if (tool->isEditing())
			{
				if (tool->getEditingObject())
				{
					mBeam->setTargetObject(tool->getEditingObject());
				}
				else
				{
					mBeam->setPositionGlobal(tool->getEditingPointGlobal());
				}
			}
			else
			{
				const LLPickInfo& pick = gViewerWindow->getLastPick();
				mBeam->setPositionGlobal(pick.mPosGlobal);
			}

		}
		if (mBeamTimer.getElapsedTimeF32() > 0.25f)
		{
			mBeam->setColor(LLColor4U(gAgent.getEffectColor()));
			mBeam->setNeedsSendToSim(TRUE);
			mBeamTimer.reset();
		}
	}
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
	mRoot.setWorldPosition(getPositionAgent()); // teleport
	setChanged(TRANSLATED);
	if (mDrawable.notNull())
	{
		gPipeline.updateMoveNormalAsync(mDrawable);
	}
	mRoot.updateWorldMatrixChildren();
}

//------------------------------------------------------------------------
// updateCharacter()
// called on both your avatar and other avatars
//------------------------------------------------------------------------
BOOL LLVOAvatar::updateCharacter(LLAgent &agent)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	// update screen joint size

	if (mScreenp)
	{
		F32 aspect = LLViewerCamera::getInstance()->getAspect();
		LLVector3 scale(1.f, aspect, 1.f);
		mScreenp->setScale(scale);
		mScreenp->updateWorldMatrixChildren();
		resetHUDAttachments();
	}

	// clear debug text
	mDebugText.clear();
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
									  motionp->getID().asString().c_str(),
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

	if (gNoRender)
	{
		// Hack if we're running drones...
		if (mIsSelf)
		{
			gAgent.setPositionAgent(getPositionAgent());
		}
		return FALSE;
	}


	LLVector3d root_pos_global;

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

	if (visible && !mIsSelf && !mIsDummy && sUseImpostors && !mNeedsAnimUpdate && !sFreezeCounter)
	{
		F32 impostor_area = 256.f*512.f*(8.125f - LLVOAvatar::sLODFactor*8.f);
		if (LLMuteList::getInstance()->isMuted(getID()))
		{ // muted avatars update at 16 hz
			mUpdatePeriod = 16;
		}
		else if (visible && mVisibilityRank <= LLVOAvatar::sMaxVisible * 0.25f)
		{ //first 25% of max visible avatars are not impostored
			mUpdatePeriod = 1;
		}
		else if (visible && mVisibilityRank > LLVOAvatar::sMaxVisible * 0.75f)
		{ //back 25% of max visible avatars are slow updating impostors
			mUpdatePeriod = 8;
		}
		else if (visible && mVisibilityRank > (U32) LLVOAvatar::sMaxVisible)
		{ //background avatars are REALLY slow updating impostors
			mUpdatePeriod = 16;
		}
		else if (visible && mImpostorPixelArea <= impostor_area)
		{  // stuff in between gets an update period based on pixel area
			mUpdatePeriod = llclamp((S32) sqrtf(impostor_area*4.f/mImpostorPixelArea), 2, 8);
		}
		else if (visible && mVisibilityRank > LLVOAvatar::sMaxVisible * 0.25f)
		{ // force nearby impostors in ultra crowded areas
			mUpdatePeriod = 2;
		}
		else
		{ // not impostored
			mUpdatePeriod = 1;
		}

		visible = (LLDrawable::getCurrentFrame()+mID.mData[0])%mUpdatePeriod == 0 ? TRUE : FALSE;
	}

	if (!visible)
	{
		updateMotions(LLCharacter::HIDDEN_UPDATE);
		return FALSE;
	}

	// change animation time quanta based on avatar render load
	if (!mIsSelf && !mIsDummy)
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

	BOOL throttle = TRUE;

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
			throttle = FALSE;

			// put the pelvis at slaved position/mRotation
			mRoot.setWorldPosition( getPositionAgent() ); // first frame
			mRoot.setWorldRotation( getRotation() );
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

		if (mIsSelf)
		{
			gAgent.setPositionAgent(getRenderPosition());
		}

		root_pos = gAgent.getPosGlobalFromAgent(getRenderPosition());

		resolveHeightGlobal(root_pos, ground_under_pelvis, normal);
		F32 foot_to_ground = (F32) (root_pos.mdV[VZ] - mPelvisToFoot - ground_under_pelvis.mdV[VZ]);
		BOOL in_air = ( (!LLWorld::getInstance()->getRegionFromPosGlobal(ground_under_pelvis)) || 
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

		if (newPosition != mRoot.getXform()->getWorldPosition())
		{		
			mRoot.touch();
			mRoot.setWorldPosition(newPosition ); // regular update
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
			if (mIsSelf)
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
			if (mIsSelf && gAgent.cameraMouselook())
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

			LLQuaternion root_rotation = mRoot.getWorldMatrix().quaternion();
			F32 root_roll, root_pitch, root_yaw;
			root_rotation.getEulerAngles(&root_roll, &root_pitch, &root_yaw);

			if (sDebugAvatarRotation)
			{
				llinfos << "root_roll " << RAD_TO_DEG * root_roll 
					<< " root_pitch " << RAD_TO_DEG * root_pitch
					<< " root_yaw " << RAD_TO_DEG * root_yaw
					<< llendl;
			}

			// When moving very slow, the pelvis is allowed to deviate from the
			// forward direction to allow it to hold it's position while the torso
			// and head turn.  Once in motion, it must conform however.
			BOOL self_in_mouselook = mIsSelf && gAgent.cameraMouselook();

			LLVector3 pelvisDir( mRoot.getWorldMatrix().getFwdRow4().mV );
			F32 pelvis_rot_threshold = clamp_rescale(speed, 0.1f, 1.0f, PELVIS_ROT_THRESHOLD_SLOW, PELVIS_ROT_THRESHOLD_FAST);
						
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

			if (mIsSelf && mTurning)
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

			mRoot.setWorldRotation( slerp(u, mRoot.getWorldRotation(), wQv) );
			
		}
	}
	else if (mDrawable.notNull())
	{
		mRoot.setPosition(mDrawable->getPosition());
		mRoot.setRotation(mDrawable->getRotation());
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
//			F32 gain = clamp_rescale( mSpeedAccum,
//							AUDIO_STEP_LO_SPEED, AUDIO_STEP_HI_SPEED,
//							AUDIO_STEP_LO_GAIN, AUDIO_STEP_HI_GAIN );

			const F32 STEP_VOLUME = 0.5f;
			const LLUUID& step_sound_id = getStepSound();

			LLVector3d foot_pos_global = gAgent.getPosGlobalFromAgent(foot_pos_agent);

			if (LLViewerParcelMgr::getInstance()->canHearSound(foot_pos_global)
				&& !LLMuteList::getInstance()->isMuted(getID(), LLMute::flagObjectSounds))
			{
				gAudiop->triggerSound(step_sound_id, getID(), STEP_VOLUME, LLAudioEngine::AUDIO_TYPE_AMBIENT, foot_pos_global);
			}
		}
	}

	mRoot.updateWorldMatrixChildren();

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
	midEyePt -= mDrawable.notNull() ? mDrawable->getWorldPosition() : mRoot.getWorldPosition();
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

		if( mIsSelf )
		{
			if( !gAgent.areWearablesLoaded())
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
				llinfos << "Avatar " << firstname->getString() << " updating visiblity" << llendl;
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
				 iter != mAttachmentPoints.end(); )
			{
				attachment_map_t::iterator curiter = iter++;
				LLViewerJointAttachment* attachment = curiter->second;
				if (attachment->getObject())
				{
					if(attachment->getObject()->mDrawable->isVisible())
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
		// this breaks off-screen chat bubbles
		//if (mNameText)
		//{
		//	mNameText->markDead();
		//	mNameText = NULL;
		//	sNumVisibleChatBubbles--;
		//}
	}

	mVisible = visible;
}

//------------------------------------------------------------------------
// needsRenderBeam()
//------------------------------------------------------------------------
BOOL LLVOAvatar::needsRenderBeam()
{
	if (gNoRender)
	{
		return FALSE;
	}
	LLTool *tool = LLToolMgr::getInstance()->getCurrentTool();

	BOOL is_touching_or_grabbing = (tool == LLToolGrab::getInstance() && LLToolGrab::getInstance()->isEditing());
	if (LLToolGrab::getInstance()->getEditingObject() && 
		LLToolGrab::getInstance()->getEditingObject()->isAttachment())
	{
		// don't render selection beam on hud objects
		is_touching_or_grabbing = FALSE;
	}
	return is_touching_or_grabbing || (mState & AGENT_STATE_EDITING && LLSelectMgr::getInstance()->shouldShowSelection());
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

	if (mDirtyMesh || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY))
	{	//LOD changed or new mesh created, allocate new vertex buffer if needed
		updateMeshData();
		mDirtyMesh = FALSE;
		mNeedsSkin = TRUE;
		mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
	}

	if (LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) <= 0)
	{
		if (mNeedsSkin)
		{
			//generate animated mesh
			mMeshLOD[MESH_ID_LOWER_BODY]->updateJointGeometry();
			mMeshLOD[MESH_ID_UPPER_BODY]->updateJointGeometry();

			if( isWearingWearableType( WT_SKIRT ) )
			{
				mMeshLOD[MESH_ID_SKIRT]->updateJointGeometry();
			}

			if (!mIsSelf || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
				mMeshLOD[MESH_ID_EYELASH]->updateJointGeometry();
				mMeshLOD[MESH_ID_HEAD]->updateJointGeometry();
				mMeshLOD[MESH_ID_HAIR]->updateJointGeometry();
			}
			mNeedsSkin = FALSE;
			
			LLVertexBuffer* vb = mDrawable->getFace(0)->mVertexBuffer;
			if (vb)
			{
				vb->setBuffer(0);
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
			llinfos << "Avatar " << firstname->getString() << " in render" << llendl;
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

	if (mIsSelf && !gAgent.needsRenderAvatar())
	{
		return num_indices;
	}

	// render collision normal
	// *NOTE: this is disabled (there is no UI for enabling sShowFootPlane) due
	// to DEV-14477.  the code is left here to aid in tracking down the cause
	// of the crash in the future. -brad
	if (!gRenderForSelect && sShowFootPlane && mDrawable.notNull())
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
		bool should_alpha_mask = mHasBakedHair && isTextureDefined(TEX_HEAD_BAKED) && isTextureDefined(TEX_UPPER_BAKED) 
								&& isTextureDefined(TEX_LOWER_BAKED) && mBakedTextureData[BAKED_HEAD].mIsLoaded
								&& mBakedTextureData[BAKED_UPPER].mIsLoaded && mBakedTextureData[BAKED_LOWER].mIsLoaded;
		LLGLState test(GL_ALPHA_TEST, should_alpha_mask);

		if (should_alpha_mask)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		
		BOOL first_pass = TRUE;
		if (!LLDrawPoolAvatar::sSkipOpaque)
		{
			if (!mIsSelf || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
			{
				if (isTextureVisible(TEX_HEAD_BAKED))
				{
					num_indices += mMeshLOD[MESH_ID_HEAD]->render(mAdjustedPixelArea);
					first_pass = FALSE;
				}
			}
			if (isTextureVisible(TEX_UPPER_BAKED))
			{
				num_indices += mMeshLOD[MESH_ID_UPPER_BODY]->render(mAdjustedPixelArea, first_pass);
				first_pass = FALSE;
			}
			
			if (isTextureVisible(TEX_LOWER_BAKED))
			{
				num_indices += mMeshLOD[MESH_ID_LOWER_BODY]->render(mAdjustedPixelArea, first_pass);
				first_pass = FALSE;
			}
		}

		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);

		if (!LLDrawPoolAvatar::sSkipTransparent || LLPipeline::sImpostorRender)
		{
			LLGLEnable blend(GL_BLEND);
			LLGLEnable test(GL_ALPHA_TEST);
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
	if( isWearingWearableType( WT_SKIRT ) && isTextureVisible(TEX_SKIRT_BAKED) )
	{
		gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.25f);
		num_indices += mMeshLOD[MESH_ID_SKIRT]->render(mAdjustedPixelArea, FALSE);
		first_pass = FALSE;
		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}

	if (!mIsSelf || gAgent.needsRenderHead() || LLPipeline::sShadowRender)
	{
		if (LLPipeline::sImpostorRender)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}
		
		if (isTextureVisible(TEX_HEAD_BAKED))
		{
			num_indices += mMeshLOD[MESH_ID_EYELASH]->render(mAdjustedPixelArea, first_pass);
			first_pass = FALSE;
		}
		if (isTextureVisible(TEX_HAIR_BAKED))
		{
			num_indices += mMeshLOD[MESH_ID_HAIR]->render(mAdjustedPixelArea, first_pass);
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

	if (mIsSelf && (!gAgent.needsRenderAvatar() || !gAgent.needsRenderHead()))
	{
		return 0;
	}
	
	if (!mIsBuilt)
	{
		return 0;
	}

	if (isTextureVisible(TEX_EYES_BAKED))
	{
		// If the meshes need to be drawn, enable alpha masking but not blending
		bool should_alpha_mask = mHasBakedHair && mBakedTextureData[BAKED_EYES].mIsLoaded;
		LLGLState test(GL_ALPHA_TEST, should_alpha_mask);
		
		if (should_alpha_mask)
		{
			gGL.setAlphaRejectSettings(LLRender::CF_GREATER, 0.5f);
		}

		num_indices += mMeshLOD[MESH_ID_EYEBALL_LEFT]->render(mAdjustedPixelArea);
		num_indices += mMeshLOD[MESH_ID_EYEBALL_RIGHT]->render(mAdjustedPixelArea);

		gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	}
	
	return num_indices;
}

U32 LLVOAvatar::renderFootShadows()
{
	U32 num_indices = 0;

	if (!mIsBuilt)
	{
		return 0;
	}

	if (mIsSelf && (!gAgent.needsRenderAvatar() || !gAgent.needsRenderHead()))
	{
		return 0;
	}
	
	if (!mIsBuilt)
	{
		return 0;
	}
	
	// Don't render foot shadows if your lower body is completely invisible.
	// (non-humanoid avatars rule!)
	if (! isTextureVisible(TEX_LOWER_BAKED))
	{
		return 0;
	}

	// Update the shadow, tractor, and text label geometry.
	if (mDrawable->isState(LLDrawable::REBUILD_SHADOW) && !isImpostor())
	{
		updateShadowFaces();
		mDrawable->clearState(LLDrawable::REBUILD_SHADOW);
	}

	U32 foot_mask = LLVertexBuffer::MAP_VERTEX |
					LLVertexBuffer::MAP_TEXCOORD0;

	LLGLDepthTest test(GL_TRUE, GL_FALSE);
	//render foot shadows
	LLGLEnable blend(GL_BLEND);
	gGL.getTexUnit(0)->bind(mShadowImagep.get());
	glColor4fv(mShadow0Facep->getRenderColor().mV);
	mShadow0Facep->renderIndexed(foot_mask);
	glColor4fv(mShadow1Facep->getRenderColor().mV);
	mShadow1Facep->renderIndexed(foot_mask);
	
	return num_indices;
}

U32 LLVOAvatar::renderImpostor(LLColor4U color)
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
	gGL.getTexUnit(0)->bind(&mImpostor);
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

//------------------------------------------------------------------------
// LLVOAvatar::updateTextures()
//------------------------------------------------------------------------
void LLVOAvatar::updateTextures(LLAgent &agent)
{
	BOOL render_avatar = TRUE;

	if (mIsDummy || gNoRender)
	{
		return;
	}

	if( mIsSelf )
	{
		render_avatar = TRUE;
	}
	else
	{
		render_avatar = isVisible() && !mCulled;
	}

	std::vector<bool> layer_baked;
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		layer_baked.push_back(isTextureDefined(mBakedTextureData[i].mTextureIndex));
		// bind the texture so that they'll be decoded slightly 
		// inefficient, we can short-circuit this if we have to
		if( render_avatar && !gGLManager.mIsDisabled )
		{
			if (layer_baked[i] && !mBakedTextureData[i].mIsLoaded)
			{
				gGL.getTexUnit(0)->bind(getTEImage( mBakedTextureData[i].mTextureIndex ));
			}
		}
	}

	/*
	// JAMESDEBUG
	if (mIsSelf)
	{
		S32 null_count = 0;
		S32 default_count = 0;
		for (U32 i = 0; i < getNumTEs(); i++)
		{
			const LLTextureEntry* te = getTE(i);
			if (te)
			{
				if (te->getID() == LLUUID::null)
				{
					null_count++;
				}
				else if (te->getID() == IMG_DEFAULT_AVATAR)
				{
					default_count++;
				}
			}
		}
		llinfos << "JAMESDEBUG my avatar TE null " << null_count << " default " << default_count << llendl;
	}
	*/

	mMaxPixelArea = 0.f;
	mMinPixelArea = 99999999.f;
	mHasGrey = FALSE; // debug
	for (U32 index = 0; index < getNumTEs(); index++)
	{
		LLViewerImage *imagep = getTEImage(index);
		if (imagep)
		{
			// Debugging code - maybe non-self avatars are downloading textures?
			//llinfos << "avatar self " << mIsSelf << " tex " << i 
			//	<< " decode " << imagep->getDecodePriority()
			//	<< " boost " << boost_avatar 
			//	<< " size " << imagep->getWidth() << "x" << imagep->getHeight()
			//	<< " discard " << imagep->getDiscardLevel()
			//	<< " desired " << imagep->getDesiredDiscardLevel()
			//	<< llendl;
			
			const LLTextureEntry *te = getTE(index);
			F32 texel_area_ratio = fabs(te->mScaleS * te->mScaleT);
			S32 boost_level = mIsSelf ? LLViewerImage::BOOST_AVATAR_BAKED_SELF : LLViewerImage::BOOST_AVATAR_BAKED;
			
			// Spam if this is a baked texture, not set to default image, without valid host info
			if (isIndexBakedTexture((ETextureIndex)index)
				&& imagep->getID() != IMG_DEFAULT_AVATAR
				&& !imagep->getTargetHost().isOk())
			{
				LL_WARNS_ONCE("Texture") << "LLVOAvatar::updateTextures No host for texture "
					<< imagep->getID() << " for avatar "
					<< (mIsSelf ? "<myself>" : getID().asString()) 
					<< " on host " << getRegion()->getHost() << llendl;
			}

			/* switch(index)
				case TEX_HEAD_BODYPAINT:
					addLocalTextureStats( LOCTEX_HEAD_BODYPAINT, imagep, texel_area_ratio, render_avatar, head_baked ); */
			const LLVOAvatarDictionary::TextureDictionaryEntry *texture_dict = LLVOAvatarDictionary::getInstance()->getTexture((ETextureIndex)index);
			if (texture_dict->mIsUsedByBakedTexture)
			{
				const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
				if (texture_dict->mIsLocalTexture)
				{
					addLocalTextureStats((ETextureIndex)index, imagep, texel_area_ratio, render_avatar, layer_baked[baked_index]);
				}
				else if (texture_dict->mIsBakedTexture)
				{
					if (layer_baked[baked_index])
					{
						addBakedTextureStats( imagep, mPixelArea, texel_area_ratio, boost_level );
					}
				}
			}
		}
	}

	if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
	{
		setDebugText(llformat("%4.0f:%4.0f", fsqrtf(mMinPixelArea),fsqrtf(mMaxPixelArea)));
	}	
	
	if( render_avatar )
	{
		mShadowImagep->addTextureStats(mPixelArea);
	}
}


void LLVOAvatar::addLocalTextureStats( ETextureIndex idx, LLViewerImage* imagep,
									   F32 texel_area_ratio, BOOL render_avatar, BOOL covered_by_baked )
{
	if (!isIndexLocalTexture(idx)) return;

	if (!covered_by_baked && render_avatar) // render_avatar is always true if mIsSelf
	{
		if (getLocalTextureID(idx) != IMG_DEFAULT_AVATAR)
		{
			F32 desired_pixels;
			if( mIsSelf )
			{
				desired_pixels = llmin(mPixelArea, (F32)TEX_IMAGE_AREA_SELF );
				imagep->setBoostLevel(LLViewerImage::BOOST_AVATAR_SELF);
			}
			else
			{
				desired_pixels = llmin(mPixelArea, (F32)TEX_IMAGE_AREA_OTHER );
				imagep->setBoostLevel(LLViewerImage::BOOST_AVATAR);
			}
			imagep->addTextureStats( desired_pixels / texel_area_ratio );
			if (imagep->getDiscardLevel() < 0)
			{
				mHasGrey = TRUE; // for statistics gathering
			}
		}
		else
		{
			// texture asset is missing
			mHasGrey = TRUE; // for statistics gathering
		}
	}
}

			    
void LLVOAvatar::addBakedTextureStats( LLViewerImage* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level)
{
	mMaxPixelArea = llmax(pixel_area, mMaxPixelArea);
	mMinPixelArea = llmin(pixel_area, mMinPixelArea);
	imagep->addTextureStats(pixel_area / texel_area_ratio);
	imagep->setBoostLevel(boost_level);
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
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	if (gNoRender)
	{
		return;
	}

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
	if (mIsSelf)
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
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
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
					//if (mIsSelf)
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
			mIsSitting = TRUE;
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
			mIsSitting = FALSE;
		}
		stopMotion(anim_id);
		result = TRUE;
	}

	return result;
}

//-----------------------------------------------------------------------------
// isAnyAnimationSignaled()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims)
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

//-----------------------------------------------------------------------------
// startMotion()
// id is the asset if of the animation to start
// time_offset is the offset into the animation at which to start playing
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::startMotion(const LLUUID& id, F32 time_offset)
{
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
	// start special case female walk for female avatars
	if (getSex() == SEX_FEMALE)
	{
		if (id == ANIM_AGENT_WALK)
		{
			return LLCharacter::startMotion(ANIM_AGENT_FEMALE_WALK, time_offset);
		}
		else if (id == ANIM_AGENT_SIT)
		{
			return LLCharacter::startMotion(ANIM_AGENT_SIT_FEMALE, time_offset);
		}
	}

	if (mIsSelf && id == ANIM_AGENT_AWAY)
	{
		gAgent.setAFK();
	}

	return LLCharacter::startMotion(id, time_offset);
}

//-----------------------------------------------------------------------------
// stopMotion()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::stopMotion(const LLUUID& id, BOOL stop_immediate)
{
	if (mIsSelf)
	{
		gAgent.onAnimStop(id);
	}

	if (id == ANIM_AGENT_WALK)
	{
		LLCharacter::stopMotion(ANIM_AGENT_FEMALE_WALK, stop_immediate);
	}
	else if (id == ANIM_AGENT_SIT)
	{
		LLCharacter::stopMotion(ANIM_AGENT_SIT_FEMALE, stop_immediate);
	}

	return LLCharacter::stopMotion(id, stop_immediate);
}

//-----------------------------------------------------------------------------
// stopMotionFromSource()
//-----------------------------------------------------------------------------
void LLVOAvatar::stopMotionFromSource(const LLUUID& source_id)
{
	if (!mIsSelf)
	{
		return;
	}
	AnimSourceIterator motion_it;

	for(motion_it = mAnimationSources.find(source_id); motion_it != mAnimationSources.end();)
	{
		gAgent.sendAnimationRequest( motion_it->second, ANIM_REQUEST_STOP );
		mAnimationSources.erase(motion_it++);
	}

	LLViewerObject* object = gObjectList.findObject(source_id);
	if (object)
	{
		object->mFlags &= ~FLAGS_ANIM_SOURCE;
	}
}

//-----------------------------------------------------------------------------
// getVolumePos()
//-----------------------------------------------------------------------------
LLVector3 LLVOAvatar::getVolumePos(S32 joint_index, LLVector3& volume_offset)
{
	if (joint_index > mNumCollisionVolumes)
	{
		return LLVector3::zero;
	}

	return mCollisionVolumes[joint_index].getVolumePos(volume_offset);
}

//-----------------------------------------------------------------------------
// findCollisionVolume()
//-----------------------------------------------------------------------------
LLJoint* LLVOAvatar::findCollisionVolume(U32 volume_id)
{
	if ((S32)volume_id > mNumCollisionVolumes)
	{
		return NULL;
	}
	
	return &mCollisionVolumes[volume_id];
}

//-----------------------------------------------------------------------------
// findCollisionVolume()
//-----------------------------------------------------------------------------
S32 LLVOAvatar::getCollisionVolumeID(std::string &name)
{
	for (S32 i = 0; i < mNumCollisionVolumes; i++)
	{
		if (mCollisionVolumes[i].getName() == name)
		{
			return i;
		}
	}

	return -1;
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
const LLUUID& LLVOAvatar::getID()
{
	return mID;
}

//-----------------------------------------------------------------------------
// getJoint()
//-----------------------------------------------------------------------------
// RN: avatar joints are multi-rooted to include screen-based attachments
LLJoint *LLVOAvatar::getJoint( const std::string &name )
{
	LLJoint* jointp = NULL;
	if (mScreenp)
	{
		jointp = mScreenp->findJoint(name);
	}
	if (!jointp)
	{
		jointp = mRoot.findJoint(name);
	}
	return jointp;
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

	if (gNoRender || mIsDummy)
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
// LLVOAvatar::getHeadMesh()
//-----------------------------------------------------------------------------
LLPolyMesh*	LLVOAvatar::getHeadMesh()
{
	return mMeshLOD[MESH_ID_HEAD]->mMeshParts[0]->getMesh();
}


//-----------------------------------------------------------------------------
// LLVOAvatar::getUpperBodyMesh()
//-----------------------------------------------------------------------------
LLPolyMesh*	LLVOAvatar::getUpperBodyMesh()
{
	return mMeshLOD[MESH_ID_UPPER_BODY]->mMeshParts[0]->getMesh();
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
// allocateCharacterJoints()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::allocateCharacterJoints( U32 num )
{
	delete [] mSkeleton;
	mSkeleton = NULL;
	mNumJoints = 0;

	mSkeleton = new LLViewerJoint[num];
	
	for(S32 joint_num = 0; joint_num < (S32)num; joint_num++)
	{
		mSkeleton[joint_num].setJointNum(joint_num);
	}

	if (!mSkeleton)
	{
		return FALSE;
	}

	mNumJoints = num;
	return TRUE;
}

//-----------------------------------------------------------------------------
// allocateCollisionVolumes()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::allocateCollisionVolumes( U32 num )
{
	delete [] mCollisionVolumes;
	mCollisionVolumes = NULL;
	mNumCollisionVolumes = 0;

	mCollisionVolumes = new LLViewerJointCollisionVolume[num];
	if (!mCollisionVolumes)
	{
		return FALSE;
	}

	mNumCollisionVolumes = num;
	return TRUE;
}


//-----------------------------------------------------------------------------
// getCharacterJoint()
//-----------------------------------------------------------------------------
LLJoint *LLVOAvatar::getCharacterJoint( U32 num )
{
	if ((S32)num >= mNumJoints 
	    || (S32)num < 0)
	{
		return NULL;
	}
	return (LLJoint*)&mSkeleton[num];
}

//-----------------------------------------------------------------------------
// requestStopMotion()
//-----------------------------------------------------------------------------
void LLVOAvatar::requestStopMotion( LLMotion* motion )
{
	// Only agent avatars should handle the stop motion notifications.
	if ( mIsSelf )
	{
		// Notify agent that motion has stopped
		gAgent.requestStopMotion( motion );
	}
}

//-----------------------------------------------------------------------------
// loadAvatar()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::loadAvatar()
{
// 	LLFastTimer t(LLFastTimer::FTM_LOAD_AVATAR);
	
	// avatar_skeleton.xml
	if( !buildSkeleton(sAvatarSkeletonInfo) )
	{
		llwarns << "avatar file: buildSkeleton() failed" << llendl;
		return FALSE;
	}

	// avatar_lad.xml : <skeleton>
	if( !loadSkeletonNode() )
	{
		llwarns << "avatar file: loadNodeSkeleton() failed" << llendl;
		return FALSE;
	}
	
	// avatar_lad.xml : <mesh>
	if( !loadMeshNodes() )
	{
		llwarns << "avatar file: loadNodeMesh() failed" << llendl;
		return FALSE;
	}
	
	// avatar_lad.xml : <global_color>
	if( sAvatarXmlInfo->mTexSkinColorInfo )
	{
		mTexSkinColor = new LLTexGlobalColor( this );
		if( !mTexSkinColor->setInfo( sAvatarXmlInfo->mTexSkinColorInfo ) )
		{
			llwarns << "avatar file: mTexSkinColor->setInfo() failed" << llendl;
			return FALSE;
		}
	}
	else
	{
		llwarns << "<global_color> name=\"skin_color\" not found" << llendl;
		return FALSE;
	}
	if( sAvatarXmlInfo->mTexHairColorInfo )
	{
		mTexHairColor = new LLTexGlobalColor( this );
		if( !mTexHairColor->setInfo( sAvatarXmlInfo->mTexHairColorInfo ) )
		{
			llwarns << "avatar file: mTexHairColor->setInfo() failed" << llendl;
			return FALSE;
		}
	}
	else
	{
		llwarns << "<global_color> name=\"hair_color\" not found" << llendl;
		return FALSE;
	}
	if( sAvatarXmlInfo->mTexEyeColorInfo )
	{
		mTexEyeColor = new LLTexGlobalColor( this );
		if( !mTexEyeColor->setInfo( sAvatarXmlInfo->mTexEyeColorInfo ) )
		{
			llwarns << "avatar file: mTexEyeColor->setInfo() failed" << llendl;
			return FALSE;
		}
	}
	else
	{
		llwarns << "<global_color> name=\"eye_color\" not found" << llendl;
		return FALSE;
	}
	
	// avatar_lad.xml : <layer_set>
	if (sAvatarXmlInfo->mLayerInfoList.empty())
	{
		llwarns << "avatar file: missing <layer_set> node" << llendl;
	}
	else
	{
		LLVOAvatarXmlInfo::layer_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mLayerInfoList.begin();
			 iter != sAvatarXmlInfo->mLayerInfoList.end(); iter++)
		{
			LLTexLayerSetInfo *info = *iter;
			LLTexLayerSet* layer_set = new LLTexLayerSet( this );
			if (!layer_set->setInfo(info))
			{
				stop_glerror();
				delete layer_set;
				llwarns << "avatar file: layer_set->parseData() failed" << llendl;
				return FALSE;
			}
			bool found_baked_entry = false;
			for (LLVOAvatarDictionary::baked_map_t::const_iterator baked_iter = LLVOAvatarDictionary::getInstance()->getBakedTextures().begin();
				 baked_iter != LLVOAvatarDictionary::getInstance()->getBakedTextures().end();
				 baked_iter++)
			{
				const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = baked_iter->second;
				if (layer_set->isBodyRegion(baked_dict->mName))
				{
					mBakedTextureData[baked_iter->first].mTexLayerSet = layer_set;
					found_baked_entry = true;
					break;
				}
			}
			if (!found_baked_entry)
			{
				llwarns << "<layer_set> has invalid body_region attribute" << llendl;
				delete layer_set;
				return FALSE;
			}
		}
	}
	
	// avatar_lad.xml : <driver_parameters>
	{
		LLVOAvatarXmlInfo::driver_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mDriverInfoList.begin();
			 iter != sAvatarXmlInfo->mDriverInfoList.end(); iter++)
		{
			LLDriverParamInfo *info = *iter;
			LLDriverParam* driver_param = new LLDriverParam( this );
			if (driver_param->setInfo(info))
			{
				addVisualParam( driver_param );
			}
			else
			{
				delete driver_param;
				llwarns << "avatar file: driver_param->parseData() failed" << llendl;
				return FALSE;
			}
		}
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
// loadSkeletonNode(): loads <skeleton> node from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::loadSkeletonNode ()
{
	mRoot.addChild( &mSkeleton[0] );

	for (std::vector<LLViewerJoint *>::iterator iter = mMeshLOD.begin();
		iter != mMeshLOD.end(); iter++)
	{
		LLViewerJoint *joint = (LLViewerJoint *) *iter;
		joint->mUpdateXform = FALSE;
		joint->setMeshesToChildren();
	}

	mRoot.addChild(mMeshLOD[MESH_ID_HEAD]);
	mRoot.addChild(mMeshLOD[MESH_ID_EYELASH]);
	mRoot.addChild(mMeshLOD[MESH_ID_UPPER_BODY]);
	mRoot.addChild(mMeshLOD[MESH_ID_LOWER_BODY]);
	mRoot.addChild(mMeshLOD[MESH_ID_SKIRT]);
	mRoot.addChild(mMeshLOD[MESH_ID_HEAD]);

	LLViewerJoint *skull = (LLViewerJoint*)mRoot.findJoint("mSkull");
	if (skull)
	{
		skull->addChild(mMeshLOD[MESH_ID_HAIR] );
	}

	LLViewerJoint *eyeL = (LLViewerJoint*)mRoot.findJoint("mEyeLeft");
	if (eyeL)
	{
		eyeL->addChild( mMeshLOD[MESH_ID_EYEBALL_LEFT] );
	}

	LLViewerJoint *eyeR = (LLViewerJoint*)mRoot.findJoint("mEyeRight");
	if (eyeR)
	{
		eyeR->addChild( mMeshLOD[MESH_ID_EYEBALL_RIGHT] );
	}

	// SKELETAL DISTORTIONS
	{
		LLVOAvatarXmlInfo::skeletal_distortion_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mSkeletalDistortionInfoList.begin();
			 iter != sAvatarXmlInfo->mSkeletalDistortionInfoList.end(); iter++)
		{
			LLPolySkeletalDistortionInfo *info = *iter;
			LLPolySkeletalDistortion *param = new LLPolySkeletalDistortion(this);
			if (!param->setInfo(info))
			{
				delete param;
				return FALSE;
			}
			else
			{
				addVisualParam(param);
			}				
		}
	}
	
	// ATTACHMENTS
	{
		LLVOAvatarXmlInfo::attachment_info_list_t::iterator iter;
		for (iter = sAvatarXmlInfo->mAttachmentInfoList.begin();
			 iter != sAvatarXmlInfo->mAttachmentInfoList.end(); iter++)
		{
			LLVOAvatarXmlInfo::LLVOAvatarAttachmentInfo *info = *iter;
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
// loadMeshNodes(): loads <mesh> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::loadMeshNodes()
{
	for (LLVOAvatarXmlInfo::mesh_info_list_t::const_iterator meshinfo_iter = sAvatarXmlInfo->mMeshInfoList.begin();
		 meshinfo_iter != sAvatarXmlInfo->mMeshInfoList.end(); 
		 meshinfo_iter++)
	{
		const LLVOAvatarXmlInfo::LLVOAvatarMeshInfo *info = *meshinfo_iter;
		const std::string &type = info->mType;
		S32 lod = info->mLOD;

		LLViewerJointMesh* mesh = NULL;
		U8 mesh_id = 0;
		BOOL found_mesh_id = FALSE;

		/* if (type == "hairMesh")
			switch(lod)
			  case 0:
				mesh = &mHairMesh0; */
		for (LLVOAvatarDictionary::mesh_map_t::const_iterator mesh_iter = LLVOAvatarDictionary::getInstance()->getMeshes().begin();
			 mesh_iter != LLVOAvatarDictionary::getInstance()->getMeshes().end();
			 mesh_iter++)
		{
			const EMeshIndex mesh_index = mesh_iter->first;
			const LLVOAvatarDictionary::MeshDictionaryEntry *mesh_dict = mesh_iter->second;
			if (type.compare(mesh_dict->mName) == 0)
			{
				mesh_id = mesh_index;
				found_mesh_id = TRUE;
				break;
			}
		}

		if (found_mesh_id)
		{
			if (lod < (S32)mMeshLOD[mesh_id]->mMeshParts.size())
			{
				mesh = mMeshLOD[mesh_id]->mMeshParts[lod];
			}
			else
			{
				llwarns << "Avatar file: <mesh> has invalid lod setting " << lod << llendl;
				return FALSE;
			}
		}
		else 
		{
			llwarns << "Ignoring unrecognized mesh type: " << type << llendl;
			return FALSE;
		}

		//	llinfos << "Parsing mesh data for " << type << "..." << llendl;

		// If this isn't set to white (1.0), avatars will *ALWAYS* be darker than their surroundings.
		// Do not touch!!!
		mesh->setColor( 1.0f, 1.0f, 1.0f, 1.0f );

		LLPolyMesh *poly_mesh = NULL;

		if (!info->mReferenceMeshName.empty())
		{
			polymesh_map_t::const_iterator polymesh_iter = mMeshes.find(info->mReferenceMeshName);
			if (polymesh_iter != mMeshes.end())
			{
				poly_mesh = LLPolyMesh::getMesh(info->mMeshFileName, polymesh_iter->second);
				poly_mesh->setAvatar(this);
			}
			else
			{
				// This should never happen
				LL_WARNS("Avatar") << "Could not find avatar mesh: " << info->mReferenceMeshName << LL_ENDL;
			}
		}
		else
		{
			poly_mesh = LLPolyMesh::getMesh(info->mMeshFileName);
			poly_mesh->setAvatar(this);
		}

		if( !poly_mesh )
		{
			llwarns << "Failed to load mesh of type " << type << llendl;
			return FALSE;
		}

		// Multimap insert
		mMeshes.insert(std::make_pair(info->mMeshFileName, poly_mesh));
	
		mesh->setMesh( poly_mesh );
		mesh->setLOD( info->mMinPixelArea );

		for (LLVOAvatarXmlInfo::LLVOAvatarMeshInfo::morph_info_list_t::const_iterator xmlinfo_iter = info->mPolyMorphTargetInfoList.begin();
			 xmlinfo_iter != info->mPolyMorphTargetInfoList.end(); 
			 xmlinfo_iter++)
		{
			const LLVOAvatarXmlInfo::LLVOAvatarMeshInfo::morph_info_pair_t *info_pair = &(*xmlinfo_iter);
			LLPolyMorphTarget *param = new LLPolyMorphTarget(mesh->getMesh());
			if (!param->setInfo(info_pair->first))
			{
				delete param;
				return FALSE;
			}
			else
			{
				if (info_pair->second)
				{
					addSharedVisualParam(param);
				}
				else
				{
					addVisualParam(param);
				}
			}				
		}
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// updateVisualParams()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateVisualParams()
{
	if (gNoRender)
	{
		return;
	}

	setSex( (getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE );

	LLCharacter::updateVisualParams();

	if (mLastSkeletonSerialNum != mSkeletonSerialNum)
	{
		computeBodySize();
		mLastSkeletonSerialNum = mSkeletonSerialNum;
		mRoot.updateWorldMatrixChildren();
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
	LLMemType mt(LLMemType::MTYPE_AVATAR);

	if (mDrawable.isNull())
	{
		return;
	}

	const LLVector3* ext = mDrawable->getSpatialExtents();
	LLVector3 center = (ext[1] + ext[0]) * 0.5f;
	LLVector3 size = (ext[1]-ext[0])*0.5f;

	mImpostorPixelArea = LLPipeline::calcPixelArea(center, size, *LLViewerCamera::getInstance());

	F32 range = mDrawable->mDistanceWRTCamera;

	if (range < 0.001f)		// range == zero
	{
		mAppAngle = 180.f;
	}
	else
	{
		F32 radius = size.length();
		mAppAngle = (F32) atan2( radius, range) * RAD_TO_DEG;
	}

	// We always want to look good to ourselves
	if( mIsSelf )
	{
		mPixelArea = llmax( mPixelArea, F32(TEX_IMAGE_SIZE_SELF / 16) );
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

	{
		if (mIsSelf)
		{
			if(gAgent.cameraCustomizeAvatar() || gAgent.cameraMouselook())
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
		BOOL res = mRoot.updateLOD(mAdjustedPixelArea, TRUE);
 		if (res)
		{
			sNumLODChangesThisFrame++;
			dirtyMesh();
			return TRUE;
		}
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
	
	LLFace *facep;

	// Add faces for the foot shadows
	facep = mDrawable->addFace((LLFacePool*) NULL, mShadowImagep);
	mShadow0Facep = facep;

	facep = mDrawable->addFace((LLFacePool*) NULL, mShadowImagep);
	mShadow1Facep = facep;

	mNumInitFaces = mDrawable->getNumFaces() ;

	dirtyMesh();
	return mDrawable;
}


//-----------------------------------------------------------------------------
// updateGeometry()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(LLFastTimer::FTM_UPDATE_AVATAR);
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
// updateShadowFaces()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateShadowFaces()
{
	LLFace *face0p = mShadow0Facep;
	LLFace *face1p = mShadow1Facep;

	//
	// render avatar shadows
	//
	if (mInAir || mUpdatePeriod >= IMPOSTOR_PERIOD)
	{
		face0p->setSize(0, 0);
		face1p->setSize(0, 0);
		return;
	}

	LLSprite sprite(mShadowImagep.notNull() ? mShadowImagep->getID() : LLUUID::null);
	sprite.setFollow(FALSE);
	const F32 cos_angle = gSky.getSunDirection().mV[2];
	F32 cos_elev = sqrt(1 - cos_angle * cos_angle);
	if (cos_angle < 0) cos_elev = -cos_elev;
	sprite.setSize(0.4f + cos_elev * 0.8f, 0.3f);
	LLVector3 sun_vec = gSky.mVOSkyp ? gSky.mVOSkyp->getToSun() : LLVector3(0.f, 0.f, 0.f);

	if (mShadowImagep->getHasGLTexture())
	{
		LLVector3 normal;
		LLVector3d shadow_pos;
		LLVector3 shadow_pos_agent;
		F32 foot_height;

		if (mFootLeftp)
		{
			LLVector3 joint_world_pos = mFootLeftp->getWorldPosition();
			// this only does a ray straight down from the foot, as our client-side ray-tracing is very limited now
			// but we make an explicit ray trace call in expectation of future improvements
			resolveRayCollisionAgent(gAgent.getPosGlobalFromAgent(joint_world_pos), 
				gAgent.getPosGlobalFromAgent(gSky.getSunDirection() + joint_world_pos), shadow_pos, normal);
			shadow_pos_agent = gAgent.getPosAgentFromGlobal(shadow_pos);
			foot_height = joint_world_pos.mV[VZ] - shadow_pos_agent.mV[VZ];

			// Pull sprite in direction of surface normal
			shadow_pos_agent += normal * SHADOW_OFFSET_AMT;

			// Render sprite
			sprite.setNormal(normal);
			if (mIsSelf && gAgent.getCameraMode() == CAMERA_MODE_MOUSELOOK)
			{
				sprite.setColor(0.f, 0.f, 0.f, 0.f);
			}
			else
			{
				sprite.setColor(0.f, 0.f, 0.f, clamp_rescale(foot_height, MIN_SHADOW_HEIGHT, MAX_SHADOW_HEIGHT, 0.5f, 0.f));
			}
			sprite.setPosition(shadow_pos_agent);

			LLVector3 foot_to_knee = mKneeLeftp->getWorldPosition() - joint_world_pos;
			//foot_to_knee.normalize();
			foot_to_knee -= projected_vec(foot_to_knee, sun_vec);
			sprite.setYaw(azimuth(sun_vec - foot_to_knee));
		
			sprite.updateFace(*face0p);
		}

		if (mFootRightp)
		{
			LLVector3 joint_world_pos = mFootRightp->getWorldPosition();
			// this only does a ray straight down from the foot, as our client-side ray-tracing is very limited now
			// but we make an explicit ray trace call in expectation of future improvements
			resolveRayCollisionAgent(gAgent.getPosGlobalFromAgent(joint_world_pos), 
				gAgent.getPosGlobalFromAgent(gSky.getSunDirection() + joint_world_pos), shadow_pos, normal);
			shadow_pos_agent = gAgent.getPosAgentFromGlobal(shadow_pos);
			foot_height = joint_world_pos.mV[VZ] - shadow_pos_agent.mV[VZ];

			// Pull sprite in direction of surface normal
			shadow_pos_agent += normal * SHADOW_OFFSET_AMT;

			// Render sprite
			sprite.setNormal(normal);
			if (mIsSelf && gAgent.getCameraMode() == CAMERA_MODE_MOUSELOOK)
			{
				sprite.setColor(0.f, 0.f, 0.f, 0.f);
			}
			else
			{
				sprite.setColor(0.f, 0.f, 0.f, clamp_rescale(foot_height, MIN_SHADOW_HEIGHT, MAX_SHADOW_HEIGHT, 0.5f, 0.f));
			}
			sprite.setPosition(shadow_pos_agent);

			LLVector3 foot_to_knee = mKneeRightp->getWorldPosition() - joint_world_pos;
			//foot_to_knee.normalize();
			foot_to_knee -= projected_vec(foot_to_knee, sun_vec);
			sprite.setYaw(azimuth(sun_vec - foot_to_knee));
	
			sprite.updateFace(*face1p);
		}
	}
}

//-----------------------------------------------------------------------------
// updateSexDependentLayerSets()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateSexDependentLayerSets( BOOL set_by_user )
{
	invalidateComposite( mBakedTextureData[BAKED_HEAD].mTexLayerSet,  set_by_user );
	invalidateComposite( mBakedTextureData[BAKED_UPPER].mTexLayerSet,	set_by_user );
	invalidateComposite( mBakedTextureData[BAKED_LOWER].mTexLayerSet,	set_by_user );
	updateMeshTextures();
}

//-----------------------------------------------------------------------------
// dirtyMesh()
//-----------------------------------------------------------------------------
void LLVOAvatar::dirtyMesh()
{
	mDirtyMesh = TRUE;
}

//-----------------------------------------------------------------------------
// hideSkirt()
//-----------------------------------------------------------------------------
void LLVOAvatar::hideSkirt()
{
	mMeshLOD[MESH_ID_SKIRT]->setVisible(FALSE, TRUE);
}


//-----------------------------------------------------------------------------
// requestLayerSetUpdate()
//-----------------------------------------------------------------------------
void LLVOAvatar::requestLayerSetUpdate(ETextureIndex index )
{
	/* switch(index)
		case LOCTEX_UPPER_BODYPAINT:  
		case LOCTEX_UPPER_SHIRT:
			if( mUpperBodyLayerSet )
				mUpperBodyLayerSet->requestUpdate(); */
	const LLVOAvatarDictionary::TextureDictionaryEntry *texture_dict = LLVOAvatarDictionary::getInstance()->getTexture(index);
	if (!texture_dict->mIsLocalTexture || !texture_dict->mIsUsedByBakedTexture)
		return;
	const EBakedTextureIndex baked_index = texture_dict->mBakedTextureIndex;
	if (mBakedTextureData[baked_index].mTexLayerSet)
	{
		mBakedTextureData[baked_index].mTexLayerSet->requestUpdate();
	}
}

void LLVOAvatar::setParent(LLViewerObject* parent)
{
	if (parent == NULL)
	{
		getOffObject();
		LLViewerObject::setParent(parent);
		if (isSelf())
		{
			gAgent.resetCamera();
		}
	}
	else
	{
		LLViewerObject::setParent(parent);
		sitOnObject(parent);
	}
}

void LLVOAvatar::addChild(LLViewerObject *childp)
{
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
	detachObject(childp);
}

LLViewerJointAttachment* LLVOAvatar::getTargetAttachmentPoint(LLViewerObject* viewer_object)
{
	S32 attachmentID = ATTACHMENT_ID_FROM_STATE(viewer_object->getState());

	LLViewerJointAttachment* attachment = get_if_there(mAttachmentPoints, attachmentID, (LLViewerJointAttachment*)NULL);

	if (!attachment)
	{
		llwarns << "Object attachment point invalid: " << attachmentID << llendl;
	}

	return attachment;
}

//-----------------------------------------------------------------------------
// attachObject()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::attachObject(LLViewerObject *viewer_object)
{
	LLViewerJointAttachment* attachment = getTargetAttachmentPoint(viewer_object);

	if (!attachment || !attachment->addObject(viewer_object))
	{
		return FALSE;
	}

	if (viewer_object->isSelected())
	{
		LLSelectMgr::getInstance()->updateSelectionCenter();
		LLSelectMgr::getInstance()->updatePointAt();
	}

	if (mIsSelf)
	{
		updateAttachmentVisibility(gAgent.getCameraMode());
		
		// Then make sure the inventory is in sync with the avatar.
		gInventory.addChangedMask( LLInventoryObserver::LABEL, attachment->getItemID() );
		gInventory.notifyObservers();
	}

	return TRUE;
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
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getIsHUDAttachment())
		{
			LLViewerObject* obj = attachment->getObject();
			if (obj && obj->mDrawable.notNull())
			{
				gPipeline.markMoved(obj->mDrawable);
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
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		// only one object per attachment point for now
		if (attachment->getObject() == viewer_object)
		{
			LLUUID item_id = attachment->getItemID();
			attachment->removeObject(viewer_object);
			if (mIsSelf)
			{
				// the simulator should automatically handle
				// permission revocation

				stopMotionFromSource(viewer_object->getID());
				LLFollowCamMgr::setCameraActive(viewer_object->getID(), FALSE);

				LLViewerObject::const_child_list_t& child_list = viewer_object->getChildren();
				for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
					 iter != child_list.end(); iter++)
				{
					LLViewerObject* child_objectp = *iter;
					// the simulator should automatically handle
					// permissions revocation

					stopMotionFromSource(child_objectp->getID());
					LLFollowCamMgr::setCameraActive(child_objectp->getID(), FALSE);
				}

			}
			lldebugs << "Detaching object " << viewer_object->mID << " from " << attachment->getName() << llendl;
			if (mIsSelf)
			{
				// Then make sure the inventory is in sync with the avatar.
				gInventory.addChangedMask(LLInventoryObserver::LABEL, item_id);
				gInventory.notifyObservers();
			}
			return TRUE;
		}
	}

	
	return FALSE;
}

//-----------------------------------------------------------------------------
// sitOnObject()
//-----------------------------------------------------------------------------
void LLVOAvatar::sitOnObject(LLViewerObject *sit_object)
{
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
	mIsSitting = TRUE;
	mRoot.getXform()->setParent(&sit_object->mDrawable->mXform); // LLVOAvatar::sitOnObject
	mRoot.setPosition(getPosition());
	mRoot.updateWorldMatrixChildren();

	stopMotion(ANIM_AGENT_BODY_NOISE);

	if (mIsSelf)
	{
		// Might be first sit
		LLFirstUse::useSit();

		gAgent.setFlying(FALSE);
		gAgent.setThirdPersonHeadOffset(LLVector3::zero);
		//interpolate to new camera position
		gAgent.startCameraAnimation();
		// make sure we are not trying to autopilot
		gAgent.stopAutoPilot();
		gAgent.setupSitCamera();
		if (gAgent.mForceMouselook) gAgent.changeCameraToMouselook();
	}
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
			 iter != child_list.end(); iter++)
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

	mIsSitting = FALSE;
	mRoot.getXform()->setParent(NULL); // LLVOAvatar::getOffObject
	mRoot.setPosition(cur_position_world);
	mRoot.setRotation(cur_rotation_world);
	mRoot.getXform()->update();

	startMotion(ANIM_AGENT_BODY_NOISE);

	if (mIsSelf)
	{
		LLQuaternion av_rot = gAgent.getFrameAgent().getQuaternion();
		LLQuaternion obj_rot = sit_object ? sit_object->getRenderRotation() : LLQuaternion::DEFAULT;
		av_rot = av_rot * obj_rot;
		LLVector3 at_axis = LLVector3::x_axis;
		at_axis = at_axis * av_rot;
		at_axis.mV[VZ] = 0.f;
		at_axis.normalize();
		gAgent.resetAxes(at_axis);

		//reset orientation
//		mRoot.setRotation(avWorldRot);
		gAgent.setThirdPersonHeadOffset(LLVector3(0.f, 0.f, 1.f));

		gAgent.setSitCamera(LLUUID::null);
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

//-----------------------------------------------------------------------------
// isWearingAttachment()
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::isWearingAttachment( const LLUUID& inv_item_id )
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if( attachment->getItemID() == inv_item_id )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// getWornAttachment()
//-----------------------------------------------------------------------------
LLViewerObject* LLVOAvatar::getWornAttachment( const LLUUID& inv_item_id )
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if( attachment->getItemID() == inv_item_id )
		{
			return attachment->getObject();
		}
	}
	return NULL;
}

const std::string LLVOAvatar::getAttachedPointName(const LLUUID& inv_item_id)
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if( attachment->getItemID() == inv_item_id )
		{
			return attachment->getName();
		}
	}

	return LLStringUtil::null;
}


//-----------------------------------------------------------------------------
// static 
// onLocalTextureLoaded()
//-----------------------------------------------------------------------------

void LLVOAvatar::onLocalTextureLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src_raw, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	//llinfos << "onLocalTextureLoaded: " << src_vi->getID() << llendl;

	const LLUUID& src_id = src_vi->getID();
	LLAvatarTexData *data = (LLAvatarTexData *)userdata;
	if (success)
	{
		LLVOAvatar *self = (LLVOAvatar *)gObjectList.findObject(data->mAvatarID);
		if (self)
		{
			ETextureIndex index = data->mIndex;
			if (!self->isIndexLocalTexture(index)) return;
			LocalTextureData &local_tex_data = self->mLocalTextureData[index];
			if(!local_tex_data.mIsBakedReady &&
			   local_tex_data.mImage.notNull() &&
			   (local_tex_data.mImage->getID() == src_id) &&
			   discard_level < local_tex_data.mDiscard)
			{
				local_tex_data.mDiscard = discard_level;
				if ( self->isSelf() && !gAgent.cameraCustomizeAvatar() )
				{
					self->requestLayerSetUpdate( index );
				}
				else if( self->isSelf() && gAgent.cameraCustomizeAvatar() )
				{
					LLVisualParamHint::requestHintUpdates();
				}
				self->updateMeshTextures();
			}
		}
	}
	else if (final)
	{
		LLVOAvatar *self = (LLVOAvatar *)gObjectList.findObject(data->mAvatarID);
		if (self)
		{
			ETextureIndex index = data->mIndex;
			if (!self->isIndexLocalTexture(index)) return;
			LocalTextureData &local_tex_data = self->mLocalTextureData[index];
			// Failed: asset is missing
			if(!local_tex_data.mIsBakedReady &&
			   local_tex_data.mImage.notNull() &&
			   local_tex_data.mImage->getID() == src_id)
			{
				local_tex_data.mDiscard = 0;
				self->requestLayerSetUpdate( index );
				self->updateMeshTextures();
			}
		}
	}

	if( final || !success )
	{
		delete data;
	}
}

void LLVOAvatar::updateComposites()
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if ( mBakedTextureData[i].mTexLayerSet 
			&& ((i != BAKED_SKIRT) || isWearingWearableType( WT_SKIRT )) )
		{
			mBakedTextureData[i].mTexLayerSet->updateComposite();
		}
	}
}

LLColor4 LLVOAvatar::getGlobalColor( const std::string& color_name )
{
	if( color_name=="skin_color" && mTexSkinColor )
	{
		return mTexSkinColor->getColor();
	}
	else
	if( color_name=="hair_color" && mTexHairColor )
	{
		return mTexHairColor->getColor();
	}
	if( color_name=="eye_color" && mTexEyeColor )
	{
		return mTexEyeColor->getColor();
	}
	else
	{
//		return LLColor4( .5f, .5f, .5f, .5f );
		return LLColor4( 0.f, 1.f, 1.f, 1.f ); // good debugging color
	}
}


void LLVOAvatar::invalidateComposite( LLTexLayerSet* layerset, BOOL set_by_user )
{
	if( !layerset || !layerset->getUpdatesEnabled() )
	{
		return;
	}

	/* Debug spam. JC
	const char* layer_name = "";
	if (layerset == mHeadLayerSet)
	{
		layer_name = "head";
	}
	else if (layerset == mUpperBodyLayerSet)
	{
		layer_name = "upperbody";
	}
	else if (layerset == mLowerBodyLayerSet)
	{
		layer_name = "lowerbody";
	}
	else if (layerset == mEyesLayerSet)
	{
		layer_name = "eyes";
	}
	else if (layerset == mHairLayerSet)
	{
		layer_name = "hair";
	}
	else if (layerset == mSkirtLayerSet)
	{
		layer_name = "skirt";
	}
	else
	{
		layer_name = "unknown";
	}
	llinfos << "LLVOAvatar::invalidComposite() " << layer_name << llendl;
	*/

	layerset->requestUpdate();

	if( set_by_user )
	{
		llassert( mIsSelf );

		ETextureIndex baked_te = getBakedTE( layerset );
		setTEImage( baked_te, gImageList.getImage(IMG_DEFAULT_AVATAR) );
		layerset->requestUpload();
	}
}

void LLVOAvatar::invalidateAll()
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		invalidateComposite(mBakedTextureData[i].mTexLayerSet, TRUE);
	}
	updateMeshTextures();
}

void LLVOAvatar::onGlobalColorChanged( LLTexGlobalColor* global_color, BOOL set_by_user )
{
	if( global_color == mTexSkinColor )
	{
//		llinfos << "invalidateComposite cause: onGlobalColorChanged( skin color )" << llendl; 
		invalidateComposite( mBakedTextureData[BAKED_HEAD].mTexLayerSet,  set_by_user );
		invalidateComposite( mBakedTextureData[BAKED_UPPER].mTexLayerSet,	set_by_user );
		invalidateComposite( mBakedTextureData[BAKED_LOWER].mTexLayerSet,	set_by_user );
	}
	else
	if( global_color == mTexHairColor )
	{
//		llinfos << "invalidateComposite cause: onGlobalColorChanged( hair color )" << llendl; 
		invalidateComposite( mBakedTextureData[BAKED_HEAD].mTexLayerSet,  set_by_user );
		invalidateComposite( mBakedTextureData[BAKED_HAIR].mTexLayerSet,  set_by_user );
		
		// ! BACKWARDS COMPATIBILITY !
		// Fix for dealing with avatars from viewers that don't bake hair.
		if (!isTextureDefined(mBakedTextureData[BAKED_HAIR].mTextureIndex))
		{
			LLColor4 color = mTexHairColor->getColor();
			for (U32 i = 0; i < mBakedTextureData[BAKED_HAIR].mMeshes.size(); i++)
			{
				mBakedTextureData[BAKED_HAIR].mMeshes[i]->setColor( color.mV[VX], color.mV[VY], color.mV[VZ], color.mV[VW] );
			}
		}
	}
	else
	if( global_color == mTexEyeColor )
	{
//		llinfos << "invalidateComposite cause: onGlobalColorChanged( eyecolor )" << llendl; 
		invalidateComposite( mBakedTextureData[BAKED_EYES].mTexLayerSet,  set_by_user );
	}
	updateMeshTextures();
}

void LLVOAvatar::forceBakeAllTextures(bool slam_for_debug)
{
	llinfos << "TAT: forced full rebake. " << llendl;

	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		ETextureIndex baked_index = mBakedTextureData[i].mTextureIndex;
		LLTexLayerSet* layer_set = getLayerSet(baked_index);
		if (layer_set)
		{
			if (slam_for_debug)
			{
				layer_set->setUpdatesEnabled(TRUE);
				layer_set->cancelUpload();
			}

			BOOL set_by_user = TRUE;
			invalidateComposite(layer_set, set_by_user);
			LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TEX_REBAKES);
		}
		else
		{
			llwarns << "TAT: NO LAYER SET FOR " << (S32)baked_index << llendl;
		}
	}

	// Don't know if this is needed
	updateMeshTextures();
}


// static
void LLVOAvatar::processRebakeAvatarTextures(LLMessageSystem* msg, void**)
{
	LLUUID texture_id;
	msg->getUUID("TextureData", "TextureID", texture_id);

	LLVOAvatar* self = gAgent.getAvatarObject();
	if (!self) return;

	// If this is a texture corresponding to one of our baked entries, 
	// just rebake that layer set.
	BOOL found = FALSE;

	/* ETextureIndex baked_texture_indices[BAKED_NUM_INDICES] =
			TEX_HEAD_BAKED,
			TEX_UPPER_BAKED, */
	for (LLVOAvatarDictionary::texture_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
		 iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
		 iter++)
	{
		const ETextureIndex index = iter->first;
		const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = iter->second;
		if (text_dict->mIsBakedTexture)
		{
			if (texture_id == self->getTEImage(index)->getID())
			{
				LLTexLayerSet* layer_set = self->getLayerSet(index);
				if (layer_set)
				{
					llinfos << "TAT: rebake - matched entry " << (S32)index << llendl;
					// Apparently set_by_user == force upload
					BOOL set_by_user = TRUE;
					self->invalidateComposite(layer_set, set_by_user);
					found = TRUE;
					LLViewerStats::getInstance()->incStat(LLViewerStats::ST_TEX_REBAKES);
				}
			}
		}
	}

	// If texture not found, rebake all entries.
	if (!found)
	{
		self->forceBakeAllTextures();
	}
	else
	{
		// Not sure if this is necessary, but forceBakeAllTextures() does it.
		self->updateMeshTextures();
	}
}


BOOL LLVOAvatar::getLocalTextureRaw(ETextureIndex index, LLImageRaw* image_raw)
{
	if (!isIndexLocalTexture(index)) return FALSE;

    BOOL success = FALSE;

	if (getLocalTextureID(index) == IMG_DEFAULT_AVATAR)
	{
		success = TRUE;
	}
	else
	{
		LocalTextureData &local_tex_data = mLocalTextureData[index];
		if(local_tex_data.mImage->readBackRaw(-1, image_raw, false))
		{
			success = TRUE;
		}
		else
		{
			// No data loaded yet
			setLocalTexture( (ETextureIndex)index, getTEImage( index ), FALSE );
		}
	}
	return success;
}

BOOL LLVOAvatar::getLocalTextureGL(ETextureIndex index, LLImageGL** image_gl_pp)
{
	if (!isIndexLocalTexture(index)) return FALSE;

	BOOL success = FALSE;
	*image_gl_pp = NULL;

	if (getLocalTextureID(index) == IMG_DEFAULT_AVATAR)
	{
		success = TRUE;
	}
	else
	{
		LocalTextureData &local_tex_data = mLocalTextureData[index];
		*image_gl_pp = local_tex_data.mImage;
		success = TRUE;
	}

	if( !success )
	{
//		llinfos << "getLocalTextureGL(" << index << ") had no data" << llendl;
	}
	return success;
}

const LLUUID& LLVOAvatar::getLocalTextureID(ETextureIndex index)
{
	if (!isIndexLocalTexture(index)) return IMG_DEFAULT_AVATAR;
	
	if (mLocalTextureData[index].mImage.notNull())
	{
		return mLocalTextureData[index].mImage->getID();
	}
	else
	{
		return IMG_DEFAULT_AVATAR;
	}
}

// static
void LLVOAvatar::dumpTotalLocalTextureByteCount()
{
	S32 total_gl_bytes = 0;
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* cur = (LLVOAvatar*) *iter;
		S32 gl_bytes = 0;
		cur->getLocalTextureByteCount(&gl_bytes );
		total_gl_bytes += gl_bytes;
	}
	llinfos << "Total Avatar LocTex GL:" << (total_gl_bytes/1024) << "KB" << llendl;
}

BOOL LLVOAvatar::isVisible()
{
	return mDrawable.notNull()
		&& (mDrawable->isVisible() || mIsDummy);
}


// call periodically to keep isFullyLoaded up to date.
// returns true if the value has changed.
BOOL LLVOAvatar::updateIsFullyLoaded()
{
    // a "heuristic" to determine if we have enough avatar data to render
    // (to avoid rendering a "Ruth" - DEV-3168)

	BOOL loading = FALSE;

	// do we have a shape?
	if (visualParamWeightsAreDefault())
	{
		loading = TRUE;
	}

	// 
	if (mIsSelf)
	{
		if (!isTextureDefined(TEX_HAIR))
		{
			loading = TRUE;
		}
	}
	else if (!isTextureDefined(TEX_LOWER_BAKED) || !isTextureDefined(TEX_UPPER_BAKED) || !isTextureDefined(TEX_HEAD_BAKED))
	{
		loading = TRUE;
	}
	
	// special case to keep nudity off orientation island -
	// this is fragilely dependent on the compositing system,
	// which gets available textures in the following order:
	//
	// 1) use the baked texture
	// 2) use the layerset
	// 3) use the previously baked texture
	//
	// on orientation island case (3) can show naked skin.
	// so we test for that here:
	//
	// if we were previously unloaded, and we don't have enough
	// texture info for our shirt/pants, stay unloaded:
	if (!mPreviousFullyLoaded)
	{
		if ((!isLocalTextureDataAvailable(mBakedTextureData[BAKED_LOWER].mTexLayerSet)) &&
			(!isTextureDefined(TEX_LOWER_BAKED)))
		{
			loading = TRUE;
		}

		if ((!isLocalTextureDataAvailable(mBakedTextureData[BAKED_UPPER].mTexLayerSet)) &&
			(!isTextureDefined(TEX_UPPER_BAKED)))
		{
			loading = TRUE;
		}
	}

	
	// we wait a little bit before giving the all clear,
	// to let textures settle down
	const F32 PAUSE = 1.f;
	if (loading)
		mFullyLoadedTimer.reset();
	
	mFullyLoaded = (mFullyLoadedTimer.getElapsedTimeF32() > PAUSE);

	
	// did our loading state "change" from last call?
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


BOOL LLVOAvatar::isFullyLoaded()
{
	if (gSavedSettings.getBOOL("RenderUnloadedAvatar"))
		return TRUE;
	else
		return mFullyLoaded;
}


//-----------------------------------------------------------------------------
// findMotion()
//-----------------------------------------------------------------------------
LLMotion*		LLVOAvatar::findMotion(const LLUUID& id)
{
	return mMotionController.findMotion(id);
}

// Counts the memory footprint of local textures.
void LLVOAvatar::getLocalTextureByteCount( S32* gl_bytes )
{
	*gl_bytes = 0;
	for( S32 i = 0; i < TEX_NUM_INDICES; i++ )
	{
		if (!isIndexLocalTexture((ETextureIndex)i)) continue;
		LLViewerImage* image_gl = mLocalTextureData[(ETextureIndex)i].mImage;
		if( image_gl )
		{
			S32 bytes = (S32)image_gl->getWidth() * image_gl->getHeight() * image_gl->getComponents();

			if( image_gl->getHasGLTexture() )
			{
				*gl_bytes += bytes;
			}
		}
	}
}


BOOL LLVOAvatar::bindScratchTexture( LLGLenum format )
{
	U32 texture_bytes = 0;
	GLuint gl_name = getScratchTexName( format, &texture_bytes );
	if( gl_name )
	{
		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, gl_name);
		stop_glerror();

		F32* last_bind_time = LLVOAvatar::sScratchTexLastBindTime.getIfThere( format );
		if( last_bind_time )
		{
			if( *last_bind_time != LLImageGL::sLastFrameTime )
			{
				*last_bind_time = LLImageGL::sLastFrameTime;
				LLImageGL::updateBoundTexMem(texture_bytes);
			}
		}
		else
		{
			LLImageGL::updateBoundTexMem(texture_bytes);
			LLVOAvatar::sScratchTexLastBindTime.addData( format, new F32(LLImageGL::sLastFrameTime) );
		}

		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


LLGLuint LLVOAvatar::getScratchTexName( LLGLenum format, U32* texture_bytes )
{
	S32 components;
	GLenum internal_format;
	switch( format )
	{
	case GL_LUMINANCE:			components = 1; internal_format = GL_LUMINANCE8;		break;
	case GL_ALPHA:				components = 1; internal_format = GL_ALPHA8;			break;
	case GL_COLOR_INDEX:		components = 1; internal_format = GL_COLOR_INDEX8_EXT;	break;
	case GL_LUMINANCE_ALPHA:	components = 2; internal_format = GL_LUMINANCE8_ALPHA8;	break;
	case GL_RGB:				components = 3; internal_format = GL_RGB8;				break;
	case GL_RGBA:				components = 4; internal_format = GL_RGBA8;				break;
	default:	llassert(0);	components = 4; internal_format = GL_RGBA8;				break;
	}

	*texture_bytes = components * SCRATCH_TEX_WIDTH * SCRATCH_TEX_HEIGHT;
	
	if( LLVOAvatar::sScratchTexNames.checkData( format ) )
	{
		return *( LLVOAvatar::sScratchTexNames.getData( format ) );
	}
	else
	{

		LLGLSUIDefault gls_ui;

		U32 name = 0;
		LLImageGL::generateTextures(1, &name );
		stop_glerror();

		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, name);
		stop_glerror();

		LLImageGL::setManualImage(
			GL_TEXTURE_2D, 0, internal_format, 
			SCRATCH_TEX_WIDTH, SCRATCH_TEX_HEIGHT,
			format, GL_UNSIGNED_BYTE, NULL );
		stop_glerror();

		gGL.getTexUnit(0)->setTextureFilteringOption(LLTexUnit::TFO_BILINEAR);
		gGL.getTexUnit(0)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
		stop_glerror();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
		stop_glerror();

		LLVOAvatar::sScratchTexNames.addData( format, new LLGLuint( name ) );

		LLVOAvatar::sScratchTexBytes += *texture_bytes;
		LLImageGL::sGlobalTextureMemory += *texture_bytes;
		return name;
	}
}



//-----------------------------------------------------------------------------
// setLocalTextureTE()
//-----------------------------------------------------------------------------
void LLVOAvatar::setLocTexTE( U8 te, LLViewerImage* image, BOOL set_by_user )
{
	if( !mIsSelf )
	{
		llassert( 0 );
		return;
	}

	if( te >= TEX_NUM_INDICES )
	{
		llassert(0);
		return;
	}

	if( getTEImage( te )->getID() == image->getID() )
	{
		return;
	}

	if (isIndexBakedTexture((ETextureIndex)te))
	{
		llassert(0);
		return;
	}

	LLTexLayerSet* layer_set = getLayerSet((ETextureIndex)te);
	if (layer_set)
	{
		invalidateComposite(layer_set, set_by_user);
	}

	setTEImage( te, image );
	updateMeshTextures();

	if( gAgent.cameraCustomizeAvatar() )
	{
		LLVisualParamHint::requestHintUpdates();
	}
}

void LLVOAvatar::setupComposites()
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		bool layer_baked = isTextureDefined(mBakedTextureData[i].mTextureIndex);
		if (mBakedTextureData[i].mTexLayerSet)
		{
			mBakedTextureData[i].mTexLayerSet->setUpdatesEnabled( !layer_baked );
		}
	}
}

//-----------------------------------------------------------------------------
// updateMeshTextures()
// Uses the current TE values to set the meshes' and layersets' textures.
//-----------------------------------------------------------------------------
void LLVOAvatar::updateMeshTextures()
{
    // llinfos << "updateMeshTextures" << llendl;
	if (gNoRender) return;

	// if user has never specified a texture, assign the default
	for (U32 i=0; i < getNumTEs(); i++)
	{
		const LLViewerImage* te_image = getTEImage(i);
		if(!te_image || te_image->getID().isNull() || (te_image->getID() == IMG_DEFAULT))
		{
			setTEImage(i, gImageList.getImage(i == TEX_HAIR ? IMG_DEFAULT : IMG_DEFAULT_AVATAR)); // IMG_DEFAULT_AVATAR = a special texture that's never rendered.
		}
	}

	const BOOL self_customizing = mIsSelf && gAgent.cameraCustomizeAvatar(); // During face edit mode, we don't use baked textures
	const BOOL other_culled = !mIsSelf && mCulled;

	std::vector<bool> is_layer_baked;
	is_layer_baked.resize(mBakedTextureData.size(), false);

	std::vector<bool> use_lkg_baked_layer; // lkg = "last known good"
	use_lkg_baked_layer.resize(mBakedTextureData.size(), false);

	for (U32 i=0; i < mBakedTextureData.size(); i++)
	{
		is_layer_baked[i] = isTextureDefined(mBakedTextureData[i].mTextureIndex);

		if (!other_culled)
		{
			// When an avatar is changing clothes and not in Appearance mode,
			// use the last-known good baked texture until it finish the first
			// render of the new layerset.
			use_lkg_baked_layer[i] = (!is_layer_baked[i] 
									  && (mBakedTextureData[i].mLastTextureIndex != IMG_DEFAULT_AVATAR) 
									  && mBakedTextureData[i].mTexLayerSet 
									  && !mBakedTextureData[i].mTexLayerSet->getComposite()->isInitialized());
			if (use_lkg_baked_layer[i])
			{
				mBakedTextureData[i].mTexLayerSet->setUpdatesEnabled(TRUE);
			}
		}
		else
		{
			use_lkg_baked_layer[i] = (!is_layer_baked[i] 
									  && mBakedTextureData[i].mLastTextureIndex != IMG_DEFAULT_AVATAR);
			if (mBakedTextureData[i].mTexLayerSet)
			{
				mBakedTextureData[i].mTexLayerSet->destroyComposite();
			}
		}

	}

	// Baked textures should be requested from the sim this avatar is on. JC
	const LLHost target_host = getObjectHost();
	if (!target_host.isOk())
	{
		llwarns << "updateMeshTextures: invalid host for object: " << getID() << llendl;
	}

	for (U32 i=0; i < mBakedTextureData.size(); i++)
	{
		if (use_lkg_baked_layer[i])
		{
			LLViewerImage* baked_img = gImageList.getImageFromHost( mBakedTextureData[i].mLastTextureIndex, target_host );
			for (U32 k=0; k < mBakedTextureData[i].mMeshes.size(); k++)
			{
				mBakedTextureData[i].mMeshes[k]->setTexture( baked_img );
			}
		}
		else if (!self_customizing && is_layer_baked[i])
		{
			LLViewerImage* baked_img = getTEImage( mBakedTextureData[i].mTextureIndex );
			if( baked_img->getID() == mBakedTextureData[i].mLastTextureIndex )
			{
				// Even though the file may not be finished loading, we'll consider it loaded and use it (rather than doing compositing).
				useBakedTexture( baked_img->getID() );
			}
			else
			{
				mBakedTextureData[i].mIsLoaded = FALSE;
				if ( (i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER) )
				{
					baked_img->setLoadedCallback(onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, TRUE, TRUE, new LLTextureMaskData( mID ));	
				}
				baked_img->setLoadedCallback(onBakedTextureLoaded, SWITCH_TO_BAKED_DISCARD, FALSE, FALSE, new LLUUID( mID ) );
			}
		}
		else if (mBakedTextureData[i].mTexLayerSet 
				 && !other_culled 
				 && (i != BAKED_HAIR || is_layer_baked[i] || mIsSelf)) // ! BACKWARDS COMPATIBILITY ! workaround for old viewers.
		{
			mBakedTextureData[i].mTexLayerSet->createComposite();
			mBakedTextureData[i].mTexLayerSet->setUpdatesEnabled( TRUE );
			for (U32 k=0; k < mBakedTextureData[i].mMeshes.size(); k++)
			{
				mBakedTextureData[i].mMeshes[k]->setLayerSet( mBakedTextureData[i].mTexLayerSet );
			}
		}
	}
	
	// ! BACKWARDS COMPATIBILITY !
	// Workaround for viewing avatars from old viewers that haven't baked hair textures.
	// if (!isTextureDefined(mBakedTextureData[BAKED_HAIR].mTextureIndex))
	if (!is_layer_baked[BAKED_HAIR])
	{
		const LLColor4 color = mTexHairColor ? mTexHairColor->getColor() : LLColor4(1,1,1,1);
		LLViewerImage* hair_img = getTEImage( TEX_HAIR );
		for (U32 i = 0; i < mBakedTextureData[BAKED_HAIR].mMeshes.size(); i++)
		{
			mBakedTextureData[BAKED_HAIR].mMeshes[i]->setColor( color.mV[VX], color.mV[VY], color.mV[VZ], color.mV[VW] );
			mBakedTextureData[BAKED_HAIR].mMeshes[i]->setTexture( hair_img );
		}
		mHasBakedHair = FALSE;
	} 
	else 
	{
		for (U32 i = 0; i < mBakedTextureData[BAKED_HAIR].mMeshes.size(); i++)
		{
			mBakedTextureData[BAKED_HAIR].mMeshes[i]->setColor( 1.f, 1.f, 1.f, 1.f );
		}
		mHasBakedHair = TRUE;
	}
	
	/* // Head
	   BOOL head_baked_ready = (is_layer_baked[BAKED_HEAD] && mBakedTextureData[BAKED_HEAD].mIsLoaded) || other_culled;
	   setLocalTexture( TEX_HEAD_BODYPAINT, getTEImage( TEX_HEAD_BODYPAINT ), head_baked_ready ); */
	for (LLVOAvatarDictionary::baked_map_t::const_iterator baked_iter = LLVOAvatarDictionary::getInstance()->getBakedTextures().begin();
		 baked_iter != LLVOAvatarDictionary::getInstance()->getBakedTextures().end();
		 baked_iter++)
	{
		const EBakedTextureIndex baked_index = baked_iter->first;
		const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = baked_iter->second;
		
		for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
			 local_tex_iter != baked_dict->mLocalTextures.end();
			 local_tex_iter++)
		{
			const ETextureIndex texture_index = *local_tex_iter;
			const BOOL is_baked_ready = (is_layer_baked[baked_index] && mBakedTextureData[baked_index].mIsLoaded) || other_culled;
			setLocalTexture(texture_index, getTEImage(texture_index), is_baked_ready );
		}
	}
	removeMissingBakedTextures();
}

//-----------------------------------------------------------------------------
// setLocalTexture()
//-----------------------------------------------------------------------------
void LLVOAvatar::setLocalTexture( ETextureIndex index, LLViewerImage* tex, BOOL baked_version_ready )
{
	if (!isIndexLocalTexture(index)) return;

	S32 desired_discard = mIsSelf ? 0 : 2;
	LocalTextureData &local_tex_data = mLocalTextureData[index];
	if (!baked_version_ready)
	{
		if (tex != local_tex_data.mImage || local_tex_data.mIsBakedReady)
		{
			local_tex_data.mDiscard = MAX_DISCARD_LEVEL+1;
		}
		if (tex->getID() != IMG_DEFAULT_AVATAR)
		{
			if (local_tex_data.mDiscard > desired_discard)
			{
				S32 tex_discard = tex->getDiscardLevel();
				if (tex_discard >= 0 && tex_discard <= desired_discard)
				{
					local_tex_data.mDiscard = tex_discard;
					if( mIsSelf && !gAgent.cameraCustomizeAvatar() )
					{
						requestLayerSetUpdate( index );
					}
					else if( mIsSelf && gAgent.cameraCustomizeAvatar() )
					{
						LLVisualParamHint::requestHintUpdates();
					}
				}
				else
				{
					tex->setLoadedCallback( onLocalTextureLoaded, desired_discard, TRUE, FALSE, new LLAvatarTexData(getID(), index) );
				}
			}
			tex->setMinDiscardLevel(desired_discard);
		}
	}
	local_tex_data.mIsBakedReady = baked_version_ready;
	local_tex_data.mImage = tex;
}

//-----------------------------------------------------------------------------
// requestLayerSetUploads()
//-----------------------------------------------------------------------------
void LLVOAvatar::requestLayerSetUploads()
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		bool layer_baked = isTextureDefined(mBakedTextureData[i].mTextureIndex);
		if ( !layer_baked && mBakedTextureData[i].mTexLayerSet )
		{
			mBakedTextureData[i].mTexLayerSet->requestUpload();
		}
	}
}


//-----------------------------------------------------------------------------
// setCompositeUpdatesEnabled()
//-----------------------------------------------------------------------------
void LLVOAvatar::setCompositeUpdatesEnabled( BOOL b )
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if (mBakedTextureData[i].mTexLayerSet )
		{
			mBakedTextureData[i].mTexLayerSet->setUpdatesEnabled( b );
		}
	}
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

S32 LLVOAvatar::getLocalDiscardLevel( ETextureIndex index )
{
	if (!isIndexLocalTexture(index)) return FALSE;

	LocalTextureData &local_tex_data = mLocalTextureData[index];
	if (index >= 0
		&& getLocalTextureID(index) != IMG_DEFAULT_AVATAR
		&& !local_tex_data.mImage->isMissingAsset())
	{
		return local_tex_data.mImage->getDiscardLevel();
	}
	else
	{
		// We don't care about this (no image associated with the layer) treat as fully loaded.
		return 0;
	}
}

//-----------------------------------------------------------------------------
// isLocalTextureDataFinal()
// Returns true if the highest quality discard level exists for every texture
// in the layerset.
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::isLocalTextureDataFinal( LLTexLayerSet* layerset )
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if (layerset == mBakedTextureData[i].mTexLayerSet)
		{
			const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = LLVOAvatarDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)i);
			for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
				 local_tex_iter != baked_dict->mLocalTextures.end();
				 local_tex_iter++)
			{
				if (getLocalDiscardLevel(*local_tex_iter) != 0)
				{
					return FALSE;
				}
			}
			return TRUE;
		}
	}

	llassert(0);
	return FALSE;
}

//-----------------------------------------------------------------------------
// isLocalTextureDataAvailable()
// Returns true if at least the lowest quality discard level exists for every texture
// in the layerset.
//-----------------------------------------------------------------------------
BOOL LLVOAvatar::isLocalTextureDataAvailable( LLTexLayerSet* layerset )
{
	/* if( layerset == mBakedTextureData[BAKED_HEAD].mTexLayerSet )
	   return getLocalDiscardLevel( TEX_HEAD_BODYPAINT ) >= 0; */
	for (LLVOAvatarDictionary::baked_map_t::const_iterator baked_iter = LLVOAvatarDictionary::getInstance()->getBakedTextures().begin();
		 baked_iter != LLVOAvatarDictionary::getInstance()->getBakedTextures().end();
		 baked_iter++)
	{
		const EBakedTextureIndex baked_index = baked_iter->first;
		if (layerset == mBakedTextureData[baked_index].mTexLayerSet)
		{
			bool ret = true;
			const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = baked_iter->second;
			for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
				 local_tex_iter != baked_dict->mLocalTextures.end();
				 local_tex_iter++)
			{
				ret &= (getLocalDiscardLevel(*local_tex_iter) >= 0);
			}
			return ret;
		}
	}
	llassert(0);
	return FALSE;
}


//-----------------------------------------------------------------------------
// getBakedTE()
// Used by the LayerSet.  (Layer sets don't in general know what textures depend on them.)
//-----------------------------------------------------------------------------
ETextureIndex LLVOAvatar::getBakedTE( LLTexLayerSet* layerset )
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if (layerset == mBakedTextureData[i].mTexLayerSet )
		{
			return mBakedTextureData[i].mTextureIndex;
		}
	}

	llassert(0);
	return TEX_HEAD_BAKED;
}

//-----------------------------------------------------------------------------
// setNewBakedTexture()
// A new baked texture has been successfully uploaded and we can start using it now.
//-----------------------------------------------------------------------------
void LLVOAvatar::setNewBakedTexture( ETextureIndex te, const LLUUID& uuid )
{
	// Baked textures live on other sims.
	LLHost target_host = getObjectHost();	
	setTEImage( te, gImageList.getImageFromHost( uuid, target_host ) );
	updateMeshTextures();
	dirtyMesh();

	LLVOAvatar::cullAvatarsByPixelArea();

	/* switch(te)
		case TEX_HEAD_BAKED:
			llinfos << "New baked texture: HEAD" << llendl; */
	const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = LLVOAvatarDictionary::getInstance()->getTexture(te);
	if (text_dict->mIsBakedTexture)
	{
		llinfos << "New baked texture: " << text_dict->mName << " UUID: " << uuid <<llendl;
	}
	else
	{
		llwarns << "New baked texture: unknown te " << te << llendl;
	}
	
	//	dumpAvatarTEs( "setNewBakedTexture() send" );
	// RN: throttle uploads
	if (!hasPendingBakedUploads())
	{
		gAgent.sendAgentSetAppearance();
	}
}

bool LLVOAvatar::hasPendingBakedUploads()
{
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		bool upload_pending = (mBakedTextureData[i].mTexLayerSet && mBakedTextureData[i].mTexLayerSet->getComposite()->uploadPending());
		if (upload_pending)
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// setCachedBakedTexture()
// A baked texture id was received from a cache query, make it active
//-----------------------------------------------------------------------------
void LLVOAvatar::setCachedBakedTexture( ETextureIndex te, const LLUUID& uuid )
{
	setTETexture( te, uuid );

	/* switch(te)
		case TEX_HEAD_BAKED:
			if( mHeadLayerSet )
				mHeadLayerSet->cancelUpload(); */
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		if ( mBakedTextureData[i].mTextureIndex == te && mBakedTextureData[i].mTexLayerSet)
		{
			mBakedTextureData[i].mTexLayerSet->cancelUpload();
		}
	}
}

//-----------------------------------------------------------------------------
// releaseUnneccesaryTextures()
// release any component texture UUIDs for which we have a baked texture
//-----------------------------------------------------------------------------
void LLVOAvatar::releaseUnnecessaryTextures()
{
	// Backwards Compat: detect if the baked hair texture actually wasn't sent, and if so set to default
	if (isTextureDefined(TEX_HAIR_BAKED) && getTEImage(TEX_HAIR_BAKED)->getID() == getTEImage(TEX_SKIRT_BAKED)->getID())
	{
		if (getTEImage(TEX_HAIR_BAKED)->getID() != IMG_INVISIBLE)
		{
			// Regression case of messaging system. Expected 21 textures, received 20. last texture is not valid so set to default
			setTETexture(TEX_HAIR_BAKED, IMG_DEFAULT_AVATAR);
		}
	}

	for (U8 baked_index = 0; baked_index < BAKED_NUM_INDICES; baked_index++)
	{
		const LLVOAvatarDictionary::BakedDictionaryEntry * bakedDicEntry = LLVOAvatarDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)baked_index);
		// skip if this is a skirt and av is not wearing one, or if we don't have a baked texture UUID
		if (!isTextureDefined(bakedDicEntry->mTextureIndex)
			&& ( (baked_index != BAKED_SKIRT) || isWearingWearableType(WT_SKIRT) ))
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

//-----------------------------------------------------------------------------
// static
// onCustomizeStart()
//-----------------------------------------------------------------------------
void LLVOAvatar::onCustomizeStart()
{
	// We're no longer doing any baking or invalidating on entering 
	// appearance editing mode. Leaving function in place in case 
	// further changes require us to do something at this point - Nyx
}

//-----------------------------------------------------------------------------
// static
// onCustomizeEnd()
//-----------------------------------------------------------------------------
void LLVOAvatar::onCustomizeEnd()
{
	LLVOAvatar *avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		avatarp->invalidateAll();
		avatarp->requestLayerSetUploads();
	}
}

void LLVOAvatar::onChangeSelfInvisible(BOOL newvalue)
{
	LLVOAvatar *avatarp = gAgent.getAvatarObject();
	if (avatarp)
	{
		if (newvalue)
		{
			// we have just requested to set the avatar's baked textures to invisible
			avatarp->setInvisible(TRUE);
		}
		else
		{
			avatarp->setInvisible(FALSE);
		}
	}
}


BOOL LLVOAvatar::teToColorParams( ETextureIndex te, const char* param_name[3] )
{
	switch( te )
	{
	case TEX_UPPER_SHIRT:
		param_name[0] = "shirt_red";
		param_name[1] = "shirt_green";
		param_name[2] = "shirt_blue";
		break;

	case TEX_LOWER_PANTS:
		param_name[0] = "pants_red";
		param_name[1] = "pants_green";
		param_name[2] = "pants_blue";
		break;

	case TEX_LOWER_SHOES:
		param_name[0] = "shoes_red";
		param_name[1] = "shoes_green";
		param_name[2] = "shoes_blue";
		break;

	case TEX_LOWER_SOCKS:
		param_name[0] = "socks_red";
		param_name[1] = "socks_green";
		param_name[2] = "socks_blue";
		break;

	case TEX_UPPER_JACKET:
	case TEX_LOWER_JACKET:
		param_name[0] = "jacket_red";
		param_name[1] = "jacket_green";
		param_name[2] = "jacket_blue";
		break;

	case TEX_UPPER_GLOVES:
		param_name[0] = "gloves_red";
		param_name[1] = "gloves_green";
		param_name[2] = "gloves_blue";
		break;

	case TEX_UPPER_UNDERSHIRT:
		param_name[0] = "undershirt_red";
		param_name[1] = "undershirt_green";
		param_name[2] = "undershirt_blue";
		break;
	
	case TEX_LOWER_UNDERPANTS:
		param_name[0] = "underpants_red";
		param_name[1] = "underpants_green";
		param_name[2] = "underpants_blue";
		break;

	case TEX_SKIRT:
		param_name[0] = "skirt_red";
		param_name[1] = "skirt_green";
		param_name[2] = "skirt_blue";
		break;

	default:
		llassert(0);
		return FALSE;
	}

	return TRUE;
}

void LLVOAvatar::setClothesColor( ETextureIndex te, const LLColor4& new_color, BOOL set_by_user )
{
	const char* param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		setVisualParamWeight( param_name[0], new_color.mV[VX], set_by_user );
		setVisualParamWeight( param_name[1], new_color.mV[VY], set_by_user );
		setVisualParamWeight( param_name[2], new_color.mV[VZ], set_by_user );
	}
}

LLColor4 LLVOAvatar::getClothesColor( ETextureIndex te )
{
	LLColor4 color;
	const char* param_name[3];
	if( teToColorParams( te, param_name ) )
	{
		color.mV[VX] = getVisualParamWeight( param_name[0] );
		color.mV[VY] = getVisualParamWeight( param_name[1] );
		color.mV[VZ] = getVisualParamWeight( param_name[2] );
	}
	return color;
}




void LLVOAvatar::dumpAvatarTEs( const std::string& context )
{	
	/* const char* te_name[] = {
			"TEX_HEAD_BODYPAINT   ",
			"TEX_UPPER_SHIRT      ", */
	llinfos << (mIsSelf ? "Self: " : "Other: ") << context << llendl;
	for (LLVOAvatarDictionary::texture_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
		 iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
		 iter++)
	{
		const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = iter->second;
		const LLViewerImage* te_image = getTEImage(iter->first);
		if( !te_image )
		{
			llinfos << "       " << text_dict->mName << ": null ptr" << llendl;
		}
		else if( te_image->getID().isNull() )
		{
			llinfos << "       " << text_dict->mName << ": null UUID" << llendl;
		}
		else if( te_image->getID() == IMG_DEFAULT )
		{
			llinfos << "       " << text_dict->mName << ": IMG_DEFAULT" << llendl;
		}
		else if( te_image->getID() == IMG_DEFAULT_AVATAR )
		{
			llinfos << "       " << text_dict->mName << ": IMG_DEFAULT_AVATAR" << llendl;
		}
		else
		{
			llinfos << "       " << text_dict->mName << ": " << te_image->getID() << llendl;
		}
	}
}

//-----------------------------------------------------------------------------
// updateAttachmentVisibility()
//-----------------------------------------------------------------------------
void LLVOAvatar::updateAttachmentVisibility(U32 camera_mode)
{
	for (attachment_map_t::iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getIsHUDAttachment())
		{
			attachment->setAttachmentVisibility(TRUE);
		}
		else
		{
			switch (camera_mode)
			{
			case CAMERA_MODE_MOUSELOOK:
				if (LLVOAvatar::sVisibleInFirstPerson && attachment->getVisibleInFirstPerson())
				{
					attachment->setAttachmentVisibility(TRUE);
				}
				else
				{
					attachment->setAttachmentVisibility(FALSE);
				}
				break;
			default:
				attachment->setAttachmentVisibility(TRUE);
				break;
			}
		}
	}
}

// Given a texture entry, determine which wearable type owns it.
// static
LLUUID LLVOAvatar::getDefaultTEImageID(ETextureIndex index )
{
	/* switch( index )
		case TEX_UPPER_SHIRT:		return LLUUID( gSavedSettings.getString("UIImgDefaultShirtUUID") ); */
	const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = LLVOAvatarDictionary::getInstance()->getTexture(index);
	const std::string &default_image_name = text_dict->mDefaultImageName;
	if (default_image_name == "")
	{
		return IMG_DEFAULT_AVATAR;
	}
	else
	{
		return LLUUID(gSavedSettings.getString(default_image_name));
	}
}


void LLVOAvatar::setInvisible(BOOL newvalue)
{
	if (newvalue)
	{
		setCompositeUpdatesEnabled(FALSE);
		for (U32 i = 0; i < mBakedTextureData.size(); i++ )
		{
			setNewBakedTexture(mBakedTextureData[i].mTextureIndex, IMG_INVISIBLE);
		}
		gAgent.sendAgentSetAppearance();
	}
	else
	{
		setCompositeUpdatesEnabled(TRUE);
		invalidateAll();
		requestLayerSetUploads();
		gAgent.sendAgentSetAppearance();
	}
}

// Given a texture entry, determine which wearable type owns it.
// static
EWearableType LLVOAvatar::getTEWearableType(ETextureIndex index )
{
	/* switch(index)
		case TEX_UPPER_SHIRT:
			return WT_SHIRT; */
	return LLVOAvatarDictionary::getInstance()->getTexture(index)->mWearableType;
}

// Unlike most wearable functions, this works for both self and other.
BOOL LLVOAvatar::isWearingWearableType( EWearableType type )
{
	if (mIsDummy) return TRUE;

	switch( type )
	{
		case WT_SHAPE:
		case WT_SKIN:
		case WT_HAIR:
		case WT_EYES:
			return TRUE;  // everyone has all bodyparts
		default:
			break; // Do nothing
	}

	/* switch(type)
		case WT_SHIRT:
			indicator_te = TEX_UPPER_SHIRT; */
	for (LLVOAvatarDictionary::texture_map_t::const_iterator tex_iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
		 tex_iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
		 tex_iter++)
	{
		const LLVOAvatarDefines::ETextureIndex index = tex_iter->first;
		const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = tex_iter->second;
		if (text_dict->mWearableType == type)
		{
			// If you're checking your own clothing, check the component texture
			if (mIsSelf)
			{
				if (isTextureDefined(index))
				{
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}

			// If you're checking another avatar's clothing, you don't have component textures.
			// Thus, you must check to see if the corresponding baked texture is defined.
			// NOTE: this is a poor substitute if you actually want to know about individual pieces of clothing
			// this works for detecting a skirt (most important), but is ineffective at any piece of clothing that
			// gets baked into a texture that always exists (upper or lower).
			const std::string name = text_dict->mName;
			for (LLVOAvatarDictionary::baked_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getBakedTextures().begin();
				iter != LLVOAvatarDictionary::getInstance()->getBakedTextures().end();
				iter++)
			{
				const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = iter->second;
				if (baked_dict->mName == name)
				{
					if (isTextureDefined(baked_dict->mTextureIndex))
					{
						return TRUE;
					}
					else
					{
						return FALSE;
					}
				}
			}
			return FALSE;
		}
	}
	return FALSE;
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
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment)
		{
			attachment->clampObjectPosition();
		}
	}
}

BOOL LLVOAvatar::hasHUDAttachment() const
{
	for (attachment_map_t::const_iterator iter = mAttachmentPoints.begin(); 
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::const_iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getIsHUDAttachment() && attachment->getObject())
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
		 iter != mAttachmentPoints.end(); )
	{
		attachment_map_t::const_iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getIsHUDAttachment() && attachment->getObject())
		{
			LLViewerObject* hud_object = attachment->getObject();

			// initialize bounding box to contain identity orientation and center point for attached object
			bbox.addPointLocal(hud_object->getPosition());
			// add rotated bounding box for attached object
			bbox.addBBoxAgent(hud_object->getBoundingBoxAgent());
			LLViewerObject::const_child_list_t& child_list = hud_object->getChildren();
			for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
				 iter != child_list.end(); iter++)
			{
				LLViewerObject* child_objectp = *iter;
				bbox.addBBoxAgent(child_objectp->getBoundingBoxAgent());
			}
		}
	}

	return bbox;
}

void LLVOAvatar::rebuildHUD()
{
}

//-----------------------------------------------------------------------------
// onFirstTEMessageReceived()
//-----------------------------------------------------------------------------
void LLVOAvatar::onFirstTEMessageReceived()
{
	if( !mFirstTEMessageReceived )
	{
		mFirstTEMessageReceived = TRUE;

		for (U32 i = 0; i < mBakedTextureData.size(); i++)
		{
			bool layer_baked = isTextureDefined(mBakedTextureData[i].mTextureIndex);

			// Use any baked textures that we have even if they haven't downloaded yet.
			// (That is, don't do a transition from unbaked to baked.)
			if (layer_baked)
			{
				LLViewerImage* image = getTEImage( mBakedTextureData[i].mTextureIndex );
				mBakedTextureData[i].mLastTextureIndex = image->getID();
				// If we have more than one texture for the other baked layers, we'll want to call this for them too.
				if ( (i == BAKED_HEAD) || (i == BAKED_UPPER) || (i == BAKED_LOWER) )
				{
					image->setLoadedCallback( onBakedTextureMasksLoaded, MORPH_MASK_REQUESTED_DISCARD, TRUE, TRUE, new LLTextureMaskData( mID ));
				}
				image->setLoadedCallback( onInitialBakedTextureLoaded, MAX_DISCARD_LEVEL, FALSE, FALSE, new LLUUID( mID ) );
			}
		}

		updateMeshTextures();
	}
}

//-----------------------------------------------------------------------------
// processAvatarAppearance()
//-----------------------------------------------------------------------------
void LLVOAvatar::processAvatarAppearance( LLMessageSystem* mesgsys )
{
	if (gSavedSettings.getBOOL("BlockAvatarAppearanceMessages"))
	{
		llwarns << "Blocking AvatarAppearance message" << llendl;
		return;
	}
	
	LLMemType mt(LLMemType::MTYPE_AVATAR);
	
//	llinfos << "processAvatarAppearance start " << mID << llendl;
	BOOL is_first_appearance_message = !mFirstAppearanceMessageReceived;

	mFirstAppearanceMessageReceived = TRUE;

	if( mIsSelf )
	{
		llwarns << "Received AvatarAppearance for self" << llendl;
		if( mFirstTEMessageReceived )
		{
//			llinfos << "processAvatarAppearance end  " << mID << llendl;
			return;
		}
	}

	if (gNoRender)
	{
		return;
	}

	ESex old_sex = getSex();

//	llinfos << "ady LLVOAvatar::processAvatarAppearance()" << llendl;
//	dumpAvatarTEs( "PRE  processAvatarAppearance()" );
	unpackTEMessage(mesgsys, _PREHASH_ObjectData);
//	dumpAvatarTEs( "POST processAvatarAppearance()" );

	//llinfos << "Received AvatarAppearance: " << (mIsSelf ? "(self): " : "(other): ")  << std::endl <<
	//	(isTextureDefined(TEX_HEAD_BAKED)  ? "HEAD " : "head " ) << (getTEImage(TEX_HEAD_BAKED)->getID()) << std::endl <<
	//	(isTextureDefined(TEX_UPPER_BAKED) ? "UPPER " : "upper " ) << (getTEImage(TEX_UPPER_BAKED)->getID()) << std::endl <<
	//	(isTextureDefined(TEX_LOWER_BAKED) ? "LOWER " : "lower " ) << (getTEImage(TEX_LOWER_BAKED)->getID()) << std::endl <<
	//	(isTextureDefined(TEX_SKIRT_BAKED) ? "SKIRT " : "skirt " ) << (getTEImage(TEX_SKIRT_BAKED)->getID()) << std::endl <<
	//	(isTextureDefined(TEX_HAIR_BAKED) ? "HAIR" : "hair " ) << (getTEImage(TEX_HAIR_BAKED)->getID()) << std::endl <<
	//	(isTextureDefined(TEX_EYES_BAKED)  ? "EYES" : "eyes" ) << (getTEImage(TEX_EYES_BAKED)->getID()) << llendl ;
 
	if( !mFirstTEMessageReceived )
	{
		onFirstTEMessageReceived();
	}

	setCompositeUpdatesEnabled( FALSE );

	if (!mIsSelf)
	{
		releaseUnnecessaryTextures();
	}
	
	updateMeshTextures(); // enables updates for laysets without baked textures.

	// parse visual params
	S32 num_blocks = mesgsys->getNumberOfBlocksFast(_PREHASH_VisualParam);
	if( num_blocks > 1 )
	{
		BOOL params_changed = FALSE;
		BOOL interp_params = FALSE;
		
		LLVisualParam* param = getFirstVisualParam();
		if (!param)
		{
			llwarns << "No visual params!" << llendl;
		}
		else
		{
			for( S32 i = 0; i < num_blocks; i++ )
			{
				while( param && (param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) )
				{
					param = getNextVisualParam();
				}
						
				if( !param )
				{
					llwarns << "Number of params in AvatarAppearance msg does not match number of params in avatar xml file." << llendl;
					return;
				}

				U8 value;
				mesgsys->getU8Fast(_PREHASH_VisualParam, _PREHASH_ParamValue, value, i);
				F32 newWeight = U8_to_F32(value, param->getMinWeight(), param->getMaxWeight());

				if (is_first_appearance_message || (param->getWeight() != newWeight))
				{
					//llinfos << "Received update for param " << param->getDisplayName() << " at value " << newWeight << llendl;
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
				param = getNextVisualParam();
			}
		}

		while( param && (param->getGroup() != VISUAL_PARAM_GROUP_TWEAKABLE) )
		{
			param = getNextVisualParam();
		}
		if( param )
		{
			llwarns << "Number of params in AvatarAppearance msg does not match number of params in avatar xml file." << llendl;
			return;
		}

		if (params_changed)
		{
			if (interp_params)
			{
				startAppearanceAnimation(FALSE, FALSE);
			}
			updateVisualParams();

			ESex new_sex = getSex();
			if( old_sex != new_sex )
			{
				updateSexDependentLayerSets( FALSE );
			}	
		}
	}
	else
	{
		llwarns << "AvatarAppearance msg received without any parameters, object: " << getID() << llendl;
	}

	setCompositeUpdatesEnabled( TRUE );

	llassert( getSex() == ((getVisualParamWeight( "male" ) > 0.5f) ? SEX_MALE : SEX_FEMALE) );

	// If all of the avatars are completely baked, release the global image caches to conserve memory.
	LLVOAvatar::cullAvatarsByPixelArea();

//	llinfos << "processAvatarAppearance end " << mID << llendl;
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

void LLVOAvatar::onBakedTextureMasksLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	if (!userdata) return;

	//llinfos << "onBakedTextureMasksLoaded: " << src_vi->getID() << llendl;
	const LLMemType mt(LLMemType::MTYPE_AVATAR);
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
				llerrs << "No auxiliary source data for onBakedTextureMasksLoaded" << llendl;
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
			     if (self->mBakedTextureData[BAKED_HEAD].mTexLayerSet)
				     //llinfos << "onBakedTextureMasksLoaded for head " << id << " discard = " << discard_level << llendl;
					 self->mBakedTextureData[BAKED_HEAD].mTexLayerSet->applyMorphMask(aux_src->getData(), aux_src->getWidth(), aux_src->getHeight(), 1);
					 maskData->mLastDiscardLevel = discard_level; */
			bool found_texture_id = false;
			for (LLVOAvatarDictionary::texture_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
				 iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
				 iter++)
			{

				const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = iter->second;
				if (text_dict->mIsUsedByBakedTexture)
				{
					const ETextureIndex texture_index = iter->first;
					const LLViewerImage *baked_img = self->getTEImage(texture_index);
					if (id == baked_img->getID())
					{
						const EBakedTextureIndex baked_index = text_dict->mBakedTextureIndex;
						if (self->mBakedTextureData[baked_index].mTexLayerSet)
						{
							//llinfos << "onBakedTextureMasksLoaded for " << text_dict->mName << " " << id << " discard = " << discard_level << llendl;
							self->mBakedTextureData[baked_index].mTexLayerSet->applyMorphMask(aux_src->getData(), aux_src->getWidth(), aux_src->getHeight(), 1);
							maskData->mLastDiscardLevel = discard_level;
							if (self->mBakedTextureData[baked_index].mMaskTexName)
							{
								LLImageGL::deleteTextures(1, &(self->mBakedTextureData[baked_index].mMaskTexName));
							}
							self->mBakedTextureData[baked_index].mMaskTexName = gl_name;
						}
						else
						{
							llwarns << "onBakedTextureMasksLoaded: no LayerSet for " << text_dict->mName << "." << llendl;
						}
						found_texture_id = true;
						break;
					}
				}
			}
			if (!found_texture_id)
			{
				llinfos << "onBakedTextureMasksLoaded(): unexpected image id: " << id << llendl;
			}
			self->dirtyMesh();
		}
		else
		{
            // this can happen when someone uses an old baked texture possibly provided by 
            // viewer-side baked texture caching
			llwarns << "Masks loaded callback but NO aux source!" << llendl;
		}
	}

	if (final || !success)
	{
		delete maskData;
	}
}

// static
void LLVOAvatar::onInitialBakedTextureLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata )
{
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = (LLVOAvatar *)gObjectList.findObject(*avatar_idp);

	if (!success && selfp)
	{
		selfp->removeMissingBakedTextures();
	}
	if (final || !success )
	{
		delete avatar_idp;
	}
}

void LLVOAvatar::onBakedTextureLoaded(BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata)
{
	//llinfos << "onBakedTextureLoaded: " << src_vi->getID() << llendl;

	LLUUID id = src_vi->getID();
	LLUUID *avatar_idp = (LLUUID *)userdata;
	LLVOAvatar *selfp = (LLVOAvatar *)gObjectList.findObject(*avatar_idp);

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
	/* if(id == head_baked->getID())
		 mHeadBakedLoaded = TRUE;
		 mLastHeadBakedID = id;
		 mHeadMesh0.setTexture( head_baked );
		 mHeadMesh1.setTexture( head_baked ); */
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		LLViewerImage* image_baked = getTEImage( mBakedTextureData[i].mTextureIndex );
		if (id == image_baked->getID())
		{
			mBakedTextureData[i].mIsLoaded = true;
			mBakedTextureData[i].mLastTextureIndex = id;
			for (U32 k = 0; k < mBakedTextureData[i].mMeshes.size(); k++)
			{
				mBakedTextureData[i].mMeshes[k]->setTexture( image_baked );
			}
			if (mBakedTextureData[i].mTexLayerSet)
			{
				mBakedTextureData[i].mTexLayerSet->destroyComposite();
			}
			const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = LLVOAvatarDictionary::getInstance()->getBakedTexture((EBakedTextureIndex)i);
			for (texture_vec_t::const_iterator local_tex_iter = baked_dict->mLocalTextures.begin();
				 local_tex_iter != baked_dict->mLocalTextures.end();
				 local_tex_iter++)
			{
				setLocalTexture(*local_tex_iter, getTEImage(*local_tex_iter), TRUE);
			}
		}
	}

	dirtyMesh();
}

// static
void LLVOAvatar::dumpArchetypeXML( void* )
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	LLAPRFile outfile ;
	outfile.open(gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,"new archetype.xml"), LL_APR_WB );
	apr_file_t* file = outfile.getFileHandle() ;
	if( !file )
	{
		return;
	}

	apr_file_printf( file, "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n" );
	apr_file_printf( file, "<linden_genepool version=\"1.0\">\n" );
	apr_file_printf( file, "\n\t<archetype name=\"???\">\n" );

	// only body parts, not clothing.
	for( S32 type = WT_SHAPE; type <= WT_EYES; type++ )
	{
		const std::string& wearable_name = LLWearable::typeToTypeName( (EWearableType) type );
		apr_file_printf( file, "\n\t\t<!-- wearable: %s -->\n", wearable_name.c_str() );

		for( LLVisualParam* param = avatar->getFirstVisualParam(); param; param = avatar->getNextVisualParam() )
		{
			LLViewerVisualParam* viewer_param = (LLViewerVisualParam*)param;
			if( (viewer_param->getWearableType() == type) && 
				(viewer_param->getGroup() == VISUAL_PARAM_GROUP_TWEAKABLE) )
			{
				apr_file_printf( file, "\t\t<param id=\"%d\" name=\"%s\" value=\"%.3f\"/>\n",
						 viewer_param->getID(), viewer_param->getName().c_str(), viewer_param->getWeight() );
			}
		}

		for(U8 te = 0; te < TEX_NUM_INDICES; te++)
		{
			if( LLVOAvatar::getTEWearableType((ETextureIndex)te) == type )
			{
				LLViewerImage* te_image = avatar->getTEImage((ETextureIndex)te);
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
}


U32 LLVOAvatar::getVisibilityRank()
{
	return mVisibilityRank;
}

void LLVOAvatar::setVisibilityRank(U32 rank)
{
	if (mDrawable.isNull() || mDrawable->isDead())
	{ //do nothing
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
		if( inst == this )
		{
			return rank;
		}
		else
		if( !inst->isDead() && !inst->isFullyBaked() )
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
	U32 rank = 0;
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
			inst->setVisibilityRank(0);
		}
		else if (inst->mDrawable.notNull() && inst->mDrawable->isVisible())
		{
			inst->setVisibilityRank(rank++);
		}
	}

	S32 grey_avatars = 0;
	if ( LLVOAvatar::areAllNearbyInstancesBaked(grey_avatars) )
	{
		LLVOAvatar::deleteCachedImages(false);
	}
	else
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

const LLUUID& LLVOAvatar::grabLocalTexture(ETextureIndex index)
{
	if (canGrabLocalTexture(index))
	{
		return getTEImage( index )->getID();
	}
	return LLUUID::null;
}

BOOL LLVOAvatar::canGrabLocalTexture(ETextureIndex index)
{
	// Check if the texture hasn't been baked yet.
	if (!isTextureDefined(index))
	{
		lldebugs << "getTEImage( " << (U32) index << " )->getID() == IMG_DEFAULT_AVATAR" << llendl;
		return FALSE;
	}

	if (gAgent.isGodlike())
		return TRUE;

	// Check permissions of textures that show up in the
	// baked texture.  We don't want people copying people's
	// work via baked textures.
	/* switch(index)
		case TEX_EYES_BAKED:
			textures.push_back(TEX_EYES_IRIS); */
	const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = LLVOAvatarDictionary::getInstance()->getTexture(index);
	if (!text_dict->mIsUsedByBakedTexture) return FALSE;

	const EBakedTextureIndex baked_index = text_dict->mBakedTextureIndex;
	const LLVOAvatarDictionary::BakedDictionaryEntry *baked_dict = LLVOAvatarDictionary::getInstance()->getBakedTexture(baked_index);
	for (texture_vec_t::const_iterator iter = baked_dict->mLocalTextures.begin();
		 iter != baked_dict->mLocalTextures.end();
		 iter++)
	{
		const ETextureIndex t_index = (*iter);
		lldebugs << "Checking index " << (U32) t_index << llendl;
		const LLUUID& texture_id = getTEImage( t_index )->getID();
		if (texture_id != IMG_DEFAULT_AVATAR)
		{
			// Search inventory for this texture.
			LLViewerInventoryCategory::cat_array_t cats;
			LLViewerInventoryItem::item_array_t items;
			LLAssetIDMatches asset_id_matches(texture_id);
			gInventory.collectDescendentsIf(LLUUID::null,
									cats,
									items,
									LLInventoryModel::INCLUDE_TRASH,
									asset_id_matches);

			BOOL can_grab = FALSE;
			lldebugs << "item count for asset " << texture_id << ": " << items.count() << llendl;
			if (items.count())
			{
				// search for full permissions version
				for (S32 i = 0; i < items.count(); i++)
				{
					LLInventoryItem* itemp = items[i];
					LLPermissions item_permissions = itemp->getPermissions();
					if ( item_permissions.allowOperationBy(
								PERM_MODIFY, gAgent.getID(), gAgent.getGroupID()) &&
						 item_permissions.allowOperationBy(
								PERM_COPY, gAgent.getID(), gAgent.getGroupID()) &&
						 item_permissions.allowOperationBy(
								PERM_TRANSFER, gAgent.getID(), gAgent.getGroupID()) )
					{
						can_grab = TRUE;
						break;
					}
				}
			}
			if (!can_grab) return FALSE;
		}
	}

	return TRUE;
}

void LLVOAvatar::dumpLocalTextures()
{
	llinfos << "Local Textures:" << llendl;

	/* ETextureIndex baked_equiv[] = {
		TEX_UPPER_BAKED,
	   if (isTextureDefined(baked_equiv[i])) */
	for (LLVOAvatarDictionary::texture_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
		 iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
		 iter++)
	{
		const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = iter->second;
		if (!text_dict->mIsLocalTexture || !text_dict->mIsUsedByBakedTexture)
			continue;

		const EBakedTextureIndex baked_index = text_dict->mBakedTextureIndex;
		const ETextureIndex baked_equiv = LLVOAvatarDictionary::getInstance()->getBakedTexture(baked_index)->mTextureIndex;

		const std::string &name = text_dict->mName;
		const LocalTextureData &local_tex_data = mLocalTextureData[iter->first];
		if (isTextureDefined(baked_equiv))
		{
#if LL_RELEASE_FOR_DOWNLOAD
			// End users don't get to trivially see avatar texture IDs, makes textures
			// easier to steal. JC
			llinfos << "LocTex " << name << ": Baked " << llendl;
#else
			llinfos << "LocTex " << name << ": Baked " << getTEImage( baked_equiv )->getID() << llendl;
#endif
		}
		else if (local_tex_data.mImage.notNull())
		{
			if( local_tex_data.mImage->getID() == IMG_DEFAULT_AVATAR )
			{
				llinfos << "LocTex " << name << ": None" << llendl;
			}
			else
			{
				const LLViewerImage* image = local_tex_data.mImage;

				llinfos << "LocTex " << name << ": "
						<< "Discard " << image->getDiscardLevel() << ", "
						<< "(" << image->getWidth() << ", " << image->getHeight() << ") " 
#if !LL_RELEASE_FOR_DOWNLOAD
					// End users don't get to trivially see avatar texture IDs,
					// makes textures easier to steal
						<< image->getID() << " "
#endif
						<< "Priority: " << image->getDecodePriority()
						<< llendl;
			}
		}
		else
		{
			llinfos << "LocTex " << name << ": No LLViewerImage" << llendl;
		}
	}
}

void LLVOAvatar::startAppearanceAnimation(BOOL set_by_user, BOOL play_sound)
{
	if(!mAppearanceAnimating)
	{
		mAppearanceAnimSetByUser = set_by_user;
		mAppearanceAnimating = TRUE;
		mAppearanceMorphTimer.reset();
		mLastAppearanceBlendTime = 0.f;
	}
}


void LLVOAvatar::removeMissingBakedTextures()
{	
	if (!mIsSelf) return;

	BOOL removed = FALSE;
	for (U32 i = 0; i < mBakedTextureData.size(); i++)
	{
		const S32 te = mBakedTextureData[i].mTextureIndex;
		if (getTEImage(te)->isMissingAsset())
		{
			setTEImage(te, gImageList.getImage(IMG_DEFAULT_AVATAR));
			removed = TRUE;
		}
	}

	if (removed)
	{
		for(U32 i = 0; i < mBakedTextureData.size(); i++)
		{
			invalidateComposite(mBakedTextureData[i].mTexLayerSet, FALSE);
		}
		updateMeshTextures();
		requestLayerSetUploads();
	}
}


//-----------------------------------------------------------------------------
// LLVOAvatarXmlInfo
//-----------------------------------------------------------------------------

LLVOAvatarXmlInfo::LLVOAvatarXmlInfo()
	: mTexSkinColorInfo(0), mTexHairColorInfo(0), mTexEyeColorInfo(0)
{
}

LLVOAvatarXmlInfo::~LLVOAvatarXmlInfo()
{
	std::for_each(mMeshInfoList.begin(), mMeshInfoList.end(), DeletePointer());
	std::for_each(mSkeletalDistortionInfoList.begin(), mSkeletalDistortionInfoList.end(), DeletePointer());		
	std::for_each(mAttachmentInfoList.begin(), mAttachmentInfoList.end(), DeletePointer());
	delete mTexSkinColorInfo;
	delete mTexHairColorInfo;
	delete mTexEyeColorInfo;
	std::for_each(mLayerInfoList.begin(), mLayerInfoList.end(), DeletePointer());		
	std::for_each(mDriverInfoList.begin(), mDriverInfoList.end(), DeletePointer());		
}

//-----------------------------------------------------------------------------
// LLVOAvatarBoneInfo::parseXml()
//-----------------------------------------------------------------------------
BOOL LLVOAvatarBoneInfo::parseXml(LLXmlTreeNode* node)
{
	if (node->hasName("bone"))
	{
		mIsJoint = TRUE;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!node->getFastAttributeString(name_string, mName))
		{
			llwarns << "Bone without name" << llendl;
			return FALSE;
		}
	}
	else if (node->hasName("collision_volume"))
	{
		mIsJoint = FALSE;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!node->getFastAttributeString(name_string, mName))
		{
			mName = "Collision Volume";
		}
	}
	else
	{
		llwarns << "Invalid node " << node->getName() << llendl;
		return FALSE;
	}

	static LLStdStringHandle pos_string = LLXmlTree::addAttributeString("pos");
	if (!node->getFastAttributeVector3(pos_string, mPos))
	{
		llwarns << "Bone without position" << llendl;
		return FALSE;
	}

	static LLStdStringHandle rot_string = LLXmlTree::addAttributeString("rot");
	if (!node->getFastAttributeVector3(rot_string, mRot))
	{
		llwarns << "Bone without rotation" << llendl;
		return FALSE;
	}
	
	static LLStdStringHandle scale_string = LLXmlTree::addAttributeString("scale");
	if (!node->getFastAttributeVector3(scale_string, mScale))
	{
		llwarns << "Bone without scale" << llendl;
		return FALSE;
	}

	if (mIsJoint)
	{
		static LLStdStringHandle pivot_string = LLXmlTree::addAttributeString("pivot");
		if (!node->getFastAttributeVector3(pivot_string, mPivot))
		{
			llwarns << "Bone without pivot" << llendl;
			return FALSE;
		}
	}

	// parse children
	LLXmlTreeNode* child;
	for( child = node->getFirstChild(); child; child = node->getNextChild() )
	{
		LLVOAvatarBoneInfo *child_info = new LLVOAvatarBoneInfo;
		if (!child_info->parseXml(child))
		{
			delete child_info;
			return FALSE;
		}
		mChildList.push_back(child_info);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLVOAvatarSkeletonInfo::parseXml()
//-----------------------------------------------------------------------------
BOOL LLVOAvatarSkeletonInfo::parseXml(LLXmlTreeNode* node)
{
	static LLStdStringHandle num_bones_string = LLXmlTree::addAttributeString("num_bones");
	if (!node->getFastAttributeS32(num_bones_string, mNumBones))
	{
		llwarns << "Couldn't find number of bones." << llendl;
		return FALSE;
	}

	static LLStdStringHandle num_collision_volumes_string = LLXmlTree::addAttributeString("num_collision_volumes");
	node->getFastAttributeS32(num_collision_volumes_string, mNumCollisionVolumes);

	LLXmlTreeNode* child;
	for( child = node->getFirstChild(); child; child = node->getNextChild() )
	{
		LLVOAvatarBoneInfo *info = new LLVOAvatarBoneInfo;
		if (!info->parseXml(child))
		{
			delete info;
			llwarns << "Error parsing bone in skeleton file" << llendl;
			return FALSE;
		}
		mBoneInfoList.push_back(info);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlSkeletonNode(): parses <skeleton> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatarXmlInfo::parseXmlSkeletonNode(LLXmlTreeNode* root)
{
	LLXmlTreeNode* node = root->getChildByName( "skeleton" );
	if( !node )
	{
		llwarns << "avatar file: missing <skeleton>" << llendl;
		return FALSE;
	}

	LLXmlTreeNode* child;

	// SKELETON DISTORTIONS
	for (child = node->getChildByName( "param" );
		 child;
		 child = node->getNextNamedChild())
	{
		if (!child->getChildByName("param_skeleton"))
		{
			if (child->getChildByName("param_morph"))
			{
				llwarns << "Can't specify morph param in skeleton definition." << llendl;
			}
			else
			{
				llwarns << "Unknown param type." << llendl;
			}
			continue;
		}
		
		LLPolySkeletalDistortionInfo *info = new LLPolySkeletalDistortionInfo;
		if (!info->parseXml(child))
		{
			delete info;
			return FALSE;
		}

		mSkeletalDistortionInfoList.push_back(info);
	}

	// ATTACHMENT POINTS
	for (child = node->getChildByName( "attachment_point" );
		 child;
		 child = node->getNextNamedChild())
	{
		LLVOAvatarAttachmentInfo* info = new LLVOAvatarAttachmentInfo();

		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (!child->getFastAttributeString(name_string, info->mName))
		{
			llwarns << "No name supplied for attachment point." << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle joint_string = LLXmlTree::addAttributeString("joint");
		if (!child->getFastAttributeString(joint_string, info->mJointName))
		{
			llwarns << "No bone declared in attachment point " << info->mName << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle position_string = LLXmlTree::addAttributeString("position");
		if (child->getFastAttributeVector3(position_string, info->mPosition))
		{
			info->mHasPosition = TRUE;
		}

		static LLStdStringHandle rotation_string = LLXmlTree::addAttributeString("rotation");
		if (child->getFastAttributeVector3(rotation_string, info->mRotationEuler))
		{
			info->mHasRotation = TRUE;
		}
		 static LLStdStringHandle group_string = LLXmlTree::addAttributeString("group");
		if (child->getFastAttributeS32(group_string, info->mGroup))
		{
			if (info->mGroup == -1)
				info->mGroup = -1111; // -1 = none parsed, < -1 = bad value
		}

		static LLStdStringHandle id_string = LLXmlTree::addAttributeString("id");
		if (!child->getFastAttributeS32(id_string, info->mAttachmentID))
		{
			llwarns << "No id supplied for attachment point " << info->mName << llendl;
			delete info;
			continue;
		}

		static LLStdStringHandle slot_string = LLXmlTree::addAttributeString("pie_slice");
		child->getFastAttributeS32(slot_string, info->mPieMenuSlice);
			
		static LLStdStringHandle visible_in_first_person_string = LLXmlTree::addAttributeString("visible_in_first_person");
		child->getFastAttributeBOOL(visible_in_first_person_string, info->mVisibleFirstPerson);

		static LLStdStringHandle hud_attachment_string = LLXmlTree::addAttributeString("hud");
		child->getFastAttributeBOOL(hud_attachment_string, info->mIsHUDAttachment);

		mAttachmentInfoList.push_back(info);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlMeshNodes(): parses <mesh> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatarXmlInfo::parseXmlMeshNodes(LLXmlTreeNode* root)
{
	for (LLXmlTreeNode* node = root->getChildByName( "mesh" );
		 node;
		 node = root->getNextNamedChild())
	{
		LLVOAvatarMeshInfo *info = new LLVOAvatarMeshInfo;

		// attribute: type
		static LLStdStringHandle type_string = LLXmlTree::addAttributeString("type");
		if( !node->getFastAttributeString( type_string, info->mType ) )
		{
			llwarns << "Avatar file: <mesh> is missing type attribute.  Ignoring element. " << llendl;
			delete info;
			return FALSE;  // Ignore this element
		}
		
		static LLStdStringHandle lod_string = LLXmlTree::addAttributeString("lod");
		if (!node->getFastAttributeS32( lod_string, info->mLOD ))
		{
			llwarns << "Avatar file: <mesh> is missing lod attribute.  Ignoring element. " << llendl;
			delete info;
			return FALSE;  // Ignore this element
		}

		static LLStdStringHandle file_name_string = LLXmlTree::addAttributeString("file_name");
		if( !node->getFastAttributeString( file_name_string, info->mMeshFileName ) )
		{
			llwarns << "Avatar file: <mesh> is missing file_name attribute.  Ignoring: " << info->mType << llendl;
			delete info;
			return FALSE;  // Ignore this element
		}

		static LLStdStringHandle reference_string = LLXmlTree::addAttributeString("reference");
		node->getFastAttributeString( reference_string, info->mReferenceMeshName );
		
		// attribute: min_pixel_area
		static LLStdStringHandle min_pixel_area_string = LLXmlTree::addAttributeString("min_pixel_area");
		static LLStdStringHandle min_pixel_width_string = LLXmlTree::addAttributeString("min_pixel_width");
		if (!node->getFastAttributeF32( min_pixel_area_string, info->mMinPixelArea ))
		{
			F32 min_pixel_area = 0.1f;
			if (node->getFastAttributeF32( min_pixel_width_string, min_pixel_area ))
			{
				// this is square root of pixel area (sensible to use linear space in defining lods)
				min_pixel_area = min_pixel_area * min_pixel_area;
			}
			info->mMinPixelArea = min_pixel_area;
		}
		
		// Parse visual params for this node only if we haven't already
		for (LLXmlTreeNode* child = node->getChildByName( "param" );
			 child;
			 child = node->getNextNamedChild())
		{
			if (!child->getChildByName("param_morph"))
			{
				if (child->getChildByName("param_skeleton"))
				{
					llwarns << "Can't specify skeleton param in a mesh definition." << llendl;
				}
				else
				{
					llwarns << "Unknown param type." << llendl;
				}
				continue;
			}

			LLPolyMorphTargetInfo *morphinfo = new LLPolyMorphTargetInfo();
			if (!morphinfo->parseXml(child))
			{
				delete morphinfo;
				delete info;
				return -1;
			}
			BOOL shared = FALSE;
			static LLStdStringHandle shared_string = LLXmlTree::addAttributeString("shared");
			child->getFastAttributeBOOL(shared_string, shared);

			info->mPolyMorphTargetInfoList.push_back(LLVOAvatarMeshInfo::morph_info_pair_t(morphinfo, shared));
		}

		mMeshInfoList.push_back(info);
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlColorNodes(): parses <global_color> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatarXmlInfo::parseXmlColorNodes(LLXmlTreeNode* root)
{
	for (LLXmlTreeNode* color_node = root->getChildByName( "global_color" );
		 color_node;
		 color_node = root->getNextNamedChild())
	{
		std::string global_color_name;
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (color_node->getFastAttributeString( name_string, global_color_name ) )
		{
			if( global_color_name == "skin_color" )
			{
				if (mTexSkinColorInfo)
				{
					llwarns << "avatar file: multiple instances of skin_color" << llendl;
					return FALSE;
				}
				mTexSkinColorInfo = new LLTexGlobalColorInfo;
				if( !mTexSkinColorInfo->parseXml( color_node ) )
				{
					delete mTexSkinColorInfo; mTexSkinColorInfo = 0;
					llwarns << "avatar file: mTexSkinColor->parseXml() failed" << llendl;
					return FALSE;
				}
			}
			else if( global_color_name == "hair_color" )
			{
				if (mTexHairColorInfo)
				{
					llwarns << "avatar file: multiple instances of hair_color" << llendl;
					return FALSE;
				}
				mTexHairColorInfo = new LLTexGlobalColorInfo;
				if( !mTexHairColorInfo->parseXml( color_node ) )
				{
					delete mTexHairColorInfo; mTexHairColorInfo = 0;
					llwarns << "avatar file: mTexHairColor->parseXml() failed" << llendl;
					return FALSE;
				}
			}
			else if( global_color_name == "eye_color" )
			{
				if (mTexEyeColorInfo)
				{
					llwarns << "avatar file: multiple instances of eye_color" << llendl;
					return FALSE;
				}
				mTexEyeColorInfo = new LLTexGlobalColorInfo;
				if( !mTexEyeColorInfo->parseXml( color_node ) )
				{
					llwarns << "avatar file: mTexEyeColor->parseXml() failed" << llendl;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlLayerNodes(): parses <layer_set> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatarXmlInfo::parseXmlLayerNodes(LLXmlTreeNode* root)
{
	for (LLXmlTreeNode* layer_node = root->getChildByName( "layer_set" );
		 layer_node;
		 layer_node = root->getNextNamedChild())
	{
		LLTexLayerSetInfo* layer_info = new LLTexLayerSetInfo();
		if( layer_info->parseXml( layer_node ) )
		{
			mLayerInfoList.push_back(layer_info);
		}
		else
		{
			delete layer_info;
			llwarns << "avatar file: layer_set->parseXml() failed" << llendl;
			return FALSE;
		}
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// parseXmlDriverNodes(): parses <driver_parameters> nodes from XML tree
//-----------------------------------------------------------------------------
BOOL LLVOAvatarXmlInfo::parseXmlDriverNodes(LLXmlTreeNode* root)
{
	LLXmlTreeNode* driver = root->getChildByName( "driver_parameters" );
	if( driver )
	{
		for (LLXmlTreeNode* grand_child = driver->getChildByName( "param" );
			 grand_child;
			 grand_child = driver->getNextNamedChild())
		{
			if( grand_child->getChildByName( "param_driver" ) )
			{
				LLDriverParamInfo* driver_info = new LLDriverParamInfo();
				if( driver_info->parseXml( grand_child ) )
				{
					mDriverInfoList.push_back(driver_info);
				}
				else
				{
					delete driver_info;
					llwarns << "avatar file: driver_param->parseXml() failed" << llendl;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

// warning: order(N) not order(1)
S32 LLVOAvatar::getAttachmentCount()
{
	S32 count = mAttachmentPoints.size();
	return count;
}

//virtual
void LLVOAvatar::updateRegion(LLViewerRegion *regionp)
{
	if (mIsSelf)
	{
		if (regionp->getHandle() != mLastRegionHandle)
		{
			if (mLastRegionHandle != 0)
			{
				++mRegionCrossingCount;
				F64 delta = (F64)mRegionCrossingTimer.getElapsedTimeF32();
				F64 avg = (mRegionCrossingCount == 1) ? 0 : LLViewerStats::getInstance()->getStat(LLViewerStats::ST_CROSSING_AVG);
				F64 delta_avg = (delta + avg*(mRegionCrossingCount-1)) / mRegionCrossingCount;
				LLViewerStats::getInstance()->setStat(LLViewerStats::ST_CROSSING_AVG, delta_avg);

				F64 max = (mRegionCrossingCount == 1) ? 0 : LLViewerStats::getInstance()->getStat(LLViewerStats::ST_CROSSING_MAX);
				max = llmax(delta, max);
				LLViewerStats::getInstance()->setStat(LLViewerStats::ST_CROSSING_MAX, max);
			}
			mLastRegionHandle = regionp->getHandle();
		}
		mRegionCrossingTimer.reset();
	}
}

std::string LLVOAvatar::getFullname() const
{
	std::string name;

	LLNameValue* first = getNVPair("FirstName"); 
	LLNameValue* last  = getNVPair("LastName"); 
	if (first && last)
	{
		name += first->getString();
		name += " ";
		name += last->getString();
	}

	return name;
}

LLTexLayerSet* LLVOAvatar::getLayerSet(ETextureIndex index) const
{
	/* switch(index)
		case TEX_HEAD_BAKED:
		case TEX_HEAD_BODYPAINT:
			return mHeadLayerSet; */
	const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = LLVOAvatarDictionary::getInstance()->getTexture(index);
	if (text_dict->mIsUsedByBakedTexture)
	{
		const EBakedTextureIndex baked_index = text_dict->mBakedTextureIndex;
		return mBakedTextureData[baked_index].mTexLayerSet;
	}
	return NULL;
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
	BOOL res = updateJointLODs();

	LLFace* facep = mDrawable->getFace(0);
	if (facep->mVertexBuffer.isNull() ||
		LLVertexBuffer::sEnableVBOs &&
		((facep->mVertexBuffer->getUsage() == GL_STATIC_DRAW ? TRUE : FALSE) !=
		(facep->getPool()->getVertexShaderLevel() > 0 ? TRUE : FALSE)))
	{
		mDirtyMesh = TRUE;
	}

	if (mDirtyMesh || mDrawable->isState(LLDrawable::REBUILD_GEOMETRY))
	{	//LOD changed or new mesh created, allocate new vertex buffer if needed
		updateMeshData();
		mDirtyMesh = FALSE;
		mNeedsSkin = TRUE;
		mDrawable->clearState(LLDrawable::REBUILD_GEOMETRY);
	}
	
	updateVisibility();

	return res;
}

U32 LLVOAvatar::getPartitionType() const
{ //avatars merely exist as drawables in the bridge partition
	return LLViewerRegion::PARTITION_BRIDGE;
}

//static
void LLVOAvatar::updateImpostors() 
{
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* avatar = (LLVOAvatar*) *iter;
		
		if (!avatar->isDead() && avatar->needsImpostorUpdate() && avatar->isVisible() && avatar->isImpostor())
		{
			gPipeline.generateImpostor(avatar);
		}
	}
}

BOOL LLVOAvatar::isImpostor() const
{
	return (sUseImpostors && mUpdatePeriod >= IMPOSTOR_PERIOD) ? TRUE : FALSE;
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

void LLVOAvatar::getImpostorValues(LLVector3* extents, LLVector3& angle, F32& distance) const
{
	const LLVector3* ext = mDrawable->getSpatialExtents();
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
	if (!gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_SHAME))
	{
		return;
	}

	U32 shame = 1;

	std::set<LLUUID> textures;

	attachment_map_t::const_iterator iter;
	for (iter = mAttachmentPoints.begin(); 
		iter != mAttachmentPoints.end();
		++iter)
	{
		LLViewerJointAttachment* attachment = iter->second;
		LLViewerObject* object = attachment->getObject();
		if (object && !object->isHUDAttachment())
		{
			LLDrawable* drawable = object->mDrawable;
			if (drawable)
			{
				shame += 10;
				LLVOVolume* volume = drawable->getVOVolume();
				if (volume)
				{
					shame += calc_shame(volume, textures);
				}
			}
		}
	}	

	shame += textures.size() * 5;

	setDebugText(llformat("%d", shame));
	F32 green = 1.f-llclamp(((F32) shame-1024.f)/1024.f, 0.f, 1.f);
	F32 red = llmin((F32) shame/1024.f, 1.f);
	mText->setColor(LLColor4(red,green,0,1));
}

// static
BOOL LLVOAvatar::isIndexLocalTexture(ETextureIndex index)
{
	if (index < 0 || index >= TEX_NUM_INDICES) return false;
	return LLVOAvatarDictionary::getInstance()->getTexture(index)->mIsLocalTexture;
}

// static
BOOL LLVOAvatar::isIndexBakedTexture(ETextureIndex index)
{
	if (index < 0 || index >= TEX_NUM_INDICES) return false;
	return LLVOAvatarDictionary::getInstance()->getTexture(index)->mIsBakedTexture;
}

const std::string LLVOAvatar::getBakedStatusForPrintout() const
{
	std::string line;

	for (LLVOAvatarDictionary::texture_map_t::const_iterator iter = LLVOAvatarDictionary::getInstance()->getTextures().begin();
		 iter != LLVOAvatarDictionary::getInstance()->getTextures().end();
		 iter++)
	{
		const ETextureIndex index = iter->first;
		const LLVOAvatarDictionary::TextureDictionaryEntry *text_dict = iter->second;
		if (text_dict->mIsBakedTexture)
		{
			line += text_dict->mName;
			if (isTextureDefined(index))
			{
				line += "_baked";
			}
			line += " ";
		}
	}
	return line;
}


U32 calc_shame(LLVOVolume* volume, std::set<LLUUID> &textures)
{
	if (!volume)
	{
		return 0;
	}

	U32 shame = 0;

	U32 invisi = 0;
	U32 shiny = 0;
	U32 glow = 0;
	U32 alpha = 0;
	U32 flexi = 0;
	U32 animtex = 0;
	U32 particles = 0;
	U32 scale = 0;
	U32 bump = 0;
	U32 planar = 0;
	
	if (volume->isFlexible())
	{
		flexi = 1;
	}
	if (volume->isParticleSource())
	{
		particles = 1;
	}

	const LLVector3& sc = volume->getScale();
	scale += (U32) sc.mV[0] + (U32) sc.mV[1] + (U32) sc.mV[2];

	LLDrawable* drawablep = volume->mDrawable;

	if (volume->isSculpted())
	{
		LLSculptParams *sculpt_params = (LLSculptParams *) volume->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
		LLUUID sculpt_id = sculpt_params->getSculptTexture();
		textures.insert(sculpt_id);
	}

	for (S32 i = 0; i < drawablep->getNumFaces(); ++i)
	{
		LLFace* face = drawablep->getFace(i);
		const LLTextureEntry* te = face->getTextureEntry();
		LLViewerImage* img = face->getTexture();

		textures.insert(img->getID());

		if (face->getPoolType() == LLDrawPool::POOL_ALPHA)
		{
			alpha++;
		}
		else if (img->getPrimaryFormat() == GL_ALPHA)
		{
			invisi = 1;
		}

		if (te)
		{
			if (te->getBumpmap())
			{
				bump = 1;
			}
			if (te->getShiny())
			{
				shiny = 1;
			}
			if (te->getGlow() > 0.f)
			{
				glow = 1;
			}
			if (face->mTextureMatrix != NULL)
			{
				animtex++;
			}
			if (te->getTexGen())
			{
				planar++;
			}
		}
	}

	shame += invisi + shiny + glow + alpha*4 + flexi*8 + animtex*4 + particles*16+bump*4+scale+planar;

	LLViewerObject::const_child_list_t& child_list = volume->getChildren();
	for (LLViewerObject::child_list_t::const_iterator iter = child_list.begin();
		 iter != child_list.end(); iter++)
	{
		LLViewerObject* child_objectp = *iter;
		LLDrawable* child_drawablep = child_objectp->mDrawable;
		if (child_drawablep)
		{
			LLVOVolume* child_volumep = child_drawablep->getVOVolume();
			if (child_volumep)
			{
				shame += calc_shame(child_volumep, textures);
			}
		}
	}

	return shame;
}

//-----------------------------------------------------------------------------
// Utility functions
//-----------------------------------------------------------------------------

F32 calc_bouncy_animation(F32 x)
{
	return -(cosf(x * F_PI * 2.5f - F_PI_BY_TWO))*(0.4f + x * -0.1f) + x * 1.3f;
}
