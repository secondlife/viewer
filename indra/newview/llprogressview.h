/** 
 * @file llprogressview.h
 * @brief LLProgressView class definition
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLPROGRESSVIEW_H
#define LL_LLPROGRESSVIEW_H

#include "llview.h"
#include "llframetimer.h"

class LLImageRaw;
class LLButton;

class LLProgressView : public LLView
{
public:
	LLProgressView(const std::string& name, const LLRect& rect);
	virtual ~LLProgressView();

	virtual EWidgetType getWidgetType() const;
	virtual LLString getWidgetTag() const;

	/*virtual*/ void draw();
	/*virtual*/ BOOL handleHover(S32 x, S32 y, MASK mask);
	/*virtual*/ BOOL handleKeyHere(KEY key, MASK mask, BOOL called_from_parent);
	/*virtual*/ void setVisible(BOOL visible);

	void setText(const LLString& text);
	void setPercent(const F32 percent);

	// Set it to NULL when you want to eliminate the message.
	void setMessage(const LLString& msg);
	
	void setCancelButtonVisible(BOOL b, const LLString& label);

	static void onCancelButtonClicked( void* );

protected:
	BOOL mDrawBackground;
	F32 mPercentDone;
	LLString mText;
	LLString mMessage;
	LLButton*	mCancelBtn;
	LLFrameTimer	mFadeTimer;
	LLFrameTimer mProgressTimer;

	static LLProgressView* sInstance;
};

#endif // LL_LLPROGRESSVIEW_H
