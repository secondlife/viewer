/** 
 * @file glowV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

uniform vec2 glowDelta;

void main() 
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy + glowDelta*(-3.5);
	gl_TexCoord[1].xy = gl_MultiTexCoord0.xy + glowDelta*(-2.5);
	gl_TexCoord[2].xy = gl_MultiTexCoord0.xy + glowDelta*(-1.5);
	gl_TexCoord[3].xy = gl_MultiTexCoord0.xy + glowDelta*(-0.5);
	gl_TexCoord[0].zw = gl_MultiTexCoord0.xy + glowDelta*(0.5);
	gl_TexCoord[1].zw = gl_MultiTexCoord0.xy + glowDelta*(1.5);
	gl_TexCoord[2].zw = gl_MultiTexCoord0.xy + glowDelta*(2.5);
	gl_TexCoord[3].zw = gl_MultiTexCoord0.xy + glowDelta*(3.5);
}
