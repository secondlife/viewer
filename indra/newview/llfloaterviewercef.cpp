/**
 * @file llfloaterviewercef.cpp
 * @author Callum Prentice
 * @brief LLFloaterViewerCef class implementation
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

#include "llviewerprecompiledheaders.h"

#include "lltabcontainer.h"
#include "llcombobox.h"
#include "llmediactrl.h"

#include "llfloaterviewercef.h"

LLFloaterViewerCef::LLFloaterViewerCef(const LLSD& key)
    :   LLFloater("floater_viewer_cef")
{
    mTabNumber = 101;
}

LLFloaterViewerCef::~LLFloaterViewerCef()
{
}

bool LLFloaterViewerCef::postBuild()
{
    mTabs = getChild<LLTabContainer>("viewer_cef_tabs");

    closeFloaterBtn = getChild<LLButton>("close_floater_btn");
    closeFloaterBtn->setCommitCallback([this](LLUICtrl*, const LLSD&)
    {
        closeFloater(false);
    });

    addCefPluginTabBtn = getChild<LLButton>("add_plugin_tab_btn");
    addCefPluginTabBtn->setCommitCallback([this](LLUICtrl*, const LLSD&)
    {
        addCefPluginTab(mTabNumber++, true);
    });

    addCefNativeTabBtn = getChild<LLButton>("add_native_tab_btn");
    addCefNativeTabBtn->setCommitCallback([this](LLUICtrl*, const LLSD&)
    {
        addCefNativeTab(mTabNumber++, true);
    });

    closeTabBtn = getChild<LLButton>("close_tab_btn");
    closeTabBtn->setCommitCallback([this](LLUICtrl*, const LLSD&)
    {
        closeSelectedTab();
    });

    getChild<LLButton>("bm_1_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        activateBookmark(btn);
    });
    getChild<LLButton>("bm_2_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        activateBookmark(btn);
    });
    getChild<LLButton>("bm_3_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        activateBookmark(btn);
    });
    getChild<LLButton>("bm_4_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        activateBookmark(btn);
    });

    addCefPluginTab(mTabNumber++, true);
    addCefPluginTab(mTabNumber++, false);
    addCefNativeTab(mTabNumber++, false);
    addCefNativeTab(mTabNumber++, false);

    this->center();

    return true;
}

void LLFloaterViewerCef::addCefPluginTab(int tab_number, bool selected)
{
    LLPanelCefPluginTab* cef_plugin_tab;

    cef_plugin_tab = new LLPanelCefPluginTab(tab_number);
    cef_plugin_tab->buildFromFile("panel_cef_plugin_tab.xml");

    if (selected)
    {
        mTabs->addTabPanel(LLTabContainer::TabPanelParams().panel(cef_plugin_tab).select_tab(true));
    }
    else
    {
        mTabs->addTabPanel(cef_plugin_tab);
    }

    if (mTabs->getTabCount() == mMaxTabCount)
    {
        addCefPluginTabBtn->setEnabled(false);
    }
}

void LLFloaterViewerCef::addCefNativeTab(int tab_number, bool selected)
{
    LLPanelCefNativeTab* cef_native_tab;

    cef_native_tab = new LLPanelCefNativeTab(tab_number);
    cef_native_tab->buildFromFile("panel_cef_plugin_tab.xml");

    if (selected)
    {
        mTabs->addTabPanel(LLTabContainer::TabPanelParams().panel(cef_native_tab).select_tab(true));
    }
    else
    {
        mTabs->addTabPanel(cef_native_tab);
    }

    if (mTabs->getTabCount() == mMaxTabCount)
    {
        addCefPluginTabBtn->setEnabled(false);
    }
}

void LLFloaterViewerCef::closeSelectedTab()
{
    LLPanel* current_tab = mTabs->getCurrentPanel();
    if (current_tab != nullptr)
    {
        mTabs->removeTabPanel(current_tab);
    }

    if (mTabs->getTabCount() < mMaxTabCount)
    {
        addCefPluginTabBtn->setEnabled(true);
    }
}

void LLFloaterViewerCef::activateBookmark(LLUICtrl* btn)
{
    if (btn != nullptr)
    {
        LLPanel* current_tab = mTabs->getCurrentPanel();
        if (current_tab != nullptr)
        {
            LLMediaCtrl* web_browser = current_tab->getChild<LLMediaCtrl>("browser");
            if (web_browser != nullptr)
            {
                const std::string url = btn->getToolTip();
                web_browser->navigateTo(url);
            }
        }
    }
}

LLPanelCefPluginTab::LLPanelCefPluginTab(int tab_id) :
    LLPanel(),
    mTabId(tab_id),
    mAddressCombo(nullptr),
    mWebBrowser(nullptr)
{
}

LLPanelCefPluginTab::~LLPanelCefPluginTab()
{
}

bool LLPanelCefPluginTab::postBuild()
{
    std::string label(STRINGIZE("CEF Plugin: " << mTabId));
    setLabel(label);

    mWebBrowser = getChild<LLMediaCtrl>("browser");

    getChild<LLButton>("back_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        mWebBrowser->navigateBack();
    });
    getChild<LLButton>("forward_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        mWebBrowser->navigateForward();
    });
    getChild<LLButton>("reload_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        mWebBrowser->refresh();
    });

    mAddressCombo = getChild<LLComboBox>("address");
    mAddressCombo->setCommitCallback(
        [this](LLUICtrl * combo, const LLSD&)
    {
        std::string url = combo->getValue().asString();
        LLStringUtil::trim(url);
        if (url.length() > 0)
        {
            mWebBrowser->navigateTo(url);
        };
    });

    getChild<LLButton>("go_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        std::string url = mAddressCombo->getValue().asString();
        LLStringUtil::trim(url);
        if (url.length() > 0)
        {
            mWebBrowser->navigateTo(url);
        };
    });

    const std::string home_page_url = getString("home_page_url");
    mWebBrowser->navigateTo(home_page_url);

    mWebBrowser->addObserver(this);

    return true;
}

void LLPanelCefPluginTab::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
    if (event == MEDIA_EVENT_NAME_CHANGED)
    {
        // FIXME: doesn't update label
        std::string page_title = self->getMediaName();
        std::cout << "MEDIA_EVENT_NAME_CHANGED: " << page_title << std::endl;
        this->setLabel(page_title);

    }
    if (event == MEDIA_EVENT_LOCATION_CHANGED)
    {
        const std::string url = self->getLocation();
        std::cout << "MEDIA_EVENT_LOCATION_CHANGED: " << url << std::endl;
        if (url.length())
        {
            mAddressCombo->remove(url);
            mAddressCombo->add(url);
            mAddressCombo->selectByValue(url);
        }
    }
}


LLPanelCefNativeTab::LLPanelCefNativeTab(int tab_id) : LLPanel(), mTabId(tab_id), mAddressCombo(nullptr), mWebBrowser(nullptr)
{
}

LLPanelCefNativeTab::~LLPanelCefNativeTab()
{
}

bool LLPanelCefNativeTab::postBuild()
{
    std::string label(STRINGIZE("CEF Native: " << mTabId));
    setLabel(label);

    mWebBrowser = getChild<LLMediaCtrl>("browser");

    getChild<LLButton>("back_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        mWebBrowser->navigateBack();
    });
    getChild<LLButton>("forward_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        mWebBrowser->navigateForward();
    });
    getChild<LLButton>("reload_btn")->setCommitCallback([this](LLUICtrl * btn, const LLSD&)
    {
        mWebBrowser->refresh();
    });

    mAddressCombo = getChild<LLComboBox>("address");
    mAddressCombo->setCommitCallback(
        [this](LLUICtrl * combo, const LLSD&)
    {
        std::string url = combo->getValue().asString();
        LLStringUtil::trim(url);
        if (url.length() > 0)
        {
            mWebBrowser->navigateTo(url);
        };
    });

    getChild<LLButton>("go_btn")->setCommitCallback(
        [this](LLUICtrl * btn, const LLSD&)
    {
        std::string url = mAddressCombo->getValue().asString();
        LLStringUtil::trim(url);
        if (url.length() > 0)
        {
            mWebBrowser->navigateTo(url);
        };
    });

    const std::string home_page_url = getString("home_page_url");
    mWebBrowser->navigateTo(home_page_url);

    mWebBrowser->addObserver(this);

    return true;
}

void LLPanelCefNativeTab::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
    if (event == MEDIA_EVENT_NAME_CHANGED)
    {
        // FIXME: doesn't update label
        std::string page_title = self->getMediaName();
        std::cout << "MEDIA_EVENT_NAME_CHANGED: " << page_title << std::endl;
        this->setLabel(page_title);
    }
    if (event == MEDIA_EVENT_LOCATION_CHANGED)
    {
        const std::string url = self->getLocation();
        std::cout << "MEDIA_EVENT_LOCATION_CHANGED: " << url << std::endl;
        if (url.length())
        {
            mAddressCombo->remove(url);
            mAddressCombo->add(url);
            mAddressCombo->selectByValue(url);
        }
    }
}
