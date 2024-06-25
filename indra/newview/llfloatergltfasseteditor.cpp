/**
 * @file llfloatergltfasseteditor.cpp
 * @author Brad Payne
 * @brief LLFloaterFontTest class implementation
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

#include "llfolderviewitem.h"
#include "llgltffolderviews.h"

bool LLGLTFSort::operator()(const LLGLTFItem* const& a, const LLGLTFItem* const& b) const
{
    // Comparison operator: returns "true" is a comes before b, "false" otherwise
    S32 compare = LLStringUtil::compareDict(a->getName(), b->getName());
    return (compare < 0);
}

/// LLGLTFViewModel

LLGLTFViewModel::LLGLTFViewModel()
    : base_t(new LLGLTFSort(), new LLGLTFFilter())
{}

void LLGLTFViewModel::sort(LLFolderViewFolder* folder)
{
    base_t::sort(folder);
}

 /// LLGLTFNode
// LLUICtrlFactory::create<LLGLTFNode>(params);
class LLGLTFNode : public LLFolderViewItem
{
public:
    struct Params : public LLInitParam::Block<Params, LLFolderViewItem::Params>
    {
        Params();
    };
    ~LLGLTFNode();
protected:
    LLGLTFNode(const Params& p);
};

LLGLTFNode::LLGLTFNode(const LLGLTFNode::Params& p)
    : LLFolderViewItem(p)
{
}

LLGLTFNode::~LLGLTFNode()
{
}

LLFloaterGltfAssetEditor::LLFloaterGltfAssetEditor(const LLSD& key)
    : LLFloater(key)
{
    setTitle("GLTF Asset Editor (WIP)");
}

/// LLFloaterGltfAssetEditor

LLFloaterGltfAssetEditor::~LLFloaterGltfAssetEditor()
{
}

bool LLFloaterGltfAssetEditor::postBuild()
{
    mItemListPanel = getChild<LLPanel>("item_list_panel");

    // Create the root model and view for all conversation sessions
    LLGLTFItem* base_item = new LLGLTFItem(mGLTFViewModel);

    LLFolderView::Params p(LLUICtrlFactory::getDefaultParams<LLFolderView>());
    p.name = getName();
    p.title = getLabel();
    p.rect = LLRect(0, 0, getRect().getWidth(), 0);
    p.parent_panel = mItemListPanel;
    p.tool_tip = p.name;
    p.listener = base_item;
    p.view_model = &mGLTFViewModel;
    p.root = NULL;
    p.use_ellipses = true;
    p.options_menu = "menu_gltf.xml"; // *TODO : create this or fix to be optional
    mConversationsRoot = LLUICtrlFactory::create<LLFolderView>(p);
    mConversationsRoot->setCallbackRegistrar(&mCommitCallbackRegistrar);
    mConversationsRoot->setEnableRegistrar(&mEnableCallbackRegistrar);

    return true;
}
