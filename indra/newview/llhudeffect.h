/** 
 * @file llhudeffect.h
 * @brief LLHUDEffect class definition
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLHUDEFFECT_H
#define LL_LLHUDEFFECT_H

#include "llhudobject.h"

#include "lluuid.h"
#include "v4coloru.h"

const F32 LL_HUD_DUR_SHORT = 1.f;

class LLMessageSystem;


class LLHUDEffect : public LLHUDObject
{
public:
    void setNeedsSendToSim(const BOOL send_to_sim);
    BOOL getNeedsSendToSim() const;
    void setOriginatedHere(const BOOL orig_here);
    BOOL getOriginatedHere() const;

    void setDuration(const F32 duration);
    void setColor(const LLColor4U &color);
    void setID(const LLUUID &id);
    const LLUUID &getID() const;

    BOOL isDead() const;

    friend class LLHUDManager;
protected:
    LLHUDEffect(const U8 type);
    ~LLHUDEffect();

    /*virtual*/ void render();

    virtual void packData(LLMessageSystem *mesgsys);
    virtual void unpackData(LLMessageSystem *mesgsys, S32 blocknum);
    virtual void update();

    static void getIDType(LLMessageSystem *mesgsys, S32 blocknum, LLUUID &uuid, U8 &type);

protected:
    LLUUID      mID;
    F32         mDuration;
    LLColor4U   mColor;

    BOOL        mNeedsSendToSim;
    BOOL        mOriginatedHere;
};

#endif // LL_LLHUDEFFECT_H
