/** 
 * @file customalphaF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform sampler2D diffuseMap;

uniform float custom_alpha;

void main() 
{
	vec4 color = gl_Color*texture2D(diffuseMap, gl_TexCoord[0].xy);
	color.a *= custom_alpha;
	gl_FragColor = color;
}
