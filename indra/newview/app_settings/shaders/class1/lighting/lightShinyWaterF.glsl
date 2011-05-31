/** 
 * @file lightShinyWaterF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 



uniform sampler2D diffuseMap;
uniform samplerCube environmentMap;

void shiny_lighting_water() 
{
	vec4 color = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	gl_FragColor = color;
}

