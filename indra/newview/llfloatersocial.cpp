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
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llfacebookconnect.h"
#include "llfloaterreg.h"
#include "lliconctrl.h"
#include "llresmgr.h"		// LLLocale
#include "llsdserialize.h"
#include "llloadingindicator.h"
#include "llplugincookiestore.h"
#include "llslurl.h"
#include "lltrans.h"
#include "llsnapshotlivepreview.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"

static LLRegisterPanelClassWrapper<LLSocialStatusPanel> t_panel_status("llsocialstatuspanel");
static LLRegisterPanelClassWrapper<LLSocialPhotoPanel> t_panel_photo("llsocialphotopanel");
static LLRegisterPanelClassWrapper<LLSocialCheckinPanel> t_panel_checkin("llsocialcheckinpanel");
static LLRegisterPanelClassWrapper<LLSocialAccountPanel> t_panel_account("llsocialaccountpanel");

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const std::string DEFAULT_CHECKIN_LOCATION_URL = "http://maps.secondlife.com/";
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

///////////////////////////
//LLSocialStatusPanel//////
///////////////////////////

LLSocialStatusPanel::LLSocialStatusPanel() :
	mMessageTextEditor(NULL),
	mPostButton(NULL),
    mCancelButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendStatus", boost::bind(&LLSocialStatusPanel::onSend, this));
}

BOOL LLSocialStatusPanel::postBuild()
{
	mMessageTextEditor = getChild<LLUICtrl>("status_message");
	mPostButton = getChild<LLUICtrl>("post_status_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_status_btn");

	return LLPanel::postBuild();
}

void LLSocialStatusPanel::draw()
{
    if (mMessageTextEditor && mPostButton && mCancelButton)
	{
        bool no_ongoing_connection = !(LLFacebookConnect::instance().isTransactionOngoing());
        std::string message = mMessageTextEditor->getValue().asString();
        mMessageTextEditor->setEnabled(no_ongoing_connection);
        mCancelButton->setEnabled(no_ongoing_connection);
        mPostButton->setEnabled(no_ongoing_connection && !message.empty());
    }

	LLPanel::draw();
}

void LLSocialStatusPanel::onSend()
{
	LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialStatusPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FacebookConnectState").listen("LLSocialStatusPanel", boost::bind(&LLSocialStatusPanel::onFacebookConnectStateChange, this, _1));
		
	// Connect to Facebook if necessary and then post
	if (LLFacebookConnect::instance().isConnected())
	{
		sendStatus();
	}
	else
	{
		LLFacebookConnect::instance().checkConnectionToFacebook(true);
	}
}

bool LLSocialStatusPanel::onFacebookConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFacebookConnect::FB_CONNECTED:
			sendStatus();
			break;

		case LLFacebookConnect::FB_POSTED:
			LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialStatusPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLSocialStatusPanel::sendStatus()
{
	std::string message = mMessageTextEditor->getValue().asString();
	if (!message.empty())
	{
		LLFacebookConnect::instance().updateStatus(message);
	}
}

void LLSocialStatusPanel::clearAndClose()
{
	mMessageTextEditor->setValue("");

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

///////////////////////////
//LLSocialPhotoPanel///////
///////////////////////////

LLSocialPhotoPanel::LLSocialPhotoPanel() :
mSnapshotPanel(NULL),
mResolutionComboBox(NULL),
mRefreshBtn(NULL),
mWorkingLabel(NULL),
mThumbnailPlaceholder(NULL),
mCaptionTextBox(NULL),
mLocationCheckbox(NULL),
mPostButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendPhoto", boost::bind(&LLSocialPhotoPanel::onSend, this));
	mCommitCallbackRegistrar.add("SocialSharing.RefreshPhoto", boost::bind(&LLSocialPhotoPanel::onClickNewSnapshot, this));
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
	setVisibleCallback(boost::bind(&LLSocialPhotoPanel::onVisibilityChange, this, _2));
	
	mSnapshotPanel = getChild<LLUICtrl>("snapshot_panel");
	mResolutionComboBox = getChild<LLUICtrl>("resolution_combobox");
	mResolutionComboBox->setCommitCallback(boost::bind(&LLSocialPhotoPanel::updateResolution, this, TRUE));
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
    mWorkingLabel = getChild<LLUICtrl>("working_lbl");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");
	mCaptionTextBox = getChild<LLUICtrl>("photo_caption");
	mLocationCheckbox = getChild<LLUICtrl>("add_location_cb");
	mPostButton = getChild<LLUICtrl>("post_photo_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_photo_btn");

	return LLPanel::postBuild();
}

void LLSocialPhotoPanel::draw()
{ 
	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());

    // Enable interaction only if no transaction with the service is on-going (prevent duplicated posts)
    bool no_ongoing_connection = !(LLFacebookConnect::instance().isTransactionOngoing());
    mCancelButton->setEnabled(no_ongoing_connection);
    mCaptionTextBox->setEnabled(no_ongoing_connection);
    mResolutionComboBox->setEnabled(no_ongoing_connection);
    mRefreshBtn->setEnabled(no_ongoing_connection);
    mLocationCheckbox->setEnabled(no_ongoing_connection);
    
    // Display the preview if one is available
	if (previewp && previewp->getThumbnailImage())
	{
		const LLRect& thumbnail_rect = mThumbnailPlaceholder->getRect();
		const S32 thumbnail_w = previewp->getThumbnailWidth();
		const S32 thumbnail_h = previewp->getThumbnailHeight();

		// calc preview offset within the preview rect
		const S32 local_offset_x = (thumbnail_rect.getWidth()  - thumbnail_w) / 2 ;
		const S32 local_offset_y = (thumbnail_rect.getHeight() - thumbnail_h) / 2 ;

		// calc preview offset within the floater rect
        // Hack : To get the full offset, we need to take into account each and every offset of each widgets up to the floater.
        // This is almost as arbitrary as using a fixed offset so that's what we do here for the sake of simplicity.
        // *TODO : Get the offset looking through the hierarchy of widgets, should be done in postBuild() so to avoid traversing the hierarchy each time.
		S32 offset_x = thumbnail_rect.mLeft + local_offset_x - 1;
		S32 offset_y = thumbnail_rect.mBottom + local_offset_y - 39;
        
		mSnapshotPanel->localPointToOtherView(offset_x, offset_y, &offset_x, &offset_y, getParentByType<LLFloater>());
        
		gGL.matrixMode(LLRender::MM_MODELVIEW);
		// Apply floater transparency to the texture unless the floater is focused.
		F32 alpha = getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency();
		LLColor4 color = LLColor4::white;
		gl_draw_scaled_image(offset_x, offset_y, 
			thumbnail_w, thumbnail_h,
			previewp->getThumbnailImage(), color % alpha);

		previewp->drawPreviewRect(offset_x, offset_y) ;
	}

    // Update the visibility of the working (computing preview) label
    mWorkingLabel->setVisible(!(previewp && previewp->getSnapshotUpToDate()));
    
    // Enable Post if we have a preview to send and no on going connection being processed
    mPostButton->setEnabled(no_ongoing_connection && (previewp && previewp->getSnapshotUpToDate()));
    
    // Draw the rest of the panel on top of it
	LLPanel::draw();
}

LLSnapshotLivePreview* LLSocialPhotoPanel::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}

void LLSocialPhotoPanel::onVisibilityChange(const LLSD& new_visibility)
{
	bool visible = new_visibility.asBoolean();
	if (visible)
	{
		if (mPreviewHandle.get())
		{
			LLSnapshotLivePreview* preview = getPreviewView();
			if(preview)
			{
				lldebugs << "opened, updating snapshot" << llendl;
				preview->updateSnapshot(TRUE);
			}
		}
		else
		{
			LLRect full_screen_rect = getRootView()->getRect();
			LLSnapshotLivePreview::Params p;
			p.rect(full_screen_rect);
			LLSnapshotLivePreview* previewp = new LLSnapshotLivePreview(p);
			mPreviewHandle = previewp->getHandle();	

			previewp->setSnapshotType(previewp->SNAPSHOT_WEB);
			previewp->setSnapshotFormat(LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG);
			//previewp->setSnapshotQuality(98);
			previewp->setThumbnailPlaceholderRect(mThumbnailPlaceholder->getRect());

			updateControls();
		}
	}
}

void LLSocialPhotoPanel::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		//setStatus(Impl::STATUS_READY);
		lldebugs << "updating snapshot" << llendl;
		previewp->updateSnapshot(TRUE);
	}
}

void LLSocialPhotoPanel::onSend()
{
	LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialPhotoPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FacebookConnectState").listen("LLSocialPhotoPanel", boost::bind(&LLSocialPhotoPanel::onFacebookConnectStateChange, this, _1));
	
	// Connect to Facebook if necessary and then post
	if (LLFacebookConnect::instance().isConnected())
	{
		sendPhoto();
	}
	else
	{
		LLFacebookConnect::instance().checkConnectionToFacebook(true);
	}
}

bool LLSocialPhotoPanel::onFacebookConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFacebookConnect::FB_CONNECTED:
			sendPhoto();
			break;

		case LLFacebookConnect::FB_POSTED:
			LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialPhotoPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLSocialPhotoPanel::sendPhoto()
{
	// Get the caption
	std::string caption = mCaptionTextBox->getValue().asString();

	// Add the location if required
	bool add_location = mLocationCheckbox->getValue().asBoolean();
	if (add_location)
	{
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		if (caption.empty())
			caption = slurl.getSLURLString();
		else
			caption = caption + " " + slurl.getSLURLString();
	}

	// Get the image
	LLSnapshotLivePreview* previewp = getPreviewView();
	
	// Post to Facebook
	LLFacebookConnect::instance().sharePhoto(previewp->getFormattedImage(), caption);

	updateControls();
}

void LLSocialPhotoPanel::clearAndClose()
{
	mCaptionTextBox->setValue("");

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

void LLSocialPhotoPanel::updateControls()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_bytes = previewp && previewp->getDataSize() > 0;
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();
	LLSnapshotLivePreview::ESnapshotType shot_type = (previewp ? previewp->getSnapshotType() : LLSnapshotLivePreview::SNAPSHOT_POSTCARD);

	// *TODO: Separate maximum size for Web images from postcards
	lldebugs << "Is snapshot up-to-date? " << got_snap << llendl;

	LLLocale locale(LLLocale::USER_LOCALE);
	std::string bytes_string;
	if (got_snap)
	{
		LLResMgr::getInstance()->getIntegerString(bytes_string, (previewp->getDataSize()) >> 10 );
	}

	//getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : getString("unknown")); <---uses localized string
	getChild<LLUICtrl>("file_size_label")->setTextArg("[SIZE]", got_snap ? bytes_string : "unknown");
	getChild<LLUICtrl>("file_size_label")->setColor(
		shot_type == LLSnapshotLivePreview::SNAPSHOT_POSTCARD 
		&& got_bytes
		&& previewp->getDataSize() > MAX_POSTCARD_DATASIZE ? LLUIColor(LLColor4::red) : LLUIColorTable::instance().getColor( "LabelTextColor" ));

	updateResolution(FALSE);
}

void LLSocialPhotoPanel::updateResolution(BOOL do_update)
{
	LLComboBox* combobox = static_cast<LLComboBox *>(mResolutionComboBox);

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

		checkAspectRatio(width);

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
                LLSocialPhotoPanel::onClickNewSnapshot();
			}
		}
		
	}
}

void LLSocialPhotoPanel::checkAspectRatio(S32 index)
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

LLUICtrl* LLSocialPhotoPanel::getRefreshBtn()
{
	return mRefreshBtn;
}

////////////////////////
//LLSocialCheckinPanel//
////////////////////////

LLSocialCheckinPanel::LLSocialCheckinPanel() :
    mMapUrl(""),
    mReloadingMapTexture(false)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendCheckin", boost::bind(&LLSocialCheckinPanel::onSend, this));
}

BOOL LLSocialCheckinPanel::postBuild()
{
    // Keep pointers to widgets so we don't traverse the UI hierarchy too often
	mPostButton = getChild<LLUICtrl>("post_place_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_place_btn");
	mMessageTextEditor = getChild<LLUICtrl>("place_caption");
    mMapLoadingIndicator = getChild<LLUICtrl>("map_loading_indicator");
    mMapPlaceholder = getChild<LLIconCtrl>("map_placeholder");
    mMapCheckBox = getChild<LLCheckBoxCtrl>("add_place_view_cb");
    mMapCheckBoxValue = mMapCheckBox->get();
    
	return LLPanel::postBuild();
}

void LLSocialCheckinPanel::draw()
{
    bool no_ongoing_connection = !(LLFacebookConnect::instance().isTransactionOngoing());
    mPostButton->setEnabled(no_ongoing_connection);
    mCancelButton->setEnabled(no_ongoing_connection);
    mMessageTextEditor->setEnabled(no_ongoing_connection);

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
        mMapCheckBox->setEnabled(no_ongoing_connection);
        mMapCheckBox->set(mMapCheckBoxValue);
    }

	LLPanel::draw();
}

void LLSocialCheckinPanel::onSend()
{
	LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialCheckinPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FacebookConnectState").listen("LLSocialCheckinPanel", boost::bind(&LLSocialCheckinPanel::onFacebookConnectStateChange, this, _1));
	
	// Connect to Facebook if necessary and then post
	if (LLFacebookConnect::instance().isConnected())
	{
		sendCheckin();
	}
	else
	{
		LLFacebookConnect::instance().checkConnectionToFacebook(true);
	}
}

bool LLSocialCheckinPanel::onFacebookConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFacebookConnect::FB_CONNECTED:
			sendCheckin();
			break;

		case LLFacebookConnect::FB_POSTED:
			LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialCheckinPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLSocialCheckinPanel::sendCheckin()
{
	// Get the location SLURL
	LLSLURL slurl;
	LLAgentUI::buildSLURL(slurl);
	std::string slurl_string = slurl.getSLURLString();

	// Use a valid http:// URL if the scheme is secondlife:// 
	LLURI slurl_uri(slurl_string);
	if (slurl_uri.scheme() == LLSLURL::SLURL_SECONDLIFE_SCHEME)
	{
		slurl_string = DEFAULT_CHECKIN_LOCATION_URL;
	}
    
	// Get the region name
	std::string region_name = gAgent.getRegion()->getName();
    
	// Get the region description
	std::string description;
	LLAgentUI::buildLocationString(description, LLAgentUI::LOCATION_FORMAT_NORMAL_COORDS, gAgent.getPositionAgent());
    
	// Optionally add the region map view
	bool add_map_view = mMapCheckBox->getValue().asBoolean();
    std::string map_url = (add_map_view ? get_map_url() : DEFAULT_CHECKIN_ICON_URL);
    
	// Get the caption
	std::string caption = mMessageTextEditor->getValue().asString();

	// Post to Facebook
	LLFacebookConnect::instance().postCheckin(slurl_string, region_name, description, map_url, caption);
}

void LLSocialCheckinPanel::clearAndClose()
{
	mMessageTextEditor->setValue("");

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

///////////////////////////
//LLSocialAccountPanel//////
///////////////////////////

LLSocialAccountPanel::LLSocialAccountPanel() : 
mAccountCaptionLabel(NULL),
mAccountNameLabel(NULL),
mPanelButtons(NULL),
mConnectButton(NULL),
mDisconnectButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Connect", boost::bind(&LLSocialAccountPanel::onConnect, this));
	mCommitCallbackRegistrar.add("SocialSharing.Disconnect", boost::bind(&LLSocialAccountPanel::onDisconnect, this));

	setVisibleCallback(boost::bind(&LLSocialAccountPanel::onVisibilityChange, this, _2));
}

BOOL LLSocialAccountPanel::postBuild()
{
	mAccountCaptionLabel = getChild<LLTextBox>("account_caption_label");
	mAccountNameLabel = getChild<LLTextBox>("account_name_label");
	mPanelButtons = getChild<LLUICtrl>("panel_buttons");
	mConnectButton = getChild<LLUICtrl>("connect_btn");
	mDisconnectButton = getChild<LLUICtrl>("disconnect_btn");

	return LLPanel::postBuild();
}

void LLSocialAccountPanel::draw()
{
	LLFacebookConnect::EConnectionState connection_state = LLFacebookConnect::instance().getConnectionState();

	//Disable the 'disconnect' button and the 'use another account' button when disconnecting in progress
	bool disconnecting = connection_state == LLFacebookConnect::FB_DISCONNECTING;
	mDisconnectButton->setEnabled(!disconnecting);

	//Disable the 'connect' button when a connection is in progress
	bool connecting = connection_state == LLFacebookConnect::FB_CONNECTION_IN_PROGRESS;
	mConnectButton->setEnabled(!connecting);

	LLPanel::draw();
}

void LLSocialAccountPanel::onVisibilityChange(const LLSD& new_visibility)
{
	bool visible = new_visibility.asBoolean();

	if(visible)
	{
		LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialAccountPanel");
		LLEventPumps::instance().obtain("FacebookConnectState").listen("LLSocialAccountPanel", boost::bind(&LLSocialAccountPanel::onFacebookConnectStateChange, this, _1));

		LLEventPumps::instance().obtain("FacebookConnectInfo").stopListening("LLSocialAccountPanel");
		LLEventPumps::instance().obtain("FacebookConnectInfo").listen("LLSocialAccountPanel", boost::bind(&LLSocialAccountPanel::onFacebookConnectInfoChange, this));

		//Connected
		if(LLFacebookConnect::instance().isConnected())
		{
			showConnectedLayout();
		}
		//Check if connected (show disconnected layout in meantime)
		else
		{
			showDisconnectedLayout();
		}
        if ((LLFacebookConnect::instance().getConnectionState() == LLFacebookConnect::FB_NOT_CONNECTED) ||
            (LLFacebookConnect::instance().getConnectionState() == LLFacebookConnect::FB_CONNECTION_FAILED))
        {
            LLFacebookConnect::instance().checkConnectionToFacebook();
        }
	}
	else
	{
		LLEventPumps::instance().obtain("FacebookConnectState").stopListening("LLSocialAccountPanel");
		LLEventPumps::instance().obtain("FacebookConnectInfo").stopListening("LLSocialAccountPanel");
	}
}

bool LLSocialAccountPanel::onFacebookConnectStateChange(const LLSD& data)
{
	if(LLFacebookConnect::instance().isConnected())
	{
		//In process of disconnecting so leave the layout as is
		if(data.get("enum").asInteger() != LLFacebookConnect::FB_DISCONNECTING)
		{
			showConnectedLayout();
		}
	}
	else
	{
		showDisconnectedLayout();
	}

	return false;
}

bool LLSocialAccountPanel::onFacebookConnectInfoChange()
{
	LLSD info = LLFacebookConnect::instance().getInfo();
	std::string clickable_name;

	//Strings of format [http://www.somewebsite.com Click Me] become clickable text
	if(info.has("link") && info.has("name"))
	{
		clickable_name = "[" + info["link"].asString() + " " + info["name"].asString() + "]";
	}

	mAccountNameLabel->setText(clickable_name);

	return false;
}

void LLSocialAccountPanel::showConnectButton()
{
	if(!mConnectButton->getVisible())
	{
		mConnectButton->setVisible(TRUE);
		mDisconnectButton->setVisible(FALSE);
	}
}

void LLSocialAccountPanel::hideConnectButton()
{
	if(mConnectButton->getVisible())
	{
		mConnectButton->setVisible(FALSE);
		mDisconnectButton->setVisible(TRUE);
	}
}

void LLSocialAccountPanel::showDisconnectedLayout()
{
	mAccountCaptionLabel->setText(getString("facebook_disconnected"));
	mAccountNameLabel->setText(std::string(""));
	showConnectButton();
}

void LLSocialAccountPanel::showConnectedLayout()
{
	LLFacebookConnect::instance().loadFacebookInfo();

	mAccountCaptionLabel->setText(getString("facebook_connected"));
	hideConnectButton();
}

void LLSocialAccountPanel::onConnect()
{
	LLFacebookConnect::instance().checkConnectionToFacebook(true);

	//Clear only the facebook browser cookies so that the facebook login screen appears
	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".facebook.com"); 
}

void LLSocialAccountPanel::onDisconnect()
{
	LLFacebookConnect::instance().disconnectFromFacebook();

	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".facebook.com"); 
}

////////////////////////
//LLFloaterSocial///////
////////////////////////

LLFloaterSocial::LLFloaterSocial(const LLSD& key) : LLFloater(key),
    mSocialPhotoPanel(NULL),
    mStatusErrorText(NULL),
    mStatusLoadingText(NULL),
    mStatusLoadingIndicator(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterSocial::onCancel, this));
}

void LLFloaterSocial::onCancel()
{
    closeFloater();
}

BOOL LLFloaterSocial::postBuild()
{
    // Keep tab of the Photo Panel
	mSocialPhotoPanel = static_cast<LLSocialPhotoPanel*>(getChild<LLUICtrl>("panel_social_photo"));
    // Connection status widgets
    mStatusErrorText = getChild<LLTextBox>("connection_error_text");
    mStatusLoadingText = getChild<LLTextBox>("connection_loading_text");
    mStatusLoadingIndicator = getChild<LLUICtrl>("connection_loading_indicator");
	return LLFloater::postBuild();
}

// static
void LLFloaterSocial::preUpdate()
{
	LLFloaterSocial* instance = LLFloaterReg::findTypedInstance<LLFloaterSocial>("social");
	if (instance)
	{
		//Will set file size text to 'unknown'
		instance->mSocialPhotoPanel->updateControls();
	}
}

// static
void LLFloaterSocial::postUpdate()
{
	LLFloaterSocial* instance = LLFloaterReg::findTypedInstance<LLFloaterSocial>("social");
	if (instance)
	{
		//Will set the file size text
		instance->mSocialPhotoPanel->updateControls();

		// The refresh button is initially hidden. We show it after the first update,
		// i.e. after snapshot is taken
		LLUICtrl * refresh_button = instance->mSocialPhotoPanel->getRefreshBtn();

		if (!refresh_button->getVisible())
		{
			refresh_button->setVisible(true);
		}
		
	}
}

void LLFloaterSocial::draw()
{
    if (mStatusErrorText && mStatusLoadingText && mStatusLoadingIndicator)
    {
        mStatusErrorText->setVisible(false);
        mStatusLoadingText->setVisible(false);
        mStatusLoadingIndicator->setVisible(false);
        LLFacebookConnect::EConnectionState connection_state = LLFacebookConnect::instance().getConnectionState();
        std::string status_text;
        
        switch (connection_state)
        {
        case LLFacebookConnect::FB_NOT_CONNECTED:
            // No status displayed when first opening the panel and no connection done
        case LLFacebookConnect::FB_CONNECTED:
            // When successfully connected, no message is displayed
        case LLFacebookConnect::FB_POSTED:
            // No success message to show since we actually close the floater after successful posting completion
            break;
        case LLFacebookConnect::FB_CONNECTION_IN_PROGRESS:
            // Connection loading indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookConnecting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
            break;
        case LLFacebookConnect::FB_POSTING:
            // Posting indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookPosting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
			break;
        case LLFacebookConnect::FB_CONNECTION_FAILED:
            // Error connecting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookErrorConnecting");
            mStatusErrorText->setValue(status_text);
            break;
        case LLFacebookConnect::FB_POST_FAILED:
            // Error posting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialFacebookErrorPosting");
            mStatusErrorText->setValue(status_text);
            break;
		case LLFacebookConnect::FB_DISCONNECTING:
			// Disconnecting loading indicator
			mStatusLoadingText->setVisible(true);
			status_text = LLTrans::getString("SocialFacebookDisconnecting");
			mStatusLoadingText->setValue(status_text);
			mStatusLoadingIndicator->setVisible(true);
			break;
		case LLFacebookConnect::FB_DISCONNECT_FAILED:
			// Error disconnecting from the service
			mStatusErrorText->setVisible(true);
			status_text = LLTrans::getString("SocialFacebookErrorDisconnecting");
			mStatusErrorText->setValue(status_text);
			break;
        }
    }
	LLFloater::draw();
}

