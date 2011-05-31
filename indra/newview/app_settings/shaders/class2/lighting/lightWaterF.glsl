/** 
 * @file lightWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


vec3 atmosLighting(vec3 light);
vec4 applyWaterFog(vec4 color);

void default_lighting_water()
{
	vec4 color = diffuseLookup(gl_TexCoord[0].xy) * gl_Color;

	color.rgb = atmosLighting(color.rgb);

	gl_FragColor = applyWaterFog(color);
}

