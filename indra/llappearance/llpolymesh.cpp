/** 
 * @file llpolymesh.cpp
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"
#include "llpolymesh.h"
#include "llfasttimer.h"
#include "llmemory.h"

//#include "llviewercontrol.h"
#include "llxmltree.h"
#include "llavatarappearance.h"
#include "llwearable.h"
#include "lldir.h"
#include "llvolume.h"
#include "llendianswizzle.h"


#define HEADER_ASCII "Linden Mesh 1.0"
#define HEADER_BINARY "Linden Binary Mesh 1.0"

//extern LLControlGroup gSavedSettings;                           // read only

LLPolyMorphData *clone_morph_param_duplicate(const LLPolyMorphData *src_data,
					     const std::string &name);
LLPolyMorphData *clone_morph_param_direction(const LLPolyMorphData *src_data,
					     const LLVector3 &direction,
					     const std::string &name);
LLPolyMorphData *clone_morph_param_cleavage(const LLPolyMorphData *src_data,
                                            F32 scale,
                                            const std::string &name);

//-----------------------------------------------------------------------------
// Global table of loaded LLPolyMeshes
//-----------------------------------------------------------------------------
LLPolyMesh::LLPolyMeshSharedDataTable LLPolyMesh::sGlobalSharedMeshList;

//-----------------------------------------------------------------------------
// LLPolyMeshSharedData()
//-----------------------------------------------------------------------------
LLPolyMeshSharedData::LLPolyMeshSharedData()
{
        mNumVertices = 0;
        mBaseCoords = NULL;
        mBaseNormals = NULL;
        mBaseBinormals = NULL;
        mTexCoords = NULL;
        mDetailTexCoords = NULL;
        mWeights = NULL;
        mHasWeights = FALSE;
        mHasDetailTexCoords = FALSE;

        mNumFaces = 0;
        mFaces = NULL;

        mNumJointNames = 0;
        mJointNames = NULL;

        mTriangleIndices = NULL;
        mNumTriangleIndices = 0;

        mReferenceData = NULL;

        mLastIndexOffset = -1;
}

//-----------------------------------------------------------------------------
// ~LLPolyMeshSharedData()
//-----------------------------------------------------------------------------
LLPolyMeshSharedData::~LLPolyMeshSharedData()
{
        freeMeshData();
        for_each(mMorphData.begin(), mMorphData.end(), DeletePointer());
        mMorphData.clear();
}

//-----------------------------------------------------------------------------
// setupLOD()
//-----------------------------------------------------------------------------
void LLPolyMeshSharedData::setupLOD(LLPolyMeshSharedData* reference_data)
{
        mReferenceData = reference_data;

        if (reference_data)
        {
                mBaseCoords = reference_data->mBaseCoords;
                mBaseNormals = reference_data->mBaseNormals;
                mBaseBinormals = reference_data->mBaseBinormals;
                mTexCoords = reference_data->mTexCoords;
                mDetailTexCoords = reference_data->mDetailTexCoords;
                mWeights = reference_data->mWeights;
                mHasWeights = reference_data->mHasWeights;
                mHasDetailTexCoords = reference_data->mHasDetailTexCoords;
        }
}

//-----------------------------------------------------------------------------
// LLPolyMeshSharedData::freeMeshData()
//-----------------------------------------------------------------------------
void LLPolyMeshSharedData::freeMeshData()
{
        if (!mReferenceData)
        {
                mNumVertices = 0;

                ll_aligned_free_16(mBaseCoords);
                mBaseCoords = NULL;

                ll_aligned_free_16(mBaseNormals);
                mBaseNormals = NULL;

                ll_aligned_free_16(mBaseBinormals);
                mBaseBinormals = NULL;

                ll_aligned_free_16(mTexCoords);
                mTexCoords = NULL;

                ll_aligned_free_16(mDetailTexCoords);
                mDetailTexCoords = NULL;

                ll_aligned_free_16(mWeights);
                mWeights = NULL;
        }

        mNumFaces = 0;
        delete [] mFaces;
        mFaces = NULL;

        mNumJointNames = 0;
        delete [] mJointNames;
        mJointNames = NULL;

        delete [] mTriangleIndices;
        mTriangleIndices = NULL;

//      mVertFaceMap.deleteAllData();
}

// compate_int is used by the qsort function to sort the index array
int compare_int(const void *a, const void *b);

//-----------------------------------------------------------------------------
// genIndices()
//-----------------------------------------------------------------------------
void LLPolyMeshSharedData::genIndices(S32 index_offset)
{
        if (index_offset == mLastIndexOffset)
        {
                return;
        }

        delete []mTriangleIndices;
        mTriangleIndices = new U32[mNumTriangleIndices];

        S32 cur_index = 0;
        for (S32 i = 0; i < mNumFaces; i++)
        {
                mTriangleIndices[cur_index] = mFaces[i][0] + index_offset;
                cur_index++;
                mTriangleIndices[cur_index] = mFaces[i][1] + index_offset;
                cur_index++;
                mTriangleIndices[cur_index] = mFaces[i][2] + index_offset;
                cur_index++;
        }

        mLastIndexOffset = index_offset;
}

//--------------------------------------------------------------------
// LLPolyMeshSharedData::getNumKB()
//--------------------------------------------------------------------
U32 LLPolyMeshSharedData::getNumKB()
{
        U32 num_kb = sizeof(LLPolyMesh);

        if (!isLOD())
        {
                num_kb += mNumVertices *
                        ( sizeof(LLVector3) +   // coords
                          sizeof(LLVector3) +             // normals
                          sizeof(LLVector2) );    // texCoords
        }

        if (mHasDetailTexCoords && !isLOD())
        {
                num_kb += mNumVertices * sizeof(LLVector2);     // detailTexCoords
        }

        if (mHasWeights && !isLOD())
        {
                num_kb += mNumVertices * sizeof(float);         // weights
        }

        num_kb += mNumFaces * sizeof(LLPolyFace);       // faces

        num_kb /= 1024;
        return num_kb;
}

//-----------------------------------------------------------------------------
// LLPolyMeshSharedData::allocateVertexData()
//-----------------------------------------------------------------------------
BOOL LLPolyMeshSharedData::allocateVertexData( U32 numVertices )
{
        U32 i;
        mBaseCoords = (LLVector4a*) ll_aligned_malloc_16(numVertices*sizeof(LLVector4a));
        mBaseNormals = (LLVector4a*) ll_aligned_malloc_16(numVertices*sizeof(LLVector4a));
        mBaseBinormals = (LLVector4a*) ll_aligned_malloc_16(numVertices*sizeof(LLVector4a));
        mTexCoords = (LLVector2*) ll_aligned_malloc_16(numVertices*sizeof(LLVector2));
        mDetailTexCoords = (LLVector2*) ll_aligned_malloc_16(numVertices*sizeof(LLVector2));
        mWeights = (F32*) ll_aligned_malloc_16(numVertices*sizeof(F32));
        for (i = 0; i < numVertices; i++)
        {
			mBaseCoords[i].clear();
			mBaseNormals[i].clear();
			mBaseBinormals[i].clear();
			mTexCoords[i].clear();
            mWeights[i] = 0.f;
        }
        mNumVertices = numVertices;
        return TRUE;
}

//-----------------------------------------------------------------------------
// LLPolyMeshSharedData::allocateFaceData()
//-----------------------------------------------------------------------------
BOOL LLPolyMeshSharedData::allocateFaceData( U32 numFaces )
{
        mFaces = new LLPolyFace[ numFaces ];
        mNumFaces = numFaces;
        mNumTriangleIndices = mNumFaces * 3;
        return TRUE;
}

//-----------------------------------------------------------------------------
// LLPolyMeshSharedData::allocateJointNames()
//-----------------------------------------------------------------------------
BOOL LLPolyMeshSharedData::allocateJointNames( U32 numJointNames )
{
        mJointNames = new std::string[ numJointNames ];
        mNumJointNames = numJointNames;
        return TRUE;
}

//--------------------------------------------------------------------
// LLPolyMeshSharedData::loadMesh()
//--------------------------------------------------------------------
BOOL LLPolyMeshSharedData::loadMesh( const std::string& fileName )
{
        //-------------------------------------------------------------------------
        // Open the file
        //-------------------------------------------------------------------------
        if(fileName.empty())
        {
                llerrs << "Filename is Empty!" << llendl;
                return FALSE;
        }
        LLFILE* fp = LLFile::fopen(fileName, "rb");                     /*Flawfinder: ignore*/
        if (!fp)
        {
                llerrs << "can't open: " << fileName << llendl;
                return FALSE;
        }

        //-------------------------------------------------------------------------
        // Read a chunk
        //-------------------------------------------------------------------------
        char header[128];               /*Flawfinder: ignore*/
        if (fread(header, sizeof(char), 128, fp) != 128)
        {
                llwarns << "Short read" << llendl;
        }

        //-------------------------------------------------------------------------
        // Check for proper binary header
        //-------------------------------------------------------------------------
        BOOL status = FALSE;
        if ( strncmp(header, HEADER_BINARY, strlen(HEADER_BINARY)) == 0 )       /*Flawfinder: ignore*/
        {
                lldebugs << "Loading " << fileName << llendl;

                //----------------------------------------------------------------
                // File Header (seek past it)
                //----------------------------------------------------------------
                fseek(fp, 24, SEEK_SET);

                //----------------------------------------------------------------
                // HasWeights
                //----------------------------------------------------------------
                U8 hasWeights;
                size_t numRead = fread(&hasWeights, sizeof(U8), 1, fp);
                if (numRead != 1)
                {
                        llerrs << "can't read HasWeights flag from " << fileName << llendl;
                        return FALSE;
                }
                if (!isLOD())
                {
                        mHasWeights = (hasWeights==0) ? FALSE : TRUE;
                }

                //----------------------------------------------------------------
                // HasDetailTexCoords
                //----------------------------------------------------------------
                U8 hasDetailTexCoords;
                numRead = fread(&hasDetailTexCoords, sizeof(U8), 1, fp);
                if (numRead != 1)
                {
                        llerrs << "can't read HasDetailTexCoords flag from " << fileName << llendl;
                        return FALSE;
                }

                //----------------------------------------------------------------
                // Position
                //----------------------------------------------------------------
                LLVector3 position;
                numRead = fread(position.mV, sizeof(float), 3, fp);
                llendianswizzle(position.mV, sizeof(float), 3);
                if (numRead != 3)
                {
                        llerrs << "can't read Position from " << fileName << llendl;
                        return FALSE;
                }
                setPosition( position );

                //----------------------------------------------------------------
                // Rotation
                //----------------------------------------------------------------
                LLVector3 rotationAngles;
                numRead = fread(rotationAngles.mV, sizeof(float), 3, fp);
                llendianswizzle(rotationAngles.mV, sizeof(float), 3);
                if (numRead != 3)
                {
                        llerrs << "can't read RotationAngles from " << fileName << llendl;
                        return FALSE;
                }

                U8 rotationOrder;
                numRead = fread(&rotationOrder, sizeof(U8), 1, fp);

                if (numRead != 1)
                {
                        llerrs << "can't read RotationOrder from " << fileName << llendl;
                        return FALSE;
                }

                rotationOrder = 0;

                setRotation( mayaQ(     rotationAngles.mV[0],
                                        rotationAngles.mV[1],
                                        rotationAngles.mV[2],
                                        (LLQuaternion::Order)rotationOrder ) );

                //----------------------------------------------------------------
                // Scale
                //----------------------------------------------------------------
                LLVector3 scale;
                numRead = fread(scale.mV, sizeof(float), 3, fp);
                llendianswizzle(scale.mV, sizeof(float), 3);
                if (numRead != 3)
                {
                        llerrs << "can't read Scale from " << fileName << llendl;
                        return FALSE;
                }
                setScale( scale );

                //-------------------------------------------------------------------------
                // Release any existing mesh geometry
                //-------------------------------------------------------------------------
                freeMeshData();

                U16 numVertices = 0;

                //----------------------------------------------------------------
                // NumVertices
                //----------------------------------------------------------------
                if (!isLOD())
                {
                        numRead = fread(&numVertices, sizeof(U16), 1, fp);
                        llendianswizzle(&numVertices, sizeof(U16), 1);
                        if (numRead != 1)
                        {
                                llerrs << "can't read NumVertices from " << fileName << llendl;
                                return FALSE;
                        }

                        allocateVertexData( numVertices );      

						for (U16 i = 0; i < numVertices; ++i)
						{
							//----------------------------------------------------------------
							// Coords
							//----------------------------------------------------------------
							numRead = fread(&mBaseCoords[i], sizeof(float), 3, fp);
							llendianswizzle(&mBaseCoords[i], sizeof(float), 3);
							if (numRead != 3)
							{
									llerrs << "can't read Coordinates from " << fileName << llendl;
									return FALSE;
							}
						}

						for (U16 i = 0; i < numVertices; ++i)
						{
							//----------------------------------------------------------------
							// Normals
							//----------------------------------------------------------------
							numRead = fread(&mBaseNormals[i], sizeof(float), 3, fp);
							llendianswizzle(&mBaseNormals[i], sizeof(float), 3);
							if (numRead != 3)
							{
									llerrs << " can't read Normals from " << fileName << llendl;
									return FALSE;
							}
						}

						for (U16 i = 0; i < numVertices; ++i)
						{
							//----------------------------------------------------------------
							// Binormals
							//----------------------------------------------------------------
							numRead = fread(&mBaseBinormals[i], sizeof(float), 3, fp);
							llendianswizzle(&mBaseBinormals[i], sizeof(float), 3);
							if (numRead != 3)
							{
									llerrs << " can't read Binormals from " << fileName << llendl;
									return FALSE;
							}
						}

                        //----------------------------------------------------------------
                        // TexCoords
                        //----------------------------------------------------------------
                        numRead = fread(mTexCoords, 2*sizeof(float), numVertices, fp);
                        llendianswizzle(mTexCoords, sizeof(float), 2*numVertices);
                        if (numRead != numVertices)
                        {
                                llerrs << "can't read TexCoords from " << fileName << llendl;
                                return FALSE;
                        }

                        //----------------------------------------------------------------
                        // DetailTexCoords
                        //----------------------------------------------------------------
                        if (mHasDetailTexCoords)
                        {
                                numRead = fread(mDetailTexCoords, 2*sizeof(float), numVertices, fp);
                                llendianswizzle(mDetailTexCoords, sizeof(float), 2*numVertices);
                                if (numRead != numVertices)
                                {
                                        llerrs << "can't read DetailTexCoords from " << fileName << llendl;
                                        return FALSE;
                                }
                        }

                        //----------------------------------------------------------------
                        // Weights
                        //----------------------------------------------------------------
                        if (mHasWeights)
                        {
                                numRead = fread(mWeights, sizeof(float), numVertices, fp);
                                llendianswizzle(mWeights, sizeof(float), numVertices);
                                if (numRead != numVertices)
                                {
                                        llerrs << "can't read Weights from " << fileName << llendl;
                                        return FALSE;
                                }
                        }
                }

                //----------------------------------------------------------------
                // NumFaces
                //----------------------------------------------------------------
                U16 numFaces;
                numRead = fread(&numFaces, sizeof(U16), 1, fp);
                llendianswizzle(&numFaces, sizeof(U16), 1);
                if (numRead != 1)
                {
                        llerrs << "can't read NumFaces from " << fileName << llendl;
                        return FALSE;
                }
                allocateFaceData( numFaces );


                //----------------------------------------------------------------
                // Faces
                //----------------------------------------------------------------
                U32 i;
                U32 numTris = 0;
                for (i = 0; i < numFaces; i++)
                {
                        S16 face[3];
                        numRead = fread(face, sizeof(U16), 3, fp);
                        llendianswizzle(face, sizeof(U16), 3);
                        if (numRead != 3)
                        {
                                llerrs << "can't read Face[" << i << "] from " << fileName << llendl;
                                return FALSE;
                        }
                        if (mReferenceData)
                        {
                                llassert(face[0] < mReferenceData->mNumVertices);
                                llassert(face[1] < mReferenceData->mNumVertices);
                                llassert(face[2] < mReferenceData->mNumVertices);
                        }
                        
                        if (isLOD())
                        {
                                // store largest index in case of LODs
                                for (S32 j = 0; j < 3; j++)
                                {
                                        if (face[j] > mNumVertices - 1)
                                        {
                                                mNumVertices = face[j] + 1;
                                        }
                                }
                        }
                        mFaces[i][0] = face[0];
                        mFaces[i][1] = face[1];
                        mFaces[i][2] = face[2];

//                      S32 j;
//                      for(j = 0; j < 3; j++)
//                      {
//                              LLDynamicArray<S32> *face_list = mVertFaceMap.getIfThere(face[j]);
//                              if (!face_list)
//                              {
//                                      face_list = new LLDynamicArray<S32>;
//                                      mVertFaceMap.addData(face[j], face_list);
//                              }
//                              face_list->put(i);
//                      }

                        numTris++;
                }

                lldebugs << "verts: " << numVertices 
                         << ", faces: "   << numFaces
                         << ", tris: "    << numTris
                         << llendl;

                //----------------------------------------------------------------
                // NumSkinJoints
                //----------------------------------------------------------------
                if (!isLOD())
                {
                        U16 numSkinJoints = 0;
                        if ( mHasWeights )
                        {
                                numRead = fread(&numSkinJoints, sizeof(U16), 1, fp);
                                llendianswizzle(&numSkinJoints, sizeof(U16), 1);
                                if (numRead != 1)
                                {
                                        llerrs << "can't read NumSkinJoints from " << fileName << llendl;
                                        return FALSE;
                                }
                                allocateJointNames( numSkinJoints );
                        }

                        //----------------------------------------------------------------
                        // SkinJoints
                        //----------------------------------------------------------------
                        for (i=0; i < numSkinJoints; i++)
                        {
                                char jointName[64+1];
                                numRead = fread(jointName, sizeof(jointName)-1, 1, fp);
                                jointName[sizeof(jointName)-1] = '\0'; // ensure nul-termination
                                if (numRead != 1)
                                {
                                        llerrs << "can't read Skin[" << i << "].Name from " << fileName << llendl;
                                        return FALSE;
                                }

                                std::string *jn = &mJointNames[i];
                                *jn = jointName;
                        }

                        //-------------------------------------------------------------------------
                        // look for morph section
                        //-------------------------------------------------------------------------
                        char morphName[64+1];
                        morphName[sizeof(morphName)-1] = '\0'; // ensure nul-termination
                        while(fread(&morphName, sizeof(char), 64, fp) == 64)
                        {
                                if (!strcmp(morphName, "End Morphs"))
                                {
                                        // we reached the end of the morphs
                                        break;
                                }
                                LLPolyMorphData* morph_data = new LLPolyMorphData(std::string(morphName));

                                BOOL result = morph_data->loadBinary(fp, this);

                                if (!result)
                                {
                                        delete morph_data;
                                        continue;
                                }

                                mMorphData.insert(morph_data);

                                if (!strcmp(morphName, "Breast_Female_Cleavage"))
                                {
                                        mMorphData.insert(clone_morph_param_cleavage(morph_data,
                                                                                     .75f,
                                                                                     "Breast_Physics_LeftRight_Driven"));
                                }

                                if (!strcmp(morphName, "Breast_Female_Cleavage"))
                                {
                                        mMorphData.insert(clone_morph_param_duplicate(morph_data,
										      "Breast_Physics_InOut_Driven"));
                                }
                                if (!strcmp(morphName, "Breast_Gravity"))
                                {
                                        mMorphData.insert(clone_morph_param_duplicate(morph_data,
										      "Breast_Physics_UpDown_Driven"));
                                }

                                if (!strcmp(morphName, "Big_Belly_Torso"))
                                {
                                        mMorphData.insert(clone_morph_param_direction(morph_data,
										      LLVector3(0,0,0.05f),
										      "Belly_Physics_Torso_UpDown_Driven"));
                                }

                                if (!strcmp(morphName, "Big_Belly_Legs"))
                                {
                                        mMorphData.insert(clone_morph_param_direction(morph_data,
										      LLVector3(0,0,0.05f),
										      "Belly_Physics_Legs_UpDown_Driven"));
                                }

                                if (!strcmp(morphName, "skirt_belly"))
                                {
                                        mMorphData.insert(clone_morph_param_direction(morph_data,
										      LLVector3(0,0,0.05f),
										      "Belly_Physics_Skirt_UpDown_Driven"));
                                }

                                if (!strcmp(morphName, "Small_Butt"))
                                {
                                        mMorphData.insert(clone_morph_param_direction(morph_data,
										      LLVector3(0,0,0.05f),
										      "Butt_Physics_UpDown_Driven"));
                                }
                                if (!strcmp(morphName, "Small_Butt"))
                                {
                                        mMorphData.insert(clone_morph_param_direction(morph_data,
										      LLVector3(0,0.03f,0),
										      "Butt_Physics_LeftRight_Driven"));
                                }
                        }

                        S32 numRemaps;
                        if (fread(&numRemaps, sizeof(S32), 1, fp) == 1)
                        {
                                llendianswizzle(&numRemaps, sizeof(S32), 1);
                                for (S32 i = 0; i < numRemaps; i++)
                                {
                                        S32 remapSrc;
                                        S32 remapDst;
                                        if (fread(&remapSrc, sizeof(S32), 1, fp) != 1)
                                        {
                                                llerrs << "can't read source vertex in vertex remap data" << llendl;
                                                break;
                                        }
                                        if (fread(&remapDst, sizeof(S32), 1, fp) != 1)
                                        {
                                                llerrs << "can't read destination vertex in vertex remap data" << llendl;
                                                break;
                                        }
                                        llendianswizzle(&remapSrc, sizeof(S32), 1);
                                        llendianswizzle(&remapDst, sizeof(S32), 1);

                                        mSharedVerts[remapSrc] = remapDst;
                                }
                        }
                }

                status = TRUE;
        }
        else
        {
                llerrs << "invalid mesh file header: " << fileName << llendl;
                status = FALSE;
        }

        if (0 == mNumJointNames)
        {
                allocateJointNames(1);
        }

        fclose( fp );

        return status;
}

//-----------------------------------------------------------------------------
// getSharedVert()
//-----------------------------------------------------------------------------
const S32 *LLPolyMeshSharedData::getSharedVert(S32 vert)
{
        if (mSharedVerts.count(vert) > 0)
        {
                return &mSharedVerts[vert];
        }
        return NULL;
}

//-----------------------------------------------------------------------------
// getUV()
//-----------------------------------------------------------------------------
const LLVector2 &LLPolyMeshSharedData::getUVs(U32 index)
{
        // TODO: convert all index variables to S32
        llassert((S32)index < mNumVertices);

        return mTexCoords[index];
}

//-----------------------------------------------------------------------------
// LLPolyMesh()
//-----------------------------------------------------------------------------
LLPolyMesh::LLPolyMesh(LLPolyMeshSharedData *shared_data, LLPolyMesh *reference_mesh)
{       
	LLMemType mt(LLMemType::MTYPE_AVATAR_MESH);

	llassert(shared_data);

	mSharedData = shared_data;
	mReferenceMesh = reference_mesh;
	mAvatarp = NULL;
	mVertexData = NULL;

	mCurVertexCount = 0;
	mFaceIndexCount = 0;
	mFaceIndexOffset = 0;
	mFaceVertexCount = 0;
	mFaceVertexOffset = 0;

	if (shared_data->isLOD() && reference_mesh)
	{
		mCoords = reference_mesh->mCoords;
		mNormals = reference_mesh->mNormals;
		mScaledNormals = reference_mesh->mScaledNormals;
		mBinormals = reference_mesh->mBinormals;
		mScaledBinormals = reference_mesh->mScaledBinormals;
		mTexCoords = reference_mesh->mTexCoords;
		mClothingWeights = reference_mesh->mClothingWeights;
	}
	else
	{
		// Allocate memory without initializing every vector
		// NOTE: This makes asusmptions about the size of LLVector[234]
		S32 nverts = mSharedData->mNumVertices;
		//make sure it's an even number of verts for alignment
		nverts += nverts%2;
		S32 nfloats = nverts * (
					4 + //coords
					4 + //normals
					4 + //weights
					2 + //coords
					4 + //scaled normals
					4 + //binormals
					4); //scaled binormals

		//use 16 byte aligned vertex data to make LLPolyMesh SSE friendly
		mVertexData = (F32*) ll_aligned_malloc_16(nfloats*4);
		S32 offset = 0;
		mCoords				= 	(LLVector4a*)(mVertexData + offset); offset += 4*nverts;
		mNormals			=	(LLVector4a*)(mVertexData + offset); offset += 4*nverts;
		mClothingWeights	= 	(LLVector4a*)(mVertexData + offset); offset += 4*nverts;
		mTexCoords			= 	(LLVector2*)(mVertexData + offset);  offset += 2*nverts;
		mScaledNormals		=   (LLVector4a*)(mVertexData + offset); offset += 4*nverts;
		mBinormals			=   (LLVector4a*)(mVertexData + offset); offset += 4*nverts;
		mScaledBinormals	=   (LLVector4a*)(mVertexData + offset); offset += 4*nverts; 
		initializeForMorph();
	}
}


//-----------------------------------------------------------------------------
// ~LLPolyMesh()
//-----------------------------------------------------------------------------
LLPolyMesh::~LLPolyMesh()
{
        S32 i;
        for (i = 0; i < mJointRenderData.count(); i++)
        {
                delete mJointRenderData[i];
                mJointRenderData[i] = NULL;
        }

		ll_aligned_free_16(mVertexData);

}


//-----------------------------------------------------------------------------
// LLPolyMesh::getMesh()
//-----------------------------------------------------------------------------
LLPolyMesh *LLPolyMesh::getMesh(const std::string &name, LLPolyMesh* reference_mesh)
{
        //-------------------------------------------------------------------------
        // search for an existing mesh by this name
        //-------------------------------------------------------------------------
        LLPolyMeshSharedData* meshSharedData = get_if_there(sGlobalSharedMeshList, name, (LLPolyMeshSharedData*)NULL);
        if (meshSharedData)
        {
//              llinfos << "Polymesh " << name << " found in global mesh table." << llendl;
                LLPolyMesh *poly_mesh = new LLPolyMesh(meshSharedData, reference_mesh);
                return poly_mesh;
        }

        //-------------------------------------------------------------------------
        // if not found, create a new one, add it to the list
        //-------------------------------------------------------------------------
        std::string full_path;
        full_path = gDirUtilp->getExpandedFilename(LL_PATH_CHARACTER,name);

        LLPolyMeshSharedData *mesh_data = new LLPolyMeshSharedData();
        if (reference_mesh)
        {
                mesh_data->setupLOD(reference_mesh->getSharedData());
        }
        if ( ! mesh_data->loadMesh( full_path ) )
        {
                delete mesh_data;
                return NULL;
        }

        LLPolyMesh *poly_mesh = new LLPolyMesh(mesh_data, reference_mesh);

//      llinfos << "Polymesh " << name << " added to global mesh table." << llendl;
        sGlobalSharedMeshList[name] = poly_mesh->mSharedData;

        return poly_mesh;
}

//-----------------------------------------------------------------------------
// LLPolyMesh::freeAllMeshes()
//-----------------------------------------------------------------------------
void LLPolyMesh::freeAllMeshes()
{
        // delete each item in the global lists
        for_each(sGlobalSharedMeshList.begin(), sGlobalSharedMeshList.end(), DeletePairedPointer());
        sGlobalSharedMeshList.clear();
}

LLPolyMeshSharedData *LLPolyMesh::getSharedData() const
{
        return mSharedData;
}


//--------------------------------------------------------------------
// LLPolyMesh::dumpDiagInfo()
//--------------------------------------------------------------------
void LLPolyMesh::dumpDiagInfo()
{
        // keep track of totals
        U32 total_verts = 0;
        U32 total_faces = 0;
        U32 total_kb = 0;

        std::string buf;

        llinfos << "-----------------------------------------------------" << llendl;
        llinfos << "       Global PolyMesh Table (DEBUG only)" << llendl;
        llinfos << "   Verts    Faces  Mem(KB) Name" << llendl;
        llinfos << "-----------------------------------------------------" << llendl;

        // print each loaded mesh, and it's memory usage
        for(LLPolyMeshSharedDataTable::iterator iter = sGlobalSharedMeshList.begin();
            iter != sGlobalSharedMeshList.end(); ++iter)
        {
                const std::string& mesh_name = iter->first;
                LLPolyMeshSharedData* mesh = iter->second;

                S32 num_verts = mesh->mNumVertices;
                S32 num_faces = mesh->mNumFaces;
                U32 num_kb = mesh->getNumKB();

                buf = llformat("%8d %8d %8d %s", num_verts, num_faces, num_kb, mesh_name.c_str());
                llinfos << buf << llendl;

                total_verts += num_verts;
                total_faces += num_faces;
                total_kb += num_kb;
        }

        llinfos << "-----------------------------------------------------" << llendl;
        buf = llformat("%8d %8d %8d TOTAL", total_verts, total_faces, total_kb );
        llinfos << buf << llendl;
        llinfos << "-----------------------------------------------------" << llendl;
}

//-----------------------------------------------------------------------------
// getWritableCoords()
//-----------------------------------------------------------------------------
LLVector4a *LLPolyMesh::getWritableCoords()
{
        return mCoords;
}

//-----------------------------------------------------------------------------
// getWritableNormals()
//-----------------------------------------------------------------------------
LLVector4a *LLPolyMesh::getWritableNormals()
{
        return mNormals;
}

//-----------------------------------------------------------------------------
// getWritableBinormals()
//-----------------------------------------------------------------------------
LLVector4a *LLPolyMesh::getWritableBinormals()
{
        return mBinormals;
}


//-----------------------------------------------------------------------------
// getWritableClothingWeights()
//-----------------------------------------------------------------------------
LLVector4a       *LLPolyMesh::getWritableClothingWeights()
{
        return mClothingWeights;
}

//-----------------------------------------------------------------------------
// getWritableTexCoords()
//-----------------------------------------------------------------------------
LLVector2       *LLPolyMesh::getWritableTexCoords()
{
        return mTexCoords;
}

//-----------------------------------------------------------------------------
// getScaledNormals()
//-----------------------------------------------------------------------------
LLVector4a *LLPolyMesh::getScaledNormals()
{
        return mScaledNormals;
}

//-----------------------------------------------------------------------------
// getScaledBinormals()
//-----------------------------------------------------------------------------
LLVector4a *LLPolyMesh::getScaledBinormals()
{
        return mScaledBinormals;
}


//-----------------------------------------------------------------------------
// initializeForMorph()
//-----------------------------------------------------------------------------
void LLPolyMesh::initializeForMorph()
{
    LLVector4a::memcpyNonAliased16((F32*) mCoords, (F32*) mSharedData->mBaseCoords, sizeof(LLVector4a) * mSharedData->mNumVertices);
	LLVector4a::memcpyNonAliased16((F32*) mNormals, (F32*) mSharedData->mBaseNormals, sizeof(LLVector4a) * mSharedData->mNumVertices);
	LLVector4a::memcpyNonAliased16((F32*) mScaledNormals, (F32*) mSharedData->mBaseNormals, sizeof(LLVector4a) * mSharedData->mNumVertices);
	LLVector4a::memcpyNonAliased16((F32*) mBinormals, (F32*) mSharedData->mBaseNormals, sizeof(LLVector4a) * mSharedData->mNumVertices);
	LLVector4a::memcpyNonAliased16((F32*) mScaledBinormals, (F32*) mSharedData->mBaseNormals, sizeof(LLVector4a) * mSharedData->mNumVertices);
	LLVector4a::memcpyNonAliased16((F32*) mTexCoords, (F32*) mSharedData->mTexCoords, sizeof(LLVector2) * (mSharedData->mNumVertices + mSharedData->mNumVertices%2));

	for (U32 i = 0; i < mSharedData->mNumVertices; ++i)
	{
		mClothingWeights[i].clear();
	}
}

//-----------------------------------------------------------------------------
// getMorphData()
//-----------------------------------------------------------------------------
LLPolyMorphData*        LLPolyMesh::getMorphData(const std::string& morph_name)
{
        if (!mSharedData)
                return NULL;
        for (LLPolyMeshSharedData::morphdata_list_t::iterator iter = mSharedData->mMorphData.begin();
             iter != mSharedData->mMorphData.end(); ++iter)
        {
                LLPolyMorphData *morph_data = *iter;
                if (morph_data->getName() == morph_name)
                {
                        return morph_data;
                }
        }
        return NULL;
}

//-----------------------------------------------------------------------------
// removeMorphData()
//-----------------------------------------------------------------------------
// // erasing but not deleting seems bad, but fortunately we don't actually use this...
// void LLPolyMesh::removeMorphData(LLPolyMorphData *morph_target)
// {
//      if (!mSharedData)
//              return;
//      mSharedData->mMorphData.erase(morph_target);
// }

//-----------------------------------------------------------------------------
// deleteAllMorphData()
//-----------------------------------------------------------------------------
// void LLPolyMesh::deleteAllMorphData()
// {
//      if (!mSharedData)
//              return;

//      for_each(mSharedData->mMorphData.begin(), mSharedData->mMorphData.end(), DeletePointer());
//      mSharedData->mMorphData.clear();
// }

//-----------------------------------------------------------------------------
// getWritableWeights()
//-----------------------------------------------------------------------------
F32*    LLPolyMesh::getWritableWeights() const
{
        return mSharedData->mWeights;
}

// End
