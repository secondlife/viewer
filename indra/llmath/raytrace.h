/**
 * @file raytrace.h
 * @brief Ray intersection tests for primitives.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#pragma once

class LLVector3;
class LLQuaternion;

// Pruned 2026-04-09 to keep only the functions still reachable from production
// code (line_plane in llviewerwindow.cpp, linesegment_sphere in llvoavatar.cpp,
// linesegment_tetrahedron in llvotree.cpp). The 14 dead ray_/linesegment_ helpers
// were deleted along with their LLQuaternion uses; the remaining 2 LLQuaternion
// uses (ray_tetrahedron and linesegment_tetrahedron) are next on the Phase 4
// drain list once the LLQuaternion class itself is deleted.
//
// All functions produce results in the same reference frame as the arguments.
// Direction/normal vectors are assumed unit; intersection out-params are
// undefined when the function returns false.

// returns true if line is not parallel to plane.
bool line_plane(const LLVector3 &line_point, const LLVector3 &line_direction,
                const LLVector3 &plane_point, const LLVector3 plane_normal,
                LLVector3 &intersection);

// helper used by ray_tetrahedron; kept because the production caller chain
// goes linesegment_tetrahedron -> ray_tetrahedron -> ray_triangle -> ray_plane.
bool ray_plane(const LLVector3 &ray_point, const LLVector3 &ray_direction,
               const LLVector3 &plane_point, const LLVector3 plane_normal,
               LLVector3 &intersection);

// point_0 through point_2 define the plane_normal via the right-hand rule:
// circle from point_0 to point_2 with fingers ==> thumb points in direction of normal
bool ray_triangle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
                  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
                  LLVector3 &intersection, LLVector3 &intersection_normal);

bool ray_sphere(const LLVector3 &ray_point, const LLVector3 &ray_direction,
                const LLVector3 &sphere_center, F32 sphere_radius,
                LLVector3 &intersection, LLVector3 &intersection_normal);

bool ray_tetrahedron(const LLVector3 &ray_point, const LLVector3 &ray_direction,
                     const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
                     LLVector3 &intersection, LLVector3 &intersection_normal);

bool linesegment_sphere(const LLVector3 &point_a, const LLVector3 &point_b,
                const LLVector3 &sphere_center, F32 sphere_radius,
                LLVector3 &intersection, LLVector3 &intersection_normal);

bool linesegment_tetrahedron(const LLVector3 &point_a, const LLVector3 &point_b,
                             const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
                             LLVector3 &intersection, LLVector3 &intersection_normal);
