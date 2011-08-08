/** 
 * @file pointLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

attribute vec3 position;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

varying vec4 vary_light;
varying vec4 vary_fragcoord;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	vary_fragcoord = pos;
		
	vec4 tex = vec4(texcoord0,0,1);
	tex.w = 1.0;
	
	vary_light = vec4(texcoord0,0,1);
	
	gl_Position = pos;
		
	gl_FrontColor = diffuse_color;
}
