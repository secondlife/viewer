/** 
 * @file llvoclouds.h
 * @brief Description of LLVOClouds class
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

#ifndef LL_LLVOCLOUDS_H
#define LL_LLVOCLOUDS_H

#include "llviewerobject.h"
#include "lltable.h"
#include "v4coloru.h"

class LLViewerImage;
class LLViewerCloudGroup;

class LLCloudGroup;


class LLVOClouds : public LLAlphaObject
{
public:
	LLVOClouds(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp );

	// Initialize data that's only inited once per class.
	static void initClass();

	void updateDrawable(BOOL force_damped); 

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL        updateGeometry(LLDrawable *drawable);
	/*virtual*/ void		getGeometry(S32 te, 
							LLStrider<LLVector3>& verticesp, 
							LLStrider<LLVector3>& normalsp, 
							LLStrider<LLVector2>& texcoordsp, 
							LLStrider<LLColor4U>& colorsp, 
							LLStrider<U32>& indicesp);

	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.
	BOOL isParticle();
	F32 getPartSize(S32 idx);

	/*virtual*/ void updateTextures(LLAgent &agent);
	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent); // generate accurate apparent angle and area
	
	void updateFaceSize(S32 idx) { }
	BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);

	virtual U32 getPartitionType() const;

	void setCloudGroup(LLCloudGroup *cgp)		{ mCloudGroupp = cgp; }
protected:
	virtual ~LLVOClouds();

	LLCloudGroup *mCloudGroupp;
};

extern LLUUID gCloudTextureID;

#endif // LL_VO_CLOUDS_H
