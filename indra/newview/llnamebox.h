/** 
 * @file llnamebox.h
 * @brief display and refresh a name from the name cache
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLNAMEBOX_H
#define LL_LLNAMEBOX_H

#include <set>

#include "llview.h"
#include "llstring.h"
#include "llfontgl.h"
#include "linked_lists.h"
#include "lltextbox.h"

class LLNameBox
:	public LLTextBox
{
public:
	LLNameBox(const std::string& name, const LLRect& rect, const LLUUID& name_id = LLUUID::null, BOOL is_group = FALSE, const LLFontGL* font = NULL, BOOL mouse_opaque = TRUE );
		// By default, follows top and left and is mouse-opaque.
		// If no text, text = name.
		// If no font, uses default system font.
	virtual ~LLNameBox();

	void setNameID(const LLUUID& name_id, BOOL is_group);

	void refresh(const LLUUID& id, const char* first, const char* last,
				 BOOL is_group);

	static void refreshAll(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group);

private:
	static std::set<LLNameBox*> sInstances;

private:
	LLUUID mNameID;

};

#endif
