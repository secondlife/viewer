/** 
 * @file llgesture.h
 * @brief A gesture is a combination of a triggering chat phrase or
 * key, a sound, an animation, and a chat string.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLGESTURE_H
#define LL_LLGESTURE_H

#include "llanimationstates.h"
#include "lluuid.h"
#include "llstring.h"
#include "lldarray.h"

class LLGesture
{
public:
	LLGesture();
	LLGesture(KEY key, MASK mask, const std::string &trigger, 
		const LLUUID &sound_item_id, const std::string &animation, 
		const std::string &output_string);

	LLGesture(U8 **buffer, S32 max_size); // deserializes, advances buffer
	LLGesture(const LLGesture &gesture);
	const LLGesture &operator=(const LLGesture &rhs);

	virtual ~LLGesture() {};

	// Accessors
	KEY					getKey() const			{ return mKey; }
	MASK				getMask() const			{ return mMask; }
	const std::string&	getTrigger() const		{ return mTrigger; }
	const LLUUID&		getSound() const		{ return mSoundItemID; }
	const std::string&	getAnimation() const	{ return mAnimation; }
	const std::string&	getOutputString() const	{ return mOutputString; }

	// Triggers if a key/mask matches it
	virtual BOOL trigger(KEY key, MASK mask);

	// Triggers if case-insensitive substring matches (assumes string is lowercase)
	virtual BOOL trigger(const std::string &string);

	// non-endian-neutral serialization
	U8 *serialize(U8 *buffer) const;
	U8 *deserialize(U8 *buffer, S32 max_size);
	static S32 getMaxSerialSize();

protected:
	KEY					mKey;			// usually a function key
	MASK				mMask;			// usually MASK_NONE, or MASK_SHIFT
	std::string		mTrigger;		// string, no whitespace allowed
	std::string		mTriggerLower;	// lowercase version of mTrigger
	LLUUID				mSoundItemID;	// ItemID of sound to play, LLUUID::null if none
	std::string			mAnimation;		// canonical name of animation or face animation
	std::string		mOutputString;	// string to say

	static const S32	MAX_SERIAL_SIZE;
};

class LLGestureList
{
public:
	LLGestureList();
	virtual ~LLGestureList();

	// Triggers if a key/mask matches one in the list
	BOOL trigger(KEY key, MASK mask);

	// Triggers if substring matches and generates revised string.
	BOOL triggerAndReviseString(const std::string &string, std::string* revised_string);

	// Used for construction from UI
	S32 count() const						{ return mList.count(); }
	virtual LLGesture* get(S32 i) const		{ return mList.get(i); }
	virtual void put(LLGesture* gesture)	{ mList.put( gesture ); }
	void deleteAll();

	// non-endian-neutral serialization
	U8 *serialize(U8 *buffer) const;
	U8 *deserialize(U8 *buffer, S32 max_size);
	S32 getMaxSerialSize();

protected:
	// overridden by child class to use local LLGesture implementation
	virtual LLGesture *create_gesture(U8 **buffer, S32 max_size);

protected:
	LLDynamicArray<LLGesture*>	mList;

	static const S32	SERIAL_HEADER_SIZE;
};

#endif
