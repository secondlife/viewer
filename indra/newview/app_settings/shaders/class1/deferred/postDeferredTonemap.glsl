/**
 * @file postDeferredTonemap.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform sampler2D exposureMap;

uniform sampler2D tonemapTex;

uniform float gamma;
uniform vec2 screen_res;
in vec2 vary_fragcoord;

vec3 linear_to_srgb(vec3 cl);

//===============================================================
// tone mapping taken from Khronos sample implementation
//===============================================================

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3
(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);


// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3
(
    1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

// ACES tone map (faster approximation)
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneMapACES_Narkowicz(vec3 color)
{
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}


// ACES filmic tone map approximation
// see https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 RRTAndODTFit(vec3 color)
{
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}


// tone mapping
vec3 toneMapACES_Hill(vec3 color)
{
    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

// Khronos Neutral tonemapping
// https://github.com/KhronosGroup/ToneMapping/tree/main
// Input color is non-negative and resides in the Linear Rec. 709 color space.
// Output color is also Linear Rec. 709, but in the [0, 1] range.
vec3 PBRNeutralToneMapping( vec3 color )
{
  const float startCompression = 0.8 - 0.04;
  const float desaturation = 0.15;

  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  color -= offset;

  float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression) return color;

  const float d = 1. - startCompression;
  float newPeak = 1. - d * d / (peak + d - startCompression);
  color *= newPeak / peak;

  float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
  return mix(color, newPeak * vec3(1, 1, 1), g);
}

// https://github.com/godotengine/godot/blob/master/servers/rendering/renderer_rd/shaders/effects/tonemap.glsl

// Polynomial approximation of EaryChow's AgX sigmoid curve.
// x must be within the range [0.0, 1.0]
vec3 agx_contrast_approx(vec3 x) {
	// Generated with Excel trendline
	// Input data: Generated using python sigmoid with EaryChow's configuration and 57 steps
	// Additional padding values were added to give correct intersections at 0.0 and 1.0
	// 6th order, intercept of 0.0 to remove an operation and ensure intersection at 0.0
	vec3 x2 = x * x;
	vec3 x4 = x2 * x2;
	return 0.021 * x + 4.0111 * x2 - 25.682 * x2 * x + 70.359 * x4 - 74.778 * x4 * x + 27.069 * x4 * x2;
}

// This is an approximation and simplification of EaryChow's AgX implementation that is used by Blender.
// This code is based off of the script that generates the AgX_Base_sRGB.cube LUT that Blender uses.
// Source: https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBasesRGB.py
vec3 tonemap_agx(vec3 color) {
	// Combined linear sRGB to linear Rec 2020 and Blender AgX inset matrices:
	const mat3 srgb_to_rec2020_agx_inset_matrix = mat3(
			0.54490813676363087053, 0.14044005884001287035, 0.088827411851915368603,
			0.37377945959812267119, 0.75410959864013760045, 0.17887712465043811023,
			0.081384976686407536266, 0.10543358536857773485, 0.73224999956948382528);

	// Combined inverse AgX outset matrix and linear Rec 2020 to linear sRGB matrices.
	const mat3 agx_outset_rec2020_to_srgb_matrix = mat3(
			1.9645509602733325934, -0.29932243390911083839, -0.16436833806080403409,
			-0.85585845117807513559, 1.3264510741502356555, -0.23822464068860595117,
			-0.10886710826831608324, -0.027084020983874825605, 1.402665347143271889);

	// LOG2_MIN      = -10.0
	// LOG2_MAX      =  +6.5
	// MIDDLE_GRAY   =  0.18
	const float min_ev = -12.4739311883324; // log2(pow(2, LOG2_MIN) * MIDDLE_GRAY)
	const float max_ev = 4.02606881166759; // log2(pow(2, LOG2_MAX) * MIDDLE_GRAY)

	// Large negative values in one channel and large positive values in other
	// channels can result in a colour that appears darker and more saturated than
	// desired after passing it through the inset matrix. For this reason, it is
	// best to prevent negative input values.
	// This is done before the Rec. 2020 transform to allow the Rec. 2020
	// transform to be combined with the AgX inset matrix. This results in a loss
	// of color information that could be correctly interpreted within the
	// Rec. 2020 color space as positive RGB values, but it is less common for Godot
	// to provide this function with negative sRGB values and therefore not worth
	// the performance cost of an additional matrix multiplication.
	// A value of 2e-10 intentionally introduces insignificant error to prevent
	// log2(0.0) after the inset matrix is applied; color will be >= 1e-10 after
	// the matrix transform.
	color = max(color, 2e-10);

	// Do AGX in rec2020 to match Blender and then apply inset matrix.
	color = srgb_to_rec2020_agx_inset_matrix * color;

	// Log2 space encoding.
	// Must be clamped because agx_contrast_approx may not work
	// well with values outside of the range [0.0, 1.0]
	color = clamp(log2(color), min_ev, max_ev);
	color = (color - min_ev) / (max_ev - min_ev);

	// Apply sigmoid function approximation.
	color = agx_contrast_approx(color);

	// Convert back to linear before applying outset matrix.
	color = pow(color, vec3(2.4));

	// Apply outset to make the result more chroma-laden and then go back to linear sRGB.
	color = agx_outset_rec2020_to_srgb_matrix * color;

	// Blender's lusRGB.compensate_low_side is too complex for this shader, so
	// simply return the color, even if it has negative components. These negative
	// components may be useful for subsequent color adjustments.
	return color;
}

// https://github.com/h3r2tic/tony-mc-mapface/blob/main/shader/tony_mc_mapface.hlsl
// texture: https://github.com/SergeyMakeev/tony-mc-mapface-fuse
// godot pr: https://github.com/godotengine/godot/pull/97095

vec3 tony_mc_mapface(vec3 stimuli) {
	// return textureLod(tonemapTex, vary_fragcoord, 0).rgb;

	vec3 encoded = stimuli / (stimuli + 1.0);
	encoded = clamp(encoded, 0.0, 1.0);

	const float LUT_DIMS = 48.0;
	vec3 uv = encoded * ((LUT_DIMS - 1.0) / LUT_DIMS) + 0.5 / LUT_DIMS;

	vec3 p = uv * LUT_DIMS;

	float iz = floor(p.z - 0.5);
	float fz = p.z - 0.5 - iz;

	float x1 = iz * LUT_DIMS + p.x;
	float x2 = (iz + 1.0) * LUT_DIMS + p.x;
	float y = p.y;

	// const vec2 image_size = vec2(LUT_DIMS * LUT_DIMS, LUT_DIMS);
	const vec2 image_size = vec2(512, 512);
	const float split_every = 6.0;
	const float row_height = 64.0;

	vec2 uv1 = vec2(x1, y);
	uv1.y += floor(uv1.x / (LUT_DIMS * split_every)) * row_height;
	uv1.x = mod(uv1.x, LUT_DIMS * split_every);
	uv1 /= image_size;

	vec2 uv2 = vec2(x2, y);
	uv2.y += floor(uv2.x / (LUT_DIMS * split_every)) * row_height;
	uv2.x = mod(uv2.x, LUT_DIMS * split_every);
	uv2 /= image_size;

	vec3 sp1 = textureLod(tonemapTex, uv1, 0).rgb;
	vec3 sp2 = textureLod(tonemapTex, uv2, 0).rgb;

	vec3 linear = mix(sp1, sp2, fz);
    return linear;

	// vec3 srgb = linear * 12.92;
	// if (linear.r > 0.0031308) srgb.r = 1.055 * pow(linear.r, 1.0 / 2.4) - 0.055;
	// if (linear.g > 0.0031308) srgb.g = 1.055 * pow(linear.g, 1.0 / 2.4) - 0.055;
	// if (linear.b > 0.0031308) srgb.b = 1.055 * pow(linear.b, 1.0 / 2.4) - 0.055;

	// return srgb;
}

uniform float exposure;
uniform float tonemap_mix;
uniform int tonemap_type;

vec3 toneMap(vec3 color)
{
#ifndef NO_POST
    float exp_scale = texture(exposureMap, vec2(0.5,0.5)).r;

    color *= exposure * exp_scale;

    vec3 clamped_color = clamp(color.rgb, vec3(0.0), vec3(1.0));

    switch(tonemap_type)
    {
    case 0:
        color = PBRNeutralToneMapping(color);
        break;
    case 1:
        color = toneMapACES_Hill(color);
        break;
    case 2:
        color = tonemap_agx(color);
        break;
    case 3:
        color = tony_mc_mapface(color);
        break;
    }

    // mix tonemapped and linear here to provide adjustment
    color = mix(clamped_color, color, tonemap_mix);
#endif

    return color;
}

//=================================
// borrowed noise from:
//  <https://www.shadertoy.com/view/4dS3Wd>
//  By Morgan McGuire @morgan3d, http://graphicscodex.com
//
float hash(float n) { return fract(sin(n) * 1e4); }
float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float noise(vec2 x) {
    vec2 i = floor(x);
    vec2 f = fract(x);

    // Four corners in 2D of a tile
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Simple 2D lerp using smoothstep envelope between the values.
    // return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
    //          mix(c, d, smoothstep(0.0, 1.0, f.x)),
    //          smoothstep(0.0, 1.0, f.y)));

    // Same code, with the clamps in smoothstep and common subexpressions
    // optimized away.
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

//===============================================================

void debugExposure(inout vec3 color)
{
    float exp_scale = texture(exposureMap, vec2(0.5,0.5)).r;
    exp_scale *= 0.5;
    if (abs(vary_fragcoord.y-exp_scale) < 0.01 && vary_fragcoord.x < 0.1)
    {
        color = vec3(1,0,0);
    }
}

vec3 legacyGamma(vec3 color)
{
    vec3 c = 1. - clamp(color, vec3(0.), vec3(1.));
    c = 1. - pow(c, vec3(gamma)); // s/b inverted already CPU-side

    return c;
}

void main()
{
    //this is the one of the rare spots where diffuseRect contains linear color values (not sRGB)
    vec4 diff = texture(diffuseRect, vary_fragcoord);

#ifdef TONEMAP
#ifndef NO_POST
    diff.rgb = toneMap(diff.rgb);
#endif
#ifndef GAMMA_CORRECT
    diff.rgb = clamp(diff.rgb, vec3(0.0), vec3(1.0));
#endif
#endif

#ifdef GAMMA_CORRECT
    diff.rgb = linear_to_srgb(diff.rgb);

#ifdef LEGACY_GAMMA
    diff.rgb = legacyGamma(diff.rgb);
#endif

    vec2 tc = vary_fragcoord.xy*screen_res*4.0;
    vec3 seed = (diff.rgb+vec3(1.0))*vec3(tc.xy, tc.x+tc.y);
    vec3 nz = vec3(noise(seed.rg), noise(seed.gb), noise(seed.rb));
    diff.rgb += nz*0.003;
#endif

    //debugExposure(diff.rgb);
    frag_color = max(diff, vec4(0));
}

