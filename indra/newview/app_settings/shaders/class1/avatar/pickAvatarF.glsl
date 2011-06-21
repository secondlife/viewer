/** 
 * @file pickAvatarF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform sampler2D diffuseMap;

void main() 
{
	gl_FragColor = vec4(gl_Color.rgb, texture2D(diffuseMap, gl_TexCoord[0].xy).a);
}
