/** 
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDRAWPOOLAVATAR_H
#define LL_LLDRAWPOOLAVATAR_H

#include "lldrawpool.h"

class LLVOAvatar;

class LLDrawPoolAvatar : public LLFacePool
{
protected:
	S32					mNumFaces;
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
							LLVertexBuffer::MAP_TEXCOORD |
							LLVertexBuffer::MAP_WEIGHT |
							LLVertexBuffer::MAP_CLOTHWEIGHT |
							LLVertexBuffer::MAP_BINORMAL
							
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
	/*virtual*/ void renderForSelect();

	void beginRigid();
	void beginFootShadow();
	void beginSkinned();
		
	void endRigid();
	void endFootShadow();
	void endSkinned();
		
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	virtual S32 getMaterialAttribIndex() { return 0; }

	void renderAvatars(LLVOAvatar *single_avatar, S32 pass = -1); // renders only one avatar if single_avatar is not null.
};



extern S32 AVATAR_OFFSET_POS;
extern S32 AVATAR_OFFSET_NORMAL;
extern S32 AVATAR_OFFSET_TEX0;
extern S32 AVATAR_OFFSET_TEX1;
extern S32 AVATAR_VERTEX_BYTES;
const S32 AVATAR_BUFFER_ELEMENTS = 8192; // Needs to be enough to store all avatar vertices.

extern BOOL gAvatarEmbossBumpMap;
#endif // LL_LLDRAWPOOLAVATAR_H
