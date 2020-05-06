/** 
 * @file depthToShadowVolumeG.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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
#extension GL_ARB_geometry_shader4  : enable
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

layout (triangles) in;
layout (triangle_strip, max_vertices = 128) out;

uniform sampler2DRect depthMap;
uniform mat4 shadowMatrix[6]; 
uniform vec4 lightpos;

VARYING vec2 vary_texcoord0;

out vec3 to_vec;

void cross_products(out vec4 ns[3], int a, int b, int c)
{
   ns[0] = cross(gl_PositionIn[b].xyz - gl_PositionIn[a].xyz, gl_PositionIn[c].xyz - gl_PositionIn[a].xyz);
   ns[1] = cross(gl_PositionIn[c].xyz - gl_PositionIn[b].xyz, gl_PositionIn[a].xyz - gl_PositionIn[b].xyz);
   ns[2] = cross(gl_PositionIn[a].xyz - gl_PositionIn[c].xyz, gl_PositionIn[b].xyz - gl_PositionIn[c].xyz);
}

vec3 getLightDirection(vec4 lightpos, vec3 pos)
{

    vec3 lightdir = lightpos.xyz - lightpos.w * pos;
    return lightdir;
}

void emitTri(vec4 v[3])
{
    gl_Position = proj_matrix * v[0];
    EmitVertex();

    gl_Position = proj_matrix * v[1];
    EmitVertex();

    gl_Position = proj_matrix * v[2];
    EmitVertex();

    EndPrimitive();
}

void emitQuad(vec4 v[4]
{
    // Emit a quad as a triangle strip.
    gl_Position = proj_matrix*v[0];
    EmitVertex();

    gl_Position = proj_matrix*v[1];
    EmitVertex();

    gl_Position = proj_matrix*v[2];
    EmitVertex();

    gl_Position = proj_matrix*v[3];
    EmitVertex(); 

    EndPrimitive();
}

void emitPrimitives(int layer)
{
    int i = layer;
    gl_Layer = i;

    vec4 depth1 = vec4(texture2DRect(depthMap, tc0).rg, texture2DRect(depthMap, tc1).rg));
    vec3 depth2 = vec4(texture2DRect(depthMap, tc2).rg, texture2DRect(depthMap, tc3).rg));
    vec3 depth3 = vec4(texture2DRect(depthMap, tc4).rg, texture2DRect(depthMap, tc5).rg));
    vec3 depth4 = vec4(texture2DRect(depthMap, tc6).rg, texture2DRect(depthMap, tc7).rg));

    depth1 = min(depth1, depth2);
    depth1 = min(depth1, depth3);
    depth1 = min(depth1, depth4);

    vec2 depth = min(depth1.xy, depth1.zw);

    int side = sqrt(gl_VerticesIn);

    for (int j = 0; j < side; j++)
    {
        for (int k = 0; k < side; ++k)
        {
            vec3 pos = gl_PositionIn[(j * side) + k].xyz;
            vec4 v = shadowMatrix[i] * vec4(pos, 1.0);
            gl_Position = v;
            to_vec = pos - light_position.xyz * depth;
            EmitVertex();
        }

        EndPrimitive();
    }

    vec3 norms[3]; // Normals
    vec3 lightdir3]; // Directions toward light

    vec4 v[4]; // Temporary vertices

    vec4 or_pos[3] =
    {  // Triangle oriented toward light source
        gl_PositionIn[0],
        gl_PositionIn[2],
        gl_PositionIn[4]
    };

    // Compute normal at each vertex.
    cross_products(n, 0, 2, 4);

    // Compute direction from vertices to light.
    lightdir[0] = getLightDirection(lightpos, gl_PositionIn[0].xyz);
    lightdir[1] = getLightDirection(lightpos, gl_PositionIn[2].xyz);
    lightdir[2] = getLightDirection(lightpos, gl_PositionIn[4].xyz);

    // Check if the main triangle faces the light.
    bool faces_light = true;
    if (!(dot(ns[0],d[0]) > 0
         |dot(ns[1],d[1]) > 0
         |dot(ns[2],d[2]) > 0))
    {
        // Flip vertex winding order in or_pos.
        or_pos[1] = gl_PositionIn[4];
        or_pos[2] = gl_PositionIn[2];
        faces_light = false;
    }

    // Near cap: simply render triangle.
    emitTri(or_pos);

    // Far cap: extrude positions to infinity.
    v[0] =vec4(lightpos.w * or_pos[0].xyz - lightpos.xyz,0);
    v[1] =vec4(lightpos.w * or_pos[2].xyz - lightpos.xyz,0);
    v[2] =vec4(lightpos.w * or_pos[1].xyz - lightpos.xyz,0);

    emitTri(v);

    // Loop over all edges and extrude if needed.
    for ( int i=0; i<3; i++ ) 
    {
       // Compute indices of neighbor triangle.
       int v0 = i*2;
       int nb = (i*2+1);
       int v1 = (i*2+2) % 6;
       cross_products(n, v0, nb, v1);

       // Compute direction to light, again as above.
       d[0] =lightpos.xyz-lightpos.w*gl_PositionIn[v0].xyz;
       d[1] =lightpos.xyz-lightpos.w*gl_PositionIn[nb].xyz;
       d[2] =lightpos.xyz-lightpos.w*gl_PositionIn[v1].xyz;

       bool is_parallel = gl_PositionIn[nb].w < 1e-5;

       // Extrude the edge if it does not have a
       // neighbor, or if it's a possible silhouette.
       if (is_parallel ||
          ( faces_light != (dot(ns[0],d[0])>0 ||
                            dot(ns[1],d[1])>0 ||
                            dot(ns[2],d[2])>0) ))
       {
           // Make sure sides are oriented correctly.
           int i0 = faces_light ? v0 : v1;
           int i1 = faces_light ? v1 : v0;

           v[0] = gl_PositionIn[i0];
           v[1] = vec4(lightpos.w*gl_PositionIn[i0].xyz - lightpos.xyz, 0);
           v[2] = gl_PositionIn[i1];
           v[3] = vec4(lightpos.w*gl_PositionIn[i1].xyz - lightpos.xyz, 0);

           emitQuad(v);
       }
    }
}

void main()
{
    // Output
    emitPrimitives(0);
}
