/** 
 * @file glowV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
attribute vec3 position;
attribute vec2 texcoord0;

uniform vec2 glowDelta;

void main() 
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1.0);
	
	gl_TexCoord[0].xy = texcoord0 + glowDelta*(-3.5);
	gl_TexCoord[1].xy = texcoord0 + glowDelta*(-2.5);
	gl_TexCoord[2].xy = texcoord0 + glowDelta*(-1.5);
	gl_TexCoord[3].xy = texcoord0 + glowDelta*(-0.5);
	gl_TexCoord[0].zw = texcoord0 + glowDelta*(0.5);
	gl_TexCoord[1].zw = texcoord0 + glowDelta*(1.5);
	gl_TexCoord[2].zw = texcoord0 + glowDelta*(2.5);
	gl_TexCoord[3].zw = texcoord0 + glowDelta*(3.5);
}
