/** 
 * @file llfloaterwindowsize.cpp
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
 
#include "llviewerprecompiledheaders.h"

#include "llfloaterwindowsize.h"

// Viewer includes
#include "llviewerwindow.h"

// Linden library includes
#include "llcombobox.h"
#include "llfloater.h"
#include "llfloaterreg.h"
#include "lluictrl.h"

// System libraries
#include <boost/regex.hpp>

// Extract from strings of the form "<width> x <height>", e.g. "640 x 480".
bool extractWindowSizeFromString(const std::string& instr, U32 *width, U32 *height)
{
	boost::cmatch what;
	// matches (any number)(any non-number)(any number)
	const boost::regex expression("([0-9]+)[^0-9]+([0-9]+)");
	if (boost::regex_match(instr.c_str(), what, expression))
	{
		*width = atoi(what[1].first);
		*height = atoi(what[2].first);
		return true;
	}
	
	*width = 0;
	*height = 0;
	return false;
}


///----------------------------------------------------------------------------
/// Class LLFloaterWindowSize
///----------------------------------------------------------------------------
class LLFloaterWindowSize
:	public LLFloater
{
	friend class LLFloaterReg;
private:
	LLFloaterWindowSize(const LLSD& key);
	virtual ~LLFloaterWindowSize();

public:
	/*virtual*/ BOOL postBuild();
	void initWindowSizeControls();
	void onClickSet();
	void onClickCancel();
};


LLFloaterWindowSize::LLFloaterWindowSize(const LLSD& key) 
:	LLFloater(key)
{
	//LLUICtrlFactory::getInstance()->buildFloater(this, "floater_window_size.xml");	
}

LLFloaterWindowSize::~LLFloaterWindowSize()
{
}

BOOL LLFloaterWindowSize::postBuild()
{
	center();
	initWindowSizeControls();
	getChild<LLUICtrl>("set_btn")->setCommitCallback(
		boost::bind(&LLFloaterWindowSize::onClickSet, this));
	getChild<LLUICtrl>("cancel_btn")->setCommitCallback(
		boost::bind(&LLFloaterWindowSize::onClickCancel, this));
	setDefaultBtn("set_btn");
	return TRUE;
}

void LLFloaterWindowSize::initWindowSizeControls()
{
	LLComboBox* ctrl_window_size = getChild<LLComboBox>("window_size_combo");
	
	// Look to see if current window size matches existing window sizes, if so then
	// just set the selection value...
	const U32 height = gViewerWindow->getWindowHeightRaw();
	const U32 width = gViewerWindow->getWindowWidthRaw();
	for (S32 i=0; i < ctrl_window_size->getItemCount(); i++)
	{
		U32 height_test = 0;
		U32 width_test = 0;
		ctrl_window_size->setCurrentByIndex(i);
		std::string resolution = ctrl_window_size->getValue().asString();
		if (extractWindowSizeFromString(resolution, &width_test, &height_test))
		{
			if ((height_test == height) && (width_test == width))
			{
				return;
			}
		}
	}
	// ...otherwise, add a new entry with the current window height/width.
	LLUIString resolution_label = getString("resolution_format");
	resolution_label.setArg("[RES_X]", llformat("%d", width));
	resolution_label.setArg("[RES_Y]", llformat("%d", height));
	ctrl_window_size->add(resolution_label, ADD_TOP);
	ctrl_window_size->setCurrentByIndex(0);
}

void LLFloaterWindowSize::onClickSet()
{
	LLComboBox* ctrl_window_size = getChild<LLComboBox>("window_size_combo");
	U32 width = 0;
	U32 height = 0;
	std::string resolution = ctrl_window_size->getValue().asString();
	if (extractWindowSizeFromString(resolution, &width, &height))
	{
		LLViewerWindow::movieSize(width, height);
	}
	closeFloater();
}

void LLFloaterWindowSize::onClickCancel()
{
	closeFloater();
}

///----------------------------------------------------------------------------
/// LLFloaterWindowSizeUtil
///----------------------------------------------------------------------------
void LLFloaterWindowSizeUtil::registerFloater()
{
	LLFloaterReg::add("window_size", "floater_window_size.xml",
		&LLFloaterReg::build<LLFloaterWindowSize>);

}
