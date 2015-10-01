 /** 
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLDRAWPOOLAVATAR_H
#define LL_LLDRAWPOOLAVATAR_H

#include "lldrawpool.h"

class LLVOAvatar;
class LLGLSLShader;
class LLFace;
class LLMeshSkinInfo;
class LLVolume;
class LLVolumeFace;


class LLDrawPoolAvatar : public LLFacePool
{
public:
	enum
	{
		SHADER_LEVEL_BUMP = 2,
		SHADER_LEVEL_CLOTH = 3
	};
	
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD0 |
							LLVertexBuffer::MAP_WEIGHT |
							LLVertexBuffer::MAP_CLOTHWEIGHT
	};

	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	virtual S32 getVertexShaderLevel() const;

	LLDrawPoolAvatar();

	static LLMatrix4& getModelView();

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ S32  getNumPasses();
	/*virtual*/ void beginRenderPass(S32 pass);
	/*virtual*/ void endRenderPass(S32 pass);
	/*virtual*/ void prerender();
	/*virtual*/ void render(S32 pass = 0);

	/*virtual*/ S32 getNumDeferredPasses();
	/*virtual*/ void beginDeferredPass(S32 pass);
	/*virtual*/ void endDeferredPass(S32 pass);
	/*virtual*/ void renderDeferred(S32 pass);
	
	/*virtual*/ S32 getNumPostDeferredPasses();
	/*virtual*/ void beginPostDeferredPass(S32 pass);
	/*virtual*/ void endPostDeferredPass(S32 pass);
	/*virtual*/ void renderPostDeferred(S32 pass);

	/*virtual*/ S32 getNumShadowPasses();
	/*virtual*/ void beginShadowPass(S32 pass);
	/*virtual*/ void endShadowPass(S32 pass);
	/*virtual*/ void renderShadow(S32 pass);

	void beginRigid();
	void beginImpostor();
	void beginSkinned();
	
	void endRigid();
	void endImpostor();
	void endSkinned();

	void beginDeferredImpostor();
	void beginDeferredRigid();
	void beginDeferredSkinned();
	
	void endDeferredImpostor();
	void endDeferredRigid();
	void endDeferredSkinned();
	
	void beginPostDeferredAlpha();
	void endPostDeferredAlpha();

	void beginRiggedSimple();
	void beginRiggedFullbright();
	void beginRiggedFullbrightShiny();
	void beginRiggedShinySimple();
	void beginRiggedAlpha();
	void beginRiggedFullbrightAlpha();
	void beginRiggedGlow();
	void beginDeferredRiggedAlpha();
	void beginDeferredRiggedMaterial(S32 pass);
	void beginDeferredRiggedMaterialAlpha(S32 pass);

	void endRiggedSimple();
	void endRiggedFullbright();
	void endRiggedFullbrightShiny();
	void endRiggedShinySimple();
	void endRiggedAlpha();
	void endRiggedFullbrightAlpha();
	void endRiggedGlow();
	void endDeferredRiggedAlpha();
	void endDeferredRiggedMaterial(S32 pass);
	void endDeferredRiggedMaterialAlpha(S32 pass);

	void beginDeferredRiggedSimple();
	void beginDeferredRiggedBump();
	
	void endDeferredRiggedSimple();
	void endDeferredRiggedBump();
		
	void getRiggedGeometry(LLFace* face, LLPointer<LLVertexBuffer>& buffer, U32 data_mask, const LLMeshSkinInfo* skin, LLVolume* volume, const LLVolumeFace& vol_face);
    static void initSkinningMatrixPalette(LLMatrix4* mat, S32 count, const LLMeshSkinInfo* skin, LLVOAvatar *avatar);
    static void getPerVertexSkinMatrix(F32* weights, LLMatrix4a* mat, bool handle_bad_scale, LLMatrix4a& final_mat);
	void updateRiggedFaceVertexBuffer(LLVOAvatar* avatar,
									  LLFace* facep, 
									  const LLMeshSkinInfo* skin, 
									  LLVolume* volume,
									  const LLVolumeFace& vol_face);
	void updateRiggedVertexBuffers(LLVOAvatar* avatar);

	void renderRigged(LLVOAvatar* avatar, U32 type, bool glow = false);
	void renderRiggedSimple(LLVOAvatar* avatar);
	void renderRiggedAlpha(LLVOAvatar* avatar);
	void renderRiggedFullbrightAlpha(LLVOAvatar* avatar);
	void renderRiggedFullbright(LLVOAvatar* avatar);
	void renderRiggedShinySimple(LLVOAvatar* avatar);
	void renderRiggedFullbrightShiny(LLVOAvatar* avatar);
	void renderRiggedGlow(LLVOAvatar* avatar);
	void renderDeferredRiggedSimple(LLVOAvatar* avatar);
	void renderDeferredRiggedBump(LLVOAvatar* avatar);
	void renderDeferredRiggedMaterial(LLVOAvatar* avatar, S32 pass);
	
	typedef enum
	{
		RIGGED_MATERIAL=0,
		RIGGED_MATERIAL_ALPHA,
		RIGGED_MATERIAL_ALPHA_MASK,
		RIGGED_MATERIAL_ALPHA_EMISSIVE,
		RIGGED_SPECMAP,
		RIGGED_SPECMAP_BLEND,
		RIGGED_SPECMAP_MASK,
		RIGGED_SPECMAP_EMISSIVE,
		RIGGED_NORMMAP,
		RIGGED_NORMMAP_BLEND,
		RIGGED_NORMMAP_MASK,
		RIGGED_NORMMAP_EMISSIVE,
		RIGGED_NORMSPEC,
		RIGGED_NORMSPEC_BLEND,
		RIGGED_NORMSPEC_MASK,
		RIGGED_NORMSPEC_EMISSIVE,
		RIGGED_SIMPLE,
		RIGGED_FULLBRIGHT,
		RIGGED_SHINY,
		RIGGED_FULLBRIGHT_SHINY,
		RIGGED_GLOW,
		RIGGED_ALPHA,
		RIGGED_FULLBRIGHT_ALPHA,
		RIGGED_DEFERRED_BUMP,
		RIGGED_DEFERRED_SIMPLE,
		NUM_RIGGED_PASSES,
		RIGGED_UNKNOWN,
	} eRiggedPass;

	typedef enum
	{
		RIGGED_MATERIAL_MASK =
						LLVertexBuffer::MAP_VERTEX | 
						LLVertexBuffer::MAP_NORMAL | 
						LLVertexBuffer::MAP_TEXCOORD0 |
						LLVertexBuffer::MAP_COLOR |
						LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_MATERIAL_ALPHA_VMASK = RIGGED_MATERIAL_MASK,
		RIGGED_MATERIAL_ALPHA_MASK_MASK = RIGGED_MATERIAL_MASK,
		RIGGED_MATERIAL_ALPHA_EMISSIVE_MASK = RIGGED_MATERIAL_MASK,
		RIGGED_SPECMAP_VMASK =
						LLVertexBuffer::MAP_VERTEX | 
						LLVertexBuffer::MAP_NORMAL | 
						LLVertexBuffer::MAP_TEXCOORD0 |
						LLVertexBuffer::MAP_TEXCOORD2 |
						LLVertexBuffer::MAP_COLOR |
						LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_SPECMAP_BLEND_MASK = RIGGED_SPECMAP_VMASK,
		RIGGED_SPECMAP_MASK_MASK = RIGGED_SPECMAP_VMASK,
		RIGGED_SPECMAP_EMISSIVE_MASK = RIGGED_SPECMAP_VMASK,
		RIGGED_NORMMAP_VMASK =
						LLVertexBuffer::MAP_VERTEX | 
						LLVertexBuffer::MAP_NORMAL | 
						LLVertexBuffer::MAP_TANGENT | 
						LLVertexBuffer::MAP_TEXCOORD0 |
						LLVertexBuffer::MAP_TEXCOORD1 |
						LLVertexBuffer::MAP_COLOR |
						LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_NORMMAP_BLEND_MASK = RIGGED_NORMMAP_VMASK,
		RIGGED_NORMMAP_MASK_MASK = RIGGED_NORMMAP_VMASK,
		RIGGED_NORMMAP_EMISSIVE_MASK = RIGGED_NORMMAP_VMASK,
		RIGGED_NORMSPEC_VMASK =
						LLVertexBuffer::MAP_VERTEX | 
						LLVertexBuffer::MAP_NORMAL | 
						LLVertexBuffer::MAP_TANGENT | 
						LLVertexBuffer::MAP_TEXCOORD0 |
						LLVertexBuffer::MAP_TEXCOORD1 |
						LLVertexBuffer::MAP_TEXCOORD2 |
						LLVertexBuffer::MAP_COLOR |
						LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_NORMSPEC_BLEND_MASK = RIGGED_NORMSPEC_VMASK,
		RIGGED_NORMSPEC_MASK_MASK = RIGGED_NORMSPEC_VMASK,
		RIGGED_NORMSPEC_EMISSIVE_MASK = RIGGED_NORMSPEC_VMASK,
		RIGGED_SIMPLE_MASK = LLVertexBuffer::MAP_VERTEX | 
							 LLVertexBuffer::MAP_NORMAL | 
							 LLVertexBuffer::MAP_TEXCOORD0 |
							 LLVertexBuffer::MAP_COLOR |
							 LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_FULLBRIGHT_MASK = LLVertexBuffer::MAP_VERTEX | 
							 LLVertexBuffer::MAP_TEXCOORD0 |
							 LLVertexBuffer::MAP_COLOR |
							 LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_SHINY_MASK = RIGGED_SIMPLE_MASK,
		RIGGED_FULLBRIGHT_SHINY_MASK = RIGGED_SIMPLE_MASK,							 
		RIGGED_GLOW_MASK = LLVertexBuffer::MAP_VERTEX | 
							 LLVertexBuffer::MAP_TEXCOORD0 |
							 LLVertexBuffer::MAP_EMISSIVE |
							 LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_ALPHA_MASK = RIGGED_SIMPLE_MASK,
		RIGGED_FULLBRIGHT_ALPHA_MASK = RIGGED_FULLBRIGHT_MASK,
		RIGGED_DEFERRED_BUMP_MASK = LLVertexBuffer::MAP_VERTEX | 
							 LLVertexBuffer::MAP_NORMAL | 
							 LLVertexBuffer::MAP_TEXCOORD0 |
							 LLVertexBuffer::MAP_TANGENT |
							 LLVertexBuffer::MAP_COLOR |
							 LLVertexBuffer::MAP_WEIGHT4,
		RIGGED_DEFERRED_SIMPLE_MASK = LLVertexBuffer::MAP_VERTEX | 
							 LLVertexBuffer::MAP_NORMAL | 
							 LLVertexBuffer::MAP_TEXCOORD0 |
							 LLVertexBuffer::MAP_COLOR |
							 LLVertexBuffer::MAP_WEIGHT4,
	} eRiggedDataMask;

	void addRiggedFace(LLFace* facep, U32 type);
	void removeRiggedFace(LLFace* facep); 

	std::vector<LLFace*> mRiggedFace[NUM_RIGGED_PASSES];

	/*virtual*/ LLViewerTexture *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	void renderAvatars(LLVOAvatar *single_avatar, S32 pass = -1); // renders only one avatar if single_avatar is not null.


	static BOOL sSkipOpaque;
	static BOOL sSkipTransparent;
	static S32 sDiffuseChannel;
	static F32 sMinimumAlpha;

	static LLGLSLShader* sVertexProgram;
};

class LLVertexBufferAvatar : public LLVertexBuffer
{
public:
	LLVertexBufferAvatar();
};

extern S32 AVATAR_OFFSET_POS;
extern S32 AVATAR_OFFSET_NORMAL;
extern S32 AVATAR_OFFSET_TEX0;
extern S32 AVATAR_OFFSET_TEX1;
extern S32 AVATAR_VERTEX_BYTES;
const S32 AVATAR_BUFFER_ELEMENTS = 8192; // Needs to be enough to store all avatar vertices.

extern BOOL gAvatarEmbossBumpMap;
#endif // LL_LLDRAWPOOLAVATAR_H
