/** 
 * @file llchathistory.h
 * @brief LLTextEditor base class
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
		LLView* getHeader(const LLChat& chat,const LLStyle::Params& style_params, const LLSD& args);

		void onClickMoreText();

	public:
		~LLChatHistory();

		void initFromParams(const Params&);

		/**
		 * Appends a widget message.
		 * If last user appended message, concurs with current user,
		 * separator is added before the message, otherwise header is added.
		 * The args LLSD contains:
		 * - use_plain_text_chat_history (bool) - whether to add message as plain text.
		 * - owner_id (LLUUID) - the owner ID for object chat
		 * @param chat - base chat message.
		 * @param args - additional arguments
		 * @param input_append_params - font style.
		 */
		void appendMessage(const LLChat& chat, const LLSD &args = LLSD(), const LLStyle::Params& input_append_params = LLStyle::Params());
		/*virtual*/ void clear();

	private:
		std::string mLastFromName;
		LLUUID mLastFromID;
		LLDate mLastMessageTime;
		bool mIsLastMessageFromLog;
		//std::string mLastMessageTimeStr;

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

		S32 mPrependNewLineState;

		bool isNeedPrependNewline() {return (mPrependNewLineState-- > 0);}

		class LLLayoutPanel*	mMoreChatPanel;
		LLTextBox*		mMoreChatText;
		LLTextEditor*	mEditor;
		typedef std::set<std::string> unread_chat_source_t;
		unread_chat_source_t mUnreadChatSources;
};
#endif /* LLCHATHISTORY_H_ */
