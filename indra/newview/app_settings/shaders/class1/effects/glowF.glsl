/** 
 * @file glowF.glsl
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2D diffuseMap;
uniform float glowStrength;

VARYING vec4 vary_texcoord0;
VARYING vec4 vary_texcoord1;
VARYING vec4 vary_texcoord2;
VARYING vec4 vary_texcoord3;

void main()
{

	vec4 col = vec4(0.0, 0.0, 0.0, 0.0);

	// ATI compiler falls down on array initialization.
	float kern[8];
		kern[0] = 0.25; kern[1] = 0.5; kern[2] = 0.8; kern[3] = 1.0;
		kern[4] = 1.0;  kern[5] = 0.8; kern[6] = 0.5; kern[7] = 0.25;
	
	col += kern[0] * texture2D(diffuseMap, vary_texcoord0.xy);	
	col += kern[1] * texture2D(diffuseMap, vary_texcoord1.xy);
	col += kern[2] * texture2D(diffuseMap, vary_texcoord2.xy);	
	col += kern[3] * texture2D(diffuseMap, vary_texcoord3.xy);	
	col += kern[4] * texture2D(diffuseMap, vary_texcoord0.zw);	
	col += kern[5] * texture2D(diffuseMap, vary_texcoord1.zw);	
	col += kern[6] * texture2D(diffuseMap, vary_texcoord2.zw);	
	col += kern[7] * texture2D(diffuseMap, vary_texcoord3.zw);	
	
	frag_color = vec4(col.rgb * glowStrength, col.a);
}
