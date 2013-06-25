/** 
* @file llfloatersocial.cpp
* @brief Implementation of llfloatersocial
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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

#include "llfloatersocial.h"

#include "llagent.h"
#include "llagentui.h"
#include "llfacebookconnect.h"
#include "llfloaterreg.h"
#include "lliconctrl.h"
#include "llloadingindicator.h"
#include "llslurl.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"

static LLRegisterPanelClassWrapper<LLSocialStatusPanel> t_panel_status("llsocialstatuspanel");
static LLRegisterPanelClassWrapper<LLSocialPhotoPanel> t_panel_photo("llsocialphotopanel");
static LLRegisterPanelClassWrapper<LLSocialCheckinPanel> t_panel_checkin("llsocialcheckinpanel");

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const std::string DEFAULT_CHECKIN_ICON_URL = "http://logok.org/wp-content/uploads/2010/07/podcastlogo1.jpg";

std::string get_map_url()
{
    LLVector3d center_agent;
    if (gAgent.getRegion())
    {
        center_agent = gAgent.getRegion()->getCenterGlobal();
    }
    int x_pos = center_agent[0] / 256.0;
    int y_pos = center_agent[1] / 256.0;
    std::string map_url = gSavedSettings.getString("CurrentMapServerURL") + llformat("map-1-%d-%d-objects.jpg", x_pos, y_pos);
    return map_url;
}

LLSocialStatusPanel::LLSocialStatusPanel() :
	mMessageTextEditor(NULL),
	mPostStatusButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendStatus", boost::bind(&LLSocialStatusPanel::onSend, this));
}

BOOL LLSocialStatusPanel::postBuild()
{
	mMessageTextEditor = getChild<LLUICtrl>("status_message");
	mPostStatusButton = getChild<LLUICtrl>("post_status_btn");

	return LLPanel::postBuild();
}

void LLSocialStatusPanel::draw()
{
	if (mMessageTextEditor && mPostStatusButton)
	{
		std::string message = mMessageTextEditor->getValue().asString();
		mPostStatusButton->setEnabled(!message.empty());
	}

	LLPanel::draw();
}

void LLSocialStatusPanel::onSend()
{
	if (mMessageTextEditor)
	{
		std::string message = mMessageTextEditor->getValue().asString();
		if (!message.empty())
		{
			LLFacebookConnect::instance().updateStatus(message);
	
			LLFloater* floater = getParentByType<LLFloater>();
			if (floater)
			{
				floater->closeFloater();
			}
		}
	}
}

LLSocialPhotoPanel::LLSocialPhotoPanel() :
mRefreshBtn(NULL),
mRefreshLabel(NULL),
mSucceessLblPanel(NULL),
mFailureLblPanel(NULL),
mThumbnailPlaceholder(NULL)
{
	mCommitCallbackRegistrar.add("PostToFacebook.Send", boost::bind(&LLSocialPhotoPanel::onSend, this));
}

LLSocialPhotoPanel::~LLSocialPhotoPanel()
{
	if(mPreviewHandle.get())
	{
		mPreviewHandle.get()->die();
	}
}

BOOL LLSocialPhotoPanel::postBuild()
{
	mResolutionComboBox = getChild<LLUICtrl>("resolution_combobox");
	mResolutionComboBox->setCommitCallback(boost::bind(&LLSocialPhotoPanel::onResolutionComboCommit, this));
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	childSetAction("new_snapshot_btn", boost::bind(&LLSocialPhotoPanel::onClickNewSnapshot, this));
	mRefreshLabel = getChild<LLUICtrl>("refresh_lbl");
	mSucceessLblPanel = getChild<LLUICtrl>("succeeded_panel");
	mFailureLblPanel = getChild<LLUICtrl>("failed_panel");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");

	LLRect full_screen_rect = getRootView()->getRect();
	LLSnapshotLivePreview::Params p;
	p.rect(full_screen_rect);
	LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
	mPreviewHandle = previewp->getHandle();	

	previewp->setThumbnailPlaceholderRect(getThumbnailPlaceholderRect());

	return LLPanel::postBuild();
}

void LLSocialPhotoPanel::onResolutionComboCommit()
{
	LLFloaterSocial* instance = LLFloaterReg::findTypedInstance<LLFloaterSocial>("social");
	updateResolution(mResolutionComboBox, instance); 
}

void LLSocialPhotoPanel::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = static_cast<LLSnapshotLivePreview*>(mPreviewHandle.get());
	//LLFloaterSnapshot *view = (LLFloaterSnapshot *)data;
	if (previewp /*&& view*/)
	{
		//view->impl.setStatus(Impl::STATUS_READY);
		lldebugs << "updating snapshot" << llendl;
		previewp->updateSnapshot(TRUE);
	}
}

void LLSocialPhotoPanel::updateResolution(LLUICtrl* ctrl, void* data, BOOL do_update)
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

	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;

		if (width == 0 || height == 0)
		{
			// take resolution from current window size
			lldebugs << "Setting preview res from window: " << gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << llendl;
			previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
		}
		else
		{
			// use the resolution from the selected pre-canned drop-down choice
			lldebugs << "Setting preview res selected from combo: " << width << "x" << height << llendl;
			previewp->setSize(width, height);
		}

		checkAspectRatio(view, width) ;

		previewp->getSize(width, height);
		
		if(original_width != width || original_height != height)
		{
			previewp->setSize(width, height);

			// hide old preview as the aspect ratio could be wrong
			lldebugs << "updating thumbnail" << llendl;
			
			previewp->updateSnapshot(FALSE, TRUE);
			if(do_update)
			{
				lldebugs << "Will update controls" << llendl;
				updateControls();
				setNeedRefresh(true);
			}
		}
		
	}
}

void LLSocialPhotoPanel::setNeedRefresh(bool need)
{
	mRefreshLabel->setVisible(need);
	mNeedRefresh = need;
}

void LLSocialPhotoPanel::checkAspectRatio(LLFloaterSnapshot *view, S32 index)
{
	LLSnapshotLivePreview *previewp = getPreviewView() ;

	BOOL keep_aspect = FALSE;

	if (0 == index) // current window size
	{
		keep_aspect = TRUE;
	}
	else // predefined resolution
	{
		keep_aspect = FALSE;
	}

	if (previewp)
	{
		previewp->mKeepAspectRatio = keep_aspect;
	}
}

LLSnapshotLivePreview* LLSocialPhotoPanel::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}


void LLSocialPhotoPanel::updateControls()
{


	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_bytes = previewp && previewp->getDataSize() > 0;
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();

	// *TODO: Separate maximum size for Web images from postcards
	lldebugs << "Is snapshot up-to-date? " << got_snap << llendl;

	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_snap)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}

	//getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : getString("unknown"));
	getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : "unknown");
	getChild<LLUICtrl>("file_size_label")->setColor(
		true 
		&& got_bytes
		&& previewp->getDataSize() > MAX_POSTCARD_DATASIZE ? LLUIColor(LLColor4::red) : LLUIColorTable::instance().getColor( "LabelTextColor" ));

	LLComboBox* combo = getChild<LLComboBox>("resolution_combobox");
	LLFloaterSocial* instance = LLFloaterReg::findTypedInstance<LLFloaterSocial>("social");
	updateResolution(combo, instance, FALSE);
}

void LLSocialPhotoPanel::draw()
{ 
	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());

	LLPanel::draw();

	if(previewp && previewp->getThumbnailImage())
	{
		bool working = false; //impl.getStatus() == Impl::STATUS_WORKING;
		const LLRect& thumbnail_rect = getThumbnailPlaceholderRect();
		const S32 thumbnail_w = previewp->getThumbnailWidth();
		const S32 thumbnail_h = previewp->getThumbnailHeight();

		// calc preview offset within the preview rect
		const S32 local_offset_x = (thumbnail_rect.getWidth() - thumbnail_w) / 2 ;
		const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2 ; // preview y pos within the preview rect

		// calc preview offset within the floater rect
		S32 offset_x = thumbnail_rect.mLeft + local_offset_x;
		S32 offset_y = thumbnail_rect.mBottom + local_offset_y;

		LLUICtrl * snapshot_panel = getChild<LLUICtrl>("snapshot_panel");
		snapshot_panel->localPointToOtherView(offset_x, offset_y, &offset_x, &offset_y, gFloaterView->getParentFloater(this));

		gGL.matrixMode(LLRender::MM_MODELVIEW);
		// Apply floater transparency to the texture unless the floater is focused.
		F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
		LLColor4 color = working ? LLColor4::grey4 : LLColor4::white;
		gl_draw_scaled_image(offset_x, offset_y, 
			thumbnail_w, thumbnail_h,
			previewp->getThumbnailImage(), color % alpha);

		previewp->drawPreviewRect(offset_x, offset_y) ;

		// Draw some controls on top of the preview thumbnail.
		static const S32 PADDING = 5;
		static const S32 REFRESH_LBL_BG_HEIGHT = 32;

		// Reshape and position the posting result message panels at the top of the thumbnail.
		// Do this regardless of current posting status (finished or not) to avoid flicker
		// when the result message is displayed for the first time.
		// if (impl.getStatus() == Impl::STATUS_FINISHED)
		{
			LLRect result_lbl_rect = mSucceessLblPanel->getRect();
			const S32 result_lbl_h = result_lbl_rect.getHeight();
			result_lbl_rect.setLeftTopAndSize(local_offset_x, local_offset_y + thumbnail_h, thumbnail_w - 1, result_lbl_h);
			mSucceessLblPanel->reshape(result_lbl_rect.getWidth(), result_lbl_h);
			mSucceessLblPanel->setRect(result_lbl_rect);
			mFailureLblPanel->reshape(result_lbl_rect.getWidth(), result_lbl_h);
			mFailureLblPanel->setRect(result_lbl_rect);
		}

		// Position the refresh button in the bottom left corner of the thumbnail.
		mRefreshBtn->setOrigin(local_offset_x + PADDING, local_offset_y + PADDING);

		if (mNeedRefresh)
		{
			// Place the refresh hint text to the right of the refresh button.
			const LLRect& refresh_btn_rect = mRefreshBtn->getRect();
			mRefreshLabel->setOrigin(refresh_btn_rect.mLeft + refresh_btn_rect.getWidth() + PADDING, refresh_btn_rect.mBottom);

			// Draw the refresh hint background.
			LLRect refresh_label_bg_rect(offset_x, offset_y + REFRESH_LBL_BG_HEIGHT, offset_x + thumbnail_w - 1, offset_y);
			gl_rect_2d(refresh_label_bg_rect, LLColor4::white % 0.9f, TRUE);
		}

		gGL.pushUIMatrix();
		S32 x_pos;
		S32 y_pos;
		snapshot_panel->localPointToOtherView(thumbnail_rect.mLeft, thumbnail_rect.mBottom, &x_pos, &y_pos, gFloaterView->getParentFloater(this));
		
		LLUI::translate((F32) x_pos, (F32) y_pos);
		mThumbnailPlaceholder->draw();
		gGL.popUIMatrix();
	}
}

void LLSocialPhotoPanel::onSend()
{
	std::string caption = getChild<LLUICtrl>("caption")->getValue().asString();
	bool add_location = getChild<LLUICtrl>("add_location_cb")->getValue().asBoolean();

	if (add_location)
	{
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		if (caption.empty())
			caption = slurl.getSLURLString();
		else
			caption = caption + " " + slurl.getSLURLString();
	}

	LLSnapshotLivePreview* previewp = getPreviewView();

	LLFacebookConnect::instance().sharePhoto(previewp->getFormattedImage(), caption);
	updateControls();

	// Close the floater once "Post" has been pushed
	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}


LLSocialCheckinPanel::LLSocialCheckinPanel() :
    mMapUrl(""),
    mReloadingMapTexture(false)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendCheckin", boost::bind(&LLSocialCheckinPanel::onSend, this));
}

BOOL LLSocialCheckinPanel::postBuild()
{
    // Keep pointers to widgets so we don't traverse the UI hierarchy too often
    mMapLoadingIndicator = getChild<LLUICtrl>("map_loading_indicator");
    mMapPlaceholder = getChild<LLIconCtrl>("map_placeholder");
    mMapCheckBox = getChild<LLCheckBoxCtrl>("add_place_view_cb");
    mMapCheckBoxValue = mMapCheckBox->get();
    
	return LLPanel::postBuild();
}

void LLSocialCheckinPanel::draw()
{
    std::string map_url = get_map_url();
    // Did we change location?
    if (map_url != mMapUrl)
    {
        mMapUrl = map_url;
        // Load the map tile
        mMapTexture = LLViewerTextureManager::getFetchedTextureFromUrl(mMapUrl, FTT_MAP_TILE, TRUE, LLGLTexture::BOOST_NONE, LLViewerTexture::LOD_TEXTURE);
        mMapTexture->setBoostLevel(LLGLTexture::BOOST_MAP);
        mReloadingMapTexture = true;
        // In the meantime, put the "loading" indicator on, hide the tile map and disable the checkbox
        mMapLoadingIndicator->setVisible(true);
        mMapPlaceholder->setVisible(false);
        mMapCheckBoxValue = mMapCheckBox->get();
        mMapCheckBox->set(false);
        mMapCheckBox->setEnabled(false);
    }
    // Are we done loading the map tile?
    if (mReloadingMapTexture && mMapTexture->isFullyLoaded())
    {
        // Don't do it again next time around
        mReloadingMapTexture = false;
        // Convert the map texture to the appropriate image object
        LLPointer<LLUIImage> ui_image = new LLUIImage(mMapUrl, mMapTexture);
        // Load the map widget with the correct map tile image
        mMapPlaceholder->setImage(ui_image);
        // Now hide the loading indicator, bring the tile in view and reenable the checkbox with its previous value
        mMapLoadingIndicator->setVisible(false);
        mMapPlaceholder->setVisible(true);
        mMapCheckBox->setEnabled(true);
        mMapCheckBox->set(mMapCheckBoxValue);
    }
    
	LLPanel::draw();
}

void LLSocialCheckinPanel::onSend()
{
	// Get the location SLURL
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	std::string slurl_string = slurl.getSLURLString();
    
	// Get the region name
	std::string region_name = gAgent.getRegion()->getName();
    
	// Get the region description
	std::string description;
	LLAgentUI::buildLocationString(description, LLAgentUI::LOCATION_FORMAT_NORMAL_COORDS, gAgent.getPositionAgent());
    
	// Optionally add the region map view
	bool add_map_view = getChild<LLUICtrl>("add_place_view_cb")->getValue().asBoolean();
    std::string map_url = (add_map_view ? get_map_url() : DEFAULT_CHECKIN_ICON_URL);
    
	// Get the caption
	std::string caption = getChild<LLUICtrl>("place_caption")->getValue().asString();

    // Post all that to Facebook
	LLFacebookConnect::instance().postCheckin(slurl_string, region_name, description, map_url, caption);
    
    // Close the floater once "Post" has been pushed
	LLFloater* floater = getParentByType<LLFloater>();
    if (floater)
    {
        floater->closeFloater();
    }
}


LLFloaterSocial::LLFloaterSocial(const LLSD& key) : LLFloater(key),
    mSocialPhotoPanel(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterSocial::onCancel, this));
}

void LLFloaterSocial::onCancel()
{
    closeFloater();
}

BOOL LLFloaterSocial::postBuild()
{
	mSocialPhotoPanel = static_cast<LLSocialPhotoPanel*>(getChild<LLUICtrl>("social_photo_tab"));
	return LLFloater::postBuild();
}

/*virtual*/
void LLFloaterSocial::draw()
{
    LLFloater::draw();
}

void LLFloaterSocial::onOpen(const LLSD& key)
{
	LLSnapshotLivePreview* preview = static_cast<LLSnapshotLivePreview *>(mSocialPhotoPanel->mPreviewHandle.get());
	if(preview)
	{
		lldebugs << "opened, updating snapshot" << llendl;
		preview->updateSnapshot(TRUE);
	}
}

// static
void LLFloaterSocial::preUpdate()
{
	// FIXME: duplicated code
	LLFloaterSocial* instance = LLFloaterReg::findTypedInstance<LLFloaterSocial>("social");
	if (instance)
	{
		// Disable the send/post/save buttons until snapshot is ready.
		instance->mSocialPhotoPanel->updateControls();

		// Force hiding the "Refresh to save" hint because we know we've just started refresh.
		instance->mSocialPhotoPanel->setNeedRefresh(false);
	}
}

// static
void LLFloaterSocial::postUpdate()
{
	// FIXME: duplicated code
	LLFloaterSocial* instance = LLFloaterReg::findTypedInstance<LLFloaterSocial>("social");
	if (instance)
	{
		// Enable the send/post/save buttons.
		instance->mSocialPhotoPanel->updateControls();

		// We've just done refresh.
		instance->mSocialPhotoPanel->setNeedRefresh(false);

		// The refresh button is initially hidden. We show it after the first update,
		// i.e. when preview appears.
		if (!instance->mSocialPhotoPanel->mRefreshBtn->getVisible())
		{
			instance->mSocialPhotoPanel->mRefreshBtn->setVisible(true);
		}
		
	}
}
