/** 
 * @file avatarShadowF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


uniform sampler2D diffuseMap;

varying vec4 post_pos;

void main() 
{
	//gl_FragColor = vec4(1,1,1,gl_Color.a * texture2D(diffuseMap, gl_TexCoord[0].xy).a);
	gl_FragColor = vec4(1,1,1,1);

	gl_FragDepth = max(post_pos.z/post_pos.w*0.5+0.5, 0.0);
}

