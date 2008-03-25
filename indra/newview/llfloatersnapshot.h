/** 
 * @file llfloatersnapshot.h
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#ifndef LL_LLFLOATERSNAPSHOT_H
#define LL_LLFLOATERSNAPSHOT_H

#include "llfloater.h"

#include "llmemory.h"
#include "llimagegl.h"
#include "llcharacter.h"

class LLFloaterSnapshot : public LLFloater
{
public:
    LLFloaterSnapshot();
	virtual ~LLFloaterSnapshot();

	virtual BOOL postBuild();
	virtual void draw();
	virtual void onClose(bool app_quitting);

	static void show(void*);
	static void hide(void*);
	static void update();

	static S32  getUIWinHeightLong()  {return sUIWinHeightLong ;}
	static S32  getUIWinHeightShort() {return sUIWinHeightShort ;}
	static S32  getUIWinWidth()       {return sUIWinWidth ;}

private:
	class Impl;
	Impl& impl;

	static LLFloaterSnapshot* sInstance;
	static S32    sUIWinHeightLong ;
	static S32    sUIWinHeightShort ;
	static S32    sUIWinWidth ;
};

class LLSnapshotFloaterView : public LLFloaterView
{
public:
	LLSnapshotFloaterView( const LLString& name, const LLRect& rect );
	virtual ~LLSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleHover(S32 x, S32 y, MASK mask);
};

extern LLSnapshotFloaterView* gSnapshotFloaterView;
#endif // LL_LLFLOATERSNAPSHOT_H
