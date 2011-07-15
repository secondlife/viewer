/**
 * @file atmosphericsHelpersV.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * $/LicenseInfo$
 */
 


// Output variables
vec3 getSunlitColor();
vec3 getAmblitColor();
vec3 getAdditiveColor();
vec3 getAtmosAttenuation();
vec3 getPositionEye();

uniform float scene_light_strength;

vec3 atmosAmbient(vec3 light)
{
	return getAmblitColor() + light / 2.0;
}

vec3 atmosAffectDirectionalLight(float lightIntensity)
{
	return getSunlitColor() * lightIntensity;
}

vec3 atmosGetDiffuseSunlightColor()
{
	return getSunlitColor();
}

vec3 scaleDownLight(vec3 light)
{
	return (light / scene_light_strength );
}

vec3 scaleUpLight(vec3 light)
{
	return (light * scene_light_strength);
}

