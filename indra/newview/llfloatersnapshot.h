/** 
 * @file llfloatersnapshot.h
 * @brief Snapshot preview window, allowing saving, e-mailing, etc.
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

#ifndef LL_LLFLOATERSNAPSHOT_H
#define LL_LLFLOATERSNAPSHOT_H

#include "llfloater.h"
#include "lltransientdockablefloater.h"


class LLFloaterSnapshot : public LLTransientDockableFloater
{
public:
	typedef enum e_snapshot_format
	{
		SNAPSHOT_FORMAT_PNG,
		SNAPSHOT_FORMAT_JPEG,
		SNAPSHOT_FORMAT_BMP
	} ESnapshotFormat;

	enum ESnapshotMode
	{
		SNAPSHOT_SHARE,
		SNAPSHOT_SAVE,
		SNAPSHOT_MAIN
	};

	LLFloaterSnapshot(const LLSD& key);
	virtual ~LLFloaterSnapshot();
    
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();
	/*virtual*/ void onOpen(const LLSD& key);
	/*virtual*/ void onClose(bool app_quitting);
	
	static void update();
	
	bool updateButtons(ESnapshotMode mode);
	
	static S32  getUIWinHeightLong()  {return sUIWinHeightLong ;}
	static S32  getUIWinHeightShort() {return sUIWinHeightShort ;}
	static S32  getUIWinWidth()       {return sUIWinWidth ;}

private:
	class Impl;
	Impl& impl;

	static S32    sUIWinHeightLong ;
	static S32    sUIWinHeightShort ;
	static S32    sUIWinWidth ;

	LLRect mRefreshBtnRect;
};

class LLSnapshotFloaterView : public LLFloaterView
{
public:
	struct Params 
	:	public LLInitParam::Block<Params, LLFloaterView::Params>
	{
	};

protected:
	LLSnapshotFloaterView (const Params& p);
	friend class LLUICtrlFactory;

public:
	virtual ~LLSnapshotFloaterView();

	/*virtual*/	BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/	BOOL handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/	BOOL handleHover(S32 x, S32 y, MASK mask);
};

extern LLSnapshotFloaterView* gSnapshotFloaterView;

#endif // LL_LLFLOATERSNAPSHOT_H
