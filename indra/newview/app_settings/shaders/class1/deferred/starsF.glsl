/** 
 * @file starsF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2D diffuseMap;

void main() 
{
	vec4 col = gl_Color * texture2D(diffuseMap, gl_TexCoord[0].xy);
	
	gl_FragData[0] = col;
	gl_FragData[1] = vec4(0,0,0,0);
	gl_FragData[2] = vec4(0,0,1,0);	
}
