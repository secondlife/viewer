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

inline LLVector3 polar_to_cart(F32 latitude, F32 longitude)
{
	return LLVector3(sin(F_TWO_PI * latitude) * cos(F_TWO_PI * longitude),
					 sin(F_TWO_PI * latitude) * sin(F_TWO_PI * longitude),
					 cos(F_TWO_PI * latitude));
}


void LLRenderSphere::renderGGL()
{
	S32 const LATITUDE_SLICES = 20;
	S32 const LONGITUDE_SLICES = 30;

	if (mSpherePoints.empty())
	{
		mSpherePoints.resize(LATITUDE_SLICES + 1);
		for (S32 lat_i = 0; lat_i < LATITUDE_SLICES + 1; lat_i++)
		{
			mSpherePoints[lat_i].resize(LONGITUDE_SLICES + 1);
			for (S32 lon_i = 0; lon_i < LONGITUDE_SLICES + 1; lon_i++)
			{
				F32 lat = (F32)lat_i / LATITUDE_SLICES;
				F32 lon = (F32)lon_i / LONGITUDE_SLICES;

				mSpherePoints[lat_i][lon_i] = polar_to_cart(lat, lon);
			}
		}
	}
	
	gGL.begin(LLRender::TRIANGLES);

	for (S32 lat_i = 0; lat_i < LATITUDE_SLICES; lat_i++)
	{
		for (S32 lon_i = 0; lon_i < LONGITUDE_SLICES; lon_i++)
		{
			gGL.vertex3fv(mSpherePoints[lat_i][lon_i].mV);
			gGL.vertex3fv(mSpherePoints[lat_i][lon_i+1].mV);
			gGL.vertex3fv(mSpherePoints[lat_i+1][lon_i].mV);

			gGL.vertex3fv(mSpherePoints[lat_i+1][lon_i].mV);
			gGL.vertex3fv(mSpherePoints[lat_i][lon_i+1].mV);
			gGL.vertex3fv(mSpherePoints[lat_i+1][lon_i+1].mV);
		}
	}
	gGL.end();
}
