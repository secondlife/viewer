/** 
 * @file lltoolgun.h
 * @brief LLToolGun class header file
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

#ifndef LL_TOOLGUN_H
#define LL_TOOLGUN_H

#include "lltool.h"
#include "llui.h"


class LLToolGun : public LLTool
{
public:
	LLToolGun( LLToolComposite* composite=NULL );

	virtual void	draw() override;

	virtual void	handleSelect() override;
	virtual void	handleDeselect() override;

	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask) override;
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask) override;

	virtual LLTool*	getOverrideTool(MASK mask) override { return NULL; }
	virtual BOOL	clipMouseWhenDown() override		{ return FALSE; }
private:
	BOOL mIsSelected;
};

#endif
