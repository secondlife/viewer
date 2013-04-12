/** 
 * @file llhudeffectlookat.cpp
 * @brief LLHUDEffectLookAt class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llhudeffectlookat.h"

#include "llrender.h"

#include "message.h"
#include "llagent.h"
#include "llagentcamera.h"
#include "llvoavatar.h"
#include "lldrawable.h"
#include "llviewerobjectlist.h"
#include "llrendersphere.h"
#include "llselectmgr.h"
#include "llglheaders.h"


#include "llxmltree.h"


BOOL LLHUDEffectLookAt::sDebugLookAt = FALSE;

// packet layout
const S32 SOURCE_AVATAR = 0;
const S32 TARGET_OBJECT = 16;
const S32 TARGET_POS = 32;
const S32 LOOKAT_TYPE = 56;
const S32 PKT_SIZE = 57;

// throttle
const F32 MAX_SENDS_PER_SEC = 4.f;

const F32 MIN_DELTAPOS_FOR_UPDATE_SQUARED = 0.05f * 0.05f;
const F32 MIN_TARGET_OFFSET_SQUARED = 0.0001f;


// can't use actual F32_MAX, because we add this to the current frametime
const F32 MAX_TIMEOUT = F32_MAX / 2.f;

/**
 * Simple data class holding values for a particular type of attention.
 */
class LLAttention
{
public:
	LLAttention()
		: mTimeout(0.f),
		  mPriority(0.f)
	{}
	LLAttention(F32 timeout, F32 priority, const std::string& name, LLColor3 color) :
	  mTimeout(timeout), mPriority(priority), mName(name), mColor(color)
	{
	}
	F32 mTimeout, mPriority;
	std::string mName;
	LLColor3 mColor;
};

/**
 * Simple data class holding a list of attentions, one for every type.
 */
class LLAttentionSet
{
public:
	LLAttentionSet(const LLAttention attentions[])
	{
		for(int i=0; i<LOOKAT_NUM_TARGETS; i++)
		{
			mAttentions[i] = attentions[i];
		}
	}
	LLAttention mAttentions[LOOKAT_NUM_TARGETS];
	LLAttention& operator[](int idx) { return mAttentions[idx]; }
};

// Default attribute set data.
// Used to initialize the global attribute set objects, one of which will be
// refered to by the hud object at any given time.
// Note that the values below are only the default values and that any or all of them
// can be overwritten with customizing data from the XML file. The actual values below
// are those that will give exactly the same look-at behavior as before the ability
// to customize was added. - MG
static const 
	LLAttention 
		BOY_ATTS[] = { // default set of masculine attentions
			LLAttention(MAX_TIMEOUT, 0, "None",			 LLColor3(0.3f, 0.3f, 0.3f)), // LOOKAT_TARGET_NONE
			LLAttention(3.f,         1, "Idle",		     LLColor3(0.5f, 0.5f, 0.5f)), // LOOKAT_TARGET_IDLE
			LLAttention(4.f,         3, "AutoListen",	 LLColor3(0.5f, 0.5f, 0.5f)), // LOOKAT_TARGET_AUTO_LISTEN
			LLAttention(2.f,         2, "FreeLook",		 LLColor3(0.5f, 0.5f, 0.9f)), // LOOKAT_TARGET_FREELOOK
			LLAttention(4.f,         3, "Respond",	     LLColor3(0.0f, 0.0f, 0.0f)), // LOOKAT_TARGET_RESPOND
			LLAttention(1.f,         4, "Hover",		 LLColor3(0.5f, 0.9f, 0.5f)), // LOOKAT_TARGET_HOVER
			LLAttention(MAX_TIMEOUT, 0, "Conversation",  LLColor3(0.1f, 0.1f, 0.5f)), // LOOKAT_TARGET_CONVERSATION
			LLAttention(MAX_TIMEOUT, 6, "Select",		 LLColor3(0.9f, 0.5f, 0.5f)), // LOOKAT_TARGET_SELECT
			LLAttention(MAX_TIMEOUT, 6, "Focus",		 LLColor3(0.9f, 0.5f, 0.9f)), // LOOKAT_TARGET_FOCUS
			LLAttention(MAX_TIMEOUT, 7, "Mouselook",	 LLColor3(0.9f, 0.9f, 0.5f)), // LOOKAT_TARGET_MOUSELOOK
			LLAttention(0.f,         8, "Clear",		 LLColor3(1.0f, 1.0f, 1.0f)), // LOOKAT_TARGET_CLEAR
		},																				
		GIRL_ATTS[] = { // default set of feminine attentions													
			LLAttention(MAX_TIMEOUT, 0, "None",			 LLColor3(0.3f, 0.3f, 0.3f)), // LOOKAT_TARGET_NONE
			LLAttention(3.f,         1, "Idle",		     LLColor3(0.5f, 0.5f, 0.5f)), // LOOKAT_TARGET_IDLE
			LLAttention(4.f,         3, "AutoListen",	 LLColor3(0.5f, 0.5f, 0.5f)), // LOOKAT_TARGET_AUTO_LISTEN
			LLAttention(2.f,         2, "FreeLook",		 LLColor3(0.5f, 0.5f, 0.9f)), // LOOKAT_TARGET_FREELOOK
			LLAttention(4.f,         3, "Respond",	     LLColor3(0.0f, 0.0f, 0.0f)), // LOOKAT_TARGET_RESPOND
			LLAttention(1.f,         4, "Hover",		 LLColor3(0.5f, 0.9f, 0.5f)), // LOOKAT_TARGET_HOVER
			LLAttention(MAX_TIMEOUT, 0, "Conversation",  LLColor3(0.1f, 0.1f, 0.5f)), // LOOKAT_TARGET_CONVERSATION
			LLAttention(MAX_TIMEOUT, 6, "Select",		 LLColor3(0.9f, 0.5f, 0.5f)), // LOOKAT_TARGET_SELECT
			LLAttention(MAX_TIMEOUT, 6, "Focus",		 LLColor3(0.9f, 0.5f, 0.9f)), // LOOKAT_TARGET_FOCUS
			LLAttention(MAX_TIMEOUT, 7, "Mouselook",	 LLColor3(0.9f, 0.9f, 0.5f)), // LOOKAT_TARGET_MOUSELOOK
			LLAttention(0.f,         8, "Clear",		 LLColor3(1.0f, 1.0f, 1.0f)), // LOOKAT_TARGET_CLEAR
		};

static LLAttentionSet
	gBoyAttentions(BOY_ATTS),
	gGirlAttentions(GIRL_ATTS);


static BOOL loadGender(LLXmlTreeNode* gender)
{
	if( !gender)
	{
		return FALSE;
	}
	std::string str;
	gender->getAttributeString("name", str);
	LLAttentionSet& attentions = (str.compare("Masculine") == 0) ? gBoyAttentions : gGirlAttentions;
	for (LLXmlTreeNode* attention_node = gender->getChildByName( "param" );
		 attention_node;
		 attention_node = gender->getNextNamedChild())
	{
		attention_node->getAttributeString("attention", str);
		LLAttention* attention;
		if     (str == "idle")         attention = &attentions[LOOKAT_TARGET_IDLE];
		else if(str == "auto_listen")  attention = &attentions[LOOKAT_TARGET_AUTO_LISTEN];
		else if(str == "freelook")     attention = &attentions[LOOKAT_TARGET_FREELOOK];
		else if(str == "respond")      attention = &attentions[LOOKAT_TARGET_RESPOND];
		else if(str == "hover")        attention = &attentions[LOOKAT_TARGET_HOVER];
		else if(str == "conversation") attention = &attentions[LOOKAT_TARGET_CONVERSATION];
		else if(str == "select")       attention = &attentions[LOOKAT_TARGET_SELECT];
		else if(str == "focus")        attention = &attentions[LOOKAT_TARGET_FOCUS];
		else if(str == "mouselook")    attention = &attentions[LOOKAT_TARGET_MOUSELOOK];
		else return FALSE;

		F32 priority, timeout;
		attention_node->getAttributeF32("priority", priority);
		attention_node->getAttributeF32("timeout", timeout);
		if(timeout < 0) timeout = MAX_TIMEOUT;
		attention->mPriority = priority;
		attention->mTimeout = timeout;
	}	
	return TRUE;
}

static BOOL loadAttentions()
{
	static BOOL first_time = TRUE;
	if( ! first_time) 
	{
		return TRUE; // maybe not ideal but otherwise it can continue to fail forever.
	}
	first_time = FALSE;
	
	std::string filename;
	filename = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,"attentions.xml");
	LLXmlTree xml_tree;
	BOOL success = xml_tree.parseFile( filename, FALSE );
	if( !success )
	{
		return FALSE;
	}
	LLXmlTreeNode* root = xml_tree.getRoot();
	if( !root )
	{
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// <linden_attentions version="1.0"> (root)
	//-------------------------------------------------------------------------
	if( !root->hasName( "linden_attentions" ) )
	{
		llwarns << "Invalid linden_attentions file header: " << filename << llendl;
		return FALSE;
	}

	std::string version;
	static LLStdStringHandle version_string = LLXmlTree::addAttributeString("version");
	if( !root->getFastAttributeString( version_string, version ) || (version != "1.0") )
	{
		llwarns << "Invalid linden_attentions file version: " << version << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// <gender>
	//-------------------------------------------------------------------------
	for (LLXmlTreeNode* child = root->getChildByName( "gender" );
		 child;
		 child = root->getNextNamedChild())
	{
		if( !loadGender( child ) )
		{
			return FALSE;
		}
	}

	return TRUE;
}




//-----------------------------------------------------------------------------
// LLHUDEffectLookAt()
//-----------------------------------------------------------------------------
LLHUDEffectLookAt::LLHUDEffectLookAt(const U8 type) : 
	LLHUDEffect(type), 
	mKillTime(0.f),
	mLastSendTime(0.f)
{
	clearLookAtTarget();
	// parse the default sets
	loadAttentions();
	// initialize current attention set. switches when avatar sex changes.
	mAttentions = &gGirlAttentions;
}

//-----------------------------------------------------------------------------
// ~LLHUDEffectLookAt()
//-----------------------------------------------------------------------------
LLHUDEffectLookAt::~LLHUDEffectLookAt()
{
}

//-----------------------------------------------------------------------------
// packData()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::packData(LLMessageSystem *mesgsys)
{
	// Pack the default data
	LLHUDEffect::packData(mesgsys);

	// Pack the type-specific data.  Uses a fun packed binary format.  Whee!
	U8 packed_data[PKT_SIZE];
	memset(packed_data, 0, PKT_SIZE);

	if (mSourceObject)
	{
		htonmemcpy(&(packed_data[SOURCE_AVATAR]), mSourceObject->mID.mData, MVT_LLUUID, 16);
	}
	else
	{
		htonmemcpy(&(packed_data[SOURCE_AVATAR]), LLUUID::null.mData, MVT_LLUUID, 16);
	}

	// pack both target object and position
	// position interpreted as offset if target object is non-null
	if (mTargetObject)
	{
		htonmemcpy(&(packed_data[TARGET_OBJECT]), mTargetObject->mID.mData, MVT_LLUUID, 16);
	}
	else
	{
		htonmemcpy(&(packed_data[TARGET_OBJECT]), LLUUID::null.mData, MVT_LLUUID, 16);
	}

	htonmemcpy(&(packed_data[TARGET_POS]), mTargetOffsetGlobal.mdV, MVT_LLVector3d, 24);

	U8 lookAtTypePacked = (U8)mTargetType;
	
	htonmemcpy(&(packed_data[LOOKAT_TYPE]), &lookAtTypePacked, MVT_U8, 1);

	mesgsys->addBinaryDataFast(_PREHASH_TypeData, packed_data, PKT_SIZE);

	mLastSendTime = mTimer.getElapsedTimeF32();
}

//-----------------------------------------------------------------------------
// unpackData()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::unpackData(LLMessageSystem *mesgsys, S32 blocknum)
{
	LLVector3d new_target;
	U8 packed_data[PKT_SIZE];

	LLUUID dataId;
	mesgsys->getUUIDFast(_PREHASH_Effect, _PREHASH_ID, dataId, blocknum);

	if (!gAgentCamera.mLookAt.isNull() && dataId == gAgentCamera.mLookAt->getID())
	{
		return;
	}

	LLHUDEffect::unpackData(mesgsys, blocknum);
	LLUUID source_id;
	LLUUID target_id;
	S32 size = mesgsys->getSizeFast(_PREHASH_Effect, blocknum, _PREHASH_TypeData);
	if (size != PKT_SIZE)
	{
		llwarns << "LookAt effect with bad size " << size << llendl;
		return;
	}
	mesgsys->getBinaryDataFast(_PREHASH_Effect, _PREHASH_TypeData, packed_data, PKT_SIZE, blocknum);
	
	htonmemcpy(source_id.mData, &(packed_data[SOURCE_AVATAR]), MVT_LLUUID, 16);

	LLViewerObject *objp = gObjectList.findObject(source_id);
	if (objp && objp->isAvatar())
	{
		setSourceObject(objp);
	}
	else
	{
		//llwarns << "Could not find source avatar for lookat effect" << llendl;
		return;
	}

	htonmemcpy(target_id.mData, &(packed_data[TARGET_OBJECT]), MVT_LLUUID, 16);

	objp = gObjectList.findObject(target_id);

	htonmemcpy(new_target.mdV, &(packed_data[TARGET_POS]), MVT_LLVector3d, 24);

	if (objp)
	{
		setTargetObjectAndOffset(objp, new_target);
	}
	else if (target_id.isNull())
	{
		setTargetPosGlobal(new_target);
	}
	else
	{
		//llwarns << "Could not find target object for lookat effect" << llendl;
	}

	U8 lookAtTypeUnpacked = 0;
	htonmemcpy(&lookAtTypeUnpacked, &(packed_data[LOOKAT_TYPE]), MVT_U8, 1);
	mTargetType = (ELookAtType)lookAtTypeUnpacked;

	if (mTargetType == LOOKAT_TARGET_NONE)
	{
		clearLookAtTarget();
	}
}

//-----------------------------------------------------------------------------
// setTargetObjectAndOffset()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::setTargetObjectAndOffset(LLViewerObject *objp, LLVector3d offset)
{
	mTargetObject = objp;
	mTargetOffsetGlobal = offset;
}

//-----------------------------------------------------------------------------
// setTargetPosGlobal()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::setTargetPosGlobal(const LLVector3d &target_pos_global)
{
	mTargetObject = NULL;
	mTargetOffsetGlobal = target_pos_global;
}

//-----------------------------------------------------------------------------
// setLookAt()
// called by agent logic to set look at behavior locally, and propagate to sim
//-----------------------------------------------------------------------------
BOOL LLHUDEffectLookAt::setLookAt(ELookAtType target_type, LLViewerObject *object, LLVector3 position)
{
	if (!mSourceObject)
	{
		return FALSE;
	}
	
	if (target_type >= LOOKAT_NUM_TARGETS)
	{
		llwarns << "Bad target_type " << (int)target_type << " - ignoring." << llendl;
		return FALSE;
	}

	// must be same or higher priority than existing effect
	if ((*mAttentions)[target_type].mPriority < (*mAttentions)[mTargetType].mPriority)
	{
		return FALSE;
	}

	F32 current_time  = mTimer.getElapsedTimeF32();

	// type of lookat behavior or target object has changed
	BOOL lookAtChanged = (target_type != mTargetType) || (object != mTargetObject);

	// lookat position has moved a certain amount and we haven't just sent an update
	lookAtChanged = lookAtChanged || ((dist_vec_squared(position, mLastSentOffsetGlobal) > MIN_DELTAPOS_FOR_UPDATE_SQUARED) && 
		((current_time - mLastSendTime) > (1.f / MAX_SENDS_PER_SEC)));

	if (lookAtChanged)
	{
		mLastSentOffsetGlobal = position;
		F32 timeout = (*mAttentions)[target_type].mTimeout;
		setDuration(timeout);
		setNeedsSendToSim(TRUE);
	}
 
	if (target_type == LOOKAT_TARGET_CLEAR)
	{
		clearLookAtTarget();
	}
	else
	{
		mTargetType = target_type;
		mTargetObject = object;
		if (object)
		{
			mTargetOffsetGlobal.setVec(position);
		}
		else
		{
			mTargetOffsetGlobal = gAgent.getPosGlobalFromAgent(position);
		}
		mKillTime = mTimer.getElapsedTimeF32() + mDuration;

		update();
	}
	return TRUE;
}

//-----------------------------------------------------------------------------
// clearLookAtTarget()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::clearLookAtTarget()
{
	mTargetObject = NULL;
	mTargetOffsetGlobal.clearVec();
	mTargetType = LOOKAT_TARGET_NONE;
	if (mSourceObject.notNull())
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->stopMotion(ANIM_AGENT_HEAD_ROT);
	}
}

//-----------------------------------------------------------------------------
// markDead()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::markDead()
{
	if (mSourceObject.notNull())
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->removeAnimationData("LookAtPoint");
	}

	mSourceObject = NULL;
	clearLookAtTarget();
	LLHUDEffect::markDead();
}

void LLHUDEffectLookAt::setSourceObject(LLViewerObject* objectp)
{
	// restrict source objects to avatars
	if (objectp && objectp->isAvatar())
	{
		LLHUDEffect::setSourceObject(objectp);
	}
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::render()
{
	if (sDebugLookAt && mSourceObject.notNull())
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLVector3 target = mTargetPos + ((LLVOAvatar*)(LLViewerObject*)mSourceObject)->mHeadp->getWorldPosition();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		gGL.pushMatrix();
		gGL.translatef(target.mV[VX], target.mV[VY], target.mV[VZ]);
		gGL.scalef(0.3f, 0.3f, 0.3f);
		gGL.begin(LLRender::LINES);
		{
			LLColor3 color = (*mAttentions)[mTargetType].mColor;
			gGL.color3f(color.mV[VRED], color.mV[VGREEN], color.mV[VBLUE]);
			gGL.vertex3f(-1.f, 0.f, 0.f);
			gGL.vertex3f(1.f, 0.f, 0.f);

			gGL.vertex3f(0.f, -1.f, 0.f);
			gGL.vertex3f(0.f, 1.f, 0.f);

			gGL.vertex3f(0.f, 0.f, -1.f);
			gGL.vertex3f(0.f, 0.f, 1.f);
		} gGL.end();
		gGL.popMatrix();
	}
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
void LLHUDEffectLookAt::update()
{
	// If the target object is dead, set the target object to NULL
	if (!mTargetObject.isNull() && mTargetObject->isDead())
	{
		clearLookAtTarget();
	}

	// if source avatar is null or dead, mark self as dead and return
	if (mSourceObject.isNull() || mSourceObject->isDead())
	{
		markDead();
		return;
	}

	// make sure the proper set of avatar attention are currently being used.
	LLVOAvatar* source_avatar = (LLVOAvatar*)(LLViewerObject*)mSourceObject;
	// for now the first cut will just switch on sex. future development could adjust 
	// timeouts according to avatar age and/or other features. 
	mAttentions = (source_avatar->getSex() == SEX_MALE) ? &gBoyAttentions : &gGirlAttentions;
	//printf("updated to %s\n", (source_avatar->getSex() == SEX_MALE) ? "male" : "female");

	F32 time = mTimer.getElapsedTimeF32();

	// clear out the effect if time is up
	if (mKillTime != 0.f && time > mKillTime)
	{
		if (mTargetType != LOOKAT_TARGET_NONE)
		{
			clearLookAtTarget();
			// look at timed out (only happens on own avatar), so tell everyone
			setNeedsSendToSim(TRUE);
		}
	}

	if (mTargetType != LOOKAT_TARGET_NONE)
	{
		if (calcTargetPosition())
		{
			LLMotion* head_motion = ((LLVOAvatar*)(LLViewerObject*)mSourceObject)->findMotion(ANIM_AGENT_HEAD_ROT);
			if (!head_motion || head_motion->isStopped())
			{
				((LLVOAvatar*)(LLViewerObject*)mSourceObject)->startMotion(ANIM_AGENT_HEAD_ROT);
			}
		}
	}

	if (sDebugLookAt)
	{
		((LLVOAvatar*)(LLViewerObject*)mSourceObject)->addDebugText((*mAttentions)[mTargetType].mName);
	}
}

/**
 * Initializes the mTargetPos member from the current mSourceObjec and mTargetObject
 * (and possibly mTargetOffsetGlobal).
 * When mTargetObject is another avatar, it sets mTargetPos to be their eyes.
 * 
 * Has the side-effect of also calling setAnimationData("LookAtPoint") with the new
 * mTargetPos on the source object which is assumed to be an avatar.
 *
 * Returns whether we successfully calculated a finite target position.
 */
bool LLHUDEffectLookAt::calcTargetPosition()
{
	LLViewerObject *target_obj = (LLViewerObject *)mTargetObject;
	LLVector3 local_offset;
	
	if (target_obj)
	{
		local_offset.setVec(mTargetOffsetGlobal);
	}
	else
	{
		local_offset = gAgent.getPosAgentFromGlobal(mTargetOffsetGlobal);
	}

	LLVOAvatar* source_avatar = (LLVOAvatar*)(LLViewerObject*)mSourceObject;
	if (!source_avatar->isBuilt())
		return false;
	
	if (target_obj && target_obj->mDrawable.notNull())
	{
		LLQuaternion target_rot;
		if (target_obj->isAvatar())
		{
			LLVOAvatar *target_av = (LLVOAvatar *)target_obj;

			BOOL looking_at_self = source_avatar->isSelf() && target_av->isSelf();

			// if selecting self, stare forward
			if (looking_at_self && mTargetOffsetGlobal.magVecSquared() < MIN_TARGET_OFFSET_SQUARED)
			{
				//sets the lookat point in front of the avatar
				mTargetOffsetGlobal.setVec(5.0, 0.0, 0.0);
				local_offset.setVec(mTargetOffsetGlobal);
			}

			// look the other avatar in the eye. note: what happens if target is self? -MG
			mTargetPos = target_av->mHeadp->getWorldPosition();
			if (mTargetType == LOOKAT_TARGET_MOUSELOOK || mTargetType == LOOKAT_TARGET_FREELOOK)
			{
				// mouselook and freelook target offsets are absolute
				target_rot = LLQuaternion::DEFAULT;
			}
			else if (looking_at_self && gAgentCamera.cameraCustomizeAvatar())
			{
				// *NOTE: We have to do this because animation
				// overrides do not set lookat behavior.
				// *TODO: animation overrides for lookat behavior.
				target_rot = target_av->mPelvisp->getWorldRotation();
			}
			else
			{
				target_rot = target_av->mRoot->getWorldRotation();
			}
		}
		else // target obj is not an avatar
		{
			if (target_obj->mDrawable->getGeneration() == -1)
			{
				mTargetPos = target_obj->getPositionAgent();
				target_rot = target_obj->getWorldRotation();
			}
			else
			{
				mTargetPos = target_obj->getRenderPosition();
				target_rot = target_obj->getRenderRotation();
			}
		}

		mTargetPos += (local_offset * target_rot);
	}
	else // no target obj or it's not drawable
	{
		mTargetPos = local_offset;
	}

	mTargetPos -= source_avatar->mHeadp->getWorldPosition();

	if (!mTargetPos.isFinite())
		return false;

	source_avatar->setAnimationData("LookAtPoint", (void *)&mTargetPos);

	return true;
}
