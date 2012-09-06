/**
 * @file llavatarappearance.h
 * @brief Declaration of LLAvatarAppearance class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_AVATAR_APPEARANCE_H
#define LL_AVATAR_APPEARANCE_H

#include "llcharacter.h"
//#include "llframetimer.h"
#include "llavatarappearancedefines.h"
#include "lljoint.h"

class LLTexLayerSet;
class LLTexGlobalColor;
class LLWearableData;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLAvatarAppearance
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLAvatarAppearance : public LLCharacter
{
	LOG_CLASS(LLAvatarAppearance);

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/
private:
	// Hide default constructor.
	LLAvatarAppearance() {}

public:
	LLAvatarAppearance(LLWearableData* wearable_data);
	virtual ~LLAvatarAppearance();

/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/
public:
	virtual bool 	isSelf() const { return false; } // True if this avatar is for this viewer's agent
	virtual BOOL	isValid() const;
	virtual BOOL	isUsingBakedTextures() const = 0;
	
/**                    State
 **                                                                            **
 *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    RENDERING
 **/
	BOOL		mIsDummy; // for special views

	//--------------------------------------------------------------------
	// Morph masks
	//--------------------------------------------------------------------
public:
	void 	addMaskedMorph(LLAvatarAppearanceDefines::EBakedTextureIndex index, LLVisualParam* morph_target, BOOL invert, std::string layer);
	virtual void	applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index = LLAvatarAppearanceDefines::BAKED_NUM_INDICES) = 0;

/**                    Rendering
 **                                                                            **
 *******************************************************************************/

	//--------------------------------------------------------------------
	// Composites
	//--------------------------------------------------------------------
public:
	virtual void	invalidateComposite(LLTexLayerSet* layerset, BOOL upload_result) = 0;

/********************************************************************************
 **                                                                            **
 **                    MESHES
 **/

public:
	virtual void	updateMeshTextures() = 0;
	virtual void	dirtyMesh() = 0; // Dirty the avatar mesh
protected:
	virtual void	dirtyMesh(S32 priority) = 0; // Dirty the avatar mesh, with priority

/**                    Meshes
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    APPEARANCE
 **/

	//--------------------------------------------------------------------
	// Clothing colors (convenience functions to access visual parameters)
	//--------------------------------------------------------------------
public:
	void			setClothesColor(LLAvatarAppearanceDefines::ETextureIndex te, const LLColor4& new_color, BOOL upload_bake);
	LLColor4		getClothesColor(LLAvatarAppearanceDefines::ETextureIndex te);
	static BOOL		teToColorParams(LLAvatarAppearanceDefines::ETextureIndex te, U32 *param_name);

	//--------------------------------------------------------------------
	// Global colors
	//--------------------------------------------------------------------
public:
	LLColor4		getGlobalColor(const std::string& color_name ) const;
	virtual void	onGlobalColorChanged(const LLTexGlobalColor* global_color, BOOL upload_bake) = 0;
protected:
	LLTexGlobalColor* mTexSkinColor;
	LLTexGlobalColor* mTexHairColor;
	LLTexGlobalColor* mTexEyeColor;

	//--------------------------------------------------------------------
	// Visibility
	//--------------------------------------------------------------------
public:
	static LLColor4 getDummyColor();
/**                    Appearance
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    WEARABLES
 **/

public:
	LLWearableData*			getWearableData() { return mWearableData; }
	const LLWearableData*	getWearableData() const { return mWearableData; }
	virtual BOOL isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te, U32 index = 0 ) const = 0;
	virtual BOOL			isWearingWearableType(LLWearableType::EType type ) const;

private:
	LLWearableData* mWearableData;

/********************************************************************************
 **                                                                            **
 **                    BAKED TEXTURES
 **/
protected:
	struct LLMaskedMorph;
	typedef std::deque<LLMaskedMorph *> 	morph_list_t;
	struct BakedTextureData
	{
		LLUUID								mLastTextureIndex;
		LLTexLayerSet*		 				mTexLayerSet; // Only exists for self
		bool								mIsLoaded;
		bool								mIsUsed;
		LLAvatarAppearanceDefines::ETextureIndex 	mTextureIndex;
		U32									mMaskTexName;
		// Stores pointers to the joint meshes that this baked texture deals with
		std::vector< LLJoint* > 	mMeshes;  // std::vector<LLViewerJointMesh> mJoints[i]->mMeshParts
		morph_list_t						mMaskedMorphs;
	};
	typedef std::vector<BakedTextureData> 	bakedtexturedata_vec_t;
	bakedtexturedata_vec_t 					mBakedTextureDatas;


/********************************************************************************
 **                                                                            **
 **                    SUPPORT CLASSES
 **/

	class LLMaskedMorph
	{
	public:
		LLMaskedMorph(LLVisualParam *morph_target, BOOL invert, std::string layer);

		LLVisualParam	*mMorphTarget;
		BOOL				mInvert;
		std::string			mLayer;
	};

};

#endif // LL_AVATAR_APPEARANCE_H
