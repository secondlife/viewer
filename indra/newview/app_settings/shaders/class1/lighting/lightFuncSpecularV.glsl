/** 
 * @file lightFuncSpecularV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


float calcDirectionalLight(vec3 n, vec3 l)
{
	float a = max(dot(n,l),0.0);
	return a;
}

float calcDirectionalSpecular(vec3 view, vec3 n, vec3 l)
{
	return pow(max(dot(reflect(view, n),l), 0.0),8.0);
}

float calcDirectionalLightSpecular(inout vec4 specular, vec3 view, vec3 n, vec3 l, vec3 lightCol, float da)
{
	
	specular.rgb += calcDirectionalSpecular(view,n,l)*lightCol*da;
	return max(dot(n,l),0.0);
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

