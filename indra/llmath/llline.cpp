/**
 * @file llline.cpp
 * @author Andrew Meadows
 * @brief Simple line class that can compute nearest approach between two lines
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llline.h"
#include "llrand.h"

#include <glm/geometric.hpp>

const F32 SOME_VERY_SMALL_NUMBER = 1.0e-8f;

LLLine::LLLine()
:   mPoint(0.f, 0.f, 0.f),
    mDirection(1.f, 0.f, 0.f)
{ }

LLLine::LLLine( const glm::vec3& first_point, const glm::vec3& second_point )
{
    setPoints(first_point, second_point);
}

void LLLine::setPoints( const glm::vec3& first_point, const glm::vec3& second_point )
{
    mPoint = first_point;
    mDirection = glm::normalize(second_point - first_point);
}

void LLLine::setPointDirection( const glm::vec3& first_point, const glm::vec3& second_point )
{
    setPoints(first_point, first_point + second_point);
}

bool LLLine::intersects( const glm::vec3& point, F32 radius ) const
{
    glm::vec3 other_direction = point - mPoint;
    glm::vec3 nearest_point = mPoint + mDirection * glm::dot(other_direction, mDirection);
    F32 nearest_approach = glm::length(nearest_point - point);
    return (nearest_approach <= radius);
}

// returns the point on this line that is closest to some_point
glm::vec3 LLLine::nearestApproach( const glm::vec3& some_point ) const
{
    return (mPoint + mDirection * glm::dot(some_point - mPoint, mDirection));
}

// the accuracy of this method sucks when you give it two nearly
// parallel lines, so you should probably check for parallelism
// before you call this
//
// returns the point on this line that is closest to other_line
glm::vec3 LLLine::nearestApproach( const LLLine& other_line ) const
{
    glm::vec3 between_points = other_line.mPoint - mPoint;
    F32 dir_dot_dir = glm::dot(mDirection, other_line.mDirection);
    F32 one_minus_dir_dot_dir = 1.0f - fabs(dir_dot_dir);
    if ( one_minus_dir_dot_dir < SOME_VERY_SMALL_NUMBER )
    {
#ifdef LL_DEBUG
        LL_WARNS() << "LLLine::nearestApproach() was given two very "
            << "nearly parallel lines dir1 = (" << mDirection.x << "," << mDirection.y << "," << mDirection.z << ")"
            << " dir2 = (" << other_line.mDirection.x << "," << other_line.mDirection.y << "," << other_line.mDirection.z << ")"
            << " with 1-dot_product = " << one_minus_dir_dot_dir << LL_ENDL;
#endif
        // the lines are approximately parallel
        // We shouldn't fall in here because this check should have been made
        // BEFORE this function was called.  We dare not continue with the
        // computations for fear of division by zero, but we have to return
        // something so we return a bogus point -- caller beware.
        return 0.5f * (mPoint + other_line.mPoint);
    }

    F32 odir_dot_bp = glm::dot(other_line.mDirection, between_points);

    F32 numerator = 0;
    F32 denominator = 0;
    for (S32 i=0; i<3; i++)
    {
        F32 factor = dir_dot_dir * other_line.mDirection[i] - mDirection[i];
        numerator += ( between_points[i] - odir_dot_bp * other_line.mDirection[i] ) * factor;
        denominator -= factor * factor;
    }

    F32 length_to_nearest_approach = numerator / denominator;

    return mPoint + length_to_nearest_approach * mDirection;
}

std::ostream& operator<<( std::ostream& output_stream, const LLLine& line )
{
    output_stream << "{point=(" << line.mPoint.x << "," << line.mPoint.y << "," << line.mPoint.z << "),"
                  << "dir=(" << line.mDirection.x << "," << line.mDirection.y << "," << line.mDirection.z << ")}";
    return output_stream;
}


F32 ALMOST_PARALLEL = 0.99f;
F32 TOO_SMALL_FOR_DIVISION = 0.0001f;

// returns 'true' if this line intersects the plane
// on success stores the intersection point in 'result'
bool LLLine::intersectsPlane( glm::vec3& result, const LLLine& plane ) const
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

    F32 plane_dir_dot = glm::dot(plane.mDirection, mDirection);
    if (fabs(plane_dir_dot) < TOO_SMALL_FOR_DIVISION)
    {
        return false;
    }

    F32 plane_dot = glm::dot(plane.mDirection, plane.mPoint);
    F32 length = ( plane_dot - glm::dot(plane.mDirection, mPoint) ) / plane_dir_dot;
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

    F32 dir_dot = fabs(glm::dot(first_plane.mDirection, second_plane.mDirection));
    if (dir_dot > ALMOST_PARALLEL)
    {
        // the planes are nearly parallel
        return false;
    }

    glm::vec3 direction = glm::normalize(glm::cross(first_plane.mDirection, second_plane.mDirection));

    glm::vec3 first_intersection;
    {
        LLLine intersection_line(first_plane);
        intersection_line.mDirection = glm::normalize(glm::cross(direction, first_plane.mDirection));
        intersection_line.intersectsPlane(first_intersection, second_plane);
    }

    result.mPoint = first_intersection;
    result.mDirection = direction;

    return true;
}


