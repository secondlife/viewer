/** 
 * @file llradiogroup.cpp
 * @brief LLRadioGroup base class
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

// An invisible view containing multiple mutually exclusive toggling 
// buttons (usually radio buttons).  Automatically handles the mutex
// condition by highlighting only one button at a time.

#include "linden_common.h"

#include "llboost.h"

#include "llradiogroup.h"
#include "indra_constants.h"

#include "llviewborder.h"
#include "llcontrol.h"
#include "llui.h"
#include "llfocusmgr.h"

LLRadioGroup::LLRadioGroup(const LLString& name, const LLRect& rect,
						   const LLString& control_name,
						   LLUICtrlCallback callback,
						   void* userdata,
						   BOOL border)
:	LLUICtrl(name, rect, TRUE, callback, userdata, FOLLOWS_LEFT | FOLLOWS_TOP),
	mSelectedIndex(0)
{
	setControlName(control_name, NULL);
	init(border);
}

LLRadioGroup::LLRadioGroup(const LLString& name, const LLRect& rect,
						   S32 initial_index,
						   LLUICtrlCallback callback,
						   void* userdata,
						   BOOL border) :
	LLUICtrl(name, rect, TRUE, callback, userdata, FOLLOWS_LEFT | FOLLOWS_TOP),
	mSelectedIndex(initial_index)
{
	init(border);
}

void LLRadioGroup::init(BOOL border)
{
	if (border)
	{
		addChild( new LLViewBorder( "radio group border", 
									LLRect(0, mRect.getHeight(), mRect.getWidth(), 0), 
									LLViewBorder::BEVEL_NONE, 
									LLViewBorder::STYLE_LINE, 
									1 ) );
	}
	mHasBorder = border;
}




LLRadioGroup::~LLRadioGroup()
{
}


// virtual
void LLRadioGroup::setEnabled(BOOL enabled)
{
	for (child_list_const_iter_t child_iter = getChildList()->begin();
		 child_iter != getChildList()->end(); ++child_iter)
	{
		LLView *child = *child_iter;
		child->setEnabled(enabled);
	}
	LLView::setEnabled(enabled);
}

void LLRadioGroup::setIndexEnabled(S32 index, BOOL enabled)
{
	S32 count = 0;
	for (button_list_t::iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* child = *iter;
		if (count == index)
		{
			child->setEnabled(enabled);
			if (index == mSelectedIndex && enabled == FALSE)
			{
				setSelectedIndex(-1);
			}
			break;
		}
		count++;
	}
	count = 0;
	if (mSelectedIndex < 0)
	{
		// Set to highest enabled value < index,
		// or lowest value above index if none lower are enabled
		// or 0 if none are enabled
		for (button_list_t::iterator iter = mRadioButtons.begin();
			 iter != mRadioButtons.end(); ++iter)
		{
			LLRadioCtrl* child = *iter;
			if (count >= index && mSelectedIndex >= 0)
			{
				break;
			}
			if (child->getEnabled())
			{
				setSelectedIndex(count);
			}
			count++;
		}
		if (mSelectedIndex < 0)
		{
			setSelectedIndex(0);
		}
	}
}

S32 LLRadioGroup::getSelectedIndex() const
{
	return mSelectedIndex;
}

BOOL LLRadioGroup::setSelectedIndex(S32 index, BOOL from_event)
{
	if (index < 0 || index >= (S32)mRadioButtons.size())
	{
		return FALSE;
	}

	mSelectedIndex = index;

	if (!from_event)
	{
		setControlValue(getSelectedIndex());
	}

	return TRUE;
}

BOOL LLRadioGroup::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	BOOL handled = FALSE;
	// do any of the tab buttons have keyboard focus?
	if (getEnabled() && !called_from_parent && mask == MASK_NONE)
	{
		switch(key)
		{
		case KEY_DOWN:
			if (!setSelectedIndex((getSelectedIndex() + 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		case KEY_UP:
			if (!setSelectedIndex((getSelectedIndex() - 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		case KEY_LEFT:
			if (!setSelectedIndex((getSelectedIndex() - 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		case KEY_RIGHT:
			if (!setSelectedIndex((getSelectedIndex() + 1)))
			{
				make_ui_sound("UISndInvalidOp");
			}
			else
			{
				onCommit();
			}
			handled = TRUE;
			break;
		default:
			break;
		}
	}
	return handled;
}

void LLRadioGroup::draw()
{
	S32 current_button = 0;

	BOOL take_focus = FALSE;
	if (gFocusMgr.childHasKeyboardFocus(this))
	{
		take_focus = TRUE;
	}

	for (button_list_t::iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;
		BOOL selected = (current_button == mSelectedIndex);
		radio->setValue( selected );
		if (take_focus && selected && !gFocusMgr.childHasKeyboardFocus(radio))
		{
			radio->focusFirstItem();
		}
		current_button++;
	}

	LLView::draw();
}


// When adding a button, we need to ensure that the radio
// group gets a message when the button is clicked.
LLRadioCtrl* LLRadioGroup::addRadioButton(const LLString& name, const LLString& label, const LLRect& rect, const LLFontGL* font )
{
	// Highlight will get fixed in draw method above
	LLRadioCtrl* radio = new LLRadioCtrl(name, rect, label, font,
		onClickButton, this);
	addChild(radio);
	mRadioButtons.push_back(radio);
	return radio;
}

// Handle one button being clicked.  All child buttons must have this
// function as their callback function.

// static
void LLRadioGroup::onClickButton(LLUICtrl* ui_ctrl, void* userdata)
{
	// llinfos << "LLRadioGroup::onClickButton" << llendl;

	LLRadioCtrl* clickedRadio = (LLRadioCtrl*) ui_ctrl;
	LLRadioGroup* self = (LLRadioGroup*) userdata;

	S32 counter = 0;
	for (button_list_t::iterator iter = self->mRadioButtons.begin();
		 iter != self->mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;
		if (radio == clickedRadio)
		{
			// llinfos << "clicked button " << counter << llendl;
			self->setSelectedIndex(counter);
			self->setControlValue(counter);
			
			// BUG: Calls click callback even if button didn't actually change
			self->onCommit();

			return;
		}

		counter++;
	}

	llwarns << "LLRadioGroup::onClickButton - clicked button that isn't a child" << llendl;
}

void LLRadioGroup::setValue( const LLSD& value )
{
	LLString value_name = value.asString();
	int idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;
		if (radio->getName() == value_name)
		{
			setSelectedIndex(idx);
			idx = -1;
			break;
		}
		++idx;
	}
	if (idx != -1)
	{
		// string not found, try integer
		if (value.isInteger())
		{
			setSelectedIndex((S32) value.asInteger(), TRUE);
		}
		else
		{
			llwarns << "LLRadioGroup::setValue: value not found: " << value_name << llendl;
		}
	}
}

LLSD LLRadioGroup::getValue() const
{
	int index = getSelectedIndex();
	int idx = 0;
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		if (idx == index) return LLSD((*iter)->getName());
		++idx;
	}
	return LLSD();
}

// virtual
LLXMLNodePtr LLRadioGroup::getXML(bool save_children) const
{
	LLXMLNodePtr node = LLUICtrl::getXML();

	// Attributes

	node->createChild("draw_border", TRUE)->setBoolValue(mHasBorder);

	// Contents

	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;

		LLXMLNodePtr child_node = radio->LLView::getXML();
		child_node->setStringValue(radio->getLabel());
		child_node->setName("radio_item");

		node->addChild(child_node);
	}

	return node;
}

// static
LLView* LLRadioGroup::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("radio_group");
	node->getAttributeString("name", name);

	U32 initial_value = 0;
	node->getAttributeU32("initial_value", initial_value);

	BOOL draw_border = TRUE;
	node->getAttributeBOOL("draw_border", draw_border);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	LLRadioGroup* radio_group = new LLRadioGroup(name, 
		rect,
		initial_value,
		NULL,
		NULL,
		draw_border);

	const LLString& contents = node->getValue();

	LLRect group_rect = radio_group->getRect();

	LLFontGL *font = LLView::selectFont(node);

	if (contents.find_first_not_of(" \n\t") != contents.npos)
	{
		// ...old school default vertical layout
		typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
		boost::char_separator<char> sep("\t\n");
		tokenizer tokens(contents, sep);
		tokenizer::iterator token_iter = tokens.begin();
	
		const S32 HPAD = 4, VPAD = 4;
		S32 cur_y = group_rect.getHeight() - VPAD;
	
		while(token_iter != tokens.end())
		{
			const char* line = token_iter->c_str();
			LLRect rect(HPAD, cur_y, group_rect.getWidth() - (2 * HPAD), cur_y - 15);
			cur_y -= VPAD + 15;
			radio_group->addRadioButton("radio", line, rect, font);
			++token_iter;
		}
		llwarns << "Legacy radio group format used! Please convert to use <radio_item> tags!" << llendl;
	}
	else
	{
		// ...per pixel layout
		LLXMLNodePtr child;
		for (child = node->getFirstChild(); child.notNull(); child = child->getNextSibling())
		{
			if (child->hasName("radio_item"))
			{
				LLRect item_rect;
				createRect(child, item_rect, radio_group, rect);

				LLString radioname("radio");
				child->getAttributeString("name", radioname);
				LLString item_label = child->getTextContents();
				LLRadioCtrl* radio = radio_group->addRadioButton(radioname, item_label.c_str(), item_rect, font);

				radio->initFromXML(child, radio_group);
			}
		}
	}

	radio_group->initFromXML(node, parent);

	return radio_group;
}

// LLCtrlSelectionInterface functions
BOOL	LLRadioGroup::setCurrentByID( const LLUUID& id )
{
	return FALSE;
}

LLUUID	LLRadioGroup::getCurrentID()
{
	return LLUUID::null;
}

BOOL	LLRadioGroup::setSelectedByValue(LLSD value, BOOL selected)
{
	S32 idx = 0;
	std::string value_string = value.asString();
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		if((*iter)->getName() == value_string)
		{
			setSelectedIndex(idx);
			return TRUE;
		}
		idx++;
	}

	return FALSE;
}

LLSD	LLRadioGroup::getSimpleSelectedValue()
{
	return getValue();	
}

BOOL	LLRadioGroup::isSelected(LLSD value)
{
	S32 idx = 0;
	std::string value_string = value.asString();
	for (button_list_t::const_iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		if((*iter)->getName() == value_string)
		{
			if (idx == mSelectedIndex) 
			{
				return TRUE;
			}
		}
		idx++;
	}
	return FALSE;
}

BOOL	LLRadioGroup::operateOnSelection(EOperation op)
{
	return FALSE;
}

BOOL	LLRadioGroup::operateOnAll(EOperation op)
{
	return FALSE;
}


LLRadioCtrl::LLRadioCtrl(const LLString& name, const LLRect& rect, const LLString& label,	
						 const LLFontGL* font, void (*commit_callback)(LLUICtrl*, void*), void* callback_userdata) :
				LLCheckBoxCtrl(name, rect, label, font, commit_callback, callback_userdata, FALSE, RADIO_STYLE)
{
	setTabStop(FALSE);
}

LLRadioCtrl::~LLRadioCtrl()
{
}

void LLRadioCtrl::setValue(const LLSD& value)
{
	LLCheckBoxCtrl::setValue(value);
	mButton->setTabStop(value.asBoolean());
}

