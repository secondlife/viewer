/** 
 * @file llfloatermemleak.cpp
 * @brief LLFloatermemleak class definition
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
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

LLFloaterMemLeak* LLFloaterMemLeak::sInstance = NULL;
U32 LLFloaterMemLeak::sMemLeakingSpeed = 0 ; //bytes leaked per frame
U32 LLFloaterMemLeak::sMaxLeakedMem = 0 ; //maximum allowed leaked memory
U32 LLFloaterMemLeak::sTotalLeaked = 0 ;
S32 LLFloaterMemLeak::sStatus = LLFloaterMemLeak::STOP ;
BOOL LLFloaterMemLeak::sbAllocationFailed = FALSE ;

LLFloaterMemLeak::LLFloaterMemLeak() : LLFloater("Memory Leaking Simulation Floater")
{
}

LLFloaterMemLeak::~LLFloaterMemLeak()
{
	release() ;
		
	sMemLeakingSpeed = 0 ; //bytes leaked per frame
	sMaxLeakedMem = 0 ; //maximum allowed leaked memory	
	sInstance = NULL ;
}

void LLFloaterMemLeak::release()
{
	for(S32 i = 0 ; i < (S32)mLeakedMem.size() ; i++)
	{
		delete[] mLeakedMem[i] ;
	}
	mLeakedMem.clear() ;

	sStatus = STOP ;
	sTotalLeaked = 0 ;
	sbAllocationFailed = FALSE ;
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
		sStatus = STOP ;
		sbAllocationFailed = TRUE ;
	}
}

//----------------------
void LLFloaterMemLeak::onChangeLeakingSpeed(LLUICtrl* ctrl, void* userData)
{
	LLFloaterMemLeak *mem_leak = (LLFloaterMemLeak *)userData;		
	if (mem_leak)
	{
		F32 tmp ;
		tmp = mem_leak->childGetValue("leak_speed").asReal();

		if(tmp > (F32)0xFFFFFFFF)
		{
			sMemLeakingSpeed = 0xFFFFFFFF ;
		}
		else
		{
			sMemLeakingSpeed = (U32)tmp ;
		}
	}
}

void LLFloaterMemLeak::onChangeMaxMemLeaking(LLUICtrl* ctrl, void* userData)
{
	LLFloaterMemLeak *mem_leak = (LLFloaterMemLeak *)userData;		
	if (mem_leak)
	{
		F32 tmp ;
		tmp = mem_leak->childGetValue("max_leak").asReal();
		if(tmp > (F32)0xFFF)
		{
			sMaxLeakedMem = 0xFFFFFFFF ;
		}
		else
		{
			sMaxLeakedMem = ((U32)tmp) << 20 ;
		}
	}
}

void LLFloaterMemLeak::onClickStart(void* userData)
{
	sStatus = START ;
}

void LLFloaterMemLeak::onClickStop(void* userData)
{
	sStatus = STOP ;
}

void LLFloaterMemLeak::onClickRelease(void* userData)
{
	sStatus = RELEASE ;
}

void LLFloaterMemLeak::onClickClose(void* userData)
{
	if (sInstance)
	{
		sInstance->setVisible(FALSE);
	}
}

//----------------------------------------------

BOOL LLFloaterMemLeak::postBuild(void) 
{
	childSetCommitCallback("leak_speed", onChangeLeakingSpeed, this);
	childSetCommitCallback("max_leak", onChangeMaxMemLeaking, this);

	childSetAction("start_btn", onClickStart, this);
	childSetAction("stop_btn", onClickStop, this);
	childSetAction("release_btn", onClickRelease, this);
	childSetAction("close_btn", onClickClose, this);

	return TRUE ;
}

void LLFloaterMemLeak::draw()
{
	//show total memory leaked
	if(sTotalLeaked > 0)
	{
		std::string bytes_string;
		LLResMgr::getInstance()->getIntegerString(bytes_string, sTotalLeaked >> 10 );
		childSetTextArg("total_leaked_label", "[SIZE]", bytes_string);
	}
	else
	{
		childSetTextArg("total_leaked_label", "[SIZE]", LLStringExplicit("0"));
	}

	if(sbAllocationFailed)
	{
		childSetTextArg("note_label_1", "[NOTE1]", LLStringExplicit("Memory leaking simulation stops. Reduce leaking speed or"));
		childSetTextArg("note_label_2", "[NOTE2]", LLStringExplicit("increase max leaked memory, then press Start to continue."));
	}
	else
	{
		childSetTextArg("note_label_1", "[NOTE1]", LLStringExplicit(""));
		childSetTextArg("note_label_2", "[NOTE2]", LLStringExplicit(""));
	}

	LLFloater::draw();
}

// static instance of it
LLFloaterMemLeak* LLFloaterMemLeak::instance()
{
	if (!sInstance)
	{
		sInstance = new LLFloaterMemLeak();
		LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_mem_leaking.xml", NULL, FALSE);

		if(sInstance)
		{
			F32 a, b ;
			a = sInstance->childGetValue("leak_speed").asReal();
			if(a > (F32)(0xFFFFFFFF))
			{
				sMemLeakingSpeed = 0xFFFFFFFF ;
			}
			else
			{
				sMemLeakingSpeed = (U32)a ;
			}
			b = sInstance->childGetValue("max_leak").asReal();
			if(b > (F32)0xFFF)
			{
				sMaxLeakedMem = 0xFFFFFFFF ;
			}
			else
			{
				sMaxLeakedMem = ((U32)b) << 20 ;
			}

			sbAllocationFailed = FALSE ;
		}
	}
	return sInstance ;
}

void LLFloaterMemLeak::show(void*)
{
	instance()->open();
}

LLFloaterMemLeak* LLFloaterMemLeak::getInstance()
{
	return sInstance ;
}

