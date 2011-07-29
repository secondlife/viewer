/** 
 * @file lightWaterAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform float minimum_alpha;
uniform float maximum_alpha;

vec3 atmosLighting(vec3 light);
vec4 applyWaterFog(vec4 color);

void default_lighting_water()
{
	vec4 color = diffuseLookup(gl_TexCoord[0].xy) * gl_Color;

	if (color.a < minimum_alpha || color.a > maximum_alpha)
	{
		discard;
	}

	color.rgb = atmosLighting(color.rgb);

	gl_FragColor = applyWaterFog(color);
}

