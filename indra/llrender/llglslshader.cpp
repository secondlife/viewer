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

#if LL_DARWIN
#include "OpenGL/OpenGL.h"
#endif

#ifdef LL_RELEASE_FOR_DOWNLOAD
#define UNIFORM_ERRS LL_WARNS_ONCE("Shader")
#else
#define UNIFORM_ERRS LL_ERRS("Shader")
#endif

// Lots of STL stuff in here, using namespace std to keep things more readable
using std::vector;
using std::pair;
using std::make_pair;
using std::string;

GLhandleARB LLGLSLShader::sCurBoundShader = 0;
LLGLSLShader* LLGLSLShader::sCurBoundShaderPtr = NULL;
S32 LLGLSLShader::sIndexedTextureChannels = 0;
bool LLGLSLShader::sNoFixedFunction = false;
bool LLGLSLShader::sProfileEnabled = false;
std::set<LLGLSLShader*> LLGLSLShader::sInstances;
U64 LLGLSLShader::sTotalTimeElapsed = 0;
U32 LLGLSLShader::sTotalTrianglesDrawn = 0;
U64 LLGLSLShader::sTotalSamplesDrawn = 0;
U32 LLGLSLShader::sTotalDrawCalls = 0;

//UI shader -- declared here so llui_libtest will link properly
LLGLSLShader    gUIProgram;
LLGLSLShader    gSolidColorProgram;

BOOL shouldChange(const LLVector4& v1, const LLVector4& v2)
{
    return v1 != v2;
}

LLShaderFeatures::LLShaderFeatures()
    : atmosphericHelpers(false)
    , calculatesLighting(false)
    , calculatesAtmospherics(false)
    , hasLighting(false)
    , isAlphaLighting(false)
    , isShiny(false)
    , isFullbright(false)
    , isSpecular(false)
    , hasWaterFog(false)
    , hasTransport(false)
    , hasSkinning(false)
    , hasObjectSkinning(false)
    , hasAtmospherics(false)
    , hasGamma(false)
    , mIndexedTextureChannels(0)
    , disableTextureIndex(false)
    , hasAlphaMask(false)
    , attachNothing(false)
{
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
    sTotalDrawCalls = 0;

    for (std::set<LLGLSLShader*>::iterator iter = sInstances.begin(); iter != sInstances.end(); ++iter)
    {
        (*iter)->clearStats();
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
void LLGLSLShader::finishProfile(bool emit_report)
{
    sProfileEnabled = false;

    if (emit_report)
    {
        std::vector<LLGLSLShader*> sorted;

        for (std::set<LLGLSLShader*>::iterator iter = sInstances.begin(); iter != sInstances.end(); ++iter)
        {
            sorted.push_back(*iter);
        }

        std::sort(sorted.begin(), sorted.end(), LLGLSLShaderCompareTimeElapsed());

        for (std::vector<LLGLSLShader*>::iterator iter = sorted.begin(); iter != sorted.end(); ++iter)
        {
            (*iter)->dumpStats();
        }
            
    LL_INFOS() << "-----------------------------------" << LL_ENDL;
    LL_INFOS() << "Total rendering time: " << llformat("%.4f ms", sTotalTimeElapsed/1000000.f) << LL_ENDL;
    LL_INFOS() << "Total samples drawn: " << llformat("%.4f million", sTotalSamplesDrawn/1000000.f) << LL_ENDL;
    LL_INFOS() << "Total triangles drawn: " << llformat("%.3f million", sTotalTrianglesDrawn/1000000.f) << LL_ENDL;
    }
}

void LLGLSLShader::clearStats()
{
    mTrianglesDrawn = 0;
    mTimeElapsed = 0;
    mSamplesDrawn = 0;
    mDrawCalls = 0;
    mTextureStateFetched = false;
    mTextureMagFilter.clear();
    mTextureMinFilter.clear();
}

void LLGLSLShader::dumpStats()
{
    if (mDrawCalls > 0)
    {
        LL_INFOS() << "=============================================" << LL_ENDL;
        LL_INFOS() << mName << LL_ENDL;
        for (U32 i = 0; i < mShaderFiles.size(); ++i)
        {
            LL_INFOS() << mShaderFiles[i].first << LL_ENDL;
        }
        for (U32 i = 0; i < mTexture.size(); ++i)
        {
            GLint idx = mTexture[i];
            
            if (idx >= 0)
            {
                GLint uniform_idx = getUniformLocation(i);
                LL_INFOS() << mUniformNameMap[uniform_idx] << " - " << std::hex << mTextureMagFilter[i] << "/" << mTextureMinFilter[i] << std::dec << LL_ENDL;
            }
        }
        LL_INFOS() << "=============================================" << LL_ENDL;
    
        F32 ms = mTimeElapsed/1000000.f;
        F32 seconds = ms/1000.f;

        F32 pct_tris = (F32) mTrianglesDrawn/(F32)sTotalTrianglesDrawn*100.f;
        F32 tris_sec = (F32) (mTrianglesDrawn/1000000.0);
        tris_sec /= seconds;

        F32 pct_samples = (F32) ((F64)mSamplesDrawn/(F64)sTotalSamplesDrawn)*100.f;
        F32 samples_sec = (F32) mSamplesDrawn/1000000000.0;
        samples_sec /= seconds;

        F32 pct_calls = (F32) mDrawCalls/(F32)sTotalDrawCalls*100.f;
        U32 avg_batch = mTrianglesDrawn/mDrawCalls;

        LL_INFOS() << "Triangles Drawn: " << mTrianglesDrawn <<  " " << llformat("(%.2f pct of total, %.3f million/sec)", pct_tris, tris_sec ) << LL_ENDL;
        LL_INFOS() << "Draw Calls: " << mDrawCalls << " " << llformat("(%.2f pct of total, avg %d tris/call)", pct_calls, avg_batch) << LL_ENDL;
        LL_INFOS() << "SamplesDrawn: " << mSamplesDrawn << " " << llformat("(%.2f pct of total, %.3f billion/sec)", pct_samples, samples_sec) << LL_ENDL;
        LL_INFOS() << "Time Elapsed: " << mTimeElapsed << " " << llformat("(%.2f pct of total, %.5f ms)\n", (F32) ((F64)mTimeElapsed/(F64)sTotalTimeElapsed)*100.f, ms) << LL_ENDL;
    }
}

//static
void LLGLSLShader::startProfile()
{
    if (sProfileEnabled && sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->placeProfileQuery();
    }

}

//static
void LLGLSLShader::stopProfile(U32 count, U32 mode)
{
    if (sProfileEnabled && sCurBoundShaderPtr)
    {
        sCurBoundShaderPtr->readProfileQuery(count, mode);
    }
}

void LLGLSLShader::placeProfileQuery()
{
#if !LL_DARWIN
    if (mTimerQuery == 0)
    {
        glGenQueriesARB(1, &mSamplesQuery);
        glGenQueriesARB(1, &mTimerQuery);
    }

    if (!mTextureStateFetched)
    {
        mTextureStateFetched = true;
        mTextureMagFilter.resize(mTexture.size());
        mTextureMinFilter.resize(mTexture.size());

        U32 cur_active = gGL.getCurrentTexUnitIndex();

        for (U32 i = 0; i < mTexture.size(); ++i)
        {
            GLint idx = mTexture[i];

            if (idx >= 0)
            {
                gGL.getTexUnit(idx)->activate();

                U32 mag = 0xFFFFFFFF;
                U32 min = 0xFFFFFFFF;

                U32 type = LLTexUnit::getInternalType(gGL.getTexUnit(idx)->getCurrType());

                glGetTexParameteriv(type, GL_TEXTURE_MAG_FILTER, (GLint*) &mag);
                glGetTexParameteriv(type, GL_TEXTURE_MIN_FILTER, (GLint*) &min);

                mTextureMagFilter[i] = mag;
                mTextureMinFilter[i] = min;
            }
        }

        gGL.getTexUnit(cur_active)->activate();
    }


    glBeginQueryARB(GL_SAMPLES_PASSED, mSamplesQuery);
    glBeginQueryARB(GL_TIME_ELAPSED, mTimerQuery);
#endif
}

void LLGLSLShader::readProfileQuery(U32 count, U32 mode)
{
#if !LL_DARWIN
    glEndQueryARB(GL_TIME_ELAPSED);
    glEndQueryARB(GL_SAMPLES_PASSED);
    
    U64 time_elapsed = 0;
    glGetQueryObjectui64v(mTimerQuery, GL_QUERY_RESULT, &time_elapsed);

    U64 samples_passed = 0;
    glGetQueryObjectui64v(mSamplesQuery, GL_QUERY_RESULT, &samples_passed);

    sTotalTimeElapsed += time_elapsed;
    mTimeElapsed += time_elapsed;

    sTotalSamplesDrawn += samples_passed;
    mSamplesDrawn += samples_passed;

    U32 tri_count = 0;
    switch (mode)
    {
        case LLRender::TRIANGLES: tri_count = count/3; break;
        case LLRender::TRIANGLE_FAN: tri_count = count-2; break;
        case LLRender::TRIANGLE_STRIP: tri_count = count-2; break;
        default: tri_count = count; break; //points lines etc just use primitive count
    }

    mTrianglesDrawn += tri_count;
    sTotalTrianglesDrawn += tri_count;

    sTotalDrawCalls++;
    mDrawCalls++;
#endif
}



LLGLSLShader::LLGLSLShader()
    : mProgramObject(0), 
      mAttributeMask(0),
      mTotalUniformSize(0),
      mActiveTextureChannels(0), 
      mShaderLevel(0), 
      mShaderGroup(SG_DEFAULT), 
      mUniformsDirty(FALSE),
      mTimerQuery(0),
      mSamplesQuery(0)

{
    
}

LLGLSLShader::~LLGLSLShader()
{
}

void LLGLSLShader::unload()
{
    mShaderFiles.clear();
    mDefines.clear();

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
        GLhandleARB obj[1024];
        GLsizei count;

        glGetAttachedObjectsARB(mProgramObject, sizeof(obj)/sizeof(obj[0]), &count, obj);
        for (GLsizei i = 0; i < count; i++)
        {
            glDetachObjectARB(mProgramObject, obj[i]);
                glDeleteObjectARB(obj[i]);
            }

        glDeleteObjectARB(mProgramObject);

        mProgramObject = 0;
    }

    if (mTimerQuery)
    {
        glDeleteQueriesARB(1, &mTimerQuery);
        mTimerQuery = 0;
    }

    if (mSamplesQuery)
    {
        glDeleteQueriesARB(1, &mSamplesQuery);
        mSamplesQuery = 0;
    }

    //hack to make apple not complain
    glGetError();

    stop_glerror();
}

BOOL LLGLSLShader::createShader(std::vector<LLStaticHashedString> * attributes,
                                std::vector<LLStaticHashedString> * uniforms,
                                U32 varying_count,
                                const char** varyings)
{
    unloadInternal();

    sInstances.insert(this);

    //reloading, reset matrix hash values
    for (U32 i = 0; i < LLRender::NUM_MATRIX_MODES; ++i)
    {
        mMatHash[i] = 0xFFFFFFFF;
    }
    mLightHash = 0xFFFFFFFF;

    llassert_always(!mShaderFiles.empty());
    BOOL success = TRUE;

    // Create program
    mProgramObject = glCreateProgramObjectARB();
    
#if LL_DARWIN
    // work-around missing mix(vec3,vec3,bvec3)
    mDefines["OLD_SELECT"] = "1";
#endif
    
    //compile new source
    vector< pair<string,GLenum> >::iterator fileIter = mShaderFiles.begin();
    for ( ; fileIter != mShaderFiles.end(); fileIter++ )
    {
        GLhandleARB shaderhandle = LLShaderMgr::instance()->loadShaderFile((*fileIter).first, mShaderLevel, (*fileIter).second, &mDefines, mFeatures.mIndexedTextureChannels);
        LL_DEBUGS("ShaderLoading") << "SHADER FILE: " << (*fileIter).first << " mShaderLevel=" << mShaderLevel << LL_ENDL;
        if (shaderhandle > 0)
        {
            attachObject(shaderhandle);
        }
        else
        {
            success = FALSE;
        }
    }

    // Attach existing objects
    if (!LLShaderMgr::instance()->attachShaderFeatures(this))
    {
        return FALSE;
    }

    if (gGLManager.mGLSLVersionMajor < 2 && gGLManager.mGLSLVersionMinor < 3)
    { //indexed texture rendering requires GLSL 1.3 or later
        //attachShaderFeatures may have set the number of indexed texture channels, so set to 1 again
        mFeatures.mIndexedTextureChannels = llmin(mFeatures.mIndexedTextureChannels, 1);
    }

#ifdef GL_INTERLEAVED_ATTRIBS
    if (varying_count > 0 && varyings)
    {
        glTransformFeedbackVaryings(mProgramObject, varying_count, varyings, GL_INTERLEAVED_ATTRIBS);
    }
#endif

    // Map attributes and uniforms
    if (success)
    {
        success = mapAttributes(attributes);
    }
    if (success)
    {
        success = mapUniforms(uniforms);
    }
    if( !success )
    {
        LL_WARNS("ShaderLoading") << "Failed to link shader: " << mName << LL_ENDL;

        // Try again using a lower shader level;
        if (mShaderLevel > 0)
        {
            LL_WARNS("ShaderLoading") << "Failed to link using shader level " << mShaderLevel << " trying again using shader level " << (mShaderLevel - 1) << LL_ENDL;
            mShaderLevel--;
            return createShader(attributes,uniforms);
        }
    }
    else if (mFeatures.mIndexedTextureChannels > 0)
    { //override texture channels for indexed texture rendering
        bind();
        S32 channel_count = mFeatures.mIndexedTextureChannels;

        for (S32 i = 0; i < channel_count; i++)
        {
            LLStaticHashedString uniName(llformat("tex%d", i));
            uniform1i(uniName, i);
        }

        S32 cur_tex = channel_count; //adjust any texture channels that might have been overwritten
        for (U32 i = 0; i < mTexture.size(); i++)
        {
            if (mTexture[i] > -1 && mTexture[i] < channel_count)
            {
                llassert(cur_tex < gGLManager.mNumTextureImageUnits);
                uniform1i(i, cur_tex);
                mTexture[i] = cur_tex++;
            }
        }
        unbind();
    }

    return success;
}

BOOL LLGLSLShader::attachObject(std::string object)
{
    if (LLShaderMgr::instance()->mShaderObjects.count(object) > 0)
    {
        stop_glerror();
        glAttachObjectARB(mProgramObject, LLShaderMgr::instance()->mShaderObjects[object]);
        stop_glerror();
        return TRUE;
    }
    else
    {
        LL_WARNS("ShaderLoading") << "Attempting to attach shader object that hasn't been compiled: " << object << LL_ENDL;
        return FALSE;
    }
}

void LLGLSLShader::attachObject(GLhandleARB object)
{
    if (object != 0)
    {
        stop_glerror();
        glAttachObjectARB(mProgramObject, object);
        stop_glerror();
    }
    else
    {
        LL_WARNS("ShaderLoading") << "Attempting to attach non existing shader object. " << LL_ENDL;
    }
}

void LLGLSLShader::attachObjects(GLhandleARB* objects, S32 count)
{
    for (S32 i = 0; i < count; i++)
    {
        attachObject(objects[i]);
    }
}

BOOL LLGLSLShader::mapAttributes(const std::vector<LLStaticHashedString> * attributes)
{
    //before linking, make sure reserved attributes always have consistent locations
    for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
    {
        const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
        glBindAttribLocationARB(mProgramObject, i, (const GLcharARB *) name);
    }
    
    //link the program
    BOOL res = link();

    mAttribute.clear();
    U32 numAttributes = (attributes == NULL) ? 0 : attributes->size();
    mAttribute.resize(LLShaderMgr::instance()->mReservedAttribs.size() + numAttributes, -1);
    
    if (res)
    { //read back channel locations

        mAttributeMask = 0;

        //read back reserved channels first
        for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
        {
            const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
            S32 index = glGetAttribLocationARB(mProgramObject, (const GLcharARB *)name);
            if (index != -1)
            {
                mAttribute[i] = index;
                mAttributeMask |= 1 << i;
                LL_DEBUGS("ShaderLoading") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
            }
        }
        if (attributes != NULL)
        {
            for (U32 i = 0; i < numAttributes; i++)
            {
                const char* name = (*attributes)[i].String().c_str();
                S32 index = glGetAttribLocationARB(mProgramObject, name);
                if (index != -1)
                {
                    mAttribute[LLShaderMgr::instance()->mReservedAttribs.size() + i] = index;
                    LL_DEBUGS("ShaderLoading") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
                }
            }
        }

        return TRUE;
    }
    
    return FALSE;
}

void LLGLSLShader::mapUniform(GLint index, const vector<LLStaticHashedString> * uniforms)
{
    if (index == -1)
    {
        return;
    }

    GLenum type;
    GLsizei length;
    GLint size = -1;
    char name[1024];        /* Flawfinder: ignore */
    name[0] = 0;


    glGetActiveUniformARB(mProgramObject, index, 1024, &length, &size, &type, (GLcharARB *)name);
#if !LL_DARWIN
    if (size > 0)
    {
        switch(type)
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
#endif

    S32 location = glGetUniformLocationARB(mProgramObject, name);
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
        mUniformNameMap[location] = name;
        mUniformMap[hashedName] = location;

        LL_DEBUGS("ShaderLoading") << "Uniform " << name << " is at location " << location << LL_ENDL;
    
        //find the index of this uniform
        for (S32 i = 0; i < (S32) LLShaderMgr::instance()->mReservedUniforms.size(); i++)
        {
            if ( (mUniform[i] == -1)
                && (LLShaderMgr::instance()->mReservedUniforms[i] == name))
            {
                //found it
                mUniform[i] = location;
                mTexture[i] = mapUniformTextureChannel(location, type);
                return;
            }
        }

        if (uniforms != NULL)
        {
            for (U32 i = 0; i < uniforms->size(); i++)
            {
                if ( (mUniform[i+LLShaderMgr::instance()->mReservedUniforms.size()] == -1)
                    && ((*uniforms)[i].String() == name))
                {
                    //found it
                    mUniform[i+LLShaderMgr::instance()->mReservedUniforms.size()] = location;
                    mTexture[i+LLShaderMgr::instance()->mReservedUniforms.size()] = mapUniformTextureChannel(location, type);
                    return;
                }
            }
        }
    }
}

void LLGLSLShader::addPermutation(std::string name, std::string value)
{
    mDefines[name] = value;
}

void LLGLSLShader::removePermutation(std::string name)
{
    mDefines[name].erase();
}

GLint LLGLSLShader::mapUniformTextureChannel(GLint location, GLenum type)
{
    if ((type >= GL_SAMPLER_1D_ARB && type <= GL_SAMPLER_2D_RECT_SHADOW_ARB) ||
        type == GL_SAMPLER_2D_MULTISAMPLE)
    {   //this here is a texture
        glUniform1iARB(location, mActiveTextureChannels);
        LL_DEBUGS("ShaderLoading") << "Assigned to texture channel " << mActiveTextureChannels << LL_ENDL;
        return mActiveTextureChannels++;
    }
    return -1;
}

BOOL LLGLSLShader::mapUniforms(const vector<LLStaticHashedString> * uniforms)
{
	BOOL res = TRUE;

	mTotalUniformSize = 0;
	mActiveTextureChannels = 0;
	mUniform.clear();
	mUniformMap.clear();
	mUniformNameMap.clear();
	mTexture.clear();
	mValue.clear();
	//initialize arrays
	U32 numUniforms = (uniforms == NULL) ? 0 : uniforms->size();
	mUniform.resize(numUniforms + LLShaderMgr::instance()->mReservedUniforms.size(), -1);
	mTexture.resize(numUniforms + LLShaderMgr::instance()->mReservedUniforms.size(), -1);

	bind();

	//get the number of active uniforms
	GLint activeCount;
	glGetObjectParameterivARB(mProgramObject, GL_OBJECT_ACTIVE_UNIFORMS_ARB, &activeCount);

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
	And tickets: MAINT-4165, MAINT-4839, MAINT-3568
	*/


	S32 diffuseMap = glGetUniformLocationARB(mProgramObject, "diffuseMap");
	S32 bumpMap = glGetUniformLocationARB(mProgramObject, "bumpMap");
	S32 environmentMap = glGetUniformLocationARB(mProgramObject, "environmentMap");

	std::set<S32> skip_index;

	if (-1 != diffuseMap && (-1 != bumpMap || -1 != environmentMap))
	{
		GLenum type;
		GLsizei length;
		GLint size = -1;
		char name[1024];

		diffuseMap = bumpMap = environmentMap = -1;

		for (S32 i = 0; i < activeCount; i++)
		{
			name[0] = '\0';

			glGetActiveUniformARB(mProgramObject, i, 1024, &length, &size, &type, (GLcharARB *)name);

			if (-1 == diffuseMap && std::string(name) == "diffuseMap")
			{
				diffuseMap = i;
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
		}

		bool bumpLessDiff = bumpMap < diffuseMap && -1 != bumpMap;
		bool envLessDiff = environmentMap < diffuseMap && -1 != environmentMap;

		if (bumpLessDiff && envLessDiff)
		{
			mapUniform(diffuseMap, uniforms);
			mapUniform(bumpMap, uniforms);
			mapUniform(environmentMap, uniforms);

			skip_index.insert(diffuseMap);
			skip_index.insert(bumpMap);
			skip_index.insert(environmentMap);
		}
		else if (bumpLessDiff)
		{
			mapUniform(diffuseMap, uniforms);
			mapUniform(bumpMap, uniforms);

			skip_index.insert(diffuseMap);
			skip_index.insert(bumpMap);
		}
		else if (envLessDiff)
		{
			mapUniform(diffuseMap, uniforms);
			mapUniform(environmentMap, uniforms);

			skip_index.insert(diffuseMap);
			skip_index.insert(environmentMap);
		}
	}

	//........................................................................................

	for (S32 i = 0; i < activeCount; i++)
	{
		//........................................................................................
		if (skip_index.end() != skip_index.find(i)) continue;
		//........................................................................................

		mapUniform(i, uniforms);
	}
	//........................................................................................................................................

	unbind();

	LL_DEBUGS("ShaderLoading") << "Total Uniform Size: " << mTotalUniformSize << LL_ENDL;
	return res;
}


BOOL LLGLSLShader::link(BOOL suppress_errors)
{
    BOOL success = LLShaderMgr::instance()->linkProgramObject(mProgramObject, suppress_errors);

    if (!suppress_errors)
    {
        LLShaderMgr::instance()->dumpObjectLog(mProgramObject, !success, mName);
    }

    return success;
}

void LLGLSLShader::bind()
{
    gGL.flush();
    if (gGLManager.mHasShaderObjects)
    {
        LLVertexBuffer::unbind();
        glUseProgramObjectARB(mProgramObject);
        sCurBoundShader = mProgramObject;
        sCurBoundShaderPtr = this;
        if (mUniformsDirty)
        {
            LLShaderMgr::instance()->updateShaderUniforms(this);
            mUniformsDirty = FALSE;
        }
    }
}

void LLGLSLShader::unbind()
{
    gGL.flush();
    if (gGLManager.mHasShaderObjects)
    {
        stop_glerror();
        if (gGLManager.mIsNVIDIA)
        {
            for (U32 i = 0; i < mAttribute.size(); ++i)
            {
                vertexAttrib4f(i, 0,0,0,1);
                stop_glerror();
            }
        }
        LLVertexBuffer::unbind();
        glUseProgramObjectARB(0);
        sCurBoundShader = 0;
        sCurBoundShaderPtr = NULL;
        stop_glerror();
    }
}

void LLGLSLShader::bindNoShader(void)
{
    LLVertexBuffer::unbind();
    if (gGLManager.mHasShaderObjects)
    {
        glUseProgramObjectARB(0);
        sCurBoundShader = 0;
        sCurBoundShaderPtr = NULL;
    }
}

S32 LLGLSLShader::bindTexture(const std::string &uniform, LLTexture *texture, LLTexUnit::eTextureType mode)
{
    S32 channel = 0;
    channel = getUniformLocation(uniform);
    
    return bindTexture(channel, texture, mode);
}

S32 LLGLSLShader::bindTexture(S32 uniform, LLTexture *texture, LLTexUnit::eTextureType mode)
{
    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
        return -1;
    }
    
    uniform = mTexture[uniform];
    
    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->bind(texture, mode);
    }
    
    return uniform;
}

S32 LLGLSLShader::unbindTexture(const std::string &uniform, LLTexUnit::eTextureType mode)
{
    S32 channel = 0;
    channel = getUniformLocation(uniform);
    
    return unbindTexture(channel);
}

S32 LLGLSLShader::unbindTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
        return -1;
    }
    
    uniform = mTexture[uniform];
    
    if (uniform > -1)
    {
        gGL.getTexUnit(uniform)->unbind(mode);
    }
    
    return uniform;
}

S32 LLGLSLShader::enableTexture(S32 uniform, LLTexUnit::eTextureType mode)
{
    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
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
    if (uniform < 0 || uniform >= (S32)mTexture.size())
    {
        UNIFORM_ERRS << "Uniform out of range: " << uniform << LL_ENDL;
        return -1;
    }
    S32 index = mTexture[uniform];
    if (index != -1 && gGL.getTexUnit(index)->getCurrType() != LLTexUnit::TT_NONE)
    {
        if (gDebugGL && gGL.getTexUnit(index)->getCurrType() != mode)
        {
            if (gDebugSession)
            {
                gFailLog << "Texture channel " << index << " texture type corrupted." << std::endl;
                ll_fail("LLGLSLShader::disableTexture failed");
            }
            else
            {
                LL_ERRS() << "Texture channel " << index << " texture type corrupted." << LL_ENDL;
            }
        }
        gGL.getTexUnit(index)->disable();
    }
    return index;
}

void LLGLSLShader::uniform1i(U32 index, GLint x)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            if (iter == mValue.end() || iter->second.mV[0] != x)
            {
                glUniform1iARB(mUniform[index], x);
                mValue[mUniform[index]] = LLVector4(x,0.f,0.f,0.f);
            }
        }
    }
}

void LLGLSLShader::uniform1f(U32 index, GLfloat x)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            if (iter == mValue.end() || iter->second.mV[0] != x)
            {
                glUniform1fARB(mUniform[index], x);
                mValue[mUniform[index]] = LLVector4(x,0.f,0.f,0.f);
            }
        }
    }
}

void LLGLSLShader::uniform2f(U32 index, GLfloat x, GLfloat y)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(x,y,0.f,0.f);
            if (iter == mValue.end() || shouldChange(iter->second,vec))
            {
                glUniform2fARB(mUniform[index], x, y);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(x,y,z,0.f);
            if (iter == mValue.end() || shouldChange(iter->second,vec))
            {
                glUniform3fARB(mUniform[index], x, y, z);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(x,y,z,w);
            if (iter == mValue.end() || shouldChange(iter->second,vec))
            {
                glUniform4fARB(mUniform[index], x, y, z, w);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform1iv(U32 index, U32 count, const GLint* v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0],0.f,0.f,0.f);
            if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
            {
                glUniform1ivARB(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform1fv(U32 index, U32 count, const GLfloat* v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0],0.f,0.f,0.f);
            if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
            {
                glUniform1fvARB(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform2fv(U32 index, U32 count, const GLfloat* v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0],v[1],0.f,0.f);
            if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
            {
                glUniform2fvARB(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform3fv(U32 index, U32 count, const GLfloat* v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0],v[1],v[2],0.f);
            if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
            {
                glUniform3fvARB(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniform4fv(U32 index, U32 count, const GLfloat* v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            std::map<GLint, LLVector4>::iterator iter = mValue.find(mUniform[index]);
            LLVector4 vec(v[0],v[1],v[2],v[3]);
            if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
            {
                glUniform4fvARB(mUniform[index], count, v);
                mValue[mUniform[index]] = vec;
            }
        }
    }
}

void LLGLSLShader::uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix2fvARB(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix3fvARB(mUniform[index], count, transpose, v);
        }
    }
}

void LLGLSLShader::uniformMatrix3x4fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v)
{
	if (mProgramObject > 0)
	{	
		if (mUniform.size() <= index)
		{
			UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
			return;
		}

		if (mUniform[index] >= 0)
		{
			glUniformMatrix3x4fv(mUniform[index], count, transpose, v);
		}
	}
}

void LLGLSLShader::uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v)
{
    if (mProgramObject > 0)
    {   
        if (mUniform.size() <= index)
        {
            UNIFORM_ERRS << "Uniform index out of bounds." << LL_ENDL;
            return;
        }

        if (mUniform[index] >= 0)
        {
            glUniformMatrix4fvARB(mUniform[index], count, transpose, v);
        }
    }
}

GLint LLGLSLShader::getUniformLocation(const LLStaticHashedString& uniform)
{
    GLint ret = -1;
    if (mProgramObject > 0)
    {
        LLStaticStringTable<GLint>::iterator iter = mUniformMap.find(uniform);
        if (iter != mUniformMap.end())
        {
            if (gDebugGL)
            {
                stop_glerror();
                if (iter->second != glGetUniformLocationARB(mProgramObject, uniform.String().c_str()))
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
    GLint ret = -1;
    if (mProgramObject > 0)
    {
        llassert(index < mUniform.size());
        return mUniform[index];
    }

    return ret;
}

GLint LLGLSLShader::getAttribLocation(U32 attrib)
{
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
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(v,0.f,0.f,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec))
        {
            glUniform1iARB(location, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2i(const LLStaticHashedString& uniform, GLint i, GLint j)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(i,j,0.f,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec))
        {
            glUniform2iARB(location, i, j);
            mValue[location] = vec;
        }
    }
}


void LLGLSLShader::uniform1f(const LLStaticHashedString& uniform, GLfloat v)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(v,0.f,0.f,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec))
        {
            glUniform1fARB(location, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(x,y,0.f,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec))
        {
            glUniform2fARB(location, x,y);
            mValue[location] = vec;
        }
    }

}

void LLGLSLShader::uniform3f(const LLStaticHashedString& uniform, GLfloat x, GLfloat y, GLfloat z)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(x,y,z,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec))
        {
            glUniform3fARB(location, x,y,z);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform1fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(v[0],0.f,0.f,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
        {
            glUniform1fvARB(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform2fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(v[0],v[1],0.f,0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
        {
            glUniform2fvARB(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform3fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        LLVector4 vec(v[0],v[1],v[2],0.f);
        if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
        {
            glUniform3fvARB(location, count, v);
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniform4fv(const LLStaticHashedString& uniform, U32 count, const GLfloat* v)
{
    GLint location = getUniformLocation(uniform);

    if (location >= 0)
    {
        LLVector4 vec(v);
        std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
        if (iter == mValue.end() || shouldChange(iter->second,vec) || count != 1)
        {
            stop_glerror();
            glUniform4fvARB(location, count, v);
            stop_glerror();
            mValue[location] = vec;
        }
    }
}

void LLGLSLShader::uniformMatrix4fv(const LLStaticHashedString& uniform, U32 count, GLboolean transpose, const GLfloat* v)
{
    GLint location = getUniformLocation(uniform);
                
    if (location >= 0)
    {
        stop_glerror();
        glUniformMatrix4fvARB(location, count, transpose, v);
        stop_glerror();
    }
}


void LLGLSLShader::vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    if (mAttribute[index] > 0)
    {
        glVertexAttrib4fARB(mAttribute[index], x, y, z, w);
    }
}

void LLGLSLShader::vertexAttrib4fv(U32 index, GLfloat* v)
{
    if (mAttribute[index] > 0)
    {
        glVertexAttrib4fvARB(mAttribute[index], v);
    }
}

void LLGLSLShader::setMinimumAlpha(F32 minimum)
{
    gGL.flush();
    uniform1f(LLShaderMgr::MINIMUM_ALPHA, minimum);
}
