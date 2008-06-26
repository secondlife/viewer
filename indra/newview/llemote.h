/** 
 * @file llemote.h
 * @brief Definition of LLEmote class
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

#ifndef LL_LLEMOTE_H
#define LL_LLEMOTE_H

//-----------------------------------------------------------------------------
// Header files
//-----------------------------------------------------------------------------
#include "llmotion.h"
#include "lltimer.h"

#define MIN_REQUIRED_PIXEL_AREA_EMOTE 2000.f

#define EMOTE_MORPH_FADEIN_TIME 0.3f
#define EMOTE_MORPH_IN_TIME 1.1f
#define EMOTE_MORPH_FADEOUT_TIME 1.4f

class LLVisualParam;

//-----------------------------------------------------------------------------
// class LLEmote
//-----------------------------------------------------------------------------
class LLEmote :
	public LLMotion
{
public:
	// Constructor
	LLEmote(const LLUUID &id);

	// Destructor
	virtual ~LLEmote();

public:
	//-------------------------------------------------------------------------
	// functions to support MotionController and MotionRegistry
	//-------------------------------------------------------------------------

	// static constructor
	// all subclasses must implement such a function and register it
	static LLMotion *create(const LLUUID &id) { return new LLEmote(id); }

public:
	//-------------------------------------------------------------------------
	// animation callbacks to be implemented by subclasses
	//-------------------------------------------------------------------------

	// motions must specify whether or not they loop
	virtual BOOL getLoop() { return FALSE; }

	// motions must report their total duration
	virtual F32 getDuration() { return EMOTE_MORPH_FADEIN_TIME + EMOTE_MORPH_IN_TIME + EMOTE_MORPH_FADEOUT_TIME; }

	// motions must report their "ease in" duration
	virtual F32 getEaseInDuration() { return EMOTE_MORPH_FADEIN_TIME; }

	// motions must report their "ease out" duration.
	virtual F32 getEaseOutDuration() { return EMOTE_MORPH_FADEOUT_TIME; }

	// called to determine when a motion should be activated/deactivated based on avatar pixel coverage
	virtual F32 getMinPixelArea() { return MIN_REQUIRED_PIXEL_AREA_EMOTE; }

	// motions must report their priority
	virtual LLJoint::JointPriority getPriority() { return LLJoint::MEDIUM_PRIORITY; }

	virtual LLMotionBlendType getBlendType() { return NORMAL_BLEND; }

	// run-time (post constructor) initialization,
	// called after parameters have been set
	// must return true to indicate success and be available for activation
	virtual LLMotionInitStatus onInitialize(LLCharacter *character);

	// called when a motion is activated
	// must return TRUE to indicate success, or else
	// it will be deactivated 
	virtual BOOL onActivate();

	// called per time step
	// must return TRUE while it is active, and
	// must return FALSE when the motion is completed.
	virtual BOOL onUpdate(F32 time, U8* joint_mask);

	// called when a motion is deactivated
	virtual void onDeactivate();

	virtual BOOL canDeprecate() { return FALSE; }

protected:

	LLCharacter*		mCharacter;

	LLVisualParam*		mParam;
};



#endif // LL_LLEMOTE_H


