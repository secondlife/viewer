/** 
 * @file alphaV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */

#version 120

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
void calcAtmospherics(vec3 inPositionEye);

float calcDirectionalLight(vec3 n, vec3 l);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);
vec3 scaleUpLight(vec3 light);

varying vec3 vary_ambient;
varying vec3 vary_directional;
varying vec3 vary_fragcoord;
varying vec3 vary_position;
varying vec3 vary_light;
varying vec3 vary_pointlight_col;

uniform float near_clip;
uniform float shadow_offset;
uniform float shadow_bias;

float calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	//normalize light vector
	lv *= 1.0/d;
	
	//distance attenuation
	float dist2 = d*d/(la*la);
	float da = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);

	// spotlight coefficient.
	float spot = max(dot(-ln, lv), is_pointlight);
	da *= spot*spot; // GL_SPOT_EXPONENT=2

	//angular attenuation
	da *= calcDirectionalLight(n, lv);

	return da;	
}

void main()
{
	//transform vertex
	gl_Position = ftransform(); 
	
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	
	vec4 pos = (gl_ModelViewMatrix * gl_Vertex);
	vec3 norm = normalize(gl_NormalMatrix * gl_Normal);
	
	float dp_directional_light = max(0.0, dot(norm, gl_LightSource[0].position.xyz));
	vary_position = pos.xyz + gl_LightSource[0].position.xyz * (1.0-dp_directional_light)*shadow_offset;
		
	calcAtmospherics(pos.xyz);

	//vec4 color = calcLighting(pos.xyz, norm, gl_Color, vec4(0.));
	vec4 col = vec4(0.0, 0.0, 0.0, gl_Color.a);

	// Collect normal lights
	col.rgb += gl_LightSource[2].diffuse.rgb*calcPointLightOrSpotLight(pos.xyz, norm, gl_LightSource[2].position, gl_LightSource[2].spotDirection.xyz, gl_LightSource[2].linearAttenuation, gl_LightSource[2].quadraticAttenuation, gl_LightSource[2].specular.a);
	col.rgb += gl_LightSource[3].diffuse.rgb*calcPointLightOrSpotLight(pos.xyz, norm, gl_LightSource[3].position, gl_LightSource[3].spotDirection.xyz, gl_LightSource[3].linearAttenuation, gl_LightSource[3].quadraticAttenuation ,gl_LightSource[3].specular.a);
	col.rgb += gl_LightSource[4].diffuse.rgb*calcPointLightOrSpotLight(pos.xyz, norm, gl_LightSource[4].position, gl_LightSource[4].spotDirection.xyz, gl_LightSource[4].linearAttenuation, gl_LightSource[4].quadraticAttenuation, gl_LightSource[4].specular.a);
	col.rgb += gl_LightSource[5].diffuse.rgb*calcPointLightOrSpotLight(pos.xyz, norm, gl_LightSource[5].position, gl_LightSource[5].spotDirection.xyz, gl_LightSource[5].linearAttenuation, gl_LightSource[5].quadraticAttenuation, gl_LightSource[5].specular.a);
	col.rgb += gl_LightSource[6].diffuse.rgb*calcPointLightOrSpotLight(pos.xyz, norm, gl_LightSource[6].position, gl_LightSource[6].spotDirection.xyz, gl_LightSource[6].linearAttenuation, gl_LightSource[6].quadraticAttenuation, gl_LightSource[6].specular.a);
	col.rgb += gl_LightSource[7].diffuse.rgb*calcPointLightOrSpotLight(pos.xyz, norm, gl_LightSource[7].position, gl_LightSource[7].spotDirection.xyz, gl_LightSource[7].linearAttenuation, gl_LightSource[7].quadraticAttenuation, gl_LightSource[7].specular.a);
	
	vary_pointlight_col = col.rgb*gl_Color.rgb;

	col.rgb = vec3(0,0,0);

	// Add windlight lights
	col.rgb = atmosAmbient(vec3(0.));
	
	vary_light = gl_LightSource[0].position.xyz;
	
	vary_ambient = col.rgb*gl_Color.rgb;
	vary_directional.rgb = gl_Color.rgb*atmosAffectDirectionalLight(max(calcDirectionalLight(norm, gl_LightSource[0].position.xyz), (1.0-gl_Color.a)*(1.0-gl_Color.a)));
	
	col.rgb = col.rgb*gl_Color.rgb;
	
	gl_FrontColor = col;

	gl_FogFragCoord = pos.z;
	
	pos = gl_ModelViewProjectionMatrix * gl_Vertex;
	vary_fragcoord.xyz = pos.xyz + vec3(0,0,near_clip);
	
}
