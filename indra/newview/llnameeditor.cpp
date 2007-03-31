/** 
 * @file llnameeditor.cpp
 * @brief Name Editor to refresh a name.
 *
 * Copyright (c) 2003-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"
 
#include "llnameeditor.h"
#include "llcachename.h"
#include "llagent.h"

#include "llfontgl.h"

#include "lluuid.h"
#include "llrect.h"
#include "llstring.h"
#include "llui.h"


// statics
std::set<LLNameEditor*> LLNameEditor::sInstances;

LLNameEditor::LLNameEditor(const std::string& name, const LLRect& rect,
		const LLUUID& name_id, 
		BOOL is_group,
		const LLFontGL* glfont,
		S32 max_text_length,
		void (*commit_callback)(LLUICtrl* caller, void* user_data),
		void (*keystroke_callback)(LLLineEditor* caller, void* user_data),
		void (*focus_lost_callback)(LLUICtrl* caller, void* user_data),
		void* userdata,
		LLLinePrevalidateFunc prevalidate_func,
		LLViewBorder::EBevel border_bevel,
		LLViewBorder::EStyle border_style,
		S32 border_thickness)
:	LLLineEditor(name, rect, 
				 "(retrieving)", 
				 glfont, 
				 max_text_length, 
				 commit_callback, 
				 keystroke_callback,
				 focus_lost_callback,
				 userdata,
				 prevalidate_func,
				 border_bevel,
				 border_style,
				 border_thickness),
	mNameID(name_id)
{
	LLNameEditor::sInstances.insert(this);
	if(!name_id.isNull())
	{
		setNameID(name_id, is_group);
	}
}


LLNameEditor::~LLNameEditor()
{
	LLNameEditor::sInstances.erase(this);
}

void LLNameEditor::setNameID(const LLUUID& name_id, BOOL is_group)
{
	mNameID = name_id;

	char first[DB_FIRST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
	char last[DB_LAST_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
	char group_name[DB_GROUP_NAME_BUF_SIZE];		/*Flawfinder: ignore*/
	LLString name;

	if (!is_group)
	{
		gCacheName->getName(name_id, first, last);

		name.assign(first);
		name.append(1, ' ');
		name.append(last);
	}
	else
	{
		gCacheName->getGroupName(name_id, group_name);
		name.assign(group_name);
	}

	setText(name);
}

void LLNameEditor::refresh(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group)
{
	if (id == mNameID)
	{
		LLString name;

		name.assign(firstname);
		if (!is_group)
		{
			name.append(1, ' ');
			name.append(lastname);
		}

		setText(name);
	}
}

void LLNameEditor::refreshAll(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group)
{
	std::set<LLNameEditor*>::iterator it;
	for (it = LLNameEditor::sInstances.begin();
		 it != LLNameEditor::sInstances.end();
		 ++it)
	{
		LLNameEditor* box = *it;
		box->refresh(id, firstname, lastname, is_group);
	}
}

void LLNameEditor::setValue( LLSD value )
{
	setNameID(value.asUUID(), FALSE);
}

LLSD LLNameEditor::getValue() const
{
	return LLSD(mNameID);
}

LLView* LLNameEditor::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("name_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	S32 max_text_length = 128;
	node->getAttributeS32("max_length", max_text_length);

	LLFontGL* font = LLView::selectFont(node);
	
	LLViewBorder::EStyle border_style = LLViewBorder::STYLE_LINE;
	LLString border_string;
	node->getAttributeString("border_style", border_string);
	LLString::toLower(border_string);

	if (border_string == "texture")
	{
		border_style = LLViewBorder::STYLE_TEXTURE;
	}

	S32 border_thickness = 1;
	node->getAttributeS32("border_thickness", border_thickness);

	LLUICtrlCallback commit_callback = NULL;

	LLNameEditor* line_editor = new LLNameEditor(name,
								rect, 
								LLUUID::null, FALSE,
								font,
								max_text_length,
								commit_callback,
								NULL,
								NULL,
								NULL,
								NULL,
								LLViewBorder::BEVEL_IN,
								border_style,
								border_thickness);

	LLString label;
	if(node->getAttributeString("label", label))
	{
		line_editor->setLabel(label);
	}
	line_editor->setColorParameters(node);
	line_editor->initFromXML(node, parent);
	
	return line_editor;
}
