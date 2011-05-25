/** 
 * @file avatarAlphaV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);
mat4 getSkinnedTransform();
void calcAtmospherics(vec3 inPositionEye);

float calcDirectionalLight(vec3 n, vec3 l);
float calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float is_pointlight);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);
vec3 scaleUpLight(vec3 light);

varying vec3 vary_position;
varying vec3 vary_ambient;
varying vec3 vary_directional;
varying vec3 vary_fragcoord;
varying vec3 vary_pointlight_col;

uniform float near_clip;

float calcPointLightOrSpotLight(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	float da = 0.0;

	if (d > 0.0 && la > 0.0 && fa > 0.0)
	{
		//normalize light vector
		lv *= 1.0/d;
	
		//distance attenuation
		float dist2 = d*d/(la*la);
		da = clamp(1.0-(dist2-1.0*(1.0-fa))/fa, 0.0, 1.0);

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= calcDirectionalLight(n, lv);
	}

	return da;	
}

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
				
	vec4 pos;
	vec3 norm;
	
	mat4 trans = getSkinnedTransform();
	pos.x = dot(trans[0], gl_Vertex);
	pos.y = dot(trans[1], gl_Vertex);
	pos.z = dot(trans[2], gl_Vertex);
	pos.w = 1.0;
	
	norm.x = dot(trans[0].xyz, gl_Normal);
	norm.y = dot(trans[1].xyz, gl_Normal);
	norm.z = dot(trans[2].xyz, gl_Normal);
	norm = normalize(norm);
		
	vec4 frag_pos = gl_ProjectionMatrix * pos;
	gl_Position = frag_pos;
	
	vary_position = pos.xyz;
	
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
	
	vary_ambient = col.rgb*gl_Color.rgb;
	vary_directional = gl_Color.rgb*atmosAffectDirectionalLight(max(calcDirectionalLight(norm, gl_LightSource[0].position.xyz), (1.0-gl_Color.a)*(1.0-gl_Color.a)));
	
	col.rgb = min(col.rgb*gl_Color.rgb, 1.0);
	
	gl_FrontColor = col;

	gl_FogFragCoord = pos.z;
	
	vary_fragcoord.xyz = frag_pos.xyz + vec3(0,0,near_clip);
}


