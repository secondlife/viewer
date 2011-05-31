/** 
 * @file avatarShadowV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


mat4 getSkinnedTransform();

attribute vec4 weight;

varying vec4 post_pos;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
				
	vec4 pos;
	vec3 norm;
	
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], gl_Vertex);
	pos.y = dot(trans[1], gl_Vertex);
	pos.z = dot(trans[2], gl_Vertex);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, gl_Normal);
	norm.y = dot(trans[1].xyz, gl_Normal);
	norm.z = dot(trans[2].xyz, gl_Normal);
	norm = normalize(norm);
	
	pos = gl_ProjectionMatrix * pos;
	post_pos = pos;

	gl_Position = vec4(pos.x, pos.y, pos.w*0.5, pos.w);
	
	gl_FrontColor = gl_Color;
}


