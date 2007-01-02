/** 
 * @file lltoolbar.h
 * @brief Large friendly buttons at bottom of screen.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H

#include "llpanel.h"

#include "llframetimer.h"

// "Constants" loaded from settings.xml at start time
extern S32 TOOL_BAR_HEIGHT;

#if LL_DARWIN
	class LLFakeResizeHandle;
#endif // LL_DARWIN

class LLToolBar
:	public LLPanel
{
public:
	LLToolBar(const std::string& name, const LLRect& rect );
	~LLToolBar();

	/*virtual*/ BOOL postBuild();

	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 LLString& tooltip_msg);

	/*virtual*/ void reshape(S32 width, S32 height, BOOL called_from_parent);

	static void toggle(void*);
	static BOOL visible(void*);

	// Move buttons to appropriate locations based on rect.
	void layoutButtons();

	// Per-frame refresh call
	void refresh();

	// callbacks
	static void onClickIM(void*);
	static void onClickChat(void* data);
	static void onClickFriends(void* data);
	static void onClickAppearance(void* data);
	static void onClickClothing(void* data);
	static void onClickFly(void*);
	static void onClickSit(void*);
	static void onClickSnapshot(void* data);
	static void onClickDirectory(void* data);
	static void onClickBuild(void* data);
	static void onClickRadar(void* data);
	static void onClickMap(void* data);
	static void onClickInventory(void* data);

	static F32 sInventoryAutoOpenTime;

private:
	BOOL		mInventoryAutoOpen;
	LLFrameTimer mInventoryAutoOpenTimer;
#if LL_DARWIN
	LLFakeResizeHandle *mResizeHandle;
#endif // LL_DARWIN
};

extern LLToolBar *gToolBar;

#endif
