/** 
 * @file lightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

float calcDirectionalLight(vec3 n, vec3 l);

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseLight)
{
	vec4 col;
	col.a = color.a;
	
	col.rgb = gl_LightModel.ambient.rgb + baseLight.rgb;
	
	col.rgb += gl_LightSource[0].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[0].position.xyz);
	col.rgb += gl_LightSource[1].diffuse.rgb*calcDirectionalLight(norm, gl_LightSource[1].position.xyz);
						
	col.rgb = min(col.rgb*color.rgb, 1.0);
	
	return col;	
}

