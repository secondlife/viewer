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
		if (features->hasWaterFog)
		{
			if (!shader->attachObject("windlight/atmosphericsVarsWaterV.glsl"))
			{
				return FALSE;
			}
		}
		else if (!shader->attachObject("windlight/atmosphericsVarsV.glsl"))
		{
			return FALSE;
		}
	}

	if (features->calculatesLighting || features->atmosphericHelpers)
	{
		if (!shader->attachObject("windlight/atmosphericsHelpersV.glsl"))
		{
			return FALSE;
		}
	}
		
	if (features->calculatesLighting)
	{
		if (features->isSpecular)
		{
			if (!shader->attachObject("lighting/lightFuncSpecularV.glsl"))
			{
				return FALSE;
			}
		
			if (!features->isAlphaLighting)
			{
				if (!shader->attachObject("lighting/sumLightsSpecularV.glsl"))
				{
					return FALSE;
				}
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
			
			if (!features->isAlphaLighting)
			{
				if (!shader->attachObject("lighting/sumLightsV.glsl"))
				{
					return FALSE;
				}
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
		if (features->hasWaterFog)
		{
			if (!shader->attachObject("windlight/atmosphericsVarsWaterF.glsl"))
			{
				return FALSE;
			}
		}
		else if (!shader->attachObject("windlight/atmosphericsVarsF.glsl"))
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
			if (features->disableTextureIndex)
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightWaterAlphaMaskNonIndexedF.glsl"))
					{
						return FALSE;
					}
				}
				else
				{
					if (!shader->attachObject("lighting/lightWaterNonIndexedF.glsl"))
					{
						return FALSE;
					}
				}
			}
			else 
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightWaterAlphaMaskF.glsl"))
					{
						return FALSE;
					}
				}
				else
				{
					if (!shader->attachObject("lighting/lightWaterF.glsl"))
					{
						return FALSE;
					}
				}
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
		
		else
		{
			if (features->disableTextureIndex)
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightAlphaMaskNonIndexedF.glsl"))
					{
						return FALSE;
					}
				}
				else
				{
					if (!shader->attachObject("lighting/lightNonIndexedF.glsl"))
					{
						return FALSE;
					}
				}
			}
			else 
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightAlphaMaskF.glsl"))
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
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
	}
	
	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	else if (features->isFullbright)
	{
	
		if (features->isShiny && features->hasWaterFog)
		{
			if (features->disableTextureIndex)
			{
				if (!shader->attachObject("lighting/lightFullbrightShinyWaterNonIndexedF.glsl"))
				{
					return FALSE;
				}
			}
			else 
			{
				if (!shader->attachObject("lighting/lightFullbrightShinyWaterF.glsl"))
				{
					return FALSE;
				}
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
		else if (features->hasWaterFog)
		{
			if (features->disableTextureIndex)
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightFullbrightWaterNonIndexedAlphaMaskF.glsl"))
					{
						return FALSE;
					}
				}
				else if (!shader->attachObject("lighting/lightFullbrightWaterNonIndexedF.glsl"))
				{
					return FALSE;
				}
			}
			else 
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightFullbrightWaterAlphaMaskF.glsl"))
					{
						return FALSE;
					}
				}
				else if (!shader->attachObject("lighting/lightFullbrightWaterF.glsl"))
				{
					return FALSE;
				}
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
		
		else if (features->isShiny)
		{
			if (features->disableTextureIndex)
			{
				if (!shader->attachObject("lighting/lightFullbrightShinyNonIndexedF.glsl"))
				{
					return FALSE;
				}
			}
			else 
			{
				if (!shader->attachObject("lighting/lightFullbrightShinyF.glsl"))
				{
					return FALSE;
				}
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
		
		else
		{
			if (features->disableTextureIndex)
			{

				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightFullbrightNonIndexedAlphaMaskF.glsl"))
					{
						return FALSE;
					}
				}
				else
				{
					if (!shader->attachObject("lighting/lightFullbrightNonIndexedF.glsl"))
					{
						return FALSE;
					}
				}
			}
			else 
			{
				if (features->hasAlphaMask)
				{
					if (!shader->attachObject("lighting/lightFullbrightAlphaMaskF.glsl"))
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
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
	}

	// NOTE order of shader object attaching is VERY IMPORTANT!!!
	else if (features->isShiny)
	{
	
		if (features->hasWaterFog)
		{
			if (features->disableTextureIndex)
			{
				if (!shader->attachObject("lighting/lightShinyWaterNonIndexedF.glsl"))
				{
					return FALSE;
				}
			}
			else 
			{
				if (!shader->attachObject("lighting/lightShinyWaterF.glsl"))
				{
					return FALSE;
				}
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
		
		else 
		{
			if (features->disableTextureIndex)
			{
				if (!shader->attachObject("lighting/lightShinyNonIndexedF.glsl"))
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
				shader->mFeatures.mIndexedTextureChannels = llmax(LLGLSLShader::sIndexedTextureChannels-1, 1);
			}
		}
	}

	if (features->mIndexedTextureChannels <= 1)
	{
		if (!shader->attachObject("objects/nonindexedTextureV.glsl"))
		{
			return FALSE;
		}
	}
	else
	{
		if (!shader->attachObject("objects/indexedTextureV.glsl"))
		{
			return FALSE;
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
			LL_INFOS("ShaderLoading") << log << LL_ENDL;
		}
	}
 }

GLhandleARB LLShaderMgr::loadShaderFile(const std::string& filename, S32 & shader_level, GLenum type, S32 texture_index_channels)
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
	//or any shaders longer than 4096 lines... deal - DaveP
	GLcharARB buff[1024];
	GLcharARB* text[4096];
	GLuint count = 0;

	S32 major_version = gGLManager.mGLSLVersionMajor;
	S32 minor_version = gGLManager.mGLSLVersionMinor;
	
	if (major_version == 1 && minor_version < 30)
	{
		if (minor_version < 10)
		{
			//should NEVER get here -- if major version is 1 and minor version is less than 10, 
			// viewer should never attempt to use shaders, continuing will result in undefined behavior
			llerrs << "Unsupported GLSL Version." << llendl;
		}

		if (minor_version <= 19)
		{
			text[count++] = strdup("#version 110\n");
			text[count++] = strdup("#define ATTRIBUTE attribute\n");
			text[count++] = strdup("#define VARYING varying\n");
			text[count++] = strdup("#define VARYING_FLAT varying\n");
		}
		else if (minor_version <= 29)
		{
			//set version to 1.20
			text[count++] = strdup("#version 120\n");
			text[count++] = strdup("#define FXAA_GLSL_120 1\n");
			text[count++] = strdup("#define FXAA_FAST_PIXEL_OFFSET 0\n");
			text[count++] = strdup("#define ATTRIBUTE attribute\n");
			text[count++] = strdup("#define VARYING varying\n");
			text[count++] = strdup("#define VARYING_FLAT varying\n");
		}
	}
	else
	{  
		if (major_version < 4)
		{
			//set version to 1.30
			text[count++] = strdup("#version 130\n");

			//some implementations of GLSL 1.30 require integer precision be explicitly declared
			text[count++] = strdup("precision mediump int;\n");
			text[count++] = strdup("precision highp float;\n");
		}
		else
		{ //set version to 400
			text[count++] = strdup("#version 400\n");
		}

		text[count++] = strdup("#define DEFINE_GL_FRAGCOLOR 1\n");
		text[count++] = strdup("#define FXAA_GLSL_130 1\n");

		text[count++] = strdup("#define ATTRIBUTE in\n");

		if (type == GL_VERTEX_SHADER_ARB)
		{ //"varying" state is "out" in a vertex program, "in" in a fragment program 
			// ("varying" is deprecated after version 1.20)
			text[count++] = strdup("#define VARYING out\n");
			text[count++] = strdup("#define VARYING_FLAT flat out\n");
		}
		else
		{
			text[count++] = strdup("#define VARYING in\n");
			text[count++] = strdup("#define VARYING_FLAT flat in\n");
		}

		//backwards compatibility with legacy texture lookup syntax
		text[count++] = strdup("#define texture2D texture\n");
		text[count++] = strdup("#define textureCube texture\n");
		text[count++] = strdup("#define texture2DLod textureLod\n");
		text[count++] = strdup("#define	shadow2D(a,b) vec2(texture(a,b))\n");

		if (major_version > 1 || minor_version >= 40)
		{ //GLSL 1.40 replaces texture2DRect et al with texture
			text[count++] = strdup("#define texture2DRect texture\n");
			text[count++] = strdup("#define shadow2DRect(a,b) vec2(texture(a,b))\n");
		}
	}

	//copy preprocessor definitions into buffer
	for (std::map<std::string,std::string>::iterator iter = mDefinitions.begin(); iter != mDefinitions.end(); ++iter)
	{
		std::string define = "#define " + iter->first + " " + iter->second + "\n";
		text[count++] = (GLcharARB *) strdup(define.c_str());
	}

	if (texture_index_channels > 0 && type == GL_FRAGMENT_SHADER_ARB)
	{
		//use specified number of texture channels for indexed texture rendering

		/* prepend shader code that looks like this:

		uniform sampler2D tex0;
		uniform sampler2D tex1;
		uniform sampler2D tex2;
		.
		.
		.
		uniform sampler2D texN;
		
		VARYING_FLAT ivec4 vary_texture_index;

		vec4 ret = vec4(1,0,1,1);

		vec4 diffuseLookup(vec2 texcoord)
		{
			switch (vary_texture_index.r))
			{
				case 0: ret = texture2D(tex0, texcoord); break;
				case 1: ret = texture2D(tex1, texcoord); break;
				case 2: ret = texture2D(tex2, texcoord); break;
				.
				.
				.
				case N: return texture2D(texN, texcoord); break;
			}

			return ret;
		}
		*/

		//uniform declartion
		for (S32 i = 0; i < texture_index_channels; ++i)
		{
			std::string decl = llformat("uniform sampler2D tex%d;\n", i);
			text[count++] = strdup(decl.c_str());
		}

		if (texture_index_channels > 1)
		{
			text[count++] = strdup("VARYING_FLAT int vary_texture_index;\n");
		}

		text[count++] = strdup("vec4 diffuseLookup(vec2 texcoord)\n");
		text[count++] = strdup("{\n");
		
		
		if (texture_index_channels == 1)
		{ //don't use flow control, that's silly
			text[count++] = strdup("return texture2D(tex0, texcoord);\n");
			text[count++] = strdup("}\n");
		}
		else if (major_version > 1 || minor_version >= 30)
		{  //switches are supported in GLSL 1.30 and later
			if (gGLManager.mIsNVIDIA)
			{ //switches are unreliable on some NVIDIA drivers
				for (U32 i = 0; i < texture_index_channels; ++i)
				{
					std::string if_string = llformat("\t%sif (vary_texture_index == %d) { return texture2D(tex%d, texcoord); }\n", i > 0 ? "else " : "", i, i); 
					text[count++] = strdup(if_string.c_str());
				}
				text[count++] = strdup("\treturn vec4(1,0,1,1);\n");
				text[count++] = strdup("}\n");
			}
			else
			{
				text[count++] = strdup("\tvec4 ret = vec4(1,0,1,1);\n");
				text[count++] = strdup("\tswitch (vary_texture_index)\n");
				text[count++] = strdup("\t{\n");
		
				//switch body
				for (S32 i = 0; i < texture_index_channels; ++i)
				{
					std::string case_str = llformat("\t\tcase %d: return texture2D(tex%d, texcoord);\n", i, i);
					text[count++] = strdup(case_str.c_str());
				}

				text[count++] = strdup("\t}\n");
				text[count++] = strdup("\treturn ret;\n");
				text[count++] = strdup("}\n");
			}
		}
		else
		{ //should never get here.  Indexed texture rendering requires GLSL 1.30 or later 
			// (for passing integers between vertex and fragment shaders)
			llerrs << "Indexed texture rendering requires GLSL 1.30 or later." << llendl;
		}
	}

	//copy file into memory
	while( fgets((char *)buff, 1024, file) != NULL && count < LL_ARRAY_SIZE(text) ) 
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

#if LL_WINDOWS
				std::stringstream ostr;
				//dump shader source for debugging
				for (GLuint i = 0; i < count; i++)
				{
					ostr << i << ": " << text[i];

					if (i % 128 == 0)
					{ //dump every 128 lines

						LL_WARNS("ShaderLoading") << "\n" << ostr.str() << llendl;
						ostr = std::stringstream();
					}

				}

				LL_WARNS("ShaderLoading") << "\n" << ostr.str() << llendl;
#endif // LL_WINDOWS

				ret = 0;
			}
		}
	}
	else
	{
		ret = 0;
	}
	stop_glerror();

	//free memory
	for (GLuint i = 0; i < count; i++)
	{
		free(text[i]);
	}

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
			return loadShaderFile(filename,shader_level,type,texture_index_channels);
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

#if LL_DARWIN

	// For some reason this absolutely kills the frame rate when VBO's are enabled
	if (0)
	{
		// Force an evaluation of the gl state so the driver can tell if the shader will run in hardware or software
		// per Apple's suggestion
		LLGLSLShader::sNoFixedFunction = false;
		
		glUseProgramObjectARB(obj);

		gGL.begin(LLRender::TRIANGLES);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.vertex3f(0.0f, 0.0f, 0.0f);
		gGL.end();
		gGL.flush();
		
		glUseProgramObjectARB(0);
		
		LLGLSLShader::sNoFixedFunction = true;

		// Query whether the shader can or cannot run in hardware
		// http://developer.apple.com/qa/qa2007/qa1502.html
		GLint vertexGPUProcessing, fragmentGPUProcessing;
		CGLContextObj ctx = CGLGetCurrentContext();
		CGLGetParameter(ctx, kCGLCPGPUVertexProcessing, &vertexGPUProcessing);	
		CGLGetParameter(ctx, kCGLCPGPUFragmentProcessing, &fragmentGPUProcessing);
		if (!fragmentGPUProcessing || !vertexGPUProcessing)
		{
			LL_WARNS("ShaderLoading") << "GLSL Linker: Running in Software:" << LL_ENDL;
			success = GL_FALSE;
			suppress_errors = FALSE;		
		}
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

//virtual
void LLShaderMgr::initAttribsAndUniforms()
{
	//MUST match order of enum in LLVertexBuffer.h
	mReservedAttribs.push_back("position");
	mReservedAttribs.push_back("normal");
	mReservedAttribs.push_back("texcoord0");
	mReservedAttribs.push_back("texcoord1");
	mReservedAttribs.push_back("texcoord2");
	mReservedAttribs.push_back("texcoord3");
	mReservedAttribs.push_back("diffuse_color");
	mReservedAttribs.push_back("emissive");
	mReservedAttribs.push_back("binormal");
	mReservedAttribs.push_back("weight");
	mReservedAttribs.push_back("weight4");
	mReservedAttribs.push_back("clothing");
	mReservedAttribs.push_back("texture_index");
	
	//matrix state
	mReservedUniforms.push_back("modelview_matrix");
	mReservedUniforms.push_back("projection_matrix");
	mReservedUniforms.push_back("inv_proj");
	mReservedUniforms.push_back("modelview_projection_matrix");
	mReservedUniforms.push_back("normal_matrix");
	mReservedUniforms.push_back("texture_matrix0");
	mReservedUniforms.push_back("texture_matrix1");
	mReservedUniforms.push_back("texture_matrix2");
	mReservedUniforms.push_back("texture_matrix3");
	llassert(mReservedUniforms.size() == LLShaderMgr::TEXTURE_MATRIX3+1);

	mReservedUniforms.push_back("viewport");

	mReservedUniforms.push_back("light_position");
	mReservedUniforms.push_back("light_direction");
	mReservedUniforms.push_back("light_attenuation");
	mReservedUniforms.push_back("light_diffuse");
	mReservedUniforms.push_back("light_ambient");
	mReservedUniforms.push_back("light_count");
	mReservedUniforms.push_back("light");
	mReservedUniforms.push_back("light_col");
	mReservedUniforms.push_back("far_z");

	llassert(mReservedUniforms.size() == LLShaderMgr::MULTI_LIGHT_FAR_Z+1);


	mReservedUniforms.push_back("proj_mat");
	mReservedUniforms.push_back("proj_near");
	mReservedUniforms.push_back("proj_p");
	mReservedUniforms.push_back("proj_n");
	mReservedUniforms.push_back("proj_origin");
	mReservedUniforms.push_back("proj_range");
	mReservedUniforms.push_back("proj_ambiance");
	mReservedUniforms.push_back("proj_shadow_idx");
	mReservedUniforms.push_back("shadow_fade");
	mReservedUniforms.push_back("proj_focus");
	mReservedUniforms.push_back("proj_lod");
	mReservedUniforms.push_back("proj_ambient_lod");

	llassert(mReservedUniforms.size() == LLShaderMgr::PROJECTOR_AMBIENT_LOD+1);

	mReservedUniforms.push_back("color");
		
	mReservedUniforms.push_back("diffuseMap");
	mReservedUniforms.push_back("specularMap");
	mReservedUniforms.push_back("bumpMap");
	mReservedUniforms.push_back("environmentMap");
	mReservedUniforms.push_back("cloude_noise_texture");
	mReservedUniforms.push_back("fullbright");
	mReservedUniforms.push_back("lightnorm");
	mReservedUniforms.push_back("sunlight_color_copy");
	mReservedUniforms.push_back("ambient");
	mReservedUniforms.push_back("blue_horizon");
	mReservedUniforms.push_back("blue_density");
	mReservedUniforms.push_back("haze_horizon");
	mReservedUniforms.push_back("haze_density");
	mReservedUniforms.push_back("cloud_shadow");
	mReservedUniforms.push_back("density_multiplier");
	mReservedUniforms.push_back("distance_multiplier");
	mReservedUniforms.push_back("max_y");
	mReservedUniforms.push_back("glow");
	mReservedUniforms.push_back("cloud_color");
	mReservedUniforms.push_back("cloud_pos_density1");
	mReservedUniforms.push_back("cloud_pos_density2");
	mReservedUniforms.push_back("cloud_scale");
	mReservedUniforms.push_back("gamma");
	mReservedUniforms.push_back("scene_light_strength");

	llassert(mReservedUniforms.size() == LLShaderMgr::SCENE_LIGHT_STRENGTH+1);

	mReservedUniforms.push_back("center");
	mReservedUniforms.push_back("size");
	mReservedUniforms.push_back("falloff");

	mReservedUniforms.push_back("box_center");
	mReservedUniforms.push_back("box_size");


	mReservedUniforms.push_back("minLuminance");
	mReservedUniforms.push_back("maxExtractAlpha");
	mReservedUniforms.push_back("lumWeights");
	mReservedUniforms.push_back("warmthWeights");
	mReservedUniforms.push_back("warmthAmount");
	mReservedUniforms.push_back("glowStrength");
	mReservedUniforms.push_back("glowDelta");

	llassert(mReservedUniforms.size() == LLShaderMgr::GLOW_DELTA+1);


	mReservedUniforms.push_back("minimum_alpha");

	mReservedUniforms.push_back("shadow_matrix");
	mReservedUniforms.push_back("env_mat");
	mReservedUniforms.push_back("shadow_clip");
	mReservedUniforms.push_back("sun_wash");
	mReservedUniforms.push_back("shadow_noise");
	mReservedUniforms.push_back("blur_size");
	mReservedUniforms.push_back("ssao_radius");
	mReservedUniforms.push_back("ssao_max_radius");
	mReservedUniforms.push_back("ssao_factor");
	mReservedUniforms.push_back("ssao_factor_inv");
	mReservedUniforms.push_back("ssao_effect_mat");
	mReservedUniforms.push_back("screen_res");
	mReservedUniforms.push_back("near_clip");
	mReservedUniforms.push_back("shadow_offset");
	mReservedUniforms.push_back("shadow_bias");
	mReservedUniforms.push_back("spot_shadow_bias");
	mReservedUniforms.push_back("spot_shadow_offset");
	mReservedUniforms.push_back("sun_dir");
	mReservedUniforms.push_back("shadow_res");
	mReservedUniforms.push_back("proj_shadow_res");
	mReservedUniforms.push_back("depth_cutoff");
	mReservedUniforms.push_back("norm_cutoff");
	mReservedUniforms.push_back("shadow_target_width");
	
	llassert(mReservedUniforms.size() == LLShaderMgr::DEFERRED_SHADOW_TARGET_WIDTH+1);

	mReservedUniforms.push_back("tc_scale");
	mReservedUniforms.push_back("rcp_screen_res");
	mReservedUniforms.push_back("rcp_frame_opt");
	mReservedUniforms.push_back("rcp_frame_opt2");
	
	mReservedUniforms.push_back("focal_distance");
	mReservedUniforms.push_back("blur_constant");
	mReservedUniforms.push_back("tan_pixel_angle");
	mReservedUniforms.push_back("magnification");
	mReservedUniforms.push_back("max_cof");
	mReservedUniforms.push_back("res_scale");
	mReservedUniforms.push_back("dof_width");
	mReservedUniforms.push_back("dof_height");

	mReservedUniforms.push_back("depthMap");
	mReservedUniforms.push_back("shadowMap0");
	mReservedUniforms.push_back("shadowMap1");
	mReservedUniforms.push_back("shadowMap2");
	mReservedUniforms.push_back("shadowMap3");
	mReservedUniforms.push_back("shadowMap4");
	mReservedUniforms.push_back("shadowMap5");

	llassert(mReservedUniforms.size() == LLShaderMgr::DEFERRED_SHADOW5+1);

	mReservedUniforms.push_back("normalMap");
	mReservedUniforms.push_back("positionMap");
	mReservedUniforms.push_back("diffuseRect");
	mReservedUniforms.push_back("specularRect");
	mReservedUniforms.push_back("noiseMap");
	mReservedUniforms.push_back("lightFunc");
	mReservedUniforms.push_back("lightMap");
	mReservedUniforms.push_back("bloomMap");
	mReservedUniforms.push_back("projectionMap");

	llassert(mReservedUniforms.size() == END_RESERVED_UNIFORMS);

	std::set<std::string> dupe_check;

	for (U32 i = 0; i < mReservedUniforms.size(); ++i)
	{
		if (dupe_check.find(mReservedUniforms[i]) != dupe_check.end())
		{
			llerrs << "Duplicate reserved uniform name found: " << mReservedUniforms[i] << llendl;
		}
		dupe_check.insert(mReservedUniforms[i]);
	}
}

