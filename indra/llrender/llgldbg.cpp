/** 
 * @file llgldbg.cpp
 * @brief Definitions for OpenGL debugging support
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

// This file sets some global GL parameters, and implements some 
// useful functions for GL operations.

#include "linden_common.h"

#include "llgldbg.h"

#include "llgl.h"
#include "llglheaders.h"


//------------------------------------------------------------------------
// cmstr()
//------------------------------------------------------------------------
const char *cmstr(int i)
{
	switch( i )
	{
	case GL_EMISSION: return "GL_EMISSION";
	case GL_AMBIENT: return "GL_AMBIENT";
	case GL_DIFFUSE: return "GL_DIFFUSE";
	case GL_SPECULAR: return "GL_SPECULAR";
	case GL_AMBIENT_AND_DIFFUSE: return "GL_AMBIENT_AND_DIFFUSE";
	}
	return "UNKNOWN";
}

//------------------------------------------------------------------------
// facestr()
//------------------------------------------------------------------------
const char *facestr(int i)
{
	switch( i )
	{
	case GL_FRONT: return "GL_FRONT";
	case GL_BACK: return "GL_BACK";
	case GL_FRONT_AND_BACK: return "GL_FRONT_AND_BACK";
	}
	return "UNKNOWN";
}

//------------------------------------------------------------------------
// boolstr()
//------------------------------------------------------------------------
const char *boolstr(int b)
{
	return b ? "GL_TRUE" : "GL_FALSE";
}

//------------------------------------------------------------------------
// fv4()
//------------------------------------------------------------------------
const char *fv4(F32 *f)
{
	static char str[128];
	sprintf(str, "%8.3f %8.3f %8.3f %8.3f", f[0], f[1], f[2], f[3]);
	return str;
}

//------------------------------------------------------------------------
// fv3()
//------------------------------------------------------------------------
const char *fv3(F32 *f)
{
	static char str[128];	/* Flawfinder: ignore */
	snprintf(str, sizeof(str), "%8.3f, %8.3f, %8.3f", f[0], f[1], f[2]);	/* Flawfinder: ignore */
	return str;
}

//------------------------------------------------------------------------
// fv1()
//------------------------------------------------------------------------
const char *fv1(F32 *f)
{
	static char str[128];	/* Flawfinder: ignore */
	snprintf(str, sizeof(str), "%8.3f", f[0]);		/* Flawfinder: ignore */
	return str;
}

//------------------------------------------------------------------------
// llgl_dump()
//------------------------------------------------------------------------
void llgl_dump()
{
	int i;
	F32 fv[16];
	GLboolean b;

	llinfos << "==========================" << llendl;
	llinfos << "OpenGL State" << llendl;
	llinfos << "==========================" << llendl;

	llinfos << "-----------------------------------" << llendl;
	llinfos << "Current Values" << llendl;
	llinfos << "-----------------------------------" << llendl;

	glGetFloatv(GL_CURRENT_COLOR, fv);
	llinfos << "GL_CURRENT_COLOR          : " << fv4(fv) << llendl;

	glGetFloatv(GL_CURRENT_NORMAL, fv);
	llinfos << "GL_CURRENT_NORMAL          : " << fv3(fv) << llendl;

	llinfos << "-----------------------------------" << llendl;
	llinfos << "Lighting" << llendl;
	llinfos << "-----------------------------------" << llendl;

	llinfos << "GL_LIGHTING                : " << boolstr(glIsEnabled(GL_LIGHTING)) << llendl;

	llinfos << "GL_COLOR_MATERIAL          : " << boolstr(glIsEnabled(GL_COLOR_MATERIAL)) << llendl;

	glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, (GLint*)&i);
	llinfos << "GL_COLOR_MATERIAL_PARAMETER: " << cmstr(i) << llendl;

	glGetIntegerv(GL_COLOR_MATERIAL_FACE, (GLint*)&i);
	llinfos << "GL_COLOR_MATERIAL_FACE     : " << facestr(i) << llendl;

	fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
	glGetMaterialfv(GL_FRONT, GL_AMBIENT, fv);
	llinfos << "GL_AMBIENT material        : " << fv4(fv) << llendl;

	fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
	glGetMaterialfv(GL_FRONT, GL_DIFFUSE, fv);
	llinfos << "GL_DIFFUSE material        : " << fv4(fv) << llendl;

	fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
	glGetMaterialfv(GL_FRONT, GL_SPECULAR, fv);
	llinfos << "GL_SPECULAR material       : " << fv4(fv) << llendl;

	fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
	glGetMaterialfv(GL_FRONT, GL_EMISSION, fv);
	llinfos << "GL_EMISSION material       : " << fv4(fv) << llendl;

	fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
	glGetMaterialfv(GL_FRONT, GL_SHININESS, fv);
	llinfos << "GL_SHININESS material      : " << fv1(fv) << llendl;

	fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
	glGetFloatv(GL_LIGHT_MODEL_AMBIENT, fv);
	llinfos << "GL_LIGHT_MODEL_AMBIENT     : " << fv4(fv) << llendl;

	glGetBooleanv(GL_LIGHT_MODEL_LOCAL_VIEWER, &b);
	llinfos << "GL_LIGHT_MODEL_LOCAL_VIEWER: " << boolstr(b) << llendl;

	glGetBooleanv(GL_LIGHT_MODEL_TWO_SIDE, &b);
	llinfos << "GL_LIGHT_MODEL_TWO_SIDE    : " << boolstr(b) << llendl;

	for (int l=0; l<8; l++)
	{
	b = glIsEnabled(GL_LIGHT0+l);
	llinfos << "GL_LIGHT" << l << "                  : " << boolstr(b) << llendl;

	if (!b)
		continue;

	glGetLightfv(GL_LIGHT0+l, GL_AMBIENT, fv);
	llinfos << "  GL_AMBIENT light         : " << fv4(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_DIFFUSE, fv);
	llinfos << "  GL_DIFFUSE light         : " << fv4(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_SPECULAR, fv);
	llinfos << "  GL_SPECULAR light        : " << fv4(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_POSITION, fv);
	llinfos << "  GL_POSITION light        : " << fv4(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_CONSTANT_ATTENUATION, fv);
	llinfos << "  GL_CONSTANT_ATTENUATION  : " << fv1(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_QUADRATIC_ATTENUATION, fv);
	llinfos << "  GL_QUADRATIC_ATTENUATION : " << fv1(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_SPOT_DIRECTION, fv);
	llinfos << "  GL_SPOT_DIRECTION        : " << fv4(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_SPOT_EXPONENT, fv);
	llinfos << "  GL_SPOT_EXPONENT         : " << fv1(fv) << llendl;

	glGetLightfv(GL_LIGHT0+l, GL_SPOT_CUTOFF, fv);
	llinfos << "  GL_SPOT_CUTOFF           : " << fv1(fv) << llendl;
	}

	llinfos << "-----------------------------------" << llendl;
	llinfos << "Pixel Operations" << llendl;
	llinfos << "-----------------------------------" << llendl;

	llinfos << "GL_ALPHA_TEST              : " << boolstr(glIsEnabled(GL_ALPHA_TEST)) << llendl;
	llinfos << "GL_DEPTH_TEST              : " << boolstr(glIsEnabled(GL_DEPTH_TEST)) << llendl;

	glGetBooleanv(GL_DEPTH_WRITEMASK, &b);
	llinfos << "GL_DEPTH_WRITEMASK         : " << boolstr(b) << llendl;
	
	llinfos << "GL_BLEND                   : " << boolstr(glIsEnabled(GL_BLEND)) << llendl;
	llinfos << "GL_DITHER                  : " << boolstr(glIsEnabled(GL_DITHER)) << llendl;
}

// End
