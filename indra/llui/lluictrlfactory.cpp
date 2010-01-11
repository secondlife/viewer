/** 
 * @file lluictrlfactory.cpp
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#define LLUICTRLFACTORY_CPP
#include "lluictrlfactory.h"

#include "llxmlnode.h"

#include <fstream>
#include <boost/tokenizer.hpp>

// other library includes
#include "llcontrol.h"
#include "lldir.h"
#include "v4color.h"
#include "v3dmath.h"
#include "llquaternion.h"

// this library includes
#include "llfloater.h"

LLFastTimer::DeclareTimer FTM_WIDGET_CONSTRUCTION("Widget Construction");
LLFastTimer::DeclareTimer FTM_INIT_FROM_PARAMS("Widget InitFromParams");
LLFastTimer::DeclareTimer FTM_WIDGET_SETUP("Widget Setup");

//-----------------------------------------------------------------------------

// UI Ctrl class for padding
class LLUICtrlLocate : public LLUICtrl
{
public:
	struct Params : public LLInitParam::Block<Params, LLUICtrl::Params>
	{
		Params()
		{
			name = "locate";
			tab_stop = false;
		}
	};

	LLUICtrlLocate(const Params& p) : LLUICtrl(p) {}
	virtual void draw() { }

};

static LLDefaultChildRegistry::Register<LLUICtrlLocate> r1("locate");

// Build time optimization, generate this once in .cpp file
template class LLUICtrlFactory* LLSingleton<class LLUICtrlFactory>::getInstance();

//-----------------------------------------------------------------------------
// LLUICtrlFactory()
//-----------------------------------------------------------------------------
LLUICtrlFactory::LLUICtrlFactory()
	: mDummyPanel(NULL) // instantiated when first needed
{
}

LLUICtrlFactory::~LLUICtrlFactory()
{
	// go ahead and leak mDummyPanel since this is static destructor time
	//delete mDummyPanel;
	//mDummyPanel = NULL;
}

void LLUICtrlFactory::loadWidgetTemplate(const std::string& widget_tag, LLInitParam::BaseBlock& block)
{
	std::string filename = std::string("widgets") + gDirUtilp->getDirDelimiter() + widget_tag + ".xml";
	LLXMLNodePtr root_node;

	if (LLUICtrlFactory::getLayeredXMLNode(filename, root_node))
	{
		LLXUIParser::instance().readXUI(root_node, block, filename);
	}
}

static LLFastTimer::DeclareTimer FTM_CREATE_CHILDREN("Create XUI Children");

//static 
void LLUICtrlFactory::createChildren(LLView* viewp, LLXMLNodePtr node, const widget_registry_t& registry, LLXMLNodePtr output_node)
{
	LLFastTimer ft(FTM_CREATE_CHILDREN);
	if (node.isNull()) return;

	for (LLXMLNodePtr child_node = node->getFirstChild(); child_node.notNull(); child_node = child_node->getNextSibling())
	{
		LLXMLNodePtr outputChild;
		if (output_node) 
		{
			outputChild = output_node->createChild("", FALSE);
		}

		if (!instance().createFromXML(child_node, viewp, LLStringUtil::null, registry, outputChild))
		{
			// child_node is not a valid child for the current parent
			std::string child_name = std::string(child_node->getName()->mString);
			if (LLDefaultChildRegistry::instance().getValue(child_name))
			{
				// This means that the registry assocaited with the parent widget does not have an entry
				// for the child widget
				// You might need to add something like:
				// static ParentWidgetRegistry::Register<ChildWidgetType> register("child_widget_name");
				llwarns << child_name << " is not a valid child of " << node->getName()->mString << llendl;
			}
			else
			{
				llwarns << "Could not create widget named " << child_node->getName()->mString << llendl;
			}
		}

		if (outputChild && !outputChild->mChildren && outputChild->mAttributes.empty() && outputChild->getValue().empty())
		{
			output_node->deleteChild(outputChild);
		}
	}

}

static LLFastTimer::DeclareTimer FTM_XML_PARSE("XML Reading/Parsing");
//-----------------------------------------------------------------------------
// getLayeredXMLNode()
//-----------------------------------------------------------------------------
bool LLUICtrlFactory::getLayeredXMLNode(const std::string &xui_filename, LLXMLNodePtr& root)
{
	LLFastTimer timer(FTM_XML_PARSE);
	return LLXMLNode::getLayeredXMLNode(xui_filename, root, LLUI::getXUIPaths());
}


//-----------------------------------------------------------------------------
// getLocalizedXMLNode()
//-----------------------------------------------------------------------------
bool LLUICtrlFactory::getLocalizedXMLNode(const std::string &xui_filename, LLXMLNodePtr& root)
{
	LLFastTimer timer(FTM_XML_PARSE);
	std::string full_filename = gDirUtilp->findSkinnedFilename(LLUI::getLocalizedSkinPath(), xui_filename);
	if (!LLXMLNode::parseFile(full_filename, root, NULL))
	{
		return false;
	}
	else
	{
		return true;
	}
}

static LLFastTimer::DeclareTimer FTM_BUILD_FLOATERS("Build Floaters");

//-----------------------------------------------------------------------------
// buildFloater()
//-----------------------------------------------------------------------------
bool LLUICtrlFactory::buildFloater(LLFloater* floaterp, const std::string& filename, LLXMLNodePtr output_node)
{
	LLFastTimer timer(FTM_BUILD_FLOATERS);
	LLXMLNodePtr root;

	//if exporting, only load the language being exported, 
	//instead of layering localized version on top of english
	if (output_node)
	{
		if (!LLUICtrlFactory::getLocalizedXMLNode(filename, root))
		{
			llwarns << "Couldn't parse floater from: " << LLUI::getLocalizedSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
			return false;
		}
	}
	else if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		llwarns << "Couldn't parse floater from: " << LLUI::getSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
		return false;
	}
	
	// root must be called floater
	if( !(root->hasName("floater") || root->hasName("multi_floater")) )
	{
		llwarns << "Root node should be named floater in: " << filename << llendl;
		return false;
	}
	
	bool res = true;
	
	lldebugs << "Building floater " << filename << llendl;
	mFileNames.push_back(gDirUtilp->findSkinnedFilename(LLUI::getSkinPath(), filename));
	{
		if (!floaterp->getFactoryMap().empty())
		{
			mFactoryStack.push_front(&floaterp->getFactoryMap());
		}

		 // for local registry callbacks; define in constructor, referenced in XUI or postBuild
		floaterp->getCommitCallbackRegistrar().pushScope();
		floaterp->getEnableCallbackRegistrar().pushScope();
		
		res = floaterp->initFloaterXML(root, floaterp->getParent(), output_node);

		floaterp->setXMLFilename(filename);
		
		floaterp->getCommitCallbackRegistrar().popScope();
		floaterp->getEnableCallbackRegistrar().popScope();
		
		if (!floaterp->getFactoryMap().empty())
		{
			mFactoryStack.pop_front();
		}
	}
	mFileNames.pop_back();
	
	return res;
}

//-----------------------------------------------------------------------------
// saveToXML()
//-----------------------------------------------------------------------------
S32 LLUICtrlFactory::saveToXML(LLView* viewp, const std::string& filename)
{
	return 0;
}

static LLFastTimer::DeclareTimer FTM_BUILD_PANELS("Build Panels");

//-----------------------------------------------------------------------------
// buildPanel()
//-----------------------------------------------------------------------------
BOOL LLUICtrlFactory::buildPanel(LLPanel* panelp, const std::string& filename, LLXMLNodePtr output_node)
{
	LLFastTimer timer(FTM_BUILD_PANELS);
	BOOL didPost = FALSE;
	LLXMLNodePtr root;

	//if exporting, only load the language being exported, 
	//instead of layering localized version on top of english
	if (output_node)
	{	
		if (!LLUICtrlFactory::getLocalizedXMLNode(filename, root))
		{
			llwarns << "Couldn't parse panel from: " << LLUI::getLocalizedSkinPath() + gDirUtilp->getDirDelimiter() + filename  << llendl;
			return didPost;
		}
	}
	else if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		llwarns << "Couldn't parse panel from: " << LLUI::getSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
		return didPost;
	}

	// root must be called panel
	if( !root->hasName("panel" ) )
	{
		llwarns << "Root node should be named panel in : " << filename << llendl;
		return didPost;
	}

	lldebugs << "Building panel " << filename << llendl;

	mFileNames.push_back(gDirUtilp->findSkinnedFilename(LLUI::getSkinPath(), filename));
	{
		if (!panelp->getFactoryMap().empty())
		{
			mFactoryStack.push_front(&panelp->getFactoryMap());
		}
		
		 // for local registry callbacks; define in constructor, referenced in XUI or postBuild
		panelp->getCommitCallbackRegistrar().pushScope();
		panelp->getEnableCallbackRegistrar().pushScope();
		
		didPost = panelp->initPanelXML(root, NULL, output_node);

		panelp->getCommitCallbackRegistrar().popScope();
		panelp->getEnableCallbackRegistrar().popScope();
		
		panelp->setXMLFilename(filename);

		if (!panelp->getFactoryMap().empty())
		{
			mFactoryStack.pop_front();
		}
	}
	mFileNames.pop_back();
	return didPost;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static LLFastTimer::DeclareTimer FTM_CREATE_FROM_XML("Create child widget");

LLView *LLUICtrlFactory::createFromXML(LLXMLNodePtr node, LLView* parent, const std::string& filename, const widget_registry_t& registry, LLXMLNodePtr output_node)
{
	LLFastTimer timer(FTM_CREATE_FROM_XML);
	std::string ctrl_type = node->getName()->mString;
	LLStringUtil::toLower(ctrl_type);

	const LLWidgetCreatorFunc* funcp = registry.getValue(ctrl_type);
	if (funcp == NULL)
	{
		return NULL;
	}

	if (parent == NULL)
	{
		if (mDummyPanel == NULL)
		{
			LLPanel::Params p;
			mDummyPanel = create<LLPanel>(p);
		}
		parent = mDummyPanel;
	}
	LLView *view = (*funcp)(node, parent, output_node);	
	
	return view;
}

//-----------------------------------------------------------------------------
// createFactoryPanel()
//-----------------------------------------------------------------------------
LLPanel* LLUICtrlFactory::createFactoryPanel(const std::string& name)
{
	std::deque<const LLCallbackMap::map_t*>::iterator itor;
	for (itor = mFactoryStack.begin(); itor != mFactoryStack.end(); ++itor)
	{
		const LLCallbackMap::map_t* factory_map = *itor;

		// Look up this panel's name in the map.
		LLCallbackMap::map_const_iter_t iter = factory_map->find( name );
		if (iter != factory_map->end())
		{
			// Use the factory to create the panel, instead of using a default LLPanel.
			LLPanel *ret = (LLPanel*) iter->second.mCallback( iter->second.mData );
			return ret;
		}
	}
	LLPanel::Params panel_p;
	return create<LLPanel>(panel_p);
}

//-----------------------------------------------------------------------------

//static
BOOL LLUICtrlFactory::getAttributeColor(LLXMLNodePtr node, const std::string& name, LLColor4& color)
{
	std::string colorstring;
	BOOL res = node->getAttributeString(name.c_str(), colorstring);
	if (res)
	{
		if (LLUIColorTable::instance().colorExists(colorstring))
		{
			color.setVec(LLUIColorTable::instance().getColor(colorstring));
		}
		else
		{
			res = FALSE;
		}
	}
	if (!res)
	{
		res = LLColor4::parseColor(colorstring, &color);
	}	
	if (!res)
	{
		res = node->getAttributeColor(name.c_str(), color);
	}
	return res;
}

//static
void LLUICtrlFactory::setCtrlParent(LLView* view, LLView* parent, S32 tab_group)
{
	if (tab_group == S32_MAX) tab_group = parent->getLastTabGroup();
	parent->addChild(view, tab_group);
}


// Avoid directly using LLUI and LLDir in the template code
//static
std::string LLUICtrlFactory::findSkinnedFilename(const std::string& filename)
{
	return gDirUtilp->findSkinnedFilename(LLUI::getSkinPath(), filename);
}

void LLUICtrlFactory::pushFactoryFunctions(const LLCallbackMap::map_t* map)
{
	mFactoryStack.push_back(map);
}

void LLUICtrlFactory::popFactoryFunctions()
{
	if (!mFactoryStack.empty())
	{
		mFactoryStack.pop_back();
	}
}

//static 
void LLUICtrlFactory::copyName(LLXMLNodePtr src, LLXMLNodePtr dest)
{
	dest->setName(src->getName()->mString);
}

// adds a widget and its param block to various registries
//static 
void LLUICtrlFactory::registerWidget(const std::type_info* widget_type, const std::type_info* param_block_type, dummy_widget_creator_func_t creator_func, const std::string& tag)
{
	// associate parameter block type with template .xml file
	std::string* existing_tag = LLWidgetNameRegistry::instance().getValue(param_block_type);
	if (existing_tag != NULL && *existing_tag != tag)
	{
		llerrs << "Duplicate entry for T::Params, try creating empty param block in derived classes that inherit T::Params" << llendl;
	}
	LLWidgetNameRegistry ::instance().defaultRegistrar().add(param_block_type, tag);
	// associate widget type with factory function
	LLDefaultWidgetRegistry::instance().defaultRegistrar().add(widget_type, creator_func);
	//FIXME: comment this in when working on schema generation
	//LLWidgetTypeRegistry::instance().defaultRegistrar().add(tag, widget_type);
	//LLDefaultParamBlockRegistry::instance().defaultRegistrar().add(widget_type, &getEmptyParamBlock<T>);
}

//static
dummy_widget_creator_func_t* LLUICtrlFactory::getDefaultWidgetFunc(const std::type_info* widget_type)
{
	return LLDefaultWidgetRegistry::instance().getValue(widget_type);
}

//static 
const std::string* LLUICtrlFactory::getWidgetTag(const std::type_info* widget_type)
{
	return LLWidgetNameRegistry::instance().getValue(widget_type);
}

