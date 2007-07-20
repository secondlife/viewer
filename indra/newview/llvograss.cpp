/** 
 * @file llvograss.cpp
 * @brief Not a blade, but a clump of grass
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llvograss.h"

#include "imageids.h"
#include "llviewercontrol.h"

#include "llagent.h"
#include "llviewerwindow.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llsurface.h"
#include "llsurfacepatch.h"
#include "llvosky.h"
#include "llviewercamera.h"
#include "llviewerimagelist.h"
#include "llviewerregion.h"
#include "pipeline.h"
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
	setTEImage(0, gImageList.getImage(sSpeciesTable[mSpecies]->mTextureID));
}


void alert_done(S32 option, void* user_data)
{
	return;
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

		if (newGrass->mTextureID.isNull())
		{
			LLString textureName;

			static LLStdStringHandle texture_name_string = LLXmlTree::addAttributeString("texture_name");
			success &= grass_def->getFastAttributeString(texture_name_string, textureName);
			newGrass->mTextureID.set( gViewerArt.getString(textureName) );
		}

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
			LLString name;
			static LLStdStringHandle name_string = LLXmlTree::addAttributeString("name");
			grass_def->getFastAttributeString(name_string, name);
			llwarns << "Incomplete definition of grass " << name << llendl;
		}
	}

	BOOL have_all_grass = TRUE;
	LLString err;
	char buffer[10];		/* Flawfinder: ignore */

	for (S32 i=0;i<sMaxGrassSpecies;++i)
	{
		if (!sSpeciesTable.count(i))
		{
			snprintf(buffer,10," %d",i);		/* Flawfinder: ignore */
			err.append(buffer);
			have_all_grass = FALSE;
		}
	}

	if (!have_all_grass) 
	{
		LLStringBase<char>::format_map_t args;
		args["[SPECIES]"] = err;
		gViewerWindow->alertXml("ErrorUndefinedGrasses", args, alert_done );
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

	if (  (getVelocity().magVecSquared() > 0.f)
		||(getAcceleration().magVecSquared() > 0.f)
		||(getAngularVelocity().magVecSquared() > 0.f))
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
	F32 range = relative_position.magVec();

	F32 max_scale = getMaxScale();

	mAppAngle = (F32) atan2( max_scale, range) * RAD_TO_DEG;

	// Compute pixels per meter at the given range
	F32 pixels_per_meter = gCamera->getViewHeightInPixels() / (tan(gCamera->getView()) * range);

	// Assume grass texture is a 5 meter by 5 meter sprite at the grass object's center
	mPixelArea = (pixels_per_meter) * (pixels_per_meter) * 25.f;
}


// BUG could speed this up by caching the relative_position and range calculations
void LLVOGrass::updateTextures(LLAgent &agent)
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

BOOL LLVOGrass::updateGeometry(LLDrawable *drawable)
{
	LLFastTimer ftm(LLFastTimer::FTM_UPDATE_GRASS);
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
	
	mDepth = (face->mCenterLocal - gCamera->getOrigin())*gCamera->getAtAxis();
	mDrawable->setPosition(face->mCenterLocal);
	mDrawable->movePartition();
	LLPipeline::sCompiles++;
}

void LLVOGrass::getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U32>& indicesp)
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
		normal1.normVec();
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
	return LLPipeline::PARTITION_GRASS;
}

LLGrassPartition::LLGrassPartition()
{
	mDrawableType = LLPipeline::RENDER_TYPE_GRASS;
	mPartitionType = LLPipeline::PARTITION_GRASS;
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
