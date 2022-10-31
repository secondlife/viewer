/** 
 * @file llvowlsky.h
 * @brief LLVOWLSky class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#ifndef LL_VOWLSKY_H
#define LL_VOWLSKY_H

#include "llviewerobject.h"

class LLVOWLSky : public LLStaticViewerObject {
private:
    inline static U32 getNumStacks(void);
    inline static U32 getNumSlices(void);
    inline static U32 getStripsNumVerts(void);
    inline static U32 getStripsNumIndices(void);
    inline static U32 getStarsNumVerts(void);
    inline static U32 getStarsNumIndices(void);

public:
    LLVOWLSky(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

    /*virtual*/ void         idleUpdate(LLAgent &agent, const F64 &time);
    /*virtual*/ BOOL         isActive(void) const;
    /*virtual*/ LLDrawable * createDrawable(LLPipeline *pipeline);
    /*virtual*/ BOOL         updateGeometry(LLDrawable *drawable);

    void drawStars(void);
    void drawDome(void);
    void drawFsSky(void); // fullscreen sky for advanced atmo
    void resetVertexBuffers(void);
    
    void cleanupGL();
    void restoreGL();

private:

    // helper function for initializing the stars.
    void initStars();

    // helper function for building the strips vertex buffer.
    // note begin_stack and end_stack follow stl iterator conventions,
    // begin_stack is the first stack to be included, end_stack is the first
    // stack not to be included.
    static void buildStripsBuffer(U32 begin_stack, U32 end_stack,
                                  LLStrider<LLVector3> & vertices,
                                  LLStrider<LLVector2> & texCoords,
                                  LLStrider<U16> & indices,
                                  const F32 RADIUS,
                                  const U32& num_slices,
                                  const U32& num_stacks);

    // helper function for updating the stars colors.
    void updateStarColors();

    // helper function for updating the stars geometry.
    BOOL updateStarGeometry(LLDrawable *drawable);

private:
    LLPointer<LLVertexBuffer>                   mFsSkyVerts;
    std::vector< LLPointer<LLVertexBuffer> >    mStripsVerts;
    LLPointer<LLVertexBuffer>                   mStarsVerts;

    std::vector<LLVector3>  mStarVertices;              // Star verticies
    std::vector<LLColor4>   mStarColors;                // Star colors
    std::vector<F32>        mStarIntensities;           // Star intensities
};

#endif // LL_VOWLSKY_H
