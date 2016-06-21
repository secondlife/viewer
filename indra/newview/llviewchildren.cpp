/** 
 * @file llviewchildren.cpp
 * @brief LLViewChildren class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#include "llviewchildren.h"

#include "lluictrlfactory.h"

// viewer includes
#include "llbutton.h"
#include "lliconctrl.h"
#include "lltextbox.h"
#include "lluuid.h"
#include "llpanel.h"
#include "llviewercontrol.h"


// *NOTE: Do not use mParent reference in the constructor, since it is
// potentially not fully constructud.
LLViewChildren::LLViewChildren(LLPanel& parent)
	: mParent(parent)
{
}


void LLViewChildren::show(const std::string& id, bool visible)
{
	mParent.getChildView(id)->setVisible(visible);
}

void LLViewChildren::enable(const std::string& id, bool enabled)
{
	mParent.getChildView(id)->setEnabled(enabled);
}

void LLViewChildren::setText(
	const std::string& id, const std::string& text, bool visible)
{
	LLTextBox* child = mParent.getChild<LLTextBox>(id);
	if (child)
	{
		child->setVisible(visible);
		child->setText(text);
	}
}

void LLViewChildren::setBadge(const std::string& id, Badge badge, bool visible)
{
	LLIconCtrl* child = mParent.getChild<LLIconCtrl>(id);
	if (child)
	{
		child->setVisible(visible);
		switch (badge)
		{
			default:
			case BADGE_OK:		child->setValue(std::string("badge_ok.j2c"));	break;
			case BADGE_NOTE:	child->setValue(std::string("badge_note.j2c"));	break;
			case BADGE_WARN:
			case BADGE_ERROR:
				child->setValue(std::string("badge_warn.j2c"));	break;
		}
	}
}

void LLViewChildren::setAction(const std::string& id,
	void(*function)(void*), void* value)
{
	LLButton* button = mParent.getChild<LLButton>(id);
	if (button)
	{
		button->setClickedCallback(function, value);
	}
}


