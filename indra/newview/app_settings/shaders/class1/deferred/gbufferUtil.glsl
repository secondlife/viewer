/**
 * @file class1/deferred/gbufferUtil.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
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

uniform sampler2D diffuseRect;
uniform sampler2D specularRect;

#if defined(HAS_EMISSIVE)
uniform sampler2D emissiveRect;
#endif

vec4 getNormRaw(vec2 screenpos);
vec4 decodeNormal(vec4 norm);

GBufferInfo getGBuffer(vec2 screenpos)
{
    GBufferInfo ret;
    vec4 diffInfo = vec4(0);
    vec4 specInfo = vec4(0);
    vec4 emissInfo = vec4(0);

    diffInfo = texture(diffuseRect, screenpos.xy);
    specInfo = texture(specularRect, screenpos.xy);
    vec4 normInfo = getNormRaw(screenpos);

#if defined(HAS_EMISSIVE)
    emissInfo = texture(emissiveRect, screenpos.xy);
#endif

    ret.albedo = diffInfo;
    ret.normal = decodeNormal(normInfo).xyz;
    ret.specular = specInfo;
    ret.envIntensity = normInfo.b;
    ret.gbufferFlag = normInfo.w;
    ret.emissive = emissInfo;

    return ret;
}
