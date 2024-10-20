/**
 * @file llfloatergltfasseteditor.cpp
 * @author Andrii Kleshchev
 * @brief LLFloaterGltfAssetEditor class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfloatergltfasseteditor.h"

#include "gltf/asset.h"
#include "llcallbacklist.h"
#include "llmenubutton.h"
#include "llselectmgr.h"
#include "llspinctrl.h"
#include "llviewerobject.h"

const LLColor4U DEFAULT_WHITE(255, 255, 255);

/// LLFloaterGLTFAssetEditor

LLFloaterGLTFAssetEditor::LLFloaterGLTFAssetEditor(const LLSD& key)
    : LLFloater(key)
    , mUIColor(LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE))
{
    setTitle("GLTF Asset Editor (WIP)");

    mCommitCallbackRegistrar.add("PanelObject.menuDoToSelected", [this](LLUICtrl* ctrl, const LLSD& data) { onMenuDoToSelected(data); });
    mEnableCallbackRegistrar.add("PanelObject.menuEnable", [this](LLUICtrl* ctrl, const LLSD& data) { return onMenuEnableItem(data); });
}

LLFloaterGLTFAssetEditor::~LLFloaterGLTFAssetEditor()
{
    if (mScroller)
    {
        mItemListPanel->removeChild(mScroller);
        delete mScroller;
        mScroller = NULL;
    }
}

bool LLFloaterGLTFAssetEditor::postBuild()
{
    // Position
    mMenuClipboardPos = getChild<LLMenuButton>("clipboard_pos_btn");
    mCtrlPosX = getChild<LLSpinCtrl>("Pos X", true);
    mCtrlPosX->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    mCtrlPosY = getChild<LLSpinCtrl>("Pos Y", true);
    mCtrlPosY->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    mCtrlPosZ = getChild<LLSpinCtrl>("Pos Z", true);
    mCtrlPosZ->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });

    // Scale
    mMenuClipboardScale = getChild<LLMenuButton>("clipboard_size_btn");
    mCtrlScaleX = getChild<LLSpinCtrl>("Scale X", true);
    mCtrlScaleX->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    mCtrlScaleY = getChild<LLSpinCtrl>("Scale Y", true);
    mCtrlScaleY->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    mCtrlScaleZ = getChild<LLSpinCtrl>("Scale Z", true);
    mCtrlScaleZ->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });

    // Rotation
    mMenuClipboardRot = getChild<LLMenuButton>("clipboard_rot_btn");
    mCtrlRotX = getChild<LLSpinCtrl>("Rot X", true);
    mCtrlRotX->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    mCtrlRotY = getChild<LLSpinCtrl>("Rot Y", true);
    mCtrlRotY->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    mCtrlRotZ = getChild<LLSpinCtrl>("Rot Z", true);
    mCtrlPosZ->setCommitCallback([this](LLUICtrl* ctrl, const LLSD& param) { onCommitTransform(); });
    setTransformsEnabled(false);
    // todo: do multiple panels based on selected element.
    mTransformsPanel = getChild<LLPanel>("transform_panel", true);
    mTransformsPanel->setVisible(false);

    mItemListPanel = getChild<LLPanel>("item_list_panel", true);
    initFolderRoot();

    return true;
}

void LLFloaterGLTFAssetEditor::initFolderRoot()
{
    if (mScroller || mFolderRoot)
    {
        LL_ERRS() << "Folder root already initialized" << LL_ENDL;
        return;
    }

    LLRect scroller_view_rect = mItemListPanel->getRect();
    scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
    LLScrollContainer::Params scroller_params(LLUICtrlFactory::getDefaultParams<LLFolderViewScrollContainer>());
    scroller_params.rect(scroller_view_rect);
    scroller_params.name("folder_scroller");
    mScroller = LLUICtrlFactory::create<LLFolderViewScrollContainer>(scroller_params);
    mScroller->setFollowsAll();

    // Insert that scroller into the panel widgets hierarchy
    mItemListPanel->addChild(mScroller);

    // Create the root model
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
    mFolderRoot->setSelectCallback([this](const std::deque<LLFolderViewItem*>& items, bool user_action) { onFolderSelectionChanged(items, user_action); });
    mScroller->setVisible(true);
}

void LLFloaterGLTFAssetEditor::onOpen(const LLSD& key)
{
    gIdleCallbacks.addFunction(idle, this);
    loadFromSelection();
}

void LLFloaterGLTFAssetEditor::onClose(bool app_quitting)
{
    gIdleCallbacks.deleteFunction(idle, this);
    mAsset = nullptr;
    mObject = nullptr;
}

void LLFloaterGLTFAssetEditor::clearRoot()
{
    LLFolderViewFolder::folders_t::iterator folders_it = mFolderRoot->getFoldersBegin();
    while (folders_it != mFolderRoot->getFoldersEnd())
    {
        (*folders_it)->destroyView();
        folders_it = mFolderRoot->getFoldersBegin();
    }
    mNodeToItemMap.clear();
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
        name = getString("node_title");
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

    mNodeToItemMap[node_id] = view;

    for (S32& node_id : node.mChildren)
    {
        loadFromNode(node_id, view);
    }

    if (node.mMesh != LL::GLTF::INVALID_INDEX && mAsset->mMeshes.size() > node.mMesh)
    {
        std::string name = mAsset->mMeshes[node.mMesh].mName;
        if (name.empty())
        {
            name = getString("mesh_title");
        }
        loadItem(node.mMesh, name, LLGLTFFolderItem::TYPE_MESH, view);
    }

    if (node.mSkin != LL::GLTF::INVALID_INDEX && mAsset->mSkins.size() > node.mSkin)
    {
        std::string name = mAsset->mSkins[node.mSkin].mName;
        if (name.empty())
        {
            name = getString("skin_title");
        }
        loadItem(node.mSkin, name, LLGLTFFolderItem::TYPE_SKIN, view);
    }

    view->setChildrenInited(true);
}

void LLFloaterGLTFAssetEditor::loadFromSelection()
{
    clearRoot();

    if (LLSelectMgr::getInstance()->getSelection()->getObjectCount() != 1)
    {
        mAsset = nullptr;
        mObject = nullptr;
        return;
    }

    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode(NULL);
    LLViewerObject* objectp = node->getObject();
    if (!objectp)
    {
        mAsset = nullptr;
        mObject = nullptr;
        return;
    }

    if (!objectp->mGLTFAsset)
    {
        mAsset = nullptr;
        mObject = nullptr;
        return;
    }
    mAsset = objectp->mGLTFAsset;
    mObject = objectp;

    if (node->mName.empty())
    {
        setTitle(getString("floater_title"));
    }
    else
    {
        setTitle(node->mName);
    }

    LLUIColor item_color = LLUIColorTable::instance().getColor("MenuItemEnabledColor", DEFAULT_WHITE);
    for (S32 i = 0; i < mAsset->mScenes.size(); i++)
    {
        LL::GLTF::Scene& scene = mAsset->mScenes[i];
        std::string name = scene.mName;
        if (scene.mName.empty())
        {
            name = getString("scene_title");
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

void LLFloaterGLTFAssetEditor::dirty()
{
    if (!mObject || !mAsset || !mFolderRoot)
    {
        return;
    }

    if (LLSelectMgr::getInstance()->getSelection()->getObjectCount() > 1)
    {
        if (getVisible())
        {
            closeFloater();
        }
        return;
    }

    LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->getFirstNode(NULL);
    if (!node)
    {
        // not yet updated?
        // Todo: Subscribe to deletion in some way
        return;
    }

    LLViewerObject* objectp = node->getObject();
    if (mObject != objectp || !objectp->mGLTFAsset)
    {
        if (getVisible())
        {
            closeFloater();
        }
        return;
    }

    if (mAsset != objectp->mGLTFAsset)
    {
        loadFromSelection();
        return;
    }

    auto found = mNodeToItemMap.find(node->mSelectedGLTFNode);
    if (found != mNodeToItemMap.end())
    {
        LLFolderViewItem* itemp = found->second;
        itemp->arrangeAndSet(true, false);
        loadNodeTransforms(node->mSelectedGLTFNode);
    }
}

void LLFloaterGLTFAssetEditor::onFolderSelectionChanged(const std::deque<LLFolderViewItem*>& items, bool user_action)
{
    if (items.empty())
    {
        setTransformsEnabled(false);
        return;
    }

    LLFolderViewItem* item = items.front();
    LLGLTFFolderItem* vmi = static_cast<LLGLTFFolderItem*>(item->getViewModelItem());

    switch (vmi->getType())
    {
    case LLGLTFFolderItem::TYPE_SCENE:
        {
            setTransformsEnabled(false);
            LLSelectMgr::getInstance()->selectObjectOnly(mObject, SELECT_ALL_TES, -1, -1);
            break;
        }
    case LLGLTFFolderItem::TYPE_NODE:
        {
            setTransformsEnabled(true);
            loadNodeTransforms(vmi->getItemId());
            LLSelectMgr::getInstance()->selectObjectOnly(mObject, SELECT_ALL_TES, vmi->getItemId(), 0);
            break;
        }
    case LLGLTFFolderItem::TYPE_MESH:
    case LLGLTFFolderItem::TYPE_SKIN:
        {
            if (item->getParent()) // should be a node
            {
                LLFolderViewFolder* parent = item->getParentFolder();
                LLGLTFFolderItem* parent_vmi = static_cast<LLGLTFFolderItem*>(parent->getViewModelItem());
                LLSelectMgr::getInstance()->selectObjectOnly(mObject, SELECT_ALL_TES, parent_vmi->getItemId(), 0);
            }

            setTransformsEnabled(false);
            break;
        }
    default:
        {
            setTransformsEnabled(false);
            break;
        }
    }
}

void LLFloaterGLTFAssetEditor::setTransformsEnabled(bool val)
{
    mMenuClipboardPos->setEnabled(val);
    mCtrlPosX->setEnabled(val);
    mCtrlPosY->setEnabled(val);
    mCtrlPosZ->setEnabled(val);
    mMenuClipboardScale->setEnabled(val);
    mCtrlScaleX->setEnabled(val);
    mCtrlScaleY->setEnabled(val);
    mCtrlScaleZ->setEnabled(val);
    mMenuClipboardRot->setEnabled(val);
    mCtrlRotX->setEnabled(val);
    mCtrlRotY->setEnabled(val);
    mCtrlRotZ->setEnabled(val);
}

void LLFloaterGLTFAssetEditor::loadNodeTransforms(S32 node_id)
{
    if (node_id < 0 || node_id >= mAsset->mNodes.size())
    {
        LL_ERRS() << "Node id out of range: " << node_id << LL_ENDL;
        return;
    }

    LL::GLTF::Node& node = mAsset->mNodes[node_id];
    node.makeTRSValid();

    mCtrlPosX->set(node.mTranslation[0]);
    mCtrlPosY->set(node.mTranslation[1]);
    mCtrlPosZ->set(node.mTranslation[2]);

    mCtrlScaleX->set(node.mScale[0]);
    mCtrlScaleY->set(node.mScale[1]);
    mCtrlScaleZ->set(node.mScale[2]);

    LLQuaternion object_rot = LLQuaternion(node.mRotation[0], node.mRotation[1], node.mRotation[2], node.mRotation[3]);
    object_rot.getEulerAngles(&(mLastEulerDegrees.mV[VX]), &(mLastEulerDegrees.mV[VY]), &(mLastEulerDegrees.mV[VZ]));
    mLastEulerDegrees *= RAD_TO_DEG;
    mLastEulerDegrees.mV[VX] = fmod(ll_round(mLastEulerDegrees.mV[VX], OBJECT_ROTATION_PRECISION) + 360.f, 360.f);
    mLastEulerDegrees.mV[VY] = fmod(ll_round(mLastEulerDegrees.mV[VY], OBJECT_ROTATION_PRECISION) + 360.f, 360.f);
    mLastEulerDegrees.mV[VZ] = fmod(ll_round(mLastEulerDegrees.mV[VZ], OBJECT_ROTATION_PRECISION) + 360.f, 360.f);

    mCtrlRotX->set(mLastEulerDegrees.mV[VX]);
    mCtrlRotY->set(mLastEulerDegrees.mV[VY]);
    mCtrlRotZ->set(mLastEulerDegrees.mV[VZ]);
}

void LLFloaterGLTFAssetEditor::onCommitTransform()
{
    if (!mFolderRoot)
    {
        LL_ERRS() << "Folder root not initialized" << LL_ENDL;
        return;
    }

    LLFolderViewItem* item = mFolderRoot->getCurSelectedItem();
    if (!item)
    {
        LL_ERRS() << "Nothing selected" << LL_ENDL;
        return;
    }

    LLGLTFFolderItem* vmi = static_cast<LLGLTFFolderItem*>(item->getViewModelItem());

    if (!vmi || vmi->getType() != LLGLTFFolderItem::TYPE_NODE)
    {
        LL_ERRS() << "Only nodes implemented" << LL_ENDL;
        return;
    }
    S32 node_id = vmi->getItemId();
    LL::GLTF::Node& node = mAsset->mNodes[node_id];

    LL::GLTF::vec3 tr(mCtrlPosX->get(), mCtrlPosY->get(), mCtrlPosZ->get());
    node.setTranslation(tr);

    LL::GLTF::vec3 scale(mCtrlScaleX->get(), mCtrlScaleY->get(), mCtrlScaleZ->get());
    node.setScale(scale);

    LLVector3 new_rot(mCtrlRotX->get(), mCtrlRotY->get(), mCtrlRotZ->get());
    new_rot.mV[VX] = ll_round(new_rot.mV[VX], OBJECT_ROTATION_PRECISION);
    new_rot.mV[VY] = ll_round(new_rot.mV[VY], OBJECT_ROTATION_PRECISION);
    new_rot.mV[VZ] = ll_round(new_rot.mV[VZ], OBJECT_ROTATION_PRECISION);

    // Note: must compare before conversion to radians, some value can go 'around' 360
    LLVector3 delta = new_rot - mLastEulerDegrees;

    if (delta.magVec() >= 0.0005f)
    {
        mLastEulerDegrees = new_rot;
        new_rot *= DEG_TO_RAD;

        LLQuaternion rotation;
        rotation.setQuat(new_rot.mV[VX], new_rot.mV[VY], new_rot.mV[VZ]);
        LL::GLTF::quat q;
        q[0] = rotation.mQ[VX];
        q[1] = rotation.mQ[VY];
        q[2] = rotation.mQ[VZ];
        q[3] = rotation.mQ[VW];

        node.setRotation(q);
    }

    mAsset->updateTransforms();
}

void LLFloaterGLTFAssetEditor::onMenuDoToSelected(const LLSD& userdata)
{
    std::string command = userdata.asString();

    if (command == "psr_paste")
    {
        // todo: implement
        // onPastePos();
        // onPasteSize();
        // onPasteRot();
    }
    else if (command == "pos_paste")
    {
        // todo: implement
    }
    else if (command == "size_paste")
    {
        // todo: implement
    }
    else if (command == "rot_paste")
    {
        // todo: implement
    }
    else if (command == "psr_copy")
    {
        // onCopyPos();
        // onCopySize();
        // onCopyRot();
    }
    else if (command == "pos_copy")
    {
        // todo: implement
    }
    else if (command == "size_copy")
    {
        // todo: implement
    }
    else if (command == "rot_copy")
    {
        // todo: implement
    }
}

bool LLFloaterGLTFAssetEditor::onMenuEnableItem(const LLSD& userdata)
{
    if (!mFolderRoot)
    {
        return false;
    }

    LLFolderViewItem* item = mFolderRoot->getCurSelectedItem();
    if (!item)
    {
        return false;
    }

    LLGLTFFolderItem* vmi = static_cast<LLGLTFFolderItem*>(item->getViewModelItem());

    if (!vmi || vmi->getType() != LLGLTFFolderItem::TYPE_NODE)
    {
        return false;
    }

    std::string command = userdata.asString();
    if (command == "pos_paste" || command == "size_paste" || command == "rot_paste")
    {
        // todo: implement
        return true;
    }
    if (command == "psr_copy")
    {
        // todo: implement
        return true;
    }

    return false;
}

