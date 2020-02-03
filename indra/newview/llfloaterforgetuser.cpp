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

#include "llappviewer.h"
#include "llcheckboxctrl.h"
#include "llfavoritesbar.h"
#include "llnotificationsutil.h"
#include "llpanellogin.h"        // for helper function getUserName() and to repopulate list if nessesary
#include "llscrolllistctrl.h"
#include "llsecapi.h"
#include "llstartup.h"
#include "llviewercontrol.h"
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
    mScrollList = getChild<LLScrollListCtrl>("user_list");


    bool show_grid_marks = gSavedSettings.getBOOL("ForceShowGrid");
    show_grid_marks |= !LLGridManager::getInstance()->isInProductionGrid();

    std::map<std::string, std::string> known_grids = LLGridManager::getInstance()->getKnownGrids();

    if (!show_grid_marks)
    {
        // Figure out if there are records for more than one grid in storage
        for (std::map<std::string, std::string>::iterator grid_iter = known_grids.begin();
            grid_iter != known_grids.end();
            grid_iter++)
        {
            if (!grid_iter->first.empty()
                && grid_iter->first != MAINGRID) // a workaround since 'mIsInProductionGrid' might not be set
            {
                if (!gSecAPIHandler->emptyCredentialMap("login_list", grid_iter->first))
                {
                    show_grid_marks = true;
                    break;
                }

                // "Legacy" viewer support
                LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(grid_iter->first);
                if (cred.notNull())
                {
                    const LLSD &ident = cred->getIdentifier();
                    if (ident.isMap() && ident.has("type"))
                    {
                        show_grid_marks = true;
                        break;
                    }
                }
            }
        }
    }

    mUserGridsCount.clear();
    if (!show_grid_marks)
    {
        // just load maingrid
        loadGridToList(MAINGRID, false);
    }
    else
    {
        for (std::map<std::string, std::string>::iterator grid_iter = known_grids.begin();
            grid_iter != known_grids.end();
            grid_iter++)
        {
            if (!grid_iter->first.empty())
            {
                loadGridToList(grid_iter->first, true);
            }
        }
    }

    mScrollList->selectFirstItem();
    bool enable_button = mScrollList->getFirstSelectedIndex() != -1;
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
    LLScrollListCtrl *scroll_list = getChild<LLScrollListCtrl>("user_list");
    LLSD user_data = scroll_list->getSelectedValue();
    const std::string user_id = user_data["user_id"];

    LLCheckBoxCtrl *chk_box = getChild<LLCheckBoxCtrl>("delete_data");
    BOOL delete_data = chk_box->getValue();

    if (delete_data && mUserGridsCount[user_id] > 1)
    {
        // more than 1 grid uses this id
        LLNotificationsUtil::add("LoginRemoveMultiGridUserData", LLSD(), LLSD(), boost::bind(&LLFloaterForgetUser::onConfirmForget, this, _1, _2));
        return;
    }

    processForgetUser();
}

bool LLFloaterForgetUser::onConfirmForget(const LLSD& notification, const LLSD& response)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        processForgetUser();
    }
    return false;
}

// static 
bool LLFloaterForgetUser::onConfirmLogout(const LLSD& notification, const LLSD& response, const std::string &fav_id, const std::string &grid)
{
    S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
    if (option == 0)
    {
        // Remove creds
        gSecAPIHandler->removeFromCredentialMap("login_list", grid, LLStartUp::getUserId());

        LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(grid);
        if (cred.notNull() && cred->userID() == LLStartUp::getUserId())
        {
            gSecAPIHandler->deleteCredential(cred);
        }

        // Clean favorites
        LLFavoritesOrderStorage::removeFavoritesRecordOfUser(fav_id, grid);

        // mark data for removal
        LLAppViewer::instance()->purgeUserDataOnExit();
        LLAppViewer::instance()->requestQuit();
    }
    return false;
}

void LLFloaterForgetUser::processForgetUser()
{
    LLScrollListCtrl *scroll_list = getChild<LLScrollListCtrl>("user_list");
    LLCheckBoxCtrl *chk_box = getChild<LLCheckBoxCtrl>("delete_data");
    BOOL delete_data = chk_box->getValue();
    LLSD user_data = scroll_list->getSelectedValue();
    const std::string user_id = user_data["user_id"];
    const std::string grid = user_data["grid"];
    const std::string user_name = user_data["label"]; // for favorites

    if (delete_data && user_id == LLStartUp::getUserId() && LLStartUp::getStartupState() > STATE_LOGIN_WAIT)
    {
        // we can't delete data for user that is currently logged in
        // we need to pass grid because we are deleting data universal to grids, but specific grid's user
        LLNotificationsUtil::add("LoginCantRemoveCurUsername", LLSD(), LLSD(), boost::bind(onConfirmLogout, _1, _2, user_name, grid));
        return;
    }

    // key is used for name of user's folder and in credencials
    // user_name is edentical to favorite's username
    forgetUser(user_id, user_name, grid, delete_data);
    mLoginPanelDirty = true;
    if (delete_data)
    {
        mUserGridsCount[user_id] = 0; //no data left to care about
    }
    else
    {
        mUserGridsCount[user_id]--;
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

//static
void LLFloaterForgetUser::forgetUser(const std::string &userid, const std::string &fav_id, const std::string &grid, bool delete_data)
{
    // Remove creds
    gSecAPIHandler->removeFromCredentialMap("login_list", grid, userid);

    LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(grid);
    if (cred.notNull() && cred->userID() == userid)
    {
        gSecAPIHandler->deleteCredential(cred);
    }

    // Clean data
    if (delete_data)
    {
        std::string user_path = gDirUtilp->getOSUserAppDir() + gDirUtilp->getDirDelimiter() + userid;
        gDirUtilp->deleteDirAndContents(user_path);

        // Clean favorites
        LLFavoritesOrderStorage::removeFavoritesRecordOfUser(fav_id, grid);

        // Note: we do not clean user-related files from cache because there are id dependent (inventory)
        // files and cache has separate cleaning mechanism either way.
        // Also this only cleans user from current grid, not all of them.
    }
}

void LLFloaterForgetUser::loadGridToList(const std::string &grid, bool show_grid_name)
{
    std::string grid_label;
    if (show_grid_name)
    {
        grid_label = LLGridManager::getInstance()->getGridId(grid); //login id (shortened label)
    }
    if (gSecAPIHandler->hasCredentialMap("login_list", grid))
    {
        LLSecAPIHandler::credential_map_t credencials;
        gSecAPIHandler->loadCredentialMap("login_list", grid, credencials);

        LLSecAPIHandler::credential_map_t::iterator cr_iter = credencials.begin();
        LLSecAPIHandler::credential_map_t::iterator cr_end = credencials.end();
        while (cr_iter != cr_end)
        {
            if (cr_iter->second.notNull()) // basic safety
            {
                std::string user_label = LLPanelLogin::getUserName(cr_iter->second);
                LLSD user_data;
                user_data["user_id"] = cr_iter->first;
                user_data["label"] = user_label;
                user_data["grid"] = grid;

                if (show_grid_name)
                {
                    user_label += " (" + grid_label + ")";
                }

                LLScrollListItem::Params item_params;
                item_params.value(user_data);
                item_params.columns.add()
                    .value(user_label)
                    .column("user")
                    .font(LLFontGL::getFontSansSerifSmall());
                mScrollList->addRow(item_params, ADD_BOTTOM);

                // Add one to grid count
                std::map<std::string, S32>::iterator found = mUserGridsCount.find(cr_iter->first);
                if (found != mUserGridsCount.end())
                {
                    found->second++;
                }
                else
                {
                    mUserGridsCount[cr_iter->first] = 1;
                }
            }
            cr_iter++;
        }
    }
    else
    {
        // "Legacy" viewer support
        LLPointer<LLCredential> cred = gSecAPIHandler->loadCredential(grid);
        if (cred.notNull())
        {
            const LLSD &ident = cred->getIdentifier();
            if (ident.isMap() && ident.has("type"))
            {
                std::string user_label = LLPanelLogin::getUserName(cred);
                LLSD user_data;
                user_data["user_id"] = cred->userID();
                user_data["label"] = user_label;
                user_data["grid"] = grid;

                if (show_grid_name)
                {
                    user_label += " (" + grid_label + ")";
                }

                LLScrollListItem::Params item_params;
                item_params.value(user_data);
                item_params.columns.add()
                    .value(user_label)
                    .column("user")
                    .font(LLFontGL::getFontSansSerifSmall());
                mScrollList->addRow(item_params, ADD_BOTTOM);

                // Add one to grid count
                std::map<std::string, S32>::iterator found = mUserGridsCount.find(cred->userID());
                if (found != mUserGridsCount.end())
                {
                    found->second++;
                }
                else
                {
                    mUserGridsCount[cred->userID()] = 1;
                }
            }
        }
    }
}


