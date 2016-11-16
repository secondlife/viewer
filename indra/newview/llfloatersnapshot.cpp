/** 
 * @file llfloatersnapshot.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersnapshot.h"

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
LLSnapshotFloaterView* gSnapshotFloaterView = NULL;

const F32 AUTO_SNAPSHOT_TIME_DELAY = 1.f;

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const S32 MAX_TEXTURE_SIZE = 512 ; //max upload texture size 512 * 512

static LLDefaultChildRegistry::Register<LLSnapshotFloaterView> r("snapshot_floater_view");

// virtual
LLPanelSnapshot* LLFloaterSnapshot::Impl::getActivePanel(LLFloaterSnapshotBase* floater, bool ok_if_not_found)
{
	LLSideTrayPanelContainer* panel_container = floater->getChild<LLSideTrayPanelContainer>("panel_container");
	LLPanelSnapshot* active_panel = dynamic_cast<LLPanelSnapshot*>(panel_container->getCurrentPanel());
	if (!ok_if_not_found)
	{
		llassert_always(active_panel != NULL);
	}
	return active_panel;
}

// virtual
LLSnapshotModel::ESnapshotType LLFloaterSnapshotBase::ImplBase::getActiveSnapshotType(LLFloaterSnapshotBase* floater)
{
	LLPanelSnapshot* spanel = getActivePanel(floater);

	//return type;
    if (spanel)
    {
        return spanel->getSnapshotType();
    }
	return LLSnapshotModel::SNAPSHOT_WEB;
}

// virtual
LLSnapshotModel::ESnapshotFormat LLFloaterSnapshot::Impl::getImageFormat(LLFloaterSnapshotBase* floater)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	// FIXME: if the default is not PNG, profile uploads may fail.
	return active_panel ? active_panel->getImageFormat() : LLSnapshotModel::SNAPSHOT_FORMAT_PNG;
}

LLSpinCtrl* LLFloaterSnapshot::Impl::getWidthSpinner(LLFloaterSnapshotBase* floater)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	return active_panel ? active_panel->getWidthSpinner() : floater->getChild<LLSpinCtrl>("snapshot_width");
}

LLSpinCtrl* LLFloaterSnapshot::Impl::getHeightSpinner(LLFloaterSnapshotBase* floater)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	return active_panel ? active_panel->getHeightSpinner() : floater->getChild<LLSpinCtrl>("snapshot_height");
}

void LLFloaterSnapshot::Impl::enableAspectRatioCheckbox(LLFloaterSnapshotBase* floater, BOOL enable)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		active_panel->enableAspectRatioCheckbox(enable);
	}
}

void LLFloaterSnapshot::Impl::setAspectRatioCheckboxValue(LLFloaterSnapshotBase* floater, BOOL checked)
{
	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		active_panel->getChild<LLUICtrl>(active_panel->getAspectRatioCBName())->setValue(checked);
	}
}

LLSnapshotLivePreview* LLFloaterSnapshotBase::getPreviewView()
{
	return impl->getPreviewView();
}

LLSnapshotLivePreview* LLFloaterSnapshotBase::ImplBase::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}

// virtual
LLSnapshotModel::ESnapshotLayerType LLFloaterSnapshot::Impl::getLayerType(LLFloaterSnapshotBase* floater)
{
	LLSnapshotModel::ESnapshotLayerType type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
	LLSD value = floater->getChild<LLUICtrl>("layer_types")->getValue();
	const std::string id = value.asString();
	if (id == "colors")
		type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
	else if (id == "depth")
		type = LLSnapshotModel::SNAPSHOT_TYPE_DEPTH;
	return type;
}

void LLFloaterSnapshot::Impl::setResolution(LLFloaterSnapshotBase* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
		combo->setVisible(TRUE);
	updateResolution(combo, floater, FALSE); // to sync spinners with combo
}

//virtual 
void LLFloaterSnapshotBase::ImplBase::updateLayout(LLFloaterSnapshotBase* floaterp)
{
	LLSnapshotLivePreview* previewp = getPreviewView();

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
	if(mAdvanced)
	{
		floater_width = floater_width + panel_width;
	}

	LLUICtrl* thumbnail_placeholder = floaterp->getChild<LLUICtrl>("thumbnail_placeholder");
	thumbnail_placeholder->setVisible(mAdvanced);
	thumbnail_placeholder->reshape(panel_width, thumbnail_placeholder->getRect().getHeight());
	floaterp->getChild<LLUICtrl>("image_res_text")->setVisible(mAdvanced);
	floaterp->getChild<LLUICtrl>("file_size_label")->setVisible(mAdvanced);
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
			floaterp->impl->mAvatarPauseHandles.push_back(avatarp->requestPause());
		}

		// freeze everything else
		gSavedSettings.setBOOL("FreezeTime", TRUE);

		if (LLToolMgr::getInstance()->getCurrentToolset() != gCameraToolset)
		{
			floaterp->impl->mLastToolset = LLToolMgr::getInstance()->getCurrentToolset();
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
		floaterp->impl->mAvatarPauseHandles.clear();

		// thaw everything else
		gSavedSettings.setBOOL("FreezeTime", FALSE);

		// restore last tool (e.g. pie menu, etc)
		if (floaterp->impl->mLastToolset)
		{
			LLToolMgr::getInstance()->setCurrentToolset(floaterp->impl->mLastToolset);
		}
	}
}

// This is the main function that keeps all the GUI controls in sync with the saved settings.
// It should be called anytime a setting is changed that could affect the controls.
// No other methods should be changing any of the controls directly except for helpers called by this method.
// The basic pattern for programmatically changing the GUI settings is to first set the
// appropriate saved settings and then call this method to sync the GUI with them.
// FIXME: The above comment seems obsolete now.
// virtual
void LLFloaterSnapshot::Impl::updateControls(LLFloaterSnapshotBase* floater)
{
	LLSnapshotModel::ESnapshotType shot_type = getActiveSnapshotType(floater);
	LLSnapshotModel::ESnapshotFormat shot_format = (LLSnapshotModel::ESnapshotFormat)gSavedSettings.getS32("SnapshotFormat");
	LLSnapshotModel::ESnapshotLayerType layer_type = getLayerType(floater);

	floater->getChild<LLComboBox>("local_format_combo")->selectNthItem(gSavedSettings.getS32("SnapshotFormat"));
	floater->getChildView("layer_types")->setEnabled(shot_type == LLSnapshotModel::SNAPSHOT_LOCAL);

	LLPanelSnapshot* active_panel = getActivePanel(floater);
	if (active_panel)
	{
		LLSpinCtrl* width_ctrl = getWidthSpinner(floater);
		LLSpinCtrl* height_ctrl = getHeightSpinner(floater);

		// Initialize spinners.
		if (width_ctrl->getValue().asInteger() == 0)
		{
			S32 w = gViewerWindow->getWindowWidthRaw();
			LL_DEBUGS() << "Initializing width spinner (" << width_ctrl->getName() << "): " << w << LL_ENDL;
			width_ctrl->setValue(w);
			if (getActiveSnapshotType(floater) == LLSnapshotModel::SNAPSHOT_TEXTURE)
			{
				width_ctrl->setIncrement(w >> 1);
			}
		}
		if (height_ctrl->getValue().asInteger() == 0)
		{
			S32 h = gViewerWindow->getWindowHeightRaw();
			LL_DEBUGS() << "Initializing height spinner (" << height_ctrl->getName() << "): " << h << LL_ENDL;
			height_ctrl->setValue(h);
			if (getActiveSnapshotType(floater) == LLSnapshotModel::SNAPSHOT_TEXTURE)
			{
				height_ctrl->setIncrement(h >> 1);
			}
		}

		// Clamp snapshot resolution to window size when showing UI or HUD in snapshot.
		if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{
			S32 width = gViewerWindow->getWindowWidthRaw();
			S32 height = gViewerWindow->getWindowHeightRaw();

			width_ctrl->setMaxValue(width);

			height_ctrl->setMaxValue(height);

			if (width_ctrl->getValue().asInteger() > width)
			{
				width_ctrl->forceSetValue(width);
			}
			if (height_ctrl->getValue().asInteger() > height)
			{
				height_ctrl->forceSetValue(height);
			}
		}
		else
		{
			width_ctrl->setMaxValue(6016);
			height_ctrl->setMaxValue(6016);
		}
	}
		
	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_bytes = previewp && previewp->getDataSize() > 0;
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
	floater->getChild<LLUICtrl>("file_size_label")->setColor(
			shot_type == LLSnapshotModel::SNAPSHOT_POSTCARD
			&& got_bytes
			&& previewp->getDataSize() > MAX_POSTCARD_DATASIZE ? LLUIColor(LLColor4::red) : LLUIColorTable::instance().getColor( "LabelTextColor" ));

	// Update the width and height spinners based on the corresponding resolution combos. (?)
	switch(shot_type)
	{
	  case LLSnapshotModel::SNAPSHOT_WEB:
		layer_type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
		floater->getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution(floater, "profile_size_combo");
		break;
	  case LLSnapshotModel::SNAPSHOT_POSTCARD:
		layer_type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
		floater->getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution(floater, "postcard_size_combo");
		break;
	  case LLSnapshotModel::SNAPSHOT_TEXTURE:
		layer_type = LLSnapshotModel::SNAPSHOT_TYPE_COLOR;
		floater->getChild<LLUICtrl>("layer_types")->setValue("colors");
		setResolution(floater, "texture_size_combo");
		break;
	  case  LLSnapshotModel::SNAPSHOT_LOCAL:
		setResolution(floater, "local_size_combo");
		break;
	  default:
		break;
	}
	setAspectRatioCheckboxValue(floater, !floater->impl->mAspectRatioCheckOff && gSavedSettings.getBOOL("KeepAspectForSnapshot"));

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

//virtual
void LLFloaterSnapshotBase::ImplBase::setStatus(EStatus status, bool ok, const std::string& msg)
{
	switch (status)
	{
	case STATUS_READY:
		setWorking(false);
		setFinished(false);
		break;
	case STATUS_WORKING:
		setWorking(true);
		setFinished(false);
		break;
	case STATUS_FINISHED:
		setWorking(false);
		setFinished(true, ok, msg);
		break;
	}

	mStatus = status;
}

// virtual
void LLFloaterSnapshotBase::ImplBase::setNeedRefresh(bool need)
{
	if (!mFloater) return;

	// Don't display the "Refresh to save" message if we're in auto-refresh mode.
	if (gSavedSettings.getBOOL("AutoSnapshot"))
	{
		need = false;
	}

	mFloater->setRefreshLabelVisible(need);
	mNeedRefresh = need;
}

// virtual
void LLFloaterSnapshotBase::ImplBase::checkAutoSnapshot(LLSnapshotLivePreview* previewp, BOOL update_thumbnail)
{
	if (previewp)
	{
		BOOL autosnap = gSavedSettings.getBOOL("AutoSnapshot");
		LL_DEBUGS() << "updating " << (autosnap ? "snapshot" : "thumbnail") << LL_ENDL;
		previewp->updateSnapshot(autosnap, update_thumbnail, autosnap ? AUTO_SNAPSHOT_TIME_DELAY : 0.f);
	}
}

// static
void LLFloaterSnapshotBase::ImplBase::onClickNewSnapshot(void* data)
{
	LLFloaterSnapshotBase* floater = (LLFloaterSnapshotBase *)data;
	LLSnapshotLivePreview* previewp = floater->getPreviewView();
	if (previewp)
	{
		floater->impl->setStatus(ImplBase::STATUS_READY);
		LL_DEBUGS() << "updating snapshot" << LL_ENDL;
		previewp->mForceUpdateSnapshot = TRUE;
	}
}

// static
void LLFloaterSnapshotBase::ImplBase::onClickAutoSnap(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "AutoSnapshot", check->get() );
	
	LLFloaterSnapshotBase *view = (LLFloaterSnapshotBase *)data;		
	if (view)
	{
		view->impl->checkAutoSnapshot(view->getPreviewView());
		view->impl->updateControls(view);
	}
}

// static
void LLFloaterSnapshotBase::ImplBase::onClickFilter(LLUICtrl *ctrl, void* data)
{
	LLFloaterSnapshotBase *view = (LLFloaterSnapshotBase *)data;
	if (view)
	{
		view->impl->updateControls(view);
		LLSnapshotLivePreview* previewp = view->getPreviewView();
        if (previewp)
        {
			view->impl->checkAutoSnapshot(previewp);
            // Note : index 0 of the filter drop down is assumed to be "No filter" in whichever locale
            LLComboBox* filterbox = static_cast<LLComboBox *>(view->getChild<LLComboBox>("filters_combobox"));
            std::string filter_name = (filterbox->getCurrentIndex() ? filterbox->getSimple() : "");
            previewp->setFilter(filter_name);
            previewp->updateSnapshot(TRUE);
        }
	}
}

// static
void LLFloaterSnapshotBase::ImplBase::onClickUICheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderUIInSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		LLSnapshotLivePreview* previewp = view->getPreviewView();
		if(previewp)
		{
			previewp->updateSnapshot(TRUE, TRUE);
		}
		view->impl->updateControls(view);
	}
}

// static
void LLFloaterSnapshotBase::ImplBase::onClickHUDCheck(LLUICtrl *ctrl, void* data)
{
	LLCheckBoxCtrl *check = (LLCheckBoxCtrl *)ctrl;
	gSavedSettings.setBOOL( "RenderHUDInSnapshot", check->get() );
	
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (view)
	{
		LLSnapshotLivePreview* previewp = view->getPreviewView();
		if(previewp)
		{
			previewp->updateSnapshot(TRUE, TRUE);
		}
		view->impl->updateControls(view);
	}
}

void LLFloaterSnapshot::Impl::applyKeepAspectCheck(LLFloaterSnapshotBase* view, BOOL checked)
{
	gSavedSettings.setBOOL("KeepAspectForSnapshot", checked);

	if (view)
	{
		LLPanelSnapshot* active_panel = getActivePanel(view);
		if (checked && active_panel)
		{
			LLComboBox* combo = view->getChild<LLComboBox>(active_panel->getImageSizeComboName());
			combo->setCurrentByIndex(combo->getItemCount() - 1); // "custom" is always the last index
		}

		LLSnapshotLivePreview* previewp = getPreviewView() ;
		if(previewp)
		{
			previewp->mKeepAspectRatio = gSavedSettings.getBOOL("KeepAspectForSnapshot") ;

			S32 w, h ;
			previewp->getSize(w, h) ;
			updateSpinners(view, previewp, w, h, TRUE); // may change w and h

			LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
			previewp->setSize(w, h) ;
			previewp->updateSnapshot(TRUE);
			checkAutoSnapshot(previewp, TRUE);
		}
	}
}

// static
void LLFloaterSnapshotBase::ImplBase::onCommitFreezeFrame(LLUICtrl* ctrl, void* data)
{
	LLCheckBoxCtrl* check_box = (LLCheckBoxCtrl*)ctrl;
	LLFloaterSnapshotBase *view = (LLFloaterSnapshotBase *)data;
	LLSnapshotLivePreview* previewp = view->getPreviewView();
		
	if (!view || !check_box || !previewp)
	{
		return;
	}

	gSavedSettings.setBOOL("UseFreezeFrame", check_box->get());

	if (check_box->get())
	{
		previewp->prepareFreezeFrame();
	}

	view->impl->updateLayout(view);
}

void LLFloaterSnapshot::Impl::checkAspectRatio(LLFloaterSnapshotBase *view, S32 index)
{
	LLSnapshotLivePreview *previewp = getPreviewView() ;

	// Don't round texture sizes; textures are commonly stretched in world, profiles, etc and need to be "squashed" during upload, not cropped here
	if (LLSnapshotModel::SNAPSHOT_TEXTURE == getActiveSnapshotType(view))
	{
		previewp->mKeepAspectRatio = FALSE ;
		return ;
	}

	BOOL keep_aspect = FALSE, enable_cb = FALSE;

	if (0 == index) // current window size
	{
		enable_cb = FALSE;
		keep_aspect = TRUE;
	}
	else if (-1 == index) // custom
	{
		enable_cb = TRUE;
		keep_aspect = gSavedSettings.getBOOL("KeepAspectForSnapshot");
	}
	else // predefined resolution
	{
		enable_cb = FALSE;
		keep_aspect = FALSE;
	}

	view->impl->mAspectRatioCheckOff = !enable_cb;

	if (previewp)
	{
		previewp->mKeepAspectRatio = keep_aspect;
	}
}

// Show/hide upload progress indicators.
void LLFloaterSnapshotBase::ImplBase::setWorking(bool working)
{
	LLUICtrl* working_lbl = mFloater->getChild<LLUICtrl>("working_lbl");
	working_lbl->setVisible(working);
	mFloater->getChild<LLUICtrl>("working_indicator")->setVisible(working);

	if (working)
	{
		const std::string panel_name = getActivePanel(mFloater, false)->getName();
		const std::string prefix = panel_name.substr(getSnapshotPanelPrefix().size());
		std::string progress_text = mFloater->getString(prefix + "_" + "progress_str");
		working_lbl->setValue(progress_text);
	}

	// All controls should be disabled while posting.
	mFloater->setCtrlsEnabled(!working);
	LLPanelSnapshot* active_panel = getActivePanel(mFloater);
	if (active_panel)
	{
		active_panel->enableControls(!working);
	}
}

//virtual
std::string LLFloaterSnapshot::Impl::getSnapshotPanelPrefix()
{
	return "panel_snapshot_";
}

// Show/hide upload status message.
// virtual
void LLFloaterSnapshot::Impl::setFinished(bool finished, bool ok, const std::string& msg)
{
	mFloater->setSuccessLabelPanelVisible(finished && ok);
	mFloater->setFailureLabelPanelVisible(finished && !ok);

	if (finished)
	{
		LLUICtrl* finished_lbl = mFloater->getChild<LLUICtrl>(ok ? "succeeded_lbl" : "failed_lbl");
		std::string result_text = mFloater->getString(msg + "_" + (ok ? "succeeded_str" : "failed_str"));
		finished_lbl->setValue(result_text);

		LLSideTrayPanelContainer* panel_container = mFloater->getChild<LLSideTrayPanelContainer>("panel_container");
		panel_container->openPreviousPanel();
		panel_container->getCurrentPanel()->onOpen(LLSD());
	}
}

// Apply a new resolution selected from the given combobox.
void LLFloaterSnapshot::Impl::updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;
	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
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
	
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;
		
		if (gSavedSettings.getBOOL("RenderUIInSnapshot") || gSavedSettings.getBOOL("RenderHUDInSnapshot"))
		{ //clamp snapshot resolution to window size when showing UI or HUD in snapshot
			width = llmin(width, gViewerWindow->getWindowWidthRaw());
			height = llmin(height, gViewerWindow->getWindowHeightRaw());
		}

		if (width == 0 || height == 0)
		{
			// take resolution from current window size
			LL_DEBUGS() << "Setting preview res from window: " << gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << LL_ENDL;
			previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
		}
		else if (width == -1 || height == -1)
		{
			// load last custom value
			S32 new_width = 0, new_height = 0;
			LLPanelSnapshot* spanel = getActivePanel(view);
			if (spanel)
			{
				LL_DEBUGS() << "Loading typed res from panel " << spanel->getName() << LL_ENDL;
				new_width = spanel->getTypedPreviewWidth();
				new_height = spanel->getTypedPreviewHeight();

				// Limit custom size for inventory snapshots to 512x512 px.
				if (getActiveSnapshotType(view) == LLSnapshotModel::SNAPSHOT_TEXTURE)
				{
					new_width = llmin(new_width, MAX_TEXTURE_SIZE);
					new_height = llmin(new_height, MAX_TEXTURE_SIZE);
				}
			}
			else
			{
				LL_DEBUGS() << "No custom res chosen, setting preview res from window: "
					<< gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << LL_ENDL;
				new_width = gViewerWindow->getWindowWidthRaw();
				new_height = gViewerWindow->getWindowHeightRaw();
			}

			llassert(new_width > 0 && new_height > 0);
			previewp->setSize(new_width, new_height);
		}
		else
		{
			// use the resolution from the selected pre-canned drop-down choice
			LL_DEBUGS() << "Setting preview res selected from combo: " << width << "x" << height << LL_ENDL;
			previewp->setSize(width, height);
		}

		checkAspectRatio(view, width) ;

		previewp->getSize(width, height);

		// We use the height spinner here because we come here via the aspect ratio
		// checkbox as well and we want height always changing to width by default.
		// If we use the width spinner we would change width according to height by
		// default, that is not what we want.
		updateSpinners(view, previewp, width, height, !getHeightSpinner(view)->isDirty()); // may change width and height
		
		if(getWidthSpinner(view)->getValue().asInteger() != width || getHeightSpinner(view)->getValue().asInteger() != height)
		{
			getWidthSpinner(view)->setValue(width);
			getHeightSpinner(view)->setValue(height);
			if (getActiveSnapshotType(view) == LLSnapshotModel::SNAPSHOT_TEXTURE)
			{
				getWidthSpinner(view)->setIncrement(width >> 1);
				getHeightSpinner(view)->setIncrement(height >> 1);
			}
		}

		if(original_width != width || original_height != height)
		{
			previewp->setSize(width, height);

			// hide old preview as the aspect ratio could be wrong
			checkAutoSnapshot(previewp, FALSE);
			LL_DEBUGS() << "updating thumbnail" << LL_ENDL;
			getPreviewView()->updateSnapshot(TRUE);
			if(do_update)
			{
				LL_DEBUGS() << "Will update controls" << LL_ENDL;
				updateControls(view);
			}
		}
	}
}

// static
void LLFloaterSnapshot::Impl::onCommitLayerTypes(LLUICtrl* ctrl, void*data)
{
	LLComboBox* combobox = (LLComboBox*)ctrl;

	LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
		
	if (view)
	{
		LLSnapshotLivePreview* previewp = view->getPreviewView();
		if (previewp)
		{
			previewp->setSnapshotBufferType((LLSnapshotModel::ESnapshotLayerType)combobox->getCurrentIndex());
		}
		view->impl->checkAutoSnapshot(previewp, TRUE);
	}
}

void LLFloaterSnapshot::Impl::onImageQualityChange(LLFloaterSnapshotBase* view, S32 quality_val)
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setSnapshotQuality(quality_val);
	}
}

void LLFloaterSnapshot::Impl::onImageFormatChange(LLFloaterSnapshotBase* view)
{
	if (view)
	{
		gSavedSettings.setS32("SnapshotFormat", getImageFormat(view));
		LL_DEBUGS() << "image format changed, updating snapshot" << LL_ENDL;
		getPreviewView()->updateSnapshot(TRUE);
		updateControls(view);
	}
}

// Sets the named size combo to "custom" mode.
void LLFloaterSnapshot::Impl::comboSetCustom(LLFloaterSnapshotBase* floater, const std::string& comboname)
{
	LLComboBox* combo = floater->getChild<LLComboBox>(comboname);
	combo->setCurrentByIndex(combo->getItemCount() - 1); // "custom" is always the last index
	checkAspectRatio(floater, -1); // -1 means custom
}

// Update supplied width and height according to the constrain proportions flag; limit them by max_val.
BOOL LLFloaterSnapshot::Impl::checkImageSize(LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL isWidthChanged, S32 max_value)
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

void LLFloaterSnapshot::Impl::setImageSizeSpinnersValues(LLFloaterSnapshotBase* view, S32 width, S32 height)
{
	getWidthSpinner(view)->forceSetValue(width);
	getHeightSpinner(view)->forceSetValue(height);
	if (getActiveSnapshotType(view) == LLSnapshotModel::SNAPSHOT_TEXTURE)
	{
		getWidthSpinner(view)->setIncrement(width >> 1);
		getHeightSpinner(view)->setIncrement(height >> 1);
	}
}

void LLFloaterSnapshot::Impl::updateSpinners(LLFloaterSnapshotBase* view, LLSnapshotLivePreview* previewp, S32& width, S32& height, BOOL is_width_changed)
{
	getWidthSpinner(view)->resetDirty();
	getHeightSpinner(view)->resetDirty();
	if (checkImageSize(previewp, width, height, is_width_changed, previewp->getMaxImageSize()))
	{
		setImageSizeSpinnersValues(view, width, height);
	}
}

void LLFloaterSnapshot::Impl::applyCustomResolution(LLFloaterSnapshotBase* view, S32 w, S32 h)
{
	LL_DEBUGS() << "applyCustomResolution(" << w << ", " << h << ")" << LL_ENDL;
	if (!view) return;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		S32 curw,curh;
		previewp->getSize(curw, curh);

		if (w != curw || h != curh)
		{
			//if to upload a snapshot, process spinner input in a special way.
			previewp->setMaxImageSize((S32) getWidthSpinner(view)->getMaxValue()) ;

			previewp->setSize(w,h);
			checkAutoSnapshot(previewp, FALSE);
			comboSetCustom(view, "profile_size_combo");
			comboSetCustom(view, "postcard_size_combo");
			comboSetCustom(view, "texture_size_combo");
			comboSetCustom(view, "local_size_combo");
			LL_DEBUGS() << "applied custom resolution, updating thumbnail" << LL_ENDL;
			previewp->updateSnapshot(TRUE);
		}
	}
}

// static
void LLFloaterSnapshot::Impl::onSnapshotUploadFinished(LLFloaterSnapshotBase* floater, bool status)
{
	floater->impl->setStatus(STATUS_FINISHED, status, "profile");
}

// static
void LLFloaterSnapshot::Impl::onSendingPostcardFinished(LLFloaterSnapshotBase* floater, bool status)
{
	floater->impl->setStatus(STATUS_FINISHED, status, "postcard");
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshotBase
///----------------------------------------------------------------------------

// Default constructor
LLFloaterSnapshotBase::LLFloaterSnapshotBase(const LLSD& key)
    : LLFloater(key),
	  mRefreshBtn(NULL),
	  mRefreshLabel(NULL),
	  mSucceessLblPanel(NULL),
	  mFailureLblPanel(NULL)
{
}

LLFloaterSnapshotBase::~LLFloaterSnapshotBase()
{
	if (impl->mPreviewHandle.get()) impl->mPreviewHandle.get()->die();

	//unfreeze everything else
	gSavedSettings.setBOOL("FreezeTime", FALSE);

	if (impl->mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl->mLastToolset);
	}

	delete impl;
}

///----------------------------------------------------------------------------
/// Class LLFloaterSnapshot
///----------------------------------------------------------------------------

// Default constructor
LLFloaterSnapshot::LLFloaterSnapshot(const LLSD& key)
    : LLFloaterSnapshotBase(key)
{
	impl = new Impl(this);
}

LLFloaterSnapshot::~LLFloaterSnapshot()
{
}

// virtual
BOOL LLFloaterSnapshot::postBuild()
{
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	childSetAction("new_snapshot_btn", ImplBase::onClickNewSnapshot, this);
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");

	childSetCommitCallback("ui_check", ImplBase::onClickUICheck, this);
	getChild<LLUICtrl>("ui_check")->setValue(gSavedSettings.getBOOL("RenderUIInSnapshot"));

	childSetCommitCallback("hud_check", ImplBase::onClickHUDCheck, this);
	getChild<LLUICtrl>("hud_check")->setValue(gSavedSettings.getBOOL("RenderHUDInSnapshot"));

	((Impl*)impl)->setAspectRatioCheckboxValue(this, gSavedSettings.getBOOL("KeepAspectForSnapshot"));

	childSetCommitCallback("layer_types", Impl::onCommitLayerTypes, this);
	getChild<LLUICtrl>("layer_types")->setValue("colors");
	getChildView("layer_types")->setEnabled(FALSE);

	getChild<LLUICtrl>("freeze_frame_check")->setValue(gSavedSettings.getBOOL("UseFreezeFrame"));
	childSetCommitCallback("freeze_frame_check", ImplBase::onCommitFreezeFrame, this);

	getChild<LLUICtrl>("auto_snapshot_check")->setValue(gSavedSettings.getBOOL("AutoSnapshot"));
	childSetCommitCallback("auto_snapshot_check", ImplBase::onClickAutoSnap, this);

    getChild<LLButton>("retract_btn")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onExtendFloater, this));
    getChild<LLButton>("extend_btn")->setCommitCallback(boost::bind(&LLFloaterSnapshot::onExtendFloater, this));

	// Filters
	LLComboBox* filterbox = getChild<LLComboBox>("filters_combobox");
	std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
	for (U32 i = 0; i < filter_list.size(); i++)
	{
		filterbox->add(filter_list[i]);
	}
	childSetCommitCallback("filters_combobox", ImplBase::onClickFilter, this);
    
	LLWebProfile::setImageUploadResultCallback(boost::bind(&Impl::onSnapshotUploadFinished, this, _1));
	LLPostCard::setPostResultCallback(boost::bind(&Impl::onSendingPostcardFinished, this, _1));

	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

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

	impl->mPreviewHandle = previewp->getHandle();
    previewp->setContainer(this);
	impl->updateControls(this);
	impl->setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));
	impl->updateLayout(this);
	

	previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());

	return TRUE;
}

// virtual
void LLFloaterSnapshotBase::draw()
{
	LLSnapshotLivePreview* previewp = getPreviewView();

	if (previewp && (previewp->isSnapshotActive() || previewp->getThumbnailLock()))
	{
		// don't render snapshot window in snapshot, even if "show ui" is turned on
		return;
	}

	LLFloater::draw();

	if (previewp && !isMinimized() && mThumbnailPlaceholder->getVisible())
	{		
		if(previewp->getThumbnailImage())
		{
			bool working = impl->getStatus() == ImplBase::STATUS_WORKING;
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
			mThumbnailPlaceholder->draw();
			gGL.popUIMatrix();
		}
	}
	impl->updateLayout(this);
}

//virtual
void LLFloaterSnapshot::onOpen(const LLSD& key)
{
	LLSnapshotLivePreview* preview = getPreviewView();
	if(preview)
	{
		LL_DEBUGS() << "opened, updating snapshot" << LL_ENDL;
		preview->updateSnapshot(TRUE);
	}
	focusFirstItem(FALSE);
	gSnapshotFloaterView->setEnabled(TRUE);
	gSnapshotFloaterView->setVisible(TRUE);
	gSnapshotFloaterView->adjustToFitScreen(this, FALSE);

	impl->updateControls(this);
	impl->setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));
	impl->updateLayout(this);

	// Initialize default tab.
	getChild<LLSideTrayPanelContainer>("panel_container")->getCurrentPanel()->onOpen(LLSD());
}

void LLFloaterSnapshot::onExtendFloater()
{
	impl->setAdvanced(gSavedSettings.getBOOL("AdvanceSnapshot"));
}

//virtual
void LLFloaterSnapshotBase::onClose(bool app_quitting)
{
	getParent()->setMouseOpaque(FALSE);

	//unfreeze everything, hide fullscreen preview
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->setVisible(FALSE);
		previewp->setEnabled(FALSE);
	}

	gSavedSettings.setBOOL("FreezeTime", FALSE);
	impl->mAvatarPauseHandles.clear();

	if (impl->mLastToolset)
	{
		LLToolMgr::getInstance()->setCurrentToolset(impl->mLastToolset);
	}
}

// virtual
S32 LLFloaterSnapshotBase::notify(const LLSD& info)
{
	if (info.has("set-ready"))
	{
		impl->setStatus(ImplBase::STATUS_READY);
		return 1;
	}

	if (info.has("set-working"))
	{
		impl->setStatus(ImplBase::STATUS_WORKING);
		return 1;
	}

	if (info.has("set-finished"))
	{
		LLSD data = info["set-finished"];
		impl->setStatus(ImplBase::STATUS_FINISHED, data["ok"].asBoolean(), data["msg"].asString());
		return 1;
	}

	if (info.has("snapshot-updating"))
	{
		// Disable the send/post/save buttons until snapshot is ready.
		impl->updateControls(this);
		return 1;
	}

	if (info.has("snapshot-updated"))
	{
		// Enable the send/post/save buttons.
		impl->updateControls(this);
		// We've just done refresh.
		impl->setNeedRefresh(false);

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

// virtual
S32 LLFloaterSnapshot::notify(const LLSD& info)
{
	bool res = LLFloaterSnapshotBase::notify(info);
	if (res)
		return res;
	// A child panel wants to change snapshot resolution.
	if (info.has("combo-res-change"))
	{
		std::string combo_name = info["combo-res-change"]["control-name"].asString();
		((Impl*)impl)->updateResolution(getChild<LLUICtrl>(combo_name), this);
		return 1;
	}

	if (info.has("custom-res-change"))
	{
		LLSD res = info["custom-res-change"];
		((Impl*)impl)->applyCustomResolution(this, res["w"].asInteger(), res["h"].asInteger());
		return 1;
	}

	if (info.has("keep-aspect-change"))
	{
		((Impl*)impl)->applyKeepAspectCheck(this, info["keep-aspect-change"].asBoolean());
		return 1;
	}

	if (info.has("image-quality-change"))
	{
		((Impl*)impl)->onImageQualityChange(this, info["image-quality-change"].asInteger());
		return 1;
	}

	if (info.has("image-format-change"))
	{
		((Impl*)impl)->onImageFormatChange(this);
		return 1;
	}
    
	return 0;
}

BOOL LLFloaterSnapshotBase::ImplBase::updatePreviewList(bool initialized)
{
	LLFloaterFacebook* floater_facebook = LLFloaterReg::findTypedInstance<LLFloaterFacebook>("facebook");
	LLFloaterFlickr* floater_flickr = LLFloaterReg::findTypedInstance<LLFloaterFlickr>("flickr");
	LLFloaterTwitter* floater_twitter = LLFloaterReg::findTypedInstance<LLFloaterTwitter>("twitter");

	if (!initialized && !floater_facebook && !floater_flickr && !floater_twitter)
		return FALSE;

	BOOL changed = FALSE;
	LL_DEBUGS() << "npreviews: " << LLSnapshotLivePreview::sList.size() << LL_ENDL;
	for (std::set<LLSnapshotLivePreview*>::iterator iter = LLSnapshotLivePreview::sList.begin();
		iter != LLSnapshotLivePreview::sList.end(); ++iter)
	{
		changed |= LLSnapshotLivePreview::onIdle(*iter);
	}
	return changed;
}


void LLFloaterSnapshotBase::ImplBase::updateLivePreview()
{
	if (ImplBase::updatePreviewList(true) && mFloater)
	{
		LL_DEBUGS() << "changed" << LL_ENDL;
		updateControls(mFloater);
	}
}

//static 
void LLFloaterSnapshot::update()
{
	LLFloaterSnapshot* inst = findInstance();
	if (inst != NULL)
	{
		inst->impl->updateLivePreview();
	}
	else
	{
		ImplBase::updatePreviewList(false);
	}
}

// static
LLFloaterSnapshot* LLFloaterSnapshot::findInstance()
{
	return LLFloaterReg::findTypedInstance<LLFloaterSnapshot>("snapshot");
}

// static
LLFloaterSnapshot* LLFloaterSnapshot::getInstance()
{
	return LLFloaterReg::getTypedInstance<LLFloaterSnapshot>("snapshot");
}

// virtual
void LLFloaterSnapshot::saveTexture()
{
	LL_DEBUGS() << "saveTexture" << LL_ENDL;

	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return;
	}

	previewp->saveTexture();
}

BOOL LLFloaterSnapshot::saveLocal()
{
	LL_DEBUGS() << "saveLocal" << LL_ENDL;
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return FALSE;
	}

	return previewp->saveLocal();
}

void LLFloaterSnapshotBase::postSave()
{
	impl->updateControls(this);
	impl->setStatus(ImplBase::STATUS_WORKING);
}

// virtual
void LLFloaterSnapshotBase::postPanelSwitch()
{
	impl->updateControls(this);

	// Remove the success/failure indicator whenever user presses a snapshot option button.
	impl->setStatus(ImplBase::STATUS_READY);
}

void LLFloaterSnapshotBase::inventorySaveFailed()
{
    impl->updateControls(this);
    impl->setStatus(ImplBase::STATUS_FINISHED, false, "inventory");
}

LLPointer<LLImageFormatted> LLFloaterSnapshotBase::getImageData()
{
	// FIXME: May not work for textures.

	LLSnapshotLivePreview* previewp = getPreviewView();
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

const LLVector3d& LLFloaterSnapshotBase::getPosTakenGlobal()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (!previewp)
	{
		llassert(previewp != NULL);
		return LLVector3d::zero;
	}

	return previewp->getPosTakenGlobal();
}

// static
void LLFloaterSnapshot::setAgentEmail(const std::string& email)
{
	LLFloaterSnapshot* instance = findInstance();
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

LLSnapshotFloaterView::LLSnapshotFloaterView (const Params& p) : LLFloaterView (p)
{
}

LLSnapshotFloaterView::~LLSnapshotFloaterView()
{
}

// virtual
BOOL LLSnapshotFloaterView::handleKey(KEY key, MASK mask, BOOL called_from_parent)
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

// virtual
BOOL LLSnapshotFloaterView::handleMouseDown(S32 x, S32 y, MASK mask)
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

// virtual
BOOL LLSnapshotFloaterView::handleMouseUp(S32 x, S32 y, MASK mask)
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

// virtual
BOOL LLSnapshotFloaterView::handleHover(S32 x, S32 y, MASK mask)
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
