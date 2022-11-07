/** 
 * @file llpanelavatartag.h
 * @brief Avatar row panel
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

#ifndef LL_LLPANELAVATARTAG_H
#define LL_LLPANELAVATARTAG_H

#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"

class LLAvatarIconCtrl;
class LLTextBox;

/**
 * Avatar Tag panel.
 * 
 * Test.
 * 
 * Contains avatar name 
 * Provide methods for setting avatar id, state, muted status and speech power.
 */ 
class LLPanelAvatarTag : public LLPanel
{
public:
    LLPanelAvatarTag(const LLUUID& key, const std::string im_time);
    virtual ~LLPanelAvatarTag();
    
    /**
     * Set avatar ID.
     * 
     * After the ID is set, it is possible to track the avatar status and get its name.
     */
    void            setAvatarId(const LLUUID& avatar_id);
    void            setTime    (const std::string& time);

    const LLUUID&   getAvatarId() const { return mAvatarId; }

    /*virtual*/ BOOL postBuild();
    /*virtual*/ void draw();    
    
    virtual boost::signals2::connection setLeftButtonClickCallback(
                                                                   const commit_callback_t& cb);
    virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

    void onClick();
private:
    void setName(const std::string& name);

    /**
     * Called by LLCacheName when the avatar name gets updated.
     */
    void nameUpdatedCallback(
        const LLUUID& id,
        const std::string& first,
        const std::string& last,
        BOOL is_group);
    
    LLAvatarIconCtrl*       mIcon;          /// status tracking avatar icon
    LLTextBox*              mName;          /// displays avatar name
    LLTextBox*              mTime;          /// displays time
    LLUUID                  mAvatarId;
//  LLFrameTimer            mFadeTimer;
};

#endif
