/** 
 * @file llfloaterbuycurrencyhtml.h
 * @brief buy currency implemented in HTML floater - uses embedded media browser control
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

#ifndef LL_LLFLOATERBUYCURRENCYHTML_H
#define LL_LLFLOATERBUYCURRENCYHTML_H

#include "llfloater.h"
#include "llmediactrl.h"

class LLFloaterBuyCurrencyHTML : 
    public LLFloater, 
    public LLViewerMediaObserver
{
    public:
        LLFloaterBuyCurrencyHTML( const LLSD& key );

        /*virtual*/ BOOL postBuild();
        /*virtual*/ void onClose( bool app_quitting );

        // inherited from LLViewerMediaObserver
        /*virtual*/ void handleMediaEvent( LLPluginClassMedia* self, EMediaEvent event );

        // allow our controlling parent to tell us paramters
        void setParams( bool specific_sum_requested, const std::string& message, S32 sum );

        // parse and construct URL and set browser to navigate there.
        void navigateToFinalURL();

    private:
        LLMediaCtrl* mBrowser;
        bool mSpecificSumRequested;
        std::string mMessage;
        S32 mSum;
};

#endif  // LL_LLFLOATERBUYCURRENCYHTML_H
