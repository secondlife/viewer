/**
 * @file lldndbutton.h
 * @brief Declaration of the drag-n-drop button.
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
	struct Params : public LLInitParam::Block<Params, LLButton::Params> {};

	LLDragAndDropButton(const Params& params);

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
