/** 
 * @file postDeferredF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2D bloomMap;

uniform vec2 screen_res;
varying vec2 vary_fragcoord;

void main() 
{
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	gl_FragColor = diff + bloom;
}
