/**
 * @file llfloatersidepanelcontainer.h
 * @brief LLFloaterSidePanelContainer class
 *
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLFLOATERSIDEPANELCONTAINER_H
#define LL_LLFLOATERSIDEPANELCONTAINER_H

#include "llfloater.h"

/**
 * Class LLFloaterSidePanelContainer
 *
 * Provides an interface for all former Side Tray panels.
 *
 * This class helps to make sure that clicking a floater containing the side panel
 * doesn't make transient floaters (e.g. IM windows) hide, so that it's possible to
 * drag an inventory item from My Inventory window to a docked IM window,
 * i.e. share the item (see VWR-22891).
 */
class LLFloaterSidePanelContainer : public LLFloater
{
private:
    static const std::string sMainPanelName;

public:
    LLFloaterSidePanelContainer(const LLSD& key, const Params& params = getDefaultParams());
    ~LLFloaterSidePanelContainer();

    bool postBuild() override;

    void onOpen(const LLSD& key) override;

    void closeFloater(bool app_quitting = false) override;

    void onClickCloseBtn(bool app_qutting) override;

    void cleanup() { destroy(); }

    LLPanel* openChildPanel(std::string_view panel_name, const LLSD& params);

    static LLFloater* getTopmostInventoryFloater();

    static void showPanel(std::string_view floater_name, const LLSD& key);

    static void showPanel(std::string_view floater_name, std::string_view panel_name, const LLSD& key);

    static LLPanel* getPanel(std::string_view floater_name, std::string_view panel_name = sMainPanelName);

    static LLPanel* findPanel(std::string_view floater_name, std::string_view panel_name = sMainPanelName);

    /**
     * Gets the panel of given type T (doesn't show it or do anything else with it).
     *
     * @param floater_name a string specifying the floater to be searched for a child panel.
     * @param panel_name a string specifying the child panel to get.
     * @returns a pointer to the panel of given type T.
     */
    template <typename T>
    static T* findPanel(std::string_view floater_name, std::string_view panel_name = sMainPanelName)
    {
        return dynamic_cast<T*>(findPanel(floater_name, panel_name));
    }
    template <typename T>
    static T* getPanel(std::string_view floater_name, std::string_view panel_name = sMainPanelName)
    {
        T* panel = dynamic_cast<T*>(getPanel(floater_name, panel_name));
        if (!panel)
        {
            LL_WARNS() << "Child named \"" << panel_name << "\" of type " << typeid(T*).name() << " not found" << LL_ENDL;
        }
        return panel;
    }

protected:
    void onCloseMsgCallback(const LLSD& notification, const LLSD& response);

    LLPanel* mMainPanel = nullptr;
};

#endif // LL_LLFLOATERSIDEPANELCONTAINER_H
