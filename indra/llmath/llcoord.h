/** 
 * @file llcoord.h
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
	bool operator==(const LLCoordGL& other) const { return mX == other.mX && mY == other.mY; }
	bool operator!=(const LLCoordGL& other) const { return !(*this == other); }
};

//bool operator ==(const LLCoordGL& a, const LLCoordGL& b);

// Window coords include things like window borders,
// menu regions, etc.
class LLCoordWindow : public LLCoord
{
public:
	LLCoordWindow() : LLCoord()
	{}
	LLCoordWindow(S32 x, S32 y) : LLCoord(x, y)
	{}
	bool operator==(const LLCoordWindow& other) const { return mX == other.mX && mY == other.mY; }
	bool operator!=(const LLCoordWindow& other) const { return !(*this == other); }
};


// Screen coords start at left, top = 0, 0
class LLCoordScreen : public LLCoord
{
public:
	LLCoordScreen() : LLCoord()
	{}
	LLCoordScreen(S32 x, S32 y) : LLCoord(x, y)
	{}
	bool operator==(const LLCoordScreen& other) const { return mX == other.mX && mY == other.mY; }
	bool operator!=(const LLCoordScreen& other) const { return !(*this == other); }
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
	bool operator==(const LLCoordFont& other) const { return mX == other.mX && mY == other.mY; }
	bool operator!=(const LLCoordFont& other) const { return !(*this == other); }
};
	

#endif
