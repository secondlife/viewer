/** 
 * @file llpolymesh.h
 * @brief Implementation of LLPolyMesh class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#ifndef LL_LLPOLYMESH_H
#define LL_LLPOLYMESH_H

#include <string>
#include <map>
#include "llstl.h"

#include "v3math.h"
#include "v2math.h"
#include "llquaternion.h"
#include "llskipmap.h"
#include "llassoclist.h"
#include "llpolymorph.h"
#include "llptrskipmap.h"
#include "lljoint.h"
//#include "lldarray.h"

class LLSkinJoint;
class LLVOAvatar;

//#define USE_STRIPS	// Use tri-strips for rendering.

//-----------------------------------------------------------------------------
// LLPolyFace
// A set of 4 vertex indices.
// An LLPolyFace can represent either a triangle or quad.
// If the last index is -1, it's a triangle.
//-----------------------------------------------------------------------------
typedef S32 LLPolyFace[3];

//struct PrimitiveGroup;

//-----------------------------------------------------------------------------
// LLPolyMesh
// A polyhedra consisting of any number of triangles and quads.
// All instances contain a set of faces, and optionally may include
// faces grouped into named face sets.
//-----------------------------------------------------------------------------
class LLPolyMorphTarget;

class LLPolyMeshSharedData
{
	friend class LLPolyMesh;
private:
	// transform data
	LLVector3				mPosition;
	LLQuaternion			mRotation;
	LLVector3				mScale;
							
	// vertex data			
	S32						mNumVertices;
	LLVector3				*mBaseCoords;
	LLVector3				*mBaseNormals;
	LLVector3				*mBaseBinormals;
	LLVector2				*mTexCoords;
	LLVector2				*mDetailTexCoords;
	F32						*mWeights;
	
	BOOL					mHasWeights;
	BOOL					mHasDetailTexCoords;

	// face data			
	S32						mNumFaces;
	LLPolyFace				*mFaces;
							
	// face set data		
	U32						mNumJointNames;
	std::string*			mJointNames;

	// morph targets
	typedef LLLinkedList<LLPolyMorphData> LLPolyMorphDataList;
	LLPolyMorphDataList			mMorphData;

	std::map<S32, S32> 			mSharedVerts;
	
	LLPolyMeshSharedData*		mReferenceData;
	S32							mLastIndexOffset;

public:
	// Temporarily...
	// Triangle indices
	U32				mNumTriangleIndices;
	U32				*mTriangleIndices;

private:
	LLPolyMeshSharedData();

	~LLPolyMeshSharedData();

	void setupLOD(LLPolyMeshSharedData* reference_data);

	// Frees all mesh memory resources 
	void freeMeshData();

	void setPosition( const LLVector3 &pos ) { 	mPosition = pos; }
	void setRotation( const LLQuaternion &rot ) { mRotation = rot; }
	void setScale( const LLVector3 &scale ) { mScale = scale; }

	BOOL allocateVertexData( U32 numVertices );

	BOOL allocateFaceData( U32 numFaces );

	BOOL allocateJointNames( U32 numJointNames );

	// Retrieve the number of KB of memory used by this instance
	U32 getNumKB();

	// Load mesh data from file
	BOOL loadMesh( const char *fileName );

public:
	void genIndices(S32 offset);

	const LLVector2 &getUVs(U32 index);

	const S32	*getSharedVert(S32 vert);

	BOOL isLOD() { return (mReferenceData != NULL); }
};


class LLJointRenderData
{
public:
	LLJointRenderData(const LLMatrix4* world_matrix, LLSkinJoint* skin_joint) : mWorldMatrix(world_matrix), mSkinJoint(skin_joint){}
	~LLJointRenderData(){}

	const LLMatrix4*		mWorldMatrix;
	LLSkinJoint*			mSkinJoint;
};


class LLPolyMesh
{
public:
	
	// Constructor
	LLPolyMesh(LLPolyMeshSharedData *shared_data, LLPolyMesh *reference_mesh);

	// Destructor 
	~LLPolyMesh();

	// Requests a mesh by name.
	// If the mesh already exists in the global mesh table, it is returned,
	// otherwise it is loaded from file, added to the table, and returned.
	static LLPolyMesh *getMesh( const LLString &name, LLPolyMesh* reference_mesh = NULL);

	// Frees all loaded meshes.
	// This should only be called once you know there are no outstanding
	// references to these objects.  Generally, upon exit of the application.
	static void freeAllMeshes();

	//--------------------------------------------------------------------
	// Transform Data Access
	//--------------------------------------------------------------------
	// Get position
	const LLVector3 &getPosition() { 
		llassert (mSharedData);
		return mSharedData->mPosition; 
	}

	// Get rotation
	const LLQuaternion &getRotation() { 
		llassert (mSharedData);
		return mSharedData->mRotation; 
	}

	// Get scale
	const LLVector3 &getScale() { 
		llassert (mSharedData);
		return mSharedData->mScale; 
	}

	//--------------------------------------------------------------------
	// Vertex Data Access
	//--------------------------------------------------------------------
	// Get number of vertices
	U32 getNumVertices() { 
		llassert (mSharedData);
		return mSharedData->mNumVertices; 
	}

	// Returns whether or not the mesh has detail texture coords
	BOOL hasDetailTexCoords() { 
		llassert (mSharedData);
		return mSharedData->mHasDetailTexCoords; 
	}

	// Returns whether or not the mesh has vertex weights
	BOOL hasWeights() const{ 
		llassert (mSharedData);
		return mSharedData->mHasWeights; 
	}

	// Get coords
	const LLVector3	*getCoords() const{
		return mCoords;
	}

	// non const version
	LLVector3 *getWritableCoords();

	// Get normals
	const LLVector3	*getNormals() const{ 
		return mNormals; 
	}

	// Get normals
	const LLVector3	*getBinormals() const{ 
		return mBinormals; 
	}

	// Get base mesh normals
	const LLVector3 *getBaseNormals() const{
		llassert(mSharedData);
		return mSharedData->mBaseNormals;
	}

	// Get base mesh normals
	const LLVector3 *getBaseBinormals() const{
		llassert(mSharedData);
		return mSharedData->mBaseBinormals;
	}

	// intermediate morphed normals and output normals
	LLVector3 *getWritableNormals();
	LLVector3 *getScaledNormals();

	LLVector3 *getWritableBinormals();
	LLVector3 *getScaledBinormals();

	// Get texCoords
	const LLVector2	*getTexCoords() const { 
		return mTexCoords; 
	}

	// non const version
	LLVector2 *getWritableTexCoords();

	// Get detailTexCoords
	const LLVector2	*getDetailTexCoords() const { 
		llassert (mSharedData);
		return mSharedData->mDetailTexCoords; 
	}

	// Get weights
	const F32 *getWeights() const {
		llassert (mSharedData);
		return mSharedData->mWeights;
	}

	F32			*getWritableWeights() const;

	LLVector4	*getWritableClothingWeights();

	const LLVector4		*getClothingWeights()
	{
		return mClothingWeights;	
	}

	//--------------------------------------------------------------------
	// Face Data Access
	//--------------------------------------------------------------------
	// Get number of faces
	S32 getNumFaces() { 
		llassert (mSharedData);
		return mSharedData->mNumFaces; 
	}

	// Get faces
	LLPolyFace *getFaces() { 
		llassert (mSharedData);
		return mSharedData->mFaces;
	}

	U32 getNumJointNames() { 
		llassert (mSharedData);
		return mSharedData->mNumJointNames; 
	}

	std::string *getJointNames() { 
		llassert (mSharedData);
		return mSharedData->mJointNames;
	}

	LLPolyMorphData*	getMorphData(const char *morph_name);
	void	removeMorphData(LLPolyMorphData *morph_target);
	void	deleteAllMorphData();

	LLPolyMeshSharedData *getSharedData() const;
	LLPolyMesh *getReferenceMesh() { return mReferenceMesh ? mReferenceMesh : this; }

	// Get indices
	U32*	getIndices() { return mSharedData ? mSharedData->mTriangleIndices : NULL; }

	BOOL	isLOD() { return mSharedData && mSharedData->isLOD(); }

	void setAvatar(LLVOAvatar* avatarp) { mAvatarp = avatarp; }
	LLVOAvatar* getAvatar() { return mAvatarp; }

	LLDynamicArray<LLJointRenderData*>	mJointRenderData;

	U32				mFaceVertexOffset;
	U32				mFaceVertexCount;
	U32				mFaceIndexOffset;
	U32				mFaceIndexCount;
	U32				mCurVertexCount;
private:
	void initializeForMorph();

	// Dumps diagnostic information about the global mesh table
	static void dumpDiagInfo();

protected:
	// mesh data shared across all instances of a given mesh
	LLPolyMeshSharedData	*mSharedData;
	// Single array of floats for allocation / deletion
	F32						*mVertexData;
	// deformed vertices (resulting from application of morph targets)
	LLVector3				*mCoords;
	// deformed normals (resulting from application of morph targets)
	LLVector3				*mScaledNormals;
	// output normals (after normalization)
	LLVector3				*mNormals;
	// deformed binormals (resulting from application of morph targets)
	LLVector3				*mScaledBinormals;
	// output binormals (after normalization)
	LLVector3				*mBinormals;
	// weight values that mark verts as clothing/skin
	LLVector4				*mClothingWeights;
	// output texture coordinates
	LLVector2				*mTexCoords;
	
	LLPolyMesh				*mReferenceMesh;

	// global mesh list
	typedef LLAssocList<std::string, LLPolyMeshSharedData*> LLPolyMeshSharedDataTable; 
	static LLPolyMeshSharedDataTable sGlobalSharedMeshList;

	// Backlink only; don't make this an LLPointer.
	LLVOAvatar* mAvatarp;
};

//-----------------------------------------------------------------------------
// LLPolySkeletalDeformationInfo
// Shared information for LLPolySkeletalDeformations
//-----------------------------------------------------------------------------
struct LLPolySkeletalBoneInfo
{
	LLPolySkeletalBoneInfo(LLString &name, LLVector3 &scale, LLVector3 &pos, BOOL haspos)
		: mBoneName(name),
		  mScaleDeformation(scale),
		  mPositionDeformation(pos),
		  mHasPositionDeformation(haspos) {}
	LLString mBoneName;
	LLVector3 mScaleDeformation;
	LLVector3 mPositionDeformation;
	BOOL mHasPositionDeformation;
};

class LLPolySkeletalDistortionInfo : public LLViewerVisualParamInfo
{
	friend class LLPolySkeletalDistortion;
public:
	LLPolySkeletalDistortionInfo();
	/*virtual*/ ~LLPolySkeletalDistortionInfo() {};
	
	/*virtual*/ BOOL parseXml(LLXmlTreeNode* node);

protected:
	typedef std::vector<LLPolySkeletalBoneInfo> bone_info_list_t;
	bone_info_list_t mBoneInfoList;
};

//-----------------------------------------------------------------------------
// LLPolySkeletalDeformation
// A set of joint scale data for deforming the avatar mesh
//-----------------------------------------------------------------------------
class LLPolySkeletalDistortion : public LLViewerVisualParam
{
public:
	LLPolySkeletalDistortion(LLVOAvatar *avatarp);
	~LLPolySkeletalDistortion();

	// Special: These functions are overridden by child classes
	LLPolySkeletalDistortionInfo*	getInfo() const { return (LLPolySkeletalDistortionInfo*)mInfo; }
	//   This sets mInfo and calls initialization functions
	BOOL							setInfo(LLPolySkeletalDistortionInfo *info);

	// LLVisualParam Virtual functions
	///*virtual*/ BOOL				parseData(LLXmlTreeNode* node);
	/*virtual*/ void				apply( ESex sex );
	
	// LLViewerVisualParam Virtual functions
	/*virtual*/ F32					getTotalDistortion() { return 0.1f; }
	/*virtual*/ const LLVector3&	getAvgDistortion()	{ return mDefaultVec; }
	/*virtual*/ F32					getMaxDistortion() { return 0.1f; }
	/*virtual*/ LLVector3			getVertexDistortion(S32 index, LLPolyMesh *poly_mesh){return LLVector3(0.001f, 0.001f, 0.001f);}
	/*virtual*/ const LLVector3*	getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh){index = 0; poly_mesh = NULL; return &mDefaultVec;};
	/*virtual*/ const LLVector3*	getNextDistortion(U32 *index, LLPolyMesh **poly_mesh){index = 0; poly_mesh = NULL; return NULL;};

protected:
	typedef std::map<LLJoint*, LLVector3> joint_vec_map_t;
	joint_vec_map_t mJointScales;
	joint_vec_map_t mJointOffsets;
	LLVector3	mDefaultVec;
	// Backlink only; don't make this an LLPointer.
	LLVOAvatar *mAvatar;
};

#endif // LL_LLPOLYMESH_H

