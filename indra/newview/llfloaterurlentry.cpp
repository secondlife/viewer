/**
 * @file llfloaterurlentry.cpp
 * @brief LLFloaterURLEntry class implementation
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterurlentry.h"

#include "llpanellandmedia.h"
#include "llpanelface.h"

#include "llcombobox.h"
#include "llmimetypes.h"
#include "llnotificationsutil.h"
#include "llurlhistory.h"
#include "lluictrlfactory.h"
#include "llwindow.h"
#include "llviewerwindow.h"
#include "llcorehttputil.h"

static LLFloaterURLEntry* sInstance = NULL;

//-----------------------------------------------------------------------------
// LLFloaterURLEntry()
//-----------------------------------------------------------------------------
LLFloaterURLEntry::LLFloaterURLEntry(LLHandle<LLPanel> parent)
    : LLFloater(LLSD()),
      mPanelLandMediaHandle(parent)
{
    buildFromFile("floater_url_entry.xml");
}

//-----------------------------------------------------------------------------
// ~LLFloaterURLEntry()
//-----------------------------------------------------------------------------
LLFloaterURLEntry::~LLFloaterURLEntry()
{
    sInstance = NULL;
}

BOOL LLFloaterURLEntry::postBuild()
{
    mMediaURLEdit = getChild<LLComboBox>("media_entry");

    // Cancel button
    childSetAction("cancel_btn", onBtnCancel, this);

    // Cancel button
    childSetAction("clear_btn", onBtnClear, this);
    // clear media list button
    LLSD parcel_history = LLURLHistory::getURLHistory("parcel");
    bool enable_clear_button = parcel_history.size() > 0 ? true : false;
    getChildView("clear_btn")->setEnabled(enable_clear_button );

    // OK button
    childSetAction("ok_btn", onBtnOK, this);

    setDefaultBtn("ok_btn");
    buildURLHistory();

    return TRUE;
}
void LLFloaterURLEntry::buildURLHistory()
{
    LLCtrlListInterface* url_list = childGetListInterface("media_entry");
    if (url_list)
    {
        url_list->operateOnAll(LLCtrlListInterface::OP_DELETE);
    }

    // Get all of the entries in the "parcel" collection
    LLSD parcel_history = LLURLHistory::getURLHistory("parcel");

    LLSD::array_iterator iter_history =
        parcel_history.beginArray();
    LLSD::array_iterator end_history =
        parcel_history.endArray();
    for(; iter_history != end_history; ++iter_history)
    {
        url_list->addSimpleElement((*iter_history).asString());
    }
}

void LLFloaterURLEntry::headerFetchComplete(S32 status, const std::string& mime_type)
{
    LLPanelLandMedia* panel_media = dynamic_cast<LLPanelLandMedia*>(mPanelLandMediaHandle.get());
    if (panel_media)
    {
        // status is ignored for now -- error = "none/none"
        panel_media->setMediaType(mime_type);
        panel_media->setMediaURL(mMediaURLEdit->getValue().asString());
    }

    getChildView("loading_label")->setVisible( false);
    closeFloater();
}

// static
LLHandle<LLFloater> LLFloaterURLEntry::show(LLHandle<LLPanel> parent, const std::string media_url)
{
    if (!sInstance)
    {
        sInstance = new LLFloaterURLEntry(parent);
    }
    sInstance->openFloater();
    sInstance->addURLToCombobox(media_url);
    return sInstance->getHandle();
}



bool LLFloaterURLEntry::addURLToCombobox(const std::string& media_url)
{
    if(! mMediaURLEdit->setSimple( media_url ) && ! media_url.empty())
    {
        mMediaURLEdit->add( media_url );
        mMediaURLEdit->setSimple( media_url );
        return true;
    }

    // URL was not added for whatever reason (either it was empty or already existed)
    return false;
}

// static
//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterURLEntry::onBtnOK( void* userdata )
{
    LLFloaterURLEntry *self =(LLFloaterURLEntry *)userdata;

    std::string media_url   = self->mMediaURLEdit->getValue().asString();
    self->mMediaURLEdit->remove(media_url);
    LLURLHistory::removeURL("parcel", media_url);
    if(self->addURLToCombobox(media_url))
    {
        // Add this url to the parcel collection
        LLURLHistory::addURL("parcel", media_url);
    }

    // show progress bar here?
    getWindow()->incBusyCount();
    self->getChildView("loading_label")->setVisible( true);

    // leading whitespace causes problems with the MIME-type detection so strip it
    LLStringUtil::trim( media_url );

    // First check the URL scheme
    LLURI url(media_url);
    std::string scheme = url.scheme();

    // We assume that an empty scheme is an http url, as this is how we will treat it.
    if(scheme == "")
    {
        scheme = "http";
    }

    // Discover the MIME type only for "http" scheme.
    if(!media_url.empty() && 
       (scheme == "http" || scheme == "https"))
    {
        LLCoros::instance().launch("LLFloaterURLEntry::getMediaTypeCoro",
            boost::bind(&LLFloaterURLEntry::getMediaTypeCoro, media_url, self->getHandle()));
    }
    else
    {
        self->headerFetchComplete(0, scheme);
    }

    // Grey the buttons until we get the header response
    self->getChildView("ok_btn")->setEnabled(false);
    self->getChildView("cancel_btn")->setEnabled(false);
    self->getChildView("media_entry")->setEnabled(false);
    self->getChildView("clear_btn")->setEnabled(false);
}

// static
void LLFloaterURLEntry::getMediaTypeCoro(std::string url, LLHandle<LLFloater> parentHandle)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("getMediaTypeCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);

    httpOpts->setHeadersOnly(true);

    LL_INFOS("HttpCoroutineAdapter", "genericPostCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    LLFloaterURLEntry* floaterUrlEntry = (LLFloaterURLEntry*)parentHandle.get();
    if (!floaterUrlEntry)
    {
        LL_WARNS() << "Could not get URL entry floater." << LL_ENDL;
        return;
    }

    // Set empty type to none/none.  Empty string is reserved for legacy parcels
    // which have no mime type set.
    std::string resolvedMimeType = LLMIMETypes::getDefaultMimeType();

    if (!status)
    {
        floaterUrlEntry->headerFetchComplete(status.getType(), resolvedMimeType);
        return;
    }

    LLSD resultHeaders = httpResults[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS_HEADERS];

    if (resultHeaders.has(HTTP_IN_HEADER_CONTENT_TYPE))
    {
        const std::string& mediaType = resultHeaders[HTTP_IN_HEADER_CONTENT_TYPE];
        std::string::size_type idx1 = mediaType.find_first_of(";");
        std::string mimeType = mediaType.substr(0, idx1);
        if (!mimeType.empty())
        {
            resolvedMimeType = mimeType;
        }
    }

    floaterUrlEntry->headerFetchComplete(status.getType(), resolvedMimeType);

}

// static
//-----------------------------------------------------------------------------
// onBtnCancel()
//-----------------------------------------------------------------------------
void LLFloaterURLEntry::onBtnCancel( void* userdata )
{
    LLFloaterURLEntry *self =(LLFloaterURLEntry *)userdata;
    self->closeFloater();
}

// static
//-----------------------------------------------------------------------------
// onBtnClear()
//-----------------------------------------------------------------------------
void LLFloaterURLEntry::onBtnClear( void* userdata )
{
    LLNotificationsUtil::add( "ConfirmClearMediaUrlList", LLSD(), LLSD(), 
                                    boost::bind(&LLFloaterURLEntry::callback_clear_url_list, (LLFloaterURLEntry*)userdata, _1, _2) );
}

bool LLFloaterURLEntry::callback_clear_url_list(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if ( option == 0 ) // YES
    {
        // clear saved list
        LLCtrlListInterface* url_list = childGetListInterface("media_entry");
        if ( url_list )
        {
            url_list->operateOnAll( LLCtrlListInterface::OP_DELETE );
        }

        // clear current contents of combo box
        mMediaURLEdit->clear();

        // clear stored version of list
        LLURLHistory::clear("parcel");

        // cleared the list so disable Clear button
        getChildView("clear_btn")->setEnabled(false );
    }
    return false;
}

void LLFloaterURLEntry::onClose( bool app_quitting )
{
    // Decrement the cursor
    getWindow()->decBusyCount();
}
