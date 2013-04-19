/** 
 * @file llnavigationbar.cpp
 * @brief Navigation bar implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llnavigationbar.h"

#include "v2math.h"

#include "llregionhandle.h"

#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "lliconctrl.h"
#include "llmenugl.h"

#include "llagent.h"
#include "llviewerregion.h"
#include "lllandmarkactions.h"
#include "lllocationhistory.h"
#include "lllocationinputctrl.h"
#include "llpaneltopinfobar.h"
#include "llteleporthistory.h"
#include "llsearchcombobox.h"
#include "llslurl.h"
#include "llurlregistry.h"
#include "llurldispatcher.h"
#include "llviewerinventory.h"
#include "llviewermenu.h"
#include "llviewerparcelmgr.h"
#include "llworldmapmessage.h"
#include "llappviewer.h"
#include "llviewercontrol.h"
#include "llweb.h"
#include "llhints.h"

#include "llinventorymodel.h"
#include "lllandmarkactions.h"

#include "llfavoritesbar.h"
#include "llagentui.h"

#include <boost/regex.hpp>

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
		Mandatory<EType>		item_type;
		Optional<const LLFontGL*> back_item_font,
								current_item_font,
								forward_item_font;
		Optional<std::string>	back_item_image,
								forward_item_image;
		Optional<S32>			image_hpad,
								image_vpad;
		Params()
		:	item_type(),
			back_item_font("back_item_font"),
			current_item_font("current_item_font"),
			forward_item_font("forward_item_font"),
			back_item_image("back_item_image"),
			forward_item_image("forward_item_image"),
			image_hpad("image_hpad"),
			image_vpad("image_vpad")
		{}
	};

	/*virtual*/ void	draw();
	/*virtual*/ void	onMouseEnter(S32 x, S32 y, MASK mask);
	/*virtual*/ void	onMouseLeave(S32 x, S32 y, MASK mask);

private:
	LLTeleportHistoryMenuItem(const Params&);
	friend class LLUICtrlFactory;

	static const S32			ICON_WIDTH			= 16;
	static const S32			ICON_HEIGHT			= 16;

	LLIconCtrl*		mArrowIcon;
};

static LLDefaultChildRegistry::Register<LLTeleportHistoryMenuItem> r("teleport_history_menu_item");


LLTeleportHistoryMenuItem::LLTeleportHistoryMenuItem(const Params& p)
:	LLMenuItemCallGL(p),
	mArrowIcon(NULL)
{
	// Set appearance depending on the item type.
	if (p.item_type == TYPE_BACKWARD)
	{
		setFont( p.back_item_font );
	}
	else if (p.item_type == TYPE_CURRENT)
	{
		setFont( p.current_item_font );
	}
	else
	{
		setFont( p.forward_item_font );
	}

	LLIconCtrl::Params icon_params;
	icon_params.name("icon");
	LLRect rect(0, ICON_HEIGHT, ICON_WIDTH, 0);
	rect.translate( p.image_hpad, p.image_vpad );
	icon_params.rect( rect );
	icon_params.mouse_opaque(false);
	icon_params.follows.flags(FOLLOWS_LEFT | FOLLOWS_TOP);
	icon_params.visible(false);

	mArrowIcon = LLUICtrlFactory::create<LLIconCtrl> (icon_params);

	// no image for the current item
	if (p.item_type == TYPE_BACKWARD)
		mArrowIcon->setValue( p.back_item_image() );
	else if (p.item_type == TYPE_FORWARD)
		mArrowIcon->setValue( p.forward_item_image() );

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

static LLDefaultChildRegistry::Register<LLPullButton> menu_button("pull_button");

LLPullButton::LLPullButton(const LLPullButton::Params& params) :
	LLButton(params)
{
	setDirectionFromName(params.direction);
}
boost::signals2::connection LLPullButton::setClickDraggingCallback(const commit_signal_t::slot_type& cb)
{
	return mClickDraggingSignal.connect(cb);
}

/*virtual*/
void LLPullButton::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLButton::onMouseLeave(x, y, mask);

	if (mMouseDownTimer.getStarted()) //an user have done a mouse down, if the timer started. see LLButton::handleMouseDown for details
	{
		const LLVector2 cursor_direction = LLVector2(F32(x), F32(y)) - mLastMouseDown;
		/* For now cursor_direction points to the direction of mouse movement
		 * Need to decide whether should we fire a signal. 
		 * We fire if angle between mDraggingDirection and cursor_direction is less that 45 degree
		 * Note:
		 * 0.5 * F_PI_BY_TWO equals to PI/4 radian that equals to angle of 45 degrees
		 */
		if (angle_between(mDraggingDirection, cursor_direction) < 0.5 * F_PI_BY_TWO)//call if angle < pi/4 
		{
			mClickDraggingSignal(this, LLSD());
		}
	}

}

/*virtual*/
BOOL LLPullButton::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLButton::handleMouseDown(x, y, mask);
	if (handled)
	{
		//if mouse down was handled by button, 
		//capture mouse position to calculate the direction of  mouse move  after mouseLeave event 
		mLastMouseDown.set(F32(x), F32(y));
	}
	return handled;
}

/*virtual*/
BOOL LLPullButton::handleMouseUp(S32 x, S32 y, MASK mask)
{
	// reset data to get ready for next circle 
	mLastMouseDown.clear();
	return LLButton::handleMouseUp(x, y, mask);
}
/**
 * this function is setting up dragging direction vector. 
 * Last one is just unit vector. It points to direction of mouse drag that we need to handle   
 */
void LLPullButton::setDirectionFromName(const std::string& name)
{
	if (name == "left")
	{
		mDraggingDirection.set(F32(-1), F32(0));
	}
	else if (name == "right")
	{
		mDraggingDirection.set(F32(0), F32(1));
	}
	else if (name == "down")
	{
		mDraggingDirection.set(F32(0), F32(-1));
	}
	else if (name == "up")
	{
		mDraggingDirection.set(F32(0), F32(1));
	}
}

//-- LNavigationBar ----------------------------------------------------------

/*
TODO:
- Load navbar height from saved settings (as it's done for status bar) or think of a better way.
*/

LLNavigationBar::LLNavigationBar()
:	mTeleportHistoryMenu(NULL),
	mBtnBack(NULL),
	mBtnForward(NULL),
	mBtnHome(NULL),
	mCmbLocation(NULL),
	mSaveToLocationHistory(false)
{
	buildFromFile( "panel_navigation_bar.xml");

	// set a listener function for LoginComplete event
	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&LLNavigationBar::handleLoginComplete, this));
}

LLNavigationBar::~LLNavigationBar()
{
	mTeleportFinishConnection.disconnect();
	mTeleportFailedConnection.disconnect();
}

BOOL LLNavigationBar::postBuild()
{
	mBtnBack	= getChild<LLPullButton>("back_btn");
	mBtnForward	= getChild<LLPullButton>("forward_btn");
	mBtnHome	= getChild<LLButton>("home_btn");
	
	mCmbLocation= getChild<LLLocationInputCtrl>("location_combo");

	mBtnBack->setEnabled(FALSE);
	mBtnBack->setClickedCallback(boost::bind(&LLNavigationBar::onBackButtonClicked, this));
	mBtnBack->setHeldDownCallback(boost::bind(&LLNavigationBar::onBackOrForwardButtonHeldDown, this,_1, _2));
	mBtnBack->setClickDraggingCallback(boost::bind(&LLNavigationBar::showTeleportHistoryMenu, this,_1));
	
	mBtnForward->setEnabled(FALSE);
	mBtnForward->setClickedCallback(boost::bind(&LLNavigationBar::onForwardButtonClicked, this));
	mBtnForward->setHeldDownCallback(boost::bind(&LLNavigationBar::onBackOrForwardButtonHeldDown, this, _1, _2));
	mBtnForward->setClickDraggingCallback(boost::bind(&LLNavigationBar::showTeleportHistoryMenu, this,_1));

	mBtnHome->setClickedCallback(boost::bind(&LLNavigationBar::onHomeButtonClicked, this));

	mCmbLocation->setCommitCallback(boost::bind(&LLNavigationBar::onLocationSelection, this));

	mTeleportFinishConnection = LLViewerParcelMgr::getInstance()->
		setTeleportFinishedCallback(boost::bind(&LLNavigationBar::onTeleportFinished, this, _1));

	mTeleportFailedConnection = LLViewerParcelMgr::getInstance()->
		setTeleportFailedCallback(boost::bind(&LLNavigationBar::onTeleportFailed, this));
	
	mDefaultNbRect = getRect();
	mDefaultFpRect = getChild<LLFavoritesBarCtrl>("favorite")->getRect();

	// we'll be notified on teleport history changes
	LLTeleportHistory::getInstance()->setHistoryChangedCallback(
			boost::bind(&LLNavigationBar::onTeleportHistoryChanged, this));

	LLHints::registerHintTarget("nav_bar", getHandle());

	return TRUE;
}

void LLNavigationBar::setVisible(BOOL visible)
{
	// change visibility of grandparent layout_panel to animate in and out
	if (getParent()) 
	{
		//to avoid some mysterious bugs like EXT-3352, at least try to log an incorrect parent to ping  about a problem. 
		if(getParent()->getName() != "nav_bar_container")
		{
			LL_WARNS("LLNavigationBar")<<"NavigationBar has an unknown name of the parent: "<<getParent()->getName()<< LL_ENDL;
		}
		getParent()->setVisible(visible);	
	}
}

void LLNavigationBar::draw()
{
	if (isBackgroundVisible())
	{
		static LLUICachedControl<S32> drop_shadow_floater ("DropShadowFloater", 0);
		static LLUIColor color_drop_shadow = LLUIColorTable::instance().getColor("ColorDropShadow");
		gl_drop_shadow(0, getRect().getHeight(), getRect().getWidth(), 0,
                           color_drop_shadow, drop_shadow_floater );
	}

	LLPanel::draw();
}

BOOL LLNavigationBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handled = childrenHandleRightMouseDown( x, y, mask) != NULL;
	if(!handled && !gMenuHolder->hasVisibleMenu())
	{
		show_navbar_context_menu(this,x,y);
		handled = true;
	}
					
	return handled;
}

void LLNavigationBar::onBackButtonClicked()
{
	LLTeleportHistory::getInstance()->goBack();
}

void LLNavigationBar::onBackOrForwardButtonHeldDown(LLUICtrl* ctrl, const LLSD& param)
{
	if (param["count"].asInteger() == 0)
		showTeleportHistoryMenu(ctrl);
}

void LLNavigationBar::onForwardButtonClicked()
{
	LLTeleportHistory::getInstance()->goForward();
}

void LLNavigationBar::onHomeButtonClicked()
{
	gAgent.teleportHome();
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
	LLStringUtil::trim(typed_location);

	// Will not teleport to empty location.
	if (typed_location.empty())
		return;
	//get selected item from combobox item
	LLSD value = mCmbLocation->getSelectedValue();
	if(value.isUndefined() && !mCmbLocation->getTextEntry()->isDirty())
	{
		// At this point we know that: there is no selected item in list and text field has NOT been changed
		// So there is no sense to try to change the location
		return;
	}
	/* since navbar list support autocompletion it contains several types of item: landmark, teleport hystory item,
	 * typed by user slurl or region name. Let's find out which type of item the user has selected 
	 * to make decision about adding this location into typed history. see mSaveToLocationHistory
	 * Note:
	 * Only TYPED_REGION_SLURL item will be added into LLLocationHistory 
	 */  
	
	if(value.has("item_type"))
	{

		switch(value["item_type"].asInteger())
		{
		case LANDMARK:
			
			if(value.has("AssetUUID"))
			{
				
				gAgent.teleportViaLandmark( LLUUID(value["AssetUUID"].asString()));
				return;
			}
			else
			{
				LLInventoryModel::item_array_t landmark_items =
						LLLandmarkActions::fetchLandmarksByName(typed_location,
								FALSE);
				if (!landmark_items.empty())
				{
					gAgent.teleportViaLandmark( landmark_items[0]->getAssetUUID());
					return; 
				}
			}
			break;
			
		case TELEPORT_HISTORY:
			//in case of teleport item was selected, teleport by position too.
		case TYPED_REGION_SLURL:
			if(value.has("global_pos"))
			{
				gAgent.teleportViaLocation(LLVector3d(value["global_pos"]));
				return;
			}
			break;
			
		default:
			break;		
		}
	}
	//Let's parse slurl or region name
	
	std::string region_name;
	LLVector3 local_coords(128, 128, 0);
	// Is the typed location a SLURL?
	LLSLURL slurl = LLSLURL(typed_location);
	if (slurl.getType() == LLSLURL::LOCATION)
	{
	  region_name = slurl.getRegion();
	  local_coords = slurl.getPosition();
	}
	else if(!slurl.isValid())
	{
	  // we have to do this check after previous, because LLUrlRegistry contains handlers for slurl too  
	  // but we need to know whether typed_location is a simple http url.
	  if (LLUrlRegistry::instance().isUrl(typed_location)) 
	    {
		// display http:// URLs in the media browser, or
		// anything else is sent to the search floater
		LLWeb::loadURL(typed_location);
		return;
	  }
	  else
	  {
	      // assume that an user has typed the {region name} or possible {region_name, parcel}
	      region_name  = typed_location.substr(0,typed_location.find(','));
	    }
	}
	else
	{
	  // was an app slurl, home, whatever.  Bail
	  return;
	}
	
	// Resolve the region name to its global coordinates.
	// If resolution succeeds we'll teleport.
	LLWorldMapMessage::url_callback_t cb = boost::bind(
			&LLNavigationBar::onRegionNameResponse, this,
			typed_location, region_name, local_coords, _1, _2, _3, _4);
	mSaveToLocationHistory = true;
	LLWorldMapMessage::getInstance()->sendNamedRegionRequest(region_name, cb, std::string("unused"), false);
}

void LLNavigationBar::onTeleportFailed()
{
	mSaveToLocationHistory = false;
}

void LLNavigationBar::onTeleportFinished(const LLVector3d& global_agent_pos)
{
	if (!mSaveToLocationHistory)
		return;
	LLLocationHistory* lh = LLLocationHistory::getInstance();

	//TODO*: do we need convert slurl into readable format?
	std::string location;
	/*NOTE:
	 * We can't use gAgent.getPositionAgent() in case of local teleport to build location.
	 * At this moment gAgent.getPositionAgent() contains previous coordinates.
	 * according to EXT-65 agent position is being reseted on each frame.  
	 */
		LLAgentUI::buildLocationString(location, LLAgentUI::LOCATION_FORMAT_NO_MATURITY,
					gAgent.getPosAgentFromGlobal(global_agent_pos));
	std::string tooltip (LLSLURL(gAgent.getRegion()->getName(), global_agent_pos).getSLURLString());
	
	LLLocationHistoryItem item (location,
			global_agent_pos, tooltip,TYPED_REGION_SLURL);// we can add into history only TYPED location
	//Touch it, if it is at list already, add new location otherwise
	if ( !lh->touchItem(item) ) {
		lh->addItem(item);
	}

	lh->save();
	
	mSaveToLocationHistory = false;
	
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

		LLTeleportHistoryMenuItem::Params item_params;
		item_params.label = item_params.name = hist_items[i].mTitle;
		item_params.item_type = type;
		item_params.on_click.function(boost::bind(&LLNavigationBar::onTeleportHistoryMenuItemClicked, this, i));
		LLTeleportHistoryMenuItem* new_itemp = LLUICtrlFactory::create<LLTeleportHistoryMenuItem>(item_params);
		//new_itemp->setFont()
		mTeleportHistoryMenu->addChild(new_itemp);
	}
}

void LLNavigationBar::onRegionNameResponse(
		std::string typed_location,
		std::string region_name,
		LLVector3 local_coords,
		U64 region_handle, const std::string& url, const LLUUID& snapshot_id, bool teleport)
{
	// Invalid location?
	if (region_handle)
	{
		// Teleport to the location.
		LLVector3d region_pos = from_region_handle(region_handle);
		LLVector3d global_pos = region_pos + (LLVector3d) local_coords;

		llinfos << "Teleporting to: " << LLSLURL(region_name,	global_pos).getSLURLString()  << llendl;
		gAgent.teleportViaLocation(global_pos);
	}
	else if (gSavedSettings.getBOOL("SearchFromAddressBar"))
	{
		invokeSearch(typed_location);
	}
}

void	LLNavigationBar::showTeleportHistoryMenu(LLUICtrl* btn_ctrl)
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
	
	mTeleportHistoryMenu->updateParent(LLMenuGL::sMenuContainer);
	const S32 MENU_SPAWN_PAD = -1;
	LLMenuGL::showPopup(btn_ctrl, mTeleportHistoryMenu, 0, MENU_SPAWN_PAD);
	LLButton* nav_button = dynamic_cast<LLButton*>(btn_ctrl);
	if(nav_button)
	{
		if(mHistoryMenuConnection.connected())
		{
			LL_WARNS("Navgationbar")<<"mHistoryMenuConnection should be disconnected at this moment."<<LL_ENDL;
			mHistoryMenuConnection.disconnect();
		}
		mHistoryMenuConnection = gMenuHolder->setMouseUpCallback(boost::bind(&LLNavigationBar::onNavigationButtonHeldUp, this, nav_button));
		// pressed state will be update after mouseUp in  onBackOrForwardButtonHeldUp();
		nav_button->setForcePressedState(true);
	}
	// *HACK pass the mouse capturing to the drop-down menu
	// it need to let menu handle mouseup event
	gFocusMgr.setMouseCapture(gMenuHolder);
}
/**
 * Taking into account the HACK above,  this callback-function is responsible for correct handling of mouseUp event in case of holding-down the navigation buttons..
 * We need to process this case separately to update a pressed state of navigation button.
 */
void LLNavigationBar::onNavigationButtonHeldUp(LLButton* nav_button)
{
	if(nav_button)
	{
		nav_button->setForcePressedState(false);
	}
	if(gFocusMgr.getMouseCapture() == gMenuHolder)
	{
		// we had passed mouseCapture in  showTeleportHistoryMenu()
		// now we MUST release mouseCapture to continue a proper mouseevent workflow. 
		gFocusMgr.setMouseCapture(NULL);
	}
	//gMenuHolder is using to display bunch of menus. Disconnect signal to avoid unnecessary calls.    
	mHistoryMenuConnection.disconnect();
}

void LLNavigationBar::handleLoginComplete()
{
	LLTeleportHistory::getInstance()->handleLoginComplete();
	LLPanelTopInfoBar::instance().handleLoginComplete();
	mCmbLocation->handleLoginComplete();
}

void LLNavigationBar::invokeSearch(std::string search_text)
{
	LLFloaterReg::showInstance("search", LLSD().with("category", "all").with("query", LLSD(search_text)));
}

void LLNavigationBar::clearHistoryCache()
{
	mCmbLocation->removeall();
	LLLocationHistory* lh = LLLocationHistory::getInstance();
	lh->removeItems();
	lh->save();	
	LLTeleportHistory::getInstance()->purgeItems();
}

int LLNavigationBar::getDefNavBarHeight()
{
	return mDefaultNbRect.getHeight();
}
int LLNavigationBar::getDefFavBarHeight()
{
	return mDefaultFpRect.getHeight();
}
