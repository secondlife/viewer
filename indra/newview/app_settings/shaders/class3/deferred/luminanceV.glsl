/** 
 * @file giV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
varying vec2 vary_fragcoord;

uniform vec2 screen_res;

attribute vec3 position;
attribute vec4 diffuse_color;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_Position = pos; 
	
	vary_fragcoord = (pos.xy * 0.5 + 0.5)*screen_res;	

	gl_FrontColor = diffuse_color;
}
