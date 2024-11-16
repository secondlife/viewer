/**
 * @file llvertexbuffer.h
 * @brief LLVertexBuffer wrapper for OpengGL vertex buffer objects
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

#ifndef LL_LLVERTEXBUFFER_H
#define LL_LLVERTEXBUFFER_H

#include "llgl.h"
#include "v2math.h"
#include "v3math.h"
#include "v4math.h"
#include "v4coloru.h"
#include "llstrider.h"
#include "llrender.h"
#include "lltrace.h"
#include <set>
#include <vector>
#include <list>
#include <glm/gtc/matrix_transform.hpp>

#define LL_MAX_VERTEX_ATTRIB_LOCATION 64

//============================================================================
// NOTES
// Threading:
//  All constructors should take an 'create' paramater which should only be
//  'true' when called from the main thread. Otherwise createGLBuffer() will
//  be called as soon as getVertexPointer(), etc is called (which MUST ONLY be
//  called from the main (i.e OpenGL) thread)


//============================================================================
// base class
class LLPrivateMemoryPool;
class LLVertexBuffer;

class LLVertexBufferData
{
public:
    LLVertexBufferData()
        : mVB(nullptr)
        , mMode(0)
        , mCount(0)
        , mTexName(0)
        , mProjection(glm::identity<glm::mat4>())
        , mModelView(glm::identity<glm::mat4>())
        , mTexture0(glm::identity<glm::mat4>())
    {}
    LLVertexBufferData(LLVertexBuffer* buffer, U8 mode, U32 count, U32 tex_name, const glm::mat4& model_view, const glm::mat4& projection, const glm::mat4& texture0)
        : mVB(buffer)
        , mMode(mode)
        , mCount(count)
        , mTexName(tex_name)
        , mProjection(model_view)
        , mModelView(projection)
        , mTexture0(texture0)
    {}
    void drawWithMatrix();
    void draw();
    LLPointer<LLVertexBuffer> mVB;
    U8 mMode;
    U32 mCount;
    U32 mTexName;
    glm::mat4 mProjection;
    glm::mat4 mModelView;
    glm::mat4 mTexture0;
};
typedef std::list<LLVertexBufferData> buffer_data_list_t;

class LLVertexBuffer final : public LLRefCount
{
public:
    struct MappedRegion
    {
        U32 mStart;
        U32 mEnd;
    };

    LLVertexBuffer(const LLVertexBuffer& rhs)
    {
        *this = rhs;
    }

    const LLVertexBuffer& operator=(const LLVertexBuffer& rhs)
    {
        LL_ERRS() << "Illegal operation!" << LL_ENDL;
        return *this;
    }

    static void initClass(LLWindow* window);
    static void cleanupClass();
    static void setupClientArrays(U32 data_mask);
    static void drawArrays(U32 mode, const std::vector<LLVector3>& pos);
    static void drawElements(U32 mode, const LLVector4a* pos, const LLVector2* tc, U32 num_indices, const U16* indicesp);

    static void unbind(); //unbind any bound vertex buffer

    //get the size of a vertex with the given typemask
    static U32 calcVertexSize(const U32& typemask);

    //get the size of a buffer with the given typemask and vertex count
    //fill offsets with the offset of each vertex component array into the buffer
    // indexed by the following enum
    static U32 calcOffsets(const U32& typemask, U32* offsets, U32 num_vertices);

    // flush any pending mapped buffers
    static void flushBuffers();

    //WARNING -- when updating these enums you MUST
    // 1 - update LLVertexBuffer::sTypeSize
    // 2 - update LLVertexBuffer::vb_type_name
    // 3 - add a strider accessor
    // 4 - modify LLVertexBuffer::setupVertexBuffer
    // 6 - modify LLViewerShaderMgr::mReservedAttribs

    // clang-format off
    enum AttributeType {                      // Shader attribute name, set in LLShaderMgr::initAttribsAndUniforms()
        TYPE_VERTEX = 0,        //  "position"
        TYPE_NORMAL,            //  "normal"
        TYPE_TEXCOORD0,         //  "texcoord0"
        TYPE_TEXCOORD1,         //  "texcoord1"
        TYPE_TEXCOORD2,         //  "texcoord2"
        TYPE_TEXCOORD3,         //  "texcoord3"
        TYPE_COLOR,             //  "diffuse_color"
        TYPE_EMISSIVE,          //  "emissive"
        TYPE_TANGENT,           //  "tangent"
        TYPE_WEIGHT,            //  "weight"
        TYPE_WEIGHT4,           //  "weight4"
        TYPE_CLOTHWEIGHT,       //  "clothing"
        TYPE_JOINT,             //  "joint"
        TYPE_TEXTURE_INDEX,     //  "texture_index"
        TYPE_MAX,   // TYPE_MAX is the size/boundary marker for attributes that go in the vertex buffer
        TYPE_INDEX, // TYPE_INDEX is beyond _MAX because it lives in a separate (index) buffer
    };
    // clang-format on

    enum {
        MAP_VERTEX = (1<<TYPE_VERTEX),
        MAP_NORMAL = (1<<TYPE_NORMAL),
        MAP_TEXCOORD0 = (1<<TYPE_TEXCOORD0),
        MAP_TEXCOORD1 = (1<<TYPE_TEXCOORD1),
        MAP_TEXCOORD2 = (1<<TYPE_TEXCOORD2),
        MAP_TEXCOORD3 = (1<<TYPE_TEXCOORD3),
        MAP_COLOR = (1<<TYPE_COLOR),
        MAP_EMISSIVE = (1<<TYPE_EMISSIVE),
        MAP_TANGENT = (1<<TYPE_TANGENT),
        MAP_WEIGHT = (1<<TYPE_WEIGHT),
        MAP_WEIGHT4 = (1<<TYPE_WEIGHT4),
        MAP_CLOTHWEIGHT = (1<<TYPE_CLOTHWEIGHT),
        MAP_JOINT = (1<<TYPE_JOINT),
        MAP_TEXTURE_INDEX = (1<<TYPE_TEXTURE_INDEX),
    };

protected:
    friend class LLRender;

    ~LLVertexBuffer(); // use unref()

    void setupVertexBuffer();

    void    genBuffer(U32 size);
    void    genIndices(U32 size);
    bool    createGLBuffer(U32 size);
    bool    createGLIndices(U32 size);
    void    destroyGLBuffer();
    void    destroyGLIndices();
    bool    updateNumVerts(U32 nverts);
    bool    updateNumIndices(U32 nindices);

public:
    LLVertexBuffer(U32 typemask);

    // allocate buffer
    bool    allocateBuffer(U32 nverts, U32 nindices);

    // map for data access (see also getFooStrider below)
    U8*     mapVertexBuffer(AttributeType type, U32 index, S32 count = -1);
    U8*     mapIndexBuffer(U32 index, S32 count = -1);

    // synonym for flushBuffers
    void    unmapBuffer();

    // set for rendering
    // assumes (and will assert on) the following:
    //      - this buffer has no pending unmapBuffer call
    //      - a shader is currently bound
    //      - This buffer has sufficient attributes within it to satisfy the needs of the currently bound shader
    void    setBuffer();

    // Only call each getVertexPointer, etc, once before calling unmapBuffer()
    // call unmapBuffer() after calls to getXXXStrider() before any calls to setBuffer()
    // example:
    //   vb->getVertexBuffer(verts);
    //   vb->getNormalStrider(norms);
    //   setVertsNorms(verts, norms);
    //   vb->unmapBuffer();
    bool getVertexStrider(LLStrider<LLVector3>& strider, U32 index=0, S32 count = -1);
    bool getVertexStrider(LLStrider<LLVector4a>& strider, U32 index=0, S32 count = -1);
    bool getIndexStrider(LLStrider<U16>& strider, U32 index=0, S32 count = -1);
    bool getTexCoord0Strider(LLStrider<LLVector2>& strider, U32 index=0, S32 count = -1);
    bool getTexCoord1Strider(LLStrider<LLVector2>& strider, U32 index=0, S32 count = -1);
    bool getTexCoord2Strider(LLStrider<LLVector2>& strider, U32 index=0, S32 count = -1);
    bool getNormalStrider(LLStrider<LLVector3>& strider, U32 index=0, S32 count = -1);
    bool getNormalStrider(LLStrider<LLVector4a>& strider, U32 index = 0, S32 count = -1);
    bool getTangentStrider(LLStrider<LLVector3>& strider, U32 index=0, S32 count = -1);
    bool getTangentStrider(LLStrider<LLVector4a>& strider, U32 index=0, S32 count = -1);
    bool getColorStrider(LLStrider<LLColor4U>& strider, U32 index=0, S32 count = -1);
    bool getEmissiveStrider(LLStrider<LLColor4U>& strider, U32 index=0, S32 count = -1);
    bool getWeightStrider(LLStrider<F32>& strider, U32 index=0, S32 count = -1);
    bool getWeight4Strider(LLStrider<LLVector4>& strider, U32 index=0, S32 count = -1);
    bool getClothWeightStrider(LLStrider<LLVector4>& strider, U32 index=0, S32 count = -1);

    void setPositionData(const LLVector4a* data);
    void setNormalData(const LLVector4a* data);
    void setTangentData(const LLVector4a* data);
    void setWeight4Data(const LLVector4a* data);
    void setJointData(const U64* data);
    void setTexCoord0Data(const LLVector2* data);
    void setTexCoord1Data(const LLVector2* data);
    void setColorData(const LLColor4U* data);
    void setIndexData(const U16* data);
    void setIndexData(const U32* data);

    void setPositionData(const LLVector4a* data, U32 offset, U32 count);
    void setNormalData(const LLVector4a* data, U32 offset, U32 count);
    void setTangentData(const LLVector4a* data, U32 offset, U32 count);
    void setWeight4Data(const LLVector4a* data, U32 offset, U32 count);
    void setJointData(const U64* data, U32 offset, U32 count);
    void setTexCoord0Data(const LLVector2* data, U32 offset, U32 count);
    void setTexCoord1Data(const LLVector2* data, U32 offset, U32 count);
    void setColorData(const LLColor4U* data, U32 offset, U32 count);
    void setIndexData(const U16* data, U32 offset, U32 count);
    void setIndexData(const U32* data, U32 offset, U32 count);


    U32 getNumVerts() const                 { return mNumVerts; }
    U32 getNumIndices() const               { return mNumIndices; }

    U32 getTypeMask() const                 { return mTypeMask; }
    bool hasDataType(AttributeType type) const { return ((1 << type) & getTypeMask()); }
    U32 getSize() const                     { return mSize; }
    U32 getIndicesSize() const              { return mIndicesSize; }
    U8* getMappedData() const               { return mMappedData; }
    U8* getMappedIndices() const            { return mMappedIndexData; }
    U32 getOffset(AttributeType type) const { return mOffsets[type]; }

    // these functions assume (and assert on) the current VBO being bound
    // Detailed error checking can be enabled by setting gDebugGL to true
    void draw(U32 mode, U32 count, U32 indices_offset) const;
    void drawArrays(U32 mode, U32 offset, U32 count) const;
    void drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const;

    // draw without syncing matrices.  If you're positive there have been no matrix
    // since the last call to syncMatrices, this is much faster than drawRange
    void drawRangeFast(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const;

    //for debugging, validate data in given range is valid
    bool validateRange(U32 start, U32 end, U32 count, U32 offset) const;

    #ifdef LL_PROFILER_ENABLE_RENDER_DOC
    void setLabel(const char* label);
    #endif

    void clone(LLVertexBuffer& target) const;

protected:
    U32     mGLBuffer = 0;      // GL VBO handle
    U32     mGLIndices = 0;     // GL IBO handle
    U32     mNumVerts = 0;      // Number of vertices allocated
    U32     mNumIndices = 0;    // Number of indices allocated
    U32     mIndicesType = GL_UNSIGNED_SHORT; // type of indices in index buffer
    U32     mIndicesStride = 2;     // size of each index in bytes
    U32     mOffsets[TYPE_MAX]; // byte offsets into mMappedData of each attribute

    U8* mMappedData = nullptr;  // pointer to currently mapped data (NULL if unmapped)
    U8* mMappedIndexData = nullptr; // pointer to currently mapped indices (NULL if unmapped)

    U32     mTypeMask = 0;      // bitmask of present vertex attributes

    U32     mSize = 0;          // size in bytes of mMappedData
    U32     mIndicesSize = 0;   // size in bytes of mMappedIndexData

    std::vector<MappedRegion> mMappedVertexRegions;  // list of mMappedData byte ranges that must be sent to GL
    std::vector<MappedRegion> mMappedIndexRegions;   // list of mMappedIndexData byte ranges that must be sent to GL

private:
    // DEPRECATED
    // These function signatures are deprecated, but for some reason
    // there are classes in an external package that depend on LLVertexBuffer

    // TODO: move these classes into viewer repository
    friend class LLNavShapeVBOManager;
    friend class LLNavMeshVBOManager;

    void flush_vbo(GLenum target, U32 start, U32 end, void* data, U8* dst);

    LLVertexBuffer(U32 typemask, U32 usage)
        : LLVertexBuffer(typemask)
    {}

    bool    allocateBuffer(S32 nverts, S32 nindices, bool create) { return allocateBuffer(nverts, nindices); }

    // actually unmap buffer
    void _unmapBuffer();

    // add to set of mapped buffers
    void _mapBuffer();
    bool mMapped = false;

public:

    static U64 getBytesAllocated();
    static const U32 sTypeSize[TYPE_MAX];
    static const U32 sGLMode[LLRender::NUM_MODES];
    static U32 sGLRenderBuffer;
    static U32 sGLRenderIndices;
    static U32 sLastMask;
    static U32 sVertexCount;
};

#ifdef LL_PROFILER_ENABLE_RENDER_DOC
#define LL_LABEL_VERTEX_BUFFER(buf, name) buf->setLabel(name)
#else
#define LL_LABEL_VERTEX_BUFFER(buf, name)
#endif


#endif // LL_LLVERTEXBUFFER_H
