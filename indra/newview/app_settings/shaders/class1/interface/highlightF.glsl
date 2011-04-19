/** 
 * @file highlightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform sampler2D diffuseMap;

void main() 
{
	gl_FragColor = gl_Color*texture2D(diffuseMap, gl_TexCoord[0].xy);
}
