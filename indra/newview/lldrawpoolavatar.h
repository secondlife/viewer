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

class LLDrawPoolAvatar : public LLDrawPool
{
protected:
	S32					mNumFaces;
public:
	enum
	{
		SHADER_LEVEL_BUMP = 2,
		SHADER_LEVEL_CLOTH = 3
	};
	
	LLDrawPoolAvatar();

	/*virtual*/ LLDrawPool *instancePool();

	/*virtual*/ void prerender();
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderForSelect();
	/*virtual*/ S32 rebuild();

	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

	virtual S32 getMaterialAttribIndex() { return 0; }

	void renderAvatars(LLVOAvatar *single_avatar, BOOL no_shaders = FALSE); // renders only one avatar if single_avatar is not null.
};



extern S32 AVATAR_OFFSET_POS;
extern S32 AVATAR_OFFSET_NORMAL;
extern S32 AVATAR_OFFSET_TEX0;
extern S32 AVATAR_OFFSET_TEX1;
extern S32 AVATAR_VERTEX_BYTES;
const S32 AVATAR_BUFFER_ELEMENTS = 8192; // Needs to be enough to store all avatar vertices.

extern BOOL gAvatarEmbossBumpMap;
#endif // LL_LLDRAWPOOLAVATAR_H
