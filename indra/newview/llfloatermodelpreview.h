/**
 * @file llfloatermodelpreview.h
 * @brief LLFloaterModelPreview class definition
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERMODELPREVIEW_H
#define LL_LLFLOATERMODELPREVIEW_H

#include "llfloaternamedesc.h"

#include "lldynamictexture.h"
#include "llquaternion.h"
#include "llmeshrepository.h"
#include "llmodel.h"
#include "llthread.h"
#include "llviewermenufile.h"
#include "llfloatermodeluploadbase.h"

#include "lldaeloader.h"

class LLComboBox;
class LLJoint;
class LLViewerJointMesh;
class LLVOAvatar;
class LLTextBox;
class LLVertexBuffer;
class LLModelPreview;
class LLFloaterModelPreview;
class DAE;
class daeElement;
class domProfile_COMMON;
class domInstance_geometry;
class domNode;
class domTranslate;
class domController;
class domSkin;
class domMesh;
class LLMenuButton;
class LLToggleableMenu;

class LLFloaterModelPreview : public LLFloaterModelUploadBase
{
public:
	
	class DecompRequest : public LLPhysicsDecomp::Request
	{
	public:
		S32 mContinue;
		LLPointer<LLModel> mModel;
		
		DecompRequest(const std::string& stage, LLModel* mdl);
		virtual S32 statusCallback(const char* status, S32 p1, S32 p2);
		virtual void completed();
		
	};
	static LLFloaterModelPreview* sInstance;
	
	LLFloaterModelPreview(const LLSD& key);
	virtual ~LLFloaterModelPreview();
	
	virtual BOOL postBuild();
	
	void initModelPreview();

	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks); 
	
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);

	static void onMouseCaptureLostModelPreview(LLMouseHandler*);
	static void setUploadAmount(S32 amount) { sUploadAmount = amount; }

	void setDetails(F32 x, F32 y, F32 z, F32 streaming_cost, F32 physics_cost);
	void setPreviewLOD(S32 lod);
	
	void onBrowseLOD(S32 lod);
	
	static void onReset(void* data);

	static void onUpload(void* data);
	
	void refresh();
	
	void			loadModel(S32 lod);
	void 			loadModel(S32 lod, const std::string& file_name, bool force_disable_slm = false);
	
	void onViewOptionChecked(LLUICtrl* ctrl);
	bool isViewOptionChecked(const LLSD& userdata);
	bool isViewOptionEnabled(const LLSD& userdata);
	void setViewOptionEnabled(const std::string& option, bool enabled);
	void enableViewOption(const std::string& option);
	void disableViewOption(const std::string& option);

	bool isModelLoading();

	// shows warning message if agent has no permissions to upload model
	/*virtual*/ void onPermissionsReceived(const LLSD& result);

	// called when error occurs during permissions request
	/*virtual*/ void setPermissonsErrorStatus(S32 status, const std::string& reason);

	/*virtual*/ void onModelPhysicsFeeReceived(const LLSD& result, std::string upload_url);
				void handleModelPhysicsFeeReceived();
	/*virtual*/ void setModelPhysicsFeeErrorStatus(S32 status, const std::string& reason);

	/*virtual*/ void onModelUploadSuccess();

	/*virtual*/ void onModelUploadFailure();

	bool isModelUploadAllowed();

protected:
	friend class LLModelPreview;
	friend class LLMeshFilePicker;
	friend class LLPhysicsDecomp;
	
	static void		onImportScaleCommit(LLUICtrl*, void*);
	static void		onPelvisOffsetCommit(LLUICtrl*, void*);
	static void		onUploadJointsCommit(LLUICtrl*,void*);
	static void		onUploadSkinCommit(LLUICtrl*,void*);

	static void		onPreviewLODCommit(LLUICtrl*,void*);
	
	static void		onGenerateNormalsCommit(LLUICtrl*,void*);
	
	void toggleGenarateNormals();

	static void		onAutoFillCommit(LLUICtrl*,void*);
	
	void onLODParamCommit(S32 lod, bool enforce_tri_limit);

	static void		onExplodeCommit(LLUICtrl*, void*);
	
	static void onPhysicsParamCommit(LLUICtrl* ctrl, void* userdata);
	static void onPhysicsStageExecute(LLUICtrl* ctrl, void* userdata);
	static void onCancel(LLUICtrl* ctrl, void* userdata);
	static void onPhysicsStageCancel(LLUICtrl* ctrl, void* userdata);
	
	static void onPhysicsBrowse(LLUICtrl* ctrl, void* userdata);
	static void onPhysicsUseLOD(LLUICtrl* ctrl, void* userdata);
	static void onPhysicsOptimize(LLUICtrl* ctrl, void* userdata);
	static void onPhysicsDecomposeBack(LLUICtrl* ctrl, void* userdata);
	static void onPhysicsSimplifyBack(LLUICtrl* ctrl, void* userdata);
		
	void			draw();
	
	void initDecompControls();
	
	void setStatusMessage(const std::string& msg);

	LLModelPreview*	mModelPreview;
	
	LLPhysicsDecomp::decomp_params mDecompParams;
	
	S32				mLastMouseX;
	S32				mLastMouseY;
	LLRect			mPreviewRect;
	static S32		sUploadAmount;
	
	std::set<LLPointer<DecompRequest> > mCurRequest;
	std::string mStatusMessage;

	//use "disabled" as false by default
	std::map<std::string, bool> mViewOptionDisabled;
	
	//store which lod mode each LOD is using
	// 0 - load from file
	// 1 - auto generate
	// 2 - use LoD above
	S32 mLODMode[4];

	LLMutex* mStatusLock;

	LLSD mModelPhysicsFee;

private:
	void onClickCalculateBtn();
	void toggleCalculateButton();

	void onLoDSourceCommit(S32 lod);

	// Toggles between "Calculate weights & fee" and "Upload" buttons.
	void toggleCalculateButton(bool visible);

	// resets display options of model preview to their defaults.
	void resetDisplayOptions();

	void createSmoothComboBox(LLComboBox* combo_box, float min, float max);

	LLButton* mUploadBtn;
	LLButton* mCalculateBtn;
};

class LLMeshFilePicker : public LLFilePickerThread
{
public:
	LLMeshFilePicker(LLModelPreview* mp, S32 lod);
	virtual void notify(const std::string& filename);

private:
	LLModelPreview* mMP;
	S32 mLOD;
};


class LLModelPreview : public LLViewerDynamicTexture, public LLMutex
{	
	typedef boost::signals2::signal<void (F32 x, F32 y, F32 z, F32 streaming_cost, F32 physics_cost)> details_signal_t;
	typedef boost::signals2::signal<void (void)> model_loaded_signal_t;
	typedef boost::signals2::signal<void (bool)> model_updated_signal_t;

public:

	typedef enum
	{
		LOD_FROM_FILE = 0,
		GENERATE,
		USE_LOD_ABOVE,
	} eLoDMode;

public:
	LLModelPreview(S32 width, S32 height, LLFloater* fmp);
	virtual ~LLModelPreview();

	void resetPreviewTarget();
	void setPreviewTarget(F32 distance);
	void setTexture(U32 name) { mTextureName = name; }

	void setPhysicsFromLOD(S32 lod);
	BOOL render();
	void update();
	void genBuffers(S32 lod, bool skinned);
	void clearBuffers();
	void refresh();
	void rotate(F32 yaw_radians, F32 pitch_radians);
	void zoom(F32 zoom_amt);
	void pan(F32 right, F32 up);
	virtual BOOL needsRender() { return mNeedsUpdate; }
	void setPreviewLOD(S32 lod);
	void clearModel(S32 lod);
    void getJointAliases(JointMap& joint_map);
	void loadModel(std::string filename, S32 lod, bool force_disable_slm = false);
	void loadModelCallback(S32 lod);
    bool lodsReady() { return !mGenLOD && mLodsQuery.empty(); }
    void queryLODs() { mGenLOD = true; };
	void genLODs(S32 which_lod = -1, U32 decimation = 3, bool enforce_tri_limit = false);
	void generateNormals();
	void restoreNormals();
	U32 calcResourceCost();
	void rebuildUploadData();
	void saveUploadData(bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position);
	void saveUploadData(const std::string& filename, bool save_skinweights, bool save_joint_positions, bool lock_scale_if_joint_position);
	void clearIncompatible(S32 lod);
	void updateStatusMessages();
	void updateLodControls(S32 lod);
	void clearGLODGroup();
	void onLODParamCommit(S32 lod, bool enforce_tri_limit);
	void addEmptyFace( LLModel* pTarget );
	
	const bool getModelPivot( void ) const { return mHasPivot; }
	void setHasPivot( bool val ) { mHasPivot = val; }
	void setModelPivot( const LLVector3& pivot ) { mModelPivot = pivot; }

	//Is a rig valid so that it can be used as a criteria for allowing for uploading of joint positions
	//Accessors for joint position upload friendly rigs
	const bool isRigValidForJointPositionUpload( void ) const { return mRigValidJointUpload; }
	void setRigValidForJointPositionUpload( bool rigValid ) { mRigValidJointUpload = rigValid; }

	//Accessors for the legacy rigs
	const bool isLegacyRigValid( void ) const { return mLegacyRigValid; }
	void setLegacyRigValid( bool rigValid ) { mLegacyRigValid = rigValid; }		

	static void	textureLoadedCallback( BOOL success, LLViewerFetchedTexture *src_vi, LLImageRaw* src, LLImageRaw* src_aux, S32 discard_level, BOOL final, void* userdata );
    static bool lodQueryCallback();
	
	boost::signals2::connection setDetailsCallback( const details_signal_t::slot_type& cb ){  return mDetailsSignal.connect(cb);  }
	boost::signals2::connection setModelLoadedCallback( const model_loaded_signal_t::slot_type& cb ){  return mModelLoadedSignal.connect(cb);  }
	boost::signals2::connection setModelUpdatedCallback( const model_updated_signal_t::slot_type& cb ){  return mModelUpdatedSignal.connect(cb);  }
	
	void setLoadState( U32 state ) { mLoadState = state; }
	U32 getLoadState() { return mLoadState; }
	
	static bool 		sIgnoreLoadedCallback;
    std::vector<S32> mLodsQuery;
    std::vector<S32> mLodsWithParsingError;

protected:

	static void			loadedCallback(LLModelLoader::scene& scene,LLModelLoader::model_list& model_list, S32 lod, void* opaque);
	static void			stateChangedCallback(U32 state, void* opaque);

	static LLJoint*	lookupJointByName(const std::string&, void* opaque);
	static U32			loadTextures(LLImportMaterial& material, void* opaque);

private:
	//Utility function for controller vertex compare
	bool verifyCount( int expected, int result );
	//Creates the dummy avatar for the preview window
	void		createPreviewAvatar( void );
	//Accessor for the dummy avatar
	LLVOAvatar* getPreviewAvatar( void ) { return mPreviewAvatar; }
	// Count amount of original models, excluding sub-models
	static U32 countRootModels(LLModelLoader::model_list models);

 protected:
	friend class LLModelLoader;
	friend class LLFloaterModelPreview;
	friend class LLFloaterModelPreview::DecompRequest;
	friend class LLPhysicsDecomp;

	LLFloater*  mFMP;

	BOOL        mNeedsUpdate;
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
	U32			mResourceCost;
	std::string mLODFile[LLModel::NUM_LODS];
	bool		mLoading;
	U32			mLoadState;
	bool		mResetJoints;
	bool		mModelNoErrors;

	std::map<std::string, bool> mViewOption;

	//GLOD object parameters (must rebuild object if these change)
	bool mLODFrozen;
	F32 mBuildShareTolerance;
	U32 mBuildQueueMode;
	U32 mBuildOperator;
	U32 mBuildBorderMode;
	U32 mRequestedLoDMode[LLModel::NUM_LODS];
	S32 mRequestedTriangleCount[LLModel::NUM_LODS];
	F32 mRequestedErrorThreshold[LLModel::NUM_LODS];
	U32 mRequestedBuildOperator[LLModel::NUM_LODS];
	U32 mRequestedQueueMode[LLModel::NUM_LODS];
	U32 mRequestedBorderMode[LLModel::NUM_LODS];
	F32 mRequestedShareTolerance[LLModel::NUM_LODS];
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
	U32 mMaxTriangleLimit;
	
	LLMeshUploadThread::instance_list mUploadData;
	std::set<LLViewerFetchedTexture * > mTextureSet;

	//map of vertex buffers to models (one vertex buffer in vector per face in model
	std::map<LLModel*, std::vector<LLPointer<LLVertexBuffer> > > mVertexBuffer[LLModel::NUM_LODS+1];

	details_signal_t mDetailsSignal;
	model_loaded_signal_t mModelLoadedSignal;
	model_updated_signal_t mModelUpdatedSignal;
	
	LLVector3	mModelPivot;
	bool		mHasPivot;
	
	float		mPelvisZOffset;
	
	bool		mRigValidJointUpload;
	bool		mLegacyRigValid;

	bool		mLastJointUpdate;

	JointNameSet		mJointsFromNode;
	JointTransformMap	mJointTransformMap;

	LLPointer<LLVOAvatar>	mPreviewAvatar;
};

#endif  // LL_LLFLOATERMODELPREVIEW_H
