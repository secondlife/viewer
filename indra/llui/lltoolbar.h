/** 
 * @file lltoolbar.h
 * @author Richard Nelson
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#ifndef LL_LLTOOLBAR_H
#define LL_LLTOOLBAR_H

#include "lluictrl.h"
#include "lllayoutstack.h"
#include "llbutton.h"


class LLToolBarButton : public LLButton
{
public:
	struct Params : public LLInitParam::Block<Params, LLButton::Params>
	{
	};

	LLToolBarButton(const Params& p) : LLButton(p) {}
};


namespace LLToolBarEnums
{
	enum ButtonType
	{
		BTNTYPE_ICONS_ONLY = 0,
		BTNTYPE_ICONS_WITH_TEXT,
	};

	enum SideType
	{
		SIDE_NONE = 0,
		SIDE_BOTTOM,
		SIDE_LEFT,
		SIDE_RIGHT,
		SIDE_TOP,
	};
}

// NOTE: This needs to occur before Param block declaration for proper compilation.
namespace LLInitParam
{
	template<>
	struct TypeValues<LLToolBarEnums::ButtonType> : public TypeValuesHelper<LLToolBarEnums::ButtonType>
	{
		static void declareValues();
	};

	template<>
	struct TypeValues<LLToolBarEnums::SideType> : public TypeValuesHelper<LLToolBarEnums::SideType>
	{
		static void declareValues();
	};
}


class LLToolBar
:	public LLUICtrl
{
public:

	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Mandatory<LLToolBarEnums::ButtonType>	button_display_mode;
		Multiple<LLToolBarButton::Params>		buttons;
		Mandatory<LLToolBarEnums::SideType>		side;

		Params();
	};

	// virtuals
	void draw();

protected:
	friend class LLUICtrlFactory;
	LLToolBar(const Params&);

	void initFromParams(const Params&);
	void addButton(LLToolBarButton* buttonp);
	void updateLayout();

private:
	std::list<LLToolBarButton*>		mButtons;
	LLToolBarEnums::ButtonType		mButtonType;
	LLToolBarEnums::SideType		mSideType;
	LLLayoutStack*					mStack;
};


#endif  // LL_LLTOOLBAR_H
