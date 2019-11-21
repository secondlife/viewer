/** 
 * @file llviewertexturemanager.h
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

#ifndef LL_LLVIEWERTEXTUREMANAGER_H
#define LL_LLVIEWERTEXTUREMANAGER_H

#include "llviewertexture.h"
#include "llfttype.h"
#include <boost/optional.hpp>

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
class LLImageJ2C;

//just an interface class, do not create instance from this class.
class LLViewerTextureManager : public LLSingleton<LLViewerTextureManager>
{
    LLSINGLETON_EMPTY_CTOR(LLViewerTextureManager);
    LOG_CLASS(LLViewerTextureManager);

    friend LLViewerFetchedTexture;
public:
    enum ETexListType
    {
        TEX_LIST_STANDARD = 0,
        TEX_LIST_SCALE
    };

    struct FetchParams
    {
        FetchParams()
        {}

        void reset()
        {
            mCallback = boost::none;
            mKeepRaw = boost::none;
            mNeesAux = boost::none;
            mUseMipMaps = boost::none;
            mForceToSaveRaw = boost::none;
            mDesiredDiscard = boost::none;
            mSaveKeepTime = boost::none;
            mFTType = boost::none;
            mBoostPriority = boost::none;
            mTextureType = boost::none;
            mInternalFormat = boost::none;
            mPrimaryFormat = boost::none;
            mForceUUID = boost::none;
        }

        /*TODO:VS17*/ // switch to std::optional
        boost::optional<LLViewerFetchedTexture::loaded_cb_fn>   mCallback;
        boost::optional<bool>       mKeepRaw;
        boost::optional<bool>       mNeesAux;
        boost::optional<bool>       mUseMipMaps;
        boost::optional<bool>       mForceToSaveRaw;
        boost::optional<S32>        mDesiredDiscard;
        boost::optional<F32>        mSaveKeepTime;
        boost::optional<FTType>     mFTType;
        boost::optional<LLViewerTexture::EBoostLevel>   mBoostPriority;
        boost::optional<S8>         mTextureType;
        boost::optional<LLGLint>    mInternalFormat;
        boost::optional<LLGLenum>   mPrimaryFormat;
        boost::optional<LLUUID>     mForceUUID;
    };

    typedef std::deque<LLViewerFetchedTexture::ptr_t> deque_texture_t;

    //-------------------------------------------
    void                            update();

    //-------------------------------------------
    size_t                          getTextureCount() const { return mTextureList.size(); }

    //texture pipeline tester
	static LLTexturePipelineTester*     sTesterp ;

	//returns NULL if tex is not a LLViewerFetchedTexture nor derived from LLViewerFetchedTexture.
	static LLViewerFetchedTexture::ptr_t staticCastToFetchedTexture(const LLTexture::ptr_t &tex, bool report_error = false) ;

	//
	//"find-texture" just check if the texture exists, if yes, return it, otherwise return null.
    void                            findTextures(const LLUUID& id, deque_texture_t &output) const;
    LLViewerFetchedTexture::ptr_t   findFetchedTexture(const LLUUID& id, S32 tex_type = TEX_LIST_STANDARD) const;
    LLViewerMediaTexture::ptr_t findMediaTexture(const LLUUID& id, bool setimpl=true) const;
	
	//"get-texture" will create a new texture if the texture does not exist.
    LLViewerTexture::ptr_t          getLocalTexture(bool usemipmaps = true, bool generate_gl_tex = true) const;
    LLViewerTexture::ptr_t          getLocalTexture(const LLUUID& id, bool usemipmaps, bool generate_gl_tex = true) const;
    LLViewerTexture::ptr_t          getLocalTexture(const LLImageRaw* raw, bool usemipmaps) const;
    LLViewerTexture::ptr_t          getLocalTexture(U32 width, U32 height, U8 components, bool usemipmaps, bool generate_gl_tex = true) const;

    LLViewerFetchedTexture::ptr_t   getFetchedTexture(const LLUUID &image_id, const FetchParams &params = FetchParams());
    LLViewerFetchedTexture::ptr_t   getFetchedTextureFromFile(const std::string& filename, const FetchParams &params = FetchParams());
    LLViewerFetchedTexture::ptr_t   getFetchedTextureFromSkin(const std::string& filename, const FetchParams &params = FetchParams());
    LLViewerFetchedTexture::ptr_t   getFetchedTextureFromUrl(const std::string& url, const FetchParams &params = FetchParams());

    LLViewerMediaTexture::ptr_t     getMediaTexture(const LLUUID& id, bool usemipmaps = true, LLImageGL* gl_image = nullptr);

    void                            removeTexture(const LLViewerFetchedTexture::ptr_t &texture);
    void                            removeMediaImplFromTexture(const LLUUID& media_id);
    void                            explicitAddTexture(const LLViewerFetchedTexture::ptr_t &texture, ETexListType type);

    void                            cancelAllFetches();
    void                            cancelRequest(const LLUUID &id);

    void                            setTextureDirty(const LLViewerFetchedTexture::ptr_t &texture);
    //-------------------------------------------
    // memory management
    S32Megabytes                    getMinVideoRamSetting() const;
    S32Megabytes                    getMaxVideoRamSetting(bool get_recommended, float mem_multiplier) const;
    void                            updateMaxResidentTexMem(S32Megabytes mem);
    S32Megabytes	                getMaxResidentTexMem() const	{ return mMaxResidentTexMemInMegaBytes; }
    S32Megabytes                    getMaxTotalTextureMem() const   { return mMaxTotalTextureMemInMegaBytes; }

    void                            destroyGL(bool save_state = TRUE);
    void                            restoreGL();

    //-------------------------------------------
    // file conversions
    static bool                     createUploadFile(const std::string& filename, const std::string& out_filename, const U8 codec);
    static LLPointer<LLImageJ2C>    convertToUploadFile(LLPointer<LLImageRaw> &raw_image);

    //-------------------------------------------
    // Statistics
    void                            resetStatistics();

protected:
    virtual void                    initSingleton() override;
    virtual void                    cleanupSingleton() override;

    void                            updatedSavedRaw(const LLViewerFetchedTexture::ptr_t &texture);
private:
    static const F32                MAX_INACTIVE_TIME;

    static U32                      boostLevelToPriority(LLViewerTexture::EBoostLevel boost);

    struct TextureKey
    {
        TextureKey(const LLUUID &id, ETexListType tex_type) :
            mTextureId(id),
            mTextureType(tex_type)
        { }

        TextureKey(const TextureKey &other) :
            mTextureId(other.mTextureId),
            mTextureType(other.mTextureType)
        { }

        friend bool operator<(const TextureKey& key1, const TextureKey& key2)
        {
            if (key1.mTextureId != key2.mTextureId)
            {
                return key1.mTextureId < key2.mTextureId;
            }
            else
            {
                return key1.mTextureType < key2.mTextureType;
            }
        }

        LLUUID          mTextureId;
        ETexListType    mTextureType;
    };

    typedef std::map<TextureKey, LLViewerFetchedTexture::ptr_t> map_key_texture_t;
    typedef std::list<LLViewerFetchedTexture::ptr_t>            list_texture_t;
    typedef std::set<LLViewerFetchedTexture::ptr_t>             set_texture_t;
    typedef std::map<LLUUID, LLViewerMediaTexture::ptr_t>       media_map_t;
    typedef std::deque<LLViewerTexture::ptr_t>                  deque_deadlist_t;

    bool                            mIsCleaningUp;

    map_key_texture_t               mTextureList;           // master list of all textures.
    uuid_set_t                      mOutstandingRequests;   // list of requests being fetched.
    deque_deadlist_t                mDeadlist;    // candidates for deletion.
    bool                            mDeadlistDirty;

    // Note: just raw pointers because they are never referenced, just compared against
    std::set<LLViewerFetchedTexture::ptr_t> mDirtyTextureList;

    S32Megabytes                    mMaxResidentTexMemInMegaBytes;
    S32Megabytes                    mMaxTotalTextureMemInMegaBytes;
    
    set_texture_t                   mImagePreloads; // simply holds on to LLViewerFetchedTexture references to stop them from being purged too soon
    list_texture_t                  mImageSaves;    // list of saved textures. 
    media_map_t                     mMediaMap;

    LLTextureInfo::ptr_t            mTextureDownloadInfo;   // stats for HTTP only

    void                            doPreloadImages();

    LLViewerMediaTexture::ptr_t     createMediaTexture(const LLUUID& id, bool usemipmaps = true, LLImageGL* gl_image = nullptr);
    LLViewerFetchedTexture::ptr_t   createFetchedTexture(const LLUUID &image_id, FTType f_type, BOOL usemipmaps,
                                        LLViewerFetchedTexture::EBoostLevel usage, S8 texture_type, 
                                        LLGLint internal_format = 0, LLGLenum primary_format = 0) const;

    F32                             updateCleanSavedRaw(F32 timeout);
    F32                             updateCleanDead(F32 timeout);
    F32                             updateAddToDeadlist(F32 timeout);

    void                            addTexture(LLViewerFetchedTexture::ptr_t &texturep, ETexListType list_type);

    void                            onTextureFetchDone(const LLAssetFetch::AssetRequest::ptr_t &request, const LLAssetFetch::TextureInfo &info, ETexListType text_type);

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
