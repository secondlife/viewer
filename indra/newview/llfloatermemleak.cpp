/** 
 * @file llfloatermemleak.cpp
 * @brief LLFloatermemleak class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
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

#include "llfloatermemleak.h"

#include "lluictrlfactory.h"
#include "llbutton.h"
#include "llspinctrl.h"
#include "llresmgr.h"

#include "llmath.h"
#include "llviewerwindow.h"

U32 LLFloaterMemLeak::sMemLeakingSpeed = 0 ; //bytes leaked per frame
U32 LLFloaterMemLeak::sMaxLeakedMem = 0 ; //maximum allowed leaked memory
U32 LLFloaterMemLeak::sTotalLeaked = 0 ;
S32 LLFloaterMemLeak::sStatus = LLFloaterMemLeak::STOP ;
BOOL LLFloaterMemLeak::sbAllocationFailed = FALSE ;

extern BOOL gSimulateMemLeak;

LLFloaterMemLeak::LLFloaterMemLeak(const LLSD& key)
    : LLFloater(key)
{
    setTitle("Memory Leaking Simulation Floater");
    mCommitCallbackRegistrar.add("MemLeak.ChangeLeakingSpeed",  boost::bind(&LLFloaterMemLeak::onChangeLeakingSpeed, this));
    mCommitCallbackRegistrar.add("MemLeak.ChangeMaxMemLeaking", boost::bind(&LLFloaterMemLeak::onChangeMaxMemLeaking, this));
    mCommitCallbackRegistrar.add("MemLeak.Start",   boost::bind(&LLFloaterMemLeak::onClickStart, this));
    mCommitCallbackRegistrar.add("MemLeak.Stop",    boost::bind(&LLFloaterMemLeak::onClickStop, this));
    mCommitCallbackRegistrar.add("MemLeak.Release", boost::bind(&LLFloaterMemLeak::onClickRelease, this));
    mCommitCallbackRegistrar.add("MemLeak.Close",   boost::bind(&LLFloaterMemLeak::onClickClose, this));
}
//----------------------------------------------

BOOL LLFloaterMemLeak::postBuild(void) 
{   
    F32 a, b ;
    a = getChild<LLUICtrl>("leak_speed")->getValue().asReal();
    if(a > (F32)(0xFFFFFFFF))
    {
        sMemLeakingSpeed = 0xFFFFFFFF ;
    }
    else
    {
        sMemLeakingSpeed = (U32)a ;
    }
    b = getChild<LLUICtrl>("max_leak")->getValue().asReal();
    if(b > (F32)0xFFF)
    {
        sMaxLeakedMem = 0xFFFFFFFF ;
    }
    else
    {
        sMaxLeakedMem = ((U32)b) << 20 ;
    }
    
    sbAllocationFailed = FALSE ;
    return TRUE ;
}
LLFloaterMemLeak::~LLFloaterMemLeak()
{
    release() ;
        
    sMemLeakingSpeed = 0 ; //bytes leaked per frame
    sMaxLeakedMem = 0 ; //maximum allowed leaked memory 
}

void LLFloaterMemLeak::release()
{
    if(mLeakedMem.empty())
    {
        return ;
    }

    for(S32 i = 0 ; i < (S32)mLeakedMem.size() ; i++)
    {
        delete[] mLeakedMem[i] ;
    }
    mLeakedMem.clear() ;

    sStatus = STOP ;
    sTotalLeaked = 0 ;
    sbAllocationFailed = FALSE ;
    gSimulateMemLeak = FALSE;
}

void LLFloaterMemLeak::stop()
{
    sStatus = STOP ;
    sbAllocationFailed = TRUE ;
}

void LLFloaterMemLeak::idle()
{
    if(STOP == sStatus)
    {
        return ;
    }

    sbAllocationFailed = FALSE ;
    
    if(RELEASE == sStatus)
    {
        release() ;
        return ;
    }

    char* p = NULL ;
    if(sMemLeakingSpeed > 0 && sTotalLeaked < sMaxLeakedMem)
    {
        p = new char[sMemLeakingSpeed] ;

        if(p)
        {
            mLeakedMem.push_back(p) ;
            sTotalLeaked += sMemLeakingSpeed ;
        }
    }
    if(!p)
    {
        stop();
    }
}

//----------------------
void LLFloaterMemLeak::onChangeLeakingSpeed()
{
    F32 tmp ;
    tmp =getChild<LLUICtrl>("leak_speed")->getValue().asReal();

    if(tmp > (F32)0xFFFFFFFF)
    {
        sMemLeakingSpeed = 0xFFFFFFFF ;
    }
    else
    {
        sMemLeakingSpeed = (U32)tmp ;
    }

}

void LLFloaterMemLeak::onChangeMaxMemLeaking()
{

    F32 tmp ;
    tmp =getChild<LLUICtrl>("max_leak")->getValue().asReal();
    if(tmp > (F32)0xFFF)
    {
        sMaxLeakedMem = 0xFFFFFFFF ;
    }
    else
    {
        sMaxLeakedMem = ((U32)tmp) << 20 ;
    }
    
}

void LLFloaterMemLeak::onClickStart()
{
    sStatus = START ;
    gSimulateMemLeak = TRUE;
}

void LLFloaterMemLeak::onClickStop()
{
    sStatus = STOP ;
}

void LLFloaterMemLeak::onClickRelease()
{
    sStatus = RELEASE ;
}

void LLFloaterMemLeak::onClickClose()
{
    setVisible(FALSE);
}

void LLFloaterMemLeak::draw()
{
    //show total memory leaked
    if(sTotalLeaked > 0)
    {
        std::string bytes_string;
        LLResMgr::getInstance()->getIntegerString(bytes_string, sTotalLeaked >> 10 );
        getChild<LLUICtrl>("total_leaked_label")->setTextArg("[SIZE]", bytes_string);
    }
    else
    {
        getChild<LLUICtrl>("total_leaked_label")->setTextArg("[SIZE]", LLStringExplicit("0"));
    }

    if(sbAllocationFailed)
    {
        getChild<LLUICtrl>("note_label_1")->setTextArg("[NOTE1]", LLStringExplicit("Memory leaking simulation stops. Reduce leaking speed or"));
        getChild<LLUICtrl>("note_label_2")->setTextArg("[NOTE2]", LLStringExplicit("increase max leaked memory, then press Start to continue."));
    }
    else
    {
        getChild<LLUICtrl>("note_label_1")->setTextArg("[NOTE1]", LLStringExplicit(""));
        getChild<LLUICtrl>("note_label_2")->setTextArg("[NOTE2]", LLStringExplicit(""));
    }

    LLFloater::draw();
}
