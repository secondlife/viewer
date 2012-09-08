/** 
 * @file llpolymesh.h
 * @brief Implementation of LLPolyMesh class
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

#ifndef LL_LLPOLYMESHINTERFACE_H
#define LL_LLPOLYMESHINTERFACE_H

#include <string>
#include <map>
#include "llstl.h"

#include "v3math.h"
#include "v2math.h"
#include "llquaternion.h"
#include "llpolymorph.h"
#include "lljoint.h"
//#include "lldarray.h"

class LLSkinJoint;
class LLAvatarAppearance;
class LLWearable;

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
	LLVector4a				*mBaseCoords;
	LLVector4a				*mBaseNormals;
	LLVector4a				*mBaseBinormals;
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
	typedef std::set<LLPolyMorphData*> morphdata_list_t;
	morphdata_list_t			mMorphData;

	std::map<S32, S32> 			mSharedVerts;
	
	LLPolyMeshSharedData*		mReferenceData;
	S32							mLastIndexOffset;

public:
	// Temporarily...
	// Triangle indices
	U32				mNumTriangleIndices;
	U32				*mTriangleIndices;

public:
	LLPolyMeshSharedData();
	~LLPolyMeshSharedData();

private:
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
	BOOL loadMesh( const std::string& fileName );

public:
	void genIndices(S32 offset);

	const LLVector2 &getUVs(U32 index);

	const S32	*getSharedVert(S32 vert);

	BOOL isLOD() { return (mReferenceData != NULL); }
};


class LLJointRenderData
{
public:
	LLJointRenderData(const LLMatrix4* world_matrix, LLSkinJoint* skin_joint) : mWorldMatrix(world_matrix), mSkinJoint(skin_joint) {}
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
	static LLPolyMesh *getMesh( const std::string &name, LLPolyMesh* reference_mesh = NULL);

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
	const LLVector4a	*getCoords() const{
		return mCoords;
	}

	// non const version
	LLVector4a *getWritableCoords();

	// Get normals
	const LLVector4a	*getNormals() const{ 
		return mNormals; 
	}

	// Get normals
	const LLVector4a	*getBinormals() const{ 
		return mBinormals; 
	}

	// Get base mesh normals
	const LLVector4a *getBaseNormals() const{
		llassert(mSharedData);
		return mSharedData->mBaseNormals;
	}

	// Get base mesh normals
	const LLVector4a *getBaseBinormals() const{
		llassert(mSharedData);
		return mSharedData->mBaseBinormals;
	}

	// intermediate morphed normals and output normals
	LLVector4a *getWritableNormals();
	LLVector4a *getScaledNormals();

	LLVector4a *getWritableBinormals();
	LLVector4a *getScaledBinormals();

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

	LLVector4a	*getWritableClothingWeights();

	const LLVector4a		*getClothingWeights()
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

	LLPolyMorphData*	getMorphData(const std::string& morph_name);
// 	void	removeMorphData(LLPolyMorphData *morph_target);
// 	void	deleteAllMorphData();

	LLPolyMeshSharedData *getSharedData() const;
	LLPolyMesh *getReferenceMesh() { return mReferenceMesh ? mReferenceMesh : this; }

	// Get indices
	U32*	getIndices() { return mSharedData ? mSharedData->mTriangleIndices : NULL; }

	BOOL	isLOD() { return mSharedData && mSharedData->isLOD(); }

	void setAvatar(LLAvatarAppearance* avatarp) { mAvatarp = avatarp; }
	LLAvatarAppearance* getAvatar() { return mAvatarp; }

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
	LLVector4a				*mCoords;
	// deformed normals (resulting from application of morph targets)
	LLVector4a				*mScaledNormals;
	// output normals (after normalization)
	LLVector4a				*mNormals;
	// deformed binormals (resulting from application of morph targets)
	LLVector4a				*mScaledBinormals;
	// output binormals (after normalization)
	LLVector4a				*mBinormals;
	// weight values that mark verts as clothing/skin
	LLVector4a				*mClothingWeights;
	// output texture coordinates
	LLVector2				*mTexCoords;
	
	LLPolyMesh				*mReferenceMesh;

	// global mesh list
	typedef std::map<std::string, LLPolyMeshSharedData*> LLPolyMeshSharedDataTable; 
	static LLPolyMeshSharedDataTable sGlobalSharedMeshList;

	// Backlink only; don't make this an LLPointer.
	LLAvatarAppearance* mAvatarp;
};

#endif // LL_LLPOLYMESHINTERFACE_H

