/** 
 * @file llrendersphere.cpp
 * @brief implementation of the LLRenderSphere class.
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

//	Sphere creates a set of display lists that can then be called to create 
//	a lit sphere at different LOD levels.  You only need one instance of sphere 
//	per viewer - then call the appropriate list.  

#include "linden_common.h"

#include "llrendersphere.h"
#include "llerror.h"

#include "llglheaders.h"

GLUquadricObj *gQuadObj2 = NULL;
LLRenderSphere gSphere;

void drawSolidSphere(GLdouble radius, GLint slices, GLint stacks);

void drawSolidSphere(GLdouble radius, GLint slices, GLint stacks)
{
	if (!gQuadObj2)
	{
		gQuadObj2 = gluNewQuadric();
		if (!gQuadObj2)
		{
			llwarns << "drawSolidSphere couldn't allocate quadric" << llendl;
			return;
		}
	}

	gluQuadricDrawStyle(gQuadObj2, GLU_FILL);
	gluQuadricNormals(gQuadObj2, GLU_SMOOTH);
	// If we ever changed/used the texture or orientation state
	// of quadObj, we'd need to change it to the defaults here
	// with gluQuadricTexture and/or gluQuadricOrientation.
	gluQuadricTexture(gQuadObj2, GL_TRUE);
	gluSphere(gQuadObj2, radius, slices, stacks);
}


// lat = 0 is Z-axis
// lon = 0, lat = 90 at X-axis
void lat2xyz(LLVector3 * result, F32 lat, F32 lon)
{
	// Convert a latitude and longitude to x,y,z on a normal sphere and return it in result
	F32 r;
	result->mV[VX] = (F32) (cos(lon * DEG_TO_RAD) * sin(lat * DEG_TO_RAD));
	result->mV[VY] = (F32) (sin(lon * DEG_TO_RAD) * sin(lat * DEG_TO_RAD));
	r = (F32) pow(result->mV[VX] * result->mV[VX] + result->mV[VY] * result->mV[VY], 0.5f);
	if (r == 1.0f) 
	{
		result->mV[VZ] = 0.0f;
	}
	else
	{
		result->mV[VZ] = (F32) pow(1 - r*r, 0.5f);
		if (lat > 90.01)
		{
			result->mV[VZ] *= -1.0;
		}
	}
}

void lat2xyz_rad(LLVector3 * result, F32 lat, F32 lon)
{
	// Convert a latitude and longitude to x,y,z on a normal sphere and return it in result
	F32 r;
	result->mV[VX] = (F32) (cos(lon) * sin(lat));
	result->mV[VY] = (F32) (sin(lon) * sin(lat));
	r = (F32) pow(result->mV[VX] * result->mV[VX] + result->mV[VY] * result->mV[VY], 0.5f);
	if (r == 1.0f) 
		result->mV[VZ] = 0.0f;
	else
	{
		result->mV[VZ] = (F32) pow(1 - r*r, 0.5f);
		if (lat > F_PI_BY_TWO) result->mV[VZ] *= -1.0;
	}
}

// A couple thoughts on sphere drawing:
// 1) You need more slices than stacks, but little less than 2:1
// 2) At low LOD, setting stacks to an odd number avoids a "band" around the equator, making things look smoother
void LLRenderSphere::prerender()
{
	//  Create a series of display lists for different LODs
	mDList[0] = glGenLists(1);
	glNewList(mDList[0], GL_COMPILE);
	drawSolidSphere(1.0, 30, 20);
	glEndList();

	mDList[1] = glGenLists(1);
	glNewList(mDList[1], GL_COMPILE);
	drawSolidSphere(1.0, 20, 15);
	glEndList();

	mDList[2] = glGenLists(1);
	glNewList(mDList[2], GL_COMPILE);
	drawSolidSphere(1.0, 12, 8);
	glEndList();

	mDList[3] = glGenLists(1);
	glNewList(mDList[3], GL_COMPILE);
	drawSolidSphere(1.0, 8, 5);
	glEndList();
}

void LLRenderSphere::cleanupGL()
{
	for (S32 detail = 0; detail < 4; detail++)
	{
		glDeleteLists(mDList[detail], 1);
		mDList[detail] = 0;
	}
	
	if (gQuadObj2)
	{
		gluDeleteQuadric(gQuadObj2);
		gQuadObj2 = NULL;
	}
}

// Constants here are empirically derived from my eyeballs, JNC
//
// The toughest adjustment is the cutoff for the lowest LOD
// Maybe we should have more LODs at the low end?
void LLRenderSphere::render(F32 pixel_area)
{
	S32 level_of_detail;

	if (pixel_area > 10000.f)
	{
		level_of_detail = 0;
	}
	else if (pixel_area > 800.f)
	{
		level_of_detail = 1;
	}
	else if (pixel_area > 100.f)
	{
		level_of_detail = 2;
	}
	else
	{
		level_of_detail = 3;
	}
	glCallList(mDList[level_of_detail]);
}


void LLRenderSphere::render()
{
	glCallList(mDList[0]);
}
