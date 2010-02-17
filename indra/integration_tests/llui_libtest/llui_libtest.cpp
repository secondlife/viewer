/** 
 * @file llui_libtest.cpp
 * @brief Integration test for the LLUI library
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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
#include "linden_common.h"

#include "llui_libtest.h"

// project includes
#include "llwidgetreg.h"

// linden library includes
#include "llcontrol.h"		// LLControlGroup
#include "lldir.h"
#include "llerrorcontrol.h"
#include "llfloater.h"
#include "llfontfreetype.h"
#include "llfontgl.h"
#include "lltransutil.h"
#include "llui.h"
#include "lluictrlfactory.h"

#include <iostream>

// *TODO: switch to using TUT
// *TODO: teach Parabuild about this program, run automatically after full builds

// I believe these must be globals, not stack variables.  JC
LLControlGroup gSavedSettings("Global");	// saved at end of session
LLControlGroup gSavedPerAccountSettings("PerAccount"); // saved at end of session
LLControlGroup gWarningSettings("Warnings"); // persists ignored dialogs/warnings

// We can't create LLImageGL objects because we have no window or rendering 
// context.  Provide enough of an LLUIImage to test the LLUI library without
// an underlying image.
class TestUIImage : public LLUIImage
{
public:
	TestUIImage()
	:	LLUIImage( std::string(), NULL ) // NULL ImageGL, don't deref!
	{ }

	/*virtual*/ S32 getWidth() const
	{
		return 16;
	}

	/*virtual*/ S32 getHeight() const
	{
		return 16;
	}
};


class LLTexture ;
// We need to supply dummy images
class TestImageProvider : public LLImageProviderInterface
{
public:
	/*virtual*/ LLPointer<LLUIImage> getUIImage(const std::string& name, S32 priority)
	{
		return makeImage();
	}

	/*virtual*/ LLPointer<LLUIImage> getUIImageByID(const LLUUID& id, S32 priority)
	{
		return makeImage();
	}

	/*virtual*/ void cleanUp()
	{
	}

	LLPointer<LLUIImage> makeImage()
	{
		LLPointer<LLTexture> image_gl;
		LLPointer<LLUIImage> image = new TestUIImage(); //LLUIImage( std::string(), image_gl);
		mImageList.push_back(image);
		return image;
	}
	
public:
	// Unclear if we need this, hold on to one copy of each image we make
	std::vector<LLPointer<LLUIImage> > mImageList;
};
TestImageProvider gTestImageProvider;

static std::string get_xui_dir()
{
	std::string delim = gDirUtilp->getDirDelimiter();
	return gDirUtilp->getSkinBaseDir() + delim + "base" + delim + "xui" + delim;
}

void init_llui()
{
	// Font lookup needs directory support
#if LL_DARWIN
	const char* newview_path = "../../../../newview";
#else
	const char* newview_path = "../../../newview";
#endif
	gDirUtilp->initAppDirs("SecondLife", newview_path);
	gDirUtilp->setSkinFolder("base");
	
	// colors are no longer stored in a LLControlGroup file
	LLUIColorTable::instance().loadFromSettings();

	std::string config_filename = gDirUtilp->getExpandedFilename(
																 LL_PATH_APP_SETTINGS, "settings.xml");
	gSavedSettings.loadFromFile(config_filename);
	
	// See LLAppViewer::init()
	LLUI::settings_map_t settings;
	settings["config"] = &gSavedSettings;
	settings["ignores"] = &gWarningSettings;
	settings["floater"] = &gSavedSettings;
	settings["account"] = &gSavedPerAccountSettings;
	
	// Don't use real images as we don't have a GL context
	LLUI::initClass(settings, &gTestImageProvider);
	
	const bool no_register_widgets = false;
	LLWidgetReg::initClass( no_register_widgets );
	
	// Unclear if this is needed
	LLUI::setupPaths();
	// Otherwise we get translation warnings when setting up floaters
	// (tooltips for buttons)
	std::set<std::string> default_args;
	LLTransUtil::parseStrings("strings.xml", default_args);
	LLTransUtil::parseLanguageStrings("language_settings.xml");
	LLFontManager::initClass();
	
	// Creating widgets apparently requires fonts to be initialized,
	// otherwise it crashes.
	LLFontGL::initClass(96.f, 1.f, 1.f,
						gDirUtilp->getAppRODataDir(),
						LLUI::getXUIPaths(),
						false );	// don't create gl textures
	
	LLFloaterView::Params fvparams;
	fvparams.name("Floater View");
	fvparams.rect( LLRect(0,480,640,0) );
	fvparams.mouse_opaque(false);
	fvparams.follows.flags(FOLLOWS_ALL);
	fvparams.tab_stop(false);
	gFloaterView = LLUICtrlFactory::create<LLFloaterView> (fvparams);
}

void export_test_floaters()
{
	// Convert all test floaters to new XML format
	std::string delim = gDirUtilp->getDirDelimiter();
	std::string xui_dir = get_xui_dir() + "en" + delim;
	std::string filename;
	while (gDirUtilp->getNextFileInDir(xui_dir, "floater_test_*.xml", filename, false))
	{
		if (filename.find("_new.xml") != std::string::npos)
		{
			// don't re-export other test floaters
			continue;
		}
		llinfos << "Converting " << filename << llendl;
		// Build a floater and output new attributes
		LLXMLNodePtr output_node = new LLXMLNode();
		LLFloater* floater = new LLFloater(LLSD());
		LLUICtrlFactory::getInstance()->buildFloater(floater,
													 filename,
												//	 FALSE,	// don't open floater
													 output_node);
		std::string out_filename = xui_dir + filename;
		std::string::size_type extension_pos = out_filename.rfind(".xml");
		out_filename.resize(extension_pos);
		out_filename += "_new.xml";
		
		llinfos << "Output: " << out_filename << llendl;
		LLFILE* floater_file = LLFile::fopen(out_filename.c_str(), "w");
		LLXMLNode::writeHeaderToFile(floater_file);
		output_node->writeToFile(floater_file);
		fclose(floater_file);
	}
}

int main(int argc, char** argv)
{
	// Must init LLError for llerrs to actually cause errors.
	LLError::initForApplication(".");

	init_llui();
	
	export_test_floaters();
	
	return 0;
}
