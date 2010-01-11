/** 
 * @file llvograss.cpp
 * @brief Not a blade, but a clump of grass
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "llvograss.h"

#include "imageids.h"
#include "llviewercontrol.h"

#include "llagent.h"
#include "llnotificationsutil.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llvosky.h"
#include "llviewercamera.h"
#include "llviewertexturelist.h"
#include "llviewerregion.h"
#include "pipeline.h"
#include "llspatialpartition.h"
#include "llworld.h"
#include "lldir.h"
#include "llxmltree.h"

const S32 GRASS_MAX_BLADES =	32;
const F32 GRASS_BLADE_BASE =	0.25f;			//  Width of grass at base
const F32 GRASS_BLADE_TOP =		0.25f;			//  Width of grass at top
const F32 GRASS_BLADE_HEIGHT =	0.5f;			// meters
const F32 GRASS_DISTRIBUTION_SD = 0.15f;		// empirically defined

F32 exp_x[GRASS_MAX_BLADES];
F32 exp_y[GRASS_MAX_BLADES];
F32 rot_x[GRASS_MAX_BLADES];
F32 rot_y[GRASS_MAX_BLADES];
F32 dz_x [GRASS_MAX_BLADES];
F32 dz_y [GRASS_MAX_BLADES];

F32 w_mod[GRASS_MAX_BLADES];					//  Factor to modulate wind movement by to randomize appearance

LLVOGrass::SpeciesMap LLVOGrass::sSpeciesTable;
S32 LLVOGrass::sMaxGrassSpecies = 0;


LLVOGrass::LLVOGrass(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
:	LLAlphaObject(id, pcode, regionp)
{
	mPatch               = NULL;
	mLastPatchUpdateTime = 0;
	mGrassVel.clearVec();
	mGrassBend.clearVec();
	mbCanSelect          = TRUE;

	mBladeWindAngle      = 35.f;
	mBWAOverlap          = 2.f;

	setNumTEs(1);

	setTEColor(0, LLColor4(1.0f, 1.0f, 1.0f, 1.f));
	mNumBlades = GRASS_MAX_BLADES;
}

LLVOGrass::~LLVOGrass()
{
}


void LLVOGrass::updateSpecies()
{
	mSpecies = mState;
	
	if (!sSpeciesTable.count(mSpecies))
	{
		llinfos << "Unknown grass type, substituting grass type." << llendl;
		SpeciesMap::const_iterator it = sSpeciesTable.begin();
		mSpecies = (*it).first;
	}
	setTEImage(0, LLViewerTextureManager::getFetchedTexture(sSpeciesTable[mSpecies]->mTextureID, TRUE, LLViewerTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE));
}


void LLVOGrass::initClass()
{
	LLVector3 pos(0.0f, 0.0f, 0.0f);
	//  Create nifty list of exponential distribution 0-1
	F32 x = 0.f;
	F32 y = 0.f;
	F32 rot;
	
	std::string xml_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"grass.xml");
	
	LLXmlTree grass_def_grass;

	if (!grass_def_grass.parseFile(xml_filename))
	{
		llerrs << "Failed to parse grass file." << llendl;
		return;
	}

	LLXmlTreeNode* rootp = grass_def_grass.getRoot();

	for (LLXmlTreeNode* grass_def = rootp->getFirstChild();
		grass_def;
		grass_def = rootp->getNextChild())
	{
		if (!grass_def->hasName("grass"))
		{
			llwarns << "Invalid grass definition node " << grass_def->getName() << llendl;
			continue;
		}
		F32 F32_val;
		LLUUID id;

		BOOL success = TRUE;

		S32 species;
		static LLStdStringHandle species_id_string = LLXmlTree::addAttributeString("species_id");
		if (!grass_def->getFastAttributeS32(species_id_string, species))
		{
			llwarns << "No species id defined" << llendl;
			continue;
		}

		if (species < 0)
		{
			llwarns << "Invalid species id " << species << llendl;
			continue;
		}

		GrassSpeciesData* newGrass = new GrassSpeciesData();


		static LLStdStringHandle texture_id_string = LLXmlTree::addAttributeString("texture_id");
		grass_def->getFastAttributeUUID(texture_id_string, id);
		newGrass->mTextureID = id;

		static LLStdStringHandle blade_sizex_string = LLXmlTree::addAttributeString("blade_size_x");
		success &= grass_def->getFastAttributeF32(blade_sizex_string, F32_val);
		newGrass->mBladeSizeX = F32_val;

		static LLStdStringHandle blade_sizey_string = LLXmlTree::addAttributeString("blade_size_y");
		success &= grass_def->getFastAttributeF32(blade_sizey_string, F32_val);
		newGrass->mBladeSizeY = F32_val;

		if (sSpeciesTable.count(species))
		{
			llinfos << "Grass species " << species << " already defined! Duplicate discarded." << llendl;
			delete newGrass;
			continue;
		}
		else
		{
			sSpeciesTable[species] = newGrass;
		}

		if (species >= sMaxGrassSpecies) sMaxGrassSpecies = species + 1;

		if (!success)
		{
			std::string name;
			static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
			grass_def->getFastAttributeString(name_string, name);
			llwarns << "Incomplete definition of grass " << name << llendl;
		}
	}

	BOOL have_all_grass = TRUE;
	std::string err;

	for (S32 i=0;i<sMaxGrassSpecies;++i)
	{
		if (!sSpeciesTable.count(i))
		{
			err.append(llformat(" %d",i));
			have_all_grass = FALSE;
		}
	}

	if (!have_all_grass) 
	{
		LLSD args;
		args["SPECIES"] = err;
		LLNotificationsUtil::add("ErrorUndefinedGrasses", args);
	}

	for (S32 i = 0; i < GRASS_MAX_BLADES; ++i)
	{
		if (1)   //(i%2 == 0)			Uncomment for X blading
		{
			F32 u = sqrt(-2.0f * log(ll_frand()));
			F32 v = 2.0f * F_PI * ll_frand();
			
			x = u * sin(v) * GRASS_DISTRIBUTION_SD;
			y = u * cos(v) * GRASS_DISTRIBUTION_SD;

			rot = ll_frand(F_PI);
		}
		else
		{
			rot += (F_PI*0.4f + ll_frand(0.2f*F_PI));
		}

		exp_x[i] = x;
		exp_y[i] = y;
		rot_x[i] = sin(rot);
		rot_y[i] = cos(rot);
		dz_x[i] = ll_frand(GRASS_BLADE_BASE * 0.25f);
		dz_y[i] = ll_frand(GRASS_BLADE_BASE * 0.25f);
		w_mod[i] = 0.5f + ll_frand();						//  Degree to which blade is moved by wind

	}
}

void LLVOGrass::cleanupClass()
{
	for_each(sSpeciesTable.begin(), sSpeciesTable.end(), DeletePairedPointer());
}

U32 LLVOGrass::processUpdateMessage(LLMessageSystem *mesgsys,
										  void **user_data,
										  U32 block_num,
										  const EObjectUpdateType update_type,
										  LLDataPacker *dp)
{
	// Do base class updates...
	U32 retval = LLViewerObject::processUpdateMessage(mesgsys, user_data, block_num, update_type, dp);

	updateSpecies();

	if (  (getVelocity().lengthSquared() > 0.f)
		||(getAcceleration().lengthSquared() > 0.f)
		||(getAngularVelocity().lengthSquared() > 0.f))
	{
		llinfos << "ACK! Moving grass!" << llendl;
		setVelocity(LLVector3::zero);
		setAcceleration(LLVector3::zero);
		setAngularVelocity(LLVector3::zero);
	}

	if (mDrawable)
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}

	return retval;
}

BOOL LLVOGrass::isActive() const
{
	return TRUE;
}

BOOL LLVOGrass::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
{
 	if (mDead || !(gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_GRASS)))
	{
		return TRUE;
	}
	
	if (!mDrawable)
	{
		// So drones work.
		return TRUE;
	}

	if (mPatch && (mLastPatchUpdateTime != mPatch->getLastUpdateTime()))
	{
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_VOLUME, TRUE);
	}

	return TRUE;
}


void LLVOGrass::setPixelAreaAndAngle(LLAgent &agent)
{
	// This should be the camera's center, as soon as we move to all region-local.
	LLVector3 relative_position = getPositionAgent() - agent.getCameraPositionAgent();
	F32 range = relative_position.length();

	F32 max_scale = getMaxScale();

	mAppAngle = (F32) atan2( max_scale, range) * RAD_TO_DEG;

	// Compute pixels per meter at the given range
	F32 pixels_per_meter = LLViewerCamera::getInstance()->getViewHeightInPixels() / (tan(LLViewerCamera::getInstance()->getView()) * range);

	// Assume grass texture is a 5 meter by 5 meter sprite at the grass object's center
	mPixelArea = (pixels_per_meter) * (pixels_per_meter) * 25.f;
}


// BUG could speed this up by caching the relative_position and range calculations
void LLVOGrass::updateTextures()
{
	if (getTEImage(0))
	{
		if (gPipeline.hasRenderDebugMask(LLPipeline::RENDER_DEBUG_TEXTURE_AREA))
		{
			setDebugText(llformat("%4.0f", fsqrtf(mPixelArea)));
		}
		getTEImage(0)->addTextureStats(mPixelArea);
	}
}

BOOL LLVOGrass::updateLOD()
{
	if (mDrawable->getNumFaces() <= 0)
	{
		return FALSE;
	}
	
	LLFace* face = mDrawable->getFace(0);

	F32 tan_angle = 0.f;
	S32 num_blades = 0;

	tan_angle = (mScale.mV[0]*mScale.mV[1])/mDrawable->mDistanceWRTCamera;
	num_blades = llmin(GRASS_MAX_BLADES, lltrunc(tan_angle * 5));
	num_blades = llmax(1, num_blades);
	if (num_blades >= (mNumBlades << 1))
	{
		while (mNumBlades < num_blades)
		{
			mNumBlades <<= 1;
		}

		face->setSize(mNumBlades*8, mNumBlades*12);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	else if (num_blades <= (mNumBlades >> 1))
	{
		while (mNumBlades > num_blades)
		{
			mNumBlades >>=1;
		}

		face->setSize(mNumBlades*8, mNumBlades*12);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
		return TRUE;
	}

	return FALSE;
}

LLDrawable* LLVOGrass::createDrawable(LLPipeline *pipeline)
{
	pipeline->allocDrawable(this);
	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_GRASS);
	
	return mDrawable;
}

static LLFastTimer::DeclareTimer FTM_UPDATE_GRASS("Update Grass");

BOOL LLVOGrass::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(FTM_UPDATE_GRASS);
	dirtySpatialGroup();
	plantBlades();
	return TRUE;
}

void LLVOGrass::plantBlades()
{
	// It is possible that the species of a grass is not defined
	// This is bad, but not the end of the world.
	if (!sSpeciesTable.count(mSpecies))
	{
		llinfos << "Unknown grass species " << mSpecies << llendl;
		return;
	}
	
	if (mDrawable->getNumFaces() < 1)
	{
		mDrawable->setNumFaces(1, NULL, getTEImage(0));
	}
		
	LLFace *face = mDrawable->getFace(0);

	face->setTexture(getTEImage(0));
	face->setState(LLFace::GLOBAL);
	face->setSize(mNumBlades * 8, mNumBlades * 12);
	face->mVertexBuffer = NULL;
	face->setTEOffset(0);
	face->mCenterLocal = mPosition + mRegionp->getOriginAgent();
	
	mDepth = (face->mCenterLocal - LLViewerCamera::getInstance()->getOrigin())*LLViewerCamera::getInstance()->getAtAxis();
	mDrawable->setPosition(face->mCenterLocal);
	mDrawable->movePartition();
	LLPipeline::sCompiles++;
}

void LLVOGrass::getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U16>& indicesp)
{
	mPatch = mRegionp->getLand().resolvePatchRegion(getPositionRegion());
	if (mPatch)
		mLastPatchUpdateTime = mPatch->getLastUpdateTime();
	
	LLVector3 position;
	// Create random blades of grass with gaussian distribution
	F32 x,y,xf,yf,dzx,dzy;

	LLColor4U color(255,255,255,255);

	LLFace *face = mDrawable->getFace(idx);

	F32 width  = sSpeciesTable[mSpecies]->mBladeSizeX;
	F32 height = sSpeciesTable[mSpecies]->mBladeSizeY;

	U32 index_offset = face->getGeomIndex();

	for (S32 i = 0;  i < mNumBlades; i++)
	{
		x   = exp_x[i] * mScale.mV[VX];
		y   = exp_y[i] * mScale.mV[VY];
		xf  = rot_x[i] * GRASS_BLADE_BASE * width * w_mod[i];
		yf  = rot_y[i] * GRASS_BLADE_BASE * width * w_mod[i];
		dzx = dz_x [i];
		dzy = dz_y [i];

		LLVector3 v1,v2,v3;
		F32 blade_height= GRASS_BLADE_HEIGHT * height * w_mod[i];

		*texcoordsp++   = LLVector2(0, 0);
		*texcoordsp++   = LLVector2(0, 0);
		*texcoordsp++   = LLVector2(0, 0.98f);
		*texcoordsp++   = LLVector2(0, 0.98f);
		*texcoordsp++   = LLVector2(1, 0);
		*texcoordsp++   = LLVector2(1, 0);
		*texcoordsp++   = LLVector2(1, 0.98f);
		*texcoordsp++   = LLVector2(1, 0.98f);

		position.mV[0]  = mPosition.mV[VX] + x + xf;
		position.mV[1]  = mPosition.mV[VY] + y + yf;
		position.mV[2]  = mRegionp->getLand().resolveHeightRegion(position);
		*verticesp++    = v1 = position + mRegionp->getOriginAgent();
		*verticesp++    = v1;


		position.mV[0] += dzx;
		position.mV[1] += dzy;
		position.mV[2] += blade_height;
		*verticesp++    = v2 = position + mRegionp->getOriginAgent();
		*verticesp++    = v2;

		position.mV[0]  = mPosition.mV[VX] + x - xf;
		position.mV[1]  = mPosition.mV[VY] + y - xf;
		position.mV[2]  = mRegionp->getLand().resolveHeightRegion(position);
		*verticesp++    = v3 = position + mRegionp->getOriginAgent();
		*verticesp++    = v3;

		LLVector3 normal1 = (v1-v2) % (v2-v3);
		normal1.mV[VZ] = 0.75f;
		normal1.normalize();
		LLVector3 normal2 = -normal1;
		normal2.mV[VZ] = -normal2.mV[VZ];

		position.mV[0] += dzx;
		position.mV[1] += dzy;
		position.mV[2] += blade_height;
		*verticesp++    = v1 = position + mRegionp->getOriginAgent();
		*verticesp++    = v1;

		*(normalsp++)   = normal1;
		*(normalsp++)   = normal2;
		*(normalsp++)   = normal1;
		*(normalsp++)   = normal2;

		*(normalsp++)   = normal1;
		*(normalsp++)   = normal2;
		*(normalsp++)   = normal1;
		*(normalsp++)   = normal2;

		*(colorsp++)   = color;
		*(colorsp++)   = color;
		*(colorsp++)   = color;
		*(colorsp++)   = color;
		*(colorsp++)   = color;
		*(colorsp++)   = color;
		*(colorsp++)   = color;
		*(colorsp++)   = color;
		
		*indicesp++     = index_offset + 0;
		*indicesp++     = index_offset + 2;
		*indicesp++     = index_offset + 4;

		*indicesp++     = index_offset + 2;
		*indicesp++     = index_offset + 6;
		*indicesp++     = index_offset + 4;

		*indicesp++     = index_offset + 1;
		*indicesp++     = index_offset + 5;
		*indicesp++     = index_offset + 3;

		*indicesp++     = index_offset + 3;
		*indicesp++     = index_offset + 5;
		*indicesp++     = index_offset + 7;
		index_offset   += 8;
	}

	LLPipeline::sCompiles++;
}

U32 LLVOGrass::getPartitionType() const
{
	return LLViewerRegion::PARTITION_GRASS;
}

LLGrassPartition::LLGrassPartition()
{
	mDrawableType = LLPipeline::RENDER_TYPE_GRASS;
	mPartitionType = LLViewerRegion::PARTITION_GRASS;
	mLODPeriod = 16;
	mDepthMask = TRUE;
	mSlopRatio = 0.1f;
	mRenderPass = LLRenderPass::PASS_GRASS;
	mBufferUsage = GL_DYNAMIC_DRAW_ARB;
}

// virtual
void LLVOGrass::updateDrawable(BOOL force_damped)
{
	// Force an immediate rebuild on any update
	if (mDrawable.notNull())
	{
		mDrawable->updateXform(TRUE);
		gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
	}
	clearChanged(SHIFTED);
}

// virtual 
BOOL LLVOGrass::lineSegmentIntersect(const LLVector3& start, const LLVector3& end, S32 face, BOOL pick_transparent, S32 *face_hitp,
									  LLVector3* intersection,LLVector2* tex_coord, LLVector3* normal, LLVector3* bi_normal)
	
{
	BOOL ret = FALSE;
	if (!mbCanSelect ||
		mDrawable->isDead() || 
		!gPipeline.hasRenderType(mDrawable->getRenderType()))
	{
		return FALSE;
	}

	LLVector3 dir = end-start;

	mPatch = mRegionp->getLand().resolvePatchRegion(getPositionRegion());
	
	LLVector3 position;
	// Create random blades of grass with gaussian distribution
	F32 x,y,xf,yf,dzx,dzy;

	LLColor4U color(255,255,255,255);

	F32 width  = sSpeciesTable[mSpecies]->mBladeSizeX;
	F32 height = sSpeciesTable[mSpecies]->mBladeSizeY;

	LLVector2 tc[4];
	LLVector3 v[4];
	// LLVector3 n[4]; // unused!

	F32 closest_t = 1.f;

	for (S32 i = 0;  i < mNumBlades; i++)
	{
		x   = exp_x[i] * mScale.mV[VX];
		y   = exp_y[i] * mScale.mV[VY];
		xf  = rot_x[i] * GRASS_BLADE_BASE * width * w_mod[i];
		yf  = rot_y[i] * GRASS_BLADE_BASE * width * w_mod[i];
		dzx = dz_x [i];
		dzy = dz_y [i];

		LLVector3 v1,v2,v3;
		F32 blade_height= GRASS_BLADE_HEIGHT * height * w_mod[i];

		tc[0]   = LLVector2(0, 0);
		tc[1]   = LLVector2(0, 0.98f);
		tc[2]   = LLVector2(1, 0);
		tc[3]   = LLVector2(1, 0.98f);
	
		position.mV[0]  = mPosition.mV[VX] + x + xf;
		position.mV[1]  = mPosition.mV[VY] + y + yf;
		position.mV[2]  = mRegionp->getLand().resolveHeightRegion(position);
		v[0]    = v1 = position + mRegionp->getOriginAgent();
		


		position.mV[0] += dzx;
		position.mV[1] += dzy;
		position.mV[2] += blade_height;
		v[1]    = v2 = position + mRegionp->getOriginAgent();
		
		position.mV[0]  = mPosition.mV[VX] + x - xf;
		position.mV[1]  = mPosition.mV[VY] + y - xf;
		position.mV[2]  = mRegionp->getLand().resolveHeightRegion(position);
		v[2]    = v3 = position + mRegionp->getOriginAgent();
		
		LLVector3 normal1 = (v1-v2) % (v2-v3);
		normal1.normalize();
		
		position.mV[0] += dzx;
		position.mV[1] += dzy;
		position.mV[2] += blade_height;
		v[3]    = v1 = position + mRegionp->getOriginAgent();
	

		F32 a,b,t;

		BOOL hit = FALSE;


		U32 idx0 = 0,idx1 = 0,idx2 = 0;

		if (LLTriangleRayIntersect(v[0], v[1], v[2], start, dir, &a, &b, &t, FALSE))
		{
			hit = TRUE;
			idx0 = 0; idx1 = 1; idx2 = 2;
		}
		else if (LLTriangleRayIntersect(v[1], v[3], v[2], start, dir, &a, &b, &t, FALSE))
		{
			hit = TRUE;
			idx0 = 1; idx1 = 3; idx2 = 2;
		}
		else if (LLTriangleRayIntersect(v[2], v[1], v[0], start, dir, &a, &b, &t, FALSE))
		{
			normal1 = -normal1;
			hit = TRUE;
			idx0 = 2; idx1 = 1; idx2 = 0;
		}
		else if (LLTriangleRayIntersect(v[2], v[3], v[1], start, dir, &a, &b, &t, FALSE))
		{
			normal1 = -normal1;
			hit = TRUE;
			idx0 = 2; idx1 = 3; idx2 = 1;
		}

		if (hit)
		{
			if (t >= 0.f &&
				t <= 1.f &&
				t < closest_t)
			{

				LLVector2 hit_tc = ((1.f - a - b)  * tc[idx0] +
									  a              * tc[idx1] +
									  b              * tc[idx2]);
				if (pick_transparent ||
					getTEImage(0)->getMask(hit_tc))
				{
					closest_t = t;
					if (intersection != NULL)
					{
						*intersection = start+dir*closest_t;
					}

					if (tex_coord != NULL)
					{
						*tex_coord = hit_tc;
					}

					if (normal != NULL)
					{
						*normal    = normal1;
					}
					ret = TRUE;
				}
			}
		}
	}

	return ret;
}

