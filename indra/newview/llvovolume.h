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
#include "m3math.h"		// LLMatrix3
#include "m4math.h"		// LLMatrix4
#include <map>

class LLViewerTextureAnim;
class LLDrawPool;
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


class LLRiggedVolume : public LLVolume
{
public:
	LLRiggedVolume(const LLVolumeParams& params)
		: LLVolume(params, 0.f)
	{
	}

	void update(const LLMeshSkinInfo* skin, LLVOAvatar* avatar, const LLVolume* src_volume);
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
				S32		getLOD() const							{ return mLOD; }
	const LLVector3		getPivotPositionAgent() const;
	const LLMatrix4&	getRelativeXform() const				{ return mRelativeXform; }
	const LLMatrix3&	getRelativeXformInvTrans() const		{ return mRelativeXformInvTrans; }
	/*virtual*/	const LLMatrix4	getRenderMatrix() const;
				typedef std::map<LLUUID, S32> texture_cost_t;
				U32 	getRenderCost(texture_cost_t &textures) const;
				F32		getStreamingCost(S32* bytes, S32* visible_bytes, F32* unscaled_value) const;
	/*virtual*/	F32		getStreamingCost(S32* bytes = NULL, S32* visible_bytes = NULL) { return getStreamingCost(bytes, visible_bytes, NULL); }

	/*virtual*/ U32		getTriangleCount(S32* vcount = NULL) const;
	/*virtual*/ U32		getHighLODTriangleCount();
	/*virtual*/ BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end, 
										  S32 face = -1,                        // which face to check, -1 = ALL_SIDES
										  BOOL pick_transparent = FALSE,
										  S32* face_hit = NULL,                 // which face was hit
										  LLVector3* intersection = NULL,       // return the intersection point
										  LLVector2* tex_coord = NULL,          // return the texture coordinates of the intersection point
										  LLVector3* normal = NULL,             // return the surface normal at the intersection point
										  LLVector3* bi_normal = NULL           // return the surface bi-normal at the intersection point
		);
	
				LLVector3 agentPositionToVolume(const LLVector3& pos) const;
				LLVector3 agentDirectionToVolume(const LLVector3& dir) const;
				LLVector3 volumePositionToAgent(const LLVector3& dir) const;
				LLVector3 volumeDirectionToAgent(const LLVector3& dir) const;

				
				BOOL	getVolumeChanged() const				{ return mVolumeChanged; }
				
	/*virtual*/ F32  	getRadius() const						{ return mVObjRadius; };
				const LLMatrix4& getWorldMatrix(LLXformMatrix* xform) const;

				void	markForUpdate(BOOL priority)			{ LLViewerObject::markForUpdate(priority); mVolumeChanged = TRUE; }

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
	/*virtual*/ S32		setTEScale(const U8 te, const F32 s, const F32 t);
	/*virtual*/ S32		setTEScaleS(const U8 te, const F32 s);
	/*virtual*/ S32		setTEScaleT(const U8 te, const F32 t);
	/*virtual*/ S32		setTETexGen(const U8 te, const U8 texgen);
	/*virtual*/ S32		setTEMediaTexGen(const U8 te, const U8 media);
	/*virtual*/ BOOL 	setMaterial(const U8 material);

				void	setTexture(const S32 face);
				S32     getIndexInTex() const {return mIndexInTex ;}
	/*virtual*/ BOOL	setVolume(const LLVolumeParams &volume_params, const S32 detail, bool unique_volume = false);
				void	updateSculptTexture();
				void    setIndexInTex(S32 index) { mIndexInTex = index ;}
				void	sculpt();
	 static     void    rebuildMeshAssetCallback(LLVFS *vfs,
														  const LLUUID& asset_uuid,
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
				BOOL	genBBoxes(BOOL force_global);
				void	preRebuild();
	virtual		void	updateSpatialExtents(LLVector4a& min, LLVector4a& max);
	virtual		F32		getBinRadius();
	
	virtual U32 getPartitionType() const;

	// For Lights
	void setIsLight(BOOL is_light);
	void setLightColor(const LLColor3& color);
	void setLightIntensity(F32 intensity);
	void setLightRadius(F32 radius);
	void setLightFalloff(F32 falloff);
	void setLightCutoff(F32 cutoff);
	void setLightTextureID(LLUUID id);
	void setSpotLightParams(LLVector3 params);

	BOOL getIsLight() const;
	LLColor3 getLightBaseColor() const; // not scaled by intensity
	LLColor3 getLightColor() const; // scaled by intensity
	LLUUID	getLightTextureID() const;
	bool isLightSpotlight() const;
	LLVector3 getSpotLightParams() const;
	void	updateSpotLightPriority();
	F32		getSpotLightPriority() const;

	LLViewerTexture* getLightTexture();
	F32 getLightIntensity() const;
	F32 getLightRadius() const;
	F32 getLightFalloff() const;
	F32 getLightCutoff() const;
	
	// Flexible Objects
	U32 getVolumeInterfaceID() const;
	virtual BOOL isFlexible() const;
	virtual BOOL isSculpted() const;
	virtual BOOL isMesh() const;
	virtual BOOL hasLightTexture() const;

	BOOL isVolumeGlobal() const;
	BOOL canBeFlexible() const;
	BOOL setIsFlexible(BOOL is_flexible);

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
	
	void notifyMeshLoaded();
	
	// Returns 'true' iff the media data for this object is in flight
	bool isMediaDataBeingFetched() const;

	// Returns the "last fetched" media version, or -1 if not fetched yet
	S32 getLastFetchedMediaVersion() const { return mLastFetchedMediaVersion; }

	void addMDCImpl() { ++mMDCImplCount; }
	void removeMDCImpl() { --mMDCImplCount; }
	S32 getMDCImplCount() { return mMDCImplCount; }
	

	//rigged volume update (for raycasting)
	void updateRiggedVolume();
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
	S32	computeLODDetail(F32	distance, F32 radius);
	BOOL calcLOD();
	LLFace* addFace(S32 face_index);
	void updateTEData();

	// stats tracking for render complexity
	static S32 mRenderComplexity_last;
	static S32 mRenderComplexity_current;

	void requestMediaDataUpdate(bool isNew);
	void cleanUpMediaImpls();
	void addMediaImpl(LLViewerMediaImpl* media_impl, S32 texture_index) ;
	void removeMediaImpl(S32 texture_index) ;
public:

	static S32 getRenderComplexityMax() {return mRenderComplexity_last;}
	static void updateRenderComplexity();

	LLViewerTextureAnim *mTextureAnimp;
	U8 mTexAnimMode;
private:
	friend class LLDrawable;
	friend class LLFace;

	BOOL		mFaceMappingChanged;
	LLFrameTimer mTextureUpdateTimer;
	S32			mLOD;
	BOOL		mLODChanged;
	BOOL		mSculptChanged;
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
	S32 mIndexInTex;
	S32 mMDCImplCount;

	LLPointer<LLRiggedVolume> mRiggedVolume;

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
};

#endif // LL_LLVOVOLUME_H
