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

//UI shader -- declared here so llui_libtest will link properly
LLGLSLShader	gUIProgram;
LLGLSLShader	gSolidColorProgram;

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
{
}

//===============================
// LLGLSL Shader implementation
//===============================
LLGLSLShader::LLGLSLShader()
	: mProgramObject(0), mActiveTextureChannels(0), mShaderLevel(0), mShaderGroup(SG_DEFAULT), mUniformsDirty(FALSE)
{

}

void LLGLSLShader::unload()
{
	stop_glerror();
	mAttribute.clear();
	mTexture.clear();
	mUniform.clear();
	mShaderFiles.clear();

	if (mProgramObject)
	{
		GLhandleARB obj[1024];
		GLsizei count;

		glGetAttachedObjectsARB(mProgramObject, 1024, &count, obj);
		for (GLsizei i = 0; i < count; i++)
		{
#if !LL_DARWIN
			if (glIsProgramARB(obj[i]))
#endif
			{
				glDeleteObjectARB(obj[i]);
			}
		}

		glDeleteObjectARB(mProgramObject);

		mProgramObject = 0;
	}
	
	//hack to make apple not complain
	glGetError();
	
	stop_glerror();
}

BOOL LLGLSLShader::createShader(vector<string> * attributes,
								vector<string> * uniforms,
								U32 varying_count,
								const char** varyings)
{
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
	
	//compile new source
	vector< pair<string,GLenum> >::iterator fileIter = mShaderFiles.begin();
	for ( ; fileIter != mShaderFiles.end(); fileIter++ )
	{
		GLhandleARB shaderhandle = LLShaderMgr::instance()->loadShaderFile((*fileIter).first, mShaderLevel, (*fileIter).second, mFeatures.mIndexedTextureChannels);
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
			uniform1i(llformat("tex%d", i), i);
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

BOOL LLGLSLShader::mapAttributes(const vector<string> * attributes)
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

		//read back reserved channels first
		for (U32 i = 0; i < LLShaderMgr::instance()->mReservedAttribs.size(); i++)
		{
			const char* name = LLShaderMgr::instance()->mReservedAttribs[i].c_str();
			S32 index = glGetAttribLocationARB(mProgramObject, (const GLcharARB *)name);
			if (index != -1)
			{
				mAttribute[i] = index;
				LL_DEBUGS("ShaderLoading") << "Attribute " << name << " assigned to channel " << index << LL_ENDL;
			}
		}
		if (attributes != NULL)
		{
			for (U32 i = 0; i < numAttributes; i++)
			{
				const char* name = (*attributes)[i].c_str();
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

void LLGLSLShader::mapUniform(GLint index, const vector<string> * uniforms)
{
	if (index == -1)
	{
		return;
	}

	GLenum type;
	GLsizei length;
	GLint size;
	char name[1024];		/* Flawfinder: ignore */
	name[0] = 0;

	glGetActiveUniformARB(mProgramObject, index, 1024, &length, &size, &type, (GLcharARB *)name);
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

		mUniformMap[name] = location;
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
					&& ((*uniforms)[i] == name))
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

GLint LLGLSLShader::mapUniformTextureChannel(GLint location, GLenum type)
{
	if (type >= GL_SAMPLER_1D_ARB && type <= GL_SAMPLER_2D_RECT_SHADOW_ARB ||
		type == GL_SAMPLER_2D_MULTISAMPLE)
	{	//this here is a texture
		glUniform1iARB(location, mActiveTextureChannels);
		LL_DEBUGS("ShaderLoading") << "Assigned to texture channel " << mActiveTextureChannels << LL_ENDL;
		return mActiveTextureChannels++;
	}
	return -1;
}

BOOL LLGLSLShader::mapUniforms(const vector<string> * uniforms)
{
	BOOL res = TRUE;
	
	mActiveTextureChannels = 0;
	mUniform.clear();
	mUniformMap.clear();
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

	for (S32 i = 0; i < activeCount; i++)
	{
		mapUniform(i, uniforms);
	}

	unbind();

	return res;
}

BOOL LLGLSLShader::link(BOOL suppress_errors)
{
	return LLShaderMgr::instance()->linkProgramObject(mProgramObject, suppress_errors);
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
				llerrs << "Texture channel " << index << " texture type corrupted." << llendl;
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

GLint LLGLSLShader::getUniformLocation(const string& uniform)
{
	GLint ret = -1;
	if (mProgramObject > 0)
	{
		std::map<string, GLint>::iterator iter = mUniformMap.find(uniform);
		if (iter != mUniformMap.end())
		{
			if (gDebugGL)
			{
				stop_glerror();
				if (iter->second != glGetUniformLocationARB(mProgramObject, uniform.c_str()))
				{
					llerrs << "Uniform does not match." << llendl;
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

void LLGLSLShader::uniform1i(const string& uniform, GLint v)
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

void LLGLSLShader::uniform1f(const string& uniform, GLfloat v)
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

void LLGLSLShader::uniform2f(const string& uniform, GLfloat x, GLfloat y)
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

void LLGLSLShader::uniform3f(const string& uniform, GLfloat x, GLfloat y, GLfloat z)
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

void LLGLSLShader::uniform4f(const string& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
	GLint location = getUniformLocation(uniform);

	if (location >= 0)
	{
		std::map<GLint, LLVector4>::iterator iter = mValue.find(location);
		LLVector4 vec(x,y,z,w);
		if (iter == mValue.end() || shouldChange(iter->second,vec))
		{
			glUniform4fARB(location, x,y,z,w);
			mValue[location] = vec;
		}
	}
}

void LLGLSLShader::uniform1fv(const string& uniform, U32 count, const GLfloat* v)
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

void LLGLSLShader::uniform2fv(const string& uniform, U32 count, const GLfloat* v)
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

void LLGLSLShader::uniform3fv(const string& uniform, U32 count, const GLfloat* v)
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

void LLGLSLShader::uniform4fv(const string& uniform, U32 count, const GLfloat* v)
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

void LLGLSLShader::uniformMatrix2fv(const string& uniform, U32 count, GLboolean transpose, const GLfloat* v)
{
	GLint location = getUniformLocation(uniform);
				
	if (location >= 0)
	{
		glUniformMatrix2fvARB(location, count, transpose, v);
	}
}

void LLGLSLShader::uniformMatrix3fv(const string& uniform, U32 count, GLboolean transpose, const GLfloat* v)
{
	GLint location = getUniformLocation(uniform);
				
	if (location >= 0)
	{
		glUniformMatrix3fvARB(location, count, transpose, v);
	}
}

void LLGLSLShader::uniformMatrix4fv(const string& uniform, U32 count, GLboolean transpose, const GLfloat* v)
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
