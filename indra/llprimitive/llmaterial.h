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

#include <boost/shared_ptr.hpp>

#include "llmaterialid.h"
#include "llsd.h"
#include "v4coloru.h"
#include "llpointer.h"
#include "llrefcount.h"

class LLMaterial : public LLRefCount
{
public:

	typedef enum
	{
		DIFFUSE_ALPHA_MODE_NONE = 0,
		DIFFUSE_ALPHA_MODE_BLEND = 1,
		DIFFUSE_ALPHA_MODE_MASK = 2,
		DIFFUSE_ALPHA_MODE_EMISSIVE = 3,
		DIFFUSE_ALPHA_MODE_DEFAULT = 4,
	} eDiffuseAlphaMode;

	typedef enum
	{
		SHADER_COUNT = 16,
		ALPHA_SHADER_COUNT = 4
	} eShaderCount;

	
	
	static const U8			DEFAULT_SPECULAR_LIGHT_EXPONENT = ((U8)(0.2f * 255));
	static const LLColor4U	DEFAULT_SPECULAR_LIGHT_COLOR;
	static const U8			DEFAULT_ENV_INTENSITY = 0;

	LLMaterial();
	LLMaterial(const LLSD& material_data);

	LLSD asLLSD() const;
	void fromLLSD(const LLSD& material_data);

	const LLUUID& getNormalID() const { return mNormalID; }
	void		setNormalID(const LLUUID& normal_id) { mNormalID = normal_id; }
	void		getNormalOffset(F32& offset_x, F32& offset_y) const { offset_x = mNormalOffsetX; offset_y = mNormalOffsetY; }
	F32		getNormalOffsetX() const { return mNormalOffsetX; }
	F32		getNormalOffsetY() const { return mNormalOffsetY; }

	void		setNormalOffset(F32 offset_x, F32 offset_y) { mNormalOffsetX = offset_x; mNormalOffsetY = offset_y; }
	void		setNormalOffsetX(F32 offset_x) { mNormalOffsetX = offset_x; }
	void		setNormalOffsetY(F32 offset_y) { mNormalOffsetY = offset_y; }

	void		getNormalRepeat(F32& repeat_x, F32& repeat_y) const { repeat_x = mNormalRepeatX; repeat_y = mNormalRepeatY; }
	F32		getNormalRepeatX() const { return mNormalRepeatX; }
	F32		getNormalRepeatY() const { return mNormalRepeatY; }

	void		setNormalRepeat(F32 repeat_x, F32 repeat_y) { mNormalRepeatX = repeat_x; mNormalRepeatY = repeat_y; }
	void		setNormalRepeatX(F32 repeat_x) { mNormalRepeatX = repeat_x; }
	void		setNormalRepeatY(F32 repeat_y) { mNormalRepeatY = repeat_y; }

	F32		getNormalRotation() const { return mNormalRotation; }
	void		setNormalRotation(F32 rot) { mNormalRotation = rot; }

	const LLUUID& getSpecularID() const { return mSpecularID; }
	void		setSpecularID(const LLUUID& specular_id)  { mSpecularID = specular_id; }
	void		getSpecularOffset(F32& offset_x, F32& offset_y) const { offset_x = mSpecularOffsetX; offset_y = mSpecularOffsetY; }
	F32		getSpecularOffsetX() const { return mSpecularOffsetX; }
	F32		getSpecularOffsetY() const { return mSpecularOffsetY; }

	void		setSpecularOffset(F32 offset_x, F32 offset_y) { mSpecularOffsetX = offset_x; mSpecularOffsetY = offset_y; }
	void		setSpecularOffsetX(F32 offset_x) { mSpecularOffsetX = offset_x; }
	void		setSpecularOffsetY(F32 offset_y) { mSpecularOffsetY = offset_y; }

	void		getSpecularRepeat(F32& repeat_x, F32& repeat_y) const { repeat_x = mSpecularRepeatX; repeat_y = mSpecularRepeatY; }
	F32		getSpecularRepeatX() const { return mSpecularRepeatX; }
	F32		getSpecularRepeatY() const { return mSpecularRepeatY; }

	void		setSpecularRepeat(F32 repeat_x, F32 repeat_y) { mSpecularRepeatX = repeat_x; mSpecularRepeatY = repeat_y; }
	void		setSpecularRepeatX(F32 repeat_x) { mSpecularRepeatX = repeat_x; }
	void		setSpecularRepeatY(F32 repeat_y) { mSpecularRepeatY = repeat_y; }

	F32		getSpecularRotation() const { return mSpecularRotation; }
	void		setSpecularRotation(F32 rot) { mSpecularRotation = rot; }

	const LLColor4U getSpecularLightColor() const { return mSpecularLightColor; }
	void		setSpecularLightColor(const LLColor4U& color) { mSpecularLightColor = color; }
	U8			getSpecularLightExponent() const { return mSpecularLightExponent; }
	void		setSpecularLightExponent(U8 exponent) { mSpecularLightExponent = exponent; }
	U8			getEnvironmentIntensity() const { return mEnvironmentIntensity; }
	void		setEnvironmentIntensity(U8 intensity) { mEnvironmentIntensity = intensity; }
	U8			getDiffuseAlphaMode() const { return mDiffuseAlphaMode; }
	void		setDiffuseAlphaMode(U8 alpha_mode) { mDiffuseAlphaMode = alpha_mode; }
	U8			getAlphaMaskCutoff() const { return mAlphaMaskCutoff; }
	void		setAlphaMaskCutoff(U8 cutoff) { mAlphaMaskCutoff = cutoff; }

	bool		isNull() const;
	static const LLMaterial null;

	bool		operator == (const LLMaterial& rhs) const;
	bool		operator != (const LLMaterial& rhs) const;

	U32			getShaderMask(U32 alpha_mode = DIFFUSE_ALPHA_MODE_DEFAULT);

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

typedef LLPointer<LLMaterial> LLMaterialPtr;

#endif // LL_LLMATERIAL_H

