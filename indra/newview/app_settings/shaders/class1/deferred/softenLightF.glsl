/** 
 * @file class1/deferred/softenLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
 
#extension GL_ARB_texture_rectangle : enable
#extension GL_ARB_shader_texture_lod : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect positionMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;
uniform sampler2DRect depthMap;
uniform samplerCube environmentMap;
uniform sampler2D	  lightFunc;

uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform vec4 morphFactor;
uniform vec3 camPosLocal;
//uniform vec4 camPosWorld;
uniform vec4 gamma;
uniform float global_gamma;
uniform mat3 env_mat;
uniform mat3 ssao_effect_mat;

uniform vec3 sun_dir;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 srgb_to_linear(vec3 cs);
vec3 linear_to_srgb(vec3 cl);
vec3 decode_normal (vec2 enc);

vec3 atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3 fullbrightAtmosTransportFrag(vec3 l, vec3 additive, vec3 atten);
void calcFragAtmospherics(vec3 inPositionEye, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten);

vec3 scaleSoftClip(vec3 l);
vec3 fullbrightScaleSoftClip(vec3 l);

vec4 getPosition_d(vec2 pos_screen, float depth)
{
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

vec4 getPosition(vec2 pos_screen)
{ //get position in screen space (world units) given window coordinate and depth map
	float depth = texture2DRect(depthMap, pos_screen.xy).a;
	return getPosition_d(pos_screen, depth);
}


void main() 
{
	vec2 tc = vary_fragcoord.xy;
	float depth = texture2DRect(depthMap, tc.xy).r;
	vec3 pos = getPosition_d(tc, depth).xyz;
	vec4 norm = texture2DRect(normalMap, tc);
	float envIntensity = norm.z;
	norm.xyz = decode_normal(norm.xy); // unpack norm
		
	float da = dot(norm.xyz, sun_dir.xyz);

	float final_da = max(0.0,da);
              final_da = min(final_da, 1.0f);

// why an ad hoc gamma boost here? srgb_to_linear instead?
	      final_da = pow(final_da, 1.0/1.3);

	vec4 diffuse = texture2DRect(diffuseRect, tc);

	//convert to gamma space
	diffuse.rgb = linear_to_srgb(diffuse.rgb);

	vec4 spec = texture2DRect(specularRect, vary_fragcoord.xy);
	vec3 col;
	float bloom = 0.0;
	{
        vec3 sunlit;
        vec3 amblit;
        vec3 additive;
        vec3 atten;
		calcFragAtmospherics(pos.xyz, 1.0, sunlit, amblit, additive, atten);
	
		float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
		ambient *= 0.5;
		ambient *= ambient;
		ambient = (1.0 - ambient);

		col = amblit;
        col *= ambient;
		col += (final_da * sunlit);        
		col *= diffuse.rgb;
	
		vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

		if (spec.a > 0.0) // specular reflection
		{
			// the old infinite-sky shiny reflection
			//
			
			float sa = dot(refnormpersp, sun_dir.xyz);
			vec3 dumbshiny = sunlit*(texture2D(lightFunc, vec2(sa, spec.a)).r);
			
			// add the two types of shiny together
			vec3 spec_contrib = dumbshiny * spec.rgb;
			bloom = dot(spec_contrib, spec_contrib) / 6;
			col += spec_contrib;
		}
		
		
		col = mix(col.rgb, diffuse.rgb, diffuse.a);
				
		if (envIntensity > 0.0)
		{ //add environmentmap
			vec3 env_vec = env_mat * refnormpersp;
			
			
			vec3 refcol = textureCube(environmentMap, env_vec).rgb;

			col = mix(col.rgb, refcol, 
				envIntensity);  
		}
				
		if (norm.w < 0.5)
		{
			col = mix(atmosFragLighting(col, additive, atten), fullbrightAtmosTransportFrag(col, additive, atten), diffuse.a);
			col = mix(scaleSoftClip(col), fullbrightScaleSoftClip(col), diffuse.a);
		}

		#ifdef WATER_FOG
			vec4 fogged = applyWaterFogView(pos.xyz,vec4(col, bloom));
			col = fogged.rgb;
			bloom = fogged.a;
		#endif

		col = srgb_to_linear(col);

		//col = vec3(1,0,1);
		//col.g = envIntensity;
	}

	frag_color.rgb = col.rgb;
	frag_color.a = bloom;
}

