// llsphere.h
/**
 * @file llsphere.cpp
 * @author Andrew Meadows
 * @brief Simple sphere implementation for basic geometric operations
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "stdtypes.h"
#include "v3math.h"
#include <glm/vec3.hpp>
#include <iostream>
#include <vector>

class LLSphere
{
public:
    LLSphere();
    LLSphere( const glm::vec3& center, F32 radius );

    void set( const glm::vec3& center, F32 radius );
    void setCenter( const glm::vec3& center );
    void setRadius( F32 radius );

    const glm::vec3& getCenter() const;
    F32 getRadius() const;

    // returns true if this sphere completely contains other_sphere
    bool contains(const LLSphere& other_sphere) const;

    // returns true if this sphere overlaps other_sphere
    bool overlaps(const LLSphere& other_sphere) const;

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
    glm::vec3 mCenter;
    F32 mRadius;
};


