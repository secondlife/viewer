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
#include "lldrawpoolbump.h"
#include "llface.h"
#include "llmeshrepository.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "noise.h"
#include "pipeline.h"
#include "llviewershadermgr.h"
#include "llvovolume.h"
#include "llvolume.h"
#include "llappviewer.h"
#include "llrendersphere.h"
#include "llviewerpartsim.h"

static U32 sDataMask = LLDrawPoolAvatar::VERTEX_DATA_MASK;
static U32 sBufferUsage = GL_STREAM_DRAW_ARB;
static U32 sShaderLevel = 0;


LLGLSLShader* LLDrawPoolAvatar::sVertexProgram = NULL;
BOOL	LLDrawPoolAvatar::sSkipOpaque = FALSE;
BOOL	LLDrawPoolAvatar::sSkipTransparent = FALSE;

static bool is_deferred_render = false;

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
S32 cube_channel = -1;

static const U32 rigged_data_mask[] = {
	LLDrawPoolAvatar::RIGGED_SIMPLE_MASK,
	LLDrawPoolAvatar::RIGGED_FULLBRIGHT_MASK,
	LLDrawPoolAvatar::RIGGED_SHINY_MASK,
	LLDrawPoolAvatar::RIGGED_FULLBRIGHT_SHINY_MASK,
	LLDrawPoolAvatar::RIGGED_GLOW_MASK,
	LLDrawPoolAvatar::RIGGED_ALPHA_MASK,
	LLDrawPoolAvatar::RIGGED_FULLBRIGHT_ALPHA_MASK,
	LLDrawPoolAvatar::RIGGED_DEFERRED_BUMP_MASK,						 
	LLDrawPoolAvatar::RIGGED_DEFERRED_SIMPLE_MASK,
};


static LLFastTimer::DeclareTimer FTM_SHADOW_AVATAR("Avatar Shadow");

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
		sBufferUsage = GL_DYNAMIC_DRAW_ARB;
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



void LLDrawPoolAvatar::beginDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);
	
	sSkipTransparent = TRUE;
	is_deferred_render = true;
	
	if (LLPipeline::sImpostorRender)
	{ //impostor pass does not have rigid or impostor rendering
		pass += 2;
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
	case 3:
		beginDeferredRiggedSimple();
		break;
	case 4:
		beginDeferredRiggedBump();
		break;
	}
}

void LLDrawPoolAvatar::endDeferredPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);

	sSkipTransparent = FALSE;
	is_deferred_render = false;

	if (LLPipeline::sImpostorRender)
	{
		pass += 2;
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
	case 3:
		endDeferredRiggedSimple();
		break;
	case 4:
		endDeferredRiggedBump();
		break;
	}
}

void LLDrawPoolAvatar::renderDeferred(S32 pass)
{
	render(pass);
}

S32 LLDrawPoolAvatar::getNumPostDeferredPasses()
{
	return 6;
}

void LLDrawPoolAvatar::beginPostDeferredPass(S32 pass)
{
	switch (pass)
	{
	case 0:
		beginPostDeferredAlpha();
		break;
	case 1:
		beginRiggedFullbright();
		break;
	case 2:
		beginRiggedFullbrightShiny();
		break;
	case 3:
		beginDeferredRiggedAlpha();
		break;
	case 4:
		beginRiggedFullbrightAlpha();
		break;
	case 5:
		beginRiggedGlow();
		break;
	}
}

void LLDrawPoolAvatar::beginPostDeferredAlpha()
{
	sSkipOpaque = TRUE;
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram = &gDeferredAvatarAlphaProgram;

	sRenderingSkinned = TRUE;

	gPipeline.bindDeferredShader(*sVertexProgram);
	
	enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
}

void LLDrawPoolAvatar::beginDeferredRiggedAlpha()
{
	sVertexProgram = &gDeferredSkinnedAlphaProgram;
	gPipeline.bindDeferredShader(*sVertexProgram);
	diffuse_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAvatar::endDeferredRiggedAlpha()
{
	LLVertexBuffer::unbind();
	gPipeline.unbindDeferredShader(*sVertexProgram);
	LLVertexBuffer::sWeight4Loc = -1;
	sVertexProgram = NULL;
}

void LLDrawPoolAvatar::endPostDeferredPass(S32 pass)
{
	switch (pass)
	{
	case 0:
		endPostDeferredAlpha();
		break;
	case 1:
		endRiggedFullbright();
		break;
	case 2:
		endRiggedFullbrightShiny();
		break;
	case 3:
		endDeferredRiggedAlpha();
		break;
	case 4:
		endRiggedFullbrightAlpha();
		break;
	case 5:
		endRiggedGlow();
		break;
	}
}

void LLDrawPoolAvatar::endPostDeferredAlpha()
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
	const S32 actual_pass[] =
	{ //map post deferred pass numbers to what render() expects
		2, //skinned
		4, // rigged fullbright
		6, //rigged fullbright shiny
		7, //rigged alpha
		8, //rigged fullbright alpha
		9, //rigged glow
	};

	render(actual_pass[pass]);
}


S32 LLDrawPoolAvatar::getNumShadowPasses()
{
	return 2;
}

void LLDrawPoolAvatar::beginShadowPass(S32 pass)
{
	LLFastTimer t(FTM_SHADOW_AVATAR);

	if (pass == 0)
	{
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
	else
	{
		sVertexProgram = &gDeferredAttachmentShadowProgram;
		diffuse_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
		sVertexProgram->bind();
		LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
	}
}

void LLDrawPoolAvatar::endShadowPass(S32 pass)
{
	LLFastTimer t(FTM_SHADOW_AVATAR);
	if (pass == 0)
	{
		if (sShaderLevel > 0)
		{
			sRenderingSkinned = FALSE;
			sVertexProgram->unbind();
			disable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);
		}
	}
	else
	{
		LLVertexBuffer::unbind();
		sVertexProgram->unbind();
		LLVertexBuffer::sWeight4Loc = -1;
		sVertexProgram = NULL;
	}
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
	
	if (pass == 0)
	{
		if (sShaderLevel > 0)
		{
			gAvatarMatrixParam = sVertexProgram->mUniform[LLViewerShaderMgr::AVATAR_MATRIX];
		}

		avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);
	}
	else
	{
		renderRigged(avatarp, RIGGED_SIMPLE);
		renderRigged(avatarp, RIGGED_ALPHA);
		renderRigged(avatarp, RIGGED_FULLBRIGHT);
		renderRigged(avatarp, RIGGED_FULLBRIGHT_SHINY);
		renderRigged(avatarp, RIGGED_SHINY);
		renderRigged(avatarp, RIGGED_FULLBRIGHT_ALPHA);
	}
}

S32 LLDrawPoolAvatar::getNumPasses()
{
	if (LLPipeline::sImpostorRender)
	{
		return 8;
	}
	else if (getVertexShaderLevel() > 0)
	{
		return 10;
	}
	else
	{
		return 3;
	}
}

S32 LLDrawPoolAvatar::getNumDeferredPasses()
{
	if (LLPipeline::sImpostorRender)
	{
		return 3;
	}
	else
	{
		return 5;
	}
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
	{ //impostor render does not have impostors or rigid rendering
		pass += 2;
	}

	switch (pass)
	{
	case 0:
		beginImpostor();
		break;
	case 1:
		beginRigid();
		break;
	case 2:
		beginSkinned();
		break;
	case 3:
		beginRiggedSimple();
		break;
	case 4:
		beginRiggedFullbright();
		break;
	case 5:
		beginRiggedShinySimple();
		break;
	case 6:
		beginRiggedFullbrightShiny();
		break;
	case 7:
		beginRiggedAlpha();
		break;
	case 8:
		beginRiggedFullbrightAlpha();
		break;
	case 9:
		beginRiggedGlow();
		break;
	}
}

void LLDrawPoolAvatar::endRenderPass(S32 pass)
{
	LLFastTimer t(FTM_RENDER_CHARACTERS);

	if (LLPipeline::sImpostorRender)
	{
		pass += 2;		
	}

	switch (pass)
	{
	case 0:
		endImpostor();
		break;
	case 1:
		endRigid();
		break;
	case 2:
		endSkinned();
		break;
	case 3:
		endRiggedSimple();
		break;
	case 4:
		endRiggedFullbright();
		break;
	case 5:
		endRiggedShinySimple();
		break;
	case 6:
		endRiggedFullbrightShiny();
		break;
	case 7:
		endRiggedAlpha();
		break;
	case 8:
		endRiggedFullbrightAlpha();
		break;
	case 9:
		endRiggedGlow();
		break;
	}
}

void LLDrawPoolAvatar::beginImpostor()
{
	if (!LLPipeline::sReflectionRender)
	{
		LLVOAvatar::sRenderDistance = llclamp(LLVOAvatar::sRenderDistance, 16.f, 256.f);
		LLVOAvatar::sNumVisibleAvatars = 0;
	}

	gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	diffuse_channel = 0;
}

void LLDrawPoolAvatar::endImpostor()
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

	specular_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::DEFERRED_NORMAL);
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

void LLDrawPoolAvatar::beginRiggedSimple()
{
	sVertexProgram = &gSkinnedObjectSimpleProgram;
	diffuse_channel = 0;
	gSkinnedObjectSimpleProgram.bind();
	LLVertexBuffer::sWeight4Loc = gSkinnedObjectSimpleProgram.getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedSimple()
{
	sVertexProgram = NULL;
	LLVertexBuffer::unbind();
	gSkinnedObjectSimpleProgram.unbind();
	LLVertexBuffer::sWeight4Loc = -1;
}

void LLDrawPoolAvatar::beginRiggedAlpha()
{
	sVertexProgram = &gSkinnedObjectSimpleProgram;
	diffuse_channel = 0;
	sVertexProgram->bind();
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedAlpha()
{
	sVertexProgram->unbind();
	sVertexProgram = NULL;
	LLVertexBuffer::unbind();
	LLVertexBuffer::sWeight4Loc = -1;
}


void LLDrawPoolAvatar::beginRiggedFullbrightAlpha()
{
	sVertexProgram = &gSkinnedObjectFullbrightProgram;
	diffuse_channel = 0;
	sVertexProgram->bind();
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedFullbrightAlpha()
{
	sVertexProgram->unbind();
	sVertexProgram = NULL;
	LLVertexBuffer::unbind();
	LLVertexBuffer::sWeight4Loc = -1;
}

void LLDrawPoolAvatar::beginRiggedGlow()
{
	sVertexProgram = &gSkinnedObjectFullbrightProgram;
	diffuse_channel = 0;
	sVertexProgram->bind();
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedGlow()
{
	sVertexProgram->unbind();
	sVertexProgram = NULL;
	LLVertexBuffer::unbind();
	LLVertexBuffer::sWeight4Loc = -1;
}

void LLDrawPoolAvatar::beginRiggedFullbright()
{
	sVertexProgram = &gSkinnedObjectFullbrightProgram;
	diffuse_channel = 0;
	gSkinnedObjectFullbrightProgram.bind();
	LLVertexBuffer::sWeight4Loc = gSkinnedObjectFullbrightProgram.getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedFullbright()
{
	sVertexProgram = NULL;
	LLVertexBuffer::unbind();
	gSkinnedObjectFullbrightProgram.unbind();
	LLVertexBuffer::sWeight4Loc = -1;
}

void LLDrawPoolAvatar::beginRiggedShinySimple()
{
	sVertexProgram = &gSkinnedObjectShinySimpleProgram;
	sVertexProgram->bind();
	LLDrawPoolBump::bindCubeMap(sVertexProgram, 2, diffuse_channel, cube_channel, false);
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedShinySimple()
{
	LLVertexBuffer::unbind();
	LLDrawPoolBump::unbindCubeMap(sVertexProgram, 2, diffuse_channel, cube_channel, false);
	sVertexProgram->unbind();
	sVertexProgram = NULL;
	LLVertexBuffer::sWeight4Loc = -1;
}

void LLDrawPoolAvatar::beginRiggedFullbrightShiny()
{
	sVertexProgram = &gSkinnedObjectFullbrightShinyProgram;
	sVertexProgram->bind();
	LLDrawPoolBump::bindCubeMap(sVertexProgram, 2, diffuse_channel, cube_channel, false);
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endRiggedFullbrightShiny()
{
	LLVertexBuffer::unbind();
	LLDrawPoolBump::unbindCubeMap(sVertexProgram, 2, diffuse_channel, cube_channel, false);
	sVertexProgram->unbind();
	sVertexProgram = NULL;
	LLVertexBuffer::sWeight4Loc = -1;
}


void LLDrawPoolAvatar::beginDeferredRiggedSimple()
{
	sVertexProgram = &gDeferredSkinnedDiffuseProgram;
	diffuse_channel = 0;
	sVertexProgram->bind();
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endDeferredRiggedSimple()
{
	LLVertexBuffer::unbind();
	sVertexProgram->unbind();
	LLVertexBuffer::sWeight4Loc = -1;
	sVertexProgram = NULL;
}

void LLDrawPoolAvatar::beginDeferredRiggedBump()
{
	sVertexProgram = &gDeferredSkinnedBumpProgram;
	sVertexProgram->bind();
	normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::BUMP_MAP);
	diffuse_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	LLVertexBuffer::sWeight4Loc = sVertexProgram->getAttribLocation(LLViewerShaderMgr::OBJECT_WEIGHT);
}

void LLDrawPoolAvatar::endDeferredRiggedBump()
{
	LLVertexBuffer::unbind();
	sVertexProgram->disableTexture(LLViewerShaderMgr::BUMP_MAP);
	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	sVertexProgram->unbind();
	LLVertexBuffer::sWeight4Loc = -1;
	normal_channel = -1;
	diffuse_channel = -1;
	sVertexProgram = NULL;
}

void LLDrawPoolAvatar::beginDeferredSkinned()
{
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram = &gDeferredAvatarProgram;

	sRenderingSkinned = TRUE;

	sVertexProgram->bind();
	
	enable_vertex_weighting(sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT]);

	gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::endDeferredSkinned()
{
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

	if (pass == 3)
	{
		if (is_deferred_render)
		{
			renderDeferredRiggedSimple(avatarp);
		}
		else
		{
			renderRiggedSimple(avatarp);
		}
		return;
	}

	if (pass == 4)
	{
		if (is_deferred_render)
		{
			renderDeferredRiggedBump(avatarp);
		}
		else
		{
			renderRiggedFullbright(avatarp);
		}

		return;
	}

	if (pass == 5)
	{
		renderRiggedShinySimple(avatarp);
		return;
	}

	if (pass == 6)
	{
		renderRiggedFullbrightShiny(avatarp);
		return;
	}

	if (pass >= 7 && pass < 9)
	{
		LLGLEnable blend(GL_BLEND);

		gGL.setColorMask(true, true);
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA,
					  LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
					  LLRender::BF_ZERO,
					  LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

		
		if (pass == 7)
		{
			renderRiggedAlpha(avatarp);
			return;
		}

		if (pass == 8)
		{
			renderRiggedFullbrightAlpha(avatarp);
			return;
		}
	}

	if (pass == 9)
	{
		LLGLEnable blend(GL_BLEND);
		LLGLDisable test(GL_ALPHA_TEST);
		gGL.flush();

		LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		gGL.setSceneBlendType(LLRender::BT_ADD);

		LLGLDepthTest depth(GL_TRUE, GL_FALSE);
		gGL.setColorMask(false, true);

		renderRiggedGlow(avatarp);
		gGL.setColorMask(true, false);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
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

void LLDrawPoolAvatar::updateRiggedFaceVertexBuffer(LLFace* face, const LLMeshSkinInfo* skin, LLVolume* volume, const LLVolumeFace& vol_face)
{
	U32 data_mask = 0;
	for (U32 i = 0; i < face->mRiggedIndex.size(); ++i)
	{
		if (face->mRiggedIndex[i] > -1)
		{
			data_mask |= rigged_data_mask[i];
		}
	}

	LLVertexBuffer* buff = face->mVertexBuffer;

	if (!buff || 
		buff->getTypeMask() != data_mask ||
		buff->getRequestedVerts() != vol_face.mVertices.size())
	{
		face->setGeomIndex(0);
		face->setIndicesIndex(0);
		face->setSize(vol_face.mVertices.size(), vol_face.mIndices.size());

		face->mVertexBuffer = new LLVertexBuffer(data_mask, 0);
		face->mVertexBuffer->allocateBuffer(vol_face.mVertices.size(), vol_face.mIndices.size(), true);

		U16 offset = 0;
		
		LLMatrix4 mat_vert = skin->mBindShapeMatrix;
		glh::matrix4f m((F32*) mat_vert.mMatrix);
		m = m.inverse().transpose();
		
		F32 mat3[] = 
		{ m.m[0], m.m[1], m.m[2],
		  m.m[4], m.m[5], m.m[6],
		  m.m[8], m.m[9], m.m[10] };

		LLMatrix3 mat_normal(mat3);				

		face->getGeometryVolume(*volume, face->getTEOffset(), mat_vert, mat_normal, offset, true);
		buff = face->mVertexBuffer;
	}
}

void LLDrawPoolAvatar::renderRigged(LLVOAvatar* avatar, U32 type, bool glow)
{
	for (U32 i = 0; i < mRiggedFace[type].size(); ++i)
	{
		LLFace* face = mRiggedFace[type][i];
		LLDrawable* drawable = face->getDrawable();
		if (!drawable)
		{
			continue;
		}

		LLVOVolume* vobj = drawable->getVOVolume();

		if (!vobj)
		{
			continue;
		}

		LLVolume* volume = vobj->getVolume();
		S32 te = face->getTEOffset();

		if (!volume || volume->getNumVolumeFaces() <= te)
		{
			continue;
		}

		LLUUID mesh_id = volume->getParams().getSculptID();
		if (mesh_id.isNull())
		{
			continue;
		}

		const LLMeshSkinInfo* skin = gMeshRepo.getSkinInfo(mesh_id);
		if (!skin)
		{
			continue;
		}

		const LLVolumeFace& vol_face = volume->getVolumeFace(te);
		updateRiggedFaceVertexBuffer(face, skin, volume, vol_face);
		
		U32 data_mask = rigged_data_mask[type];

		LLVertexBuffer* buff = face->mVertexBuffer;

		if (buff)
		{
			LLMatrix4 mat[64];

			for (U32 i = 0; i < skin->mJointNames.size(); ++i)
			{
				LLJoint* joint = avatar->getJoint(skin->mJointNames[i]);
				if (joint)
				{
					mat[i] = skin->mInvBindMatrix[i];
					mat[i] *= joint->getWorldMatrix();
				}
			}
			
			LLDrawPoolAvatar::sVertexProgram->uniformMatrix4fv("matrixPalette", 
				skin->mJointNames.size(),
				FALSE,
				(GLfloat*) mat[0].mMatrix);
			LLDrawPoolAvatar::sVertexProgram->uniformMatrix4fv("matrixPalette[0]", 
				skin->mJointNames.size(),
				FALSE,
				(GLfloat*) mat[0].mMatrix);

			buff->setBuffer(data_mask);

			U16 start = face->getGeomStart();
			U16 end = start + face->getGeomCount()-1;
			S32 offset = face->getIndicesStart();
			U32 count = face->getIndicesCount();

			if (glow)
			{
				glColor4f(0,0,0,face->getTextureEntry()->getGlow());
			}

			gGL.getTexUnit(diffuse_channel)->bind(face->getTexture());
			if (normal_channel > -1)
			{
				LLDrawPoolBump::bindBumpMap(face, normal_channel);
			}

			if (face->mTextureMatrix)
			{
				glMatrixMode(GL_TEXTURE);
				glLoadMatrixf((F32*) face->mTextureMatrix->mMatrix);
				buff->drawRange(LLRender::TRIANGLES, start, end, count, offset);
				glLoadIdentity();
				glMatrixMode(GL_MODELVIEW);
			}
			else
			{
				buff->drawRange(LLRender::TRIANGLES, start, end, count, offset);		
			}
		}
	}
}

void LLDrawPoolAvatar::renderDeferredRiggedSimple(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_DEFERRED_SIMPLE);
}

void LLDrawPoolAvatar::renderDeferredRiggedBump(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_DEFERRED_BUMP);
}

void LLDrawPoolAvatar::renderRiggedSimple(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_SIMPLE);
}

void LLDrawPoolAvatar::renderRiggedFullbright(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_FULLBRIGHT);
}

	
void LLDrawPoolAvatar::renderRiggedShinySimple(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_SHINY);
}

void LLDrawPoolAvatar::renderRiggedFullbrightShiny(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_FULLBRIGHT_SHINY);
}

void LLDrawPoolAvatar::renderRiggedAlpha(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_ALPHA);
}

void LLDrawPoolAvatar::renderRiggedFullbrightAlpha(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_FULLBRIGHT_ALPHA);
}

void LLDrawPoolAvatar::renderRiggedGlow(LLVOAvatar* avatar)
{
	renderRigged(avatar, RIGGED_GLOW, true);
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

void LLDrawPoolAvatar::addRiggedFace(LLFace* facep, U32 type)
{
	if (facep->mRiggedIndex.empty())
	{
		facep->mRiggedIndex.resize(LLDrawPoolAvatar::NUM_RIGGED_PASSES);
		for (U32 i = 0; i < facep->mRiggedIndex.size(); ++i)
		{
			facep->mRiggedIndex[i] = -1;
		}
	}

	if (type >= NUM_RIGGED_PASSES)
	{
		llerrs << "Invalid rigged face type." << llendl;
	}

	if (facep->mRiggedIndex[type] != -1)
	{
		llerrs << "Tried to add a rigged face that's referenced elsewhere." << llendl;
	}	

	
	facep->mRiggedIndex[type] = mRiggedFace[type].size();
	facep->mDrawPoolp = this;
	mRiggedFace[type].push_back(facep);
}

void LLDrawPoolAvatar::removeRiggedFace(LLFace* facep)
{
	
	facep->mDrawPoolp = NULL;

	for (U32 i = 0; i < NUM_RIGGED_PASSES; ++i)
	{
		S32 index = facep->mRiggedIndex[i];
		
		if (index > -1)
		{
			if (mRiggedFace[i].size() > index && mRiggedFace[i][index] == facep)
			{
				facep->mRiggedIndex[i] = -1;
				mRiggedFace[i].erase(mRiggedFace[i].begin()+index);
				for (U32 j = index; j < mRiggedFace[i].size(); ++j)
				{ //bump indexes down for faces referenced after erased face
					mRiggedFace[i][j]->mRiggedIndex[i] = j;
				}
			}
			else
			{
				llerrs << "Face reference data corrupt for rigged type " << i << llendl;
			}
		}
	}
}

LLVertexBufferAvatar::LLVertexBufferAvatar()
: LLVertexBuffer(sDataMask, 
	GL_STREAM_DRAW_ARB) //avatars are always stream draw due to morph targets
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
		
		set_vertex_weights(LLDrawPoolAvatar::sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_WEIGHT], mStride, (F32*)(base + mOffsets[TYPE_WEIGHT]));

		if (sShaderLevel >= LLDrawPoolAvatar::SHADER_LEVEL_BUMP)
		{
			set_binormals(LLDrawPoolAvatar::sVertexProgram->mAttribute[LLViewerShaderMgr::BINORMAL], mStride, (LLVector3*)(base + mOffsets[TYPE_BINORMAL]));
		}
	
		if (sShaderLevel >= LLDrawPoolAvatar::SHADER_LEVEL_CLOTH)
		{
			set_vertex_clothing_weights(LLDrawPoolAvatar::sVertexProgram->mAttribute[LLViewerShaderMgr::AVATAR_CLOTHING], mStride, (LLVector4*)(base + mOffsets[TYPE_CLOTHWEIGHT]));
		}
	}
	else
	{
		LLVertexBuffer::setupVertexBuffer(data_mask);
	}
}

