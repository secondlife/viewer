/** 
 * @file llnameeditor.h
 * @brief display and refresh a name from the name cache
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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
		void (*focus_lost_callback)(LLUICtrl* caller, void* user_data) = NULL,
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
