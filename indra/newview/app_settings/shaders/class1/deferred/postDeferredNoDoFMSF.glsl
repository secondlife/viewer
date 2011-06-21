/** 
 * @file postDeferredF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_texture_multisample : enable

uniform sampler2DMS diffuseRect;
uniform sampler2D bloomMap;

uniform vec2 screen_res;
varying vec2 vary_fragcoord;

vec4 texture2DMS(sampler2DMS tex, ivec2 tc)
{
	vec4 ret = vec4(0,0,0,0);

	for (int i = 0; i < samples; ++i)
	{
		 ret += texelFetch(tex,tc,i);
	}

	return ret/samples;
}

void main() 
{
	vec4 diff = texture2DMS(diffuseRect, ivec2(vary_fragcoord.xy));

	vec4 bloom = texture2D(bloomMap, vary_fragcoord.xy/screen_res);
	gl_FragColor = diff + bloom;
}
