/** 
 * @file llfloatermap.h
 * @brief The "mini-map" or radar in the upper right part of the screen.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_LLFLOATERMAP_H
#define LL_LLFLOATERMAP_H

#include "llfloater.h"

class LLMenuGL;
class LLNetMap;
class LLTextBox;

//
// Classes
//
class LLFloaterMap : public LLFloater
{
public:
	LLFloaterMap(const LLSD& key);
	virtual ~LLFloaterMap();
	
	/*virtual*/ BOOL 	postBuild();
	/*virtual*/ BOOL	handleDoubleClick( S32 x, S32 y, MASK mask );
	/*virtual*/ BOOL	handleRightMouseDown( S32 x, S32 y, MASK mask );
	/*virtual*/ void	reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);
	/*virtual*/ void	draw();
	/*virtual*/ void	onFocusLost();
	/*virtual*/ void	onFocusReceived();
	
private:
	void handleZoom(const LLSD& userdata);
	void handleStopTracking (const LLSD& userdata);
	void setDirectionPos( LLTextBox* text_box, F32 rotation );
	void updateMinorDirections();
	
	LLMenuGL*		mPopupMenu;

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
