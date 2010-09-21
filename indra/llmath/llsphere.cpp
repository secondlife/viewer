/** 
 * @file llsphere.cpp
 * @author Andrew Meadows
 * @brief Simple line class that can compute nearest approach between two lines
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llsphere.h"

LLSphere::LLSphere()
:	mCenter(0.f, 0.f, 0.f),
	mRadius(0.f)
{ }

LLSphere::LLSphere( const LLVector3& center, F32 radius)
{
	set(center, radius);
}

void LLSphere::set( const LLVector3& center, F32 radius )
{
	mCenter = center;
	setRadius(radius);
}

void LLSphere::setCenter( const LLVector3& center)
{
	mCenter = center;
}

void LLSphere::setRadius( F32 radius)
{
	if (radius < 0.f)
	{
		radius = -radius;
	}
	mRadius = radius;
}
	
const LLVector3& LLSphere::getCenter() const
{
	return mCenter;
}

F32 LLSphere::getRadius() const
{
	return mRadius;
}

// returns 'TRUE' if this sphere completely contains other_sphere
BOOL LLSphere::contains(const LLSphere& other_sphere) const
{
	F32 separation = (mCenter - other_sphere.mCenter).length();
	return (mRadius >= separation + other_sphere.mRadius) ? TRUE : FALSE;
}

// returns 'TRUE' if this sphere completely contains other_sphere
BOOL LLSphere::overlaps(const LLSphere& other_sphere) const
{
	F32 separation = (mCenter - other_sphere.mCenter).length();
	return (separation <= mRadius + other_sphere.mRadius) ? TRUE : FALSE;
}

// returns overlap
// negative overlap is closest approach
F32 LLSphere::getOverlap(const LLSphere& other_sphere) const
{
	// separation is distance from other_sphere's edge and this center
	return (mCenter - other_sphere.mCenter).length() - mRadius - other_sphere.mRadius;
}

bool LLSphere::operator==(const LLSphere& rhs) const
{
	// TODO? -- use approximate equality for centers?
	return (mRadius == rhs.mRadius
			&& mCenter == rhs.mCenter);
}

std::ostream& operator<<( std::ostream& output_stream, const LLSphere& sphere)
{
	output_stream << "{center=" << sphere.mCenter << "," << "radius=" << sphere.mRadius << "}";
	return output_stream;
}

// static 
// removes any spheres that are contained in others
void LLSphere::collapse(std::vector<LLSphere>& sphere_list)
{
	std::vector<LLSphere>::iterator first_itr = sphere_list.begin();
	while (first_itr != sphere_list.end())
	{
		bool delete_from_front = false;

		std::vector<LLSphere>::iterator second_itr = first_itr;
		++second_itr;
		while (second_itr != sphere_list.end())
		{
			if (second_itr->contains(*first_itr))
			{
				delete_from_front = true;
				break;
			}
			else if (first_itr->contains(*second_itr))
			{
				sphere_list.erase(second_itr++);
			}
			else
			{
				++second_itr;
			}
		}

		if (delete_from_front)
		{
			sphere_list.erase(first_itr++);
		}
		else
		{
			++first_itr;
		}
	}
}

// static
// returns the bounding sphere that contains both spheres
LLSphere LLSphere::getBoundingSphere(const LLSphere& first_sphere, const LLSphere& second_sphere)
{
	LLVector3 direction = second_sphere.mCenter - first_sphere.mCenter;

	// HACK -- it is possible to get enough floating point error in the 
	// other getBoundingSphere() method that we have to add some slop
	// at the end.  Unfortunately, this breaks the link-order invarience
	// for the linkability tests... unless we also apply the same slop
	// here.
	F32 half_milimeter = 0.0005f;

	F32 distance = direction.length();
	if (0.f == distance)
	{
		direction.setVec(1.f, 0.f, 0.f);
	}
	else
	{
		direction.normVec();
	}
	// the 'edge' is measured from the first_sphere's center
	F32 max_edge = 0.f;
	F32 min_edge = 0.f;

	max_edge = llmax(max_edge + first_sphere.getRadius(), max_edge + distance + second_sphere.getRadius() + half_milimeter);
	min_edge = llmin(min_edge - first_sphere.getRadius(), min_edge + distance - second_sphere.getRadius() - half_milimeter);
	F32 radius = 0.5f * (max_edge - min_edge);
	LLVector3 center = first_sphere.mCenter + (0.5f * (max_edge + min_edge)) * direction;
	return LLSphere(center, radius);
}

// static
// returns the bounding sphere that contains an arbitrary set of spheres
LLSphere LLSphere::getBoundingSphere(const std::vector<LLSphere>& sphere_list)
{
	// this algorithm can get relatively inaccurate when the sphere 
	// collection is 'small' (contained within a bounding sphere of about 
	// 2 meters or less)
	// TODO -- improve the accuracy for small collections of spheres

	LLSphere bounding_sphere( LLVector3(0.f, 0.f, 0.f), 0.f );
	S32 sphere_count = sphere_list.size();
	if (1 == sphere_count)
	{
		// trivial case -- single sphere
		std::vector<LLSphere>::const_iterator sphere_itr = sphere_list.begin();
		bounding_sphere = *sphere_itr;
	}
	else if (2 == sphere_count)
	{
		// trivial case -- two spheres
		std::vector<LLSphere>::const_iterator first_sphere = sphere_list.begin();
		std::vector<LLSphere>::const_iterator second_sphere = first_sphere;
		++second_sphere;
		bounding_sphere = LLSphere::getBoundingSphere(*first_sphere, *second_sphere);
	}
	else if (sphere_count > 0)
	{
		// non-trivial case -- we will approximate the solution
		//
		// NOTE -- there is a fancy/fast way to do this for large 
		// numbers of arbirary N-dimensional spheres -- you can look it
		// up on the net.  We're dealing with 3D spheres at collection
		// sizes of 256 spheres or smaller, so we just use this
		// brute force method.

		// TODO -- perhaps would be worthwile to test for the solution where
		// the largest spanning radius just happens to work.  That is, where
		// there are really two spheres that determine the bounding sphere,
		// and all others are contained therein.

		// compute the AABB
		std::vector<LLSphere>::const_iterator first_itr = sphere_list.begin();
		LLVector3 max_corner = first_itr->getCenter() + first_itr->getRadius() * LLVector3(1.f, 1.f, 1.f);
		LLVector3 min_corner = first_itr->getCenter() - first_itr->getRadius() * LLVector3(1.f, 1.f, 1.f);
		{
			std::vector<LLSphere>::const_iterator sphere_itr = sphere_list.begin();
			for (++sphere_itr; sphere_itr != sphere_list.end(); ++sphere_itr)
			{
				LLVector3 center = sphere_itr->getCenter();
				F32 radius = sphere_itr->getRadius();
				for (S32 i=0; i<3; ++i)
				{
					if (center.mV[i] + radius > max_corner.mV[i])
					{
						max_corner.mV[i] = center.mV[i] + radius;
					}
					if (center.mV[i] - radius < min_corner.mV[i])
					{
						min_corner.mV[i] = center.mV[i] - radius;
					}
				}
			}
		}

		// get the starting center and radius from the AABB
		LLVector3 diagonal = max_corner - min_corner;
		F32 bounding_radius = 0.5f * diagonal.length();
		LLVector3 bounding_center = 0.5f * (max_corner + min_corner);

		// compute the starting step-size
		F32 minimum_radius = 0.5f * llmin(diagonal.mV[VX], llmin(diagonal.mV[VY], diagonal.mV[VZ]));
		F32 step_length = bounding_radius - minimum_radius;
		S32 step_count = 0;
		S32 max_step_count = 12;
		F32 half_milimeter = 0.0005f;

		// wander the center around in search of tighter solutions
		S32 last_dx = 2;	// 2 is out of bounds --> no match
		S32 last_dy = 2;
		S32 last_dz = 2;

		while (step_length > half_milimeter
				&& step_count < max_step_count)
		{
			// the algorithm for testing the maximum radius could be expensive enough
			// that it makes sense to NOT duplicate testing when possible, so we keep
			// track of where we last tested, and only test the new points

			S32 best_dx = 0;
			S32 best_dy = 0;
			S32 best_dz = 0;

			// sample near the center of the box
			bool found_better_center = false;
			for (S32 dx = -1; dx < 2; ++dx)
			{
				for (S32 dy = -1; dy < 2; ++dy)
				{
					for (S32 dz = -1; dz < 2; ++dz)
					{
						if (dx == 0 && dy == 0 && dz == 0)
						{
							continue;
						}

						// count the number of indecies that match the last_*'s
						S32 match_count = 0;
						if (last_dx == dx) ++match_count;
						if (last_dy == dy) ++match_count;
						if (last_dz == dz) ++match_count;
						if (match_count == 2)
						{
							// we've already tested this point
							continue;
						}

						LLVector3 center = bounding_center;
						center.mV[VX] += (F32) dx * step_length;
						center.mV[VY] += (F32) dy * step_length;
						center.mV[VZ] += (F32) dz * step_length;

						// compute the radius of the bounding sphere
						F32 max_radius = 0.f;
						std::vector<LLSphere>::const_iterator sphere_itr;
						for (sphere_itr = sphere_list.begin(); sphere_itr != sphere_list.end(); ++sphere_itr)
						{
							F32 radius = (sphere_itr->getCenter() - center).length() + sphere_itr->getRadius();
							if (radius > max_radius)
							{
								max_radius = radius;
							}
						}
						if (max_radius < bounding_radius)
						{
							best_dx = dx;
							best_dy = dy;
							best_dz = dz;
							bounding_center = center;
							bounding_radius = max_radius;
							found_better_center = true;
						}
					}
				}
			}
			if (found_better_center)
			{
				// remember where we came from so we can avoid retesting
				last_dx = -best_dx;
				last_dy = -best_dy;
				last_dz = -best_dz;
			}
			else
			{
				// reduce the step size
				step_length *= 0.5f;
				//++step_count;
				// reset the last_*'s
				last_dx = 2;	// 2 is out of bounds --> no match
				last_dy = 2;
				last_dz = 2;
			}
		}

		// HACK -- it is possible to get enough floating point error for the
		// bounding sphere to too small on the order of 10e-6, but we only need
		// it to be accurate to within about half a millimeter
		bounding_radius += half_milimeter;

		// this algorithm can get relatively inaccurate when the sphere 
		// collection is 'small' (contained within a bounding sphere of about 
		// 2 meters or less)
		// TODO -- fix this
		/* debug code
		{
			std::vector<LLSphere>::const_iterator sphere_itr;
			for (sphere_itr = sphere_list.begin(); sphere_itr != sphere_list.end(); ++sphere_itr)
			{
				F32 radius = (sphere_itr->getCenter() - bounding_center).length() + sphere_itr->getRadius();
				if (radius + 0.1f > bounding_radius)
				{
					std::cout << " rad = " << radius << "  bounding - rad = " << (bounding_radius - radius) << std::endl;
				}
			}
			std::cout << "\n" << std::endl;
		}
		*/ 

		bounding_sphere.set(bounding_center, bounding_radius);
	}
	return bounding_sphere;
}


