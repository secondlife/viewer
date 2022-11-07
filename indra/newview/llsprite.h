/** 
 * @file llsprite.h
 * @brief LLSprite class definition
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

    F32 getWidth()              const   { return mWidth; } 
    F32 getHeight()             const   { return mHeight; }
    F32 getYaw()                const   { return mYaw; } 
    F32 getPitch()              const   { return mPitch; }
    F32 getAlpha()              const   { return mColor.mV[VALPHA]; }

    LLVector3 getPosition()     const   { return mPosition; }
    LLColor4  getColor()        const   { return mColor; }

    void setPosition(const LLVector3 &position);
    void setPitch(const F32 pitch);
    void setSize(const F32 width, const F32 height);
    void setYaw(const F32 yaw);
    void setFollow(const BOOL follow);
    void setUseCameraUp(const BOOL use_up);

    void setTexMode(LLGLenum mode);
    void setColor(const LLColor4 &color);
    void setColor(const F32 r, const F32 g, const F32 b, const F32 a);
    void setAlpha(const F32 alpha)                  { mColor.mV[VALPHA] = alpha; }
    void setNormal(const LLVector3 &normal)     { sNormal = normal; sNormal.normalize();}

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

