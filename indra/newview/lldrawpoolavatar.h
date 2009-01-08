/** 
 * @file lldrawpoolavatar.h
 * @brief LLDrawPoolAvatar class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
	/*virtual*/ void renderForSelect();

	void beginRigid();
	void beginFootShadow();
	void beginSkinned();
		
	void endRigid();
	void endFootShadow();
	void endSkinned();
		
	/*virtual*/ LLViewerImage *getDebugTexture();
	/*virtual*/ LLColor3 getDebugColor() const; // For AGP debug display

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
