/** 
 * @file llvotree.cpp
 * @brief LLVOTree class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llvotree.h"

#include "lldrawpooltree.h"

#include "llviewercontrol.h"
#include "lldir.h"
#include "llprimitive.h"
#include "lltree_common.h"
#include "llxmltree.h"
#include "material_codes.h"
#include "object_flags.h"

#include "llagentcamera.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "noise.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llnotificationsutil.h"
#include "raytrace.h"
#include "llglslshader.h"

extern LLPipeline gPipeline;

const S32 MAX_SLICES = 32;
const F32 LEAF_LEFT = 0.52f;
const F32 LEAF_RIGHT = 0.98f;
const F32 LEAF_TOP = 1.0f;
const F32 LEAF_BOTTOM = 0.52f;
const F32 LEAF_WIDTH = 1.f;

const S32 LLVOTree::sMAX_NUM_TREE_LOD_LEVELS = 4 ;

S32 LLVOTree::sLODVertexOffset[sMAX_NUM_TREE_LOD_LEVELS];
S32 LLVOTree::sLODVertexCount[sMAX_NUM_TREE_LOD_LEVELS];
S32 LLVOTree::sLODIndexOffset[sMAX_NUM_TREE_LOD_LEVELS];
S32 LLVOTree::sLODIndexCount[sMAX_NUM_TREE_LOD_LEVELS];
S32 LLVOTree::sLODSlices[sMAX_NUM_TREE_LOD_LEVELS] = {10, 5, 4, 3};
F32 LLVOTree::sLODAngles[sMAX_NUM_TREE_LOD_LEVELS] = {30.f, 20.f, 15.f, F_ALMOST_ZERO};

F32 LLVOTree::sTreeFactor = 1.f;

LLVOTree::SpeciesMap LLVOTree::sSpeciesTable;
S32 LLVOTree::sMaxTreeSpecies = 0;

// Tree variables and functions

LLVOTree::LLVOTree(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp):
						LLViewerObject(id, pcode, regionp)
{
	mSpecies = 0;
	mFrameCount = 0;
	mWind = mRegionp->mWind.getVelocity(getPositionRegion());
	mTrunkLOD = 0;
}


LLVOTree::~LLVOTree()
{
	if (mData)
	{
		delete[] mData;
		mData = NULL;
	}
}

//static
bool LLVOTree::isTreeRenderingStopped()
{
	return LLVOTree::sTreeFactor < LLVOTree::sLODAngles[sMAX_NUM_TREE_LOD_LEVELS - 1] ;
}

// static
void LLVOTree::initClass()
{
	std::string xml_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"trees.xml");
	
	LLXmlTree tree_def_tree;

	if (!tree_def_tree.parseFile(xml_filename))
	{
		llerrs << "Failed to parse tree file." << llendl;
	}

	LLXmlTreeNode* rootp = tree_def_tree.getRoot();

	for (LLXmlTreeNode* tree_def = rootp->getFirstChild();
		tree_def;
		tree_def = rootp->getNextChild())
		{
			if (!tree_def->hasName("tree"))
			{
				llwarns << "Invalid tree definition node " << tree_def->getName() << llendl;
				continue;
			}
			F32 F32_val;
			LLUUID id;
			S32 S32_val;

			BOOL success = TRUE;



			S32 species;
			static LLStdStringHandle species_id_string = LLXmlTree::addAttributeString("species_id");
			if (!tree_def->getFastAttributeS32(species_id_string, species))
			{
				llwarns << "No species id defined" << llendl;
				continue;
			}

			if (species < 0)
			{
				llwarns << "Invalid species id " << species << llendl;
				continue;
			}

			if (sSpeciesTable.count(species))
			{
				llwarns << "Tree species " << species << " already defined! Duplicate discarded." << llendl;
				continue;
			}

			TreeSpeciesData* newTree = new TreeSpeciesData();

			static LLStdStringHandle texture_id_string = LLXmlTree::addAttributeString("texture_id");
			success &= tree_def->getFastAttributeUUID(texture_id_string, id);
			newTree->mTextureID = id;
			
			static LLStdStringHandle droop_string = LLXmlTree::addAttributeString("droop");
			success &= tree_def->getFastAttributeF32(droop_string, F32_val);
			newTree->mDroop = F32_val;

			static LLStdStringHandle twist_string = LLXmlTree::addAttributeString("twist");
			success &= tree_def->getFastAttributeF32(twist_string, F32_val);
			newTree->mTwist = F32_val;
			
			static LLStdStringHandle branches_string = LLXmlTree::addAttributeString("branches");
			success &= tree_def->getFastAttributeF32(branches_string, F32_val);
			newTree->mBranches = F32_val;

			static LLStdStringHandle depth_string = LLXmlTree::addAttributeString("depth");
			success &= tree_def->getFastAttributeS32(depth_string, S32_val);
			newTree->mDepth = S32_val;

			static LLStdStringHandle scale_step_string = LLXmlTree::addAttributeString("scale_step");
			success &= tree_def->getFastAttributeF32(scale_step_string, F32_val);
			newTree->mScaleStep = F32_val;
			
			static LLStdStringHandle trunk_depth_string = LLXmlTree::addAttributeString("trunk_depth");
			success &= tree_def->getFastAttributeS32(trunk_depth_string, S32_val);
			newTree->mTrunkDepth = S32_val;
			
			static LLStdStringHandle branch_length_string = LLXmlTree::addAttributeString("branch_length");
			success &= tree_def->getFastAttributeF32(branch_length_string, F32_val);
			newTree->mBranchLength = F32_val;

			static LLStdStringHandle trunk_length_string = LLXmlTree::addAttributeString("trunk_length");
			success &= tree_def->getFastAttributeF32(trunk_length_string, F32_val);
			newTree->mTrunkLength = F32_val;

			static LLStdStringHandle leaf_scale_string = LLXmlTree::addAttributeString("leaf_scale");
			success &= tree_def->getFastAttributeF32(leaf_scale_string, F32_val);
			newTree->mLeafScale = F32_val;
			
			static LLStdStringHandle billboard_scale_string = LLXmlTree::addAttributeString("billboard_scale");
			success &= tree_def->getFastAttributeF32(billboard_scale_string, F32_val);
			newTree->mBillboardScale = F32_val;
			
			static LLStdStringHandle billboard_ratio_string = LLXmlTree::addAttributeString("billboard_ratio");
			success &= tree_def->getFastAttributeF32(billboard_ratio_string, F32_val);
			newTree->mBillboardRatio = F32_val;
			
			static LLStdStringHandle trunk_aspect_string = LLXmlTree::addAttributeString("trunk_aspect");
			success &= tree_def->getFastAttributeF32(trunk_aspect_string, F32_val);
			newTree->mTrunkAspect = F32_val;

			static LLStdStringHandle branch_aspect_string = LLXmlTree::addAttributeString("branch_aspect");
			success &= tree_def->getFastAttributeF32(branch_aspect_string, F32_val);
			newTree->mBranchAspect = F32_val;

			static LLStdStringHandle leaf_rotate_string = LLXmlTree::addAttributeString("leaf_rotate");
			success &= tree_def->getFastAttributeF32(leaf_rotate_string, F32_val);
			newTree->mRandomLeafRotate = F32_val;
			
			static LLStdStringHandle noise_mag_string = LLXmlTree::addAttributeString("noise_mag");
			success &= tree_def->getFastAttributeF32(noise_mag_string, F32_val);
			newTree->mNoiseMag = F32_val;

			static LLStdStringHandle noise_scale_string = LLXmlTree::addAttributeString("noise_scale");
			success &= tree_def->getFastAttributeF32(noise_scale_string, F32_val);
			newTree->mNoiseScale = F32_val;

			static LLStdStringHandle taper_string = LLXmlTree::addAttributeString("taper");
			success &= tree_def->getFastAttributeF32(taper_string, F32_val);
			newTree->mTaper = F32_val;

			static LLStdStringHandle repeat_z_string = LLXmlTree::addAttributeString("repeat_z");
			success &= tree_def->getFastAttributeF32(repeat_z_string, F32_val);
			newTree->mRepeatTrunkZ = F32_val;

			sSpeciesTable[species] = newTree;

			if (species >= sMaxTreeSpecies) sMaxTreeSpecies = species + 1;

			if (!success)
			{
				std::string name;
				static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
				tree_def->getFastAttributeString(name_string, name);
				llwarns << "Incomplete definition of tree " << name << llendl;
			}
		}
		
		BOOL have_all_trees = TRUE;
		std::string err;

		for (S32 i=0;i<sMaxTreeSpecies;++i)
		{
			if (!sSpeciesTable.count(i))
			{
				err.append(llformat(" %d",i));
				have_all_trees = FALSE;
			}
		}

		if (!have_all_trees) 
		{
			LLSD args;
			args["SPECIES"] = err;
			LLNotificationsUtil::add("ErrorUndefinedTrees", args);
		}
};

//static
void LLVOTree::cleanupClass()
{
	std::for_each(sSpeciesTable.begin(), sSpeciesTable.end(), DeletePairedPointer());
}

U32 LLVOTree::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num, EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	if (  (getVelocity().lengthSquared() > 0.f)
		||(getAcceleration().lengthSquared() > 0.f)
		||(getAngularVelocity().lengthSquared() > 0.f))
	{
		llinfos << "ACK! Moving tree!" << llendl;
		setVelocity(LLVector3::zero);
		setAcceleration(LLVector3::zero);
		setAngularVelocity(LLVector3::zero);
	}

	if (update_type == OUT_TERSE_IMPROVED)
	{
		// Nothing else needs to be done for the terse message.
		return retval;
	}

	// 
	//  Load Instance-Specific data 
	//
	if (mData)
	{
		mSpecies = ((U8 *)mData)[0];
	}
	
	if (!sSpeciesTable.count(mSpecies))
	{
		if (sSpeciesTable.size())
		{
			SpeciesMap::const_iterator it = sSpeciesTable.begin();
			mSpecies = (*it).first;
		}
	}

	//
	//  Load Species-Specific data 
	//
	static const S32 MAX_TREE_TEXTURE_VIRTURE_SIZE_RESET_INTERVAL = 32 ; //frames.
	mTreeImagep = LLViewerTextureManager::getFetchedTexture(sSpeciesTable[mSpecies]->mTextureID, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
	mTreeImagep->setMaxVirtualSizeResetInterval(MAX_TREE_TEXTURE_VIRTURE_SIZE_RESET_INTERVAL); //allow to wait for at most 16 frames to reset virtual size.

	mBranchLength = sSpeciesTable[mSpecies]->mBranchLength;
	mTrunkLength = sSpeciesTable[mSpecies]->mTrunkLength;
	mLeafScale = sSpeciesTable[mSpecies]->mLeafScale;
	mDroop = sSpeciesTable[mSpecies]->mDroop;
	mTwist = sSpeciesTable[mSpecies]->mTwist;
	mBranches = sSpeciesTable[mSpecies]->mBranches;
	mDepth = sSpeciesTable[mSpecies]->mDepth;
	mScaleStep = sSpeciesTable[mSpecies]->mScaleStep;
	mTrunkDepth = sSpeciesTable[mSpecies]->mTrunkDepth;
	mBillboardScale = sSpeciesTable[mSpecies]->mBillboardScale;
	mBillboardRatio = sSpeciesTable[mSpecies]->mBillboardRatio;
	mTrunkAspect = sSpeciesTable[mSpecies]->mTrunkAspect;
	mBranchAspect = sSpeciesTable[mSpecies]->mBranchAspect;
	
	// position change not caused by us, etc.  make sure to rebuild.
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL);

	return retval;
}

BOOL LLVOTree::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
 	if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_TREE)))
	{
		return TRUE;
	}
	
	S32 trunk_LOD = sMAX_NUM_TREE_LOD_LEVELS ;
	F32 app_angle = getAppAngle()*LLVOTree::sTreeFactor;

	for (S32 j = 0; j < sMAX_NUM_TREE_LOD_LEVELS; j++)
	{
		if (app_angle > LLVOTree::sLODAngles[j])
		{
			trunk_LOD = j;
			break;
		}
	} 

	if (mReferenceBuffer.isNull())
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	else if (trunk_LOD != mTrunkLOD)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, FALSE);
	}
	else
	{
		// we're not animating but we may *still* need to
		// regenerate the mesh if we moved, since position
		// and rotation are baked into the mesh.
		// *TODO: I don't know what's so special about trees
		// that they don't get REBUILD_POSITION automatically
		// at a higher level.
		const LLVector3 &this_position = getPositionRegion();
		if (this_position != mLastPosition)
		{
			gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_POSITION);
			mLastPosition = this_position;
		}
		else
		{
			const LLQuaternion &this_rotation = getRotation();
			
			if (this_rotation != mLastRotation)
			{
				gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_POSITION);
				mLastRotation = this_rotation;
			}
		}
	}

	mTrunkLOD = trunk_LOD;

	return TRUE;
}

const F32 TREE_BLEND_MIN = 1.f;
const F32 TREE_BLEND_RANGE = 1.f;

void LLVOTree::render(LLAgent &agent)
{
}


void LLVOTree::setPixelAreaAndAngle(LLAgent &agent)
{
	LLVector3 center = getPositionAgent();//center of tree.
	LLVector3 viewer_pos_agent = gAgentCamera.getCameraPositionAgent();
	LLVector3 lookAt = center - viewer_pos_agent;
	F32 dist = lookAt.normVec() ;	
	F32 cos_angle_to_view_dir = lookAt * LLViewerCamera::getInstance()->getXAxis() ;	
	
	F32 range = dist - getMinScale()/2;
	if (range < F_ALMOST_ZERO || isHUDAttachment())		// range == zero
	{
		mAppAngle = 180.f;
	}
	else
	{
		mAppAngle = (F32) atan2( getMaxScale(), range) * RAD_TO_DEG;		
	}

	F32 max_scale = mBillboardScale * getMaxScale();
	F32 area = max_scale * (max_scale*mBillboardRatio);
	// Compute pixels per meter at the given range
	F32 pixels_per_meter = LLViewerCamera::getInstance()->getViewHeightInPixels() / (tan(LLViewerCamera::getInstance()->getView()) * dist);
	mPixelArea = pixels_per_meter * pixels_per_meter * area ;	

	F32 importance = LLFace::calcImportanceToCamera(cos_angle_to_view_dir, dist) ;
	mPixelArea = LLFace::adjustPixelArea(importance, mPixelArea) ;
	if (mPixelArea > LLViewerCamera::getInstance()->getScreenPixelArea())
	{
		mAppAngle = 180.f;
	}

#if 0
	// mAppAngle is a bit of voodoo;
	// use the one calculated LLViewerObject::setPixelAreaAndAngle above
	// to avoid LOD miscalculations
	mAppAngle = (F32) atan2( max_scale, range) * RAD_TO_DEG;
#endif
}

void LLVOTree::updateTextures()
{
	if (mTreeImagep)
	{
		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
		{
			setDebugText(llformat("%4.0f", (F32) sqrt(mPixelArea)));
		}
		mTreeImagep->addTextureStats(mPixelArea);
	}

}


LLDrawable* LLVOTree::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setLit(FALSE);

	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_TREE);

	LLDrawPoolTree *poolp = (LLDrawPoolTree*) gPipeline.getPool(LLDrawPool::POOL_TREE, mTreeImagep);

	// Just a placeholder for an actual object...
	LLFace *facep = mDrawable->addFace(poolp, mTreeImagep);
	facep->setSize(1, 3);

	updateRadius();

	return mDrawable;
}


// Yes, I know this is bad.  I'll clean this up soon. - djs 04/02/02
const S32 LEAF_INDICES = 24;
const S32 LEAF_VERTICES = 16;

static LLFastTimer::DeclareTimer FTM_UPDATE_TREE("Update Tree");

BOOL LLVOTree::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(FTM_UPDATE_TREE);

	if(mTrunkLOD >= sMAX_NUM_TREE_LOD_LEVELS) //do not display the tree.
	{
		mReferenceBuffer = NULL ;
		LLFace * facep = drawable->getFace(0);
		if (facep)
		{
			facep->setVertexBuffer(NULL);
		}
		return TRUE ;
	}

	if (mDrawable->getFace(0) &&
		(mReferenceBuffer.isNull() || !mDrawable->getFace(0)->getVertexBuffer()))
	{
		const F32 SRR3 = 0.577350269f; // sqrt(1/3)
		const F32 SRR2 = 0.707106781f; // sqrt(1/2)
		U32 i, j;

		U32 slices = MAX_SLICES;

		S32 max_indices = LEAF_INDICES;
		S32 max_vertices = LEAF_VERTICES;
		S32 lod;

		LLFace *face = drawable->getFace(0);
		if (!face) return TRUE;

		face->mCenterAgent = getPositionAgent();
		face->mCenterLocal = face->mCenterAgent;

		for (lod = 0; lod < sMAX_NUM_TREE_LOD_LEVELS; lod++)
		{
			slices = sLODSlices[lod];
			sLODVertexOffset[lod] = max_vertices;
			sLODVertexCount[lod] = slices*slices;
			sLODIndexOffset[lod] = max_indices;
			sLODIndexCount[lod] = (slices-1)*(slices-1)*6;
			max_indices += sLODIndexCount[lod];
			max_vertices += sLODVertexCount[lod];
		}

		mReferenceBuffer = new LLVertexBuffer(LLDrawPoolTree::VERTEX_DATA_MASK, 0);
		mReferenceBuffer->allocateBuffer(max_vertices, max_indices, TRUE);

		LLStrider<LLVector3> vertices;
		LLStrider<LLVector3> normals;
		LLStrider<LLVector2> tex_coords;
		LLStrider<U16> indicesp;

		mReferenceBuffer->getVertexStrider(vertices);
		mReferenceBuffer->getNormalStrider(normals);
		mReferenceBuffer->getTexCoord0Strider(tex_coords);
		mReferenceBuffer->getIndexStrider(indicesp);
				
		S32 vertex_count = 0;
		S32 index_count = 0;
		
		// First leaf
		*(normals++) =		LLVector3(-SRR2, -SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 0.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR3, -SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
		*(vertices++) =		LLVector3(0.5f*LEAF_WIDTH, 0.f, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(-SRR3, -SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
		*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR2, -SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(0.5f*LEAF_WIDTH, 0.f, 0.f);
		vertex_count++;


		*(indicesp++) = 0;
		index_count++;
		*(indicesp++) = 1;
		index_count++;
		*(indicesp++) = 2;
		index_count++;

		*(indicesp++) = 0;
		index_count++;
		*(indicesp++) = 3;
		index_count++;
		*(indicesp++) = 1;
		index_count++;

		// Same leaf, inverse winding/normals
		*(normals++) =		LLVector3(-SRR2, SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 0.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR3, SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
		*(vertices++) =		LLVector3(0.5f*LEAF_WIDTH, 0.f, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(-SRR3, SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
		*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR2, SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(0.5f*LEAF_WIDTH, 0.f, 0.f);
		vertex_count++;

		*(indicesp++) = 4;
		index_count++;
		*(indicesp++) = 6;
		index_count++;
		*(indicesp++) = 5;
		index_count++;

		*(indicesp++) = 4;
		index_count++;
		*(indicesp++) = 5;
		index_count++;
		*(indicesp++) = 7;
		index_count++;


		// next leaf
		*(normals++) =		LLVector3(SRR2, -SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 0.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR3, SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
		*(vertices++) =		LLVector3(0.f, 0.5f*LEAF_WIDTH, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR3, -SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
		*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(SRR2, SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(0.f, 0.5f*LEAF_WIDTH, 0.f);
		vertex_count++;

		*(indicesp++) = 8;
		index_count++;
		*(indicesp++) = 9;
		index_count++;
		*(indicesp++) = 10;
		index_count++;

		*(indicesp++) = 8;
		index_count++;
		*(indicesp++) = 11;
		index_count++;
		*(indicesp++) = 9;
		index_count++;


		// other side of same leaf
		*(normals++) =		LLVector3(-SRR2, -SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 0.f);
		vertex_count++;

		*(normals++) =		LLVector3(-SRR3, SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
		*(vertices++) =		LLVector3(0.f, 0.5f*LEAF_WIDTH, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(-SRR3, -SRR3, SRR3);
		*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
		*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 1.f);
		vertex_count++;

		*(normals++) =		LLVector3(-SRR2, SRR2, 0.f);
		*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_BOTTOM);
		*(vertices++) =		LLVector3(0.f, 0.5f*LEAF_WIDTH, 0.f);
		vertex_count++;

		*(indicesp++) = 12;
		index_count++;
		*(indicesp++) = 14;
		index_count++;
		*(indicesp++) = 13;
		index_count++;

		*(indicesp++) = 12;
		index_count++;
		*(indicesp++) = 13;
		index_count++;
		*(indicesp++) = 15;
		index_count++;

		// Generate geometry for the cylinders

		// Different LOD's

		// Generate the vertices
		// Generate the indices

		for (lod = 0; lod < sMAX_NUM_TREE_LOD_LEVELS; lod++)
		{
			slices = sLODSlices[lod];
			F32 base_radius = 0.65f;
			F32 top_radius = base_radius * sSpeciesTable[mSpecies]->mTaper;
			//llinfos << "Species " << ((U32) mSpecies) << ", taper = " << sSpeciesTable[mSpecies].mTaper << llendl;
			//llinfos << "Droop " << mDroop << ", branchlength: " << mBranchLength << llendl;
			F32 angle = 0;
			F32 angle_inc = 360.f/(slices-1);
			F32 z = 0.f;
			F32 z_inc = 1.f;
			if (slices > 3)
			{
				z_inc = 1.f/(slices - 3);
			}
			F32 radius = base_radius;

			F32 x1,y1;
			F32 noise_scale = sSpeciesTable[mSpecies]->mNoiseMag;
			LLVector3 nvec;

			const F32 cap_nudge = 0.1f;			// Height to 'peak' the caps on top/bottom of branch

			const S32 fractal_depth = 5;
			F32 nvec_scale = 1.f * sSpeciesTable[mSpecies]->mNoiseScale;
			F32 nvec_scalez = 4.f * sSpeciesTable[mSpecies]->mNoiseScale;

			F32 tex_z_repeat = sSpeciesTable[mSpecies]->mRepeatTrunkZ;

			F32 start_radius;
			F32 nangle = 0;
			F32 height = 1.f;
			F32 r0;

			for (i = 0; i < slices; i++)
			{
				if (i == 0) 
				{
					z = - cap_nudge;
					r0 = 0.0;
				}
				else if (i == (slices - 1))
				{
					z = 1.f + cap_nudge;//((i - 2) * z_inc) + cap_nudge;
					r0 = 0.0;
				}
				else  
				{
					z = (i - 1) * z_inc;
					r0 = base_radius + (top_radius - base_radius)*z;
				}

				for (j = 0; j < slices; j++)
				{
					if (slices - 1 == j)
					{
						angle = 0.f;
					}
					else
					{
						angle =  j*angle_inc;
					}
				
					nangle = angle;
					
					x1 = cos(angle * DEG_TO_RAD);
					y1 = sin(angle * DEG_TO_RAD);
					LLVector2 tc;
					// This isn't totally accurate.  Should compute based on slope as well.
					start_radius = r0 * (1.f + 1.2f*fabs(z - 0.66f*height)/height);
					nvec.set(	cos(nangle * DEG_TO_RAD)*start_radius*nvec_scale, 
								sin(nangle * DEG_TO_RAD)*start_radius*nvec_scale, 
								z*nvec_scalez); 
					// First and last slice at 0 radius (to bring in top/bottom of structure)
					radius = start_radius + turbulence3((F32*)&nvec.mV, (F32)fractal_depth)*noise_scale;

					if (slices - 1 == j)
					{
						// Not 0.5 for slight slop factor to avoid edges on leaves
						tc = LLVector2(0.490f, (1.f - z/2.f)*tex_z_repeat);
					}
					else
					{
						tc = LLVector2((angle/360.f)*0.5f, (1.f - z/2.f)*tex_z_repeat);
					}

					*(vertices++) =		LLVector3(x1*radius, y1*radius, z);
					*(normals++) =		LLVector3(x1, y1, 0.f);
					*(tex_coords++) = tc;
					vertex_count++;
				}
			}

			for (i = 0; i < (slices - 1); i++)
			{
				for (j = 0; j < (slices - 1); j++)
				{
					S32 x1_offset = j+1;
					if ((j+1) == slices)
					{
						x1_offset = 0;
					}
					// Generate the matching quads
					*(indicesp) = j + (i*slices) + sLODVertexOffset[lod];
					llassert(*(indicesp) < (U32)max_vertices);
					indicesp++;
					index_count++;
					*(indicesp) = x1_offset + ((i+1)*slices) + sLODVertexOffset[lod];
					llassert(*(indicesp) < (U32)max_vertices);
					indicesp++;
					index_count++;
					*(indicesp) = j + ((i+1)*slices) + sLODVertexOffset[lod];
					llassert(*(indicesp) < (U32)max_vertices);
					indicesp++;
					index_count++;

					*(indicesp) = j + (i*slices) + sLODVertexOffset[lod];
					llassert(*(indicesp) < (U32)max_vertices);
					indicesp++;
					index_count++;
					*(indicesp) = x1_offset + (i*slices) + sLODVertexOffset[lod];
					llassert(*(indicesp) < (U32)max_vertices);
					indicesp++;
					index_count++;
					*(indicesp) = x1_offset + ((i+1)*slices) + sLODVertexOffset[lod];
					llassert(*(indicesp) < (U32)max_vertices);
					indicesp++;
					index_count++;
				}
			}
			slices /= 2; 
		}

		mReferenceBuffer->flush();
		llassert(vertex_count == max_vertices);
		llassert(index_count == max_indices);
	}

	//generate tree mesh
	updateMesh();
	
	return TRUE;
}

void LLVOTree::updateMesh()
{
	LLMatrix4 matrix;
	
	// Translate to tree base  HACK - adjustment in Z plants tree underground
	const LLVector3 &pos_region = getPositionRegion();
	//gGL.translatef(pos_agent.mV[VX], pos_agent.mV[VY], pos_agent.mV[VZ] - 0.1f);
	LLMatrix4 trans_mat;
	trans_mat.setTranslation(pos_region.mV[VX], pos_region.mV[VY], pos_region.mV[VZ] - 0.1f);
	trans_mat *= matrix;
	
	// Rotate to tree position and bend for current trunk/wind
	// Note that trunk stiffness controls the amount of bend at the trunk as 
	// opposed to the crown of the tree
	// 
	const F32 TRUNK_STIFF = 22.f;
	
	LLQuaternion rot = 
		LLQuaternion(mTrunkBend.magVec()*TRUNK_STIFF*DEG_TO_RAD, LLVector4(mTrunkBend.mV[VX], mTrunkBend.mV[VY], 0)) *
		LLQuaternion(90.f*DEG_TO_RAD, LLVector4(0,0,1)) *
		getRotation();

	LLMatrix4 rot_mat(rot);
	rot_mat *= trans_mat;

	F32 radius = getScale().magVec()*0.05f;
	LLMatrix4 scale_mat;
	scale_mat.mMatrix[0][0] = 
		scale_mat.mMatrix[1][1] =
		scale_mat.mMatrix[2][2] = radius;

	scale_mat *= rot_mat;

//	const F32 THRESH_ANGLE_FOR_BILLBOARD = 15.f;
//	const F32 BLEND_RANGE_FOR_BILLBOARD = 3.f;

	F32 droop = mDroop + 25.f*(1.f - mTrunkBend.magVec());
	
	S32 stop_depth = 0;
	F32 alpha = 1.0;
	
	U32 vert_count = 0;
	U32 index_count = 0;
	
	calcNumVerts(vert_count, index_count, mTrunkLOD, stop_depth, mDepth, mTrunkDepth, mBranches);

	LLFace* facep = mDrawable->getFace(0);
	if (!facep) return;
	LLVertexBuffer* buff = new LLVertexBuffer(LLDrawPoolTree::VERTEX_DATA_MASK, GL_STATIC_DRAW_ARB);
	buff->allocateBuffer(vert_count, index_count, TRUE);
	facep->setVertexBuffer(buff);
	
	LLStrider<LLVector3> vertices;
	LLStrider<LLVector3> normals;
	LLStrider<LLVector2> tex_coords;
	LLStrider<U16> indices;
	U16 idx_offset = 0;

	buff->getVertexStrider(vertices);
	buff->getNormalStrider(normals);
	buff->getTexCoord0Strider(tex_coords);
	buff->getIndexStrider(indices);

	genBranchPipeline(vertices, normals, tex_coords, indices, idx_offset, scale_mat, mTrunkLOD, stop_depth, mDepth, mTrunkDepth, 1.0, mTwist, droop, mBranches, alpha);
	
	mReferenceBuffer->flush();
	buff->flush();
}

void LLVOTree::appendMesh(LLStrider<LLVector3>& vertices, 
						 LLStrider<LLVector3>& normals, 
						 LLStrider<LLVector2>& tex_coords, 
						 LLStrider<U16>& indices,
						 U16& cur_idx,
						 LLMatrix4& matrix,
						 LLMatrix4& norm_mat,
						 S32 vert_start,
						 S32 vert_count,
						 S32 index_count,
						 S32 index_offset)
{
	LLStrider<LLVector3> v;
	LLStrider<LLVector3> n;
	LLStrider<LLVector2> t;
	LLStrider<U16> idx;

	mReferenceBuffer->getVertexStrider(v);
	mReferenceBuffer->getNormalStrider(n);
	mReferenceBuffer->getTexCoord0Strider(t);
	mReferenceBuffer->getIndexStrider(idx);
	
	//copy/transform vertices into mesh - check
	for (S32 i = 0; i < vert_count; i++)
	{ 
		U16 index = vert_start + i;
		*vertices++ = v[index] * matrix;
		LLVector3 norm = n[index] * norm_mat;
		norm.normalize();
		*normals++ = norm;
		*tex_coords++ = t[index];
	}

	//copy offset indices into mesh - check
	for (S32 i = 0; i < index_count; i++)
	{
		U16 index = index_offset + i;
		*indices++ = idx[index]-vert_start+cur_idx;
	}

	//increment index offset - check
	cur_idx += vert_count;
}
								 

void LLVOTree::genBranchPipeline(LLStrider<LLVector3>& vertices, 
								 LLStrider<LLVector3>& normals, 
								 LLStrider<LLVector2>& tex_coords, 
								 LLStrider<U16>& indices,
								 U16& index_offset,
								 LLMatrix4& matrix, 
								 S32 trunk_LOD, 
								 S32 stop_level, 
								 U16 depth, 
								 U16 trunk_depth,  
								 F32 scale, 
								 F32 twist, 
								 F32 droop,  
								 F32 branches, 
								 F32 alpha)
{
	//
	//  Generates a tree mesh by recursing, generating branches and then a 'leaf' texture.
	
	static F32 constant_twist;
	static F32 width = 0;

	F32 length = ((trunk_depth || (scale == 1.f))? mTrunkLength:mBranchLength);
	F32 aspect = ((trunk_depth || (scale == 1.f))? mTrunkAspect:mBranchAspect);
	
	constant_twist = 360.f/branches;

	if (stop_level >= 0)
	{
		if (depth > stop_level)
		{
			{
				llassert(sLODIndexCount[trunk_LOD] > 0);
				width = scale * length * aspect;
				LLMatrix4 scale_mat;
				scale_mat.mMatrix[0][0] = width;
				scale_mat.mMatrix[1][1] = width;
				scale_mat.mMatrix[2][2] = scale*length;
				scale_mat *= matrix;

				glh::matrix4f norm((F32*) scale_mat.mMatrix);
				LLMatrix4 norm_mat = LLMatrix4(norm.inverse().transpose().m);

				norm_mat.invert();
				appendMesh(vertices, normals, tex_coords, indices, index_offset, scale_mat, norm_mat, 
							sLODVertexOffset[trunk_LOD], sLODVertexCount[trunk_LOD], sLODIndexCount[trunk_LOD], sLODIndexOffset[trunk_LOD]);
			}
			
			// Recurse to create more branches
			for (S32 i=0; i < (S32)branches; i++) 
			{
				LLMatrix4 trans_mat;
				trans_mat.setTranslation(0,0,scale*length);
				trans_mat *= matrix;

				LLQuaternion rot = 
					LLQuaternion(20.f*DEG_TO_RAD, LLVector4(0.f, 0.f, 1.f)) *
					LLQuaternion(droop*DEG_TO_RAD, LLVector4(0.f, 1.f, 0.f)) *
					LLQuaternion(((constant_twist + ((i%2==0)?twist:-twist))*i)*DEG_TO_RAD, LLVector4(0.f, 0.f, 1.f));
				
				LLMatrix4 rot_mat(rot);
				rot_mat *= trans_mat;

				genBranchPipeline(vertices, normals, tex_coords, indices, index_offset, rot_mat, trunk_LOD, stop_level, depth - 1, 0, scale*mScaleStep, twist, droop, branches, alpha);
			}
			//  Recurse to continue trunk
			if (trunk_depth)
			{
				LLMatrix4 trans_mat;
				trans_mat.setTranslation(0,0,scale*length);
				trans_mat *= matrix;

				LLMatrix4 rot_mat(70.5f*DEG_TO_RAD, LLVector4(0,0,1));
				rot_mat *= trans_mat; // rotate a bit around Z when ascending 
				genBranchPipeline(vertices, normals, tex_coords, indices, index_offset, rot_mat, trunk_LOD, stop_level, depth, trunk_depth-1, scale*mScaleStep, twist, droop, branches, alpha);
			}
		}
		else
		{
			//
			//  Append leaves as two 90 deg crossed quads with leaf textures
			//
			{
				LLMatrix4 scale_mat;
				scale_mat.mMatrix[0][0] = 
					scale_mat.mMatrix[1][1] =
					scale_mat.mMatrix[2][2] = scale*mLeafScale;

				scale_mat *= matrix;

				glh::matrix4f norm((F32*) scale_mat.mMatrix);
				LLMatrix4 norm_mat = LLMatrix4(norm.inverse().transpose().m);

				appendMesh(vertices, normals, tex_coords, indices, index_offset, scale_mat, norm_mat, 0, LEAF_VERTICES, LEAF_INDICES, 0);	
			}
		}
	}
}



void LLVOTree::calcNumVerts(U32& vert_count, U32& index_count, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth, F32 branches)
{
	if (stop_level >= 0)
	{
		if (depth > stop_level)
		{
			index_count += sLODIndexCount[trunk_LOD];
			vert_count += sLODVertexCount[trunk_LOD];

			// Recurse to create more branches
			for (S32 i=0; i < (S32)branches; i++) 
			{
				calcNumVerts(vert_count, index_count, trunk_LOD, stop_level, depth - 1, 0, branches);
			}
			
			//  Recurse to continue trunk
			if (trunk_depth)
			{
				calcNumVerts(vert_count, index_count, trunk_LOD, stop_level, depth, trunk_depth-1, branches);
			}
		}
		else
		{
			index_count += LEAF_INDICES;
			vert_count += LEAF_VERTICES;
		}
	}
	else
	{
		index_count += LEAF_INDICES;
		vert_count += LEAF_VERTICES;
	}
}

U32 LLVOTree::drawBranchPipeline(LLMatrix4& matrix, U16* indicesp, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth,  F32 scale, F32 twist, F32 droop,  F32 branches, F32 alpha)
{
	U32 ret = 0;
	//
	//  Draws a tree by recursing, drawing branches and then a 'leaf' texture.
	//  If stop_level = -1, simply draws the whole tree as a billboarded texture
	//
	
	static F32 constant_twist;
	static F32 width = 0;

	//F32 length = ((scale == 1.f)? mTrunkLength:mBranchLength);
	//F32 aspect = ((scale == 1.f)? mTrunkAspect:mBranchAspect);
	F32 length = ((trunk_depth || (scale == 1.f))? mTrunkLength:mBranchLength);
	F32 aspect = ((trunk_depth || (scale == 1.f))? mTrunkAspect:mBranchAspect);
	
	constant_twist = 360.f/branches;

	if (!LLPipeline::sReflectionRender && stop_level >= 0)
	{
		//
		//  Draw the tree using recursion
		//
		if (depth > stop_level)
		{
			{
				llassert(sLODIndexCount[trunk_LOD] > 0);
				width = scale * length * aspect;
				LLMatrix4 scale_mat;
				scale_mat.mMatrix[0][0] = width;
				scale_mat.mMatrix[1][1] = width;
				scale_mat.mMatrix[2][2] = scale*length;
				scale_mat *= matrix;

				gGL.loadMatrix((F32*) scale_mat.mMatrix);
				gGL.syncMatrices();
 				glDrawElements(GL_TRIANGLES, sLODIndexCount[trunk_LOD], GL_UNSIGNED_SHORT, indicesp + sLODIndexOffset[trunk_LOD]);
				gPipeline.addTrianglesDrawn(LEAF_INDICES);
				stop_glerror();
				ret += sLODIndexCount[trunk_LOD];
			}
			
			// Recurse to create more branches
			for (S32 i=0; i < (S32)branches; i++) 
			{
				LLMatrix4 trans_mat;
				trans_mat.setTranslation(0,0,scale*length);
				trans_mat *= matrix;

				LLQuaternion rot = 
					LLQuaternion(20.f*DEG_TO_RAD, LLVector4(0.f, 0.f, 1.f)) *
					LLQuaternion(droop*DEG_TO_RAD, LLVector4(0.f, 1.f, 0.f)) *
					LLQuaternion(((constant_twist + ((i%2==0)?twist:-twist))*i)*DEG_TO_RAD, LLVector4(0.f, 0.f, 1.f));
				
				LLMatrix4 rot_mat(rot);
				rot_mat *= trans_mat;

				ret += drawBranchPipeline(rot_mat, indicesp, trunk_LOD, stop_level, depth - 1, 0, scale*mScaleStep, twist, droop, branches, alpha);
			}
			//  Recurse to continue trunk
			if (trunk_depth)
			{
				LLMatrix4 trans_mat;
				trans_mat.setTranslation(0,0,scale*length);
				trans_mat *= matrix;

				LLMatrix4 rot_mat(70.5f*DEG_TO_RAD, LLVector4(0,0,1));
				rot_mat *= trans_mat; // rotate a bit around Z when ascending 
				ret += drawBranchPipeline(rot_mat, indicesp, trunk_LOD, stop_level, depth, trunk_depth-1, scale*mScaleStep, twist, droop, branches, alpha);
			}
		}
		else
		{
			//
			//  Draw leaves as two 90 deg crossed quads with leaf textures
			//
			{
				LLMatrix4 scale_mat;
				scale_mat.mMatrix[0][0] = 
					scale_mat.mMatrix[1][1] =
					scale_mat.mMatrix[2][2] = scale*mLeafScale;

				scale_mat *= matrix;

			
				gGL.loadMatrix((F32*) scale_mat.mMatrix);
				gGL.syncMatrices();
				glDrawElements(GL_TRIANGLES, LEAF_INDICES, GL_UNSIGNED_SHORT, indicesp);
				gPipeline.addTrianglesDrawn(LEAF_INDICES);							
				stop_glerror();
				ret += LEAF_INDICES;
			}
		}
	}
	else
	{
		//
		//  Draw the tree as a single billboard texture 
		//

		LLMatrix4 scale_mat;
		scale_mat.mMatrix[0][0] = 
			scale_mat.mMatrix[1][1] =
			scale_mat.mMatrix[2][2] = mBillboardScale*mBillboardRatio;

		scale_mat *= matrix;
	
		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.translatef(0.0, -0.5, 0.0);
		gGL.matrixMode(LLRender::MM_MODELVIEW);
					
		gGL.loadMatrix((F32*) scale_mat.mMatrix);
		gGL.syncMatrices();
		glDrawElements(GL_TRIANGLES, LEAF_INDICES, GL_UNSIGNED_SHORT, indicesp);
		gPipeline.addTrianglesDrawn(LEAF_INDICES);
		stop_glerror();
		ret += LEAF_INDICES;

		gGL.matrixMode(LLRender::MM_TEXTURE);
		gGL.loadIdentity();
		gGL.matrixMode(LLRender::MM_MODELVIEW);
	}

	return ret;
}

void LLVOTree::updateRadius()
{
	if (mDrawable.isNull())
	{
		return;
	}
		
	mDrawable->setRadius(32.0f);
}

void LLVOTree::updateSpatialExtents(LLVector4a& newMin, LLVector4a& newMax)
{
	F32 radius = getScale().length()*0.05f;
	LLVector3 center = getRenderPosition();

	F32 sz = mBillboardScale*mBillboardRatio*radius*0.5f; 
	LLVector3 size(sz,sz,sz);

	center += LLVector3(0, 0, size.mV[2]) * getRotation();
	
	newMin.load3((center-size).mV);
	newMax.load3((center+size).mV);
	LLVector4a pos;
	pos.load3(center.mV);
	mDrawable->setPositionGroup(pos);
}

BOOL LLVOTree::lineSegmentIntersect(const LLVector3& start, const LLVector3& end, S32 face, BOOL pick_transparent, S32 *face_hitp,
									  LLVector3* intersection,LLVector2* tex_coord, LLVector3* normal, LLVector3* bi_normal)
	
{

	if (!lineSegmentBoundingBox(start, end))
	{
		return FALSE;
	}

	const LLVector4a* exta = mDrawable->getSpatialExtents();

	//VECTORIZE THIS
	LLVector3 ext[2];
	ext[0].set(exta[0].getF32ptr());
	ext[1].set(exta[1].getF32ptr());
	
	LLVector3 center = (ext[1]+ext[0])*0.5f;
	LLVector3 size = (ext[1]-ext[0]);

	LLQuaternion quat = getRotation();

	center -= LLVector3(0,0,size.magVec() * 0.25f)*quat;

	size.scaleVec(LLVector3(0.25f, 0.25f, 1.f));
	size.mV[0] = llmin(size.mV[0], 1.f);
	size.mV[1] = llmin(size.mV[1], 1.f);

	LLVector3 pos, norm;
		
	if (linesegment_tetrahedron(start, end, center, size, quat, pos, norm))
	{
		if (intersection)
		{
			*intersection = pos;
		}

		if (normal)
		{
			*normal = norm;
		}
		return TRUE;
	}
	
	return FALSE;
}

U32 LLVOTree::getPartitionType() const
{ 
	return LLViewerRegion::PARTITION_TREE; 
}

LLTreePartition::LLTreePartition()
: LLSpatialPartition(0, FALSE, GL_DYNAMIC_DRAW_ARB)
{
	mDrawableType = LLPipeline::RENDER_TYPE_TREE;
	mPartitionType = LLViewerRegion::PARTITION_TREE;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

