/** 
 * @file lldrawpooltree.cpp
 * @brief LLDrawPoolTree class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpooltree.h"

#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewerwindow.h"
#include "llvotree.h"
#include "pipeline.h"
#include "llviewercamera.h"
#include "llglslshader.h"

S32 LLDrawPoolTree::sDiffTex = 0;

LLDrawPoolTree::LLDrawPoolTree(LLViewerImage *texturep) :
	LLFacePool(POOL_TREE),
	mTexturep(texturep)
{
	mTexturep->bind(0);
	mTexturep->setClamp(FALSE, FALSE);
}

LLDrawPool *LLDrawPoolTree::instancePool()
{
	return new LLDrawPoolTree(mTexturep);
}

void LLDrawPoolTree::prerender()
{
	mVertexShaderLevel = 0;
}

void LLDrawPoolTree::beginRenderPass(S32 pass)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glAlphaFunc(GL_GREATER, 0.5f);
}

void LLDrawPoolTree::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_TREES);

	if (mDrawFace.empty())
	{
		return;
	}

	gPipeline.enableLightsDynamic(1.f);
	LLGLSPipelineAlpha gls_pipeline_alpha;
	LLOverrideFaceColor color(this, 1.f, 1.f, 1.f, 1.f);

	renderTree();
}

void LLDrawPoolTree::endRenderPass(S32 pass)
{
	glAlphaFunc(GL_GREATER, 0.01f);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolTree::renderForSelect()
{
	if (mDrawFace.empty())
	{
		return;
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	
	LLOverrideFaceColor color(this, 1.f, 1.f, 1.f, 1.f);

	LLGLSObjectSelectAlpha gls_alpha;

	glBlendFunc(GL_ONE, GL_ZERO);
	glAlphaFunc(GL_GREATER, 0.5f);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,		GL_COMBINE_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB,		GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB,		GL_MODULATE);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB,		GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB,		GL_SRC_COLOR);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB,		GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB,	GL_SRC_ALPHA);

	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB,		GL_PRIMARY_COLOR_ARB);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB,	GL_SRC_ALPHA);

	renderTree(TRUE);

	glAlphaFunc(GL_GREATER, 0.01f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisableClientState (GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolTree::renderTree(BOOL selecting)
{
	LLGLState normalize(GL_NORMALIZE, TRUE);
	
	// Bind the texture for this tree.
	LLViewerImage::bindTexture(mTexturep,sDiffTex);
	if (mTexturep)
	{
		if (mTexturep->getClampS()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		}
		if (mTexturep->getClampT()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	}

	U32 indices_drawn = 0;

	glMatrixMode(GL_MODELVIEW);
	
	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *face = *iter;
		LLDrawable *drawablep = face->getDrawable();

		if (drawablep->isDead() || face->mVertexBuffer.isNull())
		{
			continue;
		}

		face->mVertexBuffer->setBuffer(LLDrawPoolTree::VERTEX_DATA_MASK);
		U32* indicesp = (U32*) face->mVertexBuffer->getIndicesPointer();

		// Render each of the trees
		LLVOTree *treep = (LLVOTree *)drawablep->getVObj().get();

		LLColor4U color(255,255,255,255);

		if (!selecting || treep->mGLName != 0)
		{
			if (selecting)
			{
				S32 name = treep->mGLName;
				
				color = LLColor4U((U8)(name >> 16), (U8)(name >> 8), (U8)name, 255);
			}
			
			glPushMatrix();
			
			// Translate to tree base  HACK - adjustment in Z plants tree underground
			const LLVector3 &pos_agent = treep->getPositionAgent();
			glTranslatef(pos_agent.mV[VX], pos_agent.mV[VY], pos_agent.mV[VZ] - 0.1f);

			// Rotate to tree position
			F32 angle_radians, x, y, z;
			treep->getRotation().getAngleAxis(&angle_radians, &x, &y, &z);
			glRotatef(angle_radians * RAD_TO_DEG, x, y, z);

			// Rotate and bend for current trunk/wind
			// Note that trunk stiffness controls the amount of bend at the trunk as 
			// opposed to the crown of the tree
			// 
			glRotatef(90.f, 0, 0, 1);
			const F32 TRUNK_STIFF = 22.f;
			glRotatef(treep->mTrunkBend.magVec()*TRUNK_STIFF, treep->mTrunkBend.mV[VX], treep->mTrunkBend.mV[VY], 0);

			F32 radius = treep->getScale().magVec()*0.5f;
			radius *= 0.1f;
			glScalef(radius, radius, radius);

			const F32 THRESH_ANGLE_FOR_BILLBOARD = 15.f;
			const F32 BLEND_RANGE_FOR_BILLBOARD = 3.f;

			F32 droop = treep->mDroop + 25.f*(1.f - treep->mTrunkBend.magVec());
			
			S32 stop_depth = 0;
			F32 app_angle = treep->getAppAngle()*LLVOTree::sTreeFactor;
			F32 alpha = 1.0;
			S32 trunk_LOD = 0;

			for (S32 j = 0; j < 4; j++)
			{

				if (app_angle > LLVOTree::sLODAngles[j])
				{
					trunk_LOD = j;
					break;
				}
			} 

			if (app_angle < (THRESH_ANGLE_FOR_BILLBOARD - BLEND_RANGE_FOR_BILLBOARD))
			{
				//
				//  Draw only the billboard 
				//
				//  Only the billboard, can use closer to normal alpha func.
				stop_depth = -1;
				LLFacePool::LLOverrideFaceColor clr(this, color); 
				indices_drawn += treep->drawBranchPipeline(indicesp, trunk_LOD, stop_depth, treep->mDepth, treep->mTrunkDepth, 1.0, treep->mTwist, droop, treep->mBranches, alpha);
			}
			else // if (app_angle > (THRESH_ANGLE_FOR_BILLBOARD + BLEND_RANGE_FOR_BILLBOARD))
			{
				//
				//  Draw only the full geometry tree
				//
				//stop_depth = (app_angle < THRESH_ANGLE_FOR_RECURSION_REDUCTION);
				LLFacePool::LLOverrideFaceColor clr(this, color); 
				indices_drawn += treep->drawBranchPipeline(indicesp, trunk_LOD, stop_depth, treep->mDepth, treep->mTrunkDepth, 1.0, treep->mTwist, droop, treep->mBranches, alpha);
			}
			
			glPopMatrix();
		}
	}

	if (mTexturep)
	{
		if (mTexturep->getClampS()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		}
		if (mTexturep->getClampT()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}

	addIndicesDrawn(indices_drawn);
}

BOOL LLDrawPoolTree::verify() const
{
/*	BOOL ok = TRUE;

	if (!ok)
	{
		printDebugInfo();
	}
	return ok;*/

	return TRUE;
}

LLViewerImage *LLDrawPoolTree::getTexture()
{
	return mTexturep;
}

LLViewerImage *LLDrawPoolTree::getDebugTexture()
{
	return mTexturep;
}


LLColor3 LLDrawPoolTree::getDebugColor() const
{
	return LLColor3(1.f, 0.f, 1.f);
}

S32 LLDrawPoolTree::getMaterialAttribIndex() 
{ 
	return gObjectSimpleProgram.mAttribute[LLShaderMgr::MATERIAL_COLOR];
}
