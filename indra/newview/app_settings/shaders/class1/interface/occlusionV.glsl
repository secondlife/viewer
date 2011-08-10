/** 
 * @file uiV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

attribute vec3 position;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
}

