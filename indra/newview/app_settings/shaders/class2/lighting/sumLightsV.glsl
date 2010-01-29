/**
 * @file sumLightsV.glsl
 *
 * Copyright (c) 2005-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

float calcDirectionalLight(vec3 n, vec3 l);
float calcPointLight(vec3 v, vec3 n, vec4 lp, float la);
float calcPointLight2(vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float is_omnidirectional);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);

vec4 sumLights(vec3 pos, vec3 norm, vec4 color, vec4 baseLight)
{
	vec4 col = vec4(0.0, 0.0, 0.0, color.a);
	
	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	col.rgb += gl_LightSource[1].diffuse.rgb * calcDirectionalLight(norm, gl_LightSource[1].position.xyz);

	col.rgb += gl_LightSource[2].diffuse.rgb * calcPointLight2(pos, norm, gl_LightSource[2].position, gl_LightSource[2].spotDirection.xyz, gl_LightSource[2].linearAttenuation, gl_LightSource[2].specular.a);
	col.rgb += gl_LightSource[3].diffuse.rgb * calcPointLight2(pos, norm, gl_LightSource[3].position, gl_LightSource[3].spotDirection.xyz, gl_LightSource[3].linearAttenuation, gl_LightSource[3].specular.a);
	//col.rgb += gl_LightSource[4].diffuse.rgb * calcPointLight2(pos, norm, gl_LightSource[4].position, gl_LightSource[4].spotDirection.xyz, gl_LightSource[4].linearAttenuation, gl_LightSource[4].specular.a);
	col.rgb = scaleDownLight(col.rgb);

	// Add windlight lights
	col.rgb += atmosAmbient(baseLight.rgb);
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLight(norm, gl_LightSource[0].position.xyz));
				
	col.rgb = min(col.rgb*color.rgb, 1.0);
	
	return col;	
}

