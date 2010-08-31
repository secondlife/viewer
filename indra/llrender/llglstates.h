/** 
 * @file llglstates.h
 * @brief LLGL states definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

//THIS HEADER SHOULD ONLY BE INCLUDED FROM llgl.h
#ifndef LL_LLGLSTATES_H
#define LL_LLGLSTATES_H

#include "llimagegl.h"

//----------------------------------------------------------------------------

class LLGLDepthTest
{
	// Enabled by default
public:
	LLGLDepthTest(GLboolean depth_enabled, GLboolean write_enabled = GL_TRUE, GLenum depth_func = GL_LEQUAL);
	
	~LLGLDepthTest();
	
	void checkState();

	GLboolean mPrevDepthEnabled;
	GLenum mPrevDepthFunc;
	GLboolean mPrevWriteEnabled;
private:
	static GLboolean sDepthEnabled; // defaults to GL_FALSE
	static GLenum sDepthFunc; // defaults to GL_LESS
	static GLboolean sWriteEnabled; // defaults to GL_TRUE
};

//----------------------------------------------------------------------------

class LLGLSDefault
{
protected:
	LLGLEnable mColorMaterial;
	LLGLDisable mAlphaTest, mBlend, mCullFace, mDither, mFog, 
		mLineSmooth, mLineStipple, mNormalize, mPolygonSmooth,
		mTextureGenQ, mTextureGenR, mTextureGenS, mTextureGenT,
		mGLMultisample;
public:
	LLGLSDefault()
		:
		// Enable
		mColorMaterial(GL_COLOR_MATERIAL),
		// Disable
		mAlphaTest(GL_ALPHA_TEST),
		mBlend(GL_BLEND), 
		mCullFace(GL_CULL_FACE),
		mDither(GL_DITHER),
		mFog(GL_FOG), 
		mLineSmooth(GL_LINE_SMOOTH),
		mLineStipple(GL_LINE_STIPPLE),
		mNormalize(GL_NORMALIZE),
		mPolygonSmooth(GL_POLYGON_SMOOTH),
		mTextureGenQ(GL_TEXTURE_GEN_Q), 
		mTextureGenR(GL_TEXTURE_GEN_R),
		mTextureGenS(GL_TEXTURE_GEN_S), 
		mTextureGenT(GL_TEXTURE_GEN_T),
		mGLMultisample(GL_MULTISAMPLE_ARB)
	{ }
};

class LLGLSObjectSelect
{ 
protected:
	LLGLDisable mBlend, mFog, mAlphaTest;
	LLGLEnable mCullFace;
public:
	LLGLSObjectSelect()
		: mBlend(GL_BLEND), mFog(GL_FOG), 
		  mAlphaTest(GL_ALPHA_TEST),
		  mCullFace(GL_CULL_FACE)
	{ }
};

class LLGLSObjectSelectAlpha
{
protected:
	LLGLEnable mAlphaTest;
public:
	LLGLSObjectSelectAlpha()
		: mAlphaTest(GL_ALPHA_TEST)
	{}
};

//----------------------------------------------------------------------------

class LLGLSUIDefault
{ 
protected:
	LLGLEnable mBlend, mAlphaTest;
	LLGLDisable mCullFace;
	LLGLDepthTest mDepthTest;
public:
	LLGLSUIDefault() 
		: mBlend(GL_BLEND), mAlphaTest(GL_ALPHA_TEST),
		  mCullFace(GL_CULL_FACE),
		  mDepthTest(GL_FALSE, GL_TRUE, GL_LEQUAL)
	{}
};

class LLGLSNoAlphaTest // : public LLGLSUIDefault
{
protected:
	LLGLDisable mAlphaTest;
public:
	LLGLSNoAlphaTest()
		: mAlphaTest(GL_ALPHA_TEST)
	{}
};

//----------------------------------------------------------------------------

class LLGLSFog
{
protected:
	LLGLEnable mFog;
public:
	LLGLSFog()
		: mFog(GL_FOG)
	{}
};

class LLGLSNoFog
{
protected:
	LLGLDisable mFog;
public:
	LLGLSNoFog()
		: mFog(GL_FOG)
	{}
};

//----------------------------------------------------------------------------

class LLGLSPipeline
{ 
protected:
	LLGLEnable mCullFace;
	LLGLDepthTest mDepthTest;
public:
	LLGLSPipeline()
		: mCullFace(GL_CULL_FACE),
		  mDepthTest(GL_TRUE, GL_TRUE, GL_LEQUAL)
	{ }		
};

class LLGLSPipelineAlpha // : public LLGLSPipeline
{ 
protected:
	LLGLEnable mBlend, mAlphaTest;
public:
	LLGLSPipelineAlpha()
		: mBlend(GL_BLEND),
		  mAlphaTest(GL_ALPHA_TEST)
	{ }
};

class LLGLSPipelineEmbossBump
{
protected:
	LLGLDisable mFog;
public:
	LLGLSPipelineEmbossBump()
		: mFog(GL_FOG)
	{ }
};

class LLGLSPipelineSelection
{ 
protected:
	LLGLDisable mCullFace;
public:
	LLGLSPipelineSelection()
		: mCullFace(GL_CULL_FACE)
	{}
};

class LLGLSPipelineAvatar
{
protected:
	LLGLEnable mNormalize;
public:
	LLGLSPipelineAvatar()
		: mNormalize(GL_NORMALIZE)
	{}
};

class LLGLSPipelineSkyBox
{ 
protected:
	LLGLDisable mAlphaTest, mCullFace, mFog;
public:
	LLGLSPipelineSkyBox()
		: mAlphaTest(GL_ALPHA_TEST), mCullFace(GL_CULL_FACE), mFog(GL_FOG)
	{ }
};

class LLGLSTracker
{
protected:
	LLGLEnable mCullFace, mBlend, mAlphaTest;
public:
	LLGLSTracker() :
		mCullFace(GL_CULL_FACE),
		mBlend(GL_BLEND),
		mAlphaTest(GL_ALPHA_TEST)
		
	{ }
};

//----------------------------------------------------------------------------

class LLGLSSpecular
{
public:
	LLGLSSpecular(const LLColor4& color, F32 shininess)
	{
		if (shininess > 0.0f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color.mV);
			S32 shiny = (S32)(shininess*128.f);
			shiny = llclamp(shiny,0,128);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shiny);
		}
	}
	~LLGLSSpecular()
	{
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, LLColor4(0.f,0.f,0.f,0.f).mV);
		glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 0);
	}
};

//----------------------------------------------------------------------------


class LLGLSBlendFunc : public LLGLSPipeline {
protected:
	GLint mSavedSrc, mSavedDst;
	LLGLEnable mBlend;

public:
	LLGLSBlendFunc(GLenum srcFunc, GLenum dstFunc) :
		mBlend(GL_BLEND)
	{
		glGetIntegerv(GL_BLEND_SRC, &mSavedSrc);
		glGetIntegerv(GL_BLEND_DST, &mSavedDst);
		glBlendFunc(srcFunc, dstFunc);
	}

	~LLGLSBlendFunc(void) {
		glBlendFunc(mSavedSrc, mSavedDst);
	}
};


#endif
