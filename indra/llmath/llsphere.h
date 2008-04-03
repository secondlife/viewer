// llsphere.h
/**
 * @file llsphere.cpp
 * @author Andrew Meadows
 * @brief Simple sphere implementation for basic geometric operations
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_SPHERE_H
#define LL_SPHERE_H

#include "stdtypes.h"
#include "v3math.h"
#include <iostream>
#include <vector>

class LLSphere
{
public:
	LLSphere();
	LLSphere( const LLVector3& center, F32 radius );

	void set( const LLVector3& center, F32 radius );
	void setCenter( const LLVector3& center );
	void setRadius( F32 radius );

	const LLVector3& getCenter() const;
	F32 getRadius() const;

	// returns TRUE if this sphere completely contains other_sphere
	BOOL contains(const LLSphere& other_sphere) const;

	// returns TRUE if this sphere overlaps other_sphere
	BOOL overlaps(const LLSphere& other_sphere) const;

	// returns overlap distance
	// negative overlap is closest approach
	F32 getOverlap(const LLSphere& other_sphere) const;

	// removes any spheres that are contained in others
	static void collapse(std::vector<LLSphere>& sphere_list);

	// returns minimum sphere bounding sphere for a set of spheres
	static LLSphere getBoundingSphere(const LLSphere& first_sphere, const LLSphere& second_sphere);
	static LLSphere getBoundingSphere(const std::vector<LLSphere>& sphere_list);

	bool operator==(const LLSphere& rhs) const;

	friend std::ostream& operator<<( std::ostream& output_stream, const LLSphere& line );

protected:
	LLVector3 mCenter;
	F32 mRadius;
};


#endif
