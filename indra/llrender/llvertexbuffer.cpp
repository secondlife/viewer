#include "linden_common.h"

#include "llvertexbuffer.h"
// #include "llrender.h"
#include "llglheaders.h"
#include "llmemory.h"
#include "llmemtype.h"

//============================================================================

//static
S32 LLVertexBuffer::sCount = 0;
S32 LLVertexBuffer::sGLCount = 0;
BOOL LLVertexBuffer::sEnableVBOs = TRUE;
S32 LLVertexBuffer::sGLRenderBuffer = 0;
S32 LLVertexBuffer::sGLRenderIndices = 0;
U32 LLVertexBuffer::sLastMask = 0;
BOOL LLVertexBuffer::sVBOActive = FALSE;
BOOL LLVertexBuffer::sIBOActive = FALSE;
U32 LLVertexBuffer::sAllocatedBytes = 0;
BOOL LLVertexBuffer::sRenderActive = FALSE;

std::vector<U32> LLVertexBuffer::sDeleteList;
LLVertexBuffer::buffer_list_t LLVertexBuffer::sLockedList;

S32 LLVertexBuffer::sTypeOffsets[LLVertexBuffer::TYPE_MAX] =
{
	sizeof(LLVector3), // TYPE_VERTEX,
	sizeof(LLVector3), // TYPE_NORMAL,
	sizeof(LLVector2), // TYPE_TEXCOORD,
	sizeof(LLVector2), // TYPE_TEXCOORD2,
	sizeof(LLColor4U), // TYPE_COLOR,
	sizeof(LLVector3), // TYPE_BINORMAL,
	sizeof(F32),	   // TYPE_WEIGHT,
	sizeof(LLVector4), // TYPE_CLOTHWEIGHT,
};

//static
void LLVertexBuffer::initClass(bool use_vbo)
{
	sEnableVBOs = use_vbo;
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
}

//static
void LLVertexBuffer::cleanupClass()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	sLockedList.clear();
	startRender(); 
	stopRender();
	clientCopy(); // deletes GL buffers
}

//static, call before rendering VBOs
void LLVertexBuffer::startRender()
{		
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (sEnableVBOs)
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		sVBOActive = FALSE;
		sIBOActive = FALSE;
	}
	
	sRenderActive = TRUE;
	sGLRenderBuffer = 0;
	sGLRenderIndices = 0;
	sLastMask = 0;
}

void LLVertexBuffer::stopRender()
{
	sRenderActive = FALSE;
}

void LLVertexBuffer::clientCopy()
{
	if (!sDeleteList.empty())
	{
		size_t num = sDeleteList.size();
		glDeleteBuffersARB(sDeleteList.size(), (GLuint*) &(sDeleteList[0]));
		sDeleteList.clear();
		sGLCount -= num;
	}

	if (sEnableVBOs)
	{
		LLTimer timer;
		BOOL reset = TRUE;
		buffer_list_t::iterator iter = sLockedList.begin();
		while(iter != sLockedList.end())
		{
			LLVertexBuffer* buffer = *iter;
			if (buffer->isLocked() && buffer->useVBOs())
			{
				buffer->setBuffer(0);
			}
			++iter;
			if (reset)
			{
				reset = FALSE;
				timer.reset(); //skip first copy (don't count pipeline stall)
			}
			else
			{
				if (timer.getElapsedTimeF64() > 0.005)
				{
					break;
				}
			}

		}

		sLockedList.erase(sLockedList.begin(), iter);
	}
}

//----------------------------------------------------------------------------

// For debugging
struct VTNC /// Simple
{
	F32 v1,v2,v3;
	F32 n1,n2,n3;
	F32 t1,t2;
	U32 c;
};
static VTNC dbg_vtnc;

struct VTUNCB // Simple + Bump
{
	F32 v1,v2,v3;
	F32 n1,n2,n3;
	F32 t1,t2;
	F32 u1,u2;
	F32 b1,b2,b3;
	U32 c;
};
static VTUNCB dbg_vtuncb;

struct VTUNC // Surfacepatch
{
	F32 v1,v2,v3;
	F32 n1,n2,n3;
	F32 t1,t2;
	F32 u1,u2;
	U32 c;
};
static VTUNC dbg_vtunc;

struct VTNW /// Avatar
{
	F32 v1,v2,v3;
	F32 n1,n2,n3;
	F32 t1,t2;
	F32 w;
};
static VTNW dbg_vtnw;

struct VTNPAD /// Avatar Output
{
	F32 v1,v2,v3,p1;
	F32 n1,n2,n3,p2;
	F32 t1,t2,p3,p4;
};
static VTNPAD dbg_vtnpad;

//----------------------------------------------------------------------------

LLVertexBuffer::LLVertexBuffer(U32 typemask, S32 usage) :
	LLRefCount(),
	mNumVerts(0), mNumIndices(0), mUsage(usage), mGLBuffer(0), mGLIndices(0), 
	mMappedData(NULL),
	mMappedIndexData(NULL), mLocked(FALSE),
	mResized(FALSE), mEmpty(TRUE), mFinal(FALSE), mFilthy(FALSE)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (!sEnableVBOs)
	{
		mUsage = GL_STREAM_DRAW_ARB;
	}
	
	S32 stride = 0;
	for (S32 i=0; i<TYPE_MAX; i++)
	{
		U32 mask = 1<<i;
		if (typemask & mask)
		{
			mOffsets[i] = stride;
			stride += sTypeOffsets[i];
		}
	}
	mTypeMask = typemask;
	mStride = stride;
	sCount++;
}

// protected, use unref()
//virtual
LLVertexBuffer::~LLVertexBuffer()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	destroyGLBuffer();
	destroyGLIndices();
	sCount--;
	
	if (mLocked)
	{
		//pull off of locked list
		for (buffer_list_t::iterator i = sLockedList.begin(); i != sLockedList.end(); ++i)
		{
			if (*i == this)
			{
				sLockedList.erase(i);
				break;
			}
		}
	}
};

//----------------------------------------------------------------------------

void LLVertexBuffer::createGLBuffer()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);

	U32 size = getSize();
	if (mGLBuffer)
	{
		destroyGLBuffer();
	}

	if (size == 0)
	{
		return;
	}

	mMappedData = new U8[size];
	memset(mMappedData, 0, size);
	mEmpty = TRUE;

	if (useVBOs())
	{
		glGenBuffersARB(1, (GLuint*) &mGLBuffer);
		mResized = TRUE;
		sGLCount++;
	}
	else
	{
		static int gl_buffer_idx = 0;
		mGLBuffer = ++gl_buffer_idx;
	}
}

void LLVertexBuffer::createGLIndices()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	U32 size = getIndicesSize();

	if (mGLIndices)
	{
		destroyGLIndices();
	}
	
	if (size == 0)
	{
		return;
	}

	mMappedIndexData = new U8[size];
	memset(mMappedIndexData, 0, size);
	mEmpty = TRUE;

	if (useVBOs())
	{
		glGenBuffersARB(1, (GLuint*) &mGLIndices);
		mResized = TRUE;
		sGLCount++;
	}
	else
	{
		static int gl_buffer_idx = 0;
		mGLIndices = ++gl_buffer_idx;
	}
}

void LLVertexBuffer::destroyGLBuffer()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (mGLBuffer)
	{
		if (useVBOs())
		{
			sDeleteList.push_back(mGLBuffer);
		}
		
		delete [] mMappedData;
		mMappedData = NULL;
		mEmpty = TRUE;
		sAllocatedBytes -= getSize();
	}
	
	mGLBuffer = 0;
}

void LLVertexBuffer::destroyGLIndices()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (mGLIndices)
	{
		if (useVBOs())
		{
			sDeleteList.push_back(mGLIndices);
		}
		
		delete [] mMappedIndexData;
		mMappedIndexData = NULL;
		mEmpty = TRUE;
		sAllocatedBytes -= getIndicesSize();
	}

	mGLIndices = 0;
}

void LLVertexBuffer::updateNumVerts(S32 nverts)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (!mDynamicSize)
	{
		mNumVerts = nverts;
	}
	else if (mUsage == GL_STATIC_DRAW_ARB ||
		nverts > mNumVerts ||
		nverts < mNumVerts/2)
	{
		if (mUsage != GL_STATIC_DRAW_ARB)
		{
			nverts += nverts/4;
		}

		mNumVerts = nverts;
	}
}

void LLVertexBuffer::updateNumIndices(S32 nindices)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
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

void LLVertexBuffer::makeStatic()
{
	if (!sEnableVBOs)
	{
		return;
	}
	
	if (sRenderActive)
	{
		llerrs << "Make static called during render." << llendl;
	}
	
	if (mUsage != GL_STATIC_DRAW_ARB)
	{
		if (useVBOs())
		{
			if (mGLBuffer)
			{
				sDeleteList.push_back(mGLBuffer);
			}
			if (mGLIndices)
			{
				sDeleteList.push_back(mGLIndices);
			}
		}
	
		if (mGLBuffer)
		{
			sGLCount++;
			glGenBuffersARB(1, (GLuint*) &mGLBuffer);
		}
		if (mGLIndices)
		{
			sGLCount++;
			glGenBuffersARB(1, (GLuint*) &mGLIndices);
		}
			
		mUsage = GL_STATIC_DRAW_ARB;
		mResized = TRUE;

		if (!mLocked)
		{
			mLocked = TRUE;
			sLockedList.push_back(this);
		}
	}
}

void LLVertexBuffer::allocateBuffer(S32 nverts, S32 nindices, bool create)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
		
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
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
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
		
		S32 oldsize = getSize();
		S32 old_index_size = getIndicesSize();

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
				//delete old buffer, keep GL buffer for now
				U8* old = mMappedData;
				mMappedData = new U8[newsize];
				if (old)
				{	
					memcpy(mMappedData, old, llmin(newsize, oldsize));
					if (newsize > oldsize)
					{
						memset(mMappedData+oldsize, 0, newsize-oldsize);
					}

					delete [] old;
				}
				else
				{
					memset(mMappedData, 0, newsize);
					mEmpty = TRUE;
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
				//delete old buffer, keep GL buffer for now
				U8* old = mMappedIndexData;
				mMappedIndexData = new U8[new_index_size];
				if (old)
				{	
					memcpy(mMappedIndexData, old, llmin(new_index_size, old_index_size));
					if (new_index_size > old_index_size)
					{
						memset(mMappedIndexData+old_index_size, 0, new_index_size - old_index_size);
					}
					delete [] old;
				}
				else
				{
					memset(mMappedIndexData, 0, new_index_size);
					mEmpty = TRUE;
				}
				mResized = TRUE;
			}
		}
		else if (mGLIndices)
		{
			destroyGLIndices();
		}
	}
}

BOOL LLVertexBuffer::useVBOs() const
{
	//it's generally ineffective to use VBO for things that are streaming
	//when we already have a client buffer around
	if (mUsage == GL_STREAM_DRAW_ARB)
	{
		return FALSE;
	}

	return sEnableVBOs && (!sRenderActive || !mLocked);
}

//----------------------------------------------------------------------------

// Map for data access
U8* LLVertexBuffer::mapBuffer(S32 access)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (sRenderActive)
	{
		llwarns << "Buffer mapped during render frame!" << llendl;
	}
	if (!mGLBuffer && !mGLIndices)
	{
		llerrs << "LLVertexBuffer::mapBuffer() called  before createGLBuffer" << llendl;
	}
	if (mFinal)
	{
		llerrs << "LLVertexBuffer::mapBuffer() called on a finalized buffer." << llendl;
	}
	if (!mMappedData && !mMappedIndexData)
	{
		llerrs << "LLVertexBuffer::mapBuffer() called on unallocated buffer." << llendl;
	}
	
	if (!mLocked && useVBOs())
	{
		mLocked = TRUE;
		sLockedList.push_back(this);
	}
	
	return mMappedData;
}

void LLVertexBuffer::unmapBuffer()
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (mMappedData || mMappedIndexData)
	{
		if (useVBOs() && mLocked)
		{
			if (mGLBuffer)
			{
				if (mResized)
				{
					glBufferDataARB(GL_ARRAY_BUFFER_ARB, getSize(), mMappedData, mUsage);
				}
				else
				{
					if (mEmpty || mDirtyRegions.empty())
					{
						glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, 0, getSize(), mMappedData);
					}
					else
					{
						for (std::vector<DirtyRegion>::iterator i = mDirtyRegions.begin(); i != mDirtyRegions.end(); ++i)
						{
							DirtyRegion& region = *i;
							glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, region.mIndex*mStride, region.mCount*mStride, mMappedData + region.mIndex*mStride);
							glFlush();
						}
					}
				}
			}
			
			if (mGLIndices)
			{
				if (mResized)
				{
					glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, getIndicesSize(), mMappedIndexData, mUsage);
				}
				else
				{
					if (mEmpty || mDirtyRegions.empty())
					{
						glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0, getIndicesSize(), mMappedIndexData);
					}
					else
					{
						for (std::vector<DirtyRegion>::iterator i = mDirtyRegions.begin(); i != mDirtyRegions.end(); ++i)
						{
							DirtyRegion& region = *i;
							glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, region.mIndicesIndex*sizeof(U32), 
								region.mIndicesCount*sizeof(U32), mMappedIndexData + region.mIndicesIndex*sizeof(U32));
							glFlush();
						}
					}
				}
			}

			mDirtyRegions.clear();
			mFilthy = FALSE;
			mResized = FALSE;

			if (mUsage == GL_STATIC_DRAW_ARB)
			{ //static draw buffers can only be mapped a single time
				//throw out client data (we won't be using it again)
				delete [] mMappedData;
				delete [] mMappedIndexData;
				mMappedIndexData = NULL;
				mMappedData = NULL;
				mEmpty = TRUE;
				mFinal = TRUE;
			}
			else
			{
				mEmpty = FALSE;
			}
			
			mLocked = FALSE;
			
			glFlush();
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
		vbo.mapBuffer();
		if (type == LLVertexBuffer::TYPE_INDEX)
		{
			S32 stride = sizeof(T);
			strider = (T*)(vbo.getMappedIndices() + index*stride);
			strider.setStride(0);
			return TRUE;
		}
		else if (vbo.hasDataType(type))
		{
			S32 stride = vbo.getStride();
			strider = (T*)(vbo.getMappedData() + vbo.getOffset(type) + index*stride);
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
bool LLVertexBuffer::getIndexStrider(LLStrider<U32>& strider, S32 index)
{
	return VertexBufferStrider<U32,TYPE_INDEX>::get(*this, strider, index);
}
bool LLVertexBuffer::getTexCoordStrider(LLStrider<LLVector2>& strider, S32 index)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD>::get(*this, strider, index);
}
bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider, S32 index)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD2>::get(*this, strider, index);
}
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
bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4>& strider, S32 index)
{
	return VertexBufferStrider<LLVector4,TYPE_CLOTHWEIGHT>::get(*this, strider, index);
}

void LLVertexBuffer::setStride(S32 type, S32 new_stride)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	if (mNumVerts)
	{
		llerrs << "LLVertexBuffer::setOffset called with mNumVerts = " << mNumVerts << llendl;
	}
	// This code assumes that setStride() will only be called once per VBO per type.
	S32 delta = new_stride - sTypeOffsets[type];
	for (S32 i=type+1; i<TYPE_MAX; i++)
	{
		if (mTypeMask & (1<<i))
		{
			mOffsets[i] += delta;
		}
	}
	mStride += delta;
}

//----------------------------------------------------------------------------

// Set for rendering
void LLVertexBuffer::setBuffer(U32 data_mask)
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	//set up pointers if the data mask is different ...
	BOOL setup = (sLastMask != data_mask);

	if (useVBOs())
	{
		if (mGLBuffer && (mGLBuffer != sGLRenderBuffer || !sVBOActive))
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, mGLBuffer);
			sVBOActive = TRUE;
			setup = TRUE; // ... or the bound buffer changed
		}
		if (mGLIndices && (mGLIndices != sGLRenderIndices || !sIBOActive))
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, mGLIndices);
			sIBOActive = TRUE;
		}
		
		unmapBuffer();
	}
	else
	{
		if (!mDirtyRegions.empty())
		{
			mFilthy = TRUE;
			mDirtyRegions.clear();
		}
		
		if (mGLBuffer)
		{
			if (sEnableVBOs && sVBOActive)
			{
				glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
				sVBOActive = FALSE;
				setup = TRUE; // ... or a VBO is deactivated
			}
			if (sGLRenderBuffer != mGLBuffer)
			{
				setup = TRUE; // ... or a client memory pointer changed
			}
		}
		if (sEnableVBOs && mGLIndices && sIBOActive)
		{
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
			sIBOActive = FALSE;
		}
	}
	
	if (mGLIndices)
	{
		sGLRenderIndices = mGLIndices;
	}
	if (mGLBuffer)
	{
		sGLRenderBuffer = mGLBuffer;
		if (data_mask && setup)
		{
			if (!sRenderActive)
			{
				llwarns << "Vertex buffer set for rendering outside of render frame." << llendl;
			}
			setupVertexBuffer(data_mask); // subclass specific setup (virtual function)
			sLastMask = data_mask;
		}
	}
}

// virtual (default)
void LLVertexBuffer::setupVertexBuffer(U32 data_mask) const
{
	LLMemType mt(LLMemType::MTYPE_VERTEX_DATA);
	stop_glerror();
	U8* base = useVBOs() ? NULL : mMappedData;
	S32 stride = mStride;

	if ((data_mask & mTypeMask) != data_mask)
	{
		llerrs << "LLVertexBuffer::setupVertexBuffer missing required components for supplied data mask." << llendl;
	}

	if (data_mask & MAP_VERTEX)
	{
		glVertexPointer(3,GL_FLOAT, stride, (void*)(base + 0));
	}
	if (data_mask & MAP_NORMAL)
	{
		glNormalPointer(GL_FLOAT, stride, (void*)(base + mOffsets[TYPE_NORMAL]));
	}
	if (data_mask & MAP_TEXCOORD2)
	{
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(2,GL_FLOAT, stride, (void*)(base + mOffsets[TYPE_TEXCOORD2]));
	}
	if (data_mask & MAP_TEXCOORD)
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(2,GL_FLOAT, stride, (void*)(base + mOffsets[TYPE_TEXCOORD]));
	}
	if (data_mask & MAP_COLOR)
	{
		glColorPointer(4, GL_UNSIGNED_BYTE, stride, (void*)(base + mOffsets[TYPE_COLOR]));
	}
	if (data_mask & MAP_BINORMAL)
	{
		glVertexAttribPointerARB(6, 3, GL_FLOAT, FALSE,  stride, (void*)(base + mOffsets[TYPE_BINORMAL]));
	}
	if (data_mask & MAP_WEIGHT)
	{
		glVertexAttribPointerARB(1, 1, GL_FLOAT, FALSE, stride, (void*)(base + mOffsets[TYPE_WEIGHT]));
	}
	if (data_mask & MAP_CLOTHWEIGHT)
	{
		glVertexAttribPointerARB(4, 4, GL_FLOAT, TRUE,  stride, (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]));
	}

	llglassertok();
}

void LLVertexBuffer::markDirty(U32 vert_index, U32 vert_count, U32 indices_index, U32 indices_count)
{
	if (useVBOs() && !mFilthy)
	{
		if (!mDirtyRegions.empty())
		{
			DirtyRegion& region = *(mDirtyRegions.rbegin());
			if (region.mIndex + region.mCount == vert_index &&
				region.mIndicesIndex + region.mIndicesCount == indices_index)
			{
				region.mCount += vert_count;
				region.mIndicesCount += indices_count;
				return;
			}
		}

		mDirtyRegions.push_back(DirtyRegion(vert_index,vert_count,indices_index,indices_count));
	}
}

void LLVertexBuffer::markClean()
{
	if (!mResized && !mEmpty && !mFilthy)
	{
		buffer_list_t::reverse_iterator iter = sLockedList.rbegin();
		if (*iter == this)
		{
			mLocked = FALSE;
			sLockedList.pop_back();
		}
	}
}

