/** 
* @file llbottomtray.h
* @brief LLBottomTray class header file
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLBOTTOMPANEL_H
#define LL_LLBOTTOMPANEL_H

#include "llpanel.h"
#include "llimview.h"
#include "llbutton.h"

class LLChicletPanel;
class LLLayoutStack;
class LLSpeakButton;
class LLNearbyChatBar;
class LLIMChiclet;
class LLBottomTrayLite;
class LLLayoutPanel;
class LLMenuGL;

// Build time optimization, generate once in .cpp file
#ifndef LLBOTTOMTRAY_CPP
extern template class LLBottomTray* LLSingleton<class LLBottomTray>::getInstance();
#endif

/**
 * Class for buttons that should have drag'n'drop ability in bottomtray.
 * These buttons pass mouse events handling to bottomtray.
 */
class LLBottomtrayButton : public LLButton
{
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
		Optional<bool> can_drag;
		Params()
		: can_drag("can_drag", true){}
	};
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleMouseDown(S32 x, S32 y, MASK mask);

protected:
	LLBottomtrayButton(const Params& p)
	:	LLButton(p),
		mCanDrag(p.can_drag)
	{

	}
	friend class LLUICtrlFactory;

	bool mCanDrag;
};

class LLBottomTray 
	: public LLSingleton<LLBottomTray>
	, public LLPanel
	, public LLIMSessionObserver
	, public LLVoiceClientStatusObserver
{
	LOG_CLASS(LLBottomTray);
	friend class LLSingleton<LLBottomTray>;
	friend class LLBottomTrayLite;
public:
	~LLBottomTray();

	BOOL postBuild();

	LLChicletPanel*		getChicletPanel()	{return mChicletPanel;}
	LLNearbyChatBar*		getNearbyChatBar();

	void onCommitGesture(LLUICtrl* ctrl);

	// LLIMSessionObserver observe triggers
	virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id);
	virtual void sessionRemoved(const LLUUID& session_id);
	void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id);

	S32 getTotalUnreadIMCount();

	virtual void reshape(S32 width, S32 height, BOOL called_from_parent);

	virtual void setVisible(BOOL visible);

	/*virtual*/ S32 notifyParent(const LLSD& info);

	// Implements LLVoiceClientStatusObserver::onChange() to enable the speak
	// button when voice is available
	/*virtual*/ void onChange(EStatusType status, const std::string &channelURI, bool proximal);

	void showBottomTrayContextMenu(S32 x, S32 y, MASK mask);

	void showGestureButton(BOOL visible);
	void showMoveButton(BOOL visible);
	void showCameraButton(BOOL visible);
	void showSnapshotButton(BOOL visible);
	void showSpeakButton(bool visible);

	void toggleMovementControls();
	void toggleCameraControls();

	void onMouselookModeIn();
	void onMouselookModeOut();

	/**
	 * Creates IM Chiclet based on session type (IM chat or Group chat)
	 */
	LLIMChiclet* createIMChiclet(const LLUUID& session_id);

	// Below are methods that were introduced or overriden in bottomtray to handle drag'n'drop

	virtual void draw();

	/**
	 * These three methods handle drag'n'drop, they may be called directly from child buttons.
	 * handleHover and other virtual handle* couldn't be used here, because we should call LLPanel::handle*,
	 * but x and y here are often outside of bottomtray.
	 */
	void onDraggableButtonHover(S32 x, S32 y);
	void onDraggableButtonMouseDown(LLUICtrl* button, S32 x, S32 y);
	void onDraggableButtonMouseUp(LLUICtrl* button, S32 x, S32 y);


private:
	typedef enum e_resize_status_type
	{
		  RS_NORESIZE			= 0x0000
		, RS_CHICLET_PANEL		= 0x0001
		, RS_CHATBAR_INPUT		= 0x0002
		, RS_BUTTON_SNAPSHOT	= 0x0004
		, RS_BUTTON_CAMERA		= 0x0008
		, RS_BUTTON_MOVEMENT	= 0x0010
		, RS_BUTTON_GESTURES	= 0x0020
		, RS_BUTTON_SPEAK		= 0x0040
		, RS_IM_WELL			= 0x0080
		, RS_NOTIFICATION_WELL	= 0x0100
		, RS_BUTTON_BUILD		= 0x0200
		, RS_BUTTON_SEARCH		= 0x0400
		, RS_BUTTON_WORLD_MAP	= 0x0800
		, RS_BUTTON_MINI_MAP	= 0x1000

		/*
		Once new button that can be hidden on resize is added don't forget to update related places:
			- RS_BUTTONS_CAN_BE_HIDDEN enum value below.
			- initResizeStateContainers(): mStateProcessedObjectMap and mButtonsProcessOrder
		*/

		/**
		 * Specifies buttons which can be hidden when bottom tray is shrunk.
		 * They are: Gestures, Movement (Move), Camera (View), Snapshot
		 *		new: Build, Search, Map, World Map, Mini-Map.
		 */
		, RS_BUTTONS_CAN_BE_HIDDEN = RS_BUTTON_SNAPSHOT | RS_BUTTON_CAMERA | RS_BUTTON_MOVEMENT | RS_BUTTON_GESTURES
									| RS_BUTTON_BUILD | RS_BUTTON_SEARCH | RS_BUTTON_WORLD_MAP | RS_BUTTON_MINI_MAP
	}EResizeState;

	// Below are three methods that were introduced to handle drag'n'drop

	/**
	 * finds a panel under the specified LOCAL point
	 */
	LLPanel* findChildPanelByLocalCoords(S32 x, S32 y);

	/**
	 * checks whether the cursor is over an area where the dragged button may be dropped
	 */
	bool isCursorOverDraggableArea(S32 x, S32 y);

	/**
	 * Updates process(shrink/show/hide) order of buttons and order in which they'll be stored for further save/load.
	 * It is called when dragged button is dropped
	 */
	void updateButtonsOrdersAfterDnD();

	// saves order of buttons to file on disk
	void saveButtonsOrder();
	// reads order of buttons from file on disk
	void loadButtonsOrder();

	/**
	 * Updates child controls size and visibility when it is necessary to reduce total width.
	 *
	 * Process order:
	 *	- reduce chiclet panel to its minimal width;
	 *  - reduce chatbar to its minimal width;
	 *  - reduce visible buttons from right to left to their minimal width;
	 *  - hide visible buttons from right to left;
	 * When button is hidden chatbar extended to fill released space if it is necessary.
	 *
	 * @param[in] delta_width - value by which bottom tray should be shrunk. It is a negative value.
	 * @return positive value which bottom tray can not process when it reaches its minimal width.
	 *		Zero if there was enough space to process delta_width.
	 */
	S32 processWidthDecreased(S32 delta_width);

	/**
	 * Updates child controls size and visibility when it is necessary to extend total width.
	 *
	 * Process order:
	 *  - show invisible buttons should be shown from left to right if possible;
	 *  - extend visible buttons from left to right to their default width;
	 *  - extend chatbar to its maximal width;
	 *	- extend chiclet panel to all available space;
	 * When chatbar & chiclet panels are wider then their minimal width they can be reduced to allow
	 * a button gets visible in case if passed delta_width is not enough (chatbar first).
	 *
	 * @param[in] delta_width - value by which bottom tray should be extended. It is a positive value.
	 */
	void processWidthIncreased(S32 delta_width);

	/** helper function to log debug messages */
	void log(LLView* panel, const std::string& descr);

	/**
	 * Tries to show hidden by resize buttons using available width.
	 *
	 * Gets buttons visible if there is enough space. Reduces available_width in this case.
	 *
	 * @params[in, out] available_width - reference to available width to be used to show buttons.
	 * @see processShowButton()
	 * @return consumed pixels (difference in available width).
	 */
	S32 processShowButtons(S32& available_width);

	/**
	 * Tries to show panel with specified button using available width.
	 *
	 * Shows button specified by type if there is enough space. Reduces available_width in this case.
	 *
	 * @params[in] shown_object_type - type of button to be shown.
	 * @params[in, out] available_width - reference to available width to be used to show button.
	 *
	 * @return true if button can be shown, false otherwise
	 */
	bool processShowButton(EResizeState shown_object_type, S32& available_width);

	/**
	 * Hides visible panels with all buttons that may be hidden by resize if it is necessary.
	 *
	 * When button gets hidden some space is released in bottom tray.
	 * This space is taken into account for several consecutive calls for several buttons.
	 *
	 * @params[in, out] required_width - reference to required width to be released. This is a negative value.
	 *			Its absolute value is decreased by shown panel width.
	 * @params[in, out] buttons_freed_width - reference to value released over required one.
	 *			If panel's width is more than required difference is added to buttons_freed_width.
	 * @see processHideButton()
	 */
	void processHideButtons(S32& required_width, S32& buttons_freed_width);

	/**
	 * Hides panel with specified button if it is visible.
	 *
	 * When button gets hidden some space is released in bottom tray.
	 * This space is taken into account for several consecutive calls for several buttons.
	 *
	 * @params[in] processed_object_type - type of button to be hide.
	 * @params[in, out] required_width - reference to required width to be released. This is a negative value.
	 *			Its absolute value is decreased by panel width.
	 * @params[in, out] buttons_freed_width - reference to value released over required one.
	 *			If panel's width is more than required difference is added to buttons_freed_width.
	 */
	void processHideButton(EResizeState processed_object_type, S32& required_width, S32& buttons_freed_width);

	/**
	 * Shrinks shown buttons to reduce total taken space.
	 *
	 * Shrinks buttons that may be shrunk smoothly first. Then shrinks Speak button.
	 *
	 * @param[in, out] required_width - reference to width value which should be released when buttons are shrunk. It is a negative value.
	 * It is increased on the value processed by buttons.
	 * @params[in, out] buttons_freed_width - reference to value released over required one.
	 *			If width of panel with Speak button is more than required that difference is added
	 *				to buttons_freed_width.
	 *			This is because Speak button shrinks discretely unlike other buttons which are changed smoothly.
	 */
	void processShrinkButtons(S32& required_width, S32& buttons_freed_width);

	/**
	 * Shrinks panel with specified button if it is visible.
	 *
	 * @params[in] processed_object_type - type of button to be shrunk.
	 * @param[in, out] required_width - reference to width value which should be released when button is shrunk. It is a negative value.
	 * It is increased on the value released by the button.
	 */
	void processShrinkButton(EResizeState processed_object_type, S32& required_width);

	/**
	 * Extends shown buttons to increase total taken space.
	 *
	 * Extends buttons that may be extended smoothly first. Then extends Speak button.
	 *
	 * @param[in, out] available_width - reference to width value which buttons can use to be extended.
	 *		It is a positive value. It is decreased on the value processed by buttons.
	 */
	void processExtendButtons(S32& available_width);

	/**
	 * Extends the Speak button if there is anough headroom.
	 *
	 * Unlike other buttons, the Speak buttons has only two possible widths:
	 * the minimal one (without label) and the maximal (default) one.
	 *
	 * If the button is at its minimum width there is not enough headroom to
	 * reshape it to the maximum width, the method does nothing.
	 *
	 * @param available_width Available headroom.
	 * @return false if the button requires extension but there's not enough headroom, true otherwise.
	 */
	bool processExtendSpeakButton(S32& available_width);

	/**
	 * Extends shown button to increase total taken space.
	 *
	 * @params[in] processed_object_type - type of button to be extended.
	 * @param[in, out] available_width - reference to width value which button can use to be extended.
	 *		It is a positive value. It is decreased on the value processed by buttons.
	 */
	void processExtendButton(EResizeState processed_object_type, S32& available_width);

	/**
	 * Determines if specified by type object can be shown. It should be hidden by shrink before.
	 *
	 * Processes buttons a such way to show buttons in constant order:
	 *   - Gestures, Move, View, Snapshot
	 */
	bool canButtonBeShown(EResizeState processed_object_type) const;

	/**
	 * Initializes all containers stored data related to children resize state.
	 *
	 * @see mStateProcessedObjectMap
	 * @see mObjectDefaultWidthMap
	 * @see mButtonsProcessOrder
	 */
	void initResizeStateContainers();

	/**
	 * Initializes buttons' visibility depend on stored Control Settings.
	 */
	void initButtonsVisibility();

	/**
	 * Initializes listeners of Control Settings to toggle appropriate buttons' visibility.
	 *
	 * @see toggleShowButton()
	 */
	void setButtonsControlsAndListeners();

	/**
	 * Toggles visibility of specified button depend on passed value.
	 *
	 * @param button_type - type of button to be toggled
	 * @param new_visibility - new visibility of the button
	 *
	 * @see setButtonsControlsAndListeners()
	 */
	static bool toggleShowButton(EResizeState button_type, const LLSD& new_visibility);

	/**
	 * Show the button if there is enough space.
	 *
	 * @param[in]      button_type -    type of button to be shown.
	 * @param[in, out] available_width  amount of available space on the bottom bar.
	 *
	 * @return true if button was shown, false that's not possible (not enough space, etc)
	 */
	bool showButton(EResizeState button_type, S32& available_width);

	/**
	 * Sets passed visibility to object specified by resize type.
	 */
	void setTrayButtonVisible(EResizeState shown_object_type, bool visible);

	/**
	 * Sets passed visibility to object specified by resize type if it is possible.
	 *
	 * If it is impossible to show required button due to there is no enough room in bottom tray
	 * it will no be shown. Is called via context menu commands.
	 * In this case Alert Dialog will be shown to notify user about that.
	 *
	 * Method also stores resize state to be processed while future bottom tray extending:
	 *  - if hidden while resizing button should be hidden it will not be shown while extending;
	 *  - if hidden via context menu button should be shown but there is no enough room for now
	 *    it will be shown while extending.
	 */
	void setTrayButtonVisibleIfPossible(EResizeState shown_object_type, bool visible, bool raise_notification = true);

	/**
	 * Sets passed visibility to required button and fit widths of shown
	 * buttons(notice that method can shrink widths to
	 * allocate needed room in bottom tray).
	 * Returns true if visibility of required button was set.
	 */
	bool setVisibleAndFitWidths(EResizeState object_type, bool visible);

	/**
	 * Get panel containing the given button.
	 *
	 * @see mStateProcessedObjectMap
	 */
	LLPanel* getButtonPanel(EResizeState button_type);

	/**
	 * Shows/hides panel with specified well button (IM or Notification)
	 *
	 * @param[in] object_type - type of well button to be processed.
	 *		Must be one of RS_IM_WELL or RS_NOTIFICATION_WELL.
	 * @param[in] visible - flag specified whether button should be shown or hidden.
	 */
	void showWellButton(EResizeState object_type, bool visible);

	/**
	 * Handles a customization of chatbar width.
	 *
	 * When chatbar gets wider layout stack will reduce chiclet panel (it is auto-resizable)
	 *	But once chiclet panel reaches its minimal width Stack will force to reduce buttons width.
	 *	including Speak button. The similar behavior is when chatbar gets narrowly.
	 * This methods force resize behavior to resize buttons properly in these cases.
	 */
	void processChatbarCustomization(S32 new_width);

	/**
	 * @return difference between current chiclet panel width and the minimum.
	 */
	S32 getChicletPanelShrinkHeadroom() const;

	/// Get button name for debugging.
	static std::string resizeStateToString(EResizeState state);

	/// Dump a mask for debugging
	static std::string resizeStateMaskToString(MASK mask);

	/// Buttons automatically hidden due to lack of space.
	MASK mResizeState;

	/**
	 * Mapping of button types to the layout panels the buttons are wrapped in.
	 *
	 * Used by getButtonPanel().
	 */
	typedef std::map<EResizeState, LLPanel*> state_object_map_t;
	state_object_map_t mStateProcessedObjectMap;

	/// Default (maximum) widths of the layout panels.
	typedef std::map<EResizeState, S32> state_object_width_map_t;
	state_object_width_map_t mObjectDefaultWidthMap;

	typedef std::vector<EResizeState> resize_state_vec_t;

	/**
	 * Contains order in which child buttons should be processed in show/hide, extend/shrink methods.
	 */
	resize_state_vec_t mButtonsProcessOrder;

	/**
	 * Contains order in which child buttons are shown.
	 * It traces order of all bottomtray buttons that may change place via drag'n'drop and should
	 * save and load it between sessions. mButtonsProcessOrder is not enough for it because it contains only
	 * buttons that may be hidden.
	 */
	resize_state_vec_t mButtonsOrder;

protected:

	LLBottomTray(const LLSD& key = LLSD());

	static void* createNearbyChatBar(void* userdata);

	void updateContextMenu(S32 x, S32 y, MASK mask);
	void onContextMenuItemClicked(const LLSD& userdata);
	bool onContextMenuItemEnabled(const LLSD& userdata);

	// Either default or saved after user's manual resize width of nearby chat.
	// Nearby chat will not always have it, because sometimes it can be shrunk on resize,
	// but when possible it will be restored back to this value.
	S32					mDesiredNearbyChatWidth;
	LLChicletPanel* 	mChicletPanel;
	LLPanel*			mSpeakPanel;
	LLSpeakButton* 		mSpeakBtn;
	LLNearbyChatBar*	mNearbyChatBar;
	LLLayoutPanel*		mChatBarContainer;
	LLPanel*		mNearbyCharResizeHandlePanel;
	LLLayoutStack*		mToolbarStack;
	LLMenuGL*			mBottomTrayContextMenu;
	LLButton*			mCamButton;
	LLButton*			mMovementButton;
	LLBottomTrayLite*   mBottomTrayLite;
	bool                mIsInLiteMode;

	// Drag'n'Drop

	/**
	 * Is true if mouse down happened on draggable button.
	 * Set false whether on drag start or on mouse up.
	 */
	bool mCheckForDrag;
	/**
	 * These two variables hold corrdinates of mouse down on draggable button.
	 * They are used to compare with current coordinates of cursor and determine whether drag'n'drop should start.
	 */
	S32 mStartX;
	S32 mStartY;
	/**
	 * True if drag'n'drop is happening.
	 */
	bool mDragStarted;

	/**
	 * Pointer to panel which is currently dragged (though it seems to user that button is dragged,
	 * we are changing place of layout panel).
	 */
	LLPanel* mDraggedItem;
	/**
	 * Panel before which the dragged button will be inserted.
	 */
	LLPanel* mLandingTab;
	/**
	 * Image used to show position where dragged button will be dropped.
	 */
	LLUIImage* mImageDragIndication;
};

#endif // LL_LLBOTTOMPANEL_H
