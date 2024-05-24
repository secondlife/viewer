/**
 * @file llbox.cpp
 * @brief Draws a box using display lists for speed.
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

#include "llbox.h"

#include "llgl.h"
#include "llrender.h"
#include "llglheaders.h"

LLBox       gBox;

// These routines support multiple textures on a box
void LLBox::prerender()
{
    F32 size = 1.f;

    mTriangleCount = 6 * 2;

    mVertex[0][0] = mVertex[1][0] = mVertex[2][0] = mVertex[3][0] = -size / 2;
    mVertex[4][0] = mVertex[5][0] = mVertex[6][0] = mVertex[7][0] =  size / 2;
    mVertex[0][1] = mVertex[1][1] = mVertex[4][1] = mVertex[5][1] = -size / 2;
    mVertex[2][1] = mVertex[3][1] = mVertex[6][1] = mVertex[7][1] =  size / 2;
    mVertex[0][2] = mVertex[3][2] = mVertex[4][2] = mVertex[7][2] = -size / 2;
    mVertex[1][2] = mVertex[2][2] = mVertex[5][2] = mVertex[6][2] =  size / 2;
}

// These routines support multiple textures on a box
void LLBox::cleanupGL()
{
    // No GL state, a noop.
}

void LLBox::renderface(S32 which_face)
{
    /*static F32 normals[6][3] =
    {
        {-1.0f,  0.0f,  0.0f},
        { 0.0f,  1.0f,  0.0f},
        { 1.0f,  0.0f,  0.0f},
        { 0.0f, -1.0f,  0.0f},
        { 0.0f,  0.0f,  1.0f},
        { 0.0f,  0.0f, -1.0f}
        };*/
    static S32 faces[6][4] =
    {
        {0, 1, 2, 3},
        {3, 2, 6, 7},
        {7, 6, 5, 4},
        {4, 5, 1, 0},
        {5, 6, 2, 1},
        {7, 4, 0, 3}
    };

    gGL.begin(LLRender::QUADS);
        //gGL.normal3fv(&normals[which_face][0]);
        gGL.texCoord2f(1,0);
        gGL.vertex3fv(&mVertex[ faces[which_face][0] ][0]);
        gGL.texCoord2f(1,1);
        gGL.vertex3fv(&mVertex[ faces[which_face][1] ][0]);
        gGL.texCoord2f(0,1);
        gGL.vertex3fv(&mVertex[ faces[which_face][2] ][0]);
        gGL.texCoord2f(0,0);
        gGL.vertex3fv(&mVertex[ faces[which_face][3] ][0]);
    gGL.end();
}

void LLBox::render()
{
    // This is a flattend representation of the box as render here
    //                                       .
    //              (-++)        (+++)      /|\t
    //                +------------+         | (texture coordinates)
    //                |2          1|         |
    //                |     4      |        (*) --->s
    //                |    TOP     |
    //                |            |
    // (-++)     (--+)|3          0|(+-+)     (+++)        (-++)
    //   +------------+------------+------------+------------+
    //   |2          1|2          1|2          1|2          1|
    //   |     0      |     1      |     2      |     3      |
    //   |   BACK     |   RIGHT    |   FRONT    |   LEFT     |
    //   |            |            |            |            |
    //   |3          0|3          0|3          0|3          0|
    //   +------------+------------+------------+------------+
    // (-+-)     (---)|2          1|(+--)     (++-)        (-+-)
    //                |     5      |
    //                |   BOTTOM   |
    //                |            |
    //                |3          0|
    //                +------------+
    //              (-+-)        (++-)

    renderface(5);
    renderface(4);
    renderface(3);
    renderface(2);
    renderface(1);
    renderface(0);
    gGL.flush();
}
