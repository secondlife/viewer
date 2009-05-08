/** 
 * @file lltoolpipette.h
 * @brief LLToolPipette class header file
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

// A tool to pick texture entry infro from objects in world (color/texture)
// This tool assumes it is transient in the codebase and must be used
// accordingly. We should probably restructure the way tools are
// managed so that this is handled automatically.

#ifndef LL_LLTOOLPIPETTE_H
#define LL_LLTOOLPIPETTE_H

#include "lltool.h"
#include "lltextureentry.h"
#include <boost/function.hpp>
#include <boost/signals2.hpp>

class LLViewerObject;
class LLPickInfo;

class LLToolPipette
:	public LLTool, public LLSingleton<LLToolPipette>
{
public:
	LLToolPipette();
	virtual ~LLToolPipette();

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect *sticky_rect_screen);

	// Note: Don't return connection; use boost::bind + boost::signal::trackable to disconnect slots
	typedef boost::signals2::signal<void (const LLTextureEntry& te)> signal_t;
	void setToolSelectCallback(const signal_t::slot_type& cb) { mSignal.connect(cb); }
	void setResult(BOOL success, const std::string& msg);
	
	void setTextureEntry(const LLTextureEntry* entry);
	static void pickCallback(const LLPickInfo& pick_info);

protected:
	LLTextureEntry	mTextureEntry;
	signal_t		mSignal;
	BOOL			mSuccess;
	std::string		mTooltipMsg;
};

#endif //LL_LLTOOLPIPETTE_H
