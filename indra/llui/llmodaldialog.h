/** 
 * @file llmodaldialog.h
 * @brief LLModalDialog base class
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
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

#ifndef LL_LLMODALDIALOG_H
#define LL_LLMODALDIALOG_H

#include "llfloater.h"
#include "llframetimer.h"

class LLModalDialog;

// By default, a ModalDialog is modal, i.e. no other window can have focus
// However, for the sake of code reuse and simplicity, if mModal == false,
// the dialog behaves like a normal floater

class LLModalDialog : public LLFloater
{
public:
	LLModalDialog( const LLString& title, S32 width, S32 height, BOOL modal = true );
	/*virtual*/ ~LLModalDialog();

	/*virtual*/ void 	reshape(S32 width, S32 height, BOOL called_from_parent = 1);
	
	/*virtual*/ void	startModal();
	/*virtual*/ void	stopModal();

	/*virtual*/ BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleScrollWheel(S32 x, S32 y, S32 clicks);
	/*virtual*/ BOOL	handleDoubleClick(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL	handleKeyHere(KEY key, MASK mask, BOOL called_from_parent );

	/*virtual*/ void	onClose(bool app_quitting);

	/*virtual*/ void	setVisible(BOOL visible);
	/*virtual*/ void	draw();

	static void		onAppFocusLost();
	static void		onAppFocusGained();

	static S32		activeCount() { return sModalStack.size(); }
	
protected:
	void			centerOnScreen();

protected:
	LLFrameTimer 	mVisibleTime;
	BOOL			mModal; // do not change this after creation!

	static std::list<LLModalDialog*> sModalStack;  // Top of stack is currently being displayed
};

#endif  // LL_LLMODALDIALOG_H
