/** 
 * @file llchathistory.h
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

#ifndef LLCHATHISTORY_H_
#define LLCHATHISTORY_H_

#include "lltexteditor.h"
#include "lltextbox.h"
#include "llviewerchat.h"

//Chat log widget allowing addition of a message as a widget 
class LLChatHistory : public LLUICtrl
{
	public:
		struct Params : public LLInitParam::Block<Params, LLTextEditor::Params>
		{
			//Message header filename
			Optional<std::string>	message_header;
			//Message separator filename
			Optional<std::string>	message_separator;
			//Text left padding from the scroll rect
			Optional<S32>			left_text_pad;
			//Text right padding from the scroll rect
			Optional<S32>			right_text_pad;
			//Widget left padding from the scroll rect
			Optional<S32>			left_widget_pad;
			//Widget right padding from the scroll rect
			Optional<S32>			right_widget_pad;
			//Separator top padding
			Optional<S32>			top_separator_pad;
			//Separator bottom padding
			Optional<S32>			bottom_separator_pad;
			//Header top padding
			Optional<S32>			top_header_pad;
			//Header bottom padding
			Optional<S32>			bottom_header_pad;

			Optional<LLTextBox::Params>	more_chat_text;

			Params()
			:	message_header("message_header"),
				message_separator("message_separator"),
				left_text_pad("left_text_pad"),
				right_text_pad("right_text_pad"),
				left_widget_pad("left_widget_pad"),
				right_widget_pad("right_widget_pad"),
				top_separator_pad("top_separator_pad"),
				bottom_separator_pad("bottom_separator_pad"),
				top_header_pad("top_header_pad"),
				bottom_header_pad("bottom_header_pad"),
				more_chat_text("more_chat_text")
			{}

		};
	protected:
		LLChatHistory(const Params&);
		friend class LLUICtrlFactory;

		/*virtual*/ void draw();
		/**
		 * Redefinition of LLTextEditor::updateTextRect() to considerate text
		 * left/right padding params.
		 */
		//virtual void	updateTextRect();
		/**
		 * Builds a message separator.
		 * @return pointer to LLView separator object.
		 */
		LLView* getSeparator();
		/**
		 * Builds a message header.
		 * @return pointer to LLView header object.
		 */
		LLView* getHeader(const LLChat& chat,const LLStyle::Params& style_params);

		void onClickMoreText();

	public:
		~LLChatHistory();

		void initFromParams(const Params&);

		/**
		 * Appends a widget message.
		 * If last user appended message, concurs with current user,
		 * separator is added before the message, otherwise header is added.
		 * @param chat - base chat message.
		 * @param use_plain_text_chat_history  - whether to add message as plain text.
		 * @param input_append_params - font style.
		 */
		void appendMessage(const LLChat& chat, const bool use_plain_text_chat_history = false, const LLStyle::Params& input_append_params = LLStyle::Params());
		/*virtual*/ void clear();
		/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	private:
		std::string mLastFromName;
		LLUUID mLastFromID;
		LLDate mLastMessageTime;
		std::string mMessageHeaderFilename;
		std::string mMessageSeparatorFilename;

		S32 mLeftTextPad;
		S32 mRightTextPad;

		S32 mLeftWidgetPad;
		S32 mRightWidgetPad;

		S32 mTopSeparatorPad;
		S32 mBottomSeparatorPad;
		S32 mTopHeaderPad;
		S32 mBottomHeaderPad;

		LLPanel*		mMoreChatPanel;
		LLTextBox*		mMoreChatText;
		LLTextEditor*	mEditor;
		typedef std::set<std::string> unread_chat_source_t;
		unread_chat_source_t mUnreadChatSources;
};
#endif /* LLCHATHISTORY_H_ */
