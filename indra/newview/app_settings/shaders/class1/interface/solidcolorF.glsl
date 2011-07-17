/** 
 * @file twotextureaddF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
uniform sampler2D tex0;

void main() 
{
	float alpha = texture2D(tex0, gl_TexCoord[0].xy).a;

	gl_FragColor = vec4(gl_Color.rgb, alpha);
}
