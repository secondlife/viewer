/**
 * @file llphysicsshapebuilder.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llphysicsshapebuilderutil.h"

/* static */
void LLPhysicsShapeBuilderUtil::determinePhysicsShape( const LLPhysicsVolumeParams& volume_params, const LLVector3& scale, PhysicsShapeSpecification& specOut)
{
    const LLProfileParams& profile_params = volume_params.getProfileParams();
    const LLPathParams& path_params = volume_params.getPathParams();

    specOut.mScale = scale;

    const F32 avgScale = ( scale[VX] + scale[VY] + scale[VZ] )/3.0f;

    // count the scale elements that are small
    S32 min_size_counts = 0;
    for (S32 i = 0; i < 3; ++i)
    {
        if (scale[i] < SHAPE_BUILDER_CONVEXIFICATION_SIZE)
        {
            ++min_size_counts;
        }
    }

    const bool profile_complete = ( profile_params.getBegin() <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT/avgScale ) &&
        ( profile_params.getEnd() >= (1.0f - SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT/avgScale) );

    const bool path_complete =  ( path_params.getBegin() <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT/avgScale ) &&
        ( path_params.getEnd() >= (1.0f - SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT/avgScale) );

    const bool simple_params = ( volume_params.getHollow() <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_HOLLOW/avgScale )
        && ( fabs(path_params.getShearX()) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_SHEAR/avgScale )
        && ( fabs(path_params.getShearY()) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_SHEAR/avgScale )
        && ( !volume_params.isMeshSculpt() && !volume_params.isSculpt() );

    if (simple_params && profile_complete)
    {
        // Try to create an implicit shape or convexified
        bool no_taper = ( fabs(path_params.getScaleX() - 1.0f) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_TAPER/avgScale )
            && ( fabs(path_params.getScaleY() - 1.0f) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_TAPER/avgScale );

        bool no_twist = ( fabs(path_params.getTwistBegin()) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_TWIST/avgScale )
            && ( fabs(path_params.getTwistEnd()) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_TWIST/avgScale);

        // Box
        if(
            ( profile_params.getCurveType() == LL_PCODE_PROFILE_SQUARE )
            && ( path_params.getCurveType() == LL_PCODE_PATH_LINE )
            && no_taper
            && no_twist
            )
        {
            specOut.mType = PhysicsShapeSpecification::BOX;
            if ( path_complete )
            {
                return;
            }
            else
            {
                // Side lengths
                specOut.mScale[VX] = llmax( scale[VX], SHAPE_BUILDER_MIN_GEOMETRY_SIZE );
                specOut.mScale[VY] = llmax( scale[VY], SHAPE_BUILDER_MIN_GEOMETRY_SIZE );
                specOut.mScale[VZ] = llmax( scale[VZ] * (path_params.getEnd() - path_params.getBegin()), SHAPE_BUILDER_MIN_GEOMETRY_SIZE );

                specOut.mCenter.set( 0.f, 0.f, 0.5f * scale[VZ] * ( path_params.getEnd() + path_params.getBegin() - 1.0f ) );
                return;
            }
        }

        // Sphere
        if(     path_complete
            && ( profile_params.getCurveType() == LL_PCODE_PROFILE_CIRCLE_HALF )
            && ( path_params.getCurveType() == LL_PCODE_PATH_CIRCLE )
            && ( fabs(volume_params.getTaper()) <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_TAPER/avgScale )
            && no_twist
            )
        {
            if (   ( scale[VX] == scale[VZ] )
                && ( scale[VY] == scale[VZ] ) )
            {
                // perfect sphere
                specOut.mType   = PhysicsShapeSpecification::SPHERE;
                specOut.mScale  = scale;
                return;
            }
            else if (min_size_counts > 1)
            {
                // small or narrow sphere -- we can boxify
                for (S32 i=0; i<3; ++i)
                {
                    if (specOut.mScale[i] < SHAPE_BUILDER_CONVEXIFICATION_SIZE)
                    {
                        // reduce each small dimension size to split the approximation errors
                        specOut.mScale[i] *= 0.75f;
                    }
                }
                specOut.mType  = PhysicsShapeSpecification::BOX;
                return;
            }
        }

        // Cylinder
        if(    (scale[VX] == scale[VY])
            && ( profile_params.getCurveType() == LL_PCODE_PROFILE_CIRCLE )
            && ( path_params.getCurveType() == LL_PCODE_PATH_LINE )
            && ( volume_params.getBeginS() <= SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT/avgScale )
            && ( volume_params.getEndS() >= (1.0f - SHAPE_BUILDER_IMPLICIT_THRESHOLD_PATH_CUT/avgScale) )
            && no_taper
            )
        {
            if (min_size_counts > 1)
            {
                // small or narrow sphere -- we can boxify
                for (S32 i=0; i<3; ++i)
                {
                    if (specOut.mScale[i] < SHAPE_BUILDER_CONVEXIFICATION_SIZE)
                    {
                        // reduce each small dimension size to split the approximation errors
                        specOut.mScale[i] *= 0.75f;
                    }
                }

                specOut.mType = PhysicsShapeSpecification::BOX;
            }
            else
            {
                specOut.mType = PhysicsShapeSpecification::CYLINDER;
                F32 length = (volume_params.getPathParams().getEnd() - volume_params.getPathParams().getBegin()) * scale[VZ];

                specOut.mScale[VY] = specOut.mScale[VX];
                specOut.mScale[VZ] = length;
                // The minus one below fixes the fact that begin and end range from 0 to 1 not -1 to 1.
                specOut.mCenter.set( 0.f, 0.f, 0.5f * (volume_params.getPathParams().getBegin() + volume_params.getPathParams().getEnd() - 1.f) * scale[VZ] );
            }

            return;
        }
    }

    if (    (min_size_counts == 3 )
        // Possible dead code here--who wants to take it out?
        ||  (path_complete
                && profile_complete
                && ( path_params.getCurveType() == LL_PCODE_PATH_LINE )
                && (min_size_counts > 1 ) )
        )
    {
        // it isn't simple but
        // we might be able to convexify this shape if the path and profile are complete
        // or the path is linear and both path and profile are complete --> we can boxify it
        specOut.mType = PhysicsShapeSpecification::BOX;
        specOut.mScale = scale;
        return;
    }

    // Special case for big, very thin objects - bump the small dimensions up to the COLLISION_TOLERANCE
    if (min_size_counts == 1        // One dimension is small
        && avgScale > 3.f)          //  ... but others are fairly large
    {
        for (S32 i = 0; i < 3; ++i)
        {
            specOut.mScale[i] = llmax( specOut.mScale[i], COLLISION_TOLERANCE );
        }
    }

    if ( volume_params.shouldForceConvex() )
    {
        // Server distinguishes between convex of a prim vs isSculpt, but we don't care.
        specOut.mType = PhysicsShapeSpecification::USER_CONVEX;
    }
    // Make a simpler convex shape if we can.
    else if (volume_params.isConvex()           // is convex
            || min_size_counts > 1 )            // two or more small dimensions
    {
        specOut.mType = PhysicsShapeSpecification::PRIM_CONVEX;
    }
    else if (volume_params.isMeshSculpt() &&
             // Check overall dimensions, not individual triangles.
             (scale.mV[0] < SHAPE_BUILDER_USER_MESH_CONVEXIFICATION_SIZE ||
              scale.mV[1] < SHAPE_BUILDER_USER_MESH_CONVEXIFICATION_SIZE ||
              scale.mV[2] < SHAPE_BUILDER_USER_MESH_CONVEXIFICATION_SIZE
              ) )
    {
        // Server distinguishes between user-specified or default convex mesh, vs server's thin-triangle override, but we don't.
        specOut.mType = PhysicsShapeSpecification::PRIM_CONVEX;
    }
    else if ( volume_params.isSculpt() ) // Is a sculpt of any kind (mesh or legacy)
    {
        specOut.mType = volume_params.isMeshSculpt() ? PhysicsShapeSpecification::USER_MESH : PhysicsShapeSpecification::SCULPT;
    }
    else // Resort to mesh
    {
        specOut.mType = PhysicsShapeSpecification::PRIM_MESH;
    }
}
