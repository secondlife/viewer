/** 
 * @file llglslshader.h
 * @brief GLSL shader wrappers
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLGLSLSHADER_H
#define LL_LLGLSLSHADER_H

#include "llgl.h"
#include "llrender.h"

class LLShaderFeatures
{
public:
	bool calculatesLighting;
	bool calculatesAtmospherics;
	bool hasLighting; // implies no transport (it's possible to have neither though)
	bool isShiny;
	bool isFullbright; // implies no lighting
	bool isSpecular;
	bool hasWaterFog; // implies no gamma
	bool hasTransport; // implies no lighting (it's possible to have neither though)
	bool hasSkinning;	
	bool hasAtmospherics;
	bool hasGamma;

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

	void unload();
	BOOL createShader(std::vector<std::string> * attributes,
						std::vector<std::string> * uniforms);
	BOOL attachObject(std::string object);
	void attachObject(GLhandleARB object);
	void attachObjects(GLhandleARB* objects = NULL, S32 count = 0);
	BOOL mapAttributes(const std::vector<std::string> * attributes);
	BOOL mapUniforms(const std::vector<std::string> * uniforms);
	void mapUniform(GLint index, const std::vector<std::string> * uniforms);
	void uniform1f(U32 index, GLfloat v);
	void uniform2f(U32 index, GLfloat x, GLfloat y);
	void uniform3f(U32 index, GLfloat x, GLfloat y, GLfloat z);
	void uniform4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void uniform1fv(U32 index, U32 count, const GLfloat* v);
	void uniform2fv(U32 index, U32 count, const GLfloat* v);
	void uniform3fv(U32 index, U32 count, const GLfloat* v);
	void uniform4fv(U32 index, U32 count, const GLfloat* v);
	void uniform1f(const std::string& uniform, GLfloat v);
	void uniform2f(const std::string& uniform, GLfloat x, GLfloat y);
	void uniform3f(const std::string& uniform, GLfloat x, GLfloat y, GLfloat z);
	void uniform4f(const std::string& uniform, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
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

	void vertexAttrib4f(U32 index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void vertexAttrib4fv(U32 index, GLfloat* v);
	
	GLint getUniformLocation(const std::string& uniform);
	
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

#endif
