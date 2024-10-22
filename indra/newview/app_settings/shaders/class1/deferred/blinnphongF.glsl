/**
 * @file blinnphongF.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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


// blinn-phong debug stub
#ifdef OUTPUT_DIFFUSE_ONLY
out vec4 frag_color;
#else
out vec4 frag_data[4];
#endif

void main()
{

#ifdef OUTPUT_DIFFUSE_ONLY
    frag_color = vec4(1,0,1,1);
#else
    // See: C++: addDeferredAttachments(), GLSL: softenLightF
    frag_data[0] = vec4(0);
    frag_data[1] = vec4(0);
    frag_data[2] = vec4(vec3(0), GBUFFER_FLAG_HAS_PBR);
    frag_data[3] = vec4(1,0,1,0);
#endif
}

