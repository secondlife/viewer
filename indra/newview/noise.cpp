/** 
 * @file noise.cpp
 * @brief Perlin noise routines for procedural textures, etc
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "noise.h"

#include "llrand.h"


// static
#define B 0x100
S32 p[B + B + 2];
F32 g3[B + B + 2][3];
F32 g2[B + B + 2][2];
F32 g1[B + B + 2];
S32 gNoiseStart = 1;


F32 noise2(F32 *vec)
{
	U8 bx0, bx1, by0, by1;
	U32 b00, b10, b01, b11;
	F32 rx0, rx1, ry0, ry1, *q, sx, sy, a, b, u, v;
	S32 i, j;

	if (gNoiseStart) {
		gNoiseStart = 0;
		init();
	}


	fast_setup(*vec, bx0, bx1, rx0, rx1);
	fast_setup(*(vec + 1), by0, by1, ry0, ry1);

	i = *(p + bx0);
	j = *(p + bx1);

	b00 = *(p + i + by0);
	b10 = *(p + j + by0);
	b01 = *(p + i + by1);
	b11 = *(p + j + by1);

	sx = s_curve(rx0);
	sy = s_curve(ry0);


	q = *(g2 + b00);
	u = fast_at2(rx0, ry0, q);
	q = *(g2 + b10); 
	v = fast_at2(rx1, ry0, q);
	a = lerp_m(sx, u, v);

	q = *(g2 + b01); 
	u = fast_at2(rx0,ry1,q);
	q = *(g2 + b11); 
	v = fast_at2(rx1,ry1,q);
	b = lerp_m(sx, u, v);

	return lerp_m(sy, a, b);
}

