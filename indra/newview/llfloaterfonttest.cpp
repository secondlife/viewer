/** 
 * @file llfloaterfonttest.cpp
 * @author Brad Payne
 * @brief LLFloaterFontTest class implementation
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2008 Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

/**
 * Floater that appears when buying an object, giving a preview
 * of its contents and their permissions.
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterfonttest.h"
#include "lluictrlfactory.h"


LLFloaterFontTest* LLFloaterFontTest::sInstance = NULL;

LLFloaterFontTest::LLFloaterFontTest()
	:	LLFloater(std::string("floater_font_test"), LLRect(0,500,700,0), std::string("Font Test"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_font_test.xml");
}

LLFloaterFontTest::~LLFloaterFontTest()
{
	sInstance = NULL;
}

// static
void LLFloaterFontTest::show(void *unused)
{
	if (!sInstance)
		sInstance = new LLFloaterFontTest();

	sInstance->open(); /*Flawfinder: ignore*/
	sInstance->setFocus(TRUE);
}
