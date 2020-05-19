/** 
* @file llfloatercamerapresets.h
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
#ifndef LLFLOATERCAMERAPRESETS_H
#define LLFLOATERCAMERAPRESETS_H

#include "llfloater.h"
#include "llflatlistview.h"

class LLFloaterReg;

class LLFloaterCameraPresets : public LLFloater
{
    friend class LLFloaterReg;

    virtual BOOL postBuild();
    virtual void onOpen(const LLSD& key);

    void populateList();

private:
    LLFloaterCameraPresets(const LLSD& key);
    ~LLFloaterCameraPresets();

    LLFlatListView* mPresetList;
};

class LLCameraPresetFlatItem : public LLPanel
{
public:
    LLCameraPresetFlatItem(const std::string &preset_name, bool is_default);
    virtual ~LLCameraPresetFlatItem();

    void setValue(const LLSD& value);

    virtual BOOL postBuild();
    virtual void onMouseEnter(S32 x, S32 y, MASK mask);
    virtual void onMouseLeave(S32 x, S32 y, MASK mask);

private:
    void onDeleteBtnClick();
    void onResetBtnClick();

    LLButton* mDeleteBtn;
    LLButton* mResetBtn;

    std::string mPresetName;
    bool mIsDefaultPrest;

};

#endif
