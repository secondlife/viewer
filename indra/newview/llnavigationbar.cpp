/** 
 * @file llnavigationbar.cpp
 * @brief Navigation bar implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llnavigationbar.h"

#include "llfocusmgr.h"
#include "llmenugl.h"
#include "llparcel.h"
#include "llregistry.h"
#include "llwindow.h"

#include "llagent.h"
#include "llfloaterhtmlhelp.h"
#include "llfloaterreg.h"
#include "lllocationhistory.h"
#include "lllocationinputctrl.h"
#include "llteleporthistory.h"
#include "llslurl.h"
#include "llurlsimstring.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llworldmap.h"


/*
TODO:
- Load navbar height from saved settings (as it's done for status bar) or think of a better way.
- Share location info formatting code with LLStatusBar.
- Fix notifications appearing below navbar.
- Navbar should not be visible in mouselook mode.
*/

S32 NAVIGATION_BAR_HEIGHT = 60; // *HACK
LLNavigationBar* LLNavigationBar::sInstance = 0;

LLNavigationBar* LLNavigationBar::getInstance()
{
	if (!sInstance)
		sInstance = new LLNavigationBar();

	return sInstance;
}

LLNavigationBar::LLNavigationBar()
:	mTeleportHistoryMenu(NULL),
	mLocationContextMenu(NULL),
	mBtnBack(NULL),
	mBtnForward(NULL),
	mBtnHome(NULL),
	mBtnInfo(NULL),
	mBtnHelp(NULL),
	mCmbLocation(NULL),
	mLeSearch(NULL)
{
	setIsChrome(TRUE);

	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_navigation_bar.xml");

	// navigation bar can never get a tab
	setFocusRoot(FALSE);

}

LLNavigationBar::~LLNavigationBar()
{
	sInstance = 0;
}

BOOL LLNavigationBar::postBuild()
{
	mBtnBack	= getChild<LLButton>("back_btn");
	mBtnForward	= getChild<LLButton>("forward_btn");
	mBtnHome	= getChild<LLButton>("home_btn");
	mBtnInfo	= getChild<LLButton>("info_btn");
	mBtnHelp	= getChild<LLButton>("help_btn");
	
	mCmbLocation= getChild<LLLocationInputCtrl>("location_combo"); 
	mLeSearch	= getChild<LLLineEditor>("search_input");
	
	if (!mBtnBack || !mBtnForward || !mBtnHome || !mBtnInfo || !mBtnHelp ||
		!mCmbLocation || !mLeSearch)
	{
		llwarns << "Malformed navigation bar" << llendl;
		return FALSE;
	}
	
	mBtnBack->setEnabled(FALSE);
	mBtnBack->setClickedCallback(boost::bind(&LLNavigationBar::onBackButtonClicked, this));
	mBtnBack->setHeldDownCallback(boost::bind(&LLNavigationBar::onBackOrForwardButtonHeldDown, this, _2));

	mBtnForward->setEnabled(FALSE);
	mBtnForward->setClickedCallback(boost::bind(&LLNavigationBar::onForwardButtonClicked, this));
	mBtnForward->setHeldDownCallback(boost::bind(&LLNavigationBar::onBackOrForwardButtonHeldDown, this, _2));

	mBtnHome->setClickedCallback(boost::bind(&LLNavigationBar::onHomeButtonClicked, this));
	mBtnInfo->setClickedCallback(boost::bind(&LLNavigationBar::onInfoButtonClicked, this));
	mBtnHelp->setClickedCallback(boost::bind(&LLNavigationBar::onHelpButtonClicked, this));

	mCmbLocation->setFocusReceivedCallback(boost::bind(&LLNavigationBar::onLocationFocusReceived, this));
	mCmbLocation->setFocusLostCallback(boost::bind(&LLNavigationBar::onLocationFocusLost, this));
	mCmbLocation->setTextEntryCallback(boost::bind(&LLNavigationBar::onLocationTextEntry, this, _1));
	mCmbLocation->setPrearrangeCallback(boost::bind(&LLNavigationBar::onLocationPrearrange, this, _2));
	mCmbLocation->setSelectionCallback(boost::bind(&LLNavigationBar::onLocationSelection, this));
	
	mLeSearch->setCommitCallback(boost::bind(&LLNavigationBar::onSearchCommit, this));

	// Register callbacks and load the location field context menu (NB: the order matters).
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar commit_registrar;
	LLMenuItemGL::EnableCallbackRegistry::ScopedRegistrar enable_registrar;
	commit_registrar.add("Navbar.Action", boost::bind(&LLNavigationBar::onLocationContextMenuItemClicked, this, _2));
	enable_registrar.add("Navbar.EnableMenuItem", boost::bind(&LLNavigationBar::onLocationContextMenuItemEnabled, this, _2));
	mLocationContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_navbar.xml", this);
	if (!mLocationContextMenu)
	{
		llwarns << "Error loading navigation bar context menu" << llendl;
		return FALSE;
	}

	// we'll be notified on teleport history changes
	LLTeleportHistory::getInstance()->setHistoryChangedCallback(
			boost::bind(&LLNavigationBar::onTeleportHistoryChanged, this));
	
	LLLocationHistory::getInstance()->setLoadedCallback(
			boost::bind(&LLNavigationBar::onLocationHistoryLoaded, this));
	
	LLLocationHistory::getInstance()->load(); // *TODO: temporary, remove this after debugging
	LLTeleportHistory::getInstance()->load(); // *TODO: temporary, remove this after debugging
	
	return TRUE;
}

void LLNavigationBar::draw()
{
	// *TODO: It doesn't look very optimal to refresh location every frame.
	refreshLocation();
	LLPanel::draw();
}

BOOL LLNavigationBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	// If the location field is clicked then show its context menu.
	if (mCmbLocation->getRect().pointInRect(x, y))
	{

		// Pass the focus to the line editor when it is righ-clicked
		mCmbLocation->setFocus(TRUE);

		// IAN BUG why do the individual items need to be enabled individually here?
		// where are they disabled?

		if (mLocationContextMenu)
		{
			mLocationContextMenu->setItemEnabled("Cut",			mCmbLocation->canCut());
			mLocationContextMenu->setItemEnabled("Copy", 		mCmbLocation->canCopy());
			mLocationContextMenu->setItemEnabled("Paste", 		mCmbLocation->canPaste());
			mLocationContextMenu->setItemEnabled("Delete",		mCmbLocation->canDeselect());
			mLocationContextMenu->setItemEnabled("Select All",	mCmbLocation->canSelectAll());

			mLocationContextMenu->buildDrawLabels();
			mLocationContextMenu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, mLocationContextMenu, x, y);
		}
	}
	return TRUE;
}

void LLNavigationBar::onBackButtonClicked()
{
	LLTeleportHistory::getInstance()->goBack();
}

void LLNavigationBar::onBackOrForwardButtonHeldDown(const LLSD& param)
{
	if (param["count"].asInteger() == 0)
		showTeleportHistoryMenu();
}

void LLNavigationBar::onForwardButtonClicked()
{
	LLTeleportHistory::getInstance()->goForward();
}

void LLNavigationBar::onHomeButtonClicked()
{
	gAgent.teleportHome();
}

void LLNavigationBar::onInfoButtonClicked()
{
	// XXX temporary
	LLTeleportHistory::getInstance()->dump();
	LLLocationHistory::getInstance()->dump();
}

void LLNavigationBar::onHelpButtonClicked()
{
	gViewerHtmlHelp.show();
}

void LLNavigationBar::onSearchCommit()
{
	std::string search_text = mLeSearch->getText();
	LLFloaterReg::showInstance("search", LLSD().insert("panel", "all").insert("id", LLSD(search_text)));
}

void LLNavigationBar::onLocationFocusReceived()
{
	mCmbLocation->setTextEntry(gAgent.getSLURL());
}

void LLNavigationBar::onLocationFocusLost()
{
}

void LLNavigationBar::onTeleportHistoryMenuItemClicked(const LLSD& userdata)
{
	int idx = userdata.asInteger();
	LLTeleportHistory::getInstance()->goToItem(idx);
}

// This is called when user presses enter in the location input
// or selects a location from the typed locations dropdown.
void LLNavigationBar::onLocationSelection()
{
	std::string loc_str = mCmbLocation->getSimple();

	// Will not teleport to empty location.
	if (loc_str.empty())
		return;
	
	// *TODO: validate location before adding it to the history.
	S32 selected_item = mCmbLocation->getCurrentIndex();
	if (selected_item == -1) // user has typed text
	{
		LLLocationHistory* lh = LLLocationHistory::getInstance();
		mCmbLocation->add(loc_str);
		lh->addItem(loc_str);
		lh->save();
	}

	// If the input is not a SLURL treat it as a region name.
	if (!LLSLURL::isSLURL(loc_str))
	{
		loc_str = LLSLURL::buildSLURL(loc_str, 128, 128, 0);
	}
	
	teleport(loc_str);
}

void LLNavigationBar::onLocationTextEntry(LLUICtrl* ctrl)
{
	LLLineEditor* editor = dynamic_cast<LLLineEditor*>(ctrl);
	if (!editor)
		return;

	// *TODO: decide whether to populate the list here on in LLLocationInputCtrl.
	std::string text = editor->getText();
	//rebuildLocationHistory(text);
}

void LLNavigationBar::onLocationPrearrange(const LLSD& data)
{
	std::string filter = data.asString();
	rebuildLocationHistory(filter);
}

void LLNavigationBar::onLocationHistoryLoaded()
{
	rebuildLocationHistory();
}

void LLNavigationBar::onTeleportHistoryChanged()
{
	// Update navigation controls.
	LLTeleportHistory* h = LLTeleportHistory::getInstance();
	int cur_item = h->getCurrentItemIndex();
	mBtnBack->setEnabled(cur_item > 0);
	mBtnForward->setEnabled(cur_item < ((int)h->getItems().size() - 1));
}

void LLNavigationBar::refreshLocation()
{
	// Update location field.
	if (mCmbLocation && !mCmbLocation->childHasFocus())
	{
		std::string location_name;

		if (!gAgent.buildLocationString(location_name, LLAgent::LOCATION_FORMAT_FULL))
			location_name = "Unknown";

		mCmbLocation->setText(location_name);
	}
}

void LLNavigationBar::rebuildLocationHistory(std::string filter)
{
	if (!mCmbLocation)
	{
		llwarns << "Cannot find location history control" << llendl;
		return;
	}
	
	LLLocationHistory::location_list_t filtered_items;
	const LLLocationHistory::location_list_t* itemsp = NULL;
	LLLocationHistory* lh = LLLocationHistory::getInstance();
	
	if (filter.empty())
		itemsp = &lh->getItems();
	else
	{
		lh->getMatchingItems(filter, filtered_items);
		itemsp = &filtered_items;
	}
	
	mCmbLocation->removeall();
	for (LLLocationHistory::location_list_t::const_reverse_iterator it = itemsp->rbegin(); it != itemsp->rend(); it++)
		mCmbLocation->add(*it);
}

void LLNavigationBar::rebuildTeleportHistoryMenu()
{
	// Has the pop-up menu been built?
	if (mTeleportHistoryMenu)
	{
		// Clear it.
		// *TODO: LLMenuGL should have a method for removing all items.
		while (mTeleportHistoryMenu->getItemCount())
		{
			LLMenuItemGL* itemp = mTeleportHistoryMenu->getItem(0);
			mTeleportHistoryMenu->removeChild(itemp);
		}
	}
	else
	{
		// Create it.
		LLMenuGL::Params menu_p;
		menu_p.name("popup");
		menu_p.can_tear_off(false);
		menu_p.visible(false);
		menu_p.bg_visible(true);
		mTeleportHistoryMenu = LLUICtrlFactory::create<LLMenuGL>(menu_p);
		
		addChild(mTeleportHistoryMenu);
	}
	
	// Populate the menu with teleport history items.
	LLTeleportHistory* hist = LLTeleportHistory::getInstance();
	const LLTeleportHistory::slurl_list_t& hist_items = hist->getItems();
	int cur_item = hist->getCurrentItemIndex();
	
	// Items will be shown in the reverse order, just like in Firefox.
	for (int i = (int)hist_items.size()-1; i >= 0; i--)
	{
		LLMenuItemCallGL::Params item_params;
		std::string title = hist_items[i].mTitle;
		
		if (i == cur_item)
			item_params.font.style("BOLD");
		else
			title = "   " + title;

		item_params.name(title);
		item_params.label(title);
		item_params.on_click.function(boost::bind(&LLNavigationBar::onTeleportHistoryMenuItemClicked, this, i));
		mTeleportHistoryMenu->addChild(LLUICtrlFactory::create<LLMenuItemCallGL>(item_params));
	}
}

// static
void LLNavigationBar::onRegionNameResponse(
		LLVector3 local_coords,
		U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	LLVector3d region_pos = from_region_handle(region_handle);
	LLVector3d global_pos = region_pos + (LLVector3d) local_coords;
	
	llinfos << "Teleporting to: " << global_pos  << llendl;
	gAgent.teleportViaLocation(global_pos);
}

// static
void LLNavigationBar::teleport(std::string slurl)
{
	std::string sim_string = LLSLURL::stripProtocol(slurl);
	std::string region_name;
	S32 x = 128;
	S32 y = 128;
	S32 z = 0;

	LLURLSimString::parse(sim_string, &region_name, &x, &y, &z);

	// Resolve region name to global coords.
	LLVector3 local_coords(x, y, z);
	LLWorldMap::getInstance()->sendNamedRegionRequest(region_name,
			boost::bind(&LLNavigationBar::onRegionNameResponse, local_coords, _1, _2, _3, _4),
			slurl,
			false); // don't teleport
}

void	LLNavigationBar::showTeleportHistoryMenu()
{
	// Don't show the popup if teleport history is empty.
	if (LLTeleportHistory::getInstance()->isEmpty())
	{
		lldebugs << "Teleport history is empty, will not show the menu." << llendl;
		return;
	}
	
	rebuildTeleportHistoryMenu();

	if (mTeleportHistoryMenu == NULL)
		return;
	
	// *TODO: why to draw/update anything before showing the menu?
	mTeleportHistoryMenu->buildDrawLabels();
	mTeleportHistoryMenu->updateParent(LLMenuGL::sMenuContainer);
	LLRect btnBackRect = mBtnBack->getRect();
	LLMenuGL::showPopup(this, mTeleportHistoryMenu, btnBackRect.mLeft, btnBackRect.mBottom);

	// *HACK pass the mouse capturing to the drop-down menu
	gFocusMgr.setMouseCapture( mTeleportHistoryMenu );
}

void LLNavigationBar::onLocationContextMenuItemClicked(const LLSD& userdata)
{
	std::string level = userdata.asString();

	if (level == std::string("copy_url"))
	{
		LLUIString url(gAgent.getSLURL());
		LLView::getWindow()->copyTextToClipboard(url.getWString());
		lldebugs << "Copy SLURL" << llendl;
	}
	else if (level == std::string("landmark"))
	{
		// *TODO To be implemented
		lldebugs << "Add Landmark" << llendl;
	}
	else if (level == std::string("cut"))
	{
		mCmbLocation->cut();
		lldebugs << "Cut" << llendl;
	}
	else if (level == std::string("copy"))
	{
		mCmbLocation->copy();
		lldebugs << "Copy" << llendl;
	}
	else if (level == std::string("paste"))
	{
		mCmbLocation->paste();
		lldebugs << "Paste" << llendl;
	}
	else if (level == std::string("delete"))
	{
		mCmbLocation->deleteSelection();
		lldebugs << "Delete" << llendl;
	}
	else if (level == std::string("select_all"))
	{
		mCmbLocation->selectAll();
		lldebugs << "Select All" << llendl;
	}
}

bool LLNavigationBar::onLocationContextMenuItemEnabled(const LLSD& userdata)
{
	std::string level = userdata.asString();

	if (level == std::string("can_cut"))
	{
		return mCmbLocation->canCut();
	}
	else if (level == std::string("can_copy"))
	{
		return mCmbLocation->canCopy();
	}
	else if (level == std::string("can_paste"))
	{
		return mCmbLocation->canPaste();
	}
	else if (level == std::string("can_delete"))
	{
		return mCmbLocation->canDeselect();
	}
	else if (level == std::string("can_select_all"))
	{
		return mCmbLocation->canSelectAll();
	}

	return false;
}
