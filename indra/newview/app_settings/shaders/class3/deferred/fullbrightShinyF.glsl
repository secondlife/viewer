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
VARYING vec3 vary_position;

uniform samplerCube environmentMap;

// render_hud_attachments() -> HUD objects set LLShaderMgr::NO_ATMO; used in LLDrawPoolAlpha::beginRenderPass()
uniform int no_atmo;

vec3 fullbrightShinyAtmosTransport(vec3 light);
vec3 fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten);
vec3 fullbrightScaleSoftClip(vec3 light);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);

vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

// reflection probe interface
void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyEnv, 
        vec3 pos, vec3 norm, float glossiness, float envIntensity);
void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm);
void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity);

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
        vec3 pos = vary_position;
        calcAtmosphericVars(pos.xyz, vec3(0), 1.0, sunlit, amblit, additive, atten, false);

        float env_intensity = vertex_color.a;

        vec3 ambenv;
        vec3 glossenv;
        vec3 legacyenv;
        vec3 norm = normalize(vary_texcoord1.xyz);
        vec4 spec = vec4(0,0,0,0);
        sampleReflectionProbesLegacy(ambenv, glossenv, legacyenv, pos.xyz, norm.xyz, spec.a, env_intensity);
        applyLegacyEnv(color.rgb, legacyenv, spec, pos, norm, env_intensity);

        color.rgb = fullbrightAtmosTransportFrag(color.rgb, additive, atten);
        color.rgb = fullbrightScaleSoftClip(color.rgb);
	}

	color.a = 1.0;
    
    color.rgb = srgb_to_linear(color.rgb);
	frag_color = color;
}

