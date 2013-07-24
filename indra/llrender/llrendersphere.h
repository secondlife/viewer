/** 
 * @file llrendersphere.h
 * @brief interface for the LLRenderSphere class.
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

#ifndef LL_LLRENDERSPHERE_H
#define LL_LLRENDERSPHERE_H

#include "llmath.h"
#include "v3math.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "v4color.h"
#include "llgl.h"

void lat2xyz(LLVector3 * result, F32 lat, F32 lon);			// utility routine

class LLRenderSphere  
{
public:
	void render();						// render at highest LOD
	void renderGGL();                   // render using LLRender

private:
	std::vector< std::vector<LLVector3> > mSpherePoints;
};

extern LLRenderSphere gSphere;
#endif
