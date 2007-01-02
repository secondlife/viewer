
float calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	return a;
}

float calcPointLight(vec3 v, vec3 n, vec3 l, float r, float pw)
{
	//get light vector
	vec3 lv = l-v;
	
	//get distance
	float d = length(lv);
	
	//normalize light vector
	lv *= 1.0/d;
	
	//distance attenuation
	float da = max((r-d)/r, 0.0);
	
	//da = pow(da, pw);
	
	//angular attenuation
	da *= calcDirectionalLight(n, lv);
	
	return da;	
}

float calcDirectionalSpecular(vec3 view, vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	return a;
}

float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da)
{
	
	specular.rgb += calcDirectionalSpecular(view,n,l)*lightCol*da;
	return calcDirectionalLight(n,l);
}

vec3 calcPointLightSpecular(inout vec4 specular, vec3 view, vec3 v, vec3 n, vec3 l, float r, float pw, vec3 lightCol)
{
	//get light vector
	vec3 lv = l-v;
	
	//get distance
	float d = length(lv);
	
	//normalize light vector
	lv *= 1.0/d;
	
	//distance attenuation
	float da = clamp((r-d)/r, 0.0, 1.0);

	//da = pow(da, pw);
	
	//angular attenuation
	da *= calcDirectionalLightSpecular(specular, view, n, lv, lightCol, da);
	
	return da*lightCol;	
}

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

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec3 baseLight)
{
	return calcLighting(pos, norm, color, vec4(baseLight, 1.0));
}

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color)
{
	return calcLighting(pos, norm, color, vec3(0.0,0.0,0.0));
}

vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec4 baseCol)
{
	specularColor.rgb = vec3(0.0, 0.0, 0.0);
	return calcLighting(pos, norm, color, baseCol);
}

vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec3 baseCol)
{
	return calcLightingSpecular(pos, norm, color, specularColor, vec4(baseCol, 1.0));
}
