/** 
 * @file lightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

float calcDirectionalLight(vec3 n, vec3 l);

// Same as non-specular lighting in lightV.glsl
vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	specularColor.rgb = vec3(0.0, 0.0, 0.0);
	vec4 col;
	col.a = color.a;

	col.rgb = gl_LightModel.ambient.rgb + baseCol.rgb;

	col.rgb += gl_LightSource[0].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[0].position.xyz);
	col.rgb += gl_LightSource[1].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[1].position.xyz);

	col.rgb = min(col.rgb*color.rgb, 1.0);

	return col;	
}

