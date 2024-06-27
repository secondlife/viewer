/**
 * @file llfloatergltfasseteditor.cpp
 * @author Andrii Kleshchev
 * @brief LLFloaterGltfAssetEditor class implementation
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#include "llfloatergltfasseteditor.h"

#include "gltf/asset.h"
#include "llcallbacklist.h"
#include "llselectmgr.h"
#include "llviewerobject.h"

const LLColor4U DEFAULT_WHITE(255, 255, 255);

/// LLFloaterGLTFAssetEditor

LLFloaterGLTFAssetEditor::LLFloaterGLTFAssetEditor(const LLSD& key)
    : LLFloater(key)
    , mUIColor(LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE))
{
    setTitle("GLTF Asset Editor (WIP)");
}

LLFloaterGLTFAssetEditor::~LLFloaterGLTFAssetEditor()
{
    gIdleCallbacks.deleteFunction(idle, this);
}

bool LLFloaterGLTFAssetEditor::postBuild()
{
    mItemListPanel = getChild<LLPanel>("item_list_panel");

    LLRect scroller_view_rect = mItemListPanel->getRect();
    scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
    LLScrollContainer::Params scroller_params(LLUICtrlFactory::getDefaultParams<LLFolderViewScrollContainer>());
    scroller_params.rect(scroller_view_rect);
    scroller_params.name("folder_scroller");
    mScroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
    mScroller->setFollowsAll();

    // Insert that scroller into the panel widgets hierarchy
    mItemListPanel->addChild(mScroller);

    // Create the root model and view for all conversation sessions
    LLGLTFFolderItem* base_item = new LLGLTFFolderItem(mGLTFViewModel);

    LLFolderView::Params p(LLUICtrlFactory::getDefaultParams<LLFolderView>());
    p.name = "Root";
    p.title = "Root";
    p.rect = LLRect(0, 0, getRect().getWidth(), 0);
    p.parent_panel = mItemListPanel;
    p.tool_tip = p.name;
    p.listener = base_item;
    p.view_model = &mGLTFViewModel;
    p.root = NULL;
    p.use_ellipses = true;
    p.options_menu = "menu_gltf.xml"; // *TODO : create this or fix to be optional
    mFolderRoot = LLUICtrlFactory::create<LLFolderView>(p);
    mFolderRoot->setCallbackRegistrar(&mCommitCallbackRegistrar);
    mFolderRoot->setEnableRegistrar(&mEnableCallbackRegistrar);
    // Attach root to the scroller
    mScroller->addChild(mFolderRoot);
    mFolderRoot->setScrollContainer(mScroller);
    mFolderRoot->setFollowsAll();
    mFolderRoot->setOpen(true);
    mScroller->setVisible(true);

    gIdleCallbacks.addFunction(idle, this);

    return true;
}

void LLFloaterGLTFAssetEditor::onOpen(const LLSD& key)
{
    loadFromSelection();
}

void LLFloaterGLTFAssetEditor::idle(void* user_data)
{
    LLFloaterGLTFAssetEditor* floater = (LLFloaterGLTFAssetEditor*)user_data;

    if (floater->mFolderRoot)
    {
        floater->mFolderRoot->update();
    }
}

void LLFloaterGLTFAssetEditor::loadItem(S32 id, const std::string& name, LLGLTFFolderItem::EType type, LLFolderViewFolder* parent)
{
    LLGLTFFolderItem* listener = new LLGLTFFolderItem(id, name, type, mGLTFViewModel);

    LLFolderViewItem::Params params;
    params.name(name);
    params.creation_date(0);
    params.root(mFolderRoot);
    params.listener(listener);
    params.rect(LLRect());
    params.tool_tip = params.name;
    params.font_color = mUIColor;
    params.font_highlight_color = mUIColor;
    LLFolderViewItem* view = LLUICtrlFactory::create<LLFolderViewItem>(params);

    view->addToFolder(parent);
    view->setVisible(true);
}

void LLFloaterGLTFAssetEditor::loadFromNode(S32 node_id, LLFolderViewFolder* parent)
{
    if (mAsset->mNodes.size() <= node_id)
    {
        return;
    }

    LL::GLTF::Node& node = mAsset->mNodes[node_id];

    std::string name = node.mName;
    if (node.mName.empty())
    {
        name = getString("node_tittle");
    }
    else
    {
        name = node.mName;
    }

    LLGLTFFolderItem* listener = new LLGLTFFolderItem(node_id, name, LLGLTFFolderItem::TYPE_NODE, mGLTFViewModel);

    LLFolderViewFolder::Params p;
    p.root = mFolderRoot;
    p.listener = listener;
    p.name = name;
    p.tool_tip = name;
    p.font_color = mUIColor;
    p.font_highlight_color = mUIColor;
    LLFolderViewFolder* view = LLUICtrlFactory::create<LLFolderViewFolder>(p);

    view->addToFolder(parent);
    view->setVisible(true);
    view->setOpen(true);

    for (S32& node_id : node.mChildren)
    {
        loadFromNode(node_id, view);
    }

    if (node.mMesh != LL::GLTF::INVALID_INDEX && mAsset->mMeshes.size() > node.mMesh)
    {
        std::string name = mAsset->mMeshes[node.mMesh].mName;
        if (name.empty())
        {
            name = getString("mesh_tittle");
        }
        loadItem(node.mMesh, name, LLGLTFFolderItem::TYPE_MESH, view);
    }

    if (node.mSkin != LL::GLTF::INVALID_INDEX && mAsset->mSkins.size() > node.mSkin)
    {
        std::string name = mAsset->mSkins[node.mSkin].mName;
        if (name.empty())
        {
            name = getString("skin_tittle");
        }
        loadItem(node.mSkin, name, LLGLTFFolderItem::TYPE_SKIN, view);
    }

    view->setChildrenInited(true);
}

void LLFloaterGLTFAssetEditor::loadFromSelection()
{
    if (!mFolderRoot || LLSelectMgr::getInstance()->getSelection()->getObjectCount() != 1)
    {
        return;
    }

    LLViewerObject* objectp = LLSelectMgr::getInstance()->getSelection()->getFirstObject();
    if (!objectp)
    {
        return;
    }

    mAsset = objectp->mGLTFAsset;
    if (!mAsset)
    {
        return;
    }

    LLUIColor item_color = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
    for (S32 i = 0; i < mAsset->mScenes.size(); i++)
    {
        LL::GLTF::Scene& scene = mAsset->mScenes[i];
        std::string name = scene.mName;
        if (scene.mName.empty())
        {
            name = getString("scene_tittle");
        }
        else
        {
            name = scene.mName;
        }

        LLGLTFFolderItem* listener = new LLGLTFFolderItem(i, name, LLGLTFFolderItem::TYPE_SCENE, mGLTFViewModel);


        LLFolderViewFolder::Params p;
        p.name = name;
        p.root = mFolderRoot;
        p.listener = listener;
        p.tool_tip = name;
        p.font_color = mUIColor;
        p.font_highlight_color = mUIColor;
        LLFolderViewFolder* view = LLUICtrlFactory::create<LLFolderViewFolder>(p);

        view->addToFolder(mFolderRoot);
        view->setVisible(true);
        view->setOpen(true);

        for (S32& node_id : scene.mNodes)
        {
            loadFromNode(node_id, view);
        }
        view->setChildrenInited(true);
    }

    mGLTFViewModel.requestSortAll();
    mFolderRoot->setChildrenInited(true);
    mFolderRoot->arrangeAll();
    mFolderRoot->update();
}

