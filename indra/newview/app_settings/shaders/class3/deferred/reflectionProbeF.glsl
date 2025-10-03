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

#if defined(SSR)
float tapScreenSpaceReflection(int totalSamples, vec2 tc, vec3 viewPos, vec3 n, inout vec4 collectedColor, sampler2D source, float glossiness);
#endif

uniform samplerCubeArray   reflectionProbes;
uniform samplerCubeArray   irradianceProbes;

uniform sampler2D sceneMap;
uniform int cube_snapshot;
uniform float max_probe_lod;

uniform bool transparent_surface;

uniform int classic_mode;

#define MAX_REFMAP_COUNT 256  // must match LL_MAX_REFLECTION_PROBE_COUNT

layout (std140) uniform ReflectionProbes
{
    // list of OBBs for user override probes
    // box is a set of 3 planes outward facing planes and the depth of the box along that plane
    // for each box refBox[i]...
    /// box[0..2] - plane 0 .. 2 in [A,B,C,D] notation
    //  box[3][0..2] - plane thickness
    mat4 refBox[MAX_REFMAP_COUNT];
    mat4 heroBox;
    // list of bounding spheres for reflection probes sorted by distance to camera (closest first)
    vec4 refSphere[MAX_REFMAP_COUNT];
    // extra parameters
    //  x - irradiance scale
    //  y - radiance scale
    //  z - fade in
    //  w - znear
    vec4 refParams[MAX_REFMAP_COUNT];
    vec4 heroSphere;
    // index  of cube map in reflectionProbes for a corresponding reflection probe
    // e.g. cube map channel of refSphere[2] is stored in refIndex[2]
    // refIndex.x - cubemap channel in reflectionProbes
    // refIndex.y - index in refNeighbor of neighbor list (index is ivec4 index, not int index)
    // refIndex.z - number of neighbors
    // refIndex.w - priority, if negative, this probe has a box influence
    ivec4 refIndex[MAX_REFMAP_COUNT];

    // neighbor list data (refSphere indices, not cubemap array layer)
    ivec4 refNeighbor[1024];

    ivec4 refBucket[256];

    // number of reflection probes present in refSphere
    int refmapCount;

    int heroShape;
    int heroMipCount;
    int heroProbeCount;
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

bool sample_automatic = true;

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

        // never allow automatic probes to encroach on box probes
        sample_automatic = false;
    }
    else
    {
        if (refIndex[i].w == 0 && !sample_automatic)
        {
            return false;
        }

        vec3 delta = pos.xyz - refSphere[i].xyz;
        float d = dot(delta, delta);
        float r2 = refSphere[i].w;
        r2 *= r2;

        if (d > r2)
        { // outside bounding sphere
            return false;
        }
    }

    return true;
}

int getStartIndex(vec3 pos)
{
#if 1
    int idx = clamp(int(floor(-pos.z)), 0, 255);
    return clamp(refBucket[idx].x, 1, refmapCount+1);
#else
    return 1;
#endif
}

// call before sampleRef
// populate "probeIndex" with N probe indices that influence pos where N is REF_SAMPLE_COUNT
void preProbeSample(vec3 pos)
{
#if REFMAP_LEVEL > 0

    int start = getStartIndex(pos);

    // TODO: make some sort of structure that reduces the number of distance checks
    for (int i = start; i < refmapCount; ++i)
    {
        // found an influencing probe
        if (shouldSampleProbe(i, pos))
        {
            probeIndex[probeInfluences] = i;
            ++probeInfluences;

            int neighborIdx = refIndex[i].y;
            if (neighborIdx != -1)
            {
                int neighborCount = refIndex[i].z;

                int count = 0;
                while (count < neighborCount)
                {
                    // check up to REF_SAMPLE_COUNT-1 neighbors (neighborIdx is ivec4 index)

                    // sample refNeighbor[neighborIdx].x
                    int idx = refNeighbor[neighborIdx].x;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            break;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        break;
                    }

                    // sample refNeighbor[neighborIdx].y
                    idx = refNeighbor[neighborIdx].y;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            break;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        break;
                    }

                    // sample refNeighbor[neighborIdx].z
                    idx = refNeighbor[neighborIdx].z;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            break;
                        }
                    }
                    count++;
                    if (count == neighborCount)
                    {
                        break;
                    }

                    // sample refNeighbor[neighborIdx].w
                    idx = refNeighbor[neighborIdx].w;
                    if (shouldSampleProbe(idx, pos))
                    {
                        probeIndex[probeInfluences++] = idx;
                        if (probeInfluences == REF_SAMPLE_COUNT)
                        {
                            break;
                        }
                    }
                    count++;

                    ++neighborIdx;
                }

                break;
            }
        }
    }

    if (sample_automatic)
    { // probe at index 0 is a special probe for smoothing out automatic probes
        probeIndex[probeInfluences++] = 0;
    }
#else
    probeIndex[probeInfluences++] = 0;
#endif
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

// adapted -- assume that origin is inside sphere, return intersection of ray with edge of sphere
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

void swap(inout float a, inout float b)
{
    float t = a;
    a = b;
    b = a;
}

// debug implementation, make no assumptions about origin
void sphereIntersectDebug(vec3 origin, vec3 dir, vec3 center, float radius2, float depth, inout vec4 col)
{
    float t[2]; // solutions for t if the ray intersects

    // geometric solution
    vec3 L = center - origin;
    float tca = dot(L, dir);
    // if (tca < 0) return false;
    float d2 = dot(L, L) - tca * tca;
    if (d2 > radius2) return;
    float thc = sqrt(radius2 - d2);
    t[0] = tca - thc;
    t[1] = tca + thc;

    for (int i = 0; i < 2; ++i)
    {
        if (t[i] > 0)
        {
            if (t[i] > depth)
            {
                float w = 0.125/((t[i]-depth)*0.125 + 1.0);
                col += vec4(0, 0, w, w)*(1.0-min(col.a, 1.0));
            }
            else
            {
                float w = 0.25;
                col += vec4(w,w,0,w)*(1.0-min(col.a, 1.0));
            }
        }
    }

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
// d - distance to nearest wall in clip space
// scale - scale of box, default 1.0
vec3 boxIntersect(vec3 origin, vec3 dir, mat4 i, out float d, float scale)
{
    // Intersection with OBB convert to unit box space
    // Transform in local unit parallax cube space (scaled and rotated)
    mat4 clipToLocal = i;

    vec3 RayLS = mat3(clipToLocal) * dir;
    vec3 PositionLS = (clipToLocal * vec4(origin, 1.0)).xyz;

    d = 1.0-max(max(abs(PositionLS.x), abs(PositionLS.y)), abs(PositionLS.z));

    vec3 Unitary = vec3(scale);
    vec3 FirstPlaneIntersect  = (Unitary - PositionLS) / RayLS;
    vec3 SecondPlaneIntersect = (-Unitary - PositionLS) / RayLS;
    vec3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
    float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

    // Use Distance in CS directly to recover intersection
    vec3 IntersectPositionCS = origin + dir * Distance;

    return IntersectPositionCS;
}

vec3 boxIntersect(vec3 origin, vec3 dir, mat4 i, out float d)
{
    return boxIntersect(origin, dir, i, d, 1.0);
}

void debugBoxCol(vec3 ro, vec3 rd, float t, vec3 p, inout vec4 col)
{
    vec3 v = ro + rd * t;

    v -= ro;
    vec3 pos = p - ro;


    bool behind = dot(v,v) > dot(pos,pos);

    float w = 0.25;

    if (behind)
    {
        w *= 0.5;
        w /= (length(v)-length(pos))*0.5+1.0;
        col += vec4(0,0,w,w)*(1.0-min(col.a, 1.0));
    }
    else
    {
        col += vec4(w,w,0,w)*(1.0-min(col.a, 1.0));
    }
}

// cribbed from https://iquilezles.org/articles/intersectors/
// axis aligned box centered at the origin, with size boxSize
void boxIntersectionDebug( in vec3 ro, in vec3 p, vec3 boxSize, inout vec4 col)
{
    vec3 rd = normalize(p-ro);

    vec3 m = 1.0/rd; // can precompute if traversing a set of aligned boxes
    vec3 n = m*ro;   // can precompute if traversing a set of aligned boxes
    vec3 k = abs(m)*boxSize;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0) return ; // no intersection

    float t = tN < 0 ? tF : tN;

    debugBoxCol(ro, rd, t, p, col);

    if (tN > 0) // eye is outside box, check backside, too
    {
        debugBoxCol(ro, rd, tF, p, col);
    }
}


void boxIntersectDebug(vec3 origin, vec3 pos, mat4 i, inout vec4 col)
{
    mat4 clipToLocal = i;

    // transform into unit cube space
    origin = (clipToLocal * vec4(origin, 1.0)).xyz;
    pos = (clipToLocal * vec4(pos, 1.0)).xyz;

    boxIntersectionDebug(origin, pos, vec3(1), col);
}


// get the weight of a sphere probe
//  pos - position to be weighted
//  dir - normal to be weighted
//  origin - center of sphere probe
//  r - radius of probe influence volume
// i - index of probe in refSphere
// dw - distance weight
float sphereWeight(vec3 pos, vec3 dir, vec3 origin, float r, vec4 i, out float dw)
{
    float r1 = r * 0.5; // 50% of radius (outer sphere to start interpolating down)
    vec3 delta = pos.xyz - origin;
    float d2 = max(length(delta), 0.001);

    float atten = 1.0 - max(d2 - r1, 0.0) / max((r - r1), 0.001);
    float w = 1.0 / d2;

    w *= i.z;

    dw = w * atten * max(r, 1.0)*4;

    w *= atten;

    return w;
}

// Tap a reflection probe
// pos - position of pixel
// dir - pixel normal
//  w - weight of sample (distance and angular attenuation)
//  dw - weight of sample (distance only)
// lod - which mip to sample (lower is higher res, sharper reflections)
// c - center of probe
// r2 - radius of probe squared
// i - index of probe
vec3 tapRefMap(vec3 pos, vec3 dir, out float w, out float dw, float lod, vec3 c, int i)
{
    // parallax adjustment
    vec3 v;

    if (refIndex[i].w < 0)
    {  // box probe
        float d = 0;
        v = boxIntersect(pos, dir, refBox[i], d);

        w = max(d, 0.001);
    }
    else
    { // sphere probe
        float r = refSphere[i].w;

        float rr = r * r;

        v = sphereIntersect(pos, dir, c,
        refIndex[i].w < 1 ? 4096.0*4096.0 : // <== effectively disable parallax correction for automatically placed probes to keep from bombing the world with obvious spheres
                rr);

        w = sphereWeight(pos, dir, refSphere[i].xyz, r, refParams[i], dw);
    }

    v -= c;
    vec3 d = normalize(v);

    v = env_mat * v;

    vec4 ret = textureLod(reflectionProbes, vec4(v.xyz, refIndex[i].x), lod) * refParams[i].y;

    return ret.rgb;
}

// Tap an irradiance map
// pos - position of pixel
// dir - pixel normal
// w - weight of sample (distance and angular attenuation)
// dw - weight of sample (distance only)
// i - index of probe
vec3 tapIrradianceMap(vec3 pos, vec3 dir, out float w, out float dw, vec3 c, int i, vec3 amblit)
{
    // parallax adjustment
    vec3 v;
    if (refIndex[i].w < 0)
    {
        float d = 0.0;
        v = boxIntersect(pos, dir, refBox[i], d, 3.0);
        w = max(d, 0.001);
    }
    else
    {
        float r = refSphere[i].w; // radius of sphere volume

        // pad sphere for manual probe extending into automatic probe space
        float rr = r * r;

        v = sphereIntersect(pos, dir, c,
        refIndex[i].w < 1 ? 4096.0*4096.0 : // <== effectively disable parallax correction for automatically placed probes to keep from bombing the world with obvious spheres
                rr);

        w = sphereWeight(pos, dir, refSphere[i].xyz, r, refParams[i], dw);
    }

    v -= c;
    v = env_mat * v;

    vec3 col = textureLod(irradianceProbes, vec4(v.xyz, refIndex[i].x), 0).rgb * refParams[i].x;

    col = mix(amblit, col, min(refParams[i].x, 1.0));

    return col;
}

vec3 sampleProbes(vec3 pos, vec3 dir, float lod)
{
    float wsum[2];
    wsum[0] = 0;
    wsum[1] = 0;

    float dwsum[2];
    dwsum[0] = 0;
    dwsum[1] = 0;

    vec3 col[2];
    col[0] = vec3(0);
    col[1] = vec3(0);

    for (int idx = 0; idx < probeInfluences; ++idx)
    {
        int i = probeIndex[idx];
        int p = clamp(abs(refIndex[i].w), 0, 1);

        if (p == 0 && !sample_automatic)
        {
            continue;
        }

        float w = 0;
        float dw = 0;
        vec3 refcol;

        {
            refcol = tapRefMap(pos, dir, w, dw, lod, refSphere[i].xyz, i);

            col[p] += refcol.rgb*w;
            wsum[p] += w;
            dwsum[p] += dw;
        }
    }

    // mix automatic and manual probes
    if (sample_automatic && wsum[0] > 0.0)
    { // some automatic probes were sampled
        col[0] *= 1.0/wsum[0];
        if (wsum[1] > 0.0)
        { //some manual probes were sampled, mix between the two
            col[1] *= 1.0/wsum[1];
            col[1] = mix(col[0], col[1], min(dwsum[1], 1.0));
            col[0] = vec3(0);
        }
    }
    else if (wsum[1] > 0.0)
    {
        // manual probes were sampled but no automatic probes were
        col[1] *= 1.0/wsum[1];
        col[0] = vec3(0);
    }

    return col[1]+col[0];
}

vec3 sampleProbeAmbient(vec3 pos, vec3 dir, vec3 amblit)
{
    // modified copy/paste of sampleProbes follows, will likely diverge from sampleProbes further
    // as irradiance map mixing is tuned independently of radiance map mixing
    float wsum[2];
    wsum[0] = 0;
    wsum[1] = 0;

    float dwsum[2];
    dwsum[0] = 0;
    dwsum[1] = 0;

    vec3 col[2];
    col[0] = vec3(0);
    col[1] = vec3(0);

    for (int idx = 0; idx < probeInfluences; ++idx)
    {
        int i = probeIndex[idx];
        int p = clamp(abs(refIndex[i].w), 0, 1);

        if (p == 0 && !sample_automatic)
        {
            continue;
        }

        {
            float w = 0;
            float dw = 0;

            vec3 refcol = tapIrradianceMap(pos, dir, w, dw, refSphere[i].xyz, i, amblit);

            col[p] += refcol*w;
            wsum[p] += w;
            dwsum[p] += dw;
        }
    }

    // mix automatic and manual probes
    if (sample_automatic && wsum[0] > 0.0)
    { // some automatic probes were sampled
        col[0] *= 1.0/wsum[0];
        if (wsum[1] > 0.0)
        { //some manual probes were sampled, mix between the two
            col[1] *= 1.0/wsum[1];
            col[1] = mix(col[0], col[1], min(dwsum[1], 1.0));
            col[0] = vec3(0);
        }
    }
    else if (wsum[1] > 0.0)
    {
        // manual probes were sampled but no automatic probes were
        col[1] *= 1.0/wsum[1];
        col[0] = vec3(0);
    }

    return col[1]+col[0];
}

#if defined(HERO_PROBES)

uniform vec4 clipPlane;
uniform samplerCubeArray   heroProbes;

void tapHeroProbe(inout vec3 glossenv, vec3 pos, vec3 norm, float glossiness)
{
    float clipDist = dot(pos.xyz, clipPlane.xyz) + clipPlane.w;
    float w = 0;
    float dw = 0;
    float falloffMult = 10;
    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);
    if (heroShape < 1)
    {
        float d = 0;
        boxIntersect(pos, norm, heroBox, d, 1.0);

        w = max(d, 0);
    }
    else
    {
        float r = heroSphere.w;

        w = sphereWeight(pos, refnormpersp, heroSphere.xyz, r, vec4(1), dw);
    }

    clipDist = clipDist * 0.95 + 0.05;
    clipDist = clamp(clipDist * falloffMult, 0, 1);
    w = clamp(w * falloffMult * clipDist, 0, 1);
    w = mix(0, w, clamp(glossiness - 0.75, 0, 1) * 4); // We only generate a quarter of the mips for the hero probes.  Linearly interpolate between normal probes and hero probes based upon glossiness.
    glossenv = mix(glossenv, textureLod(heroProbes, vec4(env_mat * refnormpersp, 0), (1.0-glossiness)*heroMipCount).xyz, w);
}

#else

void tapHeroProbe(inout vec3 glossenv, vec3 pos, vec3 norm, float glossiness)
{
}

#endif



void doProbeSample(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, bool transparent, vec3 amblit)
{
    // TODO - don't hard code lods
    float reflection_lods = max_probe_lod;

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    ambenv = amblit;

    if (classic_mode == 0)
        ambenv = sampleProbeAmbient(pos, norm, amblit);

    float lod = (1.0-glossiness)*reflection_lods;
    glossenv = sampleProbes(pos, normalize(refnormpersp), lod);

#if defined(SSR)
    if (cube_snapshot != 1 && glossiness >= 0.9)
    {
        vec4 ssr = vec4(0);
        if (transparent)
        {
            tapScreenSpaceReflection(1, tc, pos, norm, ssr, sceneMap, 1);
            ssr.a *= glossiness;
        }
        else
        {
            tapScreenSpaceReflection(1, tc, pos, norm, ssr, sceneMap, glossiness);
        }


        glossenv = mix(glossenv, ssr.rgb, ssr.a);
    }
#endif

    tapHeroProbe(glossenv, pos, norm, glossiness);
}

void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, bool transparent, vec3 amblit)
{
    preProbeSample(pos);
    doProbeSample(ambenv, glossenv, tc, pos, norm, glossiness, transparent, amblit);
}

void sampleReflectionProbesWater(inout vec3 ambenv, inout vec3 glossenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, vec3 amblit)
{
    // don't sample automatic probes for water
    sample_automatic = false;
    preProbeSample(pos);
    sample_automatic = true;
    // always include void probe on water
    probeIndex[probeInfluences++] = 0;

    doProbeSample(ambenv, glossenv, tc, pos, norm, glossiness, false, amblit);
}

void debugTapRefMap(vec3 pos, vec3 dir, float depth, int i, inout vec4 col)
{
    vec3 origin = vec3(0,0,0);

    bool manual_probe = abs(refIndex[i].w) > 0;

    if (manual_probe)
    {
        if (refIndex[i].w < 0)
        {
            boxIntersectDebug(origin, pos, refBox[i], col);
        }
        else
        {
            float r = refSphere[i].w; // radius of sphere volume
            float rr = r * r; // radius squared

            float t = 0.0;

            sphereIntersectDebug(origin, dir, refSphere[i].xyz, rr, depth, col);
        }
    }
}

vec4 sampleReflectionProbesDebug(vec3 pos)
{
    vec4 col = vec4(0,0,0,0);

    vec3 dir = normalize(pos);

    float d = length(pos);

    for (int i = 1; i < refmapCount; ++i)
    {
        debugTapRefMap(pos, dir, d, i, col);
    }

#if 0 //debug getStartIndex
    col.g = float(getStartIndex(pos));

    col.g /= 255.0;
    col.rb = vec2(0);
    col.a = 1.0;
#endif

    return col;
}

void sampleReflectionProbesLegacy(inout vec3 ambenv, inout vec3 glossenv, inout vec3 legacyenv,
        vec2 tc, vec3 pos, vec3 norm, float glossiness, float envIntensity, bool transparent, vec3 amblit)
{
    float reflection_lods = max_probe_lod;
    preProbeSample(pos);

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    ambenv = amblit;

    if (classic_mode == 0)
        ambenv = sampleProbeAmbient(pos, norm, amblit);

    if (glossiness > 0.0)
    {
        float lod = (1.0-glossiness)*reflection_lods;
        glossenv = sampleProbes(pos, normalize(refnormpersp), lod);

    }

    if (envIntensity > 0.0)
    {
        legacyenv = sampleProbes(pos, normalize(refnormpersp), 0.0);
    }

#if defined(SSR)
    if (cube_snapshot != 1)
    {
        vec4 ssr = vec4(0);

        if (transparent)
        {
            tapScreenSpaceReflection(1, tc, pos, norm, ssr, sceneMap, 1);
            ssr.a *= glossiness;
        }
        else
        {
            tapScreenSpaceReflection(1, tc, pos, norm, ssr, sceneMap, glossiness);
        }

        glossenv = mix(glossenv, ssr.rgb, ssr.a);
        legacyenv = mix(legacyenv, ssr.rgb, ssr.a);
    }
#endif

    tapHeroProbe(glossenv, pos, norm, glossiness);
    tapHeroProbe(legacyenv, pos, norm, 1.0);

    glossenv = clamp(glossenv, vec3(0), vec3(10));
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

