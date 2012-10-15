/** 
 * @file llpolymorph.cpp
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
#include "llviewerprecompiledheaders.h"

#include "llpolymorph.h"
#include "llvoavatar.h"
#include "llwearable.h"
#include "llxmltree.h"
#include "llendianswizzle.h"

//#include "../tools/imdebug/imdebug.h"

const F32 NORMAL_SOFTEN_FACTOR = 0.65f;

//-----------------------------------------------------------------------------
// LLPolyMorphData()
//-----------------------------------------------------------------------------
LLPolyMorphData::LLPolyMorphData(const std::string& morph_name)
	: mName(morph_name)
{
	mNumIndices = 0;
	mCurrentIndex = 0;
	mTotalDistortion = 0.f;
	mAvgDistortion.clear();
	mMaxDistortion = 0.f;
	mVertexIndices = NULL;
	mCoords = NULL;
	mNormals = NULL;
	mBinormals = NULL;
	mTexCoords = NULL;

	mMesh = NULL;
}

LLPolyMorphData::LLPolyMorphData(const LLPolyMorphData &rhs) :
	mName(rhs.mName),
	mNumIndices(rhs.mNumIndices),
	mTotalDistortion(rhs.mTotalDistortion),
	mAvgDistortion(rhs.mAvgDistortion),
	mMaxDistortion(rhs.mMaxDistortion),
	mVertexIndices(NULL),
	mCoords(NULL),
	mNormals(NULL),
	mBinormals(NULL),
	mTexCoords(NULL)
{
	const S32 numVertices = mNumIndices;

	U32 size = sizeof(LLVector4a)*numVertices;

	mCoords = static_cast<LLVector4a*>( ll_aligned_malloc_16(size) );
	mNormals = static_cast<LLVector4a*>( ll_aligned_malloc_16(size) );
	mBinormals = static_cast<LLVector4a*>( ll_aligned_malloc_16(size) );
	mTexCoords = new LLVector2[numVertices];
	mVertexIndices = new U32[numVertices];
	
	for (S32 v=0; v < numVertices; v++)
	{
		mCoords[v] = rhs.mCoords[v];
		mNormals[v] = rhs.mNormals[v];
		mBinormals[v] = rhs.mBinormals[v];
		mTexCoords[v] = rhs.mTexCoords[v];
		mVertexIndices[v] = rhs.mVertexIndices[v];
	}
}

//-----------------------------------------------------------------------------
// ~LLPolyMorphData()
//-----------------------------------------------------------------------------
LLPolyMorphData::~LLPolyMorphData()
{
	freeData();
}

//-----------------------------------------------------------------------------
// loadBinary()
//-----------------------------------------------------------------------------
BOOL LLPolyMorphData::loadBinary(LLFILE *fp, LLPolyMeshSharedData *mesh)
{
	S32 numVertices;
	S32 numRead;

	numRead = fread(&numVertices, sizeof(S32), 1, fp);
	llendianswizzle(&numVertices, sizeof(S32), 1);
	if (numRead != 1)
	{
		llwarns << "Can't read number of morph target vertices" << llendl;
		return FALSE;
	}

	//-------------------------------------------------------------------------
	// free any existing data
	//-------------------------------------------------------------------------
	freeData();

	//-------------------------------------------------------------------------
	// allocate vertices
	//-------------------------------------------------------------------------
	
	U32 size = sizeof(LLVector4a)*numVertices;
	
	mCoords = static_cast<LLVector4a*>(ll_aligned_malloc_16(size));
	mNormals = static_cast<LLVector4a*>(ll_aligned_malloc_16(size));
	mBinormals = static_cast<LLVector4a*>(ll_aligned_malloc_16(size));
	
	mTexCoords = new LLVector2[numVertices];
	// Actually, we are allocating more space than we need for the skiplist
	mVertexIndices = new U32[numVertices];
	mNumIndices = 0;
	mTotalDistortion = 0.f;
	mMaxDistortion = 0.f;
	mAvgDistortion.clear();
	mMesh = mesh;

	//-------------------------------------------------------------------------
	// read vertices
	//-------------------------------------------------------------------------
	for(S32 v = 0; v < numVertices; v++)
	{
		numRead = fread(&mVertexIndices[v], sizeof(U32), 1, fp);
		llendianswizzle(&mVertexIndices[v], sizeof(U32), 1);
		if (numRead != 1)
		{
			llwarns << "Can't read morph target vertex number" << llendl;
			return FALSE;
		}

		if (mVertexIndices[v] > 10000)
		{
			llerrs << "Bad morph index: " << mVertexIndices[v] << llendl;
		}


		numRead = fread(&mCoords[v], sizeof(F32), 3, fp);
		llendianswizzle(&mCoords[v], sizeof(F32), 3);
		if (numRead != 3)
		{
			llwarns << "Can't read morph target vertex coordinates" << llendl;
			return FALSE;
		}

		F32 magnitude = mCoords[v].getLength3().getF32();
		
		mTotalDistortion += magnitude;
		LLVector4a t;
		t.setAbs(mCoords[v]);
		mAvgDistortion.add(t);
		
		if (magnitude > mMaxDistortion)
		{
			mMaxDistortion = magnitude;
		}

		numRead = fread(&mNormals[v], sizeof(F32), 3, fp);
		llendianswizzle(&mNormals[v], sizeof(F32), 3);
		if (numRead != 3)
		{
			llwarns << "Can't read morph target normal" << llendl;
			return FALSE;
		}

		numRead = fread(&mBinormals[v], sizeof(F32), 3, fp);
		llendianswizzle(&mBinormals[v], sizeof(F32), 3);
		if (numRead != 3)
		{
			llwarns << "Can't read morph target binormal" << llendl;
			return FALSE;
		}


		numRead = fread(&mTexCoords[v].mV, sizeof(F32), 2, fp);
		llendianswizzle(&mTexCoords[v].mV, sizeof(F32), 2);
		if (numRead != 2)
		{
			llwarns << "Can't read morph target uv" << llendl;
			return FALSE;
		}

		mNumIndices++;
	}

	mAvgDistortion.mul(1.f/(F32)mNumIndices);
	mAvgDistortion.normalize3fast();

	return TRUE;
}

//-----------------------------------------------------------------------------
// freeData()
//-----------------------------------------------------------------------------
void LLPolyMorphData::freeData()
{
	if (mCoords != NULL)
	{
		ll_aligned_free_16(mCoords);
		mCoords = NULL;
	}

	if (mNormals != NULL)
	{
		ll_aligned_free_16(mNormals);
		mNormals = NULL;
	}

	if (mBinormals != NULL)
	{
		ll_aligned_free_16(mBinormals);
		mBinormals = NULL;
	}

	if (mTexCoords != NULL)
	{
		delete [] mTexCoords;
		mTexCoords = NULL;
	}

	if (mVertexIndices != NULL)
	{
		delete [] mVertexIndices;
		mVertexIndices = NULL;
	}
}

//-----------------------------------------------------------------------------
// LLPolyMorphTargetInfo()
//-----------------------------------------------------------------------------
LLPolyMorphTargetInfo::LLPolyMorphTargetInfo()
	: mIsClothingMorph(FALSE)
{
}

BOOL LLPolyMorphTargetInfo::parseXml(LLXmlTreeNode* node)
{
	llassert( node->hasName( "param" ) && node->getChildByName( "param_morph" ) );

	if (!LLViewerVisualParamInfo::parseXml(node))
		return FALSE;

	// Get mixed-case name
	static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
	if( !node->getFastAttributeString( name_string, mMorphName ) )
	{
		llwarns << "Avatar file: <param> is missing name attribute" << llendl;
		return FALSE;  // Continue, ignoring this tag
	}

	static LLStdStringHandle clothing_morph_string = LLXmlTree::addAttributeString("clothing_morph");
	node->getFastAttributeBOOL(clothing_morph_string, mIsClothingMorph);

	LLXmlTreeNode *paramNode = node->getChildByName("param_morph");

        if (NULL == paramNode)
        {
                llwarns << "Failed to getChildByName(\"param_morph\")"
                        << llendl;
                return FALSE;
        }

	for (LLXmlTreeNode* child_node = paramNode->getFirstChild();
		 child_node;
		 child_node = paramNode->getNextChild())
	{
		static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
		if (child_node->hasName("volume_morph"))
		{
			std::string volume_name;
			if (child_node->getFastAttributeString(name_string, volume_name))
			{
				LLVector3 scale;
				static LLStdStringHandle scale_string = LLXmlTree::addAttributeString("scale");
				child_node->getFastAttributeVector3(scale_string, scale);
				
				LLVector3 pos;
				static LLStdStringHandle pos_string = LLXmlTree::addAttributeString("pos");
				child_node->getFastAttributeVector3(pos_string, pos);

				mVolumeInfoList.push_back(LLPolyVolumeMorphInfo(volume_name,scale,pos));
			}
		}
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
// LLPolyMorphTarget()
//-----------------------------------------------------------------------------
LLPolyMorphTarget::LLPolyMorphTarget(LLPolyMesh *poly_mesh)
	: mMorphData(NULL), mMesh(poly_mesh),
	  mVertMask(NULL),
	  mLastSex(SEX_FEMALE),
	  mNumMorphMasksPending(0)
{
}

//-----------------------------------------------------------------------------
// ~LLPolyMorphTarget()
//-----------------------------------------------------------------------------
LLPolyMorphTarget::~LLPolyMorphTarget()
{
	if (mVertMask)
	{
		delete mVertMask;
	}
}

//-----------------------------------------------------------------------------
// setInfo()
//-----------------------------------------------------------------------------
BOOL LLPolyMorphTarget::setInfo(LLPolyMorphTargetInfo* info)
{
	llassert(mInfo == NULL);
	if (info->mID < 0)
		return FALSE;
	mInfo = info;
	mID = info->mID;
	setWeight(getDefaultWeight(), FALSE );

	LLVOAvatar* avatarp = mMesh->getAvatar();
	LLPolyMorphTargetInfo::volume_info_list_t::iterator iter;
	for (iter = getInfo()->mVolumeInfoList.begin(); iter != getInfo()->mVolumeInfoList.end(); iter++)
	{
		LLPolyVolumeMorphInfo *volume_info = &(*iter);
		for (S32 i = 0; i < avatarp->mNumCollisionVolumes; i++)
		{
			if (avatarp->mCollisionVolumes[i].getName() == volume_info->mName)
			{
				mVolumeMorphs.push_back(LLPolyVolumeMorph(&avatarp->mCollisionVolumes[i],
														  volume_info->mScale,
														  volume_info->mPos));
				break;
			}
		}
	}

	std::string morph_param_name = getInfo()->mMorphName;
	
	mMorphData = mMesh->getMorphData(morph_param_name);
	if (!mMorphData)
	{
		const std::string driven_tag = "_Driven";
		U32 pos = morph_param_name.find(driven_tag);
		if (pos > 0)
		{
			morph_param_name = morph_param_name.substr(0,pos);
			mMorphData = mMesh->getMorphData(morph_param_name);
		}
	}
	if (!mMorphData)
	{
		llwarns << "No morph target named " << morph_param_name << " found in mesh." << llendl;
		return FALSE;  // Continue, ignoring this tag
	}
	return TRUE;
}

/*virtual*/ LLViewerVisualParam* LLPolyMorphTarget::cloneParam(LLWearable* wearable) const
{
	LLPolyMorphTarget *new_param = new LLPolyMorphTarget(mMesh);
	*new_param = *this;
	return new_param;
}

#if 0 // obsolete
//-----------------------------------------------------------------------------
// parseData()
//-----------------------------------------------------------------------------
BOOL LLPolyMorphTarget::parseData(LLXmlTreeNode* node)
{
	LLPolyMorphTargetInfo* info = new LLPolyMorphTargetInfo;

	info->parseXml(node);
	if (!setInfo(info))
	{
		delete info;
		return FALSE;
	}
	return TRUE;
}
#endif

//-----------------------------------------------------------------------------
// getVertexDistortion()
//-----------------------------------------------------------------------------
LLVector4a LLPolyMorphTarget::getVertexDistortion(S32 requested_index, LLPolyMesh *mesh)
{
	if (!mMorphData || mMesh != mesh) return LLVector4a::getZero();

	for(U32 index = 0; index < mMorphData->mNumIndices; index++)
	{
		if (mMorphData->mVertexIndices[index] == (U32)requested_index)
		{
			return mMorphData->mCoords[index];
		}
	}

	return LLVector4a::getZero();
}

//-----------------------------------------------------------------------------
// getFirstDistortion()
//-----------------------------------------------------------------------------
const LLVector4a *LLPolyMorphTarget::getFirstDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	if (!mMorphData) return &LLVector4a::getZero();

	LLVector4a* resultVec;
	mMorphData->mCurrentIndex = 0;
	if (mMorphData->mNumIndices)
	{
		resultVec = &mMorphData->mCoords[mMorphData->mCurrentIndex];
		if (index != NULL)
		{
			*index = mMorphData->mVertexIndices[mMorphData->mCurrentIndex];
		}
		if (poly_mesh != NULL)
		{
			*poly_mesh = mMesh;
		}

		return resultVec;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// getNextDistortion()
//-----------------------------------------------------------------------------
const LLVector4a *LLPolyMorphTarget::getNextDistortion(U32 *index, LLPolyMesh **poly_mesh)
{
	if (!mMorphData) return &LLVector4a::getZero();

	LLVector4a* resultVec;
	mMorphData->mCurrentIndex++;
	if (mMorphData->mCurrentIndex < mMorphData->mNumIndices)
	{
		resultVec = &mMorphData->mCoords[mMorphData->mCurrentIndex];
		if (index != NULL)
		{
			*index = mMorphData->mVertexIndices[mMorphData->mCurrentIndex];
		}
		if (poly_mesh != NULL)
		{
			*poly_mesh = mMesh;
		}
		return resultVec;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// getTotalDistortion()
//-----------------------------------------------------------------------------
F32	LLPolyMorphTarget::getTotalDistortion() 
{ 
	if (mMorphData) 
	{
		return mMorphData->mTotalDistortion; 
	}
	else 
	{
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// getAvgDistortion()
//-----------------------------------------------------------------------------
const LLVector4a& LLPolyMorphTarget::getAvgDistortion()	
{
	if (mMorphData) 
	{
		return mMorphData->mAvgDistortion; 
	}
	else 
	{
		return LLVector4a::getZero();
	}
}

//-----------------------------------------------------------------------------
// getMaxDistortion()
//-----------------------------------------------------------------------------
F32	LLPolyMorphTarget::getMaxDistortion() 
{
	if (mMorphData) 
	{
		return mMorphData->mMaxDistortion; 
	}
	else
	{
		return 0.f;
	}
}

//-----------------------------------------------------------------------------
// apply()
//-----------------------------------------------------------------------------
static LLFastTimer::DeclareTimer FTM_APPLY_MORPH_TARGET("Apply Morph");

void LLPolyMorphTarget::apply( ESex avatar_sex )
{
	if (!mMorphData || mNumMorphMasksPending > 0)
	{
		return;
	}

	LLFastTimer t(FTM_APPLY_MORPH_TARGET);

	mLastSex = avatar_sex;

	// Check for NaN condition (NaN is detected if a variable doesn't equal itself.
	if (mCurWeight != mCurWeight)
	{
		mCurWeight = 0.0;
	}
	if (mLastWeight != mLastWeight)
	{
		mLastWeight = mCurWeight+.001;
	}

	// perform differential update of morph
	F32 delta_weight = ( getSex() & avatar_sex ) ? (mCurWeight - mLastWeight) : (getDefaultWeight() - mLastWeight);
	// store last weight
	mLastWeight += delta_weight;

	if (delta_weight != 0.f)
	{
		llassert(!mMesh->isLOD());
		LLVector4a *coords = mMesh->getWritableCoords();

		LLVector4a *scaled_normals = mMesh->getScaledNormals();
		LLVector4a *normals = mMesh->getWritableNormals();

		LLVector4a *scaled_binormals = mMesh->getScaledBinormals();
		LLVector4a *binormals = mMesh->getWritableBinormals();

		LLVector4a *clothing_weights = mMesh->getWritableClothingWeights();
		LLVector2 *tex_coords = mMesh->getWritableTexCoords();

		F32 *maskWeightArray = (mVertMask) ? mVertMask->getMorphMaskWeights() : NULL;

		for(U32 vert_index_morph = 0; vert_index_morph < mMorphData->mNumIndices; vert_index_morph++)
		{
			S32 vert_index_mesh = mMorphData->mVertexIndices[vert_index_morph];

			F32 maskWeight = 1.f;
			if (maskWeightArray)
			{
				maskWeight = maskWeightArray[vert_index_morph];
			}


			LLVector4a pos = mMorphData->mCoords[vert_index_morph];
			pos.mul(delta_weight*maskWeight);
			coords[vert_index_mesh].add(pos);

			if (getInfo()->mIsClothingMorph && clothing_weights)
			{
				LLVector4a clothing_offset = mMorphData->mCoords[vert_index_morph];
				clothing_offset.mul(delta_weight * maskWeight);
				LLVector4a* clothing_weight = &clothing_weights[vert_index_mesh];
				clothing_weight->add(clothing_offset);
				clothing_weight->getF32ptr()[VW] = maskWeight;
			}

			// calculate new normals based on half angles
			LLVector4a norm = mMorphData->mNormals[vert_index_morph];
			norm.mul(delta_weight*maskWeight*NORMAL_SOFTEN_FACTOR);
			scaled_normals[vert_index_mesh].add(norm);
			norm = scaled_normals[vert_index_mesh];
			norm.normalize3fast();
			normals[vert_index_mesh] = norm;

			// calculate new binormals
			LLVector4a binorm = mMorphData->mBinormals[vert_index_morph];
			binorm.mul(delta_weight*maskWeight*NORMAL_SOFTEN_FACTOR);
			scaled_binormals[vert_index_mesh].add(binorm);
			LLVector4a tangent;
			tangent.setCross3(scaled_binormals[vert_index_mesh], norm);
			LLVector4a& normalized_binormal = binormals[vert_index_mesh];
			normalized_binormal.setCross3(norm, tangent); 
			normalized_binormal.normalize3fast();
			
			tex_coords[vert_index_mesh] += mMorphData->mTexCoords[vert_index_morph] * delta_weight * maskWeight;
		}

		// now apply volume changes
		for( volume_list_t::iterator iter = mVolumeMorphs.begin(); iter != mVolumeMorphs.end(); iter++ )
		{
			LLPolyVolumeMorph* volume_morph = &(*iter);
			LLVector3 scale_delta = volume_morph->mScale * delta_weight;
			LLVector3 pos_delta = volume_morph->mPos * delta_weight;
			
			volume_morph->mVolume->setScale(volume_morph->mVolume->getScale() + scale_delta);
			volume_morph->mVolume->setPosition(volume_morph->mVolume->getPosition() + pos_delta);
		}
	}

	if (mNext)
	{
		mNext->apply(avatar_sex);
	}
}

//-----------------------------------------------------------------------------
// applyMask()
//-----------------------------------------------------------------------------
void	LLPolyMorphTarget::applyMask(U8 *maskTextureData, S32 width, S32 height, S32 num_components, BOOL invert)
{
	LLVector4a *clothing_weights = getInfo()->mIsClothingMorph ? mMesh->getWritableClothingWeights() : NULL;

	if (!mVertMask)
	{
		mVertMask = new LLPolyVertexMask(mMorphData);
		mNumMorphMasksPending--;
	}
	else
	{
		// remove effect of previous mask
		F32 *maskWeights = (mVertMask) ? mVertMask->getMorphMaskWeights() : NULL;

		if (maskWeights)
		{
			LLVector4a *coords = mMesh->getWritableCoords();
			LLVector4a *scaled_normals = mMesh->getScaledNormals();
			LLVector4a *scaled_binormals = mMesh->getScaledBinormals();
			LLVector2 *tex_coords = mMesh->getWritableTexCoords();

			LLVector4Logical clothing_mask;
			clothing_mask.clear();
			clothing_mask.setElement<0>();
			clothing_mask.setElement<1>();
			clothing_mask.setElement<2>();


			for(U32 vert = 0; vert < mMorphData->mNumIndices; vert++)
			{
				F32 lastMaskWeight = mLastWeight * maskWeights[vert];
				S32 out_vert = mMorphData->mVertexIndices[vert];

				// remove effect of existing masked morph
				LLVector4a t;
				t = mMorphData->mCoords[vert];
				t.mul(lastMaskWeight);
				coords[out_vert].sub(t);

				t = mMorphData->mNormals[vert];
				t.mul(lastMaskWeight*NORMAL_SOFTEN_FACTOR);
				scaled_normals[out_vert].sub(t);

				t = mMorphData->mBinormals[vert];
				t.mul(lastMaskWeight*NORMAL_SOFTEN_FACTOR);
				scaled_binormals[out_vert].sub(t);

				tex_coords[out_vert] -= mMorphData->mTexCoords[vert] * lastMaskWeight;

				if (clothing_weights)
				{
					LLVector4a clothing_offset = mMorphData->mCoords[vert];
					clothing_offset.mul(lastMaskWeight);
					LLVector4a* clothing_weight = &clothing_weights[out_vert];
					LLVector4a t;
					t.setSub(*clothing_weight, clothing_offset);
					clothing_weight->setSelectWithMask(clothing_mask, t, *clothing_weight);
				}
			}
		}
	}

	// set last weight to 0, since we've removed the effect of this morph
	mLastWeight = 0.f;

	mVertMask->generateMask(maskTextureData, width, height, num_components, invert, clothing_weights);

	apply(mLastSex);
}


//-----------------------------------------------------------------------------
// LLPolyVertexMask()
//-----------------------------------------------------------------------------
LLPolyVertexMask::LLPolyVertexMask(LLPolyMorphData* morph_data)
{
	mWeights = new F32[morph_data->mNumIndices];
	mMorphData = morph_data;
	mWeightsGenerated = FALSE;
}

//-----------------------------------------------------------------------------
// ~LLPolyVertexMask()
//-----------------------------------------------------------------------------
LLPolyVertexMask::~LLPolyVertexMask()
{
	delete[] mWeights;
}

//-----------------------------------------------------------------------------
// generateMask()
//-----------------------------------------------------------------------------
void LLPolyVertexMask::generateMask(U8 *maskTextureData, S32 width, S32 height, S32 num_components, BOOL invert, LLVector4a *clothing_weights)
{
// RN debug output that uses Image Debugger (http://www.cs.unc.edu/~baxter/projects/imdebug/)
//	BOOL debugImg = FALSE; 
//	if (debugImg)
//	{
//		if (invert)
//		{
//			imdebug("lum rbga=rgba b=8 w=%d h=%d *-1 %p", width, height, maskTextureData);
//		}
//		else
//		{
//			imdebug("lum rbga=rgba b=8 w=%d h=%d %p", width, height, maskTextureData);
//		}
//	}
	for (U32 index = 0; index < mMorphData->mNumIndices; index++)
	{
		S32 vertIndex = mMorphData->mVertexIndices[index];
		const S32 *sharedVertIndex = mMorphData->mMesh->getSharedVert(vertIndex);
		LLVector2 uvCoords;

		if (sharedVertIndex)
		{
			uvCoords = mMorphData->mMesh->getUVs(*sharedVertIndex);
		}
		else
		{
			uvCoords = mMorphData->mMesh->getUVs(vertIndex);
		}
		U32 s = llclamp((U32)(uvCoords.mV[VX] * (F32)(width - 1)), (U32)0, (U32)width - 1);
		U32 t = llclamp((U32)(uvCoords.mV[VY] * (F32)(height - 1)), (U32)0, (U32)height - 1);
		
		mWeights[index] = ((F32) maskTextureData[((t * width + s) * num_components) + (num_components - 1)]) / 255.f;
		
		if (invert) 
		{
			mWeights[index] = 1.f - mWeights[index];
		}

		// now apply step function
		// mWeights[index] = mWeights[index] > 0.95f ? 1.f : 0.f;

		if (clothing_weights)
		{
			clothing_weights[vertIndex].getF32ptr()[VW] = mWeights[index];
		}
	}
	mWeightsGenerated = TRUE;
}

//-----------------------------------------------------------------------------
// getMaskForMorphIndex()
//-----------------------------------------------------------------------------
F32* LLPolyVertexMask::getMorphMaskWeights()
{
	if (!mWeightsGenerated)
	{
		return NULL;
	}
	
	return mWeights;
}
