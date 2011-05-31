/** 
 * @file postDeferredF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
  


#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;

uniform sampler2DRect localLightMap;
uniform sampler2DRect sunLightMap;
uniform sampler2DRect giLightMap;
uniform sampler2DRect edgeMap;

uniform sampler2D	  luminanceMap;

uniform sampler2DRect lightMap;

uniform sampler2D	  lightFunc;
uniform sampler2D	  noiseMap;

uniform float sun_lum_scale;
uniform float sun_lum_offset;
uniform float lum_scale;
uniform float lum_lod;
uniform vec4 ambient;
uniform float gi_brightness;
uniform float gi_luminance;

uniform vec4 sunlight_color;

uniform vec2 screen_res;
varying vec2 vary_fragcoord;

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	vec4 lcol = texture2DLod(luminanceMap, vec2(0.5, 0.5), lum_lod);
	
	vec3 gi_col = texture2DRect(giLightMap, vary_fragcoord.xy).rgb;
	vec4 sun_col =	texture2DRect(sunLightMap, vary_fragcoord.xy);
	vec3 local_col = texture2DRect(localLightMap, vary_fragcoord.xy).rgb;
	
	float scol = texture2DRect(lightMap, vary_fragcoord.xy).r;
			
	vec3 diff = texture2DRect(diffuseRect, vary_fragcoord.xy).rgb;
	vec4 spec = texture2DRect(specularRect, vary_fragcoord.xy);
	
	gi_col = gi_col*(diff.rgb+spec.rgb*spec.a);

	float lum = 1.0-clamp(pow(lcol.r, gi_brightness)+sun_lum_offset, 0.0, 1.0);
	
	lum *= sun_lum_scale;
	
	sun_col *= 1.0+(lum*lum_scale*scol);
					  
	vec4 col;
	col.rgb = gi_col+sun_col.rgb+local_col;
	
	col.a = sun_col.a;
	
	vec3 bcol = vec3(0,0,0);
	float tweight = 0.0;
	for (int i = 0; i < 16; i++)
	{
		float weight = (float(i)+1.0)/2.0;
		bcol += texture2DLod(luminanceMap, vary_fragcoord.xy/screen_res, weight).rgb*weight*weight*weight;
		tweight += weight*weight;
	}
	
	bcol /= tweight;
	bcol *= gi_luminance;
	col.rgb += bcol*lum;
	
	gl_FragColor = col;
	//gl_FragColor.rgb = texture2DRect(giLightMap, vary_fragcoord.xy).rgb;
}
