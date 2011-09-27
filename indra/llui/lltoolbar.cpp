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

#include "llmenugl.h"
#include "lltrans.h"

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
	min_button_width("min_button_width", 0),
	max_button_width("max_button_width", S32_MAX),
	button_height("button_height"),
	pad_left("pad_left"),
	pad_top("pad_top"),
	pad_right("pad_right"),
	pad_bottom("pad_bottom"),
	pad_between("pad_between"),
	button_panel("button_panel")
{}

LLToolBar::LLToolBar(const LLToolBar::Params& p)
:	LLUICtrl(p),
	mReadOnly(p.read_only),
	mButtonType(p.button_display_mode),
	mSideType(p.side),
	mWrap(p.wrap),
	mNeedsLayout(false),
	mButtonPanel(NULL),
	mCenteringStack(NULL),
	mMinButtonWidth(llmin(p.min_button_width(), p.max_button_width())),
	mMaxButtonWidth(llmax(p.max_button_width(), p.min_button_width())),
	mButtonHeight(p.button_height),
	mPadLeft(p.pad_left),
	mPadRight(p.pad_right),
	mPadTop(p.pad_top),
	mPadBottom(p.pad_bottom),
	mPadBetween(p.pad_between),
	mPopupMenuHandle()
{
	mButtonParams[LLToolBarEnums::BTNTYPE_ICONS_WITH_TEXT] = p.button_icon_and_text;
	mButtonParams[LLToolBarEnums::BTNTYPE_ICONS_ONLY] = p.button_icon;
}

LLToolBar::~LLToolBar()
{
	delete mPopupMenuHandle.get();
}

void LLToolBar::createContextMenu()
{
	if (!mPopupMenuHandle.get())
	{
		LLUICtrl::CommitCallbackRegistry::Registrar& commit_reg = LLUICtrl::CommitCallbackRegistry::defaultRegistrar();
		commit_reg.add("Toolbars.EnableSetting", boost::bind(&LLToolBar::onSettingEnable, this, _2));

		LLUICtrl::EnableCallbackRegistry::Registrar& enable_reg = LLUICtrl::EnableCallbackRegistry::defaultRegistrar();
		enable_reg.add("Toolbars.CheckSetting", boost::bind(&LLToolBar::isSettingChecked, this, _2));

		LLContextMenu* menu = LLUICtrlFactory::instance().createFromFile<LLContextMenu>("menu_toolbars.xml", LLMenuGL::sMenuContainer, LLMenuHolderGL::child_registry_t::instance());

		if (menu)
		{
			menu->setBackgroundColor(LLUIColorTable::instance().getColor("MenuPopupBgColor"));

			mPopupMenuHandle = menu->getHandle();
		}
		else
		{
			llwarns << "Unable to load toolbars context menu." << llendl;
		}
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
	center_panel_p.fit_content = true;
	LLLayoutPanel* center_panel = LLUICtrlFactory::create<LLLayoutPanel>(center_panel_p);
	mCenteringStack->addChild(center_panel);

	LLPanel::Params button_panel_p(p.button_panel);
	button_panel_p.rect = center_panel->getLocalRect();
		button_panel_p.follows.flags = FOLLOWS_BOTTOM|FOLLOWS_LEFT;
	mButtonPanel = LLUICtrlFactory::create<LLPanel>(button_panel_p);
	center_panel->addChild(mButtonPanel);
	
	mCenteringStack->addChild(LLUICtrlFactory::create<LLLayoutPanel>(border_panel_p));

	mNeedsLayout = true;
}

bool LLToolBar::addCommand(const LLCommandId& commandId)
{
	LLCommand * command = LLCommandManager::instance().getCommand(commandId);

	bool add_command = (command != NULL);

	if (add_command)
	{
		mButtonCommands.push_back(commandId);
		createButtons();
	}

	return add_command;
}

bool LLToolBar::hasCommand(const LLCommandId& commandId) const
{
	bool has_command = false;

	if (commandId != LLCommandId::null)
	{
		for (std::list<LLCommandId>::const_iterator cmd = mButtonCommands.begin(); cmd != mButtonCommands.end(); ++cmd)
		{
			if ((*cmd) == commandId)
			{
				has_command = true;
				break;
			}
		}
	}

	return has_command;
}

bool LLToolBar::enableCommand(const LLCommandId& commandId, bool enabled)
{
	LLButton * command_button = NULL;
	
	if (commandId != LLCommandId::null)
	{
		command_button = mButtonPanel->findChild<LLButton>(commandId.name());

		if (command_button)
		{
			command_button->setEnabled(enabled);
		}
	}

	return (command_button != NULL);
}

BOOL LLToolBar::handleRightMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL handle_it_here = !mReadOnly;

	if (handle_it_here)
	{
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

	if (setting_name == "icons_and_labels")
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

	const ButtonType current_button_type = mButtonType;

	if (setting_name == "icons_and_labels")
	{
		mButtonType = BTNTYPE_ICONS_WITH_TEXT;
	}
	else if (setting_name == "icons_only")
	{
		mButtonType = BTNTYPE_ICONS_ONLY;
	}

	if(current_button_type != mButtonType)
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
			button->reshape(llclamp(button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth), max_row_girth);
		}
		else // VERTICAL
		{
			button->reshape(max_row_girth, button->getRect().getHeight());
		}
	}
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
	S32 max_total_girth;
	S32 cur_start;
	S32 cur_row ;
	S32 row_pad_start;
	S32 row_pad_end;
	S32 girth_pad_end;
	S32 row_running_length;

	if (orientation == LLLayoutStack::HORIZONTAL)
	{
		max_length = getRect().getWidth() - mPadLeft - mPadRight;
		max_total_girth = getRect().getHeight() - mPadTop - mPadBottom;
		row_pad_start = mPadLeft;
		row_pad_end = mPadRight;
		cur_row = mPadTop;
		girth_pad_end = mPadBottom;
	}
	else // VERTICAL
	{
		max_length = getRect().getHeight() - mPadTop - mPadBottom;
		max_total_girth = getRect().getWidth() - mPadLeft - mPadRight;
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
		button->reshape(mMinButtonWidth, mButtonHeight);
		button->autoResize();

		S32 button_clamped_width = llclamp(button->getRect().getWidth(), mMinButtonWidth, mMaxButtonWidth);
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
				max_row_girth = llclamp(max_row_girth, mMinButtonWidth, mMaxButtonWidth);
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

	// re-center toolbar buttons
	mCenteringStack->updateLayout();

	// don't clear flag until after we've resized ourselves, to avoid laying out every frame
	mNeedsLayout = false;
}


void LLToolBar::draw()
{
	updateLayoutAsNeeded();
	LLUICtrl::draw();
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
		delete button;
	}
	mButtons.clear();
	
	BOOST_FOREACH(LLCommandId& command_id, mButtonCommands)
	{
		LLCommand* commandp = LLCommandManager::instance().getCommand(command_id);
		if (!commandp) continue;

		LLToolBarButton::Params button_p;
		button_p.label = LLTrans::getString(commandp->labelRef());
		button_p.image_overlay = LLUI::getUIImage(commandp->icon());
		button_p.overwriteFrom(mButtonParams[mButtonType]);
		LLToolBarButton* button = LLUICtrlFactory::create<LLToolBarButton>(button_p);

		mButtons.push_back(button);
		mButtonPanel->addChild(button);
	}

	mNeedsLayout = true;
}

