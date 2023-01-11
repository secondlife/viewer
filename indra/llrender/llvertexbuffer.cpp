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

//static
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

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
void LLVertexBuffer::setLabel(const char* label) {
	LL_LABEL_OBJECT_GL(GL_BUFFER, mGLBuffer, strlen(label), label);
}
#endif

void LLVertexBuffer::drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const
{
	validateRange(start, end, count, indices_offset);
	gGL.syncMatrices();

	llassert(mNumVerts >= 0);
	llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);

	if (mGLIndices != sGLRenderIndices)
	{
		LL_ERRS() << "Wrong index buffer bound." << LL_ENDL;
	}

	if (mGLBuffer != sGLRenderBuffer)
	{
		LL_ERRS() << "Wrong vertex buffer bound." << LL_ENDL;
	}

	if (gDebugGL && useVBOs())
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
	glDrawRangeElements(sGLMode[mode], start, end, count, GL_UNSIGNED_SHORT, 
		idx);
	LLGLSLShader::stopProfile(count, mode);
	stop_glerror();
}

void LLVertexBuffer::drawRangeFast(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const
{
    gGL.syncMatrices();

    U16* idx = ((U16*)getIndicesPointer()) + indices_offset;

        glDrawRangeElements(sGLMode[mode], start, end, count, GL_UNSIGNED_SHORT,
            idx);
}

void LLVertexBuffer::draw(U32 mode, U32 count, U32 indices_offset) const
{
	llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);
	gGL.syncMatrices();

	llassert(mNumIndices >= 0);
	if (indices_offset >= (U32) mNumIndices ||
	    indices_offset + count > (U32) mNumIndices)
	{
		LL_ERRS() << "Bad index buffer draw range: [" << indices_offset << ", " << indices_offset+count << "]" << LL_ENDL;
	}

	
	if (mGLIndices != sGLRenderIndices)
	{
		LL_ERRS() << "Wrong index buffer bound." << LL_ENDL;
	}

	if (mGLBuffer != sGLRenderBuffer)
	{
		LL_ERRS() << "Wrong vertex buffer bound." << LL_ENDL;
	}

	if (mode >= LLRender::NUM_MODES)
	{
		LL_ERRS() << "Invalid draw mode: " << mode << LL_ENDL;
		return;
	}

	stop_glerror();
	LLGLSLShader::startProfile();
    glDrawElements(sGLMode[mode], count, GL_UNSIGNED_SHORT,
		((U16*) getIndicesPointer()) + indices_offset);
	LLGLSLShader::stopProfile(count, mode);
	stop_glerror();
}


void LLVertexBuffer::drawArrays(U32 mode, U32 first, U32 count) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    llassert(LLGLSLShader::sCurBoundShaderPtr != NULL);
    gGL.syncMatrices();

#ifndef LL_RELEASE_FOR_DOWNLOAD
    llassert(mNumVerts >= 0);
	if (first >= (U32) mNumVerts ||
	    first + count > (U32) mNumVerts)
    {
		LL_ERRS() << "Bad vertex buffer draw range: [" << first << ", " << first+count << "]" << LL_ENDL;
    }

    if (mGLBuffer != sGLRenderBuffer || useVBOs() != sVBOActive)
    {
        LL_ERRS() << "Wrong vertex buffer bound." << LL_ENDL;
    }

    if (mode >= LLRender::NUM_MODES)
    {
        LL_ERRS() << "Invalid draw mode: " << mode << LL_ENDL;
        return;
    }
#endif

    LLGLSLShader::startProfile();
    {
            glDrawArrays(sGLMode[mode], first, count);
    }
    LLGLSLShader::stopProfile(count, mode);

    stop_glerror();
}

//static
void LLVertexBuffer::initClass(LLWindow* window)
{
    sEnableVBOs = true;
    sDisableVBOMapping = true;

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
	if (sGLRenderArray)
	{
		glBindVertexArray(0);
		sGLRenderArray = 0;
		sGLRenderIndices = 0;
		sIBOActive = false;
	}

	if (sVBOActive)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		sVBOActive = false;
	}
	if (sIBOActive)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

    //llassert(0 == sAllocatedBytes);
    //llassert(0 == sAllocatedIndexBytes);
}

//----------------------------------------------------------------------------

S32 LLVertexBuffer::determineUsage(S32 usage)
{
	S32 ret_usage = usage;

	if (!sEnableVBOs)
	{
		ret_usage = 0;
	}
	
	if (ret_usage == GL_STREAM_DRAW && !sUseStreamDraw)
	{
		ret_usage = 0;
	}
	
	if (ret_usage == GL_DYNAMIC_DRAW && sPreferStreamDraw)
	{
		ret_usage = GL_STREAM_DRAW;
	}
	
	if (ret_usage == 0 && LLRender::sGLCoreProfile)
	{ //MUST use VBOs for all rendering
		ret_usage = GL_STREAM_DRAW;
	}
	
	return ret_usage;
}

LLVertexBuffer::LLVertexBuffer(U32 typemask, S32 usage) 
:	LLRefCount(),

	mNumVerts(0),
	mNumIndices(0),
	mSize(0),
	mIndicesSize(0),
	mTypeMask(typemask),
	mUsage(LLVertexBuffer::determineUsage(usage)),
	mGLBuffer(0),
	mGLIndices(0),
	mMappedData(NULL),
	mMappedIndexData(NULL),
	mMappedDataUsingVBOs(false),
	mMappedIndexDataUsingVBOs(false),
	mVertexLocked(false),
	mIndexLocked(false),
	mFinal(false),
	mEmpty(true)
{
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

	sCount--;

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

//----------------------------------------------------------------------------

// batch glGenBuffers
static GLuint gen_buffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    constexpr U32 pool_size = 4096;

    thread_local static GLuint sNamePool[pool_size];
    thread_local static U32 sIndex = 0;

    if (sIndex == 0)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("gen ibo");
        sIndex = pool_size;
        glGenBuffers(pool_size, sNamePool);
    }

    return sNamePool[--sIndex];
}

// batch glDeleteBuffers
static void release_buffer(U32 buff)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
#if 0

    constexpr U32 pool_size = 4096;

    thread_local static GLuint sNamePool[pool_size];
    thread_local static U32 sIndex = 0;

    if (sIndex == pool_size)
    {
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("gen ibo");
        sIndex = 0;
        glDeleteBuffers(pool_size, sNamePool);
    }

    sNamePool[sIndex++] = buff;
#else
    glDeleteBuffers(1, &buff);
#endif
}

void LLVertexBuffer::genBuffer(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

    mSize = size;
    mMappedData = (U8*) ll_aligned_malloc_16(size);
    mGLBuffer = gen_buffer();

    glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
    glBufferData(GL_ARRAY_BUFFER, mSize, nullptr, mUsage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    sGLCount++;
}

void LLVertexBuffer::genIndices(U32 size)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;

    mIndicesSize = size;
    mMappedIndexData = (U8*) ll_aligned_malloc_16(size);

    mGLIndices = gen_buffer();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, nullptr, mUsage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	sGLCount++;
}

void LLVertexBuffer::releaseBuffer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    release_buffer(mGLBuffer);
    mGLBuffer = 0;

    ll_aligned_free_16(mMappedData);
    mMappedData = nullptr;
	
	sGLCount--;
}

void LLVertexBuffer::releaseIndices()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
    release_buffer(mGLIndices);
    mGLIndices = 0;

    ll_aligned_free_16(mMappedIndexData);
    mMappedIndexData = nullptr;

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
	}

	return success;
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
	}

	return success;
}

bool LLVertexBuffer::useVBOs() const
{
	//it's generally ineffective to use VBO for things that are streaming on apple
	return (mUsage != 0);
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
U8* LLVertexBuffer::mapVertexBuffer(S32 type, S32 index, S32 count, bool map_range)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
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

		if (mVertexLocked && map_range)
		{
			LL_ERRS() << "Attempted to map a specific range of a buffer that was already mapped." << LL_ENDL;
		}

		if (!mVertexLocked)
		{
			mVertexLocked = true;
			sMappedCount++;
			stop_glerror();	

			map_range = false;
				
			if (!mMappedData)
			{
				log_glerror();

				//check the availability of memory
				LLMemory::logMemoryInfo(true);
				
				LL_ERRS() << "memory allocation for vertex data failed." << LL_ENDL;

			}
		}
	}
	else
	{
		map_range = false;
	}
	
    return mMappedData+mOffsets[type]+sTypeSize[type]*index;
}


U8* LLVertexBuffer::mapIndexBuffer(S32 index, S32 count, bool map_range)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
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
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elem);

				if (elem != mGLIndices)
				{
					LL_ERRS() << "Wrong index buffer bound!" << LL_ENDL;
				}
			}

			map_range = false;
		}

		if (!mMappedIndexData)
		{
			log_glerror();
			LLMemory::logMemoryInfo(true);

			LL_ERRS() << "memory allocation for Index data failed. " << LL_ENDL;
		}
	}
	else
	{
		map_range = false;
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

		if (!mMappedVertexRegions.empty())
		{
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
		}
		else
		{
            llassert(false); // this shouldn't happen -- a buffer must always be explicitly mapped
		}
		
		mVertexLocked = false;
		sMappedCount--;
	}
	
	if (mMappedIndexData && mIndexLocked)
	{
        LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("unmapBuffer - index");
		bindGLIndices();
		
		if (!mMappedIndexRegions.empty())
		{
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
		}
		else
		{
            llassert(false); // this shouldn't happen -- a buffer must always be explicitly mapped
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

bool LLVertexBuffer::bindGLBuffer(bool force_bind)
{
	bool ret = false;

	if (useVBOs() && (force_bind || (mGLBuffer && (mGLBuffer != sGLRenderBuffer || !sVBOActive))))
	{
        LL_PROFILE_ZONE_SCOPED_CATEGORY_VERTEX;
		glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
		sGLRenderBuffer = mGLBuffer;
		sBindCount++;
		sVBOActive = true;
		ret = true;
	}

	return ret;
}

bool LLVertexBuffer::bindGLBufferFast()
{
    if (mGLBuffer != sGLRenderBuffer || !sVBOActive)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
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

    bool ret = false;
	if (useVBOs() && (force_bind || (mGLIndices && (mGLIndices != sGLRenderIndices || !sIBOActive))))
	{
		/*if (sMapped)
		{
			LL_ERRS() << "VBO bound while another VBO mapped!" << LL_ENDL;
		}*/
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
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
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
        sGLRenderIndices = mGLIndices;
        sBindCount++;
        sIBOActive = true;
        
        return true;
    }

    return false;
}

void LLVertexBuffer::flush(bool discard)
{
	if (useVBOs())
	{
        if (discard)
        { // discard existing VBO data if the buffer must be updated
            
            if (!mMappedVertexRegions.empty())
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("flush discard vbo");
                LL_PROFILE_ZONE_NUM(mSize);
                release_buffer(mGLBuffer);
                mGLBuffer = gen_buffer();
                bindGLBuffer();
                {
                    LL_PROFILE_GPU_ZONE("glBufferData");
                    glBufferData(GL_ARRAY_BUFFER, mSize, nullptr, mUsage);

                    for (int i = 0; i < mSize; i += 65536)
                    {
                        LL_PROFILE_GPU_ZONE("glBufferSubData");
                        S32 end = llmin(i + 65536, mSize);
                        S32 count = end - i;
                        glBufferSubData(GL_ARRAY_BUFFER, i, count, mMappedData + i);
                    }
                }
                mMappedVertexRegions.clear();
            }
            if (!mMappedIndexRegions.empty())
            {
                LL_PROFILE_ZONE_NAMED_CATEGORY_VERTEX("flush discard ibo");
                LL_PROFILE_ZONE_NUM(mIndicesSize);
                release_buffer(mGLIndices);
                mGLIndices = gen_buffer();
                bindGLIndices();
                {
                    LL_PROFILE_GPU_ZONE("glBufferData (ibo)");
                    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mMappedIndexData, mUsage);
                }
                mMappedIndexRegions.clear();
            }
        }
        else
        {
            unmapBuffer();
        }

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

	if (gDebugGL && data_mask != 0)
	{ //make sure data requirements are fulfilled
		LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
		if (shader)
		{
			U32 required_mask = 0;
			for (U32 i = 0; i < LLVertexBuffer::TYPE_TEXTURE_INDEX; ++i)
			{
				if (shader->getAttribLocation(i) > -1)
				{
					U32 required = 1 << i;
					if ((data_mask & required) == 0)
					{
						LL_WARNS() << "Missing attribute: " << LLShaderMgr::instance()->mReservedAttribs[i] << LL_ENDL;
					}

					required_mask |= required;
				}
			}

			if ((data_mask & required_mask) != required_mask)
			{
				
				U32 unsatisfied_mask = (required_mask & ~data_mask);

                for (U32 i = 0; i < TYPE_MAX; i++)
                {
                    U32 unsatisfied_flag = unsatisfied_mask & (1 << i);
                    switch (unsatisfied_flag)
                    {
                        case 0: break;
                        case MAP_VERTEX: LL_INFOS() << "Missing vert pos" << LL_ENDL; break;
                        case MAP_NORMAL: LL_INFOS() << "Missing normals" << LL_ENDL; break;
                        case MAP_TEXCOORD0: LL_INFOS() << "Missing TC 0" << LL_ENDL; break;
                        case MAP_TEXCOORD1: LL_INFOS() << "Missing TC 1" << LL_ENDL; break;
                        case MAP_TEXCOORD2: LL_INFOS() << "Missing TC 2" << LL_ENDL; break;
                        case MAP_TEXCOORD3: LL_INFOS() << "Missing TC 3" << LL_ENDL; break;
                        case MAP_COLOR: LL_INFOS() << "Missing vert color" << LL_ENDL; break;
                        case MAP_EMISSIVE: LL_INFOS() << "Missing emissive" << LL_ENDL; break;
                        case MAP_TANGENT: LL_INFOS() << "Missing tangent" << LL_ENDL; break;
                        case MAP_WEIGHT: LL_INFOS() << "Missing weight" << LL_ENDL; break;
                        case MAP_WEIGHT4: LL_INFOS() << "Missing weightx4" << LL_ENDL; break;
                        case MAP_CLOTHWEIGHT: LL_INFOS() << "Missing clothweight" << LL_ENDL; break;
                        case MAP_TEXTURE_INDEX: LL_INFOS() << "Missing tex index" << LL_ENDL; break;
                        default: LL_INFOS() << "Missing who effin knows: " << unsatisfied_flag << LL_ENDL;
                    }
                }

                // TYPE_INDEX is beyond TYPE_MAX, so check for it individually
                if (unsatisfied_mask & (1 << TYPE_INDEX))
                {
                   LL_INFOS() << "Missing indices" << LL_ENDL;
                }

				LL_ERRS() << "Shader consumption mismatches data provision." << LL_ENDL;
			}
		}
	}

	if (useVBOs())
	{
		const bool bindBuffer = bindGLBuffer();
		const bool bindIndices = bindGLIndices();
			
		setup = setup || bindBuffer || bindIndices;

		if (gDebugGL)
		{
			GLint buff;
			glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buff);
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
				glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buff);
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
			glBindVertexArray(0);
			sGLRenderArray = 0;
			sGLRenderIndices = 0;
			sIBOActive = false;
		}

		if (mGLBuffer)
		{
			if (sVBOActive)
			{
				glBindBuffer(GL_ARRAY_BUFFER, 0);
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
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
				sBindCount++;
				sIBOActive = false;
			}
			
			sGLRenderIndices = mGLIndices;
		}
	}

	setupClientArrays(data_mask);

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
	U8* base = useVBOs() ? nullptr: mMappedData;

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
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_NORMAL], ptr);
	}
	if (data_mask & MAP_TEXCOORD3)
	{
		S32 loc = TYPE_TEXCOORD3;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD3]);
		glVertexAttribPointer(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD3], ptr);
	}
	if (data_mask & MAP_TEXCOORD2)
	{
		S32 loc = TYPE_TEXCOORD2;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD2]);
		glVertexAttribPointer(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD2], ptr);
	}
	if (data_mask & MAP_TEXCOORD1)
	{
		S32 loc = TYPE_TEXCOORD1;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD1]);
		glVertexAttribPointer(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD1], ptr);
	}
	if (data_mask & MAP_TANGENT)
	{
		S32 loc = TYPE_TANGENT;
		void* ptr = (void*)(base + mOffsets[TYPE_TANGENT]);
		glVertexAttribPointer(loc, 4,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TANGENT], ptr);
	}
	if (data_mask & MAP_TEXCOORD0)
	{
		S32 loc = TYPE_TEXCOORD0;
		void* ptr = (void*)(base + mOffsets[TYPE_TEXCOORD0]);
		glVertexAttribPointer(loc,2,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_TEXCOORD0], ptr);
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
		void* ptr = (void*)(base+mOffsets[TYPE_WEIGHT4]);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_WEIGHT4], ptr);
	}
	if (data_mask & MAP_CLOTHWEIGHT)
	{
		S32 loc = TYPE_CLOTHWEIGHT;
		void* ptr = (void*)(base + mOffsets[TYPE_CLOTHWEIGHT]);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_TRUE,  LLVertexBuffer::sTypeSize[TYPE_CLOTHWEIGHT], ptr);
	}
	if (data_mask & MAP_TEXTURE_INDEX && 
			(gGLManager.mGLSLVersionMajor >= 2 || gGLManager.mGLSLVersionMinor >= 30)) //indexed texture rendering requires GLSL 1.30 or later
	{
		S32 loc = TYPE_TEXTURE_INDEX;
		void *ptr = (void*) (base + mOffsets[TYPE_VERTEX] + 12);
		glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
	}
	if (data_mask & MAP_VERTEX)
	{
		S32 loc = TYPE_VERTEX;
		void* ptr = (void*)(base + mOffsets[TYPE_VERTEX]);
		glVertexAttribPointer(loc, 3,GL_FLOAT, GL_FALSE, LLVertexBuffer::sTypeSize[TYPE_VERTEX], ptr);
	}	

	llglassertok();
	}	

void LLVertexBuffer::setupVertexBufferFast(U32 data_mask)
{
    U8* base = nullptr;

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
    bindGLBuffer();
    flush_vbo(GL_ARRAY_BUFFER, 0, sizeof(LLVector4a) * getNumVerts(), (U8*) data);
}

void LLVertexBuffer::setTexCoordData(const LLVector2* data)
{
    bindGLBuffer();
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_TEXCOORD0], mOffsets[TYPE_TEXCOORD0] + sTypeSize[TYPE_TEXCOORD0] * getNumVerts(), (U8*)data);
}

void LLVertexBuffer::setColorData(const LLColor4U* data)
{
    bindGLBuffer();
    flush_vbo(GL_ARRAY_BUFFER, mOffsets[TYPE_COLOR], mOffsets[TYPE_COLOR] + sTypeSize[TYPE_COLOR] * getNumVerts(), (U8*) data);
}


