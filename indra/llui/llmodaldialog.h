/** 
 * @file llmodaldialog.h
 * @brief LLModalDialog base class
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
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
