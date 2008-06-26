/** 
 * @file llviewchildren.cpp
 * @brief LLViewChildren class implementation
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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
	mParent.childSetVisible(id, visible);
}

void LLViewChildren::enable(const std::string& id, bool enabled)
{
	mParent.childSetEnabled(id, enabled);
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

void LLViewChildren::setWrappedText(
	const std::string& id, const std::string& text, bool visible)
{
	LLTextBox* child = mParent.getChild<LLTextBox>(id);
	if (child)
	{
		child->setVisible(visible);
		child->setWrappedText(text);
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
			case BADGE_OK:		child->setImage(std::string("badge_ok.j2c"));	break;
			case BADGE_NOTE:	child->setImage(std::string("badge_note.j2c"));	break;
			case BADGE_WARN:	child->setImage(std::string("badge_warn.j2c"));	break;
			case BADGE_ERROR:	child->setImage(std::string("badge_error.j2c"));	break;
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


