/**
 * @file lllocationinputctrl.cpp
 * @brief Combobox-like location input control
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

// file includes
#include "lllocationinputctrl.h"

// common includes
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llmenugl.h"
#include "llstring.h"
#include "lluictrlfactory.h"

// newview includes
#include "llagent.h"
#include "llinventorymodel.h"
#include "lllandmarkactions.h"
#include "lllandmarklist.h"
#include "lllocationhistory.h"
#include "llsidetray.h"
#include "llslurl.h"
#include "lltrans.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"
#include "llviewercontrol.h"
#include "llviewermenu.h"
#include "llurllineeditorctrl.h"
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
		std::vector<LLUUID>::const_iterator it = mAdded.begin(), end = mAdded.end();
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

//============================================================================


static LLDefaultChildRegistry::Register<LLLocationInputCtrl> r("location_input");

LLLocationInputCtrl::Params::Params()
:	add_landmark_image_enabled("add_landmark_image_enabled"),
	add_landmark_image_disabled("add_landmark_image_disabled"),
	add_landmark_button("add_landmark_button"),
	add_landmark_hpad("add_landmark_hpad", 0),
	info_button("info_button")
{
}

LLLocationInputCtrl::LLLocationInputCtrl(const LLLocationInputCtrl::Params& p)
:	LLComboBox(p),
	mAddLandmarkHPad(p.add_landmark_hpad),
	mInfoBtn(NULL),
	mLocationContextMenu(NULL),
	mAddLandmarkBtn(NULL)
{
	// Lets replace default LLLineEditor with LLLocationLineEditor
	// to make needed escaping while copying and cutting url
	this->removeChild(mTextEntry);
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
	params.max_length_bytes(p.max_chars);
	params.commit_callback.function(boost::bind(&LLComboBox::onTextCommit, this, _2));
	params.keystroke_callback(boost::bind(&LLComboBox::onTextEntry, this, _1));
	params.focus_lost_callback(NULL);
	params.handle_edit_keys_directly(true);
	params.commit_on_focus_lost(false);
	params.follows.flags(FOLLOWS_ALL);
	mTextEntry = LLUICtrlFactory::create<LLURLLineEditor>(params);
	this->addChild(mTextEntry);
	// LLLineEditor is replaced with LLLocationLineEditor

	// "Place information" button.
	LLButton::Params info_params = p.info_button;
	mInfoBtn = LLUICtrlFactory::create<LLButton>(info_params);
	mInfoBtn->setClickedCallback(boost::bind(&LLLocationInputCtrl::onInfoButtonClicked, this));
	addChild(mInfoBtn);

	// "Add landmark" button.
	LLButton::Params al_params = p.add_landmark_button;
	if (p.add_landmark_image_enabled())
	{
		al_params.image_unselected = p.add_landmark_image_enabled;
		al_params.image_selected = p.add_landmark_image_enabled;
	}
	if (p.add_landmark_image_disabled())
	{
		al_params.image_disabled = p.add_landmark_image_disabled;
		al_params.image_disabled_selected = p.add_landmark_image_disabled;
	}
	al_params.click_callback.function(boost::bind(&LLLocationInputCtrl::onAddLandmarkButtonClicked, this));
	mAddLandmarkBtn = LLUICtrlFactory::create<LLButton>(al_params);
	enableAddLandmarkButton(true);
	addChild(mAddLandmarkBtn);
	
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

	// - Make the "Add landmark" button updated when either current parcel gets changed
	//   or a landmark gets created or removed from the inventory.
	// - Update the location string on parcel change.
	mParcelMgrConnection = LLViewerParcelMgr::getInstance()->setAgentParcelChangedCallback(
		boost::bind(&LLLocationInputCtrl::onAgentParcelChange, this));

	mLocationHistoryConnection = LLLocationHistory::getInstance()->setLoadedCallback(
			boost::bind(&LLLocationInputCtrl::onLocationHistoryLoaded, this));

	mRemoveLandmarkObserver	= new LLRemoveLandmarkObserver(this);
	mAddLandmarkObserver	= new LLAddLandmarkObserver(this);
	gInventory.addObserver(mRemoveLandmarkObserver);
	gInventory.addObserver(mAddLandmarkObserver);
}

LLLocationInputCtrl::~LLLocationInputCtrl()
{
	gInventory.removeObserver(mRemoveLandmarkObserver);
	gInventory.removeObserver(mAddLandmarkObserver);
	delete mRemoveLandmarkObserver;
	delete mAddLandmarkObserver;

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

BOOL LLLocationInputCtrl::handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect_screen)
{
	// Let the buttons show their tooltips.
	if (LLUICtrl::handleToolTip(x, y, msg, sticky_rect_screen) && !msg.empty())
	{
		if (mList->getRect().pointInRect(x, y)) {
			LLLocationHistory* lh = LLLocationHistory::getInstance();
			const std::string tooltip = lh->getToolTip(msg);

			if (!tooltip.empty()) {
				msg = tooltip;
			}
		}

		return TRUE;
	}

	msg = LLUI::sShowXUINames ? getShowNamesToolTip() : "";
	return mTextEntry->getRect().pointInRect(x, y);
}

BOOL LLLocationInputCtrl::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = LLComboBox::handleKeyHere(key, mask);

	if (key == KEY_DOWN && hasFocus() && mList->getItemCount() != 0)
	{
		showList();
	}

	return result;
}

void LLLocationInputCtrl::onTextEntry(LLLineEditor* line_editor)
{
	KEY key = gKeyboard->currentKey();

	if (line_editor->getText().empty())
	{
		prearrangeList(); // resets filter
		hideList();
	}
	// Typing? (moving cursor should not affect showing the list)
	else if (key != KEY_LEFT && key != KEY_RIGHT && key != KEY_HOME && key != KEY_END)
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
		mHasAutocompletedText = FALSE;
	}
}

void LLLocationInputCtrl::setFocus(BOOL b)
{
	LLComboBox::setFocus(b);

	if (mTextEntry && b && !mList->getVisible())
		mTextEntry->setFocus(TRUE);
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
	if(mTextEntry->hasSelection()){
		mTextEntry->deselect();
	}
}
void	LLLocationInputCtrl::draw(){
	
	if(!hasFocus() && gSavedSettings.getBOOL("ShowCoordinatesOption")){
		refreshLocation();
	}
	LLComboBox::draw();
}

void LLLocationInputCtrl::onInfoButtonClicked()
{
	LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "agent"));
}

void LLLocationInputCtrl::onAddLandmarkButtonClicked()
{
	LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "create_landmark"));
}

void LLLocationInputCtrl::onAgentParcelChange()
{
	refresh();
}

void LLLocationInputCtrl::onLandmarkLoaded(LLLandmark* lm)
{
	(void) lm;
	updateAddLandmarkButton();
}

void LLLocationInputCtrl::onLocationHistoryLoaded()
{
	rebuildLocationHistory();
}

void LLLocationInputCtrl::onLocationPrearrange(const LLSD& data)
{
	std::string filter = data.asString();
	rebuildLocationHistory(filter);

	//Let's add landmarks to the top of the list if any
	LLInventoryModel::item_array_t landmark_items = LLLandmarkActions::fetchLandmarksByName(filter, TRUE);

	for(U32 i=0; i < landmark_items.size(); i++)
	{
		mList->addSimpleElement(landmark_items[i]->getName(), ADD_TOP);
	}
	mList->mouseOverHighlightNthItem(-1); // Clear highlight on the last selected item.
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
	updateAddLandmarkButton();	// indicate whether current parcel has been landmarked 
}

void LLLocationInputCtrl::refreshLocation()
{
	// Is one of our children focused?
	if (LLUICtrl::hasFocus() || mButton->hasFocus() || mList->hasFocus() ||
		(mTextEntry && mTextEntry->hasFocus()) || (mAddLandmarkBtn->hasFocus()))

	{
		llwarns << "Location input should not be refreshed when having focus" << llendl;
		return;
	}

	// Update location field.
	std::string location_name;
	LLAgent::ELocationFormat format =  (gSavedSettings.getBOOL("ShowCoordinatesOption") ? 
			LLAgent::LOCATION_FORMAT_WITHOUT_SIM: LLAgent::LOCATION_FORMAT_NORMAL);

	if (!gAgent.buildLocationString(location_name,format))
		location_name = "Unknown";

	setText(location_name);
}

void LLLocationInputCtrl::rebuildLocationHistory(std::string filter)
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
		add(*it);
	}
}

void LLLocationInputCtrl::focusTextEntry()
{
	// We can't use "mTextEntry->setFocus(TRUE)" instead because
	// if the "select_on_focus" parameter is true it places the cursor
	// at the beginning (after selecting text), thus screwing up updateSelection().
	if (mTextEntry)
		gFocusMgr.setKeyboardFocus(mTextEntry);
}

void LLLocationInputCtrl::enableAddLandmarkButton(bool val)
{
	// Enable/disable the button.
	mAddLandmarkBtn->setEnabled(val);
}

// Change the "Add landmark" button image
// depending on whether current parcel has been landmarked.
void LLLocationInputCtrl::updateAddLandmarkButton()
{
	enableAddLandmarkButton(!LLLandmarkActions::landmarkAlreadyExists());
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
	LLRect			info_btn_rect	= mInfoBtn->getRect();

	// info button
	info_btn_rect.setOriginAndSize(
		2, (rect.getHeight() - info_btn_rect.getHeight()) / 2,
		info_btn_rect.getWidth(), info_btn_rect.getHeight());
	mInfoBtn->setRect(info_btn_rect);

	// "Add Landmark" button
	{
		LLRect al_btn_rect = mAddLandmarkBtn->getRect();
		al_btn_rect.translate(
			hist_btn_rect.mLeft - mAddLandmarkHPad - al_btn_rect.getWidth(),
			(rect.getHeight() - al_btn_rect.getHeight()) / 2);
		mAddLandmarkBtn->setRect(al_btn_rect);
	}
}

void LLLocationInputCtrl::changeLocationPresentation()
{
	//change location presentation only if user does not  select anything and 
	//human-readable region name  is being displayed
	if(mTextEntry && !mTextEntry->hasSelection() && 
		!LLSLURL::isSLURL(mTextEntry->getText()))
	{
		mTextEntry->setText(gAgent.getUnescapedSLURL());
		mTextEntry->selectAll();
	}	
}

void LLLocationInputCtrl::onLocationContextMenuItemClicked(const LLSD& userdata)
{
	std::string item = userdata.asString();

	if (item == std::string("show_coordinates"))
	{
		gSavedSettings.setBOOL("ShowCoordinatesOption",!gSavedSettings.getBOOL("ShowCoordinatesOption"));
	}
	else if (item == std::string("landmark"))
	{
		LLInventoryModel::item_array_t items;
		LLLandmarkActions::collectParcelLandmark(items);
		
		if(items.empty())
		{
			LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "create_landmark"));
		}else{
			LLSideTray::getInstance()->showPanel("panel_places", 
					LLSD().insert("type", "landmark").insert("id",items.get(0)->getUUID()));
		}
	}
	else if (item == std::string("cut"))
	{
		mTextEntry->cut();
	}
	else if (item == std::string("copy"))
	{
		mTextEntry->copy();
	}
	else if (item == std::string("paste"))
	{
		mTextEntry->paste();
	}
	else if (item == std::string("delete"))
	{
		mTextEntry->deleteSelection();
	}
	else if (item == std::string("select_all"))
	{
		mTextEntry->selectAll();
	}
}

bool LLLocationInputCtrl::onLocationContextMenuItemEnabled(const LLSD& userdata)
{
	std::string item = userdata.asString();
	
	if (item == std::string("can_cut"))
	{
		return mTextEntry->canCut();
	}
	else if (item == std::string("can_copy"))
	{
		return mTextEntry->canCopy();
	}
	else if (item == std::string("can_paste"))
	{
		return mTextEntry->canPaste();
	}
	else if (item == std::string("can_delete"))
	{
		return mTextEntry->canDeselect();
	}
	else if (item == std::string("can_select_all"))
	{
		return mTextEntry->canSelectAll();
	}
	else if(item == std::string("show_coordinates")){
	
		return gSavedSettings.getBOOL("ShowCoordinatesOption");
	}

	return false;
}
