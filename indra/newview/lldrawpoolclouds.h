/** 
 * @file lldrawpoolclouds.h
 * @brief LLDrawPoolClouds class definition
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

#ifndef LL_LLDRAWPOOLCLOUDS_H
#define LL_LLDRAWPOOLCLOUDS_H

#include "lldrawpool.h"

class LLDrawPoolClouds : public LLDrawPool
{
public:
	enum
	{
		VERTEX_DATA_MASK =	LLVertexBuffer::MAP_VERTEX |
							LLVertexBuffer::MAP_NORMAL |
							LLVertexBuffer::MAP_TEXCOORD
	};

	BOOL addFace(LLFace* face);
	virtual U32 getVertexDataMask() { return VERTEX_DATA_MASK; }

	LLDrawPoolClouds();

	/*virtual*/ void prerender();
	/*virtual*/ LLDrawPool *instancePool();
	/*virtual*/ void enqueue(LLFace *face);
	/*virtual*/ void beginRenderPass(S32 pass);
	/*virtual*/ void render(S32 pass = 0);
	/*virtual*/ void renderForSelect();
	virtual S32 getMaterialAttribIndex() { return 0; }
};

#endif // LL_LLDRAWPOOLSKY_H
