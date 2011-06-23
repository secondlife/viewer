/** 
 * @file atmosphericVarsV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


varying vec3 vary_PositionEye;


vec3 getPositionEye()
{
	return vary_PositionEye;
}

void setPositionEye(vec3 v)
{
	vary_PositionEye = v;
}
