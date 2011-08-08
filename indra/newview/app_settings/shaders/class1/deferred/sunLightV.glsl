/** 
 * @file sunLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec3 normal;
attribute vec4 diffuse_color;
attribute vec2 texcoord0;

varying vec4 vary_light;
varying vec2 vary_fragcoord;

uniform vec2 screen_res;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_Position = pos; 
	
	vary_fragcoord = (pos.xy * 0.5 + 0.5)*screen_res;	
	vec4 tex = vec4(texcoord0,0,1);
	tex.w = 1.0;
	
	vary_light = vec4(texcoord0,0,1);
		
	gl_FrontColor = diffuse_color;
}
