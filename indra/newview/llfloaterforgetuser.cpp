/** 
 * @file llfloaterforgetuser.cpp
 * @brief LLFloaterForgetUser class definition.
 *
 * $LicenseInfo:firstyear=2019&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2019, Linden Research, Inc.
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

#include "llfloaterforgetuser.h"

#include "llcheckboxctrl.h"
#include "llfavoritesbar.h"
#include "llpanellogin.h"        // for helper function getUserName() and to repopulate list if nessesary
#include "llscrolllistctrl.h"
#include "llsecapi.h"
#include "llviewernetwork.h"


LLFloaterForgetUser::LLFloaterForgetUser(const LLSD &key)
    : LLFloater("floater_forget_user"),
    mLoginPanelDirty(false)
{

}

LLFloaterForgetUser::~LLFloaterForgetUser()
{
    if (mLoginPanelDirty)
    {
        LLPanelLogin::resetFields();
    }
}

BOOL LLFloaterForgetUser::postBuild()
{
    // Note, storage works per grid, watever is selected currently in login screen or logged in.
    // Since login screen can change grid, store the value.
    mGrid = LLGridManager::getInstance()->getGrid();

    LLScrollListCtrl *scroll_list = getChild<LLScrollListCtrl>("user_list");
    if (gSecAPIHandler->hasCredentialMap("login_list", mGrid))
    {
        LLSecAPIHandler::credential_map_t credencials;
        gSecAPIHandler->loadCredentialMap("login_list", mGrid, credencials);

        LLSecAPIHandler::credential_map_t::iterator cr_iter = credencials.begin();
        LLSecAPIHandler::credential_map_t::iterator cr_end = credencials.end();
        while (cr_iter != cr_end)
        {
            if (cr_iter->second.notNull()) // basic safety
            {
                LLScrollListItem::Params item_params;
                item_params.value(cr_iter->first);
                item_params.columns.add()
                    .value(LLPanelLogin::getUserName(cr_iter->second))
                    .column("user")
                    .font(LLFontGL::getFontSansSerifSmall());
                scroll_list->addRow(item_params, ADD_BOTTOM);
            }
            cr_iter++;
        }
        scroll_list->selectFirstItem();
    }
    else
    {
        LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(mGrid);
        if (cred.notNull())
        {
            LLScrollListItem::Params item_params;
            item_params.value(cred->userID());
            item_params.columns.add()
                .value(LLPanelLogin::getUserName(cred))
                .column("user")
                .font(LLFontGL::getFontSansSerifSmall());
            scroll_list->addRow(item_params, ADD_BOTTOM);
            scroll_list->selectFirstItem();
        }
    }

    bool enable_button = scroll_list->getFirstSelectedIndex() != -1;
    LLCheckBoxCtrl *chk_box = getChild<LLCheckBoxCtrl>("delete_data");
    chk_box->setEnabled(enable_button);
    chk_box->set(FALSE);
    LLButton *button = getChild<LLButton>("forget");
    button->setEnabled(enable_button);
    button->setCommitCallback(boost::bind(&LLFloaterForgetUser::onForgetClicked, this));

    return TRUE;
}

void LLFloaterForgetUser::onForgetClicked()
{
    mLoginPanelDirty = true;
    LLScrollListCtrl *scroll_list = getChild<LLScrollListCtrl>("user_list");
    std::string user_key = scroll_list->getSelectedValue();

    // remove creds
    gSecAPIHandler->removeFromCredentialMap("login_list", mGrid, user_key);

    LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(mGrid);
    if (cred.notNull() && cred->userID() == user_key)
    {
        gSecAPIHandler->deleteCredential(cred);
    }

    // Clean data
    LLCheckBoxCtrl *chk_box = getChild<LLCheckBoxCtrl>("delete_data");
    BOOL delete_data = chk_box->getValue();
    if (delete_data)
    {
        // key is edentical to one we use for name of user's folder
        std::string user_path = gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter() + user_key;
        gDirUtilp->deleteDirAndContents(user_path);

        // Clean favorites, label is edentical to username
        LLFavoritesOrderStorage::removeFavoritesRecordOfUser(scroll_list->getSelectedItemLabel(), mGrid);

        // Note: we do not clean user-related files from cache because there are id dependent (inventory)
        // files and cache has separate cleaning mechanism either way.
        // Also this only cleans user from current grid, not all of them.
    }


    // Update UI
    scroll_list->deleteSelectedItems();
    scroll_list->selectFirstItem();
    if (scroll_list->getFirstSelectedIndex() == -1)
    {
        LLButton *button = getChild<LLButton>("forget");
        button->setEnabled(false);
        chk_box->setEnabled(false);
    }
}


