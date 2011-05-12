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
#include "llmemory.h"

#include <boost/static_assert.hpp>
#include "llsys.h"
#include "llvertexbuffer.h"
// #include "llrender.h"
#include "llglheaders.h"
#include "llmemtype.h"
#include "llrender.h"
#include "llvector4a.h"

//============================================================================

//static
LLVBOPool LLVertexBuffer::sStreamVBOPool;
LLVBOPool LLVertexBuffer::sDynamicVBOPool;
LLVBOPool LLVertexBuffer::sStreamIBOPool;
LLVBOPool LLVertexBuffer::sDynamicIBOPool;

U32 LLVertexBuffer::sBindCount = 0;
U32 LLVertexBuffer::sSetCount = 0;
S32 LLVertexBuffer::sCount = 0;
S32 LLVertexBuffer::sGLCount = 0;
S32 LLVertexBuffer::sMappedCount = 0;
BOOL LLVertexBuffer::sDisableVBOMapping = FALSE ;
BOOL LLVertexBuffer::sEnableVBOs = TRUE;
U32 LLVertexBuffer::sGLRenderBuffer = 0;
U32 LLVertexBuffer::sGLRenderIndices = 0;
U32 LLVertexBuffer::sLastMask = 0;
BOOL LLVertexBuffer::sVBOActive = FALSE;
BOOL LLVertexBuffer::sIBOActive = FALSE;
U32 LLVertexBuffer::sAllocatedBytes = 0;
BOOL LLVertexBuffer::sMapped = FALSE;
BOOL LLVertexBuffer::sUseStreamDraw = TRUE;
BOOL LLVertexBuffer::sPreferStreamDraw = FALSE;
S32	LLVertexBuffer::sWeight4Loc = -1;

std::vector<U32> LLVertexBuffer::sDeleteList;


S32 LLVertexBuffer::sTypeSize[LLVertexBuffer::TYPE_MAX] =
{
	sizeof(LLVector4), // TYPE_VERTEX,
	sizeof(LLVector4), // TYPE_NORMAL,
	sizeof(LLVector2), // TYPE_TEXCOORD0,
	sizeof(LLVector2), // TYPE_TEXCOORD1,
	sizeof(LLVector2), // TYPE_TEXCOORD2,
	sizeof(LLVector2), // TYPE_TEXCOORD3,
	sizeof(LLColor4U), // TYPE_COLOR,
	sizeof(LLVector4), // TYPE_BINORMAL,
	sizeof(F32),	   // TYPE_WEIGHT,
	sizeof(LLVector4), // TYPE_WEIGHT4,
	sizeof(LLVector4), // TYPE_CLOTHWEIGHT,
};

U32 LLVertexBuffer::sGLMode[LLRender::NUM_MODES] = 
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
void LLVertexBuffer::setupClientArrays(U32 data_mask)
{
	/*if (LLGLImmediate::sStarted)
	{
		llerrs << "Cannot use LLGLImmediate and LLVertexBuffer simultaneously!" << llendl;
	}*/

	if (sLastMask != data_mask)
	{
		U32 mask[] =
		{
			MAP_VERTEX,
			MAP_NORMAL,
			MAP_TEXCOORD0,
			MAP_COLOR,
		};
		
		GLenum array[] =
		{
			GL_VERTEX_ARRAY,
			GL_NORMAL_ARRAY,
			GL_TEXTURE_COORD_ARRAY,
			GL_COLOR_ARRAY,
		};

		BOOL error = FALSE;
		for (U32 i = 0; i < 4; ++i)
		{
			if (sLastMask & mask[i])
			{ //was enabled
				if (!(data_mask & mask[i]) && i > 0)
				{ //needs to be disabled
					glDisableClientState(array[i]);
				}
				else if (gDebugGL)
				{ //needs to be enabled, make sure it was (DEBUG TEMPORARY)
					if (i > 0 && !glIsEnabled(array[i]))
					{
						if (gDebugSession)
						{
							error = TRUE;
							gFailLog << "Bad client state! " << array[i] << " disabled." << std::endl;
						}
						else
						{
							llerrs << "Bad client state! " << array[i] << " disabled." << llendl;
						}
					}
				}
			}
			else 
			{	//was disabled
				if (data_mask & mask[i] && i > 0)
				{ //needs to be enabled
					glEnableClientState(array[i]);
				}
				else if (gDebugGL && i > 0 && glIsEnabled(array[i]))
				{ //needs to be disabled, make sure it was (DEBUG TEMPORARY)
					if (gDebugSession)
					{
						error = TRUE;
						gFailLog << "Bad client state! " << array[i] << " enabled." << std::endl;
					}
					else
					{
						llerrs << "Bad client state! " << array[i] << " enabled." << llendl;
					}
				}
			}
		}

		if (error)
		{
			ll_fail("LLVertexBuffer::setupClientArrays failed");
		}

		U32 map_tc[] = 
		{
			MAP_TEXCOORD1,
			MAP_TEXCOORD2,
			MAP_TEXCOORD3
		};

		for (U32 i = 0; i < 3; i++)
		{
			if (sLastMask & map_tc[i])
			{
				if (!(data_mask & map_tc[i]))
				{
					glClientActiveTextureARB(GL_TEXTURE1_ARB+i);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glClientActiveTextureARB(GL_TEXTURE0_ARB);
				}
			}
			else if (data_mask & map_tc[i])
			{
				glClientActiveTextureARB(GL_TEXTURE1_ARB+i);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}

		if (sLastMask & MAP_BINORMAL)
		{
			if (!(data_mask & MAP_BINORMAL))
			{
				glClientActiveTextureARB(GL_TEXTURE2_ARB);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glClientActiveTextureARB(GL_TEXTURE0_ARB);
			}
		}
		else if (data_mask & MAP_BINORMAL)
		{
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
		}
	
		if (sLastMask & MAP_WEIGHT4)
		{
			if (sWeight4Loc < 0)
			{
				llerrs << "Weighting disabled but vertex buffer still bound!" << llendl;
			}

			if (!(data_mask & MAP_WEIGHT4))
			{ //disable 4-component skin weight			
				glDisableVertexAttribArrayARB(sWeight4Loc);
			}
		}
		else if (data_mask & MAP_WEIGHT4)
		{
			if (sWeight4Loc >= 0)
			{ //enable 4-component skin weight
				glEnableVertexAttribArrayARB(sWeight4Loc);
			}
		}
				

		sLastMask = data_mask;
	}
}

//static
void LLVertexBuffer::drawArrays(U32 mode, const std::vector<LLVector3>& pos, const std::vector<LLVector3>& norm)
{
	U32 count = pos.size();
	llassert(norm.size() >= pos.size());

	unbind();
	
	setupClientArrays(MAP_VERTEX | MAP_NORMAL);

	glVertexPointer(3, GL_FLOAT, 0, pos[0].mV);
	glNormalPointer(GL_FLOAT, 0, norm[0].mV);

	glDrawArrays(sGLMode[mode], 0, count);
}

void LLVertexBuffer::validateRange(U32 start, U32 end, U32 count, U32 indices_offset) const
{
	if (start >= (U32) mRequestedNumVerts ||
	    end >= (U32) mRequestedNumVerts)
	{
		llerrs << "Bad vertex buffer draw range: [" << start << ", " << end << "] vs " << mRequestedNumVerts << llendl;
	}

	llassert(mRequestedNumIndices >= 0);

	if (indices_offset >= (U32) mRequestedNumIndices ||
	    indices_offset + count > (U32) mRequestedNumIndices)
	{
		llerrs << "Bad index buffer draw range: [" << indices_offset << ", " << indices_offset+count << "]" << llendl;
	}

	if (gDebugGL && !useVBOs())
	{
		U16* idx = ((U16*) getIndicesPointer())+indices_offset;
		for (U32 i = 0; i < count; ++i)
		{
			if (idx[i] < start || idx[i] > end)
			{
				llerrs << "Index out of range: " << idx[i] << " not in [" << start << ", " << end << "]" << llendl;
			}
		}
	}
}

void LLVertexBuffer::drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const
{
	validateRange(start, end, count, indices_offset);

	llassert(mRequestedNumVerts >= 0);

	if (mGLIndices != sGLRenderIndices)
	{
		llerrs << "Wrong index buffer bound." << llendl;
	}

	if (mGLBuffer != sGLRenderBuffer)
	{
		llerrs << "Wrong vertex buffer bound." << llendl;
	}

	if (mode >= LLRender::NUM_MODES)
	{
		llerrs << "Invalid draw mode: " << mode << llendl;
		return;
	}

	U16* idx = ((U16*) getIndicesPointer())+indices_offset;

	stop_glerror();
	glDrawRangeElements(sGLMode[mode], start, end, count, GL_UNSIGNED_SHORT, 
		idx);
	stop_glerror();
}

void LLVertexBuffer::draw(U32 mode, U32 count, U32 indices_offset) const
{
	llassert(mRequestedNumIndices >= 0);
	if (indices_offset >= (U32) mRequestedNumIndices ||
	    indices_offset + count > (U32) mRequestedNumIndices)
	{
		llerrs << "Bad index buffer draw range: [" << indices_offset << ", " << indices_offset+count << "]" << llendl;
	}

	if (mGLIndices != sGLRenderIndices)
	{
		llerrs << "Wrong index buffer bound." << llendl;
	}

	if (mGLBuffer != sGLRenderBuffer)
	{
		llerrs << "Wrong vertex buffer bound." << llendl;
	}

	if (mode >= LLRender::NUM_MODES)
	{
		llerrs << "Invalid draw mode: " << mode << llendl;
		return;
	}

	stop_glerror();
	glDrawElements(sGLMode[mode], count, GL_UNSIGNED_SHORT,
		((U16*) getIndicesPointer()) + indices_offset);
	stop_glerror();
}

void LLVertexBuffer::drawArrays(U32 mode, U32 first, U32 count) const
{
	llassert(mRequestedNumVerts >= 0);
	if (first >= (U32) mRequestedNumVerts ||
	    first + count > (U32) mRequestedNumVerts)
	{
		llerrs << "Bad vertex buffer draw range: [" << first << ", " << first+count << "]" << llendl;
	}

	if (mGLBuffer != sGLRenderBuffer || useVBOs() != sVBOActive)
	{
		llerrs << "Wrong vertex buffer bound." << llendl;
	}

	if (mode >= LLRender::NUM_MODES)
	{
		llerrs << "Invalid draw mode: " << mode << llendl;
		return;
	}

	stop_glerror();
	glDrawArrays(sGLMode[mode], first, count);
	stop_glerror();
}

//static
void LLVertexBuffer::initClass(bool use_vbo, bool no_vbo_mapping)
{
	sEnableVBOs = use_vbo && gGLManager.mHasVertexBufferObject ;
	if(sEnableVBOs)
	{
		//llassert_always(glBindBufferARB) ; //double check the extention for VBO is loaded.

		llinfos << "VBO is enabled." << llendl ;
	}
	else
	{
		llinfos << "VBO is disabled." << llendl ;
	}

	sDisableVBOMapping = sEnableVBOs && no_vbo_mapping ;
}

//static 
void LLVertexBuffer::unbind()
{
	if (sVBOActive)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		sVBOActive = FALSE;
	}
	if (sIBOActive)
	{
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		sIBOActive = FALSE;
	}

	sGLRenderBuffer = 0;
	sGLRenderIndices = 0;

	setupClientArrays(0);
}

//static
void LLVertexBuffer::cleanupClass()
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_CLEANUP_CLASS);
	unbind();
	clientCopy(); // deletes GL buffers

	//llassert_always(!sCount) ;
}

void LLVertexBuffer::clientCopy(F64 max_time)
{
	if (!sDeleteList.empty())
	{
		glDeleteBuffersARB(sDeleteList.size(), (GLuint*) &(sDeleteList[0]));
		sDeleteList.clear();
	}
}

//----------------------------------------------------------------------------

LLVertexBuffer::LLVertexBuffer(U32 typemask, S32 usage) :
	LLRefCount(),

	mNumVerts(0),
	mNumIndices(0),
	mRequestedNumVerts(-1),
	mRequestedNumIndices(-1),
	mUsage(usage),
	mGLBuffer(0),
	mGLIndices(0), 
	mMappedData(NULL),
	mMappedIndexData(NULL), 
	mVertexLocked(FALSE),
	mIndexLocked(FALSE),
	mFinal(FALSE),
	mFilthy(FALSE),
	mEmpty(TRUE),
	mResized(FALSE),
	mDynamicSize(FALSE)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_CONSTRUCTOR);
	if (!sEnableVBOs)
	{
		mUsage = 0 ; 
	}

	if (mUsage == GL_STREAM_DRAW_ARB && !sUseStreamDraw)
	{
		mUsage = 0;
	}
	
	if (mUsage == GL_DYNAMIC_DRAW_ARB && sPreferStreamDraw)
	{
		mUsage = GL_STREAM_DRAW_ARB;
	}

	//zero out offsets
	for (U32 i = 0; i < TYPE_MAX; i++)
	{
		mOffsets[i] = 0;
	}

	mTypeMask = typemask;
	mSize = 0;
	mAlignedOffset = 0;
	mAlignedIndexOffset = 0;

	sCount++;
}

//static
S32 LLVertexBuffer::calcOffsets(const U32& typemask, S32* offsets, S32 num_vertices)
{
	S32 offset = 0;
	for (S32 i=0; i<TYPE_MAX; i++)
	{
		U32 mask = 1<<i;
		if (typemask & mask)
		{
			if (offsets)
			{
				offsets[i] = offset;
				offset += LLVertexBuffer::sTypeSize[i]*num_vertices;
				offset = (offset + 0xF) & ~0xF;
			}
		}
	}

	return offset+16;
}

//static 
S32 LLVertexBuffer::calcVertexSize(const U32& typemask)
{
	S32 size = 0;
	for (S32 i = 0; i < TYPE_MAX; i++)
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
	LLMemType mt2(LLMemType::MTYPE_VERTEX_DESTRUCTOR);
	destroyGLBuffer();
	destroyGLIndices();
	sCount--;

	llassert_always(!mMappedData && !mMappedIndexData) ;
};

//----------------------------------------------------------------------------

void LLVertexBuffer::genBuffer()
{
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		mGLBuffer = sStreamVBOPool.allocate();
	}
	else if (mUsage == GL_DYNAMIC_DRAW_ARB)
	{
		mGLBuffer = sDynamicVBOPool.allocate();
	}
	else
	{
		BOOST_STATIC_ASSERT(sizeof(mGLBuffer) == sizeof(GLuint));
		glGenBuffersARB(1, (GLuint*)&mGLBuffer);
	}
	sGLCount++;
}

void LLVertexBuffer::genIndices()
{
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		mGLIndices = sStreamIBOPool.allocate();
	}
	else if (mUsage == GL_DYNAMIC_DRAW_ARB)
	{
		mGLIndices = sDynamicIBOPool.allocate();
	}
	else
	{
		BOOST_STATIC_ASSERT(sizeof(mGLBuffer) == sizeof(GLuint));
		glGenBuffersARB(1, (GLuint*)&mGLIndices);
	}
	sGLCount++;
}

void LLVertexBuffer::releaseBuffer()
{
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		sStreamVBOPool.release(mGLBuffer);
	}
	else if (mUsage == GL_DYNAMIC_DRAW_ARB)
	{
		sDynamicVBOPool.release(mGLBuffer);
	}
	else
	{
		sDeleteList.push_back(mGLBuffer);
	}
	sGLCount--;
}

void LLVertexBuffer::releaseIndices()
{
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		sStreamIBOPool.release(mGLIndices);
	}
	else if (mUsage == GL_DYNAMIC_DRAW_ARB)
	{
		sDynamicIBOPool.release(mGLIndices);
	}
	else
	{
		sDeleteList.push_back(mGLIndices);
	}
	sGLCount--;
}

void LLVertexBuffer::createGLBuffer()
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_CREATE_VERTICES);
	
	U32 size = getSize();
	if (mGLBuffer)
	{
		destroyGLBuffer();
	}

	if (size == 0)
	{
		return;
	}

	mEmpty = TRUE;

	if (useVBOs())
	{
		mMappedData = NULL;
		genBuffer();
		mResized = TRUE;
	}
	else
	{
		static int gl_buffer_idx = 0;
		mGLBuffer = ++gl_buffer_idx;
		mMappedData = (U8*) malloc(size);
	}
}

void LLVertexBuffer::createGLIndices()
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_CREATE_INDICES);
	U32 size = getIndicesSize();

	if (mGLIndices)
	{
		destroyGLIndices();
	}
	
	if (size == 0)
	{
		return;
	}

	mEmpty = TRUE;

	//pad by 16 bytes for aligned copies
	size += 16;

	if (useVBOs())
	{
		//pad by another 16 bytes for VBO pointer adjustment
		size += 16;
		mMappedIndexData = NULL;
		genIndices();
		mResized = TRUE;
	}
	else
	{
		mMappedIndexData = (U8*) malloc(size);
		static int gl_buffer_idx = 0;
		mGLIndices = ++gl_buffer_idx;
	}
}

void LLVertexBuffer::destroyGLBuffer()
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_DESTROY_BUFFER);
	if (mGLBuffer)
	{
		if (useVBOs())
		{
			freeClientBuffer() ;

			if (mMappedData || mMappedIndexData)
			{
				llerrs << "Vertex buffer destroyed while mapped!" << llendl;
			}
			releaseBuffer();
		}
		else
		{
			free(mMappedData);
			mMappedData = NULL;
			mEmpty = TRUE;
		}

		sAllocatedBytes -= getSize();
	}
	
	mGLBuffer = 0;
	//unbind();
}

void LLVertexBuffer::destroyGLIndices()
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_DESTROY_INDICES);
	if (mGLIndices)
	{
		if (useVBOs())
		{
			freeClientBuffer() ;

			if (mMappedData || mMappedIndexData)
			{
				llerrs << "Vertex buffer destroyed while mapped." << llendl;
			}
			releaseIndices();
		}
		else
		{
			free(mMappedIndexData);
			mMappedIndexData = NULL;
			mEmpty = TRUE;
		}

		sAllocatedBytes -= getIndicesSize();
	}

	mGLIndices = 0;
	//unbind();
}

void LLVertexBuffer::updateNumVerts(S32 nverts)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_UPDATE_VERTS);

	llassert(nverts >= 0);

	if (nverts >= 65535)
	{
		llwarns << "Vertex buffer overflow!" << llendl;
		nverts = 65535;
	}

	mRequestedNumVerts = nverts;

	if (!mDynamicSize)
	{
		mNumVerts = nverts;
	}
	else if (mUsage == GL_STATIC_DRAW_ARB ||
		nverts > mNumVerts ||
		nverts < mNumVerts/2)
	{
		if (mUsage != GL_STATIC_DRAW_ARB && nverts + nverts/4 <= 65535)
		{
			nverts += nverts/4;
		}
		mNumVerts = nverts;
	}
	mSize = calcOffsets(mTypeMask, mOffsets, mNumVerts);
}

void LLVertexBuffer::updateNumIndices(S32 nindices)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_UPDATE_INDICES);

	llassert(nindices >= 0);

	mRequestedNumIndices = nindices;
	if (!mDynamicSize)
	{
		mNumIndices = nindices;
	}
	else if (mUsage == GL_STATIC_DRAW_ARB ||
		nindices > mNumIndices ||
		nindices < mNumIndices/2)
	{
		if (mUsage != GL_STATIC_DRAW_ARB)
		{
			nindices += nindices/4;
		}

		mNumIndices = nindices;
	}
}

void LLVertexBuffer::allocateBuffer(S32 nverts, S32 nindices, bool create)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_ALLOCATE_BUFFER);
		
	if (nverts < 0 || nindices < 0 ||
		nverts > 65536)
	{
		llerrs << "Bad vertex buffer allocation: " << nverts << " : " << nindices << llendl;
	}

	updateNumVerts(nverts);
	updateNumIndices(nindices);
	
	if (mMappedData)
	{
		llerrs << "LLVertexBuffer::allocateBuffer() called redundantly." << llendl;
	}
	if (create && (nverts || nindices))
	{
		createGLBuffer();
		createGLIndices();
	}
	
	sAllocatedBytes += getSize() + getIndicesSize();
}

void LLVertexBuffer::resizeBuffer(S32 newnverts, S32 newnindices)
{
	llassert(newnverts >= 0);
	llassert(newnindices >= 0);

	mRequestedNumVerts = newnverts;
	mRequestedNumIndices = newnindices;

	LLMemType mt2(LLMemType::MTYPE_VERTEX_RESIZE_BUFFER);
	mDynamicSize = TRUE;
	if (mUsage == GL_STATIC_DRAW_ARB)
	{ //always delete/allocate static buffers on resize
		destroyGLBuffer();
		destroyGLIndices();
		allocateBuffer(newnverts, newnindices, TRUE);
		mFinal = FALSE;
	}
	else if (newnverts > mNumVerts || newnindices > mNumIndices ||
			 newnverts < mNumVerts/2 || newnindices < mNumIndices/2)
	{
		sAllocatedBytes -= getSize() + getIndicesSize();
		
		updateNumVerts(newnverts);		
		updateNumIndices(newnindices);
		
		S32 newsize = getSize();
		S32 new_index_size = getIndicesSize();

		sAllocatedBytes += newsize + new_index_size;

		if (newsize)
		{
			if (!mGLBuffer)
			{ //no buffer exists, create a new one
				createGLBuffer();
			}
			else
			{
				if (!useVBOs())
				{
					free(mMappedData);
					mMappedData = (U8*) malloc(newsize);
				}
				mResized = TRUE;
			}
		}
		else if (mGLBuffer)
		{
			destroyGLBuffer();
		}
		
		if (new_index_size)
		{
			if (!mGLIndices)
			{
				createGLIndices();
			}
			else
			{
				if (!useVBOs())
				{
					free(mMappedIndexData);
					mMappedIndexData = (U8*) malloc(new_index_size);
				}
				mResized = TRUE;
			}
		}
		else if (mGLIndices)
		{
			destroyGLIndices();
		}
	}

	if (mResized && useVBOs())
	{
		freeClientBuffer() ;
		setBuffer(0);
	}
}

BOOL LLVertexBuffer::useVBOs() const
{
	//it's generally ineffective to use VBO for things that are streaming on apple
		
#if LL_DARWIN
	if (!mUsage || mUsage == GL_STREAM_DRAW_ARB)
	{
		return FALSE;
	}
#else
	if (!mUsage)
	{
		return FALSE;
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
void LLVertexBuffer::freeClientBuffer()
{
	if(useVBOs() && sDisableVBOMapping && (mMappedData || mMappedIndexData))
	{
		free(mMappedData) ;
		free(mMappedIndexData) ;
		mMappedData = NULL ;
		mMappedIndexData = NULL ;
	}
}

void LLVertexBuffer::allocateClientVertexBuffer()
{
	if(!mMappedData)
	{
		mMappedData = (U8*)malloc(getSize());
	}
}

void LLVertexBuffer::allocateClientIndexBuffer()
{
	if(!mMappedIndexData)
	{
		mMappedIndexData = (U8*)malloc(getIndicesSize());
	}
}

// Map for data access
U8* LLVertexBuffer::mapVertexBuffer(S32 type, S32 access)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_MAP_BUFFER);
	if (mFinal)
	{
		llerrs << "LLVertexBuffer::mapVeretxBuffer() called on a finalized buffer." << llendl;
	}
	if (!useVBOs() && !mMappedData && !mMappedIndexData)
	{
		llerrs << "LLVertexBuffer::mapVertexBuffer() called on unallocated buffer." << llendl;
	}
		
	if (!mVertexLocked && useVBOs())
	{
		{
			LLMemType mt_v(LLMemType::MTYPE_VERTEX_MAP_BUFFER_VERTICES);
			setBuffer(0, type);
			mVertexLocked = TRUE;
			stop_glerror();	

			if(sDisableVBOMapping)
			{
				allocateClientVertexBuffer() ;
			}
			else
			{
				U8* src = (U8*) glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
				mMappedData = LL_NEXT_ALIGNED_ADDRESS<U8>(src);
				mAlignedOffset = mMappedData - src;
			
				stop_glerror();
			}
		}
		
		
		if (!mMappedData)
		{
			log_glerror();

			//check the availability of memory
			U32 avail_phy_mem, avail_vir_mem;
			LLMemoryInfo::getAvailableMemoryKB(avail_phy_mem, avail_vir_mem) ;
			llinfos << "Available physical mwmory(KB): " << avail_phy_mem << llendl ; 
			llinfos << "Available virtual memory(KB): " << avail_vir_mem << llendl;

			if(!sDisableVBOMapping)
			{			
				//--------------------
				//print out more debug info before crash
				llinfos << "vertex buffer size: (num verts : num indices) = " << getNumVerts() << " : " << getNumIndices() << llendl ;
				GLint size ;
				glGetBufferParameterivARB(GL_ARRAY_BUFFER_ARB, GL_BUFFER_SIZE_ARB, &size) ;
				llinfos << "GL_ARRAY_BUFFER_ARB size is " << size << llendl ;
				//--------------------

				GLint buff;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &buff);
				if ((GLuint)buff != mGLBuffer)
				{
					llerrs << "Invalid GL vertex buffer bound: " << buff << llendl;
				}

							
				llerrs << "glMapBuffer returned NULL (no vertex data)" << llendl;
			}
			else
			{
				llerrs << "memory allocation for vertex data failed." << llendl ;
			}
		}
		sMappedCount++;
	}
	
	return mMappedData;
}

U8* LLVertexBuffer::mapIndexBuffer(S32 access)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_MAP_BUFFER);
	if (mFinal)
	{
		llerrs << "LLVertexBuffer::mapIndexBuffer() called on a finalized buffer." << llendl;
	}
	if (!useVBOs() && !mMappedData && !mMappedIndexData)
	{
		llerrs << "LLVertexBuffer::mapIndexBuffer() called on unallocated buffer." << llendl;
	}

	if (!mIndexLocked && useVBOs())
	{
		{
			LLMemType mt_v(LLMemType::MTYPE_VERTEX_MAP_BUFFER_INDICES);

			setBuffer(0, TYPE_INDEX);
			mIndexLocked = TRUE;
			stop_glerror();	

			if(sDisableVBOMapping)
			{
				allocateClientIndexBuffer() ;
			}
			else
			{
				U8* src = (U8*) glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
				mMappedIndexData = LL_NEXT_ALIGNED_ADDRESS<U8>(src);
				mAlignedIndexOffset = mMappedIndexData - src;
				stop_glerror();
			}
		}

		if (!mMappedIndexData)
		{
			log_glerror();

			if(!sDisableVBOMapping)
			{
				GLint buff;
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &buff);
				if ((GLuint)buff != mGLIndices)
				{
					llerrs << "Invalid GL index buffer bound: " << buff << llendl;
				}

				llerrs << "glMapBuffer returned NULL (no index data)" << llendl;
			}
			else
			{
				llerrs << "memory allocation for Index data failed. " << llendl ;
			}
		}

		sMappedCount++;
	}

	return mMappedIndexData ;
}

void LLVertexBuffer::unmapBuffer(S32 type)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_UNMAP_BUFFER);
	if (!useVBOs())
	{
		return ; //nothing to unmap
	}

	bool updated_all = false ;
	if (mMappedData && mVertexLocked && type != TYPE_INDEX)
	{
		updated_all = (mIndexLocked && type < 0) ; //both vertex and index buffers done updating

		if(sDisableVBOMapping)
		{
			stop_glerror();
			glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, getSize(), mMappedData);
			stop_glerror();
		}
		else
		{
			stop_glerror();
			glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			stop_glerror();

			mMappedData = NULL;
		}

		mVertexLocked = FALSE ;
		sMappedCount--;
	}

	if(mMappedIndexData && mIndexLocked && (type < 0 || type == TYPE_INDEX))
	{
		if(sDisableVBOMapping)
		{
			stop_glerror();
			glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, getIndicesSize(), mMappedIndexData);
			stop_glerror();
		}
		else
		{
			stop_glerror();
			glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
			stop_glerror();

			mMappedIndexData = NULL ;
		}

		mIndexLocked = FALSE ;
		sMappedCount--;
	}

	if(updated_all)
	{
		if(mUsage == GL_STATIC_DRAW_ARB)
		{
			//static draw buffers can only be mapped a single time
			//throw out client data (we won't be using it again)
			mEmpty = TRUE;
			mFinal = TRUE;
			if(sDisableVBOMapping)
			{
				freeClientBuffer() ;
			}
		}
		else
		{
			mEmpty = FALSE;
		}
	}
}

//----------------------------------------------------------------------------

template <class T,S32 type> struct VertexBufferStrider
{
	typedef LLStrider<T> strider_t;
	static bool get(LLVertexBuffer& vbo, 
					strider_t& strider, 
					S32 index)
	{
		if (type == LLVertexBuffer::TYPE_INDEX)
		{
			S32 stride = sizeof(T);

			if (vbo.mapIndexBuffer() == NULL)
			{
				llwarns << "mapIndexBuffer failed!" << llendl;
				return FALSE;
			}

			strider = (T*)(vbo.getMappedIndices() + index*stride);
			strider.setStride(0);
			return TRUE;
		}
		else if (vbo.hasDataType(type))
		{
			S32 stride = LLVertexBuffer::sTypeSize[type];

			if (vbo.mapVertexBuffer(type) == NULL)
			{
				llwarns << "mapVertexBuffer failed!" << llendl;
				return FALSE;
			}

			strider = (T*)(vbo.getMappedData() + vbo.getOffset(type)+index*stride);
			strider.setStride(stride);
			return TRUE;
		}
		else
		{
			llerrs << "VertexBufferStrider could not find valid vertex data." << llendl;
		}
		return FALSE;
	}
};

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector3>& strider, S32 index)
{
	return VertexBufferStrider<LLVector3,TYPE_VERTEX>::get(*this, strider, index);
}
bool LLVertexBuffer::getIndexStrider(LLStrider<U16>& strider, S32 index)
{
	return VertexBufferStrider<U16,TYPE_INDEX>::get(*this, strider, index);
}
bool LLVertexBuffer::getTexCoord0Strider(LLStrider<LLVector2>& strider, S32 index)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD0>::get(*this, strider, index);
}
bool LLVertexBuffer::getTexCoord1Strider(LLStrider<LLVector2>& strider, S32 index)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD1>::get(*this, strider, index);
}
/*bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider, S32 index)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD2>::get(*this, strider, index);
}
bool LLVertexBuffer::getTexCoord3Strider(LLStrider<LLVector2>& strider, S32 index)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD3>::get(*this, strider, index);
}*/
bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector3>& strider, S32 index)
{
	return VertexBufferStrider<LLVector3,TYPE_NORMAL>::get(*this, strider, index);
}
bool LLVertexBuffer::getBinormalStrider(LLStrider<LLVector3>& strider, S32 index)
{
	return VertexBufferStrider<LLVector3,TYPE_BINORMAL>::get(*this, strider, index);
}
bool LLVertexBuffer::getColorStrider(LLStrider<LLColor4U>& strider, S32 index)
{
	return VertexBufferStrider<LLColor4U,TYPE_COLOR>::get(*this, strider, index);
}
bool LLVertexBuffer::getWeightStrider(LLStrider<F32>& strider, S32 index)
{
	return VertexBufferStrider<F32,TYPE_WEIGHT>::get(*this, strider, index);
}

bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4>& strider, S32 index)
{
	return VertexBufferStrider<LLVector4,TYPE_WEIGHT4>::get(*this, strider, index);
}

bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4>& strider, S32 index)
{
	return VertexBufferStrider<LLVector4,TYPE_CLOTHWEIGHT>::get(*this, strider, index);
}

//----------------------------------------------------------------------------

// Set for rendering
void LLVertexBuffer::setBuffer(U32 data_mask, S32 type)
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_SET_BUFFER);
	//set up pointers if the data mask is different ...
	BOOL setup = (sLastMask != data_mask);

	if (useVBOs())
	{
		if (mGLBuffer && (mGLBuffer != sGLRenderBuffer || !sVBOActive))
		{
			/*if (sMapped)
			{
				llerrs << "VBO bound while another VBO mapped!" << llendl;
			}*/
			stop_glerror();
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, mGLBuffer);
			stop_glerror();
			sBindCount++;
			sVBOActive = TRUE;
			setup = TRUE; // ... or the bound buffer changed
		}
		if (mGLIndices && (mGLIndices != sGLRenderIndices || !sIBOActive))
		{
			/*if (sMapped)
			{
				llerrs << "VBO bound while another VBO mapped!" << llendl;
			}*/
			stop_glerror();
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mGLIndices);
			stop_glerror();
			sBindCount++;
			sIBOActive = TRUE;
		}
		
		BOOL error = FALSE;
		if (gDebugGL)
		{
			GLint buff;
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &buff);
			if ((GLuint)buff != mGLBuffer)
			{
				if (gDebugSession)
				{
					error = TRUE;
					gFailLog << "Invalid GL vertex buffer bound: " << buff << std::endl;
				}
				else
				{
					llerrs << "Invalid GL vertex buffer bound: " << buff << llendl;
				}
			}

			if (mGLIndices)
			{
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &buff);
				if ((GLuint)buff != mGLIndices)
				{
					if (gDebugSession)
					{
						error = TRUE;
						gFailLog << "Invalid GL index buffer bound: " << buff <<  std::endl;
					}
					else
					{
						llerrs << "Invalid GL index buffer bound: " << buff << llendl;
					}
				}
			}
		}

		if (mResized)
		{
			if (gDebugGL)
			{
				GLint buff;
				glGetIntegerv(GL_ARRAY_BUFFER_BINDING_ARB, &buff);
				if ((GLuint)buff != mGLBuffer)
				{
					if (gDebugSession)
					{
						error = TRUE;
						gFailLog << "Invalid GL vertex buffer bound: " << std::endl;
					}
					else
					{
						llerrs << "Invalid GL vertex buffer bound: " << buff << llendl;
					}
				}

				if (mGLIndices != 0)
				{
					glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB, &buff);
					if ((GLuint)buff != mGLIndices)
					{
						if (gDebugSession)
						{
							error = TRUE;
							gFailLog << "Invalid GL index buffer bound: "<< std::endl;
						}
						else
						{
							llerrs << "Invalid GL index buffer bound: " << buff << llendl;
						}
					}
				}
			}

			if (mGLBuffer)
			{
				stop_glerror();
				glBufferDataARB(GL_ARRAY_BUFFER_ARB, getSize(), NULL, mUsage);
				stop_glerror();
			}
			if (mGLIndices)
			{
				stop_glerror();
				glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, getIndicesSize(), NULL, mUsage);
				stop_glerror();
			}

			mEmpty = TRUE;
			mResized = FALSE;

			if (data_mask != 0)
			{
				if (gDebugSession)
				{
					error = TRUE;
					gFailLog << "Buffer set for rendering before being filled after resize." << std::endl;
				}
				else
				{
					llerrs << "Buffer set for rendering before being filled after resize." << llendl;
				}
			}
		}

		if (error)
		{
			ll_fail("LLVertexBuffer::mapBuffer failed");
		}
		unmapBuffer(type);
	}
	else
	{		
		if (mGLBuffer)
		{
			if (sVBOActive)
			{
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
				sBindCount++;
				sVBOActive = FALSE;
				setup = TRUE; // ... or a VBO is deactivated
			}
			if (sGLRenderBuffer != mGLBuffer)
			{
				setup = TRUE; // ... or a client memory pointer changed
			}
		}
		if (mGLIndices && sIBOActive)
		{
			/*if (sMapped)
			{
				llerrs << "VBO unbound while potentially mapped!" << llendl;
			}*/
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			sBindCount++;
			sIBOActive = FALSE;
		}
	}

	setupClientArrays(data_mask);
	
	if (mGLIndices)
	{
		sGLRenderIndices = mGLIndices;
	}
	if (mGLBuffer)
	{
		sGLRenderBuffer = mGLBuffer;
		if (data_mask && setup)
		{
			setupVertexBuffer(data_mask); // subclass specific setup (virtual function)
			sSetCount++;
		}
	}
}

// virtual (default)
void LLVertexBuffer::setupVertexBuffer(U32 data_mask) const
{
	LLMemType mt2(LLMemType::MTYPE_VERTEX_SETUP_VERTEX_BUFFER);
	stop_glerror();
	U8* base = useVBOs() ? (U8*) mAlignedOffset : mMappedData;

	if ((data_mask & mTypeMask) != data_mask)
	{
		llerrs << "LLVertexBuffer::setupVertexBuffer missing required components for supplied data mask." << llendl;
	}

	if (data_mask & MAP_NORMAL)
	{
		glNormalPointer(GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_NORMAL], (void*)(base + mOffsets[TYPE_NORMAL]));
	}
	if (data_mask & MAP_TEXCOORD3)
	{
		glClientActiveTextureARB(GL_TEXTURE3_ARB);
		glTexCoordPointer(2,GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD3], (void*)(base + mOffsets[TYPE_TEXCOORD3]));
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	if (data_mask & MAP_TEXCOORD2)
	{
		glClientActiveTextureARB(GL_TEXTURE2_ARB);
		glTexCoordPointer(2,GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD2], (void*)(base + mOffsets[TYPE_TEXCOORD2]));
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	if (data_mask & MAP_TEXCOORD1)
	{
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2,GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD1], (void*)(base + mOffsets[TYPE_TEXCOORD1]));
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	if (data_mask & MAP_BINORMAL)
	{
		glClientActiveTextureARB(GL_TEXTURE2_ARB);
		glTexCoordPointer(3,GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_BINORMAL], (void*)(base + mOffsets[TYPE_BINORMAL]));
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
	}
	if (data_mask & MAP_TEXCOORD0)
	{
		glTexCoordPointer(2,GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD0], (void*)(base + mOffsets[TYPE_TEXCOORD0]));
	}
	if (data_mask & MAP_COLOR)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, LLVertexBuffer::sTypeSize[TYPE_COLOR], (void*)(base + mOffsets[TYPE_COLOR]));
	}
	
	if (data_mask & MAP_WEIGHT)
	{
		glVertexAttribPointerARB(1, 1, GL_FLOAT, FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT], (void*)(base + mOffsets[TYPE_WEIGHT]));
	}

	if (data_mask & MAP_WEIGHT4 && sWeight4Loc != -1)
	{
		glVertexAttribPointerARB(sWeight4Loc, 4, GL_FLOAT, FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT4], (void*)(base+mOffsets[TYPE_WEIGHT4]));
	}

	if (data_mask & MAP_CLOTHWEIGHT)
	{
		glVertexAttribPointerARB(4, 4, GL_FLOAT, TRUE,  LLVertexBuffer::sTypeSize[TYPE_CLOTHWEIGHT], (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]));
	}
	if (data_mask & MAP_VERTEX)
	{
		glVertexPointer(3,GL_FLOAT, LLVertexBuffer::sTypeSize[TYPE_VERTEX], (void*)(base + 0));
	}

	llglassertok();
}

void LLVertexBuffer::markDirty(U32 vert_index, U32 vert_count, U32 indices_index, U32 indices_count)
{
	// TODO: use GL_APPLE_flush_buffer_range here
	/*if (useVBOs() && !mFilthy)
	{
	
	}*/
}
