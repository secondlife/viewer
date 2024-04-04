 /** 
 * @file llrender.cpp
 * @brief LLRender implementation
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

#include "linden_common.h"

#include "llrender.h"

#include "llvertexbuffer.h"
#include "llcubemap.h"
#include "llglslshader.h"
#include "llimagegl.h"
#include "llrendertarget.h"
#include "lltexture.h"
#include "llshadermgr.h"
#include "hbxxh.h"

#if LL_WINDOWS
extern void APIENTRY gl_debug_callback(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message,
                                GLvoid* userParam)
;
#endif

thread_local LLRender gGL;

// Handy copies of last good GL matrices
F32	gGLModelView[16];
F32	gGLLastModelView[16];
F32 gGLLastProjection[16];
F32 gGLProjection[16];

// transform from last frame's camera space to this frame's camera space (and inverse)
F32 gGLDeltaModelView[16];
F32 gGLInverseDeltaModelView[16];

S32	gGLViewport[4];


U32 LLRender::sUICalls = 0;
U32 LLRender::sUIVerts = 0;
U32 LLTexUnit::sWhiteTexture = 0;
bool LLRender::sGLCoreProfile = false;
bool LLRender::sNsightDebugSupport = false;
LLVector2 LLRender::sUIGLScaleFactor = LLVector2(1.f, 1.f);

struct LLVBCache
{
    LLPointer<LLVertexBuffer> vb;
    std::chrono::steady_clock::time_point touched;
};

static std::unordered_map<U64, LLVBCache> sVBCache;

static const GLenum sGLTextureType[] =
{
	GL_TEXTURE_2D,
	GL_TEXTURE_RECTANGLE,
	GL_TEXTURE_CUBE_MAP,
    GL_TEXTURE_CUBE_MAP_ARRAY,
	GL_TEXTURE_2D_MULTISAMPLE,
    GL_TEXTURE_3D
};

static const GLint sGLAddressMode[] =
{	
	GL_REPEAT,
	GL_MIRRORED_REPEAT,
	GL_CLAMP_TO_EDGE
};

const U32 immediate_mask = LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_COLOR | LLVertexBuffer::MAP_TEXCOORD0;

static const GLenum sGLBlendFactor[] =
{
	GL_ONE,
	GL_ZERO,
	GL_DST_COLOR,
	GL_SRC_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_DST_ALPHA,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,

	GL_ZERO // 'BF_UNDEF'
};

LLTexUnit::LLTexUnit(S32 index)
	: mCurrTexType(TT_NONE),
    mCurrColorScale(1), mCurrAlphaScale(1), mCurrTexture(0), mTexColorSpace(TCS_LINEAR),
	mHasMipMaps(false),
	mIndex(index)
{
	llassert_always(index < (S32)LL_NUM_TEXTURE_LAYERS);
}

//static
U32 LLTexUnit::getInternalType(eTextureType type)
{
	return sGLTextureType[type];
}

void LLTexUnit::refreshState(void)
{
	// We set dirty to true so that the tex unit knows to ignore caching
	// and we reset the cached tex unit state

	gGL.flush();
	
	glActiveTexture(GL_TEXTURE0 + mIndex);

	if (mCurrTexType != TT_NONE)
	{
		glBindTexture(sGLTextureType[mCurrTexType], mCurrTexture);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);	
	}

    setTextureColorSpace(mTexColorSpace);
}

void LLTexUnit::activate(void)
{
	if (mIndex < 0) return;

	if ((S32)gGL.mCurrTextureUnitIndex != mIndex || gGL.mDirty)
	{
		gGL.flush();
		glActiveTexture(GL_TEXTURE0 + mIndex);
		gGL.mCurrTextureUnitIndex = mIndex;
	}
}

void LLTexUnit::enable(eTextureType type)
{
	if (mIndex < 0) return;

	if ( (mCurrTexType != type || gGL.mDirty) && (type != TT_NONE) )
	{
		activate();
		if (mCurrTexType != TT_NONE && !gGL.mDirty)
		{
			disable(); // Force a disable of a previous texture type if it's enabled.
		}
		mCurrTexType = type;

		gGL.flush();
	}
}

void LLTexUnit::disable(void)
{
	if (mIndex < 0) return;

	if (mCurrTexType != TT_NONE)
	{
		unbind(mCurrTexType);
		mCurrTexType = TT_NONE;
	}
}

void LLTexUnit::bindFast(LLTexture* texture)
{
    LLImageGL* gl_tex = texture->getGLTexture();
    texture->setActive();
    glActiveTexture(GL_TEXTURE0 + mIndex);
    gGL.mCurrTextureUnitIndex = mIndex;
    mCurrTexture = gl_tex->getTexName();
    if (!mCurrTexture)
    {
        LL_PROFILE_ZONE_NAMED("MISSING TEXTURE");
        //if deleted, will re-generate it immediately
        texture->forceImmediateUpdate();
        gl_tex->forceUpdateBindStats();
        texture->bindDefaultImage(mIndex);
    }
    glBindTexture(sGLTextureType[gl_tex->getTarget()], mCurrTexture);
    mHasMipMaps = gl_tex->mHasMipMaps;
}

bool LLTexUnit::bind(LLTexture* texture, bool for_rendering, bool forceBind)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
	stop_glerror();
	if (mIndex >= 0)
	{
		gGL.flush();

		LLImageGL* gl_tex = NULL ;

		if (texture != NULL && (gl_tex = texture->getGLTexture()))
		{
			if (gl_tex->getTexName()) //if texture exists
			{
				//in audit, replace the selected texture by the default one.
				if ((mCurrTexture != gl_tex->getTexName()) || forceBind)
				{
					activate();
					enable(gl_tex->getTarget());
					mCurrTexture = gl_tex->getTexName();
					glBindTexture(sGLTextureType[gl_tex->getTarget()], mCurrTexture);
					if(gl_tex->updateBindStats())
					{
						texture->setActive() ;
						texture->updateBindStatsForTester() ;
					}
					mHasMipMaps = gl_tex->mHasMipMaps;
					if (gl_tex->mTexOptionsDirty)
					{
						gl_tex->mTexOptionsDirty = false;
						setTextureAddressMode(gl_tex->mAddressMode);
						setTextureFilteringOption(gl_tex->mFilterOption);
                    }
                    setTextureColorSpace(mTexColorSpace);
				}
			}
			else
			{
				//if deleted, will re-generate it immediately
				texture->forceImmediateUpdate() ;

				gl_tex->forceUpdateBindStats() ;
				return texture->bindDefaultImage(mIndex);
			}
		}
		else
		{
			if (texture)
			{
				LL_DEBUGS() << "NULL LLTexUnit::bind GL image" << LL_ENDL;
			}
			else
			{
				LL_DEBUGS() << "NULL LLTexUnit::bind texture" << LL_ENDL;
			}
			return false;
		}
	}
	else
	{ // mIndex < 0
		return false;
	}

	return true;
}

bool LLTexUnit::bind(LLImageGL* texture, bool for_rendering, bool forceBind, S32 usename)
{
	stop_glerror();
	if (mIndex < 0) return false;

    U32 texname = usename ? usename : texture->getTexName();

	if(!texture)
	{
		LL_DEBUGS() << "NULL LLTexUnit::bind texture" << LL_ENDL;
		return false;
	}

	if(!texname)
	{
		if(LLImageGL::sDefaultGLTexture && LLImageGL::sDefaultGLTexture->getTexName())
		{
			return bind(LLImageGL::sDefaultGLTexture) ;
		}
		stop_glerror();
		return false ;
	}

	if ((mCurrTexture != texname) || forceBind)
	{
		gGL.flush();
		stop_glerror();
		activate();
		stop_glerror();
		enable(texture->getTarget());
		stop_glerror();
		mCurrTexture = texname;
		glBindTexture(sGLTextureType[texture->getTarget()], mCurrTexture);
		stop_glerror();
        texture->updateBindStats();
		mHasMipMaps = texture->mHasMipMaps;
		if (texture->mTexOptionsDirty)
		{
			stop_glerror();
			texture->mTexOptionsDirty = false;
			setTextureAddressMode(texture->mAddressMode);
			setTextureFilteringOption(texture->mFilterOption);
			stop_glerror();
		}
        setTextureColorSpace(mTexColorSpace);
	}

	stop_glerror();

	return true;
}

bool LLTexUnit::bind(LLCubeMap* cubeMap)
{
	if (mIndex < 0) return false;

	gGL.flush();

	if (cubeMap == NULL)
	{
		LL_WARNS() << "NULL LLTexUnit::bind cubemap" << LL_ENDL;
		return false;
	}

	if (mCurrTexture != cubeMap->mImages[0]->getTexName())
	{
		if (LLCubeMap::sUseCubeMaps)
		{
			activate();
			enable(LLTexUnit::TT_CUBE_MAP);
            mCurrTexture = cubeMap->mImages[0]->getTexName();
			glBindTexture(GL_TEXTURE_CUBE_MAP, mCurrTexture);
			mHasMipMaps = cubeMap->mImages[0]->mHasMipMaps;
			cubeMap->mImages[0]->updateBindStats();
			if (cubeMap->mImages[0]->mTexOptionsDirty)
			{
				cubeMap->mImages[0]->mTexOptionsDirty = false;
				setTextureAddressMode(cubeMap->mImages[0]->mAddressMode);
				setTextureFilteringOption(cubeMap->mImages[0]->mFilterOption);
            }
            setTextureColorSpace(mTexColorSpace);
			return true;
		}
		else
		{
			LL_WARNS() << "Using cube map without extension!" << LL_ENDL;
			return false;
		}
	}
	return true;
}

// LLRenderTarget is unavailible on the mapserver since it uses FBOs.
bool LLTexUnit::bind(LLRenderTarget* renderTarget, bool bindDepth)
{
	if (mIndex < 0) return false;

	gGL.flush();

	if (bindDepth)
	{
        llassert(renderTarget->getDepth()); // target MUST have a depth buffer attachment

		bindManual(renderTarget->getUsage(), renderTarget->getDepth());
	}
	else
	{
		bindManual(renderTarget->getUsage(), renderTarget->getTexture());
	}

	return true;
}

bool LLTexUnit::bindManual(eTextureType type, U32 texture, bool hasMips)
{
	if (mIndex < 0)  
	{
		return false;
	}
	
	if(mCurrTexture != texture)
	{
		gGL.flush();
		
		activate();
		enable(type);
		mCurrTexture = texture;
		glBindTexture(sGLTextureType[type], texture);
        mHasMipMaps = hasMips;
        setTextureColorSpace(mTexColorSpace);
	}
	return true;
}

void LLTexUnit::unbind(eTextureType type)
{
	stop_glerror();

	if (mIndex < 0) return;

	//always flush and activate for consistency 
	//   some code paths assume unbind always flushes and sets the active texture
	gGL.flush();
	activate();

	// Disabled caching of binding state.
	if (mCurrTexType == type)
	{
		mCurrTexture = 0;

        // Always make sure our texture color space is reset to linear.  SRGB sampling should be opt-in in the vast majority of cases.  Also prevents color space "popping".
        mTexColorSpace = TCS_LINEAR;
		if (type == LLTexUnit::TT_TEXTURE)
		{
			glBindTexture(sGLTextureType[type], sWhiteTexture);
		}
		else
		{
			glBindTexture(sGLTextureType[type], 0);
		}
		stop_glerror();
	}
}

void LLTexUnit::unbindFast(eTextureType type)
{
    activate();

    // Disabled caching of binding state.
    if (mCurrTexType == type)
    {
        mCurrTexture = 0;

        // Always make sure our texture color space is reset to linear.  SRGB sampling should be opt-in in the vast majority of cases.  Also prevents color space "popping".
        mTexColorSpace = TCS_LINEAR;
        if (type == LLTexUnit::TT_TEXTURE)
        {
            glBindTexture(sGLTextureType[type], sWhiteTexture);
        }
        else
        {
            glBindTexture(sGLTextureType[type], 0);
        }
    }
}

void LLTexUnit::setTextureAddressMode(eTextureAddressMode mode)
{
	if (mIndex < 0 || mCurrTexture == 0) return;

	gGL.flush();

	activate();

	glTexParameteri (sGLTextureType[mCurrTexType], GL_TEXTURE_WRAP_S, sGLAddressMode[mode]);
	glTexParameteri (sGLTextureType[mCurrTexType], GL_TEXTURE_WRAP_T, sGLAddressMode[mode]);
	if (mCurrTexType == TT_CUBE_MAP)
	{
		glTexParameteri (GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, sGLAddressMode[mode]);
	}
}

void LLTexUnit::setTextureFilteringOption(LLTexUnit::eTextureFilterOptions option)
{
	if (mIndex < 0 || mCurrTexture == 0 || mCurrTexType == LLTexUnit::TT_MULTISAMPLE_TEXTURE) return;

	gGL.flush();

	if (option == TFO_POINT)
	{
		glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (option >= TFO_TRILINEAR && mHasMipMaps)
	{
		glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	} 
	else if (option >= TFO_BILINEAR)
	{
		if (mHasMipMaps)
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}
		else
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
	else
	{
		if (mHasMipMaps)
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
		else
		{
			glTexParameteri(sGLTextureType[mCurrTexType], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	if (gGLManager.mGLVersion >= 4.59f)
	{
		if (LLImageGL::sGlobalUseAnisotropic && option == TFO_ANISOTROPIC)
		{
			glTexParameterf(sGLTextureType[mCurrTexType], GL_TEXTURE_MAX_ANISOTROPY, gGLManager.mMaxAnisotropy);
		}
		else
		{
			glTexParameterf(sGLTextureType[mCurrTexType], GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		}
	}
}

GLint LLTexUnit::getTextureSource(eTextureBlendSrc src)
{
	switch(src)
	{
		// All four cases should return the same value.
		case TBS_PREV_COLOR:
		case TBS_PREV_ALPHA:
		case TBS_ONE_MINUS_PREV_COLOR:
		case TBS_ONE_MINUS_PREV_ALPHA:
			return GL_PREVIOUS;

		// All four cases should return the same value.
		case TBS_TEX_COLOR:
		case TBS_TEX_ALPHA:
		case TBS_ONE_MINUS_TEX_COLOR:
		case TBS_ONE_MINUS_TEX_ALPHA:
			return GL_TEXTURE;

		// All four cases should return the same value.
		case TBS_VERT_COLOR:
		case TBS_VERT_ALPHA:
		case TBS_ONE_MINUS_VERT_COLOR:
		case TBS_ONE_MINUS_VERT_ALPHA:
			return GL_PRIMARY_COLOR;

		// All four cases should return the same value.
		case TBS_CONST_COLOR:
		case TBS_CONST_ALPHA:
		case TBS_ONE_MINUS_CONST_COLOR:
		case TBS_ONE_MINUS_CONST_ALPHA:
			return GL_CONSTANT;

		default:
			LL_WARNS() << "Unknown eTextureBlendSrc: " << src << ".  Using Vertex Color instead." << LL_ENDL;
			return GL_PRIMARY_COLOR;
	}
}

GLint LLTexUnit::getTextureSourceType(eTextureBlendSrc src, bool isAlpha)
{
	switch(src)
	{
		// All four cases should return the same value.
		case TBS_PREV_COLOR:
		case TBS_TEX_COLOR:
		case TBS_VERT_COLOR:
		case TBS_CONST_COLOR:
			return (isAlpha) ? GL_SRC_ALPHA: GL_SRC_COLOR;

		// All four cases should return the same value.
		case TBS_PREV_ALPHA:
		case TBS_TEX_ALPHA:
		case TBS_VERT_ALPHA:
		case TBS_CONST_ALPHA:
			return GL_SRC_ALPHA;

		// All four cases should return the same value.
		case TBS_ONE_MINUS_PREV_COLOR:
		case TBS_ONE_MINUS_TEX_COLOR:
		case TBS_ONE_MINUS_VERT_COLOR:
		case TBS_ONE_MINUS_CONST_COLOR:
			return (isAlpha) ? GL_ONE_MINUS_SRC_ALPHA : GL_ONE_MINUS_SRC_COLOR;

		// All four cases should return the same value.
		case TBS_ONE_MINUS_PREV_ALPHA:
		case TBS_ONE_MINUS_TEX_ALPHA:
		case TBS_ONE_MINUS_VERT_ALPHA:
		case TBS_ONE_MINUS_CONST_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;

		default:
			LL_WARNS() << "Unknown eTextureBlendSrc: " << src << ".  Using Source Color or Alpha instead." << LL_ENDL;
			return (isAlpha) ? GL_SRC_ALPHA: GL_SRC_COLOR;
	}
}

void LLTexUnit::setColorScale(S32 scale)
{
	if (mCurrColorScale != scale || gGL.mDirty)
	{
		mCurrColorScale = scale;
		gGL.flush();
		glTexEnvi( GL_TEXTURE_ENV, GL_RGB_SCALE, scale );
	}
}

void LLTexUnit::setAlphaScale(S32 scale)
{
	if (mCurrAlphaScale != scale || gGL.mDirty)
	{
		mCurrAlphaScale = scale;
		gGL.flush();
		glTexEnvi( GL_TEXTURE_ENV, GL_ALPHA_SCALE, scale );
	}
}

// Useful for debugging that you've manually assigned a texture operation to the correct 
// texture unit based on the currently set active texture in opengl.
void LLTexUnit::debugTextureUnit(void)
{
	if (mIndex < 0) return;

	GLint activeTexture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
	if ((GL_TEXTURE0 + mIndex) != activeTexture)
	{
		U32 set_unit = (activeTexture - GL_TEXTURE0);
		LL_WARNS() << "Incorrect Texture Unit!  Expected: " << set_unit << " Actual: " << mIndex << LL_ENDL;
	}
}

void LLTexUnit::setTextureColorSpace(eTextureColorSpace space)
{
    mTexColorSpace = space;
}

LLLightState::LLLightState(S32 index)
: mIndex(index),
  mEnabled(false),
  mConstantAtten(1.f),
  mLinearAtten(0.f),
  mQuadraticAtten(0.f),
  mSpotExponent(0.f),
  mSpotCutoff(180.f)
{
	if (mIndex == 0)
	{
		mDiffuse.set(1,1,1,1);
        mDiffuseB.set(0,0,0,0);
		mSpecular.set(1,1,1,1);
	}

    mSunIsPrimary = true;

	mAmbient.set(0,0,0,1);
	mPosition.set(0,0,1,0);
	mSpotDirection.set(0,0,-1);
}

void LLLightState::enable()
{
	mEnabled = true;
}

void LLLightState::disable()
{
	mEnabled = false;
}

void LLLightState::setDiffuse(const LLColor4& diffuse)
{
	if (mDiffuse != diffuse)
	{
		++gGL.mLightHash;
		mDiffuse = diffuse;
	}
}

void LLLightState::setDiffuseB(const LLColor4& diffuse)
{
    if (mDiffuseB != diffuse)
	{
		++gGL.mLightHash;
		mDiffuseB = diffuse;
	}
}

void LLLightState::setSunPrimary(bool v)
{
    if (mSunIsPrimary != v)
    {
        ++gGL.mLightHash;
		mSunIsPrimary = v;
    }
}

void LLLightState::setSize(F32 v)
{
    if (mSize != v)
    {
        ++gGL.mLightHash;
        mSize = v;
    }
}

void LLLightState::setFalloff(F32 v)
{
    if (mFalloff != v)
    {
        ++gGL.mLightHash;
        mFalloff = v;
    }
}

void LLLightState::setAmbient(const LLColor4& ambient)
{
	if (mAmbient != ambient)
	{
		++gGL.mLightHash;
		mAmbient = ambient;
	}
}

void LLLightState::setSpecular(const LLColor4& specular)
{
	if (mSpecular != specular)
	{
		++gGL.mLightHash;
		mSpecular = specular;
	}
}

void LLLightState::setPosition(const LLVector4& position)
{
	//always set position because modelview matrix may have changed
	++gGL.mLightHash;
	mPosition = position;
	//transform position by current modelview matrix
	glh::vec4f pos(position.mV);
	const glh::matrix4f& mat = gGL.getModelviewMatrix();
	mat.mult_matrix_vec(pos);
	mPosition.set(pos.v);
}

void LLLightState::setConstantAttenuation(const F32& atten)
{
	if (mConstantAtten != atten)
	{
		mConstantAtten = atten;
		++gGL.mLightHash;
	}
}

void LLLightState::setLinearAttenuation(const F32& atten)
{
	if (mLinearAtten != atten)
	{
		++gGL.mLightHash;
		mLinearAtten = atten;
	}
}

void LLLightState::setQuadraticAttenuation(const F32& atten)
{
	if (mQuadraticAtten != atten)
	{
		++gGL.mLightHash;
		mQuadraticAtten = atten;
	}
}

void LLLightState::setSpotExponent(const F32& exponent)
{
	if (mSpotExponent != exponent)
	{
		++gGL.mLightHash;
		mSpotExponent = exponent;
	}
}

void LLLightState::setSpotCutoff(const F32& cutoff)
{
	if (mSpotCutoff != cutoff)
	{
		++gGL.mLightHash;
		mSpotCutoff = cutoff;
	}
}

void LLLightState::setSpotDirection(const LLVector3& direction)
{
	//always set direction because modelview matrix may have changed
	++gGL.mLightHash;
	mSpotDirection = direction;
	//transform direction by current modelview matrix
	glh::vec3f dir(direction.mV);
	const glh::matrix4f& mat = gGL.getModelviewMatrix();
	mat.mult_matrix_dir(dir);

	mSpotDirection.set(dir.v);
}

LLRender::LLRender()
  : mDirty(false),
    mCount(0),
	mQuadCycle(0),
    mMode(LLRender::TRIANGLES),
    mCurrTextureUnitIndex(0)
{	
	for (U32 i = 0; i < LL_NUM_TEXTURE_LAYERS; i++)
	{
        mTexUnits[i].mIndex = i;
	}

	for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; ++i)
	{
        mLightState[i].mIndex = i;
	}

	for (U32 i = 0; i < 4; i++)
	{
		mCurrColorMask[i] = true;
	}

	mCurrBlendColorSFactor = BF_UNDEF;
	mCurrBlendAlphaSFactor = BF_UNDEF;
	mCurrBlendColorDFactor = BF_UNDEF;
	mCurrBlendAlphaDFactor = BF_UNDEF;

	mMatrixMode = LLRender::MM_MODELVIEW;
	
	for (U32 i = 0; i < NUM_MATRIX_MODES; ++i)
	{
		mMatIdx[i] = 0;
		mMatHash[i] = 0;
		mCurMatHash[i] = 0xFFFFFFFF;
	}

	mLightHash = 0;
}

LLRender::~LLRender()
{
	shutdown();
}

bool LLRender::init(bool needs_vertex_buffer)
{
#if LL_WINDOWS
    if (gGLManager.mHasDebugOutput && gDebugGL)
    { //setup debug output callback
        //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW_ARB, 0, NULL, GL_TRUE);
        glDebugMessageCallback((GLDEBUGPROC) gl_debug_callback, NULL);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
#endif

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    gGL.setSceneBlendType(LLRender::BT_ALPHA);
    gGL.setAmbientLightColor(LLColor4::black);

    glCullFace(GL_BACK);

    // necessary for reflection maps
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#if LL_WINDOWS
    if (glGenVertexArrays == nullptr)
    {
        return false;
    }
#endif

    { //bind a dummy vertex array object so we're core profile compliant
        U32 ret;
        glGenVertexArrays(1, &ret);
        glBindVertexArray(ret);
    }

    if (needs_vertex_buffer)
    {
        initVertexBuffer();
    }
    return true;
}

void LLRender::initVertexBuffer()
{
    llassert_always(mBuffer.isNull());
    stop_glerror();
    mBuffer = new LLVertexBuffer(immediate_mask);
    mBuffer->allocateBuffer(4096, 0);
    mBuffer->getVertexStrider(mVerticesp);
    mBuffer->getTexCoord0Strider(mTexcoordsp);
    mBuffer->getColorStrider(mColorsp);
    stop_glerror();
}

void LLRender::resetVertexBuffer()
{
    mBuffer = NULL;
}

void LLRender::shutdown()
{
    resetVertexBuffer();
}

void LLRender::refreshState(void)
{
	mDirty = true;

	U32 active_unit = mCurrTextureUnitIndex;

	for (U32 i = 0; i < mTexUnits.size(); i++)
	{
		mTexUnits[i].refreshState();
	}
	
	mTexUnits[active_unit].activate();

	setColorMask(mCurrColorMask[0], mCurrColorMask[1], mCurrColorMask[2], mCurrColorMask[3]);
	
    flush();

	mDirty = false;
}

void LLRender::syncLightState()
{
    LLGLSLShader *shader = LLGLSLShader::sCurBoundShaderPtr;

    if (!shader)
    {
        return;
    }

    if (shader->mLightHash != mLightHash)
    {
        shader->mLightHash = mLightHash;

        LLVector4 position[LL_NUM_LIGHT_UNITS];
        LLVector3 direction[LL_NUM_LIGHT_UNITS];
        LLVector4 attenuation[LL_NUM_LIGHT_UNITS];
        LLVector3 diffuse[LL_NUM_LIGHT_UNITS];
        LLVector3 diffuse_b[LL_NUM_LIGHT_UNITS];
        bool      sun_primary[LL_NUM_LIGHT_UNITS];
        LLVector2 size[LL_NUM_LIGHT_UNITS];

        for (U32 i = 0; i < LL_NUM_LIGHT_UNITS; i++)
        {
            LLLightState *light = &mLightState[i];

            position[i]  = light->mPosition;
            direction[i] = light->mSpotDirection;
            attenuation[i].set(light->mLinearAtten, light->mQuadraticAtten, light->mSpecular.mV[2], light->mSpecular.mV[3]);
            diffuse[i].set(light->mDiffuse.mV);
            diffuse_b[i].set(light->mDiffuseB.mV);
            sun_primary[i] = light->mSunIsPrimary;
            size[i].set(light->mSize, light->mFalloff);
        }

        shader->uniform4fv(LLShaderMgr::LIGHT_POSITION, LL_NUM_LIGHT_UNITS, position[0].mV);
        shader->uniform3fv(LLShaderMgr::LIGHT_DIRECTION, LL_NUM_LIGHT_UNITS, direction[0].mV);
        shader->uniform4fv(LLShaderMgr::LIGHT_ATTENUATION, LL_NUM_LIGHT_UNITS, attenuation[0].mV);
        shader->uniform2fv(LLShaderMgr::LIGHT_DEFERRED_ATTENUATION, LL_NUM_LIGHT_UNITS, size[0].mV);
        shader->uniform3fv(LLShaderMgr::LIGHT_DIFFUSE, LL_NUM_LIGHT_UNITS, diffuse[0].mV);
        shader->uniform3fv(LLShaderMgr::LIGHT_AMBIENT, 1, mAmbientLightColor.mV);
        shader->uniform1i(LLShaderMgr::SUN_UP_FACTOR, sun_primary[0] ? 1 : 0);
        //shader->uniform3fv(LLShaderMgr::AMBIENT, 1, mAmbientLightColor.mV);
        //shader->uniform3fv(LLShaderMgr::SUNLIGHT_COLOR, 1, diffuse[0].mV);
        //shader->uniform3fv(LLShaderMgr::MOONLIGHT_COLOR, 1, diffuse_b[0].mV);
    }
}

void LLRender::syncMatrices()
{
    static const U32 name[] = 
	{
		LLShaderMgr::MODELVIEW_MATRIX,
		LLShaderMgr::PROJECTION_MATRIX,
		LLShaderMgr::TEXTURE_MATRIX0,
		LLShaderMgr::TEXTURE_MATRIX1,
		LLShaderMgr::TEXTURE_MATRIX2,
		LLShaderMgr::TEXTURE_MATRIX3,
	};

	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

	static glh::matrix4f cached_mvp;
    static glh::matrix4f cached_inv_mdv;
	static U32 cached_mvp_mdv_hash = 0xFFFFFFFF;
	static U32 cached_mvp_proj_hash = 0xFFFFFFFF;
	
	static glh::matrix4f cached_normal;
	static U32 cached_normal_hash = 0xFFFFFFFF;

	if (shader)
	{
		//llassert(shader);

		bool mvp_done = false;

		U32 i = MM_MODELVIEW;
		if (mMatHash[MM_MODELVIEW] != shader->mMatHash[MM_MODELVIEW])
		{ //update modelview, normal, and MVP
			glh::matrix4f& mat = mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];

            // if MDV has changed, update the cached inverse as well
            if (cached_mvp_mdv_hash != mMatHash[MM_MODELVIEW])
            {
                cached_inv_mdv = mat.inverse();
            }

			shader->uniformMatrix4fv(name[MM_MODELVIEW], 1, GL_FALSE, mat.m);
			shader->mMatHash[MM_MODELVIEW] = mMatHash[MM_MODELVIEW];

			//update normal matrix
			S32 loc = shader->getUniformLocation(LLShaderMgr::NORMAL_MATRIX);
			if (loc > -1)
			{
				if (cached_normal_hash != mMatHash[i])
				{
					cached_normal = cached_inv_mdv.transpose();
					cached_normal_hash = mMatHash[i];
				}

				glh::matrix4f& norm = cached_normal;

				F32 norm_mat[] = 
				{
					norm.m[0], norm.m[1], norm.m[2],
					norm.m[4], norm.m[5], norm.m[6],
					norm.m[8], norm.m[9], norm.m[10] 
				};

				shader->uniformMatrix3fv(LLShaderMgr::NORMAL_MATRIX, 1, GL_FALSE, norm_mat);
			}

            if (shader->getUniformLocation(LLShaderMgr::INVERSE_MODELVIEW_MATRIX))
            {                
	            shader->uniformMatrix4fv(LLShaderMgr::INVERSE_MODELVIEW_MATRIX, 1, GL_FALSE, cached_inv_mdv.m); 
            }

			//update MVP matrix
			mvp_done = true;
			loc = shader->getUniformLocation(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX);
			if (loc > -1)
			{
				U32 proj = MM_PROJECTION;

				if (cached_mvp_mdv_hash != mMatHash[i] || cached_mvp_proj_hash != mMatHash[MM_PROJECTION])
				{
					cached_mvp = mat;
					cached_mvp.mult_left(mMatrix[proj][mMatIdx[proj]]);
					cached_mvp_mdv_hash = mMatHash[i];
					cached_mvp_proj_hash = mMatHash[MM_PROJECTION];
				}

				shader->uniformMatrix4fv(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX, 1, GL_FALSE, cached_mvp.m);
			}
		}

		i = MM_PROJECTION;
		if (mMatHash[MM_PROJECTION] != shader->mMatHash[MM_PROJECTION])
		{ //update projection matrix, normal, and MVP
			glh::matrix4f& mat = mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]];

            // GZ: This was previously disabled seemingly due to a bug involving the deferred renderer's regular pushing and popping of mats.
			// We're reenabling this and cleaning up the code around that - that would've been the appropriate course initially.
			// Anything beyond the standard proj and inv proj mats are special cases.  Please setup special uniforms accordingly in the future.
            if (shader->getUniformLocation(LLShaderMgr::INVERSE_PROJECTION_MATRIX))
            {
	            glh::matrix4f inv_proj = mat.inverse();
	            shader->uniformMatrix4fv(LLShaderMgr::INVERSE_PROJECTION_MATRIX, 1, false, inv_proj.m);
            }

			// Used by some full screen effects - such as full screen lights, glow, etc.
            if (shader->getUniformLocation(LLShaderMgr::IDENTITY_MATRIX))
            {
                shader->uniformMatrix4fv(LLShaderMgr::IDENTITY_MATRIX, 1, GL_FALSE, glh::matrix4f::identity().m);
            }

			shader->uniformMatrix4fv(name[MM_PROJECTION], 1, GL_FALSE, mat.m);
			shader->mMatHash[MM_PROJECTION] = mMatHash[MM_PROJECTION];

			if (!mvp_done)
			{
				//update MVP matrix
				S32 loc = shader->getUniformLocation(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX);
				if (loc > -1)
				{
					if (cached_mvp_mdv_hash != mMatHash[MM_PROJECTION] || cached_mvp_proj_hash != mMatHash[MM_PROJECTION])
					{
						U32 mdv = MM_MODELVIEW;
						cached_mvp = mat;
						cached_mvp.mult_right(mMatrix[mdv][mMatIdx[mdv]]);
						cached_mvp_mdv_hash = mMatHash[MM_MODELVIEW];
						cached_mvp_proj_hash = mMatHash[MM_PROJECTION];
					}
									
					shader->uniformMatrix4fv(LLShaderMgr::MODELVIEW_PROJECTION_MATRIX, 1, GL_FALSE, cached_mvp.m);
				}
			}
		}

		for (i = MM_TEXTURE0; i < NUM_MATRIX_MODES; ++i)
		{
			if (mMatHash[i] != shader->mMatHash[i])
			{
				shader->uniformMatrix4fv(name[i], 1, GL_FALSE, mMatrix[i][mMatIdx[i]].m);
				shader->mMatHash[i] = mMatHash[i];
			}
		}


		if (shader->mFeatures.hasLighting || shader->mFeatures.calculatesLighting || shader->mFeatures.calculatesAtmospherics)
		{ //also sync light state
			syncLightState();
		}
	}
}

void LLRender::translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
	flush();

	{
		glh::matrix4f trans_mat(1,0,0,x,
								0,1,0,y,
								0,0,1,z,
								0,0,0,1);
	
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mult_right(trans_mat);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
	flush();
	
	{
		glh::matrix4f scale_mat(x,0,0,0,
								0,y,0,0,
								0,0,z,0,
								0,0,0,1);
	
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mult_right(scale_mat);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::ortho(F32 left, F32 right, F32 bottom, F32 top, F32 zNear, F32 zFar)
{
	flush();

	{

		glh::matrix4f ortho_mat(2.f/(right-left),0,0,	-(right+left)/(right-left),
								0,2.f/(top-bottom),0,	-(top+bottom)/(top-bottom),
								0,0,-2.f/(zFar-zNear),	-(zFar+zNear)/(zFar-zNear),
								0,0,0,1);
	
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mult_right(ortho_mat);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::rotatef(const GLfloat& a, const GLfloat& x, const GLfloat& y, const GLfloat& z)
{
	flush();

	{
		F32 r = a * DEG_TO_RAD;

		F32 c = cosf(r);
		F32 s = sinf(r);

		F32 ic = 1.f-c;

		glh::matrix4f rot_mat(x*x*ic+c,		x*y*ic-z*s,		x*z*ic+y*s,		0,
							  x*y*ic+z*s,	y*y*ic+c,		y*z*ic-x*s,		0,
							  x*z*ic-y*s,	y*z*ic+x*s,		z*z*ic+c,		0,
							  0,0,0,1);
	
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mult_right(rot_mat);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::pushMatrix()
{
	flush();
	
	{
		if (mMatIdx[mMatrixMode] < LL_MATRIX_STACK_DEPTH-1)
		{
			mMatrix[mMatrixMode][mMatIdx[mMatrixMode]+1] = mMatrix[mMatrixMode][mMatIdx[mMatrixMode]];
			++mMatIdx[mMatrixMode];
		}
		else
		{
			LL_WARNS() << "Matrix stack overflow." << LL_ENDL;
		}
	}
}

void LLRender::popMatrix()
{
	flush();
	{
		if (mMatIdx[mMatrixMode] > 0)
		{
			--mMatIdx[mMatrixMode];
			mMatHash[mMatrixMode]++;
		}
		else
		{
			LL_WARNS() << "Matrix stack underflow." << LL_ENDL;
		}
	}
}

void LLRender::loadMatrix(const GLfloat* m)
{
	flush();
	{
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].set_value((GLfloat*) m);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::multMatrix(const GLfloat* m)
{
	flush();
	{
		glh::matrix4f mat((GLfloat*) m);
	
		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].mult_right(mat);
		mMatHash[mMatrixMode]++;
	}
}

void LLRender::matrixMode(eMatrixMode mode)
{
	if (mode == MM_TEXTURE)
	{
        U32 tex_index = gGL.getCurrentTexUnitIndex();
        // the shaders don't actually reference anything beyond texture_matrix0/1 outside of terrain rendering
        llassert(tex_index <= 3);
        mode = eMatrixMode(MM_TEXTURE0 + tex_index);
        if (mode > MM_TEXTURE3)
        {
            // getCurrentTexUnitIndex() can go as high as 32 (LL_NUM_TEXTURE_LAYERS)
            // Large value will result in a crash at mMatrix
            LL_WARNS_ONCE() << "Attempted to assign matrix mode out of bounds: " << mode << LL_ENDL;
            mode = MM_TEXTURE0;
        }
	}

	mMatrixMode = mode;
}

LLRender::eMatrixMode LLRender::getMatrixMode()
{
	if (mMatrixMode >= MM_TEXTURE0 && mMatrixMode <= MM_TEXTURE3)
	{ //always return MM_TEXTURE if current matrix mode points at any texture matrix
		return MM_TEXTURE;
	}

	return mMatrixMode;
}


void LLRender::loadIdentity()
{
	flush();

	{
		llassert_always(mMatrixMode < NUM_MATRIX_MODES) ;

		mMatrix[mMatrixMode][mMatIdx[mMatrixMode]].make_identity();
		mMatHash[mMatrixMode]++;
	}
}

const glh::matrix4f& LLRender::getModelviewMatrix()
{
	return mMatrix[MM_MODELVIEW][mMatIdx[MM_MODELVIEW]];
}

const glh::matrix4f& LLRender::getProjectionMatrix()
{
	return mMatrix[MM_PROJECTION][mMatIdx[MM_PROJECTION]];
}

void LLRender::translateUI(F32 x, F32 y, F32 z)
{
	if (mUIOffset.empty())
	{
		LL_ERRS() << "Need to push a UI translation frame before offsetting" << LL_ENDL;
	}

	mUIOffset.back().mV[0] += x;
	mUIOffset.back().mV[1] += y;
	mUIOffset.back().mV[2] += z;
}

void LLRender::scaleUI(F32 x, F32 y, F32 z)
{
	if (mUIScale.empty())
	{
		LL_ERRS() << "Need to push a UI transformation frame before scaling." << LL_ENDL;
	}

	mUIScale.back().scaleVec(LLVector3(x,y,z));
}

void LLRender::pushUIMatrix()
{
	if (mUIOffset.empty())
	{
		mUIOffset.push_back(LLVector3(0,0,0));
	}
	else
	{
		mUIOffset.push_back(mUIOffset.back());
	}
	
	if (mUIScale.empty())
	{
		mUIScale.push_back(LLVector3(1,1,1));
	}
	else
	{
		mUIScale.push_back(mUIScale.back());
	}
}

void LLRender::popUIMatrix()
{
	if (mUIOffset.empty())
	{
		LL_ERRS() << "UI offset stack blown." << LL_ENDL;
	}
	mUIOffset.pop_back();
	mUIScale.pop_back();
}

LLVector3 LLRender::getUITranslation()
{
	if (mUIOffset.empty())
	{
		return LLVector3(0,0,0);
	}
	return mUIOffset.back();
}

LLVector3 LLRender::getUIScale()
{
	if (mUIScale.empty())
	{
		return LLVector3(1,1,1);
	}
	return mUIScale.back();
}


void LLRender::loadUIIdentity()
{
	if (mUIOffset.empty())
	{
		LL_ERRS() << "Need to push UI translation frame before clearing offset." << LL_ENDL;
	}
	mUIOffset.back().setVec(0,0,0);
	mUIScale.back().setVec(1,1,1);
}

void LLRender::setColorMask(bool writeColor, bool writeAlpha)
{
	setColorMask(writeColor, writeColor, writeColor, writeAlpha);
}

void LLRender::setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha)
{
	flush();

	if (mCurrColorMask[0] != writeColorR ||
		mCurrColorMask[1] != writeColorG ||
		mCurrColorMask[2] != writeColorB ||
		mCurrColorMask[3] != writeAlpha)
	{
		mCurrColorMask[0] = writeColorR;
		mCurrColorMask[1] = writeColorG;
		mCurrColorMask[2] = writeColorB;
		mCurrColorMask[3] = writeAlpha;

		glColorMask(writeColorR ? GL_TRUE : GL_FALSE, 
					writeColorG ? GL_TRUE : GL_FALSE,
					writeColorB ? GL_TRUE : GL_FALSE,
					writeAlpha ? GL_TRUE : GL_FALSE);
	}
}

void LLRender::setSceneBlendType(eBlendType type)
{
	switch (type) 
	{
		case BT_ALPHA:
			blendFunc(BF_SOURCE_ALPHA, BF_ONE_MINUS_SOURCE_ALPHA);
			break;
		case BT_ADD:
			blendFunc(BF_ONE, BF_ONE);
			break;
		case BT_ADD_WITH_ALPHA:
			blendFunc(BF_SOURCE_ALPHA, BF_ONE);
			break;
		case BT_MULT:
			blendFunc(BF_DEST_COLOR, BF_ZERO);
			break;
		case BT_MULT_ALPHA:
			blendFunc(BF_DEST_ALPHA, BF_ZERO);
			break;
		case BT_MULT_X2:
			blendFunc(BF_DEST_COLOR, BF_SOURCE_COLOR);
			break;
		case BT_REPLACE:
			blendFunc(BF_ONE, BF_ZERO);
			break;
		default:
			LL_ERRS() << "Unknown Scene Blend Type: " << type << LL_ENDL;
			break;
	}
}

void LLRender::blendFunc(eBlendFactor sfactor, eBlendFactor dfactor)
{
	llassert(sfactor < BF_UNDEF);
	llassert(dfactor < BF_UNDEF);
	if (mCurrBlendColorSFactor != sfactor || mCurrBlendColorDFactor != dfactor ||
	    mCurrBlendAlphaSFactor != sfactor || mCurrBlendAlphaDFactor != dfactor)
	{
		mCurrBlendColorSFactor = sfactor;
		mCurrBlendAlphaSFactor = sfactor;
		mCurrBlendColorDFactor = dfactor;
		mCurrBlendAlphaDFactor = dfactor;
		flush();
		glBlendFunc(sGLBlendFactor[sfactor], sGLBlendFactor[dfactor]);
	}
}

void LLRender::blendFunc(eBlendFactor color_sfactor, eBlendFactor color_dfactor,
			 eBlendFactor alpha_sfactor, eBlendFactor alpha_dfactor)
{
	llassert(color_sfactor < BF_UNDEF);
	llassert(color_dfactor < BF_UNDEF);
	llassert(alpha_sfactor < BF_UNDEF);
	llassert(alpha_dfactor < BF_UNDEF);
	
	if (mCurrBlendColorSFactor != color_sfactor || mCurrBlendColorDFactor != color_dfactor ||
	    mCurrBlendAlphaSFactor != alpha_sfactor || mCurrBlendAlphaDFactor != alpha_dfactor)
	{
		mCurrBlendColorSFactor = color_sfactor;
		mCurrBlendAlphaSFactor = alpha_sfactor;
		mCurrBlendColorDFactor = color_dfactor;
		mCurrBlendAlphaDFactor = alpha_dfactor;
		flush();
        
        glBlendFuncSeparate(sGLBlendFactor[color_sfactor], sGLBlendFactor[color_dfactor],
                           sGLBlendFactor[alpha_sfactor], sGLBlendFactor[alpha_dfactor]);
	}
}

LLTexUnit* LLRender::getTexUnit(U32 index)
{
	if (index < mTexUnits.size())
	{
		return &mTexUnits[index];
	}
	else 
	{
		LL_DEBUGS() << "Non-existing texture unit layer requested: " << index << LL_ENDL;
		return &mDummyTexUnit;
	}
}

LLLightState* LLRender::getLight(U32 index)
{
	if (index < mLightState.size())
	{
		return &mLightState[index];
	}
	
	return NULL;
}

void LLRender::setAmbientLightColor(const LLColor4& color)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE
	if (color != mAmbientLightColor)
	{
		++mLightHash;
		mAmbientLightColor = color;
	}
}

bool LLRender::verifyTexUnitActive(U32 unitToVerify)
{
	if (mCurrTextureUnitIndex == unitToVerify)
	{
		return true;
	}
	else 
	{
		LL_WARNS() << "TexUnit currently active: " << mCurrTextureUnitIndex << " (expecting " << unitToVerify << ")" << LL_ENDL;
		return false;
	}
}

void LLRender::clearErrors()
{
	while (glGetError())
	{
		//loop until no more error flags left
	}
}

void LLRender::begin(const GLuint& mode)
{
	if (mode != mMode)
	{
		if (mode == LLRender::QUADS)
		{
			mQuadCycle = 1;
		}

		if (mMode == LLRender::QUADS ||
			mMode == LLRender::LINES ||
			mMode == LLRender::TRIANGLES ||
			mMode == LLRender::POINTS)
		{
			flush();
		}
		else if (mCount != 0)
		{
			LL_ERRS() << "gGL.begin() called redundantly." << LL_ENDL;
		}
		
		mMode = mode;
	}
}

void LLRender::end()
{ 
	if (mCount == 0)
	{
		return;
		//IMM_ERRS << "GL begin and end called with no vertices specified." << LL_ENDL;
	}

	if ((mMode != LLRender::QUADS && 
		mMode != LLRender::LINES &&
		mMode != LLRender::TRIANGLES &&
		mMode != LLRender::POINTS) ||
		mCount > 2048)
	{
		flush();
	}
}
void LLRender::flush()
{
	if (mCount > 0)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_PIPELINE;
        llassert(LLGLSLShader::sCurBoundShaderPtr != nullptr);
		if (!mUIOffset.empty())
		{
			sUICalls++;
			sUIVerts += mCount;
		}
		
		//store mCount in a local variable to avoid re-entrance (drawArrays may call flush)
		U32 count = mCount;

			if (mMode == LLRender::QUADS && !sGLCoreProfile)
			{
				if (mCount%4 != 0)
				{
				count -= (mCount % 4);
				LL_WARNS() << "Incomplete quad requested." << LL_ENDL;
				}
			}
			
			if (mMode == LLRender::TRIANGLES)
			{
				if (mCount%3 != 0)
				{
				count -= (mCount % 3);
				LL_WARNS() << "Incomplete triangle requested." << LL_ENDL;
				}
			}
			
			if (mMode == LLRender::LINES)
			{
				if (mCount%2 != 0)
				{
				count -= (mCount % 2);
				LL_WARNS() << "Incomplete line requested." << LL_ENDL;
			}
		}

		mCount = 0;

        if (mBuffer)
        {

            HBXXH64 hash;
            U32 attribute_mask = LLGLSLShader::sCurBoundShaderPtr->mAttributeMask;

            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache hash");

                hash.update((U8*)mVerticesp.get(), count * sizeof(LLVector4a));
                if (attribute_mask & LLVertexBuffer::MAP_TEXCOORD0)
                {
                    hash.update((U8*)mTexcoordsp.get(), count * sizeof(LLVector2));
                }

                if (attribute_mask & LLVertexBuffer::MAP_COLOR)
                {
                    hash.update((U8*)mColorsp.get(), count * sizeof(LLColor4U));
                }

                hash.finalize();
            }
            
            
            U64 vhash = hash.digest();

            // check the VB cache before making a new vertex buffer
            // This is a giant hack to deal with (mostly) our terrible UI rendering code
            // that was built on top of OpenGL immediate mode.  Huge performance wins
            // can be had by not uploading geometry to VRAM unless absolutely necessary.
            // Most of our usage of the "immediate mode" style draw calls is actually
            // sending the same geometry over and over again.
            // To leverage this, we maintain a running hash of the vertex stream being
            // built up before a flush, and then check that hash against a VB 
            // cache just before creating a vertex buffer in VRAM
            std::unordered_map<U64, LLVBCache>::iterator cache = sVBCache.find(vhash);

            LLPointer<LLVertexBuffer> vb;

            if (cache != sVBCache.end())
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache hit");
                // cache hit, just use the cached buffer
                vb = cache->second.vb;
                cache->second.touched = std::chrono::steady_clock::now();
            }
            else
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache miss");
                vb = new LLVertexBuffer(attribute_mask);
                vb->allocateBuffer(count, 0);

                vb->setBuffer();

                vb->setPositionData((LLVector4a*) mVerticesp.get());

                if (attribute_mask & LLVertexBuffer::MAP_TEXCOORD0)
                {
                    vb->setTexCoordData(mTexcoordsp.get());
                }

                if (attribute_mask & LLVertexBuffer::MAP_COLOR)
                {
                    vb->setColorData(mColorsp.get());
                }

                vb->unbind();

                sVBCache[vhash] = { vb , std::chrono::steady_clock::now() };

                static U32 miss_count = 0;
                miss_count++;
                if (miss_count > 1024)
                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vb cache clean");
                    miss_count = 0;
                    auto now = std::chrono::steady_clock::now();

                    using namespace std::chrono_literals;
                    // every 1024 misses, clean the cache of any VBs that haven't been touched in the last second
                    for (std::unordered_map<U64, LLVBCache>::iterator iter = sVBCache.begin(); iter != sVBCache.end(); )
                    {
                        if (now - iter->second.touched > 1s)
                        {
                            iter = sVBCache.erase(iter);
                        }
                        else
                        {
                            ++iter;
                        }
                    }
                }
            }

            vb->setBuffer();

            if (mMode == LLRender::QUADS && sGLCoreProfile)
            {
                vb->drawArrays(LLRender::TRIANGLES, 0, count);
                mQuadCycle = 1;
            }
            else
            {
                vb->drawArrays(mMode, 0, count);
            }
        }
        else
        {
            // mBuffer is present in main thread and not present in an image thread
            LL_ERRS() << "A flush call from outside main rendering thread" << LL_ENDL;
        }

		
		mVerticesp[0] = mVerticesp[count];
		mTexcoordsp[0] = mTexcoordsp[count];
		mColorsp[0] = mColorsp[count];
		
		mCount = 0;
	}
}

void LLRender::vertex3f(const GLfloat& x, const GLfloat& y, const GLfloat& z)
{ 
	//the range of mVerticesp, mColorsp and mTexcoordsp is [0, 4095]
	if (mCount > 2048)
	{ //break when buffer gets reasonably full to keep GL command buffers happy and avoid overflow below
		switch (mMode)
		{
			case LLRender::POINTS: flush(); break;
			case LLRender::TRIANGLES: if (mCount%3==0) flush(); break;
			case LLRender::QUADS: if(mCount%4 == 0) flush(); break; 
			case LLRender::LINES: if (mCount%2 == 0) flush(); break;
		}
	}
			
	if (mCount > 4094)
	{
	//	LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
		return;
	}

	if (mUIOffset.empty())
	{
		mVerticesp[mCount] = LLVector3(x,y,z);
	}
	else
	{
		LLVector3 vert = (LLVector3(x,y,z)+mUIOffset.back()).scaledVec(mUIScale.back());
		mVerticesp[mCount] = vert;
	}

	if (mMode == LLRender::QUADS && LLRender::sGLCoreProfile)
	{
		mQuadCycle++;
		if (mQuadCycle == 4)
		{ //copy two vertices so fourth quad element will add a triangle
			mQuadCycle = 0;
	
			mCount++;
			mVerticesp[mCount] = mVerticesp[mCount-3];
			mColorsp[mCount] = mColorsp[mCount-3];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-3];

			mCount++;
			mVerticesp[mCount] = mVerticesp[mCount-2];
			mColorsp[mCount] = mColorsp[mCount-2];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-2];
		}
	}

	mCount++;
	mVerticesp[mCount] = mVerticesp[mCount-1];
	mColorsp[mCount] = mColorsp[mCount-1];
	mTexcoordsp[mCount] = mTexcoordsp[mCount-1];	
}

void LLRender::vertexBatchPreTransformed(LLVector3* verts, S32 vert_count)
{
	if (mCount + vert_count > 4094)
	{
		//	LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
		return;
	}

	if (sGLCoreProfile && mMode == LLRender::QUADS)
	{ //quads are deprecated, convert to triangle list
		S32 i = 0;
		
		while (i < vert_count)
		{
			//read first three
			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			//copy two
			mVerticesp[mCount++] = verts[i-3];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount++] = verts[i-1];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];
			
			//copy last one
			mVerticesp[mCount++] = verts[i++];
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}
	else
	{
		for (S32 i = 0; i < vert_count; i++)
		{
			mVerticesp[mCount] = verts[i];

			mCount++;
			mTexcoordsp[mCount] = mTexcoordsp[mCount-1];
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}

	if( mCount > 0 ) // ND: Guard against crashes if mCount is zero, yes it can happen
		mVerticesp[mCount] = mVerticesp[mCount-1];
}

void LLRender::vertexBatchPreTransformed(LLVector3* verts, LLVector2* uvs, S32 vert_count)
{
	if (mCount + vert_count > 4094)
	{
		//	LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
		return;
	}

	if (sGLCoreProfile && mMode == LLRender::QUADS)
	{ //quads are deprecated, convert to triangle list
		S32 i = 0;

		while (i < vert_count)
		{
			//read first three
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];

			//copy last two
			mVerticesp[mCount] = verts[i-3];
			mTexcoordsp[mCount++] = uvs[i-3];
			mColorsp[mCount] = mColorsp[mCount-1];

			mVerticesp[mCount] = verts[i-1];
			mTexcoordsp[mCount++] = uvs[i-1];
			mColorsp[mCount] = mColorsp[mCount-1];

			//copy last one
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount++] = uvs[i++];
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}
	else
	{
		for (S32 i = 0; i < vert_count; i++)
		{
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];

			mCount++;
			mColorsp[mCount] = mColorsp[mCount-1];
		}
	}

	if (mCount > 0)
	{
		mVerticesp[mCount] = mVerticesp[mCount - 1];
		mTexcoordsp[mCount] = mTexcoordsp[mCount - 1];
	}
}

void LLRender::vertexBatchPreTransformed(LLVector3* verts, LLVector2* uvs, LLColor4U* colors, S32 vert_count)
{
	if (mCount + vert_count > 4094)
	{
		//	LL_WARNS() << "GL immediate mode overflow.  Some geometry not drawn." << LL_ENDL;
		return;
	}

	
	if (sGLCoreProfile && mMode == LLRender::QUADS)
	{ //quads are deprecated, convert to triangle list
		S32 i = 0;

		while (i < vert_count)
		{
			//read first three
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];

			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];

			//copy last two
			mVerticesp[mCount] = verts[i-3];
			mTexcoordsp[mCount] = uvs[i-3];
			mColorsp[mCount++] = colors[i-3];

			mVerticesp[mCount] = verts[i-1];
			mTexcoordsp[mCount] = uvs[i-1];
			mColorsp[mCount++] = colors[i-1];

			//copy last one
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount++] = colors[i++];
		}
	}
	else
	{
		for (S32 i = 0; i < vert_count; i++)
		{
			mVerticesp[mCount] = verts[i];
			mTexcoordsp[mCount] = uvs[i];
			mColorsp[mCount] = colors[i];

			mCount++;
		}
	}

	if (mCount > 0)
	{
		mVerticesp[mCount] = mVerticesp[mCount - 1];
		mTexcoordsp[mCount] = mTexcoordsp[mCount - 1];
		mColorsp[mCount] = mColorsp[mCount - 1];
	}
}

void LLRender::vertex2i(const GLint& x, const GLint& y)
{
	vertex3f((GLfloat) x, (GLfloat) y, 0);	
}

void LLRender::vertex2f(const GLfloat& x, const GLfloat& y)
{ 
	vertex3f(x,y,0);
}

void LLRender::vertex2fv(const GLfloat* v)
{ 
	vertex3f(v[0], v[1], 0);
}

void LLRender::vertex3fv(const GLfloat* v)
{
	vertex3f(v[0], v[1], v[2]);
}

void LLRender::texCoord2f(const GLfloat& x, const GLfloat& y)
{ 
	mTexcoordsp[mCount] = LLVector2(x,y);
}

void LLRender::texCoord2i(const GLint& x, const GLint& y)
{ 
	texCoord2f((GLfloat) x, (GLfloat) y);
}

void LLRender::texCoord2fv(const GLfloat* tc)
{ 
	texCoord2f(tc[0], tc[1]);
}

void LLRender::color4ub(const GLubyte& r, const GLubyte& g, const GLubyte& b, const GLubyte& a)
{
	if (!LLGLSLShader::sCurBoundShaderPtr || LLGLSLShader::sCurBoundShaderPtr->mAttributeMask & LLVertexBuffer::MAP_COLOR)
	{
		mColorsp[mCount] = LLColor4U(r,g,b,a);
	}
	else
	{ //not using shaders or shader reads color from a uniform
		diffuseColor4ub(r,g,b,a);
	}
}
void LLRender::color4ubv(const GLubyte* c)
{
	color4ub(c[0], c[1], c[2], c[3]);
}

void LLRender::color4f(const GLfloat& r, const GLfloat& g, const GLfloat& b, const GLfloat& a)
{
	color4ub((GLubyte) (llclamp(r, 0.f, 1.f)*255),
		(GLubyte) (llclamp(g, 0.f, 1.f)*255),
		(GLubyte) (llclamp(b, 0.f, 1.f)*255),
		(GLubyte) (llclamp(a, 0.f, 1.f)*255));
}

void LLRender::color4fv(const GLfloat* c)
{ 
	color4f(c[0],c[1],c[2],c[3]);
}

void LLRender::color3f(const GLfloat& r, const GLfloat& g, const GLfloat& b)
{ 
	color4f(r,g,b,1);
}

void LLRender::color3fv(const GLfloat* c)
{ 
	color4f(c[0],c[1],c[2],1);
}

void LLRender::diffuseColor3f(F32 r, F32 g, F32 b)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, r,g,b,1.f);
	}
	else
	{
		glColor3f(r,g,b);
	}
}

void LLRender::diffuseColor3fv(const F32* c)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, c[0], c[1], c[2], 1.f);
	}
	else
	{
		glColor3fv(c);
	}
}

void LLRender::diffuseColor4f(F32 r, F32 g, F32 b, F32 a)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, r,g,b,a);
	}
	else
	{
		glColor4f(r,g,b,a);
	}
}

void LLRender::diffuseColor4fv(const F32* c)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader != NULL);

	if (shader)
	{
		shader->uniform4fv(LLShaderMgr::DIFFUSE_COLOR, 1, c);
	}
	else
	{
		glColor4fv(c);
	}
}

void LLRender::diffuseColor4ubv(const U8* c)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, c[0]/255.f, c[1]/255.f, c[2]/255.f, c[3]/255.f);
	}
	else
	{
		glColor4ubv(c);
	}
}

void LLRender::diffuseColor4ub(U8 r, U8 g, U8 b, U8 a)
{
	LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
	llassert(shader != NULL);

	if (shader)
	{
		shader->uniform4f(LLShaderMgr::DIFFUSE_COLOR, r/255.f, g/255.f, b/255.f, a/255.f);
	}
	else
	{
		glColor4ub(r,g,b,a);
	}
}


void LLRender::debugTexUnits(void)
{
	LL_INFOS("TextureUnit") << "Active TexUnit: " << mCurrTextureUnitIndex << LL_ENDL;
	std::string active_enabled = "false";
	for (U32 i = 0; i < mTexUnits.size(); i++)
	{
		if (getTexUnit(i)->mCurrTexType != LLTexUnit::TT_NONE)
		{
			if (i == mCurrTextureUnitIndex) active_enabled = "true";
			LL_INFOS("TextureUnit") << "TexUnit: " << i << " Enabled" << LL_ENDL;
			LL_INFOS("TextureUnit") << "Enabled As: " ;
			switch (getTexUnit(i)->mCurrTexType)
			{
				case LLTexUnit::TT_TEXTURE:
					LL_CONT << "Texture 2D";
					break;
				case LLTexUnit::TT_RECT_TEXTURE:
					LL_CONT << "Texture Rectangle";
					break;
				case LLTexUnit::TT_CUBE_MAP:
					LL_CONT << "Cube Map";
					break;
				default:
					LL_CONT << "ARGH!!! NONE!";
					break;
			}
			LL_CONT << ", Texture Bound: " << getTexUnit(i)->mCurrTexture << LL_ENDL;
		}
	}
	LL_INFOS("TextureUnit") << "Active TexUnit Enabled : " << active_enabled << LL_ENDL;
}



glh::matrix4f copy_matrix(F32* src)
{
	glh::matrix4f ret;
	ret.set_value(src);
	return ret;
}

glh::matrix4f get_current_modelview()
{
	return copy_matrix(gGLModelView);
}

glh::matrix4f get_current_projection()
{
	return copy_matrix(gGLProjection);
}

glh::matrix4f get_last_modelview()
{
	return copy_matrix(gGLLastModelView);
}

glh::matrix4f get_last_projection()
{
	return copy_matrix(gGLLastProjection);
}

void copy_matrix(const glh::matrix4f& src, F32* dst)
{
	for (U32 i = 0; i < 16; i++)
	{
		dst[i] = src.m[i];
	}
}

void set_current_modelview(const glh::matrix4f& mat)
{
	copy_matrix(mat, gGLModelView);
}

void set_current_projection(glh::matrix4f& mat)
{
	copy_matrix(mat, gGLProjection);
}

glh::matrix4f gl_ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar)
{
	glh::matrix4f ret(
		2.f/(right-left), 0.f, 0.f, -(right+left)/(right-left),
		0.f, 2.f/(top-bottom), 0.f, -(top+bottom)/(top-bottom),
		0.f, 0.f, -2.f/(zfar-znear),  -(zfar+znear)/(zfar-znear),
		0.f, 0.f, 0.f, 1.f);

	return ret;
}

glh::matrix4f gl_perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
	GLfloat f = 1.f/tanf(DEG_TO_RAD*fovy/2.f);

	return glh::matrix4f(f/aspect, 0, 0, 0,
						 0, f, 0, 0,
						 0, 0, (zFar+zNear)/(zNear-zFar), (2.f*zFar*zNear)/(zNear-zFar),
						 0, 0, -1.f, 0);
}

glh::matrix4f gl_lookat(LLVector3 eye, LLVector3 center, LLVector3 up)
{
	LLVector3 f = center-eye;
	f.normVec();
	up.normVec();
	LLVector3 s = f % up;
	LLVector3 u = s % f;

	return glh::matrix4f(s[0], s[1], s[2], 0,
					  u[0], u[1], u[2], 0,
					  -f[0], -f[1], -f[2], 0,
					  0, 0, 0, 1);
	
}
