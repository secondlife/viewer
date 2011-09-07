/**
 * @file blurf.glsl
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
 


uniform sampler2DRect RenderTexture;
uniform float bloomStrength;

varying vec4 gl_TexCoord[gl_MaxTextureCoords];
void main(void) 
{
	float blurWeights[7];
	blurWeights[0] = 0.05;
	blurWeights[1] = 0.1;
	blurWeights[2] = 0.2;
	blurWeights[3] = 0.3;
	blurWeights[4] = 0.2;
	blurWeights[5] = 0.1;
	blurWeights[6] = 0.05;
	
	vec3 color = vec3(0,0,0);
	for (int i = 0; i < 7; i++){
		color += vec3(texture2DRect(RenderTexture, gl_TexCoord[i].st)) * blurWeights[i];
	}

	color *= bloomStrength;

	gl_FragColor = vec4(color, 1.0);
}
