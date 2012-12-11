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
#include "llwlparammanager.h"
#include "llviewercontrol.h"

#define DOME_SLICES 1
const F32 LLVOWLSky::DISTANCE_TO_STARS = (HORIZON_DIST - 10.f)*0.25f;

const U32 LLVOWLSky::MIN_SKY_DETAIL = 3;
const U32 LLVOWLSky::MAX_SKY_DETAIL = 180;

inline U32 LLVOWLSky::getNumStacks(void)
{
	return llmin(MAX_SKY_DETAIL, llmax(MIN_SKY_DETAIL, gSavedSettings.getU32("WLSkyDetail")));
}

inline U32 LLVOWLSky::getNumSlices(void)
{
	return 2 * llmin(MAX_SKY_DETAIL, llmax(MIN_SKY_DETAIL, gSavedSettings.getU32("WLSkyDetail")));
}

inline U32 LLVOWLSky::getFanNumVerts(void)
{
	return getNumSlices() + 1;
}

inline U32 LLVOWLSky::getFanNumIndices(void)
{
	return getNumSlices() * 3;
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

void LLVOWLSky::initSunDirection(LLVector3 const & sun_direction,
		LLVector3 const & sun_angular_velocity)
{
}

void LLVOWLSky::idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time)
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
	t = t*t*t*t;
	
	// invert and square the parameter of the tesselation to bias things toward 1 (the horizon)
	t = 1.f - t;
	t = t*t;
	t = 1.f - t;

	return (F_PI / 8.f) * t;
}

#if !DOME_SLICES
static const F32 Q = (1.f + sqrtf(5.f))/2.f; //golden ratio

//icosahedron verts (based on asset b0c7b76e-28c6-1f87-a1de-752d5e3cd264, contact Runitai Linden for a copy)
static const LLVector3 icosahedron_vert[] =
{
	LLVector3(0,1.f,Q),
	LLVector3(0,-1.f,Q),
	LLVector3(0,-1.f,-Q),
	LLVector3(0,1.f,-Q),

	LLVector3(Q,0,1.f),
	LLVector3(-Q,0,1.f),
	LLVector3(-Q,0,-1.f),
	LLVector3(Q,0,-1.f),

	LLVector3(1,-Q,0.f),
	LLVector3(-1,-Q,0.f),
	LLVector3(-1,Q,0.f),
	LLVector3(1,Q,0.f),
};

//indices
static const U32 icosahedron_ind[] = 
{
	5,0,1,
	10,0,5,
	5,1,9,
	10,5,6,
	6,5,9,
	11,0,10,
	3,11,10,
	3,10,6,
	3,6,2,
	7,3,2,
	8,7,2,
	4,7,8,
	1,4,8,
	9,8,2,
	9,2,6,
	11,3,7,
	4,0,11,
	4,11,7,
	1,0,4,
	1,8,9,
};


//split every triangle in LLVertexBuffer into even fourths (assumes index triangle lists)
void subdivide(LLVertexBuffer& in, LLVertexBuffer* ret)
{
	S32 tri_in = in.getNumIndices()/3;

	ret->allocateBuffer(tri_in*4*3, tri_in*4*3, TRUE);

	LLStrider<LLVector3> vin, vout;
	LLStrider<U16> indin, indout;

	ret->getVertexStrider(vout);
	in.getVertexStrider(vin);

	ret->getIndexStrider(indout);
	in.getIndexStrider(indin);
	
	
	for (S32 i = 0; i < tri_in; i++)
	{
		LLVector3 v0 = vin[*indin++];
		LLVector3 v1 = vin[*indin++];
		LLVector3 v2 = vin[*indin++];

		LLVector3 v3 = (v0 + v1) * 0.5f;
		LLVector3 v4 = (v1 + v2) * 0.5f;
		LLVector3 v5 = (v2 + v0) * 0.5f;

		*vout++ = v0;
		*vout++ = v3;
		*vout++ = v5;

		*vout++ = v3;
		*vout++ = v4;
		*vout++ = v5;

		*vout++ = v3;
		*vout++ = v1;
		*vout++ = v4;

		*vout++ = v5;
		*vout++ = v4;
		*vout++ = v2;
	}
	
	for (S32 i = 0; i < ret->getNumIndices(); i++)
	{
		*indout++ = i;
	}

}

void chop(LLVertexBuffer& in, LLVertexBuffer* out)
{
	//chop off all triangles below horizon 
	F32 d = LLWLParamManager::sParamMgr->getDomeOffset() * LLWLParamManager::sParamMgr->getDomeRadius();
	
	std::vector<LLVector3> vert;
	
	LLStrider<LLVector3> vin;
	LLStrider<U16> index;

	in.getVertexStrider(vin);
	in.getIndexStrider(index);

	U32 tri_count = in.getNumIndices()/3;
	for (U32 i = 0; i < tri_count; i++)
	{
		LLVector3 &v1 = vin[index[i*3+0]];
		LLVector3 &v2 = vin[index[i*3+1]];
		LLVector3 &v3 = vin[index[i*3+2]];

		if (v1.mV[1] > d ||
			v2.mV[1] > d ||
			v3.mV[1] > d)
		{
			v1.mV[1] = llmax(v1.mV[1], d);
			v2.mV[1] = llmax(v1.mV[1], d);
			v3.mV[1] = llmax(v1.mV[1], d);

			vert.push_back(v1);
			vert.push_back(v2);
			vert.push_back(v3);
		}
	}

	out->allocateBuffer(vert.size(), vert.size(), TRUE);

	LLStrider<LLVector3> vout;
	out->getVertexStrider(vout);
	out->getIndexStrider(index);

	for (U32 i = 0; i < vert.size(); i++)
	{
		*vout++ = vert[i];
		*index++ = i;
	}	
}
#endif // !DOME_SLICES

void LLVOWLSky::resetVertexBuffers()
{
	mFanVerts = NULL;
	mStripsVerts.clear();
	mStarsVerts = NULL;

	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
}
	
void LLVOWLSky::cleanupGL()
{
	mFanVerts = NULL;
	mStripsVerts.clear();
	mStarsVerts = NULL;

	LLDrawPoolWLSky::cleanupGL();
}

void LLVOWLSky::restoreGL()
{
	LLDrawPoolWLSky::restoreGL();
	gPipeline.markRebuild(mDrawable, LLDrawable::REBUILD_ALL, TRUE);
}

static LLFastTimer::DeclareTimer FTM_GEO_SKY("Sky Geometry");

BOOL LLVOWLSky::updateGeometry(LLDrawable * drawable)
{
	LLFastTimer ftm(FTM_GEO_SKY);
	LLStrider<LLVector3>	vertices;
	LLStrider<LLVector2>	texCoords;
	LLStrider<U16>			indices;

#if DOME_SLICES
	{
		mFanVerts = new LLVertexBuffer(LLDrawPoolWLSky::SKY_VERTEX_DATA_MASK, GL_STATIC_DRAW_ARB);
		mFanVerts->allocateBuffer(getFanNumVerts(), getFanNumIndices(), TRUE);

		BOOL success = mFanVerts->getVertexStrider(vertices)
			&& mFanVerts->getTexCoord0Strider(texCoords)
			&& mFanVerts->getIndexStrider(indices);

		if(!success) 
		{
			llerrs << "Failed updating WindLight sky geometry." << llendl;
		}

		buildFanBuffer(vertices, texCoords, indices);

		mFanVerts->flush();
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

		llinfos << "WL Skydome strips in " << strips_segments << " batches." << llendl;

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

			segment->allocateBuffer(num_verts_this_seg, num_indices_this_seg, TRUE);

			// lock the buffer
			BOOL success = segment->getVertexStrider(vertices)
				&& segment->getTexCoord0Strider(texCoords)
				&& segment->getIndexStrider(indices);

			if(!success) 
			{
				llerrs << "Failed updating WindLight sky geometry." << llendl;
			}

			// fill it
			buildStripsBuffer(begin_stack, end_stack,  vertices, texCoords, indices);

			// and unlock the buffer
			segment->flush();
		}
	
		llinfos << "completed in " << llformat("%.2f", timer.getElapsedTimeF32()) << "seconds" << llendl;
	}
#else
	mStripsVerts = new LLVertexBuffer(LLDrawPoolWLSky::SKY_VERTEX_DATA_MASK, GL_STATIC_DRAW_ARB);
	
	const F32 RADIUS = LLWLParamManager::sParamMgr->getDomeRadius();

	LLPointer<LLVertexBuffer> temp = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX, 0);
	temp->allocateBuffer(12, 60, TRUE);

	BOOL success = temp->getVertexStrider(vertices)
		&& temp->getIndexStrider(indices);

	if (success)
	{
		for (U32 i = 0; i < 12; i++)
		{
			*vertices++ = icosahedron_vert[i];
		}

		for (U32 i = 0; i < 60; i++)
		{
			*indices++ = icosahedron_ind[i];
		}
	}


	LLPointer<LLVertexBuffer> temp2;
	
	for (U32 i = 0; i < 8; i++)
	{
		temp2 = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX, 0);
		subdivide(*temp, temp2);
		temp = temp2;
	}
	
	temp->getVertexStrider(vertices);
	for (S32 i = 0; i < temp->getNumVerts(); i++)
	{
		LLVector3 v = vertices[i];
		v.normVec();
		vertices[i] = v*RADIUS;
	}

	temp2 = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX, 0);
	chop(*temp, temp2);

	mStripsVerts->allocateBuffer(temp2->getNumVerts(), temp2->getNumIndices(), TRUE);
	
	success = mStripsVerts->getVertexStrider(vertices)
		&& mStripsVerts->getTexCoordStrider(texCoords)
		&& mStripsVerts->getIndexStrider(indices);

	LLStrider<LLVector3> v;
	temp2->getVertexStrider(v);
	LLStrider<U16> ind;
	temp2->getIndexStrider(ind);

	if (success)
	{
		for (S32 i = 0; i < temp2->getNumVerts(); ++i)
		{
			LLVector3 vert = *v++;
			vert.normVec();
			F32 z0 = vert.mV[2];
			F32 x0 = vert.mV[0];
			
			vert *= RADIUS;
			
			*vertices++ = vert;
			*texCoords++ = LLVector2((-z0 + 1.f) / 2.f, (-x0 + 1.f) / 2.f);
		}

		for (S32 i = 0; i < temp2->getNumIndices(); ++i)
		{
			*indices++ = *ind++;
		}
	}

	mStripsVerts->flush();
#endif

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

void LLVOWLSky::drawDome(void)
{
	if (mStripsVerts.empty())
	{
		updateGeometry(mDrawable);
	}

	LLGLDepthTest gls_depth(GL_TRUE, GL_FALSE);

	const U32 data_mask = LLDrawPoolWLSky::SKY_VERTEX_DATA_MASK;
	
#if DOME_SLICES
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

#else
	mStripsVerts->setBuffer(data_mask);
	gGL.syncMatrices();
	glDrawRangeElements(
		GL_TRIANGLES,
		0, mStripsVerts->getNumVerts()-1, mStripsVerts->getNumIndices(),
		GL_UNSIGNED_SHORT,
		mStripsVerts->getIndicesPointer());
#endif

	LLVertexBuffer::unbind();
}

void LLVOWLSky::initStars()
{
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

void LLVOWLSky::buildFanBuffer(LLStrider<LLVector3> & vertices,
							   LLStrider<LLVector2> & texCoords,
							   LLStrider<U16> & indices)
{
	const F32 RADIUS = LLWLParamManager::getInstance()->getDomeRadius();

	U32 i, num_slices;
	F32 phi0, theta, x0, y0, z0;

	// paranoia checking for SL-55986/SL-55833
	U32 count_verts = 0;
	U32 count_indices = 0;

	// apex
	*vertices++		= LLVector3(0.f, RADIUS, 0.f);
	*texCoords++	= LLVector2(0.5f, 0.5f);
	++count_verts;

	num_slices = getNumSlices();

	// and fan in a circle around the apex
	phi0 = calcPhi(1);
	for(i = 0; i < num_slices; ++i) {
		theta = 2.f * F_PI * float(i) / float(num_slices);

		// standard transformation from  spherical to
		// rectangular coordinates
		x0 = sin(phi0) * cos(theta);
		y0 = cos(phi0);
		z0 = sin(phi0) * sin(theta);

		*vertices++		= LLVector3(x0 * RADIUS, y0 * RADIUS, z0 * RADIUS);
		// generate planar uv coordinates
		// note: x and z are transposed in order for things to animate
		// correctly in the global coordinate system where +x is east and
		// +y is north
		*texCoords++	= LLVector2((-z0 + 1.f) / 2.f, (-x0 + 1.f) / 2.f);
		++count_verts;

		if (i > 0)
		{
			*indices++ = 0;
			*indices++ = i;
			*indices++ = i+1;
			count_indices += 3;
		}
	}

	// the last vertex of the last triangle should wrap around to 
	// the beginning
	*indices++ = 0;
	*indices++ = num_slices;
	*indices++ = 1;
	count_indices += 3;

	// paranoia checking for SL-55986/SL-55833
	llassert(getFanNumVerts() == count_verts);
	llassert(getFanNumIndices() == count_indices);
}

void LLVOWLSky::buildStripsBuffer(U32 begin_stack, U32 end_stack,
								  LLStrider<LLVector3> & vertices,
								  LLStrider<LLVector2> & texCoords,
								  LLStrider<U16> & indices)
{
	const F32 RADIUS = LLWLParamManager::getInstance()->getDomeRadius();

	U32 i, j, num_slices, num_stacks;
	F32 phi0, theta, x0, y0, z0;

	// paranoia checking for SL-55986/SL-55833
	U32 count_verts = 0;
	U32 count_indices = 0;

	num_slices = getNumSlices();
	num_stacks = getNumStacks();

	llassert(end_stack <= num_stacks);

	// stacks are iterated one-indexed since phi(0) was handled by the fan above
	for(i = begin_stack + 1; i <= end_stack+1; ++i) 
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
		mStarsVerts->allocateBuffer(getStarsNumVerts()*6, 0, TRUE);
	}
 
	BOOL success = mStarsVerts->getVertexStrider(verticesp)
		&& mStarsVerts->getColorStrider(colorsp)
		&& mStarsVerts->getTexCoord0Strider(texcoordsp);

	if(!success)
	{
		llerrs << "Failed updating star geometry." << llendl;
	}

	// *TODO: fix LLStrider with a real prefix increment operator so it can be
	// used as a model of OutputIterator. -Brad
	// std::copy(mStarVertices.begin(), mStarVertices.end(), verticesp);

	if (mStarVertices.size() < getStarsNumVerts())
	{
		llerrs << "Star reference geometry insufficient." << llendl;
	}

	for (U32 vtx = 0; vtx < getStarsNumVerts(); ++vtx)
	{
		LLVector3 at = mStarVertices[vtx];
		at.normVec();
		LLVector3 left = at%LLVector3(0,0,1);
		LLVector3 up = at%left;

		F32 sc = 0.5f+ll_frand()*1.25f;
		left *= sc;
		up *= sc;

		*(verticesp++)  = mStarVertices[vtx];
		*(verticesp++) = mStarVertices[vtx]+left;
		*(verticesp++) = mStarVertices[vtx]+left+up;
		*(verticesp++) = mStarVertices[vtx]+left;
		*(verticesp++) = mStarVertices[vtx]+left+up;
		*(verticesp++) = mStarVertices[vtx]+up;

		*(texcoordsp++) = LLVector2(0,0);
		*(texcoordsp++) = LLVector2(0,1);
		*(texcoordsp++) = LLVector2(1,1);
		*(texcoordsp++) = LLVector2(0,1);
		*(texcoordsp++) = LLVector2(1,1);
		*(texcoordsp++) = LLVector2(1,0);

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
