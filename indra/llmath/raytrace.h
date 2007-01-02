/** 
 * @file raytrace.h
 * @brief Ray intersection tests for primitives.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_RAYTRACE_H
#define LL_RAYTRACE_H

class LLVector3;
class LLQuaternion;

// All functions produce results in the same reference frame as the arguments.
//
// Any arguments of the form "foo_direction" or "foo_normal" are assumed to
// be normalized, or normalized vectors are stored in them.
//
// Vector arguments of the form "shape_scale" represent the scale of the
// object along the three axes.
//
// All functions return the expected TRUE or FALSE, unless otherwise noted.
// When FALSE is returned, any resulting values that might have been stored 
// are undefined.
//
// Rays are defined by a "ray_point" and a "ray_direction" (unit).
//
// Lines are defined by a "line_point" and a "line_direction" (unit).
//
// Line segements are defined by "point_a" and "point_b", and for intersection
// purposes are assumed to point from "point_a" to "point_b".
//
// A ray is different from a line in that it starts at a point and extends
// in only one direction.
//
// Intersection normals always point outside the object, normal to the object's
// surface at the point of intersection.
//
// Object rotations passed as quaternions are expected to rotate from the 
// object's local frame to the absolute frame.  So, if "foo" is a vector in
// the object's local frame, then "foo * object_rotation" is in the absolute
// frame.


// returns TRUE iff line is not parallel to plane.
BOOL line_plane(const LLVector3 &line_point, const LLVector3 &line_direction, 
				const LLVector3 &plane_point, const LLVector3 plane_normal, 
				LLVector3 &intersection);


// returns TRUE iff line is not parallel to plane.
BOOL ray_plane(const LLVector3 &ray_point, const LLVector3 &ray_direction, 
			   const LLVector3 &plane_point, const LLVector3 plane_normal, 
			   LLVector3 &intersection);


BOOL ray_circle(const LLVector3 &ray_point, const LLVector3 &ray_direction, 
				const LLVector3 &circle_center, const LLVector3 plane_normal, F32 circle_radius,
				LLVector3 &intersection);

// point_0 through point_2 define the plane_normal via the right-hand rule:
// circle from point_0 to point_2 with fingers ==> thumb points in direction of normal
BOOL ray_triangle(const LLVector3 &ray_point, const LLVector3 &ray_direction, 
				  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2, 
				  LLVector3 &intersection, LLVector3 &intersection_normal);


// point_0 is the lower-left corner, point_1 is the lower-right, point_2 is the upper-right
// right-hand-rule... curl fingers from lower-left toward lower-right then toward upper-right
// ==> thumb points in direction of normal
// assumes a parallelogram, so point_3 is determined by the other points
BOOL ray_quadrangle(const LLVector3 &ray_point, const LLVector3 &ray_direction, 
					const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2, 
					LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_sphere(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				const LLVector3 &sphere_center, F32 sphere_radius,
				LLVector3 &intersection, LLVector3 &intersection_normal);


// finite right cylinder is defined by end centers: "cyl_top", "cyl_bottom", 
// and by the cylinder radius "cyl_radius"
BOOL ray_cylinder(const LLVector3 &ray_point, const LLVector3 &ray_direction,
		          const LLVector3 &cyl_center, const LLVector3 &cyl_scale, const LLQuaternion &cyl_rotation,
				  LLVector3 &intersection, LLVector3 &intersection_normal);


// this function doesn't just return a BOOL because the return is currently
// used to decide how to break up boxes that have been hit by shots... 
// a hack that will probably be changed later
//
// returns a number representing the side of the box that was hit by the ray,
// or NO_SIDE if intersection test failed.
U32 ray_box(const LLVector3 &ray_point, const LLVector3 &ray_direction,
		    const LLVector3 &box_center, const LLVector3 &box_scale, const LLQuaternion &box_rotation,
			LLVector3 &intersection, LLVector3 &intersection_normal);


/* TODO
BOOL ray_ellipsoid(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				   const LLVector3 &e_center, const LLVector3 &e_scale, const LLQuaternion &e_rotation,
				   LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_cone(const LLVector3 &ray_point, const LLVector3 &ray_direction,
			  const LLVector3 &cone_tip, const LLVector3 &cone_bottom, 
			  const LLVector3 &cone_scale, const LLQuaternion &cone_rotation,
			  LLVector3 &intersection, LLVector3 &intersection_normal);
*/


BOOL ray_prism(const LLVector3 &ray_point, const LLVector3 &ray_direction,
			   const LLVector3 &prism_center, const LLVector3 &prism_scale, const LLQuaternion &prism_rotation,
			   LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_tetrahedron(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					 const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
					 LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_pyramid(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				 const LLVector3 &p_center, const LLVector3 &p_scale, const LLQuaternion &p_rotation,
				 LLVector3 &intersection, LLVector3 &intersection_normal);



/* TODO
BOOL ray_hemiellipsoid(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					   const LLVector3 &e_center, const LLVector3 &e_scale, const LLQuaternion &e_rotation,
					   const LLVector3 &e_cut_normal,
					   LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_hemisphere(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					const LLVector3 &sphere_center, F32 sphere_radius, const LLVector3 &sphere_cut_normal, 
					LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_hemicylinder(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					  const LLVector3 &cyl_top, const LLVector3 &cyl_bottom, F32 cyl_radius, 
					  const LLVector3 &cyl_cut_normal,
					  LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL ray_hemicone(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				  const LLVector3 &cone_tip, const LLVector3 &cone_bottom, 
				  const LLVector3 &cone_scale, const LLVector3 &cyl_cut_normal,
				  LLVector3 &intersection, LLVector3 &intersection_normal);
*/


BOOL linesegment_circle(const LLVector3 &point_a, const LLVector3 &point_b, 
						const LLVector3 &circle_center, const LLVector3 plane_normal, F32 circle_radius,
						LLVector3 &intersection);

// point_0 through point_2 define the plane_normal via the right-hand rule:
// circle from point_0 to point_2 with fingers ==> thumb points in direction of normal
BOOL linesegment_triangle(const LLVector3 &point_a, const LLVector3 &point_b, 
						  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2, 
						  LLVector3 &intersection, LLVector3 &intersection_normal);


// point_0 is the lower-left corner, point_1 is the lower-right, point_2 is the upper-right
// right-hand-rule... curl fingers from lower-left toward lower-right then toward upper-right
// ==> thumb points in direction of normal
// assumes a parallelogram, so point_3 is determined by the other points
BOOL linesegment_quadrangle(const LLVector3 &point_a, const LLVector3 &point_b, 
							const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2, 
							LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL linesegment_sphere(const LLVector3 &point_a, const LLVector3 &point_b,
				const LLVector3 &sphere_center, F32 sphere_radius,
				LLVector3 &intersection, LLVector3 &intersection_normal);


// finite right cylinder is defined by end centers: "cyl_top", "cyl_bottom", 
// and by the cylinder radius "cyl_radius"
BOOL linesegment_cylinder(const LLVector3 &point_a, const LLVector3 &point_b,
						  const LLVector3 &cyl_center, const LLVector3 &cyl_scale, const LLQuaternion &cyl_rotation,
						  LLVector3 &intersection, LLVector3 &intersection_normal);


// this function doesn't just return a BOOL because the return is currently
// used to decide how to break up boxes that have been hit by shots... 
// a hack that will probably be changed later
//
// returns a number representing the side of the box that was hit by the ray,
// or NO_SIDE if intersection test failed.
U32 linesegment_box(const LLVector3 &point_a, const LLVector3 &point_b, 
					const LLVector3 &box_center, const LLVector3 &box_scale, const LLQuaternion &box_rotation,
					LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL linesegment_prism(const LLVector3 &point_a, const LLVector3 &point_b,
					   const LLVector3 &prism_center, const LLVector3 &prism_scale, const LLQuaternion &prism_rotation,
					   LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL linesegment_tetrahedron(const LLVector3 &point_a, const LLVector3 &point_b,
							 const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
							 LLVector3 &intersection, LLVector3 &intersection_normal);


BOOL linesegment_pyramid(const LLVector3 &point_a, const LLVector3 &point_b,
						 const LLVector3 &p_center, const LLVector3 &p_scale, const LLQuaternion &p_rotation,
						 LLVector3 &intersection, LLVector3 &intersection_normal);


#endif

