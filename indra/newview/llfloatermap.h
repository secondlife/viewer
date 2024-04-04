/** 
 * @file llfloatermap.h
 * @brief The "mini-map" or radar in the upper right part of the screen.
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

#ifndef LL_LLFLOATERMAP_H
#define LL_LLFLOATERMAP_H

#include "llfloater.h"

class LLNetMap;
class LLTextBox;

//
// Classes
//
class LLFloaterMap : public LLFloater
{
public:
	LLFloaterMap(const LLSD& key);
	static LLFloaterMap* getInstance();
	virtual ~LLFloaterMap();
	
	bool 	postBuild() override;
	bool	handleDoubleClick( S32 x, S32 y, MASK mask ) override;
	void	reshape(S32 width, S32 height, bool called_from_parent = true) override;
	void	draw() override;

private:
	void setDirectionPos( LLTextBox* text_box, F32 rotation );
	void updateMinorDirections();

	LLTextBox*		mTextBoxEast;
	LLTextBox*		mTextBoxNorth;
	LLTextBox*		mTextBoxWest;
	LLTextBox*		mTextBoxSouth;

	LLTextBox*		mTextBoxSouthEast;
	LLTextBox*		mTextBoxNorthEast;
	LLTextBox*		mTextBoxNorthWest;
	LLTextBox*		mTextBoxSouthWest;
	
	LLNetMap*		mMap;
};

#endif  // LL_LLFLOATERMAP_H
