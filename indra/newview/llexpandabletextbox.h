/** 
 * @file llexpandabletextbox.h
 * @brief LLExpandableTextBox and related class definitions
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
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

#ifndef LL_LLEXPANDABLETEXTBOX_H
#define LL_LLEXPANDABLETEXTBOX_H

#include "lltexteditor.h"
#include "llscrollcontainer.h"

/**
 * LLExpandableTextBox is a text box control that will show "More" link at end of text
 * if text doesn't fit into text box. After pressing "More" the text box will expand to show
 * all text. If text is still too big, a scroll bar will appear inside expanded text box.
 */
class LLExpandableTextBox : public LLUICtrl
{
protected:

	/**
	 * Extended text box. "More" link will appear at end of text if 
	 * text is too long to fit into text box size.
	 */
	class LLTextBoxEx : public LLTextEditor
	{
	public:
		struct Params :	public LLInitParam::Block<Params, LLTextEditor::Params>
		{
			Mandatory<std::string> more_label;
			Params();
		};

		// adds or removes "More" link as needed
		/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
		/*virtual*/ void setText(const LLStringExplicit& text, const LLStyle::Params& input_params = LLStyle::Params());
		void setTextBase(const std::string& text) { LLTextBase::setText(text); }

		/**
		 * Returns difference between text box height and text height.
		 * Value is positive if text height is greater than text box height.
		 */
		virtual S32 getVerticalTextDelta();

		/**
		 * Returns the height of text rect.
		 */
		S32 getTextPixelHeight();

		/**
		 * Shows "More" link
		 */
		void showExpandText();

		/**
		 * Hides "More" link
		 */
		void hideExpandText();

	protected:

		LLTextBoxEx(const Params& p);
		friend class LLUICtrlFactory;

	private:
		std::string mExpanderLabel;

		bool mExpanderVisible;
	};

public:

	struct Params :	public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Optional<LLTextBoxEx::Params> textbox;

		Optional<LLScrollContainer::Params> scroll;

		Optional<S32> max_height;

		Optional<bool> bg_visible,
					   expanded_bg_visible;

		Optional<LLUIColor> bg_color,
						   expanded_bg_color;

		Params();
	};

	/**
	 * Sets text
	 */
	virtual void setText(const std::string& str);

	/**
	 * Returns text
	 */
	virtual std::string getText() const { return mText; }

	/**
	 * Sets text
	 */
	/*virtual*/ void setValue(const LLSD& value);

	/**
	 * Returns text
	 */
	/*virtual*/ LLSD getValue() const { return mText; }

	/**
	 * Collapses text box on focus_lost event
	 */
	/*virtual*/ void onFocusLost();

	/**
	 * Collapses text box on top_lost event
	 */
	/*virtual*/ void onTopLost();


	/**
	 * Draws text box, collapses text box if its expanded and its parent's position changed
	 */
	/*virtual*/ void draw();

protected:

	LLExpandableTextBox(const Params& p);
	friend class LLUICtrlFactory;

	/**
	 * Expands text box.
	 * A scroll bar will appear if expanded height is greater than max_height
	 */
	virtual void expandTextBox();

	/**
	 * Collapses text box.
	 */
	virtual void collapseTextBox();

	/**
	 * Collapses text box if it is expanded and its parent's position changed
	 */
	virtual void collapseIfPosChanged();

	/**
	 * Updates text box rect to avoid horizontal scroll bar
	 */
	virtual void updateTextBoxRect();

	/**
	 * User clicked on "More" link - expand text box
	 */
	virtual void onExpandClicked();

	/**
	 * Saves collapsed text box's states(rect, parent rect...)
	 */
	virtual void saveCollapsedState();

	/**
	 * Recalculate text delta considering min_height and window rect.
	 */
	virtual S32 recalculateTextDelta(S32 text_delta);

protected:

	std::string mText;
	LLTextBoxEx* mTextBox;
	LLScrollContainer* mScroll;

	S32 mMaxHeight;
	LLRect mCollapsedRect;
	bool mExpanded;
	LLRect mParentRect;

	bool mBGVisible;
	bool mExpandedBGVisible;
	LLUIColor mBGColor;
	LLUIColor mExpandedBGColor;
};

#endif //LL_LLEXPANDABLETEXTBOX_H
