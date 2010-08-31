/** 
 * @file llcylinder.cpp
 * @brief Draws a cylinder using display lists for speed.
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

#include "llviewerprecompiledheaders.h"

#include "llcylinder.h"

#include "llerror.h"
#include "math.h"
#include "llmath.h"
#include "noise.h"
#include "v3math.h"
#include "llvertexbuffer.h"
#include "llgl.h"
#include "llglheaders.h"

LLCylinder	gCylinder;
LLCone		gCone;

GLUquadricObj* gQuadObj = NULL;

static const GLint SLICES[] = { 30, 20, 12, 6 };		// same as sphere slices
static const GLint STACKS = 2;
static const GLfloat RADIUS = 0.5f;
	
// draws a cylinder or cone
// returns approximate number of triangles required
U32 draw_cylinder_side(GLint slices, GLint stacks, GLfloat base_radius, GLfloat top_radius)
{
	U32 triangles = 0;
	GLfloat height = 1.0f;

	if (!gQuadObj)
	{
		gQuadObj = gluNewQuadric();
		if (!gQuadObj) llerror("draw_cylindrical_body couldn't allocated quadric", 0);
	}

	gluQuadricDrawStyle(gQuadObj, GLU_FILL);
	gluQuadricNormals(gQuadObj, GLU_SMOOTH);
	gluQuadricOrientation(gQuadObj, GLU_OUTSIDE);
	gluQuadricTexture(gQuadObj, GL_TRUE);
	gluCylinder(gQuadObj, base_radius, top_radius, height, slices, stacks);
	triangles += stacks * (slices * 2);
	

	return triangles;
}


// Returns number of triangles required to draw
// Need to know if top or not to set lighting normals
const BOOL TOP = TRUE;
const BOOL BOTTOM = FALSE;
U32 draw_cylinder_cap(GLint slices, GLfloat base_radius, BOOL is_top)
{
	U32 triangles = 0;

	if (!gQuadObj)
	{
		gQuadObj = gluNewQuadric();
		if (!gQuadObj) llerror("draw_cylinder_base couldn't allocated quadric", 0);
	}

	gluQuadricDrawStyle(gQuadObj, GLU_FILL);
	gluQuadricNormals(gQuadObj, GLU_SMOOTH);
	gluQuadricOrientation(gQuadObj, GLU_OUTSIDE);
	gluQuadricTexture(gQuadObj, GL_TRUE);

	// no hole in the middle of the disk, and just one ring
	GLdouble inner_radius = 0.0;
	GLint rings = 1;

	// normals point in +z for top, -z for base
	if (is_top)
	{
		gluQuadricOrientation(gQuadObj, GLU_OUTSIDE);
	}
	else
	{
		gluQuadricOrientation(gQuadObj, GLU_INSIDE);
	}
	gluDisk(gQuadObj, inner_radius, base_radius, slices, rings);
	triangles += slices;

	return triangles;
}

void LLCylinder::drawSide(S32 detail)
{
	draw_cylinder_side(SLICES[detail], STACKS, RADIUS, RADIUS);
}

void LLCylinder::drawTop(S32 detail)
{
	draw_cylinder_cap(SLICES[detail], RADIUS, TOP);
}

void LLCylinder::drawBottom(S32 detail)
{
	draw_cylinder_cap(SLICES[detail], RADIUS, BOTTOM);
}

void LLCylinder::prerender()
{
}

void LLCylinder::cleanupGL()
{
	if (gQuadObj)
	{
		gluDeleteQuadric(gQuadObj);
		gQuadObj = NULL;
	}
}

void LLCylinder::render(F32 pixel_area)
{
	renderface(pixel_area, 0);
	renderface(pixel_area, 1);
	renderface(pixel_area, 2);
}


void LLCylinder::renderface(F32 pixel_area, S32 face)
{
	if (face < 0 || face > 2)
	{
		llerror("LLCylinder::renderface() invalid face number", face);
		return;
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	S32 level_of_detail;

	if (pixel_area > 20000.f)
	{
		level_of_detail = 0;
	}
	else if (pixel_area > 1600.f)
	{
		level_of_detail = 1;
	}
	else if (pixel_area > 200.f)
	{
		level_of_detail = 2;
	}
	else
	{
		level_of_detail = 3;
	}

	if (level_of_detail < 0 || CYLINDER_LEVELS_OF_DETAIL <= level_of_detail)
	{
		llerror("LLCylinder::renderface() invalid level of detail", level_of_detail);
		return;
	}

	LLVertexBuffer::unbind();
	
	switch(face)
	{
	case 0:
		glTranslatef(0.f, 0.f, -0.5f);
		drawSide(level_of_detail);
		break;
	case 1:
		glTranslatef(0.0f, 0.f, 0.5f);
		drawTop(level_of_detail);
		break;
	case 2:
		glTranslatef(0.0f, 0.f, -0.5f);
		drawBottom(level_of_detail);
		break;
	default:
		llerror("LLCylinder::renderface() fell out of switch", 0);
		break;
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


//
// Cones
//

void LLCone::prerender()
{
}

void LLCone::cleanupGL()
{
	if (gQuadObj)
	{
		gluDeleteQuadric(gQuadObj);
		gQuadObj = NULL;
	}
}

void LLCone::drawSide(S32 detail)
{
	draw_cylinder_side( SLICES[detail], STACKS, RADIUS, 0.f );	
}

void LLCone::drawBottom(S32 detail)
{
	draw_cylinder_cap( SLICES[detail], RADIUS, BOTTOM );
}

void LLCone::render(S32 level_of_detail)
{
	GLfloat height = 1.0f;

	if (level_of_detail < 0 || CONE_LEVELS_OF_DETAIL <= level_of_detail)
	{
		llerror("LLCone::render() invalid level of detail", level_of_detail);
		return;
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	// center object at 0
	glTranslatef(0.f, 0.f, - height / 2.0f);

	drawSide(level_of_detail);
	drawBottom(level_of_detail);

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}


void LLCone::renderface(S32 level_of_detail, S32 face)
{
	if (face < 0 || face > 1)
	{
		llerror("LLCone::renderface() invalid face number", face);
		return;
	}

	if (level_of_detail < 0 || CONE_LEVELS_OF_DETAIL <= level_of_detail)
	{
		llerror("LLCone::renderface() invalid level of detail", level_of_detail);
		return;
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	LLVertexBuffer::unbind();
	
	switch(face)
	{
	case 0:
		glTranslatef(0.f, 0.f, -0.5f);
		drawSide(level_of_detail);
		break;
	case 1:
		glTranslatef(0.f, 0.f, -0.5f);
		drawBottom(level_of_detail);
		break;
	default:
		llerror("LLCylinder::renderface() fell out of switch", 0);
		break;
	}

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}
