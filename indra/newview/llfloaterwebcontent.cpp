/** 
 * @file llfloaterwebcontent.cpp
 * @brief floater for displaying web content - e.g. profiles and search (eventually)
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llfloaterwebcontent.h"

#include "llfloaterreg.h"
#include "llparcel.h"
#include "llpluginclassmedia.h"
#include "lluictrlfactory.h"
#include "llmediactrl.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "llweb.h"
#include "llui.h"
#include "roles_constants.h"

#include "llurlhistory.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llcombobox.h"
#include "llwindow.h"
#include "lllayoutstack.h"
#include "llcheckboxctrl.h"

#include "llnotifications.h"

// TEMP
#include "llsdutil.h"

LLFloaterWebContent::LLFloaterWebContent(const LLSD& key)
	: LLFloater(key)
{
}

//static 
void LLFloaterWebContent::create(const std::string &url, const std::string& target, const std::string& uuid)
{
	lldebugs << "url = " << url << ", target = " << target << ", uuid = " << uuid << llendl;
	
	std::string tag = target;
	
	if(target.empty() || target == "_blank")
	{
		if(!uuid.empty())
		{
			tag = uuid;
		}
		else
		{
			// create a unique tag for this instance
			LLUUID id;
			id.generate();
			tag = id.asString();
		}
	}
	
	S32 browser_window_limit = gSavedSettings.getS32("MediaBrowserWindowLimit");
	
	if(LLFloaterReg::findInstance("web_content", tag) != NULL)
	{
		// There's already a web browser for this tag, so we won't be opening a new window.
	}
	else if(browser_window_limit != 0)
	{
		// showInstance will open a new window.  Figure out how many web browsers are already open, 
		// and close the least recently opened one if this will put us over the limit.
		
		LLFloaterReg::const_instance_list_t &instances = LLFloaterReg::getFloaterList("web_content");
		lldebugs << "total instance count is " << instances.size() << llendl;
		
		for(LLFloaterReg::const_instance_list_t::const_iterator iter = instances.begin(); iter != instances.end(); iter++)
		{
			lldebugs << "    " << (*iter)->getKey() << llendl;
		}
		
		if(instances.size() >= (size_t)browser_window_limit)
		{
			// Destroy the least recently opened instance
			(*instances.begin())->closeFloater();
		}
	}

	LLFloaterWebContent *browser = dynamic_cast<LLFloaterWebContent*> (LLFloaterReg::showInstance("web_content", tag));
	llassert(browser);
	if(browser)
	{
		browser->mUUID = uuid;

		// tell the browser instance to load the specified URL
		browser->openMedia(url, target);
		LLViewerMedia::proxyWindowOpened(target, uuid);
	}
}

//static 
void LLFloaterWebContent::closeRequest(const std::string &uuid)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("web_content");
	lldebugs << "instance list size is " << inst_list.size() << ", incoming uuid is " << uuid << llendl;
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterWebContent* i = dynamic_cast<LLFloaterWebContent*>(*iter);
		lldebugs << "    " << i->mUUID << llendl;
		if (i && i->mUUID == uuid)
		{
			i->closeFloater(false);
			return;
 		}
 	}
}

//static 
void LLFloaterWebContent::geometryChanged(const std::string &uuid, S32 x, S32 y, S32 width, S32 height)
{
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("web_content");
	lldebugs << "instance list size is " << inst_list.size() << ", incoming uuid is " << uuid << llendl;
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterWebContent* i = dynamic_cast<LLFloaterWebContent*>(*iter);
		lldebugs << "    " << i->mUUID << llendl;
		if (i && i->mUUID == uuid)
		{
			i->geometryChanged(x, y, width, height);
			return;
		}
	}
}
	
void LLFloaterWebContent::geometryChanged(S32 x, S32 y, S32 width, S32 height)
{	
	// Make sure the layout of the browser control is updated, so this calculation is correct.
	LLLayoutStack::updateClass();
		
	// TODO: need to adjust size and constrain position to make sure floaters aren't moved outside the window view, etc.
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);

	// Adjust width and height for the size of the chrome on the web Browser window.
	width += getRect().getWidth() - mWebBrowser->getRect().getWidth();
	height += getRect().getHeight() - mWebBrowser->getRect().getHeight();
	
	LLRect geom;
	geom.setOriginAndSize(x, window_size.mY - (y + height), width, height);

	lldebugs << "geometry change: " << geom << llendl;
	
	handleReshape(geom,false);
}

void LLFloaterWebContent::openMedia(const std::string& web_url, const std::string& target)
{
	mWebBrowser->setHomePageUrl(web_url);
	mWebBrowser->setTarget(target);
	mWebBrowser->navigateTo(web_url);
	setCurrentURL(web_url);
}

void LLFloaterWebContent::draw()
{
//	getChildView("go")->setEnabled(!mAddressCombo->getValue().asString().empty());

	getChildView("back")->setEnabled(mWebBrowser->canNavigateBack());
	getChildView("forward")->setEnabled(mWebBrowser->canNavigateForward());

	LLFloater::draw();
}

BOOL LLFloaterWebContent::postBuild()
{
	mWebBrowser = getChild<LLMediaCtrl>("webbrowser");
	mWebBrowser->addObserver(this);

	childSetAction("back", onClickBack, this);
	childSetAction("forward", onClickForward, this);
	childSetAction("reload", onClickRefresh, this);
	childSetAction("go", onClickGo, this);

	return TRUE;
}

//virtual
void LLFloaterWebContent::onClose(bool app_quitting)
{
	LLViewerMedia::proxyWindowClosed(mUUID);
	destroy();
}

void LLFloaterWebContent::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	if(event == MEDIA_EVENT_LOCATION_CHANGED)
	{
		setCurrentURL(self->getLocation());
	}
	else if(event == MEDIA_EVENT_NAVIGATE_COMPLETE)
	{
		// This is the event these flags are sent with.
		getChildView("back")->setEnabled(self->getHistoryBackAvailable());
		getChildView("forward")->setEnabled(self->getHistoryForwardAvailable());
	}
	else if(event == MEDIA_EVENT_CLOSE_REQUEST)
	{
		// The browser instance wants its window closed.
		closeFloater();
	}
	else if(event == MEDIA_EVENT_GEOMETRY_CHANGE)
	{
		geometryChanged(self->getGeometryX(), self->getGeometryY(), self->getGeometryWidth(), self->getGeometryHeight());
	}
}

void LLFloaterWebContent::setCurrentURL(const std::string& url)
{
	mCurrentURL = url;

	getChildView("back")->setEnabled(mWebBrowser->canNavigateBack());
	getChildView("forward")->setEnabled(mWebBrowser->canNavigateForward());
	getChildView("reload")->setEnabled(TRUE);
}

//static 
void LLFloaterWebContent::onClickRefresh(void* user_data)
{
	LLFloaterWebContent* self = (LLFloaterWebContent*)user_data;

	self->mWebBrowser->navigateTo(self->mCurrentURL);
}

//static 
void LLFloaterWebContent::onClickForward(void* user_data)
{
	LLFloaterWebContent* self = (LLFloaterWebContent*)user_data;

	self->mWebBrowser->navigateForward();
}

//static 
void LLFloaterWebContent::onClickBack(void* user_data)
{
	LLFloaterWebContent* self = (LLFloaterWebContent*)user_data;

	self->mWebBrowser->navigateBack();
}

//static 
void LLFloaterWebContent::onClickGo(void* user_data)
{
//	LLFloaterWebContent* self = (LLFloaterWebContent*)user_data;

//	self->mWebBrowser->navigateTo(self->mAddressCombo->getValue().asString());
}
