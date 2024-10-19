/**
 * @file llfloaterviewercef.h
 * @author Callum Prentice
 * @brief Floater for testing CEF native integration in the Viewer
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

#ifndef LL_LLFLOATERVIEWERCEF_H
#define LL_LLFLOATERVIEWERCEF_H

#include "llfloater.h"
#include "llmediactrl.h"

class LLTabContainer;
class LLPanel;
class LLMediaCtrl;
class LLComboBox;

class LLFloaterViewerCef : public LLFloater
{
        friend class LLFloaterReg;

    private:
        LLFloaterViewerCef(const LLSD& key);
        bool postBuild() override;
        ~LLFloaterViewerCef();

        const int mMaxTabCount = 8;

        int mTabNumber;
        LLTabContainer* mTabs;

        LLButton* addCefNativeTabBtn;
        void      addCefNativeTab(int tab_number, bool selected);

        LLButton* addCefPluginTabBtn;
        void      addCefPluginTab(int tab_number, bool selected);

        LLButton* closeTabBtn;
        void closeSelectedTab();

        void activateBookmark(LLUICtrl* btn);

        LLButton* closeFloaterBtn;
};

class LLPanelCefPluginTab :
    public LLPanel,
    public LLViewerMediaObserver
{
    public:
        LLPanelCefPluginTab(int tab_id);
        ~LLPanelCefPluginTab();

    protected:
        void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event) override;

    private:
        bool postBuild() override;

        int mTabId;

        LLComboBox*  mAddressCombo;
        LLMediaCtrl* mWebBrowser;
};

class LLPanelCefNativeTab : public LLPanel, public LLViewerMediaObserver
{
    public:
        LLPanelCefNativeTab(int tab_id);
        ~LLPanelCefNativeTab();

    protected:
        void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event) override;

    private:
        bool postBuild() override;

        int mTabId;

        LLComboBox*  mAddressCombo;
        LLMediaCtrl* mWebBrowser;
};

#endif // LL_LLFLOATERVIEWERCEF_H
