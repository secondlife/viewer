
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

BOOL				LLBakingShaderMgr::sInitialized = FALSE;
bool				LLBakingShaderMgr::sSkipReload = false;

//utility shaders
LLGLSLShader	gAlphaMaskProgram;


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
	if(NULL == sInstance)
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

	//setup preprocessor definitions
	LLShaderMgr::instance()->mDefinitions["NUM_TEX_UNITS"] = llformat("%d", gGLManager.mNumTextureImageUnits);
	
	// Make sure the compiled shader map is cleared before we recompile shaders.
	mShaderObjects.clear();
	
	initAttribsAndUniforms();

	// Shaders
	LL_INFOS("ShaderLoading") << "\n~~~~~~~~~~~~~~~~~~\n Loading Shaders:\n~~~~~~~~~~~~~~~~~~" << LL_ENDL;
	LL_INFOS("ShaderLoading") << llformat("Using GLSL %d.%d", gGLManager.mGLSLVersionMajor, gGLManager.mGLSLVersionMinor) << llendl;

	for (S32 i = 0; i < SHADER_COUNT; i++)
	{
		mVertexShaderLevel[i] = 0;
	}
	mMaxAvatarShaderLevel = 0;

	LLGLSLShader::sNoFixedFunction = false;
	LLVertexBuffer::unbind();
	if (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 10)
	{
		//using shaders, disable fixed function
		LLGLSLShader::sNoFixedFunction = true;

		//gPipeline.mVertexShadersEnabled = TRUE;
		//gPipeline.mVertexShadersLoaded = 1;

		loadShadersInterface();
	}
	else
	{
		LLGLSLShader::sNoFixedFunction = false;
		//gPipeline.mVertexShadersEnabled = FALSE;
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

BOOL LLBakingShaderMgr::loadShadersInterface()
{
	BOOL success = TRUE;

	if (success)
	{
		gAlphaMaskProgram.mName = "Alpha Mask Shader";
		gAlphaMaskProgram.mShaderFiles.clear();
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskV.glsl", GL_VERTEX_SHADER_ARB));
		gAlphaMaskProgram.mShaderFiles.push_back(make_pair("interface/alphamaskF.glsl", GL_FRAGMENT_SHADER_ARB));
		gAlphaMaskProgram.mShaderLevel = mVertexShaderLevel[SHADER_INTERFACE];
		success = gAlphaMaskProgram.createShader(NULL, NULL);
	}

	if( !success )
	{
		mVertexShaderLevel[SHADER_INTERFACE] = 0;
		return FALSE;
	}
	
	return TRUE;
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
