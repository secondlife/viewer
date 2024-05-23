/**
 * @file llmultigesture.h
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

#ifndef LL_LLMULTIGESTURE_H
#define LL_LLMULTIGESTURE_H

#include <set>
#include <string>
#include <vector>

#include "lluuid.h"
#include "llframetimer.h"

class LLDataPacker;
class LLGestureStep;

class LLMultiGesture
{
public:
    LLMultiGesture();
    virtual ~LLMultiGesture();

    // Maximum number of bytes this could hold once serialized.
    S32 getMaxSerialSize() const;

    bool serialize(LLDataPacker& dp) const;
    bool deserialize(LLDataPacker& dp);

    void dump();

    void reset();

    const std::string& getTrigger() const { return mTrigger; }
protected:
    LLMultiGesture(const LLMultiGesture& gest);
    const LLMultiGesture& operator=(const LLMultiGesture& rhs);

public:
    KEY mKey { 0 };
    MASK mMask { 0 };

    // This name can be empty if the inventory item is not around and
    // the gesture manager has not yet set the name
    std::string mName;

    // String, like "/foo" or "hello" that makes it play
    std::string mTrigger;

    // Replaces the trigger substring with this text
    std::string mReplaceText;

    std::vector<LLGestureStep*> mSteps;

    // Is the gesture currently playing?
    bool mPlaying { false };

    // "instruction pointer" for steps
    S32 mCurrentStep { 0 };

    // We're waiting for triggered animations to stop playing
    bool mWaitingAnimations { false };

    // We're waiting for key release
    bool mWaitingKeyRelease { false };

    // We're waiting a fixed amount of time
    bool mWaitingTimer { false };

    // We're waiting for triggered animations to stop playing
    bool mTriggeredByKey { false };

    // Has the key been released?
    bool mKeyReleased { false };

    // Waiting after the last step played for all animations to complete
    bool mWaitingAtEnd { false };

    // Timer for waiting
    LLFrameTimer mWaitTimer;

    void (*mDoneCallback)(LLMultiGesture* gesture, void* data) { NULL };
    void* mCallbackData { NULL };

    // Animations that we requested to start
    std::set<LLUUID> mRequestedAnimIDs;

    // Once the animation starts playing (sim says to start playing)
    // the ID is moved from mRequestedAnimIDs to here.
    std::set<LLUUID> mPlayingAnimIDs;
};


// Order must match the library_list in floater_preview_gesture.xml!

enum EStepType
{
    STEP_ANIMATION = 0,
    STEP_SOUND = 1,
    STEP_CHAT = 2,
    STEP_WAIT = 3,

    STEP_EOF = 4
};


class LLGestureStep
{
public:
    LLGestureStep() {}
    virtual ~LLGestureStep() {}

    virtual EStepType getType() = 0;

    // Return a user-readable label for this step
    virtual std::vector<std::string> getLabel() const = 0;

    virtual S32 getMaxSerialSize() const = 0;
    virtual bool serialize(LLDataPacker& dp) const = 0;
    virtual bool deserialize(LLDataPacker& dp) = 0;

    virtual void dump() = 0;
};


// By default, animation steps start animations.
// If the least significant bit is 1, it will stop animations.
const U32 ANIM_FLAG_STOP = 0x01;

class LLGestureStepAnimation : public LLGestureStep
{
public:
    LLGestureStepAnimation();
    virtual ~LLGestureStepAnimation();

    virtual EStepType getType() { return STEP_ANIMATION; }

    virtual std::vector<std::string> getLabel() const;

    virtual S32 getMaxSerialSize() const;
    virtual bool serialize(LLDataPacker& dp) const;
    virtual bool deserialize(LLDataPacker& dp);

    virtual void dump();

public:
    std::string mAnimName;
    LLUUID mAnimAssetID;
    U32 mFlags;
};


class LLGestureStepSound : public LLGestureStep
{
public:
    LLGestureStepSound();
    virtual ~LLGestureStepSound();

    virtual EStepType getType() { return STEP_SOUND; }

    virtual std::vector<std::string> getLabel() const;

    virtual S32 getMaxSerialSize() const;
    virtual bool serialize(LLDataPacker& dp) const;
    virtual bool deserialize(LLDataPacker& dp);

    virtual void dump();

public:
    std::string mSoundName;
    LLUUID mSoundAssetID;
    U32 mFlags;
};


class LLGestureStepChat : public LLGestureStep
{
public:
    LLGestureStepChat();
    virtual ~LLGestureStepChat();

    virtual EStepType getType() { return STEP_CHAT; }

    virtual std::vector<std::string> getLabel() const;

    virtual S32 getMaxSerialSize() const;
    virtual bool serialize(LLDataPacker& dp) const;
    virtual bool deserialize(LLDataPacker& dp);

    virtual void dump();

public:
    std::string mChatText;
    U32 mFlags;
};


const U32 WAIT_FLAG_TIME        = 0x01;
const U32 WAIT_FLAG_ALL_ANIM    = 0x02;
const U32 WAIT_FLAG_KEY_RELEASE = 0x04;

class LLGestureStepWait : public LLGestureStep
{
public:
    LLGestureStepWait();
    virtual ~LLGestureStepWait();

    virtual EStepType getType() { return STEP_WAIT; }

    virtual std::vector<std::string> getLabel() const;

    virtual S32 getMaxSerialSize() const;
    virtual bool serialize(LLDataPacker& dp) const;
    virtual bool deserialize(LLDataPacker& dp);

    virtual void dump();

public:
    F32 mWaitSeconds;
    U32 mFlags;
};

#endif
