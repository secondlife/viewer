/** 
 * @file glowExtractF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseMap;
uniform float minLuminance;
uniform float maxExtractAlpha;
uniform vec3 lumWeights;
uniform vec3 warmthWeights;
uniform float warmthAmount;

void main()
{
	vec4 col = texture2DRect(diffuseMap, gl_TexCoord[0].xy);	

	/// CALCULATING LUMINANCE (Using NTSC lum weights)
	/// http://en.wikipedia.org/wiki/Luma_%28video%29
	float lum = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights ) );
	float warmth = smoothstep(minLuminance, minLuminance+1.0, max(col.r * warmthWeights.r, max(col.g * warmthWeights.g, col.b * warmthWeights.b)) ); 
	
	gl_FragColor.rgb = col.rgb; 
	gl_FragColor.a = max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha);
}
