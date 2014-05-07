/**
 * @file llscripteditor.cpp
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

#include "llviewerprecompiledheaders.h"
#include "llscripteditor.h"

#include "llsyntaxid.h"

static LLDefaultChildRegistry::Register<LLScriptEditor> r("script_editor");

LLScriptEditor::Params::Params()
{

}


LLScriptEditor::LLScriptEditor(const Params& p)
:	LLTextEditor(p)
{
	
}

void LLScriptEditor::initKeywords()
{
	mKeywords.initialise(LLSyntaxIdLSL::getInstance()->getKeywordsXML());
}

LLTrace::BlockTimerStatHandle FTM_SYNTAX_HIGHLIGHTING("Syntax Highlighting");

void LLScriptEditor::loadKeywords()
{
	LL_RECORD_BLOCK_TIME(FTM_SYNTAX_HIGHLIGHTING);
	mKeywords.processTokens();
	
	segment_vec_t segment_list;
	mKeywords.findSegments(&segment_list, getWText(), mDefaultColor.get(), *this);
	
	mSegments.clear();
	segment_set_t::iterator insert_it = mSegments.begin();
	for (segment_vec_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
	{
		insert_it = mSegments.insert(insert_it, *list_it);
	}
}

void LLScriptEditor::updateSegments()
{
	if (mReflowIndex < S32_MAX && mKeywords.isLoaded() && mParseOnTheFly)
	{
		LL_RECORD_BLOCK_TIME(FTM_SYNTAX_HIGHLIGHTING);
		// HACK:  No non-ascii keywords for now
		segment_vec_t segment_list;
		mKeywords.findSegments(&segment_list, getWText(), mDefaultColor.get(), *this);
		
		clearSegments();
		for (segment_vec_t::iterator list_it = segment_list.begin(); list_it != segment_list.end(); ++list_it)
		{
			insertSegment(*list_it);
		}
	}
	
	LLTextBase::updateSegments();
}

void LLScriptEditor::clearSegments()
{
	if (!mSegments.empty())
	{
		mSegments.clear();
	}
}
