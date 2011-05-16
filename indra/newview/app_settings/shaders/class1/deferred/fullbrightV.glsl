/** 
 * @file fullbrightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

void calcAtmospherics(vec3 inPositionEye);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);
vec3 scaleUpLight(vec3 light);

varying vec3 vary_ambient;
varying vec3 vary_directional;
varying vec3 vary_normal;
varying vec3 vary_fragcoord;
uniform float near_clip;
varying vec4 vary_position;

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec4 pos = (gl_ModelViewMatrix * gl_Vertex);
	vary_position = pos;
		
	calcAtmospherics(pos.xyz);
	
	gl_FrontColor = gl_Color;

	gl_FogFragCoord = pos.z;
	
	pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
	
}
