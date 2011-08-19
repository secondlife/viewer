/** 
 * @file onetexturenocolorF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform sampler2D tex0;

void main() 
{
	gl_FragColor = texture2D(tex0, gl_TexCoord[0].xy);
}
