/**
* @file llfloatersettingscolor.h
* @brief Header file for LLFloaterSettingsColor
* @author Rye Cogtail<rye@alchemyviewer.org>
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

#ifndef LLFLOATERCOLORSETTINGS_H
#define LLFLOATERCOLORSETTINGS_H

#include "llcontrol.h"
#include "llfloater.h"

class LLColorSwatchCtrl;
class LLScrollListCtrl;
class LLSpinCtrl;
class LLTextBox;

class LLFloaterSettingsColor final
:   public LLFloater
{
    friend class LLFloaterReg;

public:

    bool postBuild() override;
    void draw() override;

    void updateControl(const std::string& color_name);

    void onCommitSettings();
    void onClickDefault();

    bool matchesSearchFilter(std::string setting_name);
    bool isSettingHidden(const std::string& color_name);

private:
    LLFloaterSettingsColor(const LLSD& key);
    virtual ~LLFloaterSettingsColor();

    void updateList(bool skip_selection = false);
    void onSettingSelect();
    void setSearchFilter(const std::string& filter);

    void updateDefaultColumn(const std::string& color_name);
    void hideUIControls();

    LLScrollListCtrl* mSettingList;

protected:
    LLUICtrl*           mDefaultButton = nullptr;
    LLTextBox*          mSettingNameText = nullptr;

    LLSpinCtrl* mAlphaSpinner = nullptr;
    LLColorSwatchCtrl* mColorSwatch = nullptr;

    std::string mSearchFilter;
};

#endif //LLFLOATERCOLORSETTINGS_H

