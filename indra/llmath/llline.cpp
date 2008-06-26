/** 
 * @file llline.cpp
 * @author Andrew Meadows
 * @brief Simple line class that can compute nearest approach between two lines
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "linden_common.h"

#include "llline.h"
#include "llrand.h"

const F32 SOME_SMALL_NUMBER = 1.0e-5f;
const F32 SOME_VERY_SMALL_NUMBER = 1.0e-8f;

LLLine::LLLine()
:	mPoint(0.f, 0.f, 0.f),
	mDirection(1.f, 0.f, 0.f)
{ }

LLLine::LLLine( const LLVector3& first_point, const LLVector3& second_point )
{
	setPoints(first_point, second_point);
}

void LLLine::setPoints( const LLVector3& first_point, const LLVector3& second_point )
{
	mPoint = first_point;
	mDirection = second_point - first_point;
	mDirection.normalize();
}

void LLLine::setPointDirection( const LLVector3& first_point, const LLVector3& second_point )
{
	setPoints(first_point, first_point + second_point);
}

bool LLLine::intersects( const LLVector3& point, F32 radius ) const
{
	LLVector3 other_direction = point - mPoint;
	LLVector3 nearest_point = mPoint + mDirection * (other_direction * mDirection);
	F32 nearest_approach = (nearest_point - point).length();
	return (nearest_approach <= radius);
}

// returns the point on this line that is closest to some_point
LLVector3 LLLine::nearestApproach( const LLVector3& some_point ) const
{
	return (mPoint + mDirection * ((some_point - mPoint) * mDirection));
}

// the accuracy of this method sucks when you give it two nearly
// parallel lines, so you should probably check for parallelism
// before you call this
// 
// returns the point on this line that is closest to other_line
LLVector3 LLLine::nearestApproach( const LLLine& other_line ) const
{
	LLVector3 between_points = other_line.mPoint - mPoint;
	F32 dir_dot_dir = mDirection * other_line.mDirection;
	F32 one_minus_dir_dot_dir = 1.0f - fabs(dir_dot_dir);
	if ( one_minus_dir_dot_dir < SOME_VERY_SMALL_NUMBER )
	{
#ifdef LL_DEBUG
		llwarns << "LLLine::nearestApproach() was given two very "
			<< "nearly parallel lines dir1 = " << mDirection 
			<< " dir2 = " << other_line.mDirection << " with 1-dot_product = " 
			<< one_minus_dir_dot_dir << llendl;
#endif
		// the lines are approximately parallel
		// We shouldn't fall in here because this check should have been made
		// BEFORE this function was called.  We dare not continue with the 
		// computations for fear of division by zero, but we have to return 
		// something so we return a bogus point -- caller beware.
		return 0.5f * (mPoint + other_line.mPoint);
	}

	F32 odir_dot_bp = other_line.mDirection * between_points;

	F32 numerator = 0;
	F32 denominator = 0;
	for (S32 i=0; i<3; i++)
	{
		F32 factor = dir_dot_dir * other_line.mDirection.mV[i] - mDirection.mV[i];
		numerator += ( between_points.mV[i] - odir_dot_bp * other_line.mDirection.mV[i] ) * factor;
		denominator -= factor * factor;
	}

	F32 length_to_nearest_approach = numerator / denominator;

	return mPoint + length_to_nearest_approach * mDirection;
}

std::ostream& operator<<( std::ostream& output_stream, const LLLine& line )
{
	output_stream << "{point=" << line.mPoint << "," << "dir=" << line.mDirection << "}";
	return output_stream;
}


F32 ALMOST_PARALLEL = 0.99f;
F32 TOO_SMALL_FOR_DIVISION = 0.0001f;

// returns 'true' if this line intersects the plane
// on success stores the intersection point in 'result'
bool LLLine::intersectsPlane( LLVector3& result, const LLLine& plane ) const
{
	// p = P + l * d     equation for a line
	// 
	// N * p = D         equation for a point
	//
	// N * (P + l * d) = D
	// N*P + l * (N*d) = D
	// l * (N*d) = D - N*P
	// l =  ( D - N*P ) / ( N*d )
	//

	F32 dot = plane.mDirection * mDirection;
	if (fabs(dot) < TOO_SMALL_FOR_DIVISION)
	{
		return false;
	}

	F32 plane_dot = plane.mDirection * plane.mPoint;
	F32 length = ( plane_dot - (plane.mDirection * mPoint) ) / dot;
	result = mPoint + length * mDirection;
	return true;
}

//static 
// returns 'true' if planes intersect, and stores the result 
// the second and third arguments are treated as planes
// where mPoint is on the plane and mDirection is the normal
// result.mPoint will be the intersection line's closest approach 
// to first_plane.mPoint
bool LLLine::getIntersectionBetweenTwoPlanes( LLLine& result, const LLLine& first_plane, const LLLine& second_plane )
{
	// TODO -- if we ever get some generic matrix solving code in our libs
	// then we should just use that, since this problem is really just
	// linear algebra.

	F32 dot = fabs(first_plane.mDirection * second_plane.mDirection);
	if (dot > ALMOST_PARALLEL)
	{
		// the planes are nearly parallel
		return false;
	}

	LLVector3 direction = first_plane.mDirection % second_plane.mDirection;
	direction.normalize();

	LLVector3 first_intersection;
	{
		LLLine intersection_line(first_plane);
		intersection_line.mDirection = direction % first_plane.mDirection;
		intersection_line.mDirection.normalize();
		intersection_line.intersectsPlane(first_intersection, second_plane);
	}

	/*
	LLVector3 second_intersection;
	{
		LLLine intersection_line(second_plane);
		intersection_line.mDirection = direction % second_plane.mDirection;
		intersection_line.mDirection.normalize();
		intersection_line.intersectsPlane(second_intersection, first_plane);
	}
	*/

	result.mPoint = first_intersection;
	result.mDirection = direction;

	return true;
}


