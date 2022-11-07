/** 
* @file llpanelhome.h
* @author Martin Reddy
* @brief The Home side tray panel
*
* $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#ifndef LL_LLPANELHOME_H
#define LL_LLPANELHOME_H

#include "llpanel.h"
#include "llsd.h"
#include "llviewermediaobserver.h"

class LLMediaCtrl;

/**
 * Base class for web-based Home side tray
 */
class LLPanelHome :
    public LLPanel,
    public LLViewerMediaObserver
{
public:
    LLPanelHome();

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void onOpen(const LLSD& key);

private:
    // inherited from LLViewerMediaObserver
    /*virtual*/ void handleMediaEvent(LLPluginClassMedia *self, EMediaEvent event);

    LLMediaCtrl *mBrowser;
    bool         mFirstView;
};

#endif //LL_LLPANELHOME_H
