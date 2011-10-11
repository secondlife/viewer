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
#include "llassettype.h"

class LLToolBar;

typedef boost::function<void (S32 x, S32 y, const LLUUID& uuid)> tool_startdrag_callback_t;
typedef boost::function<BOOL (S32 x, S32 y, const LLUUID& uuid, LLAssetType::EType type)> tool_handledrag_callback_t;
typedef boost::function<BOOL (void* data, S32 x, S32 y, LLToolBar* toolbar)> tool_handledrop_callback_t;

class LLToolBarButton : public LLButton
{
	friend class LLToolBar;
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
		Optional<LLUI::RangeS32::Params>	button_width;
		Optional<S32>						desired_height;

		Params()
		:	button_width("button_width"),
			desired_height("desired_height", 20)
		{}

	};

	LLToolBarButton(const Params& p);
	~LLToolBarButton();

	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	BOOL handleHover(S32 x, S32 y, MASK mask);
	void reshape(S32 width, S32 height, BOOL called_from_parent = true);

	void setCommandId(const LLCommandId& id) { mId = id; }

	void setStartDragCallback(tool_startdrag_callback_t cb)   { mStartDragItemCallback  = cb; }
	void setHandleDragCallback(tool_handledrag_callback_t cb) { mHandleDragItemCallback = cb; }

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseCaptureLost();

private:
	LLCommandId		mId;
	S32				mMouseDownX;
	S32				mMouseDownY;
	LLUI::RangeS32	mWidthRange;
	S32				mDesiredHeight;
	bool							mIsDragged;
	tool_startdrag_callback_t		mStartDragItemCallback;
	tool_handledrag_callback_t		mHandleDragItemCallback;

	enable_signal_t*	mIsEnabledSignal;
	enable_signal_t*	mIsRunningSignal;
	enable_signal_t*	mIsStartingSignal;
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

		Optional<S32>							pad_left,
												pad_top,
												pad_right,
												pad_bottom,
												pad_between,
												min_girth;
		// get rid of this
		Multiple<LLCommandId::Params>			commands;

		Optional<LLPanel::Params>				button_panel;

		Params();
	};

	// virtuals
	void draw();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	int  getRankFromPosition(S32 x, S32 y);	
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	
	bool addCommand(const LLCommandId& commandId, int rank = -1);
	bool removeCommand(const LLCommandId& commandId);
	bool hasCommand(const LLCommandId& commandId) const;
	bool enableCommand(const LLCommandId& commandId, bool enabled);

	void setStartDragCallback(tool_startdrag_callback_t cb)   { mStartDragItemCallback  = cb; }
	void setHandleDragCallback(tool_handledrag_callback_t cb) { mHandleDragItemCallback = cb; }
	void setHandleDropCallback(tool_handledrop_callback_t cb) { mHandleDropCallback     = cb; }
	bool isReadOnly() const { return mReadOnly; }

	LLToolBarButton* createButton(const LLCommandId& id);

protected:
	friend class LLUICtrlFactory;
	LLToolBar(const Params&);
	~LLToolBar();

	void initFromParams(const Params&);
	tool_startdrag_callback_t		mStartDragItemCallback;
	tool_handledrag_callback_t		mHandleDragItemCallback;
	tool_handledrop_callback_t		mHandleDropCallback;
	bool							mDragAndDropTarget;
	int								mRank;
	LLCommandId						mDraggedCommand;

public:
	// Methods used in loading and saving toolbar settings
	void setButtonType(LLToolBarEnums::ButtonType button_type);
	LLToolBarEnums::ButtonType getButtonType() { return mButtonType; }
	command_id_list_t& getCommandsList() { return mButtonCommands; }
	void clearCommandsList();
					   
private:
	void createContextMenu();
	void updateLayoutAsNeeded();
	void createButtons();
	void resizeButtonsInRow(std::vector<LLToolBarButton*>& buttons_in_row, S32 max_row_girth);
	BOOL isSettingChecked(const LLSD& userdata);
	void onSettingEnable(const LLSD& userdata);

	const bool						mReadOnly;

	typedef std::list<LLToolBarButton*> toolbar_button_list;
	toolbar_button_list				mButtons;
	command_id_list_t				mButtonCommands;
	typedef std::map<LLUUID, LLToolBarButton*> command_id_map;
	command_id_map					mButtonMap;

	LLToolBarEnums::ButtonType		mButtonType;
	LLLayoutStack*					mCenteringStack;
	LLLayoutStack*					mWrapStack;
	LLPanel*						mButtonPanel;
	LLToolBarEnums::SideType		mSideType;

	bool							mWrap;
	bool							mNeedsLayout;
	S32								mPadLeft,
									mPadRight,
									mPadTop,
									mPadBottom,
									mPadBetween,
									mMinGirth;

	LLToolBarButton::Params			mButtonParams[LLToolBarEnums::BTNTYPE_COUNT];

	LLHandle<class LLContextMenu>			mPopupMenuHandle;
};


#endif  // LL_LLTOOLBAR_H
