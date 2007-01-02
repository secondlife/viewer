/** 
 * @file llviewerjointmesh.cpp
 * @brief Implementation of LLViewerJointMesh class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "llviewerprecompiledheaders.h"

#if LL_WINDOWS // For Intel vector classes
	#include "fvec.h"
#endif

#include "imageids.h"
#include "llfasttimer.h"

#include "llagent.h"
#include "llagparray.h"
#include "llbox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "lldrawpoolbump.h"
#include "lldynamictexture.h"
#include "llface.h"
#include "llgldbg.h"
#include "llglheaders.h"
#include "lltexlayer.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerjointmesh.h"
#include "llvoavatar.h"
#include "llsky.h"
#include "pipeline.h"

#if !LL_DARWIN && !LL_LINUX
extern PFNGLWEIGHTPOINTERARBPROC glWeightPointerARB;
extern PFNGLWEIGHTFVARBPROC glWeightfvARB;
extern PFNGLVERTEXBLENDARBPROC glVertexBlendARB;
#endif
extern BOOL gRenderForSelect;

LLMatrix4 gBlendMat;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh::LLSkinJoint
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::LLSkinJoint()
{
	mJoint       = NULL;
}

//-----------------------------------------------------------------------------
// ~LLSkinJoint
//-----------------------------------------------------------------------------
LLSkinJoint::~LLSkinJoint()
{
	mJoint = NULL;
}


//-----------------------------------------------------------------------------
// LLSkinJoint::setupSkinJoint()
//-----------------------------------------------------------------------------
BOOL LLSkinJoint::setupSkinJoint( LLViewerJoint *joint)
{
	// find the named joint
	mJoint = joint;
	if ( !mJoint )
	{
		llinfos << "Can't find joint" << llendl;
	}

	// compute the inverse root skin matrix
	mRootToJointSkinOffset.clearVec();

	LLVector3 rootSkinOffset;
	while (joint)
	{
		rootSkinOffset += joint->getSkinOffset();
		joint = (LLViewerJoint*)joint->getParent();
	}

	mRootToJointSkinOffset = -rootSkinOffset;
	mRootToParentJointSkinOffset = mRootToJointSkinOffset;
	mRootToParentJointSkinOffset += mJoint->getSkinOffset();

	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// LLViewerJointMesh
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

BOOL LLViewerJointMesh::sPipelineRender = FALSE;
EAvatarRenderPass LLViewerJointMesh::sRenderPass = AVATAR_RENDER_PASS_SINGLE;
U32 LLViewerJointMesh::sClothingMaskImageName = 0;
LLColor4 LLViewerJointMesh::sClothingInnerColor;

//-----------------------------------------------------------------------------
// LLViewerJointMesh()
//-----------------------------------------------------------------------------
LLViewerJointMesh::LLViewerJointMesh()
	:
	mTexture( NULL ),
	mLayerSet( NULL ),
	mTestImageName( 0 ),
	mIsTransparent(FALSE)
{

	mColor[0] = 1.0f;
	mColor[1] = 1.0f;
	mColor[2] = 1.0f;
	mColor[3] = 1.0f;
	mShiny = 0.0f;
	mCullBackFaces = TRUE;

	mMesh = NULL;

	mNumSkinJoints = 0;
	mSkinJoints = NULL;

	mFace = NULL;

	mMeshID = 0;
	mUpdateXform = FALSE;

	mValid = FALSE;
}


//-----------------------------------------------------------------------------
// ~LLViewerJointMesh()
// Class Destructor
//-----------------------------------------------------------------------------
LLViewerJointMesh::~LLViewerJointMesh()
{
	mMesh = NULL;
	mTexture = NULL;
	freeSkinData();
}


//-----------------------------------------------------------------------------
// LLViewerJointMesh::allocateSkinData()
//-----------------------------------------------------------------------------
BOOL LLViewerJointMesh::allocateSkinData( U32 numSkinJoints )
{
	mSkinJoints = new LLSkinJoint[ numSkinJoints ];
	mNumSkinJoints = numSkinJoints;
	return TRUE;
}

//-----------------------------------------------------------------------------
// getSkinJointByIndex()
//-----------------------------------------------------------------------------
S32 LLViewerJointMesh::getBoundJointsByIndex(S32 index, S32 &joint_a, S32& joint_b)
{
	S32 num_joints = 0;
	if (mNumSkinJoints == 0)
	{
		return num_joints;
	}

	joint_a = -1;
	joint_b = -1;

	LLPolyMesh *reference_mesh = mMesh->getReferenceMesh();

	if (index < reference_mesh->mJointRenderData.count())
	{
		LLJointRenderData* render_datap = reference_mesh->mJointRenderData[index];
		if (render_datap->mSkinJoint)
		{
			joint_a = render_datap->mSkinJoint->mJoint->mJointNum;
		}
		num_joints++;
	}
	if (index + 1 < reference_mesh->mJointRenderData.count())
	{
		LLJointRenderData* render_datap = reference_mesh->mJointRenderData[index + 1];
		if (render_datap->mSkinJoint)
		{
			joint_b = render_datap->mSkinJoint->mJoint->mJointNum;
		}

		if (joint_a == -1)
		{
			joint_a = render_datap->mSkinJoint->mJoint->getParent()->mJointNum;
		}
		num_joints++;
	}
	return num_joints;
}

//-----------------------------------------------------------------------------
// LLViewerJointMesh::freeSkinData()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::freeSkinData()
{
	mNumSkinJoints = 0;
	delete [] mSkinJoints;
	mSkinJoints = NULL;
}

//--------------------------------------------------------------------
// LLViewerJointMesh::getColor()
//--------------------------------------------------------------------
void LLViewerJointMesh::getColor( F32 *red, F32 *green, F32 *blue, F32 *alpha )
{
	*red   = mColor[0];
	*green = mColor[1];
	*blue  = mColor[2];
	*alpha = mColor[3];
}

//--------------------------------------------------------------------
// LLViewerJointMesh::setColor()
//--------------------------------------------------------------------
void LLViewerJointMesh::setColor( F32 red, F32 green, F32 blue, F32 alpha )
{
	mColor[0] = red;
	mColor[1] = green;
	mColor[2] = blue;
	mColor[3] = alpha;
}


//--------------------------------------------------------------------
// LLViewerJointMesh::getTexture()
//--------------------------------------------------------------------
//LLViewerImage *LLViewerJointMesh::getTexture()
//{
//	return mTexture;
//}

//--------------------------------------------------------------------
// LLViewerJointMesh::setTexture()
//--------------------------------------------------------------------
void LLViewerJointMesh::setTexture( LLViewerImage *texture )
{
	mTexture = texture;

	// texture and dynamic_texture are mutually exclusive
	if( texture )
	{
		mLayerSet = NULL;
		//texture->bindTexture(0);
		//texture->setClamp(TRUE, TRUE);
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::setLayerSet()
// Sets the shape texture (takes precedence over normal texture)
//--------------------------------------------------------------------
void LLViewerJointMesh::setLayerSet( LLTexLayerSet* layer_set )
{
	mLayerSet = layer_set;
	
	// texture and dynamic_texture are mutually exclusive
	if( layer_set )
	{
		mTexture = NULL;
	}
}



//--------------------------------------------------------------------
// LLViewerJointMesh::getMesh()
//--------------------------------------------------------------------
LLPolyMesh *LLViewerJointMesh::getMesh()
{
	return mMesh;
}

//-----------------------------------------------------------------------------
// LLViewerJointMesh::setMesh()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::setMesh( LLPolyMesh *mesh )
{
	// set the mesh pointer
	mMesh = mesh;

	// release any existing skin joints
	freeSkinData();

	if ( mMesh == NULL )
	{
		return;
	}

	// acquire the transform from the mesh object
	setPosition( mMesh->getPosition() );
	setRotation( mMesh->getRotation() );
	setScale( mMesh->getScale() );

	// create skin joints if necessary
	if ( mMesh->hasWeights() && !mMesh->isLOD())
	{
		U32 numJointNames = mMesh->getNumJointNames();
		
		allocateSkinData( numJointNames );
		std::string *jointNames = mMesh->getJointNames();

		U32 jn;
		for (jn = 0; jn < numJointNames; jn++)
		{
			//llinfos << "Setting up joint " << jointNames[jn].c_str() << llendl;
			LLViewerJoint* joint = (LLViewerJoint*)(getRoot()->findJoint(jointNames[jn]) );
			mSkinJoints[jn].setupSkinJoint( joint );
		}
	}

	// setup joint array
	if (!mMesh->isLOD())
	{
		setupJoint((LLViewerJoint*)getRoot());
	}

//	llinfos << "joint render entries: " << mMesh->mJointRenderData.count() << llendl;
}

//-----------------------------------------------------------------------------
// setupJoint()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::setupJoint(LLViewerJoint* current_joint)
{
//	llinfos << "Mesh: " << getName() << llendl;

//	S32 joint_count = 0;
	U32 sj;
	for (sj=0; sj<mNumSkinJoints; sj++)
	{
		LLSkinJoint &js = mSkinJoints[sj];

		if (js.mJoint != current_joint)
		{
			continue;
		}

		// we've found a skinjoint for this joint..

		// is the last joint in the array our parent?
		if(mMesh->mJointRenderData.count() && mMesh->mJointRenderData[mMesh->mJointRenderData.count() - 1]->mWorldMatrix == &current_joint->getParent()->getWorldMatrix())
		{
			// ...then just add ourselves
			LLViewerJoint* jointp = js.mJoint;
			mMesh->mJointRenderData.put(new LLJointRenderData(&jointp->getWorldMatrix(), &js));
//			llinfos << "joint " << joint_count << js.mJoint->getName() << llendl;
//			joint_count++;
		}
		// otherwise add our parent and ourselves
		else
		{
			mMesh->mJointRenderData.put(new LLJointRenderData(&current_joint->getParent()->getWorldMatrix(), NULL));
//			llinfos << "joint " << joint_count << current_joint->getParent()->getName() << llendl;
//			joint_count++;
			mMesh->mJointRenderData.put(new LLJointRenderData(&current_joint->getWorldMatrix(), &js));
//			llinfos << "joint " << joint_count << current_joint->getName() << llendl;
//			joint_count++;
		}
	}

	// depth-first traversal
	for (LLJoint *child_joint = current_joint->mChildren.getFirstData(); 
		child_joint; 
		child_joint = current_joint->mChildren.getNextData())
	{
		setupJoint((LLViewerJoint*)child_joint);
	}
}

const S32 NUM_AXES = 3;

// register layoud
// rotation X 0-n
// rotation Y 0-n
// rotation Z 0-n
// pivot parent 0-n -- child = n+1

static LLMatrix4 gJointMat[32];
static LLMatrix3 gJointRot[32];
static LLVector4 gJointPivot[32];

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
			joint_mat *= gCamera->getModelview();
		}
		gJointMat[joint_num] = joint_mat;
		gJointRot[joint_num] = joint_mat.getMat3();
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
		pivot = pivot * gJointRot[i];
		gJointMat[i].translate(pivot);
	}

	// upload matrices
	if (hardware_skinning)
	{
		GLfloat mat[45*4];
		memset(mat, 0, sizeof(GLfloat)*45*4);

		for (joint_num = 0; joint_num < reference_mesh->mJointRenderData.count(); joint_num++)
		{
			gJointMat[joint_num].transpose();

			for (S32 axis = 0; axis < NUM_AXES; axis++)
			{
				F32* vector = gJointMat[joint_num].mMatrix[axis];
				//glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, LL_CHARACTER_MAX_JOINTS_PER_MESH * axis + joint_num+5, (GLfloat*)vector);
				U32 offset = LL_CHARACTER_MAX_JOINTS_PER_MESH*axis+joint_num;
				memcpy(mat+offset*4, vector, sizeof(GLfloat)*4);
				//glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, LL_CHARACTER_MAX_JOINTS_PER_MESH * axis + joint_num+6, (GLfloat*)vector);
				//cgGLSetParameterArray4f(gPipeline.mAvatarMatrix, offset, 1, vector);
			}
		}
		glUniform4fvARB(gPipeline.mAvatarMatrixParam, 45, mat);
	}
}

//--------------------------------------------------------------------
// LLViewerJointMesh::drawBone()
//--------------------------------------------------------------------
void LLViewerJointMesh::drawBone()
{
}

//--------------------------------------------------------------------
// LLViewerJointMesh::isTransparent()
//--------------------------------------------------------------------
BOOL LLViewerJointMesh::isTransparent()
{
	return mIsTransparent;
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

#if LL_WINDOWS || (LL_DARWIN && __i386__) // SSE optimizations in avatar code

#if LL_DARWIN
#include <xmmintrin.h>

	// On Windows, this class is defined in fvec.h.  I've only reproduced the parts of it we use here for now.
	#pragma pack(push,16) /* Must ensure class & union 16-B aligned */
	class F32vec4
	{
	protected:
		 __m128 vec;
	public:

		/* Constructors: __m128, 4 floats, 1 float */
		F32vec4() {}

		/* initialize 4 SP FP with __m128 data type */
		F32vec4(__m128 m)					{ vec = m;}

		/* Explicitly initialize each of 4 SP FPs with same float */
		explicit F32vec4(float f)	{ vec = _mm_set_ps1(f); }
	};
	#pragma pack(pop) /* 16-B aligned */
	
	
#endif

void blend_SSE_32_32_batch(const int vert_offset, const int vert_count, float* output,
					 LLStrider<LLVector3>& vertices, LLStrider<LLVector2>& texcoords, LLStrider<LLVector3>& normals, LLStrider<F32>& weights)
{
	F32 last_weight = F32_MAX;
	LLMatrix4 *blend_mat = &gBlendMat;

	for (S32 index = vert_offset; index < vert_offset + vert_count; index++)
	{
		F32     w  = weights  [index];        // register copy of weight
		F32  *vin  = &vertices[index].mV[0];  // pointer to input vertex data, assumed to be V3+T2+N3+whatever
		F32  *vout = output + index * (AVATAR_VERTEX_BYTES/sizeof(F32));    // pointer to the output vertex data, assumed to be 16 byte aligned

		if (w == last_weight)
		{
			// load input and output vertices, and last blended matrix
			__asm {
				mov		esi,  vin
				mov		edi,  vout

				mov		edx,  blend_mat
				movaps	xmm4, [edx]
				movaps	xmm5, [edx+0x10]
				movaps	xmm6, [edx+0x20]
				movaps	xmm7, [edx+0x30]
			}
		}
		else
		{
			last_weight = w;
			S32 joint = llfloor(w);
			w -= joint;

			LLMatrix4 *m0 = &(gJointMat[joint+1]);
			LLMatrix4 *m1 = &(gJointMat[joint+0]);

			// some initial code to load Matrix 0 into SSE registers
			__asm {
				mov		esi,  vin
				mov		edi,  vout

				//matrix2
				mov		edx,  m0
				movaps	xmm4, [edx]
				movaps	xmm5, [edx+0x10]
				movaps	xmm6, [edx+0x20]
				movaps	xmm7, [edx+0x30]
			};

			// if w == 1.0f, we don't need to blend.
			// but since we do the trick of blending the matrices, here, if w != 1.0,
			// we load Matrix 1 into the other 4 SSE registers and blend both matrices
			// based on the weight (which we load ingo a 16-byte aligned vector: w,w,w,w)

			if (w != 1.0f) 
			{
					F32vec4 weight(w);

				__asm { // do blending of matrices instead of verts and normals -- faster
					mov		edx,  m1
					movaps	xmm0, [edx]
					movaps	xmm1, [edx+0x10]
					movaps	xmm2, [edx+0x20]
					movaps	xmm3, [edx+0x30]

					subps	xmm4,  xmm0 // do blend for each matrix column
					subps	xmm5,  xmm1 // diff, then multiply weight and re-add
					subps	xmm6,  xmm2
					subps	xmm7,  xmm3

					mulps   xmm4,  weight
					mulps   xmm5,  weight
					mulps   xmm6,  weight
					mulps   xmm7,  weight

					addps   xmm4,  xmm0
					addps   xmm5,  xmm1
					addps   xmm6,  xmm2
					addps   xmm7,  xmm3
				};
			}

			__asm {
				// save off blended matrix
				mov		edx,   blend_mat;
				movaps	[edx], xmm4;
				movaps	[edx+0x10], xmm5;
				movaps	[edx+0x20], xmm6;
				movaps	[edx+0x30], xmm7;
			}
		}

		// now, we have either a blended matrix in xmm4-7 or the original Matrix 0
		// we then multiply each vertex and normal by this one matrix.

		// For SSE2, we would try to keep the original two matrices in other registers
		// and avoid reloading them. However, they should ramain in L1 cache in the 
		// current case.

		// One possible optimization would be to sort the vertices by weight instead
		// of just index (we still want to uniqify). If we note when two or more vertices
		// share the same weight, we can avoid doing the middle SSE code above and just
		// re-use the blended matrix for those vertices


		// now, we do the actual vertex blending
		__asm {			
			// load Vertex into xmm0.
			movaps	xmm0, [esi] // change aps to ups when input is no longer 16-baligned
			movaps	xmm1, xmm0  // copy vector into xmm0 through xmm2 (x,y,z)
			movaps	xmm2, xmm0
			shufps	xmm0, xmm0, _MM_SHUFFLE(0,0,0,0); // clone vertex (x) across vector
			shufps	xmm1, xmm1, _MM_SHUFFLE(1,1,1,1); // clone vertex (y) across vector
			shufps	xmm2, xmm2, _MM_SHUFFLE(2,2,2,2); // same for Z
			mulps	xmm0, xmm4 // do the actual matrix multipication for r0
			mulps	xmm1, xmm5 // for r1
			mulps	xmm2, xmm6 // for r2
			addps	xmm0, xmm1 // accumulate 
			addps	xmm0, xmm2 // accumulate
			addps	xmm0, xmm7 // add in the row 4 which holds the x,y,z translation. assumes w=1 (vertex-w, not weight)

			movaps  [edi], xmm0 // store aligned in output array

			// load Normal into xmm0.
			movaps	xmm0, [esi + 0x10]  // change aps to ups when input no longer 16-byte aligned
			movaps	xmm1, xmm0  // 
			movaps	xmm2, xmm0
			shufps	xmm0, xmm0, _MM_SHUFFLE(0,0,0,0); // since UV sits between vertex and normal, normal starts at element 1, not 0
			shufps	xmm1, xmm1, _MM_SHUFFLE(1,1,1,1);
			shufps	xmm2, xmm2, _MM_SHUFFLE(2,2,2,2);
			mulps	xmm0, xmm4 // multiply by matrix
			mulps	xmm1, xmm5 // multiply
			mulps	xmm2, xmm6 // multiply
			addps	xmm0, xmm1 // accumulate
			addps	xmm0, xmm2 // accumulate. note: do not add translation component to normals, save time too
			movaps  [edi + 0x10], xmm0 // store aligned 
		}

		*(LLVector2*)(vout + (AVATAR_OFFSET_TEX0/sizeof(F32))) = texcoords[index]; // write texcoord into appropriate spot. 
	}
}

#elif LL_LINUX

void blend_SSE_32_32_batch(const int vert_offset, const int vert_count, float* output,
					 LLStrider<LLVector3>& vertices, LLStrider<LLVector2>& texcoords, LLStrider<LLVector3>& normals, LLStrider<F32>& weights)
{
    assert(0);
}

#elif LL_DARWIN
// AltiVec versions of the same...

static inline vector float loadAlign(int offset, vector float *addr)
{
	vector float in0 = vec_ld(offset, addr);
	vector float in1 = vec_ld(offset + 16, addr);
	vector unsigned char perm = vec_lvsl(0, (unsigned char*)addr);
	
	return(vec_perm(in0, in1, perm));
}

static inline void storeAlign(vector float v, int offset, vector float *addr)
{
	vector float in0 = vec_ld(offset, addr);
	vector float in1 = vec_ld(offset + 16, addr);
	vector unsigned char perm = vec_lvsr(0, (unsigned char *)addr);
	vector float temp = vec_perm(v, v, perm);
	vector unsigned char mask = (vector unsigned char)vec_cmpgt(perm, vec_splat_u8(15));
	
	in0 = vec_sel(in0, temp, (vector unsigned int)mask);
	in1 = vec_sel(temp, in1, (vector unsigned int)mask);

	vec_st(in0, offset, addr);
	vec_st(in1, offset + 16, addr);
}

void blend_SSE_32_32_batch(const int vert_offset, const int vert_count, float* output,
					 LLStrider<LLVector3>& vertices, LLStrider<LLVector2>& texcoords, LLStrider<LLVector3>& normals, LLStrider<F32>& weights)
{
	F32 last_weight = F32_MAX;
//	LLMatrix4 &blend_mat = gBlendMat;

	vector float matrix0_0, matrix0_1, matrix0_2, matrix0_3;
	vector unsigned char out0perm = (vector unsigned char) ( 0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B, 0x0C,0x0D,0x0E,0x0F ); 
// 	vector unsigned char out1perm = (vector unsigned char) ( 0x00,0x01,0x02,0x03, 0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B ); 
	vector unsigned char out1perm = (vector unsigned char) ( 0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1A,0x1B, 0x0C,0x0D,0x0E,0x0F ); 

	vector float zero = (vector float)vec_splat_u32(0);

	for (U32 index = vert_offset; index < vert_offset + vert_count; index++)
	{
		F32     w  = weights  [index];        // register copy of weight
		F32  *vin  = &vertices[index].mV[0];  // pointer to input vertex data, assumed to be V3+T2+N3+whatever
		F32  *vout = output + index * (AVATAR_VERTEX_BYTES/sizeof(F32));    // pointer to the output vertex data, assumed to be 16 byte aligned
		
		// MBW -- XXX -- If this isn't the case, this code gets more complicated.
		if(0x0000000F & (U32)vin)
		{
			llerrs << "blend_SSE_batch: input not 16-byte aligned!" << llendl;
		}
		if(0x0000000F & (U32)vout)
		{
			llerrs << "blend_SSE_batch: output not 16-byte aligned!" << llendl;
		}
//		if(0x0000000F & (U32)&(blend_mat.mMatrix))
//		{
//			llerrs << "blend_SSE_batch: blend_mat not 16-byte aligned!" << llendl;
//		}
		
		if (w == last_weight)
		{
			// load last blended matrix
			// Still loaded from last time through the loop.
//			matrix0_0 = vec_ld(0x00, (vector float*)&(blend_mat.mMatrix));
//			matrix0_1 = vec_ld(0x10, (vector float*)&(blend_mat.mMatrix));
//			matrix0_2 = vec_ld(0x20, (vector float*)&(blend_mat.mMatrix));
//			matrix0_3 = vec_ld(0x30, (vector float*)&(blend_mat.mMatrix));
		}
		else
		{
			last_weight = w;
			S32 joint = llfloor(w);
			w -= joint;

			LLMatrix4 &m0 = gJointMat[joint+1];
			LLMatrix4 &m1 = gJointMat[joint+0];

			// load Matrix 0 into vector registers
			matrix0_0 = vec_ld(0x00, (vector float*)&(m0.mMatrix));
			matrix0_1 = vec_ld(0x10, (vector float*)&(m0.mMatrix));
			matrix0_2 = vec_ld(0x20, (vector float*)&(m0.mMatrix));
			matrix0_3 = vec_ld(0x30, (vector float*)&(m0.mMatrix));

			// if w == 1.0f, we don't need to blend.
			// but since we do the trick of blending the matrices, here, if w != 1.0,
			// we load Matrix 1 into the other 4 SSE registers and blend both matrices
			// based on the weight (which we load ingo a 16-byte aligned vector: w,w,w,w)

			if (w != 1.0f) 
			{
				vector float matrix1_0, matrix1_1, matrix1_2, matrix1_3;

				// This loads the weight somewhere in the vector register
				vector float weight = vec_lde(0, &(w));
				// and this splats it to all elements.
				weight = vec_splat(vec_perm(weight, weight, vec_lvsl(0, &(w))), 0);

				// do blending of matrices instead of verts and normals -- faster
				matrix1_0 = vec_ld(0x00, (vector float*)&(m1.mMatrix));
				matrix1_1 = vec_ld(0x10, (vector float*)&(m1.mMatrix));
				matrix1_2 = vec_ld(0x20, (vector float*)&(m1.mMatrix));
				matrix1_3 = vec_ld(0x30, (vector float*)&(m1.mMatrix));
				
				// m0[col] = ((m0[col] - m1[col]) * weight) + m1[col];
				matrix0_0 = vec_madd(vec_sub(matrix0_0, matrix1_0), weight, matrix1_0);
				matrix0_1 = vec_madd(vec_sub(matrix0_1, matrix1_1), weight, matrix1_1);
				matrix0_2 = vec_madd(vec_sub(matrix0_2, matrix1_2), weight, matrix1_2);
				matrix0_3 = vec_madd(vec_sub(matrix0_3, matrix1_3), weight, matrix1_3);
			}

			// save off blended matrix
//			vec_st(matrix0_0, 0x00, (vector float*)&(blend_mat.mMatrix));
//			vec_st(matrix0_1, 0x10, (vector float*)&(blend_mat.mMatrix));
//			vec_st(matrix0_2, 0x20, (vector float*)&(blend_mat.mMatrix));
//			vec_st(matrix0_3, 0x30, (vector float*)&(blend_mat.mMatrix));
		}

		// now, we have either a blended matrix in matrix0_0-3 or the original Matrix 0
		// we then multiply each vertex and normal by this one matrix.

		// For SSE2, we would try to keep the original two matrices in other registers
		// and avoid reloading them. However, they should ramain in L1 cache in the 
		// current case.

		// One possible optimization would be to sort the vertices by weight instead
		// of just index (we still want to uniqify). If we note when two or more vertices
		// share the same weight, we can avoid doing the middle SSE code above and just
		// re-use the blended matrix for those vertices


		// now, we do the actual vertex blending

		vector float in0 = vec_ld(AVATAR_OFFSET_POS, (vector float*)vin);
		vector float in1 = vec_ld(AVATAR_OFFSET_NORMAL, (vector float*)vin);
	
		// Matrix multiply vertex
		vector float out0 = vec_madd
		(
			vec_splat(in0, 0), 
			matrix0_0, 
			vec_madd
			(
				vec_splat(in0, 1),
				matrix0_1,
				vec_madd
				(
					vec_splat(in0, 2),
					matrix0_2,
					matrix0_3
				)
			)
		);
		
		// Matrix multiply normal
		vector float out1 = vec_madd
		(
			vec_splat(in1, 0), 
			matrix0_0, 
			vec_madd
			(
				vec_splat(in1, 1),
				matrix0_1,
				vec_madd
				(
					vec_splat(in1, 2),
					matrix0_2,
					// no translation for normals
					(vector float)vec_splat_u32(0)
				)
			)
		);

		// indexed store
		vec_stl(vec_perm(in0, out0, out0perm), AVATAR_OFFSET_POS,    (vector float*)vout); // Pos
		vec_stl(vec_perm(in1, out1, out1perm), AVATAR_OFFSET_NORMAL, (vector float*)vout); // Norm
		*(LLVector2*)(vout + (AVATAR_OFFSET_TEX0/sizeof(F32))) = texcoords[index]; // write texcoord into appropriate spot. 
	}
}

#endif


void llDrawElementsBatchBlend(const U32 vert_offset, const U32 vert_count, LLFace *face, const S32 index_count, const U32 *indices)
{
	U8* gAGPVertices = gPipeline.bufferGetScratchMemory();
	
	if (gAGPVertices)
	{
		LLStrider<LLVector3> vertices;
		LLStrider<LLVector3> normals; 
		LLStrider<LLVector2> tcoords0;
		LLStrider<F32>       weights; 

		LLStrider<LLVector3> o_vertices;
		LLStrider<LLVector3> o_normals;
		LLStrider<LLVector2> o_texcoords0;

		
		LLStrider<LLVector3> binormals; 
		LLStrider<LLVector2> o_texcoords1;
		// get the source vertices from the draw pool. We index these ourselves, as there was
		// no guarantee the indices for a single jointmesh were contigious
		
		LLDrawPool *pool = face->getPool();
		pool->getVertexStrider      (vertices,  0);
		pool->getTexCoordStrider   (tcoords0,  0, 0);
		pool->getNormalStrider      (normals,   0);
		pool->getBinormalStrider    (binormals, 0);
		pool->getVertexWeightStrider(weights,   0);

		// load the addresses of the output striders
		o_vertices  = (LLVector3*)(gAGPVertices + AVATAR_OFFSET_POS);		o_vertices.setStride(  AVATAR_VERTEX_BYTES);
		o_normals   = (LLVector3*)(gAGPVertices + AVATAR_OFFSET_NORMAL);	o_normals.setStride(   AVATAR_VERTEX_BYTES);
		o_texcoords0= (LLVector2*)(gAGPVertices + AVATAR_OFFSET_TEX0);		o_texcoords0.setStride(AVATAR_VERTEX_BYTES);
		o_texcoords1= (LLVector2*)(gAGPVertices + AVATAR_OFFSET_TEX1);		o_texcoords1.setStride(AVATAR_VERTEX_BYTES);

#if !LL_LINUX // !!! FIXME
		if (gGLManager.mSoftwareBlendSSE)
		{
			// do SSE blend without binormals or extra texcoords
			blend_SSE_32_32_batch(vert_offset, vert_count, (float*)gAGPVertices,
								  vertices, tcoords0, normals, weights);
		}
		else // fully backwards compatible software blending, no SSE
#endif
		{
			LLVector4 tpos0, tnorm0, tpos1, tnorm1, tbinorm0, tbinorm1;
			F32 last_weight = F32_MAX;
			LLMatrix3 gBlendRotMat;

			{
				for (U32 index=vert_offset; index < vert_offset + vert_count; index++)
				{
					// blend by first matrix
					F32     w = weights [index];
					
					if (w != last_weight)
					{
						last_weight = w;

						S32 joint = llfloor(w);
						w -= joint;

						LLMatrix4 &m0 = gJointMat[joint+1];
						LLMatrix4 &m1 = gJointMat[joint+0];
						LLMatrix3 &n0 = gJointRot[joint+1];
						LLMatrix3 &n1 = gJointRot[joint+0];

						if (w == 1.0f)
						{
							gBlendMat = m0;
							gBlendRotMat = n0;
						}	
						else
						{
							gBlendMat.mMatrix[VX][VX] = lerp(m1.mMatrix[VX][VX], m0.mMatrix[VX][VX], w);
							gBlendMat.mMatrix[VX][VY] = lerp(m1.mMatrix[VX][VY], m0.mMatrix[VX][VY], w);
							gBlendMat.mMatrix[VX][VZ] = lerp(m1.mMatrix[VX][VZ], m0.mMatrix[VX][VZ], w);

							gBlendMat.mMatrix[VY][VX] = lerp(m1.mMatrix[VY][VX], m0.mMatrix[VY][VX], w);
							gBlendMat.mMatrix[VY][VY] = lerp(m1.mMatrix[VY][VY], m0.mMatrix[VY][VY], w);
							gBlendMat.mMatrix[VY][VZ] = lerp(m1.mMatrix[VY][VZ], m0.mMatrix[VY][VZ], w);

							gBlendMat.mMatrix[VZ][VX] = lerp(m1.mMatrix[VZ][VX], m0.mMatrix[VZ][VX], w);
							gBlendMat.mMatrix[VZ][VY] = lerp(m1.mMatrix[VZ][VY], m0.mMatrix[VZ][VY], w);
							gBlendMat.mMatrix[VZ][VZ] = lerp(m1.mMatrix[VZ][VZ], m0.mMatrix[VZ][VZ], w);

							gBlendMat.mMatrix[VW][VX] = lerp(m1.mMatrix[VW][VX], m0.mMatrix[VW][VX], w);
							gBlendMat.mMatrix[VW][VY] = lerp(m1.mMatrix[VW][VY], m0.mMatrix[VW][VY], w);
							gBlendMat.mMatrix[VW][VZ] = lerp(m1.mMatrix[VW][VZ], m0.mMatrix[VW][VZ], w);

							gBlendRotMat.mMatrix[VX][VX] = lerp(n1.mMatrix[VX][VX], n0.mMatrix[VX][VX], w);
							gBlendRotMat.mMatrix[VX][VY] = lerp(n1.mMatrix[VX][VY], n0.mMatrix[VX][VY], w);
							gBlendRotMat.mMatrix[VX][VZ] = lerp(n1.mMatrix[VX][VZ], n0.mMatrix[VX][VZ], w);

							gBlendRotMat.mMatrix[VY][VX] = lerp(n1.mMatrix[VY][VX], n0.mMatrix[VY][VX], w);
							gBlendRotMat.mMatrix[VY][VY] = lerp(n1.mMatrix[VY][VY], n0.mMatrix[VY][VY], w);
							gBlendRotMat.mMatrix[VY][VZ] = lerp(n1.mMatrix[VY][VZ], n0.mMatrix[VY][VZ], w);

							gBlendRotMat.mMatrix[VZ][VX] = lerp(n1.mMatrix[VZ][VX], n0.mMatrix[VZ][VX], w);
							gBlendRotMat.mMatrix[VZ][VY] = lerp(n1.mMatrix[VZ][VY], n0.mMatrix[VZ][VY], w);
							gBlendRotMat.mMatrix[VZ][VZ] = lerp(n1.mMatrix[VZ][VZ], n0.mMatrix[VZ][VZ], w);
						}
					}

					// write result
					o_vertices  [index]     = vertices[index] * gBlendMat;
					o_normals   [index]     = normals [index] * gBlendRotMat;
					o_texcoords0[index]		= tcoords0[index];

					/*
					// Verification code.  Leave this here.  It's useful for keeping the SSE and non-SSE versions in sync.
					LLVector3 temp;
					temp = tpos0;
					if( (o_vertices[index] - temp).magVecSquared() > 0.001f )
					{
						llerrs << "V SSE: " << o_vertices[index] << " v. " << temp << llendl;
					}
	
					temp = tnorm0;
					if( (o_normals[index] - temp).magVecSquared() > 0.001f )
					{
						llerrs << "N SSE: " << o_normals[index] << " v. " << temp << llendl;
					}
	
					if( (o_texcoords0[index] - tcoords0[index]).magVecSquared() > 0.001f )
					{
						llerrs << "T0 SSE: " << o_texcoords0[index] << " v. " << tcoords0[index] << llendl;
					}
					*/
				}
			}
		}

#if LL_DARWIN
		// *HACK* *CHOKE* *PUKE*
		// No way does this belong here.
		glFlushVertexArrayRangeAPPLE(AVATAR_VERTEX_BYTES * vert_count, gAGPVertices + (AVATAR_VERTEX_BYTES * vert_offset));
#endif
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, indices); // draw it!
	}
	else
	{
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, indices);
	}
}



//--------------------------------------------------------------------
// DrawElements

// works just like glDrawElements, except it assumes GL_TRIANGLES and GL_UNSIGNED_INT indices

// why? because the destination buffer may not be the AGP buffer and the eyes do not use blending
// separate the eyes into their own drawpools and this code goes away.

//--------------------------------------------------------------------

void llDrawElements(const S32 count, const U32 *indices, LLFace *face)
{
	U8* gAGPVertices = gPipeline.bufferGetScratchMemory();
	
	if (gAGPVertices)
	{
#if LL_DARWIN
		U32   minIndex = indices[0];
		U32   maxIndex = indices[0];
#endif
		{
			LLStrider<LLVector3> vertices;
			LLStrider<LLVector3> normals; 
			LLStrider<LLVector2> tcoords;
			LLStrider<F32>       weights; 

			LLStrider<LLVector3> o_vertices;
			LLStrider<LLVector3> o_normals;
			LLStrider<LLVector2> o_texcoords0;

			LLDrawPool *pool = face->getPool();
			pool->getVertexStrider      (vertices,0);
			pool->getNormalStrider      (normals, 0);
			pool->getTexCoordStrider    (tcoords, 0);

			o_vertices  = (LLVector3*)(gAGPVertices + AVATAR_OFFSET_POS);		o_vertices.setStride(  AVATAR_VERTEX_BYTES);
			o_normals   = (LLVector3*)(gAGPVertices + AVATAR_OFFSET_NORMAL);	o_normals.setStride(   AVATAR_VERTEX_BYTES);
			o_texcoords0= (LLVector2*)(gAGPVertices + AVATAR_OFFSET_TEX0);		o_texcoords0.setStride(AVATAR_VERTEX_BYTES);

			for (S32 i=0; i < count; i++)
			{
				U32   index = indices[i];

				o_vertices  [index] = vertices[index];
				o_normals   [index] = normals [index];
				o_texcoords0[index] = tcoords [index];

#if LL_DARWIN
				maxIndex = llmax(index, maxIndex);
				minIndex = llmin(index, minIndex);
#endif
			}
		}

#if LL_DARWIN
		// *HACK* *CHOKE* *PUKE*
		// No way does this belong here.
		glFlushVertexArrayRangeAPPLE(AVATAR_VERTEX_BYTES * (maxIndex + 1 - minIndex), gAGPVertices + (AVATAR_VERTEX_BYTES * minIndex));
#endif

		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
	}
	else
	{
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
	}
}


//--------------------------------------------------------------------
// LLViewerJointMesh::drawShape()
//--------------------------------------------------------------------
U32 LLViewerJointMesh::drawShape( F32 pixelArea )
{
	if (!mValid || !mVisible) return 0;

	U32 triangle_count = 0;

	//----------------------------------------------------------------
	// if no mesh bail out now
	//----------------------------------------------------------------
	if ( !mMesh || !mFace)
	{
		return 0;
	}

	//----------------------------------------------------------------
	// if we have no faces, bail out now
	//----------------------------------------------------------------
	if ( mMesh->getNumFaces() == 0 )
	{
		return 0;
	}

	stop_glerror();
	
	//----------------------------------------------------------------
	// setup current color
	//----------------------------------------------------------------
	if (gRenderForSelect)
	{
		S32 name = mFace->getDrawable() ? mFace->getDrawable()->getVObj()->mGLName : 0;
		LLColor4U color((U8)(name >> 16), (U8)(name >> 8), (U8)name, 0xff);
		LLColor4 color_float(color);
	
		glColor4f(color_float.mV[0], color_float.mV[1], color_float.mV[2], 1.f);
	}
	else
	{
		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			glColor4f(0,0,0,1);
			
			if (gPipeline.mMaterialIndex > 0)
			{
				glVertexAttrib4fvARB(gPipeline.mMaterialIndex, mColor.mV);
			}
			
			if (mShiny && gPipeline.mSpecularIndex > 0)
			{
				glVertexAttrib4fARB(gPipeline.mSpecularIndex, 1,1,1,1);
			}
		}
		else
		{
			glColor4fv(mColor.mV);
		}
	}

	stop_glerror();
	
// 	LLGLSSpecular specular(mSpecular, gRenderForSelect ? 0.0f : mShiny);
	LLGLSSpecular specular(LLColor4(1.f,1.f,1.f,1.f), gRenderForSelect ? 0.0f : mShiny && !(mFace->getPool()->getVertexShaderLevel() > 0));

	LLGLEnable texture_2d((gRenderForSelect && isTransparent()) ? GL_TEXTURE_2D : 0);
	
	//----------------------------------------------------------------
	// setup current texture
	//----------------------------------------------------------------
	llassert( !(mTexture.notNull() && mLayerSet) );  // mutually exclusive

	//GLuint test_image_name = 0;

	// 
	LLGLState force_alpha_test(GL_ALPHA_TEST, isTransparent());

	if (mTestImageName)
	{
		LLImageGL::bindExternalTexture( mTestImageName, 0, GL_TEXTURE_2D ); 

		if (mIsTransparent)
		{
			glColor4f(1.f, 1.f, 1.f, 1.f);
		}
		else
		{
			glColor4f(0.7f, 0.6f, 0.3f, 1.f);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_INTERPOLATE_ARB);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);
			
			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,		GL_ONE_MINUS_SRC_ALPHA);
		}
	}
	else if( mLayerSet )
	{
		if(	mLayerSet->hasComposite() )
		{
			mLayerSet->getComposite()->bindTexture();
		}
		else
		{
			llwarns << "Layerset without composite" << llendl;
			gImageList.getImage(IMG_DEFAULT)->bind();
		}
	}
	else
	if ( mTexture.notNull() )
	{
		mTexture->bind();
		if (!mTexture->getClampS()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		}
		if (!mTexture->getClampT()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	else
	{
		gImageList.getImage(IMG_DEFAULT_AVATAR)->bind();
	}
	
	if (gRenderForSelect)
	{
		if (isTransparent())
		{
			//gGLSObjectSelectDepthAlpha.set();
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);  // GL_TEXTURE_ENV_COLOR is set in renderPass1
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);
		}
		else
		{
			//gGLSObjectSelectDepth.set();
		}
	}
	else
	{
		//----------------------------------------------------------------
		// by default, backface culling is enabled
		//----------------------------------------------------------------
		if (sRenderPass == AVATAR_RENDER_PASS_CLOTHING_INNER)
		{
			//LLGLSPipelineAvatar gls_pipeline_avatar;
			LLImageGL::bindExternalTexture( sClothingMaskImageName, 1, GL_TEXTURE_2D );

			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D); // Texture unit 1
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR,	sClothingInnerColor.mV);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_INTERPOLATE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_CONSTANT_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB,		GL_SRC_ALPHA);
		}
		else if (sRenderPass == AVATAR_RENDER_PASS_CLOTHING_OUTER)
		{
			//gGLSPipelineAvatarAlphaPass1.set();
			glAlphaFunc(GL_GREATER, 0.1f);
			LLImageGL::bindExternalTexture( sClothingMaskImageName, 1, GL_TEXTURE_2D );

			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_REPLACE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB,		GL_PRIMARY_COLOR_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB,		GL_SRC_COLOR);

			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnable(GL_TEXTURE_2D); // Texture unit 1
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

			glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);
		}
		else if ( isTransparent())
		{
			//gGLSNoCullFaces.set();
		}
		else
		{
			//gGLSCullFaces.set();
		}
	}

	if (mMesh->hasWeights())
	{
		uploadJointMatrices();


		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			glDrawElements(GL_TRIANGLES, mMesh->mFaceIndexCount, GL_UNSIGNED_INT, mMesh->getIndices());

			glPopMatrix();
		}
		else
		{
			if (mFace->getGeomIndex() < 0)
			{
				llerrs << "Invalid geometry index in LLViewerJointMesh::drawShape() " << mFace->getGeomIndex() << llendl;
			}

			if ((S32)(mMesh->mFaceVertexOffset + mMesh->mFaceVertexCount) > mFace->getGeomCount())
			{
				((LLVOAvatar*)mFace->getDrawable()->getVObj())->mRoot.dump();
				llerrs << "Rendering outside of vertex bounds with mesh " << mName << " at pixel area " << pixelArea << llendl;
			}
			llDrawElementsBatchBlend(mMesh->mFaceVertexOffset, mMesh->mFaceVertexCount,
									 mFace, mMesh->mFaceIndexCount, mMesh->getIndices());
		}

	}
	else
	{
		glPushMatrix();
		LLMatrix4 jointToWorld = getWorldMatrix();
		jointToWorld *= gCamera->getModelview();
		glLoadMatrixf((GLfloat*)jointToWorld.mMatrix);

		if ((mFace->getPool()->getVertexShaderLevel() > 0))
		{
			glDrawElements(GL_TRIANGLES, mMesh->mFaceIndexCount, GL_UNSIGNED_INT, mMesh->getIndices());
		}
		else // this else clause handles non-weighted vertices. llDrawElements just copies and draws
		{
			llDrawElements(mMesh->mFaceIndexCount, mMesh->getIndices(), mFace);
		}
		
		glPopMatrix();
	}

	triangle_count += mMesh->mFaceIndexCount;

	if (gRenderForSelect)
	{
		glColor4fv(mColor.mV);
	}

	if (mTestImageName)
	{
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}

	if (sRenderPass != AVATAR_RENDER_PASS_SINGLE)
	{
		LLImageGL::unbindTexture(1, GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);

		// Return to the default texture.
		LLImageGL::unbindTexture(0, GL_TEXTURE_2D);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glActiveTextureARB(GL_TEXTURE0_ARB);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_MODULATE);
		glAlphaFunc(GL_GREATER, 0.01f);
	}

	if (mTexture.notNull()) {
		if (!mTexture->getClampS()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}
		if (!mTexture->getClampT()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	}

	return triangle_count;
}

//-----------------------------------------------------------------------------
// updateFaceSizes()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::updateFaceSizes(U32 &num_vertices, F32 pixel_area)
{
	// Do a pre-alloc pass to determine sizes of data.
	if (mMesh && mValid)
	{
		mMesh->mFaceVertexOffset = num_vertices;
		mMesh->mFaceVertexCount = mMesh->getNumVertices();
		mMesh->getReferenceMesh()->mCurVertexCount = mMesh->mFaceVertexCount;
		num_vertices += mMesh->getNumVertices();

		mMesh->mFaceIndexCount = mMesh->getSharedData()->mNumTriangleIndices;
	
		mMesh->getSharedData()->genIndices(mMesh->mFaceVertexOffset);
	}
}

//-----------------------------------------------------------------------------
// updateFaceData()
//-----------------------------------------------------------------------------
void LLViewerJointMesh::updateFaceData(LLFace *face, F32 pixel_area, BOOL damp_wind)
{
	U32 i;

	if (!mValid) return;

	mFace = face;

	LLStrider<LLVector3> verticesp;
	LLStrider<LLVector3> normalsp;
	LLStrider<LLVector3> binormalsp;
	LLStrider<LLVector2> tex_coordsp;
	LLStrider<F32>		 vertex_weightsp;
	LLStrider<LLVector4> clothing_weightsp;

	// Copy data into the faces from the polymesh data.
	if (mMesh)
	{
		if (mMesh->getNumVertices())
		{
			S32 index = face->getGeometryAvatar(verticesp, normalsp, binormalsp, tex_coordsp, vertex_weightsp, clothing_weightsp);

			if (-1 == index)
			{
				return;
			}

			for (i = 0; i < mMesh->getNumVertices(); i++)
			{
				verticesp[mMesh->mFaceVertexOffset + i] = *(mMesh->getCoords() + i);
				tex_coordsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getTexCoords() + i);
				normalsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getNormals() + i);
				binormalsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getBinormals() + i);
				vertex_weightsp[mMesh->mFaceVertexOffset + i] = *(mMesh->getWeights() + i);
				if (damp_wind)
				{
					clothing_weightsp[mMesh->mFaceVertexOffset + i].setVec(0,0,0,0);
				}
				else
				{
					clothing_weightsp[mMesh->mFaceVertexOffset + i].setVec(*(mMesh->getClothingWeights() + i));
				}
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

void LLViewerJointMesh::dump()
{
	if (mValid)
	{
		llinfos << "Usable LOD " << mName << llendl;
	}
}

void LLViewerJointMesh::writeCAL3D(apr_file_t* fp, S32 material_num, LLCharacter* characterp)
{
	apr_file_printf(fp, "\t<SUBMESH NUMVERTICES=\"%d\" NUMFACES=\"%d\" MATERIAL=\"%d\" NUMLODSTEPS=\"0\" NUMSPRINGS=\"0\" NUMTEXCOORDS=\"1\">\n", mMesh->getNumVertices(), mMesh->getNumFaces(), material_num);

	const LLVector3* mesh_coords = mMesh->getCoords();
	const LLVector3* mesh_normals = mMesh->getNormals();
	const LLVector2* mesh_uvs = mMesh->getTexCoords();
	const F32* mesh_weights = mMesh->getWeights();
	LLVector3 mesh_offset;
	LLVector3 scale(1.f, 1.f, 1.f);
	S32 joint_a = -1;
	S32 joint_b = -1;
	S32 num_bound_joints = 0;
	
	if(!mMesh->hasWeights())
	{
		num_bound_joints = 1;
		LLJoint* cur_joint = this;
		while(cur_joint)
		{
			if (cur_joint->mJointNum != -1 && joint_a == -1)
			{
				joint_a = cur_joint->mJointNum;
			}
			mesh_offset += cur_joint->getSkinOffset();
			cur_joint = cur_joint->getParent();
		}
	}

	for (S32 i = 0; i < (S32)mMesh->getNumVertices(); i++)
	{
		LLVector3 coord = mesh_coords[i];

		if (mMesh->hasWeights())
		{
			// calculate joint to which this skinned vertex is bound
			num_bound_joints = getBoundJointsByIndex(llfloor(mesh_weights[i]), joint_a, joint_b);
			LLJoint* first_joint = characterp->getCharacterJoint(joint_a);
			LLJoint* second_joint = characterp->getCharacterJoint(joint_b);

			LLVector3 first_joint_offset;
			LLJoint* cur_joint = first_joint;
			while(cur_joint)
			{
				first_joint_offset += cur_joint->getSkinOffset();
				cur_joint = cur_joint->getParent();
			}

			LLVector3 second_joint_offset;
			cur_joint = second_joint;
			while(cur_joint)
			{
				second_joint_offset += cur_joint->getSkinOffset();
				cur_joint = cur_joint->getParent();
			}

			LLVector3 first_coord = coord - first_joint_offset;
			first_coord.scaleVec(first_joint->getScale());
			LLVector3 second_coord = coord - second_joint_offset;
			if (second_joint)
			{
				second_coord.scaleVec(second_joint->getScale());
			}
			
			coord = lerp(first_joint_offset + first_coord, second_joint_offset + second_coord, fmodf(mesh_weights[i], 1.f));
		}

		// add offset to move rigid mesh to target location
		coord += mesh_offset;
		coord *= 100.f;

		apr_file_printf(fp, "		<VERTEX ID=\"%d\" NUMINFLUENCES=\"%d\">\n", i, num_bound_joints);
		apr_file_printf(fp, "			<POS>%.4f %.4f %.4f</POS>\n", coord.mV[VX], coord.mV[VY], coord.mV[VZ]);
		apr_file_printf(fp, "			<NORM>%.6f %.6f %.6f</NORM>\n", mesh_normals[i].mV[VX], mesh_normals[i].mV[VY], mesh_normals[i].mV[VZ]);
		apr_file_printf(fp, "			<TEXCOORD>%.6f %.6f</TEXCOORD>\n", mesh_uvs[i].mV[VX], 1.f - mesh_uvs[i].mV[VY]);
		if (num_bound_joints >= 1)
		{
			apr_file_printf(fp, "			<INFLUENCE ID=\"%d\">%.2f</INFLUENCE>\n", joint_a + 1, 1.f - fmod(mesh_weights[i], 1.f));
		}
		if (num_bound_joints == 2)
		{
			apr_file_printf(fp, "			<INFLUENCE ID=\"%d\">%.2f</INFLUENCE>\n", joint_b + 1, fmod(mesh_weights[i], 1.f));
		}
		apr_file_printf(fp, "		</VERTEX>\n");
	}

	LLPolyFace* mesh_faces = mMesh->getFaces();
	for (S32 i = 0; i < mMesh->getNumFaces(); i++)
	{
		apr_file_printf(fp, "		<FACE VERTEXID=\"%d %d %d\" />\n", mesh_faces[i][0], mesh_faces[i][1], mesh_faces[i][2]);
	}
	
	apr_file_printf(fp, "	</SUBMESH>\n");
}

// End
