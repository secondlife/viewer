/** 
 * @file llmeshrepository.h
 * @brief Client-side repository of mesh assets.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010-2013, Linden Research, Inc.
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

#ifndef LL_MESH_REPOSITORY_H
#define LL_MESH_REPOSITORY_H

#include "llassettype.h"
#include "llmodel.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include "llvolume.h"
#include "lldeadmantimer.h"
#include "httpcommon.h"
#include "httprequest.h"
#include "httpoptions.h"
#include "httpheaders.h"
#include "httphandler.h"
#include "llthread.h"

#define LLCONVEXDECOMPINTER_STATIC 1

#include "llconvexdecomposition.h"
#include "lluploadfloaterobservers.h"

class LLVOVolume;
class LLMutex;
class LLCondition;
class LLVFS;
class LLMeshRepository;

class LLMeshUploadData
{
public:
	LLPointer<LLModel> mBaseModel;
	LLPointer<LLModel> mModel[5];
	LLUUID mUUID;
	U32 mRetries;
	std::string mRSVP;
	std::string mAssetData;
	LLSD mPostData;

	LLMeshUploadData()
	{
		mRetries = 0;
	}
};

class LLTextureUploadData
{
public:
	LLViewerFetchedTexture* mTexture;
	LLUUID mUUID;
	std::string mRSVP;
	std::string mLabel;
	U32 mRetries;
	std::string mAssetData;
	LLSD mPostData;

	LLTextureUploadData()
	{
		mRetries = 0;
	}

	LLTextureUploadData(LLViewerFetchedTexture* texture, std::string& label)
		: mTexture(texture), mLabel(label)
	{
		mRetries = 0;
	}
};

class LLPhysicsDecomp : public LLThread
{
public:

	typedef std::map<std::string, LLSD> decomp_params;

	class Request : public LLRefCount
	{
	public:
		//input params
		S32* mDecompID;
		std::string mStage;
		std::vector<LLVector3> mPositions;
		std::vector<U16> mIndices;
		decomp_params mParams;
				
		//output state
		std::string mStatusMessage;
		std::vector<LLModel::PhysicsMesh> mHullMesh;
		LLModel::convex_hull_decomposition mHull;
			
		//status message callback, called from decomposition thread
		virtual S32 statusCallback(const char* status, S32 p1, S32 p2) = 0;

		//completed callback, called from the main thread
		virtual void completed() = 0;

		virtual void setStatusMessage(const std::string& msg);

		bool isValid() const {return mPositions.size() > 2 && mIndices.size() > 2 ;}

	protected:
		//internal use
		LLVector3 mBBox[2] ;
		F32 mTriangleAreaThreshold ;

		void assignData(LLModel* mdl) ;
		void updateTriangleAreaThreshold() ;
		bool isValidTriangle(U16 idx1, U16 idx2, U16 idx3) ;
	};

	LLCondition* mSignal;
	LLMutex* mMutex;
	
	bool mInited;
	bool mQuitting;
	bool mDone;
	
	LLPhysicsDecomp();
	~LLPhysicsDecomp();

	void shutdown();
		
	void submitRequest(Request* request);
	static S32 llcdCallback(const char*, S32, S32);
	void cancel();

	void setMeshData(LLCDMeshData& mesh, bool vertex_based);
	void doDecomposition();
	void doDecompositionSingleHull();

	virtual void run();
	
	void completeCurrent();
	void notifyCompleted();

	std::map<std::string, S32> mStageID;

	typedef std::queue<LLPointer<Request> > request_queue;
	request_queue mRequestQ;

	LLPointer<Request> mCurRequest;

	std::queue<LLPointer<Request> > mCompletedQ;

};

class LLMeshRepoThread : public LLThread
{
public:

	volatile static S32 sActiveHeaderRequests;
	volatile static S32 sActiveLODRequests;
	static U32 sMaxConcurrentRequests;
	static S32 sRequestLowWater;
	static S32 sRequestHighWater;
	static S32 sRequestWaterLevel;			// Stats-use only, may read outside of thread

	LLMutex*	mMutex;
	LLMutex*	mHeaderMutex;
	LLCondition* mSignal;

	//map of known mesh headers
	typedef std::map<LLUUID, LLSD> mesh_header_map;
	mesh_header_map mMeshHeader;
	
	std::map<LLUUID, U32> mMeshHeaderSize;
	
	class HeaderRequest
	{ 
	public:
		const LLVolumeParams mMeshParams;

		HeaderRequest(const LLVolumeParams&  mesh_params)
			: mMeshParams(mesh_params)
		{
		}

		bool operator<(const HeaderRequest& rhs) const
		{
			return mMeshParams < rhs.mMeshParams;
		}
	};

	class LODRequest
	{
	public:
		LLVolumeParams  mMeshParams;
		S32 mLOD;
		F32 mScore;

		LODRequest(const LLVolumeParams&  mesh_params, S32 lod)
			: mMeshParams(mesh_params), mLOD(lod), mScore(0.f)
		{
		}
	};

	struct CompareScoreGreater
	{
		bool operator()(const LODRequest& lhs, const LODRequest& rhs)
		{
			return lhs.mScore > rhs.mScore; // greatest = first
		}
	};
	

	class LoadedMesh
	{
	public:
		LLPointer<LLVolume> mVolume;
		LLVolumeParams mMeshParams;
		S32 mLOD;

		LoadedMesh(LLVolume* volume, const LLVolumeParams&  mesh_params, S32 lod)
			: mVolume(volume), mMeshParams(mesh_params), mLOD(lod)
		{
		}

	};

	//set of requested skin info
	std::set<LLUUID> mSkinRequests;
	
	// list of completed skin info requests
	std::list<LLMeshSkinInfo> mSkinInfoQ;

	//set of requested decompositions
	std::set<LLUUID> mDecompositionRequests;

	//set of requested physics shapes
	std::set<LLUUID> mPhysicsShapeRequests;

	// list of completed Decomposition info requests
	std::list<LLModel::Decomposition*> mDecompositionQ;

	//queue of requested headers
	std::queue<HeaderRequest> mHeaderReqQ;

	//queue of requested LODs
	std::queue<LODRequest> mLODReqQ;

	//queue of unavailable LODs (either asset doesn't exist or asset doesn't have desired LOD)
	std::queue<LODRequest> mUnavailableQ;

	//queue of successfully loaded meshes
	std::queue<LoadedMesh> mLoadedQ;

	//map of pending header requests and currently desired LODs
	typedef std::map<LLVolumeParams, std::vector<S32> > pending_lod_map;
	pending_lod_map mPendingLOD;

	// llcorehttp library interface objects.
	LLCore::HttpStatus					mHttpStatus;
	LLCore::HttpRequest *				mHttpRequest;
	LLCore::HttpOptions::ptr_t			mHttpOptions;
	LLCore::HttpOptions::ptr_t			mHttpLargeOptions;
	LLCore::HttpHeaders::ptr_t			mHttpHeaders;
	LLCore::HttpRequest::policy_t		mHttpPolicyClass;
	LLCore::HttpRequest::policy_t		mHttpLegacyPolicyClass;
	LLCore::HttpRequest::policy_t		mHttpLargePolicyClass;
	LLCore::HttpRequest::priority_t		mHttpPriority;

	typedef std::set<LLCore::HttpHandler::ptr_t> http_request_set;
	http_request_set					mHttpRequestSet;			// Outstanding HTTP requests

	std::string mGetMeshCapability;
	std::string mGetMesh2Capability;
	int mGetMeshVersion;

	LLMeshRepoThread();
	~LLMeshRepoThread();

	virtual void run();

	void lockAndLoadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	void loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);

	bool fetchMeshHeader(const LLVolumeParams& mesh_params);
	bool fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	bool headerReceived(const LLVolumeParams& mesh_params, U8* data, S32 data_size);
	bool lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size);
	bool skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	LLSD& getMeshHeader(const LLUUID& mesh_id);

	void notifyLoadedMeshes();
	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	
	void loadMeshSkinInfo(const LLUUID& mesh_id);
	void loadMeshDecomposition(const LLUUID& mesh_id);
	void loadMeshPhysicsShape(const LLUUID& mesh_id);

	//send request for skin info, returns true if header info exists 
	//  (should hold onto mesh_id and try again later if header info does not exist)
	bool fetchMeshSkinInfo(const LLUUID& mesh_id);

	//send request for decomposition, returns true if header info exists 
	//  (should hold onto mesh_id and try again later if header info does not exist)
	bool fetchMeshDecomposition(const LLUUID& mesh_id);

	//send request for PhysicsShape, returns true if header info exists 
	//  (should hold onto mesh_id and try again later if header info does not exist)
	bool fetchMeshPhysicsShape(const LLUUID& mesh_id);

	static void incActiveLODRequests();
	static void decActiveLODRequests();
	static void incActiveHeaderRequests();
	static void decActiveHeaderRequests();

	// Set the caps strings and preferred version for constructing
	// mesh fetch URLs.
	//
	// Mutex:  must be holding mMutex when called
	void setGetMeshCaps(const std::string & get_mesh1,
						const std::string & get_mesh2,
						int pref_version);

	// Mutex:  acquires mMutex
	void constructUrl(LLUUID mesh_id, std::string * url, int * version);

private:
	// Issue a GET request to a URL with 'Range' header using
	// the correct policy class and other attributes.  If an invalid
	// handle is returned, the request failed and caller must retry
	// or dispose of handler.
	//
	// Threads:  Repo thread only
	LLCore::HttpHandle getByteRange(const std::string & url, int cap_version,
									size_t offset, size_t len, 
									const LLCore::HttpHandler::ptr_t &handler);
};


// Class whose instances represent a single upload-type request for
// meshes:  one fee query or one actual upload attempt.  Yes, it creates
// a unique thread for that single request.  As it is 1:1, it can also
// trivially serve as the HttpHandler object for request completion
// notifications.

class LLMeshUploadThread : public LLThread, public LLCore::HttpHandler 
{
private:
	S32 mMeshUploadTimeOut ; //maximum time in seconds to execute an uploading request.

public:
	class DecompRequest : public LLPhysicsDecomp::Request
	{
	public:
		LLPointer<LLModel> mModel;
		LLPointer<LLModel> mBaseModel;

		LLMeshUploadThread* mThread;

		DecompRequest(LLModel* mdl, LLModel* base_model, LLMeshUploadThread* thread);

		S32 statusCallback(const char* status, S32 p1, S32 p2) { return 1; }
		void completed();
	};

	LLPointer<DecompRequest> mFinalDecomp;
	volatile bool	mPhysicsComplete;

	typedef std::map<LLPointer<LLModel>, std::vector<LLVector3> > hull_map;
	hull_map		mHullMap;

	typedef std::vector<LLModelInstance> instance_list;
	instance_list	mInstanceList;

	typedef std::map<LLPointer<LLModel>, instance_list> instance_map;
	instance_map	mInstance;

	LLMutex*		mMutex;
	S32				mPendingUploads;
	LLVector3		mOrigin;
	bool			mFinished;	
	bool			mUploadTextures;
	bool			mUploadSkin;
	bool			mUploadJoints;
    bool			mLockScaleIfJointPosition;
	volatile bool	mDiscarded;

	LLHost			mHost;
	std::string		mWholeModelFeeCapability;
	std::string		mWholeModelUploadURL;

	LLMeshUploadThread(instance_list& data, LLVector3& scale, bool upload_textures,
					   bool upload_skin, bool upload_joints, bool lock_scale_if_joint_position,
                       const std::string & upload_url, bool do_upload = true,
					   LLHandle<LLWholeModelFeeObserver> fee_observer = (LLHandle<LLWholeModelFeeObserver>()),
					   LLHandle<LLWholeModelUploadObserver> upload_observer = (LLHandle<LLWholeModelUploadObserver>()));
	~LLMeshUploadThread();

	bool finished() const { return mFinished; }
	virtual void run();
	void preStart();
	void discard() ;
	bool isDiscarded() const;

	void generateHulls();

	void doWholeModelUpload();
	void requestWholeModelFee();

	void wholeModelToLLSD(LLSD& dest, bool include_textures);

	void decomposeMeshMatrix(LLMatrix4& transformation,
							 LLVector3& result_pos,
							 LLQuaternion& result_rot,
							 LLVector3& result_scale);

	void setFeeObserverHandle(LLHandle<LLWholeModelFeeObserver> observer_handle) { mFeeObserverHandle = observer_handle; }
	void setUploadObserverHandle(LLHandle<LLWholeModelUploadObserver> observer_handle) { mUploadObserverHandle = observer_handle; }

	// Inherited from LLCore::HttpHandler
	virtual void onCompleted(LLCore::HttpHandle handle, LLCore::HttpResponse * response);

        LLViewerFetchedTexture* FindViewerTexture(const LLImportMaterial& material);

private:
	LLHandle<LLWholeModelFeeObserver> mFeeObserverHandle;
	LLHandle<LLWholeModelUploadObserver> mUploadObserverHandle;

	bool mDoUpload; // if FALSE only model data will be requested, otherwise the model will be uploaded
	LLSD mModelData;
	
	// llcorehttp library interface objects.
	LLCore::HttpStatus					mHttpStatus;
	LLCore::HttpRequest *				mHttpRequest;
	LLCore::HttpOptions::ptr_t			mHttpOptions;
	LLCore::HttpHeaders::ptr_t			mHttpHeaders;
	LLCore::HttpRequest::policy_t		mHttpPolicyClass;
	LLCore::HttpRequest::priority_t		mHttpPriority;
};

class LLMeshRepository
{
public:

	//metrics
	static U32 sBytesReceived;
	static U32 sMeshRequestCount;				// Total request count, http or cached, all component types
	static U32 sHTTPRequestCount;				// Http GETs issued (not large)
	static U32 sHTTPLargeRequestCount;			// Http GETs issued for large requests
	static U32 sHTTPRetryCount;					// Total request retries whether successful or failed
	static U32 sHTTPErrorCount;					// Requests ending in error
	static U32 sLODPending;
	static U32 sLODProcessing;
	static U32 sCacheBytesRead;
	static U32 sCacheBytesWritten;
	static U32 sCacheReads;						
	static U32 sCacheWrites;
	static U32 sMaxLockHoldoffs;				// Maximum sequential locking failures
	
	static LLDeadmanTimer sQuiescentTimer;		// Time-to-complete-mesh-downloads after significant events

	F32 getStreamingCost(LLUUID mesh_id, F32 radius, S32* bytes = NULL, S32* visible_bytes = NULL, S32 detail = -1, F32 *unscaled_value = NULL);
	static F32 getStreamingCost(LLSD& header, F32 radius, S32* bytes = NULL, S32* visible_bytes = NULL, S32 detail = -1, F32 *unscaled_value = NULL);

	LLMeshRepository();

	void init();
	void shutdown();
	S32 update();

	//mesh management functions
	S32 loadMesh(LLVOVolume* volume, const LLVolumeParams& mesh_params, S32 detail = 0, S32 last_lod = -1);
	
	void notifyLoadedMeshes();
	void notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume);
	void notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod);
	void notifySkinInfoReceived(LLMeshSkinInfo& info);
	void notifyDecompositionReceived(LLModel::Decomposition* info);

	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	static S32 getActualMeshLOD(LLSD& header, S32 lod);
	const LLMeshSkinInfo* getSkinInfo(const LLUUID& mesh_id, const LLVOVolume* requesting_obj);
	LLModel::Decomposition* getDecomposition(const LLUUID& mesh_id);
	void fetchPhysicsShape(const LLUUID& mesh_id);
	bool hasPhysicsShape(const LLUUID& mesh_id);
	
	void buildHull(const LLVolumeParams& params, S32 detail);
	void buildPhysicsMesh(LLModel::Decomposition& decomp);
	
	bool meshUploadEnabled();
	bool meshRezEnabled();
	

	LLSD& getMeshHeader(const LLUUID& mesh_id);

	void uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures,
                     bool upload_skin, bool upload_joints, bool lock_scale_if_joint_position,
                     std::string upload_url, bool do_upload = true,
					 LLHandle<LLWholeModelFeeObserver> fee_observer= (LLHandle<LLWholeModelFeeObserver>()), 
                     LLHandle<LLWholeModelUploadObserver> upload_observer = (LLHandle<LLWholeModelUploadObserver>()));

	S32 getMeshSize(const LLUUID& mesh_id, S32 lod);

	// Quiescent timer management, main thread only.
	static void metricsStart();
	static void metricsStop();
	static void metricsProgress(unsigned int count);
	static void metricsUpdate();
	
	typedef std::map<LLVolumeParams, std::set<LLUUID> > mesh_load_map;
	mesh_load_map mLoadingMeshes[4];
	
	typedef std::map<LLUUID, LLMeshSkinInfo> skin_map;
	skin_map mSkinMap;

	typedef std::map<LLUUID, LLModel::Decomposition*> decomposition_map;
	decomposition_map mDecompositionMap;

	LLMutex*					mMeshMutex;
	
	std::vector<LLMeshRepoThread::LODRequest> mPendingRequests;
	
	//list of mesh ids awaiting skin info
	typedef std::map<LLUUID, std::set<LLUUID> > skin_load_map;
	skin_load_map mLoadingSkins;

	//list of mesh ids that need to send skin info fetch requests
	std::queue<LLUUID> mPendingSkinRequests;

	//list of mesh ids awaiting decompositions
	std::set<LLUUID> mLoadingDecompositions;

	//list of mesh ids that need to send decomposition fetch requests
	std::queue<LLUUID> mPendingDecompositionRequests;
	
	//list of mesh ids awaiting physics shapes
	std::set<LLUUID> mLoadingPhysicsShapes;

	//list of mesh ids that need to send physics shape fetch requests
	std::queue<LLUUID> mPendingPhysicsShapeRequests;
	
	U32 mMeshThreadCount;

	void cacheOutgoingMesh(LLMeshUploadData& data, LLSD& header);
	
	LLMeshRepoThread* mThread;
	std::vector<LLMeshUploadThread*> mUploads;
	std::vector<LLMeshUploadThread*> mUploadWaitList;

	LLPhysicsDecomp* mDecompThread;
	
	class inventory_data
	{
	public:
		LLSD mPostData;
		LLSD mResponse;

		inventory_data(const LLSD& data, const LLSD& content)
			: mPostData(data), mResponse(content)
		{
		}
	};

	std::queue<inventory_data> mInventoryQ;

	std::queue<LLSD> mUploadErrorQ;

	void uploadError(LLSD& args);
	void updateInventory(inventory_data data);

	int mGetMeshVersion;		// Shadows value in LLMeshRepoThread
};

extern LLMeshRepository gMeshRepo;

#endif

