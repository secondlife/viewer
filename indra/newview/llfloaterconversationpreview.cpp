/**
 * @file llfloaterconversationpreview.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llavatarnamecache.h"
#include "llchatservicehistory.h"
#include "llconversationlog.h"
#include "llfloaterconversationpreview.h"
#include "llimview.h"
#include "lllineeditor.h"
#include "llfloaterimnearbychat.h"
#include "llspinctrl.h"
#include "lltrans.h"
#include "llnotificationsutil.h"
#include "workqueue.h"

const std::string LL_FCP_COMPLETE_NAME("complete_name");
const std::string LL_FCP_ACCOUNT_NAME("user_name");
const std::string LL_FCP_PARTICIPANT_ID("participant_id");
const S32 CONVERSATION_HISTORY_PAGE_SIZE = 100;

LLFloaterConversationPreview::LLFloaterConversationPreview(const LLSD& session_id)
:   LLFloater(session_id),
    mChatHistory(NULL),
    mSessionID(session_id.asUUID()),
    mParticipantID(session_id[LL_FCP_PARTICIPANT_ID].asUUID()),
    mCurrentPage(0),
    mPageSize(CONVERSATION_HISTORY_PAGE_SIZE),
    mAccountName(session_id[LL_FCP_ACCOUNT_NAME]),
    mCompleteName(session_id[LL_FCP_COMPLETE_NAME]),
    mShowHistory(false),
    mMessages(NULL),
    mHistoryThreadsBusy(false),
    mIsGroup(false),
    mIsP2P(false),
    mServiceToken(0),
    mOpened(false)
{
}

LLFloaterConversationPreview::~LLFloaterConversationPreview()
{
    delete mMessages;
}

bool LLFloaterConversationPreview::postBuild()
{
    mChatHistory = getChild<LLChatHistory>("chat_history");

    const LLConversation* conv = LLConversationLog::instance().getConversation(mSessionID);
    std::string name;
    std::string file;

    if (mParticipantID.notNull())
    {
        // A direct-conversation key stays direct while its shared name is unresolved.
        mIsP2P = true;
        name = mCompleteName;
        file = mAccountName;
    }
    else if (mAccountName != "")
    {
        name = mCompleteName;
        file = mAccountName;
    }
    else if (mSessionID != LLUUID::null && conv)
    {
        name = conv->getConversationName();
        file = conv->getHistoryFileName();
        mIsGroup = (LLIMModel::LLIMSession::GROUP_SESSION == conv->getConversationType());
        mIsP2P = (LLIMModel::LLIMSession::P2P_SESSION == conv->getConversationType());
        if (mIsP2P)
        {
            mParticipantID = conv->getParticipantID();
        }
    }
    else
    {
        name = LLTrans::getString("NearbyChatTitle");
        file = "chat";
    }

    mChatHistoryFileName = file;
    if (mIsGroup && !LLStringUtil::endsWith(mChatHistoryFileName, GROUP_CHAT_SUFFIX))
    {
        mChatHistoryFileName += GROUP_CHAT_SUFFIX;
    }

    LLStringUtil::format_map_t args;
    args["[NAME]"] = name;
    std::string title = getString("Title", args);
    setTitle(title);
    getChild<LLTextBox>("chat_service_loading_text")->setValue(
        LLTrans::getString("loading_chat_logs"));

    return LLFloater::postBuild();
}

void LLFloaterConversationPreview::setPages(std::list<LLSD>* messages, const std::string& file_name)
{
    if(file_name == mChatHistoryFileName && messages)
    {
        // Preserve the reader's distance from the newest page while asynchronous
        // reloads replace the backing list.
        const S32 old_last_page = mMessages && !mMessages->empty()
            ? (static_cast<S32>(mMessages->size()) - 1) / mPageSize : 0;
        const S32 distance_from_newest = llmax(0, old_last_page - mCurrentPage);
        if (mMessages)
        {
            delete mMessages; // Clean up temporary message list with "Loading..." text
        }
        mMessages = messages;
        const S32 last_page = mMessages->empty()
            ? 0 : (static_cast<S32>(mMessages->size()) - 1) / mPageSize;
        mCurrentPage = llmax(0, last_page - distance_from_newest);

        // Publish the new page range together so draw() never renders stale bounds.
        mPageSpinner->setEnabled(true);
        mPageSpinner->setMaxValue((F32)(last_page+1));
        // Refresh the displayed page even while the editor has focus.
        mPageSpinner->forceSetValue((F32)(mCurrentPage+1));
        mPageSpinner->resetDirty();

        std::string total_page_num = llformat("/ %d", last_page+1);
        getChild<LLTextBox>("page_num_label")->setValue(total_page_num);
        mShowHistory = true;
    }
}

void LLFloaterConversationPreview::draw()
{
    // Local reads and account-scoped service work share one nonmodal status panel.
    const bool loading = mIsP2P &&
        (mServiceHistory.isLoading() ||
         LLChatServiceHistory::getSnapshot(mParticipantID).service_work_active);
    // The widget animates in draw(); hiding its panel suspends that work.
    getChildView("chat_service_loading")->setVisible(loading);

    if(mShowHistory)
    {
        showHistory();
        mShowHistory = false;
    }

    LLFloater::draw();
}

void LLFloaterConversationPreview::onOpen(const LLSD& key)
{
    if (mOpened)
    {
        return;
    }

    mOpened = true;
    ++mServiceToken;
    mServiceHistory.clear();
    mPageSpinner = getChild<LLSpinCtrl>("history_page_spin");
    mPageSpinner->setCommitCallback(
        boost::bind(&LLFloaterConversationPreview::onMoreHistoryBtnClick, this));
    mPageSpinner->setMinValue(1);
    mPageSpinner->set(1);
    mPageSpinner->setEnabled(false);

    if (mIsP2P)
    {
        // The history owner publishes on the main loop and stops with this Preview.
        mServiceHistory.load(mParticipantID, mChatHistoryFileName, 10000,
            [this](const std::list<LLSD>& history)
            {
                setPages(new std::list<LLSD>(history), mChatHistoryFileName);
            });
        LLChatServiceHistory::prioritizeResident(mParticipantID);

        return;
    }

    // Legacy Preview loads once per open, with completion fenced by its open token.
    if (LLChatServiceHistory::historySuppressed())
    {
        return;
    }

    if (!LLLogChat::getInstance()->historyThreadsFinished(mSessionID))
    {
        LLNotificationsUtil::add("ChatHistoryIsBusyAlert");
        mHistoryThreadsBusy = true;
        closeFloater();
        return;
    }

    const U64 load_token = ++mServiceToken;

    LLSD load_params;
    load_params["load_all_history"] = true;
    load_params["cut_off_todays_date"] = false;
    load_params["is_group"] = mIsGroup;

    // The temporary message list with "Loading..." text
    // Will be deleted upon loading completion in setPages() method
    delete mMessages;
    mMessages = new std::list<LLSD>();


    LLSD loading;
    loading[LL_IM_TEXT] = LLTrans::getString("loading_chat_logs");
    mMessages->push_back(loading);

    // The actual message list to load from file
    // Will be deleted in a separate thread LLDeleteHistoryThread not to freeze UI
    // LLDeleteHistoryThread is started in destructor
    std::list<LLSD>* messages = new std::list<LLSD>();

    LLLogChat *log_chat_inst = LLLogChat::getInstance();
    log_chat_inst->cleanupHistoryThreads();

    LLLoadHistoryThread* loadThread = new LLLoadHistoryThread(mChatHistoryFileName, messages, load_params);
    LLHandle<LLFloaterConversationPreview> handle =
        getDerivedHandle<LLFloaterConversationPreview>();
    LL::WorkQueue::ptr_t main = LL::WorkQueue::getInstance("mainloop");
    loadThread->setLoadEndSignal(
        [handle, load_token, main](std::list<LLSD>* loaded, const std::string& file_name)
        {
            // Copy before the delete thread reclaims loader-owned data, then fence
            // the main-thread apply with the current open token and privacy gate.
            std::shared_ptr<std::list<LLSD>> copy =
                std::make_shared<std::list<LLSD>>(*loaded);
            if (main)
            {
                main->post([handle, load_token, copy, file_name]()
                {
                    LLFloaterConversationPreview* floater = handle.get();
                    if (floater && floater->mOpened &&
                        load_token == floater->mServiceToken &&
                        !LLChatServiceHistory::historySuppressed())
                    {
                        floater->setPages(new std::list<LLSD>(*copy), file_name);
                    }
                });
            }
        });
    loadThread->start();
    log_chat_inst->addLoadHistoryThread(mSessionID, loadThread);

    LLDeleteHistoryThread* deleteThread = new LLDeleteHistoryThread(messages, loadThread);
    log_chat_inst->addDeleteHistoryThread(mSessionID, deleteThread);

    mShowHistory = true;
}

void LLFloaterConversationPreview::onClose(bool app_quitting)
{
    // Close is a lifecycle fence for callbacks, not a request to cancel shared service work.
    mOpened = false;
    ++mServiceToken;
    mServiceHistory.stop();

    if (mIsP2P)
    {
        return;
    }
    if (!mHistoryThreadsBusy)
    {
        LLDeleteHistoryThread* deleteThread = LLLogChat::getInstance()->getDeleteHistoryThread(mSessionID);
        if (deleteThread)
        {
            deleteThread->start();
        }
    }
}

void LLFloaterConversationPreview::invalidateHistory()
{
    // Advancing the token invalidates every outstanding legacy or stitched completion
    // before deletion clears the visible backing lists.
    ++mServiceToken;
    mServiceHistory.clear();

    if (mMessages)
    {
        mMessages->clear();
    }

    if (mChatHistory)
    {
        mChatHistory->clear();
    }
}

void LLFloaterConversationPreview::showHistory()
{
    // Both loaders publish on the main loop, so pages stay stable during rendering.
    mChatHistory->clear();
    if(mMessages == NULL || !mMessages->size() || mCurrentPage * mPageSize >= mMessages->size())
    {
        return;
    }
    std::ostringstream message;
    std::list<LLSD>::const_iterator iter = mMessages->begin();
    std::advance(iter, mCurrentPage * mPageSize);

    for (int msg_num = 0; iter != mMessages->end() && msg_num < mPageSize; ++iter, ++msg_num)
    {
        LLSD msg = *iter;

        LLUUID from_id      = LLUUID::null;
        std::string time    = msg["time"].asString();
        std::string from    = msg["from"].asString();
        std::string message = msg["message"].asString();

        if (msg["from_id"].isDefined())
        {
            from_id = msg["from_id"].asUUID();
        }
        else
        {
            std::string legacy_name = gCacheName->buildLegacyName(from);
            from_id = LLAvatarNameCache::getInstance()->findIdByName(legacy_name);
        }

        LLChat chat;
        chat.mFromID = from_id;
        chat.mSessionID = mSessionID;
        chat.mFromName = from;
        chat.mTimeStr = time;
        chat.mChatStyle = CHAT_STYLE_HISTORY;
        chat.mText = message;

        if (from_id.isNull() && SYSTEM_FROM == from)
        {
            chat.mSourceType = CHAT_SOURCE_SYSTEM;

        }
        else if (from_id.isNull())
        {
            chat.mSourceType = LLFloaterIMNearbyChat::isWordsName(from) ? CHAT_SOURCE_UNKNOWN : CHAT_SOURCE_OBJECT;
        }

        LLSD chat_args;
        chat_args["use_plain_text_chat_history"] =
                        gSavedSettings.getBOOL("PlainTextChatHistory");
        chat_args["show_time"] = gSavedSettings.getBOOL("IMShowTime");
        chat_args["show_names_for_p2p_conv"] = gSavedSettings.getBOOL("IMShowNamesForP2PConv");

        mChatHistory->appendMessage(chat,chat_args);
    }
}

void LLFloaterConversationPreview::onMoreHistoryBtnClick()
{
    mCurrentPage = (int)(mPageSpinner->getValueF32());
    if (!mCurrentPage)
    {
        return;
    }

    mCurrentPage--;
    mShowHistory = true;
}
