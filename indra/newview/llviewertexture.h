/** 
 * @file llviewertexture.h
 * @brief Object for managing images and their textures
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
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

#ifndef LL_LLVIEWERTEXTURE_H					
#define LL_LLVIEWERTEXTURE_H

#include "llatomic.h"
#include "llgltexture.h"
#include "lltimer.h"
#include "llframetimer.h"
#include "llhost.h"
#include "llgltypes.h"
#include "llrender.h"
#include "llmetricperformancetester.h"
#include "httpcommon.h"
#include "workqueue.h"

#include <map>
#include <list>

extern const S32Megabytes gMinVideoRam;
extern const S32Megabytes gMaxVideoRam;

class LLFace;
class LLImageGL ;
class LLImageRaw;
class LLViewerObject;
class LLViewerTexture;
class LLViewerFetchedTexture ;
class LLViewerMediaTexture ;
class LLTexturePipelineTester ;


typedef	void	(*loaded_callback_func)( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata );

class LLFileSystem;
class LLMessageSystem;
class LLViewerMediaImpl ;
class LLVOVolume ;
struct LLTextureKey;

class LLLoadedCallbackEntry
{
public:
    typedef std::set< LLTextureKey > source_callback_list_t;

public:
	LLLoadedCallbackEntry(loaded_callback_func cb,
						  S32 discard_level,
						  BOOL need_imageraw, // Needs image raw for the callback
						  void* userdata,
						  source_callback_list_t* src_callback_list,
						  LLViewerFetchedTexture* target,
						  BOOL pause);
	~LLLoadedCallbackEntry();
	void removeTexture(LLViewerFetchedTexture* tex) ;

	loaded_callback_func	mCallback;
	S32						mLastUsedDiscard;
	S32						mDesiredDiscard;
	BOOL					mNeedsImageRaw;
	BOOL                    mPaused;
	void*					mUserData;
	source_callback_list_t* mSourceCallbackList;
	
public:
	static void cleanUpCallbackList(LLLoadedCallbackEntry::source_callback_list_t* callback_list) ;
};

class LLTextureBar;

class LLViewerTexture : public LLGLTexture
{
public:
	enum
	{
		LOCAL_TEXTURE,		
		MEDIA_TEXTURE,
		DYNAMIC_TEXTURE,
		FETCHED_TEXTURE,
		LOD_TEXTURE,
		ATLAS_TEXTURE,
		INVALID_TEXTURE_TYPE
	};

	typedef std::vector<class LLFace*> ll_face_list_t;
	typedef std::vector<LLVOVolume*> ll_volume_list_t;


protected:
	virtual ~LLViewerTexture();
	LOG_CLASS(LLViewerTexture);

public:	
	static void initClass();
	static void updateClass();
	
	LLViewerTexture(BOOL usemipmaps = TRUE);
	LLViewerTexture(const LLUUID& id, BOOL usemipmaps) ;
	LLViewerTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	LLViewerTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps) ;

	virtual S8 getType() const;
	virtual BOOL isMissingAsset() const ;
	virtual void dump();	// debug info to LL_INFOS()
	
    virtual bool isViewerMediaTexture() const { return false; }

	/*virtual*/ bool bindDefaultImage(const S32 stage = 0) ;
	/*virtual*/ bool bindDebugImage(const S32 stage = 0) ;
	/*virtual*/ void forceImmediateUpdate() ;
	/*virtual*/ bool isActiveFetching();
	
	/*virtual*/ const LLUUID& getID() const { return mID; }
	void setBoostLevel(S32 level);
	S32  getBoostLevel() { return mBoostLevel; }
	void setTextureListType(S32 tex_type) { mTextureListType = tex_type; }
	S32 getTextureListType() { return mTextureListType; }

	void addTextureStats(F32 virtual_size, BOOL needs_gltexture = TRUE) const;
	void resetTextureStats();	
	void setMaxVirtualSizeResetInterval(S32 interval)const {mMaxVirtualSizeResetInterval = interval;}
	void resetMaxVirtualSizeResetCounter()const {mMaxVirtualSizeResetCounter = mMaxVirtualSizeResetInterval;}
	S32 getMaxVirtualSizeResetCounter() const { return mMaxVirtualSizeResetCounter; }

	virtual F32  getMaxVirtualSize() ;

	LLFrameTimer* getLastReferencedTimer() {return &mLastReferencedTimer ;}
	
	S32 getFullWidth() const { return mFullWidth; }
	S32 getFullHeight() const { return mFullHeight; }	
	/*virtual*/ void setKnownDrawSize(S32 width, S32 height);

	virtual void addFace(U32 channel, LLFace* facep) ;
	virtual void removeFace(U32 channel, LLFace* facep) ; 
	S32 getTotalNumFaces() const;
	S32 getNumFaces(U32 ch) const;
	const ll_face_list_t* getFaceList(U32 channel) const {llassert(channel < LLRender::NUM_TEXTURE_CHANNELS); return &mFaceList[channel];}

	virtual void addVolume(U32 channel, LLVOVolume* volumep);
	virtual void removeVolume(U32 channel, LLVOVolume* volumep);
	S32 getNumVolumes(U32 channel) const;
	const ll_volume_list_t* getVolumeList(U32 channel) const { return &mVolumeList[channel]; }

	
	virtual void setCachedRawImage(S32 discard_level, LLImageRaw* imageraw) ;
	BOOL isLargeImage() ;	
	
	void setParcelMedia(LLViewerMediaTexture* media) {mParcelMedia = media;}
	BOOL hasParcelMedia() const { return mParcelMedia != NULL;}
	LLViewerMediaTexture* getParcelMedia() const { return mParcelMedia;}

	/*virtual*/ void updateBindStatsForTester() ;
protected:
	void cleanup() ;
	void init(bool firstinit) ;
	void reorganizeFaceList() ;
	void reorganizeVolumeList() ;

	void notifyAboutMissingAsset();
	void notifyAboutCreatingTexture();

private:
	friend class LLBumpImageList;
	friend class LLUIImageList;

	virtual void switchToCachedImage();
	
	static bool isMemoryForTextureLow() ;
	static bool isMemoryForTextureSuficientlyFree();
	static void getGPUMemoryForTextures(S32Megabytes &gpu, S32Megabytes &physical);

protected:
	LLUUID mID;
	S32 mTextureListType; // along with mID identifies where to search for this texture in TextureList

	F32 mSelectedTime;				// time texture was last selected
	mutable F32 mMaxVirtualSize;	// The largest virtual size of the image, in pixels - how much data to we need?	
	mutable S32  mMaxVirtualSizeResetCounter ;
	mutable S32  mMaxVirtualSizeResetInterval;
	mutable F32 mAdditionalDecodePriority;  // priority add to mDecodePriority.
	LLFrameTimer mLastReferencedTimer;	

	ll_face_list_t    mFaceList[LLRender::NUM_TEXTURE_CHANNELS]; //reverse pointer pointing to the faces using this image as texture
	U32               mNumFaces[LLRender::NUM_TEXTURE_CHANNELS];
	LLFrameTimer      mLastFaceListUpdateTimer ;

	ll_volume_list_t  mVolumeList[LLRender::NUM_VOLUME_TEXTURE_CHANNELS];
	U32					mNumVolumes[LLRender::NUM_VOLUME_TEXTURE_CHANNELS];
	LLFrameTimer	  mLastVolumeListUpdateTimer;

	//do not use LLPointer here.
	LLViewerMediaTexture* mParcelMedia ;

	LL::WorkQueue::weak_t mMainQueue;
	LL::WorkQueue::weak_t mImageQueue;

	static F32 sTexelPixelRatio;
public:
	static const U32 sCurrentFileVersion;	
	static S32 sImageCount;
	static S32 sRawCount;
	static S32 sAuxCount;
	static LLFrameTimer sEvaluationTimer;
	static F32 sDesiredDiscardBias;
	static F32 sDesiredDiscardScale;
	static S32Bytes sBoundTextureMemory;
	static S32Bytes sTotalTextureMemory;
	static S32Megabytes sMaxBoundTextureMemory;
	static S32Megabytes sMaxTotalTextureMem;
	static S32Bytes sMaxDesiredTextureMem ;
	static S8  sCameraMovingDiscardBias;
	static F32 sCameraMovingBias;
	static S32 sMaxSculptRez ;
	static U32 sMinLargeImageSize ;
	static U32 sMaxSmallImageSize ;
	static bool sFreezeImageUpdates;
	static F32  sCurrentTime ;

	enum EDebugTexels
	{
		DEBUG_TEXELS_OFF,
		DEBUG_TEXELS_CURRENT,
		DEBUG_TEXELS_DESIRED,
		DEBUG_TEXELS_FULL
	};

	static EDebugTexels sDebugTexelsMode;

	static LLPointer<LLViewerTexture> sNullImagep; // Null texture for non-textured objects.
	static LLPointer<LLViewerTexture> sBlackImagep;	// Texture to show NOTHING (pure black)
	static LLPointer<LLViewerTexture> sCheckerBoardImagep;	// Texture to show NOTHING (pure black)
};


enum FTType
{
	FTT_UNKNOWN = -1,
	FTT_DEFAULT = 0, // standard texture fetched by id.
	FTT_SERVER_BAKE, // texture produced by appearance service and fetched from there.
	FTT_HOST_BAKE, // old-style baked texture uploaded by viewer and fetched from avatar's host.
	FTT_MAP_TILE, // tiles are fetched from map server directly.
	FTT_LOCAL_FILE // fetch directly from a local file.
};

const std::string& fttype_to_string(const FTType& fttype);

//
//textures are managed in gTextureList.
//raw image data is fetched from remote or local cache
//but the raw image this texture pointing to is fixed.
//
class LLViewerFetchedTexture : public LLViewerTexture
{
	friend class LLTextureBar; // debug info only
	friend class LLTextureView; // debug info only

protected:
	/*virtual*/ ~LLViewerFetchedTexture();
public:
	LLViewerFetchedTexture(const LLUUID& id, FTType f_type, const LLHost& host = LLHost(), BOOL usemipmaps = TRUE);
	LLViewerFetchedTexture(const LLImageRaw* raw, FTType f_type, BOOL usemipmaps);
	LLViewerFetchedTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps = TRUE);

public:
	static F32 maxDecodePriority();
	
	struct Compare
	{
		// lhs < rhs
		bool operator()(const LLPointer<LLViewerFetchedTexture> &lhs, const LLPointer<LLViewerFetchedTexture> &rhs) const
		{
			const LLViewerFetchedTexture* lhsp = (const LLViewerFetchedTexture*)lhs;
			const LLViewerFetchedTexture* rhsp = (const LLViewerFetchedTexture*)rhs;
			// greater priority is "less"
			const F32 lpriority = lhsp->getDecodePriority();
			const F32 rpriority = rhsp->getDecodePriority();
			if (lpriority > rpriority) // higher priority
				return true;
			if (lpriority < rpriority)
				return false;
			return lhsp < rhsp;
		}
	};

public:
	/*virtual*/ S8 getType() const ;
	FTType getFTType() const;
	/*virtual*/ void forceImmediateUpdate() ;
	/*virtual*/ void dump() ;

	// Set callbacks to get called when the image gets updated with higher 
	// resolution versions.
	void setLoadedCallback(loaded_callback_func cb,
						   S32 discard_level, BOOL keep_imageraw, BOOL needs_aux,
						   void* userdata, LLLoadedCallbackEntry::source_callback_list_t* src_callback_list, BOOL pause = FALSE);
	bool hasCallbacks() { return mLoadedCallbackList.empty() ? false : true; }	
	void pauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
	void unpauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
	bool doLoadedCallbacks();
	void deleteCallbackEntry(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
	void clearCallbackEntryList() ;

	void addToCreateTexture();

    //call to determine if createTexture is necessary
    BOOL preCreateTexture(S32 usename = 0);
	 // ONLY call from LLViewerTextureList or ImageGL background thread
	BOOL createTexture(S32 usename = 0);
    void postCreateTexture();
    void scheduleCreateTexture();

	void destroyTexture() ;

	virtual void processTextureStats() ;
	F32  calcDecodePriority() ;

	BOOL needsAux() const { return mNeedsAux; }

	// Host we think might have this image, used for baked av textures.
	void setTargetHost(LLHost host)			{ mTargetHost = host; }
	LLHost getTargetHost() const			{ return mTargetHost; }
	
	// Set the decode priority for this image...
	// DON'T CALL THIS UNLESS YOU KNOW WHAT YOU'RE DOING, it can mess up
	// the priority list, and cause horrible things to happen.
	void setDecodePriority(F32 priority = -1.0f);
	F32 getDecodePriority() const { return mDecodePriority; };
	F32 getAdditionalDecodePriority() const { return mAdditionalDecodePriority; };

	void setAdditionalDecodePriority(F32 priority) ;
	
	void updateVirtualSize() ;

	S32  getDesiredDiscardLevel()			 { return mDesiredDiscardLevel; }
	void setMinDiscardLevel(S32 discard) 	{ mMinDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel,(S8)discard); }

	bool updateFetch();
	bool setDebugFetching(S32 debug_level);
	bool isInDebug() const { return mInDebug; }

	void setUnremovable(BOOL value) { mUnremovable = value; }
	bool isUnremovable() const { return mUnremovable; }
	
	void clearFetchedResults(); //clear all fetched results, for debug use.

	// Override the computation of discard levels if we know the exact output
	// size of the image.  Used for UI textures to not decode, even if we have
	// more data.
	/*virtual*/ void setKnownDrawSize(S32 width, S32 height);

	void setIsMissingAsset(BOOL is_missing = true);
	/*virtual*/ BOOL isMissingAsset() const { return mIsMissingAsset; }

	// returns dimensions of original image for local files (before power of two scaling)
	// and returns 0 for all asset system images
	S32 getOriginalWidth() { return mOrigWidth; }
	S32 getOriginalHeight() { return mOrigHeight; }

	BOOL isInImageList() const {return mInImageList ;}
	void setInImageList(BOOL flag) {mInImageList = flag ;}

	LLFrameTimer* getLastPacketTimer() {return &mLastPacketTimer;}

	U32 getFetchPriority() const { return mFetchPriority ;}
	F32 getDownloadProgress() const {return mDownloadProgress ;}

	LLImageRaw* reloadRawImage(S8 discard_level) ;
	void destroyRawImage();
	bool needsToSaveRawImage();

	const std::string& getUrl() const {return mUrl;}
	//---------------
	BOOL isDeleted() ;
	BOOL isInactive() ;
	BOOL isDeletionCandidate();
	void setDeletionCandidate() ;
	void setInactive() ;
	BOOL getUseDiscard() const { return mUseMipMaps && !mDontDiscard; }	
	//---------------

	void setForSculpt();
	BOOL forSculpt() const {return mForSculpt;}
	BOOL isForSculptOnly() const;

	//raw image management	
	void        checkCachedRawSculptImage() ;
	LLImageRaw* getRawImage()const { return mRawImage ;}
	S32         getRawImageLevel() const {return mRawDiscardLevel;}
	LLImageRaw* getCachedRawImage() const { return mCachedRawImage ;}
	S32         getCachedRawImageLevel() const {return mCachedRawDiscardLevel;}
	BOOL        isCachedRawImageReady() const {return mCachedRawImageReady ;}
	BOOL        isRawImageValid()const { return mIsRawImageValid ; }	
	void        forceToSaveRawImage(S32 desired_discard = 0, F32 kept_time = 0.f) ;
	void        forceToRefetchTexture(S32 desired_discard = 0, F32 kept_time = 60.f);
	/*virtual*/ void setCachedRawImage(S32 discard_level, LLImageRaw* imageraw) ;
	void        destroySavedRawImage() ;
	LLImageRaw* getSavedRawImage() ;
	BOOL        hasSavedRawImage() const ;
	F32         getElapsedLastReferencedSavedRawImageTime() const ;
	BOOL		isFullyLoaded() const;

	BOOL        hasFetcher() const { return mHasFetcher;}
	bool        isFetching() const { return mIsFetching;}
	void        setCanUseHTTP(bool can_use_http) {mCanUseHTTP = can_use_http;}

	void        forceToDeleteRequest();
	void        loadFromFastCache();
	void        setInFastCacheList(bool in_list) { mInFastCacheList = in_list; }
	bool        isInFastCacheList() { return mInFastCacheList; }

	/*virtual*/bool  isActiveFetching(); //is actively in fetching by the fetching pipeline.

protected:
	/*virtual*/ void switchToCachedImage();
	S32 getCurrentDiscardLevelForFetching() ;

private:
	void init(bool firstinit) ;	
	void cleanup() ;

	void saveRawImage() ;
	void setCachedRawImage() ;

	//for atlas
	void resetFaceAtlas() ;
	void invalidateAtlas(BOOL rebuild_geom) ;
	BOOL insertToAtlas() ;

private:
	BOOL  mFullyLoaded;
	BOOL  mInDebug;
	BOOL  mUnremovable;
	BOOL  mInFastCacheList;
	BOOL  mForceCallbackFetch;

protected:		
	std::string mLocalFileName;

	S32 mOrigWidth;
	S32 mOrigHeight;

	// Override the computation of discard levels if we know the exact output size of the image.
	// Used for UI textures to not decode, even if we have more data.
	S32 mKnownDrawWidth;
	S32	mKnownDrawHeight;
	BOOL mKnownDrawSizeChanged ;
	std::string mUrl;
	
	S32 mRequestedDiscardLevel;
	F32 mRequestedDownloadPriority;
	S32 mFetchState;
	U32 mFetchPriority;
	F32 mDownloadProgress;
	F32 mFetchDeltaTime;
	F32 mRequestDeltaTime;
	F32 mDecodePriority;			// The priority for decoding this image.
	S32	mMinDiscardLevel;
	S8  mDesiredDiscardLevel;			// The discard level we'd LIKE to have - if we have it and there's space	
	S8  mMinDesiredDiscardLevel;	// The minimum discard level we'd like to have

	S8  mNeedsAux;					// We need to decode the auxiliary channels
	S8  mHasAux;                    // We have aux channels
	S8  mDecodingAux;				// Are we decoding high components
	S8  mIsRawImageValid;
	S8  mHasFetcher;				// We've made a fecth request
	S8  mIsFetching;				// Fetch request is active
	bool mCanUseHTTP;              //This texture can be fetched through http if true.
	LLCore::HttpStatus mLastHttpGetStatus; // Result of the most recently completed http request for this texture.

	FTType mFTType; // What category of image is this - map tile, server bake, etc?
	mutable S8 mIsMissingAsset;		// True if we know that there is no image asset with this image id in the database.		

	typedef std::list<LLLoadedCallbackEntry*> callback_list_t;
	S8              mLoadedCallbackDesiredDiscardLevel;
	BOOL            mPauseLoadedCallBacks;
	callback_list_t mLoadedCallbackList;
	F32             mLastCallBackActiveTime;

	LLPointer<LLImageRaw> mRawImage;
	S32 mRawDiscardLevel;

	// Used ONLY for cloth meshes right now.  Make SURE you know what you're 
	// doing if you use it for anything else! - djs
	LLPointer<LLImageRaw> mAuxRawImage;

	//keep a copy of mRawImage for some special purposes
	//when mForceToSaveRawImage is set.
	BOOL mForceToSaveRawImage ;
	BOOL mSaveRawImage;
	LLPointer<LLImageRaw> mSavedRawImage;
	S32 mSavedRawDiscardLevel;
	S32 mDesiredSavedRawDiscardLevel;
	F32 mLastReferencedSavedRawImageTime ;
	F32 mKeptSavedRawImageTime ;

	//a small version of the copy of the raw image (<= 64 * 64)
	LLPointer<LLImageRaw> mCachedRawImage;
	S32 mCachedRawDiscardLevel;
	BOOL mCachedRawImageReady; //the rez of the mCachedRawImage reaches the upper limit.	

	LLHost mTargetHost;	// if invalid, just request from agent's simulator

	// Timers
	LLFrameTimer mLastPacketTimer;		// Time since last packet.
	LLFrameTimer mStopFetchingTimer;	// Time since mDecodePriority == 0.f.

	BOOL  mInImageList;				// TRUE if image is in list (in which case don't reset priority!)
	// This needs to be atomic, since it is written both in the main thread
	// and in the GL image worker thread... HB
	LLAtomicBool  mNeedsCreateTexture;	

	BOOL   mForSculpt ; //a flag if the texture is used as sculpt data.
	BOOL   mIsFetched ; //is loaded from remote or from cache, not generated locally.

public:
	static LLPointer<LLViewerFetchedTexture> sMissingAssetImagep;	// Texture to show for an image asset that is not in the database
	static LLPointer<LLViewerFetchedTexture> sWhiteImagep;	// Texture to show NOTHING (whiteness)
	static LLPointer<LLViewerFetchedTexture> sDefaultImagep; // "Default" texture for error cases, the only case of fetched texture which is generated in local.
	static LLPointer<LLViewerFetchedTexture> sSmokeImagep; // Old "Default" translucent texture
	static LLPointer<LLViewerFetchedTexture> sFlatNormalImagep; // Flat normal map denoting no bumpiness on a surface
};

//
//the image data is fetched from remote or from local cache
//the resolution of the texture is adjustable: depends on the view-dependent parameters.
//
class LLViewerLODTexture : public LLViewerFetchedTexture
{
protected:
	/*virtual*/ ~LLViewerLODTexture(){}

public:
	LLViewerLODTexture(const LLUUID& id, FTType f_type, const LLHost& host = LLHost(), BOOL usemipmaps = TRUE);
	LLViewerLODTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps = TRUE);

	/*virtual*/ S8 getType() const;
	// Process image stats to determine priority/quality requirements.
	/*virtual*/ void processTextureStats();
	bool isUpdateFrozen() ;

private:
	void init(bool firstinit) ;
	bool scaleDown() ;		

private:
	F32 mDiscardVirtualSize;		// Virtual size used to calculate desired discard	
	F32 mCalculatedDiscardLevel;    // Last calculated discard level
};

//
//the image data is fetched from the media pipeline periodically
//the resolution of the texture is also adjusted by the media pipeline
//
class LLViewerMediaTexture : public LLViewerTexture
{
protected:
	/*virtual*/ ~LLViewerMediaTexture() ;

public:
	LLViewerMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;

	/*virtual*/ S8 getType() const;
	void reinit(BOOL usemipmaps = TRUE);	

	BOOL  getUseMipMaps() {return mUseMipMaps ; }
	void  setUseMipMaps(BOOL mipmap) ;	
	
	void setPlaying(BOOL playing) ;
	BOOL isPlaying() const {return mIsPlaying;}
	void setMediaImpl() ;

    virtual bool isViewerMediaTexture() const { return true; }

	void initVirtualSize() ;	
	void invalidateMediaImpl() ;

	void addMediaToFace(LLFace* facep) ;
	void removeMediaFromFace(LLFace* facep) ;

	/*virtual*/ void addFace(U32 ch, LLFace* facep) ;
	/*virtual*/ void removeFace(U32 ch, LLFace* facep) ; 

	/*virtual*/ F32  getMaxVirtualSize() ;
private:
	void switchTexture(U32 ch, LLFace* facep) ;
	BOOL findFaces() ;
	void stopPlaying() ;

private:
	//
	//an instant list, recording all faces referencing or can reference to this media texture.
	//NOTE: it is NOT thread safe. 
	//
	std::list< LLFace* > mMediaFaceList ; 

	//an instant list keeping all textures which are replaced by the current media texture,
	//is only used to avoid the removal of those textures from memory.
	std::list< LLPointer<LLViewerTexture> > mTextureList ;

	LLViewerMediaImpl* mMediaImplp ;	
	BOOL mIsPlaying ;
	U32  mUpdateVirtualSizeTime ;

public:
	static void updateClass() ;
	static void cleanUpClass() ;	

	static LLViewerMediaTexture* findMediaTexture(const LLUUID& media_id) ;
	static void removeMediaImplFromTexture(const LLUUID& media_id) ;

private:
	typedef std::map< LLUUID, LLPointer<LLViewerMediaTexture> > media_map_t ;
	static media_map_t sMediaMap ;	
};

//just an interface class, do not create instance from this class.
class LLViewerTextureManager
{
private:
	//make the constructor private to preclude creating instances from this class.
	LLViewerTextureManager(){}

public:
    //texture pipeline tester
	static LLTexturePipelineTester* sTesterp ;

	//returns NULL if tex is not a LLViewerFetchedTexture nor derived from LLViewerFetchedTexture.
	static LLViewerFetchedTexture*    staticCastToFetchedTexture(LLTexture* tex, BOOL report_error = FALSE) ;

	//
	//"find-texture" just check if the texture exists, if yes, return it, otherwise return null.
	//
	static void                       findFetchedTextures(const LLUUID& id, std::vector<LLViewerFetchedTexture*> &output);
	static void                       findTextures(const LLUUID& id, std::vector<LLViewerTexture*> &output);
	static LLViewerFetchedTexture*    findFetchedTexture(const LLUUID& id, S32 tex_type);
	static LLViewerMediaTexture*      findMediaTexture(const LLUUID& id) ;
	
	static LLViewerMediaTexture*      createMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;

	//
	//"get-texture" will create a new texture if the texture does not exist.
	//
	static LLViewerMediaTexture*      getMediaTexture(const LLUUID& id, BOOL usemipmaps = TRUE, LLImageGL* gl_image = NULL) ;
	
	static LLPointer<LLViewerTexture> getLocalTexture(BOOL usemipmaps = TRUE, BOOL generate_gl_tex = TRUE);
	static LLPointer<LLViewerTexture> getLocalTexture(const LLUUID& id, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) ;
	static LLPointer<LLViewerTexture> getLocalTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	static LLPointer<LLViewerTexture> getLocalTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps, BOOL generate_gl_tex = TRUE) ;

	static LLViewerFetchedTexture* getFetchedTexture(const LLUUID &image_id,									 
									 FTType f_type = FTT_DEFAULT,
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,		// Get the requested level immediately upon creation.
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 LLHost request_from_host = LLHost()
									 );
	
	static LLViewerFetchedTexture* getFetchedTextureFromFile(const std::string& filename,									 
									 FTType f_type = FTT_LOCAL_FILE,
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 const LLUUID& force_id = LLUUID::null
									 );

	static LLViewerFetchedTexture* getFetchedTextureFromUrl(const std::string& url,									 
									 FTType f_type,
									 BOOL usemipmap = TRUE,
									 LLViewerTexture::EBoostLevel boost_priority = LLGLTexture::BOOST_NONE,
									 S8 texture_type = LLViewerTexture::FETCHED_TEXTURE,
									 LLGLint internal_format = 0,
									 LLGLenum primary_format = 0,
									 const LLUUID& force_id = LLUUID::null
									 );

	static LLViewerFetchedTexture* getFetchedTextureFromHost(const LLUUID& image_id, FTType f_type, LLHost host) ;

	static void init() ;
	static void cleanup() ;
};
//
//this class is used for test/debug only
//it tracks the activities of the texture pipeline
//records them, and outputs them to log files
//
class LLTexturePipelineTester : public LLMetricPerformanceTesterWithSession
{
	enum
	{
		MIN_LARGE_IMAGE_AREA = 262144  //512 * 512
	};
public:
	LLTexturePipelineTester() ;
	~LLTexturePipelineTester() ;

	void update();		
	void updateTextureBindingStats(const LLViewerTexture* imagep) ;
	void updateTextureLoadingStats(const LLViewerFetchedTexture* imagep, const LLImageRaw* raw_imagep, BOOL from_cache) ;
	void updateGrayTextureBinding() ;
	void setStablizingTime() ;

private:
	void reset() ;
	void updateStablizingTime() ;

	/*virtual*/ void outputTestRecord(LLSD* sd) ;

private:
	BOOL mPause ;
private:
	BOOL mUsingDefaultTexture;            //if set, some textures are still gray.

	U32Bytes mTotalBytesUsed ;                     //total bytes of textures bound/used for the current frame.
	U32Bytes mTotalBytesUsedForLargeImage ;        //total bytes of textures bound/used for the current frame for images larger than 256 * 256.
	U32Bytes mLastTotalBytesUsed ;                 //total bytes of textures bound/used for the previous frame.
	U32Bytes mLastTotalBytesUsedForLargeImage ;    //total bytes of textures bound/used for the previous frame for images larger than 256 * 256.
		
	//
	//data size
	//
	U32Bytes mTotalBytesLoaded ;               //total bytes fetched by texture pipeline
	U32Bytes mTotalBytesLoadedFromCache ;      //total bytes fetched by texture pipeline from local cache	
	U32Bytes mTotalBytesLoadedForLargeImage ;  //total bytes fetched by texture pipeline for images larger than 256 * 256. 
	U32Bytes mTotalBytesLoadedForSculpties ;   //total bytes fetched by texture pipeline for sculpties

	//
	//time
	//NOTE: the error tolerances of the following timers is one frame time.
	//
	F32 mStartFetchingTime ;
	F32 mTotalGrayTime ;                  //total loading time when no gray textures.
	F32 mTotalStablizingTime ;            //total stablizing time when texture memory overflows
	F32 mStartTimeLoadingSculpties ;      //the start moment of loading sculpty images.
	F32 mEndTimeLoadingSculpties ;        //the end moment of loading sculpty images.
	F32 mStartStablizingTime ;
	F32 mEndStablizingTime ;

private:
	//
	//The following members are used for performance analyzing
	//
	class LLTextureTestSession : public LLTestSession
	{
	public:
		LLTextureTestSession() ;
		/*virtual*/ ~LLTextureTestSession() ;

		void reset() ;

		F32 mTotalFetchingTime ;
		F32 mTotalGrayTime ;
		F32 mTotalStablizingTime ;
		F32 mStartTimeLoadingSculpties ; 
		F32 mTotalTimeLoadingSculpties ;

		S32 mTotalBytesLoaded ; 
		S32 mTotalBytesLoadedFromCache ;
		S32 mTotalBytesLoadedForLargeImage ;
		S32 mTotalBytesLoadedForSculpties ; 

		typedef struct _texture_instant_preformance_t
		{
			S32 mAverageBytesUsedPerSecond ;         
			S32 mAverageBytesUsedForLargeImagePerSecond ;
			F32 mAveragePercentageBytesUsedPerSecond ;
			F32 mTime ;
		}texture_instant_preformance_t ;
		std::vector<texture_instant_preformance_t> mInstantPerformanceList ;
		S32 mInstantPerformanceListCounter ;
	};

	/*virtual*/ LLMetricPerformanceTesterWithSession::LLTestSession* loadTestSession(LLSD* log) ;
	/*virtual*/ void compareTestSessions(llofstream* os) ;
};

#endif
