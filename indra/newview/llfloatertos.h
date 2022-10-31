/** 
 * @file llfloatertos.h
 * @brief Terms of Service Agreement dialog
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

#ifndef LL_LLFLOATERTOS_H
#define LL_LLFLOATERTOS_H

#include "llmodaldialog.h"
#include "llassetstorage.h"
#include "llmediactrl.h"
#include <boost/function.hpp>
#include "lleventcoro.h"
#include "llcoros.h"

class LLButton;
class LLRadioGroup;
class LLTextEditor;
class LLUUID;

class LLFloaterTOS : 
    public LLModalDialog,
    public LLViewerMediaObserver
{
public:
    LLFloaterTOS(const LLSD& data);
    virtual ~LLFloaterTOS();

    BOOL postBuild();
    
    virtual void draw();

    static void     updateAgree( LLUICtrl *, void* userdata );
    static void     onContinue( void* userdata );
    static void     onCancel( void* userdata );

    void            setSiteIsAlive( bool alive );

    void            updateAgreeEnabled(bool enabled);

    // inherited from LLViewerMediaObserver
    /*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event);

private:
    static void testSiteIsAliveCoro(LLHandle<LLFloater> handle, std::string url);

    std::string     mMessage;
    bool            mLoadingScreenLoaded;
    bool            mSiteAlive;
    bool            mRealNavigateBegun;
    std::string     mReplyPumpName;


};

#endif // LL_LLFLOATERTOS_H
