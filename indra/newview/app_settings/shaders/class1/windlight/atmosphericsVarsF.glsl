/** 
 * @file atmosphericVarsF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

varying vec3 vary_PositionEye;

vec3 getPositionEye()
{
	return vary_PositionEye;
}
