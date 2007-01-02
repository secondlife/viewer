/** 
 * @file llcoord.h
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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
