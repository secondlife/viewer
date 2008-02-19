/** 
 * @file llcombobox.cpp
 * @brief LLComboBox base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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
#include "llcontrol.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "v2math.h"

// Globals
S32 LLCOMBOBOX_HEIGHT = 0;
S32 LLCOMBOBOX_WIDTH = 0;
S32 MAX_COMBO_WIDTH = 500;

LLComboBox::LLComboBox(	const LLString& name, const LLRect &rect, const LLString& label,
	void (*commit_callback)(LLUICtrl*,void*),
	void *callback_userdata
	)
:	LLUICtrl(name, rect, TRUE, commit_callback, callback_userdata, 
			 FOLLOWS_LEFT | FOLLOWS_TOP),
	mTextEntry(NULL),
	mArrowImage(NULL),
	mAllowTextEntry(FALSE),
	mMaxChars(20),
	mTextEntryTentative(TRUE),
	mListPosition(BELOW),
	mPrearrangeCallback( NULL ),
	mTextEntryCallback( NULL )
{
	// Always use text box 
	// Text label button
	mButton = new LLButton("comboxbox button",
								 LLRect(), label, NULL, LLString::null,
								 NULL, this);
	mButton->setImageUnselected("square_btn_32x128.tga");
	mButton->setImageSelected("square_btn_selected_32x128.tga");
	mButton->setImageDisabled("square_btn_32x128.tga");
	mButton->setImageDisabledSelected("square_btn_selected_32x128.tga");
	mButton->setScaleImage(TRUE);

	mButton->setMouseDownCallback(onButtonDown);
	mButton->setFont(LLFontGL::sSansSerifSmall);
	mButton->setFollows(FOLLOWS_LEFT | FOLLOWS_BOTTOM | FOLLOWS_RIGHT);
	mButton->setHAlign( LLFontGL::LEFT );
	mButton->setRightHPad(2);
	addChild(mButton);

	// disallow multiple selection
	mList = new LLScrollListCtrl(
		"ComboBox", LLRect(), 
		&LLComboBox::onItemSelected, this, FALSE);
	mList->setVisible(FALSE);
	mList->setBgWriteableColor( LLColor4(1,1,1,1) );
	mList->setCommitOnKeyboardMovement(FALSE);
	addChild(mList);

	LLRect border_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
	mBorder = new LLViewBorder( "combo border", border_rect );
	addChild( mBorder );
	mBorder->setFollowsAll();

	LLUUID arrow_image_id( LLUI::sAssetsGroup->getString("combobox_arrow.tga") );
	mArrowImage = LLUI::sImageProvider->getImageByID(arrow_image_id);
	mButton->setImageOverlay("combobox_arrow.tga", LLFontGL::RIGHT);

	updateLayout();
}


LLComboBox::~LLComboBox()
{
	// children automatically deleted, including mMenu, mButton
}

// virtual
LLXMLNodePtr LLComboBox::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	// Attributes

	node->createChild("allow_text_entry", TRUE)->setBoolValue(mAllowTextEntry);

	node->createChild("max_chars", TRUE)->setIntValue(mMaxChars);

	// Contents

	std::vector<LLScrollListItem*> data_list = mList->getAllData();
	std::vector<LLScrollListItem*>::iterator data_itor;
	for (data_itor = data_list.begin(); data_itor != data_list.end(); ++data_itor)
	{
		LLScrollListItem* item = *data_itor;
		LLScrollListCell* cell = item->getColumn(0);
		if (cell)
		{
			LLXMLNodePtr item_node = node->createChild("combo_item", FALSE);
			LLSD value = item->getValue();
			item_node->createChild("value", TRUE)->setStringValue(value.asString());
			item_node->createChild("enabled", TRUE)->setBoolValue(item->getEnabled());
			item_node->setStringValue(cell->getValue().asString());
		}
	}

	return node;
}

// static
LLView* LLComboBox::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("combo_box");
	node->getAttributeString("name", name);

	LLString label("");
	node->getAttributeString("label", label);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	BOOL allow_text_entry = FALSE;
	node->getAttributeBOOL("allow_text_entry", allow_text_entry);

	S32 max_chars = 20;
	node->getAttributeS32("max_chars", max_chars);

	LLUICtrlCallback callback = NULL;

	LLComboBox* combo_box = new LLComboBox(name,
							rect, 
							label,
							callback,
							NULL);
	combo_box->setAllowTextEntry(allow_text_entry, max_chars);

	combo_box->initFromXML(node, parent);

	const LLString& contents = node->getValue();

	if (contents.find_first_not_of(" \n\t") != contents.npos)
	{
		llerrs << "Legacy combo box item format used! Please convert to <combo_item> tags!" << llendl;
	}
	else
	{
		LLXMLNodePtr child;
		for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
		{
			if (child->hasName("combo_item"))
			{
				LLString label = child->getTextContents();

				LLString value = label;
				child->getAttributeString("value", value);

				combo_box->add(label, LLSD(value) );
			}
		}
	}

	combo_box->selectFirstItem();

	return combo_box;
}

void LLComboBox::setEnabled(BOOL enabled)
{
	LLView::setEnabled(enabled);
	mButton->setEnabled(enabled);
}

void LLComboBox::clear()
{ 
	if (mTextEntry)
	{
		mTextEntry->setText(LLString::null);
	}
	mButton->setLabelSelected(LLString::null);
	mButton->setLabelUnselected(LLString::null);
	mButton->setDisabledLabel(LLString::null);
	mButton->setDisabledSelectedLabel(LLString::null);
	mList->deselectAllItems();
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


// add item "name" to menu
LLScrollListItem* LLComboBox::add(const LLString& name, EAddPosition pos, BOOL enabled)
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos);
	item->setEnabled(enabled);
	mList->selectFirstItem();
	return item;
}

// add item "name" with a unique id to menu
LLScrollListItem* LLComboBox::add(const LLString& name, const LLUUID& id, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos, id);
	item->setEnabled(enabled);
	mList->selectFirstItem();
	return item;
}

// add item "name" with attached userdata
LLScrollListItem* LLComboBox::add(const LLString& name, void* userdata, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos);
	item->setEnabled(enabled);
	item->setUserdata( userdata );
	mList->selectFirstItem();
	return item;
}

// add item "name" with attached generic data
LLScrollListItem* LLComboBox::add(const LLString& name, LLSD value, EAddPosition pos, BOOL enabled )
{
	LLScrollListItem* item = mList->addSimpleElement(name, pos, value);
	item->setEnabled(enabled);
	mList->selectFirstItem();
	return item;
}

LLScrollListItem* LLComboBox::addSeparator(EAddPosition pos)
{
	return mList->addSeparator(pos);
}

void LLComboBox::sortByName()
{
	mList->sortByColumn(0, TRUE);
}


// Choose an item with a given name in the menu.
// Returns TRUE if the item was found.
BOOL LLComboBox::setSimple(const LLStringExplicit& name)
{
	BOOL found = mList->selectItemByLabel(name, FALSE);

	if (found)
	{
		setLabel(name);
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
			setLabel( mList->getSelectedItemLabel() );
		}
	}
}

const LLString LLComboBox::getSimple() const
{
	const LLString res = mList->getSelectedItemLabel();
	if (res.empty() && mAllowTextEntry)
	{
		return mTextEntry->getText();
	}
	else
	{
		return res;
	}
}

const LLString LLComboBox::getSelectedItemLabel(S32 column) const
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
		}
		else
		{
			mTextEntry->setTentative(mTextEntryTentative);
		}
	}
	
	if (!mAllowTextEntry)
	{
		mButton->setLabelUnselected(name);
		mButton->setLabelSelected(name);
		mButton->setDisabledLabel(name);
		mButton->setDisabledSelectedLabel(name);
	}
}


BOOL LLComboBox::remove(const LLString& name)
{
	BOOL found = mList->selectItemByLabel(name);

	if (found)
	{
		LLScrollListItem* item = mList->getFirstSelected();
		if (item)
		{
			mList->deleteSingleItem(mList->getItemIndex(item));
		}
	}

	return found;
}

BOOL LLComboBox::remove(S32 index)
{
	if (index < mList->getItemCount())
	{
		mList->deleteSingleItem(index);
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

void LLComboBox::onLostTop()
{
	hideList();
}


void LLComboBox::setButtonVisible(BOOL visible)
{
	mButton->setVisible(visible);
	if (mTextEntry)
	{
		LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
		if (visible)
		{
			text_entry_rect.mRight -= llmax(8,mArrowImage->getWidth(0)) + 2 * LLUI::sConfigGroup->getS32("DropShadowButton");
		}
		//mTextEntry->setRect(text_entry_rect);
		mTextEntry->reshape(text_entry_rect.getWidth(), text_entry_rect.getHeight(), TRUE);
	}
}

void LLComboBox::draw()
{
	if( getVisible() )
	{
		mBorder->setKeyboardFocusHighlight(hasFocus());

		mButton->setEnabled(getEnabled() /*&& !mList->isEmpty()*/);

		// Draw children normally
		LLUICtrl::draw();
	}
}

BOOL LLComboBox::setCurrentByIndex( S32 index )
{
	BOOL found = mList->selectNthItem( index );
	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
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


void LLComboBox::updateLayout()
{
	LLRect rect = getLocalRect();
	if (mAllowTextEntry)
	{
		S32 shadow_size = LLUI::sConfigGroup->getS32("DropShadowButton");
		mButton->setRect(LLRect( getRect().getWidth() - llmax(8,mArrowImage->getWidth(0)) - 2 * shadow_size,
								rect.mTop, rect.mRight, rect.mBottom));
		mButton->setTabStop(FALSE);

		if (!mTextEntry)
		{
			LLRect text_entry_rect(0, getRect().getHeight(), getRect().getWidth(), 0);
			text_entry_rect.mRight -= llmax(8,mArrowImage->getWidth(0)) + 2 * LLUI::sConfigGroup->getS32("DropShadowButton");
			// clear label on button
			LLString cur_label = mButton->getLabelSelected();
			mTextEntry = new LLLineEditor("combo_text_entry",
										text_entry_rect,
										"",
										LLFontGL::sSansSerifSmall,
										mMaxChars,
										onTextCommit,
										onTextEntry,
										NULL,
										this,
										NULL,		// prevalidate func
										LLViewBorder::BEVEL_NONE,
										LLViewBorder::STYLE_LINE,
										0);	// no border
			mTextEntry->setSelectAllonFocusReceived(TRUE);
			mTextEntry->setHandleEditKeysDirectly(TRUE);
			mTextEntry->setCommitOnFocusLost(FALSE);
			mTextEntry->setText(cur_label);
			mTextEntry->setIgnoreTab(TRUE);
			mTextEntry->setFollowsAll();
			addChild(mTextEntry);
		}
		else
		{
			mTextEntry->setVisible(TRUE);
			mTextEntry->setMaxTextLength(mMaxChars);
		}

		// clear label on button
		setLabel(LLString::null);

		mButton->setFollows(FOLLOWS_BOTTOM | FOLLOWS_TOP | FOLLOWS_RIGHT);
	}
	else if (!mAllowTextEntry)
	{
		mButton->setRect(rect);
		mButton->setTabStop(TRUE);

		if (mTextEntry)
		{
			mTextEntry->setVisible(FALSE);
		}
		mButton->setFollowsAll();
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
	mList->fitContents( 192, llfloor((F32)window_size.mY / LLUI::sGLScaleFactor.mV[VY]) - 50 );

	// Make sure that we can see the whole list
	LLRect root_view_local;
	LLView* root_view = getRootView();
	root_view->localRectToOtherView(root_view->getLocalRect(), &root_view_local, this);
	
	LLRect rect = mList->getRect();

	S32 min_width = getRect().getWidth();
	S32 max_width = llmax(min_width, MAX_COMBO_WIDTH);
	S32 list_width = llclamp(mList->getMaxContentWidth(), min_width, max_width);

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
	mList->translateIntoRect(root_view_local, FALSE);

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
	
	setUseBoundingRect(TRUE);
	gFocusMgr.setTopCtrl(this);
}

void LLComboBox::hideList()
{
	//*HACK: store the original value explicitly somewhere, not just in label
	LLString orig_selection = mAllowTextEntry ? mTextEntry->getText() : mButton->getLabelSelected();

	// assert selection in list
	mList->selectItemByLabel(orig_selection, FALSE);

	mButton->setToggleState(FALSE);
	mList->setVisible(FALSE);
	mList->highlightNthItem(-1);

	setUseBoundingRect(FALSE);
	if( gFocusMgr.getTopCtrl() == this )
	{
		gFocusMgr.setTopCtrl(NULL);
	}
}

//------------------------------------------------------------------
// static functions
//------------------------------------------------------------------

// static
void LLComboBox::onButtonDown(void *userdata)
{
	LLComboBox *self = (LLComboBox *)userdata;

	if (!self->mList->getVisible())
	{
		LLScrollListItem* last_selected_item = self->mList->getLastSelectedItem();
		if (last_selected_item)
		{
			// highlight the original selection before potentially selecting a new item
			self->mList->highlightNthItem(self->mList->getItemIndex(last_selected_item));
		}

		if( self->mPrearrangeCallback )
		{
			self->mPrearrangeCallback( self, self->mCallbackUserData );
		}

		if (self->mList->getItemCount() != 0)
		{
			self->showList();
		}

		self->setFocus( TRUE );

		// pass mouse capture on to list if button is depressed
		if (self->mButton->hasMouseCapture())
		{
			gFocusMgr.setMouseCapture(self->mList);
		}
	}
	else
	{
		self->hideList();
	} 

}

// static
void LLComboBox::onItemSelected(LLUICtrl* item, void *userdata)
{
	// Note: item is the LLScrollListCtrl
	LLComboBox *self = (LLComboBox *) userdata;

	const LLString name = self->mList->getSelectedItemLabel();

	S32 cur_id = self->getCurrentIndex();
	if (cur_id != -1)
	{
		self->setLabel(name);

		if (self->mAllowTextEntry)
		{
			gFocusMgr.setKeyboardFocus(self->mTextEntry);
			self->mTextEntry->selectAll();
		}
	}

	// hiding the list reasserts the old value stored in the text editor/dropdown button
	self->hideList();

	// commit does the reverse, asserting the value in the list
	self->onCommit();
}

BOOL LLComboBox::handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen)
{
    LLString tool_tip;

	if(LLUICtrl::handleToolTip(x, y, msg, sticky_rect_screen))
	{
		return TRUE;
	}
	
	if (LLUI::sShowXUINames)
	{
		tool_tip = getShowNamesToolTip();
	}
	else
	{
		tool_tip = getToolTip();
		if (tool_tip.empty())
		{
			tool_tip = getValue().asString();
		}
	}
	
	if( !tool_tip.empty() )
	{
		msg = tool_tip;

		// Convert rect local to screen coordinates
		localPointToScreen( 
			0, 0, 
			&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
		localPointToScreen(
			getRect().getWidth(), getRect().getHeight(),
			&(sticky_rect_screen->mRight), &(sticky_rect_screen->mTop) );
	}
	return TRUE;
}

BOOL LLComboBox::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL result = FALSE;
	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		//give list a chance to pop up and handle key
		LLScrollListItem* last_selected_item = mList->getLastSelectedItem();
		if (last_selected_item)
		{
			// highlight the original selection before potentially selecting a new item
			mList->highlightNthItem(mList->getItemIndex(last_selected_item));
		}
		result = mList->handleKeyHere(key, mask, FALSE);
		// if selection has changed, pop open list
		if (mList->getLastSelectedItem() != last_selected_item)
		{
			showList();
		}
	}
	return result;
}

BOOL LLComboBox::handleUnicodeCharHere(llwchar uni_char, BOOL called_from_parent)
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
				mList->highlightNthItem(mList->getItemIndex(last_selected_item));
			}
			result = mList->handleUnicodeCharHere(uni_char, called_from_parent);
			if (mList->getLastSelectedItem() != last_selected_item)
			{
				showList();
			}
		}
	}
	return result;
}

void LLComboBox::setAllowTextEntry(BOOL allow, S32 max_chars, BOOL set_tentative)
{
	mAllowTextEntry = allow;
	mTextEntryTentative = set_tentative;
	mMaxChars = max_chars;

	updateLayout();
}

void LLComboBox::setTextEntry(const LLStringExplicit& text)
{
	if (mTextEntry)
	{
		mTextEntry->setText(text);
		updateSelection();
	}
}

//static 
void LLComboBox::onTextEntry(LLLineEditor* line_editor, void* user_data)
{
	LLComboBox* self = (LLComboBox*)user_data;

	if (self->mTextEntryCallback)
	{
		(*self->mTextEntryCallback)(line_editor, self->mCallbackUserData);
	}

	KEY key = gKeyboard->currentKey();
	if (key == KEY_BACKSPACE || 
		key == KEY_DELETE)
	{
		if (self->mList->selectItemByLabel(line_editor->getText(), FALSE))
		{
			line_editor->setTentative(FALSE);
		}
		else
		{
			line_editor->setTentative(self->mTextEntryTentative);
			self->mList->deselectAllItems();
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
		self->setCurrentByIndex(llmin(self->getItemCount() - 1, self->getCurrentIndex() + 1));
		if (!self->mList->getVisible())
		{
			if( self->mPrearrangeCallback )
			{
				self->mPrearrangeCallback( self, self->mCallbackUserData );
			}

			if (self->mList->getItemCount() != 0)
			{
				self->showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(FALSE);
	}
	else if (key == KEY_UP)
	{
		self->setCurrentByIndex(llmax(0, self->getCurrentIndex() - 1));
		if (!self->mList->getVisible())
		{
			if( self->mPrearrangeCallback )
			{
				self->mPrearrangeCallback( self, self->mCallbackUserData );
			}

			if (self->mList->getItemCount() != 0)
			{
				self->showList();
			}
		}
		line_editor->selectAll();
		line_editor->setTentative(FALSE);
	}
	else
	{
		// RN: presumably text entry
		self->updateSelection();
	}
}

void LLComboBox::updateSelection()
{
	LLWString left_wstring = mTextEntry->getWText().substr(0, mTextEntry->getCursor());
	// user-entered portion of string, based on assumption that any selected
    // text was a result of auto-completion
	LLWString user_wstring = mTextEntry->hasSelection() ? left_wstring : mTextEntry->getWText();
	LLString full_string = mTextEntry->getText();

	// go ahead and arrange drop down list on first typed character, even
	// though we aren't showing it... some code relies on prearrange
	// callback to populate content
	if( mTextEntry->getWText().size() == 1 )
	{
		if (mPrearrangeCallback)
		{
			mPrearrangeCallback( this, mCallbackUserData );
		}
	}

	if (mList->selectItemByLabel(full_string, FALSE))
	{
		mTextEntry->setTentative(FALSE);
	}
	else if (!mList->selectItemByPrefix(left_wstring, FALSE))
	{
		mList->deselectAllItems();
		mTextEntry->setText(wstring_to_utf8str(user_wstring));
		mTextEntry->setTentative(mTextEntryTentative);
	}
	else
	{
		LLWString selected_item = utf8str_to_wstring(mList->getSelectedItemLabel());
		LLWString wtext = left_wstring + selected_item.substr(left_wstring.size(), selected_item.size());
		mTextEntry->setText(wstring_to_utf8str(wtext));
		mTextEntry->setSelection(left_wstring.size(), mTextEntry->getWText().size());
		mTextEntry->endSelection();
		mTextEntry->setTentative(FALSE);
	}
}

//static 
void LLComboBox::onTextCommit(LLUICtrl* caller, void* user_data)
{
	LLComboBox* self = (LLComboBox*)user_data;
	LLString text = self->mTextEntry->getText();
	self->setSimple(text);
	self->onCommit();
	self->mTextEntry->selectAll();
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

void LLComboBox::setColumnLabel(const LLString& column, const LLString& label)
{
	mList->setColumnLabel(column, label);
}

LLScrollListItem* LLComboBox::addElement(const LLSD& value, EAddPosition pos, void* userdata)
{
	return mList->addElement(value, pos, userdata);
}

LLScrollListItem* LLComboBox::addSimpleElement(const LLString& value, EAddPosition pos, const LLSD& id)
{
	return mList->addSimpleElement(value, pos, id);
}

void LLComboBox::clearRows()
{
	mList->clearRows();
}

void LLComboBox::sortByColumn(LLString name, BOOL ascending)
{
}

//============================================================================
//LLCtrlSelectionInterface functions

BOOL LLComboBox::setCurrentByID(const LLUUID& id)
{
	BOOL found = mList->selectByID( id );

	if (found)
	{
		setLabel(mList->getSelectedItemLabel());
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
		setLabel(mList->getSelectedItemLabel());
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



//
// LLFlyoutButton
//

const S32 FLYOUT_BUTTON_ARROW_WIDTH = 24;

LLFlyoutButton::LLFlyoutButton(
		const LLString& name, 
		const LLRect &rect,
		const LLString& label,
		void (*commit_callback)(LLUICtrl*, void*) ,
		void *callback_userdata)
:		LLComboBox(name, rect, LLString::null, commit_callback, callback_userdata),
		mToggleState(FALSE),
		mActionButton(NULL)
{
	// Always use text box 
	// Text label button
	mActionButton = new LLButton("flyout_button_main",
								 LLRect(), label, NULL, LLString::null,
								 NULL, this);
	mActionButton->setScaleImage(TRUE);

	mActionButton->setClickedCallback(onActionButtonClick);
	mActionButton->setFollowsAll();
	mActionButton->setHAlign( LLFontGL::HCENTER );
	mActionButton->setLabel(label);
	addChild(mActionButton);

	mActionButtonImage = LLUI::getUIImageByName("flyout_btn_left.tga");
	mExpanderButtonImage = LLUI::getUIImageByName("flyout_btn_right.tga");
	mActionButtonImageSelected = LLUI::getUIImageByName("flyout_btn_left_selected.tga");
	mExpanderButtonImageSelected = LLUI::getUIImageByName("flyout_btn_right_selected.tga");

	mActionButton->setImageSelected(mActionButtonImageSelected);
	mActionButton->setImageUnselected(mActionButtonImage);
	mActionButton->setImageDisabled(LLPointer<LLUIImage>(NULL));
	mActionButton->setImageDisabledSelected(LLPointer<LLUIImage>(NULL));

	mButton->setImageSelected(mExpanderButtonImageSelected);
	mButton->setImageUnselected(mExpanderButtonImage);
	mButton->setImageDisabled(LLPointer<LLUIImage>(NULL));
	mButton->setImageDisabledSelected(LLPointer<LLUIImage>(NULL));
	mButton->setRightHPad(6);

	mBorder->setVisible(FALSE);

	updateLayout();
}

//static 
LLView* LLFlyoutButton::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name = "flyout_button";
	node->getAttributeString("name", name);

	LLString label("");
	node->getAttributeString("label", label);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLUICtrlCallback callback = NULL;

	LLFlyoutButton* flyout_button = new LLFlyoutButton(name,
							rect, 
							label,
							callback,
							NULL);

	LLString list_position;
	node->getAttributeString("list_position", list_position);
	if (list_position == "below")
	{
		flyout_button->mListPosition = BELOW;
	}
	else if (list_position == "above")
	{
		flyout_button->mListPosition = ABOVE;
	}
	

	flyout_button->initFromXML(node, parent);

	LLXMLNodePtr child;
	for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
	{
		if (child->hasName("flyout_button_item"))
		{
			LLString label = child->getTextContents();

			LLString value = label;
			child->getAttributeString("value", value);

			flyout_button->add(label, LLSD(value) );
		}
	}

	flyout_button->updateLayout();

	return flyout_button;
}

void LLFlyoutButton::updateLayout()
{
	LLComboBox::updateLayout();

	mButton->setOrigin(getRect().getWidth() - FLYOUT_BUTTON_ARROW_WIDTH, 0);
	mButton->reshape(FLYOUT_BUTTON_ARROW_WIDTH, getRect().getHeight());
	mButton->setFollows(FOLLOWS_RIGHT | FOLLOWS_TOP | FOLLOWS_BOTTOM);
	mButton->setTabStop(FALSE);
	mButton->setImageOverlay(mListPosition == BELOW ? "down_arrow.tga" : "up_arrow.tga", LLFontGL::RIGHT);

	mActionButton->setOrigin(0, 0);
	mActionButton->reshape(getRect().getWidth() - FLYOUT_BUTTON_ARROW_WIDTH, getRect().getHeight());
}

//static 
void LLFlyoutButton::onActionButtonClick(void *user_data)
{
	LLFlyoutButton* buttonp = (LLFlyoutButton*)user_data;
	// remember last list selection?
	buttonp->mList->deselect();
	buttonp->onCommit();
}

void LLFlyoutButton::draw()
{
	mActionButton->setToggleState(mToggleState);
	mButton->setToggleState(mToggleState);

	//FIXME: this should be an attribute of comboboxes, whether they have a distinct label or
	// the label reflects the last selected item, for now we have to manually remove the label
	mButton->setLabel(LLString::null);
	LLComboBox::draw();	
}

void LLFlyoutButton::setEnabled(BOOL enabled)
{
	mActionButton->setEnabled(enabled);
	LLComboBox::setEnabled(enabled);
}


void LLFlyoutButton::setToggleState(BOOL state)
{
	mToggleState = state;
}

