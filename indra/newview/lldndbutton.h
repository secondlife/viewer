/**
 * @file lldndbutton.h
 * @brief Declaration of the drag-n-drop button.
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLDNDBUTTON_H
#define LL_LLDNDBUTTON_H

#include "llbutton.h"

/**
 * Class representing a button which can handle Drag-And-Drop event.
 *
 * LLDragAndDropButton does not contain any logic to handle Drag-And-Drop itself.
 * Instead it provides drag_drop_handler_t which can be set to the button.
 * Then each Drag-And-Drop will be delegated to this handler without any pre/post processing.
 *
 * All xml parameters are the same as LLButton has.
 *
 * @see LLLandmarksPanel for example of usage of this class.
 */
class LLDragAndDropButton : public LLButton
{
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
		Params();
	};

	LLDragAndDropButton(Params& params);

	typedef boost::function<bool (
		S32 /*x*/, S32 /*y*/, MASK /*mask*/, BOOL /*drop*/,
		EDragAndDropType /*cargo_type*/,
		void* /*cargo_data*/,
		EAcceptance* /*accept*/,
		std::string& /*tooltip_msg*/)> drag_drop_handler_t;


	/**
	 * Sets a handler which should process Drag-And-Drop.
	 */
	void setDragAndDropHandler(drag_drop_handler_t handler) { mDragDropHandler = handler; }


	/**
	 * Process Drag-And-Drop by delegating the event to drag_drop_handler_t.
	 * 
	 * @return BOOL - value returned by drag_drop_handler_t if it is set, FALSE otherwise.
	 */
	/*virtual*/ BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		EAcceptance* accept,
		std::string& tooltip_msg);

private:
	drag_drop_handler_t mDragDropHandler;
};


#endif // LL_LLDNDBUTTON_H
