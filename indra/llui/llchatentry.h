/**
 * @file llchatentry.h
 * @author Paul Guslisty
 * @brief Text editor widget which is used for user input
 *
 * Features:
 *			Optional line history so previous entries can be recalled by CTRL UP/DOWN
 *			Optional auto-resize behavior on input chat field
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LLCHATENTRY_H_
#define LLCHATENTRY_H_

#include "lltexteditor.h"

class LLChatEntry : public LLTextEditor
{
public:

	struct Params : public LLInitParam::Block<Params, LLTextEditor::Params>
	{
		Optional<bool>		has_history,
							is_expandable;

		Optional<int>		expand_lines_count;

		Params();
	};

	virtual ~LLChatEntry();

protected:

	friend class LLUICtrlFactory;
	LLChatEntry(const Params& p);

public:

	virtual void	draw();
	virtual	void	onCommit();

	boost::signals2::connection setTextExpandedCallback(const commit_signal_t::slot_type& cb);

private:

	/**
	 * Implements auto-resize behavior.
	 * When user's typing reaches the right edge of the chat field
	 * the chat field expands vertically by one line. The bottom of
	 * the chat field remains bottom-justified. The chat field does
	 * not expand beyond mExpandLinesCount.
	 */
	void	expandText();

	/**
	 * Implements line history so previous entries can be recalled by CTRL UP/DOWN
	 */
	void	updateHistory();

	BOOL	handleSpecialKey(const KEY key, const MASK mask);


	// Fired when text height expanded to mExpandLinesCount
	commit_signal_t*			mTextExpandedSignal;

	// line history support:
	typedef std::vector<std::string>	line_history_t;
	line_history_t::iterator			mCurrentHistoryLine;	// currently browsed history line
	line_history_t						mLineHistory;			// line history storage
	bool								mHasHistory;			// flag for enabled/disabled line history
	bool								mIsExpandable;

	int									mExpandLinesCount;
	int									mPrevLinesCount;
};

#endif /* LLCHATENTRY_H_ */
