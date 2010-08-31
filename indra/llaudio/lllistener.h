/** 
 * @file listener.h
 * @brief Description of LISTENER base class abstracting the audio support.
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#ifndef LL_LISTENER_H
#define LL_LISTENER_H

#include "v3math.h"

class LLListener 
{
 private:
 protected:
	LLVector3 mPosition;
	LLVector3 mVelocity;
	LLVector3 mListenAt;
	LLVector3 mListenUp;

 public:

 private:
 protected:
 public:  
	LLListener();
	virtual ~LLListener();
	virtual void init();  

	virtual void set(LLVector3 pos, LLVector3 vel, LLVector3 up, LLVector3 at);

	virtual void setPosition(LLVector3 pos);
	virtual void setVelocity(LLVector3 vel);

	virtual void orient(LLVector3 up, LLVector3 at);
	virtual void translate(LLVector3 offset);

	virtual void setDopplerFactor(F32 factor);
	virtual void setRolloffFactor(F32 factor);

	virtual LLVector3 getPosition();
	virtual LLVector3 getAt();
	virtual LLVector3 getUp();

	virtual F32 getDopplerFactor();
	virtual F32 getRolloffFactor();

	virtual void commitDeferredChanges();
};

#endif

