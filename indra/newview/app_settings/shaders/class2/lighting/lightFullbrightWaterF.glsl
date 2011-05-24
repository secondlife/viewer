/** 
 * @file lightFullbrightWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2D diffuseMap;

vec3 fullbrightAtmosTransport(vec3 light);
vec4 applyWaterFog(vec4 color);

void fullbright_lighting_water()
{
	vec4 color = texture2D(diffuseMap, gl_TexCoord[0].xy) * gl_Color;

	color.rgb = fullbrightAtmosTransport(color.rgb);
	
	gl_FragColor = applyWaterFog(color);
}

