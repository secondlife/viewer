/** 
 * @file llbutton.h
 * @brief Header for buttons
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLMENUBUTTON_H
#define LL_LLMENUBUTTON_H

#include "llbutton.h"

class LLToggleableMenu;

class LLMenuButton
: public LLButton
{
	LOG_CLASS(LLMenuButton);

public:
	typedef enum e_menu_position
	{
		MP_TOP_LEFT,
		MP_TOP_RIGHT,
		MP_BOTTOM_LEFT,
		MP_BOTTOM_RIGHT
	} EMenuPosition;

	struct MenuPositions
		:	public LLInitParam::TypeValuesHelper<EMenuPosition, MenuPositions>
	{
		static void declareValues();
	};

	struct Params 
	:	public LLInitParam::Block<Params, LLButton::Params>
	{
		// filename for it's toggleable menu
		Optional<std::string>	menu_filename;
		Optional<EMenuPosition, MenuPositions>	position;
	
		Params();
	};


	
	boost::signals2::connection setMouseDownCallback( const mouse_signal_t::slot_type& cb );

	/*virtual*/ bool handleMouseDown(S32 x, S32 y, MASK mask);
	/*virtual*/ bool handleKeyHere(KEY key, MASK mask );

	void hideMenu();

	LLToggleableMenu* getMenu();
	void setMenu(const std::string& menu_filename, EMenuPosition position = MP_TOP_LEFT);
	void setMenu(LLToggleableMenu* menu, EMenuPosition position = MP_TOP_LEFT, bool take_ownership = false);

	void setMenuPosition(EMenuPosition position) { mMenuPosition = position; }

protected:
	friend class LLUICtrlFactory;
	LLMenuButton(const Params&);
	~LLMenuButton();

	void toggleMenu();
	void updateMenuOrigin();

	void onMenuVisibilityChange(const LLSD& param);

private:
	void cleanup();

	LLHandle<LLView>		mMenuHandle;
	bool					mIsMenuShown;
	EMenuPosition			mMenuPosition;
	S32						mX;
	S32						mY;
	bool					mOwnMenu; // true if we manage the menu lifetime
};


#endif  // LL_LLMENUBUTTON_H
