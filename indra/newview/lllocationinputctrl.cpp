/**
 * @file lllocationinputctrl.cpp
 * @brief Combobox-like location input control
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

// file includes
#include "lllocationinputctrl.h"

// common includes
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llhelp.h"
#include "llmenugl.h"
#include "llparcel.h"
#include "llstring.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "lltooltip.h"
#include "llnotificationsutil.h"
#include "llregionflags.h"

// newview includes
#include "llagent.h"
#include "llenvmanager.h"
#include "llfloatersidepanelcontainer.h"
#include "llinventoryobserver.h"
#include "lllandmarkactions.h"
#include "lllandmarklist.h"
#include "llpathfindingmanager.h"
#include "llpathfindingnavmesh.h"
#include "llpathfindingnavmeshstatus.h"
#include "llteleporthistory.h"
#include "llslurl.h"
#include "llstatusbar.h"			// getHealth()
#include "lltrans.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "llviewerregion.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llurllineeditorctrl.h"
#include "llagentui.h"

//============================================================================
/*
 * "ADD LANDMARK" BUTTON UPDATING LOGIC
 * 
 * If the current parcel has been landmarked, we should draw
 * a special image on the button.
 * 
 * To avoid determining the appropriate image on every draw() we do that
 * only in the following cases:
 * 1) Navbar is shown for the first time after login.
 * 2) Agent moves to another parcel.
 * 3) A landmark is created or removed.
 * 
 * The first case is handled by the handleLoginComplete() method.
 * 
 * The second case is handled by setting the "agent parcel changed" callback
 * on LLViewerParcelMgr.
 * 
 * The third case is the most complex one. We have two inventory observers for that:
 * one is designated to handle adding landmarks, the other handles removal.
 * Let's see how the former works.
 * 
 * When we get notified about landmark addition, the landmark position is unknown yet. What we can
 * do at that point is initiate loading the landmark data by LLLandmarkList and set the
 * "loading finished" callback on it. Finally, when the callback is triggered,
 * we can determine whether the landmark refers to a point within the current parcel
 * and choose the appropriate image for the "Add landmark" button.
 */

/**
 * Initiates loading the landmarks that have been just added.
 *
 * Once the loading is complete we'll be notified
 * with the callback we set for LLLandmarkList.
 */
class LLAddLandmarkObserver : public LLInventoryAddedObserver
{
public:
	LLAddLandmarkObserver(LLLocationInputCtrl* input) : mInput(input) {}

private:
	/*virtual*/ void done()
	{
		uuid_vec_t::const_iterator it = mAdded.begin(), end = mAdded.end();
		for(; it != end; ++it)
		{
			LLInventoryItem* item = gInventory.getItem(*it);
			if (!item || item->getType() != LLAssetType::AT_LANDMARK)
				continue;

			// Start loading the landmark.
			LLLandmark* lm = gLandmarkList.getAsset(
					item->getAssetUUID(),
					boost::bind(&LLLocationInputCtrl::onLandmarkLoaded, mInput, _1));
			if (lm)
			{
				// Already loaded? Great, handle it immediately (the callback won't be called).
				mInput->onLandmarkLoaded(lm);
			}
		}

		mAdded.clear();
	}

	LLLocationInputCtrl* mInput;
};

/**
 * Updates the "Add landmark" button once a landmark gets removed.
 */
class LLRemoveLandmarkObserver : public LLInventoryObserver
{
public:
	LLRemoveLandmarkObserver(LLLocationInputCtrl* input) : mInput(input) {}

private:
	/*virtual*/ void changed(U32 mask)
	{
		if (mask & (~(LLInventoryObserver::LABEL|LLInventoryObserver::INTERNAL|LLInventoryObserver::ADD)))
		{
			mInput->updateAddLandmarkButton();
		}
	}

	LLLocationInputCtrl* mInput;
};

class LLParcelChangeObserver : public LLParcelObserver
{
public:
	LLParcelChangeObserver(LLLocationInputCtrl* input) : mInput(input) {}

private:
	/*virtual*/ void changed()
	{
		if (mInput)
		{
			mInput->refreshParcelIcons();
		}
	}

	LLLocationInputCtrl* mInput;
};

//============================================================================


static LLDefaultChildRegistry::Register<LLLocationInputCtrl> r("location_input");

LLLocationInputCtrl::Params::Params()
:	icon_maturity_general("icon_maturity_general"),
	icon_maturity_adult("icon_maturity_adult"),
	icon_maturity_moderate("icon_maturity_moderate"),
	add_landmark_image_enabled("add_landmark_image_enabled"),
	add_landmark_image_disabled("add_landmark_image_disabled"),
	add_landmark_image_hover("add_landmark_image_hover"),
	add_landmark_image_selected("add_landmark_image_selected"),
	add_landmark_hpad("add_landmark_hpad", 0),
	icon_hpad("icon_hpad", 0),
	add_landmark_button("add_landmark_button"),
	for_sale_button("for_sale_button"),
	info_button("info_button"),
	maturity_button("maturity_button"),
	voice_icon("voice_icon"),
	fly_icon("fly_icon"),
	push_icon("push_icon"),
	build_icon("build_icon"),
	scripts_icon("scripts_icon"),
	damage_icon("damage_icon"),
	damage_text("damage_text"),
	see_avatars_icon("see_avatars_icon"),
	maturity_help_topic("maturity_help_topic"),
	pathfinding_dirty_icon("pathfinding_dirty_icon"),
	pathfinding_disabled_icon("pathfinding_disabled_icon")
{
}

LLLocationInputCtrl::LLLocationInputCtrl(const LLLocationInputCtrl::Params& p)
:	LLComboBox(p),
	mIconHPad(p.icon_hpad),
	mAddLandmarkHPad(p.add_landmark_hpad),
	mLocationContextMenu(NULL),
	mAddLandmarkBtn(NULL),
	mForSaleBtn(NULL),
	mInfoBtn(NULL),
	mRegionCrossingSlot(),
	mNavMeshSlot(),
	mIsNavMeshDirty(false),
	mLandmarkImageOn(NULL),
	mLandmarkImageOff(NULL),
	mIconMaturityGeneral(NULL),
	mIconMaturityAdult(NULL),
	mIconMaturityModerate(NULL),
	mMaturityHelpTopic(p.maturity_help_topic)
{
	// Lets replace default LLLineEditor with LLLocationLineEditor
	// to make needed escaping while copying and cutting url
	delete mTextEntry;

	// Can't access old mTextEntry fields as they are protected, so lets build new params
	// That is C&P from LLComboBox::createLineEditor function
	static LLUICachedControl<S32> drop_shadow_button ("DropShadowButton", 0);
	S32 arrow_width = mArrowImage ? mArrowImage->getWidth() : 0;
	LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	text_entry_rect.mRight -= llmax(8,arrow_width) + 2 * drop_shadow_button;

	LLLineEditor::Params params = p.combo_editor;
	params.rect(text_entry_rect);
	params.default_text(LLStringUtil::null);
	params.max_length.bytes(p.max_chars);
	params.keystroke_callback(boost::bind(&LLLocationInputCtrl::onTextEntry, this, _1));
	params.commit_on_focus_lost(false);
	params.follows.flags(FOLLOWS_ALL);
	mTextEntry = LLUICtrlFactory::create<LLURLLineEditor>(params);
	mTextEntry->setContextMenu(NULL);
	addChild(mTextEntry);
	// LLLineEditor is replaced with LLLocationLineEditor

	// "Place information" button.
	LLButton::Params info_params = p.info_button;
	mInfoBtn = LLUICtrlFactory::create<LLButton>(info_params);
	mInfoBtn->setClickedCallback(boost::bind(&LLLocationInputCtrl::onInfoButtonClicked, this));
	addChild(mInfoBtn);

	// "Add landmark" button.
	LLButton::Params al_params = p.add_landmark_button;

	// Image for unselected state will be set in updateAddLandmarkButton(),
	// it will be either mLandmarkOn or mLandmarkOff
	if (p.add_landmark_image_enabled())
	{
		mLandmarkImageOn = p.add_landmark_image_enabled;
	}
	if (p.add_landmark_image_disabled())
	{
		mLandmarkImageOff = p.add_landmark_image_disabled;
	}

	if(p.add_landmark_image_selected)
	{
		al_params.image_selected = p.add_landmark_image_selected;
	}
	if (p.add_landmark_image_hover())
	{
		al_params.image_hover_unselected = p.add_landmark_image_hover;
	}

	al_params.click_callback.function(boost::bind(&LLLocationInputCtrl::onAddLandmarkButtonClicked, this));
	mAddLandmarkBtn = LLUICtrlFactory::create<LLButton>(al_params);
	enableAddLandmarkButton(true);
	addChild(mAddLandmarkBtn);

	if (p.icon_maturity_general())
	{
		mIconMaturityGeneral = p.icon_maturity_general;
	}		
	if (p.icon_maturity_adult())
	{
		mIconMaturityAdult = p.icon_maturity_adult;
	}
	if(p.icon_maturity_moderate())
	{
		mIconMaturityModerate = p.icon_maturity_moderate;
	}
	
	LLButton::Params maturity_button = p.maturity_button;
	mMaturityButton = LLUICtrlFactory::create<LLButton>(maturity_button);
	addChild(mMaturityButton);
	mMaturityButton->setClickedCallback(boost::bind(&LLLocationInputCtrl::onMaturityButtonClicked, this));

	LLButton::Params for_sale_button = p.for_sale_button;
	for_sale_button.tool_tip = LLTrans::getString("LocationCtrlForSaleTooltip");
	for_sale_button.click_callback.function(
		boost::bind(&LLLocationInputCtrl::onForSaleButtonClicked, this));
	mForSaleBtn = LLUICtrlFactory::create<LLButton>( for_sale_button );
	addChild(mForSaleBtn);

	// Parcel property icons
	// Must be mouse-opaque so cursor stays as an arrow when hovering to
	// see tooltip.
	LLIconCtrl::Params voice_icon = p.voice_icon;
	voice_icon.tool_tip = LLTrans::getString("LocationCtrlVoiceTooltip");
	voice_icon.mouse_opaque = true;
	mParcelIcon[VOICE_ICON] = LLUICtrlFactory::create<LLIconCtrl>(voice_icon);
	mParcelIcon[VOICE_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, VOICE_ICON));
	addChild(mParcelIcon[VOICE_ICON]);

	LLIconCtrl::Params fly_icon = p.fly_icon;
	fly_icon.tool_tip = LLTrans::getString("LocationCtrlFlyTooltip");
	fly_icon.mouse_opaque = true;
	mParcelIcon[FLY_ICON] = LLUICtrlFactory::create<LLIconCtrl>(fly_icon);
	mParcelIcon[FLY_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, FLY_ICON));
	addChild(mParcelIcon[FLY_ICON]);

	LLIconCtrl::Params push_icon = p.push_icon;
	push_icon.tool_tip = LLTrans::getString("LocationCtrlPushTooltip");
	push_icon.mouse_opaque = true;
	mParcelIcon[PUSH_ICON] = LLUICtrlFactory::create<LLIconCtrl>(push_icon);
	mParcelIcon[PUSH_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, PUSH_ICON));
	addChild(mParcelIcon[PUSH_ICON]);

	LLIconCtrl::Params build_icon = p.build_icon;
	build_icon.tool_tip = LLTrans::getString("LocationCtrlBuildTooltip");
	build_icon.mouse_opaque = true;
	mParcelIcon[BUILD_ICON] = LLUICtrlFactory::create<LLIconCtrl>(build_icon);
	mParcelIcon[BUILD_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, BUILD_ICON));
	addChild(mParcelIcon[BUILD_ICON]);

	LLIconCtrl::Params scripts_icon = p.scripts_icon;
	scripts_icon.tool_tip = LLTrans::getString("LocationCtrlScriptsTooltip");
	scripts_icon.mouse_opaque = true;
	mParcelIcon[SCRIPTS_ICON] = LLUICtrlFactory::create<LLIconCtrl>(scripts_icon);
	mParcelIcon[SCRIPTS_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, SCRIPTS_ICON));
	addChild(mParcelIcon[SCRIPTS_ICON]);

	LLIconCtrl::Params damage_icon = p.damage_icon;
	damage_icon.tool_tip = LLTrans::getString("LocationCtrlDamageTooltip");
	damage_icon.mouse_opaque = true;
	mParcelIcon[DAMAGE_ICON] = LLUICtrlFactory::create<LLIconCtrl>(damage_icon);
	mParcelIcon[DAMAGE_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, DAMAGE_ICON));
	addChild(mParcelIcon[DAMAGE_ICON]);

	LLIconCtrl::Params pathfinding_dirty_icon = p.pathfinding_dirty_icon;
	pathfinding_dirty_icon.tool_tip = LLTrans::getString("LocationCtrlPathfindingDirtyTooltip");
	pathfinding_dirty_icon.mouse_opaque = true;
	mParcelIcon[PATHFINDING_DIRTY_ICON] = LLUICtrlFactory::create<LLIconCtrl>(pathfinding_dirty_icon);
	mParcelIcon[PATHFINDING_DIRTY_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, PATHFINDING_DIRTY_ICON));
	addChild(mParcelIcon[PATHFINDING_DIRTY_ICON]);

	LLIconCtrl::Params pathfinding_disabled_icon = p.pathfinding_disabled_icon;
	pathfinding_disabled_icon.tool_tip = LLTrans::getString("LocationCtrlPathfindingDisabledTooltip");
	pathfinding_disabled_icon.mouse_opaque = true;
	mParcelIcon[PATHFINDING_DISABLED_ICON] = LLUICtrlFactory::create<LLIconCtrl>(pathfinding_disabled_icon);
	mParcelIcon[PATHFINDING_DISABLED_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, PATHFINDING_DISABLED_ICON));
	addChild(mParcelIcon[PATHFINDING_DISABLED_ICON]);

	LLTextBox::Params damage_text = p.damage_text;
	damage_text.tool_tip = LLTrans::getString("LocationCtrlDamageTooltip");
	damage_text.mouse_opaque = true;
	mDamageText = LLUICtrlFactory::create<LLTextBox>(damage_text);
	addChild(mDamageText);
	
	LLIconCtrl::Params see_avatars_icon = p.see_avatars_icon;
	see_avatars_icon.tool_tip = LLTrans::getString("LocationCtrlSeeAVsTooltip");
	see_avatars_icon.mouse_opaque = true;
	mParcelIcon[SEE_AVATARS_ICON] = LLUICtrlFactory::create<LLIconCtrl>(see_avatars_icon);
	mParcelIcon[SEE_AVATARS_ICON]->setMouseDownCallback(boost::bind(&LLLocationInputCtrl::onParcelIconClick, this, SEE_AVATARS_ICON));
	addChild(mParcelIcon[SEE_AVATARS_ICON]);
	
	// Register callbacks and load the location field context menu (NB: the order matters).
	LLUICtrl::CommitCallbackRegistry::currentRegistrar().add("Navbar.Action", boost::bind(&LLLocationInputCtrl::onLocationContextMenuItemClicked, this, _2));
	LLUICtrl::EnableCallbackRegistry::currentRegistrar().add("Navbar.EnableMenuItem", boost::bind(&LLLocationInputCtrl::onLocationContextMenuItemEnabled, this, _2));
		
	setPrearrangeCallback(boost::bind(&LLLocationInputCtrl::onLocationPrearrange, this, _2));
	getTextEntry()->setMouseUpCallback(boost::bind(&LLLocationInputCtrl::changeLocationPresentation, this));

	// Load the location field context menu
	mLocationContextMenu = LLUICtrlFactory::getInstance()->createFromFile<LLMenuGL>("menu_navbar.xml", gMenuHolder, LLViewerMenuHolderGL::child_registry_t::instance());
	if (!mLocationContextMenu)
	{
		llwarns << "Error loading navigation bar context menu" << llendl;
		
	}
	getTextEntry()->setRightMouseUpCallback(boost::bind(&LLLocationInputCtrl::onTextEditorRightClicked,this,_2,_3,_4));
	updateWidgetlayout();

	// Connecting signal for updating location on "Show Coordinates" setting change.
	LLControlVariable* coordinates_control = gSavedSettings.getControl("NavBarShowCoordinates").get();
	if (coordinates_control)
	{
		mCoordinatesControlConnection = coordinates_control->getSignal()->connect(boost::bind(&LLLocationInputCtrl::refreshLocation, this));
	}

	// Connecting signal for updating parcel icons on "Show Parcel Properties" setting change.
	LLControlVariable* parcel_properties_control = gSavedSettings.getControl("NavBarShowParcelProperties").get();
	if (parcel_properties_control)
	{
		mParcelPropertiesControlConnection = parcel_properties_control->getSignal()->connect(boost::bind(&LLLocationInputCtrl::refreshParcelIcons, this));
	}

	// - Make the "Add landmark" button updated when either current parcel gets changed
	//   or a landmark gets created or removed from the inventory.
	// - Update the location string on parcel change.
	mParcelMgrConnection = LLViewerParcelMgr::getInstance()->addAgentParcelChangedCallback(
		boost::bind(&LLLocationInputCtrl::onAgentParcelChange, this));
	// LLLocationHistory instance is being created before the location input control, so we have to update initial state of button manually.
	mButton->setEnabled(LLLocationHistory::instance().getItemCount() > 0);
	mLocationHistoryConnection = LLLocationHistory::getInstance()->setChangedCallback(
			boost::bind(&LLLocationInputCtrl::onLocationHistoryChanged, this,_1));

	mRegionCrossingSlot = LLEnvManagerNew::getInstance()->setRegionChangeCallback(boost::bind(&LLLocationInputCtrl::onRegionBoundaryCrossed, this));
	createNavMeshStatusListenerForCurrentRegion();

	mRemoveLandmarkObserver	= new LLRemoveLandmarkObserver(this);
	mAddLandmarkObserver	= new LLAddLandmarkObserver(this);
	gInventory.addObserver(mRemoveLandmarkObserver);
	gInventory.addObserver(mAddLandmarkObserver);

	mParcelChangeObserver = new LLParcelChangeObserver(this);
	LLViewerParcelMgr::getInstance()->addObserver(mParcelChangeObserver);

	mAddLandmarkTooltip = LLTrans::getString("LocationCtrlAddLandmarkTooltip");
	mEditLandmarkTooltip = LLTrans::getString("LocationCtrlEditLandmarkTooltip");
	mButton->setToolTip(LLTrans::getString("LocationCtrlComboBtnTooltip"));
	mInfoBtn->setToolTip(LLTrans::getString("LocationCtrlInfoBtnTooltip"));
}

LLLocationInputCtrl::~LLLocationInputCtrl()
{
	gInventory.removeObserver(mRemoveLandmarkObserver);
	gInventory.removeObserver(mAddLandmarkObserver);
	delete mRemoveLandmarkObserver;
	delete mAddLandmarkObserver;

	LLViewerParcelMgr::getInstance()->removeObserver(mParcelChangeObserver);
	delete mParcelChangeObserver;

	mRegionCrossingSlot.disconnect();
	mNavMeshSlot.disconnect();
	mCoordinatesControlConnection.disconnect();
	mParcelPropertiesControlConnection.disconnect();
	mParcelMgrConnection.disconnect();
	mLocationHistoryConnection.disconnect();
}

void LLLocationInputCtrl::setEnabled(BOOL enabled)
{
	LLComboBox::setEnabled(enabled);
	mAddLandmarkBtn->setEnabled(enabled);
}

void LLLocationInputCtrl::hideList()
{
	LLComboBox::hideList();
	if (mTextEntry && hasFocus())
		focusTextEntry();
}

BOOL LLLocationInputCtrl::handleToolTip(S32 x, S32 y, MASK mask)
{

	if(mAddLandmarkBtn->parentPointInView(x,y))
	{
		updateAddLandmarkTooltip();
	}
	// Let the buttons show their tooltips.
	if (LLUICtrl::handleToolTip(x, y, mask))
	{
		if (mList->getRect().pointInRect(x, y)) 
		{
			S32 loc_x, loc_y;
			//x,y - contain coordinates related to the location input control, but without taking the expanded list into account
			//So we have to convert it again into local coordinates of mList
			localPointToOtherView(x,y,&loc_x,&loc_y,mList);
			
			LLScrollListItem* item =  mList->hitItem(loc_x,loc_y);
			if (item)
			{
				LLSD value = item->getValue();
				if (value.has("tooltip"))
				{
					LLToolTipMgr::instance().show(value["tooltip"]);
				}
			}
		}

		return TRUE;
	}

	return FALSE;
}

BOOL LLLocationInputCtrl::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = LLComboBox::handleKeyHere(key, mask);

	if (key == KEY_DOWN && hasFocus() && mList->getItemCount() != 0 && !mList->getVisible())
	{
		showList();
	}

	return result;
}

void LLLocationInputCtrl::onTextEntry(LLLineEditor* line_editor)
{
	KEY key = gKeyboard->currentKey();
	MASK mask = gKeyboard->currentMask(TRUE);

	// Typing? (moving cursor should not affect showing the list)
	bool typing = mask != MASK_CONTROL && key != KEY_LEFT && key != KEY_RIGHT && key != KEY_HOME && key != KEY_END;
	bool pasting = mask == MASK_CONTROL && key == 'V';

	if (line_editor->getText().empty())
	{
		prearrangeList(); // resets filter
		hideList();
	}
	else if (typing || pasting)
	{
		prearrangeList(line_editor->getText());
		if (mList->getItemCount() != 0)
		{
			showList();
			focusTextEntry();
		}
		else
		{
			// Hide the list if it's empty.
			hideList();
		}
	}
	
	LLComboBox::onTextEntry(line_editor);
}

/**
 * Useful if we want to just set the text entry value, no matter what the list contains.
 *
 * This is faster than setTextEntry().
 */
void LLLocationInputCtrl::setText(const LLStringExplicit& text)
{
	if (mTextEntry)
	{
		mTextEntry->setText(text);
	}
	mHasAutocompletedText = FALSE;
}

void LLLocationInputCtrl::setFocus(BOOL b)
{
	LLComboBox::setFocus(b);

	if (mTextEntry && b && !mList->getVisible())
	{
		mTextEntry->setFocus(TRUE);
	}
}

void LLLocationInputCtrl::handleLoginComplete()
{
	// An agent parcel update hasn't occurred yet, so we have to
	// manually set location and the appropriate "Add landmark" icon.
	refresh();
}

//== private methods =========================================================

void LLLocationInputCtrl::onFocusReceived()
{
	prearrangeList();
}

void LLLocationInputCtrl::onFocusLost()
{
	LLUICtrl::onFocusLost();
	refreshLocation();

	// Setting cursor to 0  to show the left edge of the text. See STORM-370.
	mTextEntry->setCursor(0);

	if(mTextEntry->hasSelection()){
		mTextEntry->deselect();
	}
}

void LLLocationInputCtrl::draw()
{
	static LLUICachedControl<bool> show_coords("NavBarShowCoordinates", false);
	if(!hasFocus() && show_coords)
	{
		refreshLocation();
	}
	
	static LLUICachedControl<bool> show_icons("NavBarShowParcelProperties", false);
	if (show_icons)
	{
		refreshHealth();
	}
	LLComboBox::draw();
}

void LLLocationInputCtrl::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLComboBox::reshape(width, height, called_from_parent);

	// Setting cursor to 0  to show the left edge of the text. See EXT-4967.
	mTextEntry->setCursor(0);
	if (mTextEntry->hasSelection())
	{
		// Deselecting because selection position is changed together with
		// cursor position change.
		mTextEntry->deselect();
	}

	if (isHumanReadableLocationVisible)
	{
		refreshMaturityButton();
	}
}

void LLLocationInputCtrl::onInfoButtonClicked()
{
	LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "agent"));
}

void LLLocationInputCtrl::onForSaleButtonClicked()
{
	handle_buy_land();
}

void LLLocationInputCtrl::onAddLandmarkButtonClicked()
{
	LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();
	// Landmark exists, open it for preview and edit
	if(landmark && landmark->getUUID().notNull())
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = landmark->getUUID();

		LLFloaterSidePanelContainer::showPanel("places", key);
	}
	else
	{
		LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "create_landmark"));
	}
}

void LLLocationInputCtrl::onAgentParcelChange()
{
	refresh();
}

void LLLocationInputCtrl::onMaturityButtonClicked()
{
	LLUI::sHelpImpl->showTopic(mMaturityHelpTopic);
}

void LLLocationInputCtrl::onRegionBoundaryCrossed()
{
	createNavMeshStatusListenerForCurrentRegion();
}

void LLLocationInputCtrl::onNavMeshStatusChange(const LLPathfindingNavMeshStatus &pNavMeshStatus)
{
	mIsNavMeshDirty = pNavMeshStatus.isValid() && (pNavMeshStatus.getStatus() != LLPathfindingNavMeshStatus::kComplete);
	refreshParcelIcons();
}

void LLLocationInputCtrl::onLandmarkLoaded(LLLandmark* lm)
{
	(void) lm;
	updateAddLandmarkButton();
}

void LLLocationInputCtrl::onLocationHistoryChanged(LLLocationHistory::EChangeType event)
{
	if(event == LLLocationHistory::LOAD)
	{
		rebuildLocationHistory();
	}
	mButton->setEnabled(LLLocationHistory::instance().getItemCount() > 0);
}

void LLLocationInputCtrl::onLocationPrearrange(const LLSD& data)
{
	std::string filter = data.asString();
	rebuildLocationHistory(filter);

	//Let's add landmarks to the top of the list if any
	if(!filter.empty() )
	{
		LLInventoryModel::item_array_t landmark_items = LLLandmarkActions::fetchLandmarksByName(filter, TRUE);

		for(U32 i=0; i < landmark_items.size(); i++)
		{
			LLSD value;
			//TODO:: DO we need tooltip for Landmark??
			
			value["item_type"] = LANDMARK;
			value["AssetUUID"] =  landmark_items[i]->getAssetUUID(); 
			add(landmark_items[i]->getName(), value);
			
		}
	//Let's add teleport history items
		LLTeleportHistory* th = LLTeleportHistory::getInstance();
		LLTeleportHistory::slurl_list_t th_items = th->getItems();

		std::set<std::string> new_item_titles;// duplicate control
		LLTeleportHistory::slurl_list_t::iterator result = std::find_if(
				th_items.begin(), th_items.end(), boost::bind(
						&LLLocationInputCtrl::findTeleportItemsByTitle, this,
						_1, filter));

		while (result != th_items.end())
		{
			//mTitile format - region_name[, parcel_name]
			//mFullTitile format - region_name[, parcel_name] (local_x,local_y, local_z)
			if (new_item_titles.insert(result->mFullTitle).second)
			{
				LLSD value;
				value["item_type"] = TELEPORT_HISTORY;
				value["global_pos"] = result->mGlobalPos.getValue();
				std::string region_name = result->mTitle.substr(0, result->mTitle.find(','));
				//TODO*: add Surl to teleportitem or parse region name from title
				value["tooltip"] = LLSLURL(region_name, result->mGlobalPos).getSLURLString();
				add(result->getTitle(), value); 
			}
			result = std::find_if(result + 1, th_items.end(), boost::bind(
									&LLLocationInputCtrl::findTeleportItemsByTitle, this,
									_1, filter));
		}
	}
	sortByName();
	
	mList->mouseOverHighlightNthItem(-1); // Clear highlight on the last selected item.
}

bool LLLocationInputCtrl::findTeleportItemsByTitle(const LLTeleportHistoryItem& item, const std::string& filter)
{
	return item.mTitle.find(filter) != std::string::npos;
}

void LLLocationInputCtrl::onTextEditorRightClicked(S32 x, S32 y, MASK mask)
{
	if (mLocationContextMenu)
	{
		updateContextMenu();
		mLocationContextMenu->buildDrawLabels();
		mLocationContextMenu->updateParent(LLMenuGL::sMenuContainer);
		hideList();
		setFocus(true);
		changeLocationPresentation();
		LLMenuGL::showPopup(this, mLocationContextMenu, x, y);
	}
}

void LLLocationInputCtrl::refresh()
{
	refreshLocation();			// update location string
	refreshParcelIcons();
	updateAddLandmarkButton();	// indicate whether current parcel has been landmarked 
}

void LLLocationInputCtrl::refreshLocation()
{
	// Is one of our children focused?
	if (LLUICtrl::hasFocus() || mButton->hasFocus() || mList->hasFocus() ||
	    (mTextEntry && mTextEntry->hasFocus()) ||
	    (mAddLandmarkBtn->hasFocus()))
	{
		llwarns << "Location input should not be refreshed when having focus" << llendl;
		return;
	}

	// Update location field.
	std::string location_name;
	LLAgentUI::ELocationFormat format =
		(gSavedSettings.getBOOL("NavBarShowCoordinates")
			? LLAgentUI::LOCATION_FORMAT_FULL
			: LLAgentUI::LOCATION_FORMAT_NO_COORDS);

	if (!LLAgentUI::buildLocationString(location_name, format)) 
	{
		location_name = "???";
	}
	// store human-readable location to compare it in changeLocationPresentation()
	mHumanReadableLocation = location_name;
	setText(location_name);
	isHumanReadableLocationVisible = true;

	refreshMaturityButton();
}

// returns new right edge
static S32 layout_widget(LLUICtrl* widget, S32 right)
{
	if (widget->getVisible())
	{
		LLRect rect = widget->getRect();
		rect.mLeft = right - rect.getWidth();
		rect.mRight = right;
		widget->setRect( rect );
		right -= rect.getWidth();
	}
	return right;
}

void LLLocationInputCtrl::refreshParcelIcons()
{
	// Our "cursor" moving right to left
	S32 x = mAddLandmarkBtn->getRect().mLeft;

	LLViewerParcelMgr* vpm = LLViewerParcelMgr::getInstance();

	LLViewerRegion* agent_region = gAgent.getRegion();
	LLParcel* agent_parcel = vpm->getAgentParcel();
	if (!agent_region || !agent_parcel)
		return;

	mForSaleBtn->setVisible(vpm->canAgentBuyParcel(agent_parcel, false));

	x = layout_widget(mForSaleBtn, x);

	if (gSavedSettings.getBOOL("NavBarShowParcelProperties"))
	{
		LLParcel* current_parcel;
		LLViewerRegion* selection_region = vpm->getSelectionRegion();
		LLParcel* selected_parcel = vpm->getParcelSelection()->getParcel();

		// If agent is in selected parcel we use its properties because
		// they are updated more often by LLViewerParcelMgr than agent parcel properties.
		// See LLViewerParcelMgr::processParcelProperties().
		// This is needed to reflect parcel restrictions changes without having to leave
		// the parcel and then enter it again. See EXT-2987
		if (selected_parcel && selected_parcel->getLocalID() == agent_parcel->getLocalID()
				&& selection_region == agent_region)
		{
			current_parcel = selected_parcel;
		}
		else
		{
			current_parcel = agent_parcel;
		}

		bool allow_voice	= vpm->allowAgentVoice(agent_region, current_parcel);
		bool allow_fly		= vpm->allowAgentFly(agent_region, current_parcel);
		bool allow_push		= vpm->allowAgentPush(agent_region, current_parcel);
		bool allow_build	= vpm->allowAgentBuild(current_parcel); // true when anyone is allowed to build. See EXT-4610.
		bool allow_scripts	= vpm->allowAgentScripts(agent_region, current_parcel);
		bool allow_damage	= vpm->allowAgentDamage(agent_region, current_parcel);
		bool see_avs        = current_parcel->getSeeAVs();
		bool pathfinding_dynamic_enabled = agent_region->dynamicPathfindingEnabled();

		// Most icons are "block this ability"
		mParcelIcon[VOICE_ICON]->setVisible(   !allow_voice );
		mParcelIcon[FLY_ICON]->setVisible(     !allow_fly );
		mParcelIcon[PUSH_ICON]->setVisible(    !allow_push );
		mParcelIcon[BUILD_ICON]->setVisible(   !allow_build );
		mParcelIcon[SCRIPTS_ICON]->setVisible( !allow_scripts );
		mParcelIcon[DAMAGE_ICON]->setVisible(  allow_damage );
		mParcelIcon[PATHFINDING_DIRTY_ICON]->setVisible(mIsNavMeshDirty);
		mParcelIcon[PATHFINDING_DISABLED_ICON]->setVisible(!mIsNavMeshDirty && !pathfinding_dynamic_enabled);

		mDamageText->setVisible(allow_damage);
		mParcelIcon[SEE_AVATARS_ICON]->setVisible( !see_avs );

		// Padding goes to left of both landmark star and for sale btn
		x -= mAddLandmarkHPad;

		// Slide the parcel icons rect from right to left, adjusting rectangles
		for (S32 i = 0; i < ICON_COUNT; ++i)
		{
			x = layout_widget(mParcelIcon[i], x);
			x -= mIconHPad;
		}
		x = layout_widget(mDamageText, x);
		x -= mIconHPad;
	}
	else
	{
		for (S32 i = 0; i < ICON_COUNT; ++i)
		{
			mParcelIcon[i]->setVisible(false);
		}
		mDamageText->setVisible(false);
	}

	if (mTextEntry)
	{
		S32 left_pad, right_pad;
		mTextEntry->getTextPadding(&left_pad, &right_pad);
		right_pad = mTextEntry->getRect().mRight - x;
		mTextEntry->setTextPadding(left_pad, right_pad);
	}
}

void LLLocationInputCtrl::refreshHealth()
{
	// *FIXME: Status bar owns health information, should be in agent
	if (gStatusBar)
	{
		static S32 last_health = -1;
		S32 health = gStatusBar->getHealth();
		if (health != last_health)
		{
			std::string text = llformat("%d%%", health);
			mDamageText->setText(text);
			last_health = health;
		}
	}
}

void LLLocationInputCtrl::refreshMaturityButton()
{
	// Updating maturity rating icon.
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
		return;

	bool button_visible = true;
	LLPointer<LLUIImage> rating_image = NULL;
	std::string rating_tooltip;

	U8 sim_access = region->getSimAccess();
	switch(sim_access)
	{
	case SIM_ACCESS_PG:
		rating_image = mIconMaturityGeneral;
		rating_tooltip = LLTrans::getString("LocationCtrlGeneralIconTooltip");
		break;

	case SIM_ACCESS_ADULT:
		rating_image = mIconMaturityAdult;
		rating_tooltip = LLTrans::getString("LocationCtrlAdultIconTooltip");
		break;

	case SIM_ACCESS_MATURE:
		rating_image = mIconMaturityModerate;
		rating_tooltip = LLTrans::getString("LocationCtrlModerateIconTooltip");
		break;

	default:
		button_visible = false;
		break;
	}

	mMaturityButton->setVisible(button_visible);
	mMaturityButton->setToolTip(rating_tooltip);
	if(rating_image)
	{
		mMaturityButton->setImageUnselected(rating_image);
		mMaturityButton->setImagePressed(rating_image);
	}
	if (mMaturityButton->getVisible())
	{
		positionMaturityButton();
	}
}

void LLLocationInputCtrl::positionMaturityButton()
{
	const LLFontGL* font = mTextEntry->getFont();
	if (!font)
		return;

	S32 left_pad, right_pad;
	mTextEntry->getTextPadding(&left_pad, &right_pad);

	// Calculate the right edge of rendered text + a whitespace.
	left_pad = left_pad + font->getWidth(mTextEntry->getText()) + font->getWidth(" ");

	LLRect rect = mMaturityButton->getRect();
	mMaturityButton->setRect(rect.setOriginAndSize(left_pad, rect.mBottom, rect.getWidth(), rect.getHeight()));

	// Hide icon if it text area is not width enough to display it, show otherwise.
	mMaturityButton->setVisible(rect.mRight < mTextEntry->getRect().getWidth() - right_pad);
}

void LLLocationInputCtrl::rebuildLocationHistory(const std::string& filter)
{
	LLLocationHistory::location_list_t filtered_items;
	const LLLocationHistory::location_list_t* itemsp = NULL;
	LLLocationHistory* lh = LLLocationHistory::getInstance();
	
	if (filter.empty())
	{
		itemsp = &lh->getItems();
	}
	else
	{
		lh->getMatchingItems(filter, filtered_items);
		itemsp = &filtered_items;
	}
	
	removeall();
	for (LLLocationHistory::location_list_t::const_reverse_iterator it = itemsp->rbegin(); it != itemsp->rend(); it++)
	{
		LLSD value;
		value["tooltip"] = it->getToolTip();
		//location history can contain only typed locations
		value["item_type"] = TYPED_REGION_SLURL;
		value["global_pos"] = it->mGlobalPos.getValue();
		add(it->getLocation(), value);
	}
}

void LLLocationInputCtrl::focusTextEntry()
{
	// We can't use "mTextEntry->setFocus(TRUE)" instead because
	// if the "select_on_focus" parameter is true it places the cursor
	// at the beginning (after selecting text), thus screwing up updateSelection().
	if (mTextEntry)
	{
		gFocusMgr.setKeyboardFocus(mTextEntry);

		// Enable the text entry to handle accelerator keys (EXT-8104).
		LLEditMenuHandler::gEditMenuHandler = mTextEntry;
	}
}

void LLLocationInputCtrl::enableAddLandmarkButton(bool val)
{
	// We don't want to disable the button because it should be click able at any time, 
	// instead switch images.
	LLUIImage* img = val ? mLandmarkImageOn : mLandmarkImageOff;
	if(img)
	{
		mAddLandmarkBtn->setImageUnselected(img);
	}
}

// Change the "Add landmark" button image
// depending on whether current parcel has been landmarked.
void LLLocationInputCtrl::updateAddLandmarkButton()
{
	enableAddLandmarkButton(LLLandmarkActions::hasParcelLandmark());
}
void LLLocationInputCtrl::updateAddLandmarkTooltip()
{
	std::string tooltip;
	if(LLLandmarkActions::landmarkAlreadyExists())
	{
		tooltip = mEditLandmarkTooltip;
	}
	else
	{
		tooltip = mAddLandmarkTooltip;
	}
	mAddLandmarkBtn->setToolTip(tooltip);
}

void LLLocationInputCtrl::updateContextMenu(){

	if (mLocationContextMenu)
	{
		LLMenuItemGL* landmarkItem = mLocationContextMenu->getChild<LLMenuItemGL>("Landmark");
		if (!LLLandmarkActions::landmarkAlreadyExists())
		{
			landmarkItem->setLabel(LLTrans::getString("AddLandmarkNavBarMenu"));
		}
		else
		{
			landmarkItem->setLabel(LLTrans::getString("EditLandmarkNavBarMenu"));
		}
	}
}
void LLLocationInputCtrl::updateWidgetlayout()
{
	const LLRect&	rect			= getLocalRect();
	const LLRect&	hist_btn_rect	= mButton->getRect();

	// Info button is set in the XUI XML location_input.xml

	// "Add Landmark" button
	LLRect al_btn_rect = mAddLandmarkBtn->getRect();
	al_btn_rect.translate(
		hist_btn_rect.mLeft - mIconHPad - al_btn_rect.getWidth(),
		(rect.getHeight() - al_btn_rect.getHeight()) / 2);
	mAddLandmarkBtn->setRect(al_btn_rect);
}

void LLLocationInputCtrl::changeLocationPresentation()
{
	if (!mTextEntry)
		return;

	//change location presentation only if user does not select/paste anything and 
	//human-readable region name is being displayed
	std::string text = mTextEntry->getText();
	LLStringUtil::trim(text);
	if(!mTextEntry->hasSelection() && text == mHumanReadableLocation)
	{
		//needs unescaped one
		LLSLURL slurl;
		LLAgentUI::buildSLURL(slurl, false);
		mTextEntry->setText(LLURI::unescape(slurl.getSLURLString()));
		mTextEntry->selectAll();

		mMaturityButton->setVisible(FALSE);

		isHumanReadableLocationVisible = false;
	}
}

void LLLocationInputCtrl::onLocationContextMenuItemClicked(const LLSD& userdata)
{
	std::string item = userdata.asString();

	if (item == "show_coordinates")
	{
		gSavedSettings.setBOOL("NavBarShowCoordinates",!gSavedSettings.getBOOL("NavBarShowCoordinates"));
	}
	else if (item == "show_properties")
	{
		gSavedSettings.setBOOL("NavBarShowParcelProperties",
			!gSavedSettings.getBOOL("NavBarShowParcelProperties"));
	}
	else if (item == "landmark")
	{
		LLViewerInventoryItem* landmark = LLLandmarkActions::findLandmarkForAgentPos();
		
		if(!landmark)
		{
			LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "create_landmark"));
		}
		else
		{
			LLFloaterSidePanelContainer::showPanel("places", LLSD().with("type", "landmark").with("id",landmark->getUUID()));

		}
	}
	else if (item == "cut")
	{
		mTextEntry->cut();
	}
	else if (item == "copy")
	{
		mTextEntry->copy();
	}
	else if (item == "paste")
	{
		mTextEntry->paste();
	}
	else if (item == "delete")
	{
		mTextEntry->deleteSelection();
	}
	else if (item == "select_all")
	{
		mTextEntry->selectAll();
	}
}

bool LLLocationInputCtrl::onLocationContextMenuItemEnabled(const LLSD& userdata)
{
	std::string item = userdata.asString();
	
	if (item == "can_cut")
	{
		return mTextEntry->canCut();
	}
	else if (item == "can_copy")
	{
		return mTextEntry->canCopy();
	}
	else if (item == "can_paste")
	{
		return mTextEntry->canPaste();
	}
	else if (item == "can_delete")
	{
		return mTextEntry->canDeselect();
	}
	else if (item == "can_select_all")
	{
		return mTextEntry->canSelectAll() && (mTextEntry->getLength() > 0);
	}
	else if(item == "show_coordinates")
	{
		return gSavedSettings.getBOOL("NavBarShowCoordinates");
	}

	return false;
}

void LLLocationInputCtrl::onParcelIconClick(EParcelIcon icon)
{
	switch (icon)
	{
	case VOICE_ICON:
		LLNotificationsUtil::add("NoVoice");
		break;
	case FLY_ICON:
		LLNotificationsUtil::add("NoFly");
		break;
	case PUSH_ICON:
		LLNotificationsUtil::add("PushRestricted");
		break;
	case BUILD_ICON:
		LLNotificationsUtil::add("NoBuild");
		break;
	case PATHFINDING_DIRTY_ICON:
		LLNotificationsUtil::add("PathfindingDirty");
		break;
	case PATHFINDING_DISABLED_ICON:
		LLNotificationsUtil::add("DynamicPathfindingDisabled");
		break;
	case SCRIPTS_ICON:
	{
		LLViewerRegion* region = gAgent.getRegion();
		if(region && region->getRegionFlag(REGION_FLAGS_ESTATE_SKIP_SCRIPTS))
		{
			LLNotificationsUtil::add("ScriptsStopped");
		}
		else if(region && region->getRegionFlag(REGION_FLAGS_SKIP_SCRIPTS))
		{
			LLNotificationsUtil::add("ScriptsNotRunning");
		}
		else
		{
			LLNotificationsUtil::add("NoOutsideScripts");
		}
		break;
	}
	case DAMAGE_ICON:
		LLNotificationsUtil::add("NotSafe");
		break;
	case SEE_AVATARS_ICON:
		LLNotificationsUtil::add("SeeAvatars");
		break;
	case ICON_COUNT:
		break;
	// no default to get compiler warning when a new icon gets added
	}
}

void LLLocationInputCtrl::createNavMeshStatusListenerForCurrentRegion()
{
	if (mNavMeshSlot.connected())
	{
		mNavMeshSlot.disconnect();
	}

	LLViewerRegion *currentRegion = gAgent.getRegion();
	if (currentRegion != NULL)
	{
		mNavMeshSlot = LLPathfindingManager::getInstance()->registerNavMeshListenerForRegion(currentRegion, boost::bind(&LLLocationInputCtrl::onNavMeshStatusChange, this, _2));
		LLPathfindingManager::getInstance()->requestGetNavMeshForRegion(currentRegion, true);
	}
}
