/** 
 * @file llmultifloater.cpp
 * @brief LLFloater that hosts other floaters
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

// Floating "windows" within the GL display, like the inventory floater,
// mini-map floater, etc.

#include "linden_common.h"

#include "llmultifloater.h"
#include "llresizehandle.h"

//
// LLMultiFloater
//

LLMultiFloater::LLMultiFloater(const LLSD& key, const LLFloater::Params& params)
	: LLFloater(key),
	  mTabContainer(NULL),
	  mTabPos(LLTabContainer::TOP),
	  mAutoResize(TRUE),
	  mOrigMinWidth(0),
	  mOrigMinHeight(0)
{
}

void LLMultiFloater::buildTabContainer()
{
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	
	LLTabContainer::Params p;
	p.name(std::string("Preview Tabs"));
	p.rect(LLRect(LLPANEL_BORDER_WIDTH, getRect().getHeight() - floater_header_size, getRect().getWidth() - LLPANEL_BORDER_WIDTH, 0));
	p.tab_position(mTabPos);
	p.follows.flags(FOLLOWS_ALL);
	p.commit_callback.function(boost::bind(&LLMultiFloater::onTabSelected, this));

	mTabContainer = LLUICtrlFactory::create<LLTabContainer>(p);
	addChild(mTabContainer);
	
	if (isResizable())
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}
}

void LLMultiFloater::onOpen(const LLSD& key)
{
// 	if (mTabContainer->getTabCount() <= 0)
// 	{
// 		// for now, don't allow multifloaters
// 		// without any child floaters
// 		closeFloater();
// 	}
}

void LLMultiFloater::draw()
{
	if (mTabContainer->getTabCount() == 0)
	{
		//RN: could this potentially crash in draw hierarchy?
		closeFloater();
	}
	else
	{
		LLFloater::draw();
	}
}

BOOL LLMultiFloater::closeAllFloaters()
{
	S32	tabToClose = 0;
	S32	lastTabCount = mTabContainer->getTabCount();
	while (tabToClose < mTabContainer->getTabCount())
	{
		LLFloater* first_floater = (LLFloater*)mTabContainer->getPanelByIndex(tabToClose);
		first_floater->closeFloater();
		if(lastTabCount == mTabContainer->getTabCount())
		{
			//Tab did not actually close, possibly due to a pending Save Confirmation dialog..
			//so try and close the next one in the list...
			tabToClose++;
		}
		else
		{
			//Tab closed ok.
			lastTabCount = mTabContainer->getTabCount();
		}
	}
	if( mTabContainer->getTabCount() != 0 )
		return FALSE; // Couldn't close all the tabs (pending save dialog?) so return FALSE.
	return TRUE; //else all tabs were successfully closed...
}

void LLMultiFloater::growToFit(S32 content_width, S32 content_height)
{
	static LLUICachedControl<S32> tabcntr_close_btn_size ("UITabCntrCloseBtnSize", 0);
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	S32 tabcntr_header_height = LLPANEL_BORDER_WIDTH + tabcntr_close_btn_size;
	S32 new_width = llmax(getRect().getWidth(), content_width + LLPANEL_BORDER_WIDTH * 2);
	S32 new_height = llmax(getRect().getHeight(), content_height + floater_header_size + tabcntr_header_height);

    if (isMinimized())
    {
        LLRect newrect;
        newrect.setLeftTopAndSize(getExpandedRect().mLeft, getExpandedRect().mTop, new_width, new_height);
        setExpandedRect(newrect);
    }
	else
	{
		S32 old_height = getRect().getHeight();
		reshape(new_width, new_height);
		// keep top left corner in same position
		translate(0, old_height - new_height);
	}
}

/**
  void addFloater(LLFloater* floaterp, BOOL select_added_floater)

  Adds the LLFloater pointed to by floaterp to this.
  If floaterp is already hosted by this, then it is re-added to get
  new titles, etc.
  If select_added_floater is true, the LLFloater pointed to by floaterp will
  become the selected tab in this

  Affects: mTabContainer, floaterp
**/
void LLMultiFloater::addFloater(LLFloater* floaterp, BOOL select_added_floater, LLTabContainer::eInsertionPoint insertion_point)
{
	if (!floaterp)
	{
		return;
	}

	if (!mTabContainer)
	{
		llerrs << "Tab Container used without having been initialized." << llendl;
		return;
	}

	if (floaterp->getHost() == this)
	{
		// already hosted by me, remove
		// do this so we get updated title, etc.
		mFloaterDataMap.erase(floaterp->getHandle());
		mTabContainer->removeTabPanel(floaterp);
	}
	else if (floaterp->getHost())
	{
		// floaterp is hosted by somebody else and
		// this is adding it, so remove it from it's old host
		floaterp->getHost()->removeFloater(floaterp);
	}
	else if (floaterp->getParent() == gFloaterView)
	{
		// rehost preview floater as child panel
		gFloaterView->removeChild(floaterp);
	}

	// store original configuration
	LLFloaterData floater_data;
	floater_data.mWidth = floaterp->getRect().getWidth();
	floater_data.mHeight = floaterp->getRect().getHeight();
	floater_data.mCanMinimize = floaterp->isMinimizeable();
	floater_data.mCanResize = floaterp->isResizable();

	// remove minimize and close buttons
	floaterp->setCanMinimize(FALSE);
	floaterp->setCanResize(FALSE);
	floaterp->setCanDrag(FALSE);
	floaterp->storeRectControl();
	// avoid double rendering of floater background (makes it more opaque)
	floaterp->setBackgroundVisible(FALSE);

	if (mAutoResize)
	{
		growToFit(floater_data.mWidth, floater_data.mHeight);
	}

	//add the panel, add it to proper maps
	mTabContainer->addTabPanel(
		LLTabContainer::TabPanelParams()
			.panel(floaterp)
			.label(floaterp->getShortTitle())
			.insert_at(insertion_point));
	mFloaterDataMap[floaterp->getHandle()] = floater_data;

	updateResizeLimits();

	if ( select_added_floater )
	{
		mTabContainer->selectTabPanel(floaterp);
	}
	else
	{
		// reassert visible tab (hiding new floater if necessary)
		mTabContainer->selectTab(mTabContainer->getCurrentPanelIndex());
	}

	floaterp->setHost(this);
	if (isMinimized())
	{
		floaterp->setVisible(FALSE);
	}
	
	// Tabs sometimes overlap resize handle
	moveResizeHandlesToFront();
}

void LLMultiFloater::updateFloaterTitle(LLFloater* floaterp)
{
	S32 index = mTabContainer->getIndexForPanel(floaterp);
	if (index != -1)
	{
		mTabContainer->setPanelTitle(index, floaterp->getShortTitle());
	}
}


/**
	BOOL selectFloater(LLFloater* floaterp)

	If the LLFloater pointed to by floaterp is hosted by this,
	then its tab is selected and returns true.  Otherwise returns false.

	Affects: mTabContainer
**/
BOOL LLMultiFloater::selectFloater(LLFloater* floaterp)
{
	return mTabContainer->selectTabPanel(floaterp);
}

// virtual
void LLMultiFloater::selectNextFloater()
{
	mTabContainer->selectNextTab();
}

// virtual
void LLMultiFloater::selectPrevFloater()
{
	mTabContainer->selectPrevTab();
}

void LLMultiFloater::showFloater(LLFloater* floaterp, LLTabContainer::eInsertionPoint insertion_point)
{
	if(!floaterp) return;
	// we won't select a panel that already is selected
	// it is hard to do this internally to tab container
	// as tab selection is handled via index and the tab at a given
	// index might have changed
	if (floaterp != mTabContainer->getCurrentPanel() &&
		!mTabContainer->selectTabPanel(floaterp))
	{
		addFloater(floaterp, TRUE, insertion_point);
	}
}

void LLMultiFloater::removeFloater(LLFloater* floaterp)
{
	if (!floaterp || floaterp->getHost() != this )
		return;

	floater_data_map_t::iterator found_data_it = mFloaterDataMap.find(floaterp->getHandle());
	if (found_data_it != mFloaterDataMap.end())
	{
		LLFloaterData& floater_data = found_data_it->second;
		floaterp->setCanMinimize(floater_data.mCanMinimize);
		if (!floater_data.mCanResize)
		{
			// restore original size
			floaterp->reshape(floater_data.mWidth, floater_data.mHeight);
		}
		floaterp->setCanResize(floater_data.mCanResize);
		mFloaterDataMap.erase(found_data_it);
	}
	mTabContainer->removeTabPanel(floaterp);
	floaterp->setBackgroundVisible(TRUE);
	floaterp->setCanDrag(TRUE);
	floaterp->setHost(NULL);
	floaterp->applyRectControl();

	updateResizeLimits();

	tabOpen((LLFloater*)mTabContainer->getCurrentPanel(), false);
}

void LLMultiFloater::tabOpen(LLFloater* opened_floater, bool from_click)
{
	// default implementation does nothing
}

void LLMultiFloater::tabClose()
{
	if (mTabContainer->getTabCount() == 0)
	{
		// no more children, close myself
		closeFloater();
	}
}

void LLMultiFloater::setVisible(BOOL visible)
{
	// *FIX: shouldn't have to do this, fix adding to minimized multifloater
	LLFloater::setVisible(visible);
	
	if (mTabContainer)
	{
		LLPanel* cur_floaterp = mTabContainer->getCurrentPanel();

		if (cur_floaterp)
		{
			cur_floaterp->setVisible(visible);
		}

		// if no tab selected, and we're being shown,
		// select last tab to be added
		if (visible && !cur_floaterp)
		{
			mTabContainer->selectLastTab();
		}
	}
}

BOOL LLMultiFloater::handleKeyHere(KEY key, MASK mask)
{
	if (key == 'W' && mask == MASK_CONTROL)
	{
		LLFloater* floater = getActiveFloater();
		// is user closeable and is system closeable
		if (floater && floater->canClose() && floater->isCloseable())
		{
			floater->closeFloater();

			// EXT-5695 (Tabbed IM window loses focus if close any tabs by Ctrl+W)
			// bring back focus on tab container if there are any tab left
			if(mTabContainer->getTabCount() > 0)
			{
				mTabContainer->setFocus(TRUE);
			}
		}
		return TRUE;
	}

	return LLFloater::handleKeyHere(key, mask);
}

bool LLMultiFloater::addChild(LLView* child, S32 tab_group)
{
	LLTabContainer* tab_container = dynamic_cast<LLTabContainer*>(child);
	if (tab_container)
	{
		// store pointer to tab container
		setTabContainer(tab_container);
	}

	// then go ahead and add child as usual
	return LLFloater::addChild(child, tab_group);
}

LLFloater* LLMultiFloater::getActiveFloater()
{
	return (LLFloater*)mTabContainer->getCurrentPanel();
}

S32	LLMultiFloater::getFloaterCount()
{
	return mTabContainer->getTabCount();
}

/**
	BOOL isFloaterFlashing(LLFloater* floaterp)

	Returns true if the LLFloater pointed to by floaterp
	is currently in a flashing state and is hosted by this.
	False otherwise.

	Requires: floaterp != NULL
**/
BOOL LLMultiFloater::isFloaterFlashing(LLFloater* floaterp)
{
	if ( floaterp && floaterp->getHost() == this )
		return mTabContainer->getTabPanelFlashing(floaterp);

	return FALSE;
}

/**
	BOOL setFloaterFlashing(LLFloater* floaterp, BOOL flashing)

	Sets the current flashing state of the LLFloater pointed
	to by floaterp to be the BOOL flashing if the LLFloater pointed
	to by floaterp is hosted by this.

	Requires: floaterp != NULL
**/
void LLMultiFloater::setFloaterFlashing(LLFloater* floaterp, BOOL flashing)
{
	if ( floaterp && floaterp->getHost() == this )
		mTabContainer->setTabPanelFlashing(floaterp, flashing);
}

void LLMultiFloater::onTabSelected()
{
	LLFloater* floaterp = dynamic_cast<LLFloater*>(mTabContainer->getCurrentPanel());
	if (floaterp)
	{
		tabOpen(floaterp, true);
	}
}

void LLMultiFloater::setCanResize(BOOL can_resize)
{
	LLFloater::setCanResize(can_resize);
	if (!mTabContainer) return;
	if (isResizable() && mTabContainer->getTabPosition() == LLTabContainer::BOTTOM)
	{
		mTabContainer->setRightTabBtnOffset(RESIZE_HANDLE_WIDTH);
	}
	else
	{
		mTabContainer->setRightTabBtnOffset(0);
	}
}

BOOL LLMultiFloater::postBuild()
{
	mCloseSignal.connect(boost::bind(&LLMultiFloater::closeAllFloaters, this));
		
	// remember any original xml minimum size
	getResizeLimits(&mOrigMinWidth, &mOrigMinHeight);

	if (mTabContainer)
	{
		return TRUE;
	}

	mTabContainer = getChild<LLTabContainer>("Preview Tabs");
	
	setCanResize(mResizable);
	return TRUE;
}

void LLMultiFloater::updateResizeLimits()
{
	static LLUICachedControl<S32> tabcntr_close_btn_size ("UITabCntrCloseBtnSize", 0);
	const LLFloater::Params& default_params = LLFloater::getDefaultParams();
	S32 floater_header_size = default_params.header_height;
	S32 tabcntr_header_height = LLPANEL_BORDER_WIDTH + tabcntr_close_btn_size;
	// initialize minimum size constraint to the original xml values.
	S32 new_min_width = mOrigMinWidth;
	S32 new_min_height = mOrigMinHeight;
	// possibly increase minimum size constraint due to children's minimums.
	for (S32 tab_idx = 0; tab_idx < mTabContainer->getTabCount(); ++tab_idx)
	{
		LLFloater* floaterp = (LLFloater*)mTabContainer->getPanelByIndex(tab_idx);
		if (floaterp)
		{
			new_min_width = llmax(new_min_width, floaterp->getMinWidth() + LLPANEL_BORDER_WIDTH * 2);
			new_min_height = llmax(new_min_height, floaterp->getMinHeight() + floater_header_size + tabcntr_header_height);
		}
	}
	setResizeLimits(new_min_width, new_min_height);

	S32 cur_height = getRect().getHeight();
	S32 new_width = llmax(getRect().getWidth(), new_min_width);
	S32 new_height = llmax(getRect().getHeight(), new_min_height);

	if (isMinimized())
	{
		const LLRect& expanded = getExpandedRect();
		LLRect newrect;
		newrect.setLeftTopAndSize(expanded.mLeft, expanded.mTop, llmax(expanded.getWidth(), new_width), llmax(expanded.getHeight(), new_height));
		setExpandedRect(newrect);
	}
	else
	{
		reshape(new_width, new_height);

		// make sure upper left corner doesn't move
		translate(0, cur_height - getRect().getHeight());

		// make sure this window is visible on screen when it has been modified
		// (tab added, etc)
		gFloaterView->adjustToFitScreen(this, TRUE);
	}
}
