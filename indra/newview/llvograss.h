/** 
 * @file llvograss.h
 * @brief Description of LLVOGrass class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOGRASS_H
#define LL_LLVOGRASS_H

#include "llviewerobject.h"
#include "lldarray.h"
#include <map>

class LLSurfacePatch;
class LLViewerImage;


class LLVOGrass : public LLViewerObject
{
public:
	LLVOGrass(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	virtual ~LLVOGrass();

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();

	/*virtual*/ U32 processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, 
											const EObjectUpdateType update_type,
											LLDataPacker *dp);
	static void import(FILE *file, LLMessageSystem *mesgsys, const LLVector3 &pos);
	/*virtual*/ void exportFile(FILE *file, const LLVector3 &position);
	

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);

	/*virtual*/ void updateTextures(LLAgent &agent);											
	/*virtual*/ BOOL updateLOD();
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	void plantBlades();

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	static S32 sMaxGrassSpecies;

	struct GrassSpeciesData
	{
		LLUUID	mTextureID;
		
		F32		mBladeSizeX;
		F32		mBladeSizeY;
	};

	typedef std::map<U32, GrassSpeciesData*> SpeciesMap;

	U8				mSpecies;		// Species of grass
	F32				mBladeSizeX;
	F32				mBladeSizeY;
	
	LLSurfacePatch           *mPatch;			//  Stores the land patch where the grass is centered

	U64 mLastPatchUpdateTime;

	LLVector3		          mGrassBend;		// Accumulated wind (used for blowing trees)
	LLVector3		          mGrassVel;		
	LLVector3		          mWind;
	F32				          mBladeWindAngle;
	F32				          mBWAOverlap;

private:
	void updateSpecies();
	F32 mLastHeight;		// For cheap update hack
	S32 mNumBlades;

	static SpeciesMap sSpeciesTable;
};
#endif // LL_VO_GRASS_
