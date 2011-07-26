/**
 * @file sumLightsSpecularV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * $/LicenseInfo$
 */
  


float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da);
vec3 atmosAmbient(vec3 light);
vec3 atmosAffectDirectionalLight(float lightIntensity);
vec3 atmosGetDiffuseSunlightColor();
vec3 scaleDownLight(vec3 light);

vec4 sumLightsSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	vec4 col;
	col.a = color.a;
	
	
	vec3 view = normalize(pos);
	
	/// collect all the specular values from each calcXXXLightSpecular() function
	vec4 specularSum = vec4(0.0);

	col.rgb = gl_LightSource[1].diffuse.rgb * calcDirectionalLightSpecular(specularColor, view, norm, gl_LightSource[1].position.xyz, gl_LightSource[1].diffuse.rgb, 1.0);
	col.rgb = scaleDownLight(col.rgb);
	col.rgb += atmosAmbient(baseCol.rgb);
	col.rgb += atmosAffectDirectionalLight(calcDirectionalLightSpecular(specularSum, view, norm, gl_LightSource[0].position.xyz,atmosGetDiffuseSunlightColor() * baseCol.a, 1.0));

	col.rgb = min(col.rgb * color.rgb, 1.0);
	specularColor.rgb = min(specularColor.rgb * specularSum.rgb, 1.0);

	col.rgb += specularColor.rgb;

	return col;	
}
