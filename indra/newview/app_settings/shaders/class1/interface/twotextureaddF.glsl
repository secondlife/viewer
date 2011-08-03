/** 
 * @file twotextureaddF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform sampler2D tex0;
uniform sampler2D tex1;

void main() 
{
	gl_FragColor = texture2D(tex0, gl_TexCoord[0].xy)+texture2D(tex1, gl_TexCoord[1].xy);
}
