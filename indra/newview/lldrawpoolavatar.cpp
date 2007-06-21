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

#include "llagent.h"
#include "lldrawable.h"
#include "llface.h"
#include "llsky.h"
#include "llviewercamera.h"
#include "llviewerregion.h"
#include "noise.h"
#include "pipeline.h"
#include "llglslshader.h"

static U32 sDataMask = LLDrawPoolAvatar::VERTEX_DATA_MASK;
static U32 sBufferUsage = GL_STREAM_DRAW_ARB;
static U32 sShaderLevel = 0;
static LLGLSLShader* sVertexProgram = NULL;

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
static BOOL sRenderingSkinned = FALSE;

LLDrawPoolAvatar::LLDrawPoolAvatar() :
LLFacePool(POOL_AVATAR)
{
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

BOOL gRenderAvatar = TRUE;

S32 LLDrawPoolAvatar::getVertexShaderLevel() const
{
	return sShaderLevel;
	//return (S32) LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_AVATAR);
}

void LLDrawPoolAvatar::prerender()
{
	mVertexShaderLevel = LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_AVATAR);
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

S32 LLDrawPoolAvatar::getNumPasses()
{
	return 3;
}

void LLDrawPoolAvatar::render(S32 pass)
{
	LLFastTimer t(LLFastTimer::FTM_RENDER_CHARACTERS);
	renderAvatars(NULL, pass); // render all avatars
}

void LLDrawPoolAvatar::beginRenderPass(S32 pass)
{
	//reset vertex buffer mappings
	LLVertexBuffer::unbind();

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
	glDepthMask(GL_FALSE);
	gPipeline.enableLightsFullbright(LLColor4(1,1,1,1));
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolAvatar::endFootShadow()
{
	gPipeline.enableLightsDynamic(1.f);
	glDepthMask(GL_TRUE);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolAvatar::beginRigid()
{
	sVertexProgram = NULL;
	sShaderLevel = 0;
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	/*if (sShaderLevel > 0)
	{	//eyeballs render with the specular shader
		gAvatarEyeballProgram.bind();
		gMaterialIndex = gAvatarEyeballProgram.mAttribute[LLShaderMgr::MATERIAL_COLOR];
		gSpecularIndex = gAvatarEyeballProgram.mAttribute[LLShaderMgr::SPECULAR_COLOR];
	}*/
}

void LLDrawPoolAvatar::endRigid()
{
	sShaderLevel = mVertexShaderLevel;
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void LLDrawPoolAvatar::beginSkinned()
{
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	sVertexProgram = &gAvatarProgram;

	if (sShaderLevel > 0)  // for hardware blending
	{
		sRenderingSkinned = TRUE;
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		if (sShaderLevel >= SHADER_LEVEL_BUMP)
		{
			gMaterialIndex = sVertexProgram->mAttribute[LLShaderMgr::MATERIAL_COLOR];
			gSpecularIndex = sVertexProgram->mAttribute[LLShaderMgr::SPECULAR_COLOR];
		}
		sVertexProgram->bind();
		if (sShaderLevel >= SHADER_LEVEL_CLOTH)
		{
			enable_cloth_weights(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_CLOTHING]);
		}
		enable_vertex_weighting(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_WEIGHT]);

		if (sShaderLevel >= SHADER_LEVEL_BUMP)
		{
			enable_binormals(sVertexProgram->mAttribute[LLShaderMgr::BINORMAL]);
		}
		
		sVertexProgram->enableTexture(LLShaderMgr::BUMP_MAP);
		glActiveTextureARB(GL_TEXTURE0_ARB);
	}
}

void LLDrawPoolAvatar::endSkinned()
{
	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	if (sShaderLevel > 0)
	{
		sRenderingSkinned = FALSE;
		sVertexProgram->disableTexture(LLShaderMgr::BUMP_MAP);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		disable_vertex_weighting(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_WEIGHT]);
		if (sShaderLevel >= SHADER_LEVEL_BUMP)
		{
			disable_binormals(sVertexProgram->mAttribute[LLShaderMgr::BINORMAL]);
		}
		if ((sShaderLevel >= SHADER_LEVEL_CLOTH))
		{
			disable_cloth_weights(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_CLOTHING]);
		}

		sVertexProgram->unbind();
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
		avatarp = (LLVOAvatar *)facep->getDrawable()->getVObj().get();
	}

    if (avatarp->isDead() || avatarp->mDrawable.isNull())
	{
		return;
	}

    LLOverrideFaceColor color(this, 1.0f, 1.0f, 1.0f, 1.0f);
	
	if (pass == 0)
	{
		if (gPipeline.hasRenderDebugFeatureMask(LLPipeline::RENDER_DEBUG_FEATURE_FOOT_SHADOWS))
		{
			mIndicesDrawn += avatarp->renderFootShadows();	
		}
		return;
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

	if (pass == 1)
	{
		// render rigid meshes (eyeballs) first
		mIndicesDrawn += avatarp->renderRigid();

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
		return;
	}
	

	if (sShaderLevel > 0)
	{
		gAvatarMatrixParam = sVertexProgram->mUniform[LLShaderMgr::AVATAR_MATRIX];
	}
    
	if ((sShaderLevel >= SHADER_LEVEL_CLOTH))
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

		sVertexProgram->vertexAttrib4fv(LLShaderMgr::AVATAR_WIND, wind.mV);
		F32 phase = -1.f * (avatarp->mRipplePhase);

		F32 freq = 7.f + (noise1(avatarp->mRipplePhase) * 2.f);
		LLVector4 sin_params(freq, freq, freq, phase);
		sVertexProgram->vertexAttrib4fv(LLShaderMgr::AVATAR_SINWAVE, sin_params.mV);

		LLVector4 gravity(0.f, 0.f, -CLOTHING_GRAVITY_EFFECT, 0.f);
		gravity = gravity * rot_mat;
		sVertexProgram->vertexAttrib4fv(LLShaderMgr::AVATAR_GRAVITY, gravity.mV);
	}

	if( !single_avatar || (avatarp == single_avatar) )
	{
		if (LLVOAvatar::sShowCollisionVolumes)
		{
			LLGLSNoTexture no_texture;
			avatarp->renderCollisionVolumes();
		}

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
	
	if (!gRenderAvatar)
	{
		return;
	}

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

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	sVertexProgram = &gAvatarPickProgram;
	if (sShaderLevel > 0)
	{
		gAvatarMatrixParam = sVertexProgram->mUniform[LLShaderMgr::AVATAR_MATRIX];
	}
	glAlphaFunc(GL_GEQUAL, 0.2f);
	glBlendFunc(GL_ONE, GL_ZERO);

	S32 name = avatarp->mDrawable->getVObj()->mGLName;
	LLColor4U color((U8)(name >> 16), (U8)(name >> 8), (U8)name);
	glColor4ubv(color.mV);

	// render rigid meshes (eyeballs) first
	//mIndicesDrawn += avatarp->renderRigid();

	if ((sShaderLevel > 0) && !gUseGLPick)  // for hardware blending
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		sRenderingSkinned = TRUE;
		sVertexProgram->bind();
		enable_vertex_weighting(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_WEIGHT]);
	}
	
	mIndicesDrawn += avatarp->renderSkinned(AVATAR_RENDER_PASS_SINGLE);

	// if we're in software-blending, remember to set the fence _after_ we draw so we wait till this rendering is done
	if ((sShaderLevel > 0) && !gUseGLPick)
	{
		sRenderingSkinned = FALSE;
		sVertexProgram->unbind();
		disable_vertex_weighting(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_WEIGHT]);
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

LLVertexBufferAvatar::LLVertexBufferAvatar()
: LLVertexBuffer(sDataMask, 
	LLShaderMgr::getVertexShaderLevel(LLShaderMgr::SHADER_AVATAR) > 0 ?	
	GL_STATIC_DRAW_ARB : 
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
	
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD2]));
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(2,GL_FLOAT, mStride, (void*)(base + mOffsets[TYPE_TEXCOORD]));
		
		set_vertex_weights(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_WEIGHT], mStride, (F32*)(base + mOffsets[TYPE_WEIGHT]));

		if (sShaderLevel >= LLDrawPoolAvatar::SHADER_LEVEL_BUMP)
		{
			set_binormals(sVertexProgram->mAttribute[LLShaderMgr::BINORMAL], mStride, (LLVector3*)(base + mOffsets[TYPE_BINORMAL]));
		}
	
		if (sShaderLevel >= LLDrawPoolAvatar::SHADER_LEVEL_CLOTH)
		{
			set_vertex_clothing_weights(sVertexProgram->mAttribute[LLShaderMgr::AVATAR_CLOTHING], mStride, (LLVector4*)(base + mOffsets[TYPE_CLOTHWEIGHT]));
		}
	}
	else
	{
		LLVertexBuffer::setupVertexBuffer(data_mask);
	}
}

