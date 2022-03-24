/** 
 * @file fullbrightShinyF.glsl
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
 
/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

#ifndef HAS_DIFFUSE_LOOKUP
uniform sampler2D diffuseMap;
#endif

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_texcoord1;
VARYING vec4 vary_position;

uniform samplerCube environmentMap;

// render_hud_attachments() -> HUD objects set LLShaderMgr::NO_ATMO; used in LLDrawPoolAlpha::beginRenderPass()
uniform int no_atmo;

vec3 fullbrightShinyAtmosTransport(vec3 light);
vec3 fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten);
vec3 fullbrightScaleSoftClip(vec3 light);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

// See:
//   class1\deferred\fullbrightShinyF.glsl
//   class1\lighting\lightFullbrightShinyF.glsl
void main()
{
#ifdef HAS_DIFFUSE_LOOKUP
	vec4 color = diffuseLookup(vary_texcoord0.xy);
#else
	vec4 color = texture2D(diffuseMap, vary_texcoord0.xy);
#endif
	
	color.rgb *= vertex_color.rgb;

	// SL-9632 HUDs are affected by Atmosphere
	if (no_atmo == 0)
	{
	vec3 sunlit;
	vec3 amblit;
	vec3 additive;
	vec3 atten;
		vec3 pos = vary_position.xyz/vary_position.w;

	calcAtmosphericVars(pos.xyz, vec3(0), 1.0, sunlit, amblit, additive, atten, false);
	
	vec3 envColor = textureCube(environmentMap, vary_texcoord1.xyz).rgb;	
	float env_intensity = vertex_color.a;

	//color.rgb = srgb_to_linear(color.rgb);
		color.rgb = mix(color.rgb, envColor.rgb, env_intensity);
	color.rgb = fullbrightAtmosTransportFrag(color.rgb, additive, atten);
	color.rgb = fullbrightScaleSoftClip(color.rgb);
	}

/*
	// NOTE: HUD objects will be full bright. Uncomment if you want "some" environment lighting effecting these HUD objects.
	else
	{
		vec3  envColor = textureCube(environmentMap, vary_texcoord1.xyz).rgb;
		float env_intensity = vertex_color.a;
		color.rgb = mix(color.rgb, envColor.rgb, env_intensity);
	}
*/

	color.a = 1.0;

	//color.rgb = linear_to_srgb(color.rgb);

	frag_color = color;
}

