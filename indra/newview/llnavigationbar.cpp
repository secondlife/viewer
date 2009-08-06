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

#include <llfloaterreg.h>
#include <llfocusmgr.h>
#include <lliconctrl.h>
#include <llmenugl.h>
#include <llwindow.h>

#include "llagent.h"
#include "llfloaterhtmlhelp.h"
#include "lllandmarkactions.h"
#include "lllocationhistory.h"
#include "lllocationinputctrl.h"
#include "llteleporthistory.h"
#include "llsearcheditor.h"
#include "llsidetray.h"
#include "llslurl.h"
#include "llurlsimstring.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llworldmap.h"
#include "llappviewer.h"
#include "llviewercontrol.h"

//-- LLTeleportHistoryMenuItem -----------------------------------------------

/**
 * Item look varies depending on the type (backward/current/forward). 
 */
class LLTeleportHistoryMenuItem : public LLMenuItemCallGL
{
public:
	typedef enum e_item_type
	{
		TYPE_BACKWARD,
		TYPE_CURRENT,
		TYPE_FORWARD,
	} EType;

	struct Params : public LLInitParam::Block<Params, LLMenuItemCallGL::Params>
	{
		Mandatory<EType> item_type;

		Params() {}
		Params(EType type, std::string title);
	};

	/*virtual*/ void	draw();
	/*virtual*/ void	onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ void	onMouseLeave(S32 x, S32 y, MASK mask);

private:
	LLTeleportHistoryMenuItem(const Params&);
	friend class LLUICtrlFactory;

	static const S32			ICON_WIDTH			= 16;
	static const S32			ICON_HEIGHT			= 16;
	static const std::string	ICON_IMG_BACKWARD;
	static const std::string	ICON_IMG_FORWARD;

	LLIconCtrl*		mArrowIcon;
};

const std::string LLTeleportHistoryMenuItem::ICON_IMG_BACKWARD("teleport_history_backward.tga");
const std::string LLTeleportHistoryMenuItem::ICON_IMG_FORWARD("teleport_history_forward.tga");

LLTeleportHistoryMenuItem::Params::Params(EType type, std::string title)
{
	item_type(type);
	font.name("SansSerif");

	if (type == TYPE_CURRENT)
		font.style("BOLD");
	else
		title = "   " + title;

	name(title);
	label(title);
}

LLTeleportHistoryMenuItem::LLTeleportHistoryMenuItem(const Params& p)
:	LLMenuItemCallGL(p),
	mArrowIcon(NULL)
{
	LLIconCtrl::Params icon_params;
	icon_params.name("icon");
	icon_params.rect(LLRect(0, ICON_HEIGHT, ICON_WIDTH, 0));
	icon_params.mouse_opaque(false);
	icon_params.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
	icon_params.tab_stop(false);
	icon_params.visible(false);

	mArrowIcon = LLUICtrlFactory::create<LLIconCtrl> (icon_params);

	// no image for the current item
	if (p.item_type == TYPE_BACKWARD)
		mArrowIcon->setValue(ICON_IMG_BACKWARD);
	else if (p.item_type == TYPE_FORWARD)
		mArrowIcon->setValue(ICON_IMG_FORWARD);

	addChild(mArrowIcon);
}

void LLTeleportHistoryMenuItem::draw()
{
	// Draw menu item itself.
	LLMenuItemCallGL::draw();

	// Draw children if any. *TODO: move this to LLMenuItemGL?
	LLUICtrl::draw();
}

void LLTeleportHistoryMenuItem::onMouseEnter(S32 x, S32 y, MASK mask)
{
	mArrowIcon->setVisible(TRUE);
}

void LLTeleportHistoryMenuItem::onMouseLeave(S32 x, S32 y, MASK mask)
{
	mArrowIcon->setVisible(FALSE);
}

//-- LNavigationBar ----------------------------------------------------------

/*
TODO:
- Load navbar height from saved settings (as it's done for status bar) or think of a better way.
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
	mCmbLocation(NULL),
	mLeSearch(NULL),
	mPurgeTPHistoryItems(false)
{
	setIsChrome(TRUE);
	
	// Register callbacks and load the location field context menu (NB: the order matters).
	mCommitCallbackRegistrar.add("Navbar.Action", boost::bind(&LLNavigationBar::onLocationContextMenuItemClicked, this, _2));
	mEnableCallbackRegistrar.add("Navbar.EnableMenuItem", boost::bind(&LLNavigationBar::onLocationContextMenuItemEnabled, this, _2));
	
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_navigation_bar.xml");

	// navigation bar can never get a tab
	setFocusRoot(FALSE);

	// set a listener function for LoginComplete event
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLNavigationBar::handleLoginComplete, this));
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
	
	mCmbLocation= getChild<LLLocationInputCtrl>("location_combo"); 
	mLeSearch	= getChild<LLSearchEditor>("search_input");

	if (!mBtnBack || !mBtnForward || !mBtnHome ||
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

	mCmbLocation->setSelectionCallback(boost::bind(&LLNavigationBar::onLocationSelection, this));
	
	mLeSearch->setCommitCallback(boost::bind(&LLNavigationBar::onSearchCommit, this));

	// Load the location field context menu
	mLocationContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_navbar.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mLocationContextMenu)
	{
		llwarns << "Error loading navigation bar context menu" << llendl;
		return FALSE;
	}

	// we'll be notified on teleport history changes
	LLTeleportHistory::getInstance()->setHistoryChangedCallback(
			boost::bind(&LLNavigationBar::onTeleportHistoryChanged, this));

	return TRUE;
}

void LLNavigationBar::draw()
{
	if(mPurgeTPHistoryItems)
	{
		LLTeleportHistory::getInstance()->purgeItems();
		onTeleportHistoryChanged();
		mPurgeTPHistoryItems = false;
	}
	LLPanel::draw();
}

BOOL LLNavigationBar::handleRightMouseUp(S32 x, S32 y, MASK mask)
{
	// *HACK. We should use mCmbLocation's right click callback instead.

	// If the location field is clicked then show its context menu.
	if (mCmbLocation->getRect().pointInRect(x, y))
	{
		// Pass the focus to the line editor when it is right-clicked
		mCmbLocation->setFocus(TRUE);

		if (mLocationContextMenu)
		{
			mLocationContextMenu->buildDrawLabels();
			mLocationContextMenu->updateParent(LLMenuGL::sMenuContainer);
			LLMenuGL::showPopup(this, mLocationContextMenu, x, y);
		}
		return TRUE;
	}
	return LLPanel:: handleRightMouseUp(x, y, mask);
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

void LLNavigationBar::onSearchCommit()
{
	invokeSearch(mLeSearch->getValue().asString());
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
	std::string typed_location = mCmbLocation->getSimple();

	// Will not teleport to empty location.
	if (typed_location.empty())
		return;

	std::string region_name;
	LLVector3 local_coords(128, 128, 0);

	// Is the typed location a SLURL?
	if (LLSLURL::isSLURL(typed_location))
	{
		// Yes. Extract region name and local coordinates from it.
		S32 x = 0, y = 0, z = 0;
		if (LLURLSimString::parse(LLSLURL::stripProtocol(typed_location), &region_name, &x, &y, &z))
			local_coords.set(x, y, z);
		else
			return;
	}
	else
	{
		// Treat it as region name.
		region_name = typed_location;
	}

	// Resolve the region name to its global coordinates.
	// If resolution succeeds we'll teleport.
	LLWorldMap::url_callback_t cb = boost::bind(
			&LLNavigationBar::onRegionNameResponse, this,
			typed_location, region_name, local_coords, _1, _2, _3, _4);
	LLWorldMap::getInstance()->sendNamedRegionRequest(region_name, cb, std::string("unused"), false);
}

void LLNavigationBar::onTeleportHistoryChanged()
{
	// Update navigation controls.
	LLTeleportHistory* h = LLTeleportHistory::getInstance();
	int cur_item = h->getCurrentItemIndex();
	mBtnBack->setEnabled(cur_item > 0);
	mBtnForward->setEnabled(cur_item < ((int)h->getItems().size() - 1));
}

void LLNavigationBar::rebuildTeleportHistoryMenu()
{
	// Has the pop-up menu been built?
	if (mTeleportHistoryMenu)
	{
		// Clear it.
		mTeleportHistoryMenu->empty();
	}
	else
	{
		// Create it.
		LLMenuGL::Params menu_p;
		menu_p.name("popup");
		menu_p.can_tear_off(false);
		menu_p.visible(false);
		menu_p.bg_visible(true);
		menu_p.scrollable(true);
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
		LLTeleportHistoryMenuItem::EType type;
		if (i < cur_item)
			type = LLTeleportHistoryMenuItem::TYPE_BACKWARD;
		else if (i > cur_item)
			type = LLTeleportHistoryMenuItem::TYPE_FORWARD;
		else
			type = LLTeleportHistoryMenuItem::TYPE_CURRENT;

		LLTeleportHistoryMenuItem::Params item_params(type, hist_items[i].mTitle);
		item_params.on_click.function(boost::bind(&LLNavigationBar::onTeleportHistoryMenuItemClicked, this, i));
		mTeleportHistoryMenu->addChild(LLUICtrlFactory::create<LLTeleportHistoryMenuItem>(item_params));
	}
}

void LLNavigationBar::onRegionNameResponse(
		std::string typed_location,
		std::string region_name,
		LLVector3 local_coords,
		U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	// Invalid location?
	if (!region_handle)
	{
		invokeSearch(typed_location);
		return;
	}

	// Location is valid. Add it to the typed locations history.
	// If user has typed text this variable will contain -1.
	S32 selected_item = mCmbLocation->getCurrentIndex();

	/*
	LLLocationHistory* lh = LLLocationHistory::getInstance();
	lh->addItem(selected_item == -1 ? typed_location : mCmbLocation->getSelectedItemLabel());
	lh->save();
	*/

	// Teleport to the location.
	LLVector3d region_pos = from_region_handle(region_handle);
	LLVector3d global_pos = region_pos + (LLVector3d) local_coords;

	
	llinfos << "Teleporting to: " << global_pos  << llendl;
	gAgent.teleportViaLocation(global_pos);

	LLLocationHistory* lh = LLLocationHistory::getInstance();
	lh->addItem(selected_item == -1 ? typed_location : mCmbLocation->getSelectedItemLabel());
	lh->save();
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
	gFocusMgr.setMouseCapture( NULL );
}

void LLNavigationBar::onLocationContextMenuItemClicked(const LLSD& userdata)
{
	std::string item = userdata.asString();
	LLLineEditor* location_entry = mCmbLocation->getTextEntry();

	if (item == std::string("show_coordinates"))
	{
		gSavedSettings.setBOOL("ShowCoordinatesOption",!gSavedSettings.getBOOL("ShowCoordinatesOption"));
	}
	else if (item == std::string("landmark"))
	{
		LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "create_landmark"));

		// Floater "Add Landmark" functionality moved to Side Tray
		//LLFloaterReg::showInstance("add_landmark");
	}
	else if (item == std::string("cut"))
	{
		location_entry->cut();
	}
	else if (item == std::string("copy"))
	{
		location_entry->copy();
	}
	else if (item == std::string("paste"))
	{
		location_entry->paste();
	}
	else if (item == std::string("delete"))
	{
		location_entry->deleteSelection();
	}
	else if (item == std::string("select_all"))
	{
		location_entry->selectAll();
	}
}

bool LLNavigationBar::onLocationContextMenuItemEnabled(const LLSD& userdata)
{
	std::string item = userdata.asString();
	const LLLineEditor* location_entry = mCmbLocation->getTextEntry();

	if (item == std::string("can_cut"))
	{
		return location_entry->canCut();
	}
	else if (item == std::string("can_copy"))
	{
		return location_entry->canCopy();
	}
	else if (item == std::string("can_paste"))
	{
		return location_entry->canPaste();
	}
	else if (item == std::string("can_delete"))
	{
		return location_entry->canDeselect();
	}
	else if (item == std::string("can_select_all"))
	{
		return location_entry->canSelectAll();
	}
	else if(item == std::string("can_landmark"))
	{
		return !LLLandmarkActions::landmarkAlreadyExists();
	}else if(item == std::string("show_coordinates")){
	
		return gSavedSettings.getBOOL("ShowCoordinatesOption");
	}

	return false;
}

void LLNavigationBar::handleLoginComplete()
{
	mCmbLocation->handleLoginComplete();
}

void LLNavigationBar::invokeSearch(std::string search_text)
{
	LLFloaterReg::showInstance("search", LLSD().insert("panel", "all").insert("id", LLSD(search_text)));
}

void LLNavigationBar::clearHistoryCache()
{
	mCmbLocation->removeall();
	LLLocationHistory* lh = LLLocationHistory::getInstance();
	lh->removeItems();
	lh->save();	
	mPurgeTPHistoryItems= true;
}
