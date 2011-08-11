/** 
 * @file pickAvatarV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

mat4 getSkinnedTransform();

void main()
{
	vec4 pos;
	vec4 pos_in = vec4(position, 1.0);
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], pos_in);
	pos.y = dot(trans[1], pos_in);
	pos.z = dot(trans[2], pos_in);
	pos.w = 1.0;
			
	gl_FrontColor = diffuse_color;
	gl_TexCoord[0] = vec4(texcoord0,0,1);
	gl_Position = gl_ProjectionMatrix * pos;
}
