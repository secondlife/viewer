/** 
 * @file multiPointLightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */



varying vec4 vary_fragcoord;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord = pos;

	gl_Position = pos;
	gl_FrontColor = gl_Color;
}
