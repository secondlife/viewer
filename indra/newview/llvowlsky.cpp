/** 
 * @file llvowlsky.cpp
 * @brief LLVOWLSky class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "pipeline.h"

#include "llvowlsky.h"
#include "llsky.h"
#include "lldrawpoolwlsky.h"
#include "llface.h"
#include "llviewercontrol.h"
#include "llenvironment.h"
#include "llsettingssky.h"

static const U32 MIN_SKY_DETAIL = 8;
static const U32 MAX_SKY_DETAIL = 180;

inline U32 LLVOWLSky::getNumStacks(void)
{
	return llmin(MAX_SKY_DETAIL, llmax(MIN_SKY_DETAIL, gSavedSettings.getU32("WLSkyDetail")));
}

inline U32 LLVOWLSky::getNumSlices(void)
{
	return 2 * llmin(MAX_SKY_DETAIL, llmax(MIN_SKY_DETAIL, gSavedSettings.getU32("WLSkyDetail")));
}

inline U32 LLVOWLSky::getStripsNumVerts(void)
{
	return (getNumStacks() - 1) * getNumSlices();
}

inline U32 LLVOWLSky::getStripsNumIndices(void)
{
	return 2 * ((getNumStacks() - 2) * (getNumSlices() + 1)) + 1 ; 
}

inline U32 LLVOWLSky::getStarsNumVerts(void)
{
	return 1000;
}

inline U32 LLVOWLSky::getStarsNumIndices(void)
{
	return 1000;
}

LLVOWLSky::LLVOWLSky(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp)
	: LLStaticViewerObject(id, pcode, regionp, TRUE)
{
	initStars();
}

void LLVOWLSky::idleUpdate(LLAgent &agent, const F64 &time)
{
	
}

BOOL LLVOWLSky::isActive(void) const
{
	return FALSE;
}

LLDrawable * LLVOWLSky::createDrawable(LLPipeline * pipeline)
{
	pipeline->allocDrawable(this);

	//LLDrawPoolWLSky *poolp = static_cast<LLDrawPoolWLSky *>(
		gPipeline.getPool(LLDrawPool::POOL_WL_SKY);

	mDrawable->setRenderType(LLPipeline::RENDER_TYPE_WL_SKY);

	return mDrawable;
}

inline F32 LLVOWLSky::calcPhi(U32 i)
{
	// i should range from [0..SKY_STACKS] so t will range from [0.f .. 1.f]
	F32 t = float(i) / float(getNumStacks());

	// ^4 the parameter of the tesselation to bias things toward 0 (the dome's apex)
	t *= t;
	t *= t;

	// invert and square the parameter of the tesselation to bias things toward 1 (the horizon)
	t = 1.f - t;
	t = t*t;
	t = 1.f - t;

	return (F_PI / 8.f) * t;
}

void LLVOWLSky::resetVertexBuffers()
{
	mStripsVerts.clear();
	mStarsVerts = nullptr;
    mFsSkyVerts = nullptr;

	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
}
	
void LLVOWLSky::cleanupGL()
{
	mStripsVerts.clear();
	mStarsVerts = nullptr;
    mFsSkyVerts = nullptr;

	LLDrawPoolWLSky::cleanupGL();
}

void LLVOWLSky::restoreGL()
{
	LLDrawPoolWLSky::restoreGL();
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
}

static LLTrace::BlockTimerStatHandle FTM_GEO_SKY("Windlight Sky Geometry");

BOOL LLVOWLSky::updateGeometry(LLDrawable * drawable)
{
    LL_RECORD_BLOCK_TIME(FTM_GEO_SKY);
	LLStrider<LLVector3>	vertices;
	LLStrider<LLVector2>	texCoords;
	LLStrider<U16>			indices;

    if (mFsSkyVerts.isNull())
    {
        mFsSkyVerts = new LLVertexBuffer(LLDrawPoolWLSky::ADV_ATMO_SKY_VERTEX_DATA_MASK, GL_STATIC_DRAW_ARB);

        if (!mFsSkyVerts->allocateBuffer(4, 6, TRUE))
		{
			LL_WARNS() << "Failed to allocate Vertex Buffer on full screen sky update" << LL_ENDL;
		}

		BOOL success = mFsSkyVerts->getVertexStrider(vertices)
			        && mFsSkyVerts->getTexCoord0Strider(texCoords)
			        && mFsSkyVerts->getIndexStrider(indices);

		if(!success) 
		{
			LL_ERRS() << "Failed updating WindLight fullscreen sky geometry." << LL_ENDL;
		}

        *vertices++ = LLVector3(-1.0f, -1.0f, 0.0f);
        *vertices++ = LLVector3( 1.0f, -1.0f, 0.0f);
        *vertices++ = LLVector3(-1.0f,  1.0f, 0.0f);
        *vertices++ = LLVector3( 1.0f,  1.0f, 0.0f);

		*texCoords++ = LLVector2(0.0f, 0.0f);
        *texCoords++ = LLVector2(1.0f, 0.0f);
        *texCoords++ = LLVector2(0.0f, 1.0f);
        *texCoords++ = LLVector2(1.0f, 1.0f);

		*indices++ = 0;
		*indices++ = 1;
		*indices++ = 2;
        *indices++ = 1;
		*indices++ = 3;
		*indices++ = 2;

        mFsSkyVerts->flush();
    }

	{
		const U32 max_buffer_bytes = gSavedSettings.getS32("RenderMaxVBOSize")*1024;
		const U32 data_mask = LLDrawPoolWLSky::SKY_VERTEX_DATA_MASK;
		const U32 max_verts = max_buffer_bytes / LLVertexBuffer::calcVertexSize(data_mask);

		const U32 total_stacks = getNumStacks();

		const U32 verts_per_stack = getNumSlices();

		// each seg has to have one more row of verts than it has stacks
		// then round down
		const U32 stacks_per_seg = (max_verts - verts_per_stack) / verts_per_stack;

		// round up to a whole number of segments
		const U32 strips_segments = (total_stacks+stacks_per_seg-1) / stacks_per_seg;

		LL_INFOS() << "WL Skydome strips in " << strips_segments << " batches." << LL_ENDL;

		mStripsVerts.resize(strips_segments, NULL);

		LLTimer timer;
		timer.start();

		for (U32 i = 0; i < strips_segments ;++i)
		{
			LLVertexBuffer * segment = new LLVertexBuffer(LLDrawPoolWLSky::SKY_VERTEX_DATA_MASK, GL_STATIC_DRAW_ARB);
			mStripsVerts[i] = segment;

			U32 num_stacks_this_seg = stacks_per_seg;
			if ((i == strips_segments - 1) && (total_stacks % stacks_per_seg) != 0)
			{
				// for the last buffer only allocate what we'll use
				num_stacks_this_seg = total_stacks % stacks_per_seg;
			}

			// figure out what range of the sky we're filling
			const U32 begin_stack = i * stacks_per_seg;
			const U32 end_stack = begin_stack + num_stacks_this_seg;
			llassert(end_stack <= total_stacks);

			const U32 num_verts_this_seg = verts_per_stack * (num_stacks_this_seg+1);
			llassert(num_verts_this_seg <= max_verts);

			const U32 num_indices_this_seg = 1+num_stacks_this_seg*(2+2*verts_per_stack);
			llassert(num_indices_this_seg * sizeof(U16) <= max_buffer_bytes);

			if (!segment->allocateBuffer(num_verts_this_seg, num_indices_this_seg, TRUE))
			{
				LL_WARNS() << "Failed to allocate Vertex Buffer on update to "
					<< num_verts_this_seg << " vertices and "
					<< num_indices_this_seg << " indices" << LL_ENDL;
			}

			// lock the buffer
			BOOL success = segment->getVertexStrider(vertices)
				&& segment->getTexCoord0Strider(texCoords)
				&& segment->getIndexStrider(indices);

			if(!success) 
			{
				LL_ERRS() << "Failed updating WindLight sky geometry." << LL_ENDL;
			}

            U32 vertex_count = 0;
            U32 index_count  = 0;

			// fill it
			buildStripsBuffer(begin_stack, end_stack, vertex_count, index_count, vertices, texCoords, indices);

			// and unlock the buffer
			segment->flush();
		}
	
		LL_INFOS() << "completed in " << llformat("%.2f", timer.getElapsedTimeF32().value()) << "seconds" << LL_ENDL;
	}

	updateStarColors();
	updateStarGeometry(drawable);

	LLPipeline::sCompiles++;

	return TRUE;
}

void LLVOWLSky::drawStars(void)
{
	//  render the stars as a sphere centered at viewer camera 
	if (mStarsVerts.notNull())
	{
		mStarsVerts->setBuffer(LLDrawPoolWLSky::STAR_VERTEX_DATA_MASK);
		mStarsVerts->drawArrays(LLRender::TRIANGLES, 0, getStarsNumVerts()*4);
	}
}

void LLVOWLSky::drawFsSky(void)
{
    if (mFsSkyVerts.isNull())
	{
		updateGeometry(mDrawable);
	}

    LLGLDisable disable_blend(GL_BLEND);

	mFsSkyVerts->setBuffer(LLDrawPoolWLSky::ADV_ATMO_SKY_VERTEX_DATA_MASK);
	mFsSkyVerts->drawRange(LLRender::TRIANGLES, 0, mFsSkyVerts->getNumVerts() - 1, mFsSkyVerts->getNumIndices(), 0);
	gPipeline.addTrianglesDrawn(mFsSkyVerts->getNumIndices(), LLRender::TRIANGLES);
	LLVertexBuffer::unbind();
}

void LLVOWLSky::drawDome(void)
{
	if (mStripsVerts.empty())
	{
		updateGeometry(mDrawable);
	}

	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	const U32 data_mask = LLDrawPoolWLSky::SKY_VERTEX_DATA_MASK;
	
	std::vector< LLPointer<LLVertexBuffer> >::const_iterator strips_vbo_iter, end_strips;
	end_strips = mStripsVerts.end();
	for(strips_vbo_iter = mStripsVerts.begin(); strips_vbo_iter != end_strips; ++strips_vbo_iter)
	{
		LLVertexBuffer * strips_segment = strips_vbo_iter->get();

		strips_segment->setBuffer(data_mask);

		strips_segment->drawRange(
			LLRender::TRIANGLE_STRIP, 
			0, strips_segment->getNumVerts()-1, strips_segment->getNumIndices(), 
			0);
		gPipeline.addTrianglesDrawn(strips_segment->getNumIndices(), LLRender::TRIANGLE_STRIP);
	}

	LLVertexBuffer::unbind();
}

void LLVOWLSky::initStars()
{
    const F32 DISTANCE_TO_STARS = LLEnvironment::instance().getCurrentSky()->getDomeRadius();

	// Initialize star map
	mStarVertices.resize(getStarsNumVerts());
	mStarColors.resize(getStarsNumVerts());
	mStarIntensities.resize(getStarsNumVerts());

	std::vector<LLVector3>::iterator v_p = mStarVertices.begin();
	std::vector<LLColor4>::iterator v_c = mStarColors.begin();
	std::vector<F32>::iterator v_i = mStarIntensities.begin();

	U32 i;

	for (i = 0; i < getStarsNumVerts(); ++i)
	{
		v_p->mV[VX] = ll_frand() - 0.5f;
		v_p->mV[VY] = ll_frand() - 0.5f;
		
		// we only want stars on the top half of the dome!

		v_p->mV[VZ] = ll_frand()/2.f;

		v_p->normVec();
		*v_p *= DISTANCE_TO_STARS;
		*v_i = llmin((F32)pow(ll_frand(),2.f) + 0.1f, 1.f);
		v_c->mV[VRED]   = 0.75f + ll_frand() * 0.25f ;
		v_c->mV[VGREEN] = 1.f ;
		v_c->mV[VBLUE]  = 0.75f + ll_frand() * 0.25f ;
		v_c->mV[VALPHA] = 1.f;
		v_c->clamp();
		v_p++;
		v_c++;
		v_i++;
	}
}

void LLVOWLSky::buildStripsBuffer(U32 begin_stack,
                                  U32 end_stack,
                                  U32& vertex_count,
                                  U32& index_count,
								  LLStrider<LLVector3> & vertices,
								  LLStrider<LLVector2> & texCoords,
								  LLStrider<U16> & indices)
{
    const F32 RADIUS = LLEnvironment::instance().getCurrentSky()->getDomeRadius();

	U32 i, j, num_slices, num_stacks;
	F32 phi0, theta, x0, y0, z0;

	// paranoia checking for SL-55986/SL-55833
	U32 count_verts = 0;
	U32 count_indices = 0;

	num_slices = getNumSlices();
	num_stacks = getNumStacks();

	llassert(end_stack <= num_stacks);

	// stacks are iterated one-indexed since phi(0) was handled by the fan above
#if NEW_TESS
	for(i = begin_stack; i <= end_stack; ++i) 
#else
    for(i = begin_stack + 1; i <= end_stack+1; ++i) 
#endif
	{
		phi0 = calcPhi(i);

		for(j = 0; j < num_slices; ++j)
		{
			theta = F_TWO_PI * (float(j) / float(num_slices));

			// standard transformation from  spherical to
			// rectangular coordinates
			x0 = sin(phi0) * cos(theta);
			y0 = cos(phi0);
			z0 = sin(phi0) * sin(theta);

#if NEW_TESS
            *vertices++ = LLVector3(x0 * RADIUS, y0 * RADIUS, z0 * RADIUS);
#else
            if (i == num_stacks-2)
			{
				*vertices++ = LLVector3(x0*RADIUS, y0*RADIUS-1024.f*2.f, z0*RADIUS);
			}
			else if (i == num_stacks-1)
			{
				*vertices++ = LLVector3(0, y0*RADIUS-1024.f*2.f, 0);
			}
			else
			{
				*vertices++		= LLVector3(x0 * RADIUS, y0 * RADIUS, z0 * RADIUS);
			}
#endif

			++count_verts;

			// generate planar uv coordinates
			// note: x and z are transposed in order for things to animate
			// correctly in the global coordinate system where +x is east and
			// +y is north
			*texCoords++	= LLVector2((-z0 + 1.f) / 2.f, (-x0 + 1.f) / 2.f);
		}
	}

	//build triangle strip...
	*indices++ = 0 ;
	count_indices++ ;
	S32 k = 0 ;
	for(i = 1; i <= end_stack - begin_stack; ++i) 
	{
		*indices++ = i * num_slices + k ;
		count_indices++ ;

		k = (k+1) % num_slices ;
		for(j = 0; j < num_slices ; ++j) 
		{
			*indices++ = (i-1) * num_slices + k ;
			*indices++ = i * num_slices + k ;

			count_indices += 2 ;

			k = (k+1) % num_slices ;
		}

		if((--k) < 0)
		{
			k = num_slices - 1 ;
		}

		*indices++ = i * num_slices + k ;
		count_indices++ ;
	}

    vertex_count = count_verts;
    index_count = count_indices;
}

void LLVOWLSky::updateStarColors()
{
	std::vector<LLColor4>::iterator v_c = mStarColors.begin();
	std::vector<F32>::iterator v_i = mStarIntensities.begin();
	std::vector<LLVector3>::iterator v_p = mStarVertices.begin();

	const F32 var = 0.15f;
	const F32 min = 0.5f; //0.75f;
	//const F32 sunclose_max = 0.6f;
	//const F32 sunclose_range = 1 - sunclose_max;

	//F32 below_horizon = - llmin(0.0f, gSky.mVOSkyp->getToSunLast().mV[2]);
	//F32 brightness_factor = llmin(1.0f, below_horizon * 20);

	static S32 swap = 0;
	swap++;

	if ((swap % 2) == 1)
	{
		F32 intensity;						//  max intensity of each star
		U32 x;
		for (x = 0; x < getStarsNumVerts(); ++x)
		{
			//F32 sundir_factor = 1;
			LLVector3 tostar = *v_p;
			tostar.normVec();
			//const F32 how_close_to_sun = tostar * gSky.mVOSkyp->getToSunLast();
			//if (how_close_to_sun > sunclose_max)
			//{
			//	sundir_factor = (1 - how_close_to_sun) / sunclose_range;
			//}
			intensity = *(v_i);
			F32 alpha = v_c->mV[VALPHA] + (ll_frand() - 0.5f) * var * intensity;
			if (alpha < min * intensity)
			{
				alpha = min * intensity;
			}
			if (alpha > intensity)
			{
				alpha = intensity;
			}
			//alpha *= brightness_factor * sundir_factor;

			alpha = llclamp(alpha, 0.f, 1.f);
			v_c->mV[VALPHA] = alpha;
			v_c++;
			v_i++;
			v_p++;
		}
	}
}

BOOL LLVOWLSky::updateStarGeometry(LLDrawable *drawable)
{
	LLStrider<LLVector3> verticesp;
	LLStrider<LLColor4U> colorsp;
	LLStrider<LLVector2> texcoordsp;

	if (mStarsVerts.isNull() || !mStarsVerts->isWriteable())
	{
		mStarsVerts = new LLVertexBuffer(LLDrawPoolWLSky::STAR_VERTEX_DATA_MASK, GL_DYNAMIC_DRAW);
		if (!mStarsVerts->allocateBuffer(getStarsNumVerts()*6, 0, TRUE))
		{
			LL_WARNS() << "Failed to allocate Vertex Buffer for Sky to " << getStarsNumVerts() * 6 << " vertices" << LL_ENDL;
		}
	}
 
	BOOL success = mStarsVerts->getVertexStrider(verticesp)
		&& mStarsVerts->getColorStrider(colorsp)
		&& mStarsVerts->getTexCoord0Strider(texcoordsp);

	if(!success)
	{
		LL_ERRS() << "Failed updating star geometry." << LL_ENDL;
	}

	// *TODO: fix LLStrider with a real prefix increment operator so it can be
	// used as a model of OutputIterator. -Brad
	// std::copy(mStarVertices.begin(), mStarVertices.end(), verticesp);

	if (mStarVertices.size() < getStarsNumVerts())
	{
		LL_ERRS() << "Star reference geometry insufficient." << LL_ENDL;
	}

	for (U32 vtx = 0; vtx < getStarsNumVerts(); ++vtx)
	{
		LLVector3 at = mStarVertices[vtx];
		at.normVec();
		LLVector3 left = at%LLVector3(0,0,1);
		LLVector3 up = at%left;

		F32 sc = 16.0f + (ll_frand() * 20.0f);
		left *= sc;
		up *= sc;

		*(verticesp++)  = mStarVertices[vtx];
		*(verticesp++) = mStarVertices[vtx]+up;
		*(verticesp++) = mStarVertices[vtx]+left+up;
		*(verticesp++)  = mStarVertices[vtx];
		*(verticesp++) = mStarVertices[vtx]+left+up;
		*(verticesp++) = mStarVertices[vtx]+left;

		*(texcoordsp++) = LLVector2(1,0);
		*(texcoordsp++) = LLVector2(1,1);
		*(texcoordsp++) = LLVector2(0,1);
		*(texcoordsp++) = LLVector2(1,0);
		*(texcoordsp++) = LLVector2(0,1);
		*(texcoordsp++) = LLVector2(0,0);

		*(colorsp++)    = LLColor4U(mStarColors[vtx]);
		*(colorsp++)    = LLColor4U(mStarColors[vtx]);
		*(colorsp++)    = LLColor4U(mStarColors[vtx]);
		*(colorsp++)    = LLColor4U(mStarColors[vtx]);
		*(colorsp++)    = LLColor4U(mStarColors[vtx]);
		*(colorsp++)    = LLColor4U(mStarColors[vtx]);
	}

	mStarsVerts->flush();
	return TRUE;
}
