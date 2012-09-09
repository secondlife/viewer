/** 
 * @file llviewerjointmesh.cpp
 * @brief Implementation of LLViewerJointMesh class
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#include "imageids.h"
#include "llfasttimer.h"
#include "llrender.h"

#include "llapr.h"
#include "llbox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldynamictexture.h"
#include "llface.h"
#include "llgldbg.h"
#include "llglheaders.h"
#include "llviewertexlayer.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewertexturelist.h"
#include "llviewerjointmesh.h"
#include "llvoavatar.h"
#include "llsky.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llmath.h"
#include "v4math.h"
#include "m3math.h"
#include "m4math.h"
#include "llmatrix4a.h"

#if !LL_DARWIN && !LL_LINUX && !LL_SOLARIS
extern PFNGLWEIGHTPOINTERARBPROC glWeightPointerARB;
extern PFNGLWEIGHTFVARBPROC glWeightfvARB;
extern PFNGLVERTEXBLENDARBPROC glVertexBlendARB;
#endif

static const U32 sRenderMask = LLVertexBuffer::MAP_VERTEX |
							   LLVertexBuffer::MAP_NORMAL |
							   LLVertexBuffer::MAP_TEXCOORD0;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// LLViewerJointMesh()
//-----------------------------------------------------------------------------
LLViewerJointMesh::LLViewerJointMesh()
	:
	LLAvatarJointMesh()
{
}


//-----------------------------------------------------------------------------
// ~LLViewerJointMesh()
// Class Destructor
//-----------------------------------------------------------------------------
LLViewerJointMesh::~LLViewerJointMesh()
{
}

const S32 NUM_AXES = 3;

// register layoud
// rotation X 0-n
// rotation Y 0-n
// rotation Z 0-n
// pivot parent 0-n -- child = n+1

static LLMatrix4	gJointMatUnaligned[32];
static LLMatrix4a	gJointMatAligned[32];
static LLMatrix3	gJointRotUnaligned[32];
static LLVector4	gJointPivot[32];

//-----------------------------------------------------------------------------
// uploadJointMatrices()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::uploadJointMatrices()
{
	S32 joint_num;
	LLPolyMesh *reference_mesh = mMesh->getReferenceMesh();
	LLDrawPool *poolp = mFace ? mFace->getPool() : NULL;
	BOOL hardware_skinning = (poolp && poolp->getVertexShaderLevel() > 0) ? TRUE : FALSE;

	//calculate joint matrices
	for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
	{
		LLMatrix4 joint_mat = *reference_mesh->mJointRenderData[joint_num]->mWorldMatrix;

		if (hardware_skinning)
		{
			joint_mat *= LLDrawPoolAvatar::getModelView();
		}
		gJointMatUnaligned[joint_num] = joint_mat;
		gJointRotUnaligned[joint_num] = joint_mat.getMat3();
	}

	BOOL last_pivot_uploaded = FALSE;
	S32 j = 0;

	//upload joint pivots
	for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
	{
		LLSkinJoint *sj = reference_mesh->mJointRenderData[joint_num]->mSkinJoint;
		if (sj)
		{
			if (!last_pivot_uploaded)
			{
				LLVector4 parent_pivot(sj->mRootToParentJointSkinOffset);
				parent_pivot.mV[VW] = 0.f;
				gJointPivot[j++] = parent_pivot;
			}

			LLVector4 child_pivot(sj->mRootToJointSkinOffset);
			child_pivot.mV[VW] = 0.f;

			gJointPivot[j++] = child_pivot;

			last_pivot_uploaded = TRUE;
		}
		else
		{
			last_pivot_uploaded = FALSE;
		}
	}

	//add pivot point into transform
	for (S32 i = 0; i < j; i++)
	{
		LLVector3 pivot;
		pivot = LLVector3(gJointPivot[i]);
		pivot = pivot * gJointRotUnaligned[i];
		gJointMatUnaligned[i].translate(pivot);
	}

	// upload matrices
	if (hardware_skinning)
	{
		GLfloat mat[45*4];
		memset(mat, 0, sizeof(GLfloat)*45*4);

		for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
		{
			gJointMatUnaligned[joint_num].transpose();

			for (S32 axis = 0; axis < NUM_AXES; axis++)
			{
				F32* vector = gJointMatUnaligned[joint_num].mMatrix[axis];
				U32 offset = LL_CHARACTER_MAX_JOINTS_PER_MESH*axis+joint_num;
				memcpy(mat+offset*4, vector, sizeof(GLfloat)*4);
			}
		}
		stop_glerror();
		if (LLGLSLShader::sCurBoundShaderPtr)
		{
			LLGLSLShader::sCurBoundShaderPtr->uniform4fv(LLViewerShaderMgr::AVATAR_MATRIX, 45, mat);
		}
		stop_glerror();
	}
	else
	{
		//load gJointMatUnaligned into gJointMatAligned
		for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); ++joint_num)
		{
			gJointMatAligned[joint_num].loadu(gJointMatUnaligned[joint_num]);
		}
	}
}

//--------------------------------------------------------------------
// DrawElementsBLEND and utility code
//--------------------------------------------------------------------

// compate_int is used by the qsort function to sort the index array
int compare_int(const void *a, const void *b)
{
	if (*(U32*)a < *(U32*)b)
	{
		return -1;
	}
	else if (*(U32*)a > *(U32*)b)
	{
		return 1;
	}
	else return 0;
}

//--------------------------------------------------------------------
// LLViewerJointMesh::drawShape()
//--------------------------------------------------------------------
U32 LLViewerJointMesh::drawShape( F32 pixelArea, BOOL first_pass, BOOL is_dummy)
{
	if (!mValid || !mMesh || !mFace || !mVisible || 
		!mFace->getVertexBuffer() ||
		mMesh->getNumFaces() == 0 ||
		(LLGLSLShader::sNoFixedFunction && LLGLSLShader::sCurBoundShaderPtr == NULL))
	{
		return 0;
	}

	U32 triangle_count = 0;

	S32 diffuse_channel = LLDrawPoolAvatar::sDiffuseChannel;

	stop_glerror();
	
	//----------------------------------------------------------------
	// setup current color
	//----------------------------------------------------------------
	if (is_dummy)
		gGL.diffuseColor4fv(LLVOAvatar::getDummyColor().mV);
	else
		gGL.diffuseColor4fv(mColor.mV);

	stop_glerror();
	
	LLGLSSpecular specular(LLColor4(1.f,1.f,1.f,1.f), (mFace->getPool()->getVertexShaderLevel() > 0 || LLGLSLShader::sNoFixedFunction) ? 0.f : mShiny);

	//----------------------------------------------------------------
	// setup current texture
	//----------------------------------------------------------------
	llassert( !(mTexture.notNull() && mLayerSet) );  // mutually exclusive

	LLTexUnit::eTextureAddressMode old_mode = LLTexUnit::TAM_WRAP;
	LLViewerTexLayerSet *layerset = dynamic_cast<LLViewerTexLayerSet*>(mLayerSet);
	if (mTestImageName)
	{
		gGL.getTexUnit(diffuse_channel)->bindManual(LLTexUnit::TT_TEXTURE, mTestImageName);

		if (mIsTransparent)
		{
			gGL.diffuseColor4f(1.f, 1.f, 1.f, 1.f);
		}
		else
		{
			gGL.diffuseColor4f(0.7f, 0.6f, 0.3f, 1.f);
			gGL.getTexUnit(diffuse_channel)->setTextureColorBlend(LLTexUnit::TBO_LERP_TEX_ALPHA, LLTexUnit::TBS_TEX_COLOR, LLTexUnit::TBS_PREV_COLOR);
		}
	}
	else if( !is_dummy && layerset )
	{
		if(	layerset->hasComposite() )
		{
			gGL.getTexUnit(diffuse_channel)->bind(layerset->getViewerComposite());
		}
		else
		{
			gGL.getTexUnit(diffuse_channel)->bind(LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT));
		}
	}
	else
	if ( !is_dummy && mTexture.notNull() )
	{
		if(mTexture->hasGLTexture())
		{
			old_mode = mTexture->getAddressMode();
		}
		gGL.getTexUnit(diffuse_channel)->bind(mTexture);
		gGL.getTexUnit(diffuse_channel)->setTextureAddressMode(LLTexUnit::TAM_CLAMP);
	}
	else
	{
		gGL.getTexUnit(diffuse_channel)->bind(LLViewerTextureManager::getFetchedTexture(IMG_DEFAULT));
	}
	
	
	U32 mask = sRenderMask;

	U32 start = mMesh->mFaceVertexOffset;
	U32 end = start + mMesh->mFaceVertexCount - 1;
	U32 count = mMesh->mFaceIndexCount;
	U32 offset = mMesh->mFaceIndexOffset;

	LLVertexBuffer* buff = mFace->getVertexBuffer();

	if (mMesh->hasWeights())
	{
		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			if (first_pass)
			{
				uploadJointMatrices();
			}
			mask = mask | LLVertexBuffer::MAP_WEIGHT;
			if (mFace->getPool()->getVertexShaderLevel() > 1)
			{
				mask = mask | LLVertexBuffer::MAP_CLOTHWEIGHT;
			}
		}
		
		buff->setBuffer(mask);
		buff->drawRange(LLRender::TRIANGLES, start, end, count, offset);
	}
	else
	{
		gGL.pushMatrix();
		LLMatrix4 jointToWorld = getWorldMatrix();
		gGL.multMatrix((GLfloat*)jointToWorld.mMatrix);
		buff->setBuffer(mask);
		buff->drawRange(LLRender::TRIANGLES, start, end, count, offset);
		gGL.popMatrix();
	}
	gPipeline.addTrianglesDrawn(count);

	triangle_count += count;
	
	if (mTestImageName)
	{
		gGL.getTexUnit(diffuse_channel)->setTextureBlendType(LLTexUnit::TB_MULT);
	}

	if (mTexture.notNull() && !is_dummy)
	{
		gGL.getTexUnit(diffuse_channel)->bind(mTexture);
		gGL.getTexUnit(diffuse_channel)->setTextureAddressMode(old_mode);
	}

	return triangle_count;
}

//-----------------------------------------------------------------------------
// updateFaceSizes()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::updateFaceSizes(U32 &num_vertices, U32& num_indices, F32 pixel_area)
{
	//bump num_vertices to next multiple of 4
	num_vertices = (num_vertices + 0x3) & ~0x3;

	// Do a pre-alloc pass to determine sizes of data.
	if (mMesh && mValid)
	{
		mMesh->mFaceVertexOffset = num_vertices;
		mMesh->mFaceVertexCount = mMesh->getNumVertices();
		mMesh->mFaceIndexOffset = num_indices;
		mMesh->mFaceIndexCount = mMesh->getSharedData()->mNumTriangleIndices;

		mMesh->getReferenceMesh()->mCurVertexCount = mMesh->mFaceVertexCount;

		num_vertices += mMesh->getNumVertices();
		num_indices += mMesh->mFaceIndexCount;
	}
}

//-----------------------------------------------------------------------------
// updateFaceData()
//-----------------------------------------------------------------------------
static LLFastTimer::DeclareTimer FTM_AVATAR_FACE("Avatar Face");

void LLViewerJointMesh::updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind, bool terse_update)
{
	//IF THIS FUNCTION BREAKS, SEE LLPOLYMESH CONSTRUCTOR AND CHECK ALIGNMENT OF INPUT ARRAYS

	mFace = face;

	if (!mFace->getVertexBuffer())
	{
		return;
	}

	LLDrawPool *poolp = mFace->getPool();
	BOOL hardware_skinning = (poolp && poolp->getVertexShaderLevel() > 0) ? TRUE : FALSE;

	if (!hardware_skinning && terse_update)
	{ //no need to do terse updates if we're doing software vertex skinning
	 // since mMesh is being copied into mVertexBuffer every frame
		return;
	}


	LLFastTimer t(FTM_AVATAR_FACE);

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector2> tex_coordsp;
	LLStrider<F32>		 vertex_weightsp;
	LLStrider<LLVector4> clothing_weightsp;
	LLStrider<U16> indicesp;

	// Copy data into the faces from the polymesh data.
	if (mMesh && mValid)
	{
		const U32 num_verts = mMesh->getNumVertices();

		if (num_verts)
		{
			face->getVertexBuffer()->getIndexStrider(indicesp);
			face->getGeometryAvatar(verticesp, normalsp, tex_coordsp, vertex_weightsp, clothing_weightsp);
			
			verticesp += mMesh->mFaceVertexOffset;
			normalsp += mMesh->mFaceVertexOffset;
			
			F32* v = (F32*) verticesp.get();
			F32* n = (F32*) normalsp.get();
			
			U32 words = num_verts*4;

			LLVector4a::memcpyNonAliased16(v, (F32*) mMesh->getCoords(), words*sizeof(F32));
			LLVector4a::memcpyNonAliased16(n, (F32*) mMesh->getNormals(), words*sizeof(F32));
						
			
			if (!terse_update)
			{
				vertex_weightsp += mMesh->mFaceVertexOffset;
				clothing_weightsp += mMesh->mFaceVertexOffset;
				tex_coordsp += mMesh->mFaceVertexOffset;
		
				F32* tc = (F32*) tex_coordsp.get();
				F32* vw = (F32*) vertex_weightsp.get();
				F32* cw = (F32*) clothing_weightsp.get();	

				S32 tc_size = (num_verts*2*sizeof(F32)+0xF) & ~0xF;
				LLVector4a::memcpyNonAliased16(tc, (F32*) mMesh->getTexCoords(), tc_size);
				S32 vw_size = (num_verts*sizeof(F32)+0xF) & ~0xF;	
				LLVector4a::memcpyNonAliased16(vw, (F32*) mMesh->getWeights(), vw_size);	
				LLVector4a::memcpyNonAliased16(cw, (F32*) mMesh->getClothingWeights(), num_verts*4*sizeof(F32));	
			}

			const U32 idx_count = mMesh->getNumFaces()*3;

			indicesp += mMesh->mFaceIndexOffset;

			U16* __restrict idx = indicesp.get();
			S32* __restrict src_idx = (S32*) mMesh->getFaces();	

			const S32 offset = (S32) mMesh->mFaceVertexOffset;

			for (S32 i = 0; i < idx_count; ++i)
			{
				*(idx++) = *(src_idx++)+offset;
			}
		}
	}
}



//-----------------------------------------------------------------------------
// updateLOD()
//-----------------------------------------------------------------------------
BOOL LLViewerJointMesh::updateLOD(F32 pixel_area, BOOL activate)
{
	BOOL valid = mValid;
	setValid(activate, TRUE);
	return (valid != activate);
}

// static
void LLViewerJointMesh::updateGeometry(LLFace *mFace, LLPolyMesh *mMesh)
{
	LLStrider<LLVector3> o_vertices;
	LLStrider<LLVector3> o_normals;

	//get vertex and normal striders
	LLVertexBuffer* buffer = mFace->getVertexBuffer();
	buffer->getVertexStrider(o_vertices,  0);
	buffer->getNormalStrider(o_normals,   0);

	F32* __restrict vert = o_vertices[0].mV;
	F32* __restrict norm = o_normals[0].mV;

	const F32* __restrict weights = mMesh->getWeights();
	const LLVector4a* __restrict coords = (LLVector4a*) mMesh->getCoords();
	const LLVector4a* __restrict normals = (LLVector4a*) mMesh->getNormals();

	U32 offset = mMesh->mFaceVertexOffset*4;
	vert += offset;
	norm += offset;

	for (U32 index = 0; index < mMesh->getNumVertices(); index++)
	{
		// equivalent to joint = floorf(weights[index]);
		S32 joint = _mm_cvtt_ss2si(_mm_load_ss(weights+index));
		F32 w = weights[index] - joint;		

		LLMatrix4a gBlendMat;

		if (w != 0.f)
		{
			// blend between matrices and apply
			gBlendMat.setLerp(gJointMatAligned[joint+0],
							  gJointMatAligned[joint+1], w);

			LLVector4a res;
			gBlendMat.affineTransform(coords[index], res);
			res.store4a(vert+index*4);
			gBlendMat.rotate(normals[index], res);
			res.store4a(norm+index*4);
		}
		else
		{  // No lerp required in this case.
			LLVector4a res;
			gJointMatAligned[joint].affineTransform(coords[index], res);
			res.store4a(vert+index*4);
			gJointMatAligned[joint].rotate(normals[index], res);
			res.store4a(norm+index*4);
		}
	}

	buffer->flush();
}

void LLViewerJointMesh::updateJointGeometry()
{
	if (!(mValid
		  && mMesh
		  && mFace
		  && mMesh->hasWeights()
		  && mFace->getVertexBuffer()
		  && LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) == 0))
	{
		return;
	}

	uploadJointMatrices();
	updateGeometry(mFace, mMesh);
}

void LLViewerJointMesh::dump()
{
	if (mValid)
	{
		llinfos << "Usable LOD " << mName << llendl;
	}
}

// End
