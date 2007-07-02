/** 
 * @file llviewerjointmesh.cpp
 * @brief LLV4 math class implementation with LLViewerJointMesh class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
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

// *NOTE: SSE must be disabled for this module

#if LL_VECTORIZE
#error This module requires vectorization (i.e. SSE) mode to be disabled.
#endif

static LLV4Matrix4	sJointMat[32];

// static
void LLViewerJointMesh::updateGeometryVectorized(LLFace *face, LLPolyMesh *mesh)
{
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
}
