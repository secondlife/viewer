/** 
 * @file alphaV.glsl
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

float calcDirectionalLight(vec3 n, vec3 l);
float calcPointLight(vec3 v, vec3 n, vec4 lp, float la);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);
vec3 scaleUpLight(vec3 light);

varying vec3 vary_ambient;
varying vec3 vary_directional;
varying vec3 vary_fragcoord;
varying vec3 vary_position;

uniform float near_clip;

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec4 pos = (gl_ModelViewMatrix * gl_Vertex);
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	vary_position = pos.xyz;
	
	calcAtmospherics(pos.xyz);

	//vec4 color = calcLighting(pos.xyz, norm, gl_Color, vec4(0.));
	vec4 col;
	col.a = gl_Color.a;
	
	// Add windlight lights
	col.rgb = atmosAmbient(vec3(0.));
	col.rgb = scaleUpLight(col.rgb);

	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	col.rgb += gl_LightSource[2].diffuse.rgb*calcPointLight(pos.xyz, norm, gl_LightSource[2].position, gl_LightSource[2].linearAttenuation);
	col.rgb += gl_LightSource[3].diffuse.rgb*calcPointLight(pos.xyz, norm, gl_LightSource[3].position, gl_LightSource[3].linearAttenuation);
	col.rgb += gl_LightSource[4].diffuse.rgb*calcPointLight(pos.xyz, norm, gl_LightSource[4].position, gl_LightSource[4].linearAttenuation);
	col.rgb += gl_LightSource[5].diffuse.rgb*calcPointLight(pos.xyz, norm, gl_LightSource[5].position, gl_LightSource[5].linearAttenuation);
 	col.rgb += gl_LightSource[6].diffuse.rgb*calcPointLight(pos.xyz, norm, gl_LightSource[6].position, gl_LightSource[6].linearAttenuation);
 	col.rgb += gl_LightSource[7].diffuse.rgb*calcPointLight(pos.xyz, norm, gl_LightSource[7].position, gl_LightSource[7].linearAttenuation);
	col.rgb += gl_LightSource[1].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[1].position.xyz);
	col.rgb = scaleDownLight(col.rgb);
	
	vary_ambient = col.rgb*gl_Color.rgb;
	vary_directional.rgb = gl_Color.rgb*atmosAffectDirectionalLight(max(calcDirectionalLight(norm, gl_LightSource[0].position.xyz), (1.0-gl_Color.a)*(1.0-gl_Color.a)));
	
	col.rgb = min(col.rgb*gl_Color.rgb, 1.0);
	
	gl_FrontColor = col;

	gl_FogFragCoord = pos.z;
	
	pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
	
}
