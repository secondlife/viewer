/** 
 * @file llviewerjointmesh_vec.cpp
 * @brief Compiler-generated vectorized joint skinning code, works well on
 * Altivec processors (PowerPC Mac)
 *
 * *NOTE: See llv4math.h for notes on SSE/Altivec vector code.
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
}
