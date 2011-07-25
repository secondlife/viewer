/** 
 * @file glowcombineF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D glowMap;
uniform sampler2DRect screenMap;

void main() 
{
	gl_FragColor = texture2D(glowMap, gl_TexCoord[0].xy) +
					texture2DRect(screenMap, gl_TexCoord[1].xy);
}
