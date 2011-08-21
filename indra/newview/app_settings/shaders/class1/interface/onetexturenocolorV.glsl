/** 
 * @file onetexturenocolorV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

attribute vec3 position;
attribute vec2 texcoord0;


void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1);
	gl_TexCoord[0] = vec4(texcoord0,0,1);
}

