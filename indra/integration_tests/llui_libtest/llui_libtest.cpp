/** 
 * @file llui_libtest.cpp
 * @brief Integration test for the LLUI library
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
#include "linden_common.h"

#include "llui_libtest.h"

// project includes
#include "llwidgetreg.h"

// linden library includes
#include "llcontrol.h"		// LLControlGroup
#include "lldir.h"
#include "lldiriterator.h"
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

void init_llui()
{
	// Font lookup needs directory support
#if LL_DARWIN
	const char* newview_path = "../../../../newview";
#else
	const char* newview_path = "../../../newview";
#endif
	gDirUtilp->initAppDirs("SecondLife", newview_path);
	gDirUtilp->setSkinFolder("default", "en");
	
	// colors are no longer stored in a LLControlGroup file
	LLUIColorTable::instance().loadFromSettings();

	std::string config_filename = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "settings.xml");
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
						false );	// don't create gl textures
	
	LLFloaterView::Params fvparams;
	fvparams.name("Floater View");
	fvparams.rect( LLRect(0,480,640,0) );
	fvparams.mouse_opaque(false);
	fvparams.follows.flags(FOLLOWS_ALL);
	fvparams.tab_stop(false);
	gFloaterView = LLUICtrlFactory::create<LLFloaterView> (fvparams);
}

/*==========================================================================*|
static std::string get_xui_dir()
{
	std::string delim = gDirUtilp->getDirDelimiter();
	return gDirUtilp->getSkinBaseDir() + delim + "default" + delim + "xui" + delim;
}

// buildFromFile() no longer supports generate-output-LLXMLNode
void export_test_floaters()
{
	// Convert all test floaters to new XML format
	std::string delim = gDirUtilp->getDirDelimiter();
	std::string xui_dir = get_xui_dir() + "en" + delim;
	std::string filename;

	LLDirIterator iter(xui_dir, "floater_test_*.xml");
	while (iter.next(filename))
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
		floater->buildFromFile(	filename,
								//	 FALSE,	// don't open floater
								output_node);
		std::string out_filename = gDirUtilp->add(xui_dir, filename);
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
|*==========================================================================*/

int main(int argc, char** argv)
{
	// Must init LLError for llerrs to actually cause errors.
	LLError::initForApplication(".");

	init_llui();
	
//	export_test_floaters();
	
	return 0;
}
