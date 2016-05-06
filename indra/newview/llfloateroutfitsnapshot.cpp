/** 
 * @file llfloatersnapshot.cpp
 * @brief Snapshot preview window for saving as an outfit thumbnail in visual outfit gallery
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersnapshot.h"
#include "llfloateroutfitsnapshot.h"

#include "llagent.h"
#include "llfacebookconnect.h"
#include "llfloaterreg.h"
#include "llfloaterfacebook.h"
#include "llfloaterflickr.h"
#include "llfloatertwitter.h"
#include "llimagefiltersmanager.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llpostcard.h"
#include "llresmgr.h"		// LLLocale
#include "llsdserialize.h"
#include "llsidetraypanelcontainer.h"
#include "llsnapshotlivepreview.h"
#include "llspinctrl.h"
#include "llviewercontrol.h"
#include "lltoolfocus.h"
#include "lltoolmgr.h"
#include "llwebprofile.h"

///----------------------------------------------------------------------------
/// Local function declarations, constants, enums, and typedefs
///----------------------------------------------------------------------------
LLUICtrl* LLFloaterOutfitSnapshot::sThumbnailPlaceholder = NULL;
LLOutfitSnapshotFloaterView* gOutfitSnapshotFloaterView = NULL;

const F32 AUTO_SNAPSHOT_TIME_DELAY = 1.f;

static LLDefaultChildRegistry::Register<LLOutfitSnapshotFloaterView> r("snapshot_outfit_floater_view");


///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot::Impl
///----------------------------------------------------------------------------


class LLFloaterOutfitSnapshot::Impl
{
	LOG_CLASS(LLFloaterOutfitSnapshot::Impl);
public:
	typedef enum e_status
	{
		STATUS_READY,
		STATUS_WORKING,
		STATUS_FINISHED
	} EStatus;

	Impl()
	:	mAvatarPauseHandles(),
		mLastToolset(NULL),
		mAspectRatioCheckOff(false),
		mNeedRefresh(false),
		mStatus(STATUS_READY)
	{
	}
	~Impl()
	{
		//unpause avatars
		mAvatarPauseHandles.clear();

	}
	static void onClickNewSnapshot(void* data);
	static void onClickAutoSnap(LLUICtrl *ctrl, void* data);
	static void onClickFilter(LLUICtrl *ctrl, void* data);
	//static void onClickAdvanceSnap(LLUICtrl *ctrl, void* data);
	static void onClickUICheck(LLUICtrl *ctrl, void* data);
	static void onClickHUDCheck(LLUICtrl *ctrl, void* data);
	static void applyKeepAspectCheck(LLFloaterOutfitSnapshot* view, BOOL checked);
	static void updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update = TRUE);
	static void onCommitFreezeFrame(LLUICtrl* ctrl, void* data);
	static void onCommitLayerTypes(LLUICtrl* ctrl, void*data);
	static void onImageQualityChange(LLFloaterOutfitSnapshot* view, S32 quality_val);
	static void onImageFormatChange(LLFloaterOutfitSnapshot* view);
	static void onSnapshotUploadFinished(bool status);
	static void onSendingPostcardFinished(bool status);
	static BOOL checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value);

	static LLPanelSnapshot* getActivePanel(LLFloaterOutfitSnapshot* floater, bool ok_if_not_found = true);
    static LLPanelSnapshot::ESnapshotType getActiveSnapshotType(LLFloaterOutfitSnapshot* floater);
	static LLFloaterOutfitSnapshot::ESnapshotFormat getImageFormat(LLFloaterOutfitSnapshot* floater);

	static LLSnapshotLivePreview* getPreviewView(LLFloaterOutfitSnapshot *floater);
	static void setResolution(LLFloaterOutfitSnapshot* floater, const std::string& comboname);
	static void updateControls(LLFloaterOutfitSnapshot* floater);
	static void updateLayout(LLFloaterOutfitSnapshot* floater);
	static void setStatus(EStatus status, bool ok = true, const std::string& msg = LLStringUtil::null);
	EStatus getStatus() const { return mStatus; }
	static void setNeedRefresh(LLFloaterOutfitSnapshot* floater, bool need);

private:
	static LLViewerWindow::ESnapshotType getLayerType(LLFloaterOutfitSnapshot* floater);
	static void checkAutoSnapshot(LLSnapshotLivePreview* floater, BOOL update_thumbnail = FALSE);
	static void setWorking(LLFloaterOutfitSnapshot* floater, bool working);
	static void setFinished(LLFloaterOutfitSnapshot* floater, bool finished, bool ok = true, const std::string& msg = LLStringUtil::null);


public:
	std::vector<LLAnimPauseRequest> mAvatarPauseHandles;

	LLToolset*	mLastToolset;
	LLHandle<LLView> mPreviewHandle;
	bool mAspectRatioCheckOff ;
	bool mNeedRefresh;
	EStatus mStatus;
};



// static
LLPanelSnapshot* LLFloaterOutfitSnapshot::Impl::getActivePanel(LLFloaterOutfitSnapshot* floater, bool ok_if_not_found)
{
    LLPanel* panel = floater->getChild<LLPanel>("panel_outfit_snapshot_inventory");
    //LLPanel* panel = panel_container->getCurrentPanel();
    LLPanelSnapshot* active_panel = dynamic_cast<LLPanelSnapshot*>(panel);
	if (!ok_if_not_found)
	{
		llassert_always(active_panel != NULL);
	}
	return active_panel;
}

// static
LLPanelSnapshot::ESnapshotType LLFloaterOutfitSnapshot::Impl::getActiveSnapshotType(LLFloaterOutfitSnapshot* floater)
{
    return LLPanelSnapshot::SNAPSHOT_TEXTURE;
}

// static
LLFloaterOutfitSnapshot::ESnapshotFormat LLFloaterOutfitSnapshot::Impl::getImageFormat(LLFloaterOutfitSnapshot* floater)
{
    return LLFloaterOutfitSnapshot::SNAPSHOT_FORMAT_PNG;
}

// static
LLSnapshotLivePreview* LLFloaterOutfitSnapshot::Impl::getPreviewView(LLFloaterOutfitSnapshot *floater)
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)floater->impl.mPreviewHandle.get();
	return previewp;
}

// static
LLViewerWindow::ESnapshotType LLFloaterOutfitSnapshot::Impl::getLayerType(LLFloaterOutfitSnapshot* floater)
{
    return LLViewerWindow::SNAPSHOT_TYPE_COLOR;
}

// static
void LLFloaterOutfitSnapshot::Impl::setResolution(LLFloaterOutfitSnapshot* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
		combo->setVisible(TRUE);
	updateResolution(combo, floater, FALSE); // to sync spinners with combo
}

//static 
void LLFloaterOutfitSnapshot::Impl::updateLayout(LLFloaterOutfitSnapshot* floaterp)
{
	LLSnapshotLivePreview* previewp = getPreviewView(floaterp);

	BOOL advanced = gSavedSettings.getBOOL("AdvanceSnapshot");

	//BD - Automatically calculate the size of our snapshot window to enlarge
	//     the snapshot preview to its maximum size, this is especially helpfull
	//     for pretty much every aspect ratio other than 1:1.
	F32 panel_width = 400.f * gViewerWindow->getWorldViewAspectRatio();

	//BD - Make sure we clamp at 700 here because 700 would be for 16:9 which we
	//     consider the maximum. Everything bigger will be clamped and will have
	//     a slightly smaller preview window which most likely won't fill up the
	//     whole snapshot floater as it should.
	if(panel_width > 700.f)
	{
		panel_width = 700.f;
	}

	S32 floater_width = 224.f;
	if(advanced)
	{
		floater_width = floater_width + panel_width;
	}

	LLUICtrl* thumbnail_placeholder = floaterp->getChild<LLUICtrl>("thumbnail_placeholder");
	thumbnail_placeholder->setVisible(advanced);
	thumbnail_placeholder->reshape(panel_width, thumbnail_placeholder->getRect().getHeight());
	floaterp->getChild<LLUICtrl>("image_res_text")->setVisible(advanced);
	floaterp->getChild<LLUICtrl>("file_size_label")->setVisible(advanced);
	if(!floaterp->isMinimized())
	{
		floaterp->reshape(floater_width, floaterp->getRect().getHeight());
	}

	bool use_freeze_frame = floaterp->getChild<LLUICtrl>("freeze_frame_check")->getValue().asBoolean();

	if (use_freeze_frame)
	{
		// stop all mouse events at fullscreen preview layer
		floaterp->getParent()->setMouseOpaque(TRUE);
		
		// shrink to smaller layout
		// *TODO: unneeded?
		floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getRect().getHeight());

		// can see and interact with fullscreen preview now
		if (previewp)
		{
			previewp->setVisible(TRUE);
			previewp->setEnabled(TRUE);
		}

		//RN: freeze all avatars
		LLCharacter* avatarp;
		for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
			iter != LLCharacter::sInstances.end(); ++iter)
		{
			avatarp = *iter;
			floaterp->impl.mAvatarPauseHandles.push_back(avatarp->requestPause());
		}

		// freeze everything else
		gSavedSettings.setBOOL("FreezeTime", TRUE);

		if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset)
		{
			floaterp->impl.mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
			LLToolMgr::getInstance()->setCurrentToolset(gCameraToolset);
		}
	}
	else // turning off freeze frame mode
	{
		floaterp->getParent()->setMouseOpaque(FALSE);
		// *TODO: unneeded?
		floaterp->reshape(floaterp->getRect().getWidth(), floaterp->getRect().getHeight());
		if (previewp)
		{
			previewp->setVisible(FALSE);
			previewp->setEnabled(FALSE);
		}

		//RN: thaw all avatars
		floaterp->impl.mAvatarPauseHandles.clear();

		// thaw everything else
		gSavedSettings.setBOOL("FreezeTime", FALSE);

		// restore last tool (e.g. pie menu, etc)
		if (floaterp->impl.mLastToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(floaterp->impl.mLastToolset);
		}
	}
}

// This is the main function that keeps all the GUI controls in sync with the saved settings.
// It should be called anytime a setting is changed that could affect the controls.
// No other methods should be changing any of the controls directly except for helpers called by this method.
// The basic pattern for programmatically changing the GUI settings is to first set the
// appropriate saved settings and then call this method to sync the GUI with them.
// FIXME: The above comment seems obsolete now.
// static
void LLFloaterOutfitSnapshot::Impl::updateControls(LLFloaterOutfitSnapshot* floater)
{
    LLPanelSnapshot::ESnapshotType shot_type = getActiveSnapshotType(floater);
    LLFloaterSnapshotBase::ESnapshotFormat shot_format = (LLFloaterSnapshotBase::ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");
    LLViewerWindow::ESnapshotType layer_type = getLayerType(floater);
		
	LLSnapshotLivePreview* previewp = getPreviewView(floater);
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();

	// *TODO: Separate maximum size for Web images from postcards
	LL_DEBUGS() << "Is snapshot up-to-date? " << got_snap << LL_ENDL;

	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_snap)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}

	// Update displayed image resolution.
	LLTextBox* image_res_tb = floater->getChild<LLTextBox>("image_res_text");
	image_res_tb->setVisible(got_snap);
	if (got_snap)
	{
		image_res_tb->setTextArg("[WIDTH]", llformat("%d", previewp->getEncodedImageWidth()));
		image_res_tb->setTextArg("[HEIGHT]", llformat("%d", previewp->getEncodedImageHeight()));
	}

	floater->getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : floater->getString("unknown"));
	floater->getChild<LLUICtrl>("file_size_label")->setColor(LLUIColorTable::instance().getColor( "LabelTextColor" ));

    setResolution(floater, "texture_size_combo");

	if (previewp)
	{
		previewp->setSnapshotType(shot_type);
		previewp->setSnapshotFormat(shot_format);
		previewp->setSnapshotBufferType(layer_type);
	}

	LLPanelSnapshot* current_panel = Impl::getActivePanel(floater);
	if (current_panel)
	{
		LLSD info;
		info["have-snapshot"] = got_snap;
		current_panel->updateControls(info);
	}
	LL_DEBUGS() << "finished updating controls" << LL_ENDL;
}

// static
void LLFloaterOutfitSnapshot::Impl::setStatus(EStatus status, bool ok, const std::string& msg)
{
	LLFloaterOutfitSnapshot* floater = LLFloaterOutfitSnapshot::getInstance();
	switch (status)
	{
	case STATUS_READY:
		setWorking(floater, false);
		setFinished(floater, false);
		break;
	case STATUS_WORKING:
		setWorking(floater, true);
		setFinished(floater, false);
		break;
	case STATUS_FINISHED:
		setWorking(floater, false);
		setFinished(floater, true, ok, msg);
		break;
	}

	floater->impl.mStatus = status;
}

// static
void LLFloaterOutfitSnapshot::Impl::setNeedRefresh(LLFloaterOutfitSnapshot* floater, bool need)
{
	if (!floater) return;

	// Don't display the "Refresh to save" message if we're in auto-refresh mode.
	if (gSavedSettings.getBOOL("AutoSnapshot"))
	{
		need = false;
	}

	floater->mRefreshLabel->setVisible(need);
	floater->impl.mNeedRefresh = need;
}

// static
void LLFloaterOutfitSnapshot::Impl::checkAutoSnapshot(LLSnapshotLivePreview* previewp, BOOL update_thumbnail)
{
	if (previewp)
	{
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		LL_DEBUGS() << "updating " << (autosnap ? "snapshot" : "thumbnail") << LL_ENDL;
		previewp->updateSnapshot(autosnap, update_thumbnail, autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f);
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onClickNewSnapshot(void* data)
{
	LLSnapshotLivePreview* previewp = getPreviewView((LLFloaterOutfitSnapshot *)data);
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
	if (previewp && view)
	{
		view->impl.setStatus(Impl::STATUS_READY);
		LL_DEBUGS() << "updating snapshot" << LL_ENDL;
		previewp->mForceUpdateSnapshot = TRUE;
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onClickAutoSnap(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "AutoSnapshot", check->get() );
	
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;		
	if (view)
	{
		checkAutoSnapshot(getPreviewView(view));
		updateControls(view);
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onClickFilter(LLUICtrl *ctrl, void* data)
{
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
	if (view)
	{
		updateControls(view);
        LLSnapshotLivePreview* previewp = getPreviewView(view);
        if (previewp)
        {
            checkAutoSnapshot(previewp);
            // Note : index 0 of the filter drop down is assumed to be "No filter" in whichever locale
            LLComboBox* filterbox = static_cast<LLComboBox *>(view->getChild<LLComboBox>("filters_combobox"));
            std::string filter_name = (filterbox->getCurrentIndex() ? filterbox->getSimple() : "");
            previewp->setFilter(filter_name);
            previewp->updateSnapshot(TRUE);
        }
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onClickUICheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderUIInSnapshot", check->get() );
	
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
	if (view)
	{
		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if(previewp)
		{
			previewp->updateSnapshot(TRUE, TRUE);
		}
		updateControls(view);
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onClickHUDCheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderHUDInSnapshot", check->get() );
	
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
	if (view)
	{
		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if(previewp)
		{
			previewp->updateSnapshot(TRUE, TRUE);
		}
		updateControls(view);
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::applyKeepAspectCheck(LLFloaterOutfitSnapshot* view, BOOL checked)
{
}

// static
void LLFloaterOutfitSnapshot::Impl::onCommitFreezeFrame(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
	LLSnapshotLivePreview* previewp = getPreviewView(view);
		
	if (!view || !check_box || !previewp)
	{
		return;
	}

	gSavedSettings.setBOOL("UseFreezeFrame", check_box->get());

	if (check_box->get())
	{
		previewp->prepareFreezeFrame();
	}

	updateLayout(view);
}

// Show/hide upload progress indicators.
// static
void LLFloaterOutfitSnapshot::Impl::setWorking(LLFloaterOutfitSnapshot* floater, bool working)
{
	LLUICtrl* working_lbl = floater->getChild<LLUICtrl>("working_lbl");
	working_lbl->setVisible(working);
	floater->getChild<LLUICtrl>("working_indicator")->setVisible(working);

	if (working)
	{
		const std::string panel_name = getActivePanel(floater, false)->getName();
		const std::string prefix = panel_name.substr(std::string("panel_outfit_snapshot_").size());
		std::string progress_text = floater->getString(prefix + "_" + "progress_str");
		working_lbl->setValue(progress_text);
	}

	// All controls should be disabled while posting.
	floater->setCtrlsEnabled(!working);
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		active_panel->enableControls(!working);
	}
}

// Show/hide upload status message.
// static
void LLFloaterOutfitSnapshot::Impl::setFinished(LLFloaterOutfitSnapshot* floater, bool finished, bool ok, const std::string& msg)
{
	floater->mSucceessLblPanel->setVisible(finished && ok);
	floater->mFailureLblPanel->setVisible(finished && !ok);

	if (finished)
	{
		LLUICtrl* finished_lbl = floater->getChild<LLUICtrl>(ok ? "succeeded_lbl" : "failed_lbl");
		std::string result_text = floater->getString(msg + "_" + (ok ? "succeeded_str" : "failed_str"));
		finished_lbl->setValue(result_text);

		//LLSideTrayPanelContainer* panel_container = floater->getChild<LLSideTrayPanelContainer>("panel_container");
		//panel_container->openPreviousPanel();
		//panel_container->getCurrentPanel()->onOpen(LLSD());
        LLPanel* snapshot_panel = floater->getChild<LLPanel>("panel_outfit_snapshot_inventory");
        snapshot_panel->onOpen(LLSD());
	}
}

// Apply a new resolution selected from the given combobox.
// static
void LLFloaterOutfitSnapshot::Impl::updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
		
	if (!view || !combobox)
	{
		llassert(view && combobox);
		return;
	}

	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());
		
	S32 width = sdres[0];
	S32 height = sdres[1];
	
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;
		
		if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{ //clamp snapshot resolution to window size when showing UI or HUD in snapshot
			width = llmin(width, gViewerWindow->getWindowWidthRaw());
			height = llmin(height, gViewerWindow->getWindowHeightRaw());
		}
        

        llassert(width > 0 && height > 0);

        // use the resolution from the selected pre-canned drop-down choice
        LL_DEBUGS() << "Setting preview res selected from combo: " << width << "x" << height << LL_ENDL;
        previewp->setSize(width, height);
		
		if(original_width != width || original_height != height)
		{
			//previewp->setSize(width, height);

			// hide old preview as the aspect ratio could be wrong
			checkAutoSnapshot(previewp, FALSE);
			LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
            previewp->updateSnapshot(TRUE);
			if(do_update)
			{
				LL_DEBUGS() << "Will update controls" << LL_ENDL;
				updateControls(view);
			}
		}
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onCommitLayerTypes(LLUICtrl* ctrl, void*data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;

	LLFloaterOutfitSnapshot *view = (LLFloaterOutfitSnapshot *)data;
		
	if (view)
	{
		LLSnapshotLivePreview* previewp = getPreviewView(view);
		if (previewp)
		{
			previewp->setSnapshotBufferType((LLViewerWindow::ESnapshotType)combobox->getCurrentIndex());
		}
		checkAutoSnapshot(previewp, TRUE);
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onImageQualityChange(LLFloaterOutfitSnapshot* view, S32 quality_val)
{
	LLSnapshotLivePreview* previewp = getPreviewView(view);
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
	}
}

// static
void LLFloaterOutfitSnapshot::Impl::onImageFormatChange(LLFloaterOutfitSnapshot* view)
{
	if (view)
	{
		gSavedSettings.setS32("SnapshotFormat", getImageFormat(view));
		LL_DEBUGS() << "image format changed, updating snapshot" << LL_ENDL;
		getPreviewView(view)->updateSnapshot(TRUE);
		updateControls(view);
	}
}

// Update supplied width and height according to the constrain proportions flag; limit them by max_val.
//static
BOOL LLFloaterOutfitSnapshot::Impl::checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value)
{
	S32 w = width ;
	S32 h = height ;

	if(previewp && previewp->mKeepAspectRatio)
	{
		if(gViewerWindow->getWindowWidthRaw() < 1 || gViewerWindow->getWindowHeightRaw() < 1)
		{
			return FALSE ;
		}

		//aspect ratio of the current window
		F32 aspect_ratio = (F32)gViewerWindow->getWindowWidthRaw() / gViewerWindow->getWindowHeightRaw() ;

		//change another value proportionally
		if(isWidthChanged)
		{
			height = ll_round(width / aspect_ratio) ;
		}
		else
		{
			width = ll_round(height * aspect_ratio) ;
		}

		//bound w/h by the max_value
		if(width > max_value || height > max_value)
		{
			if(width > height)
			{
				width = max_value ;
				height = (S32)(width / aspect_ratio) ;
			}
			else
			{
				height = max_value ;
				width = (S32)(height * aspect_ratio) ;
			}
		}
	}

	return (w != width || h != height) ;
}

// static
void LLFloaterOutfitSnapshot::Impl::onSnapshotUploadFinished(bool status)
{
	setStatus(STATUS_FINISHED, status, "profile");
}


// static
void LLFloaterOutfitSnapshot::Impl::onSendingPostcardFinished(bool status)
{
	setStatus(STATUS_FINISHED, status, "postcard");
}


///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterOutfitSnapshot::LLFloaterOutfitSnapshot(const LLSD& key)
    : LLFloaterSnapshotBase(key),
	  mRefreshBtn(NULL),
	  mRefreshLabel(NULL),
	  mSucceessLblPanel(NULL),
	  mFailureLblPanel(NULL),
      mOutfitGallery(NULL),
	  impl (*(new Impl))
{
}

// Destroys the object
LLFloaterOutfitSnapshot::~LLFloaterOutfitSnapshot()
{
	if (impl.mPreviewHandle.get()) impl.mPreviewHandle.get()->die();

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (impl.mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl.mLastToolset);
	}

	delete &impl;
}


BOOL LLFloaterOutfitSnapshot::postBuild()
{
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	childSetAction("new_snapshot_btn", Impl::onClickNewSnapshot, this);
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");

	childSetCommitCallback("ui_check", Impl::onClickUICheck, this);
	getChild<LLUICtrl>("ui_check")->setValue(gSavedSettings.getBOOL("RenderUIInSnapshot"));

	childSetCommitCallback("hud_check", Impl::onClickHUDCheck, this);
	getChild<LLUICtrl>("hud_check")->setValue(gSavedSettings.getBOOL("RenderHUDInSnapshot"));

	getChild<LLUICtrl>("freeze_frame_check")->setValue(gSavedSettings.getBOOL("UseFreezeFrame"));
	childSetCommitCallback("freeze_frame_check", Impl::onCommitFreezeFrame, this);

	getChild<LLUICtrl>("auto_snapshot_check")->setValue(gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", Impl::onClickAutoSnap, this);
    

	// Filters
	LLComboBox* filterbox = getChild<LLComboBox>("filters_combobox");
    std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
    for (U32 i = 0; i < filter_list.size(); i++)
    {
        filterbox->add(filter_list[i]);
    }
    childSetCommitCallback("filters_combobox", Impl::onClickFilter, this);
    
	LLWebProfile::setImageUploadResultCallback(boost::bind(&LLFloaterOutfitSnapshot::Impl::onSnapshotUploadFinished, _1));
	LLPostCard::setPostResultCallback(boost::bind(&LLFloaterOutfitSnapshot::Impl::onSendingPostcardFinished, _1));

	sThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

	// create preview window
	LLRect full_screen_rect = getRootView()->getRect();
	LLSnapshotLivePreview::Params p;
	p.rect(full_screen_rect);
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
	LLView* parent_view = gSnapshotFloaterView->getParent();
	
	parent_view->removeChild(gSnapshotFloaterView);
	// make sure preview is below snapshot floater
	parent_view->addChild(previewp);
	parent_view->addChild(gSnapshotFloaterView);
	
	//move snapshot floater to special purpose snapshotfloaterview
	gFloaterView->removeChild(this);
	gSnapshotFloaterView->addChild(this);

	// Pre-select "Current Window" resolution.
	getChild<LLComboBox>("profile_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("postcard_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("texture_size_combo")->selectNthItem(0);
	getChild<LLComboBox>("local_size_combo")->selectNthItem(8);
	getChild<LLComboBox>("local_format_combo")->selectNthItem(0);

	impl.mPreviewHandle = previewp->getHandle();
    previewp->setContainer(this);
	impl.updateControls(this);
	impl.updateLayout(this);
	

	previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());

	return TRUE;
}

void LLFloaterOutfitSnapshot::draw()
{
	LLSnapshotLivePreview* previewp = impl.getPreviewView(this);

	if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
	{
		// don't render snapshot window in snapshot, even if "show ui" is turned on
		return;
	}

	LLFloater::draw();

	if (previewp && !isMinimized() && sThumbnailPlaceholder->getVisible())
	{		
		if(previewp->getThumbnailImage())
		{
			bool working = impl.getStatus() == Impl::STATUS_WORKING;
			const LLRect& thumbnail_rect = getThumbnailPlaceholderRect();
			const S32 thumbnail_w = previewp->getThumbnailWidth();
			const S32 thumbnail_h = previewp->getThumbnailHeight();

			// calc preview offset within the preview rect
			const S32 local_offset_x = (thumbnail_rect.getWidth() - thumbnail_w) / 2 ;
			const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2 ; // preview y pos within the preview rect

			// calc preview offset within the floater rect
			S32 offset_x = thumbnail_rect.mLeft + local_offset_x;
			S32 offset_y = thumbnail_rect.mBottom + local_offset_y;

			gGL.matrixMode(LLRender::MM_MODELVIEW);
			// Apply floater transparency to the texture unless the floater is focused.
			F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
			LLColor4 color = working ? LLColor4::grey4 : LLColor4::white;
			gl_draw_scaled_image(offset_x, offset_y, 
					thumbnail_w, thumbnail_h,
					previewp->getThumbnailImage(), color % alpha);

			previewp->drawPreviewRect(offset_x, offset_y) ;

			gGL.pushUIMatrix();
			LLUI::translate((F32) thumbnail_rect.mLeft, (F32) thumbnail_rect.mBottom);
			sThumbnailPlaceholder->draw();
			gGL.popUIMatrix();
		}
	}
	impl.updateLayout(this);
}

void LLFloaterOutfitSnapshot::onOpen(const LLSD& key)
{
	LLSnapshotLivePreview* preview = LLFloaterOutfitSnapshot::Impl::getPreviewView(this);
	if(preview)
	{
		LL_DEBUGS() << "opened, updating snapshot" << LL_ENDL;
		preview->updateSnapshot(TRUE);
	}
	focusFirstItem(FALSE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->setVisible(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(this, FALSE);

	impl.updateControls(this);
	impl.updateLayout(this);

	// Initialize default tab.
	//getChild<LLSideTrayPanelContainer>("panel_container")->getCurrentPanel()->onOpen(LLSD());
    //parent->openPanel(panel_name);
    LLPanel* snapshot_panel = getChild<LLPanel>("panel_outfit_snapshot_inventory");
    snapshot_panel->onOpen(LLSD());
    postPanelSwitch();

}

void LLFloaterOutfitSnapshot::onClose(bool app_quitting)
{
	getParent()->setMouseOpaque(FALSE);

	//unfreeze everything, hide fullscreen preview
	LLSnapshotLivePreview* previewp = LLFloaterOutfitSnapshot::Impl::getPreviewView(this);
	if (previewp)
	{
		previewp->setVisible(FALSE);
		previewp->setEnabled(FALSE);
	}

	gSavedSettings.setBOOL("FreezeTime", FALSE);
	impl.mAvatarPauseHandles.clear();

	if (impl.mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl.mLastToolset);
	}
}

// virtual
S32 LLFloaterOutfitSnapshot::notify(const LLSD& info)
{
	// A child panel wants to change snapshot resolution.
	if (info.has("combo-res-change"))
	{
		std::string combo_name = info["combo-res-change"]["control-name"].asString();
		impl.updateResolution(getChild<LLUICtrl>(combo_name), this);
		return 1;
	}
    /*
	if (info.has("custom-res-change"))
	{
		LLSD res = info["custom-res-change"];
		impl.applyCustomResolution(this, res["w"].asInteger(), res["h"].asInteger());
		return 1;
	}

	if (info.has("keep-aspect-change"))
	{
		impl.applyKeepAspectCheck(this, info["keep-aspect-change"].asBoolean());
		return 1;
	}
    */
	if (info.has("image-quality-change"))
	{
		impl.onImageQualityChange(this, info["image-quality-change"].asInteger());
		return 1;
	}

	if (info.has("image-format-change"))
	{
		impl.onImageFormatChange(this);
		return 1;
	}

	if (info.has("set-ready"))
	{
		impl.setStatus(Impl::STATUS_READY);
		return 1;
	}

	if (info.has("set-working"))
	{
		impl.setStatus(Impl::STATUS_WORKING);
		return 1;
	}

	if (info.has("set-finished"))
	{
		LLSD data = info["set-finished"];
		impl.setStatus(Impl::STATUS_FINISHED, data["ok"].asBoolean(), data["msg"].asString());
		return 1;
	}
    
	if (info.has("snapshot-updating"))
	{
        // Disable the send/post/save buttons until snapshot is ready.
        impl.updateControls(this);
		return 1;
	}

	if (info.has("snapshot-updated"))
	{
        // Enable the send/post/save buttons.
        impl.updateControls(this);
        // We've just done refresh.
        impl.setNeedRefresh(this, false);
            
        // The refresh button is initially hidden. We show it after the first update,
        // i.e. when preview appears.
        if (!mRefreshBtn->getVisible())
        {
            mRefreshBtn->setVisible(true);
        }
		return 1;
	}    
    
	return 0;
}

//static 
void LLFloaterOutfitSnapshot::update()
{
	LLFloaterOutfitSnapshot* inst = findInstance();
	LLFloaterFacebook* floater_facebook = LLFloaterReg::findTypedInstance<LLFloaterFacebook>("facebook"); 
	LLFloaterFlickr* floater_flickr = LLFloaterReg::findTypedInstance<LLFloaterFlickr>("flickr"); 
	LLFloaterTwitter* floater_twitter = LLFloaterReg::findTypedInstance<LLFloaterTwitter>("twitter"); 

	if (!inst && !floater_facebook && !floater_flickr && !floater_twitter)
		return;
	
	BOOL changed = FALSE;
	LL_DEBUGS() << "npreviews: " << LLSnapshotLivePreview::sList.size() << LL_ENDL;
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		 iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		changed |= LLSnapshotLivePreview::onIdle(*iter);
	}
    
	if (inst && changed)
	{
		LL_DEBUGS() << "changed" << LL_ENDL;
		inst->impl.updateControls(inst);
	}
}

// static
LLFloaterOutfitSnapshot* LLFloaterOutfitSnapshot::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterOutfitSnapshot>("outfit_snapshot");
}

// static
LLFloaterOutfitSnapshot* LLFloaterOutfitSnapshot::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterOutfitSnapshot>("outfit_snapshot");
}

// static
void LLFloaterOutfitSnapshot::saveTexture()
{
	LL_DEBUGS() << "saveTexture" << LL_ENDL;

	// FIXME: duplicated code
	LLFloaterOutfitSnapshot* instance = findInstance();
	if (!instance)
	{
		llassert(instance != NULL);
		return;
	}
	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return;
	}

    if (instance->mOutfitGallery)
    {
        instance->mOutfitGallery->onBeforeOutfitSnapshotSave();
    }
    previewp->saveTexture(TRUE, instance->getOutfitID().asString());
    if (instance->mOutfitGallery)
    {
        instance->mOutfitGallery->onAfterOutfitSnapshotSave();
    }
    instance->closeFloater();
}

// static
BOOL LLFloaterOutfitSnapshot::saveLocal()
{
	LL_DEBUGS() << "saveLocal" << LL_ENDL;
	// FIXME: duplicated code
	LLFloaterOutfitSnapshot* instance = findInstance();
	if (!instance)
	{
		llassert(instance != NULL);
		return FALSE;
	}
	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return FALSE;
	}

	return previewp->saveLocal();
}

// static
void LLFloaterOutfitSnapshot::postSave()
{
	LLFloaterOutfitSnapshot* instance = findInstance();
	if (!instance)
	{
		llassert(instance != NULL);
		return;
	}

	instance->impl.updateControls(instance);
	instance->impl.setStatus(Impl::STATUS_WORKING);
}

// static
void LLFloaterOutfitSnapshot::postPanelSwitch()
{
	LLFloaterOutfitSnapshot* instance = getInstance();
	instance->impl.updateControls(instance);

	// Remove the success/failure indicator whenever user presses a snapshot option button.
	instance->impl.setStatus(Impl::STATUS_READY);
}

// static
LLPointer<LLImageFormatted> LLFloaterOutfitSnapshot::getImageData()
{
	// FIXME: May not work for textures.

	LLFloaterOutfitSnapshot* instance = findInstance();
	if (!instance)
	{
		llassert(instance != NULL);
		return NULL;
	}

	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return NULL;
	}

	LLPointer<LLImageFormatted> img = previewp->getFormattedImage();
	if (!img.get())
	{
		LL_WARNS() << "Empty snapshot image data" << LL_ENDL;
		llassert(img.get() != NULL);
	}

	return img;
}

// static
const LLVector3d& LLFloaterOutfitSnapshot::getPosTakenGlobal()
{
	LLFloaterOutfitSnapshot* instance = findInstance();
	if (!instance)
	{
		llassert(instance != NULL);
		return LLVector3d::zero;
	}

	LLSnapshotLivePreview* previewp = Impl::getPreviewView(instance);
	if (!previewp)
	{
		llassert(previewp != NULL);
		return LLVector3d::zero;
	}

	return previewp->getPosTakenGlobal();
}

// static
void LLFloaterOutfitSnapshot::setAgentEmail(const std::string& email)
{
	LLFloaterOutfitSnapshot* instance = findInstance();
	if (instance)
	{
		LLSideTrayPanelContainer* panel_container = instance->getChild<LLSideTrayPanelContainer>("panel_container");
		LLPanel* postcard_panel = panel_container->getPanelByName("panel_snapshot_postcard");
		postcard_panel->notify(LLSD().with("agent-email", email));
	}
}

///----------------------------------------------------------------------------
/// Class LLSnapshotFloaterView
///----------------------------------------------------------------------------

LLOutfitSnapshotFloaterView::LLOutfitSnapshotFloaterView (const Params& p) : LLFloaterView (p)
{
}

LLOutfitSnapshotFloaterView::~LLOutfitSnapshotFloaterView()
{
}

BOOL LLOutfitSnapshotFloaterView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleKey(key, mask, called_from_parent);
	}

	if (called_from_parent)
	{
		// pass all keystrokes down
		LLFloaterView::handleKey(key, mask, called_from_parent);
	}
	else
	{
		// bounce keystrokes back down
		LLFloaterView::handleKey(key, mask, TRUE);
	}
	return TRUE;
}

BOOL LLOutfitSnapshotFloaterView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleMouseDown(x, y, mask);
	}
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleMouseDown(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseDown( x, y, mask );
	}
	return TRUE;
}

BOOL LLOutfitSnapshotFloaterView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleMouseUp(x, y, mask);
	}
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleMouseUp(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleMouseUp( x, y, mask );
	}
	return TRUE;
}

BOOL LLOutfitSnapshotFloaterView::handleHover(S32 x, S32 y, MASK mask)
{
	// use default handler when not in freeze-frame mode
	if(!gSavedSettings.getBOOL("FreezeTime"))
	{
		return LLFloaterView::handleHover(x, y, mask);
	}	
	// give floater a change to handle mouse, else camera tool
	if (childrenHandleHover(x, y, mask) == NULL)
	{
		LLToolMgr::getInstance()->getCurrentTool()->handleHover( x, y, mask );
	}
	return TRUE;
}
