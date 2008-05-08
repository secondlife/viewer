/** 
 * @file llvograss.h
 * @brief Description of LLVOGrass class
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

#ifndef LL_LLVOGRASS_H
#define LL_LLVOGRASS_H

#include "llviewerobject.h"
#include "lldarray.h"
#include <map>

class LLSurfacePatch;
class LLViewerImage;


class LLVOGrass : public LLAlphaObject
{
public:
	LLVOGrass(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);

	// Initialize data that's only inited once per class.
	static void initClass();
	static void cleanupClass();

	virtual U32 getPartitionType() const;

	/*virtual*/ U32 processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, 
											const EObjectUpdateType update_type,
											LLDataPacker *dp);
	static void import(LLFILE *file, LLMessageSystem *mesgsys, const LLVector3 &pos);
	/*virtual*/ void exportFile(LLFILE *file, const LLVector3 &position);

	void updateDrawable(BOOL force_damped);

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		getGeometry(S32 idx,
								LLStrider<LLVector3>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<U16>& indicesp);

	void updateFaceSize(S32 idx) { }
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

protected:
	~LLVOGrass();

private:
	void updateSpecies();
	F32 mLastHeight;		// For cheap update hack
	S32 mNumBlades;

	static SpeciesMap sSpeciesTable;
};
#endif // LL_VO_GRASS_
