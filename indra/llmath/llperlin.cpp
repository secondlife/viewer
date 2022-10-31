/** 
 * @file llperlin.cpp
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

#include "linden_common.h"
#include "llmath.h"

#include "llperlin.h"

#define B 0x100
#define BM 0xff
#define N 0x1000
#define NF32 (4096.f)
#define NP 12   /* 2^N */
#define NM 0xfff

static S32 p[B + B + 2];
static F32 g3[B + B + 2][3];
static F32 g2[B + B + 2][2];
static F32 g1[B + B + 2];

bool LLPerlinNoise::sInitialized = 0;

static void normalize2(F32 v[2])
{
    F32 s = 1.f/(F32)sqrt(v[0] * v[0] + v[1] * v[1]);
    v[0] = v[0] * s;
    v[1] = v[1] * s;
}

static void normalize3(F32 v[3])
{
    F32 s = 1.f/(F32)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] = v[0] * s;
    v[1] = v[1] * s;
    v[2] = v[2] * s;
}

static void fast_setup(F32 vec, U8 &b0, U8 &b1, F32 &r0, F32 &r1)
{
    S32 t_S32;

    r1  = vec + NF32;
    t_S32 = lltrunc(r1);
    b0 = (U8)t_S32;
    b1 = b0 + 1;
    r0 = r1 - t_S32;
    r1 = r0 - 1.f;
}


void LLPerlinNoise::init(void)
{
    int i, j, k;

    for (i = 0 ; i < B ; i++)
    {
        p[i] = i;

        g1[i] = (F32)((rand() % (B + B)) - B) / B;

        for (j = 0 ; j < 2 ; j++)
            g2[i][j] = (F32)((rand() % (B + B)) - B) / B;
        normalize2(g2[i]);

        for (j = 0 ; j < 3 ; j++)
            g3[i][j] = (F32)((rand() % (B + B)) - B) / B;
        normalize3(g3[i]);
    }

    while (--i)
    {
        k = p[i];
        p[i] = p[j = rand() % B];
        p[j] = k;
    }

    for (i = 0 ; i < B + 2 ; i++)
    {
        p[B + i] = p[i];
        g1[B + i] = g1[i];
        for (j = 0 ; j < 2 ; j++)
            g2[B + i][j] = g2[i][j];
        for (j = 0 ; j < 3 ; j++)
            g3[B + i][j] = g3[i][j];
    }

    sInitialized = true;
}


//============================================================================
// Noise functions

#define s_curve(t) ( t * t * (3.f - 2.f * t) )

#define lerp_m(t, a, b) ( a + t * (b - a) )

F32 LLPerlinNoise::noise1(F32 x)
{
    int bx0, bx1;
    F32 rx0, rx1, sx, t, u, v;

    if (!sInitialized)
        init();

    t = x + N;
    bx0 = (lltrunc(t)) & BM;
    bx1 = (bx0+1) & BM;
    rx0 = t - lltrunc(t);
    rx1 = rx0 - 1.f;

    sx = s_curve(rx0);

    u = rx0 * g1[ p[ bx0 ] ];
    v = rx1 * g1[ p[ bx1 ] ];

    return lerp_m(sx, u, v);
}

static F32 fast_at2(F32 rx, F32 ry, F32 *q)
{
    return rx * q[0] + ry * q[1];
}

F32 LLPerlinNoise::noise2(F32 x, F32 y)
{
    U8 bx0, bx1, by0, by1;
    U32 b00, b10, b01, b11;
    F32 rx0, rx1, ry0, ry1, *q, sx, sy, a, b, u, v;
    S32 i, j;

    if (!sInitialized)
        init();

    fast_setup(x, bx0, bx1, rx0, rx1);
    fast_setup(y, by0, by1, ry0, ry1);

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

static F32 fast_at3(F32 rx, F32 ry, F32 rz, F32 *q)
{
    return rx * q[0] + ry * q[1] + rz * q[2];
}

F32 LLPerlinNoise::noise3(F32 x, F32 y, F32 z)
{
    U8 bx0, bx1, by0, by1, bz0, bz1;
    S32 b00, b10, b01, b11;
    F32 rx0, rx1, ry0, ry1, rz0, rz1, *q, sy, sz, a, b, c, d, t, u, v;
    S32 i, j;

    if (!sInitialized)
        init();

    fast_setup(x, bx0,bx1, rx0,rx1);
    fast_setup(y, by0,by1, ry0,ry1);
    fast_setup(z, bz0,bz1, rz0,rz1);

    i = p[ bx0 ];
    j = p[ bx1 ];

    b00 = p[ i + by0 ];
    b10 = p[ j + by0 ];
    b01 = p[ i + by1 ];
    b11 = p[ j + by1 ];

    t  = s_curve(rx0);
    sy = s_curve(ry0);
    sz = s_curve(rz0);

    q = g3[ b00 + bz0 ]; 
    u = fast_at3(rx0,ry0,rz0,q);
    q = g3[ b10 + bz0 ];
    v = fast_at3(rx1,ry0,rz0,q);
    a = lerp_m(t, u, v);

    q = g3[ b01 + bz0 ];
    u = fast_at3(rx0,ry1,rz0,q);
    q = g3[ b11 + bz0 ];
    v = fast_at3(rx1,ry1,rz0,q);
    b = lerp_m(t, u, v);

    c = lerp_m(sy, a, b);

    q = g3[ b00 + bz1 ];
    u = fast_at3(rx0,ry0,rz1,q);
    q = g3[ b10 + bz1 ];
    v = fast_at3(rx1,ry0,rz1,q);
    a = lerp_m(t, u, v);

    q = g3[ b01 + bz1 ];
    u = fast_at3(rx0,ry1,rz1,q);
    q = g3[ b11 + bz1 ];
    v = fast_at3(rx1,ry1,rz1,q);
    b = lerp_m(t, u, v);

    d = lerp_m(sy, a, b);

    return lerp_m(sz, c, d);
}

F32 LLPerlinNoise::turbulence2(F32 x, F32 y, F32 freq)
{
    F32 t, lx, ly;

    for (t = 0.f ; freq >= 1.f ; freq *= 0.5f)
    {
        lx = freq * x;
        ly = freq * y;
        t += noise2(lx, ly)/freq;
    }
    return t;
}

F32 LLPerlinNoise::turbulence3(F32 x, F32 y, F32 z, F32 freq)
{
    F32 t, lx, ly, lz;

    for (t = 0.f ; freq >= 1.f ; freq *= 0.5f)
    {
        lx = freq * x;
        ly = freq * y;
        lz = freq * z;
        t += noise3(lx,ly,lz)/freq;
//      t += fabs(noise3(lx,ly,lz)) / freq;                 // Like snow - bubbly at low frequencies
//      t += sqrt(fabs(noise3(lx,ly,lz))) / freq;           // Better at low freq
//      t += (noise3(lx,ly,lz)*noise3(lx,ly,lz)) / freq;        
    }
    return t;
}

F32 LLPerlinNoise::clouds3(F32 x, F32 y, F32 z, F32 freq)
{
    F32 t, lx, ly, lz;

    for (t = 0.f ; freq >= 1.f ; freq *= 0.5f)
    {
        lx = freq * x;
        ly = freq * y;
        lz = freq * z;
//      t += noise3(lx,ly,lz)/freq;
//      t += fabs(noise3(lx,ly,lz)) / freq;                 // Like snow - bubbly at low frequencies
//      t += sqrt(fabs(noise3(lx,ly,lz))) / freq;           // Better at low freq
        t += (noise3(lx,ly,lz)*noise3(lx,ly,lz)) / freq;        
    }
    return t;
}
