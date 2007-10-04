/** 
 * @file llnamelistctrl.h
 * @brief A list of names, automatically refreshing from the name cache.
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

#ifndef LL_LLNAMELISTCTRL_H
#define LL_LLNAMELISTCTRL_H

#include <set>

#include "llscrolllistctrl.h"


class LLNameListCtrl
:	public LLScrollListCtrl
{
public:
	LLNameListCtrl(const LLString& name,
				   const LLRect& rect,
				   LLUICtrlCallback callback,
				   void* userdata,
				   BOOL allow_multiple_selection,
				   BOOL draw_border = TRUE,
				   S32 name_column_index = 0,
				   const LLString& tooltip = LLString::null);
	virtual ~LLNameListCtrl();
	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_NAME_LIST; }
	virtual LLString getWidgetTag() const { return LL_NAME_LIST_CTRL_TAG; }
	virtual LLXMLNodePtr getXML(bool save_children = true) const;
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	// Add a user to the list by name.  It will be added, the name 
	// requested from the cache, and updated as necessary.
	BOOL addNameItem(const LLUUID& agent_id, EAddPosition pos = ADD_BOTTOM,
					 BOOL enabled = TRUE, LLString& suffix = LLString::null);
	BOOL addNameItem(LLScrollListItem* item, EAddPosition pos = ADD_BOTTOM);

	virtual LLScrollListItem* addElement(const LLSD& value, EAddPosition pos = ADD_BOTTOM, void* userdata = NULL);

	// Add a user to the list by name.  It will be added, the name 
	// requested from the cache, and updated as necessary.
	void addGroupNameItem(const LLUUID& group_id, EAddPosition pos = ADD_BOTTOM,
						  BOOL enabled = TRUE);
	void addGroupNameItem(LLScrollListItem* item, EAddPosition pos = ADD_BOTTOM);


	void removeNameItem(const LLUUID& agent_id);

	void refresh(const LLUUID& id, const char* first, const char* last,
				 BOOL is_group);

	static void refreshAll(const LLUUID& id, const char* firstname,
						   const char* lastname, BOOL is_group);

	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
									  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
									  EAcceptance *accept,
									  LLString& tooltip_msg);

	void setAllowCallingCardDrop(BOOL b) { mAllowCallingCardDrop = b; }

private:
	static std::set<LLNameListCtrl*> sInstances;
	S32    	 mNameColumnIndex;
	BOOL	 mAllowCallingCardDrop;
};

#endif
