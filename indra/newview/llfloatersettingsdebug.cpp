/**
 * @file llfloatersettingsdebug.cpp
 * @brief floater for debugging internal viewer settings
 *
 * $LicenseInfo:firstyear=2022&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2022, Linden Research, Inc.
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
#include "llfloatersettingsdebug.h"
#include "llfloater.h"
#include "llfiltereditor.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
#include "llspinctrl.h"
#include "llcolorswatch.h"
#include "llviewercontrol.h"
#include "lltexteditor.h"
#include "llclipboard.h"
#include "llsdutil.h"


LLFloaterSettingsDebug::LLFloaterSettingsDebug(const LLSD& key)
:   LLFloater(key),
    mSettingList(NULL)
{
    mCommitCallbackRegistrar.add("CommitSettings",  { boost::bind(&LLFloaterSettingsDebug::onCommitSettings, this), cb_info::UNTRUSTED_BLOCK });
    mCommitCallbackRegistrar.add("ClickDefault",    { boost::bind(&LLFloaterSettingsDebug::onClickDefault, this), cb_info::UNTRUSTED_BLOCK });
}

LLFloaterSettingsDebug::~LLFloaterSettingsDebug()
{}

bool LLFloaterSettingsDebug::postBuild()
{
    enableResizeCtrls(true, false, true);

    mValSpinner1 = getChild<LLSpinCtrl>("val_spinner_1");
    mValSpinner2 = getChild<LLSpinCtrl>("val_spinner_2");
    mValSpinner3 = getChild<LLSpinCtrl>("val_spinner_3");
    mValSpinner4 = getChild<LLSpinCtrl>("val_spinner_4");
    mBooleanCombo = getChild<LLUICtrl>("boolean_combo");
    mValText = getChild<LLUICtrl>("val_text");

    mColorSwatch = getChild<LLColorSwatchCtrl>("val_color_swatch");

    mDefaultButton = getChild<LLUICtrl>("default_btn");
    mSettingNameText = getChild<LLTextBox>("setting_name_txt");

    mComment = getChild<LLTextEditor>("comment_text");
    mLLSDVal = getChild<LLTextEditor>("llsd_text");
    mCopyBtn = getChild<LLButton>("copy_btn");

    getChild<LLFilterEditor>("filter_input")->setCommitCallback(boost::bind(&LLFloaterSettingsDebug::setSearchFilter, this, _2));

    mSettingList = getChild<LLScrollListCtrl>("setting_list");
    mSettingList->setCommitOnSelectionChange(true);
    mSettingList->setCommitCallback(boost::bind(&LLFloaterSettingsDebug::onSettingSelect, this));

    mCopyBtn->setCommitCallback([this](LLUICtrl *ctrl, const LLSD &param) { onClickCopy(); });

    updateList();

    gSavedSettings.getControl("DebugSettingsHideDefault")->getCommitSignal()->connect(boost::bind(&LLFloaterSettingsDebug::updateList, this, false));

    return true;
}

void LLFloaterSettingsDebug::draw()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();
        updateControl(controlp);
    }

    LLFloater::draw();
}

void LLFloaterSettingsDebug::onCommitSettings()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (!first_selected)
    {
        return;
    }
    LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();

    if (!controlp)
    {
        return;
    }

    LLVector3 vector;
    LLVector3d vectord;
    LLQuaternion quat;
    LLRect rect;
    LLColor4 col4;
    LLColor3 col3;
    LLColor4U col4U;
    LLColor4 color_with_alpha;

    switch(controlp->type())
    {
      case TYPE_U32:
        controlp->set(mValSpinner1->getValue());
        break;
      case TYPE_S32:
        controlp->set(mValSpinner1->getValue());
        break;
      case TYPE_F32:
        controlp->set(LLSD(mValSpinner1->getValue().asReal()));
        break;
      case TYPE_BOOLEAN:
        controlp->set(mBooleanCombo->getValue());
        break;
      case TYPE_STRING:
        controlp->set(LLSD(mValText->getValue().asString()));
        break;
      case TYPE_VEC3:
        vector.mV[VX] = (F32)mValSpinner1->getValue().asReal();
        vector.mV[VY] = (F32)mValSpinner2->getValue().asReal();
        vector.mV[VZ] = (F32)mValSpinner3->getValue().asReal();
        controlp->set(vector.getValue());
        break;
      case TYPE_VEC3D:
        vectord.mdV[VX] = mValSpinner1->getValue().asReal();
        vectord.mdV[VY] = mValSpinner2->getValue().asReal();
        vectord.mdV[VZ] = mValSpinner3->getValue().asReal();
        controlp->set(vectord.getValue());
        break;
      case TYPE_QUAT:
        quat.mQ[VX] = mValSpinner1->getValueF32();
        quat.mQ[VY] = mValSpinner2->getValueF32();
        quat.mQ[VZ] = mValSpinner3->getValueF32();
        quat.mQ[VS] = mValSpinner4->getValueF32();
        controlp->set(quat.getValue());
        break;
      case TYPE_RECT:
        rect.mLeft = mValSpinner1->getValue().asInteger();
        rect.mRight = mValSpinner2->getValue().asInteger();
        rect.mBottom = mValSpinner3->getValue().asInteger();
        rect.mTop = mValSpinner4->getValue().asInteger();
        controlp->set(rect.getValue());
        break;
      case TYPE_COL4:
        col3.setValue(mColorSwatch->getValue());
        col4 = LLColor4(col3, (F32)mValSpinner4->getValue().asReal());
        controlp->set(col4.getValue());
        break;
      case TYPE_COL3:
        controlp->set(mColorSwatch->getValue());
        //col3.mV[VRED] = (F32)floaterp->mValSpinner1->getValue().asC();
        //col3.mV[VGREEN] = (F32)floaterp->mValSpinner2->getValue().asReal();
        //col3.mV[VBLUE] = (F32)floaterp->mValSpinner3->getValue().asReal();
        //controlp->set(col3.getValue());
        break;
      default:
        break;
    }
    updateDefaultColumn(controlp);
}

// static
void LLFloaterSettingsDebug::onClickDefault()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();
        if (controlp)
        {
            controlp->resetToDefault(true);
            updateDefaultColumn(controlp);
            updateControl(controlp);
        }
    }
}

// we've switched controls, or doing per-frame update, so update spinners, etc.
void LLFloaterSettingsDebug::updateControl(LLControlVariable* controlp)
{
    hideUIControls();

    if (controlp && !isSettingHidden(controlp))
    {
        eControlType type = controlp->type();

        //hide combo box only for non booleans, otherwise this will result in the combo box closing every frame
        mBooleanCombo->setVisible( type == TYPE_BOOLEAN);
        mDefaultButton->setVisible(true);
        mSettingNameText->setVisible(true);
        mSettingNameText->setText(controlp->getName());
        mSettingNameText->setToolTip(controlp->getName());
        mCopyBtn->setVisible(true);
        mComment->setVisible(true);

        std::string old_text = mComment->getText();
        std::string new_text = controlp->getComment();
        // Don't setText if not nessesary, it will reset scroll
        // This is a debug UI that reads from xml, there might
        // be use cases where comment changes, but not the name
        if (old_text != new_text)
        {
            mComment->setText(controlp->getComment());
        }

        mValSpinner1->setMaxValue(F32_MAX);
        mValSpinner2->setMaxValue(F32_MAX);
        mValSpinner3->setMaxValue(F32_MAX);
        mValSpinner4->setMaxValue(F32_MAX);
        mValSpinner1->setMinValue(-F32_MAX);
        mValSpinner2->setMinValue(-F32_MAX);
        mValSpinner3->setMinValue(-F32_MAX);
        mValSpinner4->setMinValue(-F32_MAX);
        if (!mValSpinner1->hasFocus())
        {
            mValSpinner1->setIncrement(0.1f);
        }
        if (!mValSpinner2->hasFocus())
        {
            mValSpinner2->setIncrement(0.1f);
        }
        if (!mValSpinner3->hasFocus())
        {
            mValSpinner3->setIncrement(0.1f);
        }
        if (!mValSpinner4->hasFocus())
        {
            mValSpinner4->setIncrement(0.1f);
        }

        LLSD sd = controlp->get();
        switch(type)
        {
          case TYPE_U32:
            mValSpinner1->setVisible(true);
            mValSpinner1->setLabel(std::string("value")); // Debug, don't translate
            if (!mValSpinner1->hasFocus())
            {
                mValSpinner1->setValue(sd);
                mValSpinner1->setMinValue((F32)U32_MIN);
                mValSpinner1->setMaxValue((F32)U32_MAX);
                mValSpinner1->setIncrement(1.f);
                mValSpinner1->setPrecision(0);
            }
            break;
          case TYPE_S32:
            mValSpinner1->setVisible(true);
            mValSpinner1->setLabel(std::string("value")); // Debug, don't translate
            if (!mValSpinner1->hasFocus())
            {
                mValSpinner1->setValue(sd);
                mValSpinner1->setMinValue((F32)S32_MIN);
                mValSpinner1->setMaxValue((F32)S32_MAX);
                mValSpinner1->setIncrement(1.f);
                mValSpinner1->setPrecision(0);
            }
            break;
          case TYPE_F32:
            mValSpinner1->setVisible(true);
            mValSpinner1->setLabel(std::string("value")); // Debug, don't translate
            if (!mValSpinner1->hasFocus())
            {
                mValSpinner1->setPrecision(3);
                mValSpinner1->setValue(sd);
            }
            break;
          case TYPE_BOOLEAN:
            if (!mBooleanCombo->hasFocus())
            {
                if (sd.asBoolean())
                {
                    mBooleanCombo->setValue(LLSD("true"));
                }
                else
                {
                    mBooleanCombo->setValue(LLSD(""));
                }
            }
            break;
          case TYPE_STRING:
            mValText->setVisible( true);
            if (!mValText->hasFocus())
            {
                mValText->setValue(sd);
            }
            break;
          case TYPE_VEC3:
          {
            LLVector3 v;
            v.setValue(sd);
            mValSpinner1->setVisible(true);
            mValSpinner1->setLabel(std::string("X"));
            mValSpinner2->setVisible(true);
            mValSpinner2->setLabel(std::string("Y"));
            mValSpinner3->setVisible(true);
            mValSpinner3->setLabel(std::string("Z"));
            if (!mValSpinner1->hasFocus())
            {
                mValSpinner1->setPrecision(3);
                mValSpinner1->setValue(v[VX]);
            }
            if (!mValSpinner2->hasFocus())
            {
                mValSpinner2->setPrecision(3);
                mValSpinner2->setValue(v[VY]);
            }
            if (!mValSpinner3->hasFocus())
            {
                mValSpinner3->setPrecision(3);
                mValSpinner3->setValue(v[VZ]);
            }
            break;
          }
          case TYPE_VEC3D:
          {
            LLVector3d v;
            v.setValue(sd);
            mValSpinner1->setVisible(true);
            mValSpinner1->setLabel(std::string("X"));
            mValSpinner2->setVisible(true);
            mValSpinner2->setLabel(std::string("Y"));
            mValSpinner3->setVisible(true);
            mValSpinner3->setLabel(std::string("Z"));
            if (!mValSpinner1->hasFocus())
            {
                mValSpinner1->setPrecision(3);
                mValSpinner1->setValue(v[VX]);
            }
            if (!mValSpinner2->hasFocus())
            {
                mValSpinner2->setPrecision(3);
                mValSpinner2->setValue(v[VY]);
            }
            if (!mValSpinner3->hasFocus())
            {
                mValSpinner3->setPrecision(3);
                mValSpinner3->setValue(v[VZ]);
            }
            break;
          }
          case TYPE_QUAT:
          {
              LLQuaternion q;
              q.setValue(sd);
              mValSpinner1->setVisible(true);
              mValSpinner1->setLabel(std::string("X"));
              mValSpinner2->setVisible(true);
              mValSpinner2->setLabel(std::string("Y"));
              mValSpinner3->setVisible(true);
              mValSpinner3->setLabel(std::string("Z"));
              mValSpinner4->setVisible(true);
              mValSpinner4->setLabel(std::string("S"));
              if (!mValSpinner1->hasFocus())
              {
                  mValSpinner1->setPrecision(4);
                  mValSpinner1->setValue(q.mQ[VX]);
              }
              if (!mValSpinner2->hasFocus())
              {
                  mValSpinner2->setPrecision(4);
                  mValSpinner2->setValue(q.mQ[VY]);
              }
              if (!mValSpinner3->hasFocus())
              {
                  mValSpinner3->setPrecision(4);
                  mValSpinner3->setValue(q.mQ[VZ]);
              }
              if (!mValSpinner4->hasFocus())
              {
                  mValSpinner4->setPrecision(4);
                  mValSpinner4->setValue(q.mQ[VS]);
              }
              break;
          }
          case TYPE_RECT:
          {
            LLRect r;
            r.setValue(sd);
            mValSpinner1->setVisible(true);
            mValSpinner1->setLabel(std::string("Left"));
            mValSpinner2->setVisible(true);
            mValSpinner2->setLabel(std::string("Right"));
            mValSpinner3->setVisible(true);
            mValSpinner3->setLabel(std::string("Bottom"));
            mValSpinner4->setVisible(true);
            mValSpinner4->setLabel(std::string("Top"));
            if (!mValSpinner1->hasFocus())
            {
                mValSpinner1->setPrecision(0);
                mValSpinner1->setValue(r.mLeft);
            }
            if (!mValSpinner2->hasFocus())
            {
                mValSpinner2->setPrecision(0);
                mValSpinner2->setValue(r.mRight);
            }
            if (!mValSpinner3->hasFocus())
            {
                mValSpinner3->setPrecision(0);
                mValSpinner3->setValue(r.mBottom);
            }
            if (!mValSpinner4->hasFocus())
            {
                mValSpinner4->setPrecision(0);
                mValSpinner4->setValue(r.mTop);
            }

            mValSpinner1->setMinValue((F32)S32_MIN);
            mValSpinner1->setMaxValue((F32)S32_MAX);
            mValSpinner1->setIncrement(1.f);

            mValSpinner2->setMinValue((F32)S32_MIN);
            mValSpinner2->setMaxValue((F32)S32_MAX);
            mValSpinner2->setIncrement(1.f);

            mValSpinner3->setMinValue((F32)S32_MIN);
            mValSpinner3->setMaxValue((F32)S32_MAX);
            mValSpinner3->setIncrement(1.f);

            mValSpinner4->setMinValue((F32)S32_MIN);
            mValSpinner4->setMaxValue((F32)S32_MAX);
            mValSpinner4->setIncrement(1.f);
            break;
          }
          case TYPE_COL4:
          {
            LLColor4 clr;
            clr.setValue(sd);
            mColorSwatch->setVisible(true);
            // only set if changed so color picker doesn't update
            if(clr != LLColor4(mColorSwatch->getValue()))
            {
                mColorSwatch->set(LLColor4(sd), true, false);
            }
            mValSpinner4->setVisible(true);
            mValSpinner4->setLabel(std::string("Alpha"));
            if (!mValSpinner4->hasFocus())
            {
                mValSpinner4->setPrecision(3);
                mValSpinner4->setMinValue(0.0);
                mValSpinner4->setMaxValue(1.f);
                mValSpinner4->setValue(clr.mV[VALPHA]);
            }
            break;
          }
          case TYPE_COL3:
          {
            LLColor3 clr;
            clr.setValue(sd);
            mColorSwatch->setVisible(true);
            mColorSwatch->setValue(sd);
            break;
          }
          case TYPE_LLSD:
          {
            mLLSDVal->setVisible(true);
            std::string new_text = ll_pretty_print_sd(sd);
            // Don't setText if not nessesary, it will reset scroll
            if (mLLSDVal->getText() != new_text)
            {
                mLLSDVal->setText(new_text);
            }
            break;
          }
          default:
            mComment->setText(std::string("unknown"));
            break;
        }
    }

}

void LLFloaterSettingsDebug::updateList(bool skip_selection)
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
    struct f : public LLControlGroup::ApplyFunctor
    {
        LLScrollListCtrl* setting_list;
        LLFloaterSettingsDebug* floater;
        std::string selected_setting;
        bool skip_selection;
        f(LLScrollListCtrl* list, LLFloaterSettingsDebug* floater, std::string setting, bool skip_selection)
            : setting_list(list), floater(floater), selected_setting(setting), skip_selection(skip_selection) {}
        virtual void apply(const std::string& name, LLControlVariable* control)
        {
            if (!control->isHiddenFromSettingsEditor() && floater->matchesSearchFilter(name) && !floater->isSettingHidden(control))
            {
                LLSD row;

                row["columns"][0]["column"] = "changed_setting";
                row["columns"][0]["value"] = control->isDefault() ? "" : "*";

                row["columns"][1]["column"] = "setting";
                row["columns"][1]["value"] = name;

                LLScrollListItem* item = setting_list->addElement(row, ADD_BOTTOM, (void*)control);
                if (!floater->mSearchFilter.empty() && (selected_setting == name) && !skip_selection)
                {
                    std::string lower_name(name);
                    LLStringUtil::toLower(lower_name);
                    if (LLStringUtil::startsWith(lower_name, floater->mSearchFilter))
                    {
                        item->setSelected(true);
                    }
                }
            }
        }
    } func(mSettingList, this, last_selected, skip_selection);

    std::string key = getKey().asString();
    if (key == "all" || key == "base")
    {
        gSavedSettings.applyToAll(&func);
    }
    if (key == "all" || key == "account")
    {
        gSavedPerAccountSettings.applyToAll(&func);
    }


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

        row["columns"][0]["column"] = "changed_setting";
        row["columns"][0]["value"] = "";
        row["columns"][1]["column"] = "setting";
        row["columns"][1]["value"] = "No matching settings.";

        mSettingList->addElement(row);
        hideUIControls();
    }
}

void LLFloaterSettingsDebug::onSettingSelect()
{
    LLScrollListItem* first_selected = mSettingList->getFirstSelected();
    if (first_selected)
    {
        LLControlVariable* controlp = (LLControlVariable*)first_selected->getUserdata();
        if (controlp)
        {
            updateControl(controlp);
        }
    }
}

void LLFloaterSettingsDebug::setSearchFilter(const std::string& filter)
{
    if(mSearchFilter == filter)
        return;
    mSearchFilter = filter;
    LLStringUtil::toLower(mSearchFilter);
    updateList();
}

bool LLFloaterSettingsDebug::matchesSearchFilter(std::string setting_name)
{
    // If the search filter is empty, everything passes.
    if (mSearchFilter.empty()) return true;

    LLStringUtil::toLower(setting_name);
    std::string::size_type match_name = setting_name.find(mSearchFilter);

    return (std::string::npos != match_name);
}

bool LLFloaterSettingsDebug::isSettingHidden(LLControlVariable* control)
{
    static LLCachedControl<bool> hide_default(gSavedSettings, "DebugSettingsHideDefault", false);
    return hide_default && control->isDefault();
}

void LLFloaterSettingsDebug::updateDefaultColumn(LLControlVariable* control)
{
    if (isSettingHidden(control))
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
            std::string is_default = control->isDefault() ? "" : "*";
            cell->setValue(is_default);
        }
    }
}

void LLFloaterSettingsDebug::hideUIControls()
{
    mValSpinner1->setVisible(false);
    mValSpinner2->setVisible(false);
    mValSpinner3->setVisible(false);
    mValSpinner4->setVisible(false);
    mColorSwatch->setVisible(false);
    mValText->setVisible(false);
    mDefaultButton->setVisible(false);
    mBooleanCombo->setVisible(false);
    mLLSDVal->setVisible(false);
    mSettingNameText->setVisible(false);
    mCopyBtn->setVisible(false);
    mComment->setVisible(false);
}

void LLFloaterSettingsDebug::onClickCopy()
{
    std::string setting_name = mSettingNameText->getText();
    LLClipboard::instance().copyToClipboard(utf8str_to_wstring(setting_name), 0, narrow(setting_name.size()));
}
