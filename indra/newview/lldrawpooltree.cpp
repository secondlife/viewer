/** 
 * @file lldrawpooltree.cpp
 * @brief LLDrawPoolTree class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpooltree.h"

#include "llagparray.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewerwindow.h"
#include "llvotree.h"
#include "pipeline.h"
#include "llviewercamera.h"

S32 LLDrawPoolTree::sDiffTex = 0;

LLDrawPoolTree::LLDrawPoolTree(LLViewerImage *texturep) :
	LLDrawPool(POOL_TREE, DATA_SIMPLE_IL_MASK, 0),
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
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_OBJECT);
}

void LLDrawPoolTree::beginRenderPass(S32 pass)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	if ((mVertexShaderLevel > 0))
	{
		S32 scatterTex = gPipeline.mObjectSimpleProgram.enableTexture(LLPipeline::GLSL_SCATTER_MAP);
		LLViewerImage::bindTexture(gSky.mVOSkyp->getScatterMap(), scatterTex);
		sDiffTex = gPipeline.mObjectSimpleProgram.enableTexture(LLPipeline::GLSL_DIFFUSE_MAP);
	}
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
	
	bindGLVertexPointer();
	bindGLTexCoordPointer();
	bindGLNormalPointer();

	LLOverrideFaceColor color(this, 1.f, 1.f, 1.f, 1.f);

	renderTree();
	
}

void LLDrawPoolTree::endRenderPass(S32 pass)
{
	if ((mVertexShaderLevel > 0))
	{
		gPipeline.mObjectSimpleProgram.disableTexture(LLPipeline::GLSL_SCATTER_MAP);
		gPipeline.mObjectSimpleProgram.disableTexture(LLPipeline::GLSL_DIFFUSE_MAP);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
	}

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolTree::renderForSelect()
{
	if (mDrawFace.empty() || !mMemory.count())
	{
		return;
	}

	glEnableClientState (GL_VERTEX_ARRAY);
	glEnableClientState (GL_TEXTURE_COORD_ARRAY);
	
	LLOverrideFaceColor color(this, 1.f, 1.f, 1.f, 1.f);

	LLGLSObjectSelectAlpha gls_alpha;

	glBlendFunc(GL_ONE, GL_ZERO);
	glAlphaFunc(gPickTransparent ? GL_GEQUAL : GL_GREATER, 0.f);

	bindGLVertexPointer();
	bindGLTexCoordPointer();

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

	glMatrixMode(GL_MODELVIEW);
	
	for (std::vector<LLFace*>::iterator iter = mDrawFace.begin();
		 iter != mDrawFace.end(); iter++)
	{
		LLFace *face = *iter;
		LLDrawable *drawablep = face->getDrawable();

		if (drawablep->isDead())
		{
			continue;
		}

		// Render each of the trees
		LLVOTree *treep = (LLVOTree *)drawablep->getVObj();

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

			if (app_angle > (THRESH_ANGLE_FOR_BILLBOARD + BLEND_RANGE_FOR_BILLBOARD))
			{
				//
				//  Draw only the full geometry tree
				//
				//stop_depth = (app_angle < THRESH_ANGLE_FOR_RECURSION_REDUCTION);
				glAlphaFunc(GL_GREATER, 0.5f);
				LLDrawPool::LLOverrideFaceColor clr(this, color); 
				treep->drawBranchPipeline(this, trunk_LOD, stop_depth, treep->mDepth, treep->mTrunkDepth, 1.0, treep->mTwist, droop, treep->mBranches, alpha);
			}
			else if (app_angle < (THRESH_ANGLE_FOR_BILLBOARD - BLEND_RANGE_FOR_BILLBOARD))
			{
				//
				//  Draw only the billboard 
				//
				//  Only the billboard, can use closer to normal alpha func.
				stop_depth = -1;
				glAlphaFunc(GL_GREATER, 0.4f);
				LLDrawPool::LLOverrideFaceColor clr(this, color); 
				treep->drawBranchPipeline(this, trunk_LOD, stop_depth, treep->mDepth, treep->mTrunkDepth, 1.0, treep->mTwist, droop, treep->mBranches, alpha);
			}
			else
			{
				//
				//  Draw a blended version including both billboard and full tree 
				// 
				alpha = (app_angle - THRESH_ANGLE_FOR_BILLBOARD)/BLEND_RANGE_FOR_BILLBOARD;
				BOOL billboard_depth = TRUE; // billboard gets alpha
				if (alpha > 0.5f)
				{
					billboard_depth = FALSE;
				}
				alpha = alpha/2.f + 0.5f;
								
				glAlphaFunc(GL_GREATER, alpha*0.5f);
				{
					LLGLDepthTest gls_depth(GL_TRUE, billboard_depth ? GL_FALSE : GL_TRUE);
					color.mV[3] = (U8) (llclamp(alpha, 0.0f, 1.0f) * 255);
					LLDrawPool::LLOverrideFaceColor clr(this, color); 
					treep->drawBranchPipeline(this, trunk_LOD, 0, treep->mDepth, treep->mTrunkDepth, 1.0, treep->mTwist, droop, treep->mBranches, alpha);
				}
				{
					LLGLDepthTest gls_depth(GL_TRUE, billboard_depth ? GL_TRUE : GL_FALSE);
					glAlphaFunc(GL_GREATER, (1.f - alpha)*0.1f);
					color.mV[3] = (U8) (llclamp(1.f-alpha, 0.0f, 1.0f) * 255);
					LLDrawPool::LLOverrideFaceColor clr(this, color); 
					treep->drawBranchPipeline(this, trunk_LOD, -1, treep->mDepth, treep->mTrunkDepth, 1.0, treep->mTwist, droop, treep->mBranches, 1.f - alpha);
				}
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
	glAlphaFunc(GL_GREATER, 0.01f);
}


S32 LLDrawPoolTree::rebuild()
{
	mRebuildTime++;
	if (mRebuildTime >  mRebuildFreq)
	{
		// Flush AGP to force an AGP realloc and reduce AGP fragmentation
		flushAGP();
		mRebuildTime = 0;
	}

	return 0;
}

BOOL LLDrawPoolTree::verify() const
{
	BOOL ok = TRUE;

	// shared geometry.  Just verify that it's there and correct.

	// Verify all indices in the pool are in the right range
	const U32 *indicesp = getRawIndices();
	for (U32 i = 0; i < getIndexCount(); i++)
	{
		if (indicesp[i] > getVertexCount())
		{
			ok = FALSE;
			llinfos << "Bad index in tree pool!" << llendl;
		}
	}
	
	if (!ok)
	{
		printDebugInfo();
	}
	return ok;
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
	return gPipeline.mObjectSimpleProgram.mAttribute[LLPipeline::GLSL_MATERIAL_COLOR];
}
