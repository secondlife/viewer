/** 
 * @file lldebugview.h
 * @brief A view containing debug UI elements
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLDEBUGVIEW_H
#define LL_LLDEBUGVIEW_H

// requires:
// stdtypes.h

#include "llview.h"

// declarations
class LLButton;
class LLToolView;
class LLStatusPanel;
class LLFrameStatView;
class LLFastTimerView;
class LLMemoryView;
class LLConsole;
class LLTextureView;
class LLContainerView;

class LLDebugView : public LLView
{
public:
	LLDebugView(const std::string& name, const LLRect &rect);
	~LLDebugView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	LLFrameStatView* mFrameStatView;
	LLFastTimerView* mFastTimerView;
	LLMemoryView*	 mMemoryView;
	LLConsole*		 mDebugConsolep;
	LLContainerView* mStatViewp;
};

extern LLDebugView* gDebugView;

#endif
