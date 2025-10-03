/**
 * @file llglslshader.h
 * @brief GLSL shader wrappers
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

#ifndef LL_LLGLSLSHADER_H
#define LL_LLGLSLSHADER_H

#include "llgl.h"
#include "llrender.h"
#include "llstaticstringtable.h"
#include <boost/json.hpp>
#include <unordered_map>

class LLShaderFeatures
{
public:
    S32 mIndexedTextureChannels = 0;
    bool calculatesLighting = false;
    bool calculatesAtmospherics = false;
    bool hasLighting = false; // implies no transport (it's possible to have neither though)
    bool isAlphaLighting = false; // indicates lighting shaders need not be linked in (lighting performed directly in alpha shader to match deferred lighting functions)
    bool isSpecular = false;
    bool hasTransport = false; // implies no lighting (it's possible to have neither though)
    bool hasSkinning = false;
    bool hasObjectSkinning = false;
    bool mGLTF = false;
    bool hasAtmospherics = false;
    bool hasGamma = false;
    bool hasShadows = false;
    bool hasAmbientOcclusion = false;
    bool hasSrgb = false;
    bool isDeferred = false;
    bool hasFullGBuffer = false;
    bool hasScreenSpaceReflections = false;
    bool hasAlphaMask = false;
    bool hasReflectionProbes = false;
    bool attachNothing = false;
    bool hasHeroProbes = false;
    bool isPBRTerrain = false;
    bool hasTonemap = false;
};

// ============= Structure for caching shader uniforms ===============
class LLGLSLShader;

class LLShaderUniforms
{
public:

    template<typename T>
    struct UniformSetting
    {
        S32 mUniform{ 0 };
        T mValue{};
    };

    typedef UniformSetting<S32> IntSetting;
    typedef UniformSetting<F32> FloatSetting;
    typedef UniformSetting<LLVector4> VectorSetting;
    typedef UniformSetting<LLVector3> Vector3Setting;

    void clear()
    {
        mIntegers.resize(0);
        mFloats.resize(0);
        mVectors.resize(0);
        mVector3s.resize(0);
    }

    void uniform1i(S32 index, S32 value)
    {
        mIntegers.push_back({ index, value });
    }

    void uniform1f(S32 index, F32 value)
    {
        mFloats.push_back({ index, value });
    }

    void uniform4fv(S32 index, const LLVector4& value)
    {
        mVectors.push_back({ index, value });
    }

    void uniform4fv(S32 index, const F32* value)
    {
        mVectors.push_back({ index, LLVector4(value) });
    }

    void uniform3fv(S32 index, const LLVector3& value)
    {
        mVector3s.push_back({ index, value });
    }

    void uniform3fv(S32 index, const F32* value)
    {
        mVector3s.push_back({ index, LLVector3(value) });
    }

    void apply(LLGLSLShader* shader);


    std::vector<IntSetting> mIntegers;
    std::vector<FloatSetting> mFloats;
    std::vector<VectorSetting> mVectors;
    std::vector<Vector3Setting> mVector3s;
};
class LLGLSLShader
{
public:
    // NOTE: Keep gShaderConsts and LLGLSLShader::ShaderConsts_e in sync!
    enum eShaderConsts
    {
        SHADER_CONST_CLOUD_MOON_DEPTH
        , SHADER_CONST_STAR_DEPTH
        , NUM_SHADER_CONSTS
    };

    // enum primarily used to control application sky settings uniforms
    typedef enum
    {
        SG_DEFAULT = 0,  // not sky or water specific
        SG_SKY,  //
        SG_WATER,
        SG_ANY,
        SG_COUNT
    } eGroup;

    enum UniformBlock : GLuint
    {
        UB_REFLECTION_PROBES,   // "ReflectionProbes"
        UB_GLTF_JOINTS,         // "GLTFJoints"
        UB_GLTF_NODES,          // "GLTFNodes"
        UB_GLTF_MATERIALS,      // "GLTFMaterials"
        NUM_UNIFORM_BLOCKS
    };


    static std::set<LLGLSLShader*> sInstances;
    static bool sProfileEnabled;

    LLGLSLShader();
    ~LLGLSLShader();

    static GLuint sCurBoundShader;
    static LLGLSLShader* sCurBoundShaderPtr;
    static S32 sIndexedTextureChannels;

    static U32 sMaxGLTFMaterials;
    static U32 sMaxGLTFNodes;

    static void initProfile();
    static void finishProfile(boost::json::value& stats=sDefaultStats);

    static void startProfile();
    static void stopProfile();

    void unload();
    void clearStats();
    void dumpStats(boost::json::object& stats);

    // place query objects for profiling if profiling is enabled
    // if for_runtime is true, will place timer query only whether or not profiling is enabled
    void placeProfileQuery(bool for_runtime = false);

    // Readback query objects if profiling is enabled
    // If for_runtime is true, will readback timer query iff query is available
    // Will return false if a query is pending (try again later)
    // If force_read is true, will force an immediate readback (severe performance penalty)
    bool readProfileQuery(bool for_runtime = false, bool force_read = false);

    bool createShader();
    bool attachFragmentObject(std::string object);
    bool attachVertexObject(std::string object);
    void attachObject(GLuint object);
    void attachObjects(GLuint* objects = NULL, S32 count = 0);
    bool mapAttributes();
    bool mapUniforms();
    void mapUniform(GLint index);
    void uniform1i(U32 index, GLint i);
    void uniform1f(U32 index, GLfloat v);
    void fastUniform1f(U32 index, GLfloat v);
    void uniform2f(U32 index, GLfloat x, GLfloat y);
    void uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z);
    void uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void uniform1iv(U32 index, U32 count, const GLint* i);
    void uniform4iv(U32 index, U32 count, const GLint* i);
    void uniform1fv(U32 index, U32 count, const GLfloat* v);
    void uniform2fv(U32 index, U32 count, const GLfloat* v);
    void uniform3fv(U32 index, U32 count, const GLfloat* v);
    void uniform4fv(U32 index, U32 count, const GLfloat* v);
    void uniform4uiv(U32 index, U32 count, const GLuint* v);
    void uniform2i(const LLStaticHashedString& uniform, GLint i, GLint j);
    void uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v);
    void uniform1i(const LLStaticHashedString& uniform, GLint i);
    void uniform1iv(const LLStaticHashedString& uniform, U32 count, const GLint* v);
    void uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v);
    void uniform1f(const LLStaticHashedString& uniform, GLfloat v);
    void uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y);
    void uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z);
    void uniform4f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform3fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform4uiv(const LLStaticHashedString& uniform, U32 count, const GLuint* v);
    void uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v);

    void setMinimumAlpha(F32 minimum);

    void vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void vertexAttrib4fv(U32 index, GLfloat* v);

    //GLint getUniformLocation(const std::string& uniform);
    GLint getUniformLocation(const LLStaticHashedString& uniform);
    GLint getUniformLocation(U32 index);

    GLint getAttribLocation(U32 attrib);
    GLint mapUniformTextureChannel(GLint location, GLenum type, GLint size);

    void clearPermutations();
    void addPermutation(std::string name, std::string value);
    void addPermutations(const std::map<std::string, std::string>& defines)
    {
        mDefines.insert(defines.begin(), defines.end());
    }
    void removePermutation(std::string name);

    void addConstant(const LLGLSLShader::eShaderConsts shader_const);

    //enable/disable texture channel for specified uniform
    //if given texture uniform is active in the shader,
    //the corresponding channel will be active upon return
    //returns channel texture is enabled in from [0-MAX)
    S32 enableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 disableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

    // get the texture channel of the given uniform, or -1 if uniform is not used as a texture
    S32 getTextureChannel(S32 uniform) const;

    // bindTexture returns the texture unit we've bound the texture to.
    // You can reuse the return value to unbind a texture when required.
    S32 bindTexture(const std::string& uniform, LLTexture* texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 bindTexture(S32 uniform, LLTexture* texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 bindTexture(const std::string& uniform, LLRenderTarget* texture, bool depth = false, LLTexUnit::eTextureFilterOptions mode = LLTexUnit::TFO_BILINEAR);
    S32 bindTexture(S32 uniform, LLRenderTarget* texture, bool depth = false, LLTexUnit::eTextureFilterOptions mode = LLTexUnit::TFO_BILINEAR, U32 index = 0);
    S32 unbindTexture(const std::string& uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 unbindTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);

    bool link(bool suppress_errors = false);
    void bind();
    //helper to conditionally bind mRiggedVariant instead of this
    void bind(bool rigged);

    bool isComplete() const { return mProgramObject != 0; }

    LLUUID hash();

    // Unbinds any previously bound shader by explicitly binding no shader.
    static void unbind();

    U32 mMatHash[LLRender::NUM_MATRIX_MODES];
    U32 mLightHash;

    GLuint mProgramObject;
#if LL_RELEASE_WITH_DEBUG_INFO
    struct attr_name
    {
        GLint loc;
        const char* name;
        void operator = (GLint _loc) { loc = _loc; }
        operator GLint () { return loc; }
    };
    std::vector<attr_name> mAttribute; //lookup table of attribute enum to attribute channel
#else
    std::vector<GLint> mAttribute; //lookup table of attribute enum to attribute channel
#endif
    U32 mAttributeMask;  //mask of which reserved attributes are set (lines up with LLVertexBuffer::getTypeMask())
    std::vector<GLint> mUniform;   //lookup table of uniform enum to uniform location
    LLStaticStringTable<GLint> mUniformMap; //lookup map of uniform name to uniform location
    typedef std::unordered_map<GLint, LLVector4> uniform_value_map_t;
    uniform_value_map_t mValue; //lookup map of uniform location to last known value
    std::vector<GLint> mTexture;
    S32 mTotalUniformSize;
    S32 mActiveTextureChannels;
    S32 mShaderLevel;
    S32 mShaderGroup; // see LLGLSLShader::eGroup
    bool mUniformsDirty;
    LLShaderFeatures mFeatures;
    std::vector< std::pair< std::string, GLenum > > mShaderFiles;
    std::string mName;
    typedef std::map<std::string, std::string> defines_map_t; //NOTE: this must be an ordered map to maintain hash consistency
    defines_map_t mDefines;
    static defines_map_t sGlobalDefines;
    LLUUID mShaderHash;
    bool mUsingBinaryProgram = false;

    //statistics for profiling shader performance
    bool mProfilePending = false;
    U32 mTimerQuery;
    U32 mSamplesQuery;
    U32 mPrimitivesQuery;

    U64 mTimeElapsed;
    static U64 sTotalTimeElapsed;
    U32 mTrianglesDrawn;
    static U32 sTotalTrianglesDrawn;
    U64 mSamplesDrawn;
    static U64 sTotalSamplesDrawn;
    U32 mBinds;
    static U32 sTotalBinds;

    // this pointer should be set to whichever shader represents this shader's rigged variant
    LLGLSLShader* mRiggedVariant = nullptr;

    // variants for use by GLTF renderer
    // bit 0 = alpha mode blend (1) or opaque (0)
    // bit 1 = rigged (1) or static (0)
    // bit 2 = unlit (1) or lit (0)
    // bit 3 = single (0) or multi (1) uv coordinates
    struct GLTFVariant
    {
        constexpr static U8 ALPHA_BLEND = 1;
        constexpr static U8 RIGGED = 2;
        constexpr static U8 UNLIT = 4;
        constexpr static U8 MULTI_UV = 8;
    };

    constexpr static U8 NUM_GLTF_VARIANTS = 16;

    std::vector<LLGLSLShader> mGLTFVariants;

    //helper to bind GLTF variant
    void bind(U8 variant);

    // hacky flag used for optimization in LLDrawPoolAlpha
    bool mCanBindFast = false;

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
    void setLabel(const char* label);
#endif

private:
    void unloadInternal();
    // This must be static because finishProfile() is called at least once
    // within a __try block. If we default its stats parameter to a temporary
    // json::value, that temporary must be destroyed when the stack is
    // unwound, which __try forbids.
    static boost::json::value sDefaultStats;
};

//UI shader (declared here so llui_libtest will link properly)
extern LLGLSLShader         gUIProgram;
//output vec4(color.rgb,color.a*tex0[tc0].a)
extern LLGLSLShader         gSolidColorProgram;
//Alpha mask shader (declared here so llappearance can access properly)
extern LLGLSLShader         gAlphaMaskProgram;

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
#define LL_SET_SHADER_LABEL(shader) shader.setLabel(#shader)
#else
#define LL_SET_SHADER_LABEL(shader, label)
#endif

#endif
