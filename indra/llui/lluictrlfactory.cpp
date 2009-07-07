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

#include "lluictrlfactory.h"

#include <fstream>
#include <boost/tokenizer.hpp>

// other library includes
#include "llcontrol.h"
#include "lldir.h"
#include "v4color.h"
#include "v3dmath.h"
#include "llquaternion.h"

// this library includes
#include "llbutton.h"
#include "llcheckboxctrl.h"
//#include "llcolorswatch.h"
#include "llcombobox.h"
#include "llcontrol.h"
#include "lldir.h"
#include "llevent.h"
#include "llfloater.h"
#include "lliconctrl.h"
#include "lllineeditor.h"
#include "llmenugl.h"
#include "llradiogroup.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "llscrolllistctrl.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "llui.h"
#include "llviewborder.h"

const char XML_HEADER[] = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n";

const S32 HPAD = 4;
const S32 VPAD = 4;
const S32 FLOATER_H_MARGIN = 15;
const S32 MIN_WIDGET_HEIGHT = 10;

LLFastTimer::DeclareTimer FTM_WIDGET_CONSTRUCTION("Widget Construction");
LLFastTimer::DeclareTimer FTM_INIT_FROM_PARAMS("Widget InitFromParams");
LLFastTimer::DeclareTimer FTM_WIDGET_SETUP("Widget Setup");

//-----------------------------------------------------------------------------
// Register widgets that are purely data driven here so they get linked in
#include "llstatview.h"
static LLDefaultChildRegistry::Register<LLStatView>
	register_stat_view("stat_view");

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

//-----------------------------------------------------------------------------
// LLUICtrlFactory()
//-----------------------------------------------------------------------------
LLUICtrlFactory::LLUICtrlFactory()
	: mDummyPanel(NULL) // instantiated when first needed
{
}

LLUICtrlFactory::~LLUICtrlFactory()
{
	delete mDummyPanel;
	mDummyPanel = NULL;
}

void LLUICtrlFactory::loadWidgetTemplate(const std::string& widget_tag, LLInitParam::BaseBlock& block)
{
	std::string filename = std::string("widgets") + gDirUtilp->getDirDelimiter() + widget_tag + ".xml";
	LLXMLNodePtr root_node;

	if (LLUICtrlFactory::getLayeredXMLNode(filename, root_node))
	{
		LLXUIParser::instance().readXUI(root_node, block);
	}
}

//static 
void LLUICtrlFactory::createChildren(LLView* viewp, LLXMLNodePtr node, const widget_registry_t& registry, LLXMLNodePtr output_node)
{
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
			std::string child_name = std::string(child_node->getName()->mString);
			llwarns << "Could not create widget named " << child_node->getName()->mString << llendl;
		}

		if (outputChild && !outputChild->mChildren && outputChild->mAttributes.empty() && outputChild->getValue().empty())
		{
			output_node->deleteChild(outputChild);
		}
	}

}

LLFastTimer::DeclareTimer FTM_XML_PARSE("XML Reading/Parsing");
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

static LLFastTimer::DeclareTimer BUILD_FLOATERS("Build Floaters");

//-----------------------------------------------------------------------------
// buildFloater()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::buildFloater(LLFloater* floaterp, const std::string& filename, BOOL open_floater, LLXMLNodePtr output_node)
{
	LLFastTimer timer(BUILD_FLOATERS);
	LLXMLNodePtr root;

	//if exporting, only load the language being exported, 
	//instead of layering localized version on top of english
	if (output_node)
	{
		if (!LLUICtrlFactory::getLocalizedXMLNode(filename, root))
		{
			llwarns << "Couldn't parse floater from: " << LLUI::getLocalizedSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
			return;
		}
	}
	else if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		llwarns << "Couldn't parse floater from: " << LLUI::getSkinPath() + gDirUtilp->getDirDelimiter() + filename << llendl;
		return;
	}
	
	// root must be called floater
	if( !(root->hasName("floater") || root->hasName("multi_floater")) )
	{
		llwarns << "Root node should be named floater in: " << filename << llendl;
		return;
	}

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
		
		floaterp->initFloaterXML(root, floaterp->getParent(), open_floater, output_node);

		if (LLUI::sShowXUINames)
		{
			floaterp->setToolTip(filename);
		}
		
		floaterp->getCommitCallbackRegistrar().popScope();
		floaterp->getEnableCallbackRegistrar().popScope();
		
		if (!floaterp->getFactoryMap().empty())
		{
			mFactoryStack.pop_front();
		}
	}
	mFileNames.pop_back();
}

LLFloater* LLUICtrlFactory::buildFloaterFromXML(const std::string& filename, BOOL open_floater)
{
	LLFloater* floater = new LLFloater();
	buildFloater(floater, filename, open_floater);
	return floater;
}

//-----------------------------------------------------------------------------
// saveToXML()
//-----------------------------------------------------------------------------
S32 LLUICtrlFactory::saveToXML(LLView* viewp, const std::string& filename)
{
	return 0;
}

static LLFastTimer::DeclareTimer BUILD_PANELS("Build Panels");

//-----------------------------------------------------------------------------
// buildPanel()
//-----------------------------------------------------------------------------
BOOL LLUICtrlFactory::buildPanel(LLPanel* panelp, const std::string& filename, LLXMLNodePtr output_node)
{
	LLFastTimer timer(BUILD_PANELS);
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
		
		if (LLUI::sShowXUINames)
		{
			panelp->setToolTip(filename);
		}

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

LLFastTimer::DeclareTimer FTM_CREATE_FROM_XML("Create child widget");

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
	if (LLUI::sShowXUINames && view && !filename.empty())
	{
		view->setToolTip(filename);
	}
	
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
	if (tab_group < 0) tab_group = parent->getLastTabGroup();
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


//
// LLRNGWriter - writes Relax NG schema files based on a param block
//
LLRNGWriter::LLRNGWriter()
{
	// register various callbacks for inspecting the contents of a param block
	registerInspectFunc<bool>(boost::bind(&LLRNGWriter::writeAttribute, this, "boolean", _1, _2, _3, _4));
	registerInspectFunc<std::string>(boost::bind(&LLRNGWriter::writeAttribute, this, "string", _1, _2, _3, _4));
	registerInspectFunc<U8>(boost::bind(&LLRNGWriter::writeAttribute, this, "unsignedByte", _1, _2, _3, _4));
	registerInspectFunc<S8>(boost::bind(&LLRNGWriter::writeAttribute, this, "signedByte", _1, _2, _3, _4));
	registerInspectFunc<U16>(boost::bind(&LLRNGWriter::writeAttribute, this, "unsignedShort", _1, _2, _3, _4));
	registerInspectFunc<S16>(boost::bind(&LLRNGWriter::writeAttribute, this, "signedShort", _1, _2, _3, _4));
	registerInspectFunc<U32>(boost::bind(&LLRNGWriter::writeAttribute, this, "unsignedInt", _1, _2, _3, _4));
	registerInspectFunc<S32>(boost::bind(&LLRNGWriter::writeAttribute, this, "integer", _1, _2, _3, _4));
	registerInspectFunc<F32>(boost::bind(&LLRNGWriter::writeAttribute, this, "float", _1, _2, _3, _4));
	registerInspectFunc<F64>(boost::bind(&LLRNGWriter::writeAttribute, this, "double", _1, _2, _3, _4));
	registerInspectFunc<LLColor4>(boost::bind(&LLRNGWriter::writeAttribute, this, "string", _1, _2, _3, _4));
	registerInspectFunc<LLUIColor>(boost::bind(&LLRNGWriter::writeAttribute, this, "string", _1, _2, _3, _4));
	registerInspectFunc<LLUUID>(boost::bind(&LLRNGWriter::writeAttribute, this, "string", _1, _2, _3, _4));
	registerInspectFunc<LLSD>(boost::bind(&LLRNGWriter::writeAttribute, this, "string", _1, _2, _3, _4));
}

void LLRNGWriter::writeRNG(const std::string& type_name, LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const std::string& xml_namespace)
{
	mGrammarNode = node;
	mGrammarNode->setName("grammar");
	mGrammarNode->createChild("xmlns", true)->setStringValue("http://relaxng/ns/structure/1.0");
	mGrammarNode->createChild("datatypeLibrary", true)->setStringValue("http://www.w3.org/2001/XMLSchema-datatypes");
	mGrammarNode->createChild("ns", true)->setStringValue(xml_namespace);

	node = mGrammarNode->createChild("start", false);
	node = node->createChild("ref", false);
	node->createChild("name", true)->setStringValue(type_name);
	
	node = mGrammarNode->createChild("define", false);
	node->createChild("name", true)->setStringValue(type_name);

	mElementNode = node->createChild("element", false);
	mElementNode->createChild("name", true)->setStringValue(type_name);

	block.inspectBlock(*this);
}

void LLRNGWriter::writeAttribute(const std::string& type, const Parser::name_stack_t& stack, S32 min_count, S32 max_count, const std::vector<std::string>* possible_values)
{
	name_stack_t non_empty_names;
	std::string attribute_name;
	for (name_stack_t::const_iterator it = stack.begin();
		it != stack.end();
		++it)
	{
		const std::string& name = it->first;
		if (!name.empty())
		{
			non_empty_names.push_back(*it);
		}
	}

	if (non_empty_names.empty()) return;

	for (name_stack_t::const_iterator it = non_empty_names.begin();
		it != non_empty_names.end();
		++it)
	{
		if (!attribute_name.empty())
		{
			attribute_name += ".";
		}
		attribute_name += it->first;
	}

	// singular attribute
	if (non_empty_names.size() == 1)
	{
		if (max_count == 1)
		{
			LLXMLNodePtr node = getCardinalityNode(mElementNode, min_count, max_count)->createChild("attribute", false);
			node->createChild("name", true)->setStringValue(attribute_name);
			node->createChild("data", false)->createChild("type", true)->setStringValue(type);
		}
	}
	// compound attribute
	else
	{
		std::string element_name;

		// traverse all but last element, leaving that as an attribute name
		name_stack_t::const_iterator end_it = non_empty_names.end();
		end_it--;

		for (name_stack_t::const_iterator it = non_empty_names.begin();
			it != end_it;
			++it)
		{
			if (it != non_empty_names.begin())
			{
				element_name += ".";
			}
			element_name += it->first;
		}

		elements_map_t::iterator found_it = mElementsWritten.find(element_name);
		if (found_it != mElementsWritten.end())
		{
			// reuse existing element
			LLXMLNodePtr choice_node = found_it->second;

			LLXMLNodePtr node = choice_node->mChildren->head;
			node = getCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
			node->createChild("name", true)->setStringValue(attribute_name);
			node->createChild("data", false)->createChild("type", true)->setStringValue(type);

			node = choice_node->mChildren->head->mNext->mChildren->head;
			node = getCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
			node->createChild("name", true)->setStringValue(non_empty_names.back().first);
			node->createChild("data", false)->createChild("type", true)->setStringValue(type);
		}
		else
		{
			LLXMLNodePtr choice_node = mElementNode->createChild("choice", false);

			LLXMLNodePtr node = choice_node->createChild("group", false);
			node = getCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
			node->createChild("name", true)->setStringValue(attribute_name);
			node->createChild("data", false)->createChild("type", true)->setStringValue(type);

			node = choice_node->createChild("element", false);
			node->createChild("name", true)->setStringValue(element_name);
			node = getCardinalityNode(node, min_count, max_count)->createChild("attribute", false);
			node->createChild("name", true)->setStringValue(non_empty_names.back().first);
			node->createChild("data", false)->createChild("type", true)->setStringValue(type);
			
			node = choice_node->createChild("element", false);
			node->createChild("name", true)->setStringValue(type + "." + element_name);
			node->createChild("ref", true)->createChild("name", true)->setStringValue(element_name);

			mElementsWritten[element_name] = choice_node;
		}
	}
}

LLXMLNodePtr LLRNGWriter::getCardinalityNode(LLXMLNodePtr parent_node, S32 min_count, S32 max_count)
{
	// unlinked by default, meaning this attribute is forbidden
	LLXMLNodePtr count_node = new LLXMLNode();
	if (min_count >= 1)
	{
		if (max_count == 1 && min_count == 1)
		{
			// just add raw element, will count as 1 and only 1
			count_node = mElementNode;
		}
		else
		{
			count_node = mElementNode->createChild("oneOrMore", false);
		}
	}
	else
	{
		if (max_count == 1)
		{
			count_node = mElementNode->createChild("optional", false);
		}
		else if (max_count > 1)
		{
			count_node = mElementNode->createChild("zeroOrMore", false);
		}
	}
	return count_node;
}
//
// LLXSDWriter
//
LLXSDWriter::LLXSDWriter()
{
	registerInspectFunc<bool>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:boolean", _1, _2, _3, _4));
	registerInspectFunc<std::string>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<U8>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:unsignedByte", _1, _2, _3, _4));
	registerInspectFunc<S8>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:signedByte", _1, _2, _3, _4));
	registerInspectFunc<U16>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:unsignedShort", _1, _2, _3, _4));
	registerInspectFunc<S16>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:signedShort", _1, _2, _3, _4));
	registerInspectFunc<U32>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:unsignedInt", _1, _2, _3, _4));
	registerInspectFunc<S32>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:integer", _1, _2, _3, _4));
	registerInspectFunc<F32>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:float", _1, _2, _3, _4));
	registerInspectFunc<F64>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:double", _1, _2, _3, _4));
	registerInspectFunc<LLColor4>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<LLUIColor>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<LLUUID>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
	registerInspectFunc<LLSD>(boost::bind(&LLXSDWriter::writeAttribute, this, "xs:string", _1, _2, _3, _4));
}

void LLXSDWriter::writeXSD(const std::string& type_name, LLXMLNodePtr node, const LLInitParam::BaseBlock& block, const std::string& xml_namespace)
{
	mSchemaNode = node;
	node->setName("xs:schema");
	node->createChild("attributeFormDefault", true)->setStringValue("unqualified");
	node->createChild("elementFormDefault", true)->setStringValue("qualified");
	node->createChild("targetNamespace", true)->setStringValue(xml_namespace);
	node->createChild("xmlns:xs", true)->setStringValue("http://www.w3.org/2001/XMLSchema");
	node->createChild("xmlns", true)->setStringValue(xml_namespace);

	node = node->createChild("xs:complexType", false);
	node->createChild("name", true)->setStringValue(type_name);
	node->createChild("mixed", true)->setStringValue("true");

	mAttributeNode = node;
	mElementNode = node->createChild("xs:choice", false);
	mElementNode->createChild("minOccurs", true)->setStringValue("0");
	mElementNode->createChild("maxOccurs", true)->setStringValue("unbounded");
	block.inspectBlock(*this);

	// duplicate element choices
	LLXMLNodeList children;
	mElementNode->getChildren("xs:element", children, FALSE);
	for (LLXMLNodeList::iterator child_it = children.begin(); child_it != children.end(); ++child_it)
	{
		LLXMLNodePtr child_copy = child_it->second->deepCopy();
		std::string child_name;
		child_copy->getAttributeString("name", child_name);
		child_copy->setAttributeString("name", type_name + "." + child_name);
		mElementNode->addChild(child_copy);
	}

	LLXMLNodePtr element_declaration_node = mSchemaNode->createChild("xs:element", false);
	element_declaration_node->createChild("name", true)->setStringValue(type_name);
	element_declaration_node->createChild("type", true)->setStringValue(type_name);
}

void LLXSDWriter::writeAttribute(const std::string& type, const Parser::name_stack_t& stack, S32 min_count, S32 max_count, const std::vector<std::string>* possible_values)
{
	name_stack_t non_empty_names;
	std::string attribute_name;
	for (name_stack_t::const_iterator it = stack.begin();
		it != stack.end();
		++it)
	{
		const std::string& name = it->first;
		if (!name.empty())
		{
			non_empty_names.push_back(*it);
		}
	}

	for (name_stack_t::const_iterator it = non_empty_names.begin();
		it != non_empty_names.end();
		++it)
	{
		if (!attribute_name.empty())
		{
			attribute_name += ".";
		}
		attribute_name += it->first;
	}

	// only flag non-nested attributes as mandatory, nested attributes have variant syntax
	// that can't be properly constrained in XSD
	// e.g. <foo mandatory.value="bar"/> vs <foo><mandatory value="bar"/></foo>
	bool attribute_mandatory = min_count == 1 && max_count == 1 && non_empty_names.size() == 1;

	// don't bother supporting "Multiple" params as xml attributes
	if (max_count <= 1)
	{
		// add compound attribute to root node
		addAttributeToSchema(mAttributeNode, attribute_name, type, attribute_mandatory, possible_values);
	}

	// now generated nested elements for compound attributes
	if (non_empty_names.size() > 1 && !attribute_mandatory)
	{
		std::string element_name;

		// traverse all but last element, leaving that as an attribute name
		name_stack_t::const_iterator end_it = non_empty_names.end();
		end_it--;

		for (name_stack_t::const_iterator it = non_empty_names.begin();
			it != end_it;
			++it)
		{
			if (it != non_empty_names.begin())
			{
				element_name += ".";
			}
			element_name += it->first;
		}

		std::string short_attribute_name = non_empty_names.back().first;

		LLXMLNodePtr complex_type_node;

		// find existing element node here, starting at tail of child list
		if (mElementNode->mChildren.notNull())
		{
			for(LLXMLNodePtr element = mElementNode->mChildren->tail;
				element.notNull(); 
				element = element->mPrev)
			{
				std::string name;
				if(element->getAttributeString("name", name) && name == element_name)
				{
					complex_type_node = element->mChildren->head;
					break;
				}
			}
		}
		//create complex_type node
		//
		//<xs:element
        //    maxOccurs="1"
        //    minOccurs="0"
        //    name="name">
        //       <xs:complexType>
        //       </xs:complexType>
        //</xs:element>
		if(complex_type_node.isNull())
		{
			complex_type_node = mElementNode->createChild("xs:element", false);

			complex_type_node->createChild("minOccurs", true)->setIntValue(min_count);
			complex_type_node->createChild("maxOccurs", true)->setIntValue(max_count);
			complex_type_node->createChild("name",		true)->setStringValue(element_name);
			complex_type_node = complex_type_node->createChild("xs:complexType", false);
		}

		addAttributeToSchema(complex_type_node, short_attribute_name, type, false, possible_values);
	}
}

void LLXSDWriter::addAttributeToSchema(LLXMLNodePtr type_declaration_node, const std::string& attribute_name, const std::string& type, bool mandatory, const std::vector<std::string>* possible_values)
{
	if (!attribute_name.empty())
	{
		LLXMLNodePtr new_enum_type_node;
		if (possible_values != NULL)
		{
			// custom attribute type, for example
			//<xs:simpleType>
			 // <xs:restriction
			 //    base="xs:string">
			 //     <xs:enumeration
			 //      value="a" />
			 //     <xs:enumeration
			 //      value="b" />
			 //   </xs:restriction>
			 // </xs:simpleType>
			new_enum_type_node = new LLXMLNode("xs:simpleType", false);

			LLXMLNodePtr restriction_node = new_enum_type_node->createChild("xs:restriction", false);
			restriction_node->createChild("base", true)->setStringValue("xs:string");

			for (std::vector<std::string>::const_iterator it = possible_values->begin();
				it != possible_values->end();
				++it)
			{
				LLXMLNodePtr enum_node = restriction_node->createChild("xs:enumeration", false);
				enum_node->createChild("value", true)->setStringValue(*it);
			}
		}

		string_set_t& attributes_written = mAttributesWritten[type_declaration_node];

		string_set_t::iterator found_it = std::lower_bound(attributes_written.begin(), attributes_written.end(), attribute_name);

		// attribute not yet declared
		if (found_it == attributes_written.end() || attributes_written.key_comp()(attribute_name, *found_it))
		{
			attributes_written.insert(found_it, attribute_name);

			LLXMLNodePtr attribute_node = type_declaration_node->createChild("xs:attribute", false);

			// attribute name
			attribute_node->createChild("name", true)->setStringValue(attribute_name);

			if (new_enum_type_node.notNull())
			{
				attribute_node->addChild(new_enum_type_node);
			}
			else
			{
				// simple attribute type
				attribute_node->createChild("type", true)->setStringValue(type);
			}

			// required or optional
			attribute_node->createChild("use", true)->setStringValue(mandatory ? "required" : "optional");
		}
		 // attribute exists...handle collision of same name attributes with potentially different types
		else
		{
			LLXMLNodePtr attribute_declaration;
			if (type_declaration_node.notNull())
			{
				for(LLXMLNodePtr node = type_declaration_node->mChildren->tail; 
					node.notNull(); 
					node = node->mPrev)
				{
					std::string name;
					if (node->getAttributeString("name", name) && name == attribute_name)
					{
						attribute_declaration = node;
						break;
					}
				}
			}

			bool new_type_is_enum = new_enum_type_node.notNull();
			bool existing_type_is_enum = !attribute_declaration->hasAttribute("type");

			// either type is enum, revert to string in collision
			// don't bother to check for enum equivalence
			if (new_type_is_enum || existing_type_is_enum)
			{
				if (attribute_declaration->hasAttribute("type"))
				{
					attribute_declaration->setAttributeString("type", "xs:string");
				}
				else
				{
					attribute_declaration->createChild("type", true)->setStringValue("xs:string");
				}
				attribute_declaration->deleteChildren("xs:simpleType");
			}
			else 
			{
				// check for collision of different standard types
				std::string existing_type;
				attribute_declaration->getAttributeString("type", existing_type);
				// if current type is not the same as the new type, revert to strnig
				if (existing_type != type)
				{
					// ...than use most general type, string
					attribute_declaration->setAttributeString("type", "string");
				}
			}
		}
	}
}

//
// LLXUIXSDWriter
//
void LLXUIXSDWriter::writeXSD(const std::string& type_name, const std::string& path, const LLInitParam::BaseBlock& block)
{
	std::string file_name(path);
	file_name += type_name + ".xsd";
	LLXMLNodePtr root_nodep = new LLXMLNode();

	LLXSDWriter::writeXSD(type_name, root_nodep, block, "http://www.lindenlab.com/xui");

	// add includes for all possible children
	const std::type_info* type = *LLWidgetTypeRegistry::instance().getValue(type_name);
	const widget_registry_t* widget_registryp = LLChildRegistryRegistry::instance().getValue(type);
	
	// add include declarations for all valid children
	for (widget_registry_t::Registrar::registry_map_t::const_iterator it = widget_registryp->currentRegistrar().beginItems();
		it != widget_registryp->currentRegistrar().endItems();
		++it)
	{
		std::string widget_name = it->first;
		if (widget_name == type_name)
		{
			continue;
		}
		LLXMLNodePtr nodep = new LLXMLNode("xs:include", false);
		nodep->createChild("schemaLocation", true)->setStringValue(widget_name + ".xsd");

		// add to front of schema
		mSchemaNode->addChild(nodep, mSchemaNode);
	}

	// add choices for valid children
	if (widget_registryp)
	{
		for (widget_registry_t::Registrar::registry_map_t::const_iterator it = widget_registryp->currentRegistrar().beginItems();
			it != widget_registryp->currentRegistrar().endItems();
			++it)
		{
			std::string widget_name = it->first;
            //<xs:element name="widget_name" type="widget_name">
			LLXMLNodePtr widget_node = mElementNode->createChild("xs:element", false);
			widget_node->createChild("name", true)->setStringValue(widget_name);
			widget_node->createChild("type", true)->setStringValue(widget_name);
		}
	}

	LLFILE* xsd_file = LLFile::fopen(file_name.c_str(), "w");
	LLXMLNode::writeHeaderToFile(xsd_file);
	root_nodep->writeToFile(xsd_file);
	fclose(xsd_file);
}

//
// LLXUIParser
//
LLXUIParser::LLXUIParser()
:	mLastWriteGeneration(-1),
	mCurReadDepth(0)
{
	registerParserFuncs<bool>(boost::bind(&LLXUIParser::readBoolValue, this, _1),
								boost::bind(&LLXUIParser::writeBoolValue, this, _1, _2));
	registerParserFuncs<std::string>(boost::bind(&LLXUIParser::readStringValue, this, _1),
								boost::bind(&LLXUIParser::writeStringValue, this, _1, _2));
	registerParserFuncs<U8>(boost::bind(&LLXUIParser::readU8Value, this, _1),
								boost::bind(&LLXUIParser::writeU8Value, this, _1, _2));
	registerParserFuncs<S8>(boost::bind(&LLXUIParser::readS8Value, this, _1),
								boost::bind(&LLXUIParser::writeS8Value, this, _1, _2));
	registerParserFuncs<U16>(boost::bind(&LLXUIParser::readU16Value, this, _1),
								boost::bind(&LLXUIParser::writeU16Value, this, _1, _2));
	registerParserFuncs<S16>(boost::bind(&LLXUIParser::readS16Value, this, _1),
								boost::bind(&LLXUIParser::writeS16Value, this, _1, _2));
	registerParserFuncs<U32>(boost::bind(&LLXUIParser::readU32Value, this, _1),
								boost::bind(&LLXUIParser::writeU32Value, this, _1, _2));
	registerParserFuncs<S32>(boost::bind(&LLXUIParser::readS32Value, this, _1),
								boost::bind(&LLXUIParser::writeS32Value, this, _1, _2));
	registerParserFuncs<F32>(boost::bind(&LLXUIParser::readF32Value, this, _1),
								boost::bind(&LLXUIParser::writeF32Value, this, _1, _2));
	registerParserFuncs<F64>(boost::bind(&LLXUIParser::readF64Value, this, _1),
								boost::bind(&LLXUIParser::writeF64Value, this, _1, _2));
	registerParserFuncs<LLColor4>(boost::bind(&LLXUIParser::readColor4Value, this, _1),
								boost::bind(&LLXUIParser::writeColor4Value, this, _1, _2));
	registerParserFuncs<LLUIColor>(boost::bind(&LLXUIParser::readUIColorValue, this, _1),
								boost::bind(&LLXUIParser::writeUIColorValue, this, _1, _2));
	registerParserFuncs<LLUUID>(boost::bind(&LLXUIParser::readUUIDValue, this, _1),
								boost::bind(&LLXUIParser::writeUUIDValue, this, _1, _2));
	registerParserFuncs<LLSD>(boost::bind(&LLXUIParser::readSDValue, this, _1),
								boost::bind(&LLXUIParser::writeSDValue, this, _1, _2));
}

static LLFastTimer::DeclareTimer PARSE_XUI("XUI Parsing");

void LLXUIParser::readXUI(LLXMLNodePtr node, LLInitParam::BaseBlock& block, bool silent)
{
	LLFastTimer timer(PARSE_XUI);
	mNameStack.clear();
	mCurReadDepth = 0;
	setParseSilently(silent);

	if (node.isNull())
	{
		parserWarning("Invalid node");
	}
	else
	{
		readXUIImpl(node, std::string(node->getName()->mString), block);
	}
}

void LLXUIParser::writeXUI(LLXMLNodePtr node, const LLInitParam::BaseBlock &block, const LLInitParam::BaseBlock* diff_block)
{
	mLastWriteGeneration = -1;
	mWriteRootNode = node;
	block.serializeBlock(*this, Parser::name_stack_t(), diff_block);
}

// go from a stack of names to a specific XML node
LLXMLNodePtr LLXUIParser::getNode(const name_stack_t& stack)
{
	name_stack_t name_stack;

	for (name_stack_t::const_iterator it = stack.begin();
		it != stack.end();
		++it)
	{
		if (!it->first.empty())
		{
			name_stack.push_back(*it);
		}
	}

	if (name_stack.empty() || mWriteRootNode.isNull()) return NULL;

	std::string attribute_name = name_stack.front().first;

	// heuristic to make font always attribute of parent node
	bool is_font = (attribute_name == "font");
	// XML spec says that attributes have their whitespace normalized
	// on parse: http://www.w3.org/TR/REC-xml/#AVNormalize
	// Therefore text-oriented widgets that might have carriage returns
	// have their values serialized as text contents, not the
	// initial_value attribute. JC
	if (attribute_name == "initial_value")
	{
		const char* root_node_name = mWriteRootNode->getName()->mString;
		if (!strcmp(root_node_name, "text")		// LLTextBox
			|| !strcmp(root_node_name, "text_editor") 
			|| !strcmp(root_node_name, "line_editor")) // for consistency
		{
			// writeStringValue will write to this node
			return mWriteRootNode;
		}
	}

	for (name_stack_t::const_iterator it = ++name_stack.begin();
		it != name_stack.end();
		++it)
	{
		attribute_name += ".";
		attribute_name += it->first;
	}

	// *NOTE: <string> elements for translation need to have whitespace
	// preserved like "initial_value" above, however, the <string> node
	// becomes an attribute of the containing floater or panel.
	// Because all <string> elements must have a "name" attribute, and
	// "name" is parsed first, just put the value into the last written
	// child.
	if (attribute_name == "string.value")
	{
		// The caller of will shortly call writeStringValue(), which sets
		// this node's type to string, but we don't want to export type="string".
		// Set the default for this node to suppress the export.
		static LLXMLNodePtr default_node;
		if (default_node.isNull())
		{
			default_node = new LLXMLNode();
			// Force the node to have a string type
			default_node->setStringValue( std::string() );
		}
		mLastWrittenChild->setDefault(default_node);
		// mLastWrittenChild is the "string" node part of "string.value",
		// so the caller will call writeStringValue() into that node,
		// setting the node text contents.
		return mLastWrittenChild;
	}

	LLXMLNodePtr attribute_node;

	const char* attribute_cstr = attribute_name.c_str();
	if (name_stack.size() != 1
		&& !is_font)
	{
		std::string child_node_name(mWriteRootNode->getName()->mString);
		child_node_name += ".";
		child_node_name += name_stack.front().first;

		LLXMLNodePtr child_node;

		if (mLastWriteGeneration == name_stack.front().second)
		{
			child_node = mLastWrittenChild;
		}
		else
		{
			mLastWriteGeneration = name_stack.front().second;
			child_node = mWriteRootNode->createChild(child_node_name.c_str(), false);
		}

		mLastWrittenChild = child_node;

		name_stack_t::const_iterator it = ++name_stack.begin();
		std::string short_attribute_name(it->first);

		for (++it;
			it != name_stack.end();
			++it)
		{
			short_attribute_name += ".";
			short_attribute_name += it->first;
		}

		if (child_node->hasAttribute(short_attribute_name.c_str()))
		{
			llerrs << "Attribute " << short_attribute_name << " already exists!" << llendl;
		}

		attribute_node = child_node->createChild(short_attribute_name.c_str(), true);
	}
	else
	{
		if (mWriteRootNode->hasAttribute(attribute_cstr))
		{
			mWriteRootNode->getAttribute(attribute_cstr, attribute_node);
		}
		else
		{
			attribute_node = mWriteRootNode->createChild(attribute_name.c_str(), true);
		}
	}

	return attribute_node;
}


bool LLXUIParser::readXUIImpl(LLXMLNodePtr nodep, const std::string& scope, LLInitParam::BaseBlock& block)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	bool values_parsed = false;

	// submit attributes for current node
	values_parsed |= readAttributes(nodep, block);

	// treat text contents of xml node as "value" parameter
	std::string text_contents = nodep->getSanitizedValue();
	if (!text_contents.empty())
	{
		mCurReadNode = nodep;
		mNameStack.push_back(std::make_pair(std::string("value"), newParseGeneration()));
		// child nodes are not necessarily valid parameters (could be a child widget)
		// so don't complain once we've recursed
		bool silent = mCurReadDepth > 0;
		block.submitValue(mNameStack, *this, silent);
		mNameStack.pop_back();
	}

	// then traverse children
	// child node must start with last name of parent node (our "scope")
	// for example: "<button><button.param nested_param1="foo"><param.nested_param2 nested_param3="bar"/></button.param></button>"
	// which equates to the following nesting:
	// button
	//     param
	//         nested_param1
	//         nested_param2
	//             nested_param3	
	mCurReadDepth++;
	for(LLXMLNodePtr childp = nodep->getFirstChild(); childp.notNull();)
	{
		std::string child_name(childp->getName()->mString);
		S32 num_tokens_pushed = 0;

		// for non "dotted" child nodes	check to see if child node maps to another widget type
		// and if not, treat as a child element of the current node
		// e.g. <button><rect left="10"/></button> will interpret <rect> as "button.rect"
		// since there is no widget named "rect"
		if (child_name.find(".") == std::string::npos) 
		{
			mNameStack.push_back(std::make_pair(child_name, newParseGeneration()));
			num_tokens_pushed++;
		}
		else
		{
			// parse out "dotted" name into individual tokens
			tokenizer name_tokens(child_name, sep);

			tokenizer::iterator name_token_it = name_tokens.begin();
			if(name_token_it == name_tokens.end()) 
			{
				childp = childp->getNextSibling();
				continue;
			}

			// check for proper nesting
			if(!scope.empty() && *name_token_it != scope)
			{
				childp = childp->getNextSibling();
				continue;
			}

			// now ignore first token
			++name_token_it; 

			// copy remaining tokens on to our running token list
			for(tokenizer::iterator token_to_push = name_token_it; token_to_push != name_tokens.end(); ++token_to_push)
			{
				mNameStack.push_back(std::make_pair(*token_to_push, newParseGeneration()));
				num_tokens_pushed++;
			}
		}

		// recurse and visit children XML nodes
		if(readXUIImpl(childp, mNameStack.empty() ? scope : mNameStack.back().first, block))
		{
			// child node successfully parsed, remove from DOM

			values_parsed = true;
			LLXMLNodePtr node_to_remove = childp;
			childp = childp->getNextSibling();

			nodep->deleteChild(node_to_remove);
		}
		else
		{
			childp = childp->getNextSibling();
		}

		while(num_tokens_pushed-- > 0)
		{
			mNameStack.pop_back();
		}
	}
	mCurReadDepth--;
	return values_parsed;
}

bool LLXUIParser::readAttributes(LLXMLNodePtr nodep, LLInitParam::BaseBlock& block)
{
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".");

	bool any_parsed = false;

	for(LLXMLAttribList::const_iterator attribute_it = nodep->mAttributes.begin(); 
		attribute_it != nodep->mAttributes.end(); 
		++attribute_it)
	{
		S32 num_tokens_pushed = 0;
		std::string attribute_name(attribute_it->first->mString);
		mCurReadNode = attribute_it->second;

		tokenizer name_tokens(attribute_name, sep);
		// copy remaining tokens on to our running token list
		for(tokenizer::iterator token_to_push = name_tokens.begin(); token_to_push != name_tokens.end(); ++token_to_push)
		{
			mNameStack.push_back(std::make_pair(*token_to_push, newParseGeneration()));
			num_tokens_pushed++;
		}

		// child nodes are not necessarily valid attributes, so don't complain once we've recursed
		bool silent = mCurReadDepth > 0;
		any_parsed |= block.submitValue(mNameStack, *this, silent);
		
		while(num_tokens_pushed-- > 0)
		{
			mNameStack.pop_back();
		}
	}

	return any_parsed;
}

bool LLXUIParser::readBoolValue(void* val_ptr)
{
	S32 value;
	bool success = mCurReadNode->getBoolValue(1, &value);
	*((bool*)val_ptr) = (value != FALSE);
	return success;
}

bool LLXUIParser::writeBoolValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setBoolValue(*((bool*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readStringValue(void* val_ptr)
{
	*((std::string*)val_ptr) = mCurReadNode->getSanitizedValue();
	return true;
}

bool LLXUIParser::writeStringValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setStringValue(*((std::string*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU8Value(void* val_ptr)
{
	return mCurReadNode->getByteValue(1, (U8*)val_ptr);
}

bool LLXUIParser::writeU8Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U8*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS8Value(void* val_ptr)
{
	S32 value;
	if(mCurReadNode->getIntValue(1, &value))
	{
		*((S8*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeS8Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S8*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU16Value(void* val_ptr)
{
	U32 value;
	if(mCurReadNode->getUnsignedValue(1, &value))
	{
		*((U16*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeU16Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U16*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS16Value(void* val_ptr)
{
	S32 value;
	if(mCurReadNode->getIntValue(1, &value))
	{
		*((S16*)val_ptr) = value;
		return true;
	}
	return false;
}

bool LLXUIParser::writeS16Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S16*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readU32Value(void* val_ptr)
{
	return mCurReadNode->getUnsignedValue(1, (U32*)val_ptr);
}

bool LLXUIParser::writeU32Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setUnsignedValue(*((U32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readS32Value(void* val_ptr)
{
	return mCurReadNode->getIntValue(1, (S32*)val_ptr);
}

bool LLXUIParser::writeS32Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setIntValue(*((S32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readF32Value(void* val_ptr)
{
	return mCurReadNode->getFloatValue(1, (F32*)val_ptr);
}

bool LLXUIParser::writeF32Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setFloatValue(*((F32*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readF64Value(void* val_ptr)
{
	return mCurReadNode->getDoubleValue(1, (F64*)val_ptr);
}

bool LLXUIParser::writeF64Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setDoubleValue(*((F64*)val_ptr));
		return true;
	}
	return false;
}

bool LLXUIParser::readColor4Value(void* val_ptr)
{
	LLColor4* colorp = (LLColor4*)val_ptr;
	if(mCurReadNode->getFloatValue(4, colorp->mV) >= 3)
	{
		return true;
	}

	return false;
}

bool LLXUIParser::writeColor4Value(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		LLColor4 color = *((LLColor4*)val_ptr);
		node->setFloatValue(4, color.mV);
		return true;
	}
	return false;
}

bool LLXUIParser::readUIColorValue(void* val_ptr)
{
	LLUIColor* param = (LLUIColor*)val_ptr;
	LLColor4 color;
	bool success =  mCurReadNode->getFloatValue(4, color.mV) >= 3;
	if (success)
	{
		param->set(color);
		return true;
	}
	return false;
}

bool LLXUIParser::writeUIColorValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		LLUIColor color = *((LLUIColor*)val_ptr);
		//RN: don't write out the color that is represented by a function
		// rely on param block exporting to get the reference to the color settings
		if (color.isReference()) return false;
		node->setFloatValue(4, color.get().mV);
		return true;
	}
	return false;
}

bool LLXUIParser::readUUIDValue(void* val_ptr)
{
	LLUUID temp_id;
	// LLUUID::set is destructive, so use temporary value
	if (temp_id.set(mCurReadNode->getSanitizedValue()))
	{
		*(LLUUID*)(val_ptr) = temp_id;
		return true;
	}
	return false;
}

bool LLXUIParser::writeUUIDValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setStringValue(((LLUUID*)val_ptr)->asString());
		return true;
	}
	return false;
}

bool LLXUIParser::readSDValue(void* val_ptr)
{
	*((LLSD*)val_ptr) = LLSD(mCurReadNode->getSanitizedValue());
	return true;
}

bool LLXUIParser::writeSDValue(const void* val_ptr, const name_stack_t& stack)
{
	LLXMLNodePtr node = getNode(stack);
	if (node.notNull())
	{
		node->setStringValue(((LLSD*)val_ptr)->asString());
		return true;
	}
	return false;
}

/*virtual*/ std::string LLXUIParser::getCurrentElementName()
{
	std::string full_name;
	for (name_stack_t::iterator it = mNameStack.begin();	
		it != mNameStack.end();
		++it)
	{
		full_name += it->first + "."; // build up dotted names: "button.param.nestedparam."
	}

	return full_name;
}

void LLXUIParser::parserWarning(const std::string& message)
{
#ifdef LL_WINDOWS
	// use Visual Studo friendly formatting of output message for easy access to originating xml
	llutf16string utf16str = utf8str_to_utf16str(llformat("%s(%d):\t%s", LLUICtrlFactory::getInstance()->getCurFileName().c_str(), mCurReadNode->getLineNumber(), message.c_str()).c_str());
	utf16str += '\n';
	OutputDebugString(utf16str.c_str());
#else
	Parser::parserWarning(message);
#endif
}

void LLXUIParser::parserError(const std::string& message)
{
#ifdef LL_WINDOWS
	llutf16string utf16str = utf8str_to_utf16str(llformat("%s(%d):\t%s", LLUICtrlFactory::getInstance()->getCurFileName().c_str(), mCurReadNode->getLineNumber(), message.c_str()).c_str());
	utf16str += '\n';
	OutputDebugString(utf16str.c_str());
#else
	Parser::parserError(message);
#endif
}
