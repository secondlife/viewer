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

struct CompareMappedRegion
{
    bool operator()(const LLVertexBuffer::MappedRegion& lhs, const LLVertexBuffer::MappedRegion& rhs)
    {
        return lhs.mStart < rhs.mStart;
    }
};


const U32 LL_VBO_BLOCK_SIZE = 2048;
const U32 LL_VBO_POOL_MAX_SEED_SIZE = 256*1024;

U32 vbo_block_size(U32 size)
{ //what block size will fit size?
	U32 mod = size % LL_VBO_BLOCK_SIZE;
	return mod == 0 ? size : size + (LL_VBO_BLOCK_SIZE-mod);
}

U32 vbo_block_index(U32 size)
{
    U32 blocks = vbo_block_size(size)/LL_VBO_BLOCK_SIZE;   // block count reqd
    llassert(blocks > 0);
    return blocks - 1;  // Adj index, i.e. single-block allocations are at index 0, etc
}

const U32 LL_VBO_POOL_SEED_COUNT = vbo_block_index(LL_VBO_POOL_MAX_SEED_SIZE) + 1;

#define ENABLE_GL_WORK_QUEUE 0

#if ENABLE_GL_WORK_QUEUE

#define THREAD_COUNT 1

//============================================================================

// High performance WorkQueue for usage in real-time rendering work
class GLWorkQueue
{
public:
    using Work = std::function<void()>;

    GLWorkQueue();

    void post(const Work& value);

    size_t size();

    bool done();

    // Get the next element from the queue
    Work pop();

    void runOne();

    bool runPending();

    void runUntilClose();

    void close();

    bool isClosed();

    void syncGL();

private:
    std::mutex mMutex;
    std::condition_variable mCondition;
    std::queue<Work> mQueue;
    bool mClosed = false;
};

GLWorkQueue::GLWorkQueue()
{

}

void GLWorkQueue::syncGL()
{
    /*if (mSync)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        glWaitSync(mSync, 0, GL_TIMEOUT_IGNORED);
        mSync = 0;
    }*/
}

size_t GLWorkQueue::size()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.size();
}

bool GLWorkQueue::done()
{
    return size() == 0 && isClosed();
}

void GLWorkQueue::post(const GLWorkQueue::Work& value)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mQueue.push(std::move(value));
    }

    mCondition.notify_one();
}

// Get the next element from the queue
GLWorkQueue::Work GLWorkQueue::pop()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    // Lock the mutex
    {
        std::unique_lock<std::mutex> lock(mMutex);

        // Wait for a new element to become available or for the queue to close
        {
            mCondition.wait(lock, [=] { return !mQueue.empty() || mClosed; });
        }
    }

    Work ret;

    {
        std::lock_guard<std::mutex> lock(mMutex);

        // Get the next element from the queue
        if (mQueue.size() > 0)
        {
            ret = mQueue.front();
            mQueue.pop();
        }
        else
        {
            ret = []() {};
        }
    }

    return ret;
}

void GLWorkQueue::runOne()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    Work w = pop();
    w();
    //mSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void GLWorkQueue::runUntilClose()
{
    while (!isClosed())
    {
        runOne();
    }
}

void GLWorkQueue::close()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mClosed = true;
    }

    mCondition.notify_all();
}

bool GLWorkQueue::isClosed()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_THREAD;
    std::lock_guard<std::mutex> lock(mMutex);
    return mClosed;
}

#include "llwindow.h"

class LLGLWorkerThread : public LLThread
{
public:
    LLGLWorkerThread(const std::string& name, GLWorkQueue* queue, LLWindow* window)
        : LLThread(name)
    {
        mWindow = window;
        mContext = mWindow->createSharedContext();
        mQueue = queue;
    }

    void run() override
    {
        mWindow->makeContextCurrent(mContext);
        gGL.init(false);
        mQueue->runUntilClose();
        gGL.shutdown();
        mWindow->destroySharedContext(mContext);
    }

    GLWorkQueue* mQueue;
    LLWindow* mWindow;
    void* mContext = nullptr;
};


static LLGLWorkerThread* sVBOThread[THREAD_COUNT];
static GLWorkQueue* sQueue = nullptr;

#endif

//============================================================================
// Pool of reusable VertexBuffer state

// batch calls to glGenBuffers
static GLuint gen_buffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    constexpr U32 pool_size = 4096;

    thread_local static GLuint sNamePool[pool_size];
    thread_local static U32 sIndex = 0;

    if (sIndex == 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("gen buffer");
        sIndex = pool_size;
        glGenBuffers(pool_size, sNamePool);
    }

    return sNamePool[--sIndex];
}

class LLVBOPool
{
public:
    typedef std::chrono::steady_clock::time_point Time;

    struct Entry
    {
        U8* mData;
        GLuint mGLName;
        Time mAge;
    };

    ~LLVBOPool()
    {
        clear();
    }

    typedef std::unordered_map<U32, std::list<Entry>> Pool;

    Pool mVBOPool;
    Pool mIBOPool;

    U32 mMissCount = 0;

    void allocate(GLenum type, U32 size, GLuint& name, U8*& data)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

        size = nhpo2(size);

        auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

        auto& iter = pool.find(size);
        if (iter == pool.end())
        { // cache miss, allocate a new buffer
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vbo pool miss");
            LL_PROFILE_GPU_ZONE("vbo alloc");

            ++mMissCount;
            if (mMissCount > 1024)
            { //clean cache on every 1024 misses
                mMissCount = 0;
                clean();
            }

            name = gen_buffer();
            glBindBuffer(type, name);
            glBufferData(type, size, nullptr, GL_DYNAMIC_DRAW);
            if (type == GL_ELEMENT_ARRAY_BUFFER)
            {
                LLVertexBuffer::sGLRenderIndices = name;
            }
            else
            {
                LLVertexBuffer::sGLRenderBuffer = name;
            }

            data = (U8*)ll_aligned_malloc_16(size);
        }
        else
        {
            std::list<Entry>& entries = iter->second;
            Entry& entry = entries.back();
            name = entry.mGLName;
            data = entry.mData;

            entries.pop_back();
            if (entries.empty())
            {
                pool.erase(iter);
            }
        }
    }

    void free(GLenum type, U32 size, GLuint name, U8* data)
    {
        size = nhpo2(size);

        auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

        auto& iter = pool.find(size);

        if (iter == pool.end())
        {
            std::list<Entry> newlist;
            newlist.push_front({ data, name, std::chrono::steady_clock::now() });
            pool[size] = newlist;
        }
        else
        {
            iter->second.push_front({ data, name, std::chrono::steady_clock::now() });
        }
    }

    void clean()
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

        std::unordered_map<U32, std::list<Entry>>* pools[] = { &mVBOPool, &mIBOPool };

        using namespace std::chrono_literals;

        Time cutoff = std::chrono::steady_clock::now() - 5s;

        for (auto* pool : pools)
        {
            for (Pool::iterator iter = pool->begin(); iter != pool->end(); )
            {
                auto& entries = iter->second;

                while (!entries.empty() && entries.back().mAge < cutoff)
                {
                    LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vbo cache timeout");
                    auto& entry = entries.back();
                    ll_aligned_free_16(entry.mData);
                    glDeleteBuffers(1, &entry.mGLName);
                    entries.pop_back();
                }

                if (entries.empty())
                {
                    iter = pool->erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }

    void clear()
    {
        for (auto& entries : mIBOPool)
        {
            for (auto& entry : entries.second)
            {
                ll_aligned_free_16(entry.mData);
                glDeleteBuffers(1, &entry.mGLName);
            }
        }

        for (auto& entries : mVBOPool)
        {
            for (auto& entry : entries.second)
            {
                ll_aligned_free_16(entry.mData);
                glDeleteBuffers(1, &entry.mGLName);
            }
        }

        mIBOPool.clear();
        mVBOPool.clear();
    }


};

static LLVBOPool* sVBOPool = nullptr;

//============================================================================
// 
//static
U32 LLVertexBuffer::sGLRenderBuffer = 0;
U32 LLVertexBuffer::sGLRenderIndices = 0;
U32 LLVertexBuffer::sLastMask = 0;
U32 LLVertexBuffer::sVertexCount = 0;


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
void LLVertexBuffer::setupClientArrays(U32 data_mask)
{
    if (sLastMask != data_mask)
    {
        for (U32 i = 0; i < TYPE_MAX; ++i)
        {
            S32 loc = i;

            U32 mask = 1 << i;

            if (sLastMask & (1 << i))
            { //was enabled
                if (!(data_mask & mask))
                { //needs to be disabled
                    glDisableVertexAttribArray(loc);
                }
            }
            else
            {	//was disabled
                if (data_mask & mask)
                { //needs to be enabled
                    glEnableVertexAttribArray(loc);
                }
            }
        }
    }

    sLastMask = data_mask;
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

bool LLVertexBuffer::validateRange(U32 start, U32 end, U32 count, U32 indices_offset) const
{
    if (!gDebugGL)
    {
        return true;
    }

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

	{
		U16* idx = (U16*) mMappedIndexData+indices_offset;
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
			LLVector4a* v = (LLVector4a*) mMappedData;
			
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

    return true;
}

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
void LLVertexBuffer::setLabel(const char* label) {
	LL_LABEL_OBJECT_GL(GL_BUFFER, mGLBuffer, strlen(label), label);
}
#endif

void LLVertexBuffer::drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const
{
    llassert(validateRange(start, end, count, indices_offset));
    llassert(mGLBuffer == sGLRenderBuffer);
    llassert(mGLIndices == sGLRenderIndices);
    gGL.syncMatrices();

    glDrawRangeElements(sGLMode[mode], start, end, count, GL_UNSIGNED_SHORT,
        (GLvoid*) (indices_offset * sizeof(U16)));
}

void LLVertexBuffer::draw(U32 mode, U32 count, U32 indices_offset) const
{
    drawRange(mode, 0, mNumVerts, count, indices_offset);
}


void LLVertexBuffer::drawArrays(U32 mode, U32 first, U32 count) const
{
    llassert(first + count <= mNumVerts);
    llassert(mGLBuffer == sGLRenderBuffer);
    llassert(mGLIndices == sGLRenderIndices);
    
    gGL.syncMatrices();
    glDrawArrays(sGLMode[mode], first, count);
}

//static
void LLVertexBuffer::initClass(LLWindow* window)
{
    sVBOPool = new LLVBOPool();

#if ENABLE_GL_WORK_QUEUE
    sQueue = new GLWorkQueue();

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        sVBOThread[i] = new LLGLWorkerThread("VBO Worker", sQueue, window);
        sVBOThread[i]->start();
    }
#endif
}

//static 
void LLVertexBuffer::unbind()
{
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	sGLRenderBuffer = 0;
	sGLRenderIndices = 0;
}

//static
void LLVertexBuffer::cleanupClass()
{
	unbind();
	
    delete sVBOPool;
    sVBOPool = nullptr;

#if ENABLE_GL_WORK_QUEUE
    sQueue->close();
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        sVBOThread[i]->shutdown();
        delete sVBOThread[i];
        sVBOThread[i] = nullptr;
    }

    delete sQueue;
    sQueue = nullptr;
#endif
}

//----------------------------------------------------------------------------

LLVertexBuffer::LLVertexBuffer(U32 typemask) 
:	LLRefCount(),

	mNumVerts(0),
	mNumIndices(0),
	mSize(0),
	mIndicesSize(0),
	mTypeMask(typemask),
	mGLBuffer(0),
	mGLIndices(0),
	mMappedData(NULL),
	mMappedIndexData(NULL)
{
	//zero out offsets
	for (U32 i = 0; i < TYPE_MAX; i++)
	{
		mOffsets[i] = 0;
	}
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
	
	return offset;
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

	if (mMappedData)
	{
		LL_ERRS() << "Failed to clear vertex buffer's vertices" << LL_ENDL;
	}
	if (mMappedIndexData)
	{
		LL_ERRS() << "Failed to clear vertex buffer's indices" << LL_ENDL;
	}
};

//----------------------------------------------------------------------------

void LLVertexBuffer::genBuffer(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

    mSize = size;

    if (sVBOPool)
    {
        sVBOPool->allocate(GL_ARRAY_BUFFER, size, mGLBuffer, mMappedData);
    }
}

void LLVertexBuffer::genIndices(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

    mIndicesSize = size;

    if (sVBOPool)
    {
        sVBOPool->allocate(GL_ELEMENT_ARRAY_BUFFER, size, mGLIndices, mMappedIndexData);
    }
}

void LLVertexBuffer::releaseBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

    if (sVBOPool)
    {
        sVBOPool->free(GL_ARRAY_BUFFER, mSize, mGLBuffer, mMappedData);
    }

    mGLBuffer = 0;
    mMappedData = nullptr;
}

void LLVertexBuffer::releaseIndices()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    
    if (sVBOPool)
    {
        sVBOPool->free(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mGLIndices, mMappedIndexData);
    }

    mMappedIndexData = nullptr;
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

	genBuffer(size);
	
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

	//pad by 16 bytes for aligned copies
	size += 16;

	genIndices(size);

	
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
		releaseBuffer();
	}
	
	mGLBuffer = 0;
}

void LLVertexBuffer::destroyGLIndices()
{
	if (mGLIndices || mMappedIndexData)
	{
		releaseIndices();
	}

	mGLIndices = 0;
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

	mNumVerts = nverts;
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

	mNumIndices = nindices;
	return success;
}

bool LLVertexBuffer::allocateBuffer(S32 nverts, S32 nindices)
{
	if (nverts < 0 || nindices < 0 ||
		nverts > 65536)
	{
		LL_ERRS() << "Bad vertex buffer allocation: " << nverts << " : " << nindices << LL_ENDL;
	}

	bool success = true;

	success &= updateNumVerts(nverts);
	success &= updateNumIndices(nindices);
	
	return success;
}

//----------------------------------------------------------------------------

// if no gap between region and given range exists, expand region to cover given range and return true
// otherwise return false
bool expand_region(LLVertexBuffer::MappedRegion& region, S32 start, S32 end)
{
	
	if (end < region.mStart ||
		start > region.mEnd)
	{ //gap exists, do not merge
		return false;
	}

    region.mStart = llmin(region.mStart, start);
    region.mEnd = llmax(region.mEnd, end);

	return true;
}


// Map for data access
U8* LLVertexBuffer::mapVertexBuffer(S32 type, S32 index, S32 count)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	
    if (count == -1)
    {
        count = mNumVerts - index;
    }

    S32 start = mOffsets[type] + sTypeSize[type] * index;
    S32 end = start + sTypeSize[type] * count;

	bool flagged = false;
	// flag region as mapped
	for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
	{
		MappedRegion& region = mMappedVertexRegions[i];
        if (expand_region(region, start, end))
        {
            flagged = true;
            break;
        }
	}

	if (!flagged)
	{
		//didn't expand an existing region, make a new one
        mMappedVertexRegions.push_back({ start, end });
	}
	
    return mMappedData+mOffsets[type]+sTypeSize[type]*index;
}


U8* LLVertexBuffer::mapIndexBuffer(S32 index, S32 count)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	
	if (count == -1)
	{
		count = mNumIndices-index;
	}

    S32 start = sizeof(U16) * index;
    S32 end = start + sizeof(U16) * count;

    bool flagged = false;
    // flag region as mapped
    for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
    {
        MappedRegion& region = mMappedIndexRegions[i];
        if (expand_region(region, start, end))
        {
            flagged = true;
            break;
        }
    }

    if (!flagged)
    {
        //didn't expand an existing region, make a new one
        mMappedIndexRegions.push_back({ start, end });
    }

    return mMappedIndexData + sizeof(U16)*index;
}

static void flush_vbo(GLenum target, S32 start, S32 end, void* data)
{
    if (end != 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("glBufferSubData");
        LL_PROFILE_ZONE_NUM(start);
        LL_PROFILE_ZONE_NUM(end);
        LL_PROFILE_ZONE_NUM(end-start);

        constexpr S32 block_size = 65536;

        for (S32 i = start; i < end; i += block_size)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("glBufferSubData block");
            LL_PROFILE_GPU_ZONE("glBufferSubData");
            S32 tend = llmin(i + block_size, end);
            glBufferSubData(target, i, tend - i, (U8*) data + (i-start));
        }
    }
}

void LLVertexBuffer::unmapBuffer()
{
    {
		if (!mMappedVertexRegions.empty())
		{
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - vertex");
            if (sGLRenderBuffer != mGLBuffer)
            {
                glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
            }
            
            S32 start = 0;
            S32 end = 0;

			for (U32 i = 0; i < mMappedVertexRegions.size(); ++i)
			{
				const MappedRegion& region = mMappedVertexRegions[i];
                if (region.mStart == end + 1)
                {
                    end = region.mEnd;
                }
                else
                {
                    flush_vbo(GL_ARRAY_BUFFER, start, end, (U8*)mMappedData + start);
                    start = region.mStart;
                    end = region.mEnd;
                }
			}

            flush_vbo(GL_ARRAY_BUFFER, start, end, (U8*)mMappedData + start);

			mMappedVertexRegions.clear();
            if (mGLBuffer != sGLRenderBuffer)
            {
                glBindBuffer(GL_ARRAY_BUFFER, sGLRenderBuffer);
            }
		}
	}
	
	{
		if (!mMappedIndexRegions.empty())
		{
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - index");

            if (mGLIndices != sGLRenderIndices)
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
            }
            S32 start = 0;
            S32 end = 0;

            for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
            {
                const MappedRegion& region = mMappedIndexRegions[i];
                if (region.mStart == end + 1)
                {
                    end = region.mEnd;
                }
                else
                {
                    flush_vbo(GL_ELEMENT_ARRAY_BUFFER, start, end, (U8*)mMappedIndexData + start);
                    start = region.mStart;
                    end = region.mEnd;
                }
            }

            flush_vbo(GL_ELEMENT_ARRAY_BUFFER, start, end, (U8*)mMappedIndexData + start);

			mMappedIndexRegions.clear();

            if (mGLIndices != sGLRenderIndices)
            {
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sGLRenderIndices);
            }
		}
	}
}

//----------------------------------------------------------------------------

template <class T,S32 type> struct VertexBufferStrider
{
	typedef LLStrider<T> strider_t;
	static bool get(LLVertexBuffer& vbo, 
					strider_t& strider, 
					S32 index, S32 count)
	{
		if (type == LLVertexBuffer::TYPE_INDEX)
		{
			U8* ptr = vbo.mapIndexBuffer(index, count);

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

			U8* ptr = vbo.mapVertexBuffer(type, index, count);

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

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector3>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector3,TYPE_VERTEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector4a>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a,TYPE_VERTEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getIndexStrider(LLStrider<U16>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<U16,TYPE_INDEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord0Strider(LLStrider<LLVector2>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD0>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord1Strider(LLStrider<LLVector2>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD1>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD2>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector3>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector3,TYPE_NORMAL>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector3>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector3,TYPE_TANGENT>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector4a>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a,TYPE_TANGENT>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getColorStrider(LLStrider<LLColor4U>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLColor4U,TYPE_COLOR>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getEmissiveStrider(LLStrider<LLColor4U>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLColor4U,TYPE_EMISSIVE>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getWeightStrider(LLStrider<F32>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<F32,TYPE_WEIGHT>::get(*this, strider, index, count);
}

bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector4,TYPE_WEIGHT4>::get(*this, strider, index, count);
}

bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4>& strider, S32 index, S32 count)
{
	return VertexBufferStrider<LLVector4,TYPE_CLOTHWEIGHT>::get(*this, strider, index, count);
}

//----------------------------------------------------------------------------


// Set for rendering
void LLVertexBuffer::setBuffer()
{
    // no data may be pending
    llassert(mMappedVertexRegions.empty());
    llassert(mMappedIndexRegions.empty());

    // a shader must be bound
    llassert(LLGLSLShader::sCurBoundShaderPtr);

    U32 data_mask = LLGLSLShader::sCurBoundShaderPtr->mAttributeMask;

    // this Vertex Buffer must provide all necessary attributes for currently bound shader
    llassert(((~data_mask & mTypeMask) > 0) || (mTypeMask == data_mask));

    if (sGLRenderBuffer != mGLBuffer)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
        sGLRenderBuffer = mGLBuffer;

        setupVertexBuffer();
    }
    else if (sLastMask != data_mask)
    {
        setupVertexBuffer();
        sLastMask = data_mask;
    }
    
    if (mGLIndices != sGLRenderIndices)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
        sGLRenderIndices = mGLIndices;
    }
}


// virtual (default)
void LLVertexBuffer::setupVertexBuffer()
{
    U8* base = nullptr;

    U32 data_mask = LLGLSLShader::sCurBoundShaderPtr->mAttributeMask;

    if (data_mask & MAP_NORMAL)
    {
        S32 loc = TYPE_NORMAL;
        void* ptr = (void*)(base + mOffsets[TYPE_NORMAL]);
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_NORMAL], ptr);
    }
    if (data_mask & MAP_TEXCOORD3)
    {
        S32 loc = TYPE_TEXCOORD3;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD3]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD3], ptr);
    }
    if (data_mask & MAP_TEXCOORD2)
    {
        S32 loc = TYPE_TEXCOORD2;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD2]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD2], ptr);
    }
    if (data_mask & MAP_TEXCOORD1)
    {
        S32 loc = TYPE_TEXCOORD1;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD1]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD1], ptr);
    }
    if (data_mask & MAP_TANGENT)
    {
        S32 loc = TYPE_TANGENT;
        void* ptr = (void*)(base + mOffsets[TYPE_TANGENT]);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TANGENT], ptr);
    }
    if (data_mask & MAP_TEXCOORD0)
    {
        S32 loc = TYPE_TEXCOORD0;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD0]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD0], ptr);
    }
    if (data_mask & MAP_COLOR)
    {
        S32 loc = TYPE_COLOR;
        //bind emissive instead of color pointer if emissive is present
        void* ptr = (data_mask & MAP_EMISSIVE) ? (void*)(base + mOffsets[TYPE_EMISSIVE]) : (void*)(base + mOffsets[TYPE_COLOR]);
        glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_COLOR], ptr);
    }
    if (data_mask & MAP_EMISSIVE)
    {
        S32 loc = TYPE_EMISSIVE;
        void* ptr = (void*)(base + mOffsets[TYPE_EMISSIVE]);
        glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_EMISSIVE], ptr);

        if (!(data_mask & MAP_COLOR))
        { //map emissive to color channel when color is not also being bound to avoid unnecessary shader swaps
            loc = TYPE_COLOR;
            glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_EMISSIVE], ptr);
        }
    }
    if (data_mask & MAP_WEIGHT)
    {
        S32 loc = TYPE_WEIGHT;
        void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT]);
        glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT], ptr);
    }
    if (data_mask & MAP_WEIGHT4)
    {
        S32 loc = TYPE_WEIGHT4;
        void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT4]);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT4], ptr);
    }
    if (data_mask & MAP_CLOTHWEIGHT)
    {
        S32 loc = TYPE_CLOTHWEIGHT;
        void* ptr = (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_CLOTHWEIGHT], ptr);
    }
    if (data_mask & MAP_TEXTURE_INDEX)
    {
        S32 loc = TYPE_TEXTURE_INDEX;
        void* ptr = (void*)(base + mOffsets[TYPE_VERTEX] + 12);
        glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
    }
    if (data_mask & MAP_VERTEX)
    {
        S32 loc = TYPE_VERTEX;
        void* ptr = (void*)(base + mOffsets[TYPE_VERTEX]);
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
    }
}

void LLVertexBuffer::setPositionData(const LLVector4a* data)
{
    llassert(sGLRenderBuffer == mGLBuffer);
    flush_vbo(GL_ARRAY_BUFFER, 0, sizeof(LLVector4a) * getNumVerts(), (U8*) data);
}

void LLVertexBuffer::setTexCoordData(const LLVector2* data)
{
    llassert(sGLRenderBuffer == mGLBuffer);
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD0], mOffsets[TYPE_TEXCOORD0] + sTypeSize[TYPE_TEXCOORD0] * getNumVerts(), (U8*)data);
}

void LLVertexBuffer::setColorData(const LLColor4U* data)
{
    llassert(sGLRenderBuffer == mGLBuffer);
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_COLOR], mOffsets[TYPE_COLOR] + sTypeSize[TYPE_COLOR] * getNumVerts(), (U8*) data);
}


