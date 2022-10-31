/** 
 * @file llcubemap.h
 * @brief LLCubeMap class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLCUBEMAP_H
#define LL_LLCUBEMAP_H

#include "llgl.h"

#include <vector>

class LLVector3;

// Environment map hack!
class LLCubeMap : public LLRefCount
{
    bool mIssRGB;
public:
    LLCubeMap(bool init_as_srgb);
    void init(const std::vector<LLPointer<LLImageRaw> >& rawimages);
    void initGL();
    void initRawData(const std::vector<LLPointer<LLImageRaw> >& rawimages);
    void initGLData();

    void bind();
    void enable(S32 stage);
    
    void enableTexture(S32 stage);
    S32  getStage(void) { return mTextureStage; }
    
    void disable(void);
    void disableTexture(void);
    void setMatrix(S32 stage);
    void restoreMatrix();
    void setReflection (void);

    void finishPaint();

    GLuint getGLName();

    LLVector3 map(U8 side, U16 v_val, U16 h_val) const;
    BOOL project(F32& v_val, F32& h_val, BOOL& outside,
                        U8 side, const LLVector3& dir) const;
    BOOL project(F32& v_min, F32& v_max, F32& h_min, F32& h_max, 
                        U8 side, LLVector3 dir[4]) const;
    void paintIn(LLVector3 dir[4], const LLColor4U& col);
    void destroyGL();

public:
    static bool sUseCubeMaps;

protected:
    friend class LLTexUnit;
    ~LLCubeMap();
    LLGLenum mTargets[6];
    LLPointer<LLImageGL> mImages[6];
    LLPointer<LLImageRaw> mRawImages[6];
    S32 mTextureStage;
    S32 mMatrixStage;
};

#endif
