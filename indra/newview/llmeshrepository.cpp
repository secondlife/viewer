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
#include "llhttpstatuscodes.h"
#include "llmeshrepository.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llbufferstream.h"
#include "llcallbacklist.h"
#include "llcurl.h"
#include "lldatapacker.h"
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
#include "llviewerinventory.h"
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
#include "llinventorymodel.h"
#include "llfoldertype.h"
#include "llviewerparcelmgr.h"
#include "lluploadfloaterobservers.h"

#include "boost/lexical_cast.hpp"

#ifndef LL_WINDOWS
#include "netdb.h"
#endif

#include <queue>

LLMeshRepository gMeshRepo;

const U32 MAX_MESH_REQUESTS_PER_SECOND = 100;

// Maximum mesh version to support.  Three least significant digits are reserved for the minor version, 
// with major version changes indicating a format change that is not backwards compatible and should not
// be parsed by viewers that don't specifically support that version. For example, if the integer "1" is 
// present, the version is 0.001. A viewer that can parse version 0.001 can also parse versions up to 0.999, 
// but not 1.0 (integer 1000).
// See wiki at https://wiki.secondlife.com/wiki/Mesh/Mesh_Asset_Format
const S32 MAX_MESH_VERSION = 999;

U32 LLMeshRepository::sBytesReceived = 0;
U32 LLMeshRepository::sHTTPRequestCount = 0;
U32 LLMeshRepository::sHTTPRetryCount = 0;
U32 LLMeshRepository::sLODProcessing = 0;
U32 LLMeshRepository::sLODPending = 0;

U32 LLMeshRepository::sCacheBytesRead = 0;
U32 LLMeshRepository::sCacheBytesWritten = 0;
U32 LLMeshRepository::sPeakKbps = 0;
	

const U32 MAX_TEXTURE_UPLOAD_RETRIES = 5;

static S32 dump_num = 0;
std::string make_dump_name(std::string prefix, S32 num)
{
	return prefix + boost::lexical_cast<std::string>(num) + std::string(".xml");
	
}
void dump_llsd_to_file(const LLSD& content, std::string filename);
LLSD llsd_from_file(std::string filename);

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

void get_vertex_buffer_from_mesh(LLCDMeshData& mesh, LLModel::PhysicsMesh& res, F32 scale = 1.f)
{
	res.mPositions.clear();
	res.mNormals.clear();
	
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

			res.mPositions.push_back(v0*scale);
			res.mPositions.push_back(v1*scale);
			res.mPositions.push_back(v2*scale);

			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);			
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

			res.mPositions.push_back(v0*scale);
			res.mPositions.push_back(v1*scale);
			res.mPositions.push_back(v2*scale);

			res.mNormals.push_back(n);
			res.mNormals.push_back(n);
			res.mNormals.push_back(n);			
		}
	}
}

S32 LLMeshRepoThread::sActiveHeaderRequests = 0;
S32 LLMeshRepoThread::sActiveLODRequests = 0;
U32	LLMeshRepoThread::sMaxConcurrentRequests = 1;

class LLMeshHeaderResponder : public LLCurl::Responder
{
public:
	LLVolumeParams mMeshParams;
	
	LLMeshHeaderResponder(const LLVolumeParams& mesh_params)
		: mMeshParams(mesh_params)
	{
		LLMeshRepoThread::sActiveHeaderRequests++;
	}

	~LLMeshHeaderResponder()
	{
		LLMeshRepoThread::sActiveHeaderRequests--;
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
		LLMeshRepoThread::sActiveLODRequests++;
	}

	~LLMeshLODResponder()
	{
		LLMeshRepoThread::sActiveLODRequests--;
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

void log_upload_error(S32 status, const LLSD& content, std::string stage, std::string model_name)
{
	// Add notification popup.
	LLSD args;
	std::string message = content["error"]["message"];
	std::string identifier = content["error"]["identifier"];
	args["MESSAGE"] = message;
	args["IDENTIFIER"] = identifier;
	args["LABEL"] = model_name;
	gMeshRepo.uploadError(args);

	// Log details.
	llwarns << "stage: " << stage << " http status: " << status << llendl;
	if (content.has("error"))
	{
		const LLSD& err = content["error"];
		llwarns << "err: " << err << llendl;
		llwarns << "mesh upload failed, stage '" << stage
				<< "' error '" << err["error"].asString()
				<< "', message '" << err["message"].asString()
				<< "', id '" << err["identifier"].asString()
				<< "'" << llendl;
		if (err.has("errors"))
		{
			S32 error_num = 0;
			const LLSD& err_list = err["errors"];
			for (LLSD::array_const_iterator it = err_list.beginArray();
				 it != err_list.endArray();
				 ++it)
			{
				const LLSD& err_entry = *it;
				llwarns << "error[" << error_num << "]:" << llendl;
				for (LLSD::map_const_iterator map_it = err_entry.beginMap();
					 map_it != err_entry.endMap();
					 ++map_it)
				{
					llwarns << "\t" << map_it->first << ": "
							<< map_it->second << llendl;
				}
				error_num++;
			}
		}
	}
	else
	{
		llwarns << "bad mesh, no error information available" << llendl;
	}
}

class LLWholeModelFeeResponder: public LLCurl::Responder
{
	LLMeshUploadThread* mThread;
	LLSD mModelData;
	LLHandle<LLWholeModelFeeObserver> mObserverHandle;
public:
	LLWholeModelFeeResponder(LLMeshUploadThread* thread, LLSD& model_data, LLHandle<LLWholeModelFeeObserver> observer_handle):
		mThread(thread),
		mModelData(model_data),
		mObserverHandle(observer_handle)
	{
	}
	virtual void completed(U32 status,
						   const std::string& reason,
						   const LLSD& content)
	{
		LLSD cc = content;
		if (gSavedSettings.getS32("MeshUploadFakeErrors")&1)
		{
			cc = llsd_from_file("fake_upload_error.xml");
		}
			
		mThread->mPendingUploads--;
		dump_llsd_to_file(cc,make_dump_name("whole_model_fee_response_",dump_num));

		LLWholeModelFeeObserver* observer = mObserverHandle.get();

		if (isGoodStatus(status) &&
			cc["state"].asString() == "upload")
		{
			mThread->mWholeModelUploadURL = cc["uploader"].asString();

			if (observer)
			{
				cc["data"]["upload_price"] = cc["upload_price"];
				observer->onModelPhysicsFeeReceived(cc["data"], mThread->mWholeModelUploadURL);
			}
		}
		else
		{
			llwarns << "fee request failed" << llendl;
			log_upload_error(status,cc,"fee",mModelData["name"]);
			mThread->mWholeModelUploadURL = "";

			if (observer)
			{
				observer->setModelPhysicsFeeErrorStatus(status, reason);
			}
		}
	}

};

class LLWholeModelUploadResponder: public LLCurl::Responder
{
	LLMeshUploadThread* mThread;
	LLSD mModelData;
	LLHandle<LLWholeModelUploadObserver> mObserverHandle;
	
public:
	LLWholeModelUploadResponder(LLMeshUploadThread* thread, LLSD& model_data, LLHandle<LLWholeModelUploadObserver> observer_handle):
		mThread(thread),
		mModelData(model_data),
		mObserverHandle(observer_handle)
	{
	}
	virtual void completed(U32 status,
						   const std::string& reason,
						   const LLSD& content)
	{
		LLSD cc = content;
		if (gSavedSettings.getS32("MeshUploadFakeErrors")&2)
		{
			cc = llsd_from_file("fake_upload_error.xml");
		}

		mThread->mPendingUploads--;
		dump_llsd_to_file(cc,make_dump_name("whole_model_upload_response_",dump_num));
		
		LLWholeModelUploadObserver* observer = mObserverHandle.get();

		// requested "mesh" asset type isn't actually the type
		// of the resultant object, fix it up here.
		if (isGoodStatus(status) &&
			cc["state"].asString() == "complete")
		{
			mModelData["asset_type"] = "object";
			gMeshRepo.updateInventory(LLMeshRepository::inventory_data(mModelData,cc));

			if (observer)
			{
				doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadSuccess, observer));
			}
		}
		else
		{
			llwarns << "upload failed" << llendl;
			std::string model_name = mModelData["name"].asString();
			log_upload_error(status,cc,"upload",model_name);

			if (observer)
			{
				doOnIdleOneTime(boost::bind(&LLWholeModelUploadObserver::onModelUploadFailure, observer));
			}
		}
	}
};

LLMeshRepoThread::LLMeshRepoThread()
: LLThread("mesh repo") 
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
				if (mMutex)
				{
					mMutex->lock();
					LODRequest req = mLODReqQ.front();
					mLODReqQ.pop();
					LLMeshRepository::sLODProcessing--;
					mMutex->unlock();
					if (!fetchMeshLOD(req.mMeshParams, req.mLOD, count))//failed, resubmit
					{
						mMutex->lock();
						mLODReqQ.push(req) ; 
						mMutex->unlock();
					}
				}
			}

			while (!mHeaderReqQ.empty() && count < MAX_MESH_REQUESTS_PER_SECOND && sActiveHeaderRequests < sMaxConcurrentRequests)
			{
				if (mMutex)
				{
					mMutex->lock();
					HeaderRequest req = mHeaderReqQ.front();
					mHeaderReqQ.pop();
					mMutex->unlock();
					if (!fetchMeshHeader(req.mMeshParams, count))//failed, resubmit
					{
						mMutex->lock();
						mHeaderReqQ.push(req) ;
						mMutex->unlock();
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
			LLMeshRepository::sLODProcessing++;
		}
	}
	else
	{ 
		HeaderRequest req(mesh_params);
		
		pending_lod_map::iterator pending = mPendingLOD.find(mesh_params);

		if (pending != mPendingLOD.end())
		{ //append this lod request to existing header request
			pending->second.push_back(lod);
			llassert(pending->second.size() <= LLModel::NUM_LODS)
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
	
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	bool ret = true ;
	U32 header_size = mMeshHeaderSize[mesh_id];
	
	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id]["skin"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["skin"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{
			//check VFS for mesh skin info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{				
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
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
				ret = mCurlRequest->getByteRange(http_url, headers, offset, size,
												 new LLMeshSkinInfoResponder(mesh_id, offset, size));
				if(ret)
				{
					LLMeshRepository::sHTTPRequestCount++;
				}
			}
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return ret;
}

bool LLMeshRepoThread::fetchMeshDecomposition(const LLUUID& mesh_id)
{ //protected by mMutex
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	U32 header_size = mMeshHeaderSize[mesh_id];
	bool ret = true ;
	
	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id]["physics_convex"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["physics_convex"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{
			//check VFS for mesh skin info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;

				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
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
				ret = mCurlRequest->getByteRange(http_url, headers, offset, size,
												 new LLMeshDecompositionResponder(mesh_id, offset, size));
				if(ret)
				{
					LLMeshRepository::sHTTPRequestCount++;
				}
			}
		}
	}
	else
	{	
		mHeaderMutex->unlock();
	}

	//early out was not hit, effectively fetched
	return ret;
}

bool LLMeshRepoThread::fetchMeshPhysicsShape(const LLUUID& mesh_id)
{ //protected by mMutex
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	if (mMeshHeader.find(mesh_id) == mMeshHeader.end())
	{ //we have no header info for this mesh, do nothing
		mHeaderMutex->unlock();
		return false;
	}

	U32 header_size = mMeshHeaderSize[mesh_id];
	bool ret = true ;

	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id]["physics_mesh"]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id]["physics_mesh"]["size"].asInteger();

		mHeaderMutex->unlock();

		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{
			//check VFS for mesh physics shape info
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
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
				ret = mCurlRequest->getByteRange(http_url, headers, offset, size,
												 new LLMeshPhysicsShapeResponder(mesh_id, offset, size));

				if(ret)
				{
					LLMeshRepository::sHTTPRequestCount++;
				}
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
	return ret;
}

//return false if failed to get header
bool LLMeshRepoThread::fetchMeshHeader(const LLVolumeParams& mesh_params, U32& count)
{
	{
		//look for mesh in asset in vfs
		LLVFile file(gVFS, mesh_params.getSculptID(), LLAssetType::AT_MESH);
			
		S32 size = file.getSize();

		if (size > 0)
		{ //NOTE -- if the header size is ever more than 4KB, this will break
			U8 buffer[4096];
			S32 bytes = llmin(size, 4096);
			LLMeshRepository::sCacheBytesRead += bytes;	
			file.read(buffer, bytes);
			if (headerReceived(mesh_params, buffer, bytes))
			{ //did not do an HTTP request, return false
				return true;
			}
		}
	}

	//either cache entry doesn't exist or is corrupt, request header from simulator	
	bool retval = true ;
	std::vector<std::string> headers;
	headers.push_back("Accept: application/octet-stream");

	std::string http_url = constructUrl(mesh_params.getSculptID());
	if (!http_url.empty())
	{
		//grab first 4KB if we're going to bother with a fetch.  Cache will prevent future fetches if a full mesh fits
		//within the first 4KB
		//NOTE -- this will break of headers ever exceed 4KB		
		retval = mCurlRequest->getByteRange(http_url, headers, 0, 4096, new LLMeshHeaderResponder(mesh_params));
		if(retval)
		{
			LLMeshRepository::sHTTPRequestCount++;
		}
		count++;
	}

	return retval;
}

//return false if failed to get mesh lod.
bool LLMeshRepoThread::fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod, U32& count)
{ //protected by mMutex
	if (!mHeaderMutex)
	{
		return false;
	}

	mHeaderMutex->lock();

	bool retval = true;

	LLUUID mesh_id = mesh_params.getSculptID();
	
	U32 header_size = mMeshHeaderSize[mesh_id];

	if (header_size > 0)
	{
		S32 version = mMeshHeader[mesh_id]["version"].asInteger();
		S32 offset = header_size + mMeshHeader[mesh_id][header_lod[lod]]["offset"].asInteger();
		S32 size = mMeshHeader[mesh_id][header_lod[lod]]["size"].asInteger();
		mHeaderMutex->unlock();
				
		if (version <= MAX_MESH_VERSION && offset >= 0 && size > 0)
		{

			//check VFS for mesh asset
			LLVFile file(gVFS, mesh_id, LLAssetType::AT_MESH);
			if (file.getSize() >= offset+size)
			{
				LLMeshRepository::sCacheBytesRead += size;
				file.seek(offset);
				U8* buffer = new U8[size];
				file.read(buffer, size);

				//make sure buffer isn't all 0's by checking the first 1KB (reserved block but not written)
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
				retval = mCurlRequest->getByteRange(constructUrl(mesh_id), headers, offset, size,
										   new LLMeshLODResponder(mesh_params, lod, offset, size));

				if(retval)
				{
					LLMeshRepository::sHTTPRequestCount++;
				}
				count++;
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
		LLUUID mesh_id = mesh_params.getSculptID();
		
		{
			LLMutexLock lock(mHeaderMutex);
			mMeshHeaderSize[mesh_id] = header_size;
			mMeshHeader[mesh_id] = header;
			}


		LLMutexLock lock(mMutex); // make sure only one thread access mPendingLOD at the same time.

		//check for pending requests
		pending_lod_map::iterator iter = mPendingLOD.find(mesh_params);
		if (iter != mPendingLOD.end())
		{
			for (U32 i = 0; i < iter->second.size(); ++i)
			{
				LODRequest req(mesh_params, iter->second[i]);
				mLODReqQ.push(req);
				LLMeshRepository::sLODProcessing++;
			}
		}
		mPendingLOD.erase(iter);
	}

	return true;
}

bool LLMeshRepoThread::lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size)
{
	LLPointer<LLVolume> volume = new LLVolume(mesh_params, LLVolumeLODGroup::getVolumeScaleFromDetail(lod));
	std::string mesh_string((char*) data, data_size);
	std::istringstream stream(mesh_string);

	if (volume->unpackVolumeFaces(stream, data_size))
	{
		if (volume->getNumFaces() > 0)
		{
			LoadedMesh mesh(volume, mesh_params, lod);
			{
				LLMutexLock lock(mMutex);
				mLoadedQ.push(mesh);
			}
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
		LLMeshSkinInfo info(skin);
		info.mMeshID = mesh_id;

		//llinfos<<"info pelvis offset"<<info.mPelvisOffset<<llendl;
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
		LLModel::Decomposition* d = new LLModel::Decomposition(decomp);
		d->mMeshID = mesh_id;
		mDecompositionQ.push(d);
	}

	return true;
}

bool LLMeshRepoThread::physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size)
{
	LLSD physics_shape;

	LLModel::Decomposition* d = new LLModel::Decomposition();
	d->mMeshID = mesh_id;

	if (data == NULL)
	{ //no data, no physics shape exists
		d->mPhysicsShapeMesh.clear();
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

			d->mPhysicsShapeMesh.clear();

			std::vector<LLVector3>& pos = d->mPhysicsShapeMesh.mPositions;
			std::vector<LLVector3>& norm = d->mPhysicsShapeMesh.mNormals;

			for (S32 i = 0; i < volume->getNumVolumeFaces(); ++i)
			{
				const LLVolumeFace& face = volume->getVolumeFace(i);
			
				for (S32 i = 0; i < face.mNumIndices; ++i)
				{
					U16 idx = face.mIndices[i];

					pos.push_back(LLVector3(face.mPositions[idx].getF32ptr()));
					norm.push_back(LLVector3(face.mNormals[idx].getF32ptr()));				
				}			
			}
		}
	}

	mDecompositionQ.push(d);
	return true;
}

LLMeshUploadThread::LLMeshUploadThread(LLMeshUploadThread::instance_list& data, LLVector3& scale, bool upload_textures,
										bool upload_skin, bool upload_joints, std::string upload_url, bool do_upload,
					   LLHandle<LLWholeModelFeeObserver> fee_observer, LLHandle<LLWholeModelUploadObserver> upload_observer)
: LLThread("mesh upload"),
	mDiscarded(FALSE),
	mDoUpload(do_upload),
	mWholeModelUploadURL(upload_url),
	mFeeObserverHandle(fee_observer),
	mUploadObserverHandle(upload_observer)
{
	mInstanceList = data;
	mUploadTextures = upload_textures;
	mUploadSkin = upload_skin;
	mUploadJoints = upload_joints;
	mMutex = new LLMutex(NULL);
	mCurlRequest = NULL;
	mPendingUploads = 0;
	mFinished = false;
	mOrigin = gAgent.getPositionAgent();
	mHost = gAgent.getRegionHost();
	
	mWholeModelFeeCapability = gAgent.getRegion()->getCapability("NewFileAgentInventory");

	mOrigin += gAgent.getAtAxis() * scale.magVec();

	mMeshUploadTimeOut = gSavedSettings.getS32("MeshUploadTimeOut") ;
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
	assignData(mdl) ;	

	mThread->mFinalDecomp = this;
	mThread->mPhysicsComplete = false;
}

void LLMeshUploadThread::DecompRequest::completed()
{
	if (mThread->mFinalDecomp == this)
	{
		mThread->mPhysicsComplete = true;
	}

	llassert(mHull.size() == 1);
	
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

void LLMeshUploadThread::discard()
{
	LLMutexLock lock(mMutex) ;
	mDiscarded = TRUE ;
}

BOOL LLMeshUploadThread::isDiscarded()
{
	LLMutexLock lock(mMutex) ;
	return mDiscarded ;
}

void LLMeshUploadThread::run()
{
	if (mDoUpload)
	{
		doWholeModelUpload();
	}
	else
	{
		requestWholeModelFee();
	}
}

void dump_llsd_to_file(const LLSD& content, std::string filename)
{
	if (gSavedSettings.getBOOL("MeshUploadLogXML"))
	{
		std::ofstream of(filename.c_str());
		LLSDSerialize::toPrettyXML(content,of);
	}
}

LLSD llsd_from_file(std::string filename)
{
	std::ifstream ifs(filename.c_str());
	LLSD result;
	LLSDSerialize::fromXML(result,ifs);
	return result;
}

void LLMeshUploadThread::wholeModelToLLSD(LLSD& dest, bool include_textures)
{
	LLSD result;

	LLSD res;
	result["folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_OBJECT);
	result["texture_folder_id"] = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);
	result["asset_type"] = "mesh";
	result["inventory_type"] = "object";
	result["description"] = "(No Description)";
	result["next_owner_mask"] = LLSD::Integer(LLFloaterPerms::getNextOwnerPerms());
	result["group_mask"] = LLSD::Integer(LLFloaterPerms::getGroupPerms());
	result["everyone_mask"] = LLSD::Integer(LLFloaterPerms::getEveryonePerms());

	res["mesh_list"] = LLSD::emptyArray();
	res["texture_list"] = LLSD::emptyArray();
	res["instance_list"] = LLSD::emptyArray();
	S32 mesh_num = 0;
	S32 texture_num = 0;
	
	std::set<LLViewerTexture* > textures;
	std::map<LLViewerTexture*,S32> texture_index;

	std::map<LLModel*,S32> mesh_index;
	std::string model_name;
	std::string model_metric;

	S32 instance_num = 0;
	
	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;
		LLModelInstance& first_instance = *(iter->second.begin());
		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = first_instance.mLOD[i];
		}

		if (mesh_index.find(data.mBaseModel) == mesh_index.end())
		{
			// Have not seen this model before - create a new mesh_list entry for it.
			if (model_name.empty())
			{
				model_name = data.mBaseModel->getName();
			}

			if (model_metric.empty())
			{
				model_metric = data.mBaseModel->getMetric();
			}

			std::stringstream ostr;
			
			LLModel::Decomposition& decomp =
				data.mModel[LLModel::LOD_PHYSICS].notNull() ? 
				data.mModel[LLModel::LOD_PHYSICS]->mPhysics : 
				data.mBaseModel->mPhysics;

			decomp.mBaseHull = mHullMap[data.mBaseModel];

			LLSD mesh_header = LLModel::writeModel(
				ostr,  
				data.mModel[LLModel::LOD_PHYSICS],
				data.mModel[LLModel::LOD_HIGH],
				data.mModel[LLModel::LOD_MEDIUM],
				data.mModel[LLModel::LOD_LOW],
				data.mModel[LLModel::LOD_IMPOSTOR], 
				decomp,
				mUploadSkin,
				mUploadJoints);

			data.mAssetData = ostr.str();
			std::string str = ostr.str();

			res["mesh_list"][mesh_num] = LLSD::Binary(str.begin(),str.end()); 
			mesh_index[data.mBaseModel] = mesh_num;
			mesh_num++;
		}

		// For all instances that use this model
		for (instance_list::iterator instance_iter = iter->second.begin();
			 instance_iter != iter->second.end();
			 ++instance_iter)
		{

			LLModelInstance& instance = *instance_iter;
		
			LLSD instance_entry;
		
			for (S32 i = 0; i < 5; i++)
			{
				data.mModel[i] = instance.mLOD[i];
			}
		
			LLVector3 pos, scale;
			LLQuaternion rot;
			LLMatrix4 transformation = instance.mTransform;
			decomposeMeshMatrix(transformation,pos,rot,scale);
			instance_entry["position"] = ll_sd_from_vector3(pos);
			instance_entry["rotation"] = ll_sd_from_quaternion(rot);
			instance_entry["scale"] = ll_sd_from_vector3(scale);
		
			instance_entry["material"] = LL_MCODE_WOOD;
			instance_entry["physics_shape_type"] = (U8)(LLViewerObject::PHYSICS_SHAPE_CONVEX_HULL);
			instance_entry["mesh"] = mesh_index[data.mBaseModel];

			instance_entry["face_list"] = LLSD::emptyArray();

			S32 end = llmin((S32)data.mBaseModel->mMaterialList.size(), data.mBaseModel->getNumVolumeFaces()) ;
			for (S32 face_num = 0; face_num < end; face_num++)
			{
				LLImportMaterial& material = instance.mMaterial[data.mBaseModel->mMaterialList[face_num]];
				LLSD face_entry = LLSD::emptyMap();
				LLViewerFetchedTexture *texture = material.mDiffuseMap.get();
				
				if ((texture != NULL) &&
					(textures.find(texture) == textures.end()))
				{
					textures.insert(texture);
				}

				std::stringstream texture_str;
				if (texture != NULL && include_textures && mUploadTextures)
				{
					if(texture->hasSavedRawImage())
					{											
						LLPointer<LLImageJ2C> upload_file =
							LLViewerTextureList::convertToUploadFile(texture->getSavedRawImage());
						texture_str.write((const char*) upload_file->getData(), upload_file->getDataSize());
					}
				}

				if (texture != NULL &&
					mUploadTextures &&
					texture_index.find(texture) == texture_index.end())
				{
					texture_index[texture] = texture_num;
					std::string str = texture_str.str();
					res["texture_list"][texture_num] = LLSD::Binary(str.begin(),str.end());
					texture_num++;
				}

				// Subset of TextureEntry fields.
				if (texture != NULL && mUploadTextures)
				{
					face_entry["image"] = texture_index[texture];
					face_entry["scales"] = 1.0;
					face_entry["scalet"] = 1.0;
					face_entry["offsets"] = 0.0;
					face_entry["offsett"] = 0.0;
					face_entry["imagerot"] = 0.0;
				}
				face_entry["diffuse_color"] = ll_sd_from_color4(material.mDiffuseColor);
				face_entry["fullbright"] = material.mFullbright;
				instance_entry["face_list"][face_num] = face_entry;
		    }

			res["instance_list"][instance_num] = instance_entry;
			instance_num++;
		}
	}

	if (model_name.empty()) model_name = "mesh model";
	result["name"] = model_name;
	if (model_metric.empty()) model_metric = "MUT_Unspecified";
	res["metric"] = model_metric;
	result["asset_resources"] = res;
	dump_llsd_to_file(result,make_dump_name("whole_model_",dump_num));

	dest = result;
}

void LLMeshUploadThread::generateHulls()
{
	bool has_valid_requests = false ;

	for (instance_map::iterator iter = mInstance.begin(); iter != mInstance.end(); ++iter)
	{
		LLMeshUploadData data;
		data.mBaseModel = iter->first;

		LLModelInstance& instance = *(iter->second.begin());

		for (S32 i = 0; i < 5; i++)
		{
			data.mModel[i] = instance.mLOD[i];
		}

		//queue up models for hull generation
		LLModel* physics = NULL;

		if (data.mModel[LLModel::LOD_PHYSICS].notNull())
		{
			physics = data.mModel[LLModel::LOD_PHYSICS];
		}
		else if (data.mModel[LLModel::LOD_LOW].notNull())
		{
			physics = data.mModel[LLModel::LOD_LOW];
		}
		else if (data.mModel[LLModel::LOD_MEDIUM].notNull())
		{
			physics = data.mModel[LLModel::LOD_MEDIUM];
		}
		else
		{
			physics = data.mModel[LLModel::LOD_HIGH];
		}

		llassert(physics != NULL);

		DecompRequest* request = new DecompRequest(physics, data.mBaseModel, this);
		if(request->isValid())
		{
			gMeshRepo.mDecompThread->submitRequest(request);
			has_valid_requests = true ;
		}
	}
		
	if(has_valid_requests)
	{
		while (!mPhysicsComplete)
		{
			apr_sleep(100);
		}
	}	
}

void LLMeshUploadThread::doWholeModelUpload()
{
	mCurlRequest = new LLCurlRequest();

	if (mWholeModelUploadURL.empty())
	{
		llinfos << "unable to upload, fee request failed" << llendl;
	}
	else
	{
		generateHulls();

		LLSD full_model_data;
		wholeModelToLLSD(full_model_data, true);
		LLSD body = full_model_data["asset_resources"];
		dump_llsd_to_file(body,make_dump_name("whole_model_body_",dump_num));
		LLCurlRequest::headers_t headers;

		{
			LLCurl::ResponderPtr responder = new LLWholeModelUploadResponder(this, full_model_data, mUploadObserverHandle) ;

			while(!mCurlRequest->post(mWholeModelUploadURL, headers, body, responder, mMeshUploadTimeOut))
			{
				//sleep for 10ms to prevent eating a whole core
				apr_sleep(10000);
			}
		}

		do
		{
			mCurlRequest->process();
			//sleep for 10ms to prevent eating a whole core
			apr_sleep(10000);
		} while (!LLAppViewer::isQuitting() && mCurlRequest->getQueued() > 0);
	}

	delete mCurlRequest;
	mCurlRequest = NULL;

	// Currently a no-op.
	mFinished = true;
}

void LLMeshUploadThread::requestWholeModelFee()
{
	dump_num++;

	mCurlRequest = new LLCurlRequest();

	generateHulls();

	LLSD model_data;
	wholeModelToLLSD(model_data,false);
	dump_llsd_to_file(model_data,make_dump_name("whole_model_fee_request_",dump_num));

	mPendingUploads++;
	LLCurlRequest::headers_t headers;

	{
		LLCurl::ResponderPtr responder = new LLWholeModelFeeResponder(this,model_data, mFeeObserverHandle) ;
		while(!mCurlRequest->post(mWholeModelFeeCapability, headers, model_data, responder, mMeshUploadTimeOut))
		{
			//sleep for 10ms to prevent eating a whole core
			apr_sleep(10000);
		}
	}

	do
	{
		mCurlRequest->process();
		//sleep for 10ms to prevent eating a whole core
		apr_sleep(10000);
	} while (!LLApp::isQuitting() && mCurlRequest->getQueued() > 0);

	delete mCurlRequest;
	mCurlRequest = NULL;

	// Currently a no-op.
	mFinished = true;
}

void LLMeshRepoThread::notifyLoadedMeshes()
{
	if (!mMutex)
	{
		return;
	}

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
	LLMutexLock lock(mHeaderMutex);
	mesh_header_map::iterator iter = mMeshHeader.find(mesh_params.getSculptID());

	if (iter != mMeshHeader.end())
	{
		LLSD& header = iter->second;

		return LLMeshRepository::getActualMeshLOD(header, lod);
	}

	return lod;
}

//static
S32 LLMeshRepository::getActualMeshLOD(LLSD& header, S32 lod)
{
	lod = llclamp(lod, 0, 3);

	S32 version = header["version"];

	if (header.has("404") || version > MAX_MESH_VERSION)
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
			volume->setMeshAssetLoaded(TRUE);
		}
	}

}

void LLMeshLODResponder::completedRaw(U32 status, const std::string& reason,
							  const LLChannelDescriptors& channels,
							  const LLIOPipe::buffer_ptr_t& buffer)
{
	// thread could have already be destroyed during logout
	if( !gMeshRepo.mThread )
	{
		return;
	}
	
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
	// thread could have already be destroyed during logout
	if( !gMeshRepo.mThread )
	{
		return;
	}

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
	if( !gMeshRepo.mThread )
	{
		return;
	}

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
	// thread could have already be destroyed during logout
	if( !gMeshRepo.mThread )
	{
		return;
	}

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
	// thread could have already be destroyed during logout
	if( !gMeshRepo.mThread )
	{
		return;
	}

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

		S32 version = header["version"].asInteger();

		if (version <= MAX_MESH_VERSION)
		{
			std::stringstream str;

			S32 lod_bytes = 0;

			for (U32 i = 0; i < LLModel::LOD_PHYSICS; ++i)
			{ //figure out how many bytes we'll need to reserve in the file
				std::string lod_name = header_lod[i];
				lod_bytes = llmax(lod_bytes, header[lod_name]["offset"].asInteger()+header[lod_name]["size"].asInteger());
			}
		
			//just in case skin info or decomposition is at the end of the file (which it shouldn't be)
			lod_bytes = llmax(lod_bytes, header["skin"]["offset"].asInteger() + header["skin"]["size"].asInteger());
			lod_bytes = llmax(lod_bytes, header["physics_convex"]["offset"].asInteger() + header["physics_convex"]["size"].asInteger());

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

				if (remaining > 0)
				{
					file.write(block, remaining);
				}
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

	for (U32 i = 0; i < mUploads.size(); ++i)
	{
		llinfos << "Discard the pending mesh uploads " << llendl;
		mUploads[i]->discard() ; //discard the uploading requests.
	}

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
			LLMeshRepository::sLODPending++;
		}
	}

	//do a quick search to see if we can't display something while we wait for this mesh to load
	LLVolume* volume = vobj->getVolume();

	if (volume)
	{
		LLVolumeParams params = volume->getParams();

		LLVolumeLODGroup* group = LLPrimitive::getVolumeManager()->getGroup(params);

		if (group)
		{
			//first, see if last_lod is available (don't transition down to avoid funny popping a la SH-641)
			if (last_lod >= 0)
			{
				LLVolume* lod = group->refLOD(last_lod);
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
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
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
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
				if (lod && lod->isMeshAssetLoaded() && lod->getNumVolumeFaces() > 0)
				{
					group->derefLOD(lod);
					return i;
				}

				group->derefLOD(lod);
			}
		}
	}

	return detail;
}

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

			// Handle addition of texture, if any.
			if ( data.mResponse.has("new_texture_folder_id") )
			{
				const LLUUID& folder_id = data.mResponse["new_texture_folder_id"].asUUID();

				if ( folder_id.notNull() )
				{
					LLUUID parent_id = gInventory.findCategoryUUIDForType(LLFolderType::FT_TEXTURE);

					std::string name;
					// Check if the server built a different name for the texture folder
					if ( data.mResponse.has("new_texture_folder_name") )
					{
						name = data.mResponse["new_texture_folder_name"].asString();
					}
					else
					{
						name = data.mPostData["name"].asString();
					}

					// Add the category to the internal representation
					LLPointer<LLViewerInventoryCategory> cat = 
						new LLViewerInventoryCategory(folder_id, parent_id, 
							LLFolderType::FT_NONE, name, gAgent.getID());
					cat->setVersion(LLViewerInventoryCategory::VERSION_UNKNOWN);

					LLInventoryModel::LLCategoryUpdate update(cat->getParentUUID(), 1);
					gInventory.accountForUpdate(update);
					gInventory.updateCategory(cat);
				}
			}

			on_new_single_inventory_upload_complete(
				asset_type,
				inventory_type,
				data.mPostData["asset_type"].asString(),
				data.mPostData["folder_id"].asUUID(),
				data.mPostData["name"],
				data.mPostData["description"],
				data.mResponse,
				data.mResponse["upload_price"]);
			//}
			
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

	{
		LLMutexLock lock1(mMeshMutex);
		LLMutexLock lock2(mThread->mMutex);
		
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
				LLMeshRepoThread::LODRequest& request = mPendingRequests.front();
				mThread->loadMeshLOD(request.mMeshParams, request.mLOD);
				mPendingRequests.erase(mPendingRequests.begin());
				LLMeshRepository::sLODPending--;
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
	}

	mThread->mSignal->signal();
}

void LLMeshRepository::notifySkinInfoReceived(LLMeshSkinInfo& info)
{
	mSkinMap[info.mMeshID] = info;

	skin_load_map::iterator iter = mLoadingSkins.find(info.mMeshID);
	if (iter != mLoadingSkins.end())
	{
		for (std::set<LLUUID>::iterator obj_id = iter->second.begin(); obj_id != iter->second.end(); ++obj_id)
		{
			LLVOVolume* vobj = (LLVOVolume*) gObjectList.findObject(*obj_id);
			if (vobj)
			{
				vobj->notifyMeshLoaded();
			}
		}
	}

	mLoadingSkins.erase(info.mMeshID);
}

void LLMeshRepository::notifyDecompositionReceived(LLModel::Decomposition* decomp)
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
		}
		
		{ //update system volume
			LLVolume* sys_volume = LLPrimitive::getVolumeManager()->refVolume(mesh_params, detail);
			if (sys_volume)
			{
				sys_volume->copyVolumeFaces(volume);
				sys_volume->setMeshAssetLoaded(TRUE);
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

const LLMeshSkinInfo* LLMeshRepository::getSkinInfo(const LLUUID& mesh_id, const LLVOVolume* requesting_obj)
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
			skin_load_map::iterator iter = mLoadingSkins.find(mesh_id);
			if (iter == mLoadingSkins.end())
			{ //no request pending for this skin info
				mPendingSkinRequests.push(mesh_id);
			}
			mLoadingSkins[mesh_id].insert(requesting_obj->getID());
		}
	}

	return NULL;
}

void LLMeshRepository::fetchPhysicsShape(const LLUUID& mesh_id)
{
	if (mesh_id.notNull())
	{
		LLModel::Decomposition* decomp = NULL;
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			decomp = iter->second;
		}
		
		//decomposition block hasn't been fetched yet
		if (!decomp || decomp->mPhysicsShapeMesh.empty())
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

LLModel::Decomposition* LLMeshRepository::getDecomposition(const LLUUID& mesh_id)
{
	LLModel::Decomposition* ret = NULL;

	if (mesh_id.notNull())
	{
		decomposition_map::iterator iter = mDecompositionMap.find(mesh_id);
		if (iter != mDecompositionMap.end())
		{
			ret = iter->second;
		}
		
		//decomposition block hasn't been fetched yet
		if (!ret || ret->mBaseHullMesh.empty())
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
	if (mesh.has("physics_mesh") && mesh["physics_mesh"].has("size") && (mesh["physics_mesh"]["size"].asInteger() > 0))
	{
		return true;
	}

	LLModel::Decomposition* decomp = getDecomposition(mesh_id);
	if (decomp && !decomp->mHull.empty())
	{
		return true;
	}

	return false;
}

LLSD& LLMeshRepository::getMeshHeader(const LLUUID& mesh_id)
{
	return mThread->getMeshHeader(mesh_id);
}

LLSD& LLMeshRepoThread::getMeshHeader(const LLUUID& mesh_id)
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
									bool upload_skin, bool upload_joints, std::string upload_url, bool do_upload,
								   LLHandle<LLWholeModelFeeObserver> fee_observer, LLHandle<LLWholeModelUploadObserver> upload_observer)
{
	LLMeshUploadThread* thread = new LLMeshUploadThread(data, scale, upload_textures, upload_skin, upload_joints, upload_url, 
														do_upload, fee_observer, upload_observer);
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

void LLMeshUploadThread::decomposeMeshMatrix(LLMatrix4& transformation,
											 LLVector3& result_pos,
											 LLQuaternion& result_rot,
											 LLVector3& result_scale)
{
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

	result_pos = position + mOrigin;
	result_scale = scale;
	result_rot = quat_rotation; 
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

	if (mBinding != rhs.mBinding)
	{
		return mBinding < rhs.mBinding;
	}

	return mFullbright < rhs.mFullbright;
}


void LLMeshRepository::updateInventory(inventory_data data)
{
	LLMutexLock lock(mMeshMutex);
	dump_llsd_to_file(data.mPostData,make_dump_name("update_inventory_post_data_",dump_num));
	dump_llsd_to_file(data.mResponse,make_dump_name("update_inventory_response_",dump_num));
	mInventoryQ.push(data);
}

void LLMeshRepository::uploadError(LLSD& args)
{
	LLMutexLock lock(mMeshMutex);
	mUploadErrorQ.push(args);
}

//static
F32 LLMeshRepository::getStreamingCost(LLSD& header, F32 radius, S32* bytes, S32* bytes_visible, S32 lod, F32 *unscaled_value)
{
	F32 max_distance = 512.f;

	F32 dlowest = llmin(radius/0.03f, max_distance);
	F32 dlow = llmin(radius/0.06f, max_distance);
	F32 dmid = llmin(radius/0.24f, max_distance);
	
	F32 METADATA_DISCOUNT = (F32) gSavedSettings.getU32("MeshMetaDataDiscount");  //discount 128 bytes to cover the cost of LLSD tags and compression domain overhead
	F32 MINIMUM_SIZE = (F32) gSavedSettings.getU32("MeshMinimumByteSize"); //make sure nothing is "free"

	F32 bytes_per_triangle = (F32) gSavedSettings.getU32("MeshBytesPerTriangle");

	S32 bytes_lowest = header["lowest_lod"]["size"].asInteger();
	S32 bytes_low = header["low_lod"]["size"].asInteger();
	S32 bytes_mid = header["medium_lod"]["size"].asInteger();
	S32 bytes_high = header["high_lod"]["size"].asInteger();

	if (bytes_high == 0)
	{
		return 0.f;
	}

	if (bytes_mid == 0)
	{
		bytes_mid = bytes_high;
	}

	if (bytes_low == 0)
	{
		bytes_low = bytes_mid;
	}

	if (bytes_lowest == 0)
	{
		bytes_lowest = bytes_low;
	}

	F32 triangles_lowest = llmax((F32) bytes_lowest-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_low = llmax((F32) bytes_low-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_mid = llmax((F32) bytes_mid-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;
	F32 triangles_high = llmax((F32) bytes_high-METADATA_DISCOUNT, MINIMUM_SIZE)/bytes_per_triangle;

	if (bytes)
	{
		*bytes = 0;
		*bytes += header["lowest_lod"]["size"].asInteger();
		*bytes += header["low_lod"]["size"].asInteger();
		*bytes += header["medium_lod"]["size"].asInteger();
		*bytes += header["high_lod"]["size"].asInteger();
	}

	if (bytes_visible)
	{
		lod = LLMeshRepository::getActualMeshLOD(header, lod);
		if (lod >= 0 && lod <= 3)
		{
			*bytes_visible = header[header_lod[lod]]["size"].asInteger();
		}
	}

	F32 max_area = 102932.f; //area of circle that encompasses region
	F32 min_area = 1.f;

	F32 high_area = llmin(F_PI*dmid*dmid, max_area);
	F32 mid_area = llmin(F_PI*dlow*dlow, max_area);
	F32 low_area = llmin(F_PI*dlowest*dlowest, max_area);
	F32 lowest_area = max_area;

	lowest_area -= low_area;
	low_area -= mid_area;
	mid_area -= high_area;

	high_area = llclamp(high_area, min_area, max_area);
	mid_area = llclamp(mid_area, min_area, max_area);
	low_area = llclamp(low_area, min_area, max_area);
	lowest_area = llclamp(lowest_area, min_area, max_area);

	F32 total_area = high_area + mid_area + low_area + lowest_area;
	high_area /= total_area;
	mid_area /= total_area;
	low_area /= total_area;
	lowest_area /= total_area;

	F32 weighted_avg = triangles_high*high_area +
					   triangles_mid*mid_area +
					   triangles_low*low_area +
					  triangles_lowest*lowest_area;

	if (unscaled_value)
	{
		*unscaled_value = weighted_avg;
	}

	return weighted_avg/gSavedSettings.getU32("MeshTriangleBudget")*15000.f;
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

void LLPhysicsDecomp::setMeshData(LLCDMeshData& mesh, bool vertex_based)
{
	mesh.mVertexBase = mCurRequest->mPositions[0].mV;
	mesh.mVertexStrideBytes = 12;
	mesh.mNumVertices = mCurRequest->mPositions.size();

	if(!vertex_based)
	{
		mesh.mIndexType = LLCDMeshData::INT_16;
		mesh.mIndexBase = &(mCurRequest->mIndices[0]);
		mesh.mIndexStrideBytes = 6;
	
		mesh.mNumTriangles = mCurRequest->mIndices.size()/3;
	}

	if ((vertex_based || mesh.mNumTriangles > 0) && mesh.mNumVertices > 2)
	{
		LLCDResult ret = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			ret  = LLConvexDecomposition::getInstance()->setMeshData(&mesh, vertex_based);
		}

		if (ret)
		{
			llerrs << "Convex Decomposition thread valid but could not set mesh data" << llendl;
		}
	}
}

void LLPhysicsDecomp::doDecomposition()
{
	LLCDMeshData mesh;
	S32 stage = mStageID[mCurRequest->mStage];

	if (LLConvexDecomposition::getInstance() == NULL)
	{
		// stub library. do nothing.
		return;
	}

	//load data intoLLCD
	if (stage == 0)
	{
		setMeshData(mesh, false);
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

	U32 ret = LLCD_OK;
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
	}

	mCurRequest->setStatusMessage("Executing.");

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
		
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(num_hulls);

			mCurRequest->mHullMesh.clear();
			mCurRequest->mHullMesh.resize(num_hulls);
		}

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

			get_vertex_buffer_from_mesh(mesh, mCurRequest->mHullMesh[i]);
			
			{
				LLMutexLock lock(mMutex);
				mCurRequest->mHull[i] = p;
			}
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
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();

	if (decomp == NULL)
	{
		//stub. do nothing.
		return;
	}
	
	LLCDMeshData mesh;	

	setMeshData(mesh, true);

	LLCDResult ret = decomp->buildSingleHull() ;
	if(ret)
	{
		llwarns << "Could not execute decomposition stage when attempting to create single hull." << llendl;
		make_box(mCurRequest);
	}
	else
	{
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull.clear();
			mCurRequest->mHull.resize(1);
			mCurRequest->mHullMesh.clear();
		}

		std::vector<LLVector3> p;
		LLCDHull hull;
		
		// if LLConvexDecomposition is a stub, num_hulls should have been set to 0 above, and we should not reach this code
		decomp->getSingleHull(&hull);

		const F32* v = hull.mVertexBase;

		for (S32 j = 0; j < hull.mNumVertices; ++j)
		{
			LLVector3 vert(v[0], v[1], v[2]); 
			p.push_back(vert);
			v = (F32*) (((U8*) v) + hull.mVertexStrideBytes);
		}
					
		{
			LLMutexLock lock(mMutex);
			mCurRequest->mHull[0] = p;
		}
	}		

	{
		completeCurrent();
		
	}
}


void LLPhysicsDecomp::run()
{
	LLConvexDecomposition* decomp = LLConvexDecomposition::getInstance();
	if (decomp == NULL)
	{
		// stub library. Set init to true so the main thread
		// doesn't wait for this to finish.
		mInited = true;
		return;
	}

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

void LLPhysicsDecomp::Request::assignData(LLModel* mdl) 
{
	if (!mdl)
	{
		return ;
	}

	U16 index_offset = 0;
	U16 tri[3] ;

	mPositions.clear();
	mIndices.clear();
	mBBox[1] = LLVector3(F32_MIN, F32_MIN, F32_MIN) ;
	mBBox[0] = LLVector3(F32_MAX, F32_MAX, F32_MAX) ;
		
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
			for(U32 k = 0 ; k < 3 ; k++)
			{
				mBBox[0].mV[k] = llmin(mBBox[0].mV[k], mPositions[j].mV[k]) ;
				mBBox[1].mV[k] = llmax(mBBox[1].mV[k], mPositions[j].mV[k]) ;
			}
		}

		updateTriangleAreaThreshold() ;

		for (U32 j = 0; j+2 < face.mNumIndices; j += 3)
		{
			tri[0] = face.mIndices[j] + index_offset ;
			tri[1] = face.mIndices[j + 1] + index_offset ;
			tri[2] = face.mIndices[j + 2] + index_offset ;
				
			if(isValidTriangle(tri[0], tri[1], tri[2]))
			{
				mIndices.push_back(tri[0]);
				mIndices.push_back(tri[1]);
				mIndices.push_back(tri[2]);
			}
		}

		index_offset += face.mNumVertices;
	}

	return ;
}

void LLPhysicsDecomp::Request::updateTriangleAreaThreshold() 
{
	F32 range = mBBox[1].mV[0] - mBBox[0].mV[0] ;
	range = llmin(range, mBBox[1].mV[1] - mBBox[0].mV[1]) ;
	range = llmin(range, mBBox[1].mV[2] - mBBox[0].mV[2]) ;

	mTriangleAreaThreshold = llmin(0.0002f, range * 0.000002f) ;
}

//check if the triangle area is large enough to qualify for a valid triangle
bool LLPhysicsDecomp::Request::isValidTriangle(U16 idx1, U16 idx2, U16 idx3) 
{
	LLVector3 a = mPositions[idx2] - mPositions[idx1] ;
	LLVector3 b = mPositions[idx3] - mPositions[idx1] ;
	F32 c = a * b ;

	return ((a*a) * (b*b) - c * c) > mTriangleAreaThreshold ;
}

void LLPhysicsDecomp::Request::setStatusMessage(const std::string& msg)
{
	mStatusMessage = msg;
}

LLModelInstance::LLModelInstance(LLSD& data)
{
	mLocalMeshID = data["mesh_id"].asInteger();
	mLabel = data["label"].asString();
	mTransform.setValue(data["transform"]);

	for (U32 i = 0; i < data["material"].size(); ++i)
	{
		LLImportMaterial mat(data["material"][i]);
		mMaterial[mat.mBinding] = mat;
	}
}


LLSD LLModelInstance::asLLSD()
{	
	LLSD ret;

	ret["mesh_id"] = mModel->mLocalID;
	ret["label"] = mLabel;
	ret["transform"] = mTransform.getValue();
	
	U32 i = 0;
	for (std::map<std::string, LLImportMaterial>::iterator iter = mMaterial.begin(); iter != mMaterial.end(); ++iter)
	{
		ret["material"][i++] = iter->second.asLLSD();
	}

	return ret;
}

LLImportMaterial::LLImportMaterial(LLSD& data)
{
	mDiffuseMapFilename = data["diffuse"]["filename"].asString();
	mDiffuseMapLabel = data["diffuse"]["label"].asString();
	mDiffuseColor.setValue(data["diffuse"]["color"]);
	mFullbright = data["fullbright"].asBoolean();
	mBinding = data["binding"].asString();
}


LLSD LLImportMaterial::asLLSD()
{
	LLSD ret;

	ret["diffuse"]["filename"] = mDiffuseMapFilename;
	ret["diffuse"]["label"] = mDiffuseMapLabel;
	ret["diffuse"]["color"] = mDiffuseColor.getValue();
	ret["fullbright"] = mFullbright;
	ret["binding"] = mBinding;

	return ret;
}

void LLMeshRepository::buildPhysicsMesh(LLModel::Decomposition& decomp)
{
	decomp.mMesh.resize(decomp.mHull.size());

	for (U32 i = 0; i < decomp.mHull.size(); ++i)
	{
		LLCDHull hull;
		hull.mNumVertices = decomp.mHull[i].size();
		hull.mVertexBase = decomp.mHull[i][0].mV;
		hull.mVertexStrideBytes = 12;

		LLCDMeshData mesh;
		LLCDResult res = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
		}
		if (res == LLCD_OK)
		{
			get_vertex_buffer_from_mesh(mesh, decomp.mMesh[i]);
		}
	}

	if (!decomp.mBaseHull.empty() && decomp.mBaseHullMesh.empty())
	{ //get mesh for base hull
		LLCDHull hull;
		hull.mNumVertices = decomp.mBaseHull.size();
		hull.mVertexBase = decomp.mBaseHull[0].mV;
		hull.mVertexStrideBytes = 12;

		LLCDMeshData mesh;
		LLCDResult res = LLCD_OK;
		if (LLConvexDecomposition::getInstance() != NULL)
		{
			res = LLConvexDecomposition::getInstance()->getMeshFromHull(&hull, &mesh);
		}
		if (res == LLCD_OK)
		{
			get_vertex_buffer_from_mesh(mesh, decomp.mBaseHullMesh);
		}
	}
}


bool LLMeshRepository::meshUploadEnabled()
{
	LLViewerRegion *region = gAgent.getRegion();
	if(gSavedSettings.getBOOL("MeshEnabled") &&
	   region)
	{
		return region->meshUploadEnabled();
	}
	return false;
}

bool LLMeshRepository::meshRezEnabled()
{
	LLViewerRegion *region = gAgent.getRegion();
	if(gSavedSettings.getBOOL("MeshEnabled") && 
	   region)
	{
		return region->meshRezEnabled();
	}
	return false;
}
