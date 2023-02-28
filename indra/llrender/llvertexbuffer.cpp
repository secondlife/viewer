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

    GLuint ret = 0;
    constexpr U32 pool_size = 4096;

    thread_local static GLuint sNamePool[pool_size];
    thread_local static U32 sIndex = 0;

    if (sIndex == 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("gen buffer");
        sIndex = pool_size;
        if (!gGLManager.mIsAMD)
        {
            glGenBuffers(pool_size, sNamePool);
        }
        else
        { // work around for AMD driver bug
            for (U32 i = 0; i < pool_size; ++i)
            {
                glGenBuffers(1, sNamePool + i);
            }
        }
    }

    ret = sNamePool[--sIndex];
    return ret;
}

#define ANALYZE_VBO_POOL 0

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

    U32 mTouchCount = 0;

#if ANALYZE_VBO_POOL
    U64 mDistributed = 0;
    U64 mAllocated = 0;
    U64 mReserved = 0;
    U32 mMisses = 0;
    U32 mHits = 0;
#endif

    // increase the size to some common value (e.g. a power of two) to increase hit rate
    void adjustSize(U32& size)
    {
        // size = nhpo2(size);  // (193/303)/580 MB (distributed/allocated)/reserved in VBO Pool. Overhead: 66 percent. Hit rate: 77 percent

        //(245/276)/385 MB (distributed/allocated)/reserved in VBO Pool. Overhead: 57 percent. Hit rate: 69 percent
        //(187/209)/397 MB (distributed/allocated)/reserved in VBO Pool. Overhead: 112 percent. Hit rate: 76 percent
        U32 block_size = llmax(nhpo2(size) / 8, (U32) 16);
        size += block_size - (size % block_size);
    }

    void allocate(GLenum type, U32 size, GLuint& name, U8*& data)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
        llassert(type == GL_ARRAY_BUFFER || type == GL_ELEMENT_ARRAY_BUFFER);
        llassert(name == 0); // non zero name indicates a gl name that wasn't freed
        llassert(data == nullptr);  // non null data indicates a buffer that wasn't freed
        llassert(size >= 2);  // any buffer size smaller than a single index is nonsensical

#if ANALYZE_VBO_POOL
        mDistributed += size;
        adjustSize(size);
        mAllocated += size;
#else
        adjustSize(size);
#endif

        auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

        Pool::iterator iter = pool.find(size);
        if (iter == pool.end())
        { // cache miss, allocate a new buffer
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("vbo pool miss");
            LL_PROFILE_GPU_ZONE("vbo alloc");

#if ANALYZE_VBO_POOL
            mMisses++;
#endif
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
#if ANALYZE_VBO_POOL
            mHits++;
            llassert(mReserved >= size); // assert if accounting gets messed up
            mReserved -= size;
#endif

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

        clean();
    }

    void free(GLenum type, U32 size, GLuint name, U8* data)
    {
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
        llassert(type == GL_ARRAY_BUFFER || type == GL_ELEMENT_ARRAY_BUFFER);
        llassert(size >= 2);
        llassert(name != 0);
        llassert(data != nullptr);

        clean();

#if ANALYZE_VBO_POOL
        llassert(mDistributed >= size);
        mDistributed -= size;
        adjustSize(size);
        llassert(mAllocated >= size);
        mAllocated -= size;
        mReserved += size;
#else
        adjustSize(size);
#endif

        auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

        Pool::iterator iter = pool.find(size);

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

    // clean periodically (clean gets called for every alloc/free)
    void clean()
    {
        mTouchCount++;
        if (mTouchCount < 1024) // clean every 1k touches
        {
            return;
        }
        mTouchCount = 0;

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
#if ANALYZE_VBO_POOL
                    llassert(mReserved >= iter->first);
                    mReserved -= iter->first;
#endif
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

#if ANALYZE_VBO_POOL
        LL_INFOS() << llformat("(%d/%d)/%d MB (distributed/allocated)/total in VBO Pool. Overhead: %d percent. Hit rate: %d percent", 
            mDistributed / 1000000, 
            mAllocated / 1000000, 
            (mAllocated + mReserved) / 1000000, // total bytes
            ((mAllocated+mReserved-mDistributed)*100)/llmax(mDistributed, (U64) 1), // overhead percent
            (mHits*100)/llmax(mMisses+mHits, (U32)1)) // hit rate percent
            << LL_ENDL;
#endif
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

#if ANALYZE_VBO_POOL
        mReserved = 0;
#endif

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
const U32 LLVertexBuffer::sTypeSize[LLVertexBuffer::TYPE_MAX] =
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
void LLVertexBuffer::drawElements(U32 mode, const LLVector4a* pos, const LLVector2* tc, U32 num_indices, const U16* indicesp)
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
                U32 idx = (U32) (v[i][3]+0.25f);
				if (idx >= shader->mFeatures.mIndexedTextureChannels)
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
    drawRange(mode, 0, mNumVerts-1, count, indices_offset);
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
    llassert(sVBOPool == nullptr);
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
	mTypeMask(typemask)
{
	//zero out offsets
	for (U32 i = 0; i < TYPE_MAX; i++)
	{
		mOffsets[i] = 0;
	}
}

//static
U32 LLVertexBuffer::calcOffsets(const U32& typemask, U32* offsets, U32 num_vertices)
{
    U32 offset = 0;
	for (U32 i=0; i<TYPE_TEXTURE_INDEX; i++)
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
U32 LLVertexBuffer::calcVertexSize(const U32& typemask)
{
    U32 size = 0;
	for (U32 i = 0; i < TYPE_TEXTURE_INDEX; i++)
	{
		U32 mask = 1<<i;
		if (typemask & mask)
		{
			size += LLVertexBuffer::sTypeSize[i];
		}
	}

	return size;
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
    llassert(sVBOPool);

    if (sVBOPool)
    {
        llassert(mSize == 0);
        llassert(mGLBuffer == 0);
        llassert(mMappedData == nullptr);

        mSize = size;
        sVBOPool->allocate(GL_ARRAY_BUFFER, mSize, mGLBuffer, mMappedData);
    }
}

void LLVertexBuffer::genIndices(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    llassert(sVBOPool);

    if (sVBOPool)
    {
        llassert(mIndicesSize == 0);
        llassert(mGLIndices == 0);
        llassert(mMappedIndexData == nullptr);
        mIndicesSize = size;
        sVBOPool->allocate(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mGLIndices, mMappedIndexData);
    }
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
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
        //llassert(sVBOPool);
        if (sVBOPool)
        {
            sVBOPool->free(GL_ARRAY_BUFFER, mSize, mGLBuffer, mMappedData);
        }

        mSize = 0;
        mGLBuffer = 0;
        mMappedData = nullptr;
	}
}

void LLVertexBuffer::destroyGLIndices()
{
	if (mGLIndices || mMappedIndexData)
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
        //llassert(sVBOPool);
        if (sVBOPool)
        {
            sVBOPool->free(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mGLIndices, mMappedIndexData);
        }

        mIndicesSize = 0;
        mGLIndices = 0;
        mMappedIndexData = nullptr;
	}
}

bool LLVertexBuffer::updateNumVerts(U32 nverts)
{
	llassert(nverts >= 0);

	bool success = true;

	if (nverts > 65536)
	{
		LL_WARNS() << "Vertex buffer overflow!" << LL_ENDL;
		nverts = 65536;
	}

	U32 needed_size = calcOffsets(mTypeMask, mOffsets, nverts);

    if (needed_size != mSize)
    {
        success &= createGLBuffer(needed_size);
    }

    llassert(mSize == needed_size);
	mNumVerts = nverts;
	return success;
}

bool LLVertexBuffer::updateNumIndices(U32 nindices)
{
	llassert(nindices >= 0);

	bool success = true;

	U32 needed_size = sizeof(U16) * nindices;

	if (needed_size != mIndicesSize)
	{
		success &= createGLIndices(needed_size);
	}

    llassert(mIndicesSize == needed_size);
	mNumIndices = nindices;
	return success;
}

bool LLVertexBuffer::allocateBuffer(U32 nverts, U32 nindices)
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
bool expand_region(LLVertexBuffer::MappedRegion& region, U32 start, U32 end)
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
U8* LLVertexBuffer::mapVertexBuffer(LLVertexBuffer::AttributeType type, U32 index, S32 count)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	
    if (count == -1)
    {
        count = mNumVerts - index;
    }

    U32 start = mOffsets[type] + sTypeSize[type] * index;
    U32 end = start + sTypeSize[type] * count-1;

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


U8* LLVertexBuffer::mapIndexBuffer(U32 index, S32 count)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
	
	if (count == -1)
	{
		count = mNumIndices-index;
	}

    U32 start = sizeof(U16) * index;
    U32 end = start + sizeof(U16) * count-1;

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

// flush the given byte range
//  target -- "targret" parameter for glBufferSubData
//  start -- first byte to copy
//  end -- last byte to copy (NOT last byte + 1)
//  data -- mMappedData or mMappedIndexData
static void flush_vbo(GLenum target, U32 start, U32 end, void* data)
{
    if (end != 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("glBufferSubData");
        LL_PROFILE_ZONE_NUM(start);
        LL_PROFILE_ZONE_NUM(end);
        LL_PROFILE_ZONE_NUM(end-start);

        constexpr U32 block_size = 8192;

        for (U32 i = start; i <= end; i += block_size)
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("glBufferSubData block");
            //LL_PROFILE_GPU_ZONE("glBufferSubData");
            U32 tend = llmin(i + block_size, end);
            U32 size = tend - i + 1;
            glBufferSubData(target, i, size, (U8*) data + (i-start));
        }
    }
}

void LLVertexBuffer::unmapBuffer()
{
    struct SortMappedRegion
    {
        bool operator()(const MappedRegion& lhs, const MappedRegion& rhs)
        {
            return lhs.mStart < rhs.mStart;
        }
    };

	if (!mMappedVertexRegions.empty())
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - vertex");
        if (sGLRenderBuffer != mGLBuffer)
        {
            glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
            sGLRenderBuffer = mGLBuffer;
        }
            
        U32 start = 0;
        U32 end = 0;

        std::sort(mMappedVertexRegions.begin(), mMappedVertexRegions.end(), SortMappedRegion());

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
	}
	
	if (!mMappedIndexRegions.empty())
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - index");

        if (mGLIndices != sGLRenderIndices)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
            sGLRenderIndices = mGLIndices;
        }
        U32 start = 0;
        U32 end = 0;

        std::sort(mMappedIndexRegions.begin(), mMappedIndexRegions.end(), SortMappedRegion());

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
	}
}

//----------------------------------------------------------------------------

template <class T,LLVertexBuffer::AttributeType type> struct VertexBufferStrider
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
            U32 stride = LLVertexBuffer::sTypeSize[type];

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

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector3>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector3,TYPE_VERTEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a,TYPE_VERTEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getIndexStrider(LLStrider<U16>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<U16,TYPE_INDEX>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord0Strider(LLStrider<LLVector2>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD0>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord1Strider(LLStrider<LLVector2>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD1>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector2,TYPE_TEXCOORD2>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector3>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector3,TYPE_NORMAL>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector3>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector3,TYPE_TANGENT>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector4a>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a,TYPE_TANGENT>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getColorStrider(LLStrider<LLColor4U>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLColor4U,TYPE_COLOR>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getEmissiveStrider(LLStrider<LLColor4U>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLColor4U,TYPE_EMISSIVE>::get(*this, strider, index, count);
}
bool LLVertexBuffer::getWeightStrider(LLStrider<F32>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<F32,TYPE_WEIGHT>::get(*this, strider, index, count);
}

bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4>& strider, U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4,TYPE_WEIGHT4>::get(*this, strider, index, count);
}

bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4>& strider, U32 index, S32 count)
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
    llassert((data_mask & mTypeMask) == data_mask);

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
        AttributeType loc = TYPE_NORMAL;
        void* ptr = (void*)(base + mOffsets[TYPE_NORMAL]);
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_NORMAL], ptr);
    }
    if (data_mask & MAP_TEXCOORD3)
    {
        AttributeType loc = TYPE_TEXCOORD3;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD3]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD3], ptr);
    }
    if (data_mask & MAP_TEXCOORD2)
    {
        AttributeType loc = TYPE_TEXCOORD2;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD2]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD2], ptr);
    }
    if (data_mask & MAP_TEXCOORD1)
    {
        AttributeType loc = TYPE_TEXCOORD1;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD1]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD1], ptr);
    }
    if (data_mask & MAP_TANGENT)
    {
        AttributeType loc = TYPE_TANGENT;
        void* ptr = (void*)(base + mOffsets[TYPE_TANGENT]);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TANGENT], ptr);
    }
    if (data_mask & MAP_TEXCOORD0)
    {
        AttributeType loc = TYPE_TEXCOORD0;
        void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD0]);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD0], ptr);
    }
    if (data_mask & MAP_COLOR)
    {
        AttributeType loc = TYPE_COLOR;
        //bind emissive instead of color pointer if emissive is present
        void* ptr = (data_mask & MAP_EMISSIVE) ? (void*)(base + mOffsets[TYPE_EMISSIVE]) : (void*)(base + mOffsets[TYPE_COLOR]);
        glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_COLOR], ptr);
    }
    if (data_mask & MAP_EMISSIVE)
    {
        AttributeType loc = TYPE_EMISSIVE;
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
        AttributeType loc = TYPE_WEIGHT;
        void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT]);
        glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT], ptr);
    }
    if (data_mask & MAP_WEIGHT4)
    {
        AttributeType loc = TYPE_WEIGHT4;
        void* ptr = (void*)(base + mOffsets[TYPE_WEIGHT4]);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT4], ptr);
    }
    if (data_mask & MAP_CLOTHWEIGHT)
    {
        AttributeType loc = TYPE_CLOTHWEIGHT;
        void* ptr = (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_TRUE, LLVertexBuffer::sTypeSize[TYPE_CLOTHWEIGHT], ptr);
    }
    if (data_mask & MAP_TEXTURE_INDEX)
    {
        AttributeType loc = TYPE_TEXTURE_INDEX;
        void* ptr = (void*)(base + mOffsets[TYPE_VERTEX] + 12);
        glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
    }
    if (data_mask & MAP_VERTEX)
    {
        AttributeType loc = TYPE_VERTEX;
        void* ptr = (void*)(base + mOffsets[TYPE_VERTEX]);
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
    }
}

void LLVertexBuffer::setPositionData(const LLVector4a* data)
{
    llassert(sGLRenderBuffer == mGLBuffer);
    flush_vbo(GL_ARRAY_BUFFER, 0, sizeof(LLVector4a) * getNumVerts()-1, (U8*) data);
}

void LLVertexBuffer::setTexCoordData(const LLVector2* data)
{
    llassert(sGLRenderBuffer == mGLBuffer);
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD0], mOffsets[TYPE_TEXCOORD0] + sTypeSize[TYPE_TEXCOORD0] * getNumVerts() - 1, (U8*)data);
}

void LLVertexBuffer::setColorData(const LLColor4U* data)
{
    llassert(sGLRenderBuffer == mGLBuffer);
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_COLOR], mOffsets[TYPE_COLOR] + sTypeSize[TYPE_COLOR] * getNumVerts() - 1, (U8*) data);
}


