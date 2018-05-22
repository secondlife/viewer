/** 
 * @file llflyoutcombobtn.cpp
 * @brief Represents outfit save/save as combo button.
 *
 * $LicenseInfo:firstyear=2010&license=viewerlgpl$
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

#include "llflyoutcombobtn.h"
#include "llviewermenu.h"

LLFlyoutComboBtnCtrl::LLFlyoutComboBtnCtrl(LLPanel* parent, const std::string &action_button, const std::string &flyout_button, const std::string &menu_file) :
	mParent(parent),
    mActionButton(action_button),
    mFlyoutButton(flyout_button)
{
	// register action mapping before creating menu
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar save_registar;
    save_registar.add("FlyoutCombo.Button.Action", [this](LLUICtrl *ctrl, const LLSD &data) { onFlyoutItemSelected(ctrl, data); });

    mParent->childSetAction(flyout_button, [this](LLUICtrl *ctrl, const LLSD &data) { onFlyoutButton(ctrl, data); });
    mParent->childSetAction(action_button, [this](LLUICtrl *ctrl, const LLSD &data) { onFlyoutAction(ctrl, data); });

	mFlyoutMenu = LLUICtrlFactory::getInstance()->createFromFile<LLToggleableMenu> (menu_file, gMenuHolder,
			LLViewerMenuHolderGL::child_registry_t::instance());

    // select the first item in the list.
    setSelectedItem(0);
}

void LLFlyoutComboBtnCtrl::setAction(LLUICtrl::commit_callback_t cb)
{
    mActionSignal.connect(cb);
}


U32 LLFlyoutComboBtnCtrl::getItemCount()
{
    return mFlyoutMenu->getItemCount();
}

void LLFlyoutComboBtnCtrl::setSelectedItem(S32 itemno)
{
    LLMenuItemGL *pitem = mFlyoutMenu->getItem(itemno);
    setSelectedItem(pitem);
}

void LLFlyoutComboBtnCtrl::setSelectedItem(const std::string &item)
{
    LLMenuItemGL *pitem = mFlyoutMenu->getChild<LLMenuItemGL>(item, false);
    setSelectedItem(pitem);
}

void LLFlyoutComboBtnCtrl::setSelectedItem(LLMenuItemGL *pitem)
{
    if (!pitem)
    {
        LL_WARNS("INTERFACE") << "NULL item selected" << LL_ENDL;
        return;
    }

    mSelectedName = pitem->getName();
    
    LLButton *action_button = mParent->getChild<LLButton>(mActionButton);
    action_button->setEnabled(pitem->getEnabled());
    action_button->setLabel(pitem->getLabel());
}

void LLFlyoutComboBtnCtrl::setMenuItemEnabled(const std::string& item, bool enabled)
{
    mFlyoutMenu->setItemEnabled(item, enabled);
    if (item == mSelectedName)
    {
        mParent->getChildView(mActionButton)->setEnabled(enabled);
    }
}

void LLFlyoutComboBtnCtrl::setShownBtnEnabled(bool enabled)
{
    mParent->getChildView(mActionButton)->setEnabled(enabled);
}

void LLFlyoutComboBtnCtrl::onFlyoutButton(LLUICtrl *ctrl, const LLSD &data)
{
	S32 x, y;
	LLUI::getMousePositionLocal(mParent, &x, &y);

	mFlyoutMenu->updateParent(LLMenuGL::sMenuContainer);
	LLMenuGL::showPopup(mParent, mFlyoutMenu, x, y);
}

void LLFlyoutComboBtnCtrl::onFlyoutItemSelected(LLUICtrl *ctrl, const LLSD &data)
{
    LLMenuItemGL *pmenuitem = static_cast<LLMenuItemGL*>(ctrl);
    setSelectedItem(pmenuitem);

    onFlyoutAction(pmenuitem, data);
}

void LLFlyoutComboBtnCtrl::onFlyoutAction(LLUICtrl *ctrl, const LLSD &data)
{
    LLMenuItemGL *pmenuitem = mFlyoutMenu->getChild<LLMenuItemGL>(mSelectedName);

    if (!mActionSignal.empty())
        mActionSignal(pmenuitem, data);
}

