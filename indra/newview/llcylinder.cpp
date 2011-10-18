/** 
 * @file llcylinder.cpp
 * @brief Draws a cylinder using display lists for speed.
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

LLCone		gCone;

//
// Cones
//

void LLCone::render(S32 sides)
{
	gGL.begin(LLRender::TRIANGLE_FAN);
	gGL.vertex3f(0,0,0);

	for (U32 i = 0; i < sides; i++)
	{
		F32 a = (F32) i/sides * F_PI*2.f;
		F32 x = cosf(a)*0.5f;
		F32 y = sinf(a)*0.5f;
		gGL.vertex3f(x,y,-.5f);
	}
	gGL.vertex3f(cosf(0.f)*0.5f, sinf(0.f)*0.5f, -0.5f);

	gGL.end();

	gGL.begin(LLRender::TRIANGLE_FAN);
	gGL.vertex3f(0.f, 0.f, 0.5f);
	for (U32 i = 0; i < sides; i++)
	{
		F32 a = (F32) i/sides * F_PI*2.f;
		F32 x = cosf(a)*0.5f;
		F32 y = sinf(a)*0.5f;
		gGL.vertex3f(x,y,-0.5f);
	}
	gGL.vertex3f(cosf(0.f)*0.5f, sinf(0.f)*0.5f, -0.5f);

	gGL.end();
}

