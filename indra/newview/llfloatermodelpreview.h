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
#include "llfloatermodeluploadbase.h"
#include "llmeshrepository.h"

class LLComboBox;
class LLJoint;
class LLMeshFilePicker;
class LLModelPreview;
class LLTabContainer;
class LLViewerTextEditor;


class LLJointOverrideData
{
public:
    LLJointOverrideData() : mHasConflicts(false) {};
    std::map<std::string, LLVector3> mPosOverrides; // models with overrides
    std::set<std::string> mModelsNoOverrides; // models without defined overrides
    bool mHasConflicts;
};
typedef std::map<std::string, LLJointOverrideData> joint_override_data_map_t;

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
    /*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	
	void initModelPreview();

	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	BOOL handleScrollWheel(S32 x, S32 y, S32 clicks); 
	
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);

	static void onMouseCaptureLostModelPreview(LLMouseHandler*);
	static void setUploadAmount(S32 amount) { sUploadAmount = amount; }
	static void addStringToLog(const std::string& message, const LLSD& args, bool flash, S32 lod = -1);
	static void addStringToLog(const std::string& str, bool flash);
	static void addStringToLog(const std::ostringstream& strm, bool flash);
	void clearAvatarTab(); // clears table
	void updateAvatarTab(bool highlight_overrides); // populates table and data as nessesary

	void setDetails(F32 x, F32 y, F32 z, F32 streaming_cost, F32 physics_cost);
	void setPreviewLOD(S32 lod);
	
	void onBrowseLOD(S32 lod);
	
	static void onReset(void* data);

	static void onUpload(void* data);
	
	void refresh();
	
	void			loadModel(S32 lod);
	void 			loadModel(S32 lod, const std::string& file_name, bool force_disable_slm = false);

	void			loadHighLodModel();
	
	void onViewOptionChecked(LLUICtrl* ctrl);
	void onUploadOptionChecked(LLUICtrl* ctrl);
	bool isViewOptionChecked(const LLSD& userdata);
	bool isViewOptionEnabled(const LLSD& userdata);
	void setViewOptionEnabled(const std::string& option, bool enabled);
	void enableViewOption(const std::string& option);
	void disableViewOption(const std::string& option);
	void onShowSkinWeightChecked(LLUICtrl* ctrl);

	bool isModelLoading();

	// shows warning message if agent has no permissions to upload model
	/*virtual*/ void onPermissionsReceived(const LLSD& result);

	// called when error occurs during permissions request
	/*virtual*/ void setPermissonsErrorStatus(S32 status, const std::string& reason);

	/*virtual*/ void onModelPhysicsFeeReceived(const LLSD& result, std::string upload_url);
				void handleModelPhysicsFeeReceived();
	/*virtual*/ void setModelPhysicsFeeErrorStatus(S32 status, const std::string& reason, const LLSD& result);

	/*virtual*/ void onModelUploadSuccess();

	/*virtual*/ void onModelUploadFailure();

	bool isModelUploadAllowed();

protected:
	friend class LLModelPreview;
	friend class LLMeshFilePicker;
	friend class LLPhysicsDecomp;

	void		onDescriptionKeystroke(LLUICtrl*);

	static void		onImportScaleCommit(LLUICtrl*, void*);
	static void		onPelvisOffsetCommit(LLUICtrl*, void*);

	static void		onPreviewLODCommit(LLUICtrl*,void*);
	
	static void		onGenerateNormalsCommit(LLUICtrl*,void*);
	
	void toggleGenarateNormals();

	static void		onAutoFillCommit(LLUICtrl*,void*);
	
	void onLODParamCommit(S32 lod, bool enforce_tri_limit);
	void draw3dPreview();

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

    // FIXME - this function and mStatusMessage have no visible effect, and the
    // actual status messages are managed by directly manipulation of
    // the UI element.
    void setStatusMessage(const std::string& msg);
    void addStringToLogTab(const std::string& str, bool flash);

    void setCtrlLoadFromFile(S32 lod);

	LLModelPreview*	mModelPreview;
	
	LLPhysicsDecomp::decomp_params mDecompParams;
	LLPhysicsDecomp::decomp_params mDefaultDecompParams;
	
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
    void onJointListSelection();

	void onLoDSourceCommit(S32 lod);

	void modelUpdated(bool calculate_visible);

	// Toggles between "Calculate weights & fee" and "Upload" buttons.
    void toggleCalculateButton();
	void toggleCalculateButton(bool visible);

	// resets display options of model preview to their defaults.
	void resetDisplayOptions();

	void resetUploadOptions();
	void clearLogTab();

	void createSmoothComboBox(LLComboBox* combo_box, float min, float max);

	LLButton* mUploadBtn;
	LLButton* mCalculateBtn;
	LLViewerTextEditor* mUploadLogText;
	LLTabContainer* mTabContainer;

	S32			mAvatarTabIndex; // just to avoid any issues in case of xml changes
	std::string	mSelectedJointName;

	joint_override_data_map_t mJointOverrides[LLModel::NUM_LODS];
};

#endif  // LL_LLFLOATERMODELPREVIEW_H
