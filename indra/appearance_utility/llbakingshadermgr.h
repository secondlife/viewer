/** 
 * @file llbakingshadermgr.h
 * @brief Texture Baking Shader Manager
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

#ifndef LL_BAKING_SHADER_MGR_H
#define LL_BAKING_SHADER_MGR_H

#include "llshadermgr.h"

class LLBakingShaderMgr: public LLShaderMgr
{
public:
	static BOOL sInitialized;
	static bool sSkipReload;

	LLBakingShaderMgr();
	/* virtual */ ~LLBakingShaderMgr();

	// singleton pattern implementation
	static LLBakingShaderMgr * instance();

	void initAttribsAndUniforms(void);
	void setShaders();
	void unloadShaders();
	S32 getVertexShaderLevel(S32 type);
	BOOL loadShadersInterface();

	std::vector<S32> mVertexShaderLevel;
	S32	mMaxAvatarShaderLevel;

	enum EShaderClass
	{
		SHADER_LIGHTING,
		SHADER_OBJECT,
		SHADER_AVATAR,
		SHADER_ENVIRONMENT,
		SHADER_INTERFACE,
		SHADER_EFFECT,
		SHADER_WINDLIGHT,
		SHADER_WATER,
		SHADER_DEFERRED,
		SHADER_TRANSFORM,
		SHADER_COUNT
	};

	// simple model of forward iterator
	// http://www.sgi.com/tech/stl/ForwardIterator.html
	class shader_iter
	{
	private:
		friend bool operator == (shader_iter const & a, shader_iter const & b);
		friend bool operator != (shader_iter const & a, shader_iter const & b);

		typedef std::vector<LLGLSLShader *>::const_iterator base_iter_t;
	public:
		shader_iter()
		{
		}

		shader_iter(base_iter_t iter) : mIter(iter)
		{
		}

		LLGLSLShader & operator * () const
		{
			return **mIter;
		}

		LLGLSLShader * operator -> () const
		{
			return *mIter;
		}

		shader_iter & operator++ ()
		{
			++mIter;
			return *this;
		}

		shader_iter operator++ (int)
		{
			return mIter++;
		}

	private:
		base_iter_t mIter;
	};

	shader_iter beginShaders() const;
	shader_iter endShaders() const;

	/* virtual */ std::string getShaderDirPrefix(void);

	/* virtual */ void updateShaderUniforms(LLGLSLShader * shader);

private:
	
	// the list of shaders we need to propagate parameters to.
	std::vector<LLGLSLShader *> mShaderList;

}; //LLBakingShaderMgr

inline bool operator == (LLBakingShaderMgr::shader_iter const & a, LLBakingShaderMgr::shader_iter const & b)
{
	return a.mIter == b.mIter;
}

inline bool operator != (LLBakingShaderMgr::shader_iter const & a, LLBakingShaderMgr::shader_iter const & b)
{
	return a.mIter != b.mIter;
}

extern LLVector4			gShinyOrigin;

#endif // LL_BAKING_SHADER_MGR_H
