/** 
 * @file dofCombineF.glsl
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

#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 gl_FragColor;
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect lightMap;

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform float max_cof;
uniform float res_scale;

VARYING vec2 vary_fragcoord;

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	
	vec4 dof = texture2DRect(diffuseRect, vary_fragcoord.xy*res_scale);
	
	vec4 diff = texture2DRect(lightMap, vary_fragcoord.xy);

	float a = min(abs(diff.a*2.0-1.0) * max_cof*res_scale*res_scale, 1.0);

	if (a > 0.25 && a < 0.75)
	{ //help out the transition a bit
		float sc = a/res_scale;
		
		vec4 col;
		col = texture2DRect(lightMap, vary_fragcoord.xy+vec2(sc,sc));
		col += texture2DRect(lightMap, vary_fragcoord.xy+vec2(-sc,sc));
		col += texture2DRect(lightMap, vary_fragcoord.xy+vec2(sc,-sc));
		col += texture2DRect(lightMap, vary_fragcoord.xy+vec2(-sc,-sc));
		
		diff = mix(diff, col*0.25, a);
	}

	gl_FragColor = mix(diff, dof, a);
}
