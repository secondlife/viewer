/** 
 * @file pickAvatarV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


mat4 getSkinnedTransform();

void main()
{
	vec4 pos;
		
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], gl_Vertex);
	pos.y = dot(trans[1], gl_Vertex);
	pos.z = dot(trans[2], gl_Vertex);
	pos.w = 1.0;
			
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ProjectionMatrix * pos;
}
