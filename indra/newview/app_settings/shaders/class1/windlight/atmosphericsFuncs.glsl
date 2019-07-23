/** 
 * @file class1\windlight\atmosphericsFuncs.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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
uniform vec4 lightnorm;
uniform vec4 sunlight_color;
uniform vec4 moonlight_color;
uniform int sun_up_factor;
uniform vec4 ambient_color;
uniform vec4 blue_horizon;
uniform vec4 blue_density;
uniform float haze_horizon;
uniform float haze_density;
uniform float cloud_shadow;
uniform float density_multiplier;
uniform float distance_multiplier;
uniform float max_y;
uniform vec4 glow;
uniform float scene_light_strength;
uniform mat3 ssao_effect_mat;
uniform int no_atmo;
uniform float sun_moon_glow_factor;

float getAmbientClamp()
{
    return 1.0f;
}

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao) {

    vec3 P = inPositionEye;
   
    //(TERRAIN) limit altitude
    if (P.y > max_y) P *= (max_y / P.y);
    if (P.y < -max_y) P *= (-max_y / P.y); 

    vec3 tmpLightnorm = lightnorm.xyz;

    vec3 Pn = normalize(P);
    float Plen = length(P);

    vec4 temp1 = vec4(0);
    vec3 temp2 = vec3(0);
    vec4 blue_weight;
    vec4 haze_weight;
    vec4 sunlight = (sun_up_factor == 1) ? sunlight_color : moonlight_color;
    vec4 light_atten;

    float dens_mul = density_multiplier;
    float dist_mul = distance_multiplier;

    //sunlight attenuation effect (hue and brightness) due to atmosphere
    //this is used later for sunlight modulation at various altitudes
    light_atten = (blue_density + vec4(haze_density * 0.25)) * (dens_mul * max_y);
        //I had thought blue_density and haze_density should have equal weighting,
        //but attenuation due to haze_density tends to seem too strong

    temp1 = blue_density + vec4(haze_density);
    blue_weight = blue_density / temp1;
    haze_weight = vec4(haze_density) / temp1;

    //(TERRAIN) compute sunlight from lightnorm only (for short rays like terrain)
    temp2.y = max(0.0, tmpLightnorm.y);
    if (abs(temp2.y) > 0.000001f)
    {
        temp2.y = 1. / abs(temp2.y);
    }
    temp2.y = max(0.0000001f, temp2.y);
    sunlight *= exp(-light_atten * temp2.y);

    // main atmospheric scattering line integral
    temp2.z = Plen * dens_mul;

    // Transparency (-> temp1)
    // ATI Bugfix -- can't store temp1*temp2.z*dist_mul in a variable because the ati
    // compiler gets confused.
    temp1 = exp(-temp1 * temp2.z * dist_mul);

    //final atmosphere attenuation factor
    atten = temp1.rgb;
    
    //compute haze glow
    //(can use temp2.x as temp because we haven't used it yet)
    temp2.x = dot(Pn, tmpLightnorm.xyz);

    // dampen sun additive contrib when not facing it...
    if (length(light_dir) > 0.01)
    {
        temp2.x *= max(0.0f, dot(light_dir, Pn));
    }
    temp2.x = 1. - temp2.x;
        //temp2.x is 0 at the sun and increases away from sun
    temp2.x = max(temp2.x, .001);    //was glow.y
        //set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
    temp2.x *= glow.x;
        //higher glow.x gives dimmer glow (because next step is 1 / "angle")
    temp2.x = pow(temp2.x, glow.z);
        //glow.z should be negative, so we're doing a sort of (1 / "angle") function

    //add "minimum anti-solar illumination"
    temp2.x += .25;

    temp2.x *= sun_moon_glow_factor;
   
    vec4 amb_color = ambient_color; 

    //increase ambient when there are more clouds
    vec4 tmpAmbient = amb_color + (vec4(1.) - amb_color) * cloud_shadow * 0.5;
    
    /*  decrease value and saturation (that in HSV, not HSL) for occluded areas
     * // for HSV color/geometry used here, see http://gimp-savvy.com/BOOK/index.html?node52.html
     * // The following line of code performs the equivalent of:
     * float ambAlpha = tmpAmbient.a;
     * float ambValue = dot(vec3(tmpAmbient), vec3(0.577)); // projection onto <1/rt(3), 1/rt(3), 1/rt(3)>, the neutral white-black axis
     * vec3 ambHueSat = vec3(tmpAmbient) - vec3(ambValue);
     * tmpAmbient = vec4(RenderSSAOEffect.valueFactor * vec3(ambValue) + RenderSSAOEffect.saturationFactor *(1.0 - ambFactor) * ambHueSat, ambAlpha);
     */
    if (use_ao)
    {
        tmpAmbient = vec4(mix(ssao_effect_mat * tmpAmbient.rgb, tmpAmbient.rgb, ambFactor), tmpAmbient.a);
    }

    //haze color
        additive =
        vec3(blue_horizon * blue_weight * (sunlight*(1.-cloud_shadow) + tmpAmbient)
      + (haze_horizon * haze_weight) * (sunlight*(1.-cloud_shadow) * temp2.x
          + tmpAmbient));

    //brightness of surface both sunlight and ambient
    sunlit = sunlight.rgb * 0.5;
    amblit = tmpAmbient.rgb * .25;
    additive *= vec3(1.0 - temp1);
}
