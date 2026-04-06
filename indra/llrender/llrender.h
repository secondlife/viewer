/**
 * @file llrender.h
 * @brief LLRender definition
 *
 *  This class acts as a wrapper for OpenGL calls.
 *  The goal of this class is to minimize the number of api calls due to legacy rendering
 *  code, to define an interface for a multiple rendering API abstraction of the UI
 *  rendering, and to abstract out direct rendering calls in a way that is cleaner and easier to maintain.
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

#pragma once

//#include "linden_common.h"

#include "v2math.h"
#include "v3math.h"
#include "v4coloru.h"
#include "v4math.h"
#include "llstrider.h"
#include "llpointer.h"
#include "llglheaders.h"
#include "llmatrix4a.h"
#include "glm/mat4x4.hpp"
#include <boost/align/aligned_allocator.hpp>

#include <array>
#include <list>
#include <span>

class LLVertexBuffer;
class LLCubeMap;
class LLImageGL;
class LLRenderTarget;
class LLTexture;
class LLVertexBufferData;

#define LL_MATRIX_STACK_DEPTH 32

constexpr U32 LL_NUM_TEXTURE_LAYERS = 32;
constexpr U32 LL_NUM_LIGHT_UNITS = 8;

class LLTexUnit
{
    friend class LLRender;
public:
    static U32 sWhiteTexture;

    enum class eTextureType
    {
        TT_TEXTURE = 0,         // Standard 2D Texture
        TT_RECT_TEXTURE,        // Non power of 2 texture
        TT_CUBE_MAP,            // 6-sided cube map texture
        TT_CUBE_MAP_ARRAY,      // Array of cube maps
        TT_MULTISAMPLE_TEXTURE, // see GL_ARB_texture_multisample
        TT_TEXTURE_3D,          // standard 3D Texture
        TT_NONE,                // No texture type is currently enabled
    };

    enum class eTextureAddressMode
    {
        TAM_WRAP = 0,           // Standard 2D Texture
        TAM_MIRROR,             // Non power of 2 texture
        TAM_CLAMP               // No texture type is currently enabled
    };

    enum class eTextureFilterOptions
    {   // Note: If mipmapping or anisotropic are not enabled or supported it should fall back gracefully
        TFO_POINT = 0,          // Equal to: min=point, mag=point, mip=none.
        TFO_BILINEAR,           // Equal to: min=linear, mag=linear, mip=point.
        TFO_TRILINEAR,          // Equal to: min=linear, mag=linear, mip=linear.
        TFO_ANISOTROPIC         // Equal to: min=anisotropic, max=anisotropic, mip=linear.
    };

    enum class eTextureMipGeneration
    {
        TMG_NONE = 0,           // Mipmaps are not automatically generated for this texture.
        TMG_AUTO,               // Mipmaps are automatically generated for this texture.
        TMG_MANUAL              // Mipmaps are manually generated for this texture.
    };

    enum class eTextureColorSpace
    {
        TCS_LINEAR = 0,
        TCS_SRGB
    };

    explicit LLTexUnit(S32 index = -1);

    // Refreshes renderer state of the texture unit to the cached values
    // Needed when the render context has changed and invalidated the current state
    void refreshState();

    // returns the index of this texture unit
    [[nodiscard]] S32 getIndex() const { return mIndex; }

    // Sets this tex unit to be the currently active one
    void activate();

    // Enables this texture unit for the given texture type
    // (automatically disables any previously enabled texture type)
    void enable(eTextureType type);

    // Disables the current texture unit
    void disable();

    // Binds the LLImageGL to this texture unit
    // (automatically enables the unit for the LLImageGL's texture type)
    bool bind(LLImageGL* texture, bool for_rendering = false, bool forceBind = false, S32 usename = 0);
    bool bind(LLTexture* texture, bool for_rendering = false, bool forceBind = false);

    // bind implementation for inner loops
    // makes the following assumptions:
    //  - No need for gGL.flush()
    //  - texture is not null
    //  - gl_tex->getTexName() is not zero
    //  - This texture is not being bound redundantly
    //  - USE_SRGB_DECODE is disabled
    //  - mTexOptionsDirty is false
    //  -
    void bindFast(LLTexture* texture);

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

    // Fast but unsafe version of unbind
    void unbindFast(eTextureType type);

    // Sets the addressing mode used to sample the texture
    // Warning: this stays set for the bound texture forever,
    // make sure you want to permanently change the address mode  for the bound texture.
    void setTextureAddressMode(eTextureAddressMode mode);
    // MUST already be active and bound
    void setTextureAddressModeFast(eTextureAddressMode mode, eTextureType tex_type);

    // Sets the filtering options used to sample the texture
    // Warning: this stays set for the bound texture forever,
    // make sure you want to permanently change the filtering for the bound texture.
    void setTextureFilteringOption(LLTexUnit::eTextureFilterOptions option);
    // MUST already be active and bound
    void setTextureFilteringOptionFast(LLTexUnit::eTextureFilterOptions option, eTextureType tex_type);

    [[nodiscard]] static U32 getInternalType(eTextureType type);

    [[nodiscard]] U32 getCurrTexture() { return mCurrTexture; }

    [[nodiscard]] eTextureType getCurrType() { return mCurrTexType; }

    void setHasMipMaps(bool hasMips) { mHasMipMaps = hasMips; }

protected:
    friend class LLRender;

    S32                 mIndex;
    U32                 mCurrTexture;
    eTextureType        mCurrTexType;
    bool                mHasMipMaps;

    void debugTextureUnit();
};

class LLLightState
{
public:
    explicit LLLightState(S32 index = -1);

    void enable();
    void disable();
    void setDiffuse(const LLColor4& diffuse);
    void setDiffuseB(const LLColor4& diffuse);
    void setAmbient(const LLColor4& ambient);
    void setSpecular(const LLColor4& specular);
    void setPosition(const LLVector4& position);
    void setConstantAttenuation(const F32& atten);
    void setLinearAttenuation(const F32& atten);
    void setQuadraticAttenuation(const F32& atten);
    void setSpotExponent(const F32& exponent);
    void setSpotCutoff(const F32& cutoff);
    void setSpotDirection(const LLVector3& direction);
    void setSunPrimary(bool v);
    void setSize(F32 size);
    void setFalloff(F32 falloff);

protected:
    friend class LLRender;

    S32 mIndex;
    bool mEnabled;
    LLColor4 mDiffuse;
    LLColor4 mDiffuseB;
    bool     mSunIsPrimary;
    LLColor4 mAmbient;
    LLColor4 mSpecular;
    LLVector4 mPosition;
    LLVector3 mSpotDirection;

    F32 mConstantAtten;
    F32 mLinearAtten;
    F32 mQuadraticAtten;

    F32 mSpotExponent;
    F32 mSpotCutoff;
    F32 mSize = 0.f;
    F32 mFalloff = 0.f;
};

class LLRender
{
    friend class LLTexUnit;
public:

    enum eTexIndex : U8
    {
        // Channels for material textures
        DIFFUSE_MAP            = 0,
        ALTERNATE_DIFFUSE_MAP  = 1,
        NORMAL_MAP             = 1,
        SPECULAR_MAP           = 2,
        // Channels for PBR textures
        BASECOLOR_MAP          = 3,
        METALLIC_ROUGHNESS_MAP = 4,
        GLTF_NORMAL_MAP        = 5,
        EMISSIVE_MAP           = 6,
        // Total number of channels
        NUM_TEXTURE_CHANNELS   = 7,
    };

    enum eVolumeTexIndex : U8
    {
        LIGHT_TEX = 0,
        SCULPT_TEX,
        NUM_VOLUME_TEXTURE_CHANNELS,
    };

    enum eGeomModes : U8
    {
        TRIANGLES = 0,
        TRIANGLE_STRIP,
        TRIANGLE_FAN,
        POINTS,
        LINES,
        LINE_STRIP,
        LINE_LOOP,
        NUM_MODES
    };

    enum eCompareFunc : U8
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
    };

    enum eBlendType : U8
    {
        BT_ALPHA = 0,
        BT_ADD,
        BT_ADD_WITH_ALPHA,  // Additive blend modulated by the fragment's alpha.
        BT_MULT,
        BT_MULT_ALPHA,
        BT_MULT_X2,
        BT_REPLACE
    };

    // WARNING:  this MUST match the LL_PART_BF enum in LLPartData, so set values explicitly in case someone
    // decides to add more or reorder them
    enum eBlendFactor : U8
    {
        BF_ONE = 0,
        BF_ZERO = 1,
        BF_DEST_COLOR = 2,
        BF_SOURCE_COLOR = 3,
        BF_ONE_MINUS_DEST_COLOR = 4,
        BF_ONE_MINUS_SOURCE_COLOR = 5,
        BF_DEST_ALPHA = 6,
        BF_SOURCE_ALPHA = 7,
        BF_ONE_MINUS_DEST_ALPHA = 8,
        BF_ONE_MINUS_SOURCE_ALPHA = 9,
        BF_UNDEF
    };

    enum eMatrixMode : U8
    {
        MM_MODELVIEW = 0,
        MM_PROJECTION,
        MM_TEXTURE0,
        MM_TEXTURE1,
        MM_TEXTURE2,
        MM_TEXTURE3,
        NUM_MATRIX_MODES,
        MM_TEXTURE
    };

    LLRender();
    ~LLRender();
    bool init(bool needs_vertex_buffer);
    void initVertexBuffer();
    void resetVertexBuffer();
    void shutdown();

    // Refreshes renderer state to the cached values
    // Needed when the render context has changed and invalidated the current state
    void refreshState();

    void translatef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void scalef(const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void rotatef(const GLfloat& a, const GLfloat& x, const GLfloat& y, const GLfloat& z);
    void ortho(F32 left, F32 right, F32 bottom, F32 top, F32 zNear, F32 zFar);

    void pushMatrix();
    void popMatrix();
    void loadMatrix(const GLfloat* m);
    void loadIdentity();
    void multMatrix(const GLfloat* m);
    void matrixMode(eMatrixMode mode);
    [[nodiscard]] eMatrixMode getMatrixMode();

    [[nodiscard]] const glm::mat4& getModelviewMatrix();
    [[nodiscard]] const glm::mat4& getProjectionMatrix();

    void syncMatrices();
    void syncLightState();

    void translateUI(F32 x, F32 y, F32 z);
    void scaleUI(F32 x, F32 y, F32 z);
    void pushUIMatrix();
    void popUIMatrix();
    void loadUIIdentity();
    [[nodiscard]] LLVector3 getUITranslation();
    [[nodiscard]] LLVector3 getUIScale();

    void flush();

    // if list is set, will store buffers in list for later use, if list isn't set, will use cache
    void beginList(std::list<LLVertexBufferData> *list);
    void endList();

    void begin(const GLuint& mode);
    void end();

    [[nodiscard]] U8 getMode() const { return mMode; }

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
    void diffuseColor4ub(U8 r, U8 g, U8 b, U8 a);

    void transform(LLVector3& vert);
    void transform(LLVector4a& vert);
    void untransform(LLVector3& vert);

    void batchTransform(LLVector4a* verts, U32 vert_count);

    void vertexBatchPreTransformed(std::span<const LLVector4a> verts);
    void vertexBatchPreTransformed(std::span<const LLVector4a> verts, std::span<const LLVector2> uvs);
    void vertexBatchPreTransformed(std::span<const LLVector4a> verts, std::span<const LLVector2> uvs, std::span<const LLColor4U> colors);

    void setColorMask(bool writeColor, bool writeAlpha);
    void setColorMask(bool writeColorR, bool writeColorG, bool writeColorB, bool writeAlpha);
    void setSceneBlendType(eBlendType type);

    // applies blend func to both color and alpha
    void blendFunc(eBlendFactor sfactor, eBlendFactor dfactor);
    // applies separate blend functions to color and alpha
    void blendFunc(eBlendFactor color_sfactor, eBlendFactor color_dfactor,
               eBlendFactor alpha_sfactor, eBlendFactor alpha_dfactor);

    [[nodiscard]] LLLightState* getLight(U32 index);
    void setAmbientLightColor(const LLColor4& color);

    [[nodiscard]] LLTexUnit* getTexUnit(U32 index);

    [[nodiscard]] U32 getCurrentTexUnitIndex() const { return mCurrTextureUnitIndex; }

    bool verifyTexUnitActive(U32 unitToVerify);

    void debugTexUnits();

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
    static bool sNsightDebugSupport;
    static LLVector2 sUIGLScaleFactor;
    static bool sClassicMode; // classic sky mode active

private:
    friend class LLLightState;

    LLVertexBuffer* bufferfromCache(U32 attribute_mask, U32 count);
    LLVertexBuffer* genBuffer(U32 attribute_mask, S32 count);
    void drawBuffer(LLVertexBuffer* vb, U32 mode, S32 count);
    void resetStriders(S32 count);

    eMatrixMode mMatrixMode;
    std::array<U32, NUM_MATRIX_MODES> mMatIdx;
    std::array<U32, NUM_MATRIX_MODES> mMatHash;
    glm::mat4 mMatrix[NUM_MATRIX_MODES][LL_MATRIX_STACK_DEPTH];
    std::array<U32, NUM_MATRIX_MODES> mCurMatHash;
    U32 mLightHash;
    LLColor4 mAmbientLightColor;

    bool            mDirty;
    U32             mCount;
    U32             mMode;
    U32             mCurrTextureUnitIndex;
    std::array<bool, 4> mCurrColorMask;

    LLPointer<LLVertexBuffer>   mBuffer;
    LLStrider<LLVector4a>       mVerticesp;
    LLStrider<LLVector2>        mTexcoordsp;
    LLStrider<LLColor4U>        mColorsp;
    std::array<LLTexUnit, LL_NUM_TEXTURE_LAYERS> mTexUnits;
    LLTexUnit           mDummyTexUnit;
    std::array<LLLightState, LL_NUM_LIGHT_UNITS> mLightState;

    eBlendFactor mCurrBlendColorSFactor;
    eBlendFactor mCurrBlendColorDFactor;
    eBlendFactor mCurrBlendAlphaSFactor;
    eBlendFactor mCurrBlendAlphaDFactor;

    std::vector<LLVector4a, boost::alignment::aligned_allocator<LLVector4a, 16> > mUIOffset;
    std::vector<LLVector4a, boost::alignment::aligned_allocator<LLVector4a, 16> > mUIScale;
};

extern F32 gGLModelView[16];
extern F32 gGLLastModelView[16];
extern F32 gGLLastProjection[16];
extern F32 gGLProjection[16];
extern S32 gGLViewport[4];
extern glm::mat4 gGLDeltaModelView;
extern glm::mat4 gGLInverseDeltaModelView;

extern thread_local LLRender gGL;

// This rotation matrix moves the default OpenGL reference frame
// (-Z at, Y up) to Cory's favorite reference frame (X at, Z up)
const F32 OGL_TO_CFR_ROTATION[16] = {  0.f,  0.f, -1.f,  0.f,   // -Z becomes X
                                      -1.f,  0.f,  0.f,  0.f,   // -X becomes Y
                                       0.f,  1.f,  0.f,  0.f,   //  Y becomes Z
                                       0.f,  0.f,  0.f,  1.f };

[[nodiscard]] glm::mat4 copy_matrix(F32* src);
[[nodiscard]] glm::mat4 get_current_modelview();
[[nodiscard]] glm::mat4 get_current_projection();
[[nodiscard]] glm::mat4 get_last_modelview();
[[nodiscard]] glm::mat4 get_last_projection();

void copy_matrix(const glm::mat4& src, F32* dst);
void set_current_modelview(const glm::mat4& mat);
void set_current_projection(const glm::mat4& mat);
void set_last_modelview(const glm::mat4& mat);
void set_last_projection(const glm::mat4& mat);

// glh compat
[[nodiscard]] glm::vec3 mul_mat4_vec3(const glm::mat4& mat, const glm::vec3& vec);

#define LL_SHADER_LOADING_WARNS(...) LL_WARNS()

