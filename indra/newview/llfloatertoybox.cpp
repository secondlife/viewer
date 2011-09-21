/** 
 * @file llfloatertoybox.cpp
 * @brief The toybox for flexibilizing the UI.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "llfloatertoybox.h"

#include "llbutton.h"
#include "llpanel.h"


LLFloaterToybox::LLFloaterToybox(const LLSD& key)
	: LLFloater(key)
	, mBtnRestoreDefaults(NULL)
{
	mCommitCallbackRegistrar.add("Toybox.RestoreDefaults", boost::bind(&LLFloaterToybox::onBtnRestoreDefaults, this));
}

LLFloaterToybox::~LLFloaterToybox()
{
}

BOOL LLFloaterToybox::postBuild()
{	
	center();

	mBtnRestoreDefaults = getChild<LLButton>("btn_restore_defaults");

	//
	// Create Buttons
	//
/*
	LLToyboxButtons::load();

	for (size_t i = 0; i < LLToyboxButtons::buttonCount(); i++)
	{
		LLToyboxButton * button = LLToyboxButtons::get(i);

		// Panel opacity depends on whether or not button position is established
		LLPanel * buttonPanel = createPanelForButton(button);

		mToolBar->add(buttonPanel);
	}
*/

	return TRUE;
}

void LLFloaterToybox::onOpen(const LLSD& key)
{

}

BOOL LLFloaterToybox::canClose()
{
	return TRUE;
}

void LLFloaterToybox::onClose(bool app_quitting)
{

}

void LLFloaterToybox::draw()
{
	LLFloater::draw();
}

void LLFloaterToybox::onFocusReceived()
{

}

void LLFloaterToybox::onBtnRestoreDefaults()
{

}


// eof
