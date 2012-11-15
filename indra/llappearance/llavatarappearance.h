/**
 * @file llavatarappearance.h
 * @brief Declaration of LLAvatarAppearance class
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_AVATAR_APPEARANCE_H
#define LL_AVATAR_APPEARANCE_H

#include "llcharacter.h"
#include "llavatarappearancedefines.h"
#include "llavatarjointmesh.h"
#include "lldriverparam.h"
#include "lltexlayer.h"
#include "llviewervisualparam.h"
#include "llxmltree.h"

class LLTexLayerSet;
class LLTexGlobalColor;
class LLTexGlobalColorInfo;
class LLWearableData;
class LLAvatarBoneInfo;
class LLAvatarSkeletonInfo;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LLAvatarAppearance
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLAvatarAppearance : public LLCharacter
{
	LOG_CLASS(LLAvatarAppearance);

protected:
	struct LLAvatarXmlInfo;

/********************************************************************************
 **                                                                            **
 **                    INITIALIZATION
 **/
private:
	// Hide default constructor.
	LLAvatarAppearance() {}

public:
	LLAvatarAppearance(LLWearableData* wearable_data);
	virtual ~LLAvatarAppearance();

	static void			initClass(); // initializes static members
	static void			cleanupClass();	// Cleanup data that's only init'd once per class.
	virtual void 		initInstance(); // Called after construction to initialize the instance.
	virtual BOOL		loadSkeletonNode();
	BOOL				loadMeshNodes();
	BOOL				loadLayersets();


/**                    Initialization
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    INHERITED
 **/

	//--------------------------------------------------------------------
	// LLCharacter interface and related
	//--------------------------------------------------------------------
public:
	/*virtual*/ LLJoint*		getCharacterJoint(U32 num);

	/*virtual*/ const char*		getAnimationPrefix() { return "avatar"; }
	/*virtual*/ LLVector3		getVolumePos(S32 joint_index, LLVector3& volume_offset);
	/*virtual*/ LLJoint*		findCollisionVolume(U32 volume_id);
	/*virtual*/ S32				getCollisionVolumeID(std::string &name);
	/*virtual*/ LLPolyMesh*		getHeadMesh();
	/*virtual*/ LLPolyMesh*		getUpperBodyMesh();

/**                    Inherited
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    STATE
 **/
public:
	virtual bool 	isSelf() const { return false; } // True if this avatar is for this viewer's agent
	virtual BOOL	isValid() const;
	virtual BOOL	isUsingServerBakes() const = 0;
	virtual BOOL	isUsingLocalAppearance() const = 0;
	virtual BOOL	isEditingAppearance() const = 0;

	bool isBuilt() const { return mIsBuilt; }

	
/**                    State
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    SKELETON
 **/

protected:
	virtual LLAvatarJoint*	createAvatarJoint() = 0;
	virtual LLAvatarJoint*	createAvatarJoint(S32 joint_num) = 0;
	virtual LLAvatarJointMesh*	createAvatarJointMesh() = 0;
public:
	F32					getPelvisToFoot() const { return mPelvisToFoot; }
	/*virtual*/ LLJoint*	getRootJoint() { return mRoot; }

	LLVector3			mHeadOffset; // current head position
	LLAvatarJoint		*mRoot;

	typedef std::map<std::string, LLJoint*> joint_map_t;
	joint_map_t			mJointMap;
	
	void				computeBodySize();


protected:
	static BOOL			parseSkeletonFile(const std::string& filename);
	virtual void		buildCharacter();
	virtual BOOL		loadAvatar();
	virtual void		bodySizeChanged() = 0;

	BOOL				setupBone(const LLAvatarBoneInfo* info, LLJoint* parent, S32 &current_volume_num, S32 &current_joint_num);
	BOOL				allocateCharacterJoints(U32 num);
	BOOL				buildSkeleton(const LLAvatarSkeletonInfo *info);
protected:
	void				clearSkeleton();
	BOOL				mIsBuilt; // state of deferred character building
	typedef std::vector<LLAvatarJoint*> avatar_joint_list_t;
	avatar_joint_list_t	mSkeleton;
	
	//--------------------------------------------------------------------
	// Pelvis height adjustment members.
	//--------------------------------------------------------------------
public:
	LLVector3			mBodySize;
protected:
	F32					mPelvisToFoot;

	//--------------------------------------------------------------------
	// Cached pointers to well known joints
	//--------------------------------------------------------------------
public:
	LLJoint* 		mPelvisp;
	LLJoint* 		mTorsop;
	LLJoint* 		mChestp;
	LLJoint* 		mNeckp;
	LLJoint* 		mHeadp;
	LLJoint* 		mSkullp;
	LLJoint* 		mEyeLeftp;
	LLJoint* 		mEyeRightp;
	LLJoint* 		mHipLeftp;
	LLJoint* 		mHipRightp;
	LLJoint* 		mKneeLeftp;
	LLJoint* 		mKneeRightp;
	LLJoint* 		mAnkleLeftp;
	LLJoint* 		mAnkleRightp;
	LLJoint* 		mFootLeftp;
	LLJoint* 		mFootRightp;
	LLJoint* 		mWristLeftp;
	LLJoint* 		mWristRightp;

	//--------------------------------------------------------------------
	// XML parse tree
	//--------------------------------------------------------------------
protected:
	static LLXmlTree 	sXMLTree; // avatar config file
	static LLXmlTree 	sSkeletonXMLTree; // avatar skeleton file

	static LLAvatarSkeletonInfo* 					sAvatarSkeletonInfo;
	static LLAvatarXmlInfo* 						sAvatarXmlInfo;


/**                    Skeleton
 **                                                                            **
 *******************************************************************************/


/********************************************************************************
 **                                                                            **
 **                    RENDERING
 **/
public:
	BOOL		mIsDummy; // for special views

	//--------------------------------------------------------------------
	// Morph masks
	//--------------------------------------------------------------------
public:
	void 	addMaskedMorph(LLAvatarAppearanceDefines::EBakedTextureIndex index, LLVisualParam* morph_target, BOOL invert, std::string layer);
	virtual void	applyMorphMask(U8* tex_data, S32 width, S32 height, S32 num_components, LLAvatarAppearanceDefines::EBakedTextureIndex index = LLAvatarAppearanceDefines::BAKED_NUM_INDICES) = 0;

/**                    Rendering
 **                                                                            **
 *******************************************************************************/

	//--------------------------------------------------------------------
	// Composites
	//--------------------------------------------------------------------
public:
	virtual void	invalidateComposite(LLTexLayerSet* layerset, BOOL upload_result) = 0;

/********************************************************************************
 **                                                                            **
 **                    MESHES
 **/

public:
	virtual void	updateMeshTextures() = 0;
	virtual void	dirtyMesh() = 0; // Dirty the avatar mesh
protected:
	virtual void	dirtyMesh(S32 priority) = 0; // Dirty the avatar mesh, with priority

protected:
	typedef std::multimap<std::string, LLPolyMesh*> polymesh_map_t;
	polymesh_map_t 									mPolyMeshes;
	avatar_joint_list_t								mMeshLOD;

/**                    Meshes
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    APPEARANCE
 **/

	//--------------------------------------------------------------------
	// Clothing colors (convenience functions to access visual parameters)
	//--------------------------------------------------------------------
public:
	void			setClothesColor(LLAvatarAppearanceDefines::ETextureIndex te, const LLColor4& new_color, BOOL upload_bake);
	LLColor4		getClothesColor(LLAvatarAppearanceDefines::ETextureIndex te);
	static BOOL		teToColorParams(LLAvatarAppearanceDefines::ETextureIndex te, U32 *param_name);

	//--------------------------------------------------------------------
	// Global colors
	//--------------------------------------------------------------------
public:
	LLColor4		getGlobalColor(const std::string& color_name ) const;
	virtual void	onGlobalColorChanged(const LLTexGlobalColor* global_color, BOOL upload_bake) = 0;
protected:
	LLTexGlobalColor* mTexSkinColor;
	LLTexGlobalColor* mTexHairColor;
	LLTexGlobalColor* mTexEyeColor;

	//--------------------------------------------------------------------
	// Visibility
	//--------------------------------------------------------------------
public:
	static LLColor4 getDummyColor();
/**                    Appearance
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    WEARABLES
 **/

public:
	LLWearableData*			getWearableData() { return mWearableData; }
	const LLWearableData*	getWearableData() const { return mWearableData; }
	virtual BOOL isTextureDefined(LLAvatarAppearanceDefines::ETextureIndex te, U32 index = 0 ) const = 0;
	virtual BOOL			isWearingWearableType(LLWearableType::EType type ) const;

private:
	LLWearableData* mWearableData;

/********************************************************************************
 **                                                                            **
 **                    BAKED TEXTURES
 **/
public:
	LLTexLayerSet*		getAvatarLayerSet(LLAvatarAppearanceDefines::EBakedTextureIndex baked_index) const;

protected:
	virtual LLTexLayerSet*	createTexLayerSet() = 0;

protected:
	class LLMaskedMorph;
	typedef std::deque<LLMaskedMorph *> 	morph_list_t;
	struct BakedTextureData
	{
		LLUUID								mLastTextureID;
		LLTexLayerSet*		 				mTexLayerSet; // Only exists for self
		bool								mIsLoaded;
		bool								mIsUsed;
		LLAvatarAppearanceDefines::ETextureIndex 	mTextureIndex;
		U32									mMaskTexName;
		// Stores pointers to the joint meshes that this baked texture deals with
		avatar_joint_mesh_list_t			mJointMeshes;
		morph_list_t						mMaskedMorphs;
	};
	typedef std::vector<BakedTextureData> 	bakedtexturedata_vec_t;
	bakedtexturedata_vec_t 					mBakedTextureDatas;

/********************************************************************************
 **                                                                            **
 **                    PHYSICS
 **/

	//--------------------------------------------------------------------
	// Collision volumes
	//--------------------------------------------------------------------
public:
  	S32			mNumCollisionVolumes;
	LLAvatarJointCollisionVolume* mCollisionVolumes;
protected:
	BOOL		allocateCollisionVolumes(U32 num);

/**                    Physics
 **                                                                            **
 *******************************************************************************/

/********************************************************************************
 **                                                                            **
 **                    SUPPORT CLASSES
 **/

	struct LLAvatarXmlInfo
	{
		LLAvatarXmlInfo();
		~LLAvatarXmlInfo();

		BOOL 	parseXmlSkeletonNode(LLXmlTreeNode* root);
		BOOL 	parseXmlMeshNodes(LLXmlTreeNode* root);
		BOOL 	parseXmlColorNodes(LLXmlTreeNode* root);
		BOOL 	parseXmlLayerNodes(LLXmlTreeNode* root);
		BOOL 	parseXmlDriverNodes(LLXmlTreeNode* root);
		BOOL	parseXmlMorphNodes(LLXmlTreeNode* root);

		struct LLAvatarMeshInfo
		{
			typedef std::pair<LLViewerVisualParamInfo*,BOOL> morph_info_pair_t; // LLPolyMorphTargetInfo stored here
			typedef std::vector<morph_info_pair_t> morph_info_list_t;

			LLAvatarMeshInfo() : mLOD(0), mMinPixelArea(.1f) {}
			~LLAvatarMeshInfo()
			{
				morph_info_list_t::iterator iter;
				for (iter = mPolyMorphTargetInfoList.begin(); iter != mPolyMorphTargetInfoList.end(); iter++)
				{
					delete iter->first;
				}
				mPolyMorphTargetInfoList.clear();
			}

			std::string mType;
			S32			mLOD;
			std::string	mMeshFileName;
			std::string	mReferenceMeshName;
			F32			mMinPixelArea;
			morph_info_list_t mPolyMorphTargetInfoList;
		};
		typedef std::vector<LLAvatarMeshInfo*> mesh_info_list_t;
		mesh_info_list_t mMeshInfoList;

		typedef std::vector<LLViewerVisualParamInfo*> skeletal_distortion_info_list_t; // LLPolySkeletalDistortionInfo stored here
		skeletal_distortion_info_list_t mSkeletalDistortionInfoList;

		struct LLAvatarAttachmentInfo
		{
			LLAvatarAttachmentInfo()
				: mGroup(-1), mAttachmentID(-1), mPieMenuSlice(-1), mVisibleFirstPerson(FALSE),
				  mIsHUDAttachment(FALSE), mHasPosition(FALSE), mHasRotation(FALSE) {}
			std::string mName;
			std::string mJointName;
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
		typedef std::vector<LLAvatarAttachmentInfo*> attachment_info_list_t;
		attachment_info_list_t mAttachmentInfoList;

		LLTexGlobalColorInfo *mTexSkinColorInfo;
		LLTexGlobalColorInfo *mTexHairColorInfo;
		LLTexGlobalColorInfo *mTexEyeColorInfo;

		typedef std::vector<LLTexLayerSetInfo*> layer_info_list_t;
		layer_info_list_t mLayerInfoList;

		typedef std::vector<LLDriverParamInfo*> driver_info_list_t;
		driver_info_list_t mDriverInfoList;

		struct LLAvatarMorphInfo
		{
			LLAvatarMorphInfo()
				: mInvert(FALSE) {}
			std::string mName;
			std::string mRegion;
			std::string mLayer;
			BOOL mInvert;
		};

		typedef std::vector<LLAvatarMorphInfo*> morph_info_list_t;
		morph_info_list_t	mMorphMaskInfoList;
	};


	class LLMaskedMorph
	{
	public:
		LLMaskedMorph(LLVisualParam *morph_target, BOOL invert, std::string layer);

		LLVisualParam	*mMorphTarget;
		BOOL				mInvert;
		std::string			mLayer;
	};
/**                    Support Classes
 **                                                                            **
 *******************************************************************************/
};

#endif // LL_AVATAR_APPEARANCE_H
