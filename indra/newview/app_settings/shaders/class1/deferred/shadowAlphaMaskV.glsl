/** 
 * @file shadowAlphaMaskV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

varying vec4 post_pos;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix*vec4(position.xyz, 1.0);
	
	post_pos = pos;
	
	gl_Position = vec4(pos.x, pos.y, pos.w*0.5, pos.w);
	
	gl_TexCoord[0] = gl_TextureMatrix[0] * vec4(texcoord0,0,1);
	gl_FrontColor = diffuse_color;
}
