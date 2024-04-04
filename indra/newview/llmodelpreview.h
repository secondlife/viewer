/**
 * @file llmodelpreview.h
 * @brief LLModelPreview class definition, class
 * responsible for model preview inside LLFloaterModelPreview
 *
 * $LicenseInfo:firstyear=2020&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2020, Linden Research, Inc.
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

#ifndef LL_LLMODELPREVIEW_H
#define LL_LLMODELPREVIEW_H

#include "lldynamictexture.h"
#include "llfloatermodelpreview.h"
#include "llmeshrepository.h"
#include "llmodelloader.h" //NUM_LOD
#include "llmodel.h"

class LLJoint;
class LLVOAvatar;
class LLTextBox;
class LLVertexBuffer;
class DAE;
class daeElement;
class domProfile_COMMON;
class domInstance_geometry;
class domNode;
class domTranslate;
class domController;
class domSkin;
class domMesh;

// const strings needed by classes that use model preivew
static const std::string lod_name[NUM_LOD + 1] =
{
    "lowest",
    "low",
    "medium",
    "high",
    "I went off the end of the lod_name array.  Me so smart."
};

static const std::string lod_triangles_name[NUM_LOD + 1] =
{
    "lowest_triangles",
    "low_triangles",
    "medium_triangles",
    "high_triangles",
    "I went off the end of the lod_triangles_name array.  Me so smart."
};

static const std::string lod_vertices_name[NUM_LOD + 1] =
{
    "lowest_vertices",
    "low_vertices",
    "medium_vertices",
    "high_vertices",
    "I went off the end of the lod_vertices_name array.  Me so smart."
};

static const std::string lod_status_name[NUM_LOD + 1] =
{
    "lowest_status",
    "low_status",
    "medium_status",
    "high_status",
    "I went off the end of the lod_status_name array.  Me so smart."
};

static const std::string lod_icon_name[NUM_LOD + 1] =
{
    "status_icon_lowest",
    "status_icon_low",
    "status_icon_medium",
    "status_icon_high",
    "I went off the end of the lod_status_name array.  Me so smart."
};

static const std::string lod_status_image[NUM_LOD + 1] =
{
    "ModelImport_Status_Good",
    "ModelImport_Status_Warning",
    "ModelImport_Status_Error",
    "I went off the end of the lod_status_image array.  Me so smart."
};

static const std::string lod_label_name[NUM_LOD + 1] =
{
    "lowest_label",
    "low_label",
    "medium_label",
    "high_label",
    "I went off the end of the lod_label_name array.  Me so smart."
};

class LLModelPreview : public LLViewerDynamicTexture, public LLMutex
{
    LOG_CLASS(LLModelPreview);

    typedef boost::signals2::signal<void(F32 x, F32 y, F32 z)> details_signal_t;
    typedef boost::signals2::signal<void(void)> model_loaded_signal_t;
    typedef boost::signals2::signal<void(bool)> model_updated_signal_t;

public:

    typedef enum
    {
        LOD_FROM_FILE = 0,
        MESH_OPTIMIZER_AUTO, // automatically selects method based on model or face
        MESH_OPTIMIZER_PRECISE, // combines faces into a single model, simplifies, then splits back into faces
        MESH_OPTIMIZER_SLOPPY, // uses sloppy method, works per face
        USE_LOD_ABOVE,
    } eLoDMode;

    typedef enum
    {
        LIMIT_TRIANGLES = 0,
        LIMIT_ERROR_TRESHOLD,
    } eLoDLimit;

public:
    // Todo: model preview shouldn't need floater dependency, it
    // should just expose data to floater, not control flaoter like it does
    LLModelPreview(S32 width, S32 height, LLFloater* fmp);
    virtual ~LLModelPreview();

    void resetPreviewTarget();
    void setPreviewTarget(F32 distance);
    void setTexture(U32 name) { mTextureName = name; }

    void setPhysicsFromLOD(S32 lod);
    bool render();
    void update();
    void genBuffers(S32 lod, bool skinned);
    void clearBuffers();
    void refresh();
    void rotate(F32 yaw_radians, F32 pitch_radians);
    void zoom(F32 zoom_amt);
    void pan(F32 right, F32 up);
    virtual bool needsRender() { return mNeedsUpdate; }
    void setPreviewLOD(S32 lod);
    void clearModel(S32 lod);
    void getJointAliases(JointMap& joint_map);
    void loadModel(std::string filename, S32 lod, bool force_disable_slm = false);
    void loadModelCallback(S32 lod);
    bool lodsReady() { return !mGenLOD && mLodsQuery.empty(); }
    void queryLODs() { mGenLOD = true; };
    void genMeshOptimizerLODs(S32 which_lod, S32 meshopt_mode, U32 decimation = 3, bool enforce_tri_limit = false);
    void generateNormals();
    void restoreNormals();
    void updateDimentionsAndOffsets();
    void rebuildUploadData();
    void saveUploadData(bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position);
    void saveUploadData(const std::string& filename, bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position);
    void clearIncompatible(S32 lod);
    void updateStatusMessages();
    void updateLodControls(S32 lod);
    void onLODMeshOptimizerParamCommit(S32 lod, bool enforce_tri_limit, S32 mode);
    void addEmptyFace(LLModel* pTarget);

    const bool getModelPivot(void) const { return mHasPivot; }
    void setHasPivot(bool val) { mHasPivot = val; }
    void setModelPivot(const LLVector3& pivot) { mModelPivot = pivot; }

    //Is a rig valid so that it can be used as a criteria for allowing for uploading of joint positions
    //Accessors for joint position upload friendly rigs
    const bool isRigValidForJointPositionUpload(void) const { return mRigValidJointUpload; }
    void setRigValidForJointPositionUpload(bool rigValid) { mRigValidJointUpload = rigValid; }

    //Accessors for the legacy rigs
    const bool isLegacyRigValid(void) const { return mLegacyRigFlags == 0; }
    U32 getLegacyRigFlags() const { return mLegacyRigFlags; }
    void setLegacyRigFlags(U32 rigFlags) { mLegacyRigFlags = rigFlags; }

    static void	textureLoadedCallback(bool success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, bool final, void* userdata);
    static bool lodQueryCallback();

    boost::signals2::connection setDetailsCallback(const details_signal_t::slot_type& cb){ return mDetailsSignal.connect(cb); }
    boost::signals2::connection setModelLoadedCallback(const model_loaded_signal_t::slot_type& cb){ return mModelLoadedSignal.connect(cb); }
    boost::signals2::connection setModelUpdatedCallback(const model_updated_signal_t::slot_type& cb){ return mModelUpdatedSignal.connect(cb); }

    void setLoadState(U32 state) { mLoadState = state; }
    U32 getLoadState() { return mLoadState; }

    static bool 		sIgnoreLoadedCallback;
    std::vector<S32> mLodsQuery;
    std::vector<S32> mLodsWithParsingError;
    bool mHasDegenerate;

protected:

    static void			loadedCallback(LLModelLoader::scene& scene, LLModelLoader::model_list& model_list, S32 lod, void* opaque);
    static void			stateChangedCallback(U32 state, void* opaque);

    static LLJoint*	lookupJointByName(const std::string&, void* opaque);
    static U32			loadTextures(LLImportMaterial& material, void* opaque);

    void lookupLODModelFiles(S32 lod);

private:
    //Utility function for controller vertex compare
    bool verifyCount(int expected, int result);
    //Creates the dummy avatar for the preview window
    void		createPreviewAvatar(void);
    //Accessor for the dummy avatar
    LLVOAvatar* getPreviewAvatar(void) { return mPreviewAvatar; }
    // Count amount of original models, excluding sub-models
    static U32 countRootModels(LLModelLoader::model_list models);
    LLVector3   mGroundPlane[4];
	void		renderGroundPlane(float z_offset = 0.0f);
    /// Indicates whether we should warn of high-lod meshes that do not have a corresponding physics mesh.
    /// Reset when resetting the modelpreview (i.e., when the uploader dialog is created or reset), and when
    /// about to process a physics file. Set to true immediately after the file is loaded (before rebuildUploadData()).
    ///
    /// (The rules for mapping the correspondence of high-lod meshes to physics meshes are complex. When
    /// lod rendering meshes are used, there is never an unmatched mesh. Nor is there a mismatch when
    /// the high-lod file and physics file have ony one mesh each. In these cases, this value is moot.
    /// When there are multiple meshes in each file, they are matched by name or order, and some meshes
    /// are broken up by limitations into multiple objects, and thus there can be mismatches.)
    bool mWarnOfUnmatchedPhyicsMeshes{false};
    /// A mesh to use as the default physics shape in only those cases where the physics shape is not otherwise specified.
    /// It is set only when the user chooses a physics shape file that contains a mesh with a name that matches DEFAULT_PHYSICS_MESH_NAME.
    /// It is reset when such a name is not found, and when resetting the modelpreview.
    /// Not read unless mWarnOfUnmatchedPhyicsMeshes is true.
    LLModel* mDefaultPhysicsShapeP{};

    typedef enum
    {
        MESH_OPTIMIZER_FULL,
        MESH_OPTIMIZER_NO_NORMALS,
        MESH_OPTIMIZER_NO_UVS,
        MESH_OPTIMIZER_NO_TOPOLOGY,
    } eSimplificationMode;

    // Merges faces into single mesh, simplifies using mesh optimizer,
    // then splits back into faces.
    // Returns reached simplification ratio. -1 in case of a failure.
    F32 genMeshOptimizerPerModel(LLModel *base_model, LLModel *target_model, F32 indices_ratio, F32 error_threshold, eSimplificationMode simplification_mode);
    // Simplifies specified face using mesh optimizer.
    // Returns reached simplification ratio. -1 in case of a failure.
    F32 genMeshOptimizerPerFace(LLModel *base_model, LLModel *target_model, U32 face_idx, F32 indices_ratio, F32 error_threshold, eSimplificationMode simplification_mode);

protected:
    friend class LLModelLoader;
    friend class LLFloaterModelPreview;
    friend class LLFloaterModelPreview::DecompRequest;
    friend class LLPhysicsDecomp;

    LLFloater*  mFMP;

    bool        mNeedsUpdate;
    bool		mDirty;
    bool		mGenLOD;
    U32         mTextureName;
    F32			mCameraDistance;
    F32			mCameraYaw;
    F32			mCameraPitch;
    F32			mCameraZoom;
    LLVector3	mCameraOffset;
    LLVector3	mPreviewTarget;
    LLVector3	mPreviewScale;
    S32			mPreviewLOD;
    S32			mPhysicsSearchLOD;
    std::string mLODFile[LLModel::NUM_LODS];
    bool		mLoading;
    U32			mLoadState;
    bool		mResetJoints;
    bool		mModelNoErrors;
    bool		mLookUpLodFiles;

    std::map<std::string, bool> mViewOption;

    // Model generation parameters (must rebuild object if these change)
    bool mLODFrozen;
    U32 mRequestedLoDMode[LLModel::NUM_LODS];
    S32 mRequestedTriangleCount[LLModel::NUM_LODS];
    F32 mRequestedErrorThreshold[LLModel::NUM_LODS];
    F32 mRequestedCreaseAngle[LLModel::NUM_LODS];

    LLModelLoader* mModelLoader;

    LLModelLoader::scene mScene[LLModel::NUM_LODS];
    LLModelLoader::scene mBaseScene;

    LLModelLoader::model_list mModel[LLModel::NUM_LODS];
    LLModelLoader::model_list mBaseModel;

    typedef std::vector<LLVolumeFace>		v_LLVolumeFace_t;
    typedef std::vector<v_LLVolumeFace_t>	vv_LLVolumeFace_t;

    vv_LLVolumeFace_t mModelFacesCopy[LLModel::NUM_LODS];
    vv_LLVolumeFace_t mBaseModelFacesCopy;

    U32 mGroup;
    std::map<LLPointer<LLModel>, U32> mObject;

    // Amount of triangles in original(base) model
    U32 mMaxTriangleLimit;

    LLMeshUploadThread::instance_list mUploadData;
    std::set<LLViewerFetchedTexture * > mTextureSet;

    //map of vertex buffers to models (one vertex buffer in vector per face in model
    std::map<LLModel*, std::vector<LLPointer<LLVertexBuffer> > > mVertexBuffer[LLModel::NUM_LODS + 1];

    details_signal_t mDetailsSignal;
    model_loaded_signal_t mModelLoadedSignal;
    model_updated_signal_t mModelUpdatedSignal;

    LLVector3	mModelPivot;
    bool		mHasPivot;

    float		mPelvisZOffset;

    bool		mRigValidJointUpload;
    U32			mLegacyRigFlags;

    bool		mLastJointUpdate;
    bool		mFirstSkinUpdate;

    JointNameSet		mJointsFromNode;
    JointTransformMap	mJointTransformMap;

    LLPointer<LLVOAvatar>	mPreviewAvatar;
    LLCachedControl<bool>	mImporterDebug;
};

#endif  // LL_LLMODELPREVIEW_H
