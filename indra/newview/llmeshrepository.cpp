/** 
 * @file llmeshrepository.cpp
 * @brief Mesh repository implementation.
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
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

#include "apr_pools.h"
#include "apr_dso.h"

#include "llmeshrepository.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llbufferstream.h"
#include "llcurl.h"
#include "llfasttimer.h"
#include "llfloatermodelpreview.h"
#include "llfloaterperms.h"
#include "lleconomy.h"
#include "llimagej2c.h"
#include "llhost.h"
#include "llnotificationsutil.h"
#include "llsd.h"
#include "llsdutil_math.h"
#include "llsdserialize.h"
#include "llthread.h"
#include "llvfile.h"
#include "llviewercontrol.h"
#include "llviewermenufile.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexturelist.h"
#include "llvolume.h"
#include "llvolumemgr.h"
#include "llvovolume.h"
#include "llworld.h"
#include "material_codes.h"
#include "pipeline.h"

#ifndef LL_WINDOWS
#include "netdb.h"
#endif

#include <queue>

LLFastTimer::DeclareTimer FTM_MESH_UPDATE("Mesh Update");
LLFastTimer::DeclareTimer FTM_LOAD_MESH("Load Mesh");

LLMeshRepository gMeshRepo;

const U32 MAX_MESH_REQUESTS_PER_SECOND = 100;

U32 LLMeshRepository::sBytesReceived = 0;
U32 LLMeshRepository::sHTTPRequestCount = 0;
U32 LLMeshRepository::sHTTPRetryCount = 0;
U32 LLMeshRepository::sCacheBytesRead = 0;
U32 LLMeshRepository::sCacheBytesWritten = 0;
U32 LLMeshRepository::sPeakKbps = 0;
	

const U32 MAX_TEXTURE_UPLOAD_RETRIES = 5;

std::string header_lod[] = 
{
	"lowest_lod",
	"low_lod",
	"medium_lod",
	"high_lod"
};


//get the number of bytes resident in memory for given volume
U32 get_volume_memory_size(const LLVolume* volume)
{
	U32 indices = 0;
	U32 vertices = 0;

	for (U32 i = 0; i < volume->getNumVolumeFaces(); ++i)
	{
		const LLVolumeFace& face = volume->getVolumeFace(i);
		indices += face.mNumIndices;
		vertices += face.mNumVertices;
	}


	return indices*2+vertices*11+sizeof(LLVolume)+sizeof(LLVolumeFace)*volume->getNumVolumeFaces();
}

LLVertexBuffer* get_vertex_buffer_from_mesh(LLCDMeshData& mesh, F32 scale = 1.f)
{
	LLVertexBuffer* buff = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX | LLVertexBuffer::MAP_NORMAL, 0);
	buff->allocateBuffer(mesh.mNumTriangles*3, 0, true);

	LLStrider<LLVector3> pos;
	LLStrider<LLVector3> norm;
	
	buff->getVertexStrider(pos);
	buff->getNormalStrider(norm);

	const F32* v = mesh.mVertexBase;
	
	if (mesh.mIndexType == LLCDMeshData::INT_16)
	{
		U16* idx = (U16*) mesh.mIndexBase;
		for (S32 j = 0; j < mesh.mNumTriangles; ++j)
		{ 
			F32* mp0 = (F32*) ((U8*)v+idx[0]*mesh.mVertexStrideBytes);
			F32* mp1 = (F32*) ((U8*)v+idx[1]*mesh.mVertexStrideBytes);
			F32* mp2 = (F32*) ((U8*)v+idx[2]*mesh.mVertexStrideBytes);

			idx = (U16*) (((U8*)idx)+mesh.mIndexStrideBytes);
			
			LLVector3 v0(mp0);
			LLVector3 v1(mp1);
			LLVector3 v2(mp2);

			LLVector3 n = (v1-v0)%(v2-v0);
			n.normalize();

			*pos++ = v0*scale;
			*pos++ = v1*scale;
			*pos++ = v2*scale;

			*norm++ = n;
			*norm++ = n;
			*norm++ = n;			
		}
	}
	else
	{
		U32* idx = (U32*) mesh.mIndexBase;
		for (S32 j = 0; j < mesh.mNumTriangles; ++j)
		{ 
			F32* mp0 = (F32*) ((U8*)v+idx[0]*mesh.mVertexStrideBytes);
			F32* mp1 = (F32*) ((U8*)v+idx[1]*mesh.mVertexStrideBytes);
			F32* mp2 = (F32*) ((U8*)v+idx[2]*mesh.mVertexStrideBytes);

			idx = (U32*) (((U8*)idx)+mesh.mIndexStrideBytes);
			
			LLVector3 v0(mp0);
			LLVector3 v1(mp1);
			LLVector3 v2(mp2);

			LLVector3 n = (v1-v0)%(v2-v0);
			n.normalize();

			*(pos++) = v0*scale;
			*(pos++) = v1*scale;
			*(pos++) = v2*scale;

			*(norm++) = n;
			*(norm++) = n;
			*(norm++) = n;			
		}
	}

	return buff;
}

S32 LLMeshRepoThread::sActiveHeaderRequests = 0;
S32 LLMeshRepoThread::sActiveLODRequests = 0;
U32	LLMeshRepoThread::sMaxConcurrentRequests = 1;


class LLTextureCostResponder : public LLCurl::Responder
{
public:
	LLTextureUploadData mData;
	LLMeshUploadThread* mThread;

	LLTextureCostResponder(LLTextureUploadData data, LLMeshUploadThread* thread) 
		: mData(data), mThread(thread)
	{

	}

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		mThread->mPendingConfirmations--;
		if (isGoodStatus(status))
		{
			mThread->priceResult(mData, content);	
		}
		else
		{
			llwarns << status << ": " << reason << llendl;

			if (mData.mRetries < MAX_TEXTURE_UPLOAD_RETRIES)
			{
				llwarns << "Retrying. (" << ++mData.mRetries << ")" << llendl;
			
				if (status == 499 || status == 500)
				{
					mThread->uploadTexture(mData);
				}
				else
				{
					llerrs << "Unhandled status " << status << llendl;
				}
			}
			else
			{ 
				llwarns << "Giving up after " << mData.mRetries << " retries." << llendl;
			}
		}
	}
};

class LLTextureUploadResponder : public LLCurl::Responder
{
public:
	LLTextureUploadData mData;
	LLMeshUploadThread* mThread;

	LLTextureUploadResponder(LLTextureUploadData data, LLMeshUploadThread* thread)
		: mData(data), mThread(thread)
	{
	}

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		mThread->mPendingUploads--;
		if (isGoodStatus(status))
		{
			mData.mUUID = content["new_asset"].asUUID();
			gMeshRepo.updateInventory(LLMeshRepository::inventory_data(mData.mPostData, content));
			mThread->onTextureUploaded(mData);
		}
		else
		{
			llwarns << status << ": " << reason << llendl;
			llwarns << "Retrying. (" << ++mData.mRetries << ")" << llendl;

			if (status == 404)
			{
				mThread->uploadTexture(mData);
			}
			else if (status == 499)
			{
				mThread->mConfirmedTextureQ.push(mData);
			}
			else
			{
				llerrs << "Unhandled status " << status << llendl;
			}
		}
	}
};

class LLMeshCostResponder : public LLCurl::Responder
{
public:
	LLMeshUploadData mData;
	LLMeshUploadThread* mThread;

	LLMeshCostResponder(LLMeshUploadData data, LLMeshUploadThread* thread) 
		: mData(data), mThread(thread)
	{

	}

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		mThread->mPendingConfirmations--;

		if (isGoodStatus(status))
		{
			mThread->priceResult(mData, content);	
		}
		else
		{
			llwarns << status << ": " << reason << llendl;
			llwarns << "Retrying. (" << ++mData.mRetries << ")" << llendl;
			
			if (status == 499)
			{
				mThread->uploadModel(mData);
			}
			else if (status == 400)
			{
				llwarns << "Status 400 received from server, giving up." << llendl;
			}
			else
			{
				llerrs << "Unhandled status " << status << llendl;
			}
		}
	}
};

class LLMeshUploadResponder : public LLCurl::Responder
{
public:
	LLMeshUploadData mData;
	LLMeshUploadThread* mThread;

	LLMeshUploadResponder(LLMeshUploadData data, LLMeshUploadThread* thread)
		: mData(data), mThread(thread)
	{
	}

	virtual void completed(U32 status, const std::string& reason, const LLSD& content)
	{
		mThread->mPendingUploads--;
		if (isGoodStatus(status))
		{
			mData.mUUID = content["new_asset"].asUUID();
			if (mData.mUUID.isNull())
			{
				LLSD args;
				std::string message = content["error"]["message"];
				std::string identifier = content["error"]["identifier"];
				std::string invalidity_identifier = content["error"]["invalidity_identifier"];

				args["MESSAGE"] = message;
				args["IDENTIFIER"] = identifier;
				args["INVALIDITY_IDENTIFIER"] = invalidity_identifier;
				args["LABEL"] = mData.mBaseModel->mLabel;

				gMeshRepo.uploadError(args);
			}
			else
			{
				gMeshRepo.updateInventory(LLMeshRepository::inventory_data(mData.mPostData, content));
				mThread->onModelUploaded(mData);
			}
		}
		else
		{
			llwarns << status << ": " << reason << llendl;
			llwarns << "Retrying. (" << ++mData.mRetries << ")" << llendl;

			if (status == 404)
			{
				mThread->uploadModel(mData);
			}
			else if (status == 499)
			{
				mThread->mConfirmedQ.push(mData);
			}
			else if (status != 500)
			{ //drop internal server errors on the floor, otherwise grab
				llerrs << "Unhandled status " << status << llendl;
			}
		}
	}
};


class LLMeshHeaderResponder : public LLCurl::Responder
{
public:
	LLVolumeParams mMeshParams;
	
	LLMeshHeaderResponder(const LLVolumeParams& mesh_params)
		: mMeshParams(mesh_params)
	{
	}

	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer);

};

class LLMeshLODResponder : public LLCurl::Responder
{
public:
	LLVolumeParams mMeshParams;
	S32 mLOD;
	U32 mRequestedBytes;
	U32 mOffset;

	LLMeshLODResponder(const LLVolumeParams& mesh_params, S32 lod, U32 offset, U32 requested_bytes)
		: mMeshParams(mesh_params), mLOD(lod), mOffset(offset), mRequestedBytes(requested_bytes)
	{
	}

	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer);

};

class LLMeshSkinInfoResponder : public LLCurl::Responder
{
public:
	LLUUID mMeshID;
	U32 mRequestedBytes;
	U32 mOffset;

	LLMeshSkinInfoResponder(const LLUUID& id, U32 offset, U32 size)
		: mMeshID(id), mRequestedBytes(size), mOffset(offset)
	{
	}

	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer);

};

class LLMeshDecompositionResponder : public LLCurl::Responder
{
public:
	LLUUID mMeshID;
	U32 mRequestedBytes;
	U32 mOffset;

	LLMeshDecompositionResponder(const LLUUID& id, U32 offset, U32 size)
		: mMeshID(id), mRequestedBytes(size), mOffset(offset)
	{
	}

	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer);

};

class LLMeshPhysicsShapeResponder : public LLCurl::Responder
{
public:
	LLUUID mMeshID;
	U32 mRequestedBytes;
	U32 mOffset;

	LLMeshPhysicsShapeResponder(const LLUUID& id, U32 offset, U32 size)
		: mMeshID(id), mRequestedBytes(size), mOffset(offset)
	{
	}

	virtual void completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer);

};


LLMeshRepoThread::LLMeshRepoThread()
: LLThread("mesh repo", NULL) 
{ 
	mWaiting = false;
	mMutex = new LLMutex(NULL);
	mHeaderMutex = new LLMutex(NULL);
	mSignal = new LLCondition(NULL);
}

LLMeshRepoThread::~LLMeshRepoThread()
{
	delete mMutex;
	mMutex = NULL;
	delete mHeaderMutex;
	mHeaderMutex = NULL;
	delete mSignal;
	mSignal = NULL;
}

void LLMeshRepoThread::run()
{
	mCurlRequest = new LLCurlRequest();
	LLCDResult res = LLConvexDecomposition::initThread();
	if (res != LLCD_OK)
	{
		llwarns << "convex decomposition unable to be loaded" << llendl;
	}

	while (!LLApp::isQuitting())
	{
		mWaiting = true;
		mSignal->wait();
		mWaiting = false;

		if (!LLApp::isQuitting())
		{
			static U32 count = 0;

			static F32 last_hundred = gFrameTimeSeconds;

			if (gFrameTimeSeconds - last_hundred > 1.f)
			{ //a second has gone by, clear count
				last_hundred = gFrameTimeSeconds;
				count = 0;	
			}

			// NOTE: throttling intentionally favors LOD requests over header requests
			
			while (!mLODReqQ.empty() && count < MAX_MESH_REQUESTS_PER_SECOND && sActiveLODRequests < sMaxConcurrentRequests)
			{
				{
					mMutex->lock();
					LODRequest req = mLODReqQ.front();
					mLODReqQ.pop();
					mMutex->unlock();
					if (fetchMeshLOD(req.mMeshParams, req.mLOD))
					{
						count++;
					}
				}
			}

			while (!mHeaderReqQ.empty() && count < MAX_MESH_REQUESTS_PER_SECOND && sActiveHeaderRequests < sMaxConcurrentRequests)
			{
				{
					mMutex->lock();
					HeaderRequest req = mHeaderReqQ.front();
					mHeaderReqQ.pop();
					mMutex->unlock();
					if (fetchMeshHeader(req.mMeshParams))
					{
						count++;
					}
				}
			}

			{ //mSkinRequests is protected by mSignal
				std::set<LLUUID> incomplete;
				for (std::set<LLUUID>::iterator iter = mSkinRequests.begin(); iter != mSkinRequests.end(); ++iter)
				{
					LLUUID mesh_id = *iter;
					if (!fetchMeshSkinInfo(mesh_id))
					{
						incomplete.insert(mesh_id);
					}
				}
				mSkinRequests = incomplete;
			}

			{ //mDecompositionRequests is protected by mSignal
				std::set<LLUUID> incomplete;
				for (std::set<LLUUID>::iterator iter = mDecompositionRequests.begin(); iter != mDecompositionRequests.end(); ++iter)
				{
					LLUUID mesh_id = *iter;
					if (!fetchMeshDecomposition(mesh_id))
					{
						incomplete.insert(mesh_id);
					}
				}
				mDecompositionRequests = incomplete;
			}

			{ //mPhysicsShapeRequests is protected by mSignal
				std::set<LLUUID> incomplete;
				for (std::set<LLUUID>::iterator iter = mPhysicsShapeRequests.begin(); iter != mPhysicsShapeRequests.end(); ++iter)
				{
					LLUUID mesh_id = *iter;
					if (!fetchMeshPhysicsShape(mesh_id))
					{
						incomplete.insert(mesh_id);
					}
				}
				mPhysicsShapeRequests = incomplete;
			}

			mCurlRequest->process();
		}
	}
	
	if (mSignal->isLocked())
	{ //make sure to let go of the mutex associated with the given signal before shutting down
		mSignal->unlock();
	}

	res = LLConvexDecomposition::quitThread();
	if (res != LLCD_OK)
	{
		llwarns << "convex decomposition unable to be quit" << llendl;
	}

	delete mCurlRequest;
	mCurlRequest = NULL;
}

void LLMeshRepoThread::loadMeshSkinInfo(const LLUUID& mesh_id)
{ //protected by mSignal, no locking needed here
	mSkinRequests.insert(mesh_id);
}

void LLMeshRepoThread::loadMeshDecomposition(const LLUUID& mesh_id)
{ //protected by mSignal, no locking needed here
	mDecompositionRequests.insert(mesh_id);
}

void LLMeshRepoThread::loadMeshPhysicsShape(const LLUUID& mesh_id)
{ //protected by mSignal, no locking needed here
	mPhysicsShapeRequests.insert(mesh_id);
}


void LLMeshRepoThread::loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{ //protected by mSignal, no locking needed here

	mesh_header_map::iterator iter = mMeshHeader.find(mesh_params.getSculptID());
	if (iter != mMeshHeader.end())
	{ //if we have the header, request LOD byte range
		LODRequest req(mesh_params, lod);
		{
			LLMutexLock lock(mMutex);
			mLODReqQ.push(req);
		}
	}
	else
	{ 
		HeaderRequest req(mesh_params);
		
		pending_lod_map::iterator pending = mPendingLOD.find(mesh_params);

		if (pending != mPendingLOD.end())
		{ //append this lod request to existing header request
			pending->second.push_back(lod);
			if (pending->second.size() > 4)
			{
				llerrs << "WTF?" << llendl;
			} 
		}
		else
		{ //if no header request is pending, fetch header
			LLMutexLock lock(mMutex);
			mHeaderReqQ.push(req);
			mPendingLOD[mesh_params].push_back(lod);
		}
	}
}

//static 
std::string LLMeshRepoThread::constructUrl(LLUUID mesh_id)
{
	std::string http_url;
	
	if (gAgent.getRegion())
	{
		http_url = gMeshRepo.mGetMeshCapability; 
	}

	if (!http_url.empty())
	{
		http_url += "/?mesh_id=";
		http_url += mesh_id.asString().c_str();
	}
	else
	{
		llwarns << "Current region does not have GetMesh capability!  Cannot load " << mesh_id << ".mesh" << llendl;
	}

	return http_url;
}

bool LLMeshRepoThread::fetchMeshSkinInfo(const LLUUID& mesh_id)
{ //protected by mMutex
	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	U32 header_size = mMeshHeaderSize[mesh_id];

	if (header_size > 0)
	{
		S32 offset = header_size + mMeshHeader[mesh_id]["skin"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["skin"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (offset >= 0 && size > 0)
		{
			//check VFS for mesh skin info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (skinInfoReceived(mesh_id, buffer, size))
					{
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			std::vector<std::string> headers;
			headers.push_back("Accept: application/octet-stream");

			std::string http_url = constructUrl(mesh_id);
			if (!http_url.empty())
			{
				++sActiveLODRequests;
				LLMeshRepository::sHTTPRequestCount++;
				mCurlRequest->getByteRange(constructUrl(mesh_id), headers, offset, size,
										   new LLMeshSkinInfoResponder(mesh_id, offset, size));
			}
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return true;
}

bool LLMeshRepoThread::fetchMeshDecomposition(const LLUUID& mesh_id)
{ //protected by mMutex
	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	U32 header_size = mMeshHeaderSize[mesh_id];

	if (header_size > 0)
	{
		S32 offset = header_size + mMeshHeader[mesh_id]["decomposition"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["decomposition"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (offset >= 0 && size > 0)
		{
			//check VFS for mesh skin info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (decompositionReceived(mesh_id, buffer, size))
					{
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			std::vector<std::string> headers;
			headers.push_back("Accept: application/octet-stream");

			std::string http_url = constructUrl(mesh_id);
			if (!http_url.empty())
			{
				++sActiveLODRequests;
				LLMeshRepository::sHTTPRequestCount++;
				mCurlRequest->getByteRange(http_url, headers, offset, size,
										   new LLMeshDecompositionResponder(mesh_id, offset, size));
			}
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return true;
}

bool LLMeshRepoThread::fetchMeshPhysicsShape(const LLUUID& mesh_id)
{ //protected by mMutex
	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	U32 header_size = mMeshHeaderSize[mesh_id];

	if (header_size > 0)
	{
		S32 offset = header_size + mMeshHeader[mesh_id]["physics_shape"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["physics_shape"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (offset >= 0 && size > 0)
		{
			//check VFS for mesh physics shape info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (physicsShapeReceived(mesh_id, buffer, size))
					{
						delete[] buffer;
						return true;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			std::vector<std::string> headers;
			headers.push_back("Accept: application/octet-stream");

			std::string http_url = constructUrl(mesh_id);
			if (!http_url.empty())
			{
				++sActiveLODRequests;
				LLMeshRepository::sHTTPRequestCount++;
				mCurlRequest->getByteRange(http_url, headers, offset, size,
										   new LLMeshPhysicsShapeResponder(mesh_id, offset, size));
			}
		}
		else
		{ //no physics shape whatsoever, report back NULL
			physicsShapeReceived(mesh_id, NULL, 0);
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return true;
}

bool LLMeshRepoThread::fetchMeshHeader(const LLVolumeParams& mesh_params)
{
	bool retval = false;

	{
		//look for mesh in asset in vfs
		LLVFile file(gVFS, mesh_params.getSculptID(), LLAssetType::AT_MESH);
			
		S32 size = file.getSize();

		if (size > 0)
		{
			U8 buffer[1024];
			S32 bytes = llmin(size, 1024);
			LLMeshRepository::sCacheBytesRead += bytes;	
			file.read(buffer, bytes);
			if (headerReceived(mesh_params, buffer, bytes))
			{ //did not do an HTTP request, return false
				return false;
			}
		}
	}

	//either cache entry doesn't exist or is corrupt, request header from simulator

	std::vector<std::string> headers;
	headers.push_back("Accept: application/octet-stream");

	std::string http_url = constructUrl(mesh_params.getSculptID());
	if (!http_url.empty())
	{
		++sActiveHeaderRequests;
		retval = true;
		//grab first 4KB if we're going to bother with a fetch.  Cache will prevent future fetches if a full mesh fits
		//within the first 4KB
		LLMeshRepository::sHTTPRequestCount++;
		mCurlRequest->getByteRange(http_url, headers, 0, 4096, new LLMeshHeaderResponder(mesh_params));
	}

	return retval;
}

bool LLMeshRepoThread::fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{ //protected by mMutex
	mHeaderMutex->lock();

	bool retval = false;

	LLUUID mesh_id = mesh_params.getSculptID();
	
	U32 header_size = mMeshHeaderSize[mesh_id];

	if (header_size > 0)
	{
		S32 offset = header_size + mMeshHeader[mesh_id][header_lod[lod]]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id][header_lod[lod]]["size"].asInteger();
		mHeaderMutex->unlock();
		if (offset >= 0 && size > 0)
		{

			//check VFS for mesh asset
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's (reserved block but not written)
				bool zero = true;
				for (S32 i = 0; i < llmin(size, 1024) && zero; ++i)
				{
					zero = buffer[i] > 0 ? false : true;
				}

				if (!zero)
				{ //attempt to parse
					if (lodReceived(mesh_params, lod, buffer, size))
					{
						delete[] buffer;
						return false;
					}
				}

				delete[] buffer;
			}

			//reading from VFS failed for whatever reason, fetch from sim
			std::vector<std::string> headers;
			headers.push_back("Accept: application/octet-stream");

			std::string http_url = constructUrl(mesh_id);
			if (!http_url.empty())
			{
				++sActiveLODRequests;
				retval = true;
				LLMeshRepository::sHTTPRequestCount++;
				mCurlRequest->getByteRange(constructUrl(mesh_id), headers, offset, size,
										   new LLMeshLODResponder(mesh_params, lod, offset, size));
			}
			else
			{
				mUnavailableQ.push(LODRequest(mesh_params, lod));
			}
		}
		else
		{
			mUnavailableQ.push(LODRequest(mesh_params, lod));
		}
	}
	else
	{
		mHeaderMutex->unlock();
	}

	return retval;
}

bool LLMeshRepoThread::headerReceived(const LLVolumeParams& mesh_params, U8* data, S32 data_size)
{
	LLSD header;
	
	U32 header_size = 0;
	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);

		std::string deprecated_header("<? LLSD/Binary ?>");

		if (res_str.substr(0, deprecated_header.size()) == deprecated_header)
		{
			res_str = res_str.substr(deprecated_header.size()+1, data_size);
			header_size = deprecated_header.size()+1;
		}
		data_size = res_str.size();

		std::istringstream stream(res_str);

		if (!LLSDSerialize::fromBinary(header, stream, data_size))
		{
			llwarns << "Mesh header parse error.  Not a valid mesh asset!" << llendl;
			return false;
		}

		header_size += stream.tellg();
	}
	else
	{
		llinfos
			<< "Marking header as non-existent, will not retry." << llendl;
		header["404"] = 1;
	}

	{
		U32 cost = gMeshRepo.calcResourceCost(header);

		LLUUID mesh_id = mesh_params.getSculptID();
		
		mHeaderMutex->lock();
		mMeshHeaderSize[mesh_id] = header_size;
		mMeshHeader[mesh_id] = header;
		mMeshResourceCost[mesh_id] = cost;
		mHeaderMutex->unlock();

		//check for pending requests
		pending_lod_map::iterator iter = mPendingLOD.find(mesh_params);
		if (iter != mPendingLOD.end())
		{
			LLMutexLock lock(mMutex);
			for (U32 i = 0; i < iter->second.size(); ++i)
			{
				LODRequest req(mesh_params, iter->second[i]);
				mLODReqQ.push(req);
			}
		}
		mPendingLOD.erase(iter);
	}

	return true;
}

bool LLMeshRepoThread::lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size)
{
	LLVolume* volume = new LLVolume(mesh_params, LLVolumeLODGroup::getVolumeScaleFromDetail(lod));
	std::string mesh_string((char*) data, data_size);
	std::istringstream stream(mesh_string);

	if (volume->unpackVolumeFaces(stream, data_size))
	{
		LoadedMesh mesh(volume, mesh_params, lod);
		if (volume->getNumFaces() > 0)
		{
			LLMutexLock lock(mMutex);
			mLoadedQ.push(mesh);
			return true;
		}
	}

	return false;
}

bool LLMeshRepoThread::skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD skin;

	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);

		std::istringstream stream(res_str);

		if (!unzip_llsd(skin, stream, data_size))
		{
			llwarns << "Mesh skin info parse error.  Not a valid mesh asset!" << llendl;
			return false;
		}
	}
	
	{
		LLMeshSkinInfo info;
		info.mMeshID = mesh_id;

		if (skin.has("joint_names"))
		{
			for (U32 i = 0; i < skin["joint_names"].size(); ++i)
			{
				info.mJointNames.push_back(skin["joint_names"][i]);
			}
		}

		if (skin.has("inverse_bind_matrix"))
		{
			for (U32 i = 0; i < skin["inverse_bind_matrix"].size(); ++i)
			{
				LLMatrix4 mat;
				for (U32 j = 0; j < 4; j++)
				{
					for (U32 k = 0; k < 4; k++)
					{
						mat.mMatrix[j][k] = skin["inverse_bind_matrix"][i][j*4+k].asReal();
					}
				}

				info.mInvBindMatrix.push_back(mat);
			}
		}

		if (skin.has("bind_shape_matrix"))
		{
			for (U32 j = 0; j < 4; j++)
			{
				for (U32 k = 0; k < 4; k++)
				{
					info.mBindShapeMatrix.mMatrix[j][k] = skin["bind_shape_matrix"][j*4+k].asReal();
				}
			}
		}

		if (skin.has("alt_inverse_bind_matrix"))
		{
			for (U32 i = 0; i < skin["alt_inverse_bind_matrix"].size(); ++i)
			{
				LLMatrix4 mat;
				for (U32 j = 0; j < 4; j++)
				{
					for (U32 k = 0; k < 4; k++)
					{
						mat.mMatrix[j][k] = skin["alt_inverse_bind_matrix"][i][j*4+k].asReal();
					}
				}
				
				info.mAlternateBindMatrix.push_back(mat);
			}
		}
		
		mSkinInfoQ.push(info);
	}

	return true;
}

bool LLMeshRepoThread::decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD decomp;

	if (data_size > 0)
	{
		std::string res_str((char*) data, data_size);

		std::istringstream stream(res_str);

		if (!unzip_llsd(decomp, stream, data_size))
		{
			llwarns << "Mesh decomposition parse error.  Not a valid mesh asset!" << llendl;
			return false;
		}
	}
	
	{
		LLMeshDecomposition* d = new LLMeshDecomposition();
		d->mMeshID = mesh_id;

		if (decomp.has("HullList"))
		{
			// updated for const-correctness. gcc is picky about this type of thing - Nyx
			const LLSD::Binary& hulls = decomp["HullList"].asBinary();
			const LLSD::Binary& position = decomp["Position"].asBinary();

			U16* p = (U16*) &position[0];

			d->mHull.resize(hulls.size());

			LLVector3 min;
			LLVector3 max;
			LLVector3 range;

			min.setValue(decomp["Min"]);
			max.setValue(decomp["Max"]);
			range = max-min;

			for (U32 i = 0; i < hulls.size(); ++i)
			{
				U16 count = (hulls[i] == 0) ? 256 : hulls[i];
				
				for (U32 j = 0; j < count; ++j)
				{
					d->mHull[i].push_back(LLVector3(
						(F32) p[0]/65535.f*range.mV[0]+min.mV[0],
						(F32) p[1]/65535.f*range.mV[1]+min.mV[1],
						(F32) p[2]/65535.f*range.mV[2]+min.mV[2]));
					p += 3;
				}		 

			}
				
			//get mesh for decomposition
			for (U32 i = 0; i < d->mHull.size(); ++i)
			{
				LLCDHull hull;
				hull.mNumVertices = d->mHull[i].size();
				hull.mVertexBase = d->mHull[i][0].mV;
				hull.mVertexStrideBytes = 12;

				LLCDMeshData mesh;
				LLCDResult res = LLCD_OK;
				if (LLConvexDecomposition::getInstance() != NULL)
				{
					res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
				}
				if (res != LLCD_OK)
				{
					llwarns << "could not get mesh from hull from convex decomposition lib." << llendl;
					return false;
				}


				d->mMesh.push_back(get_vertex_buffer_from_mesh(mesh));
			}	
		}

		if (decomp.has("Hull"))
		{
			const LLSD::Binary& position = decomp["Hull"].asBinary();

			U16* p = (U16*) &position[0];

			LLVector3 min;
			LLVector3 max;
			LLVector3 range;

			min.setValue(decomp["Min"]);
			max.setValue(decomp["Max"]);
			range = max-min;

			U16 count = position.size()/6;
			
			for (U32 j = 0; j < count; ++j)
			{
				d->mBaseHull.push_back(LLVector3(
					(F32) p[0]/65535.f*range.mV[0]+min.mV[0],
					(F32) p[1]/65535.f*range.mV[1]+min.mV[1],
					(F32) p[2]/65535.f*range.mV[2]+min.mV[2]));
				p += 3;
			}		 
				
			//get mesh for decomposition
			LLCDHull hull;
			hull.mNumVertices = d->mBaseHull.size();
			hull.mVertexBase = d->mBaseHull[0].mV;
			hull.mVertexStrideBytes = 12;

			LLCDMeshData mesh;
			LLCDResult res = LLCD_OK;
			if (LLConvexDecomposition::getInstance() != NULL)
			{
				res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
			}
			if (res != LLCD_OK)
			{
				llwarns << "could not get mesh from hull from convex decomposition lib." << llendl;
				return false;
			}

			d->mBaseHullMesh = get_vertex_buffer_from_mesh(mesh);
		}
		else
		{
			//empty vertex buffer to indicate decomposition has been fetched
			//but contains no base hull
			d->mBaseHullMesh = new LLVertexBuffer(0, 0);
		}

		mDecompositionQ.push(d);
	}

	return true;
}

bool LLMeshRepoThread::physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD physics_shape;

	LLMeshDecomposition* d = new LLMeshDecomposition();
	d->mMeshID = mesh_id;

	if (data == NULL)
	{ //no data, no physics shape exists
		d->mPhysicsShapeMesh = new LLVertexBuffer(0,0);
	}
	else
	{
		LLVolumeParams volume_params;
		volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
		volume_params.setSculptID(mesh_id, LL_SCULPT_TYPE_MESH);
		LLPointer<LLVolume> volume = new LLVolume(volume_params,0);
		std::string mesh_string((char*) data, data_size);
		std::istringstream stream(mesh_string);

		if (volume->unpackVolumeFaces(stream, data_size))
		{
			//load volume faces into decomposition buffer
			S32 vertex_count = 0;
			S32 index_count = 0;

			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				vertex_count += face.mNumVertices;
				index_count += face.mNumIndices;
			}

			d->mPhysicsShapeMesh = new LLVertexBuffer(LLVertexBuffer::MAP_VERTEX, 0);

			d->mPhysicsShapeMesh->allocateBuffer(vertex_count, index_count, true);

			LLStrider<LLVector3> pos;
			LLStrider<U16> idx;

			d->mPhysicsShapeMesh->getVertexStrider(pos);
			d->mPhysicsShapeMesh->getIndexStrider(idx);

			S32 idx_offset = 0;
			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
				if (idx_offset + face.mNumIndices > 65535)
				{ //avoid 16-bit index overflow
					continue;
				}

				LLVector4a::memcpyNonAliased16(pos[idx_offset].mV, face.mPositions[0].getF32ptr(), face.mNumVertices*sizeof(LLVector4a));
			
				for (S32 i = 0; i < face.mNumIndices; ++i)
				{
					*idx++ = face.mIndices[i] + idx_offset;
				}

				idx_offset += face.mNumVertices;
			}
		}
	}

	mDecompositionQ.push(d);
	return true;
}

LLMeshUploadThread::LLMeshUploadThread(LLMeshUploadThread::instance_list& data, LLVector3& scale, bool upload_textures,
										bool upload_skin, bool upload_joints)
: LLThread("mesh upload")
{
	mInstanceList = data;
	mUploadTextures = upload_textures;
	mUploadSkin = upload_skin;
	mUploadJoints = upload_joints;
	mMutex = new LLMutex(NULL);
	mCurlRequest = NULL;
	mPendingConfirmations = 0;
	mPendingUploads = 0;
	mPendingCost = 0;
	mFinished = false;
	mOrigin = gAgent.getPositionAgent();
	mHost = gAgent.getRegionHost();
	
	mUploadObjectAssetCapability = gAgent.getRegion()->getCapability("UploadObjectAsset");
	mNewInventoryCapability = gAgent.getRegion()->getCapability("NewFileAgentInventoryVariablePrice");

	mOrigin += gAgent.getAtAxis() * scale.magVec();
}

LLMeshUploadThread::~LLMeshUploadThread()
{

}

LLMeshUploadThread::DecompRequest::DecompRequest(LLModel* mdl, LLModel* base_model, LLMeshUploadThread* thread)
{
	mStage = "single_hull";
	mModel = mdl;
	mDecompID = &mdl->mDecompID;
	mBaseModel = base_model;
	mThread = thread;
	
	//copy out positions and indices
	if (mdl)
	{
		U16 index_offset = 0;

		mPositions.clear();
		mIndices.clear();
			
		//queue up vertex positions and indices
		for (S32 i = 0; i < mdl->getNumVolumeFaces(); ++i)
		{
			const LLVolumeFace& face = mdl->getVolumeFace(i);
			if (mPositions.size() + face.mNumVertices > 65535)
			{
				continue;
			}

			for (U32 j = 0; j < face.mNumVertices; ++j)
			{
				mPositions.push_back(LLVector3(face.mPositions[j].getF32ptr()));
			}

			for (U32 j = 0; j < face.mNumIndices; ++j)
			{
				mIndices.push_back(face.mIndices[j]+index_offset);
			}

			index_offset += face.mNumVertices;
		}
	}

	mThread->mFinalDecomp = this;
	mThread->mPhysicsComplete = false;
}

void LLMeshUploadThread::DecompRequest::completed()
{
	if (mThread->mFinalDecomp == this)
	{
		mThread->mPhysicsComplete = true;
	}

	if (mHull.size() != 1)
	{
		llerrs << "WTF?" << llendl;
	}

	mThread->mHullMap[mBaseModel] = mHull[0];
}

//called in the main thread.
void LLMeshUploadThread::preStart()
{
	//build map of LLModel refs to instances for callbacks
	for (instance_list::iterator iter = mInstanceList.begin(); iter != mInstanceList.end(); ++iter)
	{
		mInstance[iter->mModel].push_back(*iter);
	}
}

void LLMeshUploadThread::run()
{
	mCurlRequest = new LLCurlRequest();	

	std::set<LLViewerTexture* > textures;

	//populate upload queue with relevant models
	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;

		LLModelInstance& instance = *(iter->second.begin());

		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = instance.mLOD[i];
		}

		uploadModel(data);

		if (mUploadTextures)
		{
			for (std::vector<LLImportMaterial>::iterator material_iter = instance.mMaterial.begin();
				material_iter != instance.mMaterial.end(); ++material_iter)
			{

				if (textures.find(material_iter->mDiffuseMap.get()) == textures.end())
				{
					textures.insert(material_iter->mDiffuseMap.get());
					
					LLTextureUploadData data(material_iter->mDiffuseMap.get(), material_iter->mDiffuseMapLabel);
					uploadTexture(data);
				}
			}
		}

		//queue up models for hull generation
		LLModel* physics = NULL;

		if (data.mModel[LLModel::LOD_PHYSICS].notNull())
		{
			physics = data.mModel[LLModel::LOD_PHYSICS];
		}
		else if (data.mModel[LLModel::LOD_MEDIUM].notNull())
		{
			physics = data.mModel[LLModel::LOD_MEDIUM];
		}
		else
		{
			physics = data.mModel[LLModel::LOD_HIGH];
		}

		if (!physics)
		{
			llerrs << "WTF?" << llendl;
		}

		DecompRequest* request = new DecompRequest(physics, data.mBaseModel, this);
		gMeshRepo.mDecompThread->submitRequest(request);
	}

	while (!mPhysicsComplete)
	{
		apr_sleep(100);
	}

	//upload textures
	bool done = false;
	do
	{
		if (!mTextureQ.empty())
		{
			sendCostRequest(mTextureQ.front());
			mTextureQ.pop();
		}

		if (!mConfirmedTextureQ.empty())
		{
			doUploadTexture(mConfirmedTextureQ.front());
			mConfirmedTextureQ.pop();
		}

		mCurlRequest->process();

		done = mTextureQ.empty() && mConfirmedTextureQ.empty();
	}
	while (!done || mCurlRequest->getQueued() > 0);

	LLSD object_asset;
	object_asset["objects"] = LLSD::emptyArray();

	done = false;
	do 
	{
		static S32 count = 0;
		static F32 last_hundred = gFrameTimeSeconds;
		if (gFrameTimeSeconds - last_hundred > 1.f)
		{
			last_hundred = gFrameTimeSeconds;
			count = 0;
		}

		//how many requests to push before calling process
		const S32 PUSH_PER_PROCESS = 32;

		S32 tcount = llmin(count+PUSH_PER_PROCESS, 100);

		while (!mUploadQ.empty() && count < tcount)
		{ //send any pending upload requests
			mMutex->lock();
			LLMeshUploadData data = mUploadQ.front();
			mUploadQ.pop();
			mMutex->unlock();
			sendCostRequest(data);
			count++;
		}

		tcount = llmin(count+PUSH_PER_PROCESS, 100);
		
		while (!mConfirmedQ.empty() && count < tcount)
		{ //process any meshes that have been confirmed for upload
			LLMeshUploadData& data = mConfirmedQ.front();
			doUploadModel(data);
			mConfirmedQ.pop();
			count++;
		}
	
		tcount = llmin(count+PUSH_PER_PROCESS, 100);

		while (!mInstanceQ.empty() && count < tcount)
		{ //create any objects waiting for upload
			count++;
			object_asset["objects"].append(createObject(mInstanceQ.front()));
			mInstanceQ.pop();
		}
			
		mCurlRequest->process();
			
		done = mInstanceQ.empty() && mConfirmedQ.empty() && mUploadQ.empty();
	}
	while (!done || mCurlRequest->getQueued() > 0);

	delete mCurlRequest;
	mCurlRequest = NULL;

	// now upload the object asset
	std::string url = mUploadObjectAssetCapability;

	if (object_asset["objects"][0].has("permissions"))
	{ //copy permissions from first available object to be used for coalesced object
		object_asset["permissions"] = object_asset["objects"][0]["permissions"];
	}

	LLHTTPClient::post(url, object_asset, new LLHTTPClient::Responder());

	mFinished = true;
}

void LLMeshUploadThread::uploadModel(LLMeshUploadData& data)
{ //called from arbitrary thread
	{
		LLMutexLock lock(mMutex);
		mUploadQ.push(data);
	}
}

void LLMeshUploadThread::uploadTexture(LLTextureUploadData& data)
{ //called from mesh upload thread
	mTextureQ.push(data);	
}


static LLFastTimer::DeclareTimer FTM_NOTIFY_MESH_LOADED("Notify Loaded");
static LLFastTimer::DeclareTimer FTM_NOTIFY_MESH_UNAVAILABLE("Notify Unavailable");

void LLMeshRepoThread::notifyLoadedMeshes()
{
	while (!mLoadedQ.empty())
	{
		mMutex->lock();
		LoadedMesh mesh = mLoadedQ.front();
		mLoadedQ.pop();
		mMutex->unlock();
		
		if (mesh.mVolume && mesh.mVolume->getNumVolumeFaces() > 0)
		{
			gMeshRepo.notifyMeshLoaded(mesh.mMeshParams, mesh.mVolume);
		}
		else
		{
			gMeshRepo.notifyMeshUnavailable(mesh.mMeshParams, 
				LLVolumeLODGroup::getVolumeDetailFromScale(mesh.mVolume->getDetail()));
		}
	}

	while (!mUnavailableQ.empty())
	{
		mMutex->lock();
		LODRequest req = mUnavailableQ.front();
		mUnavailableQ.pop();
		mMutex->unlock();
		
		gMeshRepo.notifyMeshUnavailable(req.mMeshParams, req.mLOD);
	}

	while (!mSkinInfoQ.empty())
	{
		gMeshRepo.notifySkinInfoReceived(mSkinInfoQ.front());
		mSkinInfoQ.pop();
	}

	while (!mDecompositionQ.empty())
	{
		gMeshRepo.notifyDecompositionReceived(mDecompositionQ.front());
		mDecompositionQ.pop();
	}
}

S32 LLMeshRepoThread::getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod) 
{ //only ever called from main thread
	lod = llclamp(lod, 0, 3);

	LLMutexLock lock(mHeaderMutex);
	mesh_header_map::iterator iter = mMeshHeader.find(mesh_params.getSculptID());

	if (iter != mMeshHeader.end())
	{
		LLSD& header = iter->second;

		if (header.has("404"))
		{
			return -1;
		}

		if (header[header_lod[lod]]["size"].asInteger() > 0)
		{
			return lod;
		}

		//search down to find the next available lower lod
		for (S32 i = lod-1; i >= 0; --i)
		{
			if (header[header_lod[i]]["size"].asInteger() > 0)
			{
				return i;
			}
		}

		//search up to find then ext available higher lod
		for (S32 i = lod+1; i < 4; ++i)
		{
			if (header[header_lod[i]]["size"].asInteger() > 0)
			{
				return i;
			}
		}

		//header exists and no good lod found, treat as 404
		header["404"] = 1;
		return -1;
	}

	return lod;
}

U32 LLMeshRepoThread::getResourceCost(const LLUUID& mesh_id)
{
	LLMutexLock lock(mHeaderMutex);
	
	std::map<LLUUID, U32>::iterator iter = mMeshResourceCost.find(mesh_id);
	if (iter != mMeshResourceCost.end())
	{
		return iter->second;
	}

	return 0;
}

void LLMeshRepository::cacheOutgoingMesh(LLMeshUploadData& data, LLSD& header)
{
	mThread->mMeshHeader[data.mUUID] = header;

	// we cache the mesh for default parameters
	LLVolumeParams volume_params;
	volume_params.setType(LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE);
	volume_params.setSculptID(data.mUUID, LL_SCULPT_TYPE_MESH);

	for (U32 i = 0; i < 4; i++)
	{
		if (data.mModel[i].notNull())
		{
			LLPointer<LLVolume> volume = new LLVolume(volume_params, LLVolumeLODGroup::getVolumeScaleFromDetail(i));
			volume->copyVolumeFaces(data.mModel[i]);
		}
	}

}

void LLMeshLODResponder::completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
{

	LLMeshRepoThread::sActiveLODRequests--;
	S32 data_size = buffer->countAfter(channels.in(), NULL);

	if (status < 200 || status > 400)
	{
		llwarns << status << ": " << reason << llendl;
	}

	if (data_size < mRequestedBytes)
	{
		if (status == 499 || status == 503)
		{ //timeout or service unavailable, try again
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshLOD(mMeshParams, mLOD);
		}
		else
		{
			llwarns << "Unhandled status " << status << llendl;
		}
		return;
	}

	LLMeshRepository::sBytesReceived += mRequestedBytes;

	U8* data = NULL;

	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}

	if (gMeshRepo.mThread->lodReceived(mMeshParams, mLOD, data, data_size))
	{
		//good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshParams.getSculptID(), LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			file.seek(offset);
			file.write(data, size);
			LLMeshRepository::sCacheBytesWritten += size;
		}
	}

	delete [] data;
}

void LLMeshSkinInfoResponder::completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
{
	S32 data_size = buffer->countAfter(channels.in(), NULL);

	if (status < 200 || status > 400)
	{
		llwarns << status << ": " << reason << llendl;
	}

	if (data_size < mRequestedBytes)
	{
		if (status == 499 || status == 503)
		{ //timeout or service unavailable, try again
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshSkinInfo(mMeshID);
		}
		else
		{
			llwarns << "Unhandled status " << status << llendl;
		}
		return;
	}

	LLMeshRepository::sBytesReceived += mRequestedBytes;

	U8* data = NULL;

	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}

	if (gMeshRepo.mThread->skinInfoReceived(mMeshID, data, data_size))
	{
		//good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			file.seek(offset);
			file.write(data, size);
		}
	}

	delete [] data;
}

void LLMeshDecompositionResponder::completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
{
	S32 data_size = buffer->countAfter(channels.in(), NULL);

	if (status < 200 || status > 400)
	{
		llwarns << status << ": " << reason << llendl;
	}

	if (data_size < mRequestedBytes)
	{
		if (status == 499 || status == 503)
		{ //timeout or service unavailable, try again
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshDecomposition(mMeshID);
		}
		else
		{
			llwarns << "Unhandled status " << status << llendl;
		}
		return;
	}

	LLMeshRepository::sBytesReceived += mRequestedBytes;

	U8* data = NULL;

	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}

	if (gMeshRepo.mThread->decompositionReceived(mMeshID, data, data_size))
	{
		//good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			file.seek(offset);
			file.write(data, size);
		}
	}

	delete [] data;
}

void LLMeshPhysicsShapeResponder::completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
{
	S32 data_size = buffer->countAfter(channels.in(), NULL);

	if (status < 200 || status > 400)
	{
		llwarns << status << ": " << reason << llendl;
	}

	if (data_size < mRequestedBytes)
	{
		if (status == 499 || status == 503)
		{ //timeout or service unavailable, try again
			LLMeshRepository::sHTTPRetryCount++;
			gMeshRepo.mThread->loadMeshPhysicsShape(mMeshID);
		}
		else
		{
			llwarns << "Unhandled status " << status << llendl;
		}
		return;
	}

	LLMeshRepository::sBytesReceived += mRequestedBytes;

	U8* data = NULL;

	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}

	if (gMeshRepo.mThread->physicsShapeReceived(mMeshID, data, data_size))
	{
		//good fetch from sim, write to VFS for caching
		LLVFile file(gVFS, mMeshID, LLAssetType::AT_MESH, LLVFile::WRITE);

		S32 offset = mOffset;
		S32 size = mRequestedBytes;

		if (file.getSize() >= offset+size)
		{
			LLMeshRepository::sCacheBytesWritten += size;
			file.seek(offset);
			file.write(data, size);
		}
	}

	delete [] data;
}

void LLMeshHeaderResponder::completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
{
	LLMeshRepoThread::sActiveHeaderRequests--;
	if (status < 200 || status > 400)
	{
		//llwarns
		//	<< "Header responder failed with status: "
		//	<< status << ": " << reason << llendl;

		// 503 (service unavailable) or 499 (timeout)
		// can be due to server load and can be retried

		// TODO*: Add maximum retry logic, exponential backoff
		// and (somewhat more optional than the others) retries
		// again after some set period of time
		if (status == 503 || status == 499)
		{ //retry
			LLMeshRepository::sHTTPRetryCount++;
			LLMeshRepoThread::HeaderRequest req(mMeshParams);
			LLMutexLock lock(gMeshRepo.mThread->mMutex);
			gMeshRepo.mThread->mHeaderReqQ.push(req);

			return;
		}
	}

	S32 data_size = buffer->countAfter(channels.in(), NULL);

	U8* data = NULL;

	if (data_size > 0)
	{
		data = new U8[data_size];
		buffer->readAfter(channels.in(), NULL, data, data_size);
	}

	LLMeshRepository::sBytesReceived += llmin(data_size, 4096);

	if (!gMeshRepo.mThread->headerReceived(mMeshParams, data, data_size))
	{
		llwarns
			<< "Unable to parse mesh header: "
			<< status << ": " << reason << llendl;
	}
	else if (data && data_size > 0)
	{
		//header was successfully retrieved from sim, cache in vfs
		LLUUID mesh_id = mMeshParams.getSculptID();
		LLSD header = gMeshRepo.mThread->mMeshHeader[mesh_id];

		std::stringstream str;

		S32 lod_bytes = 0;

		for (U32 i = 0; i < LLModel::LOD_PHYSICS; ++i)
		{ //figure out how many bytes we'll need to reserve in the file
			std::string lod_name = header_lod[i];
			lod_bytes = llmax(lod_bytes, header[lod_name]["offset"].asInteger()+header[lod_name]["size"].asInteger());
		}
		
		//just in case skin info or decomposition is at the end of the file (which it shouldn't be)
		lod_bytes = llmax(lod_bytes, header["skin"]["offset"].asInteger() + header["skin"]["size"].asInteger());
		lod_bytes = llmax(lod_bytes, header["decomposition"]["offset"].asInteger() + header["decomposition"]["size"].asInteger());

		S32 header_bytes = (S32) gMeshRepo.mThread->mMeshHeaderSize[mesh_id];
		S32 bytes = lod_bytes + header_bytes; 

		
		//it's possible for the remote asset to have more data than is needed for the local cache
		//only allocate as much space in the VFS as is needed for the local cache
		data_size = llmin(data_size, bytes);

		LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH, LLVFile::WRITE);
		if (file.getMaxSize() >= bytes || file.setMaxSize(bytes))
		{
			LLMeshRepository::sCacheBytesWritten += data_size;

			file.write((const U8*) data, data_size);
			
			//zero out the rest of the file 
			U8 block[4096];
			memset(block, 0, 4096);

			while (bytes-file.tell() > 4096)
			{
				file.write(block, 4096);
			}

			S32 remaining = bytes-file.tell();

			if (remaining < 0 || remaining > 4096)
			{
				llerrs << "Bad padding of mesh asset cache entry." << llendl;
			}

			if (remaining > 0)
			{
				file.write(block, remaining);
			}
		}
	}

	delete [] data;
}


LLMeshRepository::LLMeshRepository()
: mMeshMutex(NULL),
  mMeshThreadCount(0),
  mThread(NULL)
{

}

void LLMeshRepository::init()
{
	mMeshMutex = new LLMutex(NULL);
	
	LLConvexDecomposition::getInstance()->initSystem();

	mDecompThread = new LLPhysicsDecomp();
	mDecompThread->start();

	while (!mDecompThread->mInited)
	{ //wait for physics decomp thread to init
		apr_sleep(100);
	}

	
	
	mThread = new LLMeshRepoThread();
	mThread->start();
}

void LLMeshRepository::shutdown()
{
	llinfos << "Shutting down mesh repository." << llendl;

	mThread->mSignal->signal();
	
	while (!mThread->isStopped())
	{
		apr_sleep(10);
	}
	delete mThread;
	mThread = NULL;

	for (U32 i = 0; i < mUploads.size(); ++i)
	{
		llinfos << "Waiting for pending mesh upload " << i << "/" << mUploads.size() << llendl;
		while (!mUploads[i]->isStopped())
		{
			apr_sleep(10);
		}
		delete mUploads[i];
	}

	mUploads.clear();

	delete mMeshMutex;
	mMeshMutex = NULL;

	llinfos << "Shutting down decomposition system." << llendl;

	if (mDecompThread)
	{
		mDecompThread->shutdown();		
		delete mDecompThread;
		mDecompThread = NULL;
	}

	LLConvexDecomposition::quitSystem();
}

//called in the main thread.
S32 LLMeshRepository::update()
{
	if(mUploadWaitList.empty())
	{
		return 0 ;
	}

	S32 size = mUploadWaitList.size() ;
	for (S32 i = 0; i < size; ++i)
	{
		mUploads.push_back(mUploadWaitList[i]);
		mUploadWaitList[i]->preStart() ;
		mUploadWaitList[i]->start() ;
	}
	mUploadWaitList.clear() ;

	return size ;
}

S32 LLMeshRepository::loadMesh(LLVOVolume* vobj, const LLVolumeParams& mesh_params, S32 detail, S32 last_lod)
{
	if (detail < 0 || detail > 4)
	{
		return detail;
	}

	LLFastTimer t(FTM_LOAD_MESH); 

	{
		LLMutexLock lock(mMeshMutex);
		//add volume to list of loading meshes
		mesh_load_map::iterator iter = mLoadingMeshes[detail].find(mesh_params);
		if (iter != mLoadingMeshes[detail].end())
		{ //request pending for this mesh, append volume id to list
			iter->second.insert(vobj->getID());
		}
		else
		{
			//first request for this mesh
			mLoadingMeshes[detail][mesh_params].insert(vobj->getID());
			mPendingRequests.push_back(LLMeshRepoThread::LODRequest(mesh_params, detail));
		}
	}

	//do a quick search to see if we can't display something while we wait for this mesh to load
	LLVolume* volume = vobj->getVolume();

	if (volume)
	{
		if (volume->getNumVolumeFaces() == 0 && !volume->isTetrahedron())
		{
			volume->makeTetrahedron();
		}

		LLVolumeParams params = volume->getParams();

		LLVolumeLODGroup* group = LLPrimitive::getVolumeManager()->getGroup(params);

		if (group)
		{
			//first, see if last_lod is available (don't transition down to avoid funny popping a la SH-641)
			if (last_lod >= 0)
			{
				LLVolume* lod = group->refLOD(last_lod);
				if (lod && !lod->isTetrahedron() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return last_lod;
				}
				group->derefLOD(lod);
			}

			//next, see what the next lowest LOD available might be
			for (S32 i = detail-1; i >= 0; --i)
			{
				LLVolume* lod = group->refLOD(i);
				if (lod && !lod->isTetrahedron() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}

				group->derefLOD(lod);
			}

			//no lower LOD is a available, is a higher lod available?
			for (S32 i = detail+1; i < 4; ++i)
			{
				LLVolume* lod = group->refLOD(i);
				if (lod && !lod->isTetrahedron() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}

				group->derefLOD(lod);
			}
		}
		else
		{
			llerrs << "WTF?" << llendl;
		}
	}

	return detail;
}

static LLFastTimer::DeclareTimer FTM_START_MESH_THREAD("Start Thread");
static LLFastTimer::DeclareTimer FTM_LOAD_MESH_LOD("Load LOD");
static LLFastTimer::DeclareTimer FTM_MESH_LOCK1("Lock 1");
static LLFastTimer::DeclareTimer FTM_MESH_LOCK2("Lock 2");

void LLMeshRepository::notifyLoadedMeshes()
{ //called from main thread

	LLMeshRepoThread::sMaxConcurrentRequests = gSavedSettings.getU32("MeshMaxConcurrentRequests");

	//clean up completed upload threads
	for (std::vector<LLMeshUploadThread*>::iterator iter = mUploads.begin(); iter != mUploads.end(); )
	{
		LLMeshUploadThread* thread = *iter;

		if (thread->isStopped() && thread->finished())
		{
			iter = mUploads.erase(iter);
			delete thread;
		}
		else
		{
			++iter;
		}
	}

	//update inventory
	if (!mInventoryQ.empty())
	{
		LLMutexLock lock(mMeshMutex);
		while (!mInventoryQ.empty())
		{
			inventory_data& data = mInventoryQ.front();

			LLAssetType::EType asset_type = LLAssetType::lookup(data.mPostData["asset_type"].asString());
			LLInventoryType::EType inventory_type = LLInventoryType::lookup(data.mPostData["inventory_type"].asString());

			on_new_single_inventory_upload_complete(
				asset_type,
				inventory_type,
				data.mPostData["asset_type"].asString(),
				data.mPostData["folder_id"].asUUID(),
				data.mPostData["name"],
				data.mPostData["description"],
				data.mResponse,
				0);
			
			mInventoryQ.pop();
		}
	}

	//call completed callbacks on finished decompositions
	mDecompThread->notifyCompleted();
	
	if (!mThread->mWaiting)
	{ //curl thread is churning, wait for it to go idle
		return;
	}

	static std::string region_name("never name a region this");

	if (gAgent.getRegion())
	{ //update capability url 
		if (gAgent.getRegion()->getName() != region_name && gAgent.getRegion()->capabilitiesReceived())
		{
			region_name = gAgent.getRegion()->getName();
		
			mGetMeshCapability = gAgent.getRegion()->getCapability("GetMesh");
		}
	}

	LLFastTimer t(FTM_MESH_UPDATE);

	{
		LLFastTimer t(FTM_MESH_LOCK1);
		mMeshMutex->lock();	
	}

	{
		LLFastTimer t(FTM_MESH_LOCK2);
		mThread->mMutex->lock();
	}
	
	//popup queued error messages from background threads
	while (!mUploadErrorQ.empty())
	{
		LLNotificationsUtil::add("MeshUploadError", mUploadErrorQ.front());
		mUploadErrorQ.pop();
	}

	S32 push_count = LLMeshRepoThread::sMaxConcurrentRequests-(LLMeshRepoThread::sActiveHeaderRequests+LLMeshRepoThread::sActiveLODRequests);

	if (push_count > 0)
	{
		//calculate "score" for pending requests

		//create score map
		std::map<LLUUID, F32> score_map;

		for (U32 i = 0; i < 4; ++i)
		{
			for (mesh_load_map::iterator iter = mLoadingMeshes[i].begin();  iter != mLoadingMeshes[i].end(); ++iter)
			{
				F32 max_score = 0.f;
				for (std::set<LLUUID>::iterator obj_iter = iter->second.begin(); obj_iter != iter->second.end(); ++obj_iter)
				{
					LLViewerObject* object = gObjectList.findObject(*obj_iter);

					if (object)
					{
						LLDrawable* drawable = object->mDrawable;
						if (drawable)
						{
							F32 cur_score = drawable->getRadius()/llmax(drawable->mDistanceWRTCamera, 1.f);
							max_score = llmax(max_score, cur_score);
						}
					}
				}
				
				score_map[iter->first.getSculptID()] = max_score;
			}
		}

		//set "score" for pending requests
		for (std::vector<LLMeshRepoThread::LODRequest>::iterator iter = mPendingRequests.begin(); iter != mPendingRequests.end(); ++iter)
		{
			iter->mScore = score_map[iter->mMeshParams.getSculptID()];
		}

		//sort by "score"
		std::sort(mPendingRequests.begin(), mPendingRequests.end(), LLMeshRepoThread::CompareScoreGreater());

		while (!mPendingRequests.empty() && push_count > 0)
		{
			LLFastTimer t(FTM_LOAD_MESH_LOD);
			LLMeshRepoThread::LODRequest& request = mPendingRequests.front();
			mThread->loadMeshLOD(request.mMeshParams, request.mLOD);
			mPendingRequests.erase(mPendingRequests.begin());
			push_count--;
		}
	}

	//send skin info requests
	while (!mPendingSkinRequests.empty())
	{
		mThread->loadMeshSkinInfo(mPendingSkinRequests.front());
		mPendingSkinRequests.pop();
	}
	
	//send decomposition requests
	while (!mPendingDecompositionRequests.empty())
	{
		mThread->loadMeshDecomposition(mPendingDecompositionRequests.front());
		mPendingDecompositionRequests.pop();
	}
	
	//send physics shapes decomposition requests
	while (!mPendingPhysicsShapeRequests.empty())
	{
		mThread->loadMeshPhysicsShape(mPendingPhysicsShapeRequests.front());
		mPendingPhysicsShapeRequests.pop();
	}
	
	mThread->notifyLoadedMeshes();

	mThread->mMutex->unlock();
	mMeshMutex->unlock();

	mThread->mSignal->signal();
}

void LLMeshRepository::notifySkinInfoReceived(LLMeshSkinInfo& info)
{
	mSkinMap[info.mMeshID] = info;
	mLoadingSkins.erase(info.mMeshID);
}

void LLMeshDecomposition::merge(const LLMeshDecomposition* rhs)
{
	if (!rhs)
	{
		return;
	}

	if (mMeshID != rhs->mMeshID)
	{
		llerrs << "Attempted to merge with decomposition of some other mesh." << llendl;
	}

	if (mBaseHull.empty())
	{ //take base hull and decomposition from rhs
		mHull = rhs->mHull;
		mBaseHull = rhs->mBaseHull;
		mMesh = rhs->mMesh;
		mBaseHullMesh = rhs->mBaseHullMesh;
	}

	if (mPhysicsShapeMesh.isNull())
	{ //take physics shape mesh from rhs
		mPhysicsShapeMesh = rhs->mPhysicsShapeMesh;
	}
}

void LLMeshRepository::notifyDecompositionReceived(LLMeshDecomposition* decomp)
{
	decomposition_map::iterator iter = mDecompositionMap.find(decomp->mMeshID);
	if (iter == mDecompositionMap.end())
	{ //just insert decomp into map
		mDecompositionMap[decomp->mMeshID] = decomp;
	}
	else
	{ //merge decomp with existing entry
		iter->second->merge(decomp);
		delete decomp;
	}

	mLoadingDecompositions.erase(decomp->mMeshID);
}

void LLMeshRepository::notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume)
{ //called from main thread
	S32 detail = LLVolumeLODGroup::getVolumeDetailFromScale(volume->getDetail());

	//get list of objects waiting to be notified this mesh is loaded
	mesh_load_map::iterator obj_iter = mLoadingMeshes[detail].find(mesh_params);

	if (volume && obj_iter != mLoadingMeshes[detail].end())
	{
		//make sure target volume is still valid
		if (volume->getNumVolumeFaces() <= 0)
		{
			llwarns << "Mesh loading returned empty volume." << llendl;
			volume->makeTetrahedron();
		}
		
		{ //update system volume
			LLVolume* sys_volume = LLPrimitive::getVolumeManager()->refVolume(mesh_params, detail);
			if (sys_volume)
			{
				sys_volume->copyVolumeFaces(volume);
				LLPrimitive::getVolumeManager()->unrefVolume(sys_volume);
			}
			else
			{
				llwarns << "Couldn't find system volume for given mesh." << llendl;
			}
		}

		//notify waiting LLVOVolume instances that their requested mesh is available
		for (std::set<LLUUID>::iterator vobj_iter = obj_iter->second.begin(); vobj_iter != obj_iter->second.end(); ++vobj_iter)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*vobj_iter);
			if (vobj)
			{
				vobj->notifyMeshLoaded();
			}
		}
		
		mLoadingMeshes[detail].erase(mesh_params);
	}
}

void LLMeshRepository::notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod)
{ //called from main thread
	//get list of objects waiting to be notified this mesh is loaded
	mesh_load_map::iterator obj_iter = mLoadingMeshes[lod].find(mesh_params);

	F32 detail = LLVolumeLODGroup::getVolumeScaleFromDetail(lod);

	if (obj_iter != mLoadingMeshes[lod].end())
	{
		for (std::set<LLUUID>::iterator vobj_iter = obj_iter->second.begin(); vobj_iter != obj_iter->second.end(); ++vobj_iter)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*vobj_iter);
			if (vobj)
			{
				LLVolume* obj_volume = vobj->getVolume();

				if (obj_volume && 
					obj_volume->getDetail() == detail &&
					obj_volume->getParams() == mesh_params)
				{ //should force volume to find most appropriate LOD
					vobj->setVolume(obj_volume->getParams(), lod);
				}
			}
		}
		
		mLoadingMeshes[lod].erase(mesh_params);
	}
}

S32 LLMeshRepository::getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod)
{ 
	return mThread->getActualMeshLOD(mesh_params, lod);
}

U32 LLMeshRepository::calcResourceCost(LLSD& header)
{
	U32 bytes = 0;

	for (U32 i = 0; i < 4; i++)
	{
		bytes += header[header_lod[i]]["size"].asInteger();
	}

	bytes += header["skin"]["size"].asInteger();

	return bytes/4096 + 1;
}

U32 LLMeshRepository::getResourceCost(const LLUUID& mesh_id)
{
	return mThread->getResourceCost(mesh_id);
}

const LLMeshSkinInfo* LLMeshRepository::getSkinInfo(const LLUUID& mesh_id)
{
	if (mesh_id.notNull())
	{
		skin_map::iterator iter = mSkinMap.find(mesh_id);
		if (iter != mSkinMap.end())
		{
			return &(iter->second);
		}
		
		//no skin info known about given mesh, try to fetch it
		{
			LLMutexLock lock(mMeshMutex);
			//add volume to list of loading meshes
			std::set<LLUUID>::iterator iter = mLoadingSkins.find(mesh_id);
			if (iter == mLoadingSkins.end())
			{ //no request pending for this skin info
				mLoadingSkins.insert(mesh_id);
				mPendingSkinRequests.push(mesh_id);
			}
		}
	}

	return NULL;
}

void LLMeshRepository::fetchPhysicsShape(const LLUUID& mesh_id)
{
	if (mesh_id.notNull())
	{
		LLMeshDecomposition* decomp = NULL;
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			decomp = iter->second;
		}
		
		//decomposition block hasn't been fetched yet
		if (!decomp || decomp->mPhysicsShapeMesh.isNull())
		{
			LLMutexLock lock(mMeshMutex);
			//add volume to list of loading meshes
			std::set<LLUUID>::iterator iter = mLoadingPhysicsShapes.find(mesh_id);
			if (iter == mLoadingPhysicsShapes.end())
			{ //no request pending for this skin info
				mLoadingPhysicsShapes.insert(mesh_id);
				mPendingPhysicsShapeRequests.push(mesh_id);
			}
		}
	}

}

const LLMeshDecomposition* LLMeshRepository::getDecomposition(const LLUUID& mesh_id)
{
	LLMeshDecomposition* ret = NULL;

	if (mesh_id.notNull())
	{
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			ret = iter->second;
		}
		
		//decomposition block hasn't been fetched yet
		if (!ret || ret->mBaseHullMesh.isNull())
		{
			LLMutexLock lock(mMeshMutex);
			//add volume to list of loading meshes
			std::set<LLUUID>::iterator iter = mLoadingDecompositions.find(mesh_id);
			if (iter == mLoadingDecompositions.end())
			{ //no request pending for this skin info
				mLoadingDecompositions.insert(mesh_id);
				mPendingDecompositionRequests.push(mesh_id);
			}
		}
	}

	return ret;
}

void LLMeshRepository::buildHull(const LLVolumeParams& params, S32 detail)
{
	LLVolume* volume = LLPrimitive::sVolumeManager->refVolume(params, detail);

	if (!volume->mHullPoints)
	{
		//all default params
		//execute first stage
		//set simplify mode to retain
		//set retain percentage to zero
		//run second stage
	}

	LLPrimitive::sVolumeManager->unrefVolume(volume);
}

bool LLMeshRepository::hasPhysicsShape(const LLUUID& mesh_id)
{
	LLSD mesh = mThread->getMeshHeader(mesh_id);
	return mesh.has("physics_shape") && mesh["physics_shape"].has("size") && (mesh["physics_shape"]["size"].asInteger() > 0);
}

const LLSD& LLMeshRepository::getMeshHeader(const LLUUID& mesh_id)
{
	return mThread->getMeshHeader(mesh_id);
}

const LLSD& LLMeshRepoThread::getMeshHeader(const LLUUID& mesh_id)
{
	static LLSD dummy_ret;
	if (mesh_id.notNull())
	{
		LLMutexLock lock(mHeaderMutex);
		mesh_header_map::iterator iter = mMeshHeader.find(mesh_id);
		if (iter != mMeshHeader.end())
		{
			return iter->second;
		}
	}

	return dummy_ret;
}


void LLMeshRepository::uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures,
									bool upload_skin, bool upload_joints)
{
	LLMeshUploadThread* thread = new LLMeshUploadThread(data, scale, upload_textures, upload_skin, upload_joints);
	mUploadWaitList.push_back(thread);
}

S32 LLMeshRepository::getMeshSize(const LLUUID& mesh_id, S32 lod)
{
	if (mThread)
	{
		LLMeshRepoThread::mesh_header_map::iterator iter = mThread->mMeshHeader.find(mesh_id);
		if (iter != mThread->mMeshHeader.end())
		{
			LLSD& header = iter->second;

			if (header.has("404"))
			{
				return -1;
			}

			S32 size = header[header_lod[lod]]["size"].asInteger();
			return size;
		}

	}

	return -1;

}

void LLMeshUploadThread::sendCostRequest(LLMeshUploadData& data)
{
	//write model file to memory buffer
	std::stringstream ostr;

	LLModel::convex_hull_decomposition& decomp =
		data.mModel[LLModel::LOD_PHYSICS].notNull() ? 
		data.mModel[LLModel::LOD_PHYSICS]->mConvexHullDecomp : 
		data.mBaseModel->mConvexHullDecomp;

	LLModel::hull dummy_hull;

	LLSD header = LLModel::writeModel(
		ostr,
		data.mModel[LLModel::LOD_PHYSICS],
		data.mModel[LLModel::LOD_HIGH],
		data.mModel[LLModel::LOD_MEDIUM],
		data.mModel[LLModel::LOD_LOW],
		data.mModel[LLModel::LOD_IMPOSTOR], 
		decomp,
		dummy_hull,
		mUploadSkin,
		mUploadJoints,
		true);

	std::string desc = data.mBaseModel->mLabel;
	
	// Grab the total vertex count of the model
	// along with other information for the "asset_resources" map
	// to send to the server.
	LLSD asset_resources = LLSD::emptyMap();


	std::string url = mNewInventoryCapability; 

	if (!url.empty())
	{
		LLSD body = generate_new_resource_upload_capability_body(
			LLAssetType::AT_MESH,
			desc,
			desc,
			LLFolderType::FT_MESH,
			LLInventoryType::IT_MESH,
			LLFloaterPerms::getNextOwnerPerms(),
			LLFloaterPerms::getGroupPerms(),
			LLFloaterPerms::getEveryonePerms());

		body["asset_resources"] = asset_resources;

		mPendingConfirmations++;
		LLCurlRequest::headers_t headers;

		data.mPostData = body;

		mCurlRequest->post(url, headers, body, new LLMeshCostResponder(data, this));
	}	
}

void LLMeshUploadThread::sendCostRequest(LLTextureUploadData& data)
{
	if (data.mTexture && data.mTexture->getDiscardLevel() >= 0)
	{
		LLSD asset_resources = LLSD::emptyMap();

		std::string url = mNewInventoryCapability; 

		if (!url.empty())
		{
			LLSD body = generate_new_resource_upload_capability_body(
				LLAssetType::AT_TEXTURE,
				data.mLabel,
				data.mLabel,
				LLFolderType::FT_TEXTURE,
				LLInventoryType::IT_TEXTURE,
				LLFloaterPerms::getNextOwnerPerms(),
				LLFloaterPerms::getGroupPerms(),
				LLFloaterPerms::getEveryonePerms());

			body["asset_resources"] = asset_resources;

			mPendingConfirmations++;
			LLCurlRequest::headers_t headers;
			
			data.mPostData = body;
			mCurlRequest->post(url, headers, body, new LLTextureCostResponder(data, this));
		}	
	}
}


void LLMeshUploadThread::doUploadModel(LLMeshUploadData& data)
{
	if (!data.mRSVP.empty())
	{
		std::stringstream ostr;

		LLModel::convex_hull_decomposition& decomp =
			data.mModel[LLModel::LOD_PHYSICS].notNull() ? 
			data.mModel[LLModel::LOD_PHYSICS]->mConvexHullDecomp : 
			data.mBaseModel->mConvexHullDecomp;

		LLModel::writeModel(
			ostr,  
			data.mModel[LLModel::LOD_PHYSICS],
			data.mModel[LLModel::LOD_HIGH],
			data.mModel[LLModel::LOD_MEDIUM],
			data.mModel[LLModel::LOD_LOW],
			data.mModel[LLModel::LOD_IMPOSTOR], 
			decomp,
			mHullMap[data.mBaseModel],
			mUploadSkin,
			mUploadJoints);

		data.mAssetData = ostr.str();

		LLCurlRequest::headers_t headers;
		mPendingUploads++;

		mCurlRequest->post(data.mRSVP, headers, data.mAssetData, new LLMeshUploadResponder(data, this));
	}
}

void LLMeshUploadThread::doUploadTexture(LLTextureUploadData& data)
{
	if (!data.mRSVP.empty())
	{
		std::stringstream ostr;
		
		if (!data.mTexture->isRawImageValid())
		{
			data.mTexture->reloadRawImage(data.mTexture->getDiscardLevel());
		}

		LLPointer<LLImageJ2C> upload_file = LLViewerTextureList::convertToUploadFile(data.mTexture->getRawImage());
		
		ostr.write((const char*) upload_file->getData(), upload_file->getDataSize());

		data.mAssetData = ostr.str();

		LLCurlRequest::headers_t headers;
		mPendingUploads++;

		mCurlRequest->post(data.mRSVP, headers, data.mAssetData, new LLTextureUploadResponder(data, this));
	}
}


void LLMeshUploadThread::onModelUploaded(LLMeshUploadData& data)
{
	createObjects(data);
}

void LLMeshUploadThread::onTextureUploaded(LLTextureUploadData& data)
{
	mTextureMap[data.mTexture] = data;
}


void LLMeshUploadThread::createObjects(LLMeshUploadData& data)
{
	instance_list& instances = mInstance[data.mBaseModel];

	for (instance_list::iterator iter = instances.begin(); iter != instances.end(); ++iter)
	{ //create prims that reference given mesh
		LLModelInstance& instance = *iter;
		instance.mMeshID = data.mUUID;
		mInstanceQ.push(instance);
	}
}

LLSD LLMeshUploadThread::createObject(LLModelInstance& instance)
{
	LLMatrix4 transformation = instance.mTransform;

	if (instance.mMeshID.isNull())
	{
		llerrs << "WTF?" << llendl;
	}

	// check for reflection
	BOOL reflected = (transformation.determinant() < 0);

	// compute position
	LLVector3 position = LLVector3(0, 0, 0) * transformation;

	// compute scale
	LLVector3 x_transformed = LLVector3(1, 0, 0) * transformation - position;
	LLVector3 y_transformed = LLVector3(0, 1, 0) * transformation - position;
	LLVector3 z_transformed = LLVector3(0, 0, 1) * transformation - position;
	F32 x_length = x_transformed.normalize();
	F32 y_length = y_transformed.normalize();
	F32 z_length = z_transformed.normalize();
	LLVector3 scale = LLVector3(x_length, y_length, z_length);

    // adjust for "reflected" geometry
	LLVector3 x_transformed_reflected = x_transformed;
	if (reflected)
	{
		x_transformed_reflected *= -1.0;
	}
	
	// compute rotation
	LLMatrix3 rotation_matrix;
	rotation_matrix.setRows(x_transformed_reflected, y_transformed, z_transformed);
	LLQuaternion quat_rotation = rotation_matrix.quaternion();
	quat_rotation.normalize(); // the rotation_matrix might not have been orthoginal.  make it so here.
	LLVector3 euler_rotation;
	quat_rotation.getEulerAngles(&euler_rotation.mV[VX], &euler_rotation.mV[VY], &euler_rotation.mV[VZ]);

	//
	// build parameter block to construct this prim
	//
	
	LLSD object_params;

	// create prim

	// set volume params
	U8 sculpt_type = LL_SCULPT_TYPE_MESH;
	if (reflected)
	{
		sculpt_type |= LL_SCULPT_FLAG_MIRROR;
	}
	LLVolumeParams  volume_params;
	volume_params.setType( LL_PCODE_PROFILE_SQUARE, LL_PCODE_PATH_LINE );
	volume_params.setBeginAndEndS( 0.f, 1.f );
	volume_params.setBeginAndEndT( 0.f, 1.f );
	volume_params.setRatio  ( 1, 1 );
	volume_params.setShear  ( 0, 0 );
	volume_params.setSculptID(instance.mMeshID, sculpt_type);
	object_params["shape"] = volume_params.asLLSD();

	object_params["material"] = LL_MCODE_WOOD;

	object_params["group-id"] = gAgent.getGroupID();
	object_params["pos"] = ll_sd_from_vector3(position + mOrigin);
	object_params["rotation"] = ll_sd_from_quaternion(quat_rotation);
	object_params["scale"] = ll_sd_from_vector3(scale);
	object_params["name"] = instance.mModel->getName();

	// load material from dae file
	object_params["facelist"] = LLSD::emptyArray();
	for (S32 i = 0; i < instance.mMaterial.size(); i++)
	{
		LLTextureEntry te;
		LLImportMaterial& mat = instance.mMaterial[i];

		te.setColor(mat.mDiffuseColor);

		LLUUID diffuse_id = mTextureMap[mat.mDiffuseMap].mUUID;

		if (diffuse_id.notNull())
		{
			te.setID(diffuse_id);
		}
		else
		{
			te.setID(LLUUID("5748decc-f629-461c-9a36-a35a221fe21f")); // blank texture
		}

		te.setFullbright(mat.mFullbright);

		object_params["facelist"][i] = te.asLLSD();
	}

	// set extra parameters
	LLSculptParams sculpt_params;
	sculpt_params.setSculptTexture(instance.mMeshID);
	sculpt_params.setSculptType(sculpt_type);
	U8 buffer[MAX_OBJECT_PARAMS_SIZE+1];
	LLDataPackerBinaryBuffer dp(buffer, MAX_OBJECT_PARAMS_SIZE);
	sculpt_params.pack(dp);
	std::vector<U8> v(dp.getCurrentSize());
	memcpy(&v[0], buffer, dp.getCurrentSize());
	LLSD extra_parameter;
	extra_parameter["extra_parameter"] = sculpt_params.mType;
	extra_parameter["param_data"] = v;
	object_params["extra_parameters"].append(extra_parameter);

	LLPermissions perm;
	perm.setOwnerAndGroup(gAgent.getID(), gAgent.getID(), LLUUID::null, false);
	perm.setCreator(gAgent.getID());

	perm.initMasks(PERM_ITEM_UNRESTRICTED | PERM_MOVE, //base
				   PERM_ITEM_UNRESTRICTED | PERM_MOVE, //owner
				   LLFloaterPerms::getEveryonePerms(),
				   LLFloaterPerms::getGroupPerms(),
				   LLFloaterPerms::getNextOwnerPerms());
		
	object_params["permissions"] = ll_create_sd_from_permissions(perm);

	object_params["physics_shape_type"] = (U8)(LLViewerObject::PHYSICS_SHAPE_CONVEX_HULL);

	return object_params;
}

void LLMeshUploadThread::priceResult(LLMeshUploadData& data, const LLSD& content)
{
	mPendingCost += content["upload_price"].asInteger();
	data.mRSVP = content["rsvp"].asString();

	mConfirmedQ.push(data);
}

void LLMeshUploadThread::priceResult(LLTextureUploadData& data, const LLSD& content)
{
	mPendingCost += content["upload_price"].asInteger();
	data.mRSVP = content["rsvp"].asString();

	mConfirmedTextureQ.push(data);
}


bool LLImportMaterial::operator<(const LLImportMaterial &rhs) const
{
	if (mDiffuseMap != rhs.mDiffuseMap)
	{
		return mDiffuseMap < rhs.mDiffuseMap;
	}

	if (mDiffuseMapFilename != rhs.mDiffuseMapFilename)
	{
		return mDiffuseMapFilename < rhs.mDiffuseMapFilename;
	}

	if (mDiffuseMapLabel != rhs.mDiffuseMapLabel)
	{
		return mDiffuseMapLabel < rhs.mDiffuseMapLabel;
	}

	if (mDiffuseColor != rhs.mDiffuseColor)
	{
		return mDiffuseColor < rhs.mDiffuseColor;
	}

	return mFullbright < rhs.mFullbright;
}


void LLMeshRepository::updateInventory(inventory_data data)
{
	LLMutexLock lock(mMeshMutex);
	mInventoryQ.push(data);
}

void LLMeshRepository::uploadError(LLSD& args)
{
	LLMutexLock lock(mMeshMutex);
	mUploadErrorQ.push(args);
}

//static
F32 LLMeshRepository::getStreamingCost(const LLSD& header, F32 radius)
{
	F32 dlowest = llmin(radius/0.06f, 256.f);
	F32 dlow = llmin(radius/0.24f, 256.f);
	F32 dmid = llmin(radius/1.0f, 256.f);
	F32 dhigh = 0.f;


	F32 bytes_lowest = header["lowest_lod"]["size"].asReal()/1024.f;
	F32 bytes_low = header["low_lod"]["size"].asReal()/1024.f;
	F32 bytes_mid = header["medium_lod"]["size"].asReal()/1024.f;
	F32 bytes_high = header["high_lod"]["size"].asReal()/1024.f;

	if (bytes_high == 0.f)
	{
		return 0.f;
	}

	if (bytes_mid == 0.f)
	{
		bytes_mid = bytes_high;
	}

	if (bytes_low == 0.f)
	{
		bytes_low = bytes_mid;
	}

	if (bytes_lowest == 0.f)
	{
		bytes_lowest = bytes_low;
	}

	F32 cost = 0.f;
	cost += llmax(256.f-dlowest, 1.f)/32.f*bytes_lowest;
	cost += llmax(dlowest-dlow, 1.f)/32.f*bytes_low;
	cost += llmax(dlow-dmid, 1.f)/32.f*bytes_mid;
	cost += llmax(dmid-dhigh, 1.f)/32.f*bytes_high;

	cost *= gSavedSettings.getF32("MeshStreamingCostScaler");
	return cost;
}


LLPhysicsDecomp::LLPhysicsDecomp()
: LLThread("Physics Decomp")
{
	mInited = false;
	mQuitting = false;
	mDone = false;

	mSignal = new LLCondition(NULL);
	mMutex = new LLMutex(NULL);
}

LLPhysicsDecomp::~LLPhysicsDecomp()
{
	shutdown();

	delete mSignal;
	mSignal = NULL;
	delete mMutex;
	mMutex = NULL;
}

void LLPhysicsDecomp::shutdown()
{
	if (mSignal)
	{
		mQuitting = true;
		mSignal->signal();

		while (!isStopped())
		{
			apr_sleep(10);
		}
	}
}

void LLPhysicsDecomp::submitRequest(LLPhysicsDecomp::Request* request)
{
	LLMutexLock lock(mMutex);
	mRequestQ.push(request);
	mSignal->signal();
}

//static
S32 LLPhysicsDecomp::llcdCallback(const char* status, S32 p1, S32 p2)
{	
	if (gMeshRepo.mDecompThread && gMeshRepo.mDecompThread->mCurRequest.notNull())
	{
		return gMeshRepo.mDecompThread->mCurRequest->statusCallback(status, p1, p2);
	}

	return 1;
}

void LLPhysicsDecomp::setMeshData(LLCDMeshData& mesh)
{
	mesh.mVertexBase = mCurRequest->mPositions[0].mV;
	mesh.mVertexStrideBytes = 12;
	mesh.mNumVertices = mCurRequest->mPositions.size();

	mesh.mIndexType = LLCDMeshData::INT_16;
	mesh.mIndexBase = &(mCurRequest->mIndices[0]);
	mesh.mIndexStrideBytes = 6;
	
	mesh.mNumTriangles = mCurRequest->mIndices.size()/3;

	LLCDResult ret = LLCD_OK;
	if (LLConvexDecomposition::getInstance() != NULL)
	{
		ret  = LLConvexDecomposition::getInstance()->setMeshData(&mesh);
	}

	if (ret)
	{
		llerrs << "Convex Decomposition thread valid but could not set mesh data" << llendl;
	}
}

void LLPhysicsDecomp::doDecomposition()
{
	LLCDMeshData mesh;
	S32 stage = mStageID[mCurRequest->mStage];

	//load data intoLLCD
	if (stage == 0)
	{
		setMeshData(mesh);
	}
		
	//build parameter map
	std::map<std::string, const LLCDParam*> param_map;

	static const LLCDParam* params = NULL;
	static S32 param_count = 0;
	if (!params)
	{
		param_count = LLConvexDecomposition::getInstance()->getParameters(&params);
	}
	
	for (S32 i = 0; i < param_count; ++i)
	{
		param_map[params[i].mName] = params+i;
	}

	//set parameter values
	for (decomp_params::iterator iter = mCurRequest->mParams.begin(); iter != mCurRequest->mParams.end(); ++iter)
	{
		const std::string& name = iter->first;
		const LLSD& value = iter->second;

		const LLCDParam* param = param_map[name];

		if (param == NULL)
		{ //couldn't find valid parameter
			continue;
		}

		U32 ret = LLCD_OK;

		if (param->mType == LLCDParam::LLCD_FLOAT)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, (F32) value.asReal());
		}
		else if (param->mType == LLCDParam::LLCD_INTEGER ||
			param->mType == LLCDParam::LLCD_ENUM)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, value.asInteger());
		}
		else if (param->mType == LLCDParam::LLCD_BOOLEAN)
		{
			ret = LLConvexDecomposition::getInstance()->setParam(param->mName, value.asBoolean());
		}

		if (ret)
		{
			llerrs << "WTF?" << llendl;
		}
	}

	mCurRequest->setStatusMessage("Executing.");

	LLCDResult ret = LLCD_OK;
	
	if (LLConvexDecomposition::getInstance() != NULL)
	{
		ret = LLConvexDecomposition::getInstance()->executeStage(stage);
	}

	if (ret)
	{
		llwarns << "Convex Decomposition thread valid but could not execute stage " << stage << llendl;
		LLMutexLock lock(mMutex);

		mCurRequest->mHull.clear();
		mCurRequest->mHullMesh.clear();

		mCurRequest->setStatusMessage("FAIL");
		
		completeCurrent();
	}
	else
	{
		mCurRequest->setStatusMessage("Reading results");

		S32 num_hulls =0;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			num_hulls = LLConvexDecomposition::getInstance()->getNumHullsFromStage(stage);
		}
		
		mMutex->lock();
		mCurRequest->mHull.clear();
		mCurRequest->mHull.resize(num_hulls);

		mCurRequest->mHullMesh.clear();
		mCurRequest->mHullMesh.resize(num_hulls);
		mMutex->unlock();

		for (S32 i = 0; i < num_hulls; ++i)
		{
			std::vector<LLVector3> p;
			LLCDHull hull;
			// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
			LLConvexDecomposition::getInstance()->getHullFromStage(stage, i, &hull);

			const F32* v = hull.mVertexBase;

			for (S32 j = 0; j < hull.mNumVertices; ++j)
			{
				LLVector3 vert(v[0], v[1], v[2]); 
				p.push_back(vert);
				v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
			}
			
			LLCDMeshData mesh;
			// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
			LLConvexDecomposition::getInstance()->getMeshFromStage(stage, i, &mesh);

			mCurRequest->mHullMesh[i] = get_vertex_buffer_from_mesh(mesh);
			
			mMutex->lock();
			mCurRequest->mHull[i] = p;
			mMutex->unlock();
		}
	
		{
			LLMutexLock lock(mMutex);

			mCurRequest->setStatusMessage("FAIL");
			completeCurrent();						
		}
	}
}

void LLPhysicsDecomp::completeCurrent()
{
	LLMutexLock lock(mMutex);
	mCompletedQ.push(mCurRequest);
	mCurRequest = NULL;
}

void LLPhysicsDecomp::notifyCompleted()
{
	if (!mCompletedQ.empty())
	{
		LLMutexLock lock(mMutex);
		while (!mCompletedQ.empty())
		{
			Request* req = mCompletedQ.front();
			req->completed();
			mCompletedQ.pop();
		}
	}
}


void make_box(LLPhysicsDecomp::Request * request)
{
	LLVector3 min,max;
	min = request->mPositions[0];
	max = min;

	for (U32 i = 0; i < request->mPositions.size(); ++i)
	{
		update_min_max(min, max, request->mPositions[i]);
	}

	request->mHull.clear();
	
	LLModel::hull box;
	box.push_back(LLVector3(min[0],min[1],min[2]));
	box.push_back(LLVector3(max[0],min[1],min[2]));
	box.push_back(LLVector3(min[0],max[1],min[2]));
	box.push_back(LLVector3(max[0],max[1],min[2]));
	box.push_back(LLVector3(min[0],min[1],max[2]));
	box.push_back(LLVector3(max[0],min[1],max[2]));
	box.push_back(LLVector3(min[0],max[1],max[2]));
	box.push_back(LLVector3(max[0],max[1],max[2]));

	request->mHull.push_back(box);
}


void LLPhysicsDecomp::doDecompositionSingleHull()
{
	LLCDMeshData mesh;
	
	setMeshData(mesh);
			
	
	//set all parameters to default
	std::map<std::string, const LLCDParam*> param_map;

	static const LLCDParam* params = NULL;
	static S32 param_count = 0;

	if (!params)
	{
		param_count = LLConvexDecomposition::getInstance()->getParameters(&params);
	}
	
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();

	for (S32 i = 0; i < param_count; ++i)
	{
		decomp->setParam(params[i].mName, params[i].mDefault.mIntOrEnumValue);
	}

	const S32 STAGE_DECOMPOSE = mStageID["Decompose"];
	const S32 STAGE_SIMPLIFY = mStageID["Simplify"];
	const S32 DECOMP_PREVIEW = 0;
	const S32 SIMPLIFY_RETAIN = 0;
	
	decomp->setParam("Decompose Quality", DECOMP_PREVIEW);
	decomp->setParam("Simplify Method", SIMPLIFY_RETAIN);
	decomp->setParam("Retain%", 0.f);

	LLCDResult ret = LLCD_OK;
	ret = decomp->executeStage(STAGE_DECOMPOSE);
	
	if (ret)
	{
		llwarns << "Could not execute decomposition stage when attempting to create single hull." << llendl;
		make_box(mCurRequest);
	}
	else
	{
		ret = decomp->executeStage(STAGE_SIMPLIFY);

		if (ret)
		{
			llwarns << "Could not execute simiplification stage when attempting to create single hull." << llendl;
			make_box(mCurRequest);
		}
		else
		{
			S32 num_hulls =0;
			if (LLConvexDecomposition::getInstance() != NULL)
			{
				num_hulls = LLConvexDecomposition::getInstance()->getNumHullsFromStage(STAGE_SIMPLIFY);
			}
			
			mMutex->lock();
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(num_hulls);
			mCurRequest->mHullMesh.clear();
			mMutex->unlock();

			for (S32 i = 0; i < num_hulls; ++i)
			{
				std::vector<LLVector3> p;
				LLCDHull hull;
				// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
				LLConvexDecomposition::getInstance()->getHullFromStage(STAGE_SIMPLIFY, i, &hull);

				const F32* v = hull.mVertexBase;

				for (S32 j = 0; j < hull.mNumVertices; ++j)
				{
					LLVector3 vert(v[0], v[1], v[2]); 
					p.push_back(vert);
					v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
				}
						
				mMutex->lock();
				mCurRequest->mHull[i] = p;
				mMutex->unlock();
			}
		}
	}


	{
		completeCurrent();
		
	}
}


void LLPhysicsDecomp::run()
{
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();
	decomp->initThread();
	mInited = true;

	static const LLCDStageData* stages = NULL;
	static S32 num_stages = 0;
	
	if (!stages)
	{
		num_stages = decomp->getStages(&stages);
	}

	for (S32 i = 0; i < num_stages; i++)
	{
		mStageID[stages[i].mName] = i;
	}

	while (!mQuitting)
	{
		mSignal->wait();
		while (!mQuitting && !mRequestQ.empty())
		{
			{
				LLMutexLock lock(mMutex);
				mCurRequest = mRequestQ.front();
				mRequestQ.pop();
			}

			S32& id = *(mCurRequest->mDecompID);
			if (id == -1)
			{
				decomp->genDecomposition(id);
			}
			decomp->bindDecomposition(id);

			if (mCurRequest->mStage == "single_hull")
			{
				doDecompositionSingleHull();
			}
			else
			{
				doDecomposition();
			}		
		}
	}

	decomp->quitThread();
	
	if (mSignal->isLocked())
	{ //let go of mSignal's associated mutex
		mSignal->unlock();
	}

	mDone = true;
}

void LLPhysicsDecomp::Request::setStatusMessage(const std::string& msg)
{
	mStatusMessage = msg;
}

