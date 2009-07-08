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
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llkeyboard.h"
#include "llstring.h"
#include "lluictrlfactory.h"
#include "v2math.h"

// newview includes
#include "llagent.h"
#include "llfloaterland.h"
#include "llinventorymodel.h"
#include "lllandmarklist.h"
#include "lllocationhistory.h"
#include "llsidetray.h"
#include "llviewerinventory.h"
#include "llviewerparcelmgr.h"

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

// Returns true if the given inventory item is a landmark pointing to the current parcel.
// Used to filter inventory items.
class LLIsAgentParcelLandmark : public LLInventoryCollectFunctor
{
public:
	/*virtual*/ bool operator()(LLInventoryCategory* cat, LLInventoryItem* item)
	{
		if (!item || item->getType() != LLAssetType::AT_LANDMARK)
			return false;

		LLLandmark* landmark = gLandmarkList.getAsset(item->getAssetUUID());
		if (!landmark) // the landmark not been loaded yet
			return false;

		LLVector3d landmark_global_pos;
		if (!landmark->getGlobalPos(landmark_global_pos))
			return false;

		return LLViewerParcelMgr::getInstance()->inAgentParcel(landmark_global_pos);
	}
};

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
	mAddLandmarkBtn(NULL)
{
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
	
	setPrearrangeCallback(boost::bind(&LLLocationInputCtrl::onLocationPrearrange, this, _2));

	updateWidgetlayout();

	// - Make the "Add landmark" button updated when either current parcel gets changed
	//   or a landmark gets created or removed from the inventory.
	// - Update the location string on parcel change.
	LLViewerParcelMgr::getInstance()->setAgentParcelChangedCallback(
		boost::bind(&LLLocationInputCtrl::onAgentParcelChange, this));

	LLLocationHistory::getInstance()->setLoadedCallback(
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
		return TRUE;
	}

	// Cursor is above the text entry.
	msg = LLUI::sShowXUINames ? getShowNamesToolTip() : gAgent.getSLURL();
	if (mTextEntry && sticky_rect_screen)
	{
		*sticky_rect_screen = mTextEntry->calcScreenRect();
	}

	return TRUE;
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
	setText(gAgent.getSLURL());
	if (mTextEntry)
		mTextEntry->endSelection(); // we don't want handleMouseUp() to "finish" the selection
}

void LLLocationInputCtrl::onFocusLost()
{
	LLUICtrl::onFocusLost();
	refreshLocation();
}

void LLLocationInputCtrl::onInfoButtonClicked()
{
	LLSideTray::getInstance()->showPanel("panel_places", LLSD().insert("type", "agent"));
}

void LLLocationInputCtrl::onAddLandmarkButtonClicked()
{
	LLFloaterReg::showInstance("add_landmark");
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
	mList->mouseOverHighlightNthItem(-1); // Clear highlight on the last selected item.
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

	if (!gAgent.buildLocationString(location_name, LLAgent::LOCATION_FORMAT_NORMAL))
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
	bool cur_parcel_landmarked = false;
	// Determine whether there are landmarks pointing to the current parcel.
	LLInventoryModel::cat_array_t cats;
	LLInventoryModel::item_array_t items;
	LLIsAgentParcelLandmark is_current_parcel_landmark;
	gInventory.collectDescendentsIf(gInventory.getRootFolderID(),
		cats,
		items,
		LLInventoryModel::EXCLUDE_TRASH,
		is_current_parcel_landmark);
	cur_parcel_landmarked = !items.empty();

	enableAddLandmarkButton(!cur_parcel_landmarked);
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
