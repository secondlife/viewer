/**
 * @file llglslshader.cpp
 * @brief GLSL helper functions and state.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "llglslshader.h"

#include "llshadermgr.h"
#include "llfile.h"
#include "llrender.h"
#include "llvertexbuffer.h"
#include "llrendertarget.h"

#include "hbxxh.h"
#include "llsdserialize.h"

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

 // Print-print list of shader included source files that are linked together via glAttachShader()
 // i.e. On macOS / OSX the AMD GLSL linker will display an error if a varying is left in an undefined state.
#define DEBUG_SHADER_INCLUDES 0

// Lots of STL stuff in here, using namespace std to keep things more readable
using std::vector;
using std::pair;
using std::make_pair;
using std::string;

GLuint LLGLSLShader::sCurBoundShader = 0;
LLGLSLShader* LLGLSLShader::sCurBoundShaderPtr = NULL;
S32 LLGLSLShader::sIndexedTextureChannels = 0;
U32 LLGLSLShader::sMaxGLTFMaterials = 0;
U32 LLGLSLShader::sMaxGLTFNodes = 0;
bool LLGLSLShader::sProfileEnabled = false;
std::set<LLGLSLShader*> LLGLSLShader::sInstances;
LLGLSLShader::defines_map_t LLGLSLShader::sGlobalDefines;
U64 LLGLSLShader::sTotalTimeElapsed = 0;
U32 LLGLSLShader::sTotalTrianglesDrawn = 0;
U64 LLGLSLShader::sTotalSamplesDrawn = 0;
U32 LLGLSLShader::sTotalBinds = 0;
boost::json::value LLGLSLShader::sDefaultStats;

//UI shader -- declared here so llui_libtest will link properly
LLGLSLShader    gUIProgram;
LLGLSLShader    gSolidColorProgram;

// NOTE: Keep gShaderConsts* and LLGLSLShader::ShaderConsts_e in sync!
const std::string gShaderConstsKey[LLGLSLShader::NUM_SHADER_CONSTS] =
{
      "LL_SHADER_CONST_CLOUD_MOON_DEPTH"
    , "LL_SHADER_CONST_STAR_DEPTH"
};

// NOTE: Keep gShaderConsts* and LLGLSLShader::ShaderConsts_e in sync!
const std::string gShaderConstsVal[LLGLSLShader::NUM_SHADER_CONSTS] =
{
      "0.99998" // SHADER_CONST_CLOUD_MOON_DEPTH // SL-14113
    , "0.99999" // SHADER_CONST_STAR_DEPTH       // SL-14113
};


bool shouldChange(const LLVector4& v1, const LLVector4& v2)
{
    return v1 != v2;
}

//===============================
// LLGLSL Shader implementation
//===============================

//static
void LLGLSLShader::initProfile()
{
    sProfileEnabled = true;
    sTotalTimeElapsed = 0;
    sTotalTrianglesDrawn = 0;
    sTotalSamplesDrawn = 0;
    sTotalBinds = 0;

    for (auto ptr : sInstances)
    {
        ptr->clearStats();
    }
}


struct LLGLSLShaderCompareTimeElapsed
{
    bool operator()(const LLGLSLShader* const& lhs, const LLGLSLShader* const& rhs)
    {
        return lhs->mTimeElapsed < rhs->mTimeElapsed;
    }
};

//static
void LLGLSLShader::finishProfile(boost::json::value& statsv)
{
    sProfileEnabled = false;

    if (! statsv.is_null())
    {
        std::vector<LLGLSLShader*> sorted(sInstances.begin(), sInstances.end());
        std::sort(sorted.begin(), sorted.end(), LLGLSLShaderCompareTimeElapsed());

        auto& stats = statsv.as_object();
        auto shadersit = stats.emplace("shaders", boost::json::array_kind).first;
        auto& shaders = shadersit->value().as_array();
        bool unbound = false;
        for (auto ptr : sorted)
        {
            if (ptr->mBinds == 0)
            {
                unbound = true;
            }
            else
            {
                auto& shaderit = shaders.emplace_back(boost::json::object_kind);
                ptr->dumpStats(shaderit.as_object());
            }
        }

        constexpr float mega = 1'000'000.f;
        float totalTimeMs = sTotalTimeElapsed / mega;
        LL_INFOS() << "-----------------------------------" << LL_ENDL;
        LL_INFOS() << "Total rendering time: " << llformat("%.4f ms", totalTimeMs) << LL_ENDL;
        LL_INFOS() << "Total samples drawn: " << llformat("%.4f million", sTotalSamplesDrawn / mega) << LL_ENDL;
        LL_INFOS() << "Total triangles drawn: " << llformat("%.3f million", sTotalTrianglesDrawn / mega) << LL_ENDL;
        LL_INFOS() << "-----------------------------------" << LL_ENDL;
        auto totalsit = stats.emplace("totals", boost::json::object_kind).first;
        auto& totals = totalsit->value().as_object();
        totals.emplace("time", totalTimeMs / 1000.0);
        totals.emplace("binds", sTotalBinds);
        totals.emplace("samples", sTotalSamplesDrawn);
        totals.emplace("triangles", sTotalTrianglesDrawn);

        auto unusedit = stats.emplace("unused", boost::json::array_kind).first;
        auto& unused = unusedit->value().as_array();
        if (unbound)
        {
            LL_INFOS() << "The following shaders were unused: " << LL_ENDL;
            for (auto ptr : sorted)
            {
                if (ptr->mBinds == 0)
                {
                    LL_INFOS() << ptr->mName << LL_ENDL;
                    unused.emplace_back(ptr->mName);
                }
            }
        }
    }
}

void LLGLSLShader::clearStats()
{
    mTrianglesDrawn = 0;
    mTimeElapsed = 0;
    mSamplesDrawn = 0;
    mBinds = 0;
}

void LLGLSLShader::dumpStats(boost::json::object& stats)
{
    stats.emplace("name", mName);
    auto filesit = stats.emplace("files", boost::json::array_kind).first;
    auto& files = filesit->value().as_array();
    LL_INFOS() << "=============================================" << LL_ENDL;
    LL_INFOS() << mName << LL_ENDL;
    for (U32 i = 0; i < mShaderFiles.size(); ++i)
    {
        LL_INFOS() << mShaderFiles[i].first << LL_ENDL;
        files.emplace_back(mShaderFiles[i].first);
    }
    LL_INFOS() << "=============================================" << LL_ENDL;

    constexpr float  mega = 1'000'000.f;
    constexpr double giga = 1'000'000'000.0;
    F32 ms = mTimeElapsed / mega;
    F32 seconds = ms / 1000.f;

    F32 pct_tris = (F32)mTrianglesDrawn / (F32)sTotalTrianglesDrawn * 100.f;
    F32 tris_sec = (F32)(mTrianglesDrawn / mega);
    tris_sec /= seconds;

    F32 pct_samples = (F32)((F64)mSamplesDrawn / (F64)sTotalSamplesDrawn) * 100.f;
    F32 samples_sec = (F32)(mSamplesDrawn / giga);
    samples_sec /= seconds;

    F32 pct_binds = (F32)mBinds / (F32)sTotalBinds * 100.f;

    LL_INFOS() << "Triangles Drawn: " << mTrianglesDrawn << " " << llformat("(%.2f pct of total, %.3f million/sec)", pct_tris, tris_sec) << LL_ENDL;
    LL_INFOS() << "Binds: " << mBinds << " " << llformat("(%.2f pct of total)", pct_binds) << LL_ENDL;
    LL_INFOS() << "SamplesDrawn: " << mSamplesDrawn << " " << llformat("(%.2f pct of total, %.3f billion/sec)", pct_samples, samples_sec) << LL_ENDL;
    LL_INFOS() << "Time Elapsed: " << mTimeElapsed << " " << llformat("(%.2f pct of total, %.5f ms)\n", (F32)((F64)mTimeElapsed / (F64)sTotalTimeElapsed) * 100.f, ms) << LL_ENDL;
    stats.emplace("time", seconds);
    stats.emplace("binds", mBinds);
    stats.emplace("samples", mSamplesDrawn);
    stats.emplace("triangles", mTrianglesDrawn);
}

//static
void LLGLSLShader::startProfile()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    if (sProfileEnabled && sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->placeProfileQuery();
    }

}

//static
void LLGLSLShader::stopProfile()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (sProfileEnabled && sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->unbind();
    }
}

void LLGLSLShader::placeProfileQuery(bool for_runtime)
{
    if (sProfileEnabled || for_runtime)
    {
        if (mTimerQuery == 0)
        {
            glGenQueries(1, &mSamplesQuery);
            glGenQueries(1, &mTimerQuery);
            glGenQueries(1, &mPrimitivesQuery);
        }

        glBeginQuery(GL_TIME_ELAPSED, mTimerQuery);

        if (!for_runtime)
        {
            glBeginQuery(GL_SAMPLES_PASSED, mSamplesQuery);
            glBeginQuery(GL_PRIMITIVES_GENERATED, mPrimitivesQuery);
        }
    }
}

bool LLGLSLShader::readProfileQuery(bool for_runtime, bool force_read)
{
    if (sProfileEnabled || for_runtime)
    {
        if (!mProfilePending)
        {
            glEndQuery(GL_TIME_ELAPSED);
            if (!for_runtime)
            {
                glEndQuery(GL_SAMPLES_PASSED);
                glEndQuery(GL_PRIMITIVES_GENERATED);
            }
            mProfilePending = for_runtime;
        }

        if (mProfilePending && for_runtime && !force_read)
        {
            GLuint64 result = 0;
            glGetQueryObjectui64v(mTimerQuery, GL_QUERY_RESULT_AVAILABLE, &result);

            if (result != GL_TRUE)
            {
                return false;
            }
        }

        GLuint64 time_elapsed = 0;
        glGetQueryObjectui64v(mTimerQuery, GL_QUERY_RESULT, &time_elapsed);
        mTimeElapsed += time_elapsed;
        mProfilePending = false;

        if (!for_runtime)
        {
            GLuint64 samples_passed = 0;
            glGetQueryObjectui64v(mSamplesQuery, GL_QUERY_RESULT, &samples_passed);

            GLuint64 primitives_generated = 0;
            glGetQueryObjectui64v(mPrimitivesQuery, GL_QUERY_RESULT, &primitives_generated);
            sTotalTimeElapsed += time_elapsed;

            sTotalSamplesDrawn += samples_passed;
            mSamplesDrawn += samples_passed;

            U32 tri_count = (U32)primitives_generated / 3;

            mTrianglesDrawn += tri_count;
            sTotalTrianglesDrawn += tri_count;

            sTotalBinds++;
            mBinds++;
        }
    }

    return true;
}



LLGLSLShader::LLGLSLShader()
    : mProgramObject(0),
    mAttributeMask(0),
    mTotalUniformSize(0),
    mActiveTextureChannels(0),
    mShaderLevel(0),
    mShaderGroup(SG_DEFAULT),
    mFeatures(),
    mUniformsDirty(false),
    mTimerQuery(0),
    mSamplesQuery(0),
    mPrimitivesQuery(0)
{

}

LLGLSLShader::~LLGLSLShader()
{
}

void LLGLSLShader::unload()
{
    mShaderFiles.clear();
    mDefines.clear();
    mFeatures = LLShaderFeatures();

    unloadInternal();
}

void LLGLSLShader::unloadInternal()
{
    sInstances.erase(this);

    stop_glerror();
    mAttribute.clear();
    mTexture.clear();
    mUniform.clear();

    if (mProgramObject)
    {
        GLuint obj[1024];
        GLsizei count = 0;
        glGetAttachedShaders(mProgramObject, 1024, &count, obj);

        for (GLsizei i = 0; i < count; i++)
        {
            glDetachShader(mProgramObject, obj[i]);
        }

        for (GLsizei i = 0; i < count; i++)
        {
            if (glIsShader(obj[i]))
            {
                glDeleteShader(obj[i]);
            }
        }

        glDeleteProgram(mProgramObject);

        mProgramObject = 0;
    }

    if (mTimerQuery)
    {
        glDeleteQueries(1, &mTimerQuery);
        mTimerQuery = 0;
    }

    if (mSamplesQuery)
    {
        glDeleteQueries(1, &mSamplesQuery);
        mSamplesQuery = 0;
    }

    //hack to make apple not complain
    glGetError();

    stop_glerror();
}

bool LLGLSLShader::createShader()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    unloadInternal();

    sInstances.insert(this);

    //reloading, reset matrix hash values
    for (U32 i = 0; i < LLRender::NUM_MATRIX_MODES; ++i)
    {
        mMatHash[i] = 0xFFFFFFFF;
    }
    mLightHash = 0xFFFFFFFF;

    llassert_always(!mShaderFiles.empty());

#if LL_DARWIN
    // work-around missing mix(vec3,vec3,bvec3)
    mDefines["OLD_SELECT"] = "1";
#endif

    mShaderHash = hash();

    // Create program
    mProgramObject = glCreateProgram();
    if (mProgramObject == 0)
    {
        // Shouldn't happen if shader related extensions, like ARB_vertex_shader, exist.
        LL_SHADER_LOADING_WARNS() << "Failed to create handle for shader: " << mName << LL_ENDL;
        unloadInternal();
        return false;
    }

    bool success = true;

    mUsingBinaryProgram =  LLShaderMgr::instance()->loadCachedProgramBinary(this);

    if (!mUsingBinaryProgram)
    {
#if DEBUG_SHADER_INCLUDES
        fprintf(stderr, "--- %s ---\n", mName.c_str());
#endif // DEBUG_SHADER_INCLUDES

        //compile new source
        vector< pair<string, GLenum> >::iterator fileIter = mShaderFiles.begin();
        for (; fileIter != mShaderFiles.end(); fileIter++)
        {
            GLuint shaderhandle = LLShaderMgr::instance()->loadShaderFile((*fileIter).first, mShaderLevel, (*fileIter).second, &mDefines, mFeatures.mIndexedTextureChannels);
            LL_DEBUGS("ShaderLoading") << "SHADER FILE: " << (*fileIter).first << " mShaderLevel=" << mShaderLevel << LL_ENDL;
            if (shaderhandle)
            {
                attachObject(shaderhandle);
            }
            else
            {
                success = false;
            }
        }
    }

    // Attach existing objects
    if (!LLShaderMgr::instance()->attachShaderFeatures(this))
    {
        unloadInternal();
        return false;
    }
    // Map attributes and uniforms
    if (success)
    {
        success = mapAttributes();
    }
    if (success)
    {
        success = mapUniforms();
    }
    if (!success)
    {
        LL_SHADER_LOADING_WARNS() << "Failed to link shader: " << mName << LL_ENDL;

        // Try again using a lower shader level;
        if (mShaderLevel > 0)
        {
            LL_SHADER_LOADING_WARNS() << "Failed to link using shader level " << mShaderLevel << " trying again using shader level " << (mShaderLevel - 1) << LL_ENDL;
            mShaderLevel--;
            return createShader();
        }
        else
        {
            // Give up and unload shader.
            unloadInternal();
        }
    }
    else if (mFeatures.mIndexedTextureChannels > 0)
    { //override texture channels for indexed texture rendering
        llassert(mFeatures.mIndexedTextureChannels == LLGLSLShader::sIndexedTextureChannels); // these numbers must always match
        bind();
        S32 channel_count = mFeatures.mIndexedTextureChannels;

        for (S32 i = 0; i < channel_count; i++)
        {
            LLStaticHashedString uniName(llformat("tex%d", i));
            uniform1i(uniName, i);
        }

        //adjust any texture channels that might have been overwritten
        for (U32 i = 0; i < mTexture.size(); i++)
        {
            if (mTexture[i] > -1)
            {
                S32 new_tex = mTexture[i] + channel_count;
                uniform1i(i, new_tex);
                mTexture[i] = new_tex;
            }
        }

        // get the true number of active texture channels
        mActiveTextureChannels = channel_count;
        for (auto& tex : mTexture)
        {
            mActiveTextureChannels = llmax(mActiveTextureChannels, tex + 1);
        }

        // when indexed texture channels are used, enforce an upper limit of 16
        // this should act as a canary in the coal mine for adding textures
        // and breaking machines that are limited to 16 texture channels
        llassert(mActiveTextureChannels <= 16);
        unbind();
    }

    LL_DEBUGS("GLSLTextureChannels") << mName << " has " << mActiveTextureChannels << " active texture channels" << LL_ENDL;

    for (U32 i = 0; i < mTexture.size(); i++)
    {
        if (mTexture[i] > -1)
        {
            LL_DEBUGS("GLSLTextureChannels") << "Texture " << LLShaderMgr::instance()->mReservedUniforms[i] << " assigned to channel " << mTexture[i] << LL_ENDL;
        }
    }

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
    setLabel(mName.c_str());
#endif

    return success;
}

#if DEBUG_SHADER_INCLUDES
void dumpAttachObject(const char* func_name, GLuint program_object, const std::string& object_path)
{
    GLchar* info_log;
    GLint      info_len_expect = 0;
    GLint      info_len_actual = 0;

    glGetShaderiv(program_object, GL_INFO_LOG_LENGTH, , &info_len_expect);
    fprintf(stderr, " * %-20s(), log size: %d, %s\n", func_name, info_len_expect, object_path.c_str());

    if (info_len_expect > 0)
    {
        fprintf(stderr, " ========== %s() ========== \n", func_name);
        info_log = new GLchar[info_len_expect];
        glGetProgramInfoLog(program_object, info_len_expect, &info_len_actual, info_log);
        fprintf(stderr, "%s\n", info_log);
        delete[] info_log;
    }
}
#endif // DEBUG_SHADER_INCLUDES

bool LLGLSLShader::attachVertexObject(std::string object_path)
{
    if (LLShaderMgr::instance()->mVertexShaderObjects.count(object_path) > 0)
    {
        stop_glerror();
        glAttachShader(mProgramObject, LLShaderMgr::instance()->mVertexShaderObjects[object_path]);
#if DEBUG_SHADER_INCLUDES
        dumpAttachObject("attachVertexObject", mProgramObject, object_path);
#endif // DEBUG_SHADER_INCLUDES
        stop_glerror();
        return true;
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach shader object: '" << object_path << "' that hasn't been compiled." << LL_ENDL;
        return false;
    }
}

bool LLGLSLShader::attachFragmentObject(std::string object_path)
{
    if(mUsingBinaryProgram)
        return true;

    if (LLShaderMgr::instance()->mFragmentShaderObjects.count(object_path) > 0)
    {
        stop_glerror();
        glAttachShader(mProgramObject, LLShaderMgr::instance()->mFragmentShaderObjects[object_path]);
#if DEBUG_SHADER_INCLUDES
        dumpAttachObject("attachFragmentObject", mProgramObject, object_path);
#endif // DEBUG_SHADER_INCLUDES
        stop_glerror();
        return true;
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach shader object: '" << object_path << "' that hasn't been compiled." << LL_ENDL;
        return false;
    }
}

void LLGLSLShader::attachObject(GLuint object)
{
    if(mUsingBinaryProgram)
        return;

    if (object != 0)
    {
        stop_glerror();
        glAttachShader(mProgramObject, object);
#if DEBUG_SHADER_INCLUDES
        std::string object_path("???");
        dumpAttachObject("attachObject", mProgramObject, object_path);
#endif // DEBUG_SHADER_INCLUDES
        stop_glerror();
    }
    else
    {
        LL_SHADER_LOADING_WARNS() << "Attempting to attach non existing shader object. " << LL_ENDL;
    }
}

void LLGLSLShader::attachObjects(GLuint* objects, S32 count)
{
    if(mUsingBinaryProgram)
        return;

    for (S32 i = 0; i < count; i++)
    {
        attachObject(objects[i]);
    }
}

bool LLGLSLShader::mapAttributes()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool res = true;
    if (!mUsingBinaryProgram)
    {
        //before linking, make sure reserved attributes always have consistent locations
        for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
        {
            const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
            glBindAttribLocation(mProgramObject, i, (const GLchar*)name);
        }

        //link the program
        res = link();
    }

    mAttribute.clear();
#if LL_RELEASE_WITH_DEBUG_INFO
    mAttribute.resize(LLShaderMgr::instance()->mReservedAttribs.size(), { -1, NULL });
#else
    mAttribute.resize(LLShaderMgr::instance()->mReservedAttribs.size(), -1);
#endif

    if (res)
    { //read back channel locations

        mAttributeMask = 0;

        //read back reserved channels first
        for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
        {
            const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
            S32 index = glGetAttribLocation(mProgramObject, (const GLchar*)name);
            if (index != -1)
            {
#if LL_RELEASE_WITH_DEBUG_INFO
                mAttribute[i] = { index, name };
#else
                mAttribute[i] = index;
#endif
                mAttributeMask |= 1 << i;
                LL_DEBUGS("ShaderUniform") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
            }
        }

        return true;
    }

    return false;
}

void LLGLSLShader::mapUniform(GLint index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (index == -1)
    {
        return;
    }

    GLenum type;
    GLsizei length;
    GLint size = -1;
    char name[1024];        /* Flawfinder: ignore */
    name[0] = 0;


    glGetActiveUniform(mProgramObject, index, 1024, &length, &size, &type, (GLchar*)name);
    if (size > 0)
    {
        switch (type)
        {
        case GL_FLOAT_VEC2: size *= 2; break;
        case GL_FLOAT_VEC3: size *= 3; break;
        case GL_FLOAT_VEC4: size *= 4; break;
        case GL_DOUBLE: size *= 2; break;
        case GL_DOUBLE_VEC2: size *= 2; break;
        case GL_DOUBLE_VEC3: size *= 6; break;
        case GL_DOUBLE_VEC4: size *= 8; break;
        case GL_INT_VEC2: size *= 2; break;
        case GL_INT_VEC3: size *= 3; break;
        case GL_INT_VEC4: size *= 4; break;
        case GL_UNSIGNED_INT_VEC2: size *= 2; break;
        case GL_UNSIGNED_INT_VEC3: size *= 3; break;
        case GL_UNSIGNED_INT_VEC4: size *= 4; break;
        case GL_BOOL_VEC2: size *= 2; break;
        case GL_BOOL_VEC3: size *= 3; break;
        case GL_BOOL_VEC4: size *= 4; break;
        case GL_FLOAT_MAT2: size *= 4; break;
        case GL_FLOAT_MAT3: size *= 9; break;
        case GL_FLOAT_MAT4: size *= 16; break;
        case GL_FLOAT_MAT2x3: size *= 6; break;
        case GL_FLOAT_MAT2x4: size *= 8; break;
        case GL_FLOAT_MAT3x2: size *= 6; break;
        case GL_FLOAT_MAT3x4: size *= 12; break;
        case GL_FLOAT_MAT4x2: size *= 8; break;
        case GL_FLOAT_MAT4x3: size *= 12; break;
        case GL_DOUBLE_MAT2: size *= 8; break;
        case GL_DOUBLE_MAT3: size *= 18; break;
        case GL_DOUBLE_MAT4: size *= 32; break;
        case GL_DOUBLE_MAT2x3: size *= 12; break;
        case GL_DOUBLE_MAT2x4: size *= 16; break;
        case GL_DOUBLE_MAT3x2: size *= 12; break;
        case GL_DOUBLE_MAT3x4: size *= 24; break;
        case GL_DOUBLE_MAT4x2: size *= 16; break;
        case GL_DOUBLE_MAT4x3: size *= 24; break;
        }
        mTotalUniformSize += size;
    }

    S32 location = glGetUniformLocation(mProgramObject, name);
    if (location != -1)
    {
        //chop off "[0]" so we can always access the first element
        //of an array by the array name
        char* is_array = strstr(name, "[0]");
        if (is_array)
        {
            is_array[0] = 0;
        }

        LLStaticHashedString hashedName(name);
        mUniformMap[hashedName] = location;

        LL_DEBUGS("ShaderUniform") << "Uniform " << name << " is at location " << location << LL_ENDL;

        //find the index of this uniform
        for (S32 i = 0; i < (S32)LLShaderMgr::instance()->mReservedUniforms.size(); i++)
        {
            if ((mUniform[i] == -1)
                && (LLShaderMgr::instance()->mReservedUniforms[i] == name))
            {
                //found it
                mUniform[i] = location;
                mTexture[i] = mapUniformTextureChannel(location, type, size);
                if (mTexture[i] != -1)
                {
                    LL_DEBUGS("GLSLTextureChannels") << name << " assigned to texture channel " << mTexture[i] << LL_ENDL;
                }
                return;
            }
        }
    }
}

void LLGLSLShader::clearPermutations()
{
    mDefines.clear();
}

void LLGLSLShader::addPermutation(std::string name, std::string value)
{
    mDefines[name] = value;
}

void LLGLSLShader::addConstant(const LLGLSLShader::eShaderConsts shader_const)
{
    addPermutation(gShaderConstsKey[shader_const], gShaderConstsVal[shader_const]);
}

void LLGLSLShader::removePermutation(std::string name)
{
    mDefines.erase(name);
}

GLint LLGLSLShader::mapUniformTextureChannel(GLint location, GLenum type, GLint size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if ((type >= GL_SAMPLER_1D && type <= GL_SAMPLER_2D_RECT_SHADOW) ||
        type == GL_SAMPLER_2D_MULTISAMPLE ||
        type == GL_SAMPLER_CUBE_MAP_ARRAY)
    {   //this here is a texture
        GLint ret = mActiveTextureChannels;
        if (size == 1)
        {
            glUniform1i(location, mActiveTextureChannels);
            mActiveTextureChannels++;
        }
        else
        {
            //is array of textures, make sequential after this texture
            GLint channel[16]; // <=== only support up to 16 texture channels
            llassert(size <= 16);
            size = llmin(size, 16);
            for (int i = 0; i < size; ++i)
            {
                channel[i] = mActiveTextureChannels++;
            }
            glUniform1iv(location, size, channel);
        }

        return ret;
    }
    return -1;
}

bool LLGLSLShader::mapUniforms()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool res = true;

    mTotalUniformSize = 0;
    mActiveTextureChannels = 0;
    mUniform.clear();
    mUniformMap.clear();
    mTexture.clear();
    mValue.clear();
    //initialize arrays
    mUniform.resize(LLShaderMgr::instance()->mReservedUniforms.size(), -1);
    mTexture.resize(LLShaderMgr::instance()->mReservedUniforms.size(), -1);

    bind();

    //get the number of active uniforms
    GLint activeCount;
    glGetProgramiv(mProgramObject, GL_ACTIVE_UNIFORMS, &activeCount);

    //........................................................................................................................................
    //........................................................................................

    /*
    EXPLANATION:
    This is part of code is temporary because as the final result the mapUniform() should be rewrited.
    But it's a huge a volume of work which is need to be a more carefully performed for avoid possible
    regression's (i.e. it should be formalized a separate ticket in JIRA).

    RESON:
    The reason of this code is that SL engine is very sensitive to fact that "diffuseMap" should be appear
    first as uniform parameter which is should get 0-"texture channel" index (see mapUniformTextureChannel() and mActiveTextureChannels)
    it influence to which is texture matrix will be updated during rendering.

    But, order of indexe's of uniform variables is not defined and GLSL compiler can change it as want
    , even if the "diffuseMap" will be appear and use first in shader code.

    As example where this situation appear see: "Deferred Material Shader 28/29/30/31"
    And tickets: MAINT-4165, MAINT-4839, MAINT-3568, MAINT-6437

    --- davep TODO -- pretty sure the entire block here is superstitious and that the uniform index has nothing to do with the texture channel
                texture channel should follow the uniform VALUE
    */


    S32 diffuseMap = glGetUniformLocation(mProgramObject, "diffuseMap");
    S32 specularMap = glGetUniformLocation(mProgramObject, "specularMap");
    S32 bumpMap = glGetUniformLocation(mProgramObject, "bumpMap");
    S32 altDiffuseMap = glGetUniformLocation(mProgramObject, "altDiffuseMap");
    S32 environmentMap = glGetUniformLocation(mProgramObject, "environmentMap");
    S32 reflectionMap = glGetUniformLocation(mProgramObject, "reflectionMap");

    std::set<S32> skip_index;

    if (-1 != diffuseMap && (-1 != specularMap || -1 != bumpMap || -1 != environmentMap || -1 != altDiffuseMap))
    {
        GLenum type;
        GLsizei length;
        GLint size = -1;
        char name[1024];

        diffuseMap = altDiffuseMap = specularMap = bumpMap = environmentMap = -1;

        for (S32 i = 0; i < activeCount; i++)
        {
            name[0] = '\0';

            glGetActiveUniform(mProgramObject, i, 1024, &length, &size, &type, (GLchar*)name);

            if (-1 == diffuseMap && std::string(name) == "diffuseMap")
            {
                diffuseMap = i;
                continue;
            }

            if (-1 == specularMap && std::string(name) == "specularMap")
            {
                specularMap = i;
                continue;
            }

            if (-1 == bumpMap && std::string(name) == "bumpMap")
            {
                bumpMap = i;
                continue;
            }

            if (-1 == environmentMap && std::string(name) == "environmentMap")
            {
                environmentMap = i;
                continue;
            }

            if (-1 == reflectionMap && std::string(name) == "reflectionMap")
            {
                reflectionMap = i;
                continue;
            }

            if (-1 == altDiffuseMap && std::string(name) == "altDiffuseMap")
            {
                altDiffuseMap = i;
                continue;
            }
        }

        bool specularDiff = specularMap < diffuseMap && -1 != specularMap;
        bool bumpLessDiff = bumpMap < diffuseMap && -1 != bumpMap;
        bool envLessDiff = environmentMap < diffuseMap && -1 != environmentMap;
        bool refLessDiff = reflectionMap < diffuseMap && -1 != reflectionMap;

        if (specularDiff || bumpLessDiff || envLessDiff || refLessDiff)
        {
            mapUniform(diffuseMap);
            skip_index.insert(diffuseMap);

            if (-1 != specularMap) {
                mapUniform(specularMap);
                skip_index.insert(specularMap);
            }

            if (-1 != bumpMap) {
                mapUniform(bumpMap);
                skip_index.insert(bumpMap);
            }

            if (-1 != environmentMap) {
                mapUniform(environmentMap);
                skip_index.insert(environmentMap);
            }

            if (-1 != reflectionMap) {
                mapUniform(reflectionMap);
                skip_index.insert(reflectionMap);
            }
        }
    }

    //........................................................................................

    for (S32 i = 0; i < activeCount; i++)
    {
        //........................................................................................
        if (skip_index.end() != skip_index.find(i)) continue;
        //........................................................................................

        mapUniform(i);
    }
    //........................................................................................................................................

    // Set up block binding, in a way supported by Apple (rather than binding = 1 in .glsl).
    // See slide 35 and more of https://docs.huihoo.com/apple/wwdc/2011/session_420__advances_in_opengl_for_mac_os_x_lion.pdf
    const char* ubo_names[] =
    {
        "ReflectionProbes", // UB_REFLECTION_PROBES
        "GLTFJoints",       // UB_GLTF_JOINTS
        "GLTFNodes",        // UB_GLTF_NODES
        "GLTFMaterials",    // UB_GLTF_MATERIALS
    };

    llassert(LL_ARRAY_SIZE(ubo_names) == NUM_UNIFORM_BLOCKS);

    for (U32 i = 0; i < NUM_UNIFORM_BLOCKS; ++i)
    {
        GLuint UBOBlockIndex = glGetUniformBlockIndex(mProgramObject, ubo_names[i]);
        if (UBOBlockIndex != GL_INVALID_INDEX)
        {
            glUniformBlockBinding(mProgramObject, UBOBlockIndex, i);
        }
    }

    unbind();

    LL_DEBUGS("ShaderUniform") << "Total Uniform Size: " << mTotalUniformSize << LL_ENDL;
    return res;
}


bool LLGLSLShader::link(bool suppress_errors)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    bool success = LLShaderMgr::instance()->linkProgramObject(mProgramObject, suppress_errors);

    if (!success && !suppress_errors)
    {
        LLShaderMgr::instance()->dumpObjectLog(mProgramObject, !success, mName);
    }

    if (success)
    {
        LLShaderMgr::instance()->saveCachedProgramBinary(this);
    }

    return success;
}

void LLGLSLShader::bind()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    llassert_always(mProgramObject != 0);

    gGL.flush();

    if (sCurBoundShader != mProgramObject)  // Don't re-bind current shader
    {
        if (sCurBoundShaderPtr)
        {
            sCurBoundShaderPtr->readProfileQuery();
        }
        LLVertexBuffer::unbind();
        glUseProgram(mProgramObject);
        sCurBoundShader = mProgramObject;
        sCurBoundShaderPtr = this;
        placeProfileQuery();
        LLVertexBuffer::setupClientArrays(mAttributeMask);
    }

    if (mUniformsDirty)
    {
        LLShaderMgr::instance()->updateShaderUniforms(this);
        mUniformsDirty = false;
    }

    llassert_always(sCurBoundShaderPtr != nullptr);
    llassert_always(sCurBoundShader == mProgramObject);
}

void LLGLSLShader::bind(U8 variant)
{
    llassert_always(mGLTFVariants.size() == LLGLSLShader::NUM_GLTF_VARIANTS);
    llassert_always(variant < LLGLSLShader::NUM_GLTF_VARIANTS);
    mGLTFVariants[variant].bind();
}

void LLGLSLShader::bind(bool rigged)
{
    if (rigged)
    {
        llassert_always(mRiggedVariant);
        mRiggedVariant->bind();
    }
    else
    {
        bind();
    }
}

void LLGLSLShader::unbind(void)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    gGL.flush();
    LLVertexBuffer::unbind();

    if (sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->readProfileQuery();
    }

    glUseProgram(0);
    sCurBoundShader = 0;
    sCurBoundShaderPtr = NULL;
}

S32 LLGLSLShader::bindTexture(const std::string& uniform, LLTexture* texture, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    S32 channel = 0;
    channel = getUniformLocation(uniform);

    return bindTexture(channel, texture, mode);
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLTexture* texture, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    uniform = mTexture[uniform];

    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->bindFast(texture);
    }

    return uniform;
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLRenderTarget* texture, bool depth, LLTexUnit::eTextureFilterOptions mode, U32 index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    uniform = getTextureChannel(uniform);

    if (uniform > -1)
    {
        if (depth) {
            gGL.getTexUnit(uniform)->bind(texture, true);
        }
        else {
            bool has_mips = mode == LLTexUnit::TFO_TRILINEAR || mode == LLTexUnit::TFO_ANISOTROPIC;
            gGL.getTexUnit(uniform)->bindManual(texture->getUsage(), texture->getTexture(index), has_mips);
        }

        gGL.getTexUnit(uniform)->setTextureFilteringOption(mode);
    }

    return uniform;
}

S32 LLGLSLShader::bindTexture(const std::string& uniform, LLRenderTarget* texture, bool depth, LLTexUnit::eTextureFilterOptions mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    S32 channel = 0;
    channel = getUniformLocation(uniform);

    return bindTexture(channel, texture, depth, mode);
}

S32 LLGLSLShader::unbindTexture(const std::string& uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    S32 channel = 0;
    channel = getUniformLocation(uniform);

    return unbindTexture(channel);
}

S32 LLGLSLShader::unbindTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    uniform = mTexture[uniform];

    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->unbindFast(mode);
    }

    return uniform;
}

S32 LLGLSLShader::getTextureChannel(S32 uniform) const
{
    return mTexture[uniform];
}

S32 LLGLSLShader::enableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }


    S32 index = mTexture[uniform];
    if (index != -1)
    {
        gGL.getTexUnit(index)->activate();
        gGL.getTexUnit(index)->enable(mode);
    }
    return index;
}

S32 LLGLSLShader::disableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << uniform << LL_ENDL;
        llassert(false);
        return -1;
    }

    S32 index = mTexture[uniform];
    if (index < 0)
    {
        // Invalid texture index - nothing to disable
        return index;
    }

    LLTexUnit* tex_unit = gGL.getTexUnit(index);
    if (!tex_unit)
    {
        // Invalid texture unit
        LL_WARNS_ONCE("Shader") << "Invalid texture unit at index: " << index << LL_ENDL;
        return index;
    }

    LLTexUnit::eTextureType curr_type = tex_unit->getCurrType();
    if (curr_type != LLTexUnit::TT_NONE)
    {
        if (gDebugGL && curr_type != mode)
        {
            if (gDebugSession)
            {
                gFailLog << "Texture channel " << index << " texture type corrupted. Expected: " << mode << ", Found: " << curr_type << std::endl;
                ll_fail("LLGLSLShader::disableTexture failed");
            }
            else
            {
                LL_ERRS() << "Texture channel " << index << " texture type corrupted. Expected: " << mode << ", Found: " << curr_type << LL_ENDL;
            }
        }
        tex_unit->disable();
    }

    return index;
}

void LLGLSLShader::uniform1i(U32 index, GLint x)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);
    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            if (iter == mValue.end() || iter->second.mV[0] != x)
            {
                glUniform1i(mUniform[index], x);
                mValue[mUniform[index]] = LLVector4((F32)x, 0.f, 0.f, 0.f);
            }
        }
    }
}

void LLGLSLShader::uniform1f(U32 index, GLfloat x)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            if (iter == mValue.end() || iter->second.mV[0] != x)
            {
                glUniform1f(mUniform[index], x);
                mValue[mUniform[index]] = LLVector4(x, 0.f, 0.f, 0.f);
            }
        }
    }
}

void LLGLSLShader::fastUniform1f(U32 index, GLfloat x)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);
    llassert(mProgramObject);
    llassert(mUniform.size() <= index);
    llassert(mUniform[index] >= 0);
    glUniform1f(mUniform[index], x);
}

void LLGLSLShader::uniform2f(U32 index, GLfloat x, GLfloat y)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(x, y, 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec))
            {
                glUniform2f(mUniform[index], x, y);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(x, y, z, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec))
            {
                glUniform3f(mUniform[index], x, y, z);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(x, y, z, w);
            if (iter == mValue.end() || shouldChange(iter->second, vec))
            {
                glUniform4f(mUniform[index], x, y, z, w);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform1iv(U32 index, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], 0.f, 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform1iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4iv(U32 index, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform1iv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}


void LLGLSLShader::uniform1fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], 0.f, 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform1fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform2fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], v[1], 0.f, 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform2fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform3fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], v[1], v[2], 0.f);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                glUniform3fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4fv(U32 index, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0], v[1], v[2], v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
                glUniform4fv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4uiv(U32 index, U32 count, const GLuint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            const auto& iter = mValue.find(mUniform[index]);
            LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
            if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
            {
                LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
                glUniform4uiv(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix2fv(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix3fv(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix3x4fv(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    llassert(sCurBoundShaderPtr == this);

    if (mProgramObject)
    {
        if (mUniform.size() <= index)
        {
            LL_WARNS_ONCE("Shader") << "Uniform index out of bounds. Size: " << (S32)mUniform.size() << " index: " << index << LL_ENDL;
            llassert(false);
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix4fv(mUniform[index], count, transpose, v);
        }
    }
}

GLint LLGLSLShader::getUniformLocation(const LLStaticHashedString& uniform)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    GLint ret = -1;
    if (mProgramObject)
    {
        LLStaticStringTable<GLint>::iterator iter = mUniformMap.find(uniform);
        if (iter != mUniformMap.end())
        {
            if (gDebugGL)
            {
                stop_glerror();
                if (iter->second != glGetUniformLocation(mProgramObject, uniform.String().c_str()))
                {
                    LL_ERRS() << "Uniform does not match." << LL_ENDL;
                }
                stop_glerror();
            }
            ret = iter->second;
        }
    }

    return ret;
}

GLint LLGLSLShader::getUniformLocation(U32 index)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    GLint ret = -1;
    if (mProgramObject)
    {
        if (index >= mUniform.size())
        {
            LL_WARNS_ONCE("Shader") << "Uniform index " << index << " out of bounds " << (S32)mUniform.size() << LL_ENDL;
            return ret;
        }
        return mUniform[index];
    }

    return ret;
}

GLint LLGLSLShader::getAttribLocation(U32 attrib)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;

    if (attrib < mAttribute.size())
    {
        return mAttribute[attrib];
    }
    else
    {
        return -1;
    }
}

void LLGLSLShader::uniform1i(const LLStaticHashedString& uniform, GLint v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec((F32)v, 0.f, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform1i(location, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform1iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], 0.f, 0.f, 0.f);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform1iv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4iv(const LLStaticHashedString& uniform, U32 count, const GLint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4iv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2i(const LLStaticHashedString& uniform, GLint i, GLint j)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec((F32)i, (F32)j, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform2i(location, i, j);
            mValue[location] = vec;
        }
    }
}


void LLGLSLShader::uniform1f(const LLStaticHashedString& uniform, GLfloat v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v, 0.f, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform1f(location, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(x, y, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform2f(location, x, y);
            mValue[location] = vec;
        }
    }

}

void LLGLSLShader::uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(x, y, z, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform3f(location, x, y, z);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(x, y, z, w);
        if (iter == mValue.end() || shouldChange(iter->second, vec))
        {
            glUniform4f(location, x, y, z, w);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v[0], 0.f, 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            glUniform1fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v[0], v[1], 0.f, 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            glUniform2fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform3fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        const auto& iter = mValue.find(location);
        LLVector4 vec(v[0], v[1], v[2], 0.f);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            glUniform3fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec(v);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4fv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4uiv(const LLStaticHashedString& uniform, U32 count, const GLuint* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec((F32)v[0], (F32)v[1], (F32)v[2], (F32)v[3]);
        const auto& iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second, vec) || count != 1)
        {
            LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
            glUniform4uiv(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        stop_glerror();
        glUniformMatrix4fv(location, count, transpose, v);
        stop_glerror();
    }
}


void LLGLSLShader::vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (mAttribute[index] > 0)
    {
        glVertexAttrib4f(mAttribute[index], x, y, z, w);
    }
}

void LLGLSLShader::vertexAttrib4fv(U32 index, GLfloat* v)
{
    if (mAttribute[index] > 0)
    {
        glVertexAttrib4fv(mAttribute[index], v);
    }
}

void LLGLSLShader::setMinimumAlpha(F32 minimum)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    gGL.flush();
    uniform1f(LLShaderMgr::MINIMUM_ALPHA, minimum);
}

void LLShaderUniforms::apply(LLGLSLShader* shader)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SHADER;
    for (auto& uniform : mIntegers)
    {
        shader->uniform1i(uniform.mUniform, uniform.mValue);
    }

    for (auto& uniform : mFloats)
    {
        shader->uniform1f(uniform.mUniform, uniform.mValue);
    }

    for (auto& uniform : mVectors)
    {
        shader->uniform4fv(uniform.mUniform, 1, uniform.mValue.mV);
    }

    for (auto& uniform : mVector3s)
    {
        shader->uniform3fv(uniform.mUniform, 1, uniform.mValue.mV);
    }
}

LLUUID LLGLSLShader::hash()
{
    HBXXH128 hash_obj;
    hash_obj.update(mName);
    hash_obj.update(&mShaderGroup, sizeof(mShaderGroup));
    hash_obj.update(&mShaderLevel, sizeof(mShaderLevel));
    for (const auto& shdr_pair : mShaderFiles)
    {
        hash_obj.update(shdr_pair.first);
        hash_obj.update(&shdr_pair.second, sizeof(GLenum));
    }
    for (const auto& define_pair : mDefines)
    {
        hash_obj.update(define_pair.first);
        hash_obj.update(define_pair.second);

    }
    for (const auto& define_pair : LLGLSLShader::sGlobalDefines)
    {
        hash_obj.update(define_pair.first);
        hash_obj.update(define_pair.second);

    }
    hash_obj.update(&mFeatures, sizeof(LLShaderFeatures));
    hash_obj.update(gGLManager.mGLVendor);
    hash_obj.update(gGLManager.mGLRenderer);
    hash_obj.update(gGLManager.mGLVersionString);
    return hash_obj.digest();
}

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
void LLGLSLShader::setLabel(const char* label) {
    LL_LABEL_OBJECT_GL(GL_PROGRAM, mProgramObject, strlen(label), label);
}
#endif
