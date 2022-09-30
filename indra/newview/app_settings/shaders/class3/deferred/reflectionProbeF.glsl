/**
 * @file class3/deferred/reflectionProbeF.glsl
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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

#define FLT_MAX 3.402823466e+38

#define REFMAP_COUNT 256
#define REF_SAMPLE_COUNT 64 //maximum number of samples to consider

uniform samplerCubeArray   reflectionProbes;
uniform samplerCubeArray   irradianceProbes;

layout (std140) uniform ReflectionProbes
{
    // list of OBBs for user override probes
    // box is a set of 3 planes outward facing planes and the depth of the box along that plane
    // for each box refBox[i]...
    /// box[0..2] - plane 0 .. 2 in [A,B,C,D] notation
    //  box[3][0..2] - plane thickness
    mat4 refBox[REFMAP_COUNT];
    // list of bounding spheres for reflection probes sorted by distance to camera (closest first)
    vec4 refSphere[REFMAP_COUNT];
    // extra parameters (currently only .x used for probe ambiance)
    vec4 refParams[REFMAP_COUNT];
    // index  of cube map in reflectionProbes for a corresponding reflection probe
    // e.g. cube map channel of refSphere[2] is stored in refIndex[2]
    // refIndex.x - cubemap channel in reflectionProbes
    // refIndex.y - index in refNeighbor of neighbor list (index is ivec4 index, not int index)
    // refIndex.z - number of neighbors
    // refIndex.w - priority, if negative, this probe has a box influence
    ivec4 refIndex[REFMAP_COUNT];

    // neighbor list data (refSphere indices, not cubemap array layer)
    ivec4 refNeighbor[1024];

    // number of reflection probes present in refSphere
    int refmapCount;
};

// Inputs
uniform mat3 env_mat;

// list of probeIndexes shader will actually use after "getRefIndex" is called
// (stores refIndex/refSphere indices, NOT rerflectionProbes layer)
int probeIndex[REF_SAMPLE_COUNT];

// number of probes stored in probeIndex
int probeInfluences = 0;

bool isAbove(vec3 pos, vec4 plane)
{
    return (dot(plane.xyz, pos) + plane.w) > 0;
}

int max_priority = 0;

// return true if probe at index i influences position pos
bool shouldSampleProbe(int i, vec3 pos)
{
    if (refIndex[i].w < 0)
    {
        vec4 v = refBox[i] * vec4(pos, 1.0);
        if (abs(v.x) > 1 || 
            abs(v.y) > 1 ||
            abs(v.z) > 1)
        {
            return false;
        }

        max_priority = max(max_priority, -refIndex[i].w);
    }
    else
    {
        vec3 delta = pos.xyz - refSphere[i].xyz;
        float d = dot(delta, delta);
        float r2 = refSphere[i].w;
        r2 *= r2;

        if (d > r2)
        { //outside bounding sphere
            return false;
        }

        max_priority = max(max_priority, refIndex[i].w);
    }

    return true;
}

// call before sampleRef
// populate "probeIndex" with N probe indices that influence pos where N is REF_SAMPLE_COUNT
// overall algorithm -- 
void preProbeSample(vec3 pos)
{
    // TODO: make some sort of structure that reduces the number of distance checks

    for (int i = 0; i < refmapCount; ++i)
    {
        // found an influencing probe
        if (shouldSampleProbe(i, pos))
        {
            probeIndex[probeInfluences] = i;
            ++probeInfluences;

            int neighborIdx = refIndex[i].y;
            if (neighborIdx != -1)
            {
                int neighborCount = min(refIndex[i].z, REF_SAMPLE_COUNT-1);

                int count = 0;
                while (count < neighborCount)
                {
                    // check up to REF_SAMPLE_COUNT-1 neighbors (neighborIdx is ivec4 index)

                    int idx = refNeighbor[neighborIdx].x;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            return;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        return;
                    }

                    idx = refNeighbor[neighborIdx].y;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            return;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        return;
                    }

                    idx = refNeighbor[neighborIdx].z;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            return;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        return;
                    }

                    idx = refNeighbor[neighborIdx].w;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            return;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        return;
                    }

                    ++neighborIdx;
                }

                return;
            }
        }
    }
}

// from https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-sphere-intersection

// original reference implementation:
/*
bool intersect(const Ray &ray) const 
{ 
        float t0, t1; // solutions for t if the ray intersects 
#if 0 
        // geometric solution
        Vec3f L = center - orig; 
        float tca = L.dotProduct(dir); 
        // if (tca < 0) return false;
        float d2 = L.dotProduct(L) - tca * tca; 
        if (d2 > radius2) return false; 
        float thc = sqrt(radius2 - d2); 
        t0 = tca - thc; 
        t1 = tca + thc; 
#else 
        // analytic solution
        Vec3f L = orig - center; 
        float a = dir.dotProduct(dir); 
        float b = 2 * dir.dotProduct(L); 
        float c = L.dotProduct(L) - radius2; 
        if (!solveQuadratic(a, b, c, t0, t1)) return false; 
#endif 
        if (t0 > t1) std::swap(t0, t1); 
 
        if (t0 < 0) { 
            t0 = t1; // if t0 is negative, let's use t1 instead 
            if (t0 < 0) return false; // both t0 and t1 are negative 
        } 
 
        t = t0; 
 
        return true; 
} */

// adapted -- assume that origin is inside sphere, return distance from origin to edge of sphere
vec3 sphereIntersect(vec3 origin, vec3 dir, vec3 center, float radius2)
{ 
        float t0, t1; // solutions for t if the ray intersects 

        vec3 L = center - origin; 
        float tca = dot(L,dir);

        float d2 = dot(L,L) - tca * tca; 

        float thc = sqrt(radius2 - d2); 
        t0 = tca - thc; 
        t1 = tca + thc; 
 
        vec3 v = origin + dir * t1;
        return v; 
} 

// from https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
/*
vec3 DirectionWS = normalize(PositionWS - CameraWS);
vec3 ReflDirectionWS = reflect(DirectionWS, NormalWS);

// Intersection with OBB convertto unit box space
// Transform in local unit parallax cube space (scaled and rotated)
vec3 RayLS = MulMatrix( float(3x3)WorldToLocal, ReflDirectionWS);
vec3 PositionLS = MulMatrix( WorldToLocal, PositionWS);

vec3 Unitary = vec3(1.0f, 1.0f, 1.0f);
vec3 FirstPlaneIntersect  = (Unitary - PositionLS) / RayLS;
vec3 SecondPlaneIntersect = (-Unitary - PositionLS) / RayLS;
vec3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

// Use Distance in WS directly to recover intersection
vec3 IntersectPositionWS = PositionWS + ReflDirectionWS * Distance;
vec3 ReflDirectionWS = IntersectPositionWS - CubemapPositionWS;

return texCUBE(envMap, ReflDirectionWS);
*/

// get point of intersection with given probe's box influence volume
// origin - ray origin in clip space
// dir - ray direction in clip space
// i - probe index in refBox/refSphere
vec3 boxIntersect(vec3 origin, vec3 dir, int i)
{
    // Intersection with OBB convertto unit box space
    // Transform in local unit parallax cube space (scaled and rotated)
    mat4 clipToLocal = refBox[i];

    vec3 RayLS = mat3(clipToLocal) * dir;
    vec3 PositionLS = (clipToLocal * vec4(origin, 1.0)).xyz;

    vec3 Unitary = vec3(1.0f, 1.0f, 1.0f);
    vec3 FirstPlaneIntersect  = (Unitary - PositionLS) / RayLS;
    vec3 SecondPlaneIntersect = (-Unitary - PositionLS) / RayLS;
    vec3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
    float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

    // Use Distance in CS directly to recover intersection
    vec3 IntersectPositionCS = origin + dir * Distance;

    return IntersectPositionCS;
}



// Tap a reflection probe
// pos - position of pixel
// dir - pixel normal
// lod - which mip to bias towards (lower is higher res, sharper reflections)
// c - center of probe
// r2 - radius of probe squared
// i - index of probe 
// vi - point at which reflection vector struck the influence volume, in clip space
vec3 tapRefMap(vec3 pos, vec3 dir, float lod, vec3 c, float r2, int i)
{
    //lod = max(lod, 1);
    // parallax adjustment

    vec3 v;
    if (refIndex[i].w < 0)
    {
        v = boxIntersect(pos, dir, i);
    }
    else
    {
        v = sphereIntersect(pos, dir, c, r2);
    }

    v -= c;
    v = env_mat * v;
    {
        return textureLod(reflectionProbes, vec4(v.xyz, refIndex[i].x), lod).rgb;
    }
}

// Tap an irradiance map
// pos - position of pixel
// dir - pixel normal
// c - center of probe
// r2 - radius of probe squared
// i - index of probe 
// vi - point at which reflection vector struck the influence volume, in clip space
vec3 tapIrradianceMap(vec3 pos, vec3 dir, vec3 c, float r2, int i)
{
    //lod = max(lod, 1);
    // parallax adjustment

    vec3 v;
    if (refIndex[i].w < 0)
    {
        v = boxIntersect(pos, dir, i);
    }
    else
    {
        v = sphereIntersect(pos, dir, c, r2);
    }

    v -= c;
    v = env_mat * v;
    {
        return texture(irradianceProbes, vec4(v.xyz, refIndex[i].x)).rgb * refParams[i].x;
    }
}

vec3 sampleProbes(vec3 pos, vec3 dir, float lod, float minweight)
{
    float wsum = 0.0;
    vec3 col = vec3(0,0,0);
    float vd2 = dot(pos,pos); // view distance squared

    for (int idx = 0; idx < probeInfluences; ++idx)
    {
        int i = probeIndex[idx];
        if (abs(refIndex[i].w) < max_priority)
        {
            continue;
        }
        float r = refSphere[i].w; // radius of sphere volume
        float p = float(abs(refIndex[i].w)); // priority
        
        float rr = r*r; // radius squred
        float r1 = r * 0.1; // 75% of radius (outer sphere to start interpolating down)
        vec3 delta = pos.xyz-refSphere[i].xyz;
        float d2 = dot(delta,delta);
        float r2 = r1*r1; 
        
        {
            vec3 refcol = tapRefMap(pos, dir, lod, refSphere[i].xyz, rr, i);
            
            float w = 1.0/d2;

            float atten = 1.0-max(d2-r2, 0.0)/(rr-r2);
            w *= atten;
            //w *= p; // boost weight based on priority
            col += refcol*w;
            
            wsum += w;
        }
    }

    if (probeInfluences <= 1)
    { //edge-of-scene probe or no probe influence, mix in with embiggened version of probes closest to camera 
        for (int idx = 0; idx < 8; ++idx)
        {
            if (refIndex[idx].w < 0)
            { // don't fallback to box probes, they are *very* specific
                continue;
            }
            int i = idx;
            vec3 delta = pos.xyz-refSphere[i].xyz;
            float d2 = dot(delta,delta);
            
            {
                vec3 refcol = tapRefMap(pos, dir, lod, refSphere[i].xyz, d2, i);
                
                float w = 1.0/d2;
                w *= w;
                col += refcol*w;
                wsum += w;
            }
        }
    }

    if (wsum > 0.0)
    {
        col *= 1.0/wsum;
    }
    
    return col;
}

vec3 sampleProbeAmbient(vec3 pos, vec3 dir)
{
    // modified copy/paste of sampleProbes follows, will likely diverge from sampleProbes further
    // as irradiance map mixing is tuned independently of radiance map mixing
    float wsum = 0.0;
    vec3 col = vec3(0,0,0);
    float vd2 = dot(pos,pos); // view distance squared

    float minweight = 1.0;

    for (int idx = 0; idx < probeInfluences; ++idx)
    {
        int i = probeIndex[idx];
        if (abs(refIndex[i].w) < max_priority)
        {
            continue;
        }
        float r = refSphere[i].w; // radius of sphere volume
        float p = float(abs(refIndex[i].w)); // priority
        
        float rr = r*r; // radius squred
        float r1 = r * 0.1; // 75% of radius (outer sphere to start interpolating down)
        vec3 delta = pos.xyz-refSphere[i].xyz;
        float d2 = dot(delta,delta);
        float r2 = r1*r1; 
        
        {
            vec3 refcol = tapIrradianceMap(pos, dir, refSphere[i].xyz, rr, i);
            
            float w = 1.0/d2;

            float atten = 1.0-max(d2-r2, 0.0)/(rr-r2);
            w *= atten;
            //w *= p; // boost weight based on priority
            col += refcol*w;
            
            wsum += w;
        }
    }

    if (probeInfluences <= 1)
    { //edge-of-scene probe or no probe influence, mix in with embiggened version of probes closest to camera 
        for (int idx = 0; idx < 8; ++idx)
        {
            if (refIndex[idx].w < 0)
            { // don't fallback to box probes, they are *very* specific
                continue;
            }
            int i = idx;
            vec3 delta = pos.xyz-refSphere[i].xyz;
            float d2 = dot(delta,delta);
            
            {
                vec3 refcol = tapIrradianceMap(pos, dir, refSphere[i].xyz, d2, i);
                
                float w = 1.0/d2;
                w *= w;
                col += refcol*w;
                wsum += w;
            }
        }
    }

    if (wsum > 0.0)
    {
        col *= 1.0/wsum;
    }
    
    return col;
}

void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec3 pos, vec3 norm, float glossiness)
{
    // TODO - don't hard code lods
    float reflection_lods = 7;
    preProbeSample(pos);

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    ambenv = sampleProbeAmbient(pos, norm);

    float lod = (1.0-glossiness)*reflection_lods;
    glossenv = sampleProbes(pos, normalize(refnormpersp), lod, 1.f);
}

void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyenv,
        vec3 pos, vec3 norm, float glossiness, float envIntensity)
{
    // TODO - don't hard code lods
    float reflection_lods = 7;
    preProbeSample(pos);

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    ambenv = sampleProbeAmbient(pos, norm);

    if (glossiness > 0.0)
    {
        float lod = (1.0-glossiness)*reflection_lods;
        glossenv = sampleProbes(pos, normalize(refnormpersp), lod, 1.f);
    }

    if (envIntensity > 0.0)
    {
        legacyenv = sampleProbes(pos, normalize(refnormpersp), 0.0, 1.f);
    }
}

void applyGlossEnv(inout vec3 color, vec3 glossenv, vec4 spec, vec3 pos, vec3 norm)
{
    glossenv *= 0.5; // fudge darker
    float fresnel = clamp(1.0+dot(normalize(pos.xyz), norm.xyz), 0.3, 1.0);
    fresnel *= fresnel;
    fresnel *= spec.a;
    glossenv *= spec.rgb*fresnel;
    glossenv *= vec3(1.0) - color; // fake energy conservation
    color.rgb += glossenv*0.5;
}

 void applyLegacyEnv(inout vec3 color, vec3 legacyenv, vec4 spec, vec3 pos, vec3 norm, float envIntensity)
 {
    vec3 reflected_color = legacyenv;
    vec3 lookAt = normalize(pos);
    float fresnel = 1.0+dot(lookAt, norm.xyz);
    fresnel *= fresnel;
    fresnel = min(fresnel+envIntensity, 1.0);
    reflected_color *= (envIntensity*fresnel);
    color = mix(color.rgb, reflected_color*0.5, envIntensity);
 }

