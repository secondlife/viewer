/** 
* @file llfloatertwitter.cpp
* @brief Implementation of llfloatertwitter
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

#include "llfloatertwitter.h"

#include "llagent.h"
#include "llagentui.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lltwitterconnect.h"
#include "llfloaterbigpreview.h"
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
#include "lltexteditor.h"

static LLPanelInjector<LLTwitterPhotoPanel> t_panel_photo("lltwitterphotopanel");
static LLPanelInjector<LLTwitterAccountPanel> t_panel_account("lltwitteraccountpanel");

const S32 MAX_POSTCARD_DATASIZE = 1024 * 1024; // one megabyte
const std::string DEFAULT_PHOTO_LOCATION_URL = "http://maps.secondlife.com/";
const std::string DEFAULT_PHOTO_QUERY_PARAMETERS = "?sourceid=slshare_photo&utm_source=twitter&utm_medium=photo&utm_campaign=slshare";
const std::string DEFAULT_STATUS_TEXT = " #SecondLife";

///////////////////////////
//LLTwitterPhotoPanel///////
///////////////////////////

LLTwitterPhotoPanel::LLTwitterPhotoPanel() :
mSnapshotPanel(NULL),
mResolutionComboBox(NULL),
mRefreshBtn(NULL),
mBtnPreview(NULL),
mWorkingLabel(NULL),
mThumbnailPlaceholder(NULL),
mStatusCounterLabel(NULL),
mStatusTextBox(NULL),
mLocationCheckbox(NULL),
mPhotoCheckbox(NULL),
mBigPreviewFloater(NULL),
mPostButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.SendPhoto", boost::bind(&LLTwitterPhotoPanel::onSend, this));
	mCommitCallbackRegistrar.add("SocialSharing.RefreshPhoto", boost::bind(&LLTwitterPhotoPanel::onClickNewSnapshot, this));
	mCommitCallbackRegistrar.add("SocialSharing.BigPreview", boost::bind(&LLTwitterPhotoPanel::onClickBigPreview, this));
}

LLTwitterPhotoPanel::~LLTwitterPhotoPanel()
{
	if(mPreviewHandle.get())
	{
		mPreviewHandle.get()->die();
	}
}

BOOL LLTwitterPhotoPanel::postBuild()
{
	setVisibleCallback(boost::bind(&LLTwitterPhotoPanel::onVisibilityChange, this, _2));
	
	mSnapshotPanel = getChild<LLUICtrl>("snapshot_panel");
	mResolutionComboBox = getChild<LLUICtrl>("resolution_combobox");
	mResolutionComboBox->setValue("[i800,i600]"); // hardcoded defaults ftw!
	mResolutionComboBox->setCommitCallback(boost::bind(&LLTwitterPhotoPanel::updateResolution, this, TRUE));
	mFilterComboBox = getChild<LLUICtrl>("filters_combobox");
	mFilterComboBox->setCommitCallback(boost::bind(&LLTwitterPhotoPanel::updateResolution, this, TRUE));
	mRefreshBtn = getChild<LLUICtrl>("new_snapshot_btn");
	mBtnPreview = getChild<LLButton>("big_preview_btn");
    mWorkingLabel = getChild<LLUICtrl>("working_lbl");
	mThumbnailPlaceholder = getChild<LLUICtrl>("thumbnail_placeholder");
	mStatusCounterLabel = getChild<LLUICtrl>("status_counter_label");
	mStatusTextBox = getChild<LLUICtrl>("photo_status");
	mStatusTextBox->setValue(DEFAULT_STATUS_TEXT);
	mLocationCheckbox = getChild<LLUICtrl>("add_location_cb");
	mLocationCheckbox->setCommitCallback(boost::bind(&LLTwitterPhotoPanel::onAddLocationToggled, this));
	mPhotoCheckbox = getChild<LLUICtrl>("add_photo_cb");
	mPhotoCheckbox->setCommitCallback(boost::bind(&LLTwitterPhotoPanel::onAddPhotoToggled, this));
	mPostButton = getChild<LLUICtrl>("post_photo_btn");
	mCancelButton = getChild<LLUICtrl>("cancel_photo_btn");
	mBigPreviewFloater = dynamic_cast<LLFloaterBigPreview*>(LLFloaterReg::getInstance("big_preview"));

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
S32 LLTwitterPhotoPanel::notify(const LLSD& info)
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

void LLTwitterPhotoPanel::draw()
{ 
	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());

    // Enable interaction only if no transaction with the service is on-going (prevent duplicated posts)
    bool no_ongoing_connection = !(LLTwitterConnect::instance().isTransactionOngoing());
    bool photo_checked = mPhotoCheckbox->getValue().asBoolean();
    mCancelButton->setEnabled(no_ongoing_connection);
    mStatusTextBox->setEnabled(no_ongoing_connection);
    mResolutionComboBox->setEnabled(no_ongoing_connection && photo_checked);
    mFilterComboBox->setEnabled(no_ongoing_connection && photo_checked);
    mRefreshBtn->setEnabled(no_ongoing_connection && photo_checked);
    mBtnPreview->setEnabled(no_ongoing_connection);
    mLocationCheckbox->setEnabled(no_ongoing_connection);
    mPhotoCheckbox->setEnabled(no_ongoing_connection);

	bool add_location = mLocationCheckbox->getValue().asBoolean();
	bool add_photo = mPhotoCheckbox->getValue().asBoolean();
	updateStatusTextLength(false);

    // Reassign the preview floater if we have the focus and the preview exists
    if (hasFocus() && isPreviewVisible())
    {
        attachPreview();
    }
    
    // Toggle the button state as appropriate
    bool preview_active = (isPreviewVisible() && mBigPreviewFloater->isFloaterOwner(getParentByType<LLFloater>()));
	mBtnPreview->setToggleState(preview_active);
    
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
		F32 alpha = (add_photo ? (getTransparencyType() == TT_ACTIVE ? 1.0f : getCurrentTransparency()) : 0.5f);
		LLColor4 color = LLColor4::white;
		gl_draw_scaled_image(offset_x, offset_y, 
			thumbnail_w, thumbnail_h,
			previewp->getThumbnailImage(), color % alpha);
	}

    // Update the visibility of the working (computing preview) label
    mWorkingLabel->setVisible(!(previewp && previewp->getSnapshotUpToDate()));
    
    // Enable Post if we have a preview to send and no on going connection being processed
    mPostButton->setEnabled(no_ongoing_connection && (previewp && previewp->getSnapshotUpToDate()) && (add_photo || add_location || !mStatusTextBox->getValue().asString().empty()));
    
    // Draw the rest of the panel on top of it
	LLPanel::draw();
}

LLSnapshotLivePreview* LLTwitterPhotoPanel::getPreviewView()
{
	LLSnapshotLivePreview* previewp = (LLSnapshotLivePreview*)mPreviewHandle.get();
	return previewp;
}

void LLTwitterPhotoPanel::onVisibilityChange(BOOL visible)
{
	if (visible)
	{
		if (mPreviewHandle.get())
		{
			LLSnapshotLivePreview* preview = getPreviewView();
			if(preview)
			{
				LL_DEBUGS() << "opened, updating snapshot" << LL_ENDL;
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
			previewp->setSnapshotFormat(LLFloaterSnapshot::SNAPSHOT_FORMAT_JPEG);
            previewp->setThumbnailSubsampled(TRUE);     // We want the preview to reflect the *saved* image
            previewp->setAllowRenderUI(FALSE);          // We do not want the rendered UI in our snapshots
            previewp->setAllowFullScreenPreview(FALSE);  // No full screen preview in SL Share mode
			previewp->setThumbnailPlaceholderRect(mThumbnailPlaceholder->getRect());

			updateControls();
		}
	}
}

void LLTwitterPhotoPanel::onAddLocationToggled()
{
	bool add_location = mLocationCheckbox->getValue().asBoolean();
	updateStatusTextLength(!add_location);
}

void LLTwitterPhotoPanel::onAddPhotoToggled()
{
	bool add_photo = mPhotoCheckbox->getValue().asBoolean();
	updateStatusTextLength(!add_photo);
}

void LLTwitterPhotoPanel::onClickNewSnapshot()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	if (previewp)
	{
		previewp->updateSnapshot(TRUE);
	}
}

void LLTwitterPhotoPanel::onClickBigPreview()
{
    // Toggle the preview
    if (isPreviewVisible())
    {
        LLFloaterReg::hideInstance("big_preview");
    }
    else
    {
        attachPreview();
        LLFloaterReg::showInstance("big_preview");
    }
}

bool LLTwitterPhotoPanel::isPreviewVisible()
{
    return (mBigPreviewFloater && mBigPreviewFloater->getVisible());
}

void LLTwitterPhotoPanel::attachPreview()
{
    if (mBigPreviewFloater)
    {
        LLSnapshotLivePreview* previewp = getPreviewView();
        mBigPreviewFloater->setPreview(previewp);
        mBigPreviewFloater->setFloaterOwner(getParentByType<LLFloater>());
    }
}

void LLTwitterPhotoPanel::onSend()
{
	LLEventPumps::instance().obtain("TwitterConnectState").stopListening("LLTwitterPhotoPanel"); // just in case it is already listening
	LLEventPumps::instance().obtain("TwitterConnectState").listen("LLTwitterPhotoPanel", boost::bind(&LLTwitterPhotoPanel::onTwitterConnectStateChange, this, _1));
	
	// Connect to Twitter if necessary and then post
	if (LLTwitterConnect::instance().isConnected())
	{
		sendPhoto();
	}
	else
	{
		LLTwitterConnect::instance().checkConnectionToTwitter(true);
	}
}

bool LLTwitterPhotoPanel::onTwitterConnectStateChange(const LLSD& data)
{
	switch (data.get("enum").asInteger())
	{
		case LLTwitterConnect::TWITTER_CONNECTED:
			sendPhoto();
			break;

		case LLTwitterConnect::TWITTER_POSTED:
			LLEventPumps::instance().obtain("TwitterConnectState").stopListening("LLTwitterPhotoPanel");
			clearAndClose();
			break;
	}

	return false;
}

void LLTwitterPhotoPanel::sendPhoto()
{
	// Get the status text
	std::string status = mStatusTextBox->getValue().asString();
	
	// Add the location if required
	bool add_location = mLocationCheckbox->getValue().asBoolean();
	if (add_location)
	{
		// Get the SLURL for the location
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl);
		std::string slurl_string = slurl.getSLURLString();

		// Use a valid http:// URL if the scheme is secondlife:// 
		LLURI slurl_uri(slurl_string);
		if (slurl_uri.scheme() == LLSLURL::SLURL_SECONDLIFE_SCHEME)
		{
			slurl_string = DEFAULT_PHOTO_LOCATION_URL;
		}

		// Add query parameters so Google Analytics can track incoming clicks!
		slurl_string += DEFAULT_PHOTO_QUERY_PARAMETERS;

		// Add it to the status (pretty crude, but we don't have a better option with photos)
		if (status.empty())
			status = slurl_string;
		else
			status = status + " " + slurl_string;
	}

	// Add the photo if required
	bool add_photo = mPhotoCheckbox->getValue().asBoolean();
	if (add_photo)
	{
		// Get the image
		LLSnapshotLivePreview* previewp = getPreviewView();
	
		// Post to Twitter
		LLTwitterConnect::instance().uploadPhoto(previewp->getFormattedImage(), status);
	}
	else
	{
		// Just post the status to Twitter
		LLTwitterConnect::instance().updateStatus(status);
	}

	updateControls();
}

void LLTwitterPhotoPanel::clearAndClose()
{
	mStatusTextBox->setValue(DEFAULT_STATUS_TEXT);

	LLFloater* floater = getParentByType<LLFloater>();
	if (floater)
	{
		floater->closeFloater();
        if (mBigPreviewFloater)
        {
            mBigPreviewFloater->closeOnFloaterOwnerClosing(floater);
        }
	}
}

void LLTwitterPhotoPanel::updateStatusTextLength(BOOL restore_old_status_text)
{
	bool add_location = mLocationCheckbox->getValue().asBoolean();
	bool add_photo = mPhotoCheckbox->getValue().asBoolean();

	// Restrict the status text length to Twitter's character limit
	LLTextEditor* status_text_box = dynamic_cast<LLTextEditor*>(mStatusTextBox);
	if (status_text_box)
	{
		int max_status_length = 140 - (add_location ? 40 : 0) - (add_photo ? 40 : 0);
		status_text_box->setMaxTextLength(max_status_length);
		if (restore_old_status_text)
		{
			if (mOldStatusText.length() > status_text_box->getText().length() && status_text_box->getText() == mOldStatusText.substr(0, status_text_box->getText().length()))
			{
				status_text_box->setText(mOldStatusText);
			}
			if (mOldStatusText.length() <= max_status_length)
			{
				mOldStatusText = "";
			}
		}
		if (status_text_box->getText().length() > max_status_length)
		{
			if (mOldStatusText.length() < status_text_box->getText().length() || status_text_box->getText() != mOldStatusText.substr(0, status_text_box->getText().length()))
			{
				mOldStatusText = status_text_box->getText();
			}
			status_text_box->setText(mOldStatusText.substr(0, max_status_length));
		}

		// Update the status character counter
		int characters_remaining = max_status_length - status_text_box->getText().length();
		mStatusCounterLabel->setValue(characters_remaining);
	}

}

void LLTwitterPhotoPanel::updateControls()
{
	LLSnapshotLivePreview* previewp = getPreviewView();
	BOOL got_snap = previewp && previewp->getSnapshotUpToDate();
    
	// *TODO: Separate maximum size for Web images from postcards
	LL_DEBUGS() << "Is snapshot up-to-date? " << got_snap << LL_ENDL;
    
	updateResolution(FALSE);
}

void LLTwitterPhotoPanel::updateResolution(BOOL do_update)
{
	LLComboBox* combobox = static_cast<LLComboBox *>(mResolutionComboBox);
	LLComboBox* filterbox = static_cast<LLComboBox *>(mFilterComboBox);

	std::string sdstring = combobox->getSelectedValue();
	LLSD sdres;
	std::stringstream sstream(sdstring);
	LLSDSerialize::fromNotation(sdres, sstream, sdstring.size());

	S32 width = sdres[0];
	S32 height = sdres[1];

    // Note : index 0 of the filter drop down is assumed to be "No filter" in whichever locale
    std::string filter_name = (filterbox->getCurrentIndex() ? filterbox->getSimple() : "");

	LLSnapshotLivePreview * previewp = static_cast<LLSnapshotLivePreview *>(mPreviewHandle.get());
	if (previewp && combobox->getCurrentIndex() >= 0)
	{
		S32 original_width = 0 , original_height = 0 ;
		previewp->getSize(original_width, original_height) ;

		if (width == 0 || height == 0)
		{
			// take resolution from current window size
			LL_DEBUGS() << "Setting preview res from window: " << gViewerWindow->getWindowWidthRaw() << "x" << gViewerWindow->getWindowHeightRaw() << LL_ENDL;
			previewp->setSize(gViewerWindow->getWindowWidthRaw(), gViewerWindow->getWindowHeightRaw());
		}
		else
		{
			// use the resolution from the selected pre-canned drop-down choice
			LL_DEBUGS() << "Setting preview res selected from combo: " << width << "x" << height << LL_ENDL;
			previewp->setSize(width, height);
		}

		checkAspectRatio(width);

		previewp->getSize(width, height);
		
		if (original_width != width || original_height != height)
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

void LLTwitterPhotoPanel::checkAspectRatio(S32 index)
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

LLUICtrl* LLTwitterPhotoPanel::getRefreshBtn()
{
	return mRefreshBtn;
}

///////////////////////////
//LLTwitterAccountPanel//////
///////////////////////////

LLTwitterAccountPanel::LLTwitterAccountPanel() : 
mAccountCaptionLabel(NULL),
mAccountNameLabel(NULL),
mPanelButtons(NULL),
mConnectButton(NULL),
mDisconnectButton(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Connect", boost::bind(&LLTwitterAccountPanel::onConnect, this));
	mCommitCallbackRegistrar.add("SocialSharing.Disconnect", boost::bind(&LLTwitterAccountPanel::onDisconnect, this));

	setVisibleCallback(boost::bind(&LLTwitterAccountPanel::onVisibilityChange, this, _2));
}

BOOL LLTwitterAccountPanel::postBuild()
{
	mAccountCaptionLabel = getChild<LLTextBox>("account_caption_label");
	mAccountNameLabel = getChild<LLTextBox>("account_name_label");
	mPanelButtons = getChild<LLUICtrl>("panel_buttons");
	mConnectButton = getChild<LLUICtrl>("connect_btn");
	mDisconnectButton = getChild<LLUICtrl>("disconnect_btn");

	return LLPanel::postBuild();
}

void LLTwitterAccountPanel::draw()
{
	LLTwitterConnect::EConnectionState connection_state = LLTwitterConnect::instance().getConnectionState();

	//Disable the 'disconnect' button and the 'use another account' button when disconnecting in progress
	bool disconnecting = connection_state == LLTwitterConnect::TWITTER_DISCONNECTING;
	mDisconnectButton->setEnabled(!disconnecting);

	//Disable the 'connect' button when a connection is in progress
	bool connecting = connection_state == LLTwitterConnect::TWITTER_CONNECTION_IN_PROGRESS;
	mConnectButton->setEnabled(!connecting);

	LLPanel::draw();
}

void LLTwitterAccountPanel::onVisibilityChange(BOOL visible)
{
	if(visible)
	{
		LLEventPumps::instance().obtain("TwitterConnectState").stopListening("LLTwitterAccountPanel");
		LLEventPumps::instance().obtain("TwitterConnectState").listen("LLTwitterAccountPanel", boost::bind(&LLTwitterAccountPanel::onTwitterConnectStateChange, this, _1));

		LLEventPumps::instance().obtain("TwitterConnectInfo").stopListening("LLTwitterAccountPanel");
		LLEventPumps::instance().obtain("TwitterConnectInfo").listen("LLTwitterAccountPanel", boost::bind(&LLTwitterAccountPanel::onTwitterConnectInfoChange, this));

		//Connected
		if(LLTwitterConnect::instance().isConnected())
		{
			showConnectedLayout();
		}
		//Check if connected (show disconnected layout in meantime)
		else
		{
			showDisconnectedLayout();
		}
        if ((LLTwitterConnect::instance().getConnectionState() == LLTwitterConnect::TWITTER_NOT_CONNECTED) ||
            (LLTwitterConnect::instance().getConnectionState() == LLTwitterConnect::TWITTER_CONNECTION_FAILED))
        {
            LLTwitterConnect::instance().checkConnectionToTwitter();
        }
	}
	else
	{
		LLEventPumps::instance().obtain("TwitterConnectState").stopListening("LLTwitterAccountPanel");
		LLEventPumps::instance().obtain("TwitterConnectInfo").stopListening("LLTwitterAccountPanel");
	}
}

bool LLTwitterAccountPanel::onTwitterConnectStateChange(const LLSD& data)
{
	if(LLTwitterConnect::instance().isConnected())
	{
		//In process of disconnecting so leave the layout as is
		if(data.get("enum").asInteger() != LLTwitterConnect::TWITTER_DISCONNECTING)
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

bool LLTwitterAccountPanel::onTwitterConnectInfoChange()
{
	LLSD info = LLTwitterConnect::instance().getInfo();
	std::string clickable_name;

	//Strings of format [http://www.somewebsite.com Click Me] become clickable text
	if(info.has("link") && info.has("name"))
	{
		clickable_name = "[" + info["link"].asString() + " " + info["name"].asString() + "]";
	}

	mAccountNameLabel->setText(clickable_name);

	return false;
}

void LLTwitterAccountPanel::showConnectButton()
{
	if(!mConnectButton->getVisible())
	{
		mConnectButton->setVisible(TRUE);
		mDisconnectButton->setVisible(FALSE);
	}
}

void LLTwitterAccountPanel::hideConnectButton()
{
	if(mConnectButton->getVisible())
	{
		mConnectButton->setVisible(FALSE);
		mDisconnectButton->setVisible(TRUE);
	}
}

void LLTwitterAccountPanel::showDisconnectedLayout()
{
	mAccountCaptionLabel->setText(getString("twitter_disconnected"));
	mAccountNameLabel->setText(std::string(""));
	showConnectButton();
}

void LLTwitterAccountPanel::showConnectedLayout()
{
	LLTwitterConnect::instance().loadTwitterInfo();

	mAccountCaptionLabel->setText(getString("twitter_connected"));
	hideConnectButton();
}

void LLTwitterAccountPanel::onConnect()
{
	LLTwitterConnect::instance().checkConnectionToTwitter(true);

	//Clear only the twitter browser cookies so that the twitter login screen appears
	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".twitter.com"); 
}

void LLTwitterAccountPanel::onDisconnect()
{
	LLTwitterConnect::instance().disconnectFromTwitter();

	LLViewerMedia::getCookieStore()->removeCookiesByDomain(".twitter.com"); 
}

////////////////////////
//LLFloaterTwitter///////
////////////////////////

LLFloaterTwitter::LLFloaterTwitter(const LLSD& key) : LLFloater(key),
    mTwitterPhotoPanel(NULL),
    mStatusErrorText(NULL),
    mStatusLoadingText(NULL),
    mStatusLoadingIndicator(NULL)
{
	mCommitCallbackRegistrar.add("SocialSharing.Cancel", boost::bind(&LLFloaterTwitter::onCancel, this));
}

void LLFloaterTwitter::onClose(bool app_quitting)
{
    LLFloaterBigPreview* big_preview_floater = dynamic_cast<LLFloaterBigPreview*>(LLFloaterReg::getInstance("big_preview"));
    if (big_preview_floater)
    {
        big_preview_floater->closeOnFloaterOwnerClosing(this);
    }
	LLFloater::onClose(app_quitting);
}

void LLFloaterTwitter::onCancel()
{
    LLFloaterBigPreview* big_preview_floater = dynamic_cast<LLFloaterBigPreview*>(LLFloaterReg::getInstance("big_preview"));
    if (big_preview_floater)
    {
        big_preview_floater->closeOnFloaterOwnerClosing(this);
    }
    closeFloater();
}

BOOL LLFloaterTwitter::postBuild()
{
    // Keep tab of the Photo Panel
	mTwitterPhotoPanel = static_cast<LLTwitterPhotoPanel*>(getChild<LLUICtrl>("panel_twitter_photo"));
    // Connection status widgets
    mStatusErrorText = getChild<LLTextBox>("connection_error_text");
    mStatusLoadingText = getChild<LLTextBox>("connection_loading_text");
    mStatusLoadingIndicator = getChild<LLUICtrl>("connection_loading_indicator");
	return LLFloater::postBuild();
}

void LLFloaterTwitter::showPhotoPanel()
{
	LLTabContainer* parent = dynamic_cast<LLTabContainer*>(mTwitterPhotoPanel->getParent());
	if (!parent)
	{
		LL_WARNS() << "Cannot find panel container" << LL_ENDL;
		return;
	}

	parent->selectTabPanel(mTwitterPhotoPanel);
}

void LLFloaterTwitter::draw()
{
    if (mStatusErrorText && mStatusLoadingText && mStatusLoadingIndicator)
    {
        mStatusErrorText->setVisible(false);
        mStatusLoadingText->setVisible(false);
        mStatusLoadingIndicator->setVisible(false);
        LLTwitterConnect::EConnectionState connection_state = LLTwitterConnect::instance().getConnectionState();
        std::string status_text;
        
        switch (connection_state)
        {
        case LLTwitterConnect::TWITTER_NOT_CONNECTED:
            // No status displayed when first opening the panel and no connection done
        case LLTwitterConnect::TWITTER_CONNECTED:
            // When successfully connected, no message is displayed
        case LLTwitterConnect::TWITTER_POSTED:
            // No success message to show since we actually close the floater after successful posting completion
            break;
        case LLTwitterConnect::TWITTER_CONNECTION_IN_PROGRESS:
            // Connection loading indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialTwitterConnecting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
            break;
        case LLTwitterConnect::TWITTER_POSTING:
            // Posting indicator
            mStatusLoadingText->setVisible(true);
            status_text = LLTrans::getString("SocialTwitterPosting");
            mStatusLoadingText->setValue(status_text);
            mStatusLoadingIndicator->setVisible(true);
			break;
        case LLTwitterConnect::TWITTER_CONNECTION_FAILED:
            // Error connecting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialTwitterErrorConnecting");
            mStatusErrorText->setValue(status_text);
            break;
        case LLTwitterConnect::TWITTER_POST_FAILED:
            // Error posting to the service
            mStatusErrorText->setVisible(true);
            status_text = LLTrans::getString("SocialTwitterErrorPosting");
            mStatusErrorText->setValue(status_text);
            break;
		case LLTwitterConnect::TWITTER_DISCONNECTING:
			// Disconnecting loading indicator
			mStatusLoadingText->setVisible(true);
			status_text = LLTrans::getString("SocialTwitterDisconnecting");
			mStatusLoadingText->setValue(status_text);
			mStatusLoadingIndicator->setVisible(true);
			break;
		case LLTwitterConnect::TWITTER_DISCONNECT_FAILED:
			// Error disconnecting from the service
			mStatusErrorText->setVisible(true);
			status_text = LLTrans::getString("SocialTwitterErrorDisconnecting");
			mStatusErrorText->setValue(status_text);
			break;
        }
    }
	LLFloater::draw();
}

