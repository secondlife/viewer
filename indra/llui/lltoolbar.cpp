/** 
 * @file lltoolbar.cpp
 * @author Richard Nelson
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "linden_common.h"

#include <boost/foreach.hpp>
#include "lltoolbar.h"

#include "llcommandmanager.h"
#include "llmenugl.h"
#include "lltrans.h"
#include "llinventory.h"
#include "lliconctrl.h"

// uncomment this and remove the one in llui.cpp when there is an external reference to this translation unit
// thanks, MSVC!
//static LLDefaultChildRegistry::Register<LLToolBar> r1("toolbar");

namespace LLToolBarEnums
{
	LLLayoutStack::ELayoutOrientation getOrientation(SideType sideType)
	{
		LLLayoutStack::ELayoutOrientation orientation = LLLayoutStack::HORIZONTAL;

		if ((sideType == SIDE_LEFT) || (sideType == SIDE_RIGHT))
		{
			orientation = LLLayoutStack::VERTICAL;
		}

		return orientation;
	}
}

using namespace LLToolBarEnums;


namespace LLInitParam
{
	void TypeValues<ButtonType>::declareValues()
	{
		declare("icons_with_text",	BTNTYPE_ICONS_WITH_TEXT);
		declare("icons_only",		BTNTYPE_ICONS_ONLY);
	}

	void TypeValues<SideType>::declareValues()
	{
		declare("bottom",	SIDE_BOTTOM);
		declare("left",		SIDE_LEFT);
		declare("right",	SIDE_RIGHT);
		declare("top",		SIDE_TOP);
	}
}

LLToolBar::Params::Params()
:	button_display_mode("button_display_mode"),
	commands("command"),
	side("side", SIDE_TOP),
	button_icon("button_icon"),
	button_icon_and_text("button_icon_and_text"),
	read_only("read_only", false),
	wrap("wrap", true),
	pad_left("pad_left"),
	pad_top("pad_top"),
	pad_right("pad_right"),
	pad_bottom("pad_bottom"),
	pad_between("pad_between"),
	min_girth("min_girth"),
	button_panel("button_panel")
{}

LLToolBar::LLToolBar(const LLToolBar::Params& p)
:	LLUICtrl(p),
	mReadOnly(p.read_only),
	mButtonType(p.button_display_mode),
	mSideType(p.side),
	mWrap(p.wrap),
	mNeedsLayout(false),
	mModified(false),
	mButtonPanel(NULL),
	mCenteringStack(NULL),
	mPadLeft(p.pad_left),
	mPadRight(p.pad_right),
	mPadTop(p.pad_top),
	mPadBottom(p.pad_bottom),
	mPadBetween(p.pad_between),
	mMinGirth(p.min_girth),
	mPopupMenuHandle(),
	mRightMouseTargetButton(NULL),
	mStartDragItemCallback(NULL),
	mHandleDragItemCallback(NULL),
	mHandleDropCallback(NULL),
	mButtonAddSignal(NULL),
	mButtonEnterSignal(NULL),
	mButtonLeaveSignal(NULL),
	mButtonRemoveSignal(NULL),
	mDragAndDropTarget(false)
{
	mButtonParams[LLToolBarEnums::BTNTYPE_ICONS_WITH_TEXT] = p.button_icon_and_text;
	mButtonParams[LLToolBarEnums::BTNTYPE_ICONS_ONLY] = p.button_icon;
}

LLToolBar::~LLToolBar()
{
	delete mPopupMenuHandle.get();
	delete mButtonAddSignal;
	delete mButtonEnterSignal;
	delete mButtonLeaveSignal;
	delete mButtonRemoveSignal;
}

void LLToolBar::createContextMenu()
{
	if (!mPopupMenuHandle.get())
	{
		// Setup bindings specific to this instance for the context menu options

		LLUICtrl::CommitCallbackRegistry::ScopedRegistrar commit_reg;
		commit_reg.add("Toolbars.EnableSetting", boost::bind(&LLToolBar::onSettingEnable, this, _2));
		commit_reg.add("Toolbars.RemoveSelectedCommand", boost::bind(&LLToolBar::onRemoveSelectedCommand, this));

		LLUICtrl::EnableCallbackRegistry::ScopedRegistrar enable_reg;
		enable_reg.add("Toolbars.CheckSetting", boost::bind(&LLToolBar::isSettingChecked, this, _2));

		// Create the context menu
		LLContextMenu* menu = LLUICtrlFactory::instance().createFromFile<LLContextMenu>("menu_toolbars.xml", LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());

		if (menu)
		{
			menu->setBackgroundColor(LLUIColorTable::instance().getColor("MenuPopupBgColor"));
			mPopupMenuHandle = menu->getHandle();
			mRemoveButtonHandle = menu->getChild<LLView>("Remove button")->getHandle();
		}
		else
		{
			llwarns << "Unable to load toolbars context menu." << llendl;
		}
	}
	
	if (mRemoveButtonHandle.get())
	{
		// Disable/Enable the "Remove button" menu item depending on whether or not a button was clicked
		mRemoveButtonHandle.get()->setEnabled(mRightMouseTargetButton != NULL);
	}
}

void LLToolBar::initFromParams(const LLToolBar::Params& p)
{
	// Initialize the base object
	LLUICtrl::initFromParams(p);
	
	LLLayoutStack::ELayoutOrientation orientation = getOrientation(p.side);

	LLLayoutStack::Params centering_stack_p;
	centering_stack_p.name = "centering_stack";
	centering_stack_p.rect = getLocalRect();
	centering_stack_p.follows.flags = FOLLOWS_ALL;
	centering_stack_p.orientation = orientation;
	centering_stack_p.mouse_opaque = false;

	mCenteringStack = LLUICtrlFactory::create<LLLayoutStack>(centering_stack_p);
	addChild(mCenteringStack);
	
	LLLayoutPanel::Params border_panel_p;
	border_panel_p.name = "border_panel";
	border_panel_p.rect = getLocalRect();
	border_panel_p.auto_resize = true;
	border_panel_p.user_resize = false;
	border_panel_p.mouse_opaque = false;
	
	mCenteringStack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	LLLayoutPanel::Params center_panel_p;
	center_panel_p.name = "center_panel";
	center_panel_p.rect = getLocalRect();
	center_panel_p.auto_resize = false;
	center_panel_p.user_resize = false;
	center_panel_p.mouse_opaque = false;
	LLLayoutPanel* center_panel = LLUICtrlFactory::create<LLLayoutPanel>(center_panel_p);
	mCenteringStack->addChild(center_panel);

	LLPanel::Params button_panel_p(p.button_panel);
	button_panel_p.rect = center_panel->getLocalRect();
		button_panel_p.follows.flags = FOLLOWS_BOTTOM|FOLLOWS_LEFT;
	mButtonPanel = LLUICtrlFactory::create<LLPanel>(button_panel_p);
	center_panel->addChild(mButtonPanel);
	
	mCenteringStack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	BOOST_FOREACH(LLCommandId id, p.commands)
	{
		addCommand(id);
	}

	mNeedsLayout = true;
}

bool LLToolBar::addCommand(const LLCommandId& commandId, int rank)
{
	LLCommand * command = LLCommandManager::instance().getCommand(commandId);
	if (!command) return false;
	
	// Create the button and do the things that don't need ordering
	LLToolBarButton* button = createButton(commandId);
	mButtonPanel->addChild(button);
	mButtonMap.insert(std::make_pair(commandId.uuid(), button));

	// Insert the command and button in the right place in their respective lists
	if ((rank >= mButtonCommands.size()) || (rank == RANK_NONE))
	{
		// In that case, back load
		mButtonCommands.push_back(command->id());
		mButtons.push_back(button);
	}
	else 
	{
		// Insert in place: iterate to the right spot...
		std::list<LLToolBarButton*>::iterator it_button = mButtons.begin();
		command_id_list_t::iterator it_command = mButtonCommands.begin();
		while (rank > 0)
		{
			++it_button;
			++it_command;
			rank--;
		}
		// ...then insert
		mButtonCommands.insert(it_command, command->id());
		mButtons.insert(it_button,button);
	}

	mNeedsLayout = true;

	updateLayoutAsNeeded();


	if (mButtonAddSignal)
	{
		(*mButtonAddSignal)(button);
	}

	return true;
}

// Remove a command from the list
// Returns the rank of the command in the original list so that doing addCommand(id,rank) right after
// a removeCommand(id) would leave the list unchanged.
// Returns RANK_NONE if the command is not found in the list
int LLToolBar::removeCommand(const LLCommandId& commandId)
{
	if (!hasCommand(commandId)) return RANK_NONE;
	
	// First erase the map record
	command_id_map::iterator it = mButtonMap.find(commandId.uuid());
	mButtonMap.erase(it);
	
	// Now iterate on the commands and buttons to identify the relevant records
	int rank = 0;
	std::list<LLToolBarButton*>::iterator it_button = mButtons.begin();
	command_id_list_t::iterator it_command = mButtonCommands.begin();
	while (*it_command != commandId)
	{
		++it_button;
		++it_command;
		++rank;
	}
	
	if (mButtonRemoveSignal)
	{
		(*mButtonRemoveSignal)(*it_button);
	}
	
	// Delete the button and erase the command and button records
	delete (*it_button);
	mButtonCommands.erase(it_command);
	mButtons.erase(it_button);

	mNeedsLayout = true;
	
	return rank;
}

void LLToolBar::clearCommandsList()
{
	// Clears the commands list
	mButtonCommands.clear();
	// This will clear the buttons
	createButtons();
}

bool LLToolBar::hasCommand(const LLCommandId& commandId) const
{
	if (commandId != LLCommandId::null)
	{
		command_id_map::const_iterator it = mButtonMap.find(commandId.uuid());
		return (it != mButtonMap.end());
	}

	return false;
}

bool LLToolBar::enableCommand(const LLCommandId& commandId, bool enabled)
{
	LLButton * command_button = NULL;
	
	if (commandId != LLCommandId::null)
	{
		command_id_map::iterator it = mButtonMap.find(commandId.uuid());
		if (it != mButtonMap.end())
		{
			command_button = it->second;
			command_button->setEnabled(enabled);
		}
	}

	return (command_button != NULL);
}

bool LLToolBar::stopCommandInProgress(const LLCommandId& commandId)
{
	//
	// Note from Leslie:
	//
	// This implementation was largely put in place to handle EXP-1348 which is related to
	// dragging and dropping the "speak" button.  The "speak" button can be in one of two
	// modes, i.e., either a toggle action or a push-to-talk action.  Because of this it
	// responds to mouse down and mouse up in different ways, based on which behavior the
	// button is currently set to obey.  This was the simplest way of getting the button
	// to turn off the microphone for both behaviors without risking duplicate state.
	//

	LLToolBarButton * command_button = NULL;

	if (commandId != LLCommandId::null)
	{
		LLCommand* command = LLCommandManager::instance().getCommand(commandId);
		llassert(command);

		// If this command has an explicit function for execution stop
		if (command->executeStopFunctionName().length() > 0)
		{
			command_id_map::iterator it = mButtonMap.find(commandId.uuid());
			if (it != mButtonMap.end())
			{
				command_button = it->second;
				llassert(command_button->mIsRunningSignal);

				// Check to see if it is running
				if ((*command_button->mIsRunningSignal)(command_button, command->isRunningParameters()))
				{
					// Trigger an additional button commit, which calls mouse down, mouse up and commit
					command_button->onCommit();
				}
			}
		}
	}

	return (command_button != NULL);
}

bool LLToolBar::flashCommand(const LLCommandId& commandId, bool flash)
{
	LLButton * command_button = NULL;

	if (commandId != LLCommandId::null)
	{
		command_id_map::iterator it = mButtonMap.find(commandId.uuid());
		if (it != mButtonMap.end())
		{
			command_button = it->second;
			command_button->setFlashing(flash ? TRUE : FALSE);
		}
	}

	return (command_button != NULL);
}

BOOL LLToolBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	LLRect button_panel_rect;
	mButtonPanel->localRectToOtherView(mButtonPanel->getLocalRect(), &button_panel_rect, this);
	BOOL handle_it_here = !mReadOnly && button_panel_rect.pointInRect(x, y);

	if (handle_it_here)
	{
		// Determine which button the mouse was over during the click in case the context menu action
		// is intended to affect the button.
		mRightMouseTargetButton = NULL;
		BOOST_FOREACH(LLToolBarButton* button, mButtons)
		{
			LLRect button_rect;
			button->localRectToOtherView(button->getLocalRect(), &button_rect, this);

			if (button_rect.pointInRect(x, y))
			{
				mRightMouseTargetButton = button;
				break;
			}
		}

		createContextMenu();

		LLContextMenu * menu = (LLContextMenu *) mPopupMenuHandle.get();

		if (menu)
		{
			menu->show(x, y);

			LLMenuGL::showPopup(this, menu, x, y);
		}
	}

	return handle_it_here;
}

BOOL LLToolBar::isSettingChecked(const LLSD& userdata)
{
	BOOL retval = FALSE;

	const std::string setting_name = userdata.asString();

	if (setting_name == "icons_with_text")
	{
		retval = (mButtonType == BTNTYPE_ICONS_WITH_TEXT);
	}
	else if (setting_name == "icons_only")
	{
		retval = (mButtonType == BTNTYPE_ICONS_ONLY);
	}

	return retval;
}

void LLToolBar::onSettingEnable(const LLSD& userdata)
{
	llassert(!mReadOnly);

	const std::string setting_name = userdata.asString();

	if (setting_name == "icons_with_text")
	{
		setButtonType(BTNTYPE_ICONS_WITH_TEXT);
	}
	else if (setting_name == "icons_only")
	{
		setButtonType(BTNTYPE_ICONS_ONLY);
	}
}

void LLToolBar::onRemoveSelectedCommand()
{
	llassert(!mReadOnly);

	if (mRightMouseTargetButton)
	{
		removeCommand(mRightMouseTargetButton->getCommandId());

		mRightMouseTargetButton = NULL;
	}
}

void LLToolBar::setButtonType(LLToolBarEnums::ButtonType button_type)
{
	bool regenerate_buttons = (mButtonType != button_type);
	
	mButtonType = button_type;

	if (regenerate_buttons)
	{
		createButtons();
	}
}

void LLToolBar::resizeButtonsInRow(std::vector<LLToolBarButton*>& buttons_in_row, S32 max_row_girth)
{
	// make buttons in current row all same girth
	BOOST_FOREACH(LLToolBarButton* button, buttons_in_row)
	{
		if (getOrientation(mSideType) == LLLayoutStack::HORIZONTAL)
		{
			button->reshape(button->mWidthRange.clamp(button->getRect().getWidth()), max_row_girth);
		}
		else // VERTICAL
		{
			button->reshape(max_row_girth, button->getRect().getHeight());
		}
	}
}

// Returns the position of the coordinates as a rank in the button list. 
// The rank is the position a tool dropped in (x,y) would assume in the button list.
// The returned value is between 0 and mButtons.size(), 0 being the first element to the left
// (or top) and mButtons.size() the last one to the right (or bottom).
// Various drag data are stored in the toolbar object though are not exposed outside (and shouldn't).
int LLToolBar::getRankFromPosition(S32 x, S32 y)
{
	if (mButtons.empty())
	{
		return RANK_NONE;
	}
	
	int rank = 0;

	// Convert the toolbar coord into button panel coords
	LLLayoutStack::ELayoutOrientation orientation = getOrientation(mSideType);
	S32 button_panel_x = 0;
	S32 button_panel_y = 0;
	localPointToOtherView(x, y, &button_panel_x, &button_panel_y, mButtonPanel);
	S32 dx = x - button_panel_x;
	S32 dy = y - button_panel_y;
	
	// Simply compare the passed coord with the buttons outbound box + padding
	std::list<LLToolBarButton*>::iterator it_button = mButtons.begin();
	std::list<LLToolBarButton*>::iterator end_button = mButtons.end();
	LLRect button_rect;
	while (it_button != end_button)
	{
		button_rect = (*it_button)->getRect();
		S32 point_x = button_rect.mRight + mPadRight;
		S32 point_y = button_rect.mBottom - mPadBottom;

		if ((button_panel_x < point_x) && (button_panel_y > point_y))
		{
			break;
		}
		rank++;
		++it_button;
	}
	
	// Update the passed coordinates to the hit button relevant corner 
	// (different depending on toolbar orientation)
	if (rank < mButtons.size())
	{
		if (orientation == LLLayoutStack::HORIZONTAL)
		{
			// Horizontal
			S32 mid_point = (button_rect.mRight + button_rect.mLeft) / 2;
			if (button_panel_x < mid_point)
			{
				mDragx = button_rect.mLeft - mPadLeft;
				mDragy = button_rect.mTop + mPadTop;
			}
			else
			{
				rank++;
				mDragx = button_rect.mRight + mPadRight - 1;
				mDragy = button_rect.mTop + mPadTop;
			}
		}
		else
		{
			// Vertical
			S32 mid_point = (button_rect.mTop + button_rect.mBottom) / 2;
			if (button_panel_y > mid_point)
			{
				mDragx = button_rect.mLeft - mPadLeft;
				mDragy = button_rect.mTop + mPadTop;
			}
			else
			{
				rank++;
				mDragx = button_rect.mLeft - mPadLeft;
				mDragy = button_rect.mBottom - mPadBottom + 1;
			}
		}
	}
	else
	{
		// We hit passed the end of the list so put the insertion point at the end
		if (orientation == LLLayoutStack::HORIZONTAL)
		{
			mDragx = button_rect.mRight + mPadRight;
			mDragy = button_rect.mTop + mPadTop;
		}
		else
		{
			mDragx = button_rect.mLeft - mPadLeft;
			mDragy = button_rect.mBottom - mPadBottom;
		}
	}

	// Update the "girth" of the caret, i.e. the width or height (depending of orientation)
	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		mDragGirth = button_rect.getHeight() + mPadBottom + mPadTop;
	}
	else
	{
		mDragGirth = button_rect.getWidth() + mPadLeft + mPadRight;
	}

	// The delta account for the coord model change (i.e. convert back to toolbar coord)
	mDragx += dx;
	mDragy += dy;
	
	return rank;
}

int LLToolBar::getRankFromPosition(const LLCommandId& id)
{
	if (!hasCommand(id))
	{
		return RANK_NONE;
	}
	int rank = 0;
	std::list<LLToolBarButton*>::iterator it_button = mButtons.begin();
	std::list<LLToolBarButton*>::iterator end_button = mButtons.end();
	while (it_button != end_button)
	{
		if ((*it_button)->mId == id)
		{
			break;
		}
		rank++;
		++it_button;
	}
	return rank;
}

void LLToolBar::updateLayoutAsNeeded()
{
	if (!mNeedsLayout) return;

	LLLayoutStack::ELayoutOrientation orientation = getOrientation(mSideType);
	
	// our terminology for orientation-agnostic layout is such that
	// length refers to a distance in the direction we stack the buttons 
	// and girth refers to a distance in the direction buttons wrap
	S32 max_row_girth = 0;
	S32 max_row_length = 0;

	S32 max_length;
	S32 cur_start;
	S32 cur_row ;
	S32 row_pad_start;
	S32 row_pad_end;
	S32 girth_pad_end;
	S32 row_running_length;

	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		max_length = getRect().getWidth() - mPadLeft - mPadRight;
		row_pad_start = mPadLeft;
		row_pad_end = mPadRight;
		cur_row = mPadTop;
		girth_pad_end = mPadBottom;
	}
	else // VERTICAL
	{
		max_length = getRect().getHeight() - mPadTop - mPadBottom;
		row_pad_start = mPadTop;
		row_pad_end = mPadBottom;
		cur_row = mPadLeft;
		girth_pad_end = mPadRight;
	}
	
	row_running_length = row_pad_start;
	cur_start = row_pad_start;


	LLRect panel_rect = mButtonPanel->getLocalRect();

	std::vector<LLToolBarButton*> buttons_in_row;

	BOOST_FOREACH(LLToolBarButton* button, mButtons)
	{
		button->reshape(button->mWidthRange.getMin(), button->mDesiredHeight);
		button->autoResize();

		S32 button_clamped_width = button->mWidthRange.clamp(button->getRect().getWidth());
		S32 button_length = (orientation == LLLayoutStack::HORIZONTAL)
							? button_clamped_width
							: button->getRect().getHeight();
		S32 button_girth = (orientation == LLLayoutStack::HORIZONTAL)
							? button->getRect().getHeight()
							: button_clamped_width;
		
		// wrap if needed
		if (mWrap
			&& row_running_length + button_length > max_length	// out of room...
			&& cur_start != row_pad_start)						// ...and not first button in row
		{
			if (orientation == LLLayoutStack::VERTICAL)
			{	// row girth (width in this case) is clamped to allowable button widths
				max_row_girth = button->mWidthRange.clamp(max_row_girth);
			}

			// make buttons in current row all same girth
			resizeButtonsInRow(buttons_in_row, max_row_girth);
			buttons_in_row.clear();

			max_row_length = llmax(max_row_length, row_running_length);
			row_running_length = row_pad_start;
			cur_start = row_pad_start;
			cur_row += max_row_girth + mPadBetween;
			max_row_girth = 0;
		}

		LLRect button_rect;
		if (orientation == LLLayoutStack::HORIZONTAL)
		{
			button_rect.setLeftTopAndSize(cur_start, panel_rect.mTop - cur_row, button_clamped_width, button->getRect().getHeight());
		}
		else // VERTICAL
		{
			button_rect.setLeftTopAndSize(cur_row, panel_rect.mTop - cur_start, button_clamped_width, button->getRect().getHeight());
		}
		button->setShape(button_rect);
		
		buttons_in_row.push_back(button);

		row_running_length += button_length + mPadBetween;
		cur_start = row_running_length;
		max_row_girth = llmax(button_girth, max_row_girth);
	}

	// final resizing in "girth" direction
	S32 total_girth =	cur_row				// current row position...
						+ max_row_girth		// ...incremented by size of final row...
						+ girth_pad_end;	// ...plus padding reserved on end
	total_girth = llmax(total_girth,mMinGirth);
	
	max_row_length = llmax(max_row_length, row_running_length - mPadBetween + row_pad_end);

	resizeButtonsInRow(buttons_in_row, max_row_girth);

	// grow and optionally shift toolbar to accommodate buttons
	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		if (mSideType == SIDE_TOP)
		{ // shift down to maintain top edge
			translate(0, getRect().getHeight() - total_girth);
		}

		reshape(getRect().getWidth(), total_girth);
		mButtonPanel->reshape(max_row_length, total_girth);
	}
	else // VERTICAL
	{
		if (mSideType == SIDE_RIGHT)
		{ // shift left to maintain right edge
			translate(getRect().getWidth() - total_girth, 0);
		}
		
		reshape(total_girth, getRect().getHeight());
		mButtonPanel->reshape(total_girth, max_row_length);
	}

	// make parent fit button panel
	mButtonPanel->getParent()->setShape(mButtonPanel->getLocalRect());

	// re-center toolbar buttons
	mCenteringStack->updateLayout();

	if (!mButtons.empty())
	{
		mButtonPanel->setVisible(TRUE);
		mButtonPanel->setMouseOpaque(TRUE);
	}

	// don't clear flag until after we've resized ourselves, to avoid laying out every frame
	mNeedsLayout = false;
}


void LLToolBar::draw()
{
	if (mButtons.empty())
	{
		mButtonPanel->setVisible(FALSE);
		mButtonPanel->setMouseOpaque(FALSE);
	}
	else
	{
		mButtonPanel->setVisible(TRUE);
		mButtonPanel->setMouseOpaque(TRUE);
	}

	// Update enable/disable state and highlight state for editable toolbars
	if (!mReadOnly)
	{
		for (toolbar_button_list::iterator btn_it = mButtons.begin(); btn_it != mButtons.end(); ++btn_it)
		{
			LLToolBarButton* btn = *btn_it;
			LLCommand* command = LLCommandManager::instance().getCommand(btn->mId);

			if (command && btn->mIsEnabledSignal)
			{
				const bool button_command_enabled = (*btn->mIsEnabledSignal)(btn, command->isEnabledParameters());
				btn->setEnabled(button_command_enabled);
			}

			if (command && btn->mIsRunningSignal)
			{
				const bool button_command_running = (*btn->mIsRunningSignal)(btn, command->isRunningParameters());
				btn->setToggleState(button_command_running);
			}
		}
	}

	updateLayoutAsNeeded();
	// rect may have shifted during layout
	LLUI::popMatrix();
	LLUI::pushMatrix();
	LLUI::translate((F32)getRect().mLeft, (F32)getRect().mBottom);

	// Position the caret 
	LLIconCtrl* caret = getChild<LLIconCtrl>("caret");
	caret->setVisible(FALSE);
	if (mDragAndDropTarget && !mButtonCommands.empty())
	{
		LLRect caret_rect = caret->getRect();
		if (getOrientation(mSideType) == LLLayoutStack::HORIZONTAL)
		{
			caret->setRect(LLRect(mDragx-caret_rect.getWidth()/2+1,
								  mDragy,
								  mDragx+caret_rect.getWidth()/2+1,
								  mDragy-mDragGirth));
		}
		else
		{
			caret->setRect(LLRect(mDragx,
								  mDragy+caret_rect.getHeight()/2,
								  mDragx+mDragGirth,
								  mDragy-caret_rect.getHeight()/2));
		}
		caret->setVisible(TRUE);
	}
		
	LLUICtrl::draw();
	caret->setVisible(FALSE);
	mDragAndDropTarget = false;
}

void LLToolBar::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLUICtrl::reshape(width, height, called_from_parent);
	mNeedsLayout = true;
}

void LLToolBar::createButtons()
{
	BOOST_FOREACH(LLToolBarButton* button, mButtons)
	{
		if (mButtonRemoveSignal)
		{
			(*mButtonRemoveSignal)(button);
		}
		
		delete button;
	}
	mButtons.clear();
	mButtonMap.clear();
	mRightMouseTargetButton = NULL;
	
	BOOST_FOREACH(LLCommandId& command_id, mButtonCommands)
	{
		LLToolBarButton* button = createButton(command_id);
		mButtons.push_back(button);
		mButtonPanel->addChild(button);
		mButtonMap.insert(std::make_pair(command_id.uuid(), button));
		
		if (mButtonAddSignal)
		{
			(*mButtonAddSignal)(button);
		}
	}
	mNeedsLayout = true;
}

void LLToolBarButton::callIfEnabled(LLUICtrl::commit_callback_t commit, LLUICtrl* ctrl, const LLSD& param )
{
	LLCommand* command = LLCommandManager::instance().getCommand(mId);

	if (!mIsEnabledSignal || (*mIsEnabledSignal)(this, command->isEnabledParameters()))
	{
		commit(ctrl, param);
	}
}

LLToolBarButton* LLToolBar::createButton(const LLCommandId& id)
{
	LLCommand* commandp = LLCommandManager::instance().getCommand(id);
	if (!commandp) return NULL;

	LLToolBarButton::Params button_p;
	button_p.name = commandp->name();
	button_p.label = LLTrans::getString(commandp->labelRef());
	button_p.tool_tip = LLTrans::getString(commandp->tooltipRef());
	button_p.image_overlay = LLUI::getUIImage(commandp->icon());
	button_p.overwriteFrom(mButtonParams[mButtonType]);
	LLToolBarButton* button = LLUICtrlFactory::create<LLToolBarButton>(button_p);

	if (!mReadOnly)
	{
		enable_callback_t isEnabledCB;

		const std::string& isEnabledFunction = commandp->isEnabledFunctionName();
		if (isEnabledFunction.length() > 0)
		{
			LLUICtrl::EnableCallbackParam isEnabledParam;
			isEnabledParam.function_name = isEnabledFunction;
			isEnabledParam.parameter = commandp->isEnabledParameters();
			isEnabledCB = initEnableCallback(isEnabledParam);

			if (NULL == button->mIsEnabledSignal)
			{
				button->mIsEnabledSignal = new enable_signal_t();
			}

			button->mIsEnabledSignal->connect(isEnabledCB);
		}

		LLUICtrl::CommitCallbackParam executeParam;
		executeParam.function_name = commandp->executeFunctionName();
		executeParam.parameter = commandp->executeParameters();

		// If we have a "stop" function then we map the command to mouse down / mouse up otherwise commit
		const std::string& executeStopFunction = commandp->executeStopFunctionName();
		if (executeStopFunction.length() > 0)
		{
			LLUICtrl::CommitCallbackParam executeStopParam;
			executeStopParam.function_name = executeStopFunction;
			executeStopParam.parameter = commandp->executeStopParameters();
			LLUICtrl::commit_callback_t execute_func = initCommitCallback(executeParam);
			LLUICtrl::commit_callback_t stop_func = initCommitCallback(executeStopParam);
			
			button->setMouseDownCallback(boost::bind(&LLToolBarButton::callIfEnabled, button, execute_func, _1, _2));
			button->setMouseUpCallback(boost::bind(&LLToolBarButton::callIfEnabled, button, stop_func, _1, _2));
		}
		else
		{
			button->setCommitCallback(executeParam);
		}

		// Set up "is running" query callback
		const std::string& isRunningFunction = commandp->isRunningFunctionName();
		if (isRunningFunction.length() > 0)
		{
			LLUICtrl::EnableCallbackParam isRunningParam;
			isRunningParam.function_name = isRunningFunction;
			isRunningParam.parameter = commandp->isRunningParameters();
			enable_signal_t::slot_type isRunningCB = initEnableCallback(isRunningParam);

			if (NULL == button->mIsRunningSignal)
			{
				button->mIsRunningSignal = new enable_signal_t();
			}

			button->mIsRunningSignal->connect(isRunningCB);
		}
	}

	// Drag and drop behavior must work also if provided in the Toybox and, potentially, any read-only toolbar
	button->setStartDragCallback(mStartDragItemCallback);
	button->setHandleDragCallback(mHandleDragItemCallback);

	button->setCommandId(id);

	return button;
}

boost::signals2::connection connectSignal(LLToolBar::button_signal_t*& signal, const LLToolBar::button_signal_t::slot_type& cb)
{
	if (!signal)
	{
		signal = new LLToolBar::button_signal_t();
	}

	return signal->connect(cb);
}

boost::signals2::connection LLToolBar::setButtonAddCallback(const button_signal_t::slot_type& cb)
{
	return connectSignal(mButtonAddSignal, cb);
}

boost::signals2::connection LLToolBar::setButtonEnterCallback(const button_signal_t::slot_type& cb)
{
	return connectSignal(mButtonEnterSignal, cb);
}

boost::signals2::connection LLToolBar::setButtonLeaveCallback(const button_signal_t::slot_type& cb)
{
	return connectSignal(mButtonLeaveSignal, cb);
}

boost::signals2::connection LLToolBar::setButtonRemoveCallback(const button_signal_t::slot_type& cb)
{
	return connectSignal(mButtonRemoveSignal, cb);
}

BOOL LLToolBar::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
										EDragAndDropType cargo_type,
										void* cargo_data,
										EAcceptance* accept,
										std::string& tooltip_msg)
{
	// If we have a drop callback, that means that we can handle the drop
	BOOL handled = (mHandleDropCallback ? TRUE : FALSE);
	
	// if drop is set, it's time to call the callback to get the operation done
	if (handled && drop)
	{
		handled = mHandleDropCallback(cargo_data, x, y ,this);
	}
	
	// We accept only single tool drop on toolbars
	*accept = (handled ? ACCEPT_YES_SINGLE : ACCEPT_NO);
	
	// We'll use that flag to change the visual aspect of the toolbar target on draw()
	mDragAndDropTarget = false;
	
	// Convert drag position into insert position and rank 
	if (!isReadOnly() && handled && !drop)
	{
		LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
		LLAssetType::EType type = inv_item->getType();
		if (type == LLAssetType::AT_WIDGET)
		{
			LLCommandId dragged_command(inv_item->getUUID());
			int orig_rank = getRankFromPosition(dragged_command);
			mDragRank = getRankFromPosition(x, y);
			// Don't DaD if we're dragging a command on itself
			mDragAndDropTarget = ((orig_rank != RANK_NONE) && ((mDragRank == orig_rank) || ((mDragRank-1) == orig_rank)) ? false : true);
			//llinfos << "Merov debug : DaD, rank = " << mDragRank << ", dragged uui = " << inv_item->getUUID() << llendl; 
			/* Do the following if you want to animate the button itself
			LLCommandId dragged_command(inv_item->getUUID());
			removeCommand(dragged_command);
			addCommand(dragged_command,rank);
			*/
		}
		else
		{
			handled = FALSE;
		}
	}
	
	return handled;
}

LLToolBarButton::LLToolBarButton(const Params& p) 
:	LLButton(p),
	mMouseDownX(0),
	mMouseDownY(0),
	mWidthRange(p.button_width),
	mDesiredHeight(p.desired_height),
	mId(""),
	mIsEnabledSignal(NULL),
	mIsRunningSignal(NULL),
	mIsStartingSignal(NULL),
	mIsDragged(false),
	mStartDragItemCallback(NULL),
	mHandleDragItemCallback(NULL),
	mOriginalImageSelected(p.image_selected),
	mOriginalImageUnselected(p.image_unselected),
	mOriginalImagePressed(p.image_pressed),
	mOriginalImagePressedSelected(p.image_pressed_selected),
	mOriginalLabelColor(p.label_color),
	mOriginalLabelColorSelected(p.label_color_selected),
	mOriginalImageOverlayColor(p.image_overlay_color),
	mOriginalImageOverlaySelectedColor(p.image_overlay_selected_color)
{
}

LLToolBarButton::~LLToolBarButton()
{
	delete mIsEnabledSignal;
	delete mIsRunningSignal;
	delete mIsStartingSignal;
}

BOOL LLToolBarButton::handleMouseDown(S32 x, S32 y, MASK mask)
{
	mMouseDownX = x;
	mMouseDownY = y;
	return LLButton::handleMouseDown(x, y, mask);
}

BOOL LLToolBarButton::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;
		
	S32 mouse_distance_squared = (x - mMouseDownX) * (x - mMouseDownX) + (y - mMouseDownY) * (y - mMouseDownY);
	S32 drag_threshold = LLUI::sSettingGroups["config"]->getS32("DragAndDropDistanceThreshold");
	if (mouse_distance_squared > drag_threshold * drag_threshold
		&& hasMouseCapture() && 
		mStartDragItemCallback && mHandleDragItemCallback)
	{
		if (!mIsDragged)
		{
			mStartDragItemCallback(x, y, this);
			mIsDragged = true;
			handled = TRUE;
		}
		else 
		{
			handled = mHandleDragItemCallback(x, y, mId.uuid(), LLAssetType::AT_WIDGET);
		}
	}
	else
	{
		handled = LLButton::handleHover(x, y, mask);
	}

	return handled;
}

void LLToolBarButton::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLUICtrl::onMouseEnter(x, y, mask);

	// Always highlight toolbar buttons, even if they are disabled
	if (!gFocusMgr.getMouseCapture() || gFocusMgr.getMouseCapture() == this)
	{
		mNeedsHighlight = TRUE;
	}

	LLToolBar* parent_toolbar = getParentByType<LLToolBar>();
	if (parent_toolbar && parent_toolbar->mButtonEnterSignal)
	{
		(*(parent_toolbar->mButtonEnterSignal))(this);
	}
}

void LLToolBarButton::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLButton::onMouseLeave(x, y, mask);
	
	LLToolBar* parent_toolbar = getParentByType<LLToolBar>();
	if (parent_toolbar && parent_toolbar->mButtonLeaveSignal)
	{
		(*(parent_toolbar->mButtonLeaveSignal))(this);
	}	
}

void LLToolBarButton::onMouseCaptureLost()
{
	mIsDragged = false;
}

void LLToolBarButton::onCommit()
{
	LLCommand* command = LLCommandManager::instance().getCommand(mId);

	if (!mIsEnabledSignal || (*mIsEnabledSignal)(this, command->isEnabledParameters()))
	{
		LLButton::onCommit();
	}
}

void LLToolBarButton::reshape(S32 width, S32 height, BOOL called_from_parent)
{
	LLButton::reshape(mWidthRange.clamp(width), height, called_from_parent);
}

void LLToolBarButton::setEnabled(BOOL enabled)
{
	if (enabled)
	{
		mImageSelected = mOriginalImageSelected;
		mImageUnselected = mOriginalImageUnselected;
		mImagePressed = mOriginalImagePressed;
		mImagePressedSelected = mOriginalImagePressedSelected;
		mUnselectedLabelColor = mOriginalLabelColor;
		mSelectedLabelColor = mOriginalLabelColorSelected;
		mImageOverlayColor = mOriginalImageOverlayColor;
		mImageOverlaySelectedColor = mOriginalImageOverlaySelectedColor;
	}
	else
	{
		mImageSelected = mImageDisabledSelected;
		mImageUnselected = mImageDisabled;
		mImagePressed = mImageDisabled;
		mImagePressedSelected = mImageDisabledSelected;
		mUnselectedLabelColor = mDisabledLabelColor;
		mSelectedLabelColor = mDisabledSelectedLabelColor;
		mImageOverlayColor = mImageOverlayDisabledColor;
		mImageOverlaySelectedColor = mImageOverlayDisabledColor;
	}
}

const std::string LLToolBarButton::getToolTip() const	
{ 
	std::string tooltip;

	if (labelIsTruncated() || getCurrentLabel().empty())
	{
		tooltip = LLTrans::getString(LLCommandManager::instance().getCommand(mId)->labelRef()) + " -- " + LLView::getToolTip();
	}
	else
	{
		tooltip = LLView::getToolTip();
	}

	LLToolBar* parent_toolbar = getParentByType<LLToolBar>();
	if (parent_toolbar && parent_toolbar->mButtonTooltipSuffix.length() > 0)
	{
		tooltip = tooltip + "\n(" + parent_toolbar->mButtonTooltipSuffix + ")";
	}

	return tooltip;
}

