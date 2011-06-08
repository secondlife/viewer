/**
 * @file sumLightsV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

float calcDirectionalLight(vec3 n, vec3 l);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 scaleDownLight(vec3 light);

vec4 sumLights(vec3 pos, vec3 norm, vec4 color, vec4 baseLight)
{
	vec4 col;
	col.a = color.a;
	
	col.rgb = gl_LightSource[1].diffuse.rgb * calcDirectionalLight(norm, gl_LightSource[1].position.xyz);
	col.rgb = scaleDownLight(col.rgb);
	col.rgb += atmosAmbient(baseLight.rgb);
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLight(norm, gl_LightSource[0].position.xyz));
	
	col.rgb = min(col.rgb*color.rgb, 1.0);
	
	return col;	
}


