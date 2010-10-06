/**
 * @file terrainWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2D detail_0;
uniform sampler2D detail_1;
uniform sampler2D detail_2;
uniform sampler2D detail_3;
uniform sampler2D alpha_ramp;

vec3 atmosLighting(vec3 light);

vec4 applyWaterFog(vec4 color);

void main()
{
	/// Note: This should duplicate the blending functionality currently used for the terrain rendering.
	
	/// TODO Confirm tex coords and bind them appropriately in vert shader.
	vec4 color0 = texture2D(detail_0, gl_TexCoord[0].xy);
	vec4 color1 = texture2D(detail_1, gl_TexCoord[0].xy);
	vec4 color2 = texture2D(detail_2, gl_TexCoord[0].xy);
	vec4 color3 = texture2D(detail_3, gl_TexCoord[0].xy);

	float alpha1 = texture2D(alpha_ramp, gl_TexCoord[0].zw).a;
	float alpha2 = texture2D(alpha_ramp,gl_TexCoord[1].xy).a;
	float alphaFinal = texture2D(alpha_ramp, gl_TexCoord[1].zw).a;
	vec4 outColor = mix( mix(color3, color2, alpha2), mix(color1, color0, alpha1), alphaFinal );
	
	/// Add WL Components
	outColor.rgb = atmosLighting(outColor.rgb * gl_Color.rgb);
	
	outColor = applyWaterFog(outColor);
	gl_FragColor = outColor;
}

