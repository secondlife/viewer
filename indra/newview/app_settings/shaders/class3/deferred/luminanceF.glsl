/** 
 * @file luminanceF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect lightMap;
uniform sampler2DRect diffuseRect;

varying vec2 vary_fragcoord;
void main() 
{
	float i = texture2DRect(lightMap, vary_fragcoord.xy).r;
	gl_FragColor.rgb = vec3(i);
	gl_FragColor.a = 1.0;
}
