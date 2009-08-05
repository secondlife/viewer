/** 
 * @file llsprite.h
 * @brief LLSprite class definition
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

#ifndef LL_LLSPRITE_H
#define LL_LLSPRITE_H

////#include "vmath.h"
//#include "llmath.h"
#include "v3math.h"
#include "v4math.h"
#include "v4color.h"
#include "lluuid.h"
#include "llgl.h"
#include "llviewertexture.h"

class LLViewerCamera;

class LLFace;

class LLSprite  
{
public:
	LLSprite(const LLUUID &image_uuid);
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
	void setNormal(const LLVector3 &normal)		{ sNormal = normal; sNormal.normalize();}

	F32 getAlpha();

	void updateFace(LLFace &face);

public:
	LLUUID mImageID;
  	LLPointer<LLViewerTexture> mImagep;
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

