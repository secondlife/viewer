/** 
 * @file llfloatertos.cpp
 * @brief Terms of Service Agreement dialog
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#include "llfloatertos.h"

// viewer includes
#include "llviewerstats.h"
#include "llviewerwindow.h"

// linden library includes
#include "llbutton.h"
#include "llevents.h"
#include "llnotificationsutil.h"
#include "llradiogroup.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluictrlfactory.h"
#include "llvfile.h"
#include "message.h"
#include "llstartup.h"              // login_alert_done
#include "llcorehttputil.h"
#include "llfloaterreg.h"

LLFloaterTOS::LLFloaterTOS(const LLSD& data)
:	LLModalDialog( data["message"].asString() ),
	mMessage(data["message"].asString()),
	mLoadingScreenLoaded(false),
	mSiteAlive(false),
	mRealNavigateBegun(false),
	mReplyPumpName(data["reply_pump"].asString())
{
}

BOOL LLFloaterTOS::postBuild()
{	
	childSetAction("Continue", onContinue, this);
	childSetAction("Cancel", onCancel, this);
	childSetCommitCallback("agree_chk", updateAgree, this);
	
	if (hasChild("tos_text"))
	{
		// this displays the critical message
		LLUICtrl *tos_text = getChild<LLUICtrl>("tos_text");
		tos_text->setEnabled( FALSE );
		tos_text->setFocus(TRUE);
		tos_text->setValue(LLSD(mMessage));

		return TRUE;
	}

	// disable Agree to TOS radio button until the page has fully loaded
        updateAgreeEnabled(false);

	// hide the SL text widget if we're displaying TOS with using a browser widget.
	LLUICtrl *editor = getChild<LLUICtrl>("tos_text");
	editor->setVisible( FALSE );

	LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos_html");
	if ( web_browser )
	{
// if we are forced to send users to an external site in their system browser
// (e.g.) Linux users because of lack of media support for HTML ToS page
// remove exisiting UI and replace with a link to external page where users can accept ToS
#ifdef EXTERNAL_TOS
		LLTextBox* header = getChild<LLTextBox>("tos_heading");
		if (header)
			header->setVisible(false);

		LLTextBox* external_prompt = getChild<LLTextBox>("external_tos_required");
		if (external_prompt)
			external_prompt->setVisible(true);

		web_browser->setVisible(false);
#else
		web_browser->addObserver(this);

		// Don't use the start_url parameter for this browser instance -- it may finish loading before we get to add our observer.
		// Store the URL separately and navigate here instead.
		web_browser->navigateTo( getString( "loading_url" ) );
		LLPluginClassMedia* media_plugin = web_browser->getMediaPlugin();
		if (media_plugin)
		{
			// All links from tos_html should be opened in external browser
			media_plugin->setOverrideClickTarget("_external");
		}
#endif
	}

	return TRUE;
}

void LLFloaterTOS::setSiteIsAlive( bool alive )
{
// if we are forced to send users to an external site in their system browser
// (e.g.) Linux users because of lack of media support for HTML ToS page
// force the regular HTML UI to deactivate so alternative is rendered instead.
#ifdef EXTERNAL_TOS
	mSiteAlive = false;
#else

	mSiteAlive = alive;
	
	// only do this for TOS pages
	if (hasChild("tos_html"))
	{
		// if the contents of the site was retrieved
		if ( alive )
		{
			// navigate to the "real" page 
			if(!mRealNavigateBegun && mSiteAlive)
			{
				LLMediaCtrl* web_browser = getChild<LLMediaCtrl>("tos_html");
				if(web_browser)
				{
					mRealNavigateBegun = true;
					web_browser->navigateTo( getString( "real_url" ) );
				}
			}
		}
		else
		{
			LL_INFOS("TOS") << "ToS page: ToS page unavailable!" << LL_ENDL;
			// normally this is set when navigation to TOS page navigation completes (so you can't accept before TOS loads)
			// but if the page is unavailable, we need to do this now
			updateAgreeEnabled(true);
			LLTextBox* tos_list = getChild<LLTextBox>("agree_list");
			tos_list->setEnabled(true);
		}
	}
#endif
}

LLFloaterTOS::~LLFloaterTOS()
{
}

// virtual
void LLFloaterTOS::draw()
{
	// draw children
	LLModalDialog::draw();
}


// update status of "Agree" checkbox and text
void LLFloaterTOS::updateAgreeEnabled(bool enabled)
{
	LLCheckBoxCtrl* tos_agreement_agree_cb = getChild<LLCheckBoxCtrl>("agree_chk");
	tos_agreement_agree_cb->setEnabled(enabled);

	LLTextBox* tos_agreement_agree_text = getChild<LLTextBox>("agree_list");
	tos_agreement_agree_text->setEnabled(enabled);
}

// static
void LLFloaterTOS::updateAgree(LLUICtrl*, void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	bool agree = self->getChild<LLUICtrl>("agree_chk")->getValue().asBoolean(); 
	self->getChildView("Continue")->setEnabled(agree);
}

// static
void LLFloaterTOS::onContinue( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	LL_INFOS("TOS") << "User agrees with TOS." << LL_ENDL;

	if(self->mReplyPumpName != "")
	{
		LLEventPumps::instance().obtain(self->mReplyPumpName).post(LLSD(true));
	}

	self->closeFloater(); // destroys this object
}

// static
void LLFloaterTOS::onCancel( void* userdata )
{
	LLFloaterTOS* self = (LLFloaterTOS*) userdata;
	LL_INFOS("TOS") << "User disagrees with TOS." << LL_ENDL;
	LLNotificationsUtil::add("MustAgreeToLogIn", LLSD(), LLSD(), login_alert_done);

	if(self->mReplyPumpName != "")
	{
		LLEventPumps::instance().obtain(self->mReplyPumpName).post(LLSD(false));
	}

	// reset state for next time we come to TOS
	self->mLoadingScreenLoaded = false;
	self->mSiteAlive = false;
	self->mRealNavigateBegun = false;
	
	// destroys this object
	self->closeFloater(); 
}

//virtual 
void LLFloaterTOS::handleMediaEvent(LLPluginClassMedia* /*self*/, EMediaEvent event)
{
	if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		if(!mLoadingScreenLoaded)
		{
			mLoadingScreenLoaded = true;
            std::string url(getString("real_url"));

            LLHandle<LLFloater> handle = getHandle();

            LLCoros::instance().launch("LLFloaterTOS::testSiteIsAliveCoro",
                boost::bind(&LLFloaterTOS::testSiteIsAliveCoro, handle, url));
		}
		else if(mRealNavigateBegun)
		{
			LL_INFOS("TOS") << "TOS: NAVIGATE COMPLETE" << LL_ENDL;
			// enable Agree to TOS check box now that page has loaded
			updateAgreeEnabled(true);
		}
	}
}

void LLFloaterTOS::testSiteIsAliveCoro(LLHandle<LLFloater> handle, std::string url)
{
    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("genericPostCoro", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
	httpOpts->setHeadersOnly(true);

    LL_INFOS("testSiteIsAliveCoro") << "Generic POST for " << url << LL_ENDL;

    LLSD result = httpAdapter->getAndSuspend(httpRequest, url);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (handle.isDead())
    {
        LL_WARNS("testSiteIsAliveCoro") << "Dialog canceled before response." << LL_ENDL;
        return;
    }

    LLFloaterTOS *that = dynamic_cast<LLFloaterTOS *>(handle.get());
    
    if (that)
        that->setSiteIsAlive(static_cast<bool>(status)); 
    else
    {
        LL_WARNS("testSiteIsAliveCoro") << "Handle was not a TOS floater." << LL_ENDL;
    }
}


