/** 
 * @file llvovolume.h
 * @brief LLVOVolume class header file
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

#ifndef LL_LLVOVOLUME_H
#define LL_LLVOVOLUME_H

#include "llviewerobject.h"
#include "llviewertexture.h"
#include "llviewermedia.h"
#include "llframetimer.h"
#include "lllocalbitmaps.h"
#include "m3math.h"		// LLMatrix3
#include "m4math.h"		// LLMatrix4
#include <map>
#include <set>


class LLViewerTextureAnim;
class LLDrawPool;
class LLMaterialID;
class LLSelectNode;
class LLObjectMediaDataClient;
class LLObjectMediaNavigateClient;
class LLVOAvatar;
class LLMeshSkinInfo;

typedef std::vector<viewer_media_t> media_list_t;

enum LLVolumeInterfaceType
{
	INTERFACE_FLEXIBLE = 1,
};

const F32 MAX_LOD_FACTOR = 4.0f;


class LLRiggedVolume : public LLVolume
{
public:
	LLRiggedVolume(const LLVolumeParams& params)
		: LLVolume(params, 0.f)
	{
	}

    using FaceIndex = S32;
    static const FaceIndex UPDATE_ALL_FACES = -1;
    static const FaceIndex DO_NOT_UPDATE_FACES = -2;
    void update(const LLMeshSkinInfo* skin, LLVOAvatar* avatar, const LLVolume* src_volume, FaceIndex face_index = UPDATE_ALL_FACES, bool rebuild_face_octrees = true);

    std::string mExtraDebugText;
};

// Base class for implementations of the volume - Primitive, Flexible Object, etc.
class LLVolumeInterface
{
public:
	virtual ~LLVolumeInterface() { }
	virtual LLVolumeInterfaceType getInterfaceType() const = 0;
	virtual void doIdleUpdate() = 0;
	virtual BOOL doUpdateGeometry(LLDrawable *drawable) = 0;
	virtual LLVector3 getPivotPosition() const = 0;
	virtual void onSetVolume(const LLVolumeParams &volume_params, const S32 detail) = 0;
	virtual void onSetScale(const LLVector3 &scale, BOOL damped) = 0;
	virtual void onParameterChanged(U16 param_type, LLNetworkData *data, BOOL in_use, bool local_origin) = 0;
	virtual void onShift(const LLVector4a &shift_vector) = 0;
	virtual bool isVolumeUnique() const = 0; // Do we need a unique LLVolume instance?
	virtual bool isVolumeGlobal() const = 0; // Are we in global space?
	virtual bool isActive() const = 0; // Is this object currently active?
	virtual const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const = 0;
	virtual void updateRelativeXform(bool force_identity = false) = 0;
	virtual U32 getID() const = 0;
	virtual void preRebuild() = 0;
};

// Class which embodies all Volume objects (with pcode LL_PCODE_VOLUME)
class LLVOVolume : public LLViewerObject
{
	LOG_CLASS(LLVOVolume);
protected:
	virtual				~LLVOVolume();

public:
	static		void	initClass();
	static		void	cleanupClass();
	static		void	preUpdateGeom();
	
	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD0) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD1) |
							(1 << LLVertexBuffer::TYPE_COLOR)
	};

public:
						LLVOVolume(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	/*virtual*/ void markDead();		// Override (and call through to parent) to clean up media references

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);

				void	deleteFaces();

				void	animateTextures();
	
	            BOOL    isVisible() const ;
	/*virtual*/ BOOL	isActive() const;
	/*virtual*/ BOOL	isAttachment() const;
	/*virtual*/ BOOL	isRootEdit() const; // overridden for sake of attachments treating themselves as a root object
	/*virtual*/ BOOL	isHUDAttachment() const;

				void	generateSilhouette(LLSelectNode* nodep, const LLVector3& view_point);
	/*virtual*/	BOOL	setParent(LLViewerObject* parent);
				S32		getLOD() const						{ return mLOD; }
				void	setNoLOD()							{ mLOD = NO_LOD; mLODChanged = TRUE; }
				bool	isNoLOD() const						{ return NO_LOD == mLOD; }
	const LLVector3		getPivotPositionAgent() const;
	const LLMatrix4&	getRelativeXform() const				{ return mRelativeXform; }
	const LLMatrix3&	getRelativeXformInvTrans() const		{ return mRelativeXformInvTrans; }
	/*virtual*/	const LLMatrix4	getRenderMatrix() const;
				typedef std::map<LLUUID, S32> texture_cost_t;
				U32 	getRenderCost(texture_cost_t &textures) const;
    /*virtual*/	F32		getEstTrianglesMax() const;
    /*virtual*/	F32		getEstTrianglesStreamingCost() const;
    /* virtual*/ F32	getStreamingCost() const;
    /*virtual*/ bool getCostData(LLMeshCostData& costs) const;

	/*virtual*/ U32		getTriangleCount(S32* vcount = NULL) const;
	/*virtual*/ U32		getHighLODTriangleCount();
	/*virtual*/ BOOL lineSegmentIntersect(const LLVector4a& start, const LLVector4a& end, 
										  S32 face = -1,                        // which face to check, -1 = ALL_SIDES
										  BOOL pick_transparent = FALSE,
										  BOOL pick_rigged = FALSE,
										  S32* face_hit = NULL,                 // which face was hit
										  LLVector4a* intersection = NULL,       // return the intersection point
										  LLVector2* tex_coord = NULL,          // return the texture coordinates of the intersection point
										  LLVector4a* normal = NULL,             // return the surface normal at the intersection point
										  LLVector4a* tangent = NULL           // return the surface tangent at the intersection point
		);
	
				LLVector3 agentPositionToVolume(const LLVector3& pos) const;
				LLVector3 agentDirectionToVolume(const LLVector3& dir) const;
				LLVector3 volumePositionToAgent(const LLVector3& dir) const;
				LLVector3 volumeDirectionToAgent(const LLVector3& dir) const;

				
				BOOL	getVolumeChanged() const				{ return mVolumeChanged; }
				
	/*virtual*/ F32  	getRadius() const						{ return mVObjRadius; };
				const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const;

				void	markForUpdate(BOOL priority);
				void	markForUnload()							{ LLViewerObject::markForUnload(TRUE); mVolumeChanged = TRUE; }
				void    faceMappingChanged()                    { mFaceMappingChanged=TRUE; };

	/*virtual*/ void	onShift(const LLVector4a &shift_vector); // Called when the drawable shifts

	/*virtual*/ void	parameterChanged(U16 param_type, bool local_origin);
	/*virtual*/ void	parameterChanged(U16 param_type, LLNetworkData* data, BOOL in_use, bool local_origin);

	/*virtual*/ U32		processUpdateMessage(LLMessageSystem *mesgsys,
											void **user_data,
											U32 block_num, const EObjectUpdateType update_type,
											LLDataPacker *dp);

	/*virtual*/ void	setSelected(BOOL sel);
	/*virtual*/ BOOL	setDrawableParent(LLDrawable* parentp);

	/*virtual*/ void	setScale(const LLVector3 &scale, BOOL damped);

	/*virtual*/ void    changeTEImage(S32 index, LLViewerTexture* new_image)  ;
	/*virtual*/ void	setNumTEs(const U8 num_tes);
	/*virtual*/ void	setTEImage(const U8 te, LLViewerTexture *imagep);
	/*virtual*/ S32		setTETexture(const U8 te, const LLUUID &uuid);
	/*virtual*/ S32		setTEColor(const U8 te, const LLColor3 &color);
	/*virtual*/ S32		setTEColor(const U8 te, const LLColor4 &color);
	/*virtual*/ S32		setTEBumpmap(const U8 te, const U8 bump);
	/*virtual*/ S32		setTEShiny(const U8 te, const U8 shiny);
	/*virtual*/ S32		setTEFullbright(const U8 te, const U8 fullbright);
	/*virtual*/ S32		setTEBumpShinyFullbright(const U8 te, const U8 bump);
	/*virtual*/ S32		setTEMediaFlags(const U8 te, const U8 media_flags);
	/*virtual*/ S32		setTEGlow(const U8 te, const F32 glow);
	/*virtual*/ S32		setTEMaterialID(const U8 te, const LLMaterialID& pMaterialID);
	
	static void	setTEMaterialParamsCallbackTE(const LLUUID& objectID, const LLMaterialID& pMaterialID, const LLMaterialPtr pMaterialParams, U32 te);

	/*virtual*/ S32		setTEMaterialParams(const U8 te, const LLMaterialPtr pMaterialParams);
	/*virtual*/ S32		setTEScale(const U8 te, const F32 s, const F32 t);
	/*virtual*/ S32		setTEScaleS(const U8 te, const F32 s);
	/*virtual*/ S32		setTEScaleT(const U8 te, const F32 t);
	/*virtual*/ S32		setTETexGen(const U8 te, const U8 texgen);
	/*virtual*/ S32		setTEMediaTexGen(const U8 te, const U8 media);
	/*virtual*/ BOOL 	setMaterial(const U8 material);

				void	setTexture(const S32 face);
				S32     getIndexInTex(U32 ch) const {return mIndexInTex[ch];}
	/*virtual*/ BOOL	setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);
				void	updateSculptTexture();
				void    setIndexInTex(U32 ch, S32 index) { mIndexInTex[ch] = index ;}
				void	sculpt();
	 static     void    rebuildMeshAssetCallback(const LLUUID& asset_uuid,
												 LLAssetType::EType type,
												 void* user_data, S32 status, LLExtStat ext_status);
					
				void	updateRelativeXform(bool force_identity = false);
	/*virtual*/ BOOL	updateGeometry(LLDrawable *drawable);
	/*virtual*/ void	updateFaceSize(S32 idx);
	/*virtual*/ BOOL	updateLOD();
				void	updateRadius();
	/*virtual*/ void	updateTextures();
				void	updateTextureVirtualSize(bool forced = false);

				void	updateFaceFlags();
				void	regenFaces();
                BOOL    genBBoxes(BOOL force_global, BOOL should_update_octree_bounds = TRUE);
				void	preRebuild();
	virtual		void	updateSpatialExtents(LLVector4a& min, LLVector4a& max);
	virtual		F32		getBinRadius();
	
	virtual U32 getPartitionType() const;

	// For Lights
	void setIsLight(BOOL is_light);
    //set the gamma-corrected (sRGB) color of this light
	void setLightSRGBColor(const LLColor3& color);
    //set the linear color of this light
    void setLightLinearColor(const LLColor3& color);

	void setLightIntensity(F32 intensity);
	void setLightRadius(F32 radius);
	void setLightFalloff(F32 falloff);
	void setLightCutoff(F32 cutoff);
	void setLightTextureID(LLUUID id);
	void setSpotLightParams(LLVector3 params);

	BOOL getIsLight() const;


    // Get the light color in sRGB color space NOT scaled by intensity.
	LLColor3 getLightSRGBBaseColor() const; 

    // Get the light color in linear color space NOT scaled by intensity.
    LLColor3 getLightLinearBaseColor() const;
    
    // Get the light color in linear color space scaled by intensity 
    //  this is the value that should be fed into shaders
    LLColor3 getLightLinearColor() const;

    // Get the light color in sRGB color space scaled by intensity.
    LLColor3 getLightSRGBColor() const; 

	LLUUID	getLightTextureID() const;
	bool isLightSpotlight() const;
	LLVector3 getSpotLightParams() const;
	void	updateSpotLightPriority();
	F32		getSpotLightPriority() const;

	LLViewerTexture* getLightTexture();
	F32 getLightIntensity() const;
	F32 getLightRadius() const;
	F32 getLightFalloff(const F32 fudge_factor = 1.f) const;
	F32 getLightCutoff() const;
	
	// Flexible Objects
	U32 getVolumeInterfaceID() const;
	virtual BOOL isFlexible() const;
	virtual BOOL isSculpted() const;
	virtual BOOL isMesh() const;
	virtual BOOL isRiggedMesh() const;
	virtual BOOL hasLightTexture() const;

    
	BOOL isVolumeGlobal() const;
	BOOL canBeFlexible() const;
	BOOL setIsFlexible(BOOL is_flexible);

    const LLMeshSkinInfo* getSkinInfo() const;

    //convenience accessor for mesh ID (which is stored in sculpt id for legacy reasons)
    const LLUUID& getMeshID() const { return getVolume()->getParams().getSculptID(); }
    
    // Extended Mesh Properties
    U32 getExtendedMeshFlags() const;
    void onSetExtendedMeshFlags(U32 flags);
    void setExtendedMeshFlags(U32 flags);
    bool canBeAnimatedObject() const;
    bool isAnimatedObject() const;
    virtual void onReparent(LLViewerObject *old_parent, LLViewerObject *new_parent);
    virtual void afterReparent();

    //virtual
    void updateRiggingInfo();
    S32 mLastRiggingInfoLOD;
    
    // Functions that deal with media, or media navigation
    
    // Update this object's media data with the given media data array
    // (typically this is only called upon a response from a server request)
	void updateObjectMediaData(const LLSD &media_data_array, const std::string &media_version);
    
    // Bounce back media at the given index to its current URL (or home URL, if current URL is empty)
	void mediaNavigateBounceBack(U8 texture_index);
    
    // Returns whether or not this object has permission to navigate or control 
	// the given media entry
	enum MediaPermType {
		MEDIA_PERM_INTERACT, MEDIA_PERM_CONTROL
	};
    bool hasMediaPermission(const LLMediaEntry* media_entry, MediaPermType perm_type);
    
	void mediaNavigated(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, std::string new_location);
	void mediaEvent(LLViewerMediaImpl *impl, LLPluginClassMedia* plugin, LLViewerMediaObserver::EMediaEvent event);
			

	// Sync the given media data with the impl and the given te
	void syncMediaData(S32 te, const LLSD &media_data, bool merge, bool ignore_agent);
	
	// Send media data update to the simulator.
	void sendMediaDataUpdate();

	viewer_media_t getMediaImpl(U8 face_id) const;
	S32 getFaceIndexWithMediaImpl(const LLViewerMediaImpl* media_impl, S32 start_face_id);
	F64 getTotalMediaInterest() const;
   
	bool hasMedia() const;
	
	LLVector3 getApproximateFaceNormal(U8 face_id);

    // Flag any corresponding avatars as needing update.
    void updateVisualComplexity();
    
	void notifyMeshLoaded();
	void notifySkinInfoLoaded(const LLMeshSkinInfo* skin);
	void notifySkinInfoUnavailable();
	
	// Returns 'true' iff the media data for this object is in flight
	bool isMediaDataBeingFetched() const;

	// Returns the "last fetched" media version, or -1 if not fetched yet
	S32 getLastFetchedMediaVersion() const { return mLastFetchedMediaVersion; }

	void addMDCImpl() { ++mMDCImplCount; }
	void removeMDCImpl() { --mMDCImplCount; }
	S32 getMDCImplCount() { return mMDCImplCount; }
	

    // Rigged volume update (for raycasting)
    // By default, this updates the bounding boxes of all the faces and builds an octree for precise per-triangle raycasting
    void updateRiggedVolume(bool force_treat_as_rigged, LLRiggedVolume::FaceIndex face_index = LLRiggedVolume::UPDATE_ALL_FACES, bool rebuild_face_octrees = true);
	LLRiggedVolume* getRiggedVolume();

	//returns true if volume should be treated as a rigged volume
	// - Build tools are open
	// - object is an attachment
	// - object is attached to self
	// - object is rendered as rigged
	bool treatAsRigged();

	//clear out rigged volume and revert back to non-rigged state for picking/LOD/distance updates
	void clearRiggedVolume();

protected:
	S32	computeLODDetail(F32 distance, F32 radius, F32 lod_factor);
	BOOL calcLOD();
	LLFace* addFace(S32 face_index);
	void updateTEData();

	// stats tracking for render complexity
	static S32 mRenderComplexity_last;
	static S32 mRenderComplexity_current;

    void onDrawableUpdateFromServer();
	void requestMediaDataUpdate(bool isNew);
	void cleanUpMediaImpls();
	void addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index) ;
	void removeMediaImpl(S32 texture_index) ;

private:
    bool lodOrSculptChanged(LLDrawable *drawable, BOOL &compiled, BOOL &shouldUpdateOctreeBounds);

public:

	static S32 getRenderComplexityMax() {return mRenderComplexity_last;}
	static void updateRenderComplexity();

	LLViewerTextureAnim *mTextureAnimp;
	U8 mTexAnimMode;
    F32 mLODDistance;
    F32 mLODAdjustedDistance;
    F32 mLODRadius;
private:
	friend class LLDrawable;
	friend class LLFace;

	BOOL		mFaceMappingChanged;
	LLFrameTimer mTextureUpdateTimer;
	S32			mLOD;
	BOOL		mLODChanged;
	BOOL		mSculptChanged;
    BOOL		mColorChanged;
	F32			mSpotLightPriority;
	LLMatrix4	mRelativeXform;
	LLMatrix3	mRelativeXformInvTrans;
	BOOL		mVolumeChanged;
	F32			mVObjRadius;
	LLVolumeInterface *mVolumeImpl;
	LLPointer<LLViewerFetchedTexture> mSculptTexture;
	LLPointer<LLViewerFetchedTexture> mLightTexture;
	media_list_t mMediaImplList;
	S32			mLastFetchedMediaVersion; // as fetched from the server, starts as -1
    U32         mServerDrawableUpdateCount;
	S32 mIndexInTex[LLRender::NUM_VOLUME_TEXTURE_CHANNELS];
	S32 mMDCImplCount;

	bool mResetDebugText;

	LLPointer<LLRiggedVolume> mRiggedVolume;

	bool mSkinInfoFailed;
	LLConstPointer<LLMeshSkinInfo> mSkinInfo;
	// statics
public:
	static F32 sLODSlopDistanceFactor;// Changing this to zero, effectively disables the LOD transition slop
	static F32 sLODFactor;				// LOD scale factor
	static F32 sDistanceFactor;			// LOD distance factor

	static LLPointer<LLObjectMediaDataClient> sObjectMediaClient;
	static LLPointer<LLObjectMediaNavigateClient> sObjectMediaNavigateClient;

protected:
	static S32 sNumLODChanges;

	friend class LLVolumeImplFlexible;

public:
	bool notifyAboutCreatingTexture(LLViewerTexture *texture);
	bool notifyAboutMissingAsset(LLViewerTexture *texture);

private:
	struct material_info 
	{
		LLRender::eTexIndex map;
		U8 te;

		material_info(LLRender::eTexIndex map_, U8 te_)
			: map(map_)
			, te(te_)
		{}
	};

	typedef std::multimap<LLUUID, material_info> mmap_UUID_MAP_t;
	mmap_UUID_MAP_t	mWaitingTextureInfo;

};

#endif // LL_LLVOVOLUME_H

