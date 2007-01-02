/** 
 * @file llclipboard.h
 * @brief LLClipboard base class
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLCLIPBOARD_H
#define LL_LLCLIPBOARD_H


#include "llstring.h"
#include "lluuid.h"

//
// Classes
//
class LLClipboard
{
protected:
	LLUUID		mSourceID;
	LLWString mString;
	
public:
	LLClipboard();
	~LLClipboard();

	void		copyFromSubstring(const LLWString &copy_from, S32 pos, S32 len, const LLUUID& source_id = LLUUID::null );


	BOOL		canPasteString();
	LLWString 	getPasteWString(LLUUID* source_id = NULL);
};


// Global singleton
extern LLClipboard gClipboard;


#endif  // LL_LLCLIPBOARD_H
