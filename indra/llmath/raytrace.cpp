/**
 * @file raytrace.cpp
 * @brief Functions called by box object scripts.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#include "linden_common.h"

#include "math.h"
#include "v3math.h"
#include "llquaternion.h"
#include "m3math.h"
#include "raytrace.h"

// Pruned 2026-04-09 (glm-quat-phase4-bugs branch) — see raytrace.h for the
// list of functions that survived. Removed: ray_plane (duplicate), ray_circle,
// ray_quadrangle, ray_cylinder, ray_box, ray_prism, ray_pyramid, linesegment_circle,
// linesegment_triangle, linesegment_quadrangle, linesegment_cylinder,
// linesegment_box, linesegment_prism, linesegment_pyramid. None had production
// callers; keeping them around just sustained LLQuaternion API surface that
// blocks the Phase 4 deletion.

bool line_plane(const LLVector3 &line_point, const LLVector3 &line_direction,
                const LLVector3 &plane_point, const LLVector3 plane_normal,
                LLVector3 &intersection)
{
    F32 N = dot(line_direction, plane_normal);
    if (0.0f == N)
    {
        // line is perpendicular to plane normal
        // so it is either entirely on plane, or not on plane at all
        return false;
    }
    // Ax + By, + Cz + D = 0
    // D = - (plane_point * plane_normal)
    // N = line_direction * plane_normal
    // intersection = line_point - ((D + plane_normal * line_point) / N) * line_direction
    intersection = line_point - ((dot(plane_normal, line_point) - dot(plane_point, plane_normal)) / N) * line_direction;
    return true;
}


bool ray_plane(const LLVector3 &ray_point, const LLVector3 &ray_direction,
               const LLVector3 &plane_point, const LLVector3 plane_normal,
               LLVector3 &intersection)
{
    F32 N = dot(ray_direction, plane_normal);
    if (0.0f == N)
    {
        // ray is perpendicular to plane normal
        // so it is either entirely on plane, or not on plane at all
        return false;
    }
    // Ax + By, + Cz + D = 0
    // D = - (plane_point * plane_normal)
    // N = ray_direction * plane_normal
    // intersection = ray_point - ((D + plane_normal * ray_point) / N) * ray_direction
    F32 alpha = -(dot(plane_normal, ray_point) - dot(plane_point, plane_normal)) / N;
    if (alpha < 0.0f)
    {
        // ray points away from plane
        return false;
    }
    intersection = ray_point + alpha * ray_direction;
    return true;
}


bool ray_triangle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
                  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
                  LLVector3 &intersection, LLVector3 &intersection_normal)
{
    LLVector3 side_01 = point_1 - point_0;
    LLVector3 side_12 = point_2 - point_1;

    intersection_normal = cross(side_01, side_12);
    intersection_normal.normalize();

    if (ray_plane(ray_point, ray_direction, point_0, intersection_normal, intersection))
    {
        LLVector3 side_20 = point_0 - point_2;
        if (dot(intersection_normal, cross(side_01, intersection - point_0)) >= 0.0f  &&
            dot(intersection_normal, cross(side_12, intersection - point_1)) >= 0.0f  &&
            dot(intersection_normal, cross(side_20, intersection - point_2)) >= 0.0f)
        {
            return true;
        }
    }
    return false;
}


bool ray_sphere(const LLVector3 &ray_point, const LLVector3 &ray_direction,
                const LLVector3 &sphere_center, F32 sphere_radius,
                LLVector3 &intersection, LLVector3 &intersection_normal)
{
    LLVector3 ray_to_sphere = sphere_center - ray_point;
    F32 ray_dot = dot(ray_to_sphere, ray_direction);

    LLVector3 closest_approach = ray_dot * ray_direction - ray_to_sphere;

    F32 shortest_distance = closest_approach.lengthSquared();
    F32 radius_squared = sphere_radius * sphere_radius;
    if (shortest_distance > radius_squared)
    {
        return false;
    }

    F32 half_chord = static_cast<F32>(sqrt(radius_squared - shortest_distance));
    closest_approach = sphere_center + closest_approach;            // closest_approach now in absolute coordinates
    intersection = closest_approach + half_chord * ray_direction;
    ray_dot = dot(ray_direction, intersection - ray_point);
    if (ray_dot < 0.0f)
    {
        // ray shoots away from sphere and is not inside it
        return false;
    }

    shortest_distance = dot(ray_direction, (closest_approach - half_chord * ray_direction) - ray_point);
    if (shortest_distance > 0.0f)
    {
        // ray enters sphere
        intersection = intersection - (2.0f * half_chord) * ray_direction;
    }
    else
    {
        // do nothing
        // ray starts inside sphere and intersects as it leaves the sphere
    }

    intersection_normal = intersection - sphere_center;
    if (sphere_radius > 0.0f)
    {
        intersection_normal *= 1.0f / sphere_radius;
    }
    else
    {
        intersection_normal.set(0.0f, 0.0f, 0.0f);
    }

    return true;
}


bool ray_tetrahedron(const LLVector3 &ray_point, const LLVector3 &ray_direction,
                     const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
                     LLVector3 &intersection, LLVector3 &intersection_normal)
{
    F32 a = 0.5f * F_SQRT3;             // height of unit triangle
    F32 b = 1.0f / F_SQRT3;             // distance of center of unit triangle to each point
    F32 c = F_SQRT2 / F_SQRT3;          // height of unit tetrahedron
    F32 d = 0.5f * F_SQRT3 / F_SQRT2;   // distance of center of tetrahedron to each point

    // if we want the tetrahedron to have unit height (c = 1.0) then we need to divide
    // each constant by hieght of a unit tetrahedron
    F32 oo_c = 1.0f / c;
    a = a * oo_c;
    b = b * oo_c;
    c = 1.0f;
    d = d * oo_c;
    F32 e = 0.5f * oo_c;

    LLVector3 point0(              0.0f,                    0.0f,  t_scale.mV[VZ] * d);
    LLVector3 point1(t_scale.mV[VX] * b,                    0.0f,  t_scale.mV[VZ] * (d-c));
    LLVector3 point2(t_scale.mV[VX] * (b-a),  e * t_scale.mV[VY],  t_scale.mV[VZ] * (d-c));
    LLVector3 point3(t_scale.mV[VX] * (b-a), -e * t_scale.mV[VY],  t_scale.mV[VZ] * (d-c));

    // transform these points into absolute frame
    point0 = (point0 * t_rotation) + t_center;
    point1 = (point1 * t_rotation) + t_center;
    point2 = (point2 * t_rotation) + t_center;
    point3 = (point3 * t_rotation) + t_center;

    // test ray intersection for each face
    bool b_hit = false;
    LLVector3 face_intersection, face_normal;
    F32 distance_squared = 1.0e12f;
    F32 temp;

    // face 0
    if (dot(ray_direction, cross(point2 - point1, point0 - point1)) < 0.0f  &&
        ray_triangle(ray_point, ray_direction, point1, point2, point0, intersection, intersection_normal))
    {
        distance_squared = (ray_point - intersection).lengthSquared();
        b_hit = true;
    }

    // face 1
    if (dot(ray_direction, cross(point3 - point2, point0 - point2)) < 0.0f  &&
        ray_triangle(ray_point, ray_direction, point2, point3, point0, face_intersection, face_normal))
    {
        if (b_hit)
        {
            temp = (ray_point - face_intersection).lengthSquared();
            if (temp < distance_squared)
            {
                distance_squared = temp;
                intersection = face_intersection;
                intersection_normal = face_normal;
            }
        }
        else
        {
            distance_squared = (ray_point - face_intersection).lengthSquared();
            intersection = face_intersection;
            intersection_normal = face_normal;
            b_hit = true;
        }
    }

    // face 2
    if (dot(ray_direction, cross(point1 - point3, point0 - point3)) < 0.0f  &&
        ray_triangle(ray_point, ray_direction, point3, point1, point0, face_intersection, face_normal))
    {
        if (b_hit)
        {
            temp = (ray_point - face_intersection).lengthSquared();
            if (temp < distance_squared)
            {
                distance_squared = temp;
                intersection = face_intersection;
                intersection_normal = face_normal;
            }
        }
        else
        {
            distance_squared = (ray_point - face_intersection).lengthSquared();
            intersection = face_intersection;
            intersection_normal = face_normal;
            b_hit = true;
        }
    }

    // face 3
    if (dot(ray_direction, cross(point2 - point3, point1 - point3)) < 0.0f  &&
        ray_triangle(ray_point, ray_direction, point3, point2, point1, face_intersection, face_normal))
    {
        if (b_hit)
        {
            temp = (ray_point - face_intersection).lengthSquared();
            if (temp < distance_squared)
            {
                intersection = face_intersection;
                intersection_normal = face_normal;
            }
        }
        else
        {
            intersection = face_intersection;
            intersection_normal = face_normal;
            b_hit = true;
        }
    }

    return b_hit;
}


bool linesegment_sphere(const LLVector3 &point_a, const LLVector3 &point_b,
                const LLVector3 &sphere_center, F32 sphere_radius,
                LLVector3 &intersection, LLVector3 &intersection_normal)
{
    LLVector3 ray_direction = point_b - point_a;
    F32 segment_length = ray_direction.normalize();

    if (ray_sphere(point_a, ray_direction, sphere_center, sphere_radius, intersection, intersection_normal))
    {
        if (segment_length >= (point_a - intersection).length())
        {
            return true;
        }
    }
    return false;
}


bool linesegment_tetrahedron(const LLVector3 &point_a, const LLVector3 &point_b,
                             const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
                             LLVector3 &intersection, LLVector3 &intersection_normal)
{
    LLVector3 ray_direction = point_b - point_a;
    F32 segment_length = ray_direction.normalize();

    if (ray_tetrahedron(point_a, ray_direction, t_center, t_scale, t_rotation, intersection, intersection_normal))
    {
        if (segment_length >= (point_a - intersection).length())
        {
            return true;
        }
    }
    return false;
}
