/** 
 * @file llnameeditor.h
 * @brief display and refresh a name from the name cache
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLNAMEEDITOR_H
#define LL_LLNAMEEDITOR_H

#include <set>

#include "llview.h"
#include "v4color.h"
#include "llstring.h"
#include "llfontgl.h"
#include "linked_lists.h"
#include "lllineeditor.h"


class LLNameEditor
:	public LLLineEditor
{
public:
	LLNameEditor(const std::string& name, const LLRect& rect,
		const LLUUID& name_id = LLUUID::null,
		BOOL is_group = FALSE,
		const LLFontGL* glfont = NULL,
		S32 max_text_length = 254,
		void (*commit_callback)(LLUICtrl* caller, void* user_data) = NULL,
		void (*keystroke_callback)(LLLineEditor* caller, void* user_data) = NULL,
		void (*focus_lost_callback)(LLLineEditor* caller, void* user_data) = NULL,
		void* userdata = NULL,
		LLLinePrevalidateFunc prevalidate_func = NULL,
		LLViewBorder::EBevel border_bevel = LLViewBorder::BEVEL_IN,
		LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE,
		S32 border_thickness = 1);
		// By default, follows top and left and is mouse-opaque.
		// If no text, text = name.
		// If no font, uses default system font.

	virtual ~LLNameEditor();

	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	void setNameID(const LLUUID& name_id, BOOL is_group);

	void refresh(const LLUUID& id, const char* first, const char* last,
				 BOOL is_group);

	static void refreshAll(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group);

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_NAME_EDITOR; }
	virtual LLString getWidgetTag() const { return LL_NAME_EDITOR_TAG; }

	// Take/return agent UUIDs
	virtual void	setValue( LLSD value );
	virtual LLSD	getValue() const;

private:
	static std::set<LLNameEditor*> sInstances;

private:
	LLUUID mNameID;

};

#endif
