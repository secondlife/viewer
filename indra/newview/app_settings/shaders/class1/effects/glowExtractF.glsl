/** 
 * @file glowExtractF.glsl
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

out vec4 frag_color;

uniform sampler2D diffuseMap;
#if HAS_NOISE
uniform sampler2D glowNoiseMap;
uniform vec2 screen_res;
#endif
uniform float minLuminance;
uniform float maxExtractAlpha;
uniform vec3 lumWeights;
uniform vec3 warmthWeights;
uniform float warmthAmount;

in vec2 vary_texcoord0;

void main()
{
	vec4 col = texture(diffuseMap, vary_texcoord0.xy);	
	/// CALCULATING LUMINANCE (Using NTSC lum weights)
	/// http://en.wikipedia.org/wiki/Luma_%28video%29
	float lum = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights ) );
	float warmth = smoothstep(minLuminance, minLuminance+1.0, max(col.r * warmthWeights.r, max(col.g * warmthWeights.g, col.b * warmthWeights.b)) ); 
	
#if HAS_NOISE
    float TRUE_NOISE_RES = 128; // See mTrueNoiseMap
    // *NOTE: Usually this is vary_fragcoord not vary_texcoord0, but glow extraction is in screen space
    vec3 glow_noise = texture(glowNoiseMap, vary_texcoord0.xy * (screen_res / TRUE_NOISE_RES)).xyz;
    // Dithering. Reduces banding effects in the reduced precision glow buffer.
    float NOISE_DEPTH = 64.0;
    col.rgb += glow_noise / NOISE_DEPTH;
    col.rgb = max(col.rgb, vec3(0));
#endif
	frag_color.rgb = col.rgb;
	frag_color.a = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);
	
}
