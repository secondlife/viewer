/**
 * @file llfloaterimsessiontab.h
 * @brief LLFloaterIMSessionTab class implements the common behavior of LNearbyChatBar
 * @brief and LLFloaterIMSession for hosting both in LLIMContainer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
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

#ifndef LL_FLOATERIMSESSIONTAB_H
#define LL_FLOATERIMSESSIONTAB_H

#include "lllayoutstack.h"
#include "llparticipantlist.h"
#include "lltransientdockablefloater.h"
#include "llviewercontrol.h"
#include "lleventtimer.h"
#include "llimview.h"
#include "llconversationmodel.h"
#include "llconversationview.h"
#include "lltexteditor.h"

class LLPanelChatControlPanel;
class LLChatEntry;
class LLChatHistory;
class LLPanelEmojiComplete;

class LLFloaterIMSessionTab
    : public LLTransientDockableFloater
    , public LLIMSessionObserver
{
    using super = LLTransientDockableFloater;

public:
    LOG_CLASS(LLFloaterIMSessionTab);

    LLFloaterIMSessionTab(const LLSD& session_id);
    ~LLFloaterIMSessionTab();

    // reload all message with new settings of visual modes
    static void processChatHistoryStyleUpdate(bool clean_messages = false);
    static void reloadEmptyFloaters();

    /**
     * Returns true if chat is displayed in multi tabbed floater
     *         false if chat is displayed in multiple windows
     */
    static bool isChatMultiTab();

    // add conversation to container
    static void addToHost(const LLUUID& session_id);

    bool isHostAttached() {return mIsHostAttached;}
    void setHostAttached(bool is_attached) {mIsHostAttached = is_attached;}

    static LLFloaterIMSessionTab* findConversation(const LLUUID& uuid);
    static LLFloaterIMSessionTab* getConversation(const LLUUID& uuid);

    bool isNearbyChat() {return mIsNearbyChat;}

    // LLFloater overrides
    void onOpen(const LLSD& key) override;
    bool postBuild() override;
    void draw() override;
    void setVisible(bool visible) override;
    void setFocus(bool focus) override;
    void closeFloater(bool app_quitting = false) override;
    void deleteAllChildren() override;

    virtual void onClickCloseBtn(bool app_quitting = false) override;

    // Handle the left hand participant list widgets
    void addConversationViewParticipant(LLConversationItem* item, bool update_view = true);
    void removeConversationViewParticipant(const LLUUID& participant_id);
    void updateConversationViewParticipant(const LLUUID& participant_id);
    void refreshConversation();
    void buildConversationViewParticipant();

    void setSortOrder(const LLConversationSort& order);
    virtual void onTearOffClicked();
    void updateGearBtn();
    void initBtns();
    virtual void updateMessages() {}
    LLConversationItem* getCurSelectedViewModelItem();
    void forceReshape();
    virtual bool handleKeyHere( KEY key, MASK mask ) override;
    bool isMessagePaneExpanded(){return mMessagePaneExpanded;}
    void setMessagePaneExpanded(bool expanded){mMessagePaneExpanded = expanded;}
    void restoreFloater();
    void saveCollapsedState();

    void updateChatIcon(const LLUUID& id);

    LLView* getChatHistory();

    // LLIMSessionObserver triggers
    virtual void sessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id, bool has_offline_msg) override {}; // Stub
    virtual void sessionActivated(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id) override {}; // Stub
    virtual void sessionRemoved(const LLUUID& session_id) override;
    virtual void sessionVoiceOrIMStarted(const LLUUID& session_id) override {};                              // Stub
    virtual void sessionIDUpdated(const LLUUID& old_session_id, const LLUUID& new_session_id) override {};   // Stub

protected:

    // callback for click on any items of the visual states menu
    void onIMSessionMenuItemClicked(const LLSD& userdata);

    // callback for check/uncheck of the expanded/collapse mode's switcher
    bool onIMCompactExpandedMenuItemCheck(const LLSD& userdata);

    //
    bool onIMShowModesMenuItemCheck(const LLSD& userdata);
    bool onIMShowModesMenuItemEnable(const LLSD& userdata);
    static void onSlide(LLFloaterIMSessionTab *self);
    static void onCollapseToLine(LLFloaterIMSessionTab *self);
    void reshapeFloater(bool collapse);

    // refresh a visual state of the Call button
    void updateCallBtnState(bool callIsActive);

    void hideOrShowTitle(); // toggle the floater's drag handle
    void hideAllStandardButtons();

    /// Update floater header and toolbar buttons when hosted/torn off state is toggled.
    void updateHeaderAndToolbar();

    // Update the input field help text and other places that need the session name
    virtual void updateSessionName(const std::string& name);

    // set the enable/disable state for the Call button
    virtual void enableDisableCallBtn();

    // process focus events to set a currently active session
    void onFocusReceived() override;
    void onFocusLost() override;

    // prepare chat's params and out one message to chatHistory
    void appendMessage(const LLChat& chat, const LLSD& args = LLSD());

    std::string appendTime();
    void assignResizeLimits();

    void updateUsedEmojis(LLWStringView text);

    S32  mFloaterExtraWidth;

    bool mIsNearbyChat;
    bool mIsP2PChat;

    bool mMessagePaneExpanded;
    bool mIsParticipantListExpanded;
    S32 mMinFloaterHeight;

    LLIMModel::LLIMSession* mSession;

    // Participants list: model and view
    LLConversationViewParticipant* createConversationViewParticipant(LLConversationItem* item);

    LLUUID mSessionID;
    LLView* mContentsView;
    LLLayoutStack* mBodyStack;
    LLLayoutStack* mParticipantListAndHistoryStack;
    LLLayoutPanel* mParticipantListPanel;   // add the widgets to that see mConversationsListPanel
    LLLayoutPanel* mRightPartPanel;
    LLLayoutPanel* mContentPanel;
    LLLayoutPanel* mToolbarPanel;
    LLLayoutPanel* mInputButtonPanel;
    LLLayoutPanel* mEmojiRecentPanel;
    LLTextBox* mEmojiRecentEmptyText;
    LLPanel* mEmojiRecentContainer;
    LLPanelEmojiComplete* mEmojiRecentIconsCtrl;
    LLParticipantList* getParticipantList();
    conversations_widgets_map mConversationsWidgets;
    LLConversationViewModel mConversationViewModel;
    LLFolderView* mConversationsRoot;
    LLScrollContainer* mScroller;

    LLChatHistory* mChatHistory;
    LLChatEntry* mInputEditor;
    LLLayoutPanel* mChatLayoutPanel;
    LLLayoutStack* mInputPanels;

    LLButton* mExpandCollapseLineBtn;
    LLButton* mExpandCollapseBtn;
    LLButton* mTearOffBtn;
    LLButton* mEmojiRecentPanelToggleBtn;
    LLButton* mEmojiPickerShowBtn;
    LLButton* mCloseBtn;
    LLButton* mGearBtn;
    LLButton* mAddBtn;
    LLButton* mVoiceButton;

    // Since mVoiceButton can work in one of two modes, "Start call" or "Hang up",
    // (with different images and tooltips depending on the currently chosen mode)
    // we should track the mode we're currently using to react on click accordingly
    bool mVoiceButtonHangUpMode { false };

private:
    // Handling selection and contextual menu
    void doToSelected(const LLSD& userdata);
    bool enableContextMenuItem(const LLSD& userdata);
    bool checkContextMenuItem(const LLSD& userdata);

    void getSelectedUUIDs(uuid_vec_t& selected_uuids);

    /// Refreshes the floater at a constant rate.
    virtual void refresh() override = 0;

    /**
     * Adjusts chat history height to fit vertically with input chat field
     * and avoid overlapping, since input chat field can be vertically expanded.
     * Implementation: chat history bottom "follows" top+top_pad of input chat field
     */
    void reshapeChatLayoutPanel();

    void onCallButtonClicked();

    void onInputEditorClicked();

    void onEmojiRecentPanelToggleBtnClicked();
    void onEmojiPickerShowBtnClicked();
    void onEmojiPickerShowBtnDown();
    void onEmojiPickerClosed();
    void initEmojiRecentPanel();
    void onEmojiRecentPanelFocusReceived();
    void onEmojiRecentPanelFocusLost();
    void onRecentEmojiPicked(const LLSD& value);

    bool checkIfTornOff();
    bool mIsHostAttached;
    bool mHasVisibleBeenInitialized;

    LLTimer* mRefreshTimer; ///< Defines the rate at which refresh() is called.

    S32 mInputEditorPad;
    S32 mChatLayoutPanelHeight;
    S32 mFloaterHeight;

    boost::signals2::connection mEmojiCloseConn;
    U32 mEmojiHelperLastCallbackFrame = { 0 };
};


#endif /* LL_FLOATERIMSESSIONTAB_H */
