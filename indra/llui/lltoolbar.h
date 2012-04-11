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
class LLToolBarButton;

typedef boost::function<void (S32 x, S32 y, LLToolBarButton* button)> tool_startdrag_callback_t;
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
	void setEnabled(BOOL enabled);
	void setCommandId(const LLCommandId& id) { mId = id; }
	LLCommandId getCommandId() { return mId; }

	void setStartDragCallback(tool_startdrag_callback_t cb)   { mStartDragItemCallback  = cb; }
	void setHandleDragCallback(tool_handledrag_callback_t cb) { mHandleDragItemCallback = cb; }

	void onMouseEnter(S32 x, S32 y, MASK mask);
	void onMouseLeave(S32 x, S32 y, MASK mask);
	void onMouseCaptureLost();

	void onCommit();

	virtual const std::string getToolTip() const;		

private:
	void callIfEnabled(LLUICtrl::commit_callback_t commit, LLUICtrl* ctrl, const LLSD& param );

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
	LLPointer<LLUIImage>	mOriginalImageSelected,
							mOriginalImageUnselected,
							mOriginalImagePressed,
							mOriginalImagePressedSelected;
	LLUIColor				mOriginalLabelColor,
							mOriginalLabelColorSelected,
							mOriginalImageOverlayColor,
							mOriginalImageOverlaySelectedColor;
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

	LLLayoutStack::ELayoutOrientation getOrientation(SideType sideType);
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
	friend class LLToolBarButton;
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

		// default command set
		Multiple<LLCommandId::Params>			commands;

		Optional<LLPanel::Params>				button_panel;

		Params();
	};

	// virtuals
	void draw();
	void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	BOOL handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
	
	static const int RANK_NONE = -1;
	bool addCommand(const LLCommandId& commandId, int rank = RANK_NONE);
	int  removeCommand(const LLCommandId& commandId);		// Returns the rank the removed command was at, RANK_NONE if not found
	bool hasCommand(const LLCommandId& commandId) const;	// is this command bound to a button in this toolbar
	bool enableCommand(const LLCommandId& commandId, bool enabled);	// enable/disable button bound to the specified command, if it exists in this toolbar
	bool stopCommandInProgress(const LLCommandId& commandId);	// stop command if it is currently active
	bool flashCommand(const LLCommandId& commandId, bool flash); // flash button associated with given command, if in this toolbar

	void setStartDragCallback(tool_startdrag_callback_t cb)   { mStartDragItemCallback  = cb; } // connects drag and drop behavior to external logic
	void setHandleDragCallback(tool_handledrag_callback_t cb) { mHandleDragItemCallback = cb; }
	void setHandleDropCallback(tool_handledrop_callback_t cb) { mHandleDropCallback     = cb; }
	bool isReadOnly() const { return mReadOnly; }

	LLToolBarButton* createButton(const LLCommandId& id);

	typedef boost::signals2::signal<void (LLView* button)> button_signal_t;
	boost::signals2::connection setButtonAddCallback(const button_signal_t::slot_type& cb);
	boost::signals2::connection setButtonEnterCallback(const button_signal_t::slot_type& cb);
	boost::signals2::connection setButtonLeaveCallback(const button_signal_t::slot_type& cb);
	boost::signals2::connection setButtonRemoveCallback(const button_signal_t::slot_type& cb);

	// append the specified string to end of tooltip
	void setTooltipButtonSuffix(const std::string& suffix) { mButtonTooltipSuffix = suffix; } 

	LLToolBarEnums::SideType getSideType() const { return mSideType; }
	bool hasButtons() const { return !mButtons.empty(); }
	bool isModified() const { return mModified; }

	int  getRankFromPosition(S32 x, S32 y);	
	int  getRankFromPosition(const LLCommandId& id);	

	// Methods used in loading and saving toolbar settings
	void setButtonType(LLToolBarEnums::ButtonType button_type);
	LLToolBarEnums::ButtonType getButtonType() { return mButtonType; }
	command_id_list_t& getCommandsList() { return mButtonCommands; }
	void clearCommandsList();

private:
	friend class LLUICtrlFactory;
	LLToolBar(const Params&);
	~LLToolBar();

	void initFromParams(const Params&);
	void createContextMenu();
	void updateLayoutAsNeeded();
	void createButtons();
	void resizeButtonsInRow(std::vector<LLToolBarButton*>& buttons_in_row, S32 max_row_girth);
	BOOL isSettingChecked(const LLSD& userdata);
	void onSettingEnable(const LLSD& userdata);
	void onRemoveSelectedCommand();

private:
	// static layout state
	const bool						mReadOnly;
	const LLToolBarEnums::SideType	mSideType;
	const bool						mWrap;
	const S32						mPadLeft,
									mPadRight,
									mPadTop,
									mPadBottom,
									mPadBetween,
									mMinGirth;

	// drag and drop state
	tool_startdrag_callback_t		mStartDragItemCallback;
	tool_handledrag_callback_t		mHandleDragItemCallback;
	tool_handledrop_callback_t		mHandleDropCallback;
	bool							mDragAndDropTarget;
	int								mDragRank;
	S32								mDragx,
									mDragy,
									mDragGirth;

	typedef std::list<LLToolBarButton*> toolbar_button_list;
	typedef std::map<LLUUID, LLToolBarButton*> command_id_map;
	toolbar_button_list				mButtons;
	command_id_list_t				mButtonCommands;
	command_id_map					mButtonMap;

	LLToolBarEnums::ButtonType		mButtonType;
	LLToolBarButton::Params			mButtonParams[LLToolBarEnums::BTNTYPE_COUNT];

	// related widgets
	LLLayoutStack*					mCenteringStack;
	LLPanel*						mButtonPanel;
	LLHandle<class LLContextMenu>	mPopupMenuHandle;
	LLHandle<class LLView>			mRemoveButtonHandle;

	LLToolBarButton*				mRightMouseTargetButton;

	bool							mNeedsLayout;
	bool							mModified;

	button_signal_t*				mButtonAddSignal;
	button_signal_t*				mButtonEnterSignal;
	button_signal_t*				mButtonLeaveSignal;
	button_signal_t*				mButtonRemoveSignal;

	std::string						mButtonTooltipSuffix;
};


#endif  // LL_LLTOOLBAR_H
