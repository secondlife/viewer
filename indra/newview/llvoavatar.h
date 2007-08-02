/** 
 * @file llvoavatar.h
 * @brief Declaration of LLVOAvatar class which is a derivation fo
 * LLViewerObject
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVOAVATAR_H
#define LL_LLVOAVATAR_H

// May help Visual Studio avoid opening this file.

#include <set>
#include <map>
#include <algorithm>
#include <deque>

#include "llchat.h"
#include "llviewerobject.h"
#include "lljointsolverrp3.h"
#include "llviewerjointshape.h"
#include "llviewerjointmesh.h"
#include "llviewerjointattachment.h"
#include "llcharacter.h"
#include "material_codes.h"
#include "llanimationstates.h"
#include "v4coloru.h"
#include "llstring.h"
#include "llframetimer.h"
#include "llxmltree.h"
#include "llwearable.h"

//Ventrella
//#include "llvoiceclient.h"
#include "llvoicevisualizer.h"
//End Ventrella

const S32 VOAVATAR_SCRATCH_TEX_WIDTH = 512;
const S32 VOAVATAR_SCRATCH_TEX_HEIGHT = 512;

const LLUUID ANIM_AGENT_BODY_NOISE		=	LLUUID("9aa8b0a6-0c6f-9518-c7c3-4f41f2c001ad"); //"body_noise"
const LLUUID ANIM_AGENT_BREATHE_ROT	=	LLUUID("4c5a103e-b830-2f1c-16bc-224aa0ad5bc8");  //"breathe_rot"
const LLUUID ANIM_AGENT_EDITING		=	LLUUID("2a8eba1d-a7f8-5596-d44a-b4977bf8c8bb");  //"editing"
const LLUUID ANIM_AGENT_EYE			=	LLUUID("5c780ea8-1cd1-c463-a128-48c023f6fbea");  //"eye"
const LLUUID ANIM_AGENT_FLY_ADJUST		=	LLUUID("db95561f-f1b0-9f9a-7224-b12f71af126e");  //"fly_adjust"
const LLUUID ANIM_AGENT_HAND_MOTION	=	LLUUID("ce986325-0ba7-6e6e-cc24-b17c4b795578");  //"hand_motion"
const LLUUID ANIM_AGENT_HEAD_ROT		=	LLUUID("e6e8d1dd-e643-fff7-b238-c6b4b056a68d");  //"head_rot"
const LLUUID ANIM_AGENT_PELVIS_FIX		=	LLUUID("0c5dd2a2-514d-8893-d44d-05beffad208b");  //"pelvis_fix"
const LLUUID ANIM_AGENT_TARGET			=	LLUUID("0e4896cb-fba4-926c-f355-8720189d5b55");  //"target"
const LLUUID ANIM_AGENT_WALK_ADJUST	=	LLUUID("829bc85b-02fc-ec41-be2e-74cc6dd7215d");  //"walk_adjust"

class LLChat;
class LLXmlTreeNode;
class LLTexLayerSet;
class LLTexGlobalColor;
class LLTexGlobalColorInfo;
class LLTexLayerSetInfo;
class LLDriverParamInfo;

class LLHUDText;
class LLHUDEffectSpiral;

class LLVertexBufferAvatar : public LLVertexBuffer
{
public:
	LLVertexBufferAvatar();
	virtual void setupVertexBuffer(U32 data_mask) const;
};

typedef enum e_mesh_id
{
	MESH_ID_HAIR,
	MESH_ID_HEAD,
	MESH_ID_UPPER_BODY,
	MESH_ID_LOWER_BODY,
	MESH_ID_SKIRT
} eMeshID;

typedef enum e_render_name
{
	RENDER_NAME_NEVER,
	RENDER_NAME_FADE,
	RENDER_NAME_ALWAYS
} eRenderName;

const S32 BAKED_TEXTURE_COUNT = 5;  // number of values in ETextureIndex that are pre-composited

//------------------------------------------------------------------------
// LLVOAvatar Support classes
//------------------------------------------------------------------------

class LLVOAvatarBoneInfo
{
	friend class LLVOAvatar;
	friend class LLVOAvatarSkeletonInfo;
public:
	LLVOAvatarBoneInfo() : mIsJoint(FALSE) {}
	~LLVOAvatarBoneInfo()
	{
		std::for_each(mChildList.begin(), mChildList.end(), DeletePointer());
	}
	BOOL parseXml(LLXmlTreeNode* node);
	
protected:
	LLString mName;
	BOOL mIsJoint;
	LLVector3 mPos;
	LLVector3 mRot;
	LLVector3 mScale;
	LLVector3 mPivot;
	typedef std::vector<LLVOAvatarBoneInfo*> child_list_t;
	child_list_t mChildList;
};

class LLVOAvatarSkeletonInfo
{
	friend class LLVOAvatar;
public:
	LLVOAvatarSkeletonInfo() :
		mNumBones(0), mNumCollisionVolumes(0) {}
	~LLVOAvatarSkeletonInfo()
	{
		std::for_each(mBoneInfoList.begin(), mBoneInfoList.end(), DeletePointer());
	}
	BOOL parseXml(LLXmlTreeNode* node);
	S32 getNumBones() { return mNumBones; }
	S32 getNumCollisionVolumes() { return mNumCollisionVolumes; }
	
protected:
	S32 mNumBones;
	S32 mNumCollisionVolumes;
	typedef std::vector<LLVOAvatarBoneInfo*> bone_info_list_t;
	bone_info_list_t mBoneInfoList;
};


//------------------------------------------------------------------------
// LLVOAvatarInfo
// One instance (in LLVOAvatar) with common data parsed from the XML files
//------------------------------------------------------------------------
class LLVOAvatarInfo
{
	friend class LLVOAvatar;
public:
	LLVOAvatarInfo();
	~LLVOAvatarInfo();
	
protected:
	BOOL 	parseXmlSkeletonNode(LLXmlTreeNode* root);
	BOOL 	parseXmlMeshNodes(LLXmlTreeNode* root);
	BOOL 	parseXmlColorNodes(LLXmlTreeNode* root);
	BOOL 	parseXmlLayerNodes(LLXmlTreeNode* root);
	BOOL 	parseXmlDriverNodes(LLXmlTreeNode* root);
	
	struct LLVOAvatarMeshInfo
	{
		typedef std::pair<LLPolyMorphTargetInfo*,BOOL> morph_info_pair_t;
		typedef std::vector<morph_info_pair_t> morph_info_list_t;

		LLVOAvatarMeshInfo() : mLOD(0), mMinPixelArea(.1f) {}
		~LLVOAvatarMeshInfo()
		{
			morph_info_list_t::iterator iter;
			for (iter = mPolyMorphTargetInfoList.begin(); iter != mPolyMorphTargetInfoList.end(); iter++)
			{
				delete iter->first;
			}
			mPolyMorphTargetInfoList.clear();
		}

		LLString 	mType;
		S32			mLOD;
		LLString	mMeshFileName;
		LLString	mReferenceMeshName;
		F32			mMinPixelArea;
		morph_info_list_t mPolyMorphTargetInfoList;
	};
	typedef std::vector<LLVOAvatarMeshInfo*> mesh_info_list_t;
	mesh_info_list_t mMeshInfoList;

	typedef std::vector<LLPolySkeletalDistortionInfo*> skeletal_distortion_info_list_t;
	skeletal_distortion_info_list_t mSkeletalDistortionInfoList;
	
	struct LLVOAvatarAttachmentInfo
	{
		LLVOAvatarAttachmentInfo()
			: mGroup(-1), mAttachmentID(-1), mPieMenuSlice(-1), mVisibleFirstPerson(FALSE),
			  mIsHUDAttachment(FALSE), mHasPosition(FALSE), mHasRotation(FALSE) {}
		LLString mName;
		LLString mJointName;
		LLVector3 mPosition;
		LLVector3 mRotationEuler;
		S32 mGroup;
		S32 mAttachmentID;
		S32 mPieMenuSlice;
		BOOL mVisibleFirstPerson;
		BOOL mIsHUDAttachment;
		BOOL mHasPosition;
		BOOL mHasRotation;
	};
	typedef std::vector<LLVOAvatarAttachmentInfo*> attachment_info_list_t;
	attachment_info_list_t mAttachmentInfoList;
	
	LLTexGlobalColorInfo *mTexSkinColorInfo;
	LLTexGlobalColorInfo *mTexHairColorInfo;
	LLTexGlobalColorInfo *mTexEyeColorInfo;

	typedef std::vector<LLTexLayerSetInfo*> layer_info_list_t;
	layer_info_list_t mLayerInfoList;

	typedef std::vector<LLDriverParamInfo*> driver_info_list_t;
	driver_info_list_t mDriverInfoList;
};

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

	struct CompareScreenAreaGreater
	{		
		bool operator()(const LLCharacter* const& lhs, const LLCharacter* const& rhs)
		{
			return lhs->getPixelArea() > rhs->getPixelArea();
		}
	};

	enum 
	{
		VERTEX_DATA_MASK =	(1 << LLVertexBuffer::TYPE_VERTEX) |
							(1 << LLVertexBuffer::TYPE_NORMAL) |
							(1 << LLVertexBuffer::TYPE_TEXCOORD) |
							(1 << LLVertexBuffer::TYPE_WEIGHT) |
							(1 << LLVertexBuffer::TYPE_CLOTHWEIGHT)							
	};

	LLVOAvatar(const LLUUID &id, const LLPCode pcode, LLViewerRegion *regionp);
	/*virtual*/ void markDead();

	//--------------------------------------------------------------------
	// LLViewerObject interface
	//--------------------------------------------------------------------
	static void initClass();	// Initialize data that's only inited once per class.
	static void cleanupClass();	// Cleanup data that's only inited once per class.
	static BOOL parseSkeletonFile(const LLString& filename);
	virtual U32 processUpdateMessage(	LLMessageSystem *mesgsys,
										void **user_data,
										U32 block_num,
										const EObjectUpdateType update_type,
										LLDataPacker *dp);
	virtual BOOL idleUpdate(LLAgent &agent, LLWorld &world, const F64 &time);
	virtual BOOL updateLOD();
	void setFootPlane(const LLVector4 &plane) { mFootPlane = plane; }
	/*virtual*/ BOOL    isActive() const; // Whether this object needs to do an idleUpdate.

	// Graphical stuff for objects - maybe broken out into render class later?

	U32 renderFootShadows();
	U32 renderRigid();
	U32 renderSkinned(EAvatarRenderPass pass);
	U32 renderTransparent();
	void renderCollisionVolumes();
	
	/*virtual*/ void updateTextures(LLAgent &agent);
	// If setting a baked texture, need to request it from a non-local sim.
	/*virtual*/ S32 setTETexture(const U8 te, const LLUUID& uuid);
	
	virtual U32 getPartitionType() const;
	
	void updateVisibility(BOOL force_invisible);
	void updateAttachmentVisibility(U32 camera_mode);
	void clampAttachmentPositions();
	S32 getAttachmentCount();	// Warning: order(N) not order(1)

	BOOL hasHUDAttachment();
	LLBBox getHUDBBox();
// 	void renderHUD(BOOL for_select); // old
	void rebuildHUD();

	static void updateAllAvatarVisiblity();

	/*virtual*/ LLDrawable* createDrawable(LLPipeline *pipeline);
	/*virtual*/ BOOL		updateGeometry(LLDrawable *drawable);
	void updateShadowFaces();

	/*virtual*/ void		setPixelAreaAndAngle(LLAgent &agent);
	BOOL					updateJointLODs();

	void writeCAL3D(std::string& path, std::string& file_base);

	virtual void updateRegion(LLViewerRegion *regionp);

	
	void updateSpatialExtents(LLVector3& newMin, LLVector3 &newMax);
	
	//--------------------------------------------------------------------
	// texture entry assignment
	//--------------------------------------------------------------------
	enum ETextureIndex
	{
		TEX_HEAD_BODYPAINT = 0,
		TEX_UPPER_SHIRT = 1,
		TEX_LOWER_PANTS = 2,
		TEX_EYES_IRIS = 3,
		TEX_HAIR = 4,
		TEX_UPPER_BODYPAINT = 5,
		TEX_LOWER_BODYPAINT = 6,
		TEX_LOWER_SHOES = 7,
		TEX_HEAD_BAKED = 8,			// Pre-composited
		TEX_UPPER_BAKED = 9,		// Pre-composited
		TEX_LOWER_BAKED = 10,		// Pre-composited
		TEX_EYES_BAKED = 11,		// Pre-composited
		TEX_LOWER_SOCKS = 12,
		TEX_UPPER_JACKET = 13,
		TEX_LOWER_JACKET = 14,
		TEX_UPPER_GLOVES = 15,
		TEX_UPPER_UNDERSHIRT = 16,
		TEX_LOWER_UNDERPANTS = 17,
		TEX_SKIRT = 18,
		TEX_SKIRT_BAKED = 19,		// Pre-composited
		TEX_NUM_ENTRIES = 20
	};
	// Note: if TEX_NUM_ENTRIES changes, update AGENT_TEXTURES in llagentinfo.h, mTextureIndexBaked, and BAKED_TEXTURE_COUNT

	static BOOL isTextureIndexBaked( S32 i )
		{
			switch(i)
			{
			case TEX_HEAD_BAKED:
			case TEX_UPPER_BAKED:
			case TEX_LOWER_BAKED:
			case TEX_EYES_BAKED:
			case TEX_SKIRT_BAKED:
				return TRUE;
			default:
				return FALSE;
			}
		}

	//--------------------------------------------------------------------
	// LLCharacter interface
	//--------------------------------------------------------------------
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
	virtual LLVector3d	getPosGlobalFromAgent(const LLVector3 &position);
	virtual LLVector3	getPosAgentFromGlobal(const LLVector3d &position);
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
	BOOL			allocateCollisionVolumes( U32 num );
	void			resetHUDAttachments();
	static void		getAnimLabels( LLDynamicArray<const char*>* labels );
	static void		getAnimNames( LLDynamicArray<const char*>* names );

	static void		onCustomizeStart();
	static void		onCustomizeEnd();

	void			getLocalTextureByteCount( S32* gl_byte_count );
	static void		dumpTotalLocalTextureByteCount();
	LLMotion*		findMotion(const LLUUID& id);

	BOOL			isVisible();
	BOOL			isSelf() { return mIsSelf; }
	BOOL			isCulled() { return mCulled; }
	
	S32				getUnbakedPixelAreaRank();
	static void		cullAvatarsByPixelArea();

	void			dumpLocalTextures();
	const LLUUID&	grabLocalTexture(ETextureIndex index);
	BOOL			canGrabLocalTexture(ETextureIndex index);
	void			startAppearanceAnimation(BOOL set_by_user, BOOL play_sound);

	void			setCompositeUpdatesEnabled(BOOL b);

	void			addChat(const LLChat& chat);
	void			clearChat();
	void			startTyping() { mTyping = TRUE; mTypingTimer.reset(); }
	void			stopTyping() { mTyping = FALSE; }

	// Returns "FirstName LastName"
	LLString		getFullname() const;

	//--------------------------------------------------------------------
	// internal (pseudo-private) functions
	//--------------------------------------------------------------------
	F32 getPelvisToFoot() { return mPelvisToFoot; }

	void buildCharacter();
	void releaseMeshData();
	void restoreMeshData();
	void updateMeshData();

	void computeBodySize();

	void updateCharacter(LLAgent &agent);
	void updateHeadOffset();

	LLUUID& getStepSound();
	void processAnimationStateChanges();
	BOOL processSingleAnimationStateChange(const LLUUID &anim_id, BOOL start);
	void resetAnimations();
	BOOL isAnyAnimationSignaled(const LLUUID *anim_array, const S32 num_anims);

	BOOL needsRenderBeam();

	// Utility functions
	void resolveHeightGlobal(const LLVector3d &inPos, LLVector3d &outPos, LLVector3 &outNorm);
	void resolveHeightAgent(const LLVector3 &inPos, LLVector3 &outPos, LLVector3 &outNorm);
	void resolveRayCollisionAgent(const LLVector3d start_pt, const LLVector3d end_pt, LLVector3d &out_pos, LLVector3 &out_norm);

	void slamPosition(); // Slam position to transmitted position (for teleport);

	BOOL loadAvatar();
	BOOL setupBone(LLVOAvatarBoneInfo* info, LLViewerJoint* parent);
	BOOL buildSkeleton(LLVOAvatarSkeletonInfo *info);

	// morph targets and such
	void processAvatarAppearance( LLMessageSystem* mesgsys );
	void onFirstTEMessageReceived();
	void updateSexDependentLayerSets( BOOL set_by_user );
	void dirtyMesh(); // Dirty the avatar mesh

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
	const LLString getAttachedPointName(const LLUUID& inv_item_id);

	static LLVOAvatar* findAvatarFromAttachment( LLViewerObject* obj );

	void			updateMeshTextures();

	//--------------------------------------------------------------------
	// local textures for compositing.
	//--------------------------------------------------------------------
	enum ELocTexIndex
	{
		LOCTEX_UPPER_SHIRT = 0,
		LOCTEX_UPPER_BODYPAINT = 1,
		LOCTEX_LOWER_PANTS = 2,
		LOCTEX_LOWER_BODYPAINT = 3,
		LOCTEX_HEAD_BODYPAINT = 4,
		LOCTEX_LOWER_SHOES = 5,
		LOCTEX_LOWER_SOCKS = 6,
		LOCTEX_UPPER_JACKET = 7,
		LOCTEX_LOWER_JACKET = 8,
		LOCTEX_UPPER_GLOVES = 9,
		LOCTEX_UPPER_UNDERSHIRT = 10,
		LOCTEX_LOWER_UNDERPANTS = 11,
		LOCTEX_EYES_IRIS = 12,
		LOCTEX_SKIRT = 13,
		LOCTEX_NUM_ENTRIES = 14
	};

	//--------------------------------------------------------------------
	// texture compositing (used only by the LLTexLayer series of classes)
	//--------------------------------------------------------------------
	LLColor4		getGlobalColor( const LLString& color_name );
	BOOL			isLocalTextureDataAvailable( LLTexLayerSet* layerset );
	BOOL			isLocalTextureDataFinal( LLTexLayerSet* layerset );
	ETextureIndex	getBakedTE( LLTexLayerSet* layerset );
	void			updateComposites();
	void			onGlobalColorChanged( LLTexGlobalColor* global_color, BOOL set_by_user );
	BOOL			getLocalTextureRaw( S32 index, LLImageRaw* image_raw_pp );
	BOOL			getLocalTextureGL( S32 index, LLImageGL** image_gl_pp );
	const LLUUID&	getLocalTextureID( S32 index );
	LLGLuint		getScratchTexName( LLGLenum format, U32* texture_bytes );
	BOOL			bindScratchTexture( LLGLenum format );
	void			invalidateComposite( LLTexLayerSet* layerset, BOOL set_by_user );
	void			forceBakeAllTextures(bool slam_for_debug = false);
	static void		processRebakeAvatarTextures(LLMessageSystem* msg, void**);
	void			setNewBakedTexture( ETextureIndex i, const LLUUID& uuid );
	void			setCachedBakedTexture( ETextureIndex i, const LLUUID& uuid );
	void			requestLayerSetUploads();
	bool			hasPendingBakedUploads();
	static void		onLocalTextureLoaded( BOOL succcess, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void		dumpArchetypeXML( void* );
	static void		dumpScratchTextureByteCount();
	static void		dumpBakedStatus();
	static void		deleteCachedImages();
	static void		destroyGL();
	static void		restoreGL();
	static void		initVertexPrograms();
	static void		cleanupVertexPrograms();
	static enum EWearableType	getTEWearableType( S32 te );
	static LLUUID			getDefaultTEImageID( S32 te );

	//--------------------------------------------------------------------
	// Clothing colors (conventience functions to access visual parameters
	//--------------------------------------------------------------------
	void			setClothesColor( ETextureIndex te, const LLColor4& new_color, BOOL set_by_user );
	LLColor4		getClothesColor( ETextureIndex te );
	BOOL			teToColorParams( ETextureIndex te, const char* param_name[3] );

	BOOL			isWearingWearableType( EWearableType type );

	//--------------------------------------------------------------------
	// texture compositing
	//--------------------------------------------------------------------
	void			setLocTexTE( U8 te, LLViewerImage* image, BOOL set_by_user );
	void			setupComposites();

	//--------------------------------------------------------------------
	// member variables
	//--------------------------------------------------------------------

	BOOL mDirtyMesh;

	LLFace *mShadow0Facep;
	LLFace *mShadow1Facep;

	LLFrameTimer mUpdateLODTimer; // controls frequency of LOD change calculations
	
	//--------------------------------------------------------------------
	// State of deferred character building
	//--------------------------------------------------------------------
	BOOL mIsBuilt;

	//--------------------------------------------------------------------
	// avatar definition name
	//--------------------------------------------------------------------
	char mAvatarDefinition[64];		/* Flawfinder: ignore */

	//--------------------------------------------------------------------
	// skeleton for skinned avatar
	//--------------------------------------------------------------------
	S32				mNumJoints;
	LLViewerJoint	*mSkeleton;

	S32								mNumCollisionVolumes;
	LLViewerJointCollisionVolume*	mCollisionVolumes;

	//--------------------------------------------------------------------
	// cached pointers to well known joints
	//--------------------------------------------------------------------
	LLViewerJoint *mPelvisp;
	LLViewerJoint *mTorsop;
	LLViewerJoint *mChestp;
	LLViewerJoint *mNeckp;
	LLViewerJoint *mHeadp;
	LLViewerJoint *mSkullp;
	LLViewerJoint *mEyeLeftp;
	LLViewerJoint *mEyeRightp;
	LLViewerJoint *mHipLeftp;
	LLViewerJoint *mHipRightp;
	LLViewerJoint *mKneeLeftp;
	LLViewerJoint *mKneeRightp;
	LLViewerJoint *mAnkleLeftp;
	LLViewerJoint *mAnkleRightp;
	LLViewerJoint *mFootLeftp;
	LLViewerJoint *mFootRightp;
	LLViewerJoint *mWristLeftp;
	LLViewerJoint *mWristRightp;

	//--------------------------------------------------------------------
	// special purpose joint for HUD attachments
	//--------------------------------------------------------------------
	LLViewerJoint *mScreenp;
	F32				mHUDTargetZoom;
	F32				mHUDCurZoom;

	//--------------------------------------------------------------------
	// mesh objects for skinned avatar
	//--------------------------------------------------------------------
	LLViewerJoint		mHairLOD;
	LLViewerJointMesh		mHairMesh0;
	LLViewerJointMesh		mHairMesh1;
	LLViewerJointMesh		mHairMesh2;
	LLViewerJointMesh		mHairMesh3;
	LLViewerJointMesh		mHairMesh4;
	LLViewerJointMesh		mHairMesh5;

	LLViewerJoint		mHeadLOD;
	LLViewerJointMesh		mHeadMesh0;
	LLViewerJointMesh		mHeadMesh1;
	LLViewerJointMesh		mHeadMesh2;
	LLViewerJointMesh		mHeadMesh3;
	LLViewerJointMesh		mHeadMesh4;

	LLViewerJoint		mEyeLashLOD;
	LLViewerJointMesh		mEyeLashMesh0;

	LLViewerJoint		mUpperBodyLOD;
	LLViewerJointMesh		mUpperBodyMesh0;
	LLViewerJointMesh		mUpperBodyMesh1;
	LLViewerJointMesh		mUpperBodyMesh2;
	LLViewerJointMesh		mUpperBodyMesh3;
	LLViewerJointMesh		mUpperBodyMesh4;

	LLViewerJoint		mLowerBodyLOD;
	LLViewerJointMesh		mLowerBodyMesh0;
	LLViewerJointMesh		mLowerBodyMesh1;
	LLViewerJointMesh		mLowerBodyMesh2;
	LLViewerJointMesh		mLowerBodyMesh3;
	LLViewerJointMesh		mLowerBodyMesh4;

	LLViewerJoint		mEyeBallLeftLOD;
	LLViewerJointMesh		mEyeBallLeftMesh0;
	LLViewerJointMesh		mEyeBallLeftMesh1;

	LLViewerJoint		mEyeBallRightLOD;
	LLViewerJointMesh		mEyeBallRightMesh0;
	LLViewerJointMesh		mEyeBallRightMesh1;

	LLViewerJoint		mSkirtLOD;
	LLViewerJointMesh		mSkirtMesh0;
	LLViewerJointMesh		mSkirtMesh1;
	LLViewerJointMesh		mSkirtMesh2;
	LLViewerJointMesh		mSkirtMesh3;
	LLViewerJointMesh		mSkirtMesh4;

	typedef std::multimap<LLString, LLPolyMesh*> mesh_map_t;
	mesh_map_t				mMeshes;

	//--------------------------------------------------------------------
	// true if this avatar is for this viewers agent
	//--------------------------------------------------------------------
	BOOL			mIsSelf;

	//--------------------------------------------------------------------
	// texture ids and pointers
	//--------------------------------------------------------------------
	LLUUID			mShadowImageID;
	LLPointer<LLViewerImage> mShadowImagep;

	LLUUID			mLastHeadBakedID;
	LLUUID			mLastUpperBodyBakedID;
	LLUUID			mLastLowerBodyBakedID;
	LLUUID			mLastEyesBakedID;
	LLUUID			mLastSkirtBakedID;

	//--------------------------------------------------------------------
	// Misc Render State
	//--------------------------------------------------------------------
	BOOL			mIsDummy; // For special views
	S32				mSpecialRenderMode; // Special lighting

	//--------------------------------------------------------------------
	// Animation timer
	//--------------------------------------------------------------------
	LLTimer		mAnimTimer;
	F32			mTimeLast;

	//--------------------------------------------------------------------
	// Measures speed (for diagnostics mostly).
	//--------------------------------------------------------------------
	F32 mSpeedAccum;

	//--------------------------------------------------------------------
	// animation state data
	//--------------------------------------------------------------------
	typedef std::map<LLUUID, S32>::iterator AnimIterator;

	std::map<LLUUID, S32> mSignaledAnimations;		// requested state of Animation name/value
	std::map<LLUUID, S32> mPlayingAnimations;		// current state of Animation name/value

	typedef std::multimap<LLUUID, LLUUID> AnimationSourceMap;
	typedef AnimationSourceMap::iterator AnimSourceIterator;
	AnimationSourceMap mAnimationSources;	// object ids that triggered anim ids

	BOOL				mTurning;		// controls hysteresis on avatar rotation

	//--------------------------------------------------------------------
	// misc. animation related state
	//--------------------------------------------------------------------
	F32				mSpeed;

	//
	// Shadow stuff
	//
	LLDrawable*		mShadow;
	BOOL			mInAir;
	LLFrameTimer	mTimeInAir;

	//--------------------------------------------------------------------
	// Keeps track of foot step state for generating sounds
	//--------------------------------------------------------------------
	BOOL			mWasOnGroundLeft;
	BOOL			mWasOnGroundRight;
	LLVector4		mFootPlane;

	//--------------------------------------------------------------------
	// Keep track of the material being stepped on
	//--------------------------------------------------------------------
	BOOL			mStepOnLand;
	U8				mStepMaterial;
	LLVector3		mStepObjectVelocity;

	//--------------------------------------------------------------------
	// Pelvis height adjustment members.
	//--------------------------------------------------------------------
	F32				mPelvisToFoot;
	LLVector3		mBodySize;
	S32				mLastSkeletonSerialNum;

	//--------------------------------------------------------------------
	// current head position
	//--------------------------------------------------------------------
	LLVector3		mHeadOffset;

	//--------------------------------------------------------------------
	// avatar skeleton
	//--------------------------------------------------------------------
	LLViewerJoint	mRoot;

	//--------------------------------------------------------------------
	// sitting state
	//--------------------------------------------------------------------
	BOOL			mIsSitting;

	//--------------------------------------------------------------------
	// Display the name, then optionally fade it out
	//--------------------------------------------------------------------
	LLFrameTimer				mTimeVisible;
	LLPointer<LLHUDText>		mNameText;
	std::deque<LLChat>			mChats;
	LLFrameTimer				mChatTimer;
	BOOL						mTyping;
	LLFrameTimer				mTypingTimer;

	//--------------------------------------------------------------------
	// destroy mesh data after being invisible for a while
	//--------------------------------------------------------------------
	BOOL			mMeshValid;
	BOOL			mVisible;
	LLFrameTimer	mMeshInvisibleTime;

	//--------------------------------------------------------------------
	// wind rippling in clothes
	//--------------------------------------------------------------------
	LLVector4		mWindVec;
	F32				mWindFreq;
	F32				mRipplePhase;
	LLFrameTimer	mRippleTimer;
	F32				mRippleTimeLast;
	LLVector3		mRippleAccel;
	LLVector3		mLastVel;
	BOOL			mBelowWater;

	//--------------------------------------------------------------------
	// appearance morphing
	//--------------------------------------------------------------------
	LLFrameTimer	mAppearanceMorphTimer;
	BOOL			mAppearanceAnimSetByUser;
	F32				mLastAppearanceBlendTime;
	BOOL			mAppearanceAnimating;

	//--------------------------------------------------------------------
	// static members
	//--------------------------------------------------------------------
	static S32		sMaxVisible;
	static S32		sCurJoint;
	static S32		sCurVolume;
	static BOOL		sShowAnimationDebug; // show animation debug info
	static BOOL		sShowFootPlane;	// show foot collision plane reported by server
	static BOOL		sShowCollisionVolumes;	// show skeletal collision volumes
	static BOOL		sVisibleInFirstPerson;

	static S32		sMaxOtherAvatarsToComposite;

	static S32		sNumLODChangesThisFrame;

	// global table of sound ids per material, and the ground
	static LLUUID	sStepSounds[LL_MCODE_END];
	static LLUUID	sStepSoundOnLand;

	static S32		sRenderName;
	static S32		sNumVisibleChatBubbles;
	static BOOL		sDebugInvisible;
	static BOOL		sShowAttachmentPoints;

	// Number of instances of this class
	static S32		sNumVisibleAvatars;

	// Scratch textures used for compositing
	static LLMap< LLGLenum, LLGLuint*> sScratchTexNames;
	static LLMap< LLGLenum, F32*> sScratchTexLastBindTime;
	static S32 sScratchTexBytes;

	// map of attachment points, by ID
	LLPtrSkipMap<S32, LLViewerJointAttachment*> mAttachmentPoints;

	// xml parse tree of avatar config file
	static LLXmlTree sXMLTree;
	// xml parse tree of avatar skeleton file
	static LLXmlTree sSkeletonXMLTree;

	// number of avatar duplicates, for debugging purposes
	static BOOL		sAvatarLoadTest;

	// user-settable LOD factor
	static F32		sLODFactor;

	// output total number of joints being touched for each avatar
	static BOOL		sJointDebug;
	static ETextureIndex sBakedTextureIndices[BAKED_TEXTURE_COUNT];

	//--------------------------------------------------------------------
	// Texture Layer Sets and Global Colors
	//--------------------------------------------------------------------	
	LLTexLayerSet*		mHeadLayerSet;
	LLTexLayerSet*		mUpperBodyLayerSet;
	LLTexLayerSet*		mLowerBodyLayerSet;
	LLTexLayerSet*		mEyesLayerSet;
	LLTexLayerSet*		mSkirtLayerSet;


protected:
	LLPointer<LLHUDEffectSpiral> mBeam;
	LLFrameTimer mBeamTimer;
	LLFrameTimer mEditEffectTimer;

	F32		mRenderPriority;
	F32		mAdjustedPixelArea;

	LLWString mNameString;
	LLString  mTitle;
	BOOL	  mNameAway;
	BOOL	  mNameBusy;
	BOOL	  mNameMute;
	BOOL      mNameAppearance;
	BOOL	  mVisibleChat;

	LLString  mDebugText;
	U64		  mLastRegionHandle;
	LLFrameTimer mRegionCrossingTimer;
	S32		  mRegionCrossingCount;
	
	//--------------------------------------------------------------------
	// local textures for compositing.
	//--------------------------------------------------------------------

	LLPointer<LLViewerImage> mLocalTexture[ LOCTEX_NUM_ENTRIES ];
	BOOL 				mLocalTextureBaked[ LOCTEX_NUM_ENTRIES ]; // Texture is covered by a baked texture
	S32 				mLocalTextureDiscard[ LOCTEX_NUM_ENTRIES ];
	LLUUID				mSavedTE[ TEX_NUM_ENTRIES ];
	BOOL				mFirstTEMessageReceived;
	BOOL				mFirstAppearanceMessageReceived;

	BOOL				mHeadBakedLoaded;
	S32					mHeadMaskDiscard;
	BOOL				mUpperBakedLoaded;
	S32					mUpperMaskDiscard;
	BOOL				mLowerBakedLoaded;
	S32					mLowerMaskDiscard;
	BOOL				mEyesBakedLoaded;
	BOOL				mSkirtBakedLoaded;

	//RN: testing 2 pass rendering
	U32					mHeadMaskTexName;
	U32					mUpperMaskTexName;
	U32					mLowerMaskTexName;

	BOOL				mCulled;
	F32					mMinPixelArea; // debug
	F32					mMaxPixelArea; // debug
	
	//--------------------------------------------------------------------
	// Global Colors
	//--------------------------------------------------------------------
	LLTexGlobalColor*	mTexSkinColor;
	LLTexGlobalColor*	mTexHairColor;
	LLTexGlobalColor*	mTexEyeColor;

	static LLVOAvatarSkeletonInfo*	sSkeletonInfo;
	static LLVOAvatarInfo*			sAvatarInfo;
	
protected:

	BOOL			loadSkeletonNode();
	BOOL			loadMeshNodes();
	
	BOOL			isFullyBaked();
	void			deleteLayerSetCaches();
	static BOOL		areAllNearbyInstancesBaked();

	static void		onBakedTextureMasksLoaded(BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);

	void			setLocalTexture(ELocTexIndex i, LLViewerImage* tex, BOOL baked_version_exits);
	
	void			requestLayerSetUpdate(LLVOAvatar::ELocTexIndex i);
	void			addLocalTextureStats(LLVOAvatar::ELocTexIndex i, LLViewerImage* imagep, F32 texel_area_ratio, BOOL rendered, BOOL covered_by_baked);
	void			addBakedTextureStats( LLViewerImage* imagep, F32 pixel_area, F32 texel_area_ratio, S32 boost_level);
	static void		onInitialBakedTextureLoaded( BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata );
	static void		onBakedTextureLoaded(BOOL success, LLViewerImage *src_vi, LLImageRaw* src, LLImageRaw* aux_src, S32 discard_level, BOOL final, void* userdata);
	void			useBakedTexture(const LLUUID& id);
	void			dumpAvatarTEs(const char* context);
	void			removeMissingBakedTextures();
	LLTexLayerSet*	getLayerSet(ETextureIndex index) const;
	LLHost			getObjectHost() const;
	S32				getLocalDiscardLevel( S32 index);
	
//Ventrella
	//-----------------------------------------------------------------------------------------------
	// the Voice Visualizer is responsible for detecting the user's voice signal, and when the
	// user speaks, it puts a voice symbol over the avatar's head, and triggering gesticulations
	//-----------------------------------------------------------------------------------------------
	private:
	LLVoiceVisualizer * mVoiceVisualizer;
	int					mCurrentGesticulationLevel;
//End Ventrella
	
};

#endif // LL_VO_AVATAR_H
