/** 
 * @file llvertexbuffer.cpp
 * @brief LLVertexBuffer implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llfasttimer.h"
#include "llsys.h"
#include "llvertexbuffer.h"
// #include "llrender.h"
#include "llglheaders.h"
#include "llrender.h"
#include "llvector4a.h"
#include "llshadermgr.h"
#include "llglslshader.h"
#include "llmemory.h"

//Next Highest Power Of Two
//helper function, returns first number > v that is a power of 2, or v if v is already a power of 2
U32 nhpo2(U32 v)
{
	U32 r = 1;
	while (r < v) {
		r *= 2;
	}
	return r;
}

//which power of 2 is i?
//assumes i is a power of 2 > 0
U32 wpo2(U32 i)
{
	llassert(i > 0);
	llassert(nhpo2(i) == i);

	U32 r = 0;

	while (i >>= 1) ++r;

	return r;
}


const U32 LL_VBO_BLOCK_SIZE = 2048;
const U32 LL_VBO_POOL_MAX_SEED_SIZE = 256*1024;

U32 vbo_block_size(U32 size)
{ //what block size will fit size?
	U32 mod = size % LL_VBO_BLOCK_SIZE;
	return mod == 0 ? size : size + (LL_VBO_BLOCK_SIZE-mod);
}

U32 vbo_block_index(U32 size)
{
	return vbo_block_size(size)/LL_VBO_BLOCK_SIZE;
}

const U32 LL_VBO_POOL_SEED_COUNT = vbo_block_index(LL_VBO_POOL_MAX_SEED_SIZE);


//============================================================================

//static
LLVBOPool LLVertexBuffer::sStreamVBOPool(GL_STREAM_DRAW_ARB, GL_ARRAY_BUFFER_ARB);
LLVBOPool LLVertexBuffer::sDynamicVBOPool(GL_DYNAMIC_DRAW_ARB, GL_ARRAY_BUFFER_ARB);
LLVBOPool LLVertexBuffer::sDynamicCopyVBOPool(GL_DYNAMIC_COPY_ARB, GL_ARRAY_BUFFER_ARB);
LLVBOPool LLVertexBuffer::sStreamIBOPool(GL_STREAM_DRAW_ARB, GL_ELEMENT_ARRAY_BUFFER_ARB);
LLVBOPool LLVertexBuffer::sDynamicIBOPool(GL_DYNAMIC_DRAW_ARB, GL_ELEMENT_ARRAY_BUFFER_ARB);

U32 LLVBOPool::sBytesPooled = 0;
U32 LLVBOPool::sIndexBytesPooled = 0;
U32 LLVBOPool::sNameIdx = 0;
U32 LLVBOPool::sNamePool[1024];

std::list<U32> LLVertexBuffer::sAvailableVAOName;
U32 LLVertexBuffer::sCurVAOName = 1;

U32 LLVertexBuffer::sAllocatedIndexBytes = 0;
U32 LLVertexBuffer::sIndexCount = 0;

U32 LLVertexBuffer::sBindCount = 0;
U32 LLVertexBuffer::sSetCount = 0;
S32 LLVertexBuffer::sCount = 0;
S32 LLVertexBuffer::sGLCount = 0;
S32 LLVertexBuffer::sMappedCount = 0;
bool LLVertexBuffer::sDisableVBOMapping = false;
bool LLVertexBuffer::sEnableVBOs = true;
U32 LLVertexBuffer::sGLRenderBuffer = 0;
U32 LLVertexBuffer::sGLRenderArray = 0;
U32 LLVertexBuffer::sGLRenderIndices = 0;
U32 LLVertexBuffer::sLastMask = 0;
bool LLVertexBuffer::sVBOActive = false;
bool LLVertexBuffer::sIBOActive = false;
U32 LLVertexBuffer::sAllocatedBytes = 0;
U32 LLVertexBuffer::sVertexCount = 0;
bool LLVertexBuffer::sMapped = false;
bool LLVertexBuffer::sUseStreamDraw = true;
bool LLVertexBuffer::sUseVAO = false;
bool LLVertexBuffer::sPreferStreamDraw = false;


U32 LLVBOPool::genBuffer()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX

	if (sNameIdx == 0)
	{
		glGenBuffersARB(1024, sNamePool);
		sNameIdx = 1024;
	}

	return sNamePool[--sNameIdx];
}

void LLVBOPool::deleteBuffer(U32 name)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX
	if (gGLManager.mInited)
	{
		LLVertexBuffer::unbind();

		glBindBufferARB(mType, name);
		glBufferDataARB(mType, 0, NULL, mUsage);
		glBindBufferARB(mType, 0);

		glDeleteBuffersARB(1, &name);
	}
}


LLVBOPool::LLVBOPool(U32 vboUsage, U32 vboType)
: mUsage(vboUsage), mType(vboType)
{
	mMissCount.resize(LL_VBO_POOL_SEED_COUNT);
	std::fill(mMissCount.begin(), mMissCount.end(), 0);
}

U8* LLVBOPool::allocate(U32& name, U32 size, bool for_seed)
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX
	llassert(vbo_block_size(size) == size);
	
	U8* ret = NULL;

	U32 i = vbo_block_index(size);

	if (mFreeList.size() <= i)
	{
		mFreeList.resize(i+1);
	}

	if (mFreeList[i].empty() || for_seed)
	{
		//make a new buffer
		name = genBuffer();
		
		glBindBufferARB(mType, name);

		if (!for_seed && i < LL_VBO_POOL_SEED_COUNT)
		{ //record this miss
			mMissCount[i]++;	
		}

		if (mType == GL_ARRAY_BUFFER_ARB)
		{
			LLVertexBuffer::sAllocatedBytes += size;
		}
		else
		{
			LLVertexBuffer::sAllocatedIndexBytes += size;
		}

		if (LLVertexBuffer::sDisableVBOMapping || mUsage != GL_DYNAMIC_DRAW_ARB)
		{
			glBufferDataARB(mType, size, 0, mUsage);
			if (mUsage != GL_DYNAMIC_COPY_ARB)
			{ //data will be provided by application
				ret = (U8*) ll_aligned_malloc<64>(size);
				if (!ret)
				{
					LL_ERRS() << "Failed to allocate "<< size << " bytes for LLVBOPool buffer " << name <<"." << LL_NEWLINE
							  << "Free list size: " << mFreeList.size() // this happens if we are out of memory so a solution might be to clear some from freelist
							  << " Allocated Bytes: " << LLVertexBuffer::sAllocatedBytes
							  << " Allocated Index Bytes: " << LLVertexBuffer::sAllocatedIndexBytes
							  << " Pooled Bytes: " << sBytesPooled
							  << " Pooled Index Bytes: " << sIndexBytesPooled
							  << LL_ENDL;
				}
			}
		}
		else
		{ //always use a true hint of static draw when allocating non-client-backed buffers
			glBufferDataARB(mType, size, 0, GL_STATIC_DRAW_ARB);
		}

		glBindBufferARB(mType, 0);

		if (for_seed)
		{ //put into pool for future use
			llassert(mFreeList.size() > i);

			Record rec;
			rec.mGLName = name;
			rec.mClientData = ret;
	
			if (mType == GL_ARRAY_BUFFER_ARB)
			{
				sBytesPooled += size;
			}
			else
			{
				sIndexBytesPooled += size;
			}
			mFreeList[i].push_back(rec);
		}
	}
	else
	{
		name = mFreeList[i].front().mGLName;
		ret = mFreeList[i].front().mClientData;

		if (mType == GL_ARRAY_BUFFER_ARB)
		{
			sBytesPooled -= size;
		}
		else
		{
			sIndexBytesPooled -= size;
		}

		mFreeList[i].pop_front();
	}

	return ret;
}

void LLVBOPool::release(U32 name, U8* buffer, U32 size)
{
	llassert(vbo_block_size(size) == size);

	deleteBuffer(name);
	ll_aligned_free_fallback((U8*) buffer);

	if (mType == GL_ARRAY_BUFFER_ARB)
	{
		LLVertexBuffer::sAllocatedBytes -= size;
	}
	else
	{
		LLVertexBuffer::sAllocatedIndexBytes -= size;
	}
}

void LLVBOPool::seedPool()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX
	U32 dummy_name = 0;

	if (mFreeList.size() < LL_VBO_POOL_SEED_COUNT)
	{
		LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("VBOPool Resize");
		mFreeList.resize(LL_VBO_POOL_SEED_COUNT);
	}

	for (U32 i = 0; i < LL_VBO_POOL_SEED_COUNT; i++)
	{
		if (mMissCount[i] > mFreeList[i].size())
		{ 
			U32 size = i*LL_VBO_BLOCK_SIZE;
		
			S32 count = mMissCount[i] - mFreeList[i].size();
			for (U32 j = 0; j < count; ++j)
			{
				allocate(dummy_name, size, true);
			}
		}
	}
}



void LLVBOPool::cleanup()
{
	U32 size = LL_VBO_BLOCK_SIZE;

	for (U32 i = 0; i < mFreeList.size(); ++i)
	{
		record_list_t& l = mFreeList[i];

		while (!l.empty())
		{
			Record& r = l.front();

			deleteBuffer(r.mGLName);
			
			if (r.mClientData)
			{
				ll_aligned_free<64>((void*) r.mClientData);
			}

			l.pop_front();

			if (mType == GL_ARRAY_BUFFER_ARB)
			{
				sBytesPooled -= size;
				LLVertexBuffer::sAllocatedBytes -= size;
			}
			else
			{
				sIndexBytesPooled -= size;
				LLVertexBuffer::sAllocatedIndexBytes -= size;
			}
		}

		size += LL_VBO_BLOCK_SIZE;
	}

	//reset miss counts
	std::fill(mMissCount.begin(), mMissCount.end(), 0);
}


//NOTE: each component must be AT LEAST 4 bytes in size to avoid a performance penalty on AMD hardware
const S32 LLVertexBuffer::sTypeSize[LLVertexBuffer::TYPE_MAX] =
{
	sizeof(LLVector4), // TYPE_VERTEX,
	sizeof(LLVector4), // TYPE_NORMAL,
	sizeof(LLVector2), // TYPE_TEXCOORD0,
	sizeof(LLVector2), // TYPE_TEXCOORD1,
	sizeof(LLVector2), // TYPE_TEXCOORD2,
	sizeof(LLVector2), // TYPE_TEXCOORD3,
	sizeof(LLColor4U), // TYPE_COLOR,
	sizeof(LLColor4U), // TYPE_EMISSIVE, only alpha is used currently
	sizeof(LLVector4), // TYPE_TANGENT,
	sizeof(F32),	   // TYPE_WEIGHT,
	sizeof(LLVector4), // TYPE_WEIGHT4,
	sizeof(LLVector4), // TYPE_CLOTHWEIGHT,
	sizeof(LLVector4), // TYPE_TEXTURE_INDEX (actually exists as position.w), no extra data, but stride is 16 bytes
};

static const std::string vb_type_name[] =
{
	"TYPE_VERTEX",
	"TYPE_NORMAL",
	"TYPE_TEXCOORD0",
	"TYPE_TEXCOORD1",
	"TYPE_TEXCOORD2",
	"TYPE_TEXCOORD3",
	"TYPE_COLOR",
	"TYPE_EMISSIVE",
	"TYPE_TANGENT",
	"TYPE_WEIGHT",
	"TYPE_WEIGHT4",
	"TYPE_CLOTHWEIGHT",
	"TYPE_TEXTURE_INDEX",
	"TYPE_MAX",
	"TYPE_INDEX",	
};

const U32 LLVertexBuffer::sGLMode[LLRender::NUM_MODES] =
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_QUADS,
	GL_LINE_LOOP,
};

//static
U32 LLVertexBuffer::getVAOName()
{
	U32 ret = 0;

	if (!sAvailableVAOName.empty())
	{
		ret = sAvailableVAOName.front();
		sAvailableVAOName.pop_front();
	}
	else
	{
#ifdef GL_ARB_vertex_array_object
		glGenVertexArrays(1, &ret);
#endif
	}

	return ret;		
}

//static
void LLVertexBuffer::releaseVAOName(U32 name)
{
	sAvailableVAOName.push_back(name);
}


//static
void LLVertexBuffer::seedPools()
{
	LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX
	sStreamVBOPool.seedPool();
	sDynamicVBOPool.seedPool();
	sDynamicCopyVBOPool.seedPool();
	sStreamIBOPool.seedPool();
	sDynamicIBOPool.seedPool();
}

//static
void LLVertexBuffer::setupClientArrays(U32 data_mask)
{
	if (sLastMask != data_mask)
	{

		if (gGLManager.mGLSLVersionMajor < 2 && gGLManager.mGLSLVersionMinor < 30)
		{
			//make sure texture index is disabled
			data_mask = data_mask & ~MAP_TEXTURE_INDEX;
		}

		for (U32 i = 0; i < TYPE_MAX; ++i)
		{
			S32 loc = i;
										
			U32 mask = 1 << i;

			if (sLastMask & (1 << i))
			{ //was enabled
				if (!(data_mask & mask))
				{ //needs to be disabled
					glDisableVertexAttribArrayARB(loc);
				}
			}
			else 
			{	//was disabled
				if (data_mask & mask)
				{ //needs to be enabled
					glEnableVertexAttribArrayARB(loc);
				}
			}
		}
				
		sLastMask = data_mask;
	}
}

//static
void LLVertexBuffer::drawArrays(U32 mode, const std::vector<LLVector3>& pos)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    gGL.begin(mode);
    for (auto& v : pos)
    {
        gGL.vertex3fv(v.mV);
    }
    gGL.end();
    gGL.flush();
}

//static
void LLVertexBuffer::drawElements(U32 mode, const LLVector4a* pos, const LLVector2* tc, S32 num_indices, const U16* indicesp)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);

	gGL.syncMatrices();

	U32 mask = LLVertexBuffer::MAP_VERTEX;
	if (tc)
	{
		mask = mask | LLVertexBuffer::MAP_TEXCOORD0;
	}

	unbind();
	
    gGL.begin(mode);

    if (tc != nullptr)
    {
        for (int i = 0; i < num_indices; ++i)
        {
            U16 idx = indicesp[i];
            gGL.texCoord2fv(tc[idx].mV);
            gGL.vertex3fv(pos[idx].getF32ptr());
        }
    }
    else
    {
        for (int i = 0; i < num_indices; ++i)
        {
            U16 idx = indicesp[i];
            gGL.vertex3fv(pos[idx].getF32ptr());
        }
    }
    gGL.end();
    gGL.flush();
}

void LLVertexBuffer::validateRange(U32 start, U32 end, U32 count, U32 indices_offset) const
{
    llassert(start < (U32)mNumVerts);
    llassert(end < (U32)mNumVerts);

	if (start >= (U32) mNumVerts ||
	    end >= (U32) mNumVerts)
	{
		LL_ERRS() << "Bad vertex buffer draw range: [" << start << ", " << end << "] vs " << mNumVerts << LL_ENDL;
	}

	llassert(mNumIndices >= 0);

	if (indices_offset >= (U32) mNumIndices ||
	    indices_offset + count > (U32) mNumIndices)
	{
		LL_ERRS() << "Bad index buffer draw range: [" << indices_offset << ", " << indices_offset+count << "]" << LL_ENDL;
	}

	if (gDebugGL && !useVBOs())
	{
		U16* idx = ((U16*) getIndicesPointer())+indices_offset;
		for (U32 i = 0; i < count; ++i)
		{
            llassert(idx[i] >= start);
            llassert(idx[i] <= end);

			if (idx[i] < start || idx[i] > end)
			{
				LL_ERRS() << "Index out of range: " << idx[i] << " not in [" << start << ", " << end << "]" << LL_ENDL;
			}
		}

		LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;

		if (shader && shader->mFeatures.mIndexedTextureChannels > 1)
		{
			LLStrider<LLVector4a> v;
			//hack to get non-const reference
			LLVertexBuffer* vb = (LLVertexBuffer*) this;
			vb->getVertexStrider(v);

			for (U32 i = start; i < end; i++)
			{
				S32 idx = (S32) (v[i][3]+0.25f);
                llassert(idx >= 0);
				if (idx < 0 || idx >= shader->mFeatures.mIndexedTextureChannels)
				{
					LL_ERRS() << "Bad texture index found in vertex data stream." << LL_ENDL;
				}
			}
		}
	}
}

void LLVertexBuffer::drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const
{
	validateRange(start, end, count, indices_offset);
	mMappable = false;
	gGL.syncMatrices();

	llassert(mNumVerts >= 0);
	llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);

	if (mGLArray)
	{
		if (mGLArray != sGLRenderArray)
		{
			LL_ERRS() << "Wrong vertex array bound." << LL_ENDL;
		}
	}
	else
	{
		if (mGLIndices != sGLRenderIndices)
		{
			LL_ERRS() << "Wrong index buffer bound." << LL_ENDL;
		}

		if (mGLBuffer != sGLRenderBuffer)
		{
			LL_ERRS() << "Wrong vertex buffer bound." << LL_ENDL;
		}
	}

	if (gDebugGL && !mGLArray && useVBOs())
	{
		GLint elem = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &elem);

		if (elem != mGLIndices)
		{
			LL_ERRS() << "Wrong index buffer bound!" << LL_ENDL;
		}
	}

	if (mode >= LLRender::NUM_MODES)
	{
		LL_ERRS() << "Invalid draw mode: " << mode << LL_ENDL;
		return;
	}

	U16* idx = ((U16*) getIndicesPointer())+indices_offset;

	stop_glerror();
	LLGLSLShader::startProfile();
    LL_PROFILER_GPU_ZONEC( "gl.DrawRangeElements", 0xFFFF00 )
	glDrawRangeElements(sGLMode[mode], start, end, count, GL_UNSIGNED_SHORT, 
		idx);
	LLGLSLShader::stopProfile(count, mode);
	stop_glerror();

	

	placeFence();
}

void LLVertexBuffer::drawRangeFast(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const
{
    mMappable = false;
    gGL.syncMatrices();

    U16* idx = ((U16*)getIndicesPointer()) + indices_offset;

    LL_PROFILER_GPU_ZONEC("gl.DrawRangeElements", 0xFFFF00)
        glDrawRangeElements(sGLMode[mode], start, end, count, GL_UNSIGNED_SHORT,
            idx);
}

void LLVertexBuffer::draw(U32 mode, U32 count, U32 indices_offset) const
{
	llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);
	mMappable = false;
	gGL.syncMatrices();

	llassert(mNumIndices >= 0);
	if (indices_offset >= (U32) mNumIndices ||
	    indices_offset + count > (U32) mNumIndices)
	{
		LL_ERRS() << "Bad index buffer draw range: [" << indices_offset << ", " << indices_offset+count << "]" << LL_ENDL;
	}

	if (mGLArray)
	{
		if (mGLArray != sGLRenderArray)
		{
			LL_ERRS() << "Wrong vertex array bound." << LL_ENDL;
		}
	}
	else
	{
		if (mGLIndices != sGLRenderIndices)
		{
			LL_ERRS() << "Wrong index buffer bound." << LL_ENDL;
		}

		if (mGLBuffer != sGLRenderBuffer)
		{
			LL_ERRS() << "Wrong vertex buffer bound." << LL_ENDL;
		}
	}

	if (mode >= LLRender::NUM_MODES)
	{
		LL_ERRS() << "Invalid draw mode: " << mode << LL_ENDL;
		return;
	}

	stop_glerror();
	LLGLSLShader::startProfile();
    LL_PROFILER_GPU_ZONEC( "gl.DrawElements", 0xA0FFA0 )
	glDrawElements(sGLMode[mode], count, GL_UNSIGNED_SHORT,
		((U16*) getIndicesPointer()) + indices_offset);
	LLGLSLShader::stopProfile(count, mode);
	stop_glerror();
	placeFence();
}


void LLVertexBuffer::drawArrays(U32 mode, U32 first, U32 count) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);
    mMappable = false;
    gGL.syncMatrices();

#ifndef LL_RELEASE_FOR_DOWNLOAD
    llassert(mNumVerts >= 0);
    if (first >= (U32)mNumVerts ||
        first + count > (U32)mNumVerts)
    {
        LL_ERRS() << "Bad vertex buffer draw range: [" << first << ", " << first + count << "]" << LL_ENDL;
    }

    if (mGLArray)
    {
        if (mGLArray != sGLRenderArray)
        {
            LL_ERRS() << "Wrong vertex array bound." << LL_ENDL;
        }
    }
    else
    {
        if (mGLBuffer != sGLRenderBuffer || useVBOs() != sVBOActive)
        {
            LL_ERRS() << "Wrong vertex buffer bound." << LL_ENDL;
        }
    }

    if (mode >= LLRender::NUM_MODES)
    {
        LL_ERRS() << "Invalid draw mode: " << mode << LL_ENDL;
        return;
    }
#endif

    LLGLSLShader::startProfile();
    {
        LL_PROFILER_GPU_ZONEC("gl.DrawArrays", 0xFF4040)
            glDrawArrays(sGLMode[mode], first, count);
    }
    LLGLSLShader::stopProfile(count, mode);

    stop_glerror();
    placeFence();
}

//static
void LLVertexBuffer::initClass(bool use_vbo, bool no_vbo_mapping)
{
	sEnableVBOs = use_vbo && gGLManager.mHasVertexBufferObject;
	sDisableVBOMapping = sEnableVBOs && no_vbo_mapping;
}

//static 
void LLVertexBuffer::unbind()
{
	if (sGLRenderArray)
	{
#if GL_ARB_vertex_array_object
		glBindVertexArray(0);
#endif
		sGLRenderArray = 0;
		sGLRenderIndices = 0;
		sIBOActive = false;
	}

	if (sVBOActive)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		sVBOActive = false;
	}
	if (sIBOActive)
	{
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		sIBOActive = false;
	}

	sGLRenderBuffer = 0;
	sGLRenderIndices = 0;

	setupClientArrays(0);
}

//static
void LLVertexBuffer::cleanupClass()
{
	unbind();
	
	sStreamIBOPool.cleanup();
	sDynamicIBOPool.cleanup();
	sStreamVBOPool.cleanup();
	sDynamicVBOPool.cleanup();
	sDynamicCopyVBOPool.cleanup();
}

//----------------------------------------------------------------------------

S32 LLVertexBuffer::determineUsage(S32 usage)
{
	S32 ret_usage = usage;

	if (!sEnableVBOs)
	{
		ret_usage = 0;
	}
	
	if (ret_usage == GL_STREAM_DRAW_ARB && !sUseStreamDraw)
	{
		ret_usage = 0;
	}
	
	if (ret_usage == GL_DYNAMIC_DRAW_ARB && sPreferStreamDraw)
	{
		ret_usage = GL_STREAM_DRAW_ARB;
	}
	
	if (ret_usage == 0 && LLRender::sGLCoreProfile)
	{ //MUST use VBOs for all rendering
		ret_usage = GL_STREAM_DRAW_ARB;
	}
	
	if (ret_usage && ret_usage != GL_STREAM_DRAW_ARB)
	{ //only stream_draw and dynamic_draw are supported when using VBOs, dynamic draw is the default
		if (ret_usage != GL_DYNAMIC_COPY_ARB)
		{
		    if (sDisableVBOMapping)
		    { //always use stream draw if VBO mapping is disabled
			    ret_usage = GL_STREAM_DRAW_ARB;
		    }
		    else
		    {
			    ret_usage = GL_DYNAMIC_DRAW_ARB;
		    }
	    }
	}
	
	return ret_usage;
}

LLVertexBuffer::LLVertexBuffer(U32 typemask, S32 usage) 
:	LLRefCount(),

	mNumVerts(0),
	mNumIndices(0),
	mAlignedOffset(0),
	mAlignedIndexOffset(0),
	mSize(0),
	mIndicesSize(0),
	mTypeMask(typemask),
	mUsage(LLVertexBuffer::determineUsage(usage)),
	mGLBuffer(0),
	mGLIndices(0),
	mGLArray(0),
	mMappedData(NULL),
	mMappedIndexData(NULL),
	mMappedDataUsingVBOs(false),
	mMappedIndexDataUsingVBOs(false),
	mVertexLocked(false),
	mIndexLocked(false),
	mFinal(false),
	mEmpty(true),
	mMappable(false),
	mFence(NULL)
{
	mMappable = (mUsage == GL_DYNAMIC_DRAW_ARB && !sDisableVBOMapping);

	//zero out offsets
	for (U32 i = 0; i < TYPE_MAX; i++)
	{
		mOffsets[i] = 0;
	}

	sCount++;
}

//static
S32 LLVertexBuffer::calcOffsets(const U32& typemask, S32* offsets, S32 num_vertices)
{
	S32 offset = 0;
	for (S32 i=0; i<TYPE_TEXTURE_INDEX; i++)
	{
		U32 mask = 1<<i;
		if (typemask & mask)
		{
			if (offsets && LLVertexBuffer::sTypeSize[i])
			{
				offsets[i] = offset;
				offset += LLVertexBuffer::sTypeSize[i]*num_vertices;
				offset = (offset + 0xF) & ~0xF;
			}
		}
	}

	offsets[TYPE_TEXTURE_INDEX] = offsets[TYPE_VERTEX] + 12;
	
	return offset+16;
}

//static 
S32 LLVertexBuffer::calcVertexSize(const U32& typemask)
{
	S32 size = 0;
	for (S32 i = 0; i < TYPE_TEXTURE_INDEX; i++)
	{
		U32 mask = 1<<i;
		if (typemask & mask)
		{
			size += LLVertexBuffer::sTypeSize[i];
		}
	}

	return size;
}

S32 LLVertexBuffer::getSize() const
{
	return mSize;
}

// protected, use unref()
//virtual
LLVertexBuffer::~LLVertexBuffer()
{
	destroyGLBuffer();
	destroyGLIndices();

	if (mGLArray)
	{
#if GL_ARB_vertex_array_object
		releaseVAOName(mGLArray);
#endif
	}

	sCount--;

	if (mFence)
	{
		// Sanity check. We have weird crashes in this destructor (on delete). Yet mFence is disabled.
		// TODO: mFence was added in scope of SH-2038, but was never enabled, consider removing mFence.
		LL_ERRS() << "LLVertexBuffer destruction failed" << LL_ENDL;
		delete mFence;
		mFence = NULL;
	}

	sVertexCount -= mNumVerts;
	sIndexCount -= mNumIndices;

	if (mMappedData)
	{
		LL_ERRS() << "Failed to clear vertex buffer's vertices" << LL_ENDL;
	}
	if (mMappedIndexData)
	{
		LL_ERRS() << "Failed to clear vertex buffer's indices" << LL_ENDL;
	}
};

void LLVertexBuffer::placeFence() const
{
	/*if (!mFence && useVBOs())
	{
		if (gGLManager.mHasSync)
		{
			mFence = new LLGLSyncFence();
		}
	}

	if (mFence)
	{
		mFence->placeFence();
	}*/
}

void LLVertexBuffer::waitFence() const
{
	/*if (mFence)
	{
		mFence->wait();
	}*/
}

//----------------------------------------------------------------------------

void LLVertexBuffer::genBuffer(U32 size)
{
	mSize = vbo_block_size(size);

	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		mMappedData = sStreamVBOPool.allocate(mGLBuffer, mSize);
	}
	else if (mUsage == GL_DYNAMIC_DRAW_ARB)
	{
		mMappedData = sDynamicVBOPool.allocate(mGLBuffer, mSize);
	}
	else
	{
		mMappedData = sDynamicCopyVBOPool.allocate(mGLBuffer, mSize);
	}
	
	
	sGLCount++;
}

void LLVertexBuffer::genIndices(U32 size)
{
	mIndicesSize = vbo_block_size(size);

	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		mMappedIndexData = sStreamIBOPool.allocate(mGLIndices, mIndicesSize);
	}
	else
	{
		mMappedIndexData = sDynamicIBOPool.allocate(mGLIndices, mIndicesSize);
	}
	
	sGLCount++;
}

void LLVertexBuffer::releaseBuffer()
{
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		sStreamVBOPool.release(mGLBuffer, mMappedData, mSize);
	}
	else
	{
		sDynamicVBOPool.release(mGLBuffer, mMappedData, mSize);
	}
	
	mGLBuffer = 0;
	mMappedData = NULL;

	sGLCount--;
}

void LLVertexBuffer::releaseIndices()
{
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		sStreamIBOPool.release(mGLIndices, mMappedIndexData, mIndicesSize);
	}
	else
	{
		sDynamicIBOPool.release(mGLIndices, mMappedIndexData, mIndicesSize);
	}

	mGLIndices = 0;
	mMappedIndexData = NULL;
	
	sGLCount--;
}

bool LLVertexBuffer::createGLBuffer(U32 size)
{
	if (mGLBuffer || mMappedData)
	{
		destroyGLBuffer();
	}

	if (size == 0)
	{
		return true;
	}

	bool success = true;

	mEmpty = true;

	mMappedDataUsingVBOs = useVBOs();
	
	if (mMappedDataUsingVBOs)
	{
		genBuffer(size);
	}
	else
	{
		static int gl_buffer_idx = 0;
		mGLBuffer = ++gl_buffer_idx;
		mMappedData = (U8*)ll_aligned_malloc_16(size);
		mSize = size;
	}

	if (!mMappedData)
	{
		success = false;
	}
	return success;
}

bool LLVertexBuffer::createGLIndices(U32 size)
{
	if (mGLIndices)
	{
		destroyGLIndices();
	}
	
	if (size == 0)
	{
		return true;
	}

	bool success = true;

	mEmpty = true;

	//pad by 16 bytes for aligned copies
	size += 16;

	mMappedIndexDataUsingVBOs = useVBOs();

	if (mMappedIndexDataUsingVBOs)
	{
		//pad by another 16 bytes for VBO pointer adjustment
		size += 16;
		genIndices(size);
	}
	else
	{
		mMappedIndexData = (U8*)ll_aligned_malloc_16(size);
		static int gl_buffer_idx = 0;
		mGLIndices = ++gl_buffer_idx;
		mIndicesSize = size;
	}

	if (!mMappedIndexData)
	{
		success = false;
	}
	return success;
}

void LLVertexBuffer::destroyGLBuffer()
{
	if (mGLBuffer || mMappedData)
	{
		if (mMappedDataUsingVBOs)
		{
			releaseBuffer();
		}
		else
		{
			ll_aligned_free_16((void*)mMappedData);
			mMappedData = NULL;
			mEmpty = true;
		}
	}
	
	mGLBuffer = 0;
	//unbind();
}

void LLVertexBuffer::destroyGLIndices()
{
	if (mGLIndices || mMappedIndexData)
	{
		if (mMappedIndexDataUsingVBOs)
		{
			releaseIndices();
		}
		else
		{
			ll_aligned_free_16((void*)mMappedIndexData);
			mMappedIndexData = NULL;
			mEmpty = true;
		}
	}

	mGLIndices = 0;
	//unbind();
}

bool LLVertexBuffer::updateNumVerts(S32 nverts)
{
	llassert(nverts >= 0);

	bool success = true;

	if (nverts > 65536)
	{
		LL_WARNS() << "Vertex buffer overflow!" << LL_ENDL;
		nverts = 65536;
	}

	U32 needed_size = calcOffsets(mTypeMask, mOffsets, nverts);

	if (needed_size > mSize || needed_size <= mSize/2)
	{
		success &= createGLBuffer(needed_size);
	}

	sVertexCount -= mNumVerts;
	mNumVerts = nverts;
	sVertexCount += mNumVerts;

	return success;
}

bool LLVertexBuffer::updateNumIndices(S32 nindices)
{
	llassert(nindices >= 0);

	bool success = true;

	U32 needed_size = sizeof(U16) * nindices;

	if (needed_size > mIndicesSize || needed_size <= mIndicesSize/2)
	{
		success &= createGLIndices(needed_size);
	}

	sIndexCount -= mNumIndices;
	mNumIndices = nindices;
	sIndexCount += mNumIndices;

	return success;
}

bool LLVertexBuffer::allocateBuffer(S32 nverts, S32 nindices, bool create)
{
	stop_glerror();

	if (nverts < 0 || nindices < 0 ||
		nverts > 65536)
	{
		LL_ERRS() << "Bad vertex buffer allocation: " << nverts << " : " << nindices << LL_ENDL;
	}

	bool success = true;

	success &= updateNumVerts(nverts);
	success &= updateNumIndices(nindices);
	
	if (create && (nverts || nindices))
	{
		//actually allocate space for the vertex buffer if using VBO mapping
		flush(); //unmap

		if (gGLManager.mHasVertexArrayObject && useVBOs() && sUseVAO)
		{
#if GL_ARB_vertex_array_object
			mGLArray = getVAOName();
#endif
			setupVertexArray();
		}
	}

	return success;
}

void LLVertexBuffer::setupVertexArray()
{
	if (!mGLArray)
	{
		return;
	}

    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
#if GL_ARB_vertex_array_object
	glBindVertexArray(mGLArray);
#endif
	sGLRenderArray = mGLArray;

	static const U32 attrib_size[] = 
	{
		3, //TYPE_VERTEX,
		3, //TYPE_NORMAL,
		2, //TYPE_TEXCOORD0,
		2, //TYPE_TEXCOORD1,
		2, //TYPE_TEXCOORD2,
		2, //TYPE_TEXCOORD3,
		4, //TYPE_COLOR,
		4, //TYPE_EMISSIVE,
		4, //TYPE_TANGENT,
		1, //TYPE_WEIGHT,
		4, //TYPE_WEIGHT4,
		4, //TYPE_CLOTHWEIGHT,
		1, //TYPE_TEXTURE_INDEX
	};

	static const U32 attrib_type[] =
	{
		GL_FLOAT, //TYPE_VERTEX,
		GL_FLOAT, //TYPE_NORMAL,
		GL_FLOAT, //TYPE_TEXCOORD0,
		GL_FLOAT, //TYPE_TEXCOORD1,
		GL_FLOAT, //TYPE_TEXCOORD2,
		GL_FLOAT, //TYPE_TEXCOORD3,
		GL_UNSIGNED_BYTE, //TYPE_COLOR,
		GL_UNSIGNED_BYTE, //TYPE_EMISSIVE,
		GL_FLOAT,   //TYPE_TANGENT,
		GL_FLOAT, //TYPE_WEIGHT,
		GL_FLOAT, //TYPE_WEIGHT4,
		GL_FLOAT, //TYPE_CLOTHWEIGHT,
		GL_UNSIGNED_INT, //TYPE_TEXTURE_INDEX
	};

	static const bool attrib_integer[] =
	{
		false, //TYPE_VERTEX,
		false, //TYPE_NORMAL,
		false, //TYPE_TEXCOORD0,
		false, //TYPE_TEXCOORD1,
		false, //TYPE_TEXCOORD2,
		false, //TYPE_TEXCOORD3,
		false, //TYPE_COLOR,
		false, //TYPE_EMISSIVE,
		false, //TYPE_TANGENT,
		false, //TYPE_WEIGHT,
		false, //TYPE_WEIGHT4,
		false, //TYPE_CLOTHWEIGHT,
		true, //TYPE_TEXTURE_INDEX
	};

	static const U32 attrib_normalized[] =
	{
		GL_FALSE, //TYPE_VERTEX,
		GL_FALSE, //TYPE_NORMAL,
		GL_FALSE, //TYPE_TEXCOORD0,
		GL_FALSE, //TYPE_TEXCOORD1,
		GL_FALSE, //TYPE_TEXCOORD2,
		GL_FALSE, //TYPE_TEXCOORD3,
		GL_TRUE, //TYPE_COLOR,
		GL_TRUE, //TYPE_EMISSIVE,
		GL_FALSE,   //TYPE_TANGENT,
		GL_FALSE, //TYPE_WEIGHT,
		GL_FALSE, //TYPE_WEIGHT4,
		GL_FALSE, //TYPE_CLOTHWEIGHT,
		GL_FALSE, //TYPE_TEXTURE_INDEX
	};

	bindGLBuffer(true);
	bindGLIndices(true);

	for (U32 i = 0; i < TYPE_MAX; ++i)
	{
		if (mTypeMask & (1 << i))
		{
			glEnableVertexAttribArrayARB(i);

			if (attrib_integer[i])
			{
#if !LL_DARWIN
				//glVertexattribIPointer requires GLSL 1.30 or later
				if (gGLManager.mGLSLVersionMajor > 1 || gGLManager.mGLSLVersionMinor >= 30)
				{
					// nat 2018-10-24: VS 2017 also notices the issue
					// described below, and warns even with reinterpret_cast.
					// Cast via intptr_t to make it painfully obvious to the
					// compiler that we're doing this intentionally.
					glVertexAttribIPointer(i, attrib_size[i], attrib_type[i], sTypeSize[i],
										   reinterpret_cast<const GLvoid*>(intptr_t(mOffsets[i]))); 
				}
#endif
			}
			else
			{
				// nat 2016-12-16: With 64-bit clang compile, the compiler
				// produces an error if we simply cast mOffsets[i] -- an S32
				// -- to (GLvoid *), the type of the parameter. It correctly
				// points out that there's no way an S32 could fit a real
				// pointer value. Ruslan asserts that in this case the last
				// param is interpreted as an array data offset within the VBO
				// rather than as an actual pointer, so it's okay.
				glVertexAttribPointerARB(i, attrib_size[i], attrib_type[i],
										 attrib_normalized[i], sTypeSize[i],
										 reinterpret_cast<GLvoid*>(intptr_t(mOffsets[i]))); 
			}
		}
		else
		{
			glDisableVertexAttribArrayARB(i);
		}
	}

	//draw a dummy triangle to set index array pointer
	//glDrawElements(GL_TRIANGLES, 0, GL_UNSIGNED_SHORT, NULL);

	unbind();
}

bool LLVertexBuffer::resizeBuffer(S32 newnverts, S32 newnindices)
{
	llassert(newnverts >= 0);
	llassert(newnindices >= 0);

	bool success = true;

	success &= updateNumVerts(newnverts);		
	success &= updateNumIndices(newnindices);
	
	if (useVBOs())
	{
		flush(); //unmap

		if (mGLArray)
		{ //if size changed, offsets changed
			setupVertexArray();
		}
	}

	return success;
}

bool LLVertexBuffer::useVBOs() const
{
	//it's generally ineffective to use VBO for things that are streaming on apple
	return (mUsage != 0);
}

//----------------------------------------------------------------------------

bool expand_region(LLVertexBuffer::MappedRegion& region, S32 index, S32 count)
{
	S32 end = index+count;
	S32 region_end = region.mIndex+region.mCount;
	
	if (end < region.mIndex ||
		index > region_end)
	{ //gap exists, do not merge
		return false;
	}

	S32 new_end = llmax(end, region_end);
	S32 new_index = llmin(index, region.mIndex);
	region.mIndex = new_index;
	region.mCount = new_end-new_index;
	return true;
}


// Map for data access
U8* LLVertexBuffer::mapVertexBuffer(S32 type, S32 index, S32 count, bool map_range)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	bindGLBuffer(true);
	if (mFinal)
	{
		LL_ERRS() << "LLVertexBuffer::mapVeretxBuffer() called on a finalized buffer." << LL_ENDL;
	}
	if (!useVBOs() && !mMappedData && !mMappedIndexData)
	{
		LL_ERRS() << "LLVertexBuffer::mapVertexBuffer() called on unallocated buffer." << LL_ENDL;
	}
		
	if (useVBOs())
	{
		if (!mMappable || gGLManager.mHasMapBufferRange || gGLManager.mHasFlushBufferRange)
		{
			if (count == -1)
			{
				count = mNumVerts-index;
			}

			bool mapped = false;
			//see if range is already mapped
			for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
			{
				MappedRegion& region = mMappedVertexRegions[i];
				if (region.mType == type)
				{
					if (expand_region(region, index, count))
					{
						mapped = true;
						break;
					}
				}
			}

			if (!mapped)
			{
				//not already mapped, map new region
				MappedRegion region(type, mMappable && map_range ? -1 : index, count);
				mMappedVertexRegions.push_back(region);
			}
		}

		if (mVertexLocked && map_range)
		{
			LL_ERRS() << "Attempted to map a specific range of a buffer that was already mapped." << LL_ENDL;
		}

		if (!mVertexLocked)
		{
			mVertexLocked = true;
			sMappedCount++;
			stop_glerror();	

			if(!mMappable)
			{
				map_range = false;
			}
			else
			{
				U8* src = NULL;
				waitFence();
				if (gGLManager.mHasMapBufferRange)
				{
					if (map_range)
					{
#ifdef GL_ARB_map_buffer_range
						S32 offset = mOffsets[type] + sTypeSize[type]*index;
						S32 length = (sTypeSize[type]*count+0xF) & ~0xF;
						src = (U8*) glMapBufferRange(GL_ARRAY_BUFFER_ARB, offset, length, 
							GL_MAP_WRITE_BIT | 
							GL_MAP_FLUSH_EXPLICIT_BIT | 
							GL_MAP_INVALIDATE_RANGE_BIT);
#endif
					}
					else
					{
#ifdef GL_ARB_map_buffer_range

						if (gDebugGL)
						{
							GLint size = 0;
							glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &size);

							if (size < mSize)
							{
								LL_ERRS() << "Invalid buffer size." << LL_ENDL;
							}
						}

						src = (U8*) glMapBufferRange(GL_ARRAY_BUFFER_ARB, 0, mSize, 
							GL_MAP_WRITE_BIT | 
							GL_MAP_FLUSH_EXPLICIT_BIT);
#endif
					}
				}
				else if (gGLManager.mHasFlushBufferRange)
				{
					if (map_range)
					{
#ifndef LL_MESA_HEADLESS
						glBufferParameteriAPPLE(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_FALSE);
						glBufferParameteriAPPLE(GL_ARRAY_BUFFER_ARB, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE);
#endif
						src = (U8*) glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
					}
					else
					{
						src = (U8*) glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
					}
				}
				else
				{
					map_range = false;
					src = (U8*) glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
				}

				llassert(src != NULL);

				mMappedData = LL_NEXT_ALIGNED_ADDRESS<U8>(src);
				mAlignedOffset = mMappedData - src;
			
				stop_glerror();
			}
				
			if (!mMappedData)
			{
				log_glerror();

				//check the availability of memory
				LLMemory::logMemoryInfo(true);
			
				if(mMappable)
				{			
					//--------------------
					//print out more debug info before crash
					LL_INFOS() << "vertex buffer size: (num verts : num indices) = " << getNumVerts() << " : " << getNumIndices() << LL_ENDL;
					GLint size;
					glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &size);
					LL_INFOS() << "GL_ARRAY_BUFFER_ARB size is " << size << LL_ENDL;
					//--------------------

					GLint buff;
					glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &buff);
					if ((GLuint)buff != mGLBuffer)
					{
						LL_ERRS() << "Invalid GL vertex buffer bound: " << buff << LL_ENDL;
					}

							
					LL_ERRS() << "glMapBuffer returned NULL (no vertex data)" << LL_ENDL;
				}
				else
				{
					LL_ERRS() << "memory allocation for vertex data failed." << LL_ENDL;
				}
			}
		}
	}
	else
	{
		map_range = false;
	}
	
	if (map_range && gGLManager.mHasMapBufferRange && mMappable)
	{
		return mMappedData;
	}
	else
	{
		return mMappedData+mOffsets[type]+sTypeSize[type]*index;
	}
}


U8* LLVertexBuffer::mapIndexBuffer(S32 index, S32 count, bool map_range)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	bindGLIndices(true);
	if (mFinal)
	{
		LL_ERRS() << "LLVertexBuffer::mapIndexBuffer() called on a finalized buffer." << LL_ENDL;
	}
	if (!useVBOs() && !mMappedData && !mMappedIndexData)
	{
		LL_ERRS() << "LLVertexBuffer::mapIndexBuffer() called on unallocated buffer." << LL_ENDL;
	}

	if (useVBOs())
	{
		if (!mMappable || gGLManager.mHasMapBufferRange || gGLManager.mHasFlushBufferRange)
		{
			if (count == -1)
			{
				count = mNumIndices-index;
			}

			bool mapped = false;
			//see if range is already mapped
			for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
			{
				MappedRegion& region = mMappedIndexRegions[i];
				if (expand_region(region, index, count))
				{
					mapped = true;
					break;
				}
			}

			if (!mapped)
			{
				//not already mapped, map new region
				MappedRegion region(TYPE_INDEX, mMappable && map_range ? -1 : index, count);
				mMappedIndexRegions.push_back(region);
			}
		}

		if (mIndexLocked && map_range)
		{
			LL_ERRS() << "Attempted to map a specific range of a buffer that was already mapped." << LL_ENDL;
		}

		if (!mIndexLocked)
		{
			mIndexLocked = true;
			sMappedCount++;
			stop_glerror();	

			if (gDebugGL && useVBOs())
			{
				GLint elem = 0;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &elem);

				if (elem != mGLIndices)
				{
					LL_ERRS() << "Wrong index buffer bound!" << LL_ENDL;
				}
			}

			if(!mMappable)
			{
				map_range = false;
			}
			else
			{
				U8* src = NULL;
				waitFence();
				if (gGLManager.mHasMapBufferRange)
				{
					if (map_range)
					{
#ifdef GL_ARB_map_buffer_range
						S32 offset = sizeof(U16)*index;
						S32 length = sizeof(U16)*count;
						src = (U8*) glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER_ARB, offset, length, 
							GL_MAP_WRITE_BIT | 
							GL_MAP_FLUSH_EXPLICIT_BIT | 
							GL_MAP_INVALIDATE_RANGE_BIT);
#endif
					}
					else
					{
#ifdef GL_ARB_map_buffer_range
						src = (U8*) glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, sizeof(U16)*mNumIndices, 
							GL_MAP_WRITE_BIT | 
							GL_MAP_FLUSH_EXPLICIT_BIT);
#endif
					}
				}
				else if (gGLManager.mHasFlushBufferRange)
				{
					if (map_range)
					{
#ifndef LL_MESA_HEADLESS
						glBufferParameteriAPPLE(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_BUFFER_SERIALIZED_MODIFY_APPLE, GL_FALSE);
						glBufferParameteriAPPLE(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_BUFFER_FLUSHING_UNMAP_APPLE, GL_FALSE);
#endif
						src = (U8*) glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
					}
					else
					{
						src = (U8*) glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
					}
				}
				else
				{
					map_range = false;
					src = (U8*) glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
				}

				llassert(src != NULL);


				mMappedIndexData = src; //LL_NEXT_ALIGNED_ADDRESS<U8>(src);
				mAlignedIndexOffset = mMappedIndexData - src;
				stop_glerror();
			}
		}

		if (!mMappedIndexData)
		{
			log_glerror();
			LLMemory::logMemoryInfo(true);

			if(mMappable)
			{
				GLint buff;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &buff);
				if ((GLuint)buff != mGLIndices)
				{
					LL_ERRS() << "Invalid GL index buffer bound: " << buff << LL_ENDL;
				}

				LL_ERRS() << "glMapBuffer returned NULL (no index data)" << LL_ENDL;
			}
			else
			{
				LL_ERRS() << "memory allocation for Index data failed. " << LL_ENDL;
			}
		}
	}
	else
	{
		map_range = false;
	}

	if (map_range && gGLManager.mHasMapBufferRange && mMappable)
	{
		return mMappedIndexData;
	}
	else
	{
		return mMappedIndexData + sizeof(U16)*index;
	}
}

void LLVertexBuffer::unmapBuffer()
{
	if (!useVBOs())
	{
		return; //nothing to unmap
	}

	bool updated_all = false;
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	if (mMappedData && mVertexLocked)
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - vertex");
		bindGLBuffer(true);
		updated_all = mIndexLocked; //both vertex and index buffers done updating

		if(!mMappable)
		{
			if (!mMappedVertexRegions.empty())
			{
				stop_glerror();
				for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
				{
					const MappedRegion& region = mMappedVertexRegions[i];
					S32 offset = region.mIndex >= 0 ? mOffsets[region.mType]+sTypeSize[region.mType]*region.mIndex : 0;
					S32 length = sTypeSize[region.mType]*region.mCount;
					if (mSize >= length + offset)
					{
						glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, offset, length, (U8*)mMappedData + offset);
					}
					else
					{
						GLint size = 0;
						glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &size);
						LL_WARNS() << "Attempted to map regions to a buffer that is too small, " 
							<< "mapped size: " << mSize
							<< ", gl buffer size: " << size
							<< ", length: " << length
							<< ", offset: " << offset
							<< LL_ENDL;
					}
					stop_glerror();
				}

				mMappedVertexRegions.clear();
			}
			else
			{
				stop_glerror();
				glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, getSize(), (U8*) mMappedData);
				stop_glerror();
			}
		}
		else
		{
			if (gGLManager.mHasMapBufferRange || gGLManager.mHasFlushBufferRange)
			{
				if (!mMappedVertexRegions.empty())
				{
                    LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - flush vertex");
					for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
					{
						const MappedRegion& region = mMappedVertexRegions[i];
						S32 offset = region.mIndex >= 0 ? mOffsets[region.mType]+sTypeSize[region.mType]*region.mIndex : 0;
						S32 length = sTypeSize[region.mType]*region.mCount;
						if (gGLManager.mHasMapBufferRange)
						{
#ifdef GL_ARB_map_buffer_range
							glFlushMappedBufferRange(GL_ARRAY_BUFFER_ARB, offset, length);
#endif
						}
						else if (gGLManager.mHasFlushBufferRange)
                        {
#ifndef LL_MESA_HEADLESS
							glFlushMappedBufferRangeAPPLE(GL_ARRAY_BUFFER_ARB, offset, length);
#endif
						}
					}

					mMappedVertexRegions.clear();
				}
			}
			stop_glerror();
			glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			stop_glerror();

			mMappedData = NULL;
		}

		mVertexLocked = false;
		sMappedCount--;
	}
	
	if (mMappedIndexData && mIndexLocked)
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - index");
		bindGLIndices();
		if(!mMappable)
		{
			if (!mMappedIndexRegions.empty())
			{
				for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
				{
					const MappedRegion& region = mMappedIndexRegions[i];
					S32 offset = region.mIndex >= 0 ? sizeof(U16)*region.mIndex : 0;
					S32 length = sizeof(U16)*region.mCount;
					if (mIndicesSize >= length + offset)
					{
						glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, offset, length, (U8*) mMappedIndexData+offset);
					}
					else
					{
						GLint size = 0;
						glGetBufferParameterivARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &size);
						LL_WARNS() << "Attempted to map regions to a buffer that is too small, " 
							<< "mapped size: " << mIndicesSize
							<< ", gl buffer size: " << size
							<< ", length: " << length
							<< ", offset: " << offset
							<< LL_ENDL;
					}
					stop_glerror();
				}

				mMappedIndexRegions.clear();
			}
			else
			{
				stop_glerror();
				glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, getIndicesSize(), (U8*) mMappedIndexData);
				stop_glerror();
			}
		}
		else
		{
			if (gGLManager.mHasMapBufferRange || gGLManager.mHasFlushBufferRange)
			{
				if (!mMappedIndexRegions.empty())
				{
					for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
					{
                        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - flush index");
						const MappedRegion& region = mMappedIndexRegions[i];
						S32 offset = region.mIndex >= 0 ? sizeof(U16)*region.mIndex : 0;
						S32 length = sizeof(U16)*region.mCount;
						if (gGLManager.mHasMapBufferRange)
						{
#ifdef GL_ARB_map_buffer_range
							glFlushMappedBufferRange(GL_ELEMENT_ARRAY_BUFFER_ARB, offset, length);
#endif
						}
						else if (gGLManager.mHasFlushBufferRange)
						{
#ifdef GL_APPLE_flush_buffer_range
#ifndef LL_MESA_HEADLESS
							glFlushMappedBufferRangeAPPLE(GL_ELEMENT_ARRAY_BUFFER_ARB, offset, length);
#endif
#endif
						}
						stop_glerror();
					}

					mMappedIndexRegions.clear();
				}
			}
			
            glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);

			mMappedIndexData = NULL;
		}

		mIndexLocked = false;
		sMappedCount--;
	}

	if(updated_all)
	{
		mEmpty = false;
	}
}

//----------------------------------------------------------------------------

template <class T,S32 type> struct VertexBufferStrider
{
	typedef LLStrider<T> strider_t;
	static bool get(LLVertexBuffer& vbo, 
					strider_t& strider, 
					S32 index, S32 count, bool map_range)
	{
		if (type == LLVertexBuffer::TYPE_INDEX)
		{
			U8* ptr = vbo.mapIndexBuffer(index, count, map_range);

			if (ptr == NULL)
			{
				LL_WARNS() << "mapIndexBuffer failed!" << LL_ENDL;
				return false;
			}

			strider = (T*)ptr;
			strider.setStride(0);
			return true;
		}
		else if (vbo.hasDataType(type))
		{
			S32 stride = LLVertexBuffer::sTypeSize[type];

			U8* ptr = vbo.mapVertexBuffer(type, index, count, map_range);

			if (ptr == NULL)
			{
				LL_WARNS() << "mapVertexBuffer failed!" << LL_ENDL;
				return false;
			}

			strider = (T*)ptr;
			strider.setStride(stride);
			return true;
		}
		else
		{
			LL_ERRS() << "VertexBufferStrider could not find valid vertex data." << LL_ENDL;
		}
		return false;
	}
};

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector3>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector3,TYPE_VERTEX>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector4a>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector4a,TYPE_VERTEX>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getIndexStrider(LLStrider<U16>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<U16,TYPE_INDEX>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getTexCoord0Strider(LLStrider<LLVector2>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD0>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getTexCoord1Strider(LLStrider<LLVector2>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD1>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD2>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector3>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector3,TYPE_NORMAL>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector3>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector3,TYPE_TANGENT>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector4a>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector4a,TYPE_TANGENT>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getColorStrider(LLStrider<LLColor4U>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLColor4U,TYPE_COLOR>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getEmissiveStrider(LLStrider<LLColor4U>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLColor4U,TYPE_EMISSIVE>::get(*this, strider, index, count, map_range);
}
bool LLVertexBuffer::getWeightStrider(LLStrider<F32>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<F32,TYPE_WEIGHT>::get(*this, strider, index, count, map_range);
}

bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector4,TYPE_WEIGHT4>::get(*this, strider, index, count, map_range);
}

bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4>& strider, S32 index, S32 count, bool map_range)
{
	return VertexBufferStrider<LLVector4,TYPE_CLOTHWEIGHT>::get(*this, strider, index, count, map_range);
}

//----------------------------------------------------------------------------

bool LLVertexBuffer::bindGLArray()
{
	if (mGLArray && sGLRenderArray != mGLArray)
	{
		{
            LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
#if GL_ARB_vertex_array_object
			glBindVertexArray(mGLArray);
#endif
			sGLRenderArray = mGLArray;
		}

		//really shouldn't be necessary, but some drivers don't properly restore the
		//state of GL_ELEMENT_ARRAY_BUFFER_BINDING
		bindGLIndices();
		
		return true;
	}
		
	return false;
}

bool LLVertexBuffer::bindGLBuffer(bool force_bind)
{
	bindGLArray();

	bool ret = false;

	if (useVBOs() && (force_bind || (mGLBuffer && (mGLBuffer != sGLRenderBuffer || !sVBOActive))))
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, mGLBuffer);
		sGLRenderBuffer = mGLBuffer;
		sBindCount++;
		sVBOActive = true;

		llassert(!mGLArray || sGLRenderArray == mGLArray);

		ret = true;
	}

	return ret;
}

bool LLVertexBuffer::bindGLBufferFast()
{
    if (mGLBuffer != sGLRenderBuffer || !sVBOActive)
    {
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, mGLBuffer);
        sGLRenderBuffer = mGLBuffer;
        sBindCount++;
        sVBOActive = true;

        return true;
    }

    return false;
}

bool LLVertexBuffer::bindGLIndices(bool force_bind)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	bindGLArray();

	bool ret = false;
	if (useVBOs() && (force_bind || (mGLIndices && (mGLIndices != sGLRenderIndices || !sIBOActive))))
	{
		/*if (sMapped)
		{
			LL_ERRS() << "VBO bound while another VBO mapped!" << LL_ENDL;
		}*/
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mGLIndices);
		sGLRenderIndices = mGLIndices;
		stop_glerror();
		sBindCount++;
		sIBOActive = true;
		ret = true;
	}

	return ret;
}

bool LLVertexBuffer::bindGLIndicesFast()
{
    if (mGLIndices != sGLRenderIndices || !sIBOActive)
    {
        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mGLIndices);
        sGLRenderIndices = mGLIndices;
        sBindCount++;
        sIBOActive = true;
        
        return true;
    }

    return false;
}

void LLVertexBuffer::flush()
{
	if (useVBOs())
	{
		unmapBuffer();
	}
}

// bind for transform feedback (quick 'n dirty)
void LLVertexBuffer::bindForFeedback(U32 channel, U32 type, U32 index, U32 count)
{
#ifdef GL_TRANSFORM_FEEDBACK_BUFFER
	U32 offset = mOffsets[type] + sTypeSize[type]*index;
	U32 size= (sTypeSize[type]*count);
	glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, channel, mGLBuffer, offset, size);
#endif
}

// Set for rendering
void LLVertexBuffer::setBuffer(U32 data_mask)
{
	flush();

	//set up pointers if the data mask is different ...
	bool setup = (sLastMask != data_mask);

	if (useVBOs())
	{
		if (mGLArray)
		{
			bindGLArray();
			setup = false; //do NOT perform pointer setup if using VAO
		}
		else
		{
			const bool bindBuffer = bindGLBuffer();
			const bool bindIndices = bindGLIndices();
			
			setup = setup || bindBuffer || bindIndices;
		}

		if (gDebugGL && !mGLArray)
		{
			GLint buff;
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &buff);
			if ((GLuint)buff != mGLBuffer)
			{
				if (gDebugSession)
				{
					gFailLog << "Invalid GL vertex buffer bound: " << buff << std::endl;
				}
				else
				{
					LL_ERRS() << "Invalid GL vertex buffer bound: " << buff << LL_ENDL;
				}
			}

			if (mGLIndices)
			{
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &buff);
				if ((GLuint)buff != mGLIndices)
				{
					if (gDebugSession)
					{
						gFailLog << "Invalid GL index buffer bound: " << buff <<  std::endl;
					}
					else
					{
						LL_ERRS() << "Invalid GL index buffer bound: " << buff << LL_ENDL;
					}
				}
			}
		}

		
	}
	else
	{	
		if (sGLRenderArray)
		{
#if GL_ARB_vertex_array_object
			glBindVertexArray(0);
#endif
			sGLRenderArray = 0;
			sGLRenderIndices = 0;
			sIBOActive = false;
		}

		if (mGLBuffer)
		{
			if (sVBOActive)
			{
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
				sBindCount++;
				sVBOActive = false;
				setup = true; // ... or a VBO is deactivated
			}
			if (sGLRenderBuffer != mGLBuffer)
			{
				sGLRenderBuffer = mGLBuffer;
				setup = true; // ... or a client memory pointer changed
			}
		}
		if (mGLIndices)
		{
			if (sIBOActive)
			{
				glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
				sBindCount++;
				sIBOActive = false;
			}
			
			sGLRenderIndices = mGLIndices;
		}
	}

	if (!mGLArray)
	{
		setupClientArrays(data_mask);
	}
			
	if (mGLBuffer)
	{
		if (data_mask && setup)
		{
			setupVertexBuffer(data_mask); // subclass specific setup (virtual function)
			sSetCount++;
		}
	}
}

void LLVertexBuffer::setBufferFast(U32 data_mask)
{
    if (useVBOs())
    {
        //set up pointers if the data mask is different ...
        bool setup = (sLastMask != data_mask);

        const bool bindBuffer = bindGLBufferFast();
        const bool bindIndices = bindGLIndicesFast();

        setup = setup || bindBuffer || bindIndices;
        
        setupClientArrays(data_mask);

        if (data_mask && setup)
        {
            setupVertexBufferFast(data_mask);
            sSetCount++;
        }
    }
    else
    {
        //fallback to slow path when not using VBOs
        setBuffer(data_mask);
    }
}


// virtual (default)
void LLVertexBuffer::setupVertexBuffer(U32 data_mask)
{
	stop_glerror();
	U8* base = useVBOs() ? (U8*) mAlignedOffset : mMappedData;

	if (gDebugGL && ((data_mask & mTypeMask) != data_mask))
	{
		for (U32 i = 0; i < LLVertexBuffer::TYPE_MAX; ++i)
		{
			U32 mask = 1 << i;
			if (mask & data_mask && !(mask & mTypeMask))
			{ //bit set in data_mask, but not set in mTypeMask
				LL_WARNS() << "Missing required component " << vb_type_name[i] << LL_ENDL;
			}
		}
		LL_ERRS() << "LLVertexBuffer::setupVertexBuffer missing required components for supplied data mask." << LL_ENDL;
	}

	if (data_mask & MAP_NORMAL)
	{
		S32 loc = TYPE_NORMAL;
		void* ptr = (void*)(base + mOffsets[TYPE_NORMAL]);
		glVertexAttribPointerARB(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_NORMAL], ptr);
	}
	if (data_mask & MAP_TEXCOORD3)
	{
		S32 loc = TYPE_TEXCOORD3;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD3]);
		glVertexAttribPointerARB(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD3], ptr);
	}
	if (data_mask & MAP_TEXCOORD2)
	{
		S32 loc = TYPE_TEXCOORD2;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD2]);
		glVertexAttribPointerARB(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD2], ptr);
	}
	if (data_mask & MAP_TEXCOORD1)
	{
		S32 loc = TYPE_TEXCOORD1;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD1]);
		glVertexAttribPointerARB(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD1], ptr);
	}
	if (data_mask & MAP_TANGENT)
	{
		S32 loc = TYPE_TANGENT;
		void* ptr = (void*)(base + mOffsets[TYPE_TANGENT]);
		glVertexAttribPointerARB(loc, 4,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TANGENT], ptr);
	}
	if (data_mask & MAP_TEXCOORD0)
	{
		S32 loc = TYPE_TEXCOORD0;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD0]);
		glVertexAttribPointerARB(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD0], ptr);
	}
	if (data_mask & MAP_COLOR)
	{
		S32 loc = TYPE_COLOR;
		//bind emissive instead of color pointer if emissive is present
		void* ptr = (data_mask & MAP_EMISSIVE) ? (void*)(base + mOffsets[TYPE_EMISSIVE]) : (void*)(base + mOffsets[TYPE_COLOR]);
		glVertexAttribPointerARB(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_COLOR], ptr);
	}
	if (data_mask & MAP_EMISSIVE)
	{
		S32 loc = TYPE_EMISSIVE;
		void* ptr = (void*)(base + mOffsets[TYPE_EMISSIVE]);
		glVertexAttribPointerARB(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_EMISSIVE], ptr);

		if (!(data_mask & MAP_COLOR))
		{ //map emissive to color channel when color is not also being bound to avoid unnecessary shader swaps
			loc = TYPE_COLOR;
			glVertexAttribPointerARB(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_EMISSIVE], ptr);
		}
	}
	if (data_mask & MAP_WEIGHT)
	{
		S32 loc = TYPE_WEIGHT;
		void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT]);
		glVertexAttribPointerARB(loc, 1, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT], ptr);
	}
	if (data_mask & MAP_WEIGHT4)
	{
		S32 loc = TYPE_WEIGHT4;
		void* ptr = (void*)(base+mOffsets[TYPE_WEIGHT4]);
		glVertexAttribPointerARB(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT4], ptr);
	}
	if (data_mask & MAP_CLOTHWEIGHT)
	{
		S32 loc = TYPE_CLOTHWEIGHT;
		void* ptr = (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]);
		glVertexAttribPointerARB(loc, 4, GL_FLOAT, GL_TRUE,  LLVertexBuffer::sTypeSize[TYPE_CLOTHWEIGHT], ptr);
	}
	if (data_mask & MAP_TEXTURE_INDEX && 
			(gGLManager.mGLSLVersionMajor >= 2 || gGLManager.mGLSLVersionMinor >= 30)) //indexed texture rendering requires GLSL 1.30 or later
	{
#if !LL_DARWIN
		S32 loc = TYPE_TEXTURE_INDEX;
		void *ptr = (void*) (base + mOffsets[TYPE_VERTEX] + 12);
		glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
#endif
	}
	if (data_mask & MAP_VERTEX)
	{
		S32 loc = TYPE_VERTEX;
		void* ptr = (void*)(base + mOffsets[TYPE_VERTEX]);
		glVertexAttribPointerARB(loc, 3,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
	}	

	llglassertok();
}

void LLVertexBuffer::setupVertexBufferFast(U32 data_mask)
{
    U8* base = (U8*)mAlignedOffset;

    if (data_mask & MAP_NORMAL)
    {
        S32 loc = TYPE_NORMAL;
        void* ptr = (void*)(base + mOffsets[TYPE_NORMAL]);
        glVertexAttribPointerARB(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_NORMAL], ptr);
    }
    if (data_mask & MAP_TEXCOORD3)
    {
        S32 loc = TYPE_TEXCOORD3;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD3]);
        glVertexAttribPointerARB(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD3], ptr);
    }
    if (data_mask & MAP_TEXCOORD2)
    {
        S32 loc = TYPE_TEXCOORD2;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD2]);
        glVertexAttribPointerARB(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD2], ptr);
    }
    if (data_mask & MAP_TEXCOORD1)
    {
        S32 loc = TYPE_TEXCOORD1;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD1]);
        glVertexAttribPointerARB(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD1], ptr);
    }
    if (data_mask & MAP_TANGENT)
    {
        S32 loc = TYPE_TANGENT;
        void* ptr = (void*)(base + mOffsets[TYPE_TANGENT]);
        glVertexAttribPointerARB(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TANGENT], ptr);
    }
    if (data_mask & MAP_TEXCOORD0)
    {
        S32 loc = TYPE_TEXCOORD0;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD0]);
        glVertexAttribPointerARB(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD0], ptr);
    }
    if (data_mask & MAP_COLOR)
    {
        S32 loc = TYPE_COLOR;
        //bind emissive instead of color pointer if emissive is present
        void* ptr = (data_mask & MAP_EMISSIVE) ? (void*)(base + mOffsets[TYPE_EMISSIVE]) : (void*)(base + mOffsets[TYPE_COLOR]);
        glVertexAttribPointerARB(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_COLOR], ptr);
    }
    if (data_mask & MAP_EMISSIVE)
    {
        S32 loc = TYPE_EMISSIVE;
        void* ptr = (void*)(base + mOffsets[TYPE_EMISSIVE]);
        glVertexAttribPointerARB(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_EMISSIVE], ptr);

        if (!(data_mask & MAP_COLOR))
        { //map emissive to color channel when color is not also being bound to avoid unnecessary shader swaps
            loc = TYPE_COLOR;
            glVertexAttribPointerARB(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_EMISSIVE], ptr);
        }
    }
    if (data_mask & MAP_WEIGHT)
    {
        S32 loc = TYPE_WEIGHT;
        void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT]);
        glVertexAttribPointerARB(loc, 1, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT], ptr);
    }
    if (data_mask & MAP_WEIGHT4)
    {
        S32 loc = TYPE_WEIGHT4;
        void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT4]);
        glVertexAttribPointerARB(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT4], ptr);
    }
    if (data_mask & MAP_CLOTHWEIGHT)
    {
        S32 loc = TYPE_CLOTHWEIGHT;
        void* ptr = (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]);
        glVertexAttribPointerARB(loc, 4, GL_FLOAT, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_CLOTHWEIGHT], ptr);
    }
    if (data_mask & MAP_TEXTURE_INDEX)
    {
#if !LL_DARWIN
        S32 loc = TYPE_TEXTURE_INDEX;
        void* ptr = (void*)(base + mOffsets[TYPE_VERTEX] + 12);
        glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
#endif
    }
    if (data_mask & MAP_VERTEX)
    {
        S32 loc = TYPE_VERTEX;
        void* ptr = (void*)(base + mOffsets[TYPE_VERTEX]);
        glVertexAttribPointerARB(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
    }
}

LLVertexBuffer::MappedRegion::MappedRegion(S32 type, S32 index, S32 count)
: mType(type), mIndex(index), mCount(count)
{ 
	mEnd = mIndex+mCount;	
}	


