/** 
 * @file llfloatersnapshot.h
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2016, Linden Research, Inc.
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

#ifndef LL_LLFLOATERSNAPSHOT_H
#define LL_LLFLOATERSNAPSHOT_H

#include "llagent.h"
#include "llfloater.h"
#include "llpanelsnapshot.h"
#include "llsnapshotmodel.h"

class LLSpinCtrl;
class LLSnapshotLivePreview;

class LLFloaterSnapshotBase : public LLFloater
{
    LOG_CLASS(LLFloaterSnapshotBase);

public:

    LLFloaterSnapshotBase(const LLSD& key);
    virtual ~LLFloaterSnapshotBase();

	/*virtual*/ void draw();
	/*virtual*/ void onClose(bool app_quitting);
	virtual S32 notify(const LLSD& info);

	// TODO: create a snapshot model instead
	virtual void saveTexture() = 0;
	void postSave();
	virtual void postPanelSwitch();
	LLPointer<LLImageFormatted> getImageData();
	LLSnapshotLivePreview* getPreviewView();
	const LLVector3d& getPosTakenGlobal();

	const LLRect& getThumbnailPlaceholderRect() { return mThumbnailPlaceholder->getRect(); }

	void setRefreshLabelVisible(bool value) { mRefreshLabel->setVisible(value); }
	void setSuccessLabelPanelVisible(bool value) { mSucceessLblPanel->setVisible(value); }
	void setFailureLabelPanelVisible(bool value) { mFailureLblPanel->setVisible(value); }
	void inventorySaveFailed();

	class ImplBase;
	friend class ImplBase;
	ImplBase* impl;

protected:
	LLUICtrl* mThumbnailPlaceholder;
	LLUICtrl *mRefreshBtn, *mRefreshLabel;
	LLUICtrl *mSucceessLblPanel, *mFailureLblPanel;
};

class LLFloaterSnapshotBase::ImplBase
{
public:
	typedef enum e_status
	{
		STATUS_READY,
		STATUS_WORKING,
		STATUS_FINISHED
	} EStatus;

	ImplBase(LLFloaterSnapshotBase* floater) : mAvatarPauseHandles(),
		mLastToolset(NULL),
		mAspectRatioCheckOff(false),
		mNeedRefresh(false),
		mStatus(STATUS_READY),
		mFloater(floater)
	{}
	virtual ~ImplBase()
	{
		//unpause avatars
		mAvatarPauseHandles.clear();
	}

	static void onClickNewSnapshot(void* data);
	static void onClickAutoSnap(LLUICtrl *ctrl, void* data);
	static void onClickFilter(LLUICtrl *ctrl, void* data);
	static void onClickUICheck(LLUICtrl *ctrl, void* data);
	static void onClickHUDCheck(LLUICtrl *ctrl, void* data);
	static void onCommitFreezeFrame(LLUICtrl* ctrl, void* data);

	virtual LLPanelSnapshot* getActivePanel(LLFloaterSnapshotBase* floater, bool ok_if_not_found = true) = 0;
	virtual LLSnapshotModel::ESnapshotType getActiveSnapshotType(LLFloaterSnapshotBase* floater);
	virtual LLSnapshotModel::ESnapshotFormat getImageFormat(LLFloaterSnapshotBase* floater) = 0;
	virtual std::string getSnapshotPanelPrefix() = 0;

	LLSnapshotLivePreview* getPreviewView();
	virtual void updateControls(LLFloaterSnapshotBase* floater) = 0;
	virtual void updateLayout(LLFloaterSnapshotBase* floater);
	virtual void updateLivePreview();
	virtual void setStatus(EStatus status, bool ok = true, const std::string& msg = LLStringUtil::null);
	virtual EStatus getStatus() const { return mStatus; }
	virtual void setNeedRefresh(bool need);

	static BOOL updatePreviewList(bool initialized);

	void setAdvanced(bool advanced) { mAdvanced = advanced; }

	virtual LLSnapshotModel::ESnapshotLayerType getLayerType(LLFloaterSnapshotBase* floater) = 0;
	virtual void checkAutoSnapshot(LLSnapshotLivePreview* floater, BOOL update_thumbnail = FALSE);
	void setWorking(bool working);
	virtual void setFinished(bool finished, bool ok = true, const std::string& msg = LLStringUtil::null) = 0;

public:
	LLFloaterSnapshotBase* mFloater;
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;

	LLToolset*	mLastToolset;
	LLHandle<LLView> mPreviewHandle;
	bool mAspectRatioCheckOff;
	bool mNeedRefresh;
	bool mAdvanced;
	EStatus mStatus;
};

class LLFloaterSnapshot : public LLFloaterSnapshotBase
{
	LOG_CLASS(LLFloaterSnapshot);

public:
	LLFloaterSnapshot(const LLSD& key);
	/*virtual*/ ~LLFloaterSnapshot();
    
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ S32 notify(const LLSD& info);
	
	static void update();

	void onExtendFloater();

	static LLFloaterSnapshot* getInstance();
	static LLFloaterSnapshot* findInstance();
	/*virtual*/ void saveTexture();
	BOOL saveLocal();
	static void setAgentEmail(const std::string& email);

	class Impl;
	friend class Impl;
};

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot::Impl
///----------------------------------------------------------------------------

class LLFloaterSnapshot::Impl : public LLFloaterSnapshotBase::ImplBase
{
	LOG_CLASS(LLFloaterSnapshot::Impl);
public:
	Impl(LLFloaterSnapshotBase* floater)
		: LLFloaterSnapshotBase::ImplBase(floater)
	{}
	~Impl()
	{}

	void applyKeepAspectCheck(LLFloaterSnapshotBase* view, BOOL checked);
	void updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update = TRUE);
	static void onCommitLayerTypes(LLUICtrl* ctrl, void*data);
	void onImageQualityChange(LLFloaterSnapshotBase* view, S32 quality_val);
	void onImageFormatChange(LLFloaterSnapshotBase* view);
	void applyCustomResolution(LLFloaterSnapshotBase* view, S32 w, S32 h);
	static void onSendingPostcardFinished(LLFloaterSnapshotBase* floater, bool status);
	BOOL checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value);
	void setImageSizeSpinnersValues(LLFloaterSnapshotBase *view, S32 width, S32 height);
	void updateSpinners(LLFloaterSnapshotBase* view, LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL is_width_changed);
	static void onSnapshotUploadFinished(LLFloaterSnapshotBase* floater, bool status);

	/*virtual*/ LLPanelSnapshot* getActivePanel(LLFloaterSnapshotBase* floater, bool ok_if_not_found = true);
	/*virtual*/ LLSnapshotModel::ESnapshotFormat getImageFormat(LLFloaterSnapshotBase* floater);
	LLSpinCtrl* getWidthSpinner(LLFloaterSnapshotBase* floater);
	LLSpinCtrl* getHeightSpinner(LLFloaterSnapshotBase* floater);
	void enableAspectRatioCheckbox(LLFloaterSnapshotBase* floater, BOOL enable);
	void setAspectRatioCheckboxValue(LLFloaterSnapshotBase* floater, BOOL checked);
	/*virtual*/ std::string getSnapshotPanelPrefix();

	void setResolution(LLFloaterSnapshotBase* floater, const std::string& comboname);
	/*virtual*/ void updateControls(LLFloaterSnapshotBase* floater);

private:
	/*virtual*/ LLSnapshotModel::ESnapshotLayerType getLayerType(LLFloaterSnapshotBase* floater);
	void comboSetCustom(LLFloaterSnapshotBase *floater, const std::string& comboname);
	void checkAspectRatio(LLFloaterSnapshotBase *view, S32 index);
	void setFinished(bool finished, bool ok = true, const std::string& msg = LLStringUtil::null);
};

class LLSnapshotFloaterView : public LLFloaterView
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLFloaterView::Params>
	{
	};

protected:
	LLSnapshotFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleHover(S32 x, S32 y, MASK mask);
};

extern LLSnapshotFloaterView* gSnapshotFloaterView;

#endif // LL_LLFLOATERSNAPSHOT_H
