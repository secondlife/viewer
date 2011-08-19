/** 
 * @file lightWaterAlphaMaskNonIndexedF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform float minimum_alpha;
uniform float maximum_alpha;

uniform sampler2D diffuseMap;

vec3 atmosLighting(vec3 light);
vec4 applyWaterFog(vec4 color);

void default_lighting_water()
{
	vec4 color = texture2D(diffuseMap,gl_TexCoord[0].xy) * gl_Color;

	if (color.a < minimum_alpha || color.a > maximum_alpha)
	{
		discard;
	}

	color.rgb = atmosLighting(color.rgb);

	color = applyWaterFog(color);
	
	gl_FragColor = color;
}

