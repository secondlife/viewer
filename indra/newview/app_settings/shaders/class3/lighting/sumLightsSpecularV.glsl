/**
 * @file sumLightsV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da);
vec3 calcPointLightSpecular(inout vec4 specular, vec3 view, vec3 v, vec3 n, vec3 l, float r, float pw, vec3 lightCol);

vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 atmosGetDiffuseSunlightColor();
vec3 scaleDownLight(vec3 light);

vec4 sumLightsSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	vec4 col = vec4(0.0, 0.0, 0.0, color.a);
	
	vec3 view = normalize(pos);
	
	/// collect all the specular values from each calcXXXLightSpecular() function
	vec4 specularSum = vec4(0.0);
	
	// Collect normal lights (need to be divided by two, as we later multiply by 2)
	col.rgb += gl_LightSource[1].diffuse.rgb * calcDirectionalLightSpecular(specularColor, view, norm, gl_LightSource[1].position.xyz,gl_LightSource[1].diffuse.rgb, 1.0);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[2].position.xyz, gl_LightSource[2].linearAttenuation, gl_LightSource[2].quadraticAttenuation,gl_LightSource[2].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[3].position.xyz, gl_LightSource[3].linearAttenuation, gl_LightSource[3].quadraticAttenuation,gl_LightSource[3].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[4].position.xyz, gl_LightSource[4].linearAttenuation, gl_LightSource[4].quadraticAttenuation,gl_LightSource[4].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[5].position.xyz, gl_LightSource[5].linearAttenuation, gl_LightSource[5].quadraticAttenuation,gl_LightSource[5].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[6].position.xyz, gl_LightSource[6].linearAttenuation, gl_LightSource[6].quadraticAttenuation,gl_LightSource[6].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularSum, view, pos, norm, gl_LightSource[7].position.xyz, gl_LightSource[7].linearAttenuation, gl_LightSource[7].quadraticAttenuation,gl_LightSource[7].diffuse.rgb);
	col.rgb = scaleDownLight(col.rgb);
						
	// Add windlight lights
	col.rgb += atmosAmbient(baseCol.rgb);
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLightSpecular(specularSum, view, norm, gl_LightSource[0].position.xyz,atmosGetDiffuseSunlightColor()*baseCol.a, 1.0));

	col.rgb = min(col.rgb*color.rgb, 1.0);
	specularColor.rgb = min(specularColor.rgb*specularSum.rgb, 1.0);

	col.rgb += specularColor.rgb;
	return col;	
}
