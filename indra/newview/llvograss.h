/** 
 * @file llvograss.h
 * @brief Description of LLVOGrass class
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

#ifndef LL_LLVOGRASS_H
#define LL_LLVOGRASS_H

#include "llviewerobject.h"
#include <map>

class LLSurfacePatch;
class LLViewerTexture;


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

	void updateDrawable(bool force_damped);

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ bool		updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		getGeometry(S32 idx,
								LLStrider<LLVector4a>& verticesp,
								LLStrider<LLVector3>& normalsp, 
								LLStrider<LLVector2>& texcoordsp,
								LLStrider<LLColor4U>& colorsp, 
								LLStrider<LLColor4U>& emissivep,
								LLStrider<U16>& indicesp);

	void updateFaceSize(S32 idx) { }
	/*virtual*/ void updateTextures();											
	/*virtual*/ bool updateLOD();
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area

	void plantBlades();

	/*virtual*/ bool    isActive() const; // Whether this object needs to do an idleUpdate.
	/*virtual*/ void idleUpdate(LLAgent &agent, const F64 &time);

	/*virtual*/ bool lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, 
										  S32 face = -1,                        // which face to check, -1 = ALL_SIDES
										  bool pick_transparent = false,
										  bool pick_rigged = false,
                                          bool pick_unselectable = true,
										  S32* face_hit = NULL,                 // which face was hit
										  LLVector4a* intersection = NULL,       // return the intersection point
										  LLVector2* tex_coord = NULL,          // return the texture coordinates of the intersection point
										  LLVector4a* normal = NULL,             // return the surface normal at the intersection point
										  LLVector4a* tangent = NULL           // return the surface tangent at the intersection point
		);

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
