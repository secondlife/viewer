/** 
 * @file noise.cpp
 * @brief Perlin noise routines for procedural textures, etc
 *
 * $LicenseInfo:firstyear=2000&license=viewergpl$
 * 
 * Copyright (c) 2000-2007, Linden Research, Inc.
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

