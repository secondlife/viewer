/**
 * @file llfloatergltfasseteditor.h
 * @author Andrii Kleshchev
 * @brief LLFloaterGltfAssetEditor header file
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

#ifndef LL_LLFLOATERGLTFASSETEDITOR_H
#define LL_LLFLOATERGLTFASSETEDITOR_H

#include "llfloater.h"

#include "llgltffoldermodel.h"

namespace LL
{
    namespace GLTF
    {
        class Asset;
    }
}

class LLSpinCtrl;
class LLMenuButton;
class LLViewerObject;

class LLFloaterGLTFAssetEditor : public LLFloater
{
public:
    LLFloaterGLTFAssetEditor(const LLSD& key);
    ~LLFloaterGLTFAssetEditor();

    bool postBuild() override;
    void onOpen(const LLSD& key) override;
    void onClose(bool app_quitting) override;
    void initFolderRoot();

    LLGLTFViewModel& getRootViewModel() { return mGLTFViewModel; }

    static void idle(void* user_data);
    void loadItem(S32 id, const std::string& name, LLGLTFFolderItem::EType type, LLFolderViewFolder* parent);
    void loadFromNode(S32 node, LLFolderViewFolder* parent);
    void loadFromSelection();

    void dirty();

protected:
    void onFolderSelectionChanged(const std::deque<LLFolderViewItem*>& items, bool user_action);
    void onCommitTransform();
    void onMenuDoToSelected(const LLSD& userdata);
    bool onMenuEnableItem(const LLSD& userdata);

    void setTransformsEnabled(bool val);
    void loadNodeTransforms(S32 id);

    void clearRoot();

private:

    LLPointer<LLViewerObject> mObject;
    std::shared_ptr<LL::GLTF::Asset> mAsset;

    // Folder view related
    LLUIColor mUIColor;
    LLGLTFViewModel mGLTFViewModel;
    LLPanel* mItemListPanel = nullptr;
    LLFolderView* mFolderRoot = nullptr;
    LLScrollContainer* mScroller = nullptr;
    std::map<S32, LLFolderViewItem*> mNodeToItemMap;

    // Transforms panel
    LLVector3       mLastEulerDegrees;

    LLPanel* mTransformsPanel = nullptr;
    LLMenuButton* mMenuClipboardPos = nullptr;
    LLSpinCtrl* mCtrlPosX = nullptr;
    LLSpinCtrl* mCtrlPosY = nullptr;
    LLSpinCtrl* mCtrlPosZ = nullptr;
    LLMenuButton* mMenuClipboardScale = nullptr;
    LLSpinCtrl* mCtrlScaleX = nullptr;
    LLSpinCtrl* mCtrlScaleY = nullptr;
    LLSpinCtrl* mCtrlScaleZ = nullptr;
    LLMenuButton* mMenuClipboardRot = nullptr;
    LLSpinCtrl* mCtrlRotX = nullptr;
    LLSpinCtrl* mCtrlRotY = nullptr;
    LLSpinCtrl* mCtrlRotZ = nullptr;
};

#endif
