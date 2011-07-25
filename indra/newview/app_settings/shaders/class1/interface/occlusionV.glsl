/** 
 * @file uiV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}

