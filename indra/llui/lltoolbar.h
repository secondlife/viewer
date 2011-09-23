/** 
 * @file lltoolbar.h
 * @author Richard Nelson
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H

#include "lluictrl.h"
#include "lllayoutstack.h"
#include "llbutton.h"


class LLCommand;


class LLToolBarButton : public LLButton
{
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
	};

	LLToolBarButton(const Params& p) : LLButton(p) {}
};


namespace LLToolBarEnums
{
	enum ButtonType
	{
		BTNTYPE_ICONS_ONLY = 0,
		BTNTYPE_ICONS_WITH_TEXT,

		BTNTYPE_COUNT
	};

	enum SideType
	{
		SIDE_NONE = 0,
		SIDE_BOTTOM,
		SIDE_LEFT,
		SIDE_RIGHT,
		SIDE_TOP,
	};
}

// NOTE: This needs to occur before Param block declaration for proper compilation.
namespace LLInitParam
{
	template<>
	struct TypeValues<LLToolBarEnums::ButtonType> : public TypeValuesHelper<LLToolBarEnums::ButtonType>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLToolBarEnums::SideType> : public TypeValuesHelper<LLToolBarEnums::SideType>
	{
		static void declareValues();
	};
}


class LLToolBar
:	public LLUICtrl
{
public:

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Mandatory<LLToolBarEnums::ButtonType>	button_display_mode;
		Mandatory<LLToolBarEnums::SideType>		side;

		Optional<LLToolBarButton::Params>		button_icon,
												button_icon_and_text;

		Optional<bool>							wrap;
		Optional<S32>							min_button_width,
												max_button_width,
												button_height;
		// get rid of this
		Multiple<LLToolBarButton::Params>		buttons;

		Optional<LLUIImage*>					background_image;

		Params();
	};

	// virtuals
	void draw();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	bool addCommand(LLCommand * command);

protected:
	friend class LLUICtrlFactory;
	LLToolBar(const Params&);

	void initFromParams(const Params&);

private:
	void updateLayoutAsNeeded();
	void resizeButtonsInRow(std::vector<LLToolBarButton*>& buttons_in_row, S32 max_row_girth);

	std::list<LLToolBarButton*>		mButtons;
	LLToolBarEnums::ButtonType		mButtonType;
	LLLayoutStack*					mCenteringStack;
	LLLayoutStack*					mWrapStack;
	LLLayoutPanel*					mCenterPanel;
	LLToolBarEnums::SideType		mSideType;

	bool							mWrap;
	bool							mNeedsLayout;
	S32								mMinButtonWidth,
									mMaxButtonWidth,
									mButtonHeight;

	LLUIImagePtr					mBackgroundImage;

	LLToolBarButton::Params			mButtonParams[LLToolBarEnums::BTNTYPE_COUNT];
};


#endif  // LL_LLTOOLBAR_H
