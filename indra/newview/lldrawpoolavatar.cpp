/** 
 * @file lldrawpoolavatar.cpp
 * @brief LLDrawPoolAvatar class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lldrawpoolavatar.h"
#include "llskinningutil.h"
#include "llrender.h"

#include "llvoavatar.h"
#include "m3math.h"
#include "llmatrix4a.h"

#include "llagent.h" //for gAgent.needsRenderAvatar()
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
#include "llviewercontrol.h" // for gSavedSettings
#include "llviewertexturelist.h"

static U32 sDataMask = LLDrawPoolAvatar::VERTEX_DATA_MASK;
static U32 sBufferUsage = GL_STREAM_DRAW_ARB;
static U32 sShaderLevel = 0;

LLGLSLShader* LLDrawPoolAvatar::sVertexProgram = NULL;
BOOL	LLDrawPoolAvatar::sSkipOpaque = FALSE;
BOOL	LLDrawPoolAvatar::sSkipTransparent = FALSE;
S32 LLDrawPoolAvatar::sDiffuseChannel = 0;
F32 LLDrawPoolAvatar::sMinimumAlpha = 0.2f;

LLUUID gBlackSquareID;

static bool is_deferred_render = false;
static bool is_post_deferred_render = false;

extern BOOL gUseGLPick;

F32 CLOTHING_GRAVITY_EFFECT = 0.7f;
F32 CLOTHING_ACCEL_FORCE_FACTOR = 0.2f;

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
S32 cube_channel = -1;

static LLTrace::BlockTimerStatHandle FTM_SHADOW_AVATAR("Avatar Shadow");

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

	if (!mDrawFace.empty())
	{
		const LLFace *facep = mDrawFace[0];
		if (facep && facep->getDrawable())
		{
			LLVOAvatar* avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();
			updateRiggedVertexBuffers(avatarp);
		}
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
	LL_RECORD_BLOCK_TIME(FTM_RENDER_CHARACTERS);
	
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
	default:
		beginDeferredRiggedMaterial(pass-5);
		break;
	}
}

void LLDrawPoolAvatar::endDeferredPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_CHARACTERS);

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
	default:
		endDeferredRiggedMaterial(pass-5);
		break;
	}
}

void LLDrawPoolAvatar::renderDeferred(S32 pass)
{
	render(pass);
}

S32 LLDrawPoolAvatar::getNumPostDeferredPasses()
{
	return 10;
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
	case 9:
		beginRiggedGlow();
		break;
	default:
		beginDeferredRiggedMaterialAlpha(pass-5);
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

	sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);

	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
}

void LLDrawPoolAvatar::beginDeferredRiggedAlpha()
{
	sVertexProgram = &gDeferredSkinnedAlphaProgram;
	gPipeline.bindDeferredShader(*sVertexProgram);
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAvatar::beginDeferredRiggedMaterialAlpha(S32 pass)
{
	switch (pass)
	{
	case 0: pass = 1; break;
	case 1: pass = 5; break;
	case 2: pass = 9; break;
	default: pass = 13; break;
	}

	pass += LLMaterial::SHADER_COUNT;

	sVertexProgram = &gDeferredMaterialProgram[pass];

	if (LLPipeline::sUnderWaterRender)
	{
		sVertexProgram = &(gDeferredMaterialWaterProgram[pass]);
	}

	gPipeline.bindDeferredShader(*sVertexProgram);
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::BUMP_MAP);
	specular_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAvatar::endDeferredRiggedAlpha()
{
	LLVertexBuffer::unbind();
	gPipeline.unbindDeferredShader(*sVertexProgram);
	sDiffuseChannel = 0;
	normal_channel = -1;
	specular_channel = -1;
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
	default:
		endDeferredRiggedAlpha();
		break;
	}
}

void LLDrawPoolAvatar::endPostDeferredAlpha()
{
	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	sRenderingSkinned = FALSE;
	sSkipOpaque = FALSE;
		
	gPipeline.unbindDeferredShader(*sVertexProgram);
	sDiffuseChannel = 0;
	sShaderLevel = mVertexShaderLevel;
}

void LLDrawPoolAvatar::renderPostDeferred(S32 pass)
{
	static const S32 actual_pass[] =
	{ //map post deferred pass numbers to what render() expects
		2, //skinned
		4, // rigged fullbright
		6, //rigged fullbright shiny
		7, //rigged alpha
		8, //rigged fullbright alpha
		9, //rigged material alpha 1
		10,//rigged material alpha 2
		11,//rigged material alpha 3
		12,//rigged material alpha 4
		13, //rigged glow
	};

	S32 p = actual_pass[pass];

	if (LLPipeline::sImpostorRender)
	{ //HACK for impostors so actual pass ends up being proper pass
		p -= 2;
	}

	is_post_deferred_render = true;
	render(p);
	is_post_deferred_render = false;
}


S32 LLDrawPoolAvatar::getNumShadowPasses()
{
	return 2;
}

void LLDrawPoolAvatar::beginShadowPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_SHADOW_AVATAR);

	if (pass == 0)
	{
		sVertexProgram = &gDeferredAvatarShadowProgram;
		
		//gGL.setAlphaRejectSettings(LLRender::CF_GREATER_EQUAL, 0.2f);		

		if ((sShaderLevel > 0))  // for hardware blending
		{
			sRenderingSkinned = TRUE;
			sVertexProgram->bind();
		}

		gGL.diffuseColor4f(1,1,1,1);
	}
	else
	{
		sVertexProgram = &gDeferredAttachmentShadowProgram;
		sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
		sVertexProgram->bind();
	}
}

void LLDrawPoolAvatar::endShadowPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_SHADOW_AVATAR);
	if (pass == 0)
	{
		if (sShaderLevel > 0)
		{
			sRenderingSkinned = FALSE;
			sVertexProgram->unbind();
		}
	}
	else
	{
		LLVertexBuffer::unbind();
		sVertexProgram->unbind();
		sVertexProgram = NULL;
	}
}

void LLDrawPoolAvatar::renderShadow(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_SHADOW_AVATAR);

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
	if (impostor 
		&& LLVOAvatar::AV_DO_NOT_RENDER != avatarp->getVisualMuteSettings()
		&& LLVOAvatar::AV_ALWAYS_RENDER != avatarp->getVisualMuteSettings())
	{
		return;
	}
	
	if (pass == 0)
	{
		avatarp->renderSkinned();
	}
	else
	{
		for (U32 i = 0; i < NUM_RIGGED_PASSES; ++i)
		{
			renderRigged(avatarp, i);
		}
	}
}

S32 LLDrawPoolAvatar::getNumPasses()
{
	if (LLPipeline::sImpostorRender)
	{
		return 8;
	}
	else 
	{
		return 10;
	}
}


S32 LLDrawPoolAvatar::getNumDeferredPasses()
{
	if (LLPipeline::sImpostorRender)
	{
		return 19;
	}
	else
	{
		return 21;
	}
}


void LLDrawPoolAvatar::render(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_CHARACTERS);
	if (LLPipeline::sImpostorRender)
	{
		renderAvatars(NULL, pass+2);
		return;
	}

	renderAvatars(NULL, pass); // render all avatars
}

void LLDrawPoolAvatar::beginRenderPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_CHARACTERS);
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

	if (pass == 0)
	{ //make sure no stale colors are left over from a previous render
		gGL.diffuseColor4f(1,1,1,1);
	}
}

void LLDrawPoolAvatar::endRenderPass(S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_CHARACTERS);

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

	if (LLGLSLShader::sNoFixedFunction)
	{
		gImpostorProgram.bind();
		gImpostorProgram.setMinimumAlpha(0.01f);
	}

	gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	sDiffuseChannel = 0;
}

void LLDrawPoolAvatar::endImpostor()
{
	if (LLGLSLShader::sNoFixedFunction)
	{
		gImpostorProgram.unbind();
	}
	gPipeline.enableLightsDynamic();
}

void LLDrawPoolAvatar::beginRigid()
{
	if (gPipeline.canUseVertexShaders())
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectAlphaMaskNoColorWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectAlphaMaskNoColorProgram;
		}
		
		if (sVertexProgram != NULL)
		{	//eyeballs render with the specular shader
			sVertexProgram->bind();
			sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
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
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	sVertexProgram->bind();
	sVertexProgram->setMinimumAlpha(0.01f);
}

void LLDrawPoolAvatar::endDeferredImpostor()
{
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram->disableTexture(LLViewerShaderMgr::DEFERRED_NORMAL);
	sVertexProgram->disableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	gPipeline.unbindDeferredShader(*sVertexProgram);
   sVertexProgram = NULL;
   sDiffuseChannel = 0;
}

void LLDrawPoolAvatar::beginDeferredRigid()
{
	sVertexProgram = &gDeferredNonIndexedDiffuseAlphaMaskNoColorProgram;
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	sVertexProgram->bind();
	sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
}

void LLDrawPoolAvatar::endDeferredRigid()
{
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
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
			sVertexProgram = &gObjectAlphaMaskNoColorWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectAlphaMaskNoColorProgram;
		}
	}
	
	if (sShaderLevel > 0)  // for hardware blending
	{
		sRenderingSkinned = TRUE;

		sVertexProgram->bind();
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

	if (LLGLSLShader::sNoFixedFunction)
	{
		sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
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
	if (sShaderLevel > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gSkinnedObjectSimpleWaterProgram;
		}
		else
		{
			sVertexProgram = &gSkinnedObjectSimpleProgram;
		}
	}
	else
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectSimpleNonIndexedWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectSimpleNonIndexedProgram;
		}
	}

	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sDiffuseChannel = 0;
		sVertexProgram->bind();
	}
}

void LLDrawPoolAvatar::endRiggedSimple()
{
	LLVertexBuffer::unbind();
	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sVertexProgram->unbind();
		sVertexProgram = NULL;
	}
}

void LLDrawPoolAvatar::beginRiggedAlpha()
{
	beginRiggedSimple();
}

void LLDrawPoolAvatar::endRiggedAlpha()
{
	endRiggedSimple();
}


void LLDrawPoolAvatar::beginRiggedFullbrightAlpha()
{
	beginRiggedFullbright();
}

void LLDrawPoolAvatar::endRiggedFullbrightAlpha()
{
	endRiggedFullbright();
}

void LLDrawPoolAvatar::beginRiggedGlow()
{
	if (sShaderLevel > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gSkinnedObjectEmissiveWaterProgram;
		}
		else
		{
			sVertexProgram = &gSkinnedObjectEmissiveProgram;
		}
	}
	else
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectEmissiveNonIndexedWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectEmissiveNonIndexedProgram;
		}
	}

	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sDiffuseChannel = 0;
		sVertexProgram->bind();

		sVertexProgram->uniform1f(LLShaderMgr::TEXTURE_GAMMA, LLPipeline::sRenderDeferred ? 2.2f : 1.1f);
		F32 gamma = gSavedSettings.getF32("RenderDeferredDisplayGamma");
		sVertexProgram->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f/2.2f));
	}
}

void LLDrawPoolAvatar::endRiggedGlow()
{
	endRiggedFullbright();
}

void LLDrawPoolAvatar::beginRiggedFullbright()
{
	if (sShaderLevel > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gSkinnedObjectFullbrightWaterProgram;
		}
		else
		{
			if (LLPipeline::sRenderDeferred)
			{
				sVertexProgram = &gDeferredSkinnedFullbrightProgram;
			}
			else
			{
				sVertexProgram = &gSkinnedObjectFullbrightProgram;
			}
		}
	}
	else
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectFullbrightNonIndexedWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectFullbrightNonIndexedProgram;
		}
	}

	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sDiffuseChannel = 0;
		sVertexProgram->bind();

		if (LLPipeline::sRenderingHUDs || !LLPipeline::sRenderDeferred)
		{
			sVertexProgram->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 1.0f);
		} 
		else 
		{
			sVertexProgram->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f);

			F32 gamma = gSavedSettings.getF32("RenderDeferredDisplayGamma");
			sVertexProgram->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f/2.2f));
		}
	}
}

void LLDrawPoolAvatar::endRiggedFullbright()
{
	LLVertexBuffer::unbind();
	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sVertexProgram->unbind();
		sVertexProgram = NULL;
	}
}

void LLDrawPoolAvatar::beginRiggedShinySimple()
{
	if (sShaderLevel > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gSkinnedObjectShinySimpleWaterProgram;
		}
		else
		{
			sVertexProgram = &gSkinnedObjectShinySimpleProgram;
		}
	}
	else
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectShinyNonIndexedWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectShinyNonIndexedProgram;
		}
	}

	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sVertexProgram->bind();
		LLDrawPoolBump::bindCubeMap(sVertexProgram, 2, sDiffuseChannel, cube_channel, false);
	}
}

void LLDrawPoolAvatar::endRiggedShinySimple()
{
	LLVertexBuffer::unbind();
	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		LLDrawPoolBump::unbindCubeMap(sVertexProgram, 2, sDiffuseChannel, cube_channel, false);
		sVertexProgram->unbind();
		sVertexProgram = NULL;
	}
}

void LLDrawPoolAvatar::beginRiggedFullbrightShiny()
{
	if (sShaderLevel > 0)
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gSkinnedObjectFullbrightShinyWaterProgram;
		}
		else
		{
			if (LLPipeline::sRenderDeferred)
			{
				sVertexProgram = &gDeferredSkinnedFullbrightShinyProgram;
			}
			else
			{
				sVertexProgram = &gSkinnedObjectFullbrightShinyProgram;
			}
		}
	}
	else
	{
		if (LLPipeline::sUnderWaterRender)
		{
			sVertexProgram = &gObjectFullbrightShinyNonIndexedWaterProgram;
		}
		else
		{
			sVertexProgram = &gObjectFullbrightShinyNonIndexedProgram;
		}
	}

	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		sVertexProgram->bind();
		LLDrawPoolBump::bindCubeMap(sVertexProgram, 2, sDiffuseChannel, cube_channel, false);

		if (LLPipeline::sRenderingHUDs || !LLPipeline::sRenderDeferred)
		{
			sVertexProgram->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 1.0f);
		} 
		else 
		{
			sVertexProgram->uniform1f(LLShaderMgr::TEXTURE_GAMMA, 2.2f);
			F32 gamma = gSavedSettings.getF32("RenderDeferredDisplayGamma");
			sVertexProgram->uniform1f(LLShaderMgr::DISPLAY_GAMMA, (gamma > 0.1f) ? 1.0f / gamma : (1.0f/2.2f));
		}
	}
}

void LLDrawPoolAvatar::endRiggedFullbrightShiny()
{
	LLVertexBuffer::unbind();
	if (sShaderLevel > 0 || gPipeline.canUseVertexShaders())
	{
		LLDrawPoolBump::unbindCubeMap(sVertexProgram, 2, sDiffuseChannel, cube_channel, false);
		sVertexProgram->unbind();
		sVertexProgram = NULL;
	}
}


void LLDrawPoolAvatar::beginDeferredRiggedSimple()
{
	sVertexProgram = &gDeferredSkinnedDiffuseProgram;
	sDiffuseChannel = 0;
	sVertexProgram->bind();
}

void LLDrawPoolAvatar::endDeferredRiggedSimple()
{
	LLVertexBuffer::unbind();
	sVertexProgram->unbind();
	sVertexProgram = NULL;
}

void LLDrawPoolAvatar::beginDeferredRiggedBump()
{
	sVertexProgram = &gDeferredSkinnedBumpProgram;
	sVertexProgram->bind();
	normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::BUMP_MAP);
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
}

void LLDrawPoolAvatar::endDeferredRiggedBump()
{
	LLVertexBuffer::unbind();
	sVertexProgram->disableTexture(LLViewerShaderMgr::BUMP_MAP);
	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	sVertexProgram->unbind();
	normal_channel = -1;
	sDiffuseChannel = 0;
	sVertexProgram = NULL;
}

void LLDrawPoolAvatar::beginDeferredRiggedMaterial(S32 pass)
{
	if (pass == 1 ||
		pass == 5 ||
		pass == 9 ||
		pass == 13)
	{ //skip alpha passes
		return;
	}
	sVertexProgram = &gDeferredMaterialProgram[pass+LLMaterial::SHADER_COUNT];

	if (LLPipeline::sUnderWaterRender)
	{
		sVertexProgram = &(gDeferredMaterialWaterProgram[pass+LLMaterial::SHADER_COUNT]);
	}

	sVertexProgram->bind();
	normal_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::BUMP_MAP);
	specular_channel = sVertexProgram->enableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
}

void LLDrawPoolAvatar::endDeferredRiggedMaterial(S32 pass)
{
	if (pass == 1 ||
		pass == 5 ||
		pass == 9 ||
		pass == 13)
	{
		return;
	}

	LLVertexBuffer::unbind();
	sVertexProgram->disableTexture(LLViewerShaderMgr::BUMP_MAP);
	sVertexProgram->disableTexture(LLViewerShaderMgr::SPECULAR_MAP);
	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	sVertexProgram->unbind();
	normal_channel = -1;
	sDiffuseChannel = 0;
	sVertexProgram = NULL;
}

void LLDrawPoolAvatar::beginDeferredSkinned()
{
	sShaderLevel = mVertexShaderLevel;
	sVertexProgram = &gDeferredAvatarProgram;
	sRenderingSkinned = TRUE;

	sVertexProgram->bind();
	sVertexProgram->setMinimumAlpha(LLDrawPoolAvatar::sMinimumAlpha);
	
	sDiffuseChannel = sVertexProgram->enableTexture(LLViewerShaderMgr::DIFFUSE_MAP);
	gGL.getTexUnit(0)->activate();
}

void LLDrawPoolAvatar::endDeferredSkinned()
{
	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	sRenderingSkinned = FALSE;
	sVertexProgram->unbind();

	sVertexProgram->disableTexture(LLViewerShaderMgr::DIFFUSE_MAP);

	sShaderLevel = mVertexShaderLevel;

	gGL.getTexUnit(0)->activate();
}

static LLTrace::BlockTimerStatHandle FTM_RENDER_AVATARS("renderAvatars");


void LLDrawPoolAvatar::renderAvatars(LLVOAvatar* single_avatar, S32 pass)
{
	LL_RECORD_BLOCK_TIME(FTM_RENDER_AVATARS);

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
		if (pass==0 && (!gPipeline.hasRenderType(LLPipeline::RENDER_TYPE_PARTICLES) || LLViewerPartSim::getMaxPartCount() <= 0))
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

			 gSphere.renderGGL();
				 
			 gGL.popMatrix();
			 gGL.setColorMask(true, false);
		}
		// don't render please
		return;
	}

	BOOL impostor = avatarp->isImpostor() && !single_avatar;

	if (( avatarp->isInMuteList() 
		  || impostor 
		  || (LLVOAvatar::AV_DO_NOT_RENDER == avatarp->getVisualMuteSettings() && !avatarp->needsImpostorUpdate()) ) && pass != 0)
	{ //don't draw anything but the impostor for impostored avatars
		return;
	}
	
	if (pass == 0 && !impostor && LLPipeline::sUnderWaterRender)
	{ //don't draw foot shadows under water
		return;
	}

	if (pass == 0)
	{
		if (!LLPipeline::sReflectionRender)
		{
			LLVOAvatar::sNumVisibleAvatars++;
		}

		if (impostor || (LLVOAvatar::AV_DO_NOT_RENDER == avatarp->getVisualMuteSettings() && !avatarp->needsImpostorUpdate()))
		{
			if (LLPipeline::sRenderDeferred && !LLPipeline::sReflectionRender && avatarp->mImpostor.isComplete()) 
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
			avatarp->renderImpostor(avatarp->getMutedAVColor(), sDiffuseChannel);
		}
		return;
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

			if (LLPipeline::sRenderDeferred)
			{ //render "simple" materials
				renderRigged(avatarp, RIGGED_MATERIAL);
				renderRigged(avatarp, RIGGED_MATERIAL_ALPHA_MASK);
				renderRigged(avatarp, RIGGED_MATERIAL_ALPHA_EMISSIVE);
				renderRigged(avatarp, RIGGED_NORMMAP);
				renderRigged(avatarp, RIGGED_NORMMAP_MASK);
				renderRigged(avatarp, RIGGED_NORMMAP_EMISSIVE);	
				renderRigged(avatarp, RIGGED_SPECMAP);
				renderRigged(avatarp, RIGGED_SPECMAP_MASK);
				renderRigged(avatarp, RIGGED_SPECMAP_EMISSIVE);
				renderRigged(avatarp, RIGGED_NORMSPEC);
				renderRigged(avatarp, RIGGED_NORMSPEC_MASK);
				renderRigged(avatarp, RIGGED_NORMSPEC_EMISSIVE);
			}
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

	if (is_deferred_render && pass >= 5 && pass <= 21)
	{
		S32 p = pass-5;

		if (p != 1 &&
			p != 5 &&
			p != 9 &&
			p != 13)
		{
			renderDeferredRiggedMaterial(avatarp, p);
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

	if (pass >= 7 && pass < 13)
	{
		if (pass == 7)
		{
			renderRiggedAlpha(avatarp);

			if (LLPipeline::sRenderDeferred && !is_post_deferred_render)
			{ //render transparent materials under water
				LLGLEnable blend(GL_BLEND);

				gGL.setColorMask(true, true);
				gGL.blendFunc(LLRender::BF_SOURCE_ALPHA,
								LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
								LLRender::BF_ZERO,
								LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

				renderRigged(avatarp, RIGGED_MATERIAL_ALPHA);
				renderRigged(avatarp, RIGGED_SPECMAP_BLEND);
				renderRigged(avatarp, RIGGED_NORMMAP_BLEND);
				renderRigged(avatarp, RIGGED_NORMSPEC_BLEND);

				gGL.setColorMask(true, false);
			}
			return;
		}

		if (pass == 8)
		{
			renderRiggedFullbrightAlpha(avatarp);
			return;
		}

		if (LLPipeline::sRenderDeferred && is_post_deferred_render)
		{
			S32 p = 0;
			switch (pass)
			{
			case 9: p = 1; break;
			case 10: p = 5; break;
			case 11: p = 9; break;
			case 12: p = 13; break;
			}

			{
				LLGLEnable blend(GL_BLEND);
				renderDeferredRiggedMaterial(avatarp, p);
			}
			return;
		}
		else if (pass == 9)
		{
			renderRiggedGlow(avatarp);
			return;
		}
	}

	if (pass == 13)
	{
		renderRiggedGlow(avatarp);
		
		return;
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

		sVertexProgram->uniform4fv(LLViewerShaderMgr::AVATAR_WIND, 1, wind.mV);
		F32 phase = -1.f * (avatarp->mRipplePhase);

		F32 freq = 7.f + (noise1(avatarp->mRipplePhase) * 2.f);
		LLVector4 sin_params(freq, freq, freq, phase);
		sVertexProgram->uniform4fv(LLViewerShaderMgr::AVATAR_SINWAVE, 1, sin_params.mV);

		LLVector4 gravity(0.f, 0.f, -CLOTHING_GRAVITY_EFFECT, 0.f);
		gravity = gravity * rot_mat;
		sVertexProgram->uniform4fv(LLViewerShaderMgr::AVATAR_GRAVITY, 1, gravity.mV);
	}

	if( !single_avatar || (avatarp == single_avatar) )
	{
		avatarp->renderSkinned();
	}
}

void LLDrawPoolAvatar::getRiggedGeometry(
    LLFace* face,
    LLPointer<LLVertexBuffer>& buffer,
    U32 data_mask,
    const LLMeshSkinInfo* skin,
    LLVolume* volume,
    const LLVolumeFace& vol_face)
{
	face->setGeomIndex(0);
	face->setIndicesIndex(0);
		
	//rigged faces do not batch textures
	face->setTextureIndex(255);

	if (buffer.isNull() || buffer->getTypeMask() != data_mask || !buffer->isWriteable())
	{
        // make a new buffer
		if (sShaderLevel > 0)
		{
			buffer = new LLVertexBuffer(data_mask, GL_DYNAMIC_DRAW_ARB);
		}
		else
		{
			buffer = new LLVertexBuffer(data_mask, GL_STREAM_DRAW_ARB);
		}

		if (!buffer->allocateBuffer(vol_face.mNumVertices, vol_face.mNumIndices, true))
		{
			LL_WARNS("LLDrawPoolAvatar") << "Failed to allocate Vertex Buffer to "
				<< vol_face.mNumVertices << " vertices and "
				<< vol_face.mNumIndices << " indices" << LL_ENDL;
			// allocate dummy triangle
			buffer->allocateBuffer(1, 3, true);
			memset((U8*)buffer->getMappedData(), 0, buffer->getSize());
			memset((U8*)buffer->getMappedIndices(), 0, buffer->getIndicesSize());
		}
	}
	else
	{
        //resize existing buffer
		if(!buffer->resizeBuffer(vol_face.mNumVertices, vol_face.mNumIndices))
		{
			LL_WARNS("LLDrawPoolAvatar") << "Failed to resize Vertex Buffer to "
				<< vol_face.mNumVertices << " vertices and "
				<< vol_face.mNumIndices << " indices" << LL_ENDL;
			// allocate dummy triangle
			buffer->resizeBuffer(1, 3);
			memset((U8*)buffer->getMappedData(), 0, buffer->getSize());
			memset((U8*)buffer->getMappedIndices(), 0, buffer->getIndicesSize());
		}
	}

	face->setSize(buffer->getNumVerts(), buffer->getNumIndices());
	face->setVertexBuffer(buffer);

	U16 offset = 0;
		
	LLMatrix4 mat_vert = skin->mBindShapeMatrix;
	glh::matrix4f m((F32*) mat_vert.mMatrix);
	m = m.inverse().transpose();
		
	F32 mat3[] = 
        { m.m[0], m.m[1], m.m[2],
          m.m[4], m.m[5], m.m[6],
          m.m[8], m.m[9], m.m[10] };

	LLMatrix3 mat_normal(mat3);				

	//let getGeometryVolume know if alpha should override shiny
	U32 type = gPipeline.getPoolTypeFromTE(face->getTextureEntry(), face->getTexture());

	if (type == LLDrawPool::POOL_ALPHA)
	{
		face->setPoolType(LLDrawPool::POOL_ALPHA);
	}
	else
	{
		face->setPoolType(LLDrawPool::POOL_AVATAR);
	}

	//LL_INFOS() << "Rebuilt face " << face->getTEOffset() << " of " << face->getDrawable() << " at " << gFrameTimeSeconds << LL_ENDL;

	// Let getGeometryVolume know if a texture matrix is in play
	if (face->mTextureMatrix)
	{
		face->setState(LLFace::TEXTURE_ANIM);
	}
	else
	{
		face->clearState(LLFace::TEXTURE_ANIM);
	}
	face->getGeometryVolume(*volume, face->getTEOffset(), mat_vert, mat_normal, offset, true);

	buffer->flush();
}

void LLDrawPoolAvatar::updateRiggedFaceVertexBuffer(
    LLVOAvatar* avatar,
    LLFace* face,
    const LLMeshSkinInfo* skin,
    LLVolume* volume,
    const LLVolumeFace& vol_face)
{
	LLVector4a* weights = vol_face.mWeights;
	if (!weights)
	{
		return;
	}
    // FIXME ugly const cast
    LLSkinningUtil::scrubInvalidJoints(avatar, const_cast<LLMeshSkinInfo*>(skin));

	LLPointer<LLVertexBuffer> buffer = face->getVertexBuffer();
	LLDrawable* drawable = face->getDrawable();

	U32 data_mask = face->getRiggedVertexBufferDataMask();

    if (!vol_face.mWeightsScrubbed)
    {
        LLSkinningUtil::scrubSkinWeights(weights, vol_face.mNumVertices, skin);
        vol_face.mWeightsScrubbed = TRUE;
    }
	
	if (buffer.isNull() || 
		buffer->getTypeMask() != data_mask ||
		buffer->getNumVerts() != vol_face.mNumVertices ||
		buffer->getNumIndices() != vol_face.mNumIndices ||
		(drawable && drawable->isState(LLDrawable::REBUILD_ALL)))
	{
		if (drawable && drawable->isState(LLDrawable::REBUILD_ALL))
		{
            //rebuild EVERY face in the drawable, not just this one, to avoid missing drawable wide rebuild issues
			for (S32 i = 0; i < drawable->getNumFaces(); ++i)
			{
				LLFace* facep = drawable->getFace(i);
				U32 face_data_mask = facep->getRiggedVertexBufferDataMask();
				if (face_data_mask)
				{
					LLPointer<LLVertexBuffer> cur_buffer = facep->getVertexBuffer();
					const LLVolumeFace& cur_vol_face = volume->getVolumeFace(i);
					getRiggedGeometry(facep, cur_buffer, face_data_mask, skin, volume, cur_vol_face);
				}
			}
			drawable->clearState(LLDrawable::REBUILD_ALL);

			buffer = face->getVertexBuffer();
		}
		else
		{
			//just rebuild this face
			getRiggedGeometry(face, buffer, data_mask, skin, volume, vol_face);
		}
	}

	if (buffer.isNull() ||
		buffer->getNumVerts() != vol_face.mNumVertices ||
		buffer->getNumIndices() != vol_face.mNumIndices)
	{
		// Allocation failed
		return;
	}

	if (sShaderLevel <= 0 && face->mLastSkinTime < avatar->getLastSkinTime())
	{
		//perform software vertex skinning for this face
		LLStrider<LLVector3> position;
		LLStrider<LLVector3> normal;

		bool has_normal = buffer->hasDataType(LLVertexBuffer::TYPE_NORMAL);
		buffer->getVertexStrider(position);

		if (has_normal)
		{
			buffer->getNormalStrider(normal);
		}

		LLVector4a* pos = (LLVector4a*) position.get();

		LLVector4a* norm = has_normal ? (LLVector4a*) normal.get() : NULL;
		
		//build matrix palette
		LLMatrix4a mat[LL_MAX_JOINTS_PER_MESH_OBJECT];
        U32 count = LLSkinningUtil::getMeshJointCount(skin);
        LLSkinningUtil::initSkinningMatrixPalette((LLMatrix4*)mat, count, skin, avatar);
        LLSkinningUtil::checkSkinWeights(weights, buffer->getNumVerts(), skin);

		LLMatrix4a bind_shape_matrix;
		bind_shape_matrix.loadu(skin->mBindShapeMatrix);

        const U32 max_joints = LLSkinningUtil::getMaxJointCount();
		for (U32 j = 0; j < buffer->getNumVerts(); ++j)
		{
			LLMatrix4a final_mat;
            LLSkinningUtil::getPerVertexSkinMatrix(weights[j].getF32ptr(), mat, false, final_mat, max_joints);
			
			LLVector4a& v = vol_face.mPositions[j];

			LLVector4a t;
			LLVector4a dst;
			bind_shape_matrix.affineTransform(v, t);
			final_mat.affineTransform(t, dst);
			pos[j] = dst;

			if (norm)
			{
				LLVector4a& n = vol_face.mNormals[j];
				bind_shape_matrix.rotate(n, t);
				final_mat.rotate(t, dst);
				dst.normalize3fast();
				norm[j] = dst;
			}
		}
	}
}

void LLDrawPoolAvatar::renderRigged(LLVOAvatar* avatar, U32 type, bool glow)
{
	if (avatar->isSelf() && !gAgent.needsRenderAvatar())
	{
		return;
	}

	stop_glerror();

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

		if (!volume || volume->getNumVolumeFaces() <= te || !volume->isMeshAssetLoaded())
		{
			continue;
		}

		LLUUID mesh_id = volume->getParams().getSculptID();
		if (mesh_id.isNull())
		{
			continue;
		}

		const LLMeshSkinInfo* skin = gMeshRepo.getSkinInfo(mesh_id, vobj);
		if (!skin)
		{
			continue;
		}

		//stop_glerror();

		//const LLVolumeFace& vol_face = volume->getVolumeFace(te);
		//updateRiggedFaceVertexBuffer(avatar, face, skin, volume, vol_face);
		
		//stop_glerror();

		U32 data_mask = LLFace::getRiggedDataMask(type);

		LLVertexBuffer* buff = face->getVertexBuffer();

		if (buff)
		{
			if (sShaderLevel > 0)
			{
                // upload matrix palette to shader
				LLMatrix4a mat[LL_MAX_JOINTS_PER_MESH_OBJECT];
				U32 count = LLSkinningUtil::getMeshJointCount(skin);
                LLSkinningUtil::initSkinningMatrixPalette((LLMatrix4*)mat, count, skin, avatar);

				stop_glerror();

				F32 mp[LL_MAX_JOINTS_PER_MESH_OBJECT*12];

				for (U32 i = 0; i < count; ++i)
				{
					F32* m = (F32*) mat[i].mMatrix[0].getF32ptr();

					U32 idx = i*12;

					mp[idx+0] = m[0];
					mp[idx+1] = m[1];
					mp[idx+2] = m[2];
					mp[idx+3] = m[12];

					mp[idx+4] = m[4];
					mp[idx+5] = m[5];
					mp[idx+6] = m[6];
					mp[idx+7] = m[13];

					mp[idx+8] = m[8];
					mp[idx+9] = m[9];
					mp[idx+10] = m[10];
					mp[idx+11] = m[14];
				}

				LLDrawPoolAvatar::sVertexProgram->uniformMatrix3x4fv(LLViewerShaderMgr::AVATAR_MATRIX, 
					count,
					FALSE,
					(GLfloat*) mp);

				stop_glerror();
			}
			else
			{
				data_mask &= ~LLVertexBuffer::MAP_WEIGHT4;
			}

			U16 start = face->getGeomStart();
			U16 end = start + face->getGeomCount()-1;
			S32 offset = face->getIndicesStart();
			U32 count = face->getIndicesCount();

			/*if (glow)
			{
				gGL.diffuseColor4f(0,0,0,face->getTextureEntry()->getGlow());
			}*/

			const LLTextureEntry* te = face->getTextureEntry();
			LLMaterial* mat = te->getMaterialParams().get();

			if (mat)
			{
				//order is important here LLRender::DIFFUSE_MAP should be last, becouse it change 
				//(gGL).mCurrTextureUnitIndex
                LLViewerTexture* specular = NULL;
                if (LLPipeline::sImpostorRender)
                {
                    specular = LLViewerTextureManager::findFetchedTexture(gBlackSquareID, TEX_LIST_STANDARD);
                    llassert(NULL != specular);
                }
                else
                {
                    specular = face->getTexture(LLRender::SPECULAR_MAP);
                }
                if (specular)
                {
                    gGL.getTexUnit(specular_channel)->bind(specular);
                }
                
				gGL.getTexUnit(normal_channel)->bind(face->getTexture(LLRender::NORMAL_MAP));
				gGL.getTexUnit(sDiffuseChannel)->bind(face->getTexture(LLRender::DIFFUSE_MAP), false, true);


				LLColor4 col = mat->getSpecularLightColor();
				F32 spec = mat->getSpecularLightExponent()/255.f;

				F32 env = mat->getEnvironmentIntensity()/255.f;

				if (mat->getSpecularID().isNull())
				{
					env = te->getShiny()*0.25f;
					col.set(env,env,env,0);
					spec = env;
				}
		
				BOOL fullbright = te->getFullbright();

				sVertexProgram->uniform1f(LLShaderMgr::EMISSIVE_BRIGHTNESS, fullbright ? 1.f : 0.f);
				sVertexProgram->uniform4f(LLShaderMgr::SPECULAR_COLOR, col.mV[0], col.mV[1], col.mV[2], spec);
				sVertexProgram->uniform1f(LLShaderMgr::ENVIRONMENT_INTENSITY, env);

				if (mat->getDiffuseAlphaMode() == LLMaterial::DIFFUSE_ALPHA_MODE_MASK)
				{
					sVertexProgram->setMinimumAlpha(mat->getAlphaMaskCutoff()/255.f);
				}
				else
				{
					sVertexProgram->setMinimumAlpha(0.f);
				}

				for (U32 i = 0; i < LLRender::NUM_TEXTURE_CHANNELS; ++i)
				{
					LLViewerTexture* tex = face->getTexture(i);
					if (tex)
					{
						tex->addTextureStats(avatar->getPixelArea());
					}
				}
			}
			else
			{
				gGL.getTexUnit(sDiffuseChannel)->bind(face->getTexture());
				sVertexProgram->setMinimumAlpha(0.f);
				if (normal_channel > -1)
				{
					LLDrawPoolBump::bindBumpMap(face, normal_channel);
				}
			}

			if (face->mTextureMatrix && vobj->mTexAnimMode)
			{
				gGL.matrixMode(LLRender::MM_TEXTURE);
				gGL.loadMatrix((F32*) face->mTextureMatrix->mMatrix);
				buff->setBuffer(data_mask);
				buff->drawRange(LLRender::TRIANGLES, start, end, count, offset);
				gGL.loadIdentity();
				gGL.matrixMode(LLRender::MM_MODELVIEW);
			}
			else
			{
				buff->setBuffer(data_mask);
				buff->drawRange(LLRender::TRIANGLES, start, end, count, offset);		
			}

			gPipeline.addTrianglesDrawn(count, LLRender::TRIANGLES);
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

void LLDrawPoolAvatar::renderDeferredRiggedMaterial(LLVOAvatar* avatar, S32 pass)
{
	renderRigged(avatar, pass);
}

static LLTrace::BlockTimerStatHandle FTM_RIGGED_VBO("Rigged VBO");

void LLDrawPoolAvatar::updateRiggedVertexBuffers(LLVOAvatar* avatar)
{
	LL_RECORD_BLOCK_TIME(FTM_RIGGED_VBO);

	//update rigged vertex buffers
	for (U32 type = 0; type < NUM_RIGGED_PASSES; ++type)
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

			const LLMeshSkinInfo* skin = gMeshRepo.getSkinInfo(mesh_id, vobj);
			if (!skin)
			{
				continue;
			}

			stop_glerror();

			const LLVolumeFace& vol_face = volume->getVolumeFace(te);
			updateRiggedFaceVertexBuffer(avatar, face, skin, volume, vol_face);
		}
	}
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
	if (!mRiggedFace[RIGGED_ALPHA].empty())
	{
		LLGLEnable blend(GL_BLEND);

		gGL.setColorMask(true, true);
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA,
						LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
						LLRender::BF_ZERO,
						LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

		renderRigged(avatar, RIGGED_ALPHA);
		gGL.setColorMask(true, false);
	}
}

void LLDrawPoolAvatar::renderRiggedFullbrightAlpha(LLVOAvatar* avatar)
{
	if (!mRiggedFace[RIGGED_FULLBRIGHT_ALPHA].empty())
	{
		LLGLEnable blend(GL_BLEND);

		gGL.setColorMask(true, true);
		gGL.blendFunc(LLRender::BF_SOURCE_ALPHA,
						LLRender::BF_ONE_MINUS_SOURCE_ALPHA,
						LLRender::BF_ZERO,
						LLRender::BF_ONE_MINUS_SOURCE_ALPHA);

		renderRigged(avatar, RIGGED_FULLBRIGHT_ALPHA);
		gGL.setColorMask(true, false);
	}
}

void LLDrawPoolAvatar::renderRiggedGlow(LLVOAvatar* avatar)
{
	if (!mRiggedFace[RIGGED_GLOW].empty())
	{
		LLGLEnable blend(GL_BLEND);
		LLGLDisable test(GL_ALPHA_TEST);
		gGL.flush();

		LLGLEnable polyOffset(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -1.0f);
		gGL.setSceneBlendType(LLRender::BT_ADD);

		LLGLDepthTest depth(GL_TRUE, GL_FALSE);
		gGL.setColorMask(false, true);

		renderRigged(avatar, RIGGED_GLOW, true);

		gGL.setColorMask(true, false);
		gGL.setSceneBlendType(LLRender::BT_ALPHA);
	}
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
	if (type >= NUM_RIGGED_PASSES)
	{
		LL_ERRS() << "Invalid rigged face type." << LL_ENDL;
	}

	if (facep->getRiggedIndex(type) != -1)
	{
		LL_ERRS() << "Tried to add a rigged face that's referenced elsewhere." << LL_ENDL;
	}	
	
	facep->setRiggedIndex(type, mRiggedFace[type].size());
	facep->setPool(this);
	mRiggedFace[type].push_back(facep);
}

void LLDrawPoolAvatar::removeRiggedFace(LLFace* facep)
{
	facep->setPool(NULL);

	for (U32 i = 0; i < NUM_RIGGED_PASSES; ++i)
	{
		S32 index = facep->getRiggedIndex(i);
		
		if (index > -1)
		{
			if (mRiggedFace[i].size() > index && mRiggedFace[i][index] == facep)
			{
				facep->setRiggedIndex(i,-1);
				mRiggedFace[i].erase(mRiggedFace[i].begin()+index);
				for (U32 j = index; j < mRiggedFace[i].size(); ++j)
				{ //bump indexes down for faces referenced after erased face
					mRiggedFace[i][j]->setRiggedIndex(i, j);
				}
			}
			else
			{
				LL_ERRS() << "Face reference data corrupt for rigged type " << i
					<< ((mRiggedFace[i].size() <= index) ? "; wrong index (out of bounds)" : (mRiggedFace[i][index] != facep) ? "; wrong face pointer" : "")
					<< LL_ENDL;
			}
		}
	}
}

LLVertexBufferAvatar::LLVertexBufferAvatar()
: LLVertexBuffer(sDataMask, 
	GL_STREAM_DRAW_ARB) //avatars are always stream draw due to morph targets
{

}


