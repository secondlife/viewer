/** 
 * @file llcoord.h
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

#ifndef LL_LLCOORD_H
#define LL_LLCOORD_H

// A two-dimensional pixel value
class LLCoord
{
public:
	S32		mX;
	S32		mY;

	LLCoord():	mX(0), mY(0)
	{}
	LLCoord(S32 x, S32 y): mX(x), mY(y)
	{}
	virtual ~LLCoord()
	{}

	virtual void set(S32 x, S32 y)		{ mX = x; mY = y; }
};


// GL coordinates start in the client region of a window,
// with left, bottom = 0, 0
class LLCoordGL : public LLCoord
{
public:
	LLCoordGL() : LLCoord()
	{}
	LLCoordGL(S32 x, S32 y) : LLCoord(x, y)
	{}
};


// Window coords include things like window borders,
// menu regions, etc.
class LLCoordWindow : public LLCoord
{
public:
	LLCoordWindow() : LLCoord()
	{}
	LLCoordWindow(S32 x, S32 y) : LLCoord(x, y)
	{}
};


// Screen coords start at left, top = 0, 0
class LLCoordScreen : public LLCoord
{
public:
	LLCoordScreen() : LLCoord()
	{}
	LLCoordScreen(S32 x, S32 y) : LLCoord(x, y)
	{}
};

class LLCoordFont : public LLCoord
{
public:
	F32 mZ;
	
	LLCoordFont() : LLCoord(), mZ(0.f)
	{}
	LLCoordFont(S32 x, S32 y, F32 z = 0) : LLCoord(x,y), mZ(z)
	{}
	
	void set(S32 x, S32 y) { LLCoord::set(x,y); mZ = 0.f; }
	void set(S32 x, S32 y, F32 z) { mX = x; mY = y; mZ = z; }
};
	

#endif
