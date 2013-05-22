/** 
 * @file llcubemap.cpp
 * @brief LLCubeMap class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llworkerthread.h"

#include "llcubemap.h"

#include "v4coloru.h"
#include "v3math.h"
#include "v3dmath.h"
#include "m3math.h"
#include "m4math.h"

#include "llrender.h"
#include "llglslshader.h"

#include "llglheaders.h"

const F32 epsilon = 1e-7f;
const U16 RESOLUTION = 64;

#if LL_DARWIN
// mipmap generation on cubemap textures seems to be broken on the Mac for at least some cards.
// Since the cubemap is small (64x64 per face) and doesn't have any fine detail, turning off mipmaps is a usable workaround.
const BOOL use_cube_mipmaps = FALSE;
#else
const BOOL use_cube_mipmaps = FALSE;  //current build works best without cube mipmaps
#endif

bool LLCubeMap::sUseCubeMaps = true;

LLCubeMap::LLCubeMap()
	: mTextureStage(0),
	  mTextureCoordStage(0),
	  mMatrixStage(0)
{
	mTargets[0] = GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
	mTargets[1] = GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
	mTargets[2] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
	mTargets[3] = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
	mTargets[4] = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
	mTargets[5] = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
}

LLCubeMap::~LLCubeMap()
{
}

void LLCubeMap::initGL()
{
	llassert(gGLManager.mInited);

	if (gGLManager.mHasCubeMap && LLCubeMap::sUseCubeMaps)
	{
		// Not initialized, do stuff.
		if (mImages[0].isNull())
		{
			U32 texname = 0;
			
			LLImageGL::generateTextures(LLTexUnit::TT_CUBE_MAP, GL_RGB8, 1, &texname);

			for (int i = 0; i < 6; i++)
			{
				mImages[i] = new LLImageGL(64, 64, 4, (use_cube_mipmaps? TRUE : FALSE));
				mImages[i]->setTarget(mTargets[i], LLTexUnit::TT_CUBE_MAP);
				mRawImages[i] = new LLImageRaw(64, 64, 4);
				mImages[i]->createGLTexture(0, mRawImages[i], texname);
				
				gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_CUBE_MAP, texname); 
				mImages[i]->setAddressMode(LLTexUnit::TAM_CLAMP);
				stop_glerror();
			}
			gGL.getTexUnit(0)->disable();
		}
		disable();
	}
	else
	{
		llwarns << "Using cube map without extension!" << llendl;
	}
}

void LLCubeMap::initRawData(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
	bool flip_x[6] =	{ false, true,  false, false, true,  false };
	bool flip_y[6] = 	{ true,  true,  true,  false, true,  true  };
	bool transpose[6] = { false, false, false, false, true,  true  };
	
	// Yes, I know that this is inefficient! - djs 08/08/02
	for (int i = 0; i < 6; i++)
	{
		const U8 *sd = rawimages[i]->getData();
		U8 *td = mRawImages[i]->getData();

		S32 offset = 0;
		S32 sx, sy, so;
		for (int y = 0; y < 64; y++)
		{
			for (int x = 0; x < 64; x++)
			{
				sx = x;
				sy = y;
				if (flip_y[i])
				{
					sy = 63 - y;
				}
				if (flip_x[i])
				{
					sx = 63 - x;
				}
				if (transpose[i])
				{
					S32 temp = sx;
					sx = sy;
					sy = temp;
				}

				so = 64*sy + sx;
				so *= 4;
				*(td + offset++) = *(sd + so++);
				*(td + offset++) = *(sd + so++);
				*(td + offset++) = *(sd + so++);
				*(td + offset++) = *(sd + so++);
			}
		}
	}
}

void LLCubeMap::initGLData()
{
	for (int i = 0; i < 6; i++)
	{
		mImages[i]->setSubImage(mRawImages[i], 0, 0, 64, 64);
	}
}

void LLCubeMap::init(const std::vector<LLPointer<LLImageRaw> >& rawimages)
{
	if (!gGLManager.mIsDisabled)
	{
		initGL();
		initRawData(rawimages);
		initGLData();
	}
}

GLuint LLCubeMap::getGLName()
{
	return mImages[0]->getTexName();
}

void LLCubeMap::bind()
{
	gGL.getTexUnit(mTextureStage)->bind(this);
}

void LLCubeMap::enable(S32 stage)
{
	enableTexture(stage);
	enableTextureCoords(stage);
}

void LLCubeMap::enableTexture(S32 stage)
{
	mTextureStage = stage;
	if (gGLManager.mHasCubeMap && stage >= 0 && LLCubeMap::sUseCubeMaps)
	{
		gGL.getTexUnit(stage)->enable(LLTexUnit::TT_CUBE_MAP);
	}
}

void LLCubeMap::enableTextureCoords(S32 stage)
{
	mTextureCoordStage = stage;
	if (!LLGLSLShader::sNoFixedFunction && gGLManager.mHasCubeMap && stage >= 0 && LLCubeMap::sUseCubeMaps)
	{
		if (stage > 0)
		{
			gGL.getTexUnit(stage)->activate();
		}
		
		glEnable(GL_TEXTURE_GEN_R);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);

		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		
		if (stage > 0)
		{
			gGL.getTexUnit(0)->activate();
		}
	}
}

void LLCubeMap::disable(void)
{
	disableTexture();
	disableTextureCoords();
}

void LLCubeMap::disableTexture(void)
{
	if (gGLManager.mHasCubeMap && mTextureStage >= 0 && LLCubeMap::sUseCubeMaps)
	{
		gGL.getTexUnit(mTextureStage)->disable();
		if (mTextureStage == 0)
		{
			gGL.getTexUnit(0)->enable(LLTexUnit::TT_TEXTURE);
		}
	}
}

void LLCubeMap::disableTextureCoords(void)
{
	if (!LLGLSLShader::sNoFixedFunction && gGLManager.mHasCubeMap && mTextureCoordStage >= 0 && LLCubeMap::sUseCubeMaps)
	{
		if (mTextureCoordStage > 0)
		{
			gGL.getTexUnit(mTextureCoordStage)->activate();
		}
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_R);
		if (mTextureCoordStage > 0)
		{
			gGL.getTexUnit(0)->activate();
		}
	}
}

void LLCubeMap::setMatrix(S32 stage)
{
	mMatrixStage = stage;
	
	if (mMatrixStage < 0) return;
	
	//if (stage > 0)
	{
		gGL.getTexUnit(stage)->activate();
	}

	LLVector3 x(gGLModelView+0);
	LLVector3 y(gGLModelView+4);
	LLVector3 z(gGLModelView+8);

	LLMatrix3 mat3;
	mat3.setRows(x,y,z);
	LLMatrix4 trans(mat3);
	trans.transpose();

	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.pushMatrix();
	gGL.loadMatrix((F32 *)trans.mMatrix);
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	
	/*if (stage > 0)
	{
		gGL.getTexUnit(0)->activate();
	}*/
}

void LLCubeMap::restoreMatrix()
{
	if (mMatrixStage < 0) return;

	//if (mMatrixStage > 0)
	{
		gGL.getTexUnit(mMatrixStage)->activate();
	}
	gGL.matrixMode(LLRender::MM_TEXTURE);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	
	/*if (mMatrixStage > 0)
	{
		gGL.getTexUnit(0)->activate();
	}*/
}

void LLCubeMap::setReflection (void)
{
	gGL.getTexUnit(mTextureStage)->bindManual(LLTexUnit::TT_CUBE_MAP, getGLName());
	mImages[0]->setFilteringOption(LLTexUnit::TFO_ANISOTROPIC);
	mImages[0]->setAddressMode(LLTexUnit::TAM_CLAMP);
}

LLVector3 LLCubeMap::map(U8 side, U16 v_val, U16 h_val) const
{
	LLVector3 dir;

	const U8 curr_coef = side >> 1; // 0/1 = X axis, 2/3 = Y, 4/5 = Z
	const S8 side_dir = (((side & 1) << 1) - 1);  // even = -1, odd = 1
	const U8 i_coef = (curr_coef + 1) % 3;
	const U8 j_coef = (i_coef + 1) % 3;

	dir.mV[curr_coef] = side_dir;

	switch (side)
	{
	case 0: // negative X
		dir.mV[i_coef] = -F32((v_val<<1) + 1) / RESOLUTION + 1;
		dir.mV[j_coef] = F32((h_val<<1) + 1) / RESOLUTION - 1;
		break;
	case 1: // positive X
		dir.mV[i_coef] = -F32((v_val<<1) + 1) / RESOLUTION + 1;
		dir.mV[j_coef] = -F32((h_val<<1) + 1) / RESOLUTION + 1;
		break;
	case 2:	// negative Y
		dir.mV[i_coef] = -F32((v_val<<1) + 1) / RESOLUTION + 1;
		dir.mV[j_coef] = F32((h_val<<1) + 1) / RESOLUTION - 1;
		break;
	case 3:	// positive Y
		dir.mV[i_coef] = F32((v_val<<1) + 1) / RESOLUTION - 1;
		dir.mV[j_coef] = F32((h_val<<1) + 1) / RESOLUTION - 1;
		break;
	case 4:	// negative Z
		dir.mV[i_coef] = -F32((h_val<<1) + 1) / RESOLUTION + 1;
		dir.mV[j_coef] = -F32((v_val<<1) + 1) / RESOLUTION + 1;
		break;
	case 5: // positive Z
		dir.mV[i_coef] = -F32((h_val<<1) + 1) / RESOLUTION + 1;
		dir.mV[j_coef] = F32((v_val<<1) + 1) / RESOLUTION - 1;
		break;
	default:
		dir.mV[i_coef] = F32((v_val<<1) + 1) / RESOLUTION - 1;
		dir.mV[j_coef] = F32((h_val<<1) + 1) / RESOLUTION - 1;
	}

	dir.normVec();
	return dir;
}


BOOL LLCubeMap::project(F32& v_val, F32& h_val, BOOL& outside,
						U8 side, const LLVector3& dir) const
{
	const U8 curr_coef = side >> 1; // 0/1 = X axis, 2/3 = Y, 4/5 = Z
	const S8 side_dir = (((side & 1) << 1) - 1);  // even = -1, odd = 1
	const U8 i_coef = (curr_coef + 1) % 3;
	const U8 j_coef = (i_coef + 1) % 3;

	outside = TRUE;
	if (side_dir * dir.mV[curr_coef] < 0)
		return FALSE;

	LLVector3 ray;

	F32 norm_val = fabs(dir.mV[curr_coef]);

	if (norm_val < epsilon)
		norm_val = 1e-5f;

	ray.mV[curr_coef] = side_dir;
	ray.mV[i_coef] = dir.mV[i_coef] / norm_val;
	ray.mV[j_coef] = dir.mV[j_coef] / norm_val;


	const F32 i_val = (ray.mV[i_coef] + 1) * 0.5f * RESOLUTION;
	const F32 j_val = (ray.mV[j_coef] + 1) * 0.5f * RESOLUTION;

	switch (side)
	{
	case 0: // negative X
		v_val = RESOLUTION - i_val;
		h_val = j_val;
		break;
	case 1: // positive X
		v_val = RESOLUTION - i_val;
		h_val = RESOLUTION - j_val;
		break;
	case 2:	// negative Y
		v_val = RESOLUTION - i_val;
		h_val = j_val;
		break;
	case 3:	// positive Y
		v_val = i_val;
		h_val = j_val;
		break;
	case 4:	// negative Z
		v_val = RESOLUTION - j_val;
		h_val = RESOLUTION - i_val;
		break;
	case 5: // positive Z
		v_val = RESOLUTION - j_val;
		h_val = i_val;
		break;
	default:
		v_val = i_val;
		h_val = j_val;
	}

	outside =  ((v_val < 0) || (v_val > RESOLUTION) ||
		(h_val < 0) || (h_val > RESOLUTION));

	return TRUE;
}

BOOL LLCubeMap::project(F32& v_min, F32& v_max, F32& h_min, F32& h_max, 
						U8 side, LLVector3 dir[4]) const
{
	v_min = h_min = RESOLUTION;
	v_max = h_max = 0;

	BOOL fully_outside = TRUE;
	for (U8 vtx = 0; vtx < 4; ++vtx)
	{
		F32 v_val, h_val;
		BOOL outside;
		BOOL consider = project(v_val, h_val, outside, side, dir[vtx]);
		if (!outside)
			fully_outside = FALSE;
		if (consider)
		{
			if (v_val < v_min) v_min = v_val;
			if (v_val > v_max) v_max = v_val;
			if (h_val < h_min) h_min = h_val;
			if (h_val > h_max) h_max = h_val;
		}
	}

	v_min = llmax(0.0f, v_min);
	v_max = llmin(RESOLUTION - epsilon, v_max);
	h_min = llmax(0.0f, h_min);
	h_max = llmin(RESOLUTION - epsilon, h_max);

	return !fully_outside;
}


void LLCubeMap::paintIn(LLVector3 dir[4], const LLColor4U& col)
{
	F32 v_min, v_max, h_min, h_max;
	LLVector3 center = dir[0] + dir[1] + dir[2] + dir[3];
	center.normVec();

	for (U8 side = 0; side < 6; ++side)
	{
		if (!project(v_min, v_max, h_min, h_max, side, dir))
			continue;

		U8 *td = mRawImages[side]->getData();
		
		U16 v_minu = (U16) v_min;
		U16 v_maxu = (U16) (ceil(v_max) + 0.5);
		U16 h_minu = (U16) h_min;
		U16 h_maxu = (U16) (ceil(h_max) + 0.5);

		for (U16 v = v_minu; v < v_maxu; ++v)
			for (U16 h = h_minu; h < h_maxu; ++h)
		//for (U16 v = 0; v < RESOLUTION; ++v)
		//	for (U16 h = 0; h < RESOLUTION; ++h)
			{
				const LLVector3 ray = map(side, v, h);
				if (ray * center > 0.999)
				{
					const U32 offset = (RESOLUTION * v + h) * 4;
					for (U8 cc = 0; cc < 3; ++cc)
						td[offset + cc] = U8((td[offset + cc] + col.mV[cc]) * 0.5);
				}
			}
		mImages[side]->setSubImage(mRawImages[side], 0, 0, 64, 64);
	}
}

void LLCubeMap::destroyGL()
{
	for (S32 i = 0; i < 6; i++)
	{
		mImages[i] = NULL;
	}
}
