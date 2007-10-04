/** 
 * @file llvotree.cpp
 * @brief LLVOTree class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "llagent.h"
#include "llcylinder.h"
#include "lldrawable.h"
#include "llface.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llworld.h"
#include "noise.h"
#include "pipeline.h"
#include "llviewerwindow.h"

extern LLPipeline gPipeline;

LLGLuint mLeafDList;

S32 LLVOTree::sLODVertexOffset[4];
S32 LLVOTree::sLODVertexCount[4];
S32 LLVOTree::sLODIndexOffset[4];
S32 LLVOTree::sLODIndexCount[4];
S32 LLVOTree::sLODSlices[4] = {10, 5, 4, 3};
F32 LLVOTree::sLODAngles[4] = {30.f, 20.f, 15.f, 0.f};

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
}


LLVOTree::~LLVOTree()
{
	if (mData)
	{
		delete[] mData;
		mData = NULL;
	}
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
				LLString name;
				static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
				tree_def->getFastAttributeString(name_string, name);
				llwarns << "Incomplete definition of tree " << name << llendl;
			}
		}
		
		BOOL have_all_trees = TRUE;
		LLString err;
		char buffer[10];		/* Flawfinder: ignore */

		for (S32 i=0;i<sMaxTreeSpecies;++i)
		{
			if (!sSpeciesTable.count(i))
			{
				snprintf(buffer,10," %d",i);		/* Flawfinder: ignore */
				err.append(buffer);
				have_all_trees = FALSE;
			}
		}

		if (!have_all_trees) 
		{
			LLStringBase<char>::format_map_t args;
			args["[SPECIES]"] = err;
			gViewerWindow->alertXml("ErrorUndefinedTrees", args );
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

	if (  (getVelocity().magVecSquared() > 0.f)
		||(getAcceleration().magVecSquared() > 0.f)
		||(getAngularVelocity().magVecSquared() > 0.f))
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
	mTreeImagep = gImageList.getImage(sSpeciesTable[mSpecies]->mTextureID);
	if (mTreeImagep)
	{
		mTreeImagep->bindTexture(0);
		mTreeImagep->setClamp(TRUE, TRUE);
	}
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

	return retval;
}

BOOL LLVOTree::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
	const U16 FRAMES_PER_WIND_UPDATE = 20;				//  How many frames between wind update per tree
	const F32 TREE_WIND_SENSITIVITY = 0.005f;
	const F32 TREE_TRUNK_STIFFNESS = 0.1f;

 	if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_TREE)))
	{
		return TRUE;
	}
	
	F32 mass_inv; 

	//  For all tree objects, update the trunk bending with the current wind 
	//  Walk sprite list in order away from viewer 
	if (!(mFrameCount % FRAMES_PER_WIND_UPDATE)) 
	{
		//  If needed, Get latest wind for this tree
		mWind = mRegionp->mWind.getVelocity(getPositionRegion());
	}
	mFrameCount++;

	mass_inv = 1.f/(5.f + mDepth*mBranches*0.2f);
	mTrunkVel += (mWind * mass_inv * TREE_WIND_SENSITIVITY);		//  Pull in direction of wind
	mTrunkVel -= (mTrunkBend * mass_inv * TREE_TRUNK_STIFFNESS);		//  Restoring force in direction of trunk 	
	mTrunkBend += mTrunkVel;
	mTrunkVel *= 0.99f;									//  Add damping

	if (mTrunkBend.magVec() > 1.f)
	{
		mTrunkBend.normVec();
	}

	if (mTrunkVel.magVec() > 1.f)
	{
		mTrunkVel.normVec();
	}

	return TRUE;
}


const F32 TREE_BLEND_MIN = 1.f;
const F32 TREE_BLEND_RANGE = 1.f;


void LLVOTree::render(LLAgent &agent)
{
}


void LLVOTree::setPixelAreaAndAngle(LLAgent &agent)
{
	// First calculate values as for any other object (for mAppAngle)
	LLViewerObject::setPixelAreaAndAngle(agent);

	// Re-calculate mPixelArea accurately
	
	// This should be the camera's center, as soon as we move to all region-local.
	LLVector3 relative_position = getPositionAgent() - agent.getCameraPositionAgent();
	F32 range = relative_position.magVec();				// ugh, square root

	F32 max_scale = mBillboardScale * getMaxScale();
	F32 area = max_scale * (max_scale*mBillboardRatio);

	// Compute pixels per meter at the given range
	F32 pixels_per_meter = gCamera->getViewHeightInPixels() / 
						   (tan(gCamera->getView()) * range);

	mPixelArea = (pixels_per_meter) * (pixels_per_meter) * area;
#if 0
	// mAppAngle is a bit of voodoo;
	// use the one calculated LLViewerObject::setPixelAreaAndAngle above
	// to avoid LOD miscalculations
	mAppAngle = (F32) atan2( max_scale, range) * RAD_TO_DEG;
#endif
}

void LLVOTree::updateTextures(LLAgent &agent)
{
	if (mTreeImagep)
	{
		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
		{
			setDebugText(llformat("%4.0f", fsqrtf(mPixelArea)));
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

BOOL LLVOTree::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(LLFastTimer::FTM_UPDATE_TREE);
	U32 i, j;
	const S32 MAX_SLICES = 32;

	const F32 LEAF_LEFT = 0.52f;
	const F32 LEAF_RIGHT = 0.98f;
	const F32 LEAF_TOP = 1.0f;
	const F32 LEAF_BOTTOM = 0.52f;

	const F32 LEAF_WIDTH = 1.f;


	U32 slices = MAX_SLICES;

	S32 max_indices = LEAF_INDICES;
	S32 max_vertices = LEAF_VERTICES;
	S32 lod;

	LLFace *face = drawable->getFace(0);

	face->mCenterAgent = getPositionAgent();
	face->mCenterLocal = face->mCenterAgent;

	for (lod = 0; lod < 4; lod++)
	{
		slices = sLODSlices[lod];
		sLODVertexOffset[lod] = max_vertices;
		sLODVertexCount[lod] = slices*slices;
		sLODIndexOffset[lod] = max_indices;
		sLODIndexCount[lod] = (slices-1)*(slices-1)*6;
		max_indices += sLODIndexCount[lod];
		max_vertices += sLODVertexCount[lod];
	}

	LLStrider<LLVector3> vertices;
	LLStrider<LLVector3> normals;
	LLStrider<LLVector2> tex_coords;
	LLStrider<U32> indicesp;

	face->setSize(max_vertices, max_indices);

	face->mVertexBuffer = new LLVertexBuffer(LLDrawPoolTree::VERTEX_DATA_MASK, GL_STATIC_DRAW_ARB);
	face->mVertexBuffer->allocateBuffer(max_vertices, max_indices, TRUE);
	face->setGeomIndex(0);
	face->setIndicesIndex(0);

	face->getGeometry(vertices, normals, tex_coords, indicesp);
	

	S32 vertex_count = 0;
	S32 index_count = 0;
	
	// First leaf
	for (i = 0; i < 4; i++)
	{
		*(normals++) =		LLVector3(0.f, 0.f, 1.f);
	}

	*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
	*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 0.f);
	vertex_count++;

	*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
	*(vertices++) =		LLVector3(0.5f*LEAF_WIDTH, 0.f, 1.f);
	vertex_count++;

    *(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
	*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 1.f);
	vertex_count++;

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
	for (i = 0; i < 4; i++)
	{
		*(normals++) =		LLVector3(0.f, 0.f, 1.f);
	}

	*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
	*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 0.f);
	vertex_count++;

	//*(tex_coords++) =	LLVector2(1.f, 1.0f);
	*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
	*(vertices++) =		LLVector3(0.5f*LEAF_WIDTH, 0.f, 1.f);
	vertex_count++;

	//*(tex_coords++) =	LLVector2(0.52f, 1.0f);
	*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
	*(vertices++) =		LLVector3(-0.5f*LEAF_WIDTH, 0.f, 1.f);
	vertex_count++;

	//*(tex_coords++) =	LLVector2(1.f, 0.52f);
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


	for (i = 0; i < 4; i++)
	{
		*(normals++) =		LLVector3(0.f, 0.f, 1.f);
	}

	*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
	*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 0.f);
	vertex_count++;

	*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
	*(vertices++) =		LLVector3(0.f, 0.5f*LEAF_WIDTH, 1.f);
	vertex_count++;

    *(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
	*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 1.f);
	vertex_count++;

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

	for (i = 0; i < 4; i++)
	{
		*(normals++) =		LLVector3(0.f, 0.f, 1.f);
	}

	*(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_BOTTOM);
	*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 0.f);
	vertex_count++;

	*(tex_coords++) =	LLVector2(LEAF_RIGHT, LEAF_TOP);
	*(vertices++) =		LLVector3(0.f, 0.5f*LEAF_WIDTH, 1.f);
	vertex_count++;

    *(tex_coords++) =	LLVector2(LEAF_LEFT, LEAF_TOP);
	*(vertices++) =		LLVector3(0.f, -0.5f*LEAF_WIDTH, 1.f);
	vertex_count++;

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

	for (lod = 0; lod < 4; lod++)
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
				nvec.setVec(	cos(nangle * DEG_TO_RAD)*start_radius*nvec_scale, 
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

	llassert(vertex_count == max_vertices);
	llassert(index_count == max_indices);

	return TRUE;
}

U32 LLVOTree::drawBranchPipeline(U32* indicesp, S32 trunk_LOD, S32 stop_level, U16 depth, U16 trunk_depth,  F32 scale, F32 twist, F32 droop,  F32 branches, F32 alpha)
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

	if (stop_level >= 0)
	{
		//
		//  Draw the tree using recursion
		//
		if (depth > stop_level)
		{
			{
				llassert(sLODIndexCount[trunk_LOD] > 0);
				width = scale * length * aspect;
				glPushMatrix();
				glScalef(width,width,scale * length);
 				glDrawElements(GL_TRIANGLES, sLODIndexCount[trunk_LOD], GL_UNSIGNED_INT, indicesp + sLODIndexOffset[trunk_LOD]);
				/*glDrawRangeElements(GL_TRIANGLES,
									sLODVertexOffset[trunk_LOD],
									sLODVertexOffset[trunk_LOD] + sLODVertexCount[trunk_LOD]-1,
									sLODIndexCount[trunk_LOD],
									GL_UNSIGNED_INT,
									indicesp + sLODIndexOffset[trunk_LOD]);*/
				stop_glerror();
				ret += sLODIndexCount[trunk_LOD];
				glPopMatrix();
			}
			
			// Recurse to create more branches
			for (S32 i=0; i < (S32)branches; i++) 
			{
				glPushMatrix();
				glTranslatef(0.f, 0.f, scale * length);
				glRotatef((constant_twist + ((i%2==0)?twist:-twist))*i, 0.f, 0.f, 1.f);
				glRotatef(droop, 0.f, 1.f, 0.f);
				glRotatef(20.f, 0.f, 0.f, 1.f);				// rotate 20deg about axis of new branch to add some random variation
				ret += drawBranchPipeline(indicesp, trunk_LOD, stop_level, depth - 1, 0, scale*mScaleStep, twist, droop, branches, alpha);
				glPopMatrix();
			}
			//  Recurse to continue trunk
			if (trunk_depth)
			{
				glPushMatrix();
				glTranslatef(0.f, 0.f, scale * length);
				glRotatef(70.5f, 0.f, 0.f, 1.f);					// rotate a bit around Z when ascending 
				ret += drawBranchPipeline(indicesp, trunk_LOD, stop_level, depth, trunk_depth-1, scale*mScaleStep, twist, droop, branches, alpha);
				glPopMatrix();
			}
		}
		else
		{
			//
			//  Draw leaves as two 90 deg crossed quads with leaf textures
			//
			{
				glPushMatrix();
				//glRotatef(llFrand(50.0), llFrand(1.0), llFrand(1.0), llFrand(1.0);
				//width = scale * (TREE_BRANCH_ASPECT + TREE_LEAF_ASPECT);
				glScalef(scale*mLeafScale, scale*mLeafScale, scale*mLeafScale);
				//glScalef(1.5f*width*mLeafScale,1,1.5f*scale*mLeafScale);
 				glDrawElements(GL_TRIANGLES, LEAF_INDICES, GL_UNSIGNED_INT, indicesp);
				/*glDrawRangeElements(GL_TRIANGLES,
									0,
									LEAF_VERTICES-1,
									LEAF_INDICES,
									GL_UNSIGNED_INT,
									indicesp);*/
				stop_glerror();
				ret += LEAF_INDICES;
				glPopMatrix();
			}
		}
	}
	else
	{
		//
		//  Draw the tree as a single billboard texture 
		//

		glMatrixMode(GL_TEXTURE);
		glPushMatrix();
		glTranslatef(0.0, -0.5, 0.0);
		glMatrixMode(GL_MODELVIEW);
		{
			glPushMatrix();
			glScalef(mBillboardScale*mBillboardRatio, mBillboardScale*mBillboardRatio, mBillboardScale);
 			glDrawElements(GL_TRIANGLES, LEAF_INDICES, GL_UNSIGNED_INT, indicesp);
/*			glDrawRangeElements(GL_TRIANGLES,
								0,
								LEAF_VERTICES-1,
								LEAF_INDICES,
								GL_UNSIGNED_INT,
								indicesp);*/
			stop_glerror();
			ret += LEAF_INDICES;
			glPopMatrix();
		}
		glMatrixMode(GL_TEXTURE);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
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

void LLVOTree::updateSpatialExtents(LLVector3& newMin, LLVector3& newMax)
{
	LLVector3 center = getRenderPosition();
	LLVector3 size = getScale();
	center.mV[2] += size.mV[2];

	newMin.setVec(center-size);
	newMax.setVec(center+size);
	mDrawable->setPositionGroup((newMin + newMax) * 0.5f);
}

U32 LLVOTree::getPartitionType() const
{ 
	return LLPipeline::PARTITION_TREE; 
}

LLTreePartition::LLTreePartition()
: LLSpatialPartition(0)
{
	mRenderByGroup = FALSE;
	mDrawableType = LLPipeline::RENDER_TYPE_TREE;
	mPartitionType = LLPipeline::PARTITION_TREE;
	mSlopRatio = 0.f;
	mLODPeriod = 1;
}

