/**
* @file materialF.glsl
*
* $LicenseInfo:firstyear=2007&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2007, Linden Research, Inc.
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


/*[EXTRA_CODE_HERE]*/

//class1/deferred/materialF.glsl

// Debug stub for non-pbr material

#define DIFFUSE_ALPHA_MODE_BLEND    1

#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
out vec4 frag_color;
#else
out vec4 frag_data[4];
#endif

void main()
{
#if (DIFFUSE_ALPHA_MODE == DIFFUSE_ALPHA_MODE_BLEND)
    frag_color = vec4(1, 0, 0, 0.5);
#else
    // emissive red PBR material
    frag_data[0] = vec4(0, 0, 0, 0);
    frag_data[1] = vec4(0, 0, 0, 0);
    frag_data[2] = vec4(1, 0, 0, GBUFFER_FLAG_HAS_PBR);
    frag_data[3] = vec4(1, 0, 0, 0);
#endif
}
