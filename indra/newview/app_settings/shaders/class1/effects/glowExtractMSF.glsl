/** 
 * @file glowExtractF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

uniform sampler2DMS diffuseMap;
uniform float minLuminance;
uniform float maxExtractAlpha;
uniform vec3 lumWeights;
uniform vec3 warmthWeights;
uniform float warmthAmount;

void main()
{
	ivec2 itc = ivec2(gl_TexCoord[0].xy);
	vec4 fcol = vec4(0,0,0,0);

	for (int i = 0; i < samples; i++)
	{
		vec4 col = texelFetch(diffuseMap, itc, i);	

		/// CALCULATING LUMINANCE (Using NTSC lum weights)
		/// http://en.wikipedia.org/wiki/Luma_%28video%29
		float lum = smoothstep(minLuminance, minLuminance+1.0, dot(col.rgb, lumWeights ) );
		float warmth = smoothstep(minLuminance, minLuminance+1.0, max(col.r * warmthWeights.r, max(col.g * warmthWeights.g, col.b * warmthWeights.b)) ); 
	
		fcol += vec4(col.rgb, max(col.a, mix(lum, warmth, warmthAmount) * maxExtractAlpha));
	}

	gl_FragColor = fcol/samples;
}
