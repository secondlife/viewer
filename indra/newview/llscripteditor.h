/**
 * @file llscripteditor.h
 * @author Cinder Roxley
 * @brief Text editor widget used for viewing and editing scripts
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#ifndef LL_SCRIPTEDITOR_H
#define LL_SCRIPTEDITOR_H

#include "lltexteditor.h"

class LLScriptEditor : public LLTextEditor
{
	friend class LLUICtrlFactory;
public:
	
	struct Params : public LLInitParam::Block<Params, LLTextEditor::Params>
	{
		Params();
	};
	
	virtual ~LLScriptEditor() {};
	void	initKeywords();
	void	loadKeywords();
	void	clearSegments();
	LLKeywords::keyword_iterator_t keywordsBegin()	{ return mKeywords.begin(); }
	LLKeywords::keyword_iterator_t keywordsEnd()	{ return mKeywords.end(); }
	
protected:
	LLScriptEditor(const Params& p);
	
private:
	void	updateSegments();
	void	loadKeywords(const std::string& filename_keywords,
						 const std::string& filename_colors);
	
	LLKeywords	mKeywords;
};

#endif // LL_SCRIPTEDITOR_H
