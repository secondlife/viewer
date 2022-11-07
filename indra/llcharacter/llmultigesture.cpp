/** 
 * @file llmultigesture.cpp
 * @brief Gestures that are asset-based and can have multiple steps.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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
:   mKey(),
    mMask(),
    mName(),
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
    mSteps.clear();
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
    max_size += 64;     // version S32
    max_size += 64;     // key U8
    max_size += 64;     // mask U32
    max_size += 256;    // trigger string
    max_size += 256;    // replace string

    max_size += 64;     // step count S32

    std::vector<LLGestureStep*>::const_iterator it;
    for (it = mSteps.begin(); it != mSteps.end(); ++it)
    {
        LLGestureStep* step = *it;
        max_size += 64; // type S32
        max_size += step->getMaxSerialSize();
    }

    /* binary format
    max_size += sizeof(S32);    // version
    max_size += sizeof(mKey);
    max_size += sizeof(mMask);
    max_size += mTrigger.length() + 1;  // for null

    max_size += sizeof(S32);    // step count

    std::vector<LLGestureStep*>::const_iterator it;
    for (it = mSteps.begin(); it != mSteps.end(); ++it)
    {
        LLGestureStep* step = *it;
        max_size += sizeof(S32);    // type
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
    dp.packString(mTrigger, "trigger");
    dp.packString(mReplaceText, "replace");

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
        LL_WARNS() << "Bad LLMultiGesture version " << version
            << " should be " << GESTURE_VERSION
            << LL_ENDL;
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
        LL_WARNS() << "Bad LLMultiGesture step count " << count << LL_ENDL;
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
                LL_WARNS() << "Bad LLMultiGesture step type " << type << LL_ENDL;
                return FALSE;
            }
        }
    }
    return TRUE;
}

void LLMultiGesture::dump()
{
    LL_INFOS() << "key " << S32(mKey) << " mask " << U32(mMask) 
        << " trigger " << mTrigger
        << " replace " << mReplaceText
        << LL_ENDL;
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
:   LLGestureStep(),
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
    max_size += 256;    // anim name
    max_size += 64;     // anim asset id
    max_size += 64;     // flags

    /* binary
    max_size += mAnimName.length() + 1;
    max_size += sizeof(mAnimAssetID);
    max_size += sizeof(mFlags);
    */
    return max_size;
}

BOOL LLGestureStepAnimation::serialize(LLDataPacker& dp) const
{
    dp.packString(mAnimName, "anim_name");
    dp.packUUID(mAnimAssetID, "asset_id");
    dp.packU32(mFlags, "flags");
    return TRUE;
}

BOOL LLGestureStepAnimation::deserialize(LLDataPacker& dp)
{
    dp.unpackString(mAnimName, "anim_name");

    // Apparently an earlier version of the gesture code added \r to the end
    // of the animation names.  Get rid of it.  JC
    if (!mAnimName.empty() && mAnimName[mAnimName.length() - 1] == '\r')
    {
        // chop the last character
        mAnimName.resize(mAnimName.length() - 1);
    }

    dp.unpackUUID(mAnimAssetID, "asset_id");
    dp.unpackU32(mFlags, "flags");
    return TRUE;
}
// *NOTE: result is translated in LLPreviewGesture::getLabel()
std::vector<std::string> LLGestureStepAnimation::getLabel() const 
{
    std::vector<std::string> strings;
    
//  std::string label;
    if (mFlags & ANIM_FLAG_STOP)
    {
        strings.push_back( "AnimFlagStop");

//      label = "Stop Animation: ";
    }
    else
    {
        strings.push_back( "AnimFlagStart");

//      label = "Start Animation: "; 
    }
    strings.push_back( mAnimName);
//  label += mAnimName;
    return strings;
}

void LLGestureStepAnimation::dump()
{
    LL_INFOS() << "step animation " << mAnimName
        << " id " << mAnimAssetID
        << " flags " << mFlags
        << LL_ENDL;
}

//---------------------------------------------------------------------------
// LLGestureStepSound
//---------------------------------------------------------------------------
LLGestureStepSound::LLGestureStepSound()
:   LLGestureStep(),
    mSoundName("None"),
    mSoundAssetID(),
    mFlags(0x0)
{ }

LLGestureStepSound::~LLGestureStepSound()
{ }

S32 LLGestureStepSound::getMaxSerialSize() const
{
    S32 max_size = 0;
    max_size += 256;    // sound name
    max_size += 64;     // sound asset id
    max_size += 64;     // flags
    /* binary
    max_size += mSoundName.length() + 1;
    max_size += sizeof(mSoundAssetID);
    max_size += sizeof(mFlags);
    */
    return max_size;
}

BOOL LLGestureStepSound::serialize(LLDataPacker& dp) const
{
    dp.packString(mSoundName, "sound_name");
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
// *NOTE: result is translated in LLPreviewGesture::getLabel()
std::vector<std::string> LLGestureStepSound::getLabel() const
{
    std::vector<std::string> strings;
    strings.push_back( "Sound");
    strings.push_back( mSoundName); 
//  std::string label("Sound: ");
//  label += mSoundName;
    return strings;
}

void LLGestureStepSound::dump()
{
    LL_INFOS() << "step sound " << mSoundName
        << " id " << mSoundAssetID
        << " flags " << mFlags
        << LL_ENDL;
}


//---------------------------------------------------------------------------
// LLGestureStepChat
//---------------------------------------------------------------------------
LLGestureStepChat::LLGestureStepChat()
:   LLGestureStep(),
    mChatText(),
    mFlags(0x0)
{ }

LLGestureStepChat::~LLGestureStepChat()
{ }

S32 LLGestureStepChat::getMaxSerialSize() const
{
    S32 max_size = 0;
    max_size += 256;    // chat text
    max_size += 64;     // flags
    /* binary
    max_size += mChatText.length() + 1;
    max_size += sizeof(mFlags);
    */
    return max_size;
}

BOOL LLGestureStepChat::serialize(LLDataPacker& dp) const
{
    dp.packString(mChatText, "chat_text");
    dp.packU32(mFlags, "flags");
    return TRUE;
}

BOOL LLGestureStepChat::deserialize(LLDataPacker& dp)
{
    dp.unpackString(mChatText, "chat_text");

    dp.unpackU32(mFlags, "flags");
    return TRUE;
}
// *NOTE: result is translated in LLPreviewGesture::getLabel()
std::vector<std::string> LLGestureStepChat::getLabel() const
{
    std::vector<std::string> strings;
    strings.push_back("Chat");
    strings.push_back(mChatText);
    return strings;
}

void LLGestureStepChat::dump()
{
    LL_INFOS() << "step chat " << mChatText
        << " flags " << mFlags
        << LL_ENDL;
}


//---------------------------------------------------------------------------
// LLGestureStepWait
//---------------------------------------------------------------------------
LLGestureStepWait::LLGestureStepWait()
:   LLGestureStep(),
    mWaitSeconds(0.f),
    mFlags(0x0)
{ }

LLGestureStepWait::~LLGestureStepWait()
{ }

S32 LLGestureStepWait::getMaxSerialSize() const
{
    S32 max_size = 0;
    max_size += 64;     // wait seconds
    max_size += 64;     // flags
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
// *NOTE: result is translated in LLPreviewGesture::getLabel()
std::vector<std::string> LLGestureStepWait::getLabel() const
{
    std::vector<std::string> strings;
    strings.push_back( "Wait" );
    
//  std::string label("--- Wait: ");
    if (mFlags & WAIT_FLAG_TIME)
    {
        char buffer[64];        /* Flawfinder: ignore */
        snprintf(buffer, sizeof(buffer), "%.1f seconds", (double)mWaitSeconds); /* Flawfinder: ignore */
        strings.push_back(buffer);
//      label += buffer;
    }
    else if (mFlags & WAIT_FLAG_ALL_ANIM)
    {
        strings.push_back("until animations are done");
    //  label += "until animations are done";
    }
    else
    {
        strings.push_back("");
    }

    return strings;
}


void LLGestureStepWait::dump()
{
    LL_INFOS() << "step wait " << mWaitSeconds
        << " flags " << mFlags
        << LL_ENDL;
}
