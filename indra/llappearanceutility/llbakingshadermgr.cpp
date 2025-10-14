
/**
 * @file LLBakingShaderMgr.cpp
 * @brief Viewer shader manager implementation.
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

#include "llbakingshadermgr.h"

#include "lldir.h"
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

bool                LLBakingShaderMgr::sInitialized = false;
bool                LLBakingShaderMgr::sSkipReload = false;

//utility shaders
LLGLSLShader    gAlphaMaskProgram;


LLBakingShaderMgr::LLBakingShaderMgr() :
    mVertexShaderLevel(SHADER_COUNT, 0),
    mMaxAvatarShaderLevel(0)
{
}

LLBakingShaderMgr::~LLBakingShaderMgr()
{
    mVertexShaderLevel.clear();
    mShaderList.clear();
}

// static
LLBakingShaderMgr * LLBakingShaderMgr::instance()
{
    if(nullptr == sInstance)
    {
        sInstance = new LLBakingShaderMgr();
    }

    return static_cast<LLBakingShaderMgr*>(sInstance);
}

void LLBakingShaderMgr::initAttribsAndUniforms(void)
{
    if (mReservedAttribs.empty())
    {
        LLShaderMgr::initAttribsAndUniforms();
    }
}


//============================================================================
// Set Levels

S32 LLBakingShaderMgr::getVertexShaderLevel(S32 type)
{
    return mVertexShaderLevel[type];
}

//============================================================================
// Shader Management

void LLBakingShaderMgr::setShaders()
{
    //setShaders might be called redundantly by gSavedSettings, so return on reentrance
    static bool reentrance = false;

    if (!sInitialized || reentrance || sSkipReload)
    {
        return;
    }

    LLGLSLShader::sIndexedTextureChannels = llmax(gGLManager.mNumTextureImageUnits, 1);

    //NEVER use more than 16 texture channels (work around for prevalent driver bug)
    LLGLSLShader::sIndexedTextureChannels = llmin(LLGLSLShader::sIndexedTextureChannels, 16);

    if (gGLManager.mGLSLVersionMajor < 1 ||
        (gGLManager.mGLSLVersionMajor == 1 && gGLManager.mGLSLVersionMinor <= 20))
    { //NEVER use indexed texture rendering when GLSL version is 1.20 or earlier
        LLGLSLShader::sIndexedTextureChannels = 1;
    }

    reentrance = true;

    initAttribsAndUniforms();

    // Shaders
    LL_INFOS("ShaderLoading") << "\n~~~~~~~~~~~~~~~~~~\n Loading Shaders:\n~~~~~~~~~~~~~~~~~~" << LL_ENDL;
    LL_INFOS("ShaderLoading") << llformat("Using GLSL %d.%d", gGLManager.mGLSLVersionMajor, gGLManager.mGLSLVersionMinor) << LL_ENDL;

    for (S32 i = 0; i < SHADER_COUNT; i++)
    {
        mVertexShaderLevel[i] = 0;
    }
    mMaxAvatarShaderLevel = 0;

    LLVertexBuffer::unbind();
    bool loaded = false;
    if (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 10)
    {
        S32 light_class = 2;
        mVertexShaderLevel[SHADER_INTERFACE] = light_class;

        // loadBasicShaders
        vector< pair<string, S32> > shaders;
        if (gGLManager.mGLSLVersionMajor >= 2 || gGLManager.mGLSLVersionMinor >= 30)
        {
            shaders.push_back( make_pair( "objects/indexedTextureV.glsl",           1 ) );
        }
        shaders.push_back( make_pair( "objects/nonindexedTextureV.glsl",        1 ) );
        shaders.push_back( make_pair( "deferred/textureUtilV.glsl",             1 ) );
        loaded = true;
        for (U32 i = 0; i < shaders.size(); i++)
        {
            // Note usage of GL_VERTEX_SHADER_ARB
            if (loadShaderFile(shaders[i].first, shaders[i].second, GL_VERTEX_SHADER_ARB) == 0)
            {
                LL_WARNS("Shader") << "Failed to load vertex shader " << shaders[i].first << LL_ENDL;
                loaded = false;
                break;
            }
        }

        shaders.clear();

        // fragment shaders
        shaders.push_back( make_pair( "deferred/globalF.glsl",             1 ) );

        for (U32 i = 0; i < shaders.size(); i++)
        {
            // Note usage of GL_FRAGMENT_SHADER
            if (loadShaderFile(shaders[i].first, shaders[i].second, GL_FRAGMENT_SHADER_ARB) == 0)
            {
                LL_WARNS("Shader") << "Failed to load fragment shader " << shaders[i].first << LL_ENDL;
                loaded = false;
                break;
            }
        }

        if (loaded)
        {
            loaded = loadShadersInterface();
        }
    }

    if (!loaded)
    {
        //gPipeline.mVertexShadersEnabled = false;
        //gPipeline.mVertexShadersLoaded = 0;
        mVertexShaderLevel[SHADER_LIGHTING] = 0;
        mVertexShaderLevel[SHADER_INTERFACE] = 0;
        mVertexShaderLevel[SHADER_ENVIRONMENT] = 0;
        mVertexShaderLevel[SHADER_WATER] = 0;
        mVertexShaderLevel[SHADER_OBJECT] = 0;
        mVertexShaderLevel[SHADER_EFFECT] = 0;
        mVertexShaderLevel[SHADER_WINDLIGHT] = 0;
        mVertexShaderLevel[SHADER_AVATAR] = 0;
    }

    //gPipeline.createGLBuffers();

    reentrance = false;
}

void LLBakingShaderMgr::unloadShaders()
{
    gAlphaMaskProgram.unload();

    mVertexShaderLevel[SHADER_INTERFACE] = 0;

    //gPipeline.mVertexShadersLoaded = 0;
}

bool LLBakingShaderMgr::loadShadersInterface()
{
    gAlphaMaskProgram.mName = "Alpha Mask Shader";
    gAlphaMaskProgram.mShaderFiles.clear();
    gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskV.glsl", GL_VERTEX_SHADER_ARB));
    gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskF.glsl", GL_FRAGMENT_SHADER_ARB));
    gAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];

    if( !gAlphaMaskProgram.createShader() )
    {
        mVertexShaderLevel[SHADER_INTERFACE] = 0;
        return false;
    }

    return true;
}

std::string LLBakingShaderMgr::getShaderDirPrefix(void)
{
    return gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "shaders/class");
}

void LLBakingShaderMgr::updateShaderUniforms(LLGLSLShader * shader)
{
}

LLBakingShaderMgr::shader_iter LLBakingShaderMgr::beginShaders() const
{
    return mShaderList.begin();
}

LLBakingShaderMgr::shader_iter LLBakingShaderMgr::endShaders() const
{
    return mShaderList.end();
}
