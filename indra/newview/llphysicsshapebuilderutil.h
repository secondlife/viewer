/** 
 * @file llphysicsshapebuilder.h
 * @brief Generic system to convert LL(Physics)VolumeParams to physics shapes
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

#ifndef LL_PHYSICS_SHAPE_BUILDER_H
#define LL_PHYSICS_SHAPE_BUILDER_H

#include "indra_constants.h"
#include "llvolume.h"

#define USE_SHAPE_QUANTIZATION 0

#define SHAPE_BUILDER_DEFAULT_VOLUME_DETAIL 1

#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_HOLLOW 0.10f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_HOLLOW_SPHERES 0.90f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT 0.05f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_TAPER 0.05f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_TWIST 0.09f
#define SHAPE_BUILDER_IMPLICIT_THRESHOLD_SHEAR 0.05f

const F32 SHAPE_BUILDER_ENTRY_SNAP_SCALE_BIN_SIZE = 0.15f;
const F32 SHAPE_BUILDER_ENTRY_SNAP_PARAMETER_BIN_SIZE = 0.010f;
const F32 SHAPE_BUILDER_CONVEXIFICATION_SIZE = 2.f * COLLISION_TOLERANCE;
const F32 SHAPE_BUILDER_MIN_GEOMETRY_SIZE = 0.5f * COLLISION_TOLERANCE;

class LLPhysicsVolumeParams : public LLVolumeParams
{
public:

	LLPhysicsVolumeParams( const LLVolumeParams& params, bool forceConvex ) : 
		LLVolumeParams( params ),
		mForceConvex(forceConvex) {}

	bool operator==(const LLPhysicsVolumeParams &params) const
	{
		return ( LLVolumeParams::operator==(params) && (mForceConvex == params.mForceConvex) );
	}

	bool operator!=(const LLPhysicsVolumeParams &params) const
	{
		return !operator==(params);
	}

	bool operator<(const LLPhysicsVolumeParams &params) const
	{
		if ( LLVolumeParams::operator!=(params) )
		{
			return LLVolumeParams::operator<(params);
		}
		return (params.mForceConvex == false) && (mForceConvex == true);	
	}

	bool shouldForceConvex() const { return mForceConvex; }

private:
	bool mForceConvex;
};


class LLPhysicsShapeBuilderUtil
{
public:

	class PhysicsShapeSpecification
	{
	public:
		enum ShapeType
		{
			// Primitive types
			BOX,
			SPHERE,
			CYLINDER,

			USER_CONVEX,	// User specified they wanted the convex hull of the volume

			PRIM_CONVEX,	// Either a volume that is inherently convex but not a primitive type, or a shape
							// with dimensions such that will convexify it anyway.

 			SCULPT,			// Special case for traditional sculpts--they are the convex hull of a single particular set of volume params

			USER_MESH,		// A user mesh. May or may not contain a convex decomposition.

			PRIM_MESH,		// A non-convex volume which we have to represent accurately

			INVALID
		};

		PhysicsShapeSpecification() : 
		mType( INVALID ),
		mScale( 0.f, 0.f, 0.f ),
		mCenter( 0.f, 0.f, 0.f ) {}
		
		bool isConvex() { return (mType != USER_MESH && mType != PRIM_MESH && mType != INVALID); }
		bool isMesh() { return (mType == USER_MESH) || (mType == PRIM_MESH); }

		ShapeType getType() { return mType; }
		const LLVector3& getScale() { return mScale; }
		const LLVector3& getCenter() { return mCenter; }

	private:
		friend class LLPhysicsShapeBuilderUtil;

		ShapeType	mType;

		// Dimensions of an AABB around the shape
		LLVector3	mScale;

		// Offset of shape from origin of primitive's reference frame
		LLVector3	mCenter;
	};

	static void determinePhysicsShape( const LLPhysicsVolumeParams& volume_params, const LLVector3& scale, PhysicsShapeSpecification& specOut );
};

#endif //LL_PHYSICS_SHAPE_BUILDER_H
