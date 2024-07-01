/**
 * @file llgltffoldermodel.cpp
 * @author Andrey Kleshchev
 * @brief gltf model's folder structure related classes
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

#include "llgltffoldermodel.h"

#include "llfolderviewitem.h"

bool LLGLTFSort::operator()(const LLGLTFFolderItem* const& a, const LLGLTFFolderItem* const& b) const
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
