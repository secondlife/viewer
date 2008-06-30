/** 
 * @file llrender.h
 * @brief LLRender definition
 *
 *	This class acts as a wrapper for OpenGL calls.
 *	The goal of this class is to minimize the number of api calls due to legacy rendering
 *	code, to define an interface for a multiple rendering API abstraction of the UI
 *	rendering, and to abstract out direct rendering calls in a way that is cleaner and easier to maintain.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
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

#ifndef LL_LLGLRENDER_H
#define LL_LLGLRENDER_H

#include "stdtypes.h"
#include "llgltypes.h"
#include "llglheaders.h"
#include "llvertexbuffer.h"

class LLTexUnit
{
public:
	typedef enum 
	{
		TB_REPLACE = 0,
		TB_ADD,
		TB_MULT,
		TB_MULT_X2,
		TB_ALPHA_BLEND,
		TB_COMBINE			// Doesn't need to be set directly, setTexture___Blend() set TB_COMBINE automatically
	} eTextureBlendType;

	typedef enum 
	{
		TBO_REPLACE = 0,			// Use Source 1
		TBO_MULT,					// Multiply: ( Source1 * Source2 )
		TBO_MULT_X2,				// Multiply then scale by 2:  ( 2.0 * ( Source1 * Source2 ) )
		TBO_MULT_X4,				// Multiply then scale by 4:  ( 4.0 * ( Source1 * Source2 ) )
		TBO_ADD,					// Add: ( Source1 + Source2 )
		TBO_ADD_SIGNED,				// Add then subtract 0.5: ( ( Source1 + Source2 ) - 0.5 )
		TBO_SUBTRACT,				// Subtract Source2 from Source1: ( Source1 - Source2 )
		TBO_LERP_VERT_ALPHA,		// Interpolate based on Vertex Alpha (VA): ( Source1 * VA + Source2 * (1-VA) )
		TBO_LERP_TEX_ALPHA,			// Interpolate based on Texture Alpha (TA): ( Source1 * TA + Source2 * (1-TA) )
		TBO_LERP_PREV_ALPHA,		// Interpolate based on Previous Alpha (PA): ( Source1 * PA + Source2 * (1-PA) )
		TBO_LERP_CONST_ALPHA,		// Interpolate based on Const Alpha (CA): ( Source1 * CA + Source2 * (1-CA) )
		TBO_LERP_VERT_COLOR			// Interpolate based on Vertex Col (VC): ( Source1 * VC + Source2 * (1-VC) )
										// *Note* TBO_LERP_VERTEX_COLOR only works with setTextureColorBlend(),
										// and falls back to TBO_LERP_VERTEX_ALPHA for setTextureAlphaBlend().
	} eTextureBlendOp;

	typedef enum 
	{
		TBS_PREV_COLOR = 0,			// Color from the previous texture stage
		TBS_PREV_ALPHA,
		TBS_ONE_MINUS_PREV_COLOR,
		TBS_ONE_MINUS_PREV_ALPHA,
		TBS_TEX_COLOR,				// Color from the texture bound to this stage
		TBS_TEX_ALPHA,
		TBS_ONE_MINUS_TEX_COLOR,
		TBS_ONE_MINUS_TEX_ALPHA,
		TBS_VERT_COLOR,				// The vertex color currently set
		TBS_VERT_ALPHA,
		TBS_ONE_MINUS_VERT_COLOR,
		TBS_ONE_MINUS_VERT_ALPHA,
		TBS_CONST_COLOR,			// The constant color value currently set
		TBS_CONST_ALPHA,
		TBS_ONE_MINUS_CONST_COLOR,
		TBS_ONE_MINUS_CONST_ALPHA
	} eTextureBlendSrc;

	LLTexUnit(U32 index);
	U32 getIndex(void);

	void enable(void);
	void disable(void);
	void activate(void);

	void bindTexture(const LLImageGL* texture);
	void unbindTexture(void);

	void setTextureBlendType(eTextureBlendType type);

	inline void setTextureColorBlend(eTextureBlendOp op, eTextureBlendSrc src1, eTextureBlendSrc src2 = TBS_PREV_COLOR)
	{ setTextureCombiner(op, src1, src2, false); }

	// NOTE: If *_COLOR enums are passed to src1 or src2, the corresponding *_ALPHA enum will be used instead.
	inline void setTextureAlphaBlend(eTextureBlendOp op, eTextureBlendSrc src1, eTextureBlendSrc src2 = TBS_PREV_ALPHA)
	{ setTextureCombiner(op, src1, src2, true); }	

private:
	U32					mIndex;
	bool				mIsEnabled;
	eTextureBlendType	mCurrBlendType;
	eTextureBlendOp		mCurrColorOp;
	eTextureBlendSrc	mCurrColorSrc1;
	eTextureBlendSrc	mCurrColorSrc2;
	eTextureBlendOp		mCurrAlphaOp;
	eTextureBlendSrc	mCurrAlphaSrc1;
	eTextureBlendSrc	mCurrAlphaSrc2;
	S32					mCurrColorScale;
	S32					mCurrAlphaScale;
	
	void debugTextureUnit(void);
	void setColorScale(S32 scale);
	void setAlphaScale(S32 scale);
	GLint getTextureSource(eTextureBlendSrc src);
	GLint getTextureSourceType(eTextureBlendSrc src, bool isAlpha = false);
	void setTextureCombiner(eTextureBlendOp op, eTextureBlendSrc src1, eTextureBlendSrc src2, bool isAlpha = false);
};

class LLRender
{
	friend class LLTexUnit;
public:
	typedef enum 
	{
		CF_NEVER = 0,
		CF_ALWAYS,
		CF_LESS,
		CF_LESS_EQUAL,
		CF_EQUAL,
		CF_NOT_EQUAL,
		CF_GREATER_EQUAL,
		CF_GREATER,
		CF_DEFAULT
	}  eCompareFunc;

	typedef enum 
	{
		BT_ALPHA = 0,
		BT_ADD,
		BT_ADD_WITH_ALPHA,	// Additive blend modulated by the fragment's alpha.
		BT_MULT,
		BT_MULT_X2,
		BT_REPLACE
	} eBlendType;

	typedef enum 
	{
		BF_ONE = 0,
		BF_ZERO,
		BF_DEST_COLOR,
		BF_SOURCE_COLOR,
		BF_ONE_MINUS_DEST_COLOR,
		BF_ONE_MINUS_SOURCE_COLOR,
		BF_DEST_ALPHA,
		BF_SOURCE_ALPHA,
		BF_ONE_MINUS_DEST_ALPHA,
		BF_ONE_MINUS_SOURCE_ALPHA
	} eBlendFactor;

	LLRender();
	~LLRender();

	void translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
	void pushMatrix();
	void popMatrix();

	void flush();

	void begin(const GLuint& mode);
	void end();
	void vertex2i(const GLint& x, const GLint& y);
	void vertex2f(const GLfloat& x, const GLfloat& y);
	void vertex3f(const GLfloat& x, const GLfloat& y, const GLfloat& z);
	void vertex2fv(const GLfloat* v);
	void vertex3fv(const GLfloat* v);
	
	void texCoord2i(const GLint& x, const GLint& y);
	void texCoord2f(const GLfloat& x, const GLfloat& y);
	void texCoord2fv(const GLfloat* tc);

	void color4ub(const GLubyte& r, const GLubyte& g, const GLubyte& b, const GLubyte& a);
	void color4f(const GLfloat& r, const GLfloat& g, const GLfloat& b, const GLfloat& a);
	void color4fv(const GLfloat* c);
	void color3f(const GLfloat& r, const GLfloat& g, const GLfloat& b);
	void color3fv(const GLfloat* c);
	void color4ubv(const GLubyte* c);

	void setColorMask(bool writeColor, bool writeAlpha);
	void setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha);
	void setSceneBlendType(eBlendType type);

	void setAlphaRejectSettings(eCompareFunc func, F32 value = 0.01f);

	void blendFunc(eBlendFactor sfactor, eBlendFactor dfactor);

	LLTexUnit* getTexUnit(U32 index);

	typedef struct Vertex
	{
		GLfloat v[3];
		GLubyte c[4];
		GLfloat uv[2];
	};

public:

private:
	U32 mCount;
	U32 mMode;
	U32 mCurrTextureUnitIndex;
	LLPointer<LLVertexBuffer> mBuffer;
	LLStrider<LLVector3> mVerticesp;
	LLStrider<LLVector2> mTexcoordsp;
	LLStrider<LLColor4U> mColorsp;
	std::vector<LLTexUnit*> mTexUnits;
};

extern F64 gGLModelView[16];
extern F64 gGLLastModelView[16];
extern F64 gGLProjection[16];
extern S32 gGLViewport[4];

extern LLRender gGL;

#endif
