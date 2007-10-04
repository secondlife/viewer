/** 
 * @file llcylinder.h
 * @brief Draws a cylinder, and a cone, which is a special case cylinder
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
