/**
 * @file llvoavatar.h
 * @brief Declaration of LLVOAvatar class which is a derivation fo
 * LLViewerObject
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

#ifndef LL_LLVOAVATAR_H
#define LL_LLVOAVATAR_H

#include <map>
#include <deque>
#include <string>
#include <vector>

#include "llchat.h"
#include "lldrawpoolalpha.h"
#include "llviewerobject.h"
#include "llcharacter.h"
#include "llviewerjointmesh.h"
#include "llviewerjointattachment.h"
#include "llrendertarget.h"
#include "llwearable.h"
#include "llvoavatardefines.h"
#include "llcontrol.h"

extern const LLUUID ANIM_AGENT_BODY_NOISE;
extern const LLUUID ANIM_AGENT_BREATHE_ROT;
extern const LLUUID ANIM_AGENT_EDITING;
extern const LLUUID ANIM_AGENT_EYE;
extern const LLUUID ANIM_AGENT_FLY_ADJUST;
extern const LLUUID ANIM_AGENT_HAND_MOTION;
extern const LLUUID ANIM_AGENT_HEAD_ROT;
extern const LLUUID ANIM_AGENT_PELVIS_FIX;
extern const LLUUID ANIM_AGENT_TARGET;
extern const LLUUID ANIM_AGENT_WALK_ADJUST;

class LLTexLayerSet;
class LLVoiceVisualizer;
class LLHUDText;
class LLHUDEffectSpiral;
class LLTexGlobalColor;

class LLVOAvatarBoneInfo;
class LLVOAvatarSkeletonInfo;
class LLVOAvatarXmlInfo;

//------------------------------------------------------------------------
// LLVOAvatar
//------------------------------------------------------------------------
class LLVOAvatar :
	public LLViewerObject,
	public LLCharacter
{
protected:
	virtual ~LLVOAvatar();

public:
	LLVOAvatar(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	/*virtual*/ void markDead();
	void startDefaultMotions();

	static void updateImpostors();

	//--------------------------------------------------------------------
	// LLViewerObject interface
	//--------------------------------------------------------------------
public:
	static void initClass(); // Initialize data that's only init'd once per class.
	static void cleanupClass();	// Cleanup data that's only init'd once per class.
	static BOOL parseSkeletonFile(const std::string& filename);
	virtual U32 processUpdateMessage(LLMessageSystem *mesgsys,
									 void **user_data,
									 U32 block_num,
									 const EObjectUpdateType update_type,
									 LLDataPacker *dp);
	/*virtual*/ BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	void idleUpdateVoiceVisualizer(bool voice_enabled);
	void idleUpdateMisc(bool detailed_update);
	void idleUpdateAppearanceAnimation();
	void idleUpdateLipSync(bool voice_enabled);
	void idleUpdateLoadingEffect();
	void idleUpdateWindEffect();
	void idleUpdateNameTag(const LLVector3& root_pos_last);
	void idleUpdateRenderCost();
	void idleUpdateTractorBeam();
	void idleUpdateBelowWater();

public:
	virtual BOOL updateLOD();
	/*virtual*/ BOOL isActive() const; // Whether this object needs to do an idleUpdate.

	// Graphical stuff for objects - maybe broken out into render class later?
	U32 renderFootShadows();
	U32 renderImpostor(LLColor4U color = LLColor4U(255,255,255,255));
	U32 renderRigid();
	U32 renderSkinned(EAvatarRenderPass pass);
	U32 renderTransparent(BOOL first_pass);
	void renderCollisionVolumes();
	
	/*virtual*/ BOOL lineSegmentIntersect(const LLVector3& start, const LLVector3& end,
										  S32 face = -1,                          // which face to check, -1 = ALL_SIDES
										  BOOL pick_transparent = FALSE,
										  S32* face_hit = NULL,                   // which face was hit
										  LLVector3* intersection = NULL,         // return the intersection point
										  LLVector2* tex_coord = NULL,            // return the texture coordinates of the intersection point
										  LLVector3* normal = NULL,               // return the surface normal at the intersection point
										  LLVector3* bi_normal = NULL             // return the surface bi-normal at the intersection point
		);

	/*virtual*/ void updateTextures(LLAgent &agent);
	// If setting a baked texture, need to request it from a non-local sim.
	/*virtual*/ S32 setTETexture(const U8 te, const LLUUID& uuid);
	/*virtual*/ void onShift(const LLVector3& shift_vector);
	virtual U32 getPartitionType() const;
	
	void updateVisibility();
	void updateAttachmentVisibility(U32 camera_mode);
	void clampAttachmentPositions();
	S32 getAttachmentCount(); // Warning: order(N) not order(1)

	// HUD functions
	BOOL hasHUDAttachment() const;
	LLBBox getHUDBBox() const;
	void rebuildHUD();

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL updateGeometry(LLDrawable *drawable);

	/*virtual*/ void setPixelAreaAndAngle(LLAgent &agent);
	BOOL updateJointLODs();

	virtual void updateRegion(LLViewerRegion *regionp);
	
	virtual const LLVector3 getRenderPosition() const;
	virtual void updateDrawable(BOOL force_damped);
	void updateSpatialExtents(LLVector3& newMin, LLVector3 &newMax);
	void getSpatialExtents(LLVector3& newMin, LLVector3& newMax);
	BOOL isImpostor() const;
	BOOL needsImpostorUpdate() const;
	const LLVector3& getImpostorOffset() const;
	const LLVector2& getImpostorDim() const;
	void getImpostorValues(LLVector3* extents, LLVector3& angle, F32& distance) const;
	void cacheImpostorValues();
	void setImpostorDim(const LLVector2& dim);

	//--------------------------------------------------------------------
	// LLCharacter interface
	//--------------------------------------------------------------------
public:
	virtual const char *getAnimationPrefix() { return "avatar"; }
	virtual LLJoint *getRootJoint() { return &mRoot; }
	virtual LLVector3 getCharacterPosition();
	virtual LLQuaternion getCharacterRotation();
	virtual LLVector3 getCharacterVelocity();
	virtual LLVector3 getCharacterAngularVelocity();
	virtual F32 getTimeDilation();
	virtual void getGround(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm);
	virtual BOOL allocateCharacterJoints( U32 num );
	virtual LLJoint *getCharacterJoint( U32 num );
	virtual void requestStopMotion( LLMotion* motion );
	virtual F32 getPixelArea() const;
	virtual LLPolyMesh*	getHeadMesh();
	virtual LLPolyMesh*	getUpperBodyMesh();
	virtual LLVector3d getPosGlobalFromAgent(const LLVector3 &position);
	virtual LLVector3 getPosAgentFromGlobal(const LLVector3d &position);
	virtual void updateVisualParams();
	virtual BOOL startMotion(const LLUUID& id, F32 time_offset = 0.f);
	virtual BOOL stopMotion(const LLUUID& id, BOOL stop_immediate = FALSE);
	virtual void stopMotionFromSource(const LLUUID& source_id);
	virtual LLVector3 getVolumePos(S32 joint_index, LLVector3& volume_offset);
	virtual LLJoint* findCollisionVolume(U32 volume_id);
	virtual S32 getCollisionVolumeID(std::string &name);
	virtual void addDebugText(const std::string& text);
	virtual const LLUUID& getID();
	virtual LLJoint *getJoint( const std::string &name );

	//--------------------------------------------------------------------
	// Other public functions
	//--------------------------------------------------------------------
public:
	static void		onCustomizeStart();
	static void		onCustomizeEnd();

public:
	static void		dumpTotalLocalTextureByteCount();
protected:
	void			getLocalTextureByteCount( S32* gl_byte_count );

public:
	LLMotion*		findMotion(const LLUUID& id);

	BOOL			isVisible();
	BOOL			isSelf() const { return mIsSelf; }
	BOOL			isCulled() const { return mCulled; }

public:
	static void		cullAvatarsByPixelArea();
	void			setVisibilityRank(U32 rank); 
	U32				getVisibilityRank(); // unused
protected:
	S32				getUnbakedPixelAreaRank();

public:
	void			dumpLocalTextures();
	const LLUUID&	grabLocalTexture(LLVOAvatarDefines::ETextureIndex index);
	BOOL			canGrabLocalTexture(LLVOAvatarDefines::ETextureIndex index);
	BOOL            isTextureDefined(U8 te) const;
	BOOL			isTextureVisible(U8 te) const;
	void			startAppearanceAnimation(BOOL set_by_user, BOOL play_sound);

	void			setCompositeUpdatesEnabled(BOOL b);

	void			addChat(const LLChat& chat);
	void			clearChat();
	void			startTyping() { mTyping = TRUE; mTypingTimer.reset(); }
	void			stopTyping() { mTyping = FALSE; }

	// Returns "FirstName LastName"
	std::string		getFullname() const;

	BOOL updateCharacter(LLAgent &agent);
	void updateHeadOffset();

	F32 getPelvisToFoot() const { return mPelvisToFoot; }

public:
	BOOL isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims);
	void processAnimationStateChanges();
protected:
	BOOL processSingleAnimationStateChange(const LLUUID &anim_id, BOOL start);
	void resetAnimations();

public:
	void resolveHeightGlobal(const LLVector3d &inPos, LLVector3d &outPos, LLVector3 &outNorm);
	void resolveHeightAgent(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm);
	void resolveRayCollisionAgent(const LLVector3d start_pt, const LLVector3d end_pt, LLVector3d &out_pos, LLVector3 &out_norm);
	
	void slamPosition(); // Slam position to transmitted position (for teleport);

	// morph targets and such
	void processAvatarAppearance( LLMessageSystem* mesgsys );
	void onFirstTEMessageReceived();
	void updateSexDependentLayerSets( BOOL set_by_user );
	void dirtyMesh(); // Dirty the avatar mesh
	void hideSkirt();

	virtual void setParent(LLViewerObject* parent);
	virtual void addChild(LLViewerObject *childp);
	virtual void removeChild(LLViewerObject *childp);

	LLViewerJointAttachment* getTargetAttachmentPoint(LLViewerObject* viewer_object);
	BOOL attachObject(LLViewerObject *viewer_object);
	BOOL detachObject(LLViewerObject *viewer_object);
	void lazyAttach();

	void sitOnObject(LLViewerObject *sit_object);
	void getOffObject();

	BOOL isWearingAttachment( const LLUUID& inv_item_id );
	LLViewerObject* getWornAttachment( const LLUUID& inv_item_id );
	const std::string getAttachedPointName(const LLUUID& inv_item_id);

	static LLVOAvatar* findAvatarFromAttachment( LLViewerObject* obj );

	void			updateMeshTextures();

	//--------------------------------------------------------------------
	// texture compositing (used only by the LLTexLayer series of classes)
	//--------------------------------------------------------------------
public:
	LLColor4		getGlobalColor( const std::string& color_name );
	BOOL			isLocalTextureDataAvailable( LLTexLayerSet* layerset );
	BOOL			isLocalTextureDataFinal( LLTexLayerSet* layerset );
	LLVOAvatarDefines::ETextureIndex	getBakedTE( LLTexLayerSet* layerset );
	void			updateComposites();
	void			onGlobalColorChanged( LLTexGlobalColor* global_color, BOOL set_by_user );
	BOOL			getLocalTextureRaw( LLVOAvatarDefines::ETextureIndex index, LLImageRaw* image_raw_pp );
	BOOL			getLocalTextureGL( LLVOAvatarDefines::ETextureIndex index, LLImageGL** image_gl_pp );
	const LLUUID&	getLocalTextureID( LLVOAvatarDefines::ETextureIndex index );
	LLGLuint		getScratchTexName( LLGLenum format, U32* texture_bytes );
	BOOL			bindScratchTexture( LLGLenum format );
	void			invalidateComposite( LLTexLayerSet* layerset, BOOL set_by_user );
	void			invalidateAll();
	void			forceBakeAllTextures(bool slam_for_debug = false);
	static void		processRebakeAvatarTextures(LLMessageSystem* msg, void**);
	void			setNewBakedTexture( LLVOAvatarDefines::ETextureIndex i, const LLUUID& uuid );
	void			setCachedBakedTexture( LLVOAvatarDefines::ETextureIndex i, const LLUUID& uuid );
	void			releaseUnnecessaryTextures();
	void			requestLayerSetUploads();
	bool			hasPendingBakedUploads();
	static void		onLocalTextureLoaded( BOOL succcess, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void		dumpArchetypeXML( void* );
	static void		dumpScratchTextureByteCount();
	static void		dumpBakedStatus();
	static void		deleteCachedImages(bool clearAll=true);
	static void		destroyGL();
	static void		restoreGL();
	static void		resetImpostors();
	static enum EWearableType	getTEWearableType(LLVOAvatarDefines::ETextureIndex te );
	static LLUUID			getDefaultTEImageID(LLVOAvatarDefines::ETextureIndex te );
	static void		onChangeSelfInvisible(BOOL newvalue);
	void			setInvisible(BOOL newvalue);
	static LLColor4 getDummyColor();



	//--------------------------------------------------------------------
	// Clothing colors (conventience functions to access visual parameters
	//--------------------------------------------------------------------
public:
	void			setClothesColor( LLVOAvatarDefines::ETextureIndex te, const LLColor4& new_color, BOOL set_by_user );
	LLColor4		getClothesColor( LLVOAvatarDefines::ETextureIndex te );
	BOOL			teToColorParams( LLVOAvatarDefines::ETextureIndex te, const char* param_name[3] );

	BOOL			isWearingWearableType( EWearableType type );
	void			wearableUpdated( EWearableType type );

	//--------------------------------------------------------------------
	// texture compositing
	//--------------------------------------------------------------------
public:
	void			setLocTexTE( U8 te, LLViewerImage* image, BOOL set_by_user );
	void			setupComposites();

	//--------------------------------------------------------------------
	// Handling partially loaded avatars (Ruth)
	//--------------------------------------------------------------------
public:
	BOOL            isFullyLoaded();
	BOOL            updateIsFullyLoaded();
private:
	BOOL            mFullyLoaded;
	BOOL            mPreviousFullyLoaded;
	BOOL            mFullyLoadedInitialized;
	S32             mFullyLoadedFrameCounter;
	LLFrameTimer    mFullyLoadedTimer;

	LLCachedControl<bool>		mDebugAvatarRotation; // show animation debug info
	
	//--------------------------------------------------------------------
	// Collision Volumes
	//--------------------------------------------------------------------
public:
	S32								mNumCollisionVolumes;
	LLViewerJointCollisionVolume*	mCollisionVolumes;

	//--------------------------------------------------------------------
	// cached pointers to well known joints
	//--------------------------------------------------------------------
public:
	LLViewerJoint* mPelvisp;
	LLViewerJoint* mTorsop;
	LLViewerJoint* mChestp;
	LLViewerJoint* mNeckp;
	LLViewerJoint* mHeadp;
	LLViewerJoint* mSkullp;
	LLViewerJoint* mEyeLeftp;
	LLViewerJoint* mEyeRightp;
	LLViewerJoint* mHipLeftp;
	LLViewerJoint* mHipRightp;
	LLViewerJoint* mKneeLeftp;
	LLViewerJoint* mKneeRightp;
	LLViewerJoint* mAnkleLeftp;
	LLViewerJoint* mAnkleRightp;
	LLViewerJoint* mFootLeftp;
	LLViewerJoint* mFootRightp;
	LLViewerJoint* mWristLeftp;
	LLViewerJoint* mWristRightp;

	//--------------------------------------------------------------------
	// impostor state
	//--------------------------------------------------------------------
public:
	LLRenderTarget	mImpostor;
	BOOL			mNeedsImpostorUpdate;
private:
	LLVector3		mImpostorOffset;
	LLVector2		mImpostorDim;
	BOOL			mNeedsAnimUpdate;
	LLVector3		mImpostorExtents[2];
	LLVector3		mImpostorAngle;
	F32				mImpostorDistance;
	F32				mImpostorPixelArea;
	LLVector3		mLastAnimExtents[2];  

	//--------------------------------------------------------------------
	// Misc Render State
	//--------------------------------------------------------------------
public:
	BOOL			mIsDummy; // For special views
	S32				mSpecialRenderMode; // Special lighting

	//--------------------------------------------------------------------
	// animation state data
	//--------------------------------------------------------------------
public:
	typedef std::map<LLUUID, S32>::iterator AnimIterator;

	std::map<LLUUID, S32> mSignaledAnimations; // requested state of Animation name/value
	std::map<LLUUID, S32> mPlayingAnimations; // current state of Animation name/value

	typedef std::multimap<LLUUID, LLUUID> AnimationSourceMap;
	typedef AnimationSourceMap::iterator AnimSourceIterator;
	AnimationSourceMap mAnimationSources; // object ids that triggered anim ids

	//--------------------------------------------------------------------
	// Shadowing
	//--------------------------------------------------------------------
public:
	void updateShadowFaces();
	LLDrawable*		mShadow;
private:
	LLFace* mShadow0Facep;
	LLFace* mShadow1Facep;
	LLPointer<LLViewerImage> mShadowImagep;

	//--------------------------------------------------------------------
	// Keeps track of foot step state for generating sounds
	//--------------------------------------------------------------------
public:
	void setFootPlane(const LLVector4 &plane) { mFootPlane = plane; }
	LLVector4		mFootPlane;
private:
	BOOL			mWasOnGroundLeft;
	BOOL			mWasOnGroundRight;

	//--------------------------------------------------------------------
	// Pelvis height adjustment members.
	//--------------------------------------------------------------------
public:
	LLVector3		mBodySize;
	S32				mLastSkeletonSerialNum;
private:
	F32				mPelvisToFoot;

	//--------------------------------------------------------------------
	// Display the name, then optionally fade it out
	//--------------------------------------------------------------------
public:
	LLFrameTimer				mChatTimer;
	LLPointer<LLHUDText>		mNameText;
private:
	LLFrameTimer				mTimeVisible;
	std::deque<LLChat>			mChats;
	BOOL						mTyping;
	LLFrameTimer				mTypingTimer;

	//--------------------------------------------------------------------
	// wind rippling in clothes
	//--------------------------------------------------------------------
public:
	LLVector4		mWindVec;
	F32				mRipplePhase;
	BOOL			mBelowWater;
private:
	F32				mWindFreq;
	LLFrameTimer	mRippleTimer;
	F32				mRippleTimeLast;
	LLVector3		mRippleAccel;
	LLVector3		mLastVel;

	//--------------------------------------------------------------------
	// appearance morphing
	//--------------------------------------------------------------------
public:
	BOOL			mAppearanceAnimating;
private:
	LLFrameTimer	mAppearanceMorphTimer;
	BOOL			mAppearanceAnimSetByUser;
	F32				mLastAppearanceBlendTime;

	//--------------------------------------------------------------------
	// Attachments
	//--------------------------------------------------------------------
public:
	// map of attachment points, by ID
	typedef std::map<S32, LLViewerJointAttachment*> attachment_map_t;
	attachment_map_t mAttachmentPoints;
	std::vector<LLPointer<LLViewerObject> > mPendingAttachment;

	//--------------------------------------------------------------------
	// static preferences that are controlled by user settings/menus
	//--------------------------------------------------------------------
public:
	static S32		sRenderName;
	static BOOL		sRenderGroupTitles;
	static S32		sMaxVisible;
	static F32		sRenderDistance; //distance at which avatars will render (affected by control "RenderAvatarMaxVisible")
	static BOOL		sShowAnimationDebug; // show animation debug info
	static BOOL		sUseImpostors; //use impostors for far away avatars
	static BOOL		sShowFootPlane;	// show foot collision plane reported by server
	static BOOL		sVisibleInFirstPerson;
	static S32		sNumLODChangesThisFrame;
	static S32		sNumVisibleChatBubbles;
	static BOOL		sDebugInvisible;
	static BOOL		sShowAttachmentPoints;
	static F32		sLODFactor; // user-settable LOD factor
	static BOOL		sJointDebug; // output total number of joints being touched for each avatar

	static S32 sNumVisibleAvatars; // Number of instances of this class
	
	//--------------------------------------------------------------------
	// Miscellaneous public variables.
	//--------------------------------------------------------------------
public:
	BOOL			mInAir;
	LLFrameTimer	mTimeInAir;
	LLVector3 mHeadOffset; // current head position
	LLViewerJoint mRoot; // avatar skeleton
	BOOL mIsSitting; // sitting state

	//--------------------------------------------------------------------
	// Private member variables.
	//--------------------------------------------------------------------
private:
	BOOL mIsSelf; // True if this avatar is for this viewer's agent

	LLViewerJoint *mScreenp; // special purpose joint for HUD attachments
	BOOL mIsBuilt; // state of deferred character building
	F32 mSpeedAccum; // measures speed (for diagnostics mostly).

	
	// LLFrameTimer mUpdateLODTimer; // controls frequency of LOD change calculations
	BOOL mDirtyMesh;
	BOOL mTurning; // controls hysteresis on avatar rotation
	F32	mSpeed; // misc. animation repeated state

	// Keep track of the material being stepped on
	BOOL mStepOnLand;
	U8 mStepMaterial;
	LLVector3 mStepObjectVelocity;

	// Destroy mesh data after being invisible for a while
	BOOL			mMeshValid;
	BOOL			mVisible;
	LLFrameTimer	mMeshInvisibleTime;

	// Lip synch morph stuff
	bool mLipSyncActive; // we're morphing for lip sync
	LLVisualParam* mOohMorph; // cached pointers morphs for lip sync
	LLVisualParam* mAahMorph; // cached pointers morphs for lip sync

	// Skeleton for skinned avatar
	S32				mNumJoints;
	LLViewerJoint*	mSkeleton;

	// Scratch textures used for compositing
	static LLMap< LLGLenum, LLGLuint*> sScratchTexNames;
	static LLMap< LLGLenum, F32*> sScratchTexLastBindTime;
	static S32 sScratchTexBytes;

	// Global table of sound ids per material, and the ground
	const static LLUUID	sStepSounds[LL_MCODE_END];
	const static LLUUID	sStepSoundOnLand;

	// Xml parse tree of avatar config file
	static LLXmlTree sXMLTree;
	// Xml parse tree of avatar skeleton file
	static LLXmlTree sSkeletonXMLTree;

	// Voice Visualizer is responsible for detecting the user's voice signal, and when the
	// user speaks, it puts a voice symbol over the avatar's head, and triggering gesticulations
	LLVoiceVisualizer*  mVoiceVisualizer;
	int					mCurrentGesticulationLevel;
	
	// Animation timer
	LLTimer		mAnimTimer;
	F32			mTimeLast;	

	LLPointer<LLHUDEffectSpiral> mBeam;
	LLFrameTimer mBeamTimer;

	F32		mAdjustedPixelArea;

	LLWString mNameString;
	std::string  mTitle;
	BOOL	  mNameAway;
	BOOL	  mNameBusy;
	BOOL	  mNameMute;
	BOOL      mNameAppearance;
	BOOL	  mVisibleChat;
	BOOL      mRenderGroupTitles;

	std::string  mDebugText;
	U64		  mLastRegionHandle;
	LLFrameTimer mRegionCrossingTimer;
	S32		  mRegionCrossingCount;
	
	//--------------------------------------------------------------------
	// local textures for compositing.
	//--------------------------------------------------------------------
private:
	LLUUID				mSavedTE[ LLVOAvatarDefines::TEX_NUM_INDICES ];
	BOOL				mFirstTEMessageReceived;
	BOOL				mFirstAppearanceMessageReceived;
	BOOL				mHasBakedHair;

	BOOL				mCulled;
	U32					mVisibilityRank;
	F32					mMinPixelArea; // debug
	F32					mMaxPixelArea; // debug
	BOOL				mHasGrey; // debug
	
	//--------------------------------------------------------------------
	// Global Colors
	//--------------------------------------------------------------------
private:
	LLTexGlobalColor*	mTexSkinColor;
	LLTexGlobalColor*	mTexHairColor;
	LLTexGlobalColor*	mTexEyeColor;

	BOOL				mNeedsSkin;  //if TRUE, avatar has been animated and verts have not been updated
	S32					mUpdatePeriod;

	//--------------------------------------------------------------------
	// Internal functions
	//--------------------------------------------------------------------
protected:
	void buildCharacter();
	void releaseMeshData();
	void restoreMeshData();
	void updateMeshData();
	void computeBodySize();
	const LLUUID& getStepSound() const;
	BOOL needsRenderBeam();

	BOOL			allocateCollisionVolumes( U32 num );
	void			resetHUDAttachments();
	static void		getAnimLabels( LLDynamicArray<std::string>* labels );
	static void		getAnimNames( LLDynamicArray<std::string>* names );

	//--------------------------------------------------------------------
	// Textures and Layers
	//--------------------------------------------------------------------
protected:
	BOOL			loadSkeletonNode();
	BOOL			loadMeshNodes();
	BOOL			isFullyBaked();
	void			deleteLayerSetCaches(bool clearAll = true);
	static BOOL		areAllNearbyInstancesBaked(S32& grey_avatars);
	static void		onBakedTextureMasksLoaded(BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void			setLocalTexture(LLVOAvatarDefines::ETextureIndex i, LLViewerImage* tex, BOOL baked_version_exits);
	void			requestLayerSetUpdate(LLVOAvatarDefines::ETextureIndex i);
	void			addLocalTextureStats(LLVOAvatarDefines::ETextureIndex i, LLViewerImage* imagep, F32 texel_area_ratio, BOOL rendered, BOOL covered_by_baked);
	void			addBakedTextureStats( LLViewerImage* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level);
	static void		onInitialBakedTextureLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void		onBakedTextureLoaded(BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void			useBakedTexture(const LLUUID& id);
	void			dumpAvatarTEs(const std::string& context);
	void			removeMissingBakedTextures();
	LLTexLayerSet*	getLayerSet(LLVOAvatarDefines::ETextureIndex index) const;
	LLHost			getObjectHost() const;
	S32				getLocalDiscardLevel(LLVOAvatarDefines::ETextureIndex index);
public:
	static void updateFreezeCounter(S32 counter = 0 );
private:
	static S32 sFreezeCounter;
	
	//-----------------------------------------------------------------------------------------------
	// Avatar skeleton setup.
	//-----------------------------------------------------------------------------------------------
private:
	BOOL loadAvatar();
	BOOL setupBone(const LLVOAvatarBoneInfo* info, LLViewerJoint* parent, S32 &current_volume_num, S32 &current_joint_num);
	BOOL buildSkeleton(const LLVOAvatarSkeletonInfo *info);

	//-----------------------------------------------------------------------------------------------
	// Per-avatar information about texture data.
	// To-do: Move this to private implementation class
	//-----------------------------------------------------------------------------------------------
private:
	struct BakedTextureData
	{
		LLUUID			mLastTextureIndex;
		LLTexLayerSet*	mTexLayerSet;
		bool			mIsLoaded;
		LLVOAvatarDefines::ETextureIndex	mTextureIndex;
		U32				mMaskTexName;
		// Stores pointers to the joint meshes that this baked texture deals with
		std::vector< LLViewerJointMesh * > mMeshes;  // std::vector<LLViewerJointMesh> mJoints[i]->mMeshParts
	};
	typedef std::vector<BakedTextureData> bakedtexturedata_vec_t;
	bakedtexturedata_vec_t mBakedTextureData;

	struct LocalTextureData
	{
		LocalTextureData() : mIsBakedReady(FALSE), mDiscard(MAX_DISCARD_LEVEL+1), mImage(NULL)
		{}
		LLPointer<LLViewerImage> mImage;
		BOOL mIsBakedReady;
		S32 mDiscard;
	};
	typedef std::map<LLVOAvatarDefines::ETextureIndex, LocalTextureData> localtexture_map_t;
	localtexture_map_t mLocalTextureData;

	typedef std::multimap<std::string, LLPolyMesh*> polymesh_map_t;
	polymesh_map_t mMeshes;
	std::vector<LLViewerJoint *> mMeshLOD;
	S32 mNumInitFaces ; //number of faces generated when creating the avatar drawable, does not inculde splitted faces due to long vertex buffer.

	//-----------------------------------------------------------------------------------------------
	// Static texture/mesh/baked dictionary for avatars
	//-----------------------------------------------------------------------------------------------
public:
	static BOOL isIndexLocalTexture(LLVOAvatarDefines::ETextureIndex i);
	static BOOL isIndexBakedTexture(LLVOAvatarDefines::ETextureIndex i);
private:
	static const LLVOAvatarDefines::LLVOAvatarDictionary *getDictionary() { return sAvatarDictionary; }
	static LLVOAvatarDefines::LLVOAvatarDictionary *sAvatarDictionary;
	static LLVOAvatarSkeletonInfo* sAvatarSkeletonInfo;
	static LLVOAvatarXmlInfo* sAvatarXmlInfo;

	//-----------------------------------------------------------------------------------------------
	// Diagnostics
	//-----------------------------------------------------------------------------------------------
public:
	static F32 		sUnbakedTime; // Total seconds with >=1 unbaked avatars
	static F32 		sUnbakedUpdateTime; // Last time stats were updated (to prevent multiple updates per frame) 
	static F32 		sGreyTime; // Total seconds with >=1 grey avatars
	static F32 		sGreyUpdateTime; // Last time stats were updated (to prevent multiple updates per frame) 

	const std::string getBakedStatusForPrintout() const;
};

//-----------------------------------------------------------------------------------------------
// Inlines
//-----------------------------------------------------------------------------------------------
inline BOOL LLVOAvatar::isTextureDefined(U8 te) const
{
	return (getTEImage(te)->getID() != IMG_DEFAULT_AVATAR && getTEImage(te)->getID() != IMG_DEFAULT);
}

inline BOOL LLVOAvatar::isTextureVisible(U8 te) const
{
	return ((isTextureDefined(te) || isSelf())
			&& (getTEImage(te)->getID() != IMG_INVISIBLE 
				|| LLDrawPoolAlpha::sShowDebugAlpha));
}

#endif // LL_VO_AVATAR_H
