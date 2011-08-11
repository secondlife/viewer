/** 
 * @file highlightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

uniform vec4 highlight_color;
uniform sampler2D diffuseMap;

void main() 
{
	gl_FragColor = highlight_color*texture2D(diffuseMap, gl_TexCoord[0].xy);
}
