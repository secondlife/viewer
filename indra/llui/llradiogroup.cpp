/** 
 * @file llradiogroup.cpp
 * @brief LLRadioGroup base class
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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


#include "linden_common.h"

#include "llboost.h"

#include "llradiogroup.h"
#include "indra_constants.h"

#include "llviewborder.h"
#include "llcontrol.h"
#include "llui.h"
#include "llfocusmgr.h"
#include "lluictrlfactory.h"

static LLDefaultChildRegistry::Register<LLRadioGroup> r1("radio_group");
static RadioGroupRegistry::Register<LLRadioCtrl> register_radio_ctrl("radio_item");



LLRadioGroup::Params::Params()
:	has_border("draw_border")
{
	name = "radio_group";
	mouse_opaque = true;
	follows.flags = FOLLOWS_LEFT | FOLLOWS_TOP;
	// radio items are not tabbable until they are selected
	tab_stop = false;
}

LLRadioGroup::LLRadioGroup(const LLRadioGroup::Params& p)
:	LLUICtrl(p),
	mFont(p.font.isProvided() ? p.font() : LLFontGL::getFontSansSerifSmall()),
	mSelectedIndex(-1),
	mHasBorder(p.has_border)
{	
	if (mHasBorder)
	{
		LLViewBorder::Params params;
		params.name("radio group border");
		params.rect(LLRect(0, getRect().getHeight(), getRect().getWidth(), 0));
		params.bevel_style(LLViewBorder::BEVEL_NONE);
		LLViewBorder * vb = LLUICtrlFactory::create<LLViewBorder> (params);
		addChild (vb);
	}
}

LLRadioGroup::~LLRadioGroup()
{
}

// virtual
BOOL LLRadioGroup::postBuild()
{
	if (!mRadioButtons.empty())
	{
		mRadioButtons[0]->setTabStop(true);
	}
	if (mControlVariable)
	{
		setSelectedIndex(mControlVariable->getValue().asInteger());
	}
	return TRUE;
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
				mSelectedIndex = -1;
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

BOOL LLRadioGroup::setSelectedIndex(S32 index, BOOL from_event)
{
	if (index < 0 || index >= (S32)mRadioButtons.size())
	{
		return FALSE;
	}

	if (mSelectedIndex >= 0)
	{
		LLRadioCtrl* old_radio_item = mRadioButtons[mSelectedIndex];
		old_radio_item->setTabStop(false);
		old_radio_item->setValue( FALSE );
	}
	else
	{
		mRadioButtons[0]->setTabStop(false);
	}

	mSelectedIndex = index;

	LLRadioCtrl* radio_item = mRadioButtons[mSelectedIndex];
	radio_item->setTabStop(true);
	radio_item->setValue( TRUE );

	if (hasFocus())
	{
		mRadioButtons[mSelectedIndex]->focusFirstItem(FALSE, FALSE);
	}

	if (!from_event)
	{
		setControlValue(getSelectedIndex());
	}

	return TRUE;
}

BOOL LLRadioGroup::handleKeyHere(KEY key, MASK mask)
{
	BOOL handled = FALSE;
	// do any of the tab buttons have keyboard focus?
	if (mask == MASK_NONE)
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


// When adding a child button, we need to ensure that the radio
// group gets a message when the button is clicked.

/*virtual*/
bool LLRadioGroup::addChild(LLView* view, S32 tab_group)
{
	bool res = LLView::addChild(view, tab_group);
	if (res)
	{
		LLRadioCtrl* radio_ctrl = dynamic_cast<LLRadioCtrl*>(view);
		if (radio_ctrl)
		{
			radio_ctrl->setFont(mFont);
			radio_ctrl->setCommitCallback(boost::bind(&LLRadioGroup::onClickButton, this, _1));
			mRadioButtons.push_back(radio_ctrl);
		}
	}
	return res;
}

BOOL LLRadioGroup::handleMouseDown(S32 x, S32 y, MASK mask)
{
	// grab focus preemptively, before child button takes mousecapture
	// 
	if (hasTabStop())
	{
		focusFirstItem(FALSE, FALSE);
	}

	return LLUICtrl::handleMouseDown(x, y, mask);
}


// Handle one button being clicked.  All child buttons must have this
// function as their callback function.

void LLRadioGroup::onClickButton(LLUICtrl* ctrl)
{
	// llinfos << "LLRadioGroup::onClickButton" << llendl;
	LLRadioCtrl* clicked_radio = dynamic_cast<LLRadioCtrl*>(ctrl);
	if (!clicked_radio)
	    return;
	S32 index = 0;
	for (button_list_t::iterator iter = mRadioButtons.begin();
		 iter != mRadioButtons.end(); ++iter)
	{
		LLRadioCtrl* radio = *iter;
		if (radio == clicked_radio)
		{
			// llinfos << "clicked button " << index << llendl;
			setSelectedIndex(index);
			
			// BUG: Calls click callback even if button didn't actually change
			onCommit();

			return;
		}

		index++;
	}

	llwarns << "LLRadioGroup::onClickButton - clicked button that isn't a child" << llendl;
}

void LLRadioGroup::setValue( const LLSD& value )
{
	std::string value_name = value.asString();
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

// LLCtrlSelectionInterface functions
BOOL	LLRadioGroup::setCurrentByID( const LLUUID& id )
{
	return FALSE;
}

LLUUID	LLRadioGroup::getCurrentID() const
{
	return LLUUID::null;
}

BOOL	LLRadioGroup::setSelectedByValue(const LLSD& value, BOOL selected)
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

LLSD	LLRadioGroup::getSelectedValue()
{
	return getValue();	
}

BOOL	LLRadioGroup::isSelected(const LLSD& value) const
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

LLRadioCtrl::LLRadioCtrl(const LLRadioCtrl::Params& p)
	: LLCheckBoxCtrl(p)
{
}

BOOL LLRadioCtrl::postBuild()
{
	// Old-style radio_item used the text contents to indicate the label,
	// but new-style radio_item uses label attribute.
	std::string value = getValue().asString();
	if (!value.empty())
	{
		setLabel(value);
	}
	return TRUE;
}

LLRadioCtrl::~LLRadioCtrl()
{
}

void LLRadioCtrl::setValue(const LLSD& value)
{
	LLCheckBoxCtrl::setValue(value);
	mButton->setTabStop(value.asBoolean());
}

// *TODO: Remove this function after the initial XUI XML re-export pass.
// static
void LLRadioCtrl::setupParamsForExport(Params& p, LLView* parent)
{
	std::string label = p.label;
	if (label.empty())
	{
		// We don't have a label attribute, so move the text contents
		// stored in "value" into the label
		std::string initial_value = p.LLUICtrl::Params::initial_value();
		p.label = initial_value;
		p.LLUICtrl::Params::initial_value = LLSD();
	}

	LLCheckBoxCtrl::setupParamsForExport(p, parent);
}
