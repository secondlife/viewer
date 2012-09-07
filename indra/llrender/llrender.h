/** 
 * @file llrender.h
 * @brief LLRender definition
 *
 *	This class acts as a wrapper for OpenGL calls.
 *	The goal of this class is to minimize the number of api calls due to legacy rendering
 *	code, to define an interface for a multiple rendering API abstraction of the UI
 *	rendering, and to abstract out direct rendering calls in a way that is cleaner and easier to maintain.
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

#ifndef LL_LLGLRENDER_H
#define LL_LLGLRENDER_H

//#include "linden_common.h"

#include "v2math.h"
#include "v3math.h"
#include "v4coloru.h"
#include "v4math.h"
#include "llstrider.h"
#include "llpointer.h"
#include "llglheaders.h"
#include "llmatrix4a.h"
#include "glh/glh_linear.h"

class LLVertexBuffer;
class LLCubeMap;
class LLImageGL;
class LLRenderTarget;
class LLTexture ;

#define LL_MATRIX_STACK_DEPTH 32

class LLTexUnit
{
	friend class LLRender;
public:
	static U32 sWhiteTexture;

	typedef enum
	{
		TT_TEXTURE = 0,			// Standard 2D Texture
		TT_RECT_TEXTURE,	// Non power of 2 texture
		TT_CUBE_MAP,		// 6-sided cube map texture
		TT_MULTISAMPLE_TEXTURE, // see GL_ARB_texture_multisample
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

class LLLightState
{
public:
	LLLightState(S32 index);

	void enable();
	void disable();
	void setDiffuse(const LLColor4& diffuse);
	void setAmbient(const LLColor4& ambient);
	void setSpecular(const LLColor4& specular);
	void setPosition(const LLVector4& position);
	void setConstantAttenuation(const F32& atten);
	void setLinearAttenuation(const F32& atten);
	void setQuadraticAttenuation(const F32& atten);
	void setSpotExponent(const F32& exponent);
	void setSpotCutoff(const F32& cutoff);
	void setSpotDirection(const LLVector3& direction);

protected:
	friend class LLRender;

	S32 mIndex;
	bool mEnabled;
	LLColor4 mDiffuse;
	LLColor4 mAmbient;
	LLColor4 mSpecular;
	LLVector4 mPosition;
	LLVector3 mSpotDirection;

	F32 mConstantAtten;
	F32 mLinearAtten;
	F32 mQuadraticAtten;

	F32 mSpotExponent;
	F32 mSpotCutoff;
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
		BF_ONE_MINUS_SOURCE_ALPHA,

		BF_UNDEF
	} eBlendFactor;

	typedef enum
	{
		MM_MODELVIEW = 0,
		MM_PROJECTION,
		MM_TEXTURE0,
		MM_TEXTURE1,
		MM_TEXTURE2,
		MM_TEXTURE3,
		NUM_MATRIX_MODES,
		MM_TEXTURE
	} eMatrixMode;

	LLRender();
	~LLRender();
	void init() ;
	void shutdown();
	
	// Refreshes renderer state to the cached values
	// Needed when the render context has changed and invalidated the current state
	void refreshState(void);

	void translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
	void scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
	void rotatef(const GLfloat& a, const GLfloat& x, const GLfloat& y, const GLfloat& z);
	void ortho(F32 left, F32 right, F32 bottom, F32 top, F32 zNear, F32 zFar);

	void pushMatrix();
	void popMatrix();
	void loadMatrix(const GLfloat* m);
	void loadIdentity();
	void multMatrix(const GLfloat* m);
	void matrixMode(U32 mode);	
	U32 getMatrixMode();

	const glh::matrix4f& getModelviewMatrix();
	const glh::matrix4f& getProjectionMatrix();

	void syncMatrices();
	void syncLightState();

	void translateUI(F32 x, F32 y, F32 z);
	void scaleUI(F32 x, F32 y, F32 z);
	void pushUIMatrix();
	void popUIMatrix();
	void loadUIIdentity();
	LLVector3 getUITranslation();
	LLVector3 getUIScale();

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

	void diffuseColor3f(F32 r, F32 g, F32 b);
	void diffuseColor3fv(const F32* c);
	void diffuseColor4f(F32 r, F32 g, F32 b, F32 a);
	void diffuseColor4fv(const F32* c);
	void diffuseColor4ubv(const U8* c);

	void vertexBatchPreTransformed(LLVector3* verts, S32 vert_count);
	void vertexBatchPreTransformed(LLVector3* verts, LLVector2* uvs, S32 vert_count);
	void vertexBatchPreTransformed(LLVector3* verts, LLVector2* uvs, LLColor4U*, S32 vert_count);

	void setColorMask(bool writeColor, bool writeAlpha);
	void setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha);
	void setSceneBlendType(eBlendType type);

	void setAlphaRejectSettings(eCompareFunc func, F32 value = 0.01f);

	// applies blend func to both color and alpha
	void blendFunc(eBlendFactor sfactor, eBlendFactor dfactor);
	// applies separate blend functions to color and alpha
	void blendFunc(eBlendFactor color_sfactor, eBlendFactor color_dfactor,
		       eBlendFactor alpha_sfactor, eBlendFactor alpha_dfactor);

	LLLightState* getLight(U32 index);
	void setAmbientLightColor(const LLColor4& color);
	
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
	static U32 sUICalls;
	static U32 sUIVerts;
	static bool sGLCoreProfile;
	
private:
	friend class LLLightState;

	U32 mMatrixMode;
	U32 mMatIdx[NUM_MATRIX_MODES];
	U32 mMatHash[NUM_MATRIX_MODES];
	glh::matrix4f mMatrix[NUM_MATRIX_MODES][LL_MATRIX_STACK_DEPTH];
	U32 mCurMatHash[NUM_MATRIX_MODES];
	U32 mLightHash;
	LLColor4 mAmbientLightColor;
	
	bool			mDirty;
	U32				mQuadCycle;
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
	std::vector<LLLightState*> mLightState;

	eBlendFactor mCurrBlendColorSFactor;
	eBlendFactor mCurrBlendColorDFactor;
	eBlendFactor mCurrBlendAlphaSFactor;
	eBlendFactor mCurrBlendAlphaDFactor;

	F32				mMaxAnisotropy;

	std::vector<LLVector3> mUIOffset;
	std::vector<LLVector3> mUIScale;

};

extern F32 gGLModelView[16];
extern F32 gGLLastModelView[16];
extern F32 gGLLastProjection[16];
extern F32 gGLProjection[16];
extern S32 gGLViewport[4];

extern LLRender gGL;

#endif
