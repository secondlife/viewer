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
	S32 mIndexedTextureChannels;
	bool disableTextureIndex;
	bool hasAlphaMask;

	// char numLights;
	
	LLShaderFeatures();
};

class LLGLSLShader
{
public:

	enum 
	{
		SG_DEFAULT = 0,
		SG_SKY,
		SG_WATER
	};
	
	LLGLSLShader();

	static GLhandleARB sCurBoundShader;
	static LLGLSLShader* sCurBoundShaderPtr;
	static S32 sIndexedTextureChannels;
	static bool sNoFixedFunction;

	void unload();
	BOOL createShader(std::vector<std::string> * attributes,
						std::vector<std::string> * uniforms,
						U32 varying_count = 0,
						const char** varyings = NULL);
	BOOL attachObject(std::string object);
	void attachObject(GLhandleARB object);
	void attachObjects(GLhandleARB* objects = NULL, S32 count = 0);
	BOOL mapAttributes(const std::vector<std::string> * attributes);
	BOOL mapUniforms(const std::vector<std::string> * uniforms);
	void mapUniform(GLint index, const std::vector<std::string> * uniforms);
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
	void uniform1i(const std::string& uniform, GLint i);
	void uniform1f(const std::string& uniform, GLfloat v);
	void uniform2f(const std::string& uniform, GLfloat x, GLfloat y);
	void uniform3f(const std::string& uniform, GLfloat x, GLfloat y, GLfloat z);
	void uniform4f(const std::string& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void uniform1iv(const std::string& uniform, U32 count, const GLint* i);
	void uniform1fv(const std::string& uniform, U32 count, const GLfloat* v);
	void uniform2fv(const std::string& uniform, U32 count, const GLfloat* v);
	void uniform3fv(const std::string& uniform, U32 count, const GLfloat* v);
	void uniform4fv(const std::string& uniform, U32 count, const GLfloat* v);
	void uniformMatrix2fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
	void uniformMatrix3fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
	void uniformMatrix4fv(U32 index, U32 count, GLboolean transpose, const GLfloat *v);
	void uniformMatrix2fv(const std::string& uniform, U32 count, GLboolean transpose, const GLfloat *v);
	void uniformMatrix3fv(const std::string& uniform, U32 count, GLboolean transpose, const GLfloat *v);
	void uniformMatrix4fv(const std::string& uniform, U32 count, GLboolean transpose, const GLfloat *v);

	void setMinimumAlpha(F32 minimum);

	void vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void vertexAttrib4fv(U32 index, GLfloat* v);
	
	GLint getUniformLocation(const std::string& uniform);
	GLint getUniformLocation(U32 index);

	GLint getAttribLocation(U32 attrib);
	GLint mapUniformTextureChannel(GLint location, GLenum type);
	
	//enable/disable texture channel for specified uniform
	//if given texture uniform is active in the shader, 
	//the corresponding channel will be active upon return
	//returns channel texture is enabled in from [0-MAX)
	S32 enableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE);
	S32 disableTexture(S32 uniform, LLTexUnit::eTextureType mode = LLTexUnit::TT_TEXTURE); 
	
    BOOL link(BOOL suppress_errors = FALSE);
	void bind();
	void unbind();

	// Unbinds any previously bound shader by explicitly binding no shader.
	static void bindNoShader(void);

	U32 mMatHash[LLRender::NUM_MATRIX_MODES];
	U32 mLightHash;

	GLhandleARB mProgramObject;
	std::vector<GLint> mAttribute; //lookup table of attribute enum to attribute channel
	std::vector<GLint> mUniform;   //lookup table of uniform enum to uniform location
	std::map<std::string, GLint> mUniformMap;  //lookup map of uniform name to uniform location
	std::map<GLint, LLVector4> mValue; //lookup map of uniform location to last known value
	std::vector<GLint> mTexture;
	S32 mActiveTextureChannels;
	S32 mShaderLevel;
	S32 mShaderGroup;
	BOOL mUniformsDirty;
	LLShaderFeatures mFeatures;
	std::vector< std::pair< std::string, GLenum > > mShaderFiles;
	std::string mName;
};

//UI shader (declared here so llui_libtest will link properly)
extern LLGLSLShader			gUIProgram;
//output vec4(color.rgb,color.a*tex0[tc0].a)
extern LLGLSLShader			gSolidColorProgram;


#endif
