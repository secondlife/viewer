// All lights, no specular highlights

float calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	return a;
}

float calcPointLight(vec3 v, vec3 n, vec4 lp, float la)
{
	//get light vector
	vec3 lv = lp.xyz-v;
	
	//get distance
	float d = length(lv);
	
	//normalize light vector
	lv *= 1.0/d;
	
	//distance attenuation
	float da = clamp(1.0/(la * d), 0.0, 1.0);
	
	//angular attenuation
	da *= calcDirectionalLight(n, lv);
	
	return da;	
}

float calcDirectionalSpecular(vec3 view, vec3 n, vec3 l)
{
	return pow(max(dot(reflect(view, n),l), 0.0),8.0);
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
	float da = clamp(1.0/(r * d), 0.0, 1.0);
	
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
	col.rgb += gl_LightSource[2].diffuse.rgb*calcPointLight(pos, norm, gl_LightSource[2].position, gl_LightSource[2].linearAttenuation);
	col.rgb += gl_LightSource[3].diffuse.rgb*calcPointLight(pos, norm, gl_LightSource[3].position, gl_LightSource[3].linearAttenuation);
	col.rgb += gl_LightSource[4].diffuse.rgb*calcPointLight(pos, norm, gl_LightSource[4].position, gl_LightSource[4].linearAttenuation);
	col.rgb += gl_LightSource[5].diffuse.rgb*calcPointLight(pos, norm, gl_LightSource[5].position, gl_LightSource[5].linearAttenuation);
	col.rgb += gl_LightSource[6].diffuse.rgb*calcPointLight(pos, norm, gl_LightSource[6].position, gl_LightSource[6].linearAttenuation);
	col.rgb += gl_LightSource[7].diffuse.rgb*calcPointLight(pos, norm, gl_LightSource[7].position, gl_LightSource[7].linearAttenuation);
				
	col.rgb = min(col.rgb*color.rgb, 1.0);
	
	gl_FrontColor = vec4(col.rgb, col.a);
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
	vec4 col;
	col.a = color.a;
	
	col.rgb = gl_LightModel.ambient.rgb;
	
	vec3 view = normalize(pos);
	
	vec4 specular = specularColor;
	specularColor.rgb = vec3(0.0, 0.0, 0.0);
	
	col.rgb += baseCol.a*gl_LightSource[0].diffuse.rgb*calcDirectionalLightSpecular(specularColor, view, norm, gl_LightSource[0].position.xyz,gl_LightSource[0].diffuse.rgb*baseCol.a, 1.0);
	col.rgb += gl_LightSource[1].diffuse.rgb*calcDirectionalLightSpecular(specularColor, view, norm, gl_LightSource[1].position.xyz,gl_LightSource[1].diffuse.rgb, 1.0);
	col.rgb += calcPointLightSpecular(specularColor, view, pos, norm, gl_LightSource[2].position.xyz, gl_LightSource[2].linearAttenuation, gl_LightSource[2].quadraticAttenuation,gl_LightSource[2].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularColor, view, pos, norm, gl_LightSource[3].position.xyz, gl_LightSource[3].linearAttenuation, gl_LightSource[3].quadraticAttenuation,gl_LightSource[3].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularColor, view, pos, norm, gl_LightSource[4].position.xyz, gl_LightSource[4].linearAttenuation, gl_LightSource[4].quadraticAttenuation,gl_LightSource[4].diffuse.rgb);
	col.rgb += calcPointLightSpecular(specularColor, view, pos, norm, gl_LightSource[5].position.xyz, gl_LightSource[5].linearAttenuation, gl_LightSource[5].quadraticAttenuation,gl_LightSource[5].diffuse.rgb);
	//col.rgb += calcPointLightSpecular(specularColor, view, pos, norm, gl_LightSource[6].position.xyz, gl_LightSource[6].linearAttenuation, gl_LightSource[6].quadraticAttenuation,gl_LightSource[6].diffuse.rgb);
	//col.rgb += calcPointLightSpecular(specularColor, view, pos, norm, gl_LightSource[7].position.xyz, gl_LightSource[7].linearAttenuation, gl_LightSource[7].quadraticAttenuation,gl_LightSource[7].diffuse.rgb);
	col.rgb += baseCol.rgb;
						
	col.rgb = min(col.rgb*color.rgb, 1.0);
	specularColor.rgb = min(specularColor.rgb*specular.rgb, 1.0);

	gl_FrontColor = vec4(col.rgb+specularColor.rgb,col.a);	
	return col;	
}

vec4 calcLightingSpecular(vec3 pos, vec3 norm, vec4 color, inout vec4 specularColor, vec3 baseCol)
{
	return calcLightingSpecular(pos, norm, color, specularColor, vec4(baseCol, 1.0));
}
