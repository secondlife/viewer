/** 
 * @file llsprite.h
 * @brief LLSprite class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLSPRITE_H
#define LL_LLSPRITE_H

////#include "vmath.h"
//#include "llmath.h"
#include "v3math.h"
#include "v4math.h"
#include "v4color.h"
#include "lluuid.h"
#include "llgl.h"
#include "llviewerimage.h"

class LLViewerCamera;

class LLFace;

class LLSprite  
{
public:
	LLSprite(const LLUUID &image_uuid);
	LLSprite(const LLUUID &image_uuid, const F32 width, const F32 height, const BOOL b_usemipmap = TRUE);
	~LLSprite();

	void render(LLViewerCamera * camerap);

	F32 getWidth()				const	{ return mWidth; } 
	F32 getHeight()				const	{ return mHeight; }
	F32 getYaw()				const	{ return mYaw; } 
	F32 getPitch()				const	{ return mPitch; }
	F32	getAlpha()				const	{ return mColor.mV[VALPHA]; }

	LLVector3 getPosition()		const	{ return mPosition; }
	LLColor4  getColor()		const	{ return mColor; }

	void setPosition(const LLVector3 &position);
	void setPitch(const F32 pitch);
	void setSize(const F32 width, const F32 height);
	void setYaw(const F32 yaw);
	void setFollow(const BOOL follow);
	void setUseCameraUp(const BOOL use_up);

	void setTexMode(LLGLenum mode);
	void setColor(const LLColor4 &color);
	void setColor(const F32 r, const F32 g, const F32 b, const F32 a);
	void setAlpha(const F32 alpha)					{ mColor.mV[VALPHA] = alpha; }
	void setNormal(const LLVector3 &normal)		{ sNormal = normal; sNormal.normVec();}

	F32 getAlpha();

	void updateFace(LLFace &face);

public:
	LLUUID mImageID;
  	LLPointer<LLViewerImage> mImagep;
private:
	F32 mWidth;
	F32 mHeight;
	F32 mWidthDiv2;
	F32 mHeightDiv2;
	F32 mPitch;
	F32 mYaw;
	LLVector3 mPosition;
	BOOL mFollow;
	BOOL mUseCameraUp;

	LLColor4 mColor;
	LLGLenum mTexMode;

	// put 
	LLVector3 mScaledUp;
	LLVector3 mScaledRight;
	static LLVector3 sCameraUp;
	static LLVector3 sCameraRight;
	static LLVector3 sCameraPosition;
	static LLVector3 sNormal;
	LLVector3 mA,mB,mC,mD;   // the four corners of a quad

};

#endif

