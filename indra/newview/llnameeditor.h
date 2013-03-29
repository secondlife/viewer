/** 
 * @file llnameeditor.h
 * @brief display and refresh a name from the name cache
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLNAMEEDITOR_H
#define LL_LLNAMEEDITOR_H

#include <set>

#include "llview.h"
#include "v4color.h"
#include "llstring.h"
#include "llfontgl.h"
#include "lllineeditor.h"


class LLNameEditor
:	public LLLineEditor
{
public:
	struct Params : public LLInitParam::Block<Params, LLLineEditor::Params>
	{
		Optional<bool>		is_group;
		Optional<LLUUID>	name_id;

		Params()
		:	is_group("is_group"),
			name_id("name_id")
		{}
	};

protected:
	LLNameEditor(const Params&);
	friend class LLUICtrlFactory;
public:
	virtual ~LLNameEditor();

	void setNameID(const LLUUID& name_id, BOOL is_group);

	void refresh(const LLUUID& id, const std::string& full_name, bool is_group);

	static void refreshAll(const LLUUID& id, const std::string& full_name, bool is_group);


	// Take/return agent UUIDs
	virtual void	setValue( const LLSD& value );
	virtual LLSD	getValue() const;

private:
	static std::set<LLNameEditor*> sInstances;

private:
	LLUUID mNameID;

};

#endif
