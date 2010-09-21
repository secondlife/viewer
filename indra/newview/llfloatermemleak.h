/** 
 * @file llfloatermemleak.h
 * @brief memory leaking simulation window, debug use only
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
