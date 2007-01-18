/** 
 * @file llmultigesture.cpp
 * @brief Gestures that are asset-based and can have multiple steps.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include <algorithm>

#include "stdio.h"

#include "llmultigesture.h"

#include "llerror.h"
#include "lldatapacker.h"
#include "llstl.h"

const S32 GESTURE_VERSION = 2;

//---------------------------------------------------------------------------
// LLMultiGesture
//---------------------------------------------------------------------------
LLMultiGesture::LLMultiGesture()
:	mKey(),
	mMask(),
	mTrigger(),
	mReplaceText(),
	mSteps(),
	mPlaying(FALSE),
	mCurrentStep(0),
	mDoneCallback(NULL),
	mCallbackData(NULL)
{
	reset();
}

LLMultiGesture::~LLMultiGesture()
{
	std::for_each(mSteps.begin(), mSteps.end(), DeletePointer());
}

void LLMultiGesture::reset()
{
	mPlaying = FALSE;
	mCurrentStep = 0;
	mWaitTimer.reset();
	mWaitingTimer = FALSE;
	mWaitingAnimations = FALSE;
	mWaitingAtEnd = FALSE;
	mRequestedAnimIDs.clear();
	mPlayingAnimIDs.clear();
}

S32 LLMultiGesture::getMaxSerialSize() const
{
	S32 max_size = 0;

	// ascii format, being very conservative about possible
	// label lengths.
	max_size += 64;		// version S32
	max_size += 64;		// key U8
	max_size += 64;		// mask U32
	max_size += 256;	// trigger string
	max_size += 256;	// replace string

	max_size += 64;		// step count S32

	std::vector<LLGestureStep*>::const_iterator it;
	for (it = mSteps.begin(); it != mSteps.end(); ++it)
	{
		LLGestureStep* step = *it;
		max_size += 64;	// type S32
		max_size += step->getMaxSerialSize();
	}

	/* binary format
	max_size += sizeof(S32);	// version
	max_size += sizeof(mKey);
	max_size += sizeof(mMask);
	max_size += mTrigger.length() + 1;	// for null

	max_size += sizeof(S32);	// step count

	std::vector<LLGestureStep*>::const_iterator it;
	for (it = mSteps.begin(); it != mSteps.end(); ++it)
	{
		LLGestureStep* step = *it;
		max_size += sizeof(S32);	// type
		max_size += step->getMaxSerialSize();
	}
	*/
	
	return max_size;
}

BOOL LLMultiGesture::serialize(LLDataPacker& dp) const
{
	dp.packS32(GESTURE_VERSION, "version");
	dp.packU8(mKey, "key");
	dp.packU32(mMask, "mask");
	dp.packString(mTrigger.c_str(), "trigger");
	dp.packString(mReplaceText.c_str(), "replace");

	S32 count = (S32)mSteps.size();
	dp.packS32(count, "step_count");
	S32 i;
	for (i = 0; i < count; ++i)
	{
		LLGestureStep* step = mSteps[i];

		dp.packS32(step->getType(), "step_type");
		BOOL ok = step->serialize(dp);
		if (!ok)
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL LLMultiGesture::deserialize(LLDataPacker& dp)
{
	S32 version;
	dp.unpackS32(version, "version");
	if (version != GESTURE_VERSION)
	{
		llwarns << "Bad LLMultiGesture version " << version
			<< " should be " << GESTURE_VERSION
			<< llendl;
		return FALSE;
	}

	dp.unpackU8(mKey, "key");
	dp.unpackU32(mMask, "mask");

	
	dp.unpackString(mTrigger, "trigger");

	dp.unpackString(mReplaceText, "replace");

	S32 count;
	dp.unpackS32(count, "step_count");
	if (count < 0)
	{
		llwarns << "Bad LLMultiGesture step count " << count << llendl;
		return FALSE;
	}

	S32 i;
	for (i = 0; i < count; ++i)
	{
		S32 type;
		dp.unpackS32(type, "step_type");

		EStepType step_type = (EStepType)type;
		switch(step_type)
		{
		case STEP_ANIMATION:
			{
				LLGestureStepAnimation* step = new LLGestureStepAnimation();
				BOOL ok = step->deserialize(dp);
				if (!ok) return FALSE;
				mSteps.push_back(step);
				break;
			}
		case STEP_SOUND:
			{
				LLGestureStepSound* step = new LLGestureStepSound();
				BOOL ok = step->deserialize(dp);
				if (!ok) return FALSE;
				mSteps.push_back(step);
				break;
			}
		case STEP_CHAT:
			{
				LLGestureStepChat* step = new LLGestureStepChat();
				BOOL ok = step->deserialize(dp);
				if (!ok) return FALSE;
				mSteps.push_back(step);
				break;
			}
		case STEP_WAIT:
			{
				LLGestureStepWait* step = new LLGestureStepWait();
				BOOL ok = step->deserialize(dp);
				if (!ok) return FALSE;
				mSteps.push_back(step);
				break;
			}
		default:
			{
				llwarns << "Bad LLMultiGesture step type " << type << llendl;
				return FALSE;
			}
		}
	}
	return TRUE;
}

void LLMultiGesture::dump()
{
	llinfos << "key " << S32(mKey) << " mask " << U32(mMask) 
		<< " trigger " << mTrigger
		<< " replace " << mReplaceText
		<< llendl;
	U32 i;
	for (i = 0; i < mSteps.size(); ++i)
	{
		LLGestureStep* step = mSteps[i];
		step->dump();
	}
}

//---------------------------------------------------------------------------
// LLGestureStepAnimation
//---------------------------------------------------------------------------
LLGestureStepAnimation::LLGestureStepAnimation()
:	LLGestureStep(),
	mAnimName("None"),
	mAnimAssetID(),
	mFlags(0x0)
{ }

LLGestureStepAnimation::~LLGestureStepAnimation()
{ }

S32 LLGestureStepAnimation::getMaxSerialSize() const
{
	S32 max_size = 0;

	// ascii
	max_size += 256;	// anim name
	max_size += 64;		// anim asset id
	max_size += 64;		// flags

	/* binary
	max_size += mAnimName.length() + 1;
	max_size += sizeof(mAnimAssetID);
	max_size += sizeof(mFlags);
	*/
	return max_size;
}

BOOL LLGestureStepAnimation::serialize(LLDataPacker& dp) const
{
	dp.packString(mAnimName.c_str(), "anim_name");
	dp.packUUID(mAnimAssetID, "asset_id");
	dp.packU32(mFlags, "flags");
	return TRUE;
}

BOOL LLGestureStepAnimation::deserialize(LLDataPacker& dp)
{
	dp.unpackString(mAnimName, "anim_name");

	// Apparently an earlier version of the gesture code added \r to the end
	// of the animation names.  Get rid of it.  JC
	if (mAnimName[mAnimName.length() - 1] == '\r')
	{
		// chop the last character
		mAnimName.resize(mAnimName.length() - 1);
	}

	dp.unpackUUID(mAnimAssetID, "asset_id");
	dp.unpackU32(mFlags, "flags");
	return TRUE;
}

std::string LLGestureStepAnimation::getLabel() const
{
	std::string label;
	if (mFlags & ANIM_FLAG_STOP)
	{
		label = "Stop Animation: ";
	}
	else
	{
		label = "Start Animation: ";
	}
	label += mAnimName;
	return label;
}

void LLGestureStepAnimation::dump()
{
	llinfos << "step animation " << mAnimName
		<< " id " << mAnimAssetID
		<< " flags " << mFlags
		<< llendl;
}

//---------------------------------------------------------------------------
// LLGestureStepSound
//---------------------------------------------------------------------------
LLGestureStepSound::LLGestureStepSound()
:	LLGestureStep(),
	mSoundName("None"),
	mSoundAssetID(),
	mFlags(0x0)
{ }

LLGestureStepSound::~LLGestureStepSound()
{ }

S32 LLGestureStepSound::getMaxSerialSize() const
{
	S32 max_size = 0;
	max_size += 256;	// sound name
	max_size += 64;		// sound asset id
	max_size += 64;		// flags
	/* binary
	max_size += mSoundName.length() + 1;
	max_size += sizeof(mSoundAssetID);
	max_size += sizeof(mFlags);
	*/
	return max_size;
}

BOOL LLGestureStepSound::serialize(LLDataPacker& dp) const
{
	dp.packString(mSoundName.c_str(), "sound_name");
	dp.packUUID(mSoundAssetID, "asset_id");
	dp.packU32(mFlags, "flags");
	return TRUE;
}

BOOL LLGestureStepSound::deserialize(LLDataPacker& dp)
{
	dp.unpackString(mSoundName, "sound_name");

	dp.unpackUUID(mSoundAssetID, "asset_id");
	dp.unpackU32(mFlags, "flags");
	return TRUE;
}

std::string LLGestureStepSound::getLabel() const
{
	std::string label("Sound: ");
	label += mSoundName;
	return label;
}

void LLGestureStepSound::dump()
{
	llinfos << "step sound " << mSoundName
		<< " id " << mSoundAssetID
		<< " flags " << mFlags
		<< llendl;
}


//---------------------------------------------------------------------------
// LLGestureStepChat
//---------------------------------------------------------------------------
LLGestureStepChat::LLGestureStepChat()
:	LLGestureStep(),
	mChatText(),
	mFlags(0x0)
{ }

LLGestureStepChat::~LLGestureStepChat()
{ }

S32 LLGestureStepChat::getMaxSerialSize() const
{
	S32 max_size = 0;
	max_size += 256;	// chat text
	max_size += 64;		// flags
	/* binary
	max_size += mChatText.length() + 1;
	max_size += sizeof(mFlags);
	*/
	return max_size;
}

BOOL LLGestureStepChat::serialize(LLDataPacker& dp) const
{
	dp.packString(mChatText.c_str(), "chat_text");
	dp.packU32(mFlags, "flags");
	return TRUE;
}

BOOL LLGestureStepChat::deserialize(LLDataPacker& dp)
{
	dp.unpackString(mChatText, "chat_text");

	dp.unpackU32(mFlags, "flags");
	return TRUE;
}

std::string LLGestureStepChat::getLabel() const
{
	std::string label("Chat: ");
	label += mChatText;
	return label;
}

void LLGestureStepChat::dump()
{
	llinfos << "step chat " << mChatText
		<< " flags " << mFlags
		<< llendl;
}


//---------------------------------------------------------------------------
// LLGestureStepWait
//---------------------------------------------------------------------------
LLGestureStepWait::LLGestureStepWait()
:	LLGestureStep(),
	mWaitSeconds(0.f),
	mFlags(0x0)
{ }

LLGestureStepWait::~LLGestureStepWait()
{ }

S32 LLGestureStepWait::getMaxSerialSize() const
{
	S32 max_size = 0;
	max_size += 64;		// wait seconds
	max_size += 64;		// flags
	/* binary
	max_size += sizeof(mWaitSeconds);
	max_size += sizeof(mFlags);
	*/
	return max_size;
}

BOOL LLGestureStepWait::serialize(LLDataPacker& dp) const
{
	dp.packF32(mWaitSeconds, "wait_seconds");
	dp.packU32(mFlags, "flags");
	return TRUE;
}

BOOL LLGestureStepWait::deserialize(LLDataPacker& dp)
{
	dp.unpackF32(mWaitSeconds, "wait_seconds");
	dp.unpackU32(mFlags, "flags");
	return TRUE;
}

std::string LLGestureStepWait::getLabel() const
{
	std::string label("--- Wait: ");
	if (mFlags & WAIT_FLAG_TIME)
	{
		char buffer[64];		/* Flawfinder: ignore */
		snprintf(buffer, sizeof(buffer), "%.1f seconds", (double)mWaitSeconds);		/* Flawfinder: ignore */
		label += buffer;
	}
	else if (mFlags & WAIT_FLAG_ALL_ANIM)
	{
		label += "until animations are done";
	}

	return label;
}


void LLGestureStepWait::dump()
{
	llinfos << "step wait " << mWaitSeconds
		<< " flags " << mFlags
		<< llendl;
}
