/** 
 * @file llmeshrepository.h
 * @brief Client-side repository of mesh assets.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_MESH_REPOSITORY_H
#define LL_MESH_REPOSITORY_H

#include "llassettype.h"
#include "llmodel.h"
#include "lluuid.h"
#include "llviewertexture.h"
#include "llvolume.h"

#if LL_MESH_ENABLED

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
	LLPointer<LLViewerFetchedTexture> mTexture;
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
};

class LLModelInstance 
{
public:
	LLPointer<LLModel> mModel;
	LLPointer<LLModel> mLOD[5];
	 
	LLUUID mMeshID;

	LLMatrix4 mTransform;
	std::vector<LLImportMaterial> mMaterial;

	LLModelInstance(LLModel* model, LLMatrix4& transform, std::vector<LLImportMaterial>& materials)
		: mModel(model), mTransform(transform), mMaterial(materials)
	{
	}
};

class LLMeshSkinInfo 
{
public:
	LLUUID mMeshID;
	std::vector<std::string> mJointNames;
	std::vector<LLMatrix4> mInvBindMatrix;
	LLMatrix4 mBindShapeMatrix;
};

class LLMeshDecomposition
{
public:
	LLMeshDecomposition() { }

	LLUUID mMeshID;
	LLModel::physics_shape mHull;

	std::vector<LLPointer<LLVertexBuffer> > mMesh;
};

class LLPhysicsDecomp : public LLThread
{
public:
	LLCondition* mSignal;
	LLMutex* mMutex;
	
	LLCDMeshData mMesh;
	
	bool mInited;
	bool mQuitting;
	bool mDone;

	S32 mContinue;
	std::string mStatus;

	std::vector<LLVector3> mPositions;
	std::vector<U16> mIndices;

	S32 mStage;

	LLPhysicsDecomp();
	~LLPhysicsDecomp();

	void shutdown();
	void setStatusMessage(std::string msg);
	
	void execute(const char* stage, LLModel* mdl);
	static S32 llcdCallback(const char*, S32, S32);
	void cancel();

	virtual void run();

	std::map<std::string, S32> mStageID;
	LLPointer<LLModel> mModel;
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
	
	//queue of completed Decomposition info requests
	std::queue<LLMeshDecomposition*> mDecompositionQ;

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

	void notifyLoadedMeshes();
	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	U32 getResourceCost(const LLUUID& mesh_params);

	void loadMeshSkinInfo(const LLUUID& mesh_id);
	void loadMeshDecomposition(const LLUUID& mesh_id);

	//send request for skin info, returns true if header info exists 
	//  (should hold onto mesh_id and try again later if header info does not exist)
	bool fetchMeshSkinInfo(const LLUUID& mesh_id);

	//send request for decomposition, returns true if header info exists 
	//  (should hold onto mesh_id and try again later if header info does not exist)
	bool fetchMeshDecomposition(const LLUUID& mesh_id);
};

class LLMeshUploadThread : public LLThread 
{
public:
	typedef std::vector<LLModelInstance> instance_list;
	instance_list mInstanceList;

	typedef std::map<LLPointer<LLModel>, instance_list> instance_map;
	instance_map mInstance;

	LLMutex*					mMutex;
	LLCurlRequest* mCurlRequest;
	S32				mPendingConfirmations;
	S32				mPendingUploads;
	S32				mPendingCost;
	bool			mFinished;
	LLVector3		mOrigin;
	bool			mUploadTextures;

	LLHost			mHost;
	std::string		mUploadObjectAssetCapability;
	std::string		mNewInventoryCapability;

	std::queue<LLMeshUploadData> mUploadQ;
	std::queue<LLMeshUploadData> mConfirmedQ;
	std::queue<LLModelInstance> mInstanceQ;

	std::queue<LLTextureUploadData> mTextureQ;
	std::queue<LLTextureUploadData> mConfirmedTextureQ;

	std::map<LLPointer<LLViewerFetchedTexture>, LLTextureUploadData> mTextureMap;

	LLMeshUploadThread(instance_list& data, LLVector3& scale, bool upload_textures);
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
	

	LLMeshRepository();

	void init();
	void shutdown();

	//mesh management functions
	S32 loadMesh(LLVOVolume* volume, const LLVolumeParams& mesh_params, S32 detail = 0);
	
	void notifyLoadedMeshes();
	void notifyMeshLoaded(const LLVolumeParams& mesh_params, LLVolume* volume);
	void notifyMeshUnavailable(const LLVolumeParams& mesh_params, S32 lod);
	void notifySkinInfoReceived(LLMeshSkinInfo& info);
	void notifyDecompositionReceived(LLMeshDecomposition* info);

	S32 getActualMeshLOD(const LLVolumeParams& mesh_params, S32 lod);
	U32 calcResourceCost(LLSD& header);
	U32 getResourceCost(const LLUUID& mesh_params);
	const LLMeshSkinInfo* getSkinInfo(const LLUUID& mesh_id);
	const LLMeshDecomposition* getDecomposition(const LLUUID& mesh_id);

	void uploadModel(std::vector<LLModelInstance>& data, LLVector3& scale, bool upload_textures);

	


	typedef std::map<LLVolumeParams, std::set<LLUUID> > mesh_load_map;
	mesh_load_map mLoadingMeshes[4];
	
	typedef std::map<LLUUID, LLMeshSkinInfo> skin_map;
	skin_map mSkinMap;

	typedef std::map<LLUUID, LLMeshDecomposition*> decomposition_map;
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
	
	U32 mMeshThreadCount;

	void cacheOutgoingMesh(LLMeshUploadData& data, LLSD& header);
	
	LLMeshRepoThread* mThread;
	std::vector<LLMeshUploadThread*> mUploads;

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

};

extern LLMeshRepository gMeshRepo;

#endif

#endif

