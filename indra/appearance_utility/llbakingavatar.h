/**
 * @file llbakingavatar.h
 * @brief Declaration of LLBakingAvatar class which is a derivation of LLAvatarAppearance
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_LLBAKINGAVATAR_H
#define LL_LLBAKINGAVATAR_H

#include "llavatarappearance.h"

class LLBakingAvatar : public LLAvatarAppearance
{
	LOG_CLASS(LLBakingAvatar);

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/
public:
	void* operator new(size_t size)
	{
		return ll_aligned_malloc_16(size);
	}

	void operator delete(void* ptr)
	{
		ll_aligned_free_16(ptr);
	}

	LLBakingAvatar(LLWearableData* wearable_data);
	virtual ~LLBakingAvatar();

	static void initClass(); // initializes static members

/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    INHERITED
 **/

	//--------------------------------------------------------------------
	// LLCharacter interface
	//--------------------------------------------------------------------
public:
	/*virtual*/ LLVector3    	getCharacterPosition();
	/*virtual*/ LLQuaternion 	getCharacterRotation();
	/*virtual*/ LLVector3    	getCharacterVelocity();
	/*virtual*/ LLVector3    	getCharacterAngularVelocity();

	/*virtual*/ const LLUUID&	getID() const;
	/*virtual*/ void			addDebugText(const std::string& text);
	/*virtual*/ F32				getTimeDilation();
	/*virtual*/ void			getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm);
	/*virtual*/ F32				getPixelArea() const;
	/*virtual*/ LLVector3d		getPosGlobalFromAgent(const LLVector3 &position);
	/*virtual*/ LLVector3		getPosAgentFromGlobal(const LLVector3d &position);

	//--------------------------------------------------------------------
	// LLAvatarAppearance interface
	//--------------------------------------------------------------------
public:
	/*virtual*/ void	bodySizeChanged();
	/*virtual*/ void	applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components,
							LLAvatarAppearanceDefines::EBakedTextureIndex index);
	/*virtual*/ void	invalidateComposite(LLTexLayerSet* layerset, BOOL upload_result);
	/*virtual*/ void 	updateMeshTextures();
	/*virtual*/ void	dirtyMesh(); // Dirty the avatar mesh
	/*virtual*/ void	onGlobalColorChanged(const LLTexGlobalColor* global_color, BOOL upload_bake);
	/*virtual*/ BOOL	isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex type, U32 index = 0) const;
private:
	/*virtual*/ void	dirtyMesh(S32 priority); // Dirty the avatar mesh, with priority

	// LLAvatarAppearance instance factories:
protected:
	/*virtual*/ LLAvatarJoint*	createAvatarJoint();
	/*virtual*/ LLAvatarJoint*	createAvatarJoint(S32 joint_num);
	/*virtual*/ LLAvatarJointMesh*	createAvatarJointMesh();
	/*virtual*/ LLTexLayerSet*	createTexLayerSet();


/**                    Inherited
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/
public:
	/*virtual*/ bool 	isSelf() const { return true; }
	/*virtual*/ BOOL	isValid() const { return TRUE; }
	/*virtual*/ BOOL	isUsingBakedTextures() const { return TRUE; }

/**                    State
 **                                                                            **
 *******************************************************************************/

};

#endif /* LL_LLBAKINGAVATAR_H */

