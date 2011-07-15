/** 
 * @file waterV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


void calcAtmospherics(vec3 inPositionEye);

uniform vec2 d1;
uniform vec2 d2;
uniform float time;
uniform vec3 eyeVec;
uniform float waterHeight;

varying vec4 refCoord;
varying vec4 littleWave;
varying vec4 view;

varying vec4 vary_position;

float wave(vec2 v, float t, float f, vec2 d, float s) 
{
   return (dot(d, v)*f + t*s)*f;
}

void main()
{
	//transform vertex
	vec4 position = gl_Vertex;
	mat4 modelViewProj = gl_ModelViewProjectionMatrix;
	
	vec4 oPosition;
		    
	//get view vector
	vec3 oEyeVec;
	oEyeVec.xyz = position.xyz-eyeVec;
		
	float d = length(oEyeVec.xy);
	float ld = min(d, 2560.0);
	
	position.xy = eyeVec.xy + oEyeVec.xy/d*ld;
	view.xyz = oEyeVec;
		
	d = clamp(ld/1536.0-0.5, 0.0, 1.0);	
	d *= d;
		
	oPosition = position;
	oPosition.z = mix(oPosition.z, max(eyeVec.z*0.75, 0.0), d);
	vary_position = gl_ModelViewMatrix * oPosition;
	oPosition = modelViewProj * oPosition;
	
	refCoord.xyz = oPosition.xyz + vec3(0,0,0.2);
	
	//get wave position parameter (create sweeping horizontal waves)
	vec3 v = position.xyz;
	v.x += (cos(v.x*0.08/*+time*0.01*/)+sin(v.y*0.02))*6.0;
	    
	//push position for further horizon effect.
	position.xyz = oEyeVec.xyz*(waterHeight/oEyeVec.z);
	position.w = 1.0;
	position = position*gl_ModelViewMatrix;
	
	calcAtmospherics((gl_ModelViewMatrix * gl_Vertex).xyz);
	
	
	//pass wave parameters to pixel shader
	vec2 bigWave =  (v.xy) * vec2(0.04,0.04)  + d1 * time * 0.055;
	//get two normal map (detail map) texture coordinates
	littleWave.xy = (v.xy) * vec2(0.45, 0.9)   + d2 * time * 0.13;
	littleWave.zw = (v.xy) * vec2(0.1, 0.2) + d1 * time * 0.1;
	view.w = bigWave.y;
	refCoord.w = bigWave.x;
	
	gl_Position = oPosition;
}
