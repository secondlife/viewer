/** 
 * @file giFinalF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2D	  bloomMap;
uniform sampler2DRect edgeMap;

uniform vec2 screen_res;
varying vec2 vary_fragcoord;


void main() 
{
	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	vec4 diff = texture2DRect(diffuseRect, vary_fragcoord.xy);
	
	gl_FragColor = bloom + diff;
	//gl_FragColor.rgb = vec3(texture2DRect(edgeMap, vary_fragcoord.xy).a);
}