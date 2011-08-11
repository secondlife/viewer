/** 
 * @file avatarShadowV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


mat4 getSkinnedTransform();

attribute vec3 position;
attribute vec3 normal;
attribute vec2 texcoord0;

varying vec4 post_pos;

void main()
{
	gl_TexCoord[0] = vec4(texcoord0,0,1);
				
	vec4 pos;
	vec3 norm;
	
	vec4 pos_in = vec4(position.xyz, 1.0);
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], pos_in);
	pos.y = dot(trans[1], pos_in);
	pos.z = dot(trans[2], pos_in);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, normal);
	norm.y = dot(trans[1].xyz, normal);
	norm.z = dot(trans[2].xyz, normal);
	norm = normalize(norm);
	
	pos = gl_ProjectionMatrix * pos;
	post_pos = pos;

	gl_Position = vec4(pos.x, pos.y, pos.w*0.5, pos.w);
}


