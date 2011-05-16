/** 
 * @file sunLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

//class 1, no shadow, no SSAO, should never be called

#extension GL_ARB_texture_rectangle : enable

void main() 
{
	gl_FragColor = vec4(0,0,0,0);
}
