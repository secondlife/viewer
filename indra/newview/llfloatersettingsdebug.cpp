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

    mComment = getChild<LLTextEditor>("comment_text");
    mSettingName = getChild<LLTextBox>("setting_name_txt");
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
        controlp->set(getChild<LLUICtrl>("val_spinner_1")->getValue());
        break;
      case TYPE_S32:
        controlp->set(getChild<LLUICtrl>("val_spinner_1")->getValue());
        break;
      case TYPE_F32:
        controlp->set(LLSD(getChild<LLUICtrl>("val_spinner_1")->getValue().asReal()));
        break;
      case TYPE_BOOLEAN:
        controlp->set(getChild<LLUICtrl>("boolean_combo")->getValue());
        break;
      case TYPE_STRING:
        controlp->set(LLSD(getChild<LLUICtrl>("val_text")->getValue().asString()));
        break;
      case TYPE_VEC3:
        vector.mV[VX] = (F32)getChild<LLUICtrl>("val_spinner_1")->getValue().asReal();
        vector.mV[VY] = (F32)getChild<LLUICtrl>("val_spinner_2")->getValue().asReal();
        vector.mV[VZ] = (F32)getChild<LLUICtrl>("val_spinner_3")->getValue().asReal();
        controlp->set(vector.getValue());
        break;
      case TYPE_VEC3D:
        vectord.mdV[VX] = getChild<LLUICtrl>("val_spinner_1")->getValue().asReal();
        vectord.mdV[VY] = getChild<LLUICtrl>("val_spinner_2")->getValue().asReal();
        vectord.mdV[VZ] = getChild<LLUICtrl>("val_spinner_3")->getValue().asReal();
        controlp->set(vectord.getValue());
        break;
      case TYPE_QUAT:
        quat.mQ[VX] = getChild<LLUICtrl>("val_spinner_1")->getValue().asReal();
        quat.mQ[VY] = getChild<LLUICtrl>("val_spinner_2")->getValue().asReal();
        quat.mQ[VZ] = getChild<LLUICtrl>("val_spinner_3")->getValue().asReal();
        quat.mQ[VS] = getChild<LLUICtrl>("val_spinner_4")->getValue().asReal();;
        controlp->set(quat.getValue());
        break;
      case TYPE_RECT:
        rect.mLeft = getChild<LLUICtrl>("val_spinner_1")->getValue().asInteger();
        rect.mRight = getChild<LLUICtrl>("val_spinner_2")->getValue().asInteger();
        rect.mBottom = getChild<LLUICtrl>("val_spinner_3")->getValue().asInteger();
        rect.mTop = getChild<LLUICtrl>("val_spinner_4")->getValue().asInteger();
        controlp->set(rect.getValue());
        break;
      case TYPE_COL4:
        col3.setValue(getChild<LLUICtrl>("val_color_swatch")->getValue());
        col4 = LLColor4(col3, (F32)getChild<LLUICtrl>("val_spinner_4")->getValue().asReal());
        controlp->set(col4.getValue());
        break;
      case TYPE_COL3:
        controlp->set(getChild<LLUICtrl>("val_color_swatch")->getValue());
        //col3.mV[VRED] = (F32)floaterp->getChild<LLUICtrl>("val_spinner_1")->getValue().asC();
        //col3.mV[VGREEN] = (F32)floaterp->getChild<LLUICtrl>("val_spinner_2")->getValue().asReal();
        //col3.mV[VBLUE] = (F32)floaterp->getChild<LLUICtrl>("val_spinner_3")->getValue().asReal();
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
    LLSpinCtrl* spinner1 = getChild<LLSpinCtrl>("val_spinner_1");
    LLSpinCtrl* spinner2 = getChild<LLSpinCtrl>("val_spinner_2");
    LLSpinCtrl* spinner3 = getChild<LLSpinCtrl>("val_spinner_3");
    LLSpinCtrl* spinner4 = getChild<LLSpinCtrl>("val_spinner_4");
    LLColorSwatchCtrl* color_swatch = getChild<LLColorSwatchCtrl>("val_color_swatch");

    if (!spinner1 || !spinner2 || !spinner3 || !spinner4 || !color_swatch)
    {
        LL_WARNS() << "Could not find all desired controls by name"
            << LL_ENDL;
        return;
    }

    hideUIControls();

    if (controlp && !isSettingHidden(controlp))
    {
        eControlType type = controlp->type();

        //hide combo box only for non booleans, otherwise this will result in the combo box closing every frame
        getChildView("boolean_combo")->setVisible( type == TYPE_BOOLEAN);
        getChildView("default_btn")->setVisible(true);
        mSettingName->setVisible(true);
        mSettingName->setText(controlp->getName());
        mSettingName->setToolTip(controlp->getName());
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

        spinner1->setMaxValue(F32_MAX);
        spinner2->setMaxValue(F32_MAX);
        spinner3->setMaxValue(F32_MAX);
        spinner4->setMaxValue(F32_MAX);
        spinner1->setMinValue(-F32_MAX);
        spinner2->setMinValue(-F32_MAX);
        spinner3->setMinValue(-F32_MAX);
        spinner4->setMinValue(-F32_MAX);
        if (!spinner1->hasFocus())
        {
            spinner1->setIncrement(0.1f);
        }
        if (!spinner2->hasFocus())
        {
            spinner2->setIncrement(0.1f);
        }
        if (!spinner3->hasFocus())
        {
            spinner3->setIncrement(0.1f);
        }
        if (!spinner4->hasFocus())
        {
            spinner4->setIncrement(0.1f);
        }

        LLSD sd = controlp->get();
        switch(type)
        {
          case TYPE_U32:
            spinner1->setVisible(true);
            spinner1->setLabel(std::string("value")); // Debug, don't translate
            if (!spinner1->hasFocus())
            {
                spinner1->setValue(sd);
                spinner1->setMinValue((F32)U32_MIN);
                spinner1->setMaxValue((F32)U32_MAX);
                spinner1->setIncrement(1.f);
                spinner1->setPrecision(0);
            }
            break;
          case TYPE_S32:
            spinner1->setVisible(true);
            spinner1->setLabel(std::string("value")); // Debug, don't translate
            if (!spinner1->hasFocus())
            {
                spinner1->setValue(sd);
                spinner1->setMinValue((F32)S32_MIN);
                spinner1->setMaxValue((F32)S32_MAX);
                spinner1->setIncrement(1.f);
                spinner1->setPrecision(0);
            }
            break;
          case TYPE_F32:
            spinner1->setVisible(true);
            spinner1->setLabel(std::string("value")); // Debug, don't translate
            if (!spinner1->hasFocus())
            {
                spinner1->setPrecision(3);
                spinner1->setValue(sd);
            }
            break;
          case TYPE_BOOLEAN:
            if (!getChild<LLUICtrl>("boolean_combo")->hasFocus())
            {
                if (sd.asBoolean())
                {
                    getChild<LLUICtrl>("boolean_combo")->setValue(LLSD("true"));
                }
                else
                {
                    getChild<LLUICtrl>("boolean_combo")->setValue(LLSD(""));
                }
            }
            break;
          case TYPE_STRING:
            getChildView("val_text")->setVisible( true);
            if (!getChild<LLUICtrl>("val_text")->hasFocus())
            {
                getChild<LLUICtrl>("val_text")->setValue(sd);
            }
            break;
          case TYPE_VEC3:
          {
            LLVector3 v;
            v.setValue(sd);
            spinner1->setVisible(true);
            spinner1->setLabel(std::string("X"));
            spinner2->setVisible(true);
            spinner2->setLabel(std::string("Y"));
            spinner3->setVisible(true);
            spinner3->setLabel(std::string("Z"));
            if (!spinner1->hasFocus())
            {
                spinner1->setPrecision(3);
                spinner1->setValue(v[VX]);
            }
            if (!spinner2->hasFocus())
            {
                spinner2->setPrecision(3);
                spinner2->setValue(v[VY]);
            }
            if (!spinner3->hasFocus())
            {
                spinner3->setPrecision(3);
                spinner3->setValue(v[VZ]);
            }
            break;
          }
          case TYPE_VEC3D:
          {
            LLVector3d v;
            v.setValue(sd);
            spinner1->setVisible(true);
            spinner1->setLabel(std::string("X"));
            spinner2->setVisible(true);
            spinner2->setLabel(std::string("Y"));
            spinner3->setVisible(true);
            spinner3->setLabel(std::string("Z"));
            if (!spinner1->hasFocus())
            {
                spinner1->setPrecision(3);
                spinner1->setValue(v[VX]);
            }
            if (!spinner2->hasFocus())
            {
                spinner2->setPrecision(3);
                spinner2->setValue(v[VY]);
            }
            if (!spinner3->hasFocus())
            {
                spinner3->setPrecision(3);
                spinner3->setValue(v[VZ]);
            }
            break;
          }
          case TYPE_QUAT:
          {
              LLQuaternion q;
              q.setValue(sd);
              spinner1->setVisible(true);
              spinner1->setLabel(std::string("X"));
              spinner2->setVisible(true);
              spinner2->setLabel(std::string("Y"));
              spinner3->setVisible(true);
              spinner3->setLabel(std::string("Z"));
              spinner4->setVisible(true);
              spinner4->setLabel(std::string("S"));
              if (!spinner1->hasFocus())
              {
                  spinner1->setPrecision(4);
                  spinner1->setValue(q.mQ[VX]);
              }
              if (!spinner2->hasFocus())
              {
                  spinner2->setPrecision(4);
                  spinner2->setValue(q.mQ[VY]);
              }
              if (!spinner3->hasFocus())
              {
                  spinner3->setPrecision(4);
                  spinner3->setValue(q.mQ[VZ]);
              }
              if (!spinner4->hasFocus())
              {
                  spinner4->setPrecision(4);
                  spinner4->setValue(q.mQ[VS]);
              }
              break;
          }
          case TYPE_RECT:
          {
            LLRect r;
            r.setValue(sd);
            spinner1->setVisible(true);
            spinner1->setLabel(std::string("Left"));
            spinner2->setVisible(true);
            spinner2->setLabel(std::string("Right"));
            spinner3->setVisible(true);
            spinner3->setLabel(std::string("Bottom"));
            spinner4->setVisible(true);
            spinner4->setLabel(std::string("Top"));
            if (!spinner1->hasFocus())
            {
                spinner1->setPrecision(0);
                spinner1->setValue(r.mLeft);
            }
            if (!spinner2->hasFocus())
            {
                spinner2->setPrecision(0);
                spinner2->setValue(r.mRight);
            }
            if (!spinner3->hasFocus())
            {
                spinner3->setPrecision(0);
                spinner3->setValue(r.mBottom);
            }
            if (!spinner4->hasFocus())
            {
                spinner4->setPrecision(0);
                spinner4->setValue(r.mTop);
            }

            spinner1->setMinValue((F32)S32_MIN);
            spinner1->setMaxValue((F32)S32_MAX);
            spinner1->setIncrement(1.f);

            spinner2->setMinValue((F32)S32_MIN);
            spinner2->setMaxValue((F32)S32_MAX);
            spinner2->setIncrement(1.f);

            spinner3->setMinValue((F32)S32_MIN);
            spinner3->setMaxValue((F32)S32_MAX);
            spinner3->setIncrement(1.f);

            spinner4->setMinValue((F32)S32_MIN);
            spinner4->setMaxValue((F32)S32_MAX);
            spinner4->setIncrement(1.f);
            break;
          }
          case TYPE_COL4:
          {
            LLColor4 clr;
            clr.setValue(sd);
            color_swatch->setVisible(true);
            // only set if changed so color picker doesn't update
            if(clr != LLColor4(color_swatch->getValue()))
            {
                color_swatch->set(LLColor4(sd), true, false);
            }
            spinner4->setVisible(true);
            spinner4->setLabel(std::string("Alpha"));
            if (!spinner4->hasFocus())
            {
                spinner4->setPrecision(3);
                spinner4->setMinValue(0.0);
                spinner4->setMaxValue(1.f);
                spinner4->setValue(clr.mV[VALPHA]);
            }
            break;
          }
          case TYPE_COL3:
          {
            LLColor3 clr;
            clr.setValue(sd);
            color_swatch->setVisible(true);
            color_swatch->setValue(sd);
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
    getChildView("val_spinner_1")->setVisible(false);
    getChildView("val_spinner_2")->setVisible(false);
    getChildView("val_spinner_3")->setVisible(false);
    getChildView("val_spinner_4")->setVisible(false);
    getChildView("val_color_swatch")->setVisible(false);
    getChildView("val_text")->setVisible(false);
    getChildView("default_btn")->setVisible(false);
    getChildView("boolean_combo")->setVisible(false);
    mLLSDVal->setVisible(false);
    mSettingName->setVisible(false);
    mCopyBtn->setVisible(false);
    mComment->setVisible(false);
}

void LLFloaterSettingsDebug::onClickCopy()
{
    std::string setting_name = mSettingName->getText();
    LLClipboard::instance().copyToClipboard(utf8str_to_wstring(setting_name), 0, setting_name.size());
}
