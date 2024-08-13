/**
* @file llfloatersettingscolor.cpp
* @brief Implementation of LLFloaterSettingsColor
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

#include "llviewerprecompiledheaders.h"

#include "llfloatersettingscolor.h"

#include "llfloater.h"
#include "llfiltereditor.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
#include "llspinctrl.h"
#include "llcolorswatch.h"
#include "llviewercontrol.h"
#include "lltexteditor.h"


LLFloaterSettingsColor::LLFloaterSettingsColor(const LLSD& key)
:   LLFloater(key),
    mSettingList(NULL)
{
    mCommitCallbackRegistrar.add("CommitSettings",  boost::bind(&LLFloaterSettingsColor::onCommitSettings, this));
    mCommitCallbackRegistrar.add("ClickDefault",    boost::bind(&LLFloaterSettingsColor::onClickDefault, this));
}

LLFloaterSettingsColor::~LLFloaterSettingsColor()
{}

bool LLFloaterSettingsColor::postBuild()
{
    enableResizeCtrls(true, false, true);

    mAlphaSpinner = getChild<LLSpinCtrl>("alpha_spinner");
    mColorSwatch = getChild<LLColorSwatchCtrl>("color_swatch");

    mDefaultButton = getChild<LLUICtrl>("default_btn");
    mSettingNameText = getChild<LLTextBox>("color_name_txt");

    getChild<LLFilterEditor>("filter_input")->setCommitCallback(boost::bind(&LLFloaterSettingsColor::setSearchFilter, this, _2));

    mSettingList = getChild<LLScrollListCtrl>("setting_list");
    mSettingList->setCommitOnSelectionChange(true);
    mSettingList->setCommitCallback(boost::bind(&LLFloaterSettingsColor::onSettingSelect, this));

    updateList();

    gSavedSettings.getControl("ColorSettingsHideDefault")->getCommitSignal()->connect(boost::bind(&LLFloaterSettingsColor::updateList, this, false));

    return LLFloater::postBuild();
}

void LLFloaterSettingsColor::draw()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        if(auto cell = first_selected->getColumn(1))
        {
            updateControl(cell->getValue().asString());
        }
    }

    LLFloater::draw();
}

void LLFloaterSettingsColor::onCommitSettings()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (!first_selected)
    {
        return;
    }
    auto cell = first_selected->getColumn(1);

    if (!cell)
    {
        return;
    }

    auto color_name = cell->getValue().asString();
    if (color_name.empty())
    {
        return;
    }

    LLColor4 col4;
    LLColor3 col3;
    col3.setValue(mColorSwatch->getValue());
    col4 = LLColor4(col3, (F32)mAlphaSpinner->getValue().asReal());
    LLUIColorTable::instance().setColor(color_name, col4);

    updateDefaultColumn(color_name);
}

// static
void LLFloaterSettingsColor::onClickDefault()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        auto cell = first_selected->getColumn(1);
        if (cell)
        {
            auto name = cell->getValue().asString();
            LLUIColorTable::instance().resetToDefault(name);
            updateDefaultColumn(name);
            updateControl(name);
        }
    }
}

// we've switched controls, or doing per-frame update, so update spinners, etc.
void LLFloaterSettingsColor::updateControl(const std::string& color_name)
{
    hideUIControls();

    if (!isSettingHidden(color_name))
    {
        mDefaultButton->setVisible(true);
        mSettingNameText->setVisible(true);
        mSettingNameText->setText(color_name);
        mSettingNameText->setToolTip(color_name);

        LLColor4 clr = LLUIColorTable::instance().getColor(color_name);
        mColorSwatch->setVisible(true);
        // only set if changed so color picker doesn't update
        if (clr != LLColor4(mColorSwatch->getValue()))
        {
            mColorSwatch->setOriginal(clr);
        }
        mAlphaSpinner->setVisible(true);
        mAlphaSpinner->setLabel(std::string("Alpha"));
        if (!mAlphaSpinner->hasFocus())
        {
            mAlphaSpinner->setPrecision(3);
            mAlphaSpinner->setMinValue(0.0);
            mAlphaSpinner->setMaxValue(1.f);
            mAlphaSpinner->setValue(clr.mV[VALPHA]);
        }
    }

}

void LLFloaterSettingsColor::updateList(bool skip_selection)
{
    std::string last_selected;
    LLScrollListItem* item = mSettingList->getFirstSelected();
    if (item)
    {
        LLScrollListCell* cell = item->getColumn(1);
        if (cell)
        {
            last_selected = cell->getValue().asString();
         }
    }

    mSettingList->deleteAllItems();

    const auto& base_colors = LLUIColorTable::instance().getLoadedColors();
    for (const auto& pair : base_colors)
    {
        const auto& name = pair.first;
        if (matchesSearchFilter(name) && !isSettingHidden(name))
        {
            LLSD row;

            row["columns"][0]["column"] = "changed_color";
            row["columns"][0]["value"] = LLUIColorTable::instance().isDefault(name) ? "" : "*";

            row["columns"][1]["column"] = "color";
            row["columns"][1]["value"] = name;

            LLScrollListItem* item = mSettingList->addElement(row, ADD_BOTTOM, nullptr);
            if (!mSearchFilter.empty() && (last_selected == name) && !skip_selection)
            {
                std::string lower_name(name);
                LLStringUtil::toLower(lower_name);
                if (LLStringUtil::startsWith(lower_name, mSearchFilter))
                {
                    item->setSelected(true);
                }
            }
        }
    }

    for (const auto& pair : LLUIColorTable::instance().getUserColors())
    {
        const auto& name = pair.first;
        if (base_colors.find(name) == base_colors.end() && matchesSearchFilter(name) && !isSettingHidden(name))
        {
            LLSD row;

            row["columns"][0]["column"] = "changed_color";
            row["columns"][0]["value"] = LLUIColorTable::instance().isDefault(name) ? "" : "*";

            row["columns"][1]["column"] = "color";
            row["columns"][1]["value"] = name;

            LLScrollListItem* item = mSettingList->addElement(row, ADD_BOTTOM, nullptr);
            if (!mSearchFilter.empty() && (last_selected == name) && !skip_selection)
            {
                std::string lower_name(name);
                LLStringUtil::toLower(lower_name);
                if (LLStringUtil::startsWith(lower_name, mSearchFilter))
                {
                    item->setSelected(true);
                }
            }
        }
    }

    mSettingList->updateSort();

    if (!mSettingList->isEmpty())
    {
        if (mSettingList->hasSelectedItem())
        {
            mSettingList->scrollToShowSelected();
        }
        else if (!mSettingList->hasSelectedItem() && !mSearchFilter.empty() && !skip_selection)
        {
            if (!mSettingList->selectItemByPrefix(mSearchFilter, false, 1))
            {
                mSettingList->selectFirstItem();
            }
            mSettingList->scrollToShowSelected();
        }
    }
    else
    {
        LLSD row;

        row["columns"][0]["column"] = "changed_color";
        row["columns"][0]["value"] = "";
        row["columns"][1]["column"] = "color";
        row["columns"][1]["value"] = "No matching colors.";

        mSettingList->addElement(row);
        hideUIControls();
    }
}

void LLFloaterSettingsColor::onSettingSelect()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        auto cell = first_selected->getColumn(1);
        if (cell)
        {
            updateControl(cell->getValue().asString());
        }
    }
}

void LLFloaterSettingsColor::setSearchFilter(const std::string& filter)
{
    if(mSearchFilter == filter)
        return;
    mSearchFilter = filter;
    LLStringUtil::toLower(mSearchFilter);
    updateList();
}

bool LLFloaterSettingsColor::matchesSearchFilter(std::string setting_name)
{
    // If the search filter is empty, everything passes.
    if (mSearchFilter.empty()) return true;

    LLStringUtil::toLower(setting_name);
    std::string::size_type match_name = setting_name.find(mSearchFilter);

    return (std::string::npos != match_name);
}

bool LLFloaterSettingsColor::isSettingHidden(const std::string& color_name)
{
    static LLCachedControl<bool> hide_default(gSavedSettings, "ColorSettingsHideDefault", false);
    return hide_default && LLUIColorTable::instance().isDefault(color_name);
}

void LLFloaterSettingsColor::updateDefaultColumn(const std::string& color_name)
{
    if (isSettingHidden(color_name))
    {
        hideUIControls();
        updateList(true);
        return;
    }

    LLScrollListItem* item = mSettingList->getFirstSelected();
    if (item)
    {
        LLScrollListCell* cell = item->getColumn(0);
        if (cell)
        {
            std::string is_default = LLUIColorTable::instance().isDefault(color_name) ? "" : "*";
            cell->setValue(is_default);
        }
    }
}

void LLFloaterSettingsColor::hideUIControls()
{
    mColorSwatch->setVisible(false);
    mAlphaSpinner->setVisible(false);
    mDefaultButton->setVisible(false);
    mSettingNameText->setVisible(false);
}

