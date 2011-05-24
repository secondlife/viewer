/** 
 * @file lightFullbrightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2D diffuseMap;

vec3 fullbrightAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);

void fullbright_lighting()
{
	vec4 color = texture2D(diffuseMap, gl_TexCoord[0].xy) * gl_Color;
	
	color.rgb = fullbrightAtmosTransport(color.rgb);
	
	color.rgb = fullbrightScaleSoftClip(color.rgb);

	gl_FragColor = color;
}

