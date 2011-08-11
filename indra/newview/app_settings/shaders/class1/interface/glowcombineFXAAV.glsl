/** 
 * @file glowcombineFXAAV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;

varying vec2 vary_tc;

void main()
{
	vec4 pos = gl_ModelViewProjectionMatrix*vec4(position.xyz, 1.0);
	gl_Position = pos;

	vary_tc = pos.xy*0.5+0.5;
}

