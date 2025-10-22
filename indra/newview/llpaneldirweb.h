/**
 * @file llpaneldirweb.h
 * @brief Web panel in the legacy Search directory.
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#ifndef LL_LLPANELDIRWEB_H
#define LL_LLPANELDIRWEB_H

#include "llpanel.h"
#include "llmediactrl.h"

class LLTextBox;
class LLFloaterDirectory;
class LLWebBrowserCtrlObserver;

class LLPanelDirWeb : public LLPanel, public LLViewerMediaObserver
{
public:
    LLPanelDirWeb();
    ~LLPanelDirWeb();

    bool postBuild() override;
    void onVisibilityChange(bool new_visibility);
    void draw();

    void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

    void navigateToDefaultPage();

    void setFloaterDirectory(LLFloaterDirectory* floater) { mFloaterDirectory = floater; }

protected:
    static void onClickHome( void* data );

    LLButton* mBtnBack;
    LLButton* mBtnForward;
    LLTextBox* mStatusBarText;
    LLFloaterDirectory* mFloaterDirectory;
    LLMediaCtrl* mWebBrowser;
};

#endif
