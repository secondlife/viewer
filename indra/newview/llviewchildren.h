/** 
 * @file llviewchildren.h
 * @brief LLViewChildren class header file
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
	void show(const std::string& id, bool visible = true);
	void hide(const std::string& id) { show(id, false); }

	void enable(const std::string& id, bool enabled = true);
	void disable(const std::string& id) { enable(id, false); };

	//
	// LLTextBox
	void setText(const std::string& id,
		const std::string& text, bool visible = true);

	// LLIconCtrl
	enum Badge { BADGE_OK, BADGE_NOTE, BADGE_WARN, BADGE_ERROR };
	
	void setBadge(const std::string& id, Badge b, bool visible = true);

	
	// LLButton
	void setAction(const std::string& id, void(*function)(void*), void* value);


private:
	LLPanel& mParent;
};

#endif
