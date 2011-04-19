/** 
 * @file lightWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2D diffuseMap;

void default_lighting_water() 
{
	vec4 color = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	gl_FragColor = color;
}

