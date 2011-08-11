/**
 * @file drawQuadV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 

attribute vec3 position;
attribute vec2 texcoord0;
attribute vec2 texcoord1;


void main(void)
{
	//transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	gl_TexCoord[0] = vec4(texcoord0,0,1);
	gl_TexCoord[1] = vec4(texcoord1,0,1);
}
