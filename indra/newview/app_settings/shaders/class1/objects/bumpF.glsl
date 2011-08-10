/** 
 * @file bumpF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform sampler2D texture0;
uniform sampler2D texture1;

void main() 
{
	float tex0 = texture2D(texture0, gl_TexCoord[0].xy).a;
	float tex1 = texture2D(texture1, gl_TexCoord[1].xy).a;

	gl_FragColor = vec4(tex0+(1.0-tex1)-0.5);
}
