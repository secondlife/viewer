/** 
 * @file shadowV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


varying vec4 post_pos;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix*gl_Vertex;
	
	post_pos = pos;
	
	gl_Position = vec4(pos.x, pos.y, pos.w*0.5, pos.w);
	
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_FrontColor = gl_Color;
}
