/** 
 * @file llmultigesture.h
 * @brief Gestures that are asset-based and can have multiple steps.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

	BOOL serialize(LLDataPacker& dp) const;
	BOOL deserialize(LLDataPacker& dp);

	void dump();

	void reset();

	const std::string& getTrigger() const { return mTrigger; }
protected:
	LLMultiGesture(const LLMultiGesture& gest);
	const LLMultiGesture& operator=(const LLMultiGesture& rhs);

public:
	KEY mKey;
	MASK mMask;

	// This name can be empty if the inventory item is not around and
    // the gesture manager has not yet set the name
	std::string mName;

	// String, like "/foo" or "hello" that makes it play
	std::string mTrigger;

	// Replaces the trigger substring with this text
	std::string mReplaceText;

	std::vector<LLGestureStep*> mSteps;

	// Is the gesture currently playing?
	BOOL mPlaying;

	// "instruction pointer" for steps
	S32 mCurrentStep;

	// We're waiting for triggered animations to stop playing
	BOOL mWaitingAnimations;

	// We're waiting a fixed amount of time
	BOOL mWaitingTimer;

	// Waiting after the last step played for all animations to complete
	BOOL mWaitingAtEnd;

	// Timer for waiting
	LLFrameTimer mWaitTimer;

	void (*mDoneCallback)(LLMultiGesture* gesture, void* data);
	void* mCallbackData;

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
	virtual BOOL serialize(LLDataPacker& dp) const = 0;
	virtual BOOL deserialize(LLDataPacker& dp) = 0;

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
	virtual BOOL serialize(LLDataPacker& dp) const;
	virtual BOOL deserialize(LLDataPacker& dp);

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
	virtual BOOL serialize(LLDataPacker& dp) const;
	virtual BOOL deserialize(LLDataPacker& dp);

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
	virtual BOOL serialize(LLDataPacker& dp) const;
	virtual BOOL deserialize(LLDataPacker& dp);

	virtual void dump();

public:
	std::string mChatText;
	U32 mFlags;
};


const U32 WAIT_FLAG_TIME		= 0x01;
const U32 WAIT_FLAG_ALL_ANIM	= 0x02;

class LLGestureStepWait : public LLGestureStep
{
public:
	LLGestureStepWait();
	virtual ~LLGestureStepWait();

	virtual EStepType getType() { return STEP_WAIT; }

	virtual std::vector<std::string> getLabel() const;

	virtual S32 getMaxSerialSize() const;
	virtual BOOL serialize(LLDataPacker& dp) const;
	virtual BOOL deserialize(LLDataPacker& dp);

	virtual void dump();

public:
	F32 mWaitSeconds;
	U32 mFlags;
};

#endif
