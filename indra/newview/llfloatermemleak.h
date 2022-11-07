/** 
 * @file llfloatermemleak.h
 * @brief memory leaking simulation window, debug use only
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERMEMLEAK_H
#define LL_LLFLOATERMEMLEAK_H

#include "llfloater.h"

class LLFloaterMemLeak : public LLFloater
{
    friend class LLFloaterReg;
public:
    /// initialize all the callbacks for the menu

    virtual BOOL postBuild() ;
    virtual void draw() ;
    
    void onChangeLeakingSpeed();
    void onChangeMaxMemLeaking();
    void onClickStart();
    void onClickStop();
    void onClickRelease();
    void onClickClose();

public:
    void idle() ;
    void stop() ;

private:
    
    LLFloaterMemLeak(const LLSD& key);
    virtual ~LLFloaterMemLeak();
    void release() ;

private:
    enum 
    {
        RELEASE = -1 ,
        STOP,
        START
    } ;

    static U32 sMemLeakingSpeed ; //bytes leaked per frame
    static U32 sMaxLeakedMem ; //maximum allowed leaked memory
    static U32 sTotalLeaked ;
    static S32 sStatus ; //0: stop ; >0: start ; <0: release
    static BOOL sbAllocationFailed ;

    std::vector<char*> mLeakedMem ; 
};

#endif // LL_LLFLOATERMEMLEAK_H
