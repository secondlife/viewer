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
#include <set>
#include <vector>
#include <list>

#define LL_MAX_VERTEX_ATTRIB_LOCATION 64

//============================================================================
// NOTES
// Threading:
//  All constructors should take an 'create' paramater which should only be
//  'true' when called from the main thread. Otherwise createGLBuffer() will
//  be called as soon as getVertexPointer(), etc is called (which MUST ONLY be
//  called from the main (i.e OpenGL) thread)


//============================================================================
// gl name pools for dynamic and streaming buffers
class LLVBOPool
{
public:
	static U32 sBytesPooled;
	static U32 sIndexBytesPooled;
	
	static U32 sCurGLName;

	LLVBOPool(U32 vboUsage, U32 vboType);
		
	const U32 mUsage;
	const U32 mType;

	//size MUST be a power of 2
	volatile U8* allocate(U32& name, U32 size, bool for_seed = false);
	
	//size MUST be the size provided to allocate that returned the given name
	void release(U32 name, volatile U8* buffer, U32 size);
	
	//batch allocate buffers to be provided to the application on demand
	void seedPool();

	//destroy all records in mFreeList
	void cleanup();

	U32 genBuffer();
	void deleteBuffer(U32 name);

	class Record
	{
	public:
		U32 mGLName;
		volatile U8* mClientData;
	};

	std::list<U32> mGLNamePool;

	typedef std::list<Record> record_list_t;
	std::vector<record_list_t> mFreeList;
	std::vector<U32> mMissCount;

};


//============================================================================
// base class 
class LLPrivateMemoryPool;
class LLVertexBuffer : public LLRefCount
{
public:
	class MappedRegion
	{
	public:
		S32 mType;
		S32 mIndex;
		S32 mCount;
		
		MappedRegion(S32 type, S32 index, S32 count);
	};

	LLVertexBuffer(const LLVertexBuffer& rhs)
		: mUsage(rhs.mUsage)
	{
		*this = rhs;
	}

	const LLVertexBuffer& operator=(const LLVertexBuffer& rhs)
	{
		llerrs << "Illegal operation!" << llendl;
		return *this;
	}

	static LLVBOPool sStreamVBOPool;
	static LLVBOPool sDynamicVBOPool;
	static LLVBOPool sStreamIBOPool;
	static LLVBOPool sDynamicIBOPool;

	static std::list<U32> sAvailableVAOName;
	static U32 sCurVAOName;

	static bool	sUseStreamDraw;
	static bool sUseVAO;
	static bool	sPreferStreamDraw;

	static void seedPools();

	static U32 getVAOName();
	static void releaseVAOName(U32 name);

	static void initClass(bool use_vbo, bool no_vbo_mapping);
	static void cleanupClass();
	static void setupClientArrays(U32 data_mask);
	static void pushPositions(U32 mode, const LLVector4a* pos, U32 count);
	static void drawArrays(U32 mode, const std::vector<LLVector3>& pos, const std::vector<LLVector3>& norm);
	static void drawElements(U32 mode, const LLVector4a* pos, const LLVector2* tc, S32 num_indices, const U16* indicesp);

 	static void unbind(); //unbind any bound vertex buffer

	//get the size of a vertex with the given typemask
	static S32 calcVertexSize(const U32& typemask);

	//get the size of a buffer with the given typemask and vertex count
	//fill offsets with the offset of each vertex component array into the buffer
	// indexed by the following enum
	static S32 calcOffsets(const U32& typemask, S32* offsets, S32 num_vertices);		

	
	//WARNING -- when updating these enums you MUST 
	// 1 - update LLVertexBuffer::sTypeSize
	// 2 - add a strider accessor
	// 3 - modify LLVertexBuffer::setupVertexBuffer
	// 4 - modify LLVertexBuffer::setupClientArray
	// 5 - modify LLViewerShaderMgr::mReservedAttribs
	// 6 - update LLVertexBuffer::setupVertexArray
	enum {
		TYPE_VERTEX = 0,
		TYPE_NORMAL,
		TYPE_TEXCOORD0,
		TYPE_TEXCOORD1,
		TYPE_TEXCOORD2,
		TYPE_TEXCOORD3,
		TYPE_COLOR,
		TYPE_EMISSIVE,
		TYPE_BINORMAL,
		TYPE_WEIGHT,
		TYPE_WEIGHT4,
		TYPE_CLOTHWEIGHT,
		TYPE_TEXTURE_INDEX,
		TYPE_MAX,
		TYPE_INDEX,		
	};
	enum {
		MAP_VERTEX = (1<<TYPE_VERTEX),
		MAP_NORMAL = (1<<TYPE_NORMAL),
		MAP_TEXCOORD0 = (1<<TYPE_TEXCOORD0),
		MAP_TEXCOORD1 = (1<<TYPE_TEXCOORD1),
		MAP_TEXCOORD2 = (1<<TYPE_TEXCOORD2),
		MAP_TEXCOORD3 = (1<<TYPE_TEXCOORD3),
		MAP_COLOR = (1<<TYPE_COLOR),
		MAP_EMISSIVE = (1<<TYPE_EMISSIVE),
		// These use VertexAttribPointer and should possibly be made generic
		MAP_BINORMAL = (1<<TYPE_BINORMAL),
		MAP_WEIGHT = (1<<TYPE_WEIGHT),
		MAP_WEIGHT4 = (1<<TYPE_WEIGHT4),
		MAP_CLOTHWEIGHT = (1<<TYPE_CLOTHWEIGHT),
		MAP_TEXTURE_INDEX = (1<<TYPE_TEXTURE_INDEX),
	};
	
protected:
	friend class LLRender;

	virtual ~LLVertexBuffer(); // use unref()

	virtual void setupVertexBuffer(U32 data_mask); // pure virtual, called from mapBuffer()
	void setupVertexArray();
	
	void	genBuffer(U32 size);
	void	genIndices(U32 size);
	bool	bindGLBuffer(bool force_bind = false);
	bool	bindGLIndices(bool force_bind = false);
	bool	bindGLArray();
	void	releaseBuffer();
	void	releaseIndices();
	void	createGLBuffer(U32 size);
	void	createGLIndices(U32 size);
	void 	destroyGLBuffer();
	void 	destroyGLIndices();
	void	updateNumVerts(S32 nverts);
	void	updateNumIndices(S32 nindices); 
	void	unmapBuffer();
		
public:
	LLVertexBuffer(U32 typemask, S32 usage);
	
	// map for data access
	volatile U8*		mapVertexBuffer(S32 type, S32 index, S32 count, bool map_range);
	volatile U8*		mapIndexBuffer(S32 index, S32 count, bool map_range);

	void bindForFeedback(U32 channel, U32 type, U32 index, U32 count);

	// set for rendering
	virtual void	setBuffer(U32 data_mask); 	// calls  setupVertexBuffer() if data_mask is not 0
	void flush(); //flush pending data to GL memory
	// allocate buffer
	void	allocateBuffer(S32 nverts, S32 nindices, bool create);
	virtual void resizeBuffer(S32 newnverts, S32 newnindices);
			
	// Only call each getVertexPointer, etc, once before calling unmapBuffer()
	// call unmapBuffer() after calls to getXXXStrider() before any cals to setBuffer()
	// example:
	//   vb->getVertexBuffer(verts);
	//   vb->getNormalStrider(norms);
	//   setVertsNorms(verts, norms);
	//   vb->unmapBuffer();
	bool getVertexStrider(LLStrider<LLVector3>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getVertexStrider(LLStrider<LLVector4a>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getIndexStrider(LLStrider<U16>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTexCoord0Strider(LLStrider<LLVector2>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTexCoord1Strider(LLStrider<LLVector2>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getNormalStrider(LLStrider<LLVector3>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getBinormalStrider(LLStrider<LLVector3>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getColorStrider(LLStrider<LLColor4U>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getTextureIndexStrider(LLStrider<LLColor4U>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getEmissiveStrider(LLStrider<LLColor4U>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getWeightStrider(LLStrider<F32>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getWeight4Strider(LLStrider<LLVector4>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	bool getClothWeightStrider(LLStrider<LLVector4>& strider, S32 index=0, S32 count = -1, bool map_range = false);
	

	bool useVBOs() const;
	bool isEmpty() const					{ return mEmpty; }
	bool isLocked() const					{ return mVertexLocked || mIndexLocked; }
	S32 getNumVerts() const					{ return mNumVerts; }
	S32 getNumIndices() const				{ return mNumIndices; }
	
	volatile U8* getIndicesPointer() const			{ return useVBOs() ? (U8*) mAlignedIndexOffset : mMappedIndexData; }
	volatile U8* getVerticesPointer() const			{ return useVBOs() ? (U8*) mAlignedOffset : mMappedData; }
	U32 getTypeMask() const					{ return mTypeMask; }
	bool hasDataType(S32 type) const		{ return ((1 << type) & getTypeMask()); }
	S32 getSize() const;
	S32 getIndicesSize() const				{ return mIndicesSize; }
	volatile U8* getMappedData() const				{ return mMappedData; }
	volatile U8* getMappedIndices() const			{ return mMappedIndexData; }
	S32 getOffset(S32 type) const			{ return mOffsets[type]; }
	S32 getUsage() const					{ return mUsage; }
	bool isWriteable() const				{ return (mMappable || mUsage == GL_STREAM_DRAW_ARB) ? true : false; }

	void draw(U32 mode, U32 count, U32 indices_offset) const;
	void drawArrays(U32 mode, U32 offset, U32 count) const;
	void drawRange(U32 mode, U32 start, U32 end, U32 count, U32 indices_offset) const;

	//for debugging, validate data in given range is valid
	void validateRange(U32 start, U32 end, U32 count, U32 offset) const;

	

protected:	
	S32		mNumVerts;		// Number of vertices allocated
	S32		mNumIndices;	// Number of indices allocated
	
	ptrdiff_t mAlignedOffset;
	ptrdiff_t mAlignedIndexOffset;
	S32		mSize;
	S32		mIndicesSize;
	U32		mTypeMask;

	const S32		mUsage;			// GL usage
	
	U32		mGLBuffer;		// GL VBO handle
	U32		mGLIndices;		// GL IBO handle
	U32		mGLArray;		// GL VAO handle
	
	volatile U8* mMappedData;	// pointer to currently mapped data (NULL if unmapped)
	volatile U8* mMappedIndexData;	// pointer to currently mapped indices (NULL if unmapped)

	U32		mMappedDataUsingVBOs : 1;
	U32		mMappedIndexDataUsingVBOs : 1;
	U32		mVertexLocked : 1;			// if true, vertex buffer is being or has been written to in client memory
	U32		mIndexLocked : 1;			// if true, index buffer is being or has been written to in client memory
	U32		mFinal : 1;			// if true, buffer can not be mapped again
	U32		mEmpty : 1;			// if true, client buffer is empty (or NULL). Old values have been discarded.	
	
	mutable bool	mMappable;     // if true, use memory mapping to upload data (otherwise doublebuffer and use glBufferSubData)

	S32		mOffsets[TYPE_MAX];

	std::vector<MappedRegion> mMappedVertexRegions;
	std::vector<MappedRegion> mMappedIndexRegions;

	mutable LLGLFence* mFence;

	void placeFence() const;
	void waitFence() const;

	static S32 determineUsage(S32 usage);

private:
	static LLPrivateMemoryPool* sPrivatePoolp;

public:
	static S32 sCount;
	static S32 sGLCount;
	static S32 sMappedCount;
	static bool sMapped;
	typedef std::list<LLVertexBuffer*> buffer_list_t;
		
	static bool sDisableVBOMapping; //disable glMapBufferARB
	static bool sEnableVBOs;
	static S32 sTypeSize[TYPE_MAX];
	static U32 sGLMode[LLRender::NUM_MODES];
	static U32 sGLRenderBuffer;
	static U32 sGLRenderArray;
	static U32 sGLRenderIndices;
	static bool sVBOActive;
	static bool sIBOActive;
	static U32 sLastMask;
	static U32 sAllocatedBytes;
	static U32 sAllocatedIndexBytes;
	static U32 sVertexCount;
	static U32 sIndexCount;
	static U32 sBindCount;
	static U32 sSetCount;
};


#endif // LL_LLVERTEXBUFFER_H
