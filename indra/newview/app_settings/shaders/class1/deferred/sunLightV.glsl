/** 
 * @file sunLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;

varying vec2 vary_fragcoord;

uniform vec2 screen_res;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_Position = pos; 
	
	vary_fragcoord = (pos.xy * 0.5 + 0.5)*screen_res;	
}
