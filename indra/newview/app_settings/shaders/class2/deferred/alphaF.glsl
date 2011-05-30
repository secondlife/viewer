/** 
 * @file alphaF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */
 
#version 120

#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRectShadow shadowMap0;
uniform sampler2DRectShadow shadowMap1;
uniform sampler2DRectShadow shadowMap2;
uniform sampler2DRectShadow shadowMap3;
uniform sampler2DRect depthMap;

vec4 diffuseLookup(vec2 texcoord);

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform vec2 screen_res;
uniform vec2 shadow_res;

vec3 atmosLighting(vec3 light);
vec3 scaleSoftClip(vec3 light);

varying vec3 vary_ambient;
varying vec3 vary_directional;
varying vec3 vary_fragcoord;
varying vec3 vary_position;
varying vec3 vary_pointlight_col;

uniform float shadow_bias;

uniform mat4 inv_proj;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).a;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos.xyz /= pos.w;
	pos.w = 1.0;
	return pos;
}

float pcfShadow(sampler2DRectShadow shadowMap, vec4 stc, float scl)
{
	stc.xyz /= stc.w;
	stc.z += shadow_bias;
	
	float cs = shadow2DRect(shadowMap, stc.xyz).x;
	float shadow = cs;

	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(scl, scl, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(scl, -scl, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-scl, scl, 0.0)).x, cs);
	shadow += max(shadow2DRect(shadowMap, stc.xyz+vec3(-scl, -scl, 0.0)).x, cs);
			
	return shadow/5.0;
}


void main() 
{
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;
	
	float shadow = 1.0;
	vec4 pos = vec4(vary_position, 1.0);
	
	vec4 spos = pos;
		
	if (spos.z > -shadow_clip.w)
	{	
		vec4 lpos;
		
		if (spos.z < -shadow_clip.z)
		{
			lpos = shadow_matrix[3]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap3, lpos, 1.5);
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}
		else if (spos.z < -shadow_clip.y)
		{
			lpos = shadow_matrix[2]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap2, lpos, 1.5);
		}
		else if (spos.z < -shadow_clip.x)
		{
			lpos = shadow_matrix[1]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap1, lpos, 1.5);
		}
		else
		{
			lpos = shadow_matrix[0]*spos;
			lpos.xy *= shadow_res;
			shadow = pcfShadow(shadowMap0, lpos, 1.5);
		}
	}
	
	vec4 diff = diffuseLookup(gl_TexCoord[0].xy);

	vec4 col = vec4(vary_ambient + vary_directional.rgb*shadow, gl_Color.a);
	vec4 color = diff * col;
	
	color.rgb = atmosLighting(color.rgb);

	color.rgb = scaleSoftClip(color.rgb);

	color.rgb += diff.rgb * vary_pointlight_col.rgb;

	//gl_FragColor = gl_Color;
	gl_FragColor = color;
	//gl_FragColor.r = 0.0;
	//gl_FragColor = vec4(1,shadow,1,1);
	
}

