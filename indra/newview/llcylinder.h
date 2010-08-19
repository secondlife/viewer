/** 
 * @file llcylinder.h
 * @brief Draws a cylinder, and a cone, which is a special case cylinder
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

#ifndef LL_LLCYLINDER_H
#define LL_LLCYLINDER_H

//#include "stdtypes.h"
//#include "llgl.h"

//
// Cylinders
//
const S32 CYLINDER_LEVELS_OF_DETAIL = 4;
const S32 CYLINDER_FACES = 3;

class LLCylinder
{
public:
	void prerender();
	void drawTop(S32 detail);
	void drawSide(S32 detail);
	void drawBottom(S32 detail);
	void cleanupGL();

	void render(F32 pixel_area);
	void renderface(F32 pixel_area, S32 face);
};


//
// Cones
//

const S32 CONE_LOD_HIGHEST = 0;
const S32 CONE_LEVELS_OF_DETAIL = 4;
const S32 CONE_FACES = 2;

class LLCone
{	
public:
	void prerender();
	void cleanupGL();
	void drawSide(S32 detail);
	void drawBottom(S32 detail);
	void render(S32 level_of_detail);
	void renderface(S32 level_of_detail, S32 face);
};

extern LLCylinder gCylinder;
extern LLCone gCone;
#endif
