/**
 * @file llmaterial.h
 * @brief Material definition
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLMATERIAL_H
#define LL_LLMATERIAL_H

#include "llmaterialid.h"
#include "llsd.h"
#include "v4coloru.h"

class LLMaterial
{
public:
	LLMaterial();
	LLMaterial(const LLSD& material_data);

	LLSD asLLSD() const;
	void fromLLSD(const LLSD& material_data);

	const LLUUID& getNormalID() const { return mNormalID; }
	void		setNormalID(const LLUUID& normal_id) { mNormalID = normal_id; }
	void		getNormalOffset(F32& offset_x, F32& offset_y) const { offset_x = mNormalOffsetX; offset_y = mNormalOffsetY; }
	void		setNormalOffset(F32 offset_x, F32 offset_y) { mNormalOffsetX = offset_x; mNormalOffsetY = offset_y; }
	void		getNormalRepeat(F32& repeat_x, F32& repeat_y) const { repeat_x = mNormalRepeatX; repeat_y = mNormalRepeatY; }
	void		setNormalRepeat(F32 repeat_x, F32 repeat_y) { mNormalRepeatX = repeat_x; mNormalRepeatY = repeat_y; }
	F32			getNormalRotation() const { return mNormalRotation; }
	void		setNormalRotation(F32 rot) { mNormalRotation = rot; }

	const LLUUID& getSpecularID() const { return mSpecularID; }
	void		setSpecularID(const LLUUID& specular_id)  { mSpecularID = specular_id; }
	void		getSpecularOffset(F32& offset_x, F32& offset_y) const { offset_x = mSpecularOffsetX; offset_y = mSpecularOffsetY; }
	void		setSpecularOffset(F32 offset_x, F32 offset_y) { mSpecularOffsetX = offset_x; mSpecularOffsetY = offset_y; }
	void		getSpecularRepeat(F32& repeat_x, F32& repeat_y) const { repeat_x = mSpecularRepeatX; repeat_y = mSpecularRepeatY; }
	void		setSpecularRepeat(F32 repeat_x, F32 repeat_y) { mSpecularRepeatX = repeat_x; mSpecularRepeatY = repeat_y; }
	F32			getSpecularRotation() const { return mSpecularRotation; }
	void		setSpecularRotation(F32 rot) { mSpecularRotation = rot; }

	const LLColor4U& getSpecularLightColor() const { return mSpecularLightColor; }
	void		setSpecularLightColor(const LLColor4U& color) { mSpecularLightColor = color; }
	U8			getSpecularLightExponent() const { return mSpecularLightExponent; }
	void		setSpecularLightExponent(U8 exponent) { mSpecularLightExponent = exponent; }
	U8			getEnvironmentIntensity() const { return mEnvironmentIntensity; }
	void		setEnvironmentIntensity(U8 intensity) { mEnvironmentIntensity = intensity; }
	U8			getDiffuseAlphaMode() const { return mDiffuseAlphaMode; }
	void		setDiffuseAlphaMode(U8 alpha_mode) { mDiffuseAlphaMode = alpha_mode; }
	U8			getAlphaMaskCutoff() const { return mAlphaMaskCutoff; }
	void		setAlphaMaskCutoff(U8 cutoff) { mAlphaMaskCutoff = cutoff; }

protected:
	LLUUID		mNormalID;
	F32			mNormalOffsetX;
	F32			mNormalOffsetY;
	F32			mNormalRepeatX;
	F32			mNormalRepeatY;
	F32			mNormalRotation;

	LLUUID		mSpecularID;
	F32			mSpecularOffsetX;
	F32			mSpecularOffsetY;
	F32			mSpecularRepeatX;
	F32			mSpecularRepeatY;
	F32			mSpecularRotation;

	LLColor4U	mSpecularLightColor;
	U8			mSpecularLightExponent;
	U8			mEnvironmentIntensity;
	U8			mDiffuseAlphaMode;
	U8			mAlphaMaskCutoff;
};

#endif // LL_LLMATERIAL_H
