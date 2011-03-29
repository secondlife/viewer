/** 
 * @file llmeshrepository.h
 * @brief Client-side repository of mesh assets.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_MESH_REPOSITORY_H
#define LL_MESH_REPOSITORY_H

#include "llassettype.h"
#include "llmodel.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include "llvolume.h"

#define LLCONVEXDECOMPINTER_STATIC 1

#include "llconvexdecomposition.h"

class LLVOVolume;
class LLMeshResponder;
class LLCurlRequest;
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

class LLImportMaterial
{
public:
	LLPointer<LLViewerFetchedTexture> mDiffuseMap;
	std::string mDiffuseMapFilename;
	std::string mDiffuseMapLabel;
	LLColor4 mDiffuseColor;
	bool mFullbright;

	bool operator<(const LLImportMaterial &params) const;

	LLImportMaterial() 
		: mFullbright(false) 
	{ 
		mDiffuseColor.set(1,1,1,1);
	}

	LLImportMaterial(LLSD& data);

	LLSD asLLSD();
};

class LLModelInstance 
{
public:
	LLPointer<LLModel> mModel;
	LLPointer<LLModel> mLOD[5];
	
	std::string mLabel;

	LLUUID mMeshID;
	S32 mLocalMeshID;

	LLMatrix4 mTransform;
	std::vector<LLImportMaterial> mMaterial;

	LLModelInstance(LLModel* model, const std::string& label, LLMatrix4& transform, std::vector<LLImportMaterial>& materials)
		: mModel(model), mLabel(label), mTransform(transform), mMaterial(materials)
	{
		mLocalMeshID = -1;
	}

	LLModelInstance(LLSD& data);

	LLSD asLLSD();
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

	void setMeshData(LLCDMeshData& mesh);
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

	static S32 sActiveHeaderRequests;
	static S32 sActiveLODRequests;
	static U32 sMaxConcurrentRequests;

	LLCurlRequest* mCurlRequest;
	LLMutex*	mMutex;
	LLMutex*	mHeaderMutex;
	LLCondition* mSignal;

	bool mWaiting;

	//map of known mesh headers
	typedef std::map<LLUUID, LLSD> mesh_header_map;
	mesh_header_map mMeshHeader;
	
	std::map<LLUUID, U32> mMeshHeaderSize;
	std::map<LLUUID, U32> mMeshResourceCost;

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
	
	//queue of completed skin info requests
	std::queue<LLMeshSkinInfo> mSkinInfoQ;

	//set of requested decompositions
	std::set<LLUUID> mDecompositionRequests;

	//set of requested physics shapes
	std::set<LLUUID> mPhysicsShapeRequests;

	//queue of completed Decomposition info requests
	std::queue<LLModel::Decomposition*> mDecompositionQ;

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

	static std::string constructUrl(LLUUID mesh_id);

	LLMeshRepoThread();
	~LLMeshRepoThread();

	virtual void run();

	void loadMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	bool fetchMeshHeader(const LLVolumeParams& mesh_params);
	bool fetchMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	bool headerReceived(const LLVolumeParams& mesh_params, U8* data, S32 data_size);
	bool lodReceived(const LLVolumeParams& mesh_params, S32 lod, U8* data, S32 data_size);
	bool skinInfoReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool decompositionReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	bool physicsShapeReceived(const LLUUID& mesh_id, U8* data, S32 data_size);
	const LLSD& getMeshHeader(const LLUUID& mesh_id);

	void notifyLoadedMeshes();
	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	U32 getResourceCost(const LLUUID& mesh_params);

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


};

class LLMeshUploadThread : public LLThread 
{
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
	bool mPhysicsComplete;

	typedef std::map<LLPointer<LLModel>, std::vector<LLVector3> > hull_map;
	hull_map mHullMap;

	typedef std::vector<LLModelInstance> instance_list;
	instance_list mInstanceList;

	typedef std::map<LLPointer<LLModel>, instance_list> instance_map;
	instance_map mInstance;

	LLMutex*					mMutex;
	LLCurlRequest* mCurlRequest;
	S32				mPendingConfirmations;
	S32				mPendingUploads;
	S32				mPendingCost;
	LLVector3		mOrigin;
	bool			mFinished;	
	bool			mUploadTextures;
	bool			mUploadSkin;
	bool			mUploadJoints;
	BOOL            mDiscarded ;

	LLHost			mHost;
	std::string		mUploadObjectAssetCapability;
	std::string		mNewInventoryCapability;

	std::queue<LLMeshUploadData> mUploadQ;
	std::queue<LLMeshUploadData> mConfirmedQ;
	std::queue<LLModelInstance> mInstanceQ;

	std::queue<LLTextureUploadData> mTextureQ;
	std::queue<LLTextureUploadData> mConfirmedTextureQ;

	std::map<LLViewerFetchedTexture*, LLTextureUploadData> mTextureMap;

	LLMeshUploadThread(instance_list& data, LLVector3& scale, bool upload_textures,
			bool upload_skin, bool upload_joints);
	~LLMeshUploadThread();

	void uploadTexture(LLTextureUploadData& data);
	void doUploadTexture(LLTextureUploadData& data);
	void sendCostRequest(LLTextureUploadData& data);
	void priceResult(LLTextureUploadData& data, const LLSD& content);
	void onTextureUploaded(LLTextureUploadData& data);

	void uploadModel(LLMeshUploadData& data);
	void sendCostRequest(LLMeshUploadData& data);
	void doUploadModel(LLMeshUploadData& data);
	void onModelUploaded(LLMeshUploadData& data);
	void createObjects(LLMeshUploadData& data);
	LLSD createObject(LLModelInstance& instance);
	void priceResult(LLMeshUploadData& data, const LLSD& content);

	bool finished() { return mFinished; }
	virtual void run();
	void preStart();
	void discard() ;
	BOOL isDiscarded();
};

class LLMeshRepository
{
public:

	//metrics
	static U32 sBytesReceived;
	static U32 sHTTPRequestCount;
	static U32 sHTTPRetryCount;
	static U32 sCacheBytesRead;
	static U32 sCacheBytesWritten;
	static U32 sPeakKbps;
	
	static F32 getStreamingCost(const LLSD& header, F32 radius);

	LLMeshRepository();

	void init();
	void shutdown();
	S32 update() ;

	//mesh management functions
	S32 loadMesh(LLVOVolume* volume, const LLVolumeParams& mesh_params, S32 detail = 0, S32 last_lod = -1);
	
	void notifyLoadedMeshes();
	void notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume);
	void notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod);
	void notifySkinInfoReceived(LLMeshSkinInfo& info);
	void notifyDecompositionReceived(LLModel::Decomposition* info);

	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	U32 calcResourceCost(LLSD& header);
	U32 getResourceCost(const LLUUID& mesh_params);
	const LLMeshSkinInfo* getSkinInfo(const LLUUID& mesh_id);
	LLModel::Decomposition* getDecomposition(const LLUUID& mesh_id);
	void fetchPhysicsShape(const LLUUID& mesh_id);
	bool hasPhysicsShape(const LLUUID& mesh_id);
	
	void buildHull(const LLVolumeParams& params, S32 detail);
	void buildPhysicsMesh(LLModel::Decomposition& decomp);

	const LLSD& getMeshHeader(const LLUUID& mesh_id);

	void uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures,
			bool upload_skin, bool upload_joints);

	S32 getMeshSize(const LLUUID& mesh_id, S32 lod);

	typedef std::map<LLVolumeParams, std::set<LLUUID> > mesh_load_map;
	mesh_load_map mLoadingMeshes[4];
	
	typedef std::map<LLUUID, LLMeshSkinInfo> skin_map;
	skin_map mSkinMap;

	typedef std::map<LLUUID, LLModel::Decomposition*> decomposition_map;
	decomposition_map mDecompositionMap;

	LLMutex*					mMeshMutex;
	
	std::vector<LLMeshRepoThread::LODRequest> mPendingRequests;
	
	//list of mesh ids awaiting skin info
	std::set<LLUUID> mLoadingSkins;

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

	std::string mGetMeshCapability;

};

extern LLMeshRepository gMeshRepo;

#endif

