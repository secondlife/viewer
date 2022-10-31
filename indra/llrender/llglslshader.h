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
#include <unordered_map>

class LLShaderFeatures
{
public:
    bool atmosphericHelpers;
    bool calculatesLighting;
    bool calculatesAtmospherics;
    bool hasLighting; // implies no transport (it's possible to have neither though)
    bool isAlphaLighting; // indicates lighting shaders need not be linked in (lighting performed directly in alpha shader to match deferred lighting functions)
    bool isShiny;
    bool isFullbright; // implies no lighting
    bool isSpecular;
    bool hasWaterFog; // implies no gamma
    bool hasTransport; // implies no lighting (it's possible to have neither though)
    bool hasSkinning;   
    bool hasObjectSkinning;
    bool hasAtmospherics;
    bool hasGamma;
    bool hasShadows;
    bool hasAmbientOcclusion;
    bool hasSrgb;
    bool encodesNormal;
    bool isDeferred;
    bool hasIndirect;
    S32 mIndexedTextureChannels;
    bool disableTextureIndex;
    bool hasAlphaMask;
    bool attachNothing;

    // char numLights;
    
    LLShaderFeatures();
};

// ============= Structure for caching shader uniforms ===============
class LLGLSLShader;

class LLShaderUniforms
{
public:

    template<typename T>
    struct UniformSetting
    {
        S32 mUniform;
        T mValue;
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

    void apply(LLGLSLShader* shader);
   

    std::vector<IntSetting> mIntegers;
    std::vector<FloatSetting> mFloats;
    std::vector<VectorSetting> mVectors;
    std::vector<Vector3Setting> mVector3s;
};
class LLGLSLShader
{
public:

    // enum primarily used to control application sky settings uniforms
    typedef enum 
    {
        SG_DEFAULT = 0,  // not sky or water specific
        SG_SKY,  //
        SG_WATER,
        SG_ANY,
        SG_COUNT
    } eGroup;
    
    static std::set<LLGLSLShader*> sInstances;
    static bool sProfileEnabled;

    LLGLSLShader();
    ~LLGLSLShader();

    static GLhandleARB sCurBoundShader;
    static LLGLSLShader* sCurBoundShaderPtr;
    static S32 sIndexedTextureChannels;

    static void initProfile();
    static void finishProfile(bool emit_report = true);

    static void startProfile();
    static void stopProfile(U32 count, U32 mode);

    void unload();
    void clearStats();
    void dumpStats();
    void placeProfileQuery();
    void readProfileQuery(U32 count, U32 mode);

    BOOL createShader(std::vector<LLStaticHashedString> * attributes,
                        std::vector<LLStaticHashedString> * uniforms,
                        U32 varying_count = 0,
                        const char** varyings = NULL);
    BOOL attachFragmentObject(std::string object);
    BOOL attachVertexObject(std::string object);
    void attachObject(GLhandleARB object);
    void attachObjects(GLhandleARB* objects = NULL, S32 count = 0);
    BOOL mapAttributes(const std::vector<LLStaticHashedString> * attributes);
    BOOL mapUniforms(const std::vector<LLStaticHashedString> *);
    void mapUniform(GLint index, const std::vector<LLStaticHashedString> *);
    void uniform1i(U32 index, GLint i);
    void uniform1f(U32 index, GLfloat v);
    void uniform2f(U32 index, GLfloat x, GLfloat y);
    void uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z);
    void uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void uniform1iv(U32 index, U32 count, const GLint* i);
    void uniform1fv(U32 index, U32 count, const GLfloat* v);
    void uniform2fv(U32 index, U32 count, const GLfloat* v);
    void uniform3fv(U32 index, U32 count, const GLfloat* v);
    void uniform4fv(U32 index, U32 count, const GLfloat* v);
    void uniform2i(const LLStaticHashedString& uniform, GLint i, GLint j);
    void uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
    void uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
    void uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
    void uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
    void uniform1i(const LLStaticHashedString& uniform, GLint i);
    void uniform1f(const LLStaticHashedString& uniform, GLfloat v);
    void uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y);
    void uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z);
    void uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform3fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v);
    void uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat *v);

    void setMinimumAlpha(F32 minimum);

    void vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void vertexAttrib4fv(U32 index, GLfloat* v);
    
    //GLint getUniformLocation(const std::string& uniform);
    GLint getUniformLocation(const LLStaticHashedString& uniform);  
    GLint getUniformLocation(U32 index);

    GLint getAttribLocation(U32 attrib);
    GLint mapUniformTextureChannel(GLint location, GLenum type);
    
    void clearPermutations();
    void addPermutation(std::string name, std::string value);
    void removePermutation(std::string name);
    
    //enable/disable texture channel for specified uniform
    //if given texture uniform is active in the shader, 
    //the corresponding channel will be active upon return
    //returns channel texture is enabled in from [0-MAX)
    S32 enableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE, LLTexUnit::eTextureColorSpace space = LLTexUnit::TCS_LINEAR);
    S32 disableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE, LLTexUnit::eTextureColorSpace space = LLTexUnit::TCS_LINEAR);
    
    // bindTexture returns the texture unit we've bound the texture to.
    // You can reuse the return value to unbind a texture when required.
    S32 bindTexture(const std::string& uniform, LLTexture *texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE, LLTexUnit::eTextureColorSpace space = LLTexUnit::TCS_LINEAR);
    S32 bindTexture(S32 uniform, LLTexture *texture, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE, LLTexUnit::eTextureColorSpace space = LLTexUnit::TCS_LINEAR);
    S32 unbindTexture(const std::string& uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    S32 unbindTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
    
    BOOL link(BOOL suppress_errors = FALSE);
    void bind();
    //helper to conditionally bind mRiggedVariant instead of this
    void bind(bool rigged);
    void unbind();

    // Unbinds any previously bound shader by explicitly binding no shader.
    static void bindNoShader(void);

    U32 mMatHash[LLRender::NUM_MATRIX_MODES];
    U32 mLightHash;

    GLhandleARB mProgramObject;
#if LL_RELEASE_WITH_DEBUG_INFO
    struct attr_name
    {
        GLint loc;
        const char *name;
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
    typedef std::unordered_map<GLint, std::string> uniform_name_map_t;
    typedef std::unordered_map<GLint, LLVector4> uniform_value_map_t;
    uniform_name_map_t mUniformNameMap; //lookup map of uniform location to uniform name
    uniform_value_map_t mValue; //lookup map of uniform location to last known value
    std::vector<GLint> mTexture;
    S32 mTotalUniformSize;
    S32 mActiveTextureChannels;
    S32 mShaderLevel;
    S32 mShaderGroup; // see LLGLSLShader::eGroup
    BOOL mUniformsDirty;
    LLShaderFeatures mFeatures;
    std::vector< std::pair< std::string, GLenum > > mShaderFiles;
    std::string mName;
    typedef std::unordered_map<std::string, std::string> defines_map_t;
    defines_map_t mDefines;

    //statistcis for profiling shader performance
    U32 mTimerQuery;
    U32 mSamplesQuery;
    U64 mTimeElapsed;
    static U64 sTotalTimeElapsed;
    U32 mTrianglesDrawn;
    static U32 sTotalTrianglesDrawn;
    U64 mSamplesDrawn;
    static U64 sTotalSamplesDrawn;
    U32 mDrawCalls;
    static U32 sTotalDrawCalls;

    bool mTextureStateFetched;
    std::vector<U32> mTextureMagFilter;
    std::vector<U32> mTextureMinFilter;

    // this pointer should be set to whichever shader represents this shader's rigged variant
    LLGLSLShader* mRiggedVariant = nullptr;

private:
    void unloadInternal();
};

//UI shader (declared here so llui_libtest will link properly)
extern LLGLSLShader         gUIProgram;
//output vec4(color.rgb,color.a*tex0[tc0].a)
extern LLGLSLShader         gSolidColorProgram;
//Alpha mask shader (declared here so llappearance can access properly)
extern LLGLSLShader         gAlphaMaskProgram;


#endif
