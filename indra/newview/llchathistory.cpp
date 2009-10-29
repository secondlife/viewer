/** 
 * @file llchathistory.cpp
 * @brief LLTextEditor base class
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

#include "llviewerprecompiledheaders.h"
#include "llchathistory.h"
#include "llpanel.h"
#include "lltextbox.h"
#include "lluictrlfactory.h"
#include "llscrollcontainer.h"
#include "llavatariconctrl.h"

static LLDefaultChildRegistry::Register<LLChatHistory> r("chat_history");
static const std::string MESSAGE_USERNAME_DATE_SEPARATOR(" ----- ");

LLChatHistory::LLChatHistory(const LLChatHistory::Params& p)
: LLTextEditor(p),
mMessageHeaderFilename(p.message_header),
mMessageSeparatorFilename(p.message_separator),
mLeftTextPad(p.left_text_pad),
mRightTextPad(p.right_text_pad),
mLeftWidgetPad(p.left_widget_pad),
mRightWidgetPad(p.right_widget_pad)
{
}

LLChatHistory::~LLChatHistory()
{
	this->clear();
}

/*void LLChatHistory::updateTextRect()
{
	static LLUICachedControl<S32> texteditor_border ("UITextEditorBorder", 0);

	LLRect old_text_rect = mTextRect;
	mTextRect = mScroller->getContentWindowRect();
	mTextRect.stretch(-texteditor_border);
	mTextRect.mLeft += mLeftTextPad;
	mTextRect.mRight -= mRightTextPad;
	if (mTextRect != old_text_rect)
	{
		needsReflow();
	}
}*/

LLView* LLChatHistory::getSeparator()
{
	LLPanel* separator = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>(mMessageSeparatorFilename, NULL, LLPanel::child_registry_t::instance());
	return separator;
}

LLView* LLChatHistory::getHeader(const LLUUID& avatar_id, std::string& from, std::string& time)
{
	LLPanel* header = LLUICtrlFactory::getInstance()->createFromFile<LLPanel>(mMessageHeaderFilename, NULL, LLPanel::child_registry_t::instance());
	LLTextBox* userName = header->getChild<LLTextBox>("user_name");
	userName->setValue(from);
	LLTextBox* timeBox = header->getChild<LLTextBox>("time_box");
	timeBox->setValue(time);
	if(!avatar_id.isNull())
	{
		LLAvatarIconCtrl* icon = header->getChild<LLAvatarIconCtrl>("avatar_icon");
		icon->setValue(avatar_id);
	}
	return header;
}

void LLChatHistory::appendWidgetMessage(const LLUUID& avatar_id, std::string& from, std::string& time, std::string& message, LLStyle::Params& style_params)
{
	LLView* view = NULL;
	std::string view_text;

	if (mLastFromName == from)
	{
		view = getSeparator();
		view_text = "\n";
	}
	else
	{
		view = getHeader(avatar_id, from, time);
		view_text = from + MESSAGE_USERNAME_DATE_SEPARATOR + time + '\n';
	}
	//Prepare the rect for the view
	LLRect target_rect = getDocumentView()->getRect();
	// squeeze down the widget by subtracting padding off left and right
	target_rect.mLeft += mLeftWidgetPad + mHPad;
	target_rect.mRight -= mRightWidgetPad;
	view->reshape(target_rect.getWidth(), view->getRect().getHeight());
	view->setOrigin(target_rect.mLeft, view->getRect().mBottom);

	LLInlineViewSegment::Params p;
	p.view = view;
	p.force_newline = true;
	p.left_pad = mLeftWidgetPad;
	p.right_pad = mRightWidgetPad;

	appendWidget(p, view_text, false);

	//Append the text message
	message += '\n';
	appendText(message, FALSE, style_params);

	mLastFromName = from;
	blockUndo();
	setCursorAndScrollToEnd();
}
