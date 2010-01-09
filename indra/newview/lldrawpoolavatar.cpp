/** 
 * @file lldrawpoolavatar.cpp
 * @brief LLDrawPoolAvatar class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolavatar.h"
#include "llrender.h"

#include "llvoavatar.h"
#include "m3math.h"

#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "noise.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llappviewer.h"
#include "llrendersphere.h"
#include "llviewerpartsim.h"

static U32 sDataMask = LLDrawPoolAvatar::VERTEX_DATA_MASK;
static U32 sBufferUsage = GL_STREAM_DRAW_ARB;
static U32 sShaderLevel = 0;
static LLGLSLShader* sVertexProgram = NULL;

BOOL	LLDrawPoolAvatar::sSkipOpaque = FALSE;
BOOL	LLDrawPoolAvatar::sSkipTransparent = FALSE;

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
static BOOL sRenderingSkinned = FALSE;
S32 normal_channel = -1;
S32 specular_channel = -1;
S32 diffuse_channel = -1;

static LLFastTimerUtil::DeclareTimer FTM_SHADOW_AVATAR("Avatar Shadow");

LLDrawPoolAvatar::LLDrawPoolAvatar() : 
	LLFacePool(POOL_AVATAR)	
{
}

//-----------------------------------------------------------------------------
// instancePool()
//-----------------------------------------------------------------------------
LLDrawPool *LLDrawPoolAvatar::instancePool()
{
	return new LLDrawPoolAvatar();
}


S32 LLDrawPoolAvatar::getVertexShaderLevel() const
{
	return (S32) LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR);
}

void LLDrawPoolAvatar::prerender()
{
	mVertexShaderLevel = LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR);
	
	sShaderLevel = mVertexShaderLevel;
	
	if (sShaderLevel > 0)
	{
		sBufferUsage = GL_STATIC_DRAW_ARB;
	}
	else
	{
		sBufferUsage = GL_STREAM_DRAW_ARB;
	}
}

LLMatrix4& LLDrawPoolAvatar::getModelView()
{
	static LLMatrix4 ret;

	ret.initRows(LLVector4(gGLModelView+0),
				 LLVector4(gGLModelView+4),
				 LLVector4(gGLModelView+8),
				 LLVector4(gGLModelView+12));

	return ret;
}

//-----------------------------------------------------------------------------
// render()
//-----------------------------------------------------------------------------


S32 LLDrawPoolAvatar::getNumDeferredPasses()
{
	return getNumPasses();
}

void LLDrawPoolAvatar::beginDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);
	
	if (LLPipeline::sImpostorRender)
	{
		beginDeferredSkinned();
		return;
	}

	switch (pass)
	{
	case 0:
		beginDeferredImpostor();
		break;
	case 1:
		beginDeferredRigid();
		break;
	case 2:
		beginDeferredSkinned();
		break;
	}
}

void LLDrawPoolAvatar::endDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);

	if (LLPipeline::sImpostorRender)
	{
		endDeferredSkinned();
		return;
	}

	switch (pass)
	{
	case 0:
		endDeferredImpostor();
		break;
	case 1:
		endDeferredRigid();
		break;
	case 2:
		endDeferredSkinned();
		break;
	}
}

void LLDrawPoolAvatar::renderDeferred(S32 pass)
{
	render(pass);
}

S32 LLDrawPoolAvatar::getNumPostDeferredPasses()
{
	return 1;
}

void LLDrawPoolAvatar::beginPostDeferredPass(S32 pass)
{
	sSkipOpaque = TRUE;
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram = &gDeferredAvatarAlphaProgram;

	sRenderingSkinned = TRUE;

	gPipeline.bindDeferredShader(*sVertexProgram);
	
	enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
}

void LLDrawPoolAvatar::endPostDeferredPass(S32 pass)
{
	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	sRenderingSkinned = FALSE;
	sSkipOpaque = FALSE;
	disable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
	
	gPipeline.unbindDeferredShader(*sVertexProgram);

	sShaderLevel = mVertexShaderLevel;
}

void LLDrawPoolAvatar::renderPostDeferred(S32 pass)
{
	render(2); //pass 2 = skinned
}


S32 LLDrawPoolAvatar::getNumShadowPasses()
{
	return 1;
}

void LLDrawPoolAvatar::beginShadowPass(S32 pass)
{
	LLFastTimer t(FTM_SHADOW_AVATAR);
	
	sVertexProgram = &gDeferredAvatarShadowProgram;
	if (sShaderLevel > 0)
	{
		gAvatarMatrixParam = sVertexProgram->mUniform[LLViewerShaderMgr::AVATAR_MATRIX];
	}
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.2f);
	
	glColor4f(1,1,1,1);

	if ((sShaderLevel > 0))  // for hardware blending
	{
		sRenderingSkinned = TRUE;
		sVertexProgram->bind();
		enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
	}

}

void LLDrawPoolAvatar::endShadowPass(S32 pass)
{
	LLFastTimer t(FTM_SHADOW_AVATAR);

	if (sShaderLevel > 0)
	{
		sRenderingSkinned = FALSE;
		sVertexProgram->unbind();
		disable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
	}

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
}

void LLDrawPoolAvatar::renderShadow(S32 pass)
{
	LLFastTimer t(FTM_SHADOW_AVATAR);

	if (mDrawFace.empty())
	{
		return;
	}

	const LLFace *facep = mDrawFace[0];
	if (!facep->getDrawable())
	{
		return;
	}
	LLVOAvatar *avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();

	if (avatarp->isDead() || avatarp->mIsDummy || avatarp->mDrawable.isNull())
	{
		return;
	}

	BOOL impostor = avatarp->isImpostor();
	if (impostor)
	{
		return;
	}
	
	avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);

}

S32 LLDrawPoolAvatar::getNumPasses()
{
	return LLPipeline::sImpostorRender ? 1 : 3;
}

void LLDrawPoolAvatar::render(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);
	if (LLPipeline::sImpostorRender)
	{
		renderAvatars(NULL, 2);
		return;
	}

	renderAvatars(NULL, pass); // render all avatars
}

void LLDrawPoolAvatar::beginRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);
	//reset vertex buffer mappings
	LLVertexBuffer::unbind();

	if (LLPipeline::sImpostorRender)
	{
		beginSkinned();
		return;
	}

	switch (pass)
	{
	case 0:
		beginFootShadow();
		break;
	case 1:
		beginRigid();
		break;
	case 2:
		beginSkinned();
		break;
	}
}

void LLDrawPoolAvatar::endRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);

	if (LLPipeline::sImpostorRender)
	{
		endSkinned();
		return;
	}

	switch (pass)
	{
	case 0:
		endFootShadow();
		break;
	case 1:
		endRigid();
		break;
	case 2:
		endSkinned();
	}
}

void LLDrawPoolAvatar::beginFootShadow()
{
	if (!LLPipeline::sReflectionRender)
	{
		LLVOAvatar::sRenderDistance = llclamp(LLVOAvatar::sRenderDistance, 16.f, 256.f);
		LLVOAvatar::sNumVisibleAvatars = 0;
	}

	gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	diffuse_channel = 0;
}

void LLDrawPoolAvatar::endFootShadow()
{
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAvatar::beginRigid()
{
	if (gPipeline.canUseVertexShaders())
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectSimpleWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectSimpleProgram;
		}
		
		if (sVertexProgram != NULL)
		{	//eyeballs render with the specular shader
			sVertexProgram->bind();
		}
	}
	else
	{
		sVertexProgram = NULL;
	}
}

void LLDrawPoolAvatar::endRigid()
{
	sShaderLevel = mVertexShaderLevel;
	if (sVertexProgram != NULL)
	{
		sVertexProgram->unbind();
	}
}

void LLDrawPoolAvatar::beginDeferredImpostor()
{
	if (!LLPipeline::sReflectionRender)
	{
		LLVOAvatar::sRenderDistance = llclamp(LLVOAvatar::sRenderDistance, 16.f, 256.f);
		LLVOAvatar::sNumVisibleAvatars = 0;
	}

	sVertexProgram = &gDeferredImpostorProgram;

	normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::DEFERRED_NORMAL);
	specular_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	diffuse_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);

	sVertexProgram->bind();
}

void LLDrawPoolAvatar::endDeferredImpostor()
{
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram->disableTexture(LLViewerShaderMgr::DEFERRED_NORMAL);
	sVertexProgram->disableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	sVertexProgram->unbind();
	gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::beginDeferredRigid()
{
	sVertexProgram = &gDeferredDiffuseProgram;
				
	sVertexProgram->bind();
}

void LLDrawPoolAvatar::endDeferredRigid()
{
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram->unbind();
	gGL.getTexUnit(0)->activate();
}


void LLDrawPoolAvatar::beginSkinned()
{
	if (sShaderLevel > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gAvatarWaterProgram;
			sShaderLevel = llmin((U32) 1, sShaderLevel);
		}
		else
		{
			sVertexProgram = &gAvatarProgram;
		}
	}
	else
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectSimpleWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectSimpleProgram;
		}
	}
	
	if (sShaderLevel > 0)  // for hardware blending
	{
		sRenderingSkinned = TRUE;

		sVertexProgram->bind();
		if (sShaderLevel >= SHADER_LEVEL_CLOTH)
		{
			enable_cloth_weights(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_CLOTHING]);
		}
		enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);

		if (sShaderLevel >= SHADER_LEVEL_BUMP)
		{
			enable_binormals(sVertexProgram->mAttribute[LLViewerShaderMgr::BINORMAL]);
		}
		
		sVertexProgram->enableTexture(LLViewerShaderMgr::BUMP_MAP);
		gGL.getTexUnit(0)->activate();
	}
	else
	{
		if(gPipeline.canUseVertexShaders())
		{
			// software skinning, use a basic shader for windlight.
			// TODO: find a better fallback method for software skinning.
			sVertexProgram->bind();
		}
	}
}

void LLDrawPoolAvatar::endSkinned()
{
	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	if (sShaderLevel > 0)
	{
		sRenderingSkinned = FALSE;
		sVertexProgram->disableTexture(LLViewerShaderMgr::BUMP_MAP);
		gGL.getTexUnit(0)->activate();
		disable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
		if (sShaderLevel >= SHADER_LEVEL_BUMP)
		{
			disable_binormals(sVertexProgram->mAttribute[LLViewerShaderMgr::BINORMAL]);
		}
		if ((sShaderLevel >= SHADER_LEVEL_CLOTH))
		{
			disable_cloth_weights(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_CLOTHING]);
		}

		sVertexProgram->unbind();
		sShaderLevel = mVertexShaderLevel;
	}
	else
	{
		if(gPipeline.canUseVertexShaders())
		{
			// software skinning, use a basic shader for windlight.
			// TODO: find a better fallback method for software skinning.
			sVertexProgram->unbind();
		}
	}

	gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::beginDeferredSkinned()
{
	sSkipTransparent = TRUE;
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram = &gDeferredAvatarProgram;

	sRenderingSkinned = TRUE;

	sVertexProgram->bind();
	
	enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);

	gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::endDeferredSkinned()
{
	sSkipTransparent = FALSE;
	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	sRenderingSkinned = FALSE;
	disable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
	sVertexProgram->unbind();

	sShaderLevel = mVertexShaderLevel;

	gGL.getTexUnit(0)->activate();
}


void LLDrawPoolAvatar::renderAvatars(LLVOAvatar* single_avatar, S32 pass)
{
	if (pass == -1)
	{
		for (S32 i = 1; i < getNumPasses(); i++)
		{ //skip foot shadows
			prerender();
			beginRenderPass(i);
			renderAvatars(single_avatar, i);
			endRenderPass(i);
		}

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
		avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();
	}

    if (avatarp->isDead() || avatarp->mDrawable.isNull())
	{
		return;
	}

	if (!single_avatar && !avatarp->isFullyLoaded() )
	{
		if (pass==1 && (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) || LLViewerPartSim::getMaxPartCount() <= 0))
		{
			// debug code to draw a sphere in place of avatar
			gGL.getTexUnit(0)->bind(LLViewerFetchedTexture::sWhiteImagep);
			gGL.setColorMask(true, true);
			LLVector3 pos = avatarp->getPositionAgent();
			gGL.color4f(1.0f, 1.0f, 1.0f, 0.7f);
			
			gGL.pushMatrix();	 
			gGL.translatef((F32)(pos.mV[VX]),	 
						   (F32)(pos.mV[VY]),	 
							(F32)(pos.mV[VZ]));	 
			 gGL.scalef(0.15f, 0.15f, 0.3f);	 
			 gSphere.render();	 
			 gGL.popMatrix();
			 gGL.setColorMask(true, false);
		}
		// don't render please
		return;
	}

	BOOL impostor = avatarp->isImpostor() && !single_avatar;

	if (impostor && pass != 0)
	{ //don't draw anything but the impostor for impostored avatars
		return;
	}
	
	if (pass == 0 && !impostor && LLPipeline::sUnderWaterRender)
	{ //don't draw foot shadows under water
		return;
	}

    LLOverrideFaceColor color(this, 1.0f, 1.0f, 1.0f, 1.0f);

	if (pass == 0)
	{
		if (!LLPipeline::sReflectionRender)
		{
			LLVOAvatar::sNumVisibleAvatars++;
		}

		if (impostor)
		{
			if (LLPipeline::sRenderDeferred && avatarp->mImpostor.isComplete()) 
			{
				if (normal_channel > -1)
				{
					avatarp->mImpostor.bindTexture(2, normal_channel);
				}
				if (specular_channel > -1)
				{
					avatarp->mImpostor.bindTexture(1, specular_channel);
				}
			}
			avatarp->renderImpostor(LLColor4U(255,255,255,255), diffuse_channel);
		}
		else if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOOT_SHADOWS) && !LLPipeline::sRenderDeferred)
		{
			avatarp->renderFootShadows();	
		}
		return;
	}

	if (single_avatar && avatarp->mSpecialRenderMode >= 1) // 1=anim preview, 2=image preview,  3=morph view
	{
		gPipeline.enableLightsAvatarEdit(LLColor4(.5f, .5f, .5f, 1.f));
	}
	
	if (pass == 1)
	{
		// render rigid meshes (eyeballs) first
		avatarp->renderRigid();
		return;
	}
	
	if (sShaderLevel > 0)
	{
		gAvatarMatrixParam = sVertexProgram->mUniform[LLViewerShaderMgr::AVATAR_MATRIX];
	}
    
	if ((sShaderLevel >= SHADER_LEVEL_CLOTH))
	{
		LLMatrix4 rot_mat;
		LLViewerCamera::getInstance()->getMatrixToLocal(rot_mat);
		LLMatrix4 cfr(OGL_TO_CFR_ROTATION);
		rot_mat *= cfr;
		
		LLVector4 wind;
		wind.setVec(avatarp->mWindVec);
		wind.mV[VW] = 0;
		wind = wind * rot_mat;
		wind.mV[VW] = avatarp->mWindVec.mV[VW];

		sVertexProgram->vertexAttrib4fv(LLViewerShaderMgr::AVATAR_WIND, wind.mV);
		F32 phase = -1.f * (avatarp->mRipplePhase);

		F32 freq = 7.f + (noise1(avatarp->mRipplePhase) * 2.f);
		LLVector4 sin_params(freq, freq, freq, phase);
		sVertexProgram->vertexAttrib4fv(LLViewerShaderMgr::AVATAR_SINWAVE, sin_params.mV);

		LLVector4 gravity(0.f, 0.f, -CLOTHING_GRAVITY_EFFECT, 0.f);
		gravity = gravity * rot_mat;
		sVertexProgram->vertexAttrib4fv(LLViewerShaderMgr::AVATAR_GRAVITY, gravity.mV);
	}

	if( !single_avatar || (avatarp == single_avatar) )
	{
		avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);
	}
}

//-----------------------------------------------------------------------------
// renderForSelect()
//-----------------------------------------------------------------------------
void LLDrawPoolAvatar::renderForSelect()
{


	if (mDrawFace.empty())
	{
		return;
	}

	const LLFace *facep = mDrawFace[0];
	if (!facep->getDrawable())
	{
		return;
	}
	LLVOAvatar *avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();

	if (avatarp->isDead() || avatarp->mIsDummy || avatarp->mDrawable.isNull())
	{
		return;
	}

	S32 curr_shader_level = getVertexShaderLevel();
	S32 name = avatarp->mDrawable->getVObj()->mGLName;
	LLColor4U color((U8)(name >> 16), (U8)(name >> 8), (U8)name);

	BOOL impostor = avatarp->isImpostor();
	if (impostor)
	{
		gGL.getTexUnit(0)->setTextureColorBlend(LLTexUnit::TBO_REPLACE, LLTexUnit::TBS_VERT_COLOR);
		gGL.getTexUnit(0)->setTextureAlphaBlend(LLTexUnit::TBO_MULT, LLTexUnit::TBS_TEX_ALPHA, LLTexUnit::TBS_VERT_ALPHA);

		avatarp->renderImpostor(color);

		gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
		return;
	}

	sVertexProgram = &gAvatarPickProgram;
	if (curr_shader_level > 0)
	{
		gAvatarMatrixParam = sVertexProgram->mUniform[LLViewerShaderMgr::AVATAR_MATRIX];
	}
	gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.2f);
	gGL.setSceneBlendType(LLRender::BT_REPLACE);

	glColor4ubv(color.mV);

	if (curr_shader_level > 0)  // for hardware blending
	{
		sRenderingSkinned = TRUE;
		sVertexProgram->bind();
		enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
	}
	
	avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);

	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	if (curr_shader_level > 0)
	{
		sRenderingSkinned = FALSE;
		sVertexProgram->unbind();
		disable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
	}

	gGL.setAlphaRejectSettings(LLRender::CF_DEFAULT);
	gGL.setSceneBlendType(LLRender::BT_ALPHA);

	// restore texture mode
	gGL.getTexUnit(0)->setTextureBlendType(LLTexUnit::TB_MULT);
}

//-----------------------------------------------------------------------------
// getDebugTexture()
//-----------------------------------------------------------------------------
LLViewerTexture *LLDrawPoolAvatar::getDebugTexture()
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

LLVertexBufferAvatar::LLVertexBufferAvatar()
: LLVertexBuffer(sDataMask, 
	LLViewerShaderMgr::instance()->getVertexShaderLevel(LLViewerShaderMgr::SHADER_AVATAR) > 0 ?	
	GL_DYNAMIC_DRAW_ARB : 
	GL_STREAM_DRAW_ARB)
{

}


void LLVertexBufferAvatar::setupVertexBuffer(U32 data_mask) const
{
	if (sRenderingSkinned)
	{
		U8* base = useVBOs() ? NULL : mMappedData;

		glVertexPointer(3,GL_FLOAT, mStride, (void*)(base + 0));
		glNormalPointer(GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_NORMAL]));
		glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD0]));
		
		set_vertex_weights(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT], mStride, (F32*)(base + mOffsets[TYPE_WEIGHT]));

		if (sShaderLevel >= LLDrawPoolAvatar::SHADER_LEVEL_BUMP)
		{
			set_binormals(sVertexProgram->mAttribute[LLViewerShaderMgr::BINORMAL], mStride, (LLVector3*)(base + mOffsets[TYPE_BINORMAL]));
		}
	
		if (sShaderLevel >= LLDrawPoolAvatar::SHADER_LEVEL_CLOTH)
		{
			set_vertex_clothing_weights(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_CLOTHING], mStride, (LLVector4*)(base + mOffsets[TYPE_CLOTHWEIGHT]));
		}
	}
	else
	{
		LLVertexBuffer::setupVertexBuffer(data_mask);
	}
}

