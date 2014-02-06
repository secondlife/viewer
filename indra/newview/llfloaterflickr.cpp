/** 
* @file llfloaterflickr.cpp
* @brief Implementation of llfloaterflickr
* @author cho@lindenlab.com
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

#include "llfloaterflickr.h"

#include "llagent.h"
#include "llagentui.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llflickrconnect.h"
#include "llfloaterreg.h"
#include "lliconctrl.h"
#include "llimagefiltersmanager.h"
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
#include "lltabcontainer.h"

static LLRegisterPanelClassWrapper<LLFlickrPhotoPanel> t_panel_photo("llflickrphotopanel");
static LLRegisterPanelClassWrapper<LLFlickrAccountPanel> t_panel_account("llflickraccountpanel");

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const std::string DEFAULT_PHOTO_QUERY_PARAMETERS = "?sourceid=slshare_photo&utm_source=flickr&utm_medium=photo&utm_campaign=slshare";
const std::string DEFAULT_PHOTO_LINK_TEXT = "Visit this location now";
const std::string DEFAULT_TAG_TEXT = "secondlife ";

///////////////////////////
//LLFlickrPhotoPanel///////
///////////////////////////

LLFlickrPhotoPanel::LLFlickrPhotoPanel() :
mSnapshotPanel(NULL),
mResolutionComboBox(NULL),
mRefreshBtn(NULL),
mWorkingLabel(NULL),
mThumbnailPlaceholder(NULL),
mTitleTextBox(NULL),
mDescriptionTextBox(NULL),
mLocationCheckbox(NULL),
mTagsTextBox(NULL),
mRatingComboBox(NULL),
mPostButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendPhoto", boost::bind(&LLFlickrPhotoPanel::onSend, this));
	mCommitCallbackRegistrar.add("SocialSharing.RefreshPhoto", boost::bind(&LLFlickrPhotoPanel::onClickNewSnapshot, this));
}

LLFlickrPhotoPanel::~LLFlickrPhotoPanel()
{
	if(mPreviewHandle.get())
	{
		mPreviewHandle.get()->die();
	}
}

BOOL LLFlickrPhotoPanel::postBuild()
{
	setVisibleCallback(boost::bind(&LLFlickrPhotoPanel::onVisibilityChange, this, _2));
	
	mSnapshotPanel = getChild<LLUICtrl>("snapshot_panel");
	mResolutionComboBox = getChild<LLUICtrl>("resolution_combobox");
	mResolutionComboBox->setCommitCallback(boost::bind(&LLFlickrPhotoPanel::updateResolution, this, TRUE));
	mFilterComboBox = getChild<LLUICtrl>("filters_combobox");
	mFilterComboBox->setCommitCallback(boost::bind(&LLFlickrPhotoPanel::updateResolution, this, TRUE));
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
    mWorkingLabel = getChild<LLUICtrl>("working_lbl");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");
	mTitleTextBox = getChild<LLUICtrl>("photo_title");
	mDescriptionTextBox = getChild<LLUICtrl>("photo_description");
	mLocationCheckbox = getChild<LLUICtrl>("add_location_cb");
	mTagsTextBox = getChild<LLUICtrl>("photo_tags");
	mTagsTextBox->setValue(DEFAULT_TAG_TEXT);
	mRatingComboBox = getChild<LLUICtrl>("rating_combobox");
	mPostButton = getChild<LLUICtrl>("post_photo_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_photo_btn");

	// Update filter list
    std::vector<std::string> filter_list = LLImageFiltersManager::getInstance()->getFiltersList();
	LLComboBox* filterbox = static_cast<LLComboBox *>(mFilterComboBox);
    for (U32 i = 0; i < filter_list.size(); i++) 
	{
        filterbox->add(filter_list[i]);
    }

	return LLPanel::postBuild();
}

// virtual
S32 LLFlickrPhotoPanel::notify(const LLSD& info)
{
	if (info.has("snapshot-updating"))
	{
        // Disable the Post button and whatever else while the snapshot is not updated
        // updateControls();
		return 1;
	}
    
	if (info.has("snapshot-updated"))
	{
        // Enable the send/post/save buttons.
        updateControls();
        
		// The refresh button is initially hidden. We show it after the first update,
		// i.e. after snapshot is taken
		LLUICtrl * refresh_button = getRefreshBtn();
		if (!refresh_button->getVisible())
		{
			refresh_button->setVisible(true);
		}
		return 1;
	}
    
	return 0;
}

void LLFlickrPhotoPanel::draw()
{ 
	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());

    // Enable interaction only if no transaction with the service is on-going (prevent duplicated posts)
    bool no_ongoing_connection = !(LLFlickrConnect::instance().isTransactionOngoing());
    mCancelButton->setEnabled(no_ongoing_connection);
    mTitleTextBox->setEnabled(no_ongoing_connection);
    mDescriptionTextBox->setEnabled(no_ongoing_connection);
    mTagsTextBox->setEnabled(no_ongoing_connection);
    mRatingComboBox->setEnabled(no_ongoing_connection);
    mResolutionComboBox->setEnabled(no_ongoing_connection);
    mFilterComboBox->setEnabled(no_ongoing_connection);
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
	}

    // Update the visibility of the working (computing preview) label
    mWorkingLabel->setVisible(!(previewp && previewp->getSnapshotUpToDate()));
    
    // Enable Post if we have a preview to send and no on going connection being processed
    mPostButton->setEnabled(no_ongoing_connection && (previewp && previewp->getSnapshotUpToDate()) && (mRatingComboBox && mRatingComboBox->getValue().isDefined()));
    
    // Draw the rest of the panel on top of it
	LLPanel::draw();
}

LLSnapshotLivePreview* LLFlickrPhotoPanel::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}

void LLFlickrPhotoPanel::onVisibilityChange(const LLSD& new_visibility)
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

            previewp->setContainer(this);
			previewp->setSnapshotType(previewp->SNAPSHOT_WEB);
			previewp->setSnapshotFormat(LLFloaterSnapshot::SNAPSHOT_FORMAT_PNG);
            previewp->setThumbnailSubsampled(TRUE);     // We want the preview to reflect the *saved* image
            previewp->setAllowRenderUI(FALSE);          // We do not want the rendered UI in our snapshots
            previewp->setAllowFullScreenPreview(FALSE);  // No full screen preview in SL Share mode
			previewp->setThumbnailPlaceholderRect(mThumbnailPlaceholder->getRect());

			updateControls();
		}
	}
}

void LLFlickrPhotoPanel::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->updateSnapshot(TRUE);
	}
}

void LLFlickrPhotoPanel::onSend()
{
	LLEventPumps::instance().obtain("FlickrConnectState").stopListening("LLFlickrPhotoPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("FlickrConnectState").listen("LLFlickrPhotoPanel", boost::bind(&LLFlickrPhotoPanel::onFlickrConnectStateChange, this, _1));
	
	// Connect to Flickr if necessary and then post
	if (LLFlickrConnect::instance().isConnected())
	{
		sendPhoto();
	}
	else
	{
		LLFlickrConnect::instance().checkConnectionToFlickr(true);
	}
}

bool LLFlickrPhotoPanel::onFlickrConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLFlickrConnect::FLICKR_CONNECTED:
			sendPhoto();
			break;

		case LLFlickrConnect::FLICKR_POSTED:
			LLEventPumps::instance().obtain("FlickrConnectState").stopListening("LLFlickrPhotoPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLFlickrPhotoPanel::sendPhoto()
{
	// Get the title, description, and tags
	std::string title = mTitleTextBox->getValue().asString();
	std::string description = mDescriptionTextBox->getValue().asString();
	std::string tags = mTagsTextBox->getValue().asString();

	// Add the location if required
	bool add_location = mLocationCheckbox->getValue().asBoolean();
	if (add_location)
	{
		// Get the SLURL for the location
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		std::string slurl_string = slurl.getSLURLString();

		// Add query parameters so Google Analytics can track incoming clicks!
		slurl_string += DEFAULT_PHOTO_QUERY_PARAMETERS;

		slurl_string = "<a href=\"" + slurl_string + "\">" + DEFAULT_PHOTO_LINK_TEXT + "</a>";

		// Add it to the description (pretty crude, but we don't have a better option with photos)
		if (description.empty())
			description = slurl_string;
		else
			description = description + "\n\n" + slurl_string;
	}

	// Get the content rating
	int content_rating = mRatingComboBox->getValue().asInteger();

	// Get the image
	LLSnapshotLivePreview* previewp = getPreviewView();
	
	// Post to Flickr
	LLFlickrConnect::instance().uploadPhoto(previewp->getFormattedImage(), title, description, tags, content_rating);

	updateControls();
}

void LLFlickrPhotoPanel::clearAndClose()
{
	mTitleTextBox->setValue("");
	mDescriptionTextBox->setValue("");
	mTagsTextBox->setValue(DEFAULT_TAG_TEXT);

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
	}
}

void LLFlickrPhotoPanel::updateControls()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();

	// *TODO: Separate maximum size for Web images from postcards
	lldebugs << "Is snapshot up-to-date? " << got_snap << llendl;

	updateResolution(FALSE);
}

void LLFlickrPhotoPanel::updateResolution(BOOL do_update)
{
	LLComboBox* combobox  = static_cast<LLComboBox *>(mResolutionComboBox);
	LLComboBox* filterbox = static_cast<LLComboBox *>(mFilterComboBox);

	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());

	S32 width = sdres[0];
	S32 height = sdres[1];
    
    const std::string& filter_name = filterbox->getSimple();

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
		if ((original_width != width) || (original_height != height))
		{
			previewp->setSize(width, height);
			if (do_update)
			{
                previewp->updateSnapshot(TRUE);
				updateControls();
			}
		}
        // Get the old filter, compare to the current one "filter_name" and set if changed
        std::string original_filter = previewp->getFilter();
		if (original_filter != filter_name)
		{
            previewp->setFilter(filter_name);
			if (do_update)
			{
                previewp->updateSnapshot(FALSE, TRUE);
				updateControls();
			}
		}
	}
}

void LLFlickrPhotoPanel::checkAspectRatio(S32 index)
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

LLUICtrl* LLFlickrPhotoPanel::getRefreshBtn()
{
	return mRefreshBtn;
}

///////////////////////////
//LLFlickrAccountPanel//////
///////////////////////////

LLFlickrAccountPanel::LLFlickrAccountPanel() : 
mAccountCaptionLabel(NULL),
mAccountNameLabel(NULL),
mPanelButtons(NULL),
mConnectButton(NULL),
mDisconnectButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Connect", boost::bind(&LLFlickrAccountPanel::onConnect, this));
	mCommitCallbackRegistrar.add("SocialSharing.Disconnect", boost::bind(&LLFlickrAccountPanel::onDisconnect, this));

	setVisibleCallback(boost::bind(&LLFlickrAccountPanel::onVisibilityChange, this, _2));
}

BOOL LLFlickrAccountPanel::postBuild()
{
	mAccountCaptionLabel = getChild<LLTextBox>("account_caption_label");
	mAccountNameLabel = getChild<LLTextBox>("account_name_label");
	mPanelButtons = getChild<LLUICtrl>("panel_buttons");
	mConnectButton = getChild<LLUICtrl>("connect_btn");
	mDisconnectButton = getChild<LLUICtrl>("disconnect_btn");

	return LLPanel::postBuild();
}

void LLFlickrAccountPanel::draw()
{
	LLFlickrConnect::EConnectionState connection_state = LLFlickrConnect::instance().getConnectionState();

	//Disable the 'disconnect' button and the 'use another account' button when disconnecting in progress
	bool disconnecting = connection_state == LLFlickrConnect::FLICKR_DISCONNECTING;
	mDisconnectButton->setEnabled(!disconnecting);

	//Disable the 'connect' button when a connection is in progress
	bool connecting = connection_state == LLFlickrConnect::FLICKR_CONNECTION_IN_PROGRESS;
	mConnectButton->setEnabled(!connecting);

	LLPanel::draw();
}

void LLFlickrAccountPanel::onVisibilityChange(const LLSD& new_visibility)
{
	bool visible = new_visibility.asBoolean();

	if(visible)
	{
		LLEventPumps::instance().obtain("FlickrConnectState").stopListening("LLFlickrAccountPanel");
		LLEventPumps::instance().obtain("FlickrConnectState").listen("LLFlickrAccountPanel", boost::bind(&LLFlickrAccountPanel::onFlickrConnectStateChange, this, _1));

		LLEventPumps::instance().obtain("FlickrConnectInfo").stopListening("LLFlickrAccountPanel");
		LLEventPumps::instance().obtain("FlickrConnectInfo").listen("LLFlickrAccountPanel", boost::bind(&LLFlickrAccountPanel::onFlickrConnectInfoChange, this));

		//Connected
		if(LLFlickrConnect::instance().isConnected())
		{
			showConnectedLayout();
		}
		//Check if connected (show disconnected layout in meantime)
		else
		{
			showDisconnectedLayout();
		}
        if ((LLFlickrConnect::instance().getConnectionState() == LLFlickrConnect::FLICKR_NOT_CONNECTED) ||
            (LLFlickrConnect::instance().getConnectionState() == LLFlickrConnect::FLICKR_CONNECTION_FAILED))
        {
            LLFlickrConnect::instance().checkConnectionToFlickr();
        }
	}
	else
	{
		LLEventPumps::instance().obtain("FlickrConnectState").stopListening("LLFlickrAccountPanel");
		LLEventPumps::instance().obtain("FlickrConnectInfo").stopListening("LLFlickrAccountPanel");
	}
}

bool LLFlickrAccountPanel::onFlickrConnectStateChange(const LLSD& data)
{
	if(LLFlickrConnect::instance().isConnected())
	{
		//In process of disconnecting so leave the layout as is
		if(data.get("enum").asInteger() != LLFlickrConnect::FLICKR_DISCONNECTING)
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

bool LLFlickrAccountPanel::onFlickrConnectInfoChange()
{
	LLSD info = LLFlickrConnect::instance().getInfo();
	std::string clickable_name;

	//Strings of format [http://www.somewebsite.com Click Me] become clickable text
	if(info.has("link") && info.has("name"))
	{
		clickable_name = "[" + info["link"].asString() + " " + info["name"].asString() + "]";
	}

	mAccountNameLabel->setText(clickable_name);

	return false;
}

void LLFlickrAccountPanel::showConnectButton()
{
	if(!mConnectButton->getVisible())
	{
		mConnectButton->setVisible(TRUE);
		mDisconnectButton->setVisible(FALSE);
	}
}

void LLFlickrAccountPanel::hideConnectButton()
{
	if(mConnectButton->getVisible())
	{
		mConnectButton->setVisible(FALSE);
		mDisconnectButton->setVisible(TRUE);
	}
}

void LLFlickrAccountPanel::showDisconnectedLayout()
{
	mAccountCaptionLabel->setText(getString("flickr_disconnected"));
	mAccountNameLabel->setText(std::string(""));
	showConnectButton();
}

void LLFlickrAccountPanel::showConnectedLayout()
{
	LLFlickrConnect::instance().loadFlickrInfo();

	mAccountCaptionLabel->setText(getString("flickr_connected"));
	hideConnectButton();
}

void LLFlickrAccountPanel::onConnect()
{
	LLFlickrConnect::instance().checkConnectionToFlickr(true);

	//Clear only the flickr browser cookies so that the flickr login screen appears
	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".flickr.com"); 
}

void LLFlickrAccountPanel::onDisconnect()
{
	LLFlickrConnect::instance().disconnectFromFlickr();

	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".flickr.com"); 
}

////////////////////////
//LLFloaterFlickr///////
////////////////////////

LLFloaterFlickr::LLFloaterFlickr(const LLSD& key) : LLFloater(key),
    mFlickrPhotoPanel(NULL),
    mStatusErrorText(NULL),
    mStatusLoadingText(NULL),
    mStatusLoadingIndicator(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterFlickr::onCancel, this));
}

void LLFloaterFlickr::onCancel()
{
    closeFloater();
}

BOOL LLFloaterFlickr::postBuild()
{
    // Keep tab of the Photo Panel
	mFlickrPhotoPanel = static_cast<LLFlickrPhotoPanel*>(getChild<LLUICtrl>("panel_flickr_photo"));
    // Connection status widgets
    mStatusErrorText = getChild<LLTextBox>("connection_error_text");
    mStatusLoadingText = getChild<LLTextBox>("connection_loading_text");
    mStatusLoadingIndicator = getChild<LLUICtrl>("connection_loading_indicator");
	return LLFloater::postBuild();
}

void LLFloaterFlickr::showPhotoPanel()
{
	LLTabContainer* parent = dynamic_cast<LLTabContainer*>(mFlickrPhotoPanel->getParent());
	if (!parent)
	{
		llwarns << "Cannot find panel container" << llendl;
		return;
	}

	parent->selectTabPanel(mFlickrPhotoPanel);
}

void LLFloaterFlickr::draw()
{
    if (mStatusErrorText && mStatusLoadingText && mStatusLoadingIndicator)
    {
        mStatusErrorText->setVisible(false);
        mStatusLoadingText->setVisible(false);
        mStatusLoadingIndicator->setVisible(false);
        LLFlickrConnect::EConnectionState connection_state = LLFlickrConnect::instance().getConnectionState();
        std::string status_text;
        
        switch (connection_state)
        {
        case LLFlickrConnect::FLICKR_NOT_CONNECTED:
            // No status displayed when first opening the panel and no connection done
        case LLFlickrConnect::FLICKR_CONNECTED:
            // When successfully connected, no message is displayed
        case LLFlickrConnect::FLICKR_POSTED:
            // No success message to show since we actually close the floater after successful posting completion
            break;
        case LLFlickrConnect::FLICKR_CONNECTION_IN_PROGRESS:
            // Connection loading indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialFlickrConnecting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
            break;
        case LLFlickrConnect::FLICKR_POSTING:
            // Posting indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialFlickrPosting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
			break;
        case LLFlickrConnect::FLICKR_CONNECTION_FAILED:
            // Error connecting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialFlickrErrorConnecting");
            mStatusErrorText->setValue(status_text);
            break;
        case LLFlickrConnect::FLICKR_POST_FAILED:
            // Error posting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialFlickrErrorPosting");
            mStatusErrorText->setValue(status_text);
            break;
		case LLFlickrConnect::FLICKR_DISCONNECTING:
			// Disconnecting loading indicator
			mStatusLoadingText->setVisible(true);
			status_text = LLTrans::getString("SocialFlickrDisconnecting");
			mStatusLoadingText->setValue(status_text);
			mStatusLoadingIndicator->setVisible(true);
			break;
		case LLFlickrConnect::FLICKR_DISCONNECT_FAILED:
			// Error disconnecting from the service
			mStatusErrorText->setVisible(true);
			status_text = LLTrans::getString("SocialFlickrErrorDisconnecting");
			mStatusErrorText->setValue(status_text);
			break;
        }
    }
	LLFloater::draw();
}

