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

#ifndef LL_LLGLRENDER_H
#define LL_LLGLRENDER_H

//#include "linden_common.h"

#include "v2math.h"
#include "v3math.h"
#include "v4coloru.h"
#include "llstrider.h"
#include "llpointer.h"
#include "llglheaders.h"

class LLVertexBuffer;
class LLCubeMap;
class LLImageGL;
class LLRenderTarget;
class LLTexture ;

class LLTexUnit
{
	friend class LLRender;
public:
	typedef enum
	{
		TT_TEXTURE = 0,			// Standard 2D Texture
		TT_RECT_TEXTURE,	// Non power of 2 texture
		TT_CUBE_MAP,		// 6-sided cube map texture
		TT_NONE 		// No texture type is currently enabled
	} eTextureType;

	typedef enum
	{
		TAM_WRAP = 0,			// Standard 2D Texture
		TAM_MIRROR,				// Non power of 2 texture
		TAM_CLAMP 				// No texture type is currently enabled
	} eTextureAddressMode;

	typedef enum
	{	// Note: If mipmapping or anisotropic are not enabled or supported it should fall back gracefully
		TFO_POINT = 0,			// Equal to: min=point, mag=point, mip=none.
		TFO_BILINEAR,			// Equal to: min=linear, mag=linear, mip=point.
		TFO_TRILINEAR,			// Equal to: min=linear, mag=linear, mip=linear.
		TFO_ANISOTROPIC			// Equal to: min=anisotropic, max=anisotropic, mip=linear.
	} eTextureFilterOptions;

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

	LLTexUnit(S32 index);

	// Refreshes renderer state of the texture unit to the cached values
	// Needed when the render context has changed and invalidated the current state
	void refreshState(void);

	// returns the index of this texture unit
	S32 getIndex(void) const { return mIndex; }

	// Sets this tex unit to be the currently active one
	void activate(void); 

	// Enables this texture unit for the given texture type 
	// (automatically disables any previously enabled texture type)
	void enable(eTextureType type); 

	// Disables the current texture unit
	void disable(void);	
	
	// Binds the LLImageGL to this texture unit 
	// (automatically enables the unit for the LLImageGL's texture type)
	bool bind(LLImageGL* texture, bool for_rendering = false, bool forceBind = false);
	bool bind(LLTexture* texture, bool for_rendering = false, bool forceBind = false);

	// Binds a cubemap to this texture unit 
	// (automatically enables the texture unit for cubemaps)
	bool bind(LLCubeMap* cubeMap);

	// Binds a render target to this texture unit 
	// (automatically enables the texture unit for the RT's texture type)
	bool bind(LLRenderTarget * renderTarget, bool bindDepth = false);

	// Manually binds a texture to the texture unit 
	// (automatically enables the tex unit for the given texture type)
	bool bindManual(eTextureType type, U32 texture, bool hasMips = false);
	
	// Unbinds the currently bound texture of the given type 
	// (only if there's a texture of the given type currently bound)
	void unbind(eTextureType type);

	// Sets the addressing mode used to sample the texture
	// Warning: this stays set for the bound texture forever, 
	// make sure you want to permanently change the address mode  for the bound texture.
	void setTextureAddressMode(eTextureAddressMode mode);

	// Sets the filtering options used to sample the texture
	// Warning: this stays set for the bound texture forever, 
	// make sure you want to permanently change the filtering for the bound texture.
	void setTextureFilteringOption(LLTexUnit::eTextureFilterOptions option);

	void setTextureBlendType(eTextureBlendType type);

	inline void setTextureColorBlend(eTextureBlendOp op, eTextureBlendSrc src1, eTextureBlendSrc src2 = TBS_PREV_COLOR)
	{ setTextureCombiner(op, src1, src2, false); }

	// NOTE: If *_COLOR enums are passed to src1 or src2, the corresponding *_ALPHA enum will be used instead.
	inline void setTextureAlphaBlend(eTextureBlendOp op, eTextureBlendSrc src1, eTextureBlendSrc src2 = TBS_PREV_ALPHA)
	{ setTextureCombiner(op, src1, src2, true); }

	static U32 getInternalType(eTextureType type);

	U32 getCurrTexture(void) { return mCurrTexture; }

	eTextureType getCurrType(void) { return mCurrTexType; }

	void setHasMipMaps(bool hasMips) { mHasMipMaps = hasMips; }

protected:
	S32					mIndex;
	U32					mCurrTexture;
	eTextureType		mCurrTexType;
	eTextureBlendType	mCurrBlendType;
	eTextureBlendOp		mCurrColorOp;
	eTextureBlendSrc	mCurrColorSrc1;
	eTextureBlendSrc	mCurrColorSrc2;
	eTextureBlendOp		mCurrAlphaOp;
	eTextureBlendSrc	mCurrAlphaSrc1;
	eTextureBlendSrc	mCurrAlphaSrc2;
	S32					mCurrColorScale;
	S32					mCurrAlphaScale;
	bool				mHasMipMaps;
	
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

	typedef enum {
		TRIANGLES = 0,
		TRIANGLE_STRIP,
		TRIANGLE_FAN,
		POINTS,
		LINES,
		LINE_STRIP,
		QUADS,
		LINE_LOOP,
		NUM_MODES
	} eGeomModes;

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
		BT_MULT_ALPHA,
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
	void shutdown();
	
	// Refreshes renderer state to the cached values
	// Needed when the render context has changed and invalidated the current state
	void refreshState(void);

	void translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
	void scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
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

	U32	getCurrentTexUnitIndex(void) const { return mCurrTextureUnitIndex; }

	bool verifyTexUnitActive(U32 unitToVerify);

	void debugTexUnits(void);

	void clearErrors();

	struct Vertex
	{
		GLfloat v[3];
		GLubyte c[4];
		GLfloat uv[2];
	};

public:

private:
	bool				mDirty;
	U32				mCount;
	U32				mMode;
	U32				mCurrTextureUnitIndex;
	bool				mCurrColorMask[4];
	eCompareFunc			mCurrAlphaFunc;
	F32				mCurrAlphaFuncVal;

	LLPointer<LLVertexBuffer>	mBuffer;
	LLStrider<LLVector3>		mVerticesp;
	LLStrider<LLVector2>		mTexcoordsp;
	LLStrider<LLColor4U>		mColorsp;
	std::vector<LLTexUnit*>		mTexUnits;
	LLTexUnit*			mDummyTexUnit;

	F32				mMaxAnisotropy;
};

extern F64 gGLModelView[16];
extern F64 gGLLastModelView[16];
extern F64 gGLLastProjection[16];
extern F64 gGLProjection[16];
extern S32 gGLViewport[4];

extern LLRender gGL;

#endif
