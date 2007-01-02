/** 
 * @file llviewchildren.cpp
 * @brief LLViewChildren class implementation
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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


void LLViewChildren::show(const char* id, bool visible)
{
	mParent.childSetVisible(id, visible);
}

void LLViewChildren::enable(const char* id, bool enabled)
{
	mParent.childSetEnabled(id, enabled);
}

void LLViewChildren::setText(
	const char* id, const std::string& text, bool visible)
{
	LLTextBox* child = LLUICtrlFactory::getTextBoxByName(&mParent, id);
	if (child)
	{
		child->setVisible(visible);
		child->setText(text);
	}
}

void LLViewChildren::setWrappedText(
	const char* id, const std::string& text, bool visible)
{
	LLTextBox* child = LLUICtrlFactory::getTextBoxByName(&mParent, id);
	if (child)
	{
		child->setVisible(visible);
		child->setWrappedText(text);
	}
}

void LLViewChildren::setBadge(const char* id, Badge badge, bool visible)
{
	static LLUUID badgeOK(gViewerArt.getString("badge_ok.tga"));
	static LLUUID badgeNote(gViewerArt.getString("badge_note.tga"));
	static LLUUID badgeWarn(gViewerArt.getString("badge_warn.tga"));
	static LLUUID	badgeError(gViewerArt.getString("badge_error.tga"));
	
	LLUUID badgeUUID;
	switch (badge)
	{
		default:
		case BADGE_OK:		badgeUUID = badgeOK;	break;
		case BADGE_NOTE:	badgeUUID = badgeNote;	break;
		case BADGE_WARN:	badgeUUID = badgeWarn;	break;
		case BADGE_ERROR:	badgeUUID = badgeError;	break;
	}
	
	LLIconCtrl* child = LLUICtrlFactory::getIconByName(&mParent, id);
	if (child)
	{
		child->setVisible(visible);
		child->setImage(badgeUUID);
	}
}

void LLViewChildren::setAction(const char* id,
	void(*function)(void*), void* value)
{
	LLButton* button = LLUICtrlFactory::getButtonByName(&mParent, id);
	if (button)
	{
		button->setClickedCallback(function, value);
	}
}


