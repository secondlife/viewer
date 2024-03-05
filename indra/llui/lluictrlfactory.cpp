/** 
 * @file lluictrlfactory.cpp
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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
#include "llpanel.h"

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
	std::string filename = gDirUtilp->add("widgets", widget_tag + ".xml");
	LLXMLNodePtr root_node;
	std::vector<std::string> search_paths =
		gDirUtilp->findSkinnedFilenames(LLDir::XUI, filename);

	if (search_paths.empty())
	{
		return;
	}

	// "en" version, the default-language version of the file.
	std::string base_filename = search_paths.front();
	if (!base_filename.empty())
	{
		LLUICtrlFactory::instance().pushFileName(base_filename);

		if (!LLXMLNode::getLayeredXMLNode(root_node, search_paths))
		{
			LL_WARNS() << "Couldn't parse widget from: " << base_filename << LL_ENDL;
			return;
		}
		LLXUIParser parser;
		parser.readXUI(root_node, block, base_filename);
		LLUICtrlFactory::instance().popFileName();
	}
}

//static 
void LLUICtrlFactory::createChildren(LLView* viewp, LLXMLNodePtr node, const widget_registry_t& registry, LLXMLNodePtr output_node)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
	if (node.isNull()) return;

	for (LLXMLNodePtr child_node = node->getFirstChild(); child_node.notNull(); child_node = child_node->getNextSibling())
	{
		LLXMLNodePtr outputChild;
		if (output_node) 
		{
			outputChild = output_node->createChild("", false);
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
				LL_WARNS() << child_name << " is not a valid child of " << node->getName()->mString << LL_ENDL;
			}
			else
			{
				LL_WARNS() << "Could not create widget named " << child_node->getName()->mString << LL_ENDL;
			}
		}

		if (outputChild && !outputChild->mChildren && outputChild->mAttributes.empty() && outputChild->getValue().empty())
		{
			output_node->deleteChild(outputChild);
		}
	}

}

//-----------------------------------------------------------------------------
// getLayeredXMLNode()
//-----------------------------------------------------------------------------
bool LLUICtrlFactory::getLayeredXMLNode(const std::string &xui_filename, LLXMLNodePtr& root,
                                        LLDir::ESkinConstraint constraint, bool cacheable)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
	std::vector<std::string> paths =
		gDirUtilp->findSkinnedFilenames(LLDir::XUI, xui_filename, constraint);

	if (paths.empty())
	{
		// sometimes whole path is passed in as filename
		paths.push_back(xui_filename);
	}

	return LLXMLNode::getLayeredXMLNode(root, paths, cacheable);
}


//-----------------------------------------------------------------------------
// saveToXML()
//-----------------------------------------------------------------------------
S32 LLUICtrlFactory::saveToXML(LLView* viewp, const std::string& filename)
{
	return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

LLView *LLUICtrlFactory::createFromXML(LLXMLNodePtr node, LLView* parent, const std::string& filename, const widget_registry_t& registry, LLXMLNodePtr output_node)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_UI;
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

std::string LLUICtrlFactory::getCurFileName() 
{ 
	return mFileNames.empty() ? "" : mFileNames.back(); 
}


void LLUICtrlFactory::pushFileName(const std::string& name) 
{
	// Here we seem to be looking for the default language file ("en") rather
	// than the localized one, if any.
	mFileNames.push_back(gDirUtilp->findSkinnedFilenameBaseLang(LLDir::XUI, name));
}

void LLUICtrlFactory::popFileName() 
{ 
	mFileNames.pop_back(); 
}

//static
void LLUICtrlFactory::setCtrlParent(LLView* view, LLView* parent, S32 tab_group)
{
	if (tab_group == S32_MAX) tab_group = parent->getLastTabGroup();
	parent->addChild(view, tab_group);
}

//static 
void LLUICtrlFactory::copyName(LLXMLNodePtr src, LLXMLNodePtr dest)
{
	dest->setName(src->getName()->mString);
}

template<typename T>
const LLInitParam::BaseBlock& get_empty_param_block()
{
	static typename T::Params params;
	return params;
}

// adds a widget and its param block to various registries
//static 
void LLUICtrlFactory::registerWidget(const std::type_info* widget_type, const std::type_info* param_block_type, const std::string& name)
{
	// associate parameter block type with template .xml file
	std::string* existing_name = LLWidgetNameRegistry::instance().getValue(param_block_type);
	if (existing_name != NULL)
	{
		if(*existing_name != name)
		{
			std::cerr << "Duplicate entry for T::Params, try creating empty param block in derived classes that inherit T::Params" << std::endl;
			// forcing crash here
			char* foo = 0;
			*foo = 1;
		}
		else
		{
			// widget already registered this name
			return;
		}
	}

	LLWidgetNameRegistry::instance().defaultRegistrar().add(param_block_type, name);
	//FIXME: comment this in when working on schema generation
	//LLWidgetTypeRegistry::instance().defaultRegistrar().add(tag, widget_type);
	//LLDefaultParamBlockRegistry::instance().defaultRegistrar().add(widget_type, &get_empty_param_block<T>);
}


