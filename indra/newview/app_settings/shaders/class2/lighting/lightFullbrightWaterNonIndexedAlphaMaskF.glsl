/** 
 * @file lightFullbrightWaterNonIndexedAlphaMaskF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform float minimum_alpha;
uniform float maximum_alpha;

uniform sampler2D diffuseMap;

vec3 fullbrightAtmosTransport(vec3 light);
vec4 applyWaterFog(vec4 color);

void fullbright_lighting_water()
{
	vec4 color = texture2D(diffuseMap, gl_TexCoord[0].xy) * gl_Color;

	if (color.a < minimum_alpha || color.a > maximum_alpha)
	{
		discard;
	}

	color.rgb = fullbrightAtmosTransport(color.rgb);
	
	gl_FragColor = applyWaterFog(color);
}

