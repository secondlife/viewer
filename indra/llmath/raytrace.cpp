/** 
 * @file raytrace.cpp
 * @brief Functions called by box object scripts.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "math.h"
//#include "vmath.h"
#include "v3math.h"
#include "llquaternion.h"
#include "m3math.h"
#include "raytrace.h"


BOOL line_plane(const LLVector3 &line_point, const LLVector3 &line_direction,
				const LLVector3 &plane_point, const LLVector3 plane_normal, 
				LLVector3 &intersection)
{
	F32 N = line_direction * plane_normal;
	if (0.0f == N)
	{
		// line is perpendicular to plane normal
		// so it is either entirely on plane, or not on plane at all
		return FALSE;
	}
	// Ax + By, + Cz + D = 0
	// D = - (plane_point * plane_normal)
	// N = line_direction * plane_normal
	// intersection = line_point - ((D + plane_normal * line_point) / N) * line_direction
	intersection = line_point - ((plane_normal * line_point - plane_point * plane_normal) / N) * line_direction;
	return TRUE;
}


BOOL ray_plane(const LLVector3 &ray_point, const LLVector3 &ray_direction,
			   const LLVector3 &plane_point, const LLVector3 plane_normal, 
			   LLVector3 &intersection)
{
	F32 N = ray_direction * plane_normal;
	if (0.0f == N)
	{
		// ray is perpendicular to plane normal
		// so it is either entirely on plane, or not on plane at all
		return FALSE;
	}
	// Ax + By, + Cz + D = 0
	// D = - (plane_point * plane_normal)
	// N = ray_direction * plane_normal
	// intersection = ray_point - ((D + plane_normal * ray_point) / N) * ray_direction
	F32 alpha = -(plane_normal * ray_point - plane_point * plane_normal) / N;
	if (alpha < 0.0f)
	{
		// ray points away from plane
		return FALSE;
	}
	intersection = ray_point + alpha * ray_direction;
	return TRUE;
}


BOOL ray_circle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				const LLVector3 &circle_center, const LLVector3 plane_normal, F32 circle_radius,
				LLVector3 &intersection)
{
	if (ray_plane(ray_point, ray_direction, circle_center, plane_normal, intersection))
	{
		if (circle_radius >= (intersection - circle_center).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL ray_triangle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2, 
				  LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 side_01 = point_1 - point_0;
	LLVector3 side_12 = point_2 - point_1;

	intersection_normal = side_01 % side_12;
	intersection_normal.normVec();

	if (ray_plane(ray_point, ray_direction, point_0, intersection_normal, intersection))
	{
		LLVector3 side_20 = point_0 - point_2;
		if (intersection_normal * (side_01 % (intersection - point_0)) >= 0.0f  &&
			intersection_normal * (side_12 % (intersection - point_1)) >= 0.0f  &&
			intersection_normal * (side_20 % (intersection - point_2)) >= 0.0f)
		{
			return TRUE;
		}
	}
	return FALSE;
}


// assumes a parallelogram
BOOL ray_quadrangle(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
					LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 side_01 = point_1 - point_0;
	LLVector3 side_12 = point_2 - point_1;

	intersection_normal = side_01 % side_12;
	intersection_normal.normVec();

	if (ray_plane(ray_point, ray_direction, point_0, intersection_normal, intersection))
	{
		LLVector3 point_3 = point_0 + (side_12);
		LLVector3 side_23 = point_3 - point_2;
		LLVector3 side_30 = point_0 - point_3;
		if (intersection_normal * (side_01 % (intersection - point_0)) >= 0.0f  &&
			intersection_normal * (side_12 % (intersection - point_1)) >= 0.0f  &&
			intersection_normal * (side_23 % (intersection - point_2)) >= 0.0f  &&
			intersection_normal * (side_30 % (intersection - point_3)) >= 0.0f)
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL ray_sphere(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				const LLVector3 &sphere_center, F32 sphere_radius,
				LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_to_sphere = sphere_center - ray_point;
	F32 dot = ray_to_sphere * ray_direction;

	LLVector3 closest_approach = dot * ray_direction - ray_to_sphere;

	F32 shortest_distance = closest_approach.magVecSquared();
	F32 radius_squared = sphere_radius * sphere_radius;
	if (shortest_distance > radius_squared)
	{
		return FALSE;
	}

	F32 half_chord = (F32) sqrt(radius_squared - shortest_distance);
	closest_approach = sphere_center + closest_approach;			// closest_approach now in absolute coordinates
	intersection = closest_approach + half_chord * ray_direction;
	dot = ray_direction * (intersection - ray_point);
	if (dot < 0.0f)
	{
		// ray shoots away from sphere and is not inside it
		return FALSE;
	}

	shortest_distance = ray_direction * ((closest_approach - half_chord * ray_direction) - ray_point);
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
		intersection_normal.setVec(0.0f, 0.0f, 0.0f);
	}
	
	return TRUE;
}


BOOL ray_cylinder(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				  const LLVector3 &cyl_center, const LLVector3 &cyl_scale, const LLQuaternion &cyl_rotation,
				  LLVector3 &intersection, LLVector3 &intersection_normal)
{
	// calculate the centers of the cylinder caps in the absolute frame
	LLVector3 cyl_top(0.0f, 0.0f, 0.5f * cyl_scale.mV[VZ]);
	LLVector3 cyl_bottom(0.0f, 0.0f, -cyl_top.mV[VZ]);
	cyl_top = (cyl_top * cyl_rotation) + cyl_center;
	cyl_bottom = (cyl_bottom * cyl_rotation) + cyl_center;

	// we only handle cylinders with circular cross-sections at the moment
	F32 cyl_radius = 0.5f * llmax(cyl_scale.mV[VX], cyl_scale.mV[VY]);	// HACK until scaled cylinders are supported

	// This implementation is based on the intcyl() function from Graphics_Gems_IV, page 361
	LLVector3   cyl_axis;								// axis direction (bottom toward top)
	LLVector3 	ray_to_cyl;								// ray_point to cyl_top
	F32 		shortest_distance;						// shortest distance from ray to axis
	F32 		cyl_length;
	LLVector3	shortest_direction;
	LLVector3	temp_vector;

	cyl_axis = cyl_bottom - cyl_top;
	cyl_length = cyl_axis.normVec();
	ray_to_cyl = ray_point - cyl_bottom;
	shortest_direction = ray_direction % cyl_axis;
	shortest_distance = shortest_direction.normVec();	// recycle shortest_distance

	// check for ray parallel to cylinder axis
	if (0.0f == shortest_distance)
	{
		// ray is parallel to cylinder axis
		temp_vector = ray_to_cyl - (ray_to_cyl * cyl_axis) * cyl_axis;
		shortest_distance = temp_vector.magVec();
		if (shortest_distance <= cyl_radius)
		{
			shortest_distance = ray_to_cyl * cyl_axis;
			F32 dot = ray_direction * cyl_axis;

			if (shortest_distance > 0.0)
			{
			   	if (dot > 0.0f)
				{
					// ray points away from cylinder bottom
					return FALSE;
				}
				// ray hit bottom of cylinder from outside 
				intersection = ray_point - shortest_distance * cyl_axis;
				intersection_normal = cyl_axis;

			}
			else if (shortest_distance > -cyl_length)
			{
				// ray starts inside cylinder
				if (dot < 0.0f)
				{
					// ray hit top from inside
					intersection = ray_point - (cyl_length + shortest_distance) * cyl_axis;
					intersection_normal = -cyl_axis;
				}
				else
				{
					// ray hit bottom from inside
					intersection = ray_point - shortest_distance * cyl_axis;
					intersection_normal = cyl_axis;
				}
			}
			else
			{
				if (dot < 0.0f)
				{
					// ray points away from cylinder bottom
					return FALSE;
				}
				// ray hit top from outside
				intersection = ray_point - (shortest_distance + cyl_length) * cyl_axis;
				intersection_normal = -cyl_axis;
			}
			return TRUE;
		}
		return FALSE;
	}

	// check for intersection with infinite cylinder
	shortest_distance = (F32) fabs(ray_to_cyl * shortest_direction);
	if (shortest_distance <= cyl_radius)
	{
		F32 		dist_to_closest_point;				// dist from ray_point to closest_point
		F32 		half_chord_length;					// half length of intersection chord
		F32 		in, out;							// distances to entering/exiting points
		temp_vector = ray_to_cyl % cyl_axis;
		dist_to_closest_point = - (temp_vector * shortest_direction);
		temp_vector = shortest_direction % cyl_axis;
		temp_vector.normVec();
		half_chord_length = (F32) fabs( sqrt(cyl_radius*cyl_radius - shortest_distance * shortest_distance) /
							(ray_direction * temp_vector) );

		out = dist_to_closest_point + half_chord_length;	// dist to exiting point
		if (out < 0.0f)
		{
			// cylinder is behind the ray, so we return FALSE
			return FALSE;
		}

		in = dist_to_closest_point - half_chord_length;		// dist to entering point
		if (in < 0.0f)
		{
			// ray_point is inside the cylinder
			// so we store the exiting intersection
			intersection = ray_point + out * ray_direction;
			shortest_distance = out;
		}
		else
		{
			// ray hit cylinder from outside
			// so we store the entering intersection
			intersection = ray_point + in * ray_direction;
			shortest_distance = in;
		}

		// calculate the normal at intersection
		if (0.0f == cyl_radius)
		{
			intersection_normal.setVec(0.0f, 0.0f, 0.0f);
		}
		else
		{
			temp_vector = intersection - cyl_bottom;	
			intersection_normal = temp_vector - (temp_vector * cyl_axis) * cyl_axis;
			intersection_normal.normVec();
		}

		// check for intersection with end caps
		// calculate intersection of ray and top plane
		if (line_plane(ray_point, ray_direction, cyl_top, -cyl_axis, temp_vector))	// NOTE side-effect: changing temp_vector
		{
			shortest_distance = (temp_vector - ray_point).magVec();
			if ( (ray_direction * cyl_axis) > 0.0f)
			{
				// ray potentially enters the cylinder at top
				if (shortest_distance > out)
				{
					// ray missed the finite cylinder
					return FALSE;
				}
				if (shortest_distance > in)
				{
					// ray intersects cylinder at top plane
					intersection = temp_vector;
					intersection_normal = -cyl_axis;
					return TRUE;
				}
			}
			else
			{
				// ray potentially exits the cylinder at top
				if (shortest_distance < in)
				{
					// missed the finite cylinder
					return FALSE;
				}
			}

			// calculate intersection of ray and bottom plane
			line_plane(ray_point, ray_direction, cyl_bottom, cyl_axis, temp_vector); // NOTE side-effect: changing temp_vector
			shortest_distance = (temp_vector - ray_point).magVec();
			if ( (ray_direction * cyl_axis) < 0.0)
			{
				// ray potentially enters the cylinder at bottom
				if (shortest_distance > out)
				{
					// ray missed the finite cylinder
					return FALSE;
				}
				if (shortest_distance > in)
				{
					// ray intersects cylinder at bottom plane
					intersection = temp_vector;
					intersection_normal = cyl_axis;
					return TRUE;
				}
			}
			else
			{
				// ray potentially exits the cylinder at bottom
				if (shortest_distance < in)
				{
					// ray missed the finite cylinder
					return FALSE;
				}
			}

		}
		else
		{
			// ray is parallel to end cap planes
			temp_vector = cyl_bottom - ray_point;
			shortest_distance = temp_vector * cyl_axis;
			if (shortest_distance < 0.0f  ||  shortest_distance > cyl_length)
			{
				// ray missed finite cylinder
				return FALSE;
			}
		}

		return TRUE;
	}

	return FALSE;
}


U32 ray_box(const LLVector3 &ray_point, const LLVector3 &ray_direction, 
			const LLVector3 &box_center, const LLVector3 &box_scale, const LLQuaternion &box_rotation,
			LLVector3 &intersection, LLVector3 &intersection_normal)
{

	// Need to rotate into box frame
	LLQuaternion into_box_frame(box_rotation);		// rotates things from box frame to absolute
	into_box_frame.conjQuat();						// now rotates things into box frame
	LLVector3 line_point = (ray_point - box_center) * into_box_frame;
	LLVector3 line_direction = ray_direction * into_box_frame;

	// Suppose we have a plane:  Ax + By + Cz + D = 0
	// then, assuming [A, B, C] is a unit vector:
	//
	//    plane_normal = [A, B, C] 
	//    D = - (plane_normal * plane_point)
	// 
	// Suppose we have a line:  X = line_point + alpha * line_direction
	//
	// the intersection of the plane and line determines alpha
	//
	//    alpha = - (D + plane_normal * line_point) / (plane_normal * line_direction)

	LLVector3 line_plane_intersection;

	F32 pointX = line_point.mV[VX];
	F32 pointY = line_point.mV[VY];
	F32 pointZ = line_point.mV[VZ];

	F32 dirX = line_direction.mV[VX];
	F32 dirY = line_direction.mV[VY];
	F32 dirZ = line_direction.mV[VZ];

	// we'll be using the half-scales of the box
	F32 boxX = 0.5f * box_scale.mV[VX];
	F32 boxY = 0.5f * box_scale.mV[VY];
	F32 boxZ = 0.5f * box_scale.mV[VZ];

	// check to see if line_point is OUTSIDE the box
	if (pointX < -boxX ||
		pointX >  boxX ||
		pointY < -boxY ||
		pointY >  boxY ||
		pointZ < -boxZ ||
		pointZ >  boxZ) 
	{
		// -------------- point is OUTSIDE the box ----------------

		// front
		if (pointX > 0.0f  &&  dirX < 0.0f)
		{
			// plane_normal                = [ 1, 0, 0]
			// plane_normal*line_point     = pointX
			// plane_normal*line_direction = dirX
			// D                           = -boxX
			// alpha                       = - (-boxX + pointX) / dirX
			line_plane_intersection = line_point - ((pointX - boxX) / dirX) * line_direction;
			if (line_plane_intersection.mV[VY] <  boxY &&
				line_plane_intersection.mV[VY] > -boxY &&
				line_plane_intersection.mV[VZ] <  boxZ &&
				line_plane_intersection.mV[VZ] > -boxZ )
			{
				intersection = (line_plane_intersection * box_rotation) + box_center;
				intersection_normal = LLVector3(1.0f, 0.0f, 0.0f) * box_rotation;
				return FRONT_SIDE;
			}
		}
	
		// back
		if (pointX < 0.0f  &&  dirX > 0.0f)
		{
			// plane_normal                = [ -1, 0, 0]
			// plane_normal*line_point     = -pX 
			// plane_normal*line_direction = -direction.mV[VX]
			// D                           = -bX
			// alpha                       = - (-bX - pX) / (-dirX)
			line_plane_intersection = line_point - ((boxX + pointX)/ dirX) * line_direction;
			if (line_plane_intersection.mV[VY] <  boxY &&
				line_plane_intersection.mV[VY] > -boxY &&
				line_plane_intersection.mV[VZ] <  boxZ &&
				line_plane_intersection.mV[VZ] > -boxZ )
			{
				intersection = (line_plane_intersection * box_rotation) + box_center;
				intersection_normal = LLVector3(-1.0f, 0.0f, 0.0f) * box_rotation;
				return BACK_SIDE;
			}
		}
	
		// left
		if (pointY > 0.0f  &&  dirY < 0.0f)
		{
			// plane_normal                = [0, 1, 0]
			// plane_normal*line_point     = pointY 
			// plane_normal*line_direction = dirY
			// D                           = -boxY
			// alpha                       = - (-boxY + pointY) / dirY
			line_plane_intersection = line_point + ((boxY - pointY)/dirY) * line_direction;

			if (line_plane_intersection.mV[VX] <  boxX &&
				line_plane_intersection.mV[VX] > -boxX &&
				line_plane_intersection.mV[VZ] <  boxZ &&
				line_plane_intersection.mV[VZ] > -boxZ )
			{
				intersection = (line_plane_intersection * box_rotation) + box_center;
				intersection_normal = LLVector3(0.0f, 1.0f, 0.0f) * box_rotation;
				return LEFT_SIDE;
			}
		}
	
		// right
		if (pointY < 0.0f  &&  dirY > 0.0f)
		{
			// plane_normal                = [0, -1, 0]
			// plane_normal*line_point     = -pointY 
			// plane_normal*line_direction = -dirY
			// D                           = -boxY
			// alpha                       = - (-boxY - pointY) / (-dirY)
			line_plane_intersection = line_point - ((boxY + pointY)/dirY) * line_direction;
			if (line_plane_intersection.mV[VX] <  boxX &&
				line_plane_intersection.mV[VX] > -boxX &&
				line_plane_intersection.mV[VZ] <  boxZ &&
				line_plane_intersection.mV[VZ] > -boxZ )
			{
				intersection = (line_plane_intersection * box_rotation) + box_center;
				intersection_normal = LLVector3(0.0f, -1.0f, 0.0f) * box_rotation;
				return RIGHT_SIDE;
			}
		}
	
		// top
		if (pointZ > 0.0f  &&  dirZ < 0.0f)
		{
			// plane_normal                = [0, 0, 1]
			// plane_normal*line_point     = pointZ 
			// plane_normal*line_direction = dirZ
			// D                           = -boxZ
			// alpha                       = - (-boxZ + pointZ) / dirZ
			line_plane_intersection = line_point - ((pointZ - boxZ)/dirZ) * line_direction;
			if (line_plane_intersection.mV[VX] <  boxX &&
				line_plane_intersection.mV[VX] > -boxX &&
				line_plane_intersection.mV[VY] <  boxY &&
				line_plane_intersection.mV[VY] > -boxY )
			{
				intersection = (line_plane_intersection * box_rotation) + box_center;
				intersection_normal = LLVector3(0.0f, 0.0f, 1.0f) * box_rotation;
				return TOP_SIDE;
			}
		}
	
		// bottom
		if (pointZ < 0.0f  &&  dirZ > 0.0f)
		{
			// plane_normal                = [0, 0, -1]
			// plane_normal*line_point     = -pointZ 
			// plane_normal*line_direction = -dirZ
			// D                           = -boxZ
			// alpha                       = - (-boxZ - pointZ) / (-dirZ)
			line_plane_intersection = line_point - ((boxZ + pointZ)/dirZ) * line_direction;
			if (line_plane_intersection.mV[VX] <  boxX &&
				line_plane_intersection.mV[VX] > -boxX &&
				line_plane_intersection.mV[VY] <  boxY &&
				line_plane_intersection.mV[VY] > -boxY )
			{
				intersection = (line_plane_intersection * box_rotation) + box_center;
				intersection_normal = LLVector3(0.0f, 0.0f, -1.0f) * box_rotation;
				return BOTTOM_SIDE;
			}
		}
		return NO_SIDE;
	}

	// -------------- point is INSIDE the box ----------------

	// front
	if (dirX > 0.0f)
	{
		// plane_normal                = [ 1, 0, 0]
		// plane_normal*line_point     = pointX
		// plane_normal*line_direction = dirX
		// D                           = -boxX
		// alpha                       = - (-boxX + pointX) / dirX
		line_plane_intersection = line_point - ((pointX - boxX) / dirX) * line_direction;
		if (line_plane_intersection.mV[VY] <  boxY &&
			line_plane_intersection.mV[VY] > -boxY &&
			line_plane_intersection.mV[VZ] <  boxZ &&
			line_plane_intersection.mV[VZ] > -boxZ )
		{
			intersection = (line_plane_intersection * box_rotation) + box_center;
			intersection_normal = LLVector3(1.0f, 0.0f, 0.0f) * box_rotation;
			return FRONT_SIDE;
		}
	}

	// back
	if (dirX < 0.0f)
	{
		// plane_normal                = [ -1, 0, 0]
		// plane_normal*line_point     = -pX 
		// plane_normal*line_direction = -direction.mV[VX]
		// D                           = -bX
		// alpha                       = - (-bX - pX) / (-dirX)
		line_plane_intersection = line_point - ((boxX + pointX)/ dirX) * line_direction;
		if (line_plane_intersection.mV[VY] <  boxY &&
			line_plane_intersection.mV[VY] > -boxY &&
			line_plane_intersection.mV[VZ] <  boxZ &&
			line_plane_intersection.mV[VZ] > -boxZ )
		{
			intersection = (line_plane_intersection * box_rotation) + box_center;
			intersection_normal = LLVector3(-1.0f, 0.0f, 0.0f) * box_rotation;
			return BACK_SIDE;
		}
	}

	// left
	if (dirY > 0.0f)
	{
		// plane_normal                = [0, 1, 0]
		// plane_normal*line_point     = pointY 
		// plane_normal*line_direction = dirY
		// D                           = -boxY
		// alpha                       = - (-boxY + pointY) / dirY
		line_plane_intersection = line_point + ((boxY - pointY)/dirY) * line_direction;

		if (line_plane_intersection.mV[VX] <  boxX &&
			line_plane_intersection.mV[VX] > -boxX &&
			line_plane_intersection.mV[VZ] <  boxZ &&
			line_plane_intersection.mV[VZ] > -boxZ )
		{
			intersection = (line_plane_intersection * box_rotation) + box_center;
			intersection_normal = LLVector3(0.0f, 1.0f, 0.0f) * box_rotation;
			return LEFT_SIDE;
		}
	}

	// right
	if (dirY < 0.0f)
	{
		// plane_normal                = [0, -1, 0]
		// plane_normal*line_point     = -pointY 
		// plane_normal*line_direction = -dirY
		// D                           = -boxY
		// alpha                       = - (-boxY - pointY) / (-dirY)
		line_plane_intersection = line_point - ((boxY + pointY)/dirY) * line_direction;
		if (line_plane_intersection.mV[VX] <  boxX &&
			line_plane_intersection.mV[VX] > -boxX &&
			line_plane_intersection.mV[VZ] <  boxZ &&
			line_plane_intersection.mV[VZ] > -boxZ )
		{
			intersection = (line_plane_intersection * box_rotation) + box_center;
			intersection_normal = LLVector3(0.0f, -1.0f, 0.0f) * box_rotation;
			return RIGHT_SIDE;
		}
	}

	// top
	if (dirZ > 0.0f)
	{
		// plane_normal                = [0, 0, 1]
		// plane_normal*line_point     = pointZ 
		// plane_normal*line_direction = dirZ
		// D                           = -boxZ
		// alpha                       = - (-boxZ + pointZ) / dirZ
		line_plane_intersection = line_point - ((pointZ - boxZ)/dirZ) * line_direction;
		if (line_plane_intersection.mV[VX] <  boxX &&
			line_plane_intersection.mV[VX] > -boxX &&
			line_plane_intersection.mV[VY] <  boxY &&
			line_plane_intersection.mV[VY] > -boxY )
		{
			intersection = (line_plane_intersection * box_rotation) + box_center;
			intersection_normal = LLVector3(0.0f, 0.0f, 1.0f) * box_rotation;
			return TOP_SIDE;
		}
	}

	// bottom
	if (dirZ < 0.0f)
	{
		// plane_normal                = [0, 0, -1]
		// plane_normal*line_point     = -pointZ 
		// plane_normal*line_direction = -dirZ
		// D                           = -boxZ
		// alpha                       = - (-boxZ - pointZ) / (-dirZ)
		line_plane_intersection = line_point - ((boxZ + pointZ)/dirZ) * line_direction;
		if (line_plane_intersection.mV[VX] <  boxX &&
			line_plane_intersection.mV[VX] > -boxX &&
			line_plane_intersection.mV[VY] <  boxY &&
			line_plane_intersection.mV[VY] > -boxY )
		{
			intersection = (line_plane_intersection * box_rotation) + box_center;
			intersection_normal = LLVector3(0.0f, 0.0f, -1.0f) * box_rotation;
			return BOTTOM_SIDE;
		}
	}

	// should never get here unless line instersects at tangent point on edge or corner
	// however such cases will be EXTREMELY rare
	return NO_SIDE;
}


BOOL ray_prism(const LLVector3 &ray_point, const LLVector3 &ray_direction,
			   const LLVector3 &prism_center, const LLVector3 &prism_scale, const LLQuaternion &prism_rotation,
			   LLVector3 &intersection, LLVector3 &intersection_normal)
{
	//      (0)              Z  
	//      /| \             .  
	//    (1)|  \           /|\  _.Y
	//     | \   \           |   /|
	//     | |\   \          |  /
	//     | | \(0)\         | / 
	//     | |  \   \        |/
	//     | |   \   \      (*)----> X  
	//     |(3)---\---(2)      
	//     |/      \  /        
	//    (4)-------(5)        

	// need to calculate the points of the prism so we can run ray tests with each face
	F32 x = prism_scale.mV[VX];
	F32 y = prism_scale.mV[VY];
	F32 z = prism_scale.mV[VZ];

	F32 tx = x * 2.0f / 3.0f;
	F32 ty = y * 0.5f;
	F32 tz = z * 2.0f / 3.0f;

	LLVector3 point0(tx-x,  ty, tz);
	LLVector3 point1(tx-x, -ty, tz);
	LLVector3 point2(tx,    ty, tz-z);
	LLVector3 point3(tx-x,  ty, tz-z);
	LLVector3 point4(tx-x, -ty, tz-z);
	LLVector3 point5(tx,   -ty, tz-z);

	// transform these points into absolute frame
	point0 = (point0 * prism_rotation) + prism_center;
	point1 = (point1 * prism_rotation) + prism_center;
	point2 = (point2 * prism_rotation) + prism_center;
	point3 = (point3 * prism_rotation) + prism_center;
	point4 = (point4 * prism_rotation) + prism_center;
	point5 = (point5 * prism_rotation) + prism_center;

	// test ray intersection for each face
	BOOL b_hit = FALSE;
	LLVector3 face_intersection, face_normal;
	F32 distance_squared = 0.0f;
	F32 temp;

	// face 0
	if (ray_direction * ( (point0 - point2) % (point5 - point2)) < 0.0f  && 
		ray_quadrangle(ray_point, ray_direction, point5, point2, point0, intersection, intersection_normal))
	{
		distance_squared = (ray_point - intersection).magVecSquared();
		b_hit = TRUE;
	}

	// face 1
	if (ray_direction * ( (point0 - point3) % (point2 - point3)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point2, point3, point0, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}
	
	// face 2
	if (ray_direction * ( (point1 - point4) % (point3 - point4)) < 0.0f  &&
		ray_quadrangle(ray_point, ray_direction, point3, point4, point1, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}
	
	// face 3
	if (ray_direction * ( (point5 - point4) % (point1 - point4)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point1, point4, point5, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}

	// face 4
	if (ray_direction * ( (point4 - point5) % (point2 - point5)) < 0.0f  &&
		ray_quadrangle(ray_point, ray_direction, point2, point5, point4, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}

	return b_hit;
}


BOOL ray_tetrahedron(const LLVector3 &ray_point, const LLVector3 &ray_direction,
					 const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
					 LLVector3 &intersection, LLVector3 &intersection_normal)
{
	F32 a = 0.5f * F_SQRT3;				// height of unit triangle
	F32 b = 1.0f / F_SQRT3;				// distance of center of unit triangle to each point
	F32 c = F_SQRT2 / F_SQRT3;			// height of unit tetrahedron
	F32 d = 0.5f * F_SQRT3 / F_SQRT2;	// distance of center of tetrahedron to each point

	// if we want the tetrahedron to have unit height (c = 1.0) then we need to divide
	// each constant by hieght of a unit tetrahedron
	F32 oo_c = 1.0f / c;
	a = a * oo_c;
	b = b * oo_c;
	c = 1.0f;
	d = d * oo_c;
	F32 e = 0.5f * oo_c;

	LLVector3 point0(			   0.0f,					0.0f,  t_scale.mV[VZ] * d);
	LLVector3 point1(t_scale.mV[VX] * b,					0.0f,  t_scale.mV[VZ] * (d-c));
	LLVector3 point2(t_scale.mV[VX] * (b-a),  e * t_scale.mV[VY],  t_scale.mV[VZ] * (d-c));
	LLVector3 point3(t_scale.mV[VX] * (b-a), -e * t_scale.mV[VY],  t_scale.mV[VZ] * (d-c));

	// transform these points into absolute frame
	point0 = (point0 * t_rotation) + t_center;
	point1 = (point1 * t_rotation) + t_center;
	point2 = (point2 * t_rotation) + t_center;
	point3 = (point3 * t_rotation) + t_center;

	// test ray intersection for each face
	BOOL b_hit = FALSE;
	LLVector3 face_intersection, face_normal;
	F32 distance_squared = 1.0e12f;
	F32 temp;

	// face 0
	if (ray_direction * ( (point2 - point1) % (point0 - point1)) < 0.0f  && 
		ray_triangle(ray_point, ray_direction, point1, point2, point0, intersection, intersection_normal))
	{
		distance_squared = (ray_point - intersection).magVecSquared();
		b_hit = TRUE;
	}

	// face 1
	if (ray_direction * ( (point3 - point2) % (point0 - point2)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point2, point3, point0, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}
	
	// face 2
	if (ray_direction * ( (point1 - point3) % (point0 - point3)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point3, point1, point0, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}
	
	// face 3
	if (ray_direction * ( (point2 - point3) % (point1 - point3)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point3, point2, point1, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
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
			b_hit = TRUE;
		}
	}

	return b_hit;
}


BOOL ray_pyramid(const LLVector3 &ray_point, const LLVector3 &ray_direction,
				 const LLVector3 &p_center, const LLVector3 &p_scale, const LLQuaternion &p_rotation,
				 LLVector3 &intersection, LLVector3 &intersection_normal)
{
	// center of mass of pyramid is located 1/4 its height from the base
	F32 x = 0.5f * p_scale.mV[VX];
	F32 y = 0.5f * p_scale.mV[VY];
	F32 z = 0.25f * p_scale.mV[VZ];

	LLVector3 point0(0.0f, 0.0f,  p_scale.mV[VZ] - z);
	LLVector3 point1( x,  y, -z);
	LLVector3 point2(-x,  y, -z);
	LLVector3 point3(-x, -y, -z);
	LLVector3 point4( x, -y, -z);

	// transform these points into absolute frame
	point0 = (point0 * p_rotation) + p_center;
	point1 = (point1 * p_rotation) + p_center;
	point2 = (point2 * p_rotation) + p_center;
	point3 = (point3 * p_rotation) + p_center;
	point4 = (point4 * p_rotation) + p_center;

	// test ray intersection for each face
	BOOL b_hit = FALSE;
	LLVector3 face_intersection, face_normal;
	F32 distance_squared = 1.0e12f;
	F32 temp;

	// face 0
	if (ray_direction * ( (point1 - point4) % (point0 - point4)) < 0.0f  && 
		ray_triangle(ray_point, ray_direction, point4, point1, point0, intersection, intersection_normal))
	{
		distance_squared = (ray_point - intersection).magVecSquared();
		b_hit = TRUE;
	}

	// face 1
	if (ray_direction * ( (point2 - point1) % (point0 - point1)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point1, point2, point0, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}
	
	// face 2
	if (ray_direction * ( (point3 - point2) % (point0 - point2)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point2, point3, point0, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}

	// face 3
	if (ray_direction * ( (point4 - point3) % (point0 - point3)) < 0.0f  &&
		ray_triangle(ray_point, ray_direction, point3, point4, point0, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
			if (temp < distance_squared)
			{
				distance_squared = temp;
				intersection = face_intersection;
				intersection_normal = face_normal;
			}
		}
		else 
		{
			distance_squared = (ray_point - face_intersection).magVecSquared();
			intersection = face_intersection;
			intersection_normal = face_normal;
			b_hit = TRUE;
		}
	}
	
	// face 4
	if (ray_direction * ( (point3 - point4) % (point2 - point4)) < 0.0f  &&
		ray_quadrangle(ray_point, ray_direction, point4, point3, point2, face_intersection, face_normal))
	{
		if (TRUE == b_hit)
		{
			temp = (ray_point - face_intersection).magVecSquared();
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
			b_hit = TRUE;
		}
	}

	return b_hit;
}


BOOL linesegment_circle(const LLVector3 &point_a, const LLVector3 &point_b,
						const LLVector3 &circle_center, const LLVector3 plane_normal, F32 circle_radius,
						LLVector3 &intersection)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_circle(point_a, ray_direction, circle_center, plane_normal, circle_radius, intersection))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL linesegment_triangle(const LLVector3 &point_a, const LLVector3 &point_b,
						  const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
						  LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_triangle(point_a, ray_direction, point_0, point_1, point_2, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL linesegment_quadrangle(const LLVector3 &point_a, const LLVector3 &point_b,
							const LLVector3 &point_0, const LLVector3 &point_1, const LLVector3 &point_2,
							LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_quadrangle(point_a, ray_direction, point_0, point_1, point_2, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL linesegment_sphere(const LLVector3 &point_a, const LLVector3 &point_b,
				const LLVector3 &sphere_center, F32 sphere_radius,
				LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_sphere(point_a, ray_direction, sphere_center, sphere_radius, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL linesegment_cylinder(const LLVector3 &point_a, const LLVector3 &point_b,
				  const LLVector3 &cyl_center, const LLVector3 &cyl_scale, const LLQuaternion &cyl_rotation,
				  LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_cylinder(point_a, ray_direction, cyl_center, cyl_scale, cyl_rotation, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


U32 linesegment_box(const LLVector3 &point_a, const LLVector3 &point_b, 
					const LLVector3 &box_center, const LLVector3 &box_scale, const LLQuaternion &box_rotation,
					LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 direction = point_b - point_a;
	if (direction.isNull())
	{
		return NO_SIDE;
	}

	F32 segment_length = direction.normVec();
	U32 box_side = ray_box(point_a, direction, box_center, box_scale, box_rotation, intersection, intersection_normal);
	if (NO_SIDE == box_side  ||  segment_length < (intersection - point_a).magVec())
	{
		return NO_SIDE;
	}

	return box_side;
}


BOOL linesegment_prism(const LLVector3 &point_a, const LLVector3 &point_b,
					   const LLVector3 &prism_center, const LLVector3 &prism_scale, const LLQuaternion &prism_rotation,
					   LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_prism(point_a, ray_direction, prism_center, prism_scale, prism_rotation, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL linesegment_tetrahedron(const LLVector3 &point_a, const LLVector3 &point_b,
							 const LLVector3 &t_center, const LLVector3 &t_scale, const LLQuaternion &t_rotation,
							 LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_tetrahedron(point_a, ray_direction, t_center, t_scale, t_rotation, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}


BOOL linesegment_pyramid(const LLVector3 &point_a, const LLVector3 &point_b,
						 const LLVector3 &p_center, const LLVector3 &p_scale, const LLQuaternion &p_rotation,
						 LLVector3 &intersection, LLVector3 &intersection_normal)
{
	LLVector3 ray_direction = point_b - point_a;
	F32 segment_length = ray_direction.normVec();

	if (ray_pyramid(point_a, ray_direction, p_center, p_scale, p_rotation, intersection, intersection_normal))
	{
		if (segment_length >= (point_a - intersection).magVec())
		{
			return TRUE;
		}
	}
	return FALSE;
}





