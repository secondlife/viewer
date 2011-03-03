/** 
 * @file llviewerjointmesh_sse2.cpp
 * @brief SSE vectorized joint skinning code, only used when video card does
 * not support avatar vertex programs.
 *
 * *NOTE: Disabled on Windows builds. See llv4math.h for details.
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

// Visual Studio required settings for this file:
// Precompiled Headers OFF
// Code Generation: SSE2

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------

#include "llviewerprecompiledheaders.h"

#include "llviewerjointmesh.h"

// project includes
#include "llface.h"
#include "llpolymesh.h"

// library includes
#include "lldarray.h"
#include "llstrider.h"
#include "llv4math.h"		// for LL_VECTORIZE
#include "llv4matrix3.h"
#include "llv4matrix4.h"
#include "m4math.h"
#include "v3math.h"


#if LL_VECTORIZE


inline void matrix_translate(LLV4Matrix4& m, const LLMatrix4* w, const LLVector3& j)
{
	m.mV[VX] = _mm_loadu_ps(w->mMatrix[VX]);
	m.mV[VY] = _mm_loadu_ps(w->mMatrix[VY]);
	m.mV[VZ] = _mm_loadu_ps(w->mMatrix[VZ]);
	m.mV[VW] = _mm_loadu_ps(w->mMatrix[VW]);
	m.mV[VW] = _mm_add_ps(m.mV[VW], _mm_mul_ps(_mm_set1_ps(j.mV[VX]), m.mV[VX])); // ( ax * vx ) + vw
	m.mV[VW] = _mm_add_ps(m.mV[VW], _mm_mul_ps(_mm_set1_ps(j.mV[VY]), m.mV[VY]));
	m.mV[VW] = _mm_add_ps(m.mV[VW], _mm_mul_ps(_mm_set1_ps(j.mV[VZ]), m.mV[VZ]));
}

// static
void LLViewerJointMesh::updateGeometrySSE2(LLFace *face, LLPolyMesh *mesh)
{
	// This cannot be a file-level static because it will be initialized
	// before main() using SSE code, which will crash on non-SSE processors.
	static LLV4Matrix4	sJointMat[32];
	LLDynamicArray<LLJointRenderData*>& joint_data = mesh->getReferenceMesh()->mJointRenderData;

	//upload joint pivots/matrices
	for(S32 j = 0, jend = joint_data.count(); j < jend ; ++j )
	{
		matrix_translate(sJointMat[j], joint_data[j]->mWorldMatrix,
			joint_data[j]->mSkinJoint ?
				joint_data[j]->mSkinJoint->mRootToJointSkinOffset
				: joint_data[j+1]->mSkinJoint->mRootToParentJointSkinOffset);
	}

	F32					weight		= F32_MAX;
	LLV4Matrix4			blend_mat;

	LLStrider<LLVector3> o_vertices;
	LLStrider<LLVector3> o_normals;

	LLVertexBuffer *buffer = face->getVertexBuffer();
	buffer->getVertexStrider(o_vertices,  mesh->mFaceVertexOffset);
	buffer->getNormalStrider(o_normals,   mesh->mFaceVertexOffset);

	const F32*			weights			= mesh->getWeights();
	const LLVector3*	coords			= (const LLVector3*)mesh->getCoords();
	const LLVector3*	normals			= (const LLVector3*)mesh->getNormals();
	for (U32 index = 0, index_end = mesh->getNumVertices(); index < index_end; ++index)
	{
		if( weight != weights[index])
		{
			S32 joint = llfloor(weight = weights[index]);
			blend_mat.lerp(sJointMat[joint], sJointMat[joint+1], weight - joint);
		}
		blend_mat.multiply(coords[index], o_vertices[index]);
		((LLV4Matrix3)blend_mat).multiply(normals[index], o_normals[index]);
	}
	
	//setBuffer(0) called in LLVOAvatar::renderSkinned
}

#else

void LLViewerJointMesh::updateGeometrySSE2(LLFace *face, LLPolyMesh *mesh)
{
	LLViewerJointMesh::updateGeometryVectorized(face, mesh);
}

#endif
