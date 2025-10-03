/**
 * @file llgltffolderitem.h
 * @author Andrey Kleshchev
 * @brief LLGLTFFolderItem header file
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

#ifndef LL_LLGLTFFOLDERITEM_H
#define LL_LLGLTFFOLDERITEM_H

#include "llfloater.h"

#include "llfolderviewmodel.h"

class LLGLTFFolderItem : public LLFolderViewModelItemCommon
{
public:
    enum EType
    {
        TYPE_ROOT,
        TYPE_SCENE,
        TYPE_NODE,
        TYPE_MESH,
        TYPE_SKIN,
    };

    LLGLTFFolderItem(S32 id, const std::string &display_name, EType type, LLFolderViewModelInterface& root_view_model);
    LLGLTFFolderItem(LLFolderViewModelInterface& root_view_model);
    virtual ~LLGLTFFolderItem();

    void init();

    const std::string& getName() const override { return mName; }
    const std::string& getDisplayName() const override { return mName; }
    const std::string& getSearchableName() const override { return mName; }

    std::string getSearchableDescription() const override { return std::string(); }
    std::string getSearchableCreatorName()const override { return std::string(); }
    std::string getSearchableUUIDString() const override { return std::string(); }

    LLPointer<LLUIImage> getIcon() const override { return pIcon; }
    LLPointer<LLUIImage> getIconOpen() const override { return getIcon(); }
    LLPointer<LLUIImage> getIconOverlay() const override { return NULL; }

    LLFontGL::StyleFlags getLabelStyle() const override { return LLFontGL::NORMAL; }
    std::string getLabelSuffix() const override { return std::string(); }

    void openItem(void) override {}
    void closeItem(void) override {}
    void selectItem(void) override {}

    void navigateToFolder(bool new_window = false, bool change_mode = false) override {}

    bool isItemWearable() const override { return false; }

    bool isItemRenameable() const override { return false; }
    bool renameItem(const std::string& new_name) override { return false; }

    bool isItemMovable(void) const override { return false; } // Can be moved to another folder
    void move(LLFolderViewModelItem* parent_listener) override {}

    bool isItemRemovable(bool check_worn = true) const override { return false; }
    bool removeItem() override { return false; }
    void removeBatch(std::vector<LLFolderViewModelItem*>& batch) override {}

    bool isItemCopyable(bool can_copy_as_link = true) const override { return false; }
    bool copyToClipboard() const override { return false; }
    bool cutToClipboard() override { return false; }
    bool isCutToClipboard() override { return false; }

    bool isClipboardPasteable() const override { return false; }
    void pasteFromClipboard() override {}
    void pasteLinkFromClipboard() override {}

    void buildContextMenu(LLMenuGL& menu, U32 flags) override {};

    bool potentiallyVisible() override { return true; }; // is the item definitely visible or we haven't made up our minds yet?

    bool hasChildren() const override { return mChildren.size() > 0; }

    bool dragOrDrop(
        MASK mask,
        bool drop,
        EDragAndDropType cargo_type,
        void* cargo_data,
        std::string& tooltip_msg) override
    {
        return false;
    }

    bool filterChildItem(LLFolderViewModelItem* item, LLFolderViewFilter& filter);
    bool filter(LLFolderViewFilter& filter) override;

    EType getType() const { return mItemType; }
    S32 getItemId() const { return mItemId; }

    bool isFavorite() const override { return false; }
    bool isItemInTrash() const override { return false; }
    bool isAgentInventory() const override { return false; }
    bool isAgentInventoryRoot() const override { return false; }

private:
    LLUIImagePtr pIcon;
    std::string mName;
    EType mItemType = TYPE_ROOT;

    // mItemId can be an id in a mesh vector, node vector or any other vector.
    // mItemId is not nessesarily unique, ex: some nodes can reuse the same
    // mesh or skin, so mesh-items can have the same id.
    S32 mItemId = -1;
};

#endif
