/** 
 * @file llcombobox.cpp
 * @brief LLComboBox base class
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

// A control that displays the name of the chosen item, which when
// clicked shows a scrolling box of options.

#include "linden_common.h"

// file includes
#include "llcombobox.h"

// common includes
#include "llstring.h"

// newview includes
#include "llbutton.h"
#include "llkeyboard.h"
#include "llscrolllistctrl.h"
#include "llwindow.h"
#include "llfloater.h"
#include "llscrollbar.h"
#include "llscrolllistcell.h"
#include "llscrolllistitem.h"
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "v2math.h"
#include "lluictrlfactory.h"
#include "lltooltip.h"

// Globals
S32 MAX_COMBO_WIDTH = 500;

static LLDefaultChildRegistry::Register<LLComboBox> register_combo_box("combo_box");

void LLComboBox::PreferredPositionValues::declareValues()
{
	declare("above", ABOVE);
	declare("below", BELOW);
}

LLComboBox::ItemParams::ItemParams()
:	label("label")
{
}


LLComboBox::Params::Params()
:	allow_text_entry("allow_text_entry", false),
	allow_new_values("allow_new_values", false),
	show_text_as_tentative("show_text_as_tentative", true),
	max_chars("max_chars", 20),
	list_position("list_position", BELOW),
	items("item"),
	combo_button("combo_button"),
	combo_list("combo_list"),
	combo_editor("combo_editor"),
	drop_down_button("drop_down_button")
{
	addSynonym(items, "combo_item");
}


LLComboBox::LLComboBox(const LLComboBox::Params& p)
:	LLUICtrl(p),
	mTextEntry(NULL),
	mTextEntryTentative(p.show_text_as_tentative),
	mHasAutocompletedText(false),
	mAllowTextEntry(p.allow_text_entry),
	mAllowNewValues(p.allow_new_values),
	mMaxChars(p.max_chars),
	mPrearrangeCallback(p.prearrange_callback()),
	mTextEntryCallback(p.text_entry_callback()),
	mTextChangedCallback(p.text_changed_callback()),
	mListPosition(p.list_position),
	mLastSelectedIndex(-1),
	mLabel(p.label)
{
	// Text label button

	LLButton::Params button_params = (mAllowTextEntry ? p.combo_button : p.drop_down_button);
	button_params.mouse_down_callback.function(
		boost::bind(&LLComboBox::onButtonMouseDown, this));
	button_params.follows.flags(FOLLOWS_LEFT|FOLLOWS_BOTTOM|FOLLOWS_RIGHT);
	button_params.rect(p.rect);

	if(mAllowTextEntry)
	{
		button_params.pad_right(2);
	}

	mArrowImage = button_params.image_unselected;

	mButton = LLUICtrlFactory::create<LLButton>(button_params);

	
	if(mAllowTextEntry)
	{
		//redo to compensate for button hack that leaves space for a character
		//unless it is a "minimal combobox"(drop down)
		mButton->setRightHPad(2);
	}
	addChild(mButton);

	LLScrollListCtrl::Params params = p.combo_list;
	params.name("ComboBox");
	params.commit_callback.function(boost::bind(&LLComboBox::onItemSelected, this, _2));
	params.visible(false);
	params.commit_on_keyboard_movement(false);

	mList = LLUICtrlFactory::create<LLScrollListCtrl>(params);
	addChild(mList);

	// Mouse-down on button will transfer mouse focus to the list
	// Grab the mouse-up event and make sure the button state is correct
	mList->setMouseUpCallback(boost::bind(&LLComboBox::onListMouseUp, this));

	for (LLInitParam::ParamIterator<ItemParams>::const_iterator it = p.items.begin();
		it != p.items.end();
		++it)
	{
		LLScrollListItem::Params item_params = *it;
		if (it->label.isProvided())
		{
			item_params.columns.add().value(it->label());
		}

		mList->addRow(item_params);
	}

	createLineEditor(p);

	mTopLostSignalConnection = setTopLostCallback(boost::bind(&LLComboBox::hideList, this));
}

void LLComboBox::initFromParams(const LLComboBox::Params& p)
{
	LLUICtrl::initFromParams(p);

	if (!acceptsTextInput() && mLabel.empty())
	{
		selectFirstItem();
	}
}

// virtual
BOOL LLComboBox::postBuild()
{
	if (mControlVariable)
	{
		setValue(mControlVariable->getValue()); // selects the appropriate item
	}
	return TRUE;
}


LLComboBox::~LLComboBox()
{
	// children automatically deleted, including mMenu, mButton

	// explicitly disconect this signal, since base class destructor might fire top lost
	mTopLostSignalConnection.disconnect();
}


void LLComboBox::clear()
{ 
	if (mTextEntry)
	{
		mTextEntry->setText(LLStringUtil::null);
	}
	mButton->setLabelSelected(LLStringUtil::null);
	mButton->setLabelUnselected(LLStringUtil::null);
	mList->deselectAllItems();
	mLastSelectedIndex = -1;
}

void LLComboBox::onCommit()
{
	if (mAllowTextEntry && getCurrentIndex() != -1)
	{
		// we have selected an existing item, blitz the manual text entry with
		// the properly capitalized item
		mTextEntry->setValue(getSimple());
		mTextEntry->setTentative(FALSE);
	}
	setControlValue(getValue());
	LLUICtrl::onCommit();
}

// virtual
BOOL LLComboBox::isDirty() const
{
	BOOL grubby = FALSE;
	if ( mList )
	{
		grubby = mList->isDirty();
	}
	return grubby;
}

// virtual   Clear dirty state
void	LLComboBox::resetDirty()
{
	if ( mList )
	{
		mList->resetDirty();
	}
}

bool LLComboBox::itemExists(const std::string& name)
{
	return mList->getItemByLabel(name);
}

// add item "name" to menu
LLScrollListItem* LLComboBox::add(const std::string& name, EAddPosition pos, BOOL enabled)
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos);
	item->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

// add item "name" with a unique id to menu
LLScrollListItem* LLComboBox::add(const std::string& name, const LLUUID& id, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos, id);
	item->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

// add item "name" with attached userdata
LLScrollListItem* LLComboBox::add(const std::string& name, void* userdata, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos);
	item->setEnabled(enabled);
	item->setUserdata( userdata );
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

// add item "name" with attached generic data
LLScrollListItem* LLComboBox::add(const std::string& name, LLSD value, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos, value);
	item->setEnabled(enabled);
	if (!mAllowTextEntry && mLabel.empty())
	{
		selectFirstItem();
	}
	return item;
}

LLScrollListItem* LLComboBox::addSeparator(EAddPosition pos)
{
	return mList->addSeparator(pos);
}

void LLComboBox::sortByName(BOOL ascending)
{
	mList->sortOnce(0, ascending);
}


// Choose an item with a given name in the menu.
// Returns TRUE if the item was found.
BOOL LLComboBox::setSimple(const LLStringExplicit& name)
{
	BOOL found = mList->selectItemByLabel(name, FALSE);

	if (found)
	{
		setLabel(name);
		mLastSelectedIndex = mList->getFirstSelectedIndex();
	}

	return found;
}

// virtual
void LLComboBox::setValue(const LLSD& value)
{
	BOOL found = mList->selectByValue(value);
	if (found)
	{
		LLScrollListItem* item = mList->getFirstSelected();
		if (item)
		{
			updateLabel();
		}
		mLastSelectedIndex = mList->getFirstSelectedIndex();
	}
	else
	{
		mLastSelectedIndex = -1;
	}
}

const std::string LLComboBox::getSimple() const
{
	const std::string res = getSelectedItemLabel();
	if (res.empty() && mAllowTextEntry)
	{
		return mTextEntry->getText();
	}
	else
	{
		return res;
	}
}

const std::string LLComboBox::getSelectedItemLabel(S32 column) const
{
	return mList->getSelectedItemLabel(column);
}

// virtual
LLSD LLComboBox::getValue() const
{
	LLScrollListItem* item = mList->getFirstSelected();
	if( item )
	{
		return item->getValue();
	}
	else if (mAllowTextEntry)
	{
		return mTextEntry->getValue();
	}
	else
	{
		return LLSD();
	}
}

void LLComboBox::setLabel(const LLStringExplicit& name)
{
	if ( mTextEntry )
	{
		mTextEntry->setText(name);
		if (mList->selectItemByLabel(name, FALSE))
		{
			mTextEntry->setTentative(FALSE);
			mLastSelectedIndex = mList->getFirstSelectedIndex();
		}
		else
		{
			mTextEntry->setTentative(mTextEntryTentative);
		}
	}
	
	if (!mAllowTextEntry)
	{
		mButton->setLabel(name);
	}
}

void LLComboBox::updateLabel()
{
	// Update the combo editor with the selected
	// item label.
	if (mTextEntry)
	{
		mTextEntry->setText(getSelectedItemLabel());
		mTextEntry->setTentative(FALSE);
	}

	// If combo box doesn't allow text entry update
	// the combo button label.
	if (!mAllowTextEntry)
	{
		mButton->setLabel(getSelectedItemLabel());
	}
}

BOOL LLComboBox::remove(const std::string& name)
{
	BOOL found = mList->selectItemByLabel(name);

	if (found)
	{
		LLScrollListItem* item = mList->getFirstSelected();
		if (item)
		{
			mList->deleteSingleItem(mList->getItemIndex(item));
		}
		mLastSelectedIndex = mList->getFirstSelectedIndex();
	}

	return found;
}

BOOL LLComboBox::remove(S32 index)
{
	if (index < mList->getItemCount())
	{
		mList->deleteSingleItem(index);
		setLabel(getSelectedItemLabel());
		return TRUE;
	}
	return FALSE;
}

// Keyboard focus lost.
void LLComboBox::onFocusLost()
{
	hideList();
	// if valid selection
	if (mAllowTextEntry && getCurrentIndex() != -1)
	{
		mTextEntry->selectAll();
	}
	LLUICtrl::onFocusLost();
}

void LLComboBox::setButtonVisible(BOOL visible)
{
	static LLUICachedControl<S32> drop_shadow_button ("DropShadowButton", 0);

	mButton->setVisible(visible);
	if (mTextEntry)
	{
		LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
		if (visible)
		{
			S32 arrow_width = mArrowImage ? mArrowImage->getWidth() : 0;
			text_entry_rect.mRight -= llmax(8,arrow_width) + 2 * drop_shadow_button;
		}
		//mTextEntry->setRect(text_entry_rect);
		mTextEntry->reshape(text_entry_rect.getWidth(), text_entry_rect.getHeight(), TRUE);
	}
}

BOOL LLComboBox::setCurrentByIndex( S32 index )
{
	BOOL found = mList->selectNthItem( index );
	if (found)
	{
		setLabel(getSelectedItemLabel());
		mLastSelectedIndex = index;
	}
	return found;
}

S32 LLComboBox::getCurrentIndex() const
{
	LLScrollListItem* item = mList->getFirstSelected();
	if( item )
	{
		return mList->getItemIndex( item );
	}
	return -1;
}


void LLComboBox::createLineEditor(const LLComboBox::Params& p)
{
	static LLUICachedControl<S32> drop_shadow_button ("DropShadowButton", 0);
	LLRect rect = getLocalRect();
	if (mAllowTextEntry)
	{
		S32 arrow_width = mArrowImage ? mArrowImage->getWidth() : 0;
		S32 shadow_size = drop_shadow_button;
		mButton->setRect(LLRect( getRect().getWidth() - llmax(8,arrow_width) - 2 * shadow_size,
								rect.mTop, rect.mRight, rect.mBottom));
		mButton->setTabStop(FALSE);
		mButton->setHAlign(LLFontGL::HCENTER);

		LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
		text_entry_rect.mRight -= llmax(8,arrow_width) + 2 * drop_shadow_button;
		// clear label on button
		std::string cur_label = mButton->getLabelSelected();
		LLLineEditor::Params params = p.combo_editor;
		params.rect(text_entry_rect);
		params.default_text(LLStringUtil::null);
		params.max_length.bytes(mMaxChars);
		params.commit_callback.function(boost::bind(&LLComboBox::onTextCommit, this, _2));
		params.keystroke_callback(boost::bind(&LLComboBox::onTextEntry, this, _1));
		params.commit_on_focus_lost(false);
		params.follows.flags(FOLLOWS_ALL);
		params.label(mLabel);
		mTextEntry = LLUICtrlFactory::create<LLLineEditor> (params);
		mTextEntry->setText(cur_label);
		mTextEntry->setIgnoreTab(TRUE);
		addChild(mTextEntry);

		// clear label on button
		setLabel(LLStringUtil::null);

		mButton->setFollows(FOLLOWS_BOTTOM | FOLLOWS_TOP | FOLLOWS_RIGHT);
	}
	else
	{
		mButton->setRect(rect);
		mButton->setLabel(mLabel.getString());
		
		if (mTextEntry)
		{
			mTextEntry->setVisible(FALSE);
		}
	}
}

void* LLComboBox::getCurrentUserdata()
{
	LLScrollListItem* item = mList->getFirstSelected();
	if( item )
	{
		return item->getUserdata();
	}
	return NULL;
}


void LLComboBox::showList()
{
	// Make sure we don't go off top of screen.
	LLCoordWindow window_size;
	getWindow()->getSize(&window_size);
	//HACK: shouldn't have to know about scale here
	mList->fitContents( 192, llfloor((F32)window_size.mY / LLUI::getScaleFactor().mV[VY]) - 50 );

	// Make sure that we can see the whole list
	LLRect root_view_local;
	LLView* root_view = getRootView();
	root_view->localRectToOtherView(root_view->getLocalRect(), &root_view_local, this);
	
	LLRect rect = mList->getRect();

	S32 min_width = getRect().getWidth();
	S32 max_width = llmax(min_width, MAX_COMBO_WIDTH);
	// make sure we have up to date content width metrics
	S32 list_width = llclamp(mList->calcMaxContentWidth(), min_width, max_width);

	if (mListPosition == BELOW)
	{
		if (rect.getHeight() <= -root_view_local.mBottom)
		{
			// Move rect so it hangs off the bottom of this view
			rect.setLeftTopAndSize(0, 0, list_width, rect.getHeight() );
		}
		else
		{	
			// stack on top or bottom, depending on which has more room
			if (-root_view_local.mBottom > root_view_local.mTop - getRect().getHeight())
			{
				// Move rect so it hangs off the bottom of this view
				rect.setLeftTopAndSize(0, 0, list_width, llmin(-root_view_local.mBottom, rect.getHeight()));
			}
			else
			{
				// move rect so it stacks on top of this view (clipped to size of screen)
				rect.setOriginAndSize(0, getRect().getHeight(), list_width, llmin(root_view_local.mTop - getRect().getHeight(), rect.getHeight()));
			}
		}
	}
	else // ABOVE
	{
		if (rect.getHeight() <= root_view_local.mTop - getRect().getHeight())
		{
			// move rect so it stacks on top of this view (clipped to size of screen)
			rect.setOriginAndSize(0, getRect().getHeight(), list_width, llmin(root_view_local.mTop - getRect().getHeight(), rect.getHeight()));
		}
		else
		{
			// stack on top or bottom, depending on which has more room
			if (-root_view_local.mBottom > root_view_local.mTop - getRect().getHeight())
			{
				// Move rect so it hangs off the bottom of this view
				rect.setLeftTopAndSize(0, 0, list_width, llmin(-root_view_local.mBottom, rect.getHeight()));
			}
			else
			{
				// move rect so it stacks on top of this view (clipped to size of screen)
				rect.setOriginAndSize(0, getRect().getHeight(), list_width, llmin(root_view_local.mTop - getRect().getHeight(), rect.getHeight()));
			}
		}

	}
	mList->setOrigin(rect.mLeft, rect.mBottom);
	mList->reshape(rect.getWidth(), rect.getHeight());
	mList->translateIntoRect(root_view_local);

	// Make sure we didn't go off bottom of screen
	S32 x, y;
	mList->localPointToScreen(0, 0, &x, &y);

	if (y < 0)
	{
		mList->translate(0, -y);
	}

	// NB: this call will trigger the focuslost callback which will hide the list, so do it first
	// before finally showing the list

	mList->setFocus(TRUE);

	// Show the list and push the button down
	mButton->setToggleState(TRUE);
	mList->setVisible(TRUE);
	
	LLUI::addPopup(this);

	setUseBoundingRect(TRUE);
//	updateBoundingRect();
}

void LLComboBox::hideList()
{
	if (mList->getVisible())
	{
		// assert selection in list
		if(mAllowNewValues)
		{
			// mLastSelectedIndex = -1 means that we entered a new value, don't select
			// any of existing items in this case.
			if(mLastSelectedIndex >= 0)
				mList->selectNthItem(mLastSelectedIndex);
		}
		else if(mLastSelectedIndex >= 0)
			mList->selectNthItem(mLastSelectedIndex);

		mButton->setToggleState(FALSE);
		mList->setVisible(FALSE);
		mList->mouseOverHighlightNthItem(-1);

		setUseBoundingRect(FALSE);
		LLUI::removePopup(this);
//		updateBoundingRect();
	}
}

void LLComboBox::onButtonMouseDown()
{
	if (!mList->getVisible())
	{
		// this might change selection, so do it first
		prearrangeList();

		// highlight the last selected item from the original selection before potentially selecting a new item
		// as visual cue to original value of combo box
		LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
		if (last_selected_item)
		{
			mList->mouseOverHighlightNthItem(mList->getItemIndex(last_selected_item));
		}

		if (mList->getItemCount() != 0)
		{
			showList();
		}

		setFocus( TRUE );

		// pass mouse capture on to list if button is depressed
		if (mButton->hasMouseCapture())
		{
			gFocusMgr.setMouseCapture(mList);

			// But keep the "pressed" look, which buttons normally lose when they
			// lose focus
			mButton->setForcePressedState(true);
		}
	}
	else
	{
		hideList();
	} 

}

void LLComboBox::onListMouseUp()
{
	// In some cases this is the termination of a mouse click that started on
	// the button, so clear its pressed state
	mButton->setForcePressedState(false);
}

//------------------------------------------------------------------
// static functions
//------------------------------------------------------------------

void LLComboBox::onItemSelected(const LLSD& data)
{
	mLastSelectedIndex = getCurrentIndex();
	if (mLastSelectedIndex != -1)
	{
		updateLabel();

		if (mAllowTextEntry)
		{
			gFocusMgr.setKeyboardFocus(mTextEntry);
			mTextEntry->selectAll();
		}
	}
	// hiding the list reasserts the old value stored in the text editor/dropdown button
	hideList();

	// commit does the reverse, asserting the value in the list
	onCommit();
}

BOOL LLComboBox::handleToolTip(S32 x, S32 y, MASK mask)
{
    std::string tool_tip;

	if(LLUICtrl::handleToolTip(x, y, mask))
	{
		return TRUE;
	}
	
	tool_tip = getToolTip();
	if (tool_tip.empty())
	{
		tool_tip = getSelectedItemLabel();
	}
	
	if( !tool_tip.empty() )
	{
		LLToolTipMgr::instance().show(LLToolTip::Params()
			.message(tool_tip)
			.sticky_rect(calcScreenRect()));
	}
	return TRUE;
}

BOOL LLComboBox::handleKeyHere(KEY key, MASK mask)
{
	BOOL result = FALSE;
	if (hasFocus())
	{
		if (mList->getVisible() 
			&& key == KEY_ESCAPE && mask == MASK_NONE)
		{
			hideList();
			return TRUE;
		}
		//give list a chance to pop up and handle key
		LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
		if (last_selected_item)
		{
			// highlight the original selection before potentially selecting a new item
			mList->mouseOverHighlightNthItem(mList->getItemIndex(last_selected_item));
		}
		result = mList->handleKeyHere(key, mask);

		// will only see return key if it is originating from line editor
		// since the dropdown button eats the key
		if (key == KEY_RETURN)
		{
			// don't show list and don't eat key input when committing
			// free-form text entry with RETURN since user already knows
            // what they are trying to select
			return FALSE;
		}
		// if selection has changed, pop open list
		else if (mList->getLastSelectedItem() != last_selected_item 
					|| ((key == KEY_DOWN || key == KEY_UP)
						&& mList->getCanSelect()
						&& !mList->isEmpty()))
		{
			showList();
		}
	}
	return result;
}

BOOL LLComboBox::handleUnicodeCharHere(llwchar uni_char)
{
	BOOL result = FALSE;
	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		// space bar just shows the list
		if (' ' != uni_char )
		{
			LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
			if (last_selected_item)
			{
				// highlight the original selection before potentially selecting a new item
				mList->mouseOverHighlightNthItem(mList->getItemIndex(last_selected_item));
			}
			result = mList->handleUnicodeCharHere(uni_char);
			if (mList->getLastSelectedItem() != last_selected_item)
			{
				showList();
			}
		}
	}
	return result;
}

void LLComboBox::setTextEntry(const LLStringExplicit& text)
{
	if (mTextEntry)
	{
		mTextEntry->setText(text);
		mHasAutocompletedText = FALSE;
		updateSelection();
	}
}

void LLComboBox::onTextEntry(LLLineEditor* line_editor)
{
	if (mTextEntryCallback != NULL)
	{
		(mTextEntryCallback)(line_editor, LLSD());
	}

	KEY key = gKeyboard->currentKey();
	if (key == KEY_BACKSPACE || 
		key == KEY_DELETE)
	{
		if (mList->selectItemByLabel(line_editor->getText(), FALSE))
		{
			line_editor->setTentative(FALSE);
			mLastSelectedIndex = mList->getFirstSelectedIndex();
		}
		else
		{
			line_editor->setTentative(mTextEntryTentative);
			mList->deselectAllItems();
			mLastSelectedIndex = -1;
		}
		if (mTextChangedCallback != NULL)
		{
			(mTextChangedCallback)(line_editor, LLSD());
		}
		return;
	}

	if (key == KEY_LEFT || 
		key == KEY_RIGHT)
	{
		return;
	}

	if (key == KEY_DOWN)
	{
		setCurrentByIndex(llmin(getItemCount() - 1, getCurrentIndex() + 1));
		if (!mList->getVisible())
		{
			prearrangeList();

			if (mList->getItemCount() != 0)
			{
				showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(FALSE);
	}
	else if (key == KEY_UP)
	{
		setCurrentByIndex(llmax(0, getCurrentIndex() - 1));
		if (!mList->getVisible())
		{
			prearrangeList();

			if (mList->getItemCount() != 0)
			{
				showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(FALSE);
	}
	else
	{
		// RN: presumably text entry
		updateSelection();
	}
	if (mTextChangedCallback != NULL)
	{
		(mTextChangedCallback)(line_editor, LLSD());
	}
}

void LLComboBox::updateSelection()
{
	LLWString left_wstring = mTextEntry->getWText().substr(0, mTextEntry->getCursor());
	// user-entered portion of string, based on assumption that any selected
    // text was a result of auto-completion
	LLWString user_wstring = mHasAutocompletedText ? left_wstring : mTextEntry->getWText();
	std::string full_string = mTextEntry->getText();

	// go ahead and arrange drop down list on first typed character, even
	// though we aren't showing it... some code relies on prearrange
	// callback to populate content
	if( mTextEntry->getWText().size() == 1 )
	{
		prearrangeList(mTextEntry->getText());
	}

	if (mList->selectItemByLabel(full_string, FALSE))
	{
		mTextEntry->setTentative(FALSE);
		mLastSelectedIndex = mList->getFirstSelectedIndex();
	}
	else if (mList->selectItemByPrefix(left_wstring, FALSE))
	{
		LLWString selected_item = utf8str_to_wstring(getSelectedItemLabel());
		LLWString wtext = left_wstring + selected_item.substr(left_wstring.size(), selected_item.size());
		mTextEntry->setText(wstring_to_utf8str(wtext));
		mTextEntry->setSelection(left_wstring.size(), mTextEntry->getWText().size());
		mTextEntry->endSelection();
		mTextEntry->setTentative(FALSE);
		mHasAutocompletedText = TRUE;
		mLastSelectedIndex = mList->getFirstSelectedIndex();
	}
	else // no matching items found
	{
		mList->deselectAllItems();
		mTextEntry->setText(wstring_to_utf8str(user_wstring)); // removes text added by autocompletion
		mTextEntry->setTentative(mTextEntryTentative);
		mHasAutocompletedText = FALSE;
		mLastSelectedIndex = -1;
	}
}

void LLComboBox::onTextCommit(const LLSD& data)
{
	std::string text = mTextEntry->getText();
	setSimple(text);
	onCommit();
	mTextEntry->selectAll();
}

void LLComboBox::setFocus(BOOL b)
{
	LLUICtrl::setFocus(b);

	if (b)
	{
		mList->clearSearchString();
		if (mList->getVisible())
		{
			mList->setFocus(TRUE);
		}
	}
}

void LLComboBox::prearrangeList(std::string filter)
{
	if (mPrearrangeCallback)
	{
		mPrearrangeCallback(this, LLSD(filter));
	}
}

//============================================================================
// LLCtrlListInterface functions

S32 LLComboBox::getItemCount() const
{
	return mList->getItemCount();
}

void LLComboBox::addColumn(const LLSD& column, EAddPosition pos)
{
	mList->clearColumns();
	mList->addColumn(column, pos);
}

void LLComboBox::clearColumns()
{
	mList->clearColumns();
}

void LLComboBox::setColumnLabel(const std::string& column, const std::string& label)
{
	mList->setColumnLabel(column, label);
}

LLScrollListItem* LLComboBox::addElement(const LLSD& value, EAddPosition pos, void* userdata)
{
	return mList->addElement(value, pos, userdata);
}

LLScrollListItem* LLComboBox::addSimpleElement(const std::string& value, EAddPosition pos, const LLSD& id)
{
	return mList->addSimpleElement(value, pos, id);
}

void LLComboBox::clearRows()
{
	mList->clearRows();
}

void LLComboBox::sortByColumn(const std::string& name, BOOL ascending)
{
	mList->sortByColumn(name, ascending);
}

//============================================================================
//LLCtrlSelectionInterface functions

BOOL LLComboBox::setCurrentByID(const LLUUID& id)
{
	BOOL found = mList->selectByID( id );

	if (found)
	{
		setLabel(getSelectedItemLabel());
		mLastSelectedIndex = mList->getFirstSelectedIndex();
	}

	return found;
}

LLUUID LLComboBox::getCurrentID() const
{
	return mList->getStringUUIDSelectedItem();
}
BOOL LLComboBox::setSelectedByValue(const LLSD& value, BOOL selected)
{
	BOOL found = mList->setSelectedByValue(value, selected);
	if (found)
	{
		setLabel(getSelectedItemLabel());
	}
	return found;
}

LLSD LLComboBox::getSelectedValue()
{
	return mList->getSelectedValue();
}

BOOL LLComboBox::isSelected(const LLSD& value) const
{
	return mList->isSelected(value);
}

BOOL LLComboBox::operateOnSelection(EOperation op)
{
	if (op == OP_DELETE)
	{
		mList->deleteSelectedItems();
		return TRUE;
	}
	return FALSE;
}

BOOL LLComboBox::operateOnAll(EOperation op)
{
	if (op == OP_DELETE)
	{
		clearRows();
		return TRUE;
	}
	return FALSE;
}

BOOL LLComboBox::selectItemRange( S32 first, S32 last )
{
	return mList->selectItemRange(first, last);
}


static LLDefaultChildRegistry::Register<LLIconsComboBox> register_icons_combo_box("icons_combo_box");

LLIconsComboBox::Params::Params()
:	icon_column("icon_column", ICON_COLUMN),
	label_column("label_column", LABEL_COLUMN)
{}

LLIconsComboBox::LLIconsComboBox(const LLIconsComboBox::Params& p)
:	LLComboBox(p),
	mIconColumnIndex(p.icon_column),
	mLabelColumnIndex(p.label_column)
{}

const std::string LLIconsComboBox::getSelectedItemLabel(S32 column) const
{
	mButton->setImageOverlay(LLComboBox::getSelectedItemLabel(mIconColumnIndex), mButton->getImageOverlayHAlign());

	return LLComboBox::getSelectedItemLabel(mLabelColumnIndex);
}
