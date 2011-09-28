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

#include "llbutton.h"
#include "llcommandmanager.h"
#include "lllayoutstack.h"
#include "lluictrl.h"
#include "llcommandmanager.h"


class LLToolBarButton : public LLButton
{
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
	};

	LLToolBarButton(const Params& p);

	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	void setCommandId(const LLCommandId& id) { mId = id; }
private:
	LLCommandId		mId;
	S32				mMouseDownX;
	S32				mMouseDownY;
};


namespace LLToolBarEnums
{
	enum ButtonType
	{
		BTNTYPE_ICONS_WITH_TEXT = 0,
		BTNTYPE_ICONS_ONLY,

		BTNTYPE_COUNT
	};

	enum SideType
	{
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

		Optional<bool>							read_only,
												wrap;

		Optional<S32>							min_button_width,
												max_button_width,
												button_height;
		
		Optional<S32>							pad_left,
												pad_top,
												pad_right,
												pad_bottom,
												pad_between;
		// get rid of this
		Multiple<LLCommandId::Params>			commands;

		Optional<LLPanel::Params>				button_panel;

		Params();
	};

	// virtuals
	void draw();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);

	bool addCommand(const LLCommandId& commandId);
	bool hasCommand(const LLCommandId& commandId) const;
	bool enableCommand(const LLCommandId& commandId, bool enabled);
	command_id_list_t& getCommandsList() { return mButtonCommands; }

	LLToolBarButton* createButton(const LLCommandId& id);

protected:
	friend class LLUICtrlFactory;
	LLToolBar(const Params&);
	~LLToolBar();

	void initFromParams(const Params&);

private:
	void createContextMenu();
	void updateLayoutAsNeeded();
	void createButtons();
	void resizeButtonsInRow(std::vector<LLToolBarButton*>& buttons_in_row, S32 max_row_girth);
	BOOL isSettingChecked(const LLSD& userdata);
	void onSettingEnable(const LLSD& userdata);

	const bool						mReadOnly;

	std::list<LLToolBarButton*>		mButtons;
	command_id_list_t				mButtonCommands;
	typedef std::map<LLCommandId, LLToolBarButton*> command_id_map;
	command_id_map					mButtonMap;

	LLToolBarEnums::ButtonType		mButtonType;
	LLLayoutStack*					mCenteringStack;
	LLLayoutStack*					mWrapStack;
	LLPanel*						mButtonPanel;
	LLToolBarEnums::SideType		mSideType;

	bool							mWrap;
	bool							mNeedsLayout;
	S32								mMinButtonWidth,
									mMaxButtonWidth,
									mButtonHeight,
									mPadLeft,
									mPadRight,
									mPadTop,
									mPadBottom,
									mPadBetween;

	LLToolBarButton::Params			mButtonParams[LLToolBarEnums::BTNTYPE_COUNT];

	LLHandle<class LLContextMenu>			mPopupMenuHandle;
};


#endif  // LL_LLTOOLBAR_H
