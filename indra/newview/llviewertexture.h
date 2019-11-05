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

#include "llgltexture.h"
#include "lltimer.h"
#include "llframetimer.h"
#include "llgltypes.h"
#include "llrender.h"
#include "llmetricperformancetester.h"
#include "httpcommon.h"

#include <map>
#include <list>

#include "llfttype.h"
#include "llassetfetch.h"

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

class LLVFile;
class LLMessageSystem;
class LLViewerMediaImpl ;
class LLVOVolume ;

// class LLLoadedCallbackEntry
// {
// public:
//     typedef std::set< LLTextureKey > source_callback_list_t;
// 
// public:
// 	LLLoadedCallbackEntry(loaded_callback_func cb,
// 						  S32 discard_level,
// 						  BOOL need_imageraw, // Needs image raw for the callback
// 						  void* userdata,
// 						  source_callback_list_t* src_callback_list,
// 						  LLViewerFetchedTexture* target,
// 						  BOOL pause);
// 	~LLLoadedCallbackEntry();
// 	void removeTexture(LLViewerFetchedTexture* tex) ;
// 
// 	loaded_callback_func	mCallback;
// 	S32						mLastUsedDiscard;
// 	S32						mDesiredDiscard;
// 	BOOL					mNeedsImageRaw;
// 	BOOL                    mPaused;
// 	void*					mUserData;
// 	source_callback_list_t* mSourceCallbackList;
// 	
// public:
// //	static void cleanUpCallbackList(LLLoadedCallbackEntry::source_callback_list_t* callback_list) ;
// };

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
	static void updateClass(const F32 velocity, const F32 angular_velocity) ;
	
	LLViewerTexture(BOOL usemipmaps = TRUE);
	LLViewerTexture(const LLUUID& id, BOOL usemipmaps) ;
	LLViewerTexture(const LLImageRaw* raw, BOOL usemipmaps) ;
	LLViewerTexture(const U32 width, const U32 height, const U8 components, BOOL usemipmaps) ;

	virtual S8 getType() const;
	virtual bool            isMissingAsset() const;
	virtual void dump();	// debug info to LL_INFOS()
	
	/*virtual*/ bool bindDefaultImage(const S32 stage = 0) ;
	/*virtual*/ bool bindDebugImage(const S32 stage = 0) ;
	/*virtual*/ void forceImmediateUpdate() ;
	virtual bool isActiveFetching();
	
	/*virtual*/ const LLUUID& getID() const { return mID; }
	void setBoostLevel(S32 level);
	S32  getBoostLevel() const { return mBoostLevel; }
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
	static S32 sMinLargeImageSize ;
	static S32 sMaxSmallImageSize ;
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


const std::string& fttype_to_string(const FTType& fttype);

//
//textures are managed in LLViewerTextureList::instance().
//raw image data is fetched from remote or local cache
//but the raw image this texture pointing to is fixed.
//
class LLViewerFetchedTexture : public LLViewerTexture
{
    friend class LLViewerTextureManager;
	friend class LLTextureBar; // debug info only
	friend class LLTextureView; // debug info only
public:
    typedef std::function<void(bool success, LLPointer<LLViewerFetchedTexture> &src_vi, bool final_done)>  loaded_cb_fn;
    typedef boost::signals2::connection     connection_t;
    typedef std::set<connection_t>          connection_list_t;  // convenience for collection multiple connections.

    struct Compare
    {
        // lhs < rhs
        bool operator()(const LLPointer<LLViewerFetchedTexture> &lhs, const LLPointer<LLViewerFetchedTexture> &rhs) const
        {
            const LLViewerFetchedTexture* lhsp = (const LLViewerFetchedTexture*)lhs;
            const LLViewerFetchedTexture* rhsp = (const LLViewerFetchedTexture*)rhs;
            // greater priority is "less"
            const F32 lpriority = lhsp->getPriority();
            const F32 rpriority = rhsp->getPriority();
            if (lpriority > rpriority) // higher priority
                return true;
            if (lpriority < rpriority)
                return false;
            return lhsp < rhsp;
        }
    };

	LLViewerFetchedTexture(const LLUUID& id, FTType f_type, BOOL usemipmaps = TRUE);
	LLViewerFetchedTexture(const LLImageRaw* raw, FTType f_type, BOOL usemipmaps);
	LLViewerFetchedTexture(const std::string& url, FTType f_type, const LLUUID& id, BOOL usemipmaps = TRUE);

    virtual S8                  getType() const override        { return LLViewerTexture::FETCHED_TEXTURE; }
    FTType                      getFTType() const               { return mFTType; }

    LLPointer<LLImageRaw>       getRawImage() const             { return mRawImage; }
    LLPointer<LLImageRaw>       getAuxImage() const             { return mAuxRawImage; }


    void                        setUnremovable(bool value)      { mUnremovable = value; }
    bool                        isUnremovable() const           { return mUnremovable; }
    void                        setIsMissingAsset(bool is_missing);
    virtual bool                isMissingAsset() const override { return mIsMissingAsset; }

    bool                        needsAux() const                { return mNeedsAux; }
    bool                        hasAux() const                  { return mAuxRawImage.notNull(); }

    LLAssetFetch::FetchState    getFetchState() const;
    bool                        isFetching() const;
    virtual bool                isActiveFetching() override;    // is actively fetching or processing. (not waiting in queue)

    U32                         getPriority() const; 
    void                        setPriority(U32 priority);
    void                        adjustPriority(S32 adjustment);

    virtual void                forceImmediateUpdate() override;

    static U32                  maxPriority();

    bool                        hasCallbacks()                  { return !mAssetDoneSignal.empty(); }
    connection_t                addCallback(loaded_cb_fn cb);

protected:
    virtual                     ~LLViewerFetchedTexture();

private:
    typedef boost::signals2::signal<void(bool success, LLPointer<LLViewerFetchedTexture> &src_vi, bool final_done)> texture_done_signal_t;

    FTType                      mFTType;            // What category of image is this - map tile, server bake, etc?
    bool                        mIsMissingAsset;    // True if we know that there is no image asset with this image id in the database.		

    bool                        mUnremovable;
    bool                        mNeedsAux;          // We need to decode the auxiliary channels
    bool                        mIsRemoteFetched;   // is loaded from remote or from cache, not generated locally.
    bool                        mIsFinal;           // texture has completed fetching (for better or worse)
    bool                        mSuccess;           // texture fetched without error.

    LLPointer<LLImageRaw>       mRawImage;
    LLPointer<LLImageRaw>       mAuxRawImage;       // Used ONLY for cloth meshes right now.  Make SURE you know what you're doing if you use it for anything else! - djs
    S32                         mRawDiscardLevel;

    texture_done_signal_t       mAssetDoneSignal;

    void                        init(bool firstinit);
    void                        cleanup();

    void                        setIsFinal(bool is_final) { mIsFinal = is_final; }
    void                        setIsSuccess(bool is_success) { mSuccess = is_success; }
public:
	virtual void dump() override;

	// Set callbacks to get called when the image gets updated with higher 
	// resolution versions.
// 	void setLoadedCallback(loaded_callback_func cb,
// 						   S32 discard_level, BOOL keep_imageraw, BOOL needs_aux,
// 						   void* userdata, LLLoadedCallbackEntry::source_callback_list_t* src_callback_list, BOOL pause = FALSE);
// 	void pauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
// 	void unpauseLoadedCallbacks(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
// 	bool doLoadedCallbacks();
// 	void deleteCallbackEntry(const LLLoadedCallbackEntry::source_callback_list_t* callback_list);
// 	void clearCallbackEntryList() ;

	void addToCreateTexture();


	 // ONLY call from LLViewerTextureList
	BOOL createTexture(S32 usename = 0);
	void destroyTexture() ;

	virtual void processTextureStats() ;

	F32  calcDecodePriority() ;

	// Set the decode priority for this image...
	// DON'T CALL THIS UNLESS YOU KNOW WHAT YOU'RE DOING, it can mess up
	// the priority list, and cause horrible things to happen.
	F32 getAdditionalDecodePriority() const { return mAdditionalDecodePriority; };

	void setAdditionalDecodePriority(F32 priority) ;
	
	void updateVirtualSize() ;

    void setDesiredDiscardLevel(S32 discard);
	S32  getDesiredDiscardLevel()			 { return mDesiredDiscardLevel; }
	void setMinDiscardLevel(S32 discard) 	{ mMinDesiredDiscardLevel = llmin(mMinDesiredDiscardLevel,(S8)discard); }

	bool updateFetch();
	bool setDebugFetching(S32 debug_level);
	bool isInDebug() const { return mInDebug; }

	
	void clearFetchedResults(); //clear all fetched results, for debug use.

	// Override the computation of discard levels if we know the exact output
	// size of the image.  Used for UI textures to not decode, even if we have
	// more data.
	/*virtual*/ void setKnownDrawSize(S32 width, S32 height);


	// returns dimensions of original image for local files (before power of two scaling)
	// and returns 0 for all asset system images
	S32 getOriginalWidth() const { return mOrigWidth; }
	S32 getOriginalHeight() const { return mOrigHeight; }

	BOOL isInImageList() const {return mInImageList ;}
	void setInImageList(BOOL flag) {mInImageList = flag ;}

	F32 getDownloadProgress() const {return mDownloadProgress ;}

	LLImageRaw* reloadRawImage(S8 discard_level) ;
	void destroyRawImage();
	bool needsToSaveRawImage();

	const std::string& getUrl() const {return mUrl;}
    void setUrl(std::string url) { mUrl = url; }
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
	S32         getRawImageLevel() const {return mRawDiscardLevel;}
	LLImageRaw* getCachedRawImage() const { return mCachedRawImage ;}
	S32         getCachedRawImageLevel() const {return mCachedRawDiscardLevel;}
	bool        isCachedRawImageReady() const {return !mCachedRawImage.isNull() ;}
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

	void        forceToDeleteRequest();


protected:
	/*virtual*/ void switchToCachedImage();
	S32 getCurrentDiscardLevelForFetching() ;

private:

	void saveRawImage() ;
	void setCachedRawImage() ;

	//for atlas
	void resetFaceAtlas() ;
	void invalidateAtlas(BOOL rebuild_geom) ;
	BOOL insertToAtlas() ;

private:
	BOOL  mFullyLoaded;
	BOOL  mInDebug;
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
	F32 mDownloadProgress;
	F32 mFetchDeltaTime;
	F32 mRequestDeltaTime;
	S32	mMinDiscardLevel;
	S8  mDesiredDiscardLevel;		// The discard level we'd LIKE to have - if we have it and there's space	
	S8  mMinDesiredDiscardLevel;	// The minimum discard level we'd like to have

	S8  mDecodingAux;				// Are we decoding high components
	S8  mIsRawImageValid;
	S8  mHasFetcher;				// We've made a fetch request


//	typedef std::list<LLLoadedCallbackEntry*> callback_list_t;
// 	S8              mLoadedCallbackDesiredDiscardLevel;
// 	BOOL            mPauseLoadedCallBacks;
// 	callback_list_t mLoadedCallbackList;
// 	F32             mLastCallBackActiveTime;



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

	// Timers
	LLFrameTimer mStopFetchingTimer;	// Time since mDecodePriority == 0.f.

	BOOL  mInImageList;				// TRUE if image is in list (in which case don't reset priority!)
	bool    mNeedsCreateTexture;	

	BOOL   mForSculpt ; //a flag if the texture is used as sculpt data.

    void    onTextureFetchComplete(const LLAssetFetch::AssetRequest::ptr_t &, const LLAssetFetch::TextureInfo &);
    void    handleTextureLoadSuccess(const LLAssetFetch::AssetRequest::ptr_t &, const LLAssetFetch::TextureInfo &);
    void    handleTextureLoadError(const LLAssetFetch::AssetRequest::ptr_t &);
    void    handleTextureLoadCancel(const LLAssetFetch::AssetRequest::ptr_t &);

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
	LLViewerLODTexture(const LLUUID& id, FTType f_type, BOOL usemipmaps = TRUE);
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

#endif
