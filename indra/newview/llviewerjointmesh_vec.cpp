/** 
 * @file llviewerjointmesh_vec.cpp
 * @brief Compiler-generated vectorized joint skinning code, works well on
 * Altivec processors (PowerPC Mac)
 *
 * *NOTE: See llv4math.h for notes on SSE/Altivec vector code.
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "llviewerjointmesh.h"

#include "llface.h"
#include "llpolymesh.h"
#include "llv4math.h"
#include "llv4matrix3.h"
#include "llv4matrix4.h"

// Generic vectorized code, uses compiler defaults, works well for Altivec
// on PowerPC.

// static
void LLViewerJointMesh::updateGeometryVectorized(LLFace *face, LLPolyMesh *mesh)
{
#if 0
	static LLV4Matrix4	sJointMat[32];
	LLDynamicArray<LLJointRenderData*>& joint_data = mesh->getReferenceMesh()->mJointRenderData;
	S32 j, joint_num, joint_end = joint_data.count();
	LLV4Vector3 pivot;

	//upload joint pivots/matrices
	for(j = joint_num = 0; joint_num < joint_end ; ++joint_num )
	{
		LLSkinJoint *sj;
		const LLMatrix4 *	wm = joint_data[joint_num]->mWorldMatrix;
		if (NULL == (sj = joint_data[joint_num]->mSkinJoint))
		{
				sj = joint_data[++joint_num]->mSkinJoint;
				((LLV4Matrix3)(sJointMat[j] = *wm)).multiply(sj->mRootToParentJointSkinOffset, pivot);
				sJointMat[j++].translate(pivot);
				wm = joint_data[joint_num]->mWorldMatrix;
		}
		((LLV4Matrix3)(sJointMat[j] = *wm)).multiply(sj->mRootToJointSkinOffset, pivot);
		sJointMat[j++].translate(pivot);
	}

	F32					weight		= F32_MAX;
	LLV4Matrix4			blend_mat;

	LLStrider<LLVector3> o_vertices;
	LLStrider<LLVector3> o_normals;

	LLVertexBuffer *buffer = face->mVertexBuffer;
	buffer->getVertexStrider(o_vertices,  mesh->mFaceVertexOffset);
	buffer->getNormalStrider(o_normals,   mesh->mFaceVertexOffset);

	const F32*			weights			= mesh->getWeights();
	const LLVector3*	coords			= mesh->getCoords();
	const LLVector3*	normals			= mesh->getNormals();
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

	buffer->setBuffer(0);
#endif
}
