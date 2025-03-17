/**
 * @file llpanelcontents.cpp
 * @brief Object contents panel in the tools floater.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

// file include
#include "llpanelcontents.h"

// linden library includes
#include "llerror.h"
#include "llfiltereditor.h"
#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llinventorydefines.h"
#include "llmaterialtable.h"
#include "llpermissionsflags.h"
#include "llrect.h"
#include "llstring.h"
#include "llui.h"
#include "m3math.h"
#include "material_codes.h"

// project includes
#include "llagent.h"
#include "llpanelobjectinventory.h"
#include "llpreviewscript.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "lltool.h"
#include "lltoolcomp.h"
#include "lltoolmgr.h"
#include "lltrans.h"
#include "llviewerassettype.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "llworld.h"
#include "llfloaterperms.h"
#include "llviewerassetupload.h"

//
// Imported globals
//


//
// Globals
//
const char* LLPanelContents::TENTATIVE_SUFFIX = "_tentative";
const char* LLPanelContents::PERMS_OWNER_INTERACT_KEY = "perms_owner_interact";
const char* LLPanelContents::PERMS_OWNER_CONTROL_KEY = "perms_owner_control";
const char* LLPanelContents::PERMS_GROUP_INTERACT_KEY = "perms_group_interact";
const char* LLPanelContents::PERMS_GROUP_CONTROL_KEY = "perms_group_control";
const char* LLPanelContents::PERMS_ANYONE_INTERACT_KEY = "perms_anyone_interact";
const char* LLPanelContents::PERMS_ANYONE_CONTROL_KEY = "perms_anyone_control";

bool LLPanelContents::postBuild()
{
    setMouseOpaque(false);

    childSetAction("button new script",&LLPanelContents::onClickNewScript, this);
    childSetAction("button permissions",&LLPanelContents::onClickPermissions, this);

    mFilterEditor = getChild<LLFilterEditor>("contents_filter");
    mFilterEditor->setCommitCallback([&](LLUICtrl*, const LLSD&) { onFilterEdit(); });

    mPanelInventoryObject = getChild<LLPanelObjectInventory>("contents_inventory");

    // update permission filter once UI is fully initialized
    mSavedFolderState.setApply(false);

    return true;
}

LLPanelContents::LLPanelContents()
    :   LLPanel(),
        mPanelInventoryObject(NULL)
{
}


LLPanelContents::~LLPanelContents()
{
    // Children all cleaned up by default view destructor.
}


void LLPanelContents::getState(LLViewerObject *objectp )
{
    if( !objectp )
    {
        getChildView("button new script")->setEnabled(false);
        return;
    }

    LLUUID group_id;            // used for SL-23488
    LLSelectMgr::getInstance()->selectGetGroup(group_id);  // sets group_id as a side effect SL-23488

    // BUG? Check for all objects being editable?
    bool editable = gAgent.isGodlike()
                    || (objectp->permModify() && !objectp->isPermanentEnforced()
                           && ( objectp->permYouOwner() || ( !group_id.isNull() && gAgent.isInGroup(group_id) )));  // solves SL-23488
    bool all_volume = LLSelectMgr::getInstance()->selectionAllPCode( LL_PCODE_VOLUME );

    // Edit script button - ok if object is editable and there's an unambiguous destination for the object.
    getChildView("button new script")->setEnabled(
        editable &&
        all_volume &&
        ((LLSelectMgr::getInstance()->getSelection()->getRootObjectCount() == 1)
            || (LLSelectMgr::getInstance()->getSelection()->getObjectCount() == 1)));

    getChildView("button permissions")->setEnabled(!objectp->isPermanentEnforced());
    mPanelInventoryObject->setEnabled(!objectp->isPermanentEnforced());
}

void LLPanelContents::onFilterEdit()
{
    const std::string& filter_substring = mFilterEditor->getText();
    if (filter_substring.empty())
    {
        if (mPanelInventoryObject->getFilter().getFilterSubString().empty())
        {
            // The current filter and the new filter are empty, nothing to do
            return;
        }

        mSavedFolderState.setApply(true);
        mPanelInventoryObject->getRootFolder()->applyFunctorRecursively(mSavedFolderState);

        // Add a folder with the current item to the list of previously opened folders
        LLOpenFoldersWithSelection opener;
        mPanelInventoryObject->getRootFolder()->applyFunctorRecursively(opener);
        mPanelInventoryObject->getRootFolder()->scrollToShowSelection();
    }
    else if (mPanelInventoryObject->getFilter().getFilterSubString().empty())
    {
        // The first letter in search term, save existing folder open state
        if (!mPanelInventoryObject->getFilter().isNotDefault())
        {
            mSavedFolderState.setApply(false);
            mPanelInventoryObject->getRootFolder()->applyFunctorRecursively(mSavedFolderState);
        }
    }

    mPanelInventoryObject->getFilter().setFilterSubString(filter_substring);
}

void LLPanelContents::refresh()
{
    const bool children_ok = true;
    LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(children_ok);

    getState(object);
    if (mPanelInventoryObject)
    {
        mPanelInventoryObject->refresh();
    }
}

void LLPanelContents::clearContents()
{
    if (mPanelInventoryObject)
    {
        mPanelInventoryObject->clearInventoryTask();
    }
}

//
// Static functions
//

// static
void LLPanelContents::onClickNewScript(void *userdata)
{
    const bool children_ok = true;
    LLViewerObject* object = LLSelectMgr::getInstance()->getSelection()->getFirstRootObject(children_ok);
    if (object)
    {
        LLPermissions perm;
        perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);

        // Parameters are base, owner, everyone, group, next
        perm.initMasks(
            PERM_ALL,
            PERM_ALL,
            LLFloaterPerms::getEveryonePerms("Scripts"),
            LLFloaterPerms::getGroupPerms("Scripts"),
            PERM_MOVE | LLFloaterPerms::getNextOwnerPerms("Scripts"));
        std::string desc;
        LLViewerAssetType::generateDescriptionFor(LLAssetType::AT_LSL_TEXT, desc);

        // --------------------------------------------------------------------------------------------------
        // Begin hack
        //
        // The current state of the server doesn't allow specifying a default script template,
        // so we have to update its code immediately after creation instead.
        //
        // Moreover, _PREHASH_RezScript has more complex server-side logic than _PREHASH_CreateInventoryItem,
        // which changes the item's attributes, such as its name and UUID. The simplest way to mitigate this
        // is to create a temporary item in the user's inventory, modify it as in create_inventory_item()'s
        // callback, and then call _PREHASH_RezScript to move it into the object's inventory.
        //
        // This temporary workaround should be removed after a server-side fix.
        // See https://github.com/secondlife/viewer/issues/3731 for more information.
        //
        LLViewerRegion* region = object->getRegion();
        if (region && region->simulatorFeaturesReceived())
        {
            LLSD simulatorFeatures;
            region->getSimulatorFeatures(simulatorFeatures);
            if (simulatorFeatures["LuaScriptsEnabled"].asBoolean())
            {
                if (std::string::size_type pos = desc.find("lsl2"); pos != std::string::npos)
                {
                    desc.replace(pos, 4, "SLua");
                }

                auto scriptCreationCallback = [object](const LLUUID& inv_item)
                    {
                        if (!inv_item.isNull())
                        {
                            LLViewerInventoryItem* item = gInventory.getItem(inv_item);
                            if (item)
                            {
                                const std::string hello_lua_script =
                                    "function state_entry()\n"
                                    "   ll.Say(0, \"Hello, Avatar!\")\n"
                                    "end\n"
                                    "\n"
                                    "function touch_start(total_number)\n"
                                    "   ll.Say(0, \"Touched.\")\n"
                                    "end\n"
                                    "\n"
                                    "-- Simulate the state_entry event\n"
                                    "state_entry()\n";

                                auto scriptUploadFinished = [object, item](LLUUID itemId, LLUUID newAssetId, LLUUID newItemId, LLSD response)
                                    {
                                        LLPointer<LLViewerInventoryItem> new_script = new LLViewerInventoryItem(item);
                                        object->saveScript(new_script, true, true);

                                        // Delete the temporary item from the user's inventory after rezzing it in the object's inventory
                                        ms_sleep(50);
                                        LLMessageSystem* msg = gMessageSystem;
                                        msg->newMessageFast(_PREHASH_RemoveInventoryItem);
                                        msg->nextBlockFast(_PREHASH_AgentData);
                                        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
                                        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
                                        msg->nextBlockFast(_PREHASH_InventoryData);
                                        msg->addUUIDFast(_PREHASH_ItemID, item->getUUID());
                                        gAgent.sendReliableMessage();

                                        gInventory.deleteObject(item->getUUID());
                                        gInventory.notifyObservers();
                                    };

                                std::string url = gAgent.getRegion()->getCapability("UpdateScriptAgent");
                                if (!url.empty())
                                {
                                    LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLScriptAssetUpload>(
                                        item->getUUID(),
                                        "luau",
                                        hello_lua_script,
                                        scriptUploadFinished,
                                        nullptr));

                                    LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
                                }
                            }
                        }
                    };
                LLPointer<LLBoostFuncInventoryCallback> cb = new LLBoostFuncInventoryCallback(scriptCreationCallback);

                LLMessageSystem* msg = gMessageSystem;
                msg->newMessageFast(_PREHASH_CreateInventoryItem);
                msg->nextBlock(_PREHASH_AgentData);
                msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
                msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
                msg->nextBlock(_PREHASH_InventoryBlock);
                msg->addU32Fast(_PREHASH_CallbackID, gInventoryCallbacks.registerCB(cb));
                msg->addUUIDFast(_PREHASH_FolderID, gInventory.getRootFolderID());
                msg->addUUIDFast(_PREHASH_TransactionID, LLTransactionID::tnull);
                msg->addU32Fast(_PREHASH_NextOwnerMask, PERM_MOVE | PERM_TRANSFER);
                msg->addS8Fast(_PREHASH_Type, LLAssetType::AT_LSL_TEXT);
                msg->addS8Fast(_PREHASH_InvType, LLInventoryType::IT_LSL);
                msg->addU8Fast(_PREHASH_WearableType, NO_INV_SUBTYPE);
                msg->addStringFast(_PREHASH_Name, "New Script");
                msg->addStringFast(_PREHASH_Description, desc);

                gAgent.sendReliableMessage();
                return;
            }
        }
        //
        // End hack
        // --------------------------------------------------------------------------------------------------

        LLPointer<LLViewerInventoryItem> new_item =
            new LLViewerInventoryItem(
                LLUUID::null,
                LLUUID::null,
                perm,
                LLUUID::null,
                LLAssetType::AT_LSL_TEXT,
                LLInventoryType::IT_LSL,
                "New Script",
                desc,
                LLSaleInfo::DEFAULT,
                LLInventoryItemFlags::II_FLAGS_NONE,
                time_corrected());
        object->saveScript(new_item, true, true);

        std::string name = new_item->getName();

        // *NOTE: In order to resolve SL-22177, we needed to create
        // the script first, and then you have to click it in
        // inventory to edit it.
        // *TODO: The script creation should round-trip back to the
        // viewer so the viewer can auto-open the script and start
        // editing ASAP.
    }
}

// static
void LLPanelContents::onClickPermissions(void *userdata)
{
    LLPanelContents* self = (LLPanelContents*)userdata;
    gFloaterView->getParentFloater(self)->addDependentFloater(LLFloaterReg::showInstance("bulk_perms"));
}
