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
    for (int i = 1; i < refmapCount; ++i)
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

    if (probeInfluences == 0)
    { // probe at index 0 is a special fallback probe
        probeIndex[0] = 0;
        probeInfluences = 1;
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


void boxIntersectDebug(vec3 origin, vec3 pos, int i, inout vec4 col)
{
    mat4 clipToLocal = refBox[i];
    
    // transform into unit cube space
    origin = (clipToLocal * vec4(origin, 1.0)).xyz;
    pos = (clipToLocal * vec4(pos, 1.0)).xyz;

    boxIntersectionDebug(origin, pos, vec3(1), col);
}


// Tap a reflection probe
// pos - position of pixel
// dir - pixel normal
// vi - return value of intersection point with influence volume
// wi - return value of approximate world space position of sampled pixel
// lod - which mip to bias towards (lower is higher res, sharper reflections)
// c - center of probe
// r2 - radius of probe squared
// i - index of probe 
vec3 tapRefMap(vec3 pos, vec3 dir, out float w, out vec3 vi, out vec3 wi, float lod, vec3 c, int i)
{
    //lod = max(lod, 1);
    // parallax adjustment

    vec3 v;

    if (refIndex[i].w < 0)
    {
        v = boxIntersect(pos, dir, i);
        w = 1.0;
    }
    else
    {
        float r = refSphere[i].w; // radius of sphere volume
        float rr = r * r; // radius squared

        v = sphereIntersect(pos, dir, c, rr);

        float p = float(abs(refIndex[i].w)); // priority
 
        float r1 = r * 0.1; // 90% of radius (outer sphere to start interpolating down)
        vec3 delta = pos.xyz - refSphere[i].xyz;
        float d2 = max(dot(delta, delta), 0.001);
        float r2 = r1 * r1;

        float atten = 1.0 - max(d2 - r2, 0.0) / max((rr - r2), 0.001);

        w = 1.0 / d2;
        w *= atten;
    }

    vi = v;

    v -= c;
    vec3 d = normalize(v);

    v = env_mat * v;
    
    vec4 ret = textureLod(reflectionProbes, vec4(v.xyz, refIndex[i].x), lod);

    wi = d * ret.a * 256.0+c;

    return ret.rgb;
}

// Tap an irradiance map
// pos - position of pixel
// dir - pixel normal
// c - center of probe
// r2 - radius of probe squared
// i - index of probe 
vec3 tapIrradianceMap(vec3 pos, vec3 dir, out float w, vec3 c, int i)
{
    // parallax adjustment
    vec3 v;
    if (refIndex[i].w < 0)
    {
        v = boxIntersect(pos, dir, i);
        w = 1.0;
    }
    else
    {
        float r = refSphere[i].w; // radius of sphere volume
        float p = float(abs(refIndex[i].w)); // priority
        float rr = r * r; // radius squred

        v = sphereIntersect(pos, dir, c, rr);

        float r1 = r * 0.1; // 75% of radius (outer sphere to start interpolating down)
        vec3 delta = pos.xyz - refSphere[i].xyz;
        float d2 = dot(delta, delta);
        float r2 = r1 * r1;

        w = 1.0 / d2;

        float atten = 1.0 - max(d2 - r2, 0.0) / (rr - r2);
        w *= atten;
    }

    v -= c;
    v = env_mat * v;
    {
        return textureLod(irradianceProbes, vec4(v.xyz, refIndex[i].x), 0).rgb * refParams[i].x;
    }
}

vec3 sampleProbes(vec3 pos, vec3 dir, float lod, bool errorCorrect)
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

        float w;
        vec3 vi, wi;
        vec3 refcol;

        
        {
            if (errorCorrect && refIndex[i].w >= 0)
            { // error correction is on and this probe is a sphere
              //take a sample to get depth value, then error correct
                refcol = tapRefMap(pos, dir, w, vi, wi, abs(lod + 2), refSphere[i].xyz, i);

                //adjust lookup by distance result
                float d = length(vi - wi);
                vi += dir * d;

                vi -= refSphere[i].xyz;

                vi = env_mat * vi;

                refcol = textureLod(reflectionProbes, vec4(vi, refIndex[i].x), lod).rgb;

                // weight by vector correctness
                vec3 pi = normalize(wi - pos);
                w *= max(dot(pi, dir), 0.1);
                //w = pow(w, 32.0);
            }
            else
            {
                refcol = tapRefMap(pos, dir, w, vi, wi, lod, refSphere[i].xyz, i);
            }

            col += refcol.rgb*w;

            wsum += w;
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
        
        {
            float w;
            vec3 refcol = tapIrradianceMap(pos, dir, w, refSphere[i].xyz, i);

            col += refcol*w;
            
            wsum += w;
        }
    }

    if (wsum > 0.0)
    {
        col *= 1.0/wsum;
    }
    
    return col;
}

void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
        vec3 pos, vec3 norm, float glossiness, bool errorCorrect)
{
    // TODO - don't hard code lods
    float reflection_lods = 6;
    preProbeSample(pos);

    vec3 refnormpersp = reflect(pos.xyz, norm.xyz);

    ambenv = sampleProbeAmbient(pos, norm);

    float lod = (1.0-glossiness)*reflection_lods;
    glossenv = sampleProbes(pos, normalize(refnormpersp), lod, errorCorrect);
}

void debugTapRefMap(vec3 pos, vec3 dir, float depth, int i, inout vec4 col)
{
    vec3 origin = vec3(0,0,0);

    bool manual_probe = abs(refIndex[i].w) > 2;

    if (manual_probe)
    {
        if (refIndex[i].w < 0)
        {
            boxIntersectDebug(origin, pos, i, col);
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

    return col;
}

void sampleReflectionProbes(inout vec3 ambenv, inout vec3 glossenv,
    vec3 pos, vec3 norm, float glossiness)
{
    sampleReflectionProbes(ambenv, glossenv,
        pos, norm, glossiness, false);
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
        glossenv = sampleProbes(pos, normalize(refnormpersp), lod, false);
    }
    
    if (envIntensity > 0.0)
    {
        legacyenv = sampleProbes(pos, normalize(refnormpersp), 0.0, false);
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

