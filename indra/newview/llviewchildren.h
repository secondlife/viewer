/** 
 * @file llviewchildren.h
 * @brief LLViewChildren class header file
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWCHILDREN_H
#define LL_LLVIEWCHILDREN_H

class LLPanel;

class LLViewChildren
	// makes it easy to manipulate children of a view by id safely
	// encapsulates common operations into simple, one line calls
{
public:
	LLViewChildren(LLPanel& parent);
	
	// all views
	void show(const char* id, bool visible = true);
	void hide(const char* id) { show(id, false); }

	void enable(const char* id, bool enabled = true);
	void disable(const char* id) { enable(id, false); };

	//
	// LLTextBox
	void setText(const char* id,
		const std::string& text, bool visible = true);
	void setWrappedText(const char* id,
		const std::string& text, bool visible = true);

	// LLIconCtrl
	enum Badge { BADGE_OK, BADGE_NOTE, BADGE_WARN, BADGE_ERROR };
	
	void setBadge(const char* id, Badge b, bool visible = true);

	
	// LLButton
	void setAction(const char* id, void(*function)(void*), void* value);


private:
	LLPanel& mParent;
};

#endif
