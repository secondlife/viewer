/** 
 * @file lldrawpoolavatar.cpp
 * @brief LLDrawPoolAvatar class implementation
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolavatar.h"

#include "llvoavatar.h"
#include "m3math.h"

#include "llagparray.h"
#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "noise.h"
#include "pipeline.h"

extern F32 gFrameDTClamped;
extern BOOL gUseGLPick;

F32 CLOTHING_GRAVITY_EFFECT = 0.7f;
F32 CLOTHING_ACCEL_FORCE_FACTOR = 0.2f;
const S32 NUM_TEST_AVATARS = 30;
const S32 MIN_PIXEL_AREA_2_PASS_SKINNING = 500000000;

// Format for gAGPVertices
// vertex format for bumpmapping:
//  vertices   12
//  pad		    4
//  normals    12
//  pad		    4
//  texcoords0  8
//  texcoords1  8
// total       48
//
// for no bumpmapping
//  vertices	   12
//  texcoords	8
//  normals	   12
// total	   32
//

S32 AVATAR_OFFSET_POS = 0;
S32 AVATAR_OFFSET_NORMAL = 16;
S32 AVATAR_OFFSET_TEX0 = 32;
S32 AVATAR_OFFSET_TEX1 = 40;
S32 AVATAR_VERTEX_BYTES = 48;


BOOL gAvatarEmbossBumpMap = FALSE;

LLDrawPoolAvatar::LLDrawPoolAvatar() :
LLDrawPool(POOL_AVATAR, 
		   DATA_SIMPLE_IL_MASK, 
		   DATA_VERTEX_WEIGHTS_MASK | DATA_CLOTHING_WEIGHTS_MASK )
{
	mCleanupUnused = FALSE;

	// Overide the data layout
	mDataMaskIL = 0;
	mStride = 0;
	for (S32 i = 0; i < DATA_MAX_TYPES; i++)
	{
		mDataOffsets[i] = 0;
	}

	// Note: padding is to speed up SSE code
	mDataMaskIL |= DATA_VERTICES_MASK;
	mDataOffsets[DATA_VERTICES] = mStride;
	mStride += sDataSizes[DATA_VERTICES];

	mStride += 4;

	mDataMaskIL |= DATA_NORMALS_MASK;
	mDataOffsets[DATA_NORMALS] = mStride;
	mStride += sDataSizes[DATA_NORMALS];

	mStride += 4;

	// Note: binormals are stripped off in software blending
	mDataMaskIL |= DATA_BINORMALS_MASK;
	mDataOffsets[DATA_BINORMALS] = mStride;
	mStride += sDataSizes[DATA_BINORMALS];

	mStride += 4; // To keep the structure 16-byte aligned (for SSE happiness)

	mDataMaskIL |= DATA_TEX_COORDS0_MASK;
	mDataOffsets[DATA_TEX_COORDS0] = mStride;
	mStride += sDataSizes[DATA_TEX_COORDS0];

	mDataMaskIL |= DATA_TEX_COORDS1_MASK;
	mDataOffsets[DATA_TEX_COORDS1] = mStride;
	mStride += sDataSizes[DATA_TEX_COORDS1];

	//LLDebugVarMessageBox::show("acceleration", &CLOTHING_ACCEL_FORCE_FACTOR, 10.f, 0.1f);
	//LLDebugVarMessageBox::show("gravity", &CLOTHING_GRAVITY_EFFECT, 10.f, 0.1f);
	
}

//-----------------------------------------------------------------------------
// instancePool()
//-----------------------------------------------------------------------------
LLDrawPool *LLDrawPoolAvatar::instancePool()
{
	return new LLDrawPoolAvatar();
}


S32 LLDrawPoolAvatar::rebuild()
{
	mRebuildTime++;
	if (mRebuildTime > mRebuildFreq)
	{
		flushAGP();

		mRebuildTime = 0;
	}

	return 0;
}

BOOL gRenderAvatar = TRUE;

void LLDrawPoolAvatar::prerender()
{
	mVertexShaderLevel = gPipeline.getVertexShaderLevel(LLPipeline::SHADER_AVATAR);
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------
void LLDrawPoolAvatar::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_CHARACTERS);
	renderAvatars(NULL); // render all avatars
}

void LLDrawPoolAvatar::renderAvatars(LLVOAvatar* single_avatar, BOOL no_shaders)
{
	if (no_shaders)
	{
		mVertexShaderLevel = 0;
	}

	if (!gRenderAvatar)
	{
		return;
	}

	if (mDrawFace.empty() && !single_avatar)
	{
		return;
	}

	LLVOAvatar *avatarp;

	if (single_avatar)
	{
		avatarp = single_avatar;
	}
	else
	{
		const LLFace *facep = mDrawFace[0];
		if (!facep->getDrawable())
		{
			return;
		}
		avatarp = (LLVOAvatar *)(facep->getDrawable()->getVObj());
	}

	if (avatarp->isDead() || avatarp->mDrawable.isNull())
	{
		return;
	}

    LLOverrideFaceColor color(this, 1.0f, 1.0f, 1.0f, 1.0f);
	
	if( !single_avatar || (avatarp == single_avatar) )
	{
		if (LLVOAvatar::sShowCollisionVolumes)
		{
			LLGLSNoTexture no_texture;
			avatarp->renderCollisionVolumes();
		}

		LLGLEnable normalize(GL_NORMALIZE);

		if (avatarp->mIsSelf && LLAgent::sDebugDisplayTarget)
		{
			LLGLSNoTexture gls_no_texture;
			LLVector3 pos = avatarp->getPositionAgent();

			color.setColor(1.0f, 0.0f, 0.0f, 0.8f);
			glBegin(GL_LINES);
			{
				glVertex3fv((pos - LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.f, 0.2f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.f, 0.2f)).mV);
			}glEnd();

			pos = avatarp->mDrawable->getPositionAgent();
			color.setColor(1.0f, 0.0f, 0.0f, 0.8f);
			glBegin(GL_LINES);
			{
				glVertex3fv((pos - LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.f, 0.2f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.f, 0.2f)).mV);
			}glEnd();

			pos = avatarp->mRoot.getWorldPosition();
			color.setColor(1.0f, 1.0f, 1.0f, 0.8f);
			glBegin(GL_LINES);
			{
				glVertex3fv((pos - LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.f, 0.2f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.f, 0.2f)).mV);
			}glEnd();

			pos = avatarp->mPelvisp->getWorldPosition();
			color.setColor(0.0f, 0.0f, 1.0f, 0.8f);
			glBegin(GL_LINES);
			{
				glVertex3fv((pos - LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.2f, 0.f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.2f, 0.f)).mV);
				glVertex3fv((pos - LLVector3(0.f, 0.f, 0.2f)).mV);
				glVertex3fv((pos + LLVector3(0.f, 0.f, 0.2f)).mV);
			}glEnd();	

			color.setColor(1.0f, 1.0f, 1.0f, 1.0f);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		LLGLSLShader* vertex_program = &gPipeline.mAvatarProgram;
		if (mVertexShaderLevel > 0)
		{
			gPipeline.mAvatarMatrixParam = vertex_program->mUniform[LLPipeline::GLSL_AVATAR_MATRIX];
		}
		
		//--------------------------------------------------------------------------------
		// this is where we first hit the software blending path
		// if enabled, we need to set up the proper buffers and avoid setting other state
		//--------------------------------------------------------------------------------
		if (!(mVertexShaderLevel > 0))
		{

			// performance could be increased by better utilizing the buffers, for example, only using 1k buffers for lo-res
			// avatars. But the only problem with using fewer buffers is that we're more likely to wait for a fence to complete

			// vertex format:
			//  vertices 12
			//  texcoords 8
			//  normals  12
			//	binormals 12
			//  padding  4
			// total     48

			// Rotate to the next buffer, round-robin.
			gPipeline.bufferRotate();

			// Wait until the hardware is done reading the last set of vertices from the buffer before writing the next set.
			gPipeline.bufferWaitFence();

			// Need to do this because we may be rendering without AGP even in AGP mode
			U8* buffer_offset_start = gPipeline.bufferGetScratchMemory();
			glVertexPointer(  3, GL_FLOAT, AVATAR_VERTEX_BYTES, buffer_offset_start + AVATAR_OFFSET_POS);
			glTexCoordPointer(2, GL_FLOAT, AVATAR_VERTEX_BYTES, buffer_offset_start + AVATAR_OFFSET_TEX0);
			glNormalPointer(     GL_FLOAT, AVATAR_VERTEX_BYTES, buffer_offset_start + AVATAR_OFFSET_NORMAL);
			
		}

		if ((mVertexShaderLevel > 0))  // for hardware blending
		{
			bindGLVertexPointer();
			bindGLNormalPointer();
			bindGLTexCoordPointer(0);
		}
		
		if ((mVertexShaderLevel > 0))
		{	//eyeballs render with the specular shader
			gPipeline.mAvatarEyeballProgram.bind();
			gPipeline.mMaterialIndex = gPipeline.mAvatarEyeballProgram.mAttribute[LLPipeline::GLSL_MATERIAL_COLOR];
			gPipeline.mSpecularIndex = gPipeline.mAvatarEyeballProgram.mAttribute[LLPipeline::GLSL_SPECULAR_COLOR];

			S32 index = gPipeline.mAvatarEyeballProgram.enableTexture(LLPipeline::GLSL_SCATTER_MAP);		
			gSky.mVOSkyp->getScatterMap()->bind(index);
					
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		
		if (avatarp->mSpecialRenderMode == 0) // normal
		{
			gPipeline.enableLightsAvatar(avatarp->mDrawable->getSunShadowFactor());
		}
		else if (avatarp->mSpecialRenderMode == 1)  // anim preview
		{
			gPipeline.enableLightsAvatarEdit(LLColor4(0.7f, 0.6f, 0.3f, 1.f));
		}
		else // 2=image preview,  3=morph view
		{
			gPipeline.enableLightsAvatarEdit(LLColor4(.5f, .5f, .5f, 1.f));
		}
		
		// render rigid meshes (eyeballs) first
		mIndicesDrawn += avatarp->renderRigid();

		if ((mVertexShaderLevel > 0))
		{	
			gPipeline.mAvatarEyeballProgram.disableTexture(LLPipeline::GLSL_SCATTER_MAP);		
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		
		if (!gRenderForSelect && avatarp->mIsSelf && LLVOAvatar::sAvatarLoadTest)
		{
			LLVector3 orig_pos_root = avatarp->mRoot.getPosition();
			LLVector3 next_pos_root = orig_pos_root;
			for (S32 i = 0; i < NUM_TEST_AVATARS; i++)
			{
				next_pos_root.mV[VX] += 1.f;
				if (i % 5 == 0)
				{
					next_pos_root.mV[VY] += 1.f;
					next_pos_root.mV[VX] = orig_pos_root.mV[VX];
				}

				avatarp->mRoot.setPosition(next_pos_root); // avatar load test
				avatarp->mRoot.updateWorldMatrixChildren(); // avatar load test

				mIndicesDrawn += avatarp->renderRigid();
			}
			avatarp->mRoot.setPosition(orig_pos_root); // avatar load test
			avatarp->mRoot.updateWorldMatrixChildren(); // avatar load test
		}

		if ((mVertexShaderLevel > 0))  // for hardware blending
		{
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			if ((mVertexShaderLevel >= SHADER_LEVEL_BUMP))
			{
				bindGLTexCoordPointer(1);
				
				bindGLBinormalPointer(vertex_program->mAttribute[LLPipeline::GLSL_BINORMAL]);
				gPipeline.mMaterialIndex = vertex_program->mAttribute[LLPipeline::GLSL_MATERIAL_COLOR];
				gPipeline.mSpecularIndex = vertex_program->mAttribute[LLPipeline::GLSL_SPECULAR_COLOR];
			}
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			bindGLTexCoordPointer(0);
			vertex_program->bind();
			bindGLVertexWeightPointer(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_WEIGHT]);
			if ((mVertexShaderLevel >= SHADER_LEVEL_CLOTH))
			{
				bindGLVertexClothingWeightPointer(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_CLOTHING]);
				enable_cloth_weights(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_CLOTHING]);
			}
			enable_vertex_weighting(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_WEIGHT]);

			if ((mVertexShaderLevel >= SHADER_LEVEL_BUMP))
			{
				enable_binormals(vertex_program->mAttribute[LLPipeline::GLSL_BINORMAL]);
			}
			
			vertex_program->enableTexture(LLPipeline::GLSL_BUMP_MAP);
			S32 index = vertex_program->enableTexture(LLPipeline::GLSL_SCATTER_MAP);
			gSky.mVOSkyp->getScatterMap()->bind(index);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}

		if ((mVertexShaderLevel >= SHADER_LEVEL_CLOTH))
		{
			LLMatrix4 rot_mat;
			gCamera->getMatrixToLocal(rot_mat);
			LLMatrix4 cfr(OGL_TO_CFR_ROTATION);
			rot_mat *= cfr;
			
			LLVector4 wind;
			wind.setVec(avatarp->mWindVec);
			wind.mV[VW] = 0;
			wind = wind * rot_mat;
			wind.mV[VW] = avatarp->mWindVec.mV[VW];

			vertex_program->vertexAttrib4fv(LLPipeline::GLSL_AVATAR_WIND, wind.mV);
			F32 phase = -1.f * (avatarp->mRipplePhase);

			F32 freq = 7.f + (noise1(avatarp->mRipplePhase) * 2.f);
			LLVector4 sin_params(freq, freq, freq, phase);
			vertex_program->vertexAttrib4fv(LLPipeline::GLSL_AVATAR_SINWAVE, sin_params.mV);

			LLVector4 gravity(0.f, 0.f, -CLOTHING_GRAVITY_EFFECT, 0.f);
			gravity = gravity * rot_mat;
			vertex_program->vertexAttrib4fv(LLPipeline::GLSL_AVATAR_GRAVITY, gravity.mV);
		}

		mIndicesDrawn += avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);
		
		if (!gRenderForSelect && avatarp->mIsSelf && LLVOAvatar::sAvatarLoadTest)
		{
			LLVector3 orig_pos_root = avatarp->mRoot.getPosition();
			LLVector3 next_pos_root = orig_pos_root;
			for (S32 i = 0; i < NUM_TEST_AVATARS; i++)
			{
				next_pos_root.mV[VX] += 1.f;
				if (i % 5 == 0)
				{
					next_pos_root.mV[VY] += 1.f;
					next_pos_root.mV[VX] = orig_pos_root.mV[VX];
				}

				avatarp->mRoot.setPosition(next_pos_root); // avatar load test
				avatarp->mRoot.updateWorldMatrixChildren(); // avatar load test

				mIndicesDrawn += avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);
			}
			avatarp->mRoot.setPosition(orig_pos_root); // avatar load test
			avatarp->mRoot.updateWorldMatrixChildren(); // avatar load test
		}

		// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
		if (!(mVertexShaderLevel > 0))
		{
			// want for the previously bound fence to finish
			gPipeline.bufferSendFence();
		}
		else
		{
			vertex_program->disableTexture(LLPipeline::GLSL_BUMP_MAP);
			vertex_program->disableTexture(LLPipeline::GLSL_SCATTER_MAP);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			disable_vertex_weighting(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_WEIGHT]);
			if ((mVertexShaderLevel >= SHADER_LEVEL_BUMP))
			{
				disable_binormals(vertex_program->mAttribute[LLPipeline::GLSL_BINORMAL]);
			}
			if ((mVertexShaderLevel >= SHADER_LEVEL_CLOTH))
			{
				disable_cloth_weights(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_CLOTHING]);
			}

			vertex_program->unbind();
		}
	}
}

//-----------------------------------------------------------------------------
// renderForSelect()
//-----------------------------------------------------------------------------
void LLDrawPoolAvatar::renderForSelect()
{
	if (gUseGLPick)
	{
		return;
	}
	//gGLSObjectSelectDepthAlpha.set();

	if (!gRenderAvatar)
	{
		return;
	}

	if (mDrawFace.empty() || !mMemory.count())
	{
		return;
	}

	const LLFace *facep = mDrawFace[0];
	if (!facep->getDrawable())
	{
		return;
	}
	LLVOAvatar *avatarp = (LLVOAvatar *)(facep->getDrawable()->getVObj());

	if (avatarp->isDead() || avatarp->mIsDummy || avatarp->mDrawable.isNull())
	{
		return;
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	LLGLSLShader* vertex_program = &gPipeline.mAvatarPickProgram;
	if (mVertexShaderLevel > 0)
	{
		gPipeline.mAvatarMatrixParam = vertex_program->mUniform[LLPipeline::GLSL_AVATAR_MATRIX];
	}
	glAlphaFunc(GL_GEQUAL, 0.2f);
	glBlendFunc(GL_ONE, GL_ZERO);

	//--------------------------------------------------------------------------------
	// this is where we first hit the software blending path
	// if enabled, we need to set up the proper buffers and avoid setting other state
	//--------------------------------------------------------------------------------
	if (!(mVertexShaderLevel > 0) || gUseGLPick)
	{

		// Rotate to the next buffer, round-robin.
		gPipeline.bufferRotate();

		// Wait until the hardware is done reading the last set of vertices from the buffer before writing the next set.
		gPipeline.bufferWaitFence();

		// Need to do this because we may be rendering without AGP even in AGP mode
		U8* buffer_offset_start = gPipeline.bufferGetScratchMemory();
		glVertexPointer(  3, GL_FLOAT, AVATAR_VERTEX_BYTES, buffer_offset_start + AVATAR_OFFSET_POS);
		glTexCoordPointer(2, GL_FLOAT, AVATAR_VERTEX_BYTES, buffer_offset_start + AVATAR_OFFSET_TEX0);
		glNormalPointer(     GL_FLOAT, AVATAR_VERTEX_BYTES, buffer_offset_start + AVATAR_OFFSET_NORMAL);
	}

	S32 name = avatarp->mDrawable->getVObj()->mGLName;
	LLColor4U color((U8)(name >> 16), (U8)(name >> 8), (U8)name);
	glColor4ubv(color.mV);

	if ((mVertexShaderLevel > 0) && !gUseGLPick)  // for hardware blending
	{
		bindGLVertexPointer();
		bindGLNormalPointer();
		bindGLTexCoordPointer(0);
	}

	// render rigid meshes (eyeballs) first
	mIndicesDrawn += avatarp->renderRigid();

	if ((mVertexShaderLevel > 0) && !gUseGLPick)  // for hardware blending
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		bindGLTexCoordPointer(0);
		vertex_program->bind();
		bindGLVertexWeightPointer(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_WEIGHT]);
		/*if ((mVertexShaderLevel >= SHADER_LEVEL_CLOTH))
		{
			bindGLVertexClothingWeightPointer();
			enable_cloth_weights();
		}*/
		enable_vertex_weighting(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_WEIGHT]);
	}

	mIndicesDrawn += avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);

	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	if (!(mVertexShaderLevel > 0) || gUseGLPick)
	{
		// want for the previously bound fence to finish
		gPipeline.bufferSendFence();
	}
	else
	{
		vertex_program->unbind();
		disable_vertex_weighting(vertex_program->mAttribute[LLPipeline::GLSL_AVATAR_WEIGHT]);
		
		/*if ((mVertexShaderLevel >= SHADER_LEVEL_CLOTH))
		{
			disable_cloth_weights();
		}*/
	}

	glAlphaFunc(GL_GREATER, 0.01f);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// restore texture mode
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

//-----------------------------------------------------------------------------
// getDebugTexture()
//-----------------------------------------------------------------------------
LLViewerImage *LLDrawPoolAvatar::getDebugTexture()
{
	if (mReferences.empty())
	{
		return NULL;
	}
	LLFace *face = mReferences[0];
	if (!face->getDrawable())
	{
		return NULL;
	}
	const LLViewerObject *objectp = face->getDrawable()->getVObj();

	// Avatar should always have at least 1 (maybe 3?) TE's.
	return objectp->getTEImage(0);
}


LLColor3 LLDrawPoolAvatar::getDebugColor() const
{
	return LLColor3(0.f, 1.f, 0.f);
}
