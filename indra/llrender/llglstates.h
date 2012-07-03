/** 
 * @file llglstates.h
 * @brief LLGL states definitions
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
	F32 mShininess;
	LLGLSSpecular(const LLColor4& color, F32 shininess)
	{
		mShininess = shininess;
		if (mShininess > 0.0f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color.mV);
			S32 shiny = (S32)(shininess*128.f);
			shiny = llclamp(shiny,0,128);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shiny);
		}
	}
	~LLGLSSpecular()
	{
		if (mShininess > 0.f)
		{
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, LLColor4(0.f,0.f,0.f,0.f).mV);
			glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 0);
		}
	}
};

//----------------------------------------------------------------------------

#endif
