/**
 * @file llfloatergltfasseteditor.h
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

#ifndef LL_LLGLTFFOLDERMODEL_H
#define LL_LLGLTFFOLDERMODEL_H

#include "llfolderviewmodel.h"
#include "llgltffolderitem.h"

class LLGLTFSort
{
public:
    LLGLTFSort() { }
    bool operator()(const LLGLTFFolderItem* const& a, const LLGLTFFolderItem* const& b) const;
private:
};

class LLGLTFFilter : public LLFolderViewFilter
{
public:
    LLGLTFFilter() { mEmpty = ""; }
    ~LLGLTFFilter() {}

    bool                check(const LLFolderViewModelItem* item) { return true; }
    bool                checkFolder(const LLFolderViewModelItem* folder) const { return true; }
    void                setEmptyLookupMessage(const std::string& message) { }
    std::string         getEmptyLookupMessage(bool is_empty_folder = false) const { return mEmpty; }
    bool                showAllResults() const { return true; }
    std::string::size_type getStringMatchOffset(LLFolderViewModelItem* item) const { return std::string::npos; }
    std::string::size_type getFilterStringSize() const { return 0; }

    bool                isActive() const { return false; }
    bool                isModified() const { return false; }
    void                clearModified() { }
    const std::string& getName() const { return mEmpty; }
    const std::string& getFilterText() { return mEmpty; }
    void                setModified(EFilterModified behavior = FILTER_RESTART) { }

    void                resetTime(S32 timeout) { }
    bool                isTimedOut() { return false; }

    bool                isDefault() const { return true; }
    bool                isNotDefault() const { return false; }
    void                markDefault() { }
    void                resetDefault() { }

    S32                 getCurrentGeneration() const { return 0; }
    S32                 getFirstSuccessGeneration() const { return 0; }
    S32                 getFirstRequiredGeneration() const { return 0; }
private:
    std::string mEmpty;
};

class LLGLTFViewModel
    : public LLFolderViewModel<LLGLTFSort, LLGLTFFolderItem, LLGLTFFolderItem, LLGLTFFilter>
{
public:
    typedef LLFolderViewModel< LLGLTFSort, LLGLTFFolderItem, LLGLTFFolderItem, LLGLTFFilter> base_t;
    LLGLTFViewModel();

    void sort(LLFolderViewFolder* folder);
    bool startDrag(std::vector<LLFolderViewModelItem*>& items) { return false; }

private:
};

#endif
