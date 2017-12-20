/**
 * @file llmaterial.cpp
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

#include "linden_common.h"

#include "llmaterial.h"

/**
 * Materials cap parameters
 */
#define MATERIALS_CAP_NORMAL_MAP_FIELD            "NormMap"
#define MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD   "NormOffsetX"
#define MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD   "NormOffsetY"
#define MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD   "NormRepeatX"
#define MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD   "NormRepeatY"
#define MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD   "NormRotation"

#define MATERIALS_CAP_SPECULAR_MAP_FIELD          "SpecMap"
#define MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD "SpecOffsetX"
#define MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD "SpecOffsetY"
#define MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD "SpecRepeatX"
#define MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD "SpecRepeatY"
#define MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD "SpecRotation"

#define MATERIALS_CAP_SPECULAR_COLOR_FIELD        "SpecColor"
#define MATERIALS_CAP_SPECULAR_EXP_FIELD          "SpecExp"
#define MATERIALS_CAP_ENV_INTENSITY_FIELD         "EnvIntensity"
#define MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD     "AlphaMaskCutoff"
#define MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD    "DiffuseAlphaMode"

const LLColor4U LLMaterial::DEFAULT_SPECULAR_LIGHT_COLOR(255,255,255,255);

/**
 * Materials constants
 */

const F32 MATERIALS_MULTIPLIER                   = 10000.f;

/**
 * Helper functions
 */

template<typename T> T getMaterialField(const LLSD& data, const std::string& field, const LLSD::Type field_type)
{
	if ( (data.has(field)) && (field_type == data[field].type()) )
	{
		return (T)data[field];
	}
	LL_ERRS() << "Missing or mistyped field '" << field << "' in material definition" << LL_ENDL;
	return (T)LLSD();
}

// GCC didn't like the generic form above for some reason
template<> LLUUID getMaterialField(const LLSD& data, const std::string& field, const LLSD::Type field_type)
{
	if ( (data.has(field)) && (field_type == data[field].type()) )
	{
		return data[field].asUUID();
	}
	LL_ERRS() << "Missing or mistyped field '" << field << "' in material definition" << LL_ENDL;
	return LLUUID::null;
}

/**
 * LLMaterial class
 */

const LLMaterial LLMaterial::null;

LLMaterial::LLMaterial()
	: mNormalOffsetX(0.0f)
	, mNormalOffsetY(0.0f)
	, mNormalRepeatX(1.0f)
	, mNormalRepeatY(1.0f)
	, mNormalRotation(0.0f)
	, mSpecularOffsetX(0.0f)
	, mSpecularOffsetY(0.0f)
	, mSpecularRepeatX(1.0f)
	, mSpecularRepeatY(1.0f)
	, mSpecularRotation(0.0f)
	, mSpecularLightColor(LLMaterial::DEFAULT_SPECULAR_LIGHT_COLOR)
	, mSpecularLightExponent(LLMaterial::DEFAULT_SPECULAR_LIGHT_EXPONENT)
	, mEnvironmentIntensity(LLMaterial::DEFAULT_ENV_INTENSITY)
	, mDiffuseAlphaMode(LLMaterial::DIFFUSE_ALPHA_MODE_BLEND)
	, mAlphaMaskCutoff(0)
{
}

LLMaterial::LLMaterial(const LLSD& material_data)
{
	fromLLSD(material_data);
}

LLSD LLMaterial::asLLSD() const
{
	LLSD material_data;

	material_data[MATERIALS_CAP_NORMAL_MAP_FIELD] = mNormalID;
	material_data[MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD] = ll_round(mNormalOffsetX * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD] = ll_round(mNormalOffsetY * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD] = ll_round(mNormalRepeatX * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD] = ll_round(mNormalRepeatY * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD] = ll_round(mNormalRotation * MATERIALS_MULTIPLIER);

	material_data[MATERIALS_CAP_SPECULAR_MAP_FIELD] = mSpecularID;
	material_data[MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD] = ll_round(mSpecularOffsetX * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD] = ll_round(mSpecularOffsetY * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD] = ll_round(mSpecularRepeatX * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD] = ll_round(mSpecularRepeatY * MATERIALS_MULTIPLIER);
	material_data[MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD] = ll_round(mSpecularRotation * MATERIALS_MULTIPLIER);

	material_data[MATERIALS_CAP_SPECULAR_COLOR_FIELD]     = mSpecularLightColor.getValue();
	material_data[MATERIALS_CAP_SPECULAR_EXP_FIELD]       = mSpecularLightExponent;
	material_data[MATERIALS_CAP_ENV_INTENSITY_FIELD]      = mEnvironmentIntensity;
	material_data[MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD] = mDiffuseAlphaMode;
	material_data[MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD]  = mAlphaMaskCutoff;

	return material_data;
}

void LLMaterial::fromLLSD(const LLSD& material_data)
{
	mNormalID = getMaterialField<LLSD::UUID>(material_data, MATERIALS_CAP_NORMAL_MAP_FIELD, LLSD::TypeUUID);
	mNormalOffsetX  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_NORMAL_MAP_OFFSET_X_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mNormalOffsetY  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_NORMAL_MAP_OFFSET_Y_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mNormalRepeatX  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_NORMAL_MAP_REPEAT_X_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mNormalRepeatY  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_NORMAL_MAP_REPEAT_Y_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mNormalRotation = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_NORMAL_MAP_ROTATION_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;

	mSpecularID = getMaterialField<LLSD::UUID>(material_data, MATERIALS_CAP_SPECULAR_MAP_FIELD, LLSD::TypeUUID);
	mSpecularOffsetX  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_SPECULAR_MAP_OFFSET_X_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mSpecularOffsetY  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_SPECULAR_MAP_OFFSET_Y_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mSpecularRepeatX  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_SPECULAR_MAP_REPEAT_X_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mSpecularRepeatY  = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_SPECULAR_MAP_REPEAT_Y_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;
	mSpecularRotation = (F32)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_SPECULAR_MAP_ROTATION_FIELD, LLSD::TypeInteger) / MATERIALS_MULTIPLIER;

	mSpecularLightColor.setValue(getMaterialField<LLSD>(material_data, MATERIALS_CAP_SPECULAR_COLOR_FIELD, LLSD::TypeArray));
	mSpecularLightExponent = (U8)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_SPECULAR_EXP_FIELD,       LLSD::TypeInteger);
	mEnvironmentIntensity  = (U8)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_ENV_INTENSITY_FIELD,      LLSD::TypeInteger);
	mDiffuseAlphaMode      = (U8)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_DIFFUSE_ALPHA_MODE_FIELD, LLSD::TypeInteger);
	mAlphaMaskCutoff       = (U8)getMaterialField<LLSD::Integer>(material_data, MATERIALS_CAP_ALPHA_MASK_CUTOFF_FIELD,  LLSD::TypeInteger);
}

bool LLMaterial::isNull() const
{
	return (*this == null);
}

bool LLMaterial::operator == (const LLMaterial& rhs) const
{
	return 
		(mNormalID == rhs.mNormalID) && (mNormalOffsetX == rhs.mNormalOffsetX) && (mNormalOffsetY == rhs.mNormalOffsetY) &&
		(mNormalRepeatX == rhs.mNormalRepeatX) && (mNormalRepeatY == rhs.mNormalRepeatY) && (mNormalRotation == rhs.mNormalRotation) &&
		(mSpecularID == rhs.mSpecularID) && (mSpecularOffsetX == rhs.mSpecularOffsetX) && (mSpecularOffsetY == rhs.mSpecularOffsetY) &&
		(mSpecularRepeatX == rhs.mSpecularRepeatX) && (mSpecularRepeatY == rhs.mSpecularRepeatY) && (mSpecularRotation == rhs.mSpecularRotation) &&
		(mSpecularLightColor == rhs.mSpecularLightColor) && (mSpecularLightExponent == rhs.mSpecularLightExponent) &&
		(mEnvironmentIntensity == rhs.mEnvironmentIntensity) && (mDiffuseAlphaMode == rhs.mDiffuseAlphaMode) && (mAlphaMaskCutoff == rhs.mAlphaMaskCutoff);
}

bool LLMaterial::operator != (const LLMaterial& rhs) const
{
	return !(*this == rhs);
}


U32 LLMaterial::getShaderMask(U32 alpha_mode)
{ //NEVER incorporate this value into the message system -- this function will vary depending on viewer implementation
	U32 ret = 0;

	//two least significant bits are "diffuse alpha mode"
	if (alpha_mode != DIFFUSE_ALPHA_MODE_DEFAULT)
	{
		ret = alpha_mode;
	}
	else
	{
		ret = getDiffuseAlphaMode();
	}

	llassert(ret < SHADER_COUNT);

	//next bit is whether or not specular map is present
	const U32 SPEC_BIT = 0x4;

	if (getSpecularID().notNull())
	{
		ret |= SPEC_BIT;
	}

	llassert(ret < SHADER_COUNT);
	
	//next bit is whether or not normal map is present
	const U32 NORM_BIT = 0x8;
	if (getNormalID().notNull())
	{
		ret |= NORM_BIT;
	}

	llassert(ret < SHADER_COUNT);

	return ret;
}


