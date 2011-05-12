/** 
 * @file llshadermgr.cpp
 * @brief Shader manager implementation.
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

#include "llshadermgr.h"

#include "llfile.h"
#include "llrender.h"

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

LLShaderMgr * LLShaderMgr::sInstance = NULL;

LLShaderMgr::LLShaderMgr()
{
}


LLShaderMgr::~LLShaderMgr()
{
}

// static
LLShaderMgr * LLShaderMgr::instance()
{
	if(NULL == sInstance)
	{
		LL_ERRS("Shaders") << "LLShaderMgr should already have been instantiated by the application!" << LL_ENDL;
	}

	return sInstance;
}

BOOL LLShaderMgr::attachShaderFeatures(LLGLSLShader * shader)
{
	llassert_always(shader != NULL);
	LLShaderFeatures *features = & shader->mFeatures;
	
	//////////////////////////////////////
	// Attach Vertex Shader Features First
	//////////////////////////////////////
	
	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	if (features->calculatesAtmospherics)
	{
		if (!shader->attachObject("windlight/atmosphericsVarsV.glsl"))
		{
			return FALSE;
		}
	}

	if (features->calculatesLighting)
	{
		if (!shader->attachObject("windlight/atmosphericsHelpersV.glsl"))
		{
			return FALSE;
		}
		
		if (features->isSpecular)
		{
			if (!shader->attachObject("lighting/lightFuncSpecularV.glsl"))
			{
				return FALSE;
			}
			
			if (!shader->attachObject("lighting/sumLightsSpecularV.glsl"))
			{
				return FALSE;
			}
			
			if (!shader->attachObject("lighting/lightSpecularV.glsl"))
			{
				return FALSE;
			}
		}
		else 
		{
			if (!shader->attachObject("lighting/lightFuncV.glsl"))
			{
				return FALSE;
			}
			
			if (!shader->attachObject("lighting/sumLightsV.glsl"))
			{
				return FALSE;
			}
			
			if (!shader->attachObject("lighting/lightV.glsl"))
			{
				return FALSE;
			}
		}
	}
	
	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	if (features->calculatesAtmospherics)
	{
		if (!shader->attachObject("windlight/atmosphericsV.glsl"))
		{
			return FALSE;
		}
	}

	if (features->hasSkinning)
	{
		if (!shader->attachObject("avatar/avatarSkinV.glsl"))
		{
			return FALSE;
		}
	}

	if (features->hasObjectSkinning)
	{
		if (!shader->attachObject("avatar/objectSkinV.glsl"))
		{
			return FALSE;
		}
	}
	
	///////////////////////////////////////
	// Attach Fragment Shader Features Next
	///////////////////////////////////////

	if(features->calculatesAtmospherics)
	{
		if (!shader->attachObject("windlight/atmosphericsVarsF.glsl"))
		{
			return FALSE;
		}
	}

	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	if (features->hasGamma)
	{
		if (!shader->attachObject("windlight/gammaF.glsl"))
		{
			return FALSE;
		}
	}
	
	if (features->hasAtmospherics)
	{
		if (!shader->attachObject("windlight/atmosphericsF.glsl"))
		{
			return FALSE;
		}
	}
	
	if (features->hasTransport)
	{
		if (!shader->attachObject("windlight/transportF.glsl"))
		{
			return FALSE;
		}

		// Test hasFullbright and hasShiny and attach fullbright and 
		// fullbright shiny atmos transport if we split them out.
	}

	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	if (features->hasWaterFog)
	{
		if (!shader->attachObject("environment/waterFogF.glsl"))
		{
			return FALSE;
		}
	}
	
	if (features->hasLighting)
	{
	
		if (features->hasWaterFog)
		{
			if (!shader->attachObject("lighting/lightWaterF.glsl"))
			{
				return FALSE;
			}
		}
		
		else
		{
			if (!shader->attachObject("lighting/lightF.glsl"))
			{
				return FALSE;
			}
		}		
	}
	
	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	else if (features->isFullbright)
	{
	
		if (features->isShiny && features->hasWaterFog)
		{
			if (!shader->attachObject("lighting/lightFullbrightShinyWaterF.glsl"))
			{
				return FALSE;
			}
		}
		else if (features->hasWaterFog)
		{
			if (!shader->attachObject("lighting/lightFullbrightWaterF.glsl"))
			{
				return FALSE;
			}
		}
		
		else if (features->isShiny)
		{
			if (!shader->attachObject("lighting/lightFullbrightShinyF.glsl"))
			{
				return FALSE;
			}
		}
		
		else
		{
			if (!shader->attachObject("lighting/lightFullbrightF.glsl"))
			{
				return FALSE;
			}
		}
	}

	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	else if (features->isShiny)
	{
	
		if (features->hasWaterFog)
		{
			if (!shader->attachObject("lighting/lightShinyWaterF.glsl"))
			{
				return FALSE;
			}
		}
		
		else 
		{
			if (!shader->attachObject("lighting/lightShinyF.glsl"))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

//============================================================================
// Load Shader

static std::string get_object_log(GLhandleARB ret)
{
	std::string res;
	
	//get log length 
	GLint length;
	glGetObjectParameterivARB(ret, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
	if (length > 0)
	{
		//the log could be any size, so allocate appropriately
		GLcharARB* log = new GLcharARB[length];
		glGetInfoLogARB(ret, length, &length, log);
		res = std::string((char *)log);
		delete[] log;
	}
	return res;
}

void LLShaderMgr::dumpObjectLog(GLhandleARB ret, BOOL warns) 
{
	std::string log = get_object_log(ret);
	if ( log.length() > 0 )
	{
		if (warns)
		{
			LL_WARNS("ShaderLoading") << log << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("ShaderLoading") << log << LL_ENDL;
		}
	}
}

GLhandleARB LLShaderMgr::loadShaderFile(const std::string& filename, S32 & shader_level, GLenum type)
{
	GLenum error = GL_NO_ERROR;
	if (gDebugGL)
	{
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			LL_WARNS("ShaderLoading") << "GL ERROR entering loadShaderFile(): " << error << LL_ENDL;
		}
	}
	
	LL_DEBUGS("ShaderLoading") << "Loading shader file: " << filename << " class " << shader_level << LL_ENDL;

	if (filename.empty()) 
	{
		return 0;
	}


	//read in from file
	LLFILE* file = NULL;

	S32 try_gpu_class = shader_level;
	S32 gpu_class;

	//find the most relevant file
	for (gpu_class = try_gpu_class; gpu_class > 0; gpu_class--)
	{	//search from the current gpu class down to class 1 to find the most relevant shader
		std::stringstream fname;
		fname << getShaderDirPrefix();
		fname << gpu_class << "/" << filename;
		
 		LL_DEBUGS("ShaderLoading") << "Looking in " << fname.str() << LL_ENDL;
		file = LLFile::fopen(fname.str(), "r");		/* Flawfinder: ignore */
		if (file)
		{
			LL_INFOS("ShaderLoading") << "Loading file: shaders/class" << gpu_class << "/" << filename << " (Want class " << gpu_class << ")" << LL_ENDL;
			break; // done
		}
	}
	
	if (file == NULL)
	{
		LL_WARNS("ShaderLoading") << "GLSL Shader file not found: " << filename << LL_ENDL;
		return 0;
	}

	//we can't have any lines longer than 1024 characters 
	//or any shaders longer than 1024 lines... deal - DaveP
	GLcharARB buff[1024];
	GLcharARB* text[1024];
	GLuint count = 0;


	//copy file into memory
	while( fgets((char *)buff, 1024, file) != NULL && count < LL_ARRAY_SIZE(buff) ) 
	{
		text[count++] = (GLcharARB *)strdup((char *)buff); 
	}
	fclose(file);

	//create shader object
	GLhandleARB ret = glCreateShaderObjectARB(type);
	if (gDebugGL)
	{
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			LL_WARNS("ShaderLoading") << "GL ERROR in glCreateShaderObjectARB: " << error << LL_ENDL;
		}
	}
	
	//load source
	glShaderSourceARB(ret, count, (const GLcharARB**) text, NULL);

	if (gDebugGL)
	{
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			LL_WARNS("ShaderLoading") << "GL ERROR in glShaderSourceARB: " << error << LL_ENDL;
		}
	}

	//compile source
	glCompileShaderARB(ret);

	if (gDebugGL)
	{
		error = glGetError();
		if (error != GL_NO_ERROR)
		{
			LL_WARNS("ShaderLoading") << "GL ERROR in glCompileShaderARB: " << error << LL_ENDL;
		}
	}
		
	//free memory
	for (GLuint i = 0; i < count; i++)
	{
		free(text[i]);
	}
	if (error == GL_NO_ERROR)
	{
		//check for errors
		GLint success = GL_TRUE;
		glGetObjectParameterivARB(ret, GL_OBJECT_COMPILE_STATUS_ARB, &success);
		if (gDebugGL || success == GL_FALSE)
		{
			error = glGetError();
			if (error != GL_NO_ERROR || success == GL_FALSE) 
			{
				//an error occured, print log
				LL_WARNS("ShaderLoading") << "GLSL Compilation Error: (" << error << ") in " << filename << LL_ENDL;
				dumpObjectLog(ret);
				ret = 0;
			}
		}
	}
	else
	{
		ret = 0;
	}
	stop_glerror();

	//successfully loaded, save results
	if (ret)
	{
		// Add shader file to map
		mShaderObjects[filename] = ret;
		shader_level = try_gpu_class;
	}
	else
	{
		if (shader_level > 1)
		{
			shader_level--;
			return loadShaderFile(filename,shader_level,type);
		}
		LL_WARNS("ShaderLoading") << "Failed to load " << filename << LL_ENDL;	
	}
	return ret;
}

BOOL LLShaderMgr::linkProgramObject(GLhandleARB obj, BOOL suppress_errors) 
{
	//check for errors
	glLinkProgramARB(obj);
	GLint success = GL_TRUE;
	glGetObjectParameterivARB(obj, GL_OBJECT_LINK_STATUS_ARB, &success);
	if (!suppress_errors && success == GL_FALSE) 
	{
		//an error occured, print log
		LL_WARNS("ShaderLoading") << "GLSL Linker Error:" << LL_ENDL;
	}

// NOTE: Removing LL_DARWIN block as it doesn't seem to actually give the correct answer, 
// but want it for reference once I move it.
#if 0
	// Force an evaluation of the gl state so the driver can tell if the shader will run in hardware or software
	// per Apple's suggestion   
	glBegin(gGL.mMode);
	glEnd();

	// Query whether the shader can or cannot run in hardware
	// http://developer.apple.com/qa/qa2007/qa1502.html
	long vertexGPUProcessing;
	CGLContextObj ctx = CGLGetCurrentContext();
	CGLGetParameter (ctx, kCGLCPGPUVertexProcessing, &vertexGPUProcessing);	
	long fragmentGPUProcessing;
	CGLGetParameter (ctx, kCGLCPGPUFragmentProcessing, &fragmentGPUProcessing);
	if (!fragmentGPUProcessing || !vertexGPUProcessing)
	{
		LL_WARNS("ShaderLoading") << "GLSL Linker: Running in Software:" << LL_ENDL;
		success = GL_FALSE;
		suppress_errors = FALSE;		
	}
	
#else
	std::string log = get_object_log(obj);
	LLStringUtil::toLower(log);
	if (log.find("software") != std::string::npos)
	{
		LL_WARNS("ShaderLoading") << "GLSL Linker: Running in Software:" << LL_ENDL;
		success = GL_FALSE;
		suppress_errors = FALSE;
	}
#endif
	if (!suppress_errors)
	{
        dumpObjectLog(obj, !success);
	}

	return success;
}

BOOL LLShaderMgr::validateProgramObject(GLhandleARB obj)
{
	//check program validity against current GL
	glValidateProgramARB(obj);
	GLint success = GL_TRUE;
	glGetObjectParameterivARB(obj, GL_OBJECT_VALIDATE_STATUS_ARB, &success);
	if (success == GL_FALSE)
	{
		LL_WARNS("ShaderLoading") << "GLSL program not valid: " << LL_ENDL;
		dumpObjectLog(obj);
	}
	else
	{
		dumpObjectLog(obj, FALSE);
	}

	return success;
}

