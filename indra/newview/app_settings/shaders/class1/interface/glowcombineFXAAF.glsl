/** 
 * @file glowcombineFXAAF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#extension GL_ARB_texture_rectangle : enable

uniform sampler2D glowMap;
uniform sampler2DRect screenMap;

uniform vec2 screen_res;
varying vec2 vary_tc;

void main() 
{
	vec3 col = texture2D(glowMap, vary_tc).rgb +
					texture2DRect(screenMap, vary_tc*screen_res).rgb;

	
	gl_FragColor = vec4(col.rgb, dot(col.rgb, vec3(0.299, 0.587, 0.144)));
}
