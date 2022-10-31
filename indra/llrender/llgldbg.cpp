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
    static char str[128];   /* Flawfinder: ignore */
    snprintf(str, sizeof(str), "%8.3f, %8.3f, %8.3f", f[0], f[1], f[2]);    /* Flawfinder: ignore */
    return str;
}

//------------------------------------------------------------------------
// fv1()
//------------------------------------------------------------------------
const char *fv1(F32 *f)
{
    static char str[128];   /* Flawfinder: ignore */
    snprintf(str, sizeof(str), "%8.3f", f[0]);      /* Flawfinder: ignore */
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

    LL_INFOS() << "==========================" << LL_ENDL;
    LL_INFOS() << "OpenGL State" << LL_ENDL;
    LL_INFOS() << "==========================" << LL_ENDL;

    LL_INFOS() << "-----------------------------------" << LL_ENDL;
    LL_INFOS() << "Current Values" << LL_ENDL;
    LL_INFOS() << "-----------------------------------" << LL_ENDL;

    glGetFloatv(GL_CURRENT_COLOR, fv);
    LL_INFOS() << "GL_CURRENT_COLOR          : " << fv4(fv) << LL_ENDL;

    glGetFloatv(GL_CURRENT_NORMAL, fv);
    LL_INFOS() << "GL_CURRENT_NORMAL          : " << fv3(fv) << LL_ENDL;

    LL_INFOS() << "-----------------------------------" << LL_ENDL;
    LL_INFOS() << "Lighting" << LL_ENDL;
    LL_INFOS() << "-----------------------------------" << LL_ENDL;

    LL_INFOS() << "GL_LIGHTING                : " << boolstr(glIsEnabled(GL_LIGHTING)) << LL_ENDL;

    LL_INFOS() << "GL_COLOR_MATERIAL          : " << boolstr(glIsEnabled(GL_COLOR_MATERIAL)) << LL_ENDL;

    glGetIntegerv(GL_COLOR_MATERIAL_PARAMETER, (GLint*)&i);
    LL_INFOS() << "GL_COLOR_MATERIAL_PARAMETER: " << cmstr(i) << LL_ENDL;

    glGetIntegerv(GL_COLOR_MATERIAL_FACE, (GLint*)&i);
    LL_INFOS() << "GL_COLOR_MATERIAL_FACE     : " << facestr(i) << LL_ENDL;

    fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, fv);
    LL_INFOS() << "GL_AMBIENT material        : " << fv4(fv) << LL_ENDL;

    fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, fv);
    LL_INFOS() << "GL_DIFFUSE material        : " << fv4(fv) << LL_ENDL;

    fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, fv);
    LL_INFOS() << "GL_SPECULAR material       : " << fv4(fv) << LL_ENDL;

    fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
    glGetMaterialfv(GL_FRONT, GL_EMISSION, fv);
    LL_INFOS() << "GL_EMISSION material       : " << fv4(fv) << LL_ENDL;

    fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
    glGetMaterialfv(GL_FRONT, GL_SHININESS, fv);
    LL_INFOS() << "GL_SHININESS material      : " << fv1(fv) << LL_ENDL;

    fv[0] = fv[1] = fv[2] = fv[3] = 12345.6789f;
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, fv);
    LL_INFOS() << "GL_LIGHT_MODEL_AMBIENT     : " << fv4(fv) << LL_ENDL;

    glGetBooleanv(GL_LIGHT_MODEL_LOCAL_VIEWER, &b);
    LL_INFOS() << "GL_LIGHT_MODEL_LOCAL_VIEWER: " << boolstr(b) << LL_ENDL;

    glGetBooleanv(GL_LIGHT_MODEL_TWO_SIDE, &b);
    LL_INFOS() << "GL_LIGHT_MODEL_TWO_SIDE    : " << boolstr(b) << LL_ENDL;

    for (int l=0; l<8; l++)
    {
    b = glIsEnabled(GL_LIGHT0+l);
    LL_INFOS() << "GL_LIGHT" << l << "                  : " << boolstr(b) << LL_ENDL;

    if (!b)
        continue;

    glGetLightfv(GL_LIGHT0+l, GL_AMBIENT, fv);
    LL_INFOS() << "  GL_AMBIENT light         : " << fv4(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_DIFFUSE, fv);
    LL_INFOS() << "  GL_DIFFUSE light         : " << fv4(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_SPECULAR, fv);
    LL_INFOS() << "  GL_SPECULAR light        : " << fv4(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_POSITION, fv);
    LL_INFOS() << "  GL_POSITION light        : " << fv4(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_CONSTANT_ATTENUATION, fv);
    LL_INFOS() << "  GL_CONSTANT_ATTENUATION  : " << fv1(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_QUADRATIC_ATTENUATION, fv);
    LL_INFOS() << "  GL_QUADRATIC_ATTENUATION : " << fv1(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_SPOT_DIRECTION, fv);
    LL_INFOS() << "  GL_SPOT_DIRECTION        : " << fv4(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_SPOT_EXPONENT, fv);
    LL_INFOS() << "  GL_SPOT_EXPONENT         : " << fv1(fv) << LL_ENDL;

    glGetLightfv(GL_LIGHT0+l, GL_SPOT_CUTOFF, fv);
    LL_INFOS() << "  GL_SPOT_CUTOFF           : " << fv1(fv) << LL_ENDL;
    }

    LL_INFOS() << "-----------------------------------" << LL_ENDL;
    LL_INFOS() << "Pixel Operations" << LL_ENDL;
    LL_INFOS() << "-----------------------------------" << LL_ENDL;

    LL_INFOS() << "GL_ALPHA_TEST              : " << boolstr(glIsEnabled(GL_ALPHA_TEST)) << LL_ENDL;
    LL_INFOS() << "GL_DEPTH_TEST              : " << boolstr(glIsEnabled(GL_DEPTH_TEST)) << LL_ENDL;

    glGetBooleanv(GL_DEPTH_WRITEMASK, &b);
    LL_INFOS() << "GL_DEPTH_WRITEMASK         : " << boolstr(b) << LL_ENDL;
    
    LL_INFOS() << "GL_BLEND                   : " << boolstr(glIsEnabled(GL_BLEND)) << LL_ENDL;
    LL_INFOS() << "GL_DITHER                  : " << boolstr(glIsEnabled(GL_DITHER)) << LL_ENDL;
}

// End
