/** 
 * @file lluictrlfactory.cpp
 * @brief Factory class for creating UI controls
 *
 * $LicenseInfo:firstyear=2003&license=viewergpl$
 * 
 * Copyright (c) 2003-2007, Linden Research, Inc.
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

#include "linden_common.h"

#include "lluictrlfactory.h"

#include <fstream>
#include <boost/tokenizer.hpp>

// other library includes
#include "llcontrol.h"
#include "lldir.h"
#include "v4color.h"

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
#include "llspinctrl.h"
#include "lltabcontainer.h"
#include "lltabcontainervertical.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "llui.h"
#include "llviewborder.h"


const char XML_HEADER[] = "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\" ?>\n";

// *NOTE: If you add a new class derived from LLPanel, add a check for its
// widget type to LLUICtrl::getParentPanel().
// *NOTE: This MUST match EWidgetType in llui.h
//static
const LLString LLUICtrlFactory::sUICtrlNames[WIDGET_TYPE_COUNT] = 
{
	LLString("view"),			//WIDGET_TYPE_VIEW
	LLString("root_view"),		//WIDGET_TYPE_ROOT_VIEW
	LLString("floater_view"),	//WIDGET_TYPE_FLOATER_VIEW
	LLString("button"),		//WIDGET_TYPE_BUTTON
	LLString("joystick_turn"),	//WIDGET_TYPE_JOYSTICK_TURN
	LLString("joystick_slide"),	//WIDGET_TYPE_JOYSTICK_SLIDE
	LLString("check_box"),	//WIDGET_TYPE_CHECKBOX
	LLString("color_swatch"),	//WIDGET_TYPE_COLOR_SWATCH
	LLString("combo_box"),	//WIDGET_TYPE_COMBO_BOX
	LLString("line_editor"),	//WIDGET_TYPE_LINE_EDITOR
	LLString("search_editor"),	//WIDGET_TYPE_SEARCH_EDITOR
	LLString("scroll_list"),	//WIDGET_TYPE_SCROLL_LIST
	LLString("name_list"),	//WIDGET_TYPE_NAME_LIST
	LLString("web_browser"),	//WIDGET_TYPE_WEBBROWSER
	LLString("slider"),		//WIDGET_TYPE_SLIDER, actually LLSliderCtrl
	LLString("slider_bar"), //WIDGET_TYPE_SLIDER_BAR, actually LLSlider
	LLString("volume_slider"),	//WIDGET_TYPE_VOLUME_SLIDER, actually LLSlider + "volume" param
	LLString("spinner"),		//WIDGET_TYPE_SPINNER, actually LLSpinCtrl
	LLString("text_editor"),	//WIDGET_TYPE_TEXT_EDITOR
	LLString("texture_picker"),//WIDGET_TYPE_TEXTURE_PICKER
	LLString("text"),			//WIDGET_TYPE_TEXT_BOX
	LLString("pad"),			//WIDGET_TYPE_PAD
	LLString("radio_group"),	//WIDGET_TYPE_RADIO_GROUP
	LLString("icon"),			//WIDGET_TYPE_ICON
	LLString("locate"),		//WIDGET_TYPE_LOCATE
	LLString("view_border"),	//WIDGET_TYPE_VIEW_BORDER
	LLString("panel"),		//WIDGET_TYPE_PANEL
	LLString("menu"),		//WIDGET_TYPE_MENU
	LLString("pie_menu"),	//WIDGET_TYPE_PIE_MENU
	LLString("pie_menu_branch"), //WIDGET_TYPE_PIE_MENU_BRANCH
	LLString("menu_item"), //WIDGET_TYPE_MENU_ITEM
	LLString("menu_item_separator"), //WIDGET_TYPE_MENU_ITEM_SEPARATOR
	LLString("menu_separator_vertical"), //	WIDGET_TYPE_MENU_SEPARATOR_VERTICAL
	LLString("menu_item_call"), // WIDGET_TYPE_MENU_ITEM_CALL
	LLString("menu_item_check"),// WIDGET_TYPE_MENU_ITEM_CHECK
	LLString("menu_item_branch"), // WIDGET_TYPE_MENU_ITEM_BRANCH
	LLString("menu_item_branch_down"), //WIDGET_TYPE_MENU_ITEM_BRANCH_DOWN,
	LLString("menu_item_blank"), //WIDGET_TYPE_MENU_ITEM_BLANK,
	LLString("tearoff_menu"), //WIDGET_TYPE_TEAROFF_MENU
	LLString("menu_bar"),	//WIDGET_TYPE_MENU_BAR
	LLString("tab_container"),//WIDGET_TYPE_TAB_CONTAINER
	LLString("scroll_container"),//WIDGET_TYPE_SCROLL_CONTAINER
	LLString("scrollbar"),	//WIDGET_TYPE_SCROLLBAR
	LLString("inventory_panel"), //WIDGET_TYPE_INVENTORY_PANEL
	LLString("floater"),		//WIDGET_TYPE_FLOATER
	LLString("drag_handle_top"), //WIDGET_TYPE_DRAG_HANDLE_TOP
	LLString("drag_handle_left"), //WIDGET_TYPE_DRAG_HANDLE_LEFT
	LLString("resize_handle"), //WIDGET_TYPE_RESIZE_HANDLE
	LLString("resize_bar"), //WIDGET_TYPE_RESIZE_BAR
	LLString("name_editor"),	//WIDGET_TYPE_NAME_EDITOR
	LLString("multi_floater"),	//WIDGET_TYPE_MULTI_FLOATER
	LLString("media_remote"), //WIDGET_TYPE_MEDIA_REMOTE
	LLString("folder_view"), //WIDGET_TYPE_FOLDER_VIEW
	LLString("folder_item"), //WIDGET_TYPE_FOLDER_ITEM
	LLString("folder"), //WIDGET_TYPE_FOLDER
	LLString("stat_graph"), //WIDGET_TYPE_STAT_GRAPH
	LLString("stat_view"), //WIDGET_TYPE_STAT_VIEW
	LLString("stat_bar"), //WIDGET_TYPE_STAT_BAR
	LLString("drop_target"), //WIDGET_TYPE_DROP_TARGET
	LLString("texture_bar"), //WIDGET_TYPE_TEXTURE_BAR
	LLString("tex_mem_bar"), //WIDGET_TYPE_TEX_MEM_BAR
	LLString("snapshot_live_preview"), //WIDGET_TYPE_SNAPSHOT_LIVE_PREVIEW
	LLString("status_bar"), //WIDGET_TYPE_STATUS_BAR
	LLString("progress_view"), //WIDGET_TYPE_PROGRESS_VIEW
	LLString("talk_view"), //WIDGET_TYPE_TALK_VIEW
	LLString("overlay_bar"), //WIDGET_TYPE_OVERLAY_BAR
	LLString("hud_view"), //WIDGET_TYPE_HUD_VIEW
	LLString("hover_view"), //WIDGET_TYPE_HOVER_VIEW
	LLString("morph_view"), //WIDGET_TYPE_MORPH_VIEW
	LLString("net_map"), //WIDGET_TYPE_NET_MAP
	LLString("permissions_view"), //WIDGET_TYPE_PERMISSIONS_VIEW
	LLString("menu_holder"), //WIDGET_TYPE_MENU_HOLDER
	LLString("debug_view"), //WIDGET_TYPE_DEBUG_VIEW
	LLString("scrolling_panel_list"), //WIDGET_TYPE_SCROLLING_PANEL_LIST
	LLString("audio_status"), //WIDGET_TYPE_AUDIO_STATUS
	LLString("container_view"), //WIDGET_TYPE_CONTAINER_VIEW
	LLString("console"), //WIDGET_TYPE_CONSOLE
	LLString("fast_timer_view"), //WIDGET_TYPE_FAST_TIMER_VIEW
	LLString("velocity_bar"), //WIDGET_TYPE_VELOCITY_BAR
	LLString("texture_view"), //WIDGET_TYPE_TEXTURE_VIEW
	LLString("memory_view"), //WIDGET_TYPE_MEMORY_VIEW
	LLString("frame_stat_view"), //WIDGET_TYPE_FRAME_STAT_VIEW
	LLString("layout_stack"), //WIDGET_TYPE_LAYOUT_STACK
	LLString("DONT_CARE"),	//WIDGET_TYPE_DONTCARE
};

const S32 HPAD = 4;
const S32 VPAD = 4;
const S32 FLOATER_H_MARGIN = 15;
const S32 MIN_WIDGET_HEIGHT = 10;

std::vector<LLString> LLUICtrlFactory::mXUIPaths;

// UI Ctrl class for padding
class LLUICtrlLocate : public LLUICtrl
{
public:
	LLUICtrlLocate() : LLUICtrl("locate", LLRect(0,0,0,0), FALSE, NULL, NULL) { setTabStop(FALSE); }
	virtual void draw() { }

	virtual EWidgetType getWidgetType() const { return WIDGET_TYPE_LOCATE; }
	virtual LLString getWidgetTag() const { return LL_UI_CTRL_LOCATE_TAG; }

	static LLView *fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
	{
		LLString name("pad");
		node->getAttributeString("name", name);

		LLUICtrlLocate *new_ctrl = new LLUICtrlLocate();
		new_ctrl->setName(name);
		new_ctrl->initFromXML(node, parent);
		return new_ctrl;
	}
};

//-----------------------------------------------------------------------------
// LLUICtrlFactory()
//-----------------------------------------------------------------------------
LLUICtrlFactory::LLUICtrlFactory()
{
	// Register controls
	LLUICtrlCreator<LLButton>::registerCreator(LL_BUTTON_TAG, this);
	LLUICtrlCreator<LLCheckBoxCtrl>::registerCreator(LL_CHECK_BOX_CTRL_TAG, this);
	LLUICtrlCreator<LLComboBox>::registerCreator(LL_COMBO_BOX_TAG, this);
	LLUICtrlCreator<LLFlyoutButton>::registerCreator(LL_FLYOUT_BUTTON_TAG, this);
	LLUICtrlCreator<LLLineEditor>::registerCreator(LL_LINE_EDITOR_TAG, this);
	LLUICtrlCreator<LLSearchEditor>::registerCreator(LL_SEARCH_EDITOR_TAG, this);
	LLUICtrlCreator<LLScrollListCtrl>::registerCreator(LL_SCROLL_LIST_CTRL_TAG, this);
	LLUICtrlCreator<LLSliderCtrl>::registerCreator(LL_SLIDER_CTRL_TAG, this);
	LLUICtrlCreator<LLSlider>::registerCreator(LL_SLIDER_TAG, this);
	LLUICtrlCreator<LLSlider>::registerCreator(LL_VOLUME_SLIDER_CTRL_TAG, this);
	LLUICtrlCreator<LLSpinCtrl>::registerCreator(LL_SPIN_CTRL_TAG, this);
	LLUICtrlCreator<LLTextBox>::registerCreator(LL_TEXT_BOX_TAG, this);
	LLUICtrlCreator<LLRadioGroup>::registerCreator(LL_RADIO_GROUP_TAG, this);
	LLUICtrlCreator<LLIconCtrl>::registerCreator(LL_ICON_CTRL_TAG, this);
	LLUICtrlCreator<LLUICtrlLocate>::registerCreator(LL_UI_CTRL_LOCATE_TAG, this);
	LLUICtrlCreator<LLUICtrlLocate>::registerCreator(LL_PAD_TAG, this);
	LLUICtrlCreator<LLViewBorder>::registerCreator(LL_VIEW_BORDER_TAG, this);
	LLUICtrlCreator<LLTabContainerCommon>::registerCreator(LL_TAB_CONTAINER_COMMON_TAG, this);
	LLUICtrlCreator<LLScrollableContainerView>::registerCreator(LL_SCROLLABLE_CONTAINER_VIEW_TAG, this);
	LLUICtrlCreator<LLPanel>::registerCreator(LL_PANEL_TAG, this);
	LLUICtrlCreator<LLMenuGL>::registerCreator(LL_MENU_GL_TAG, this);
	LLUICtrlCreator<LLMenuBarGL>::registerCreator(LL_MENU_BAR_GL_TAG, this);
	LLUICtrlCreator<LLScrollingPanelList>::registerCreator(LL_SCROLLING_PANEL_LIST_TAG, this);
	LLUICtrlCreator<LLLayoutStack>::registerCreator(LL_LAYOUT_STACK_TAG, this);

	setupPaths();

}

void LLUICtrlFactory::setupPaths()
{
	LLString filename = gDirUtilp->getExpandedFilename(LL_PATH_SKINS, "paths.xml");

	LLXMLNodePtr root;
	BOOL success  = LLXMLNode::parseFile(filename, root, NULL);
	mXUIPaths.clear();
	
	if (!success)
	{
		LLString slash = gDirUtilp->getDirDelimiter();
		LLString dir = gDirUtilp->getAppRODataDir() + slash + "skins" + slash + "xui" + slash + "en-us" + slash;
		llwarns << "XUI::config file unable to open." << llendl;
		mXUIPaths.push_back(dir);
	}
	else
	{
		LLXMLNodePtr path;
		LLString app_dir = gDirUtilp->getAppRODataDir();
	
		for (path = root->getFirstChild(); path.notNull(); path = path->getNextSibling())
		{
			LLUIString path_val_ui(path->getValue());
			LLString language = "en-us";
			if (LLUI::sConfigGroup)
			{
				language = LLUI::sConfigGroup->getString("Language");
				if(language == "default")
				{
					language = LLUI::sConfigGroup->getString("SystemLanguage");
				}
			}
			path_val_ui.setArg("[Language]", language);
			LLString fullpath = app_dir + path_val_ui.getString();

			if (std::find(mXUIPaths.begin(), mXUIPaths.end(), fullpath) == mXUIPaths.end())
			{
				mXUIPaths.push_back(app_dir + path_val_ui.getString());
			}
		}
	}


}

//-----------------------------------------------------------------------------
// ~LLUICtrlFactory()
//-----------------------------------------------------------------------------
LLUICtrlFactory::~LLUICtrlFactory()
{
}


//-----------------------------------------------------------------------------
// getLayeredXMLNode()
//-----------------------------------------------------------------------------
bool LLUICtrlFactory::getLayeredXMLNode(const LLString &filename, LLXMLNodePtr& root)
{
	
	if (!LLXMLNode::parseFile(mXUIPaths.front() + filename, root, NULL))
	{	
		if (!LLXMLNode::parseFile(filename, root, NULL))
		{
			llwarns << "Problem reading UI description file: " << mXUIPaths.front() + filename << llendl;
			return FALSE;
		}
	}

	LLXMLNodePtr updateRoot;

	std::vector<LLString>::const_iterator itor;

	for (itor = mXUIPaths.begin(), ++itor; itor != mXUIPaths.end(); ++itor)
	{
		LLString nodeName;
		LLString updateName;

		LLXMLNode::parseFile((*itor) + filename, updateRoot, NULL);
	
		updateRoot->getAttributeString("name", updateName);
		root->getAttributeString("name", nodeName);

		if (updateName == nodeName)
		{
			LLXMLNode::updateNode(root, updateRoot);
		}
	}

	return TRUE;
}


//-----------------------------------------------------------------------------
// buildFloater()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::buildFloater(LLFloater* floaterp, const LLString &filename, 
									const LLCallbackMap::map_t* factory_map, BOOL open) /* Flawfinder: ignore */
{
	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return;
	}
	
	// root must be called floater
	if( !(root->hasName("floater") || root->hasName("multi_floater") ) )
	{
		llwarns << "Root node should be named floater in: " << filename << llendl;
		return;
	}

	if (factory_map)
	{
		mFactoryStack.push_front(factory_map);
	}

	floaterp->initFloaterXML(root, NULL, this, open);	/* Flawfinder: ignore */

	if (LLUI::sShowXUINames)
	{
		floaterp->mToolTipMsg = filename;
	}

	if (factory_map)
	{
		mFactoryStack.pop_front();
	}

	LLViewHandle handle = floaterp->getHandle();
	mBuiltFloaters[handle] = filename;
}

//-----------------------------------------------------------------------------
// saveToXML()
//-----------------------------------------------------------------------------
S32 LLUICtrlFactory::saveToXML(LLView* viewp, const LLString& filename)
{
	llofstream out(filename.c_str());
	if (!out.good())
	{
		llwarns << "Unable to open " << filename << " for output." << llendl;
		return 1;
	}

	out << XML_HEADER;

	LLXMLNodePtr xml_node = viewp->getXML();

	xml_node->writeToOstream(out);

	out.close();
	return 0;
}

//-----------------------------------------------------------------------------
// buildPanel()
//-----------------------------------------------------------------------------
BOOL LLUICtrlFactory::buildPanel(LLPanel* panelp, const LLString &filename,
									const LLCallbackMap::map_t* factory_map)
{
	BOOL didPost = FALSE;
	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return didPost;
	}

	// root must be called panel
	if( !root->hasName("panel" ) )
	{
		llwarns << "Root node should be named panel in : " << filename << llendl;
		return didPost;
	}

	if (factory_map)
	{
		mFactoryStack.push_front(factory_map);
	}

	didPost = panelp->initPanelXML(root, NULL, this);
	
	if (LLUI::sShowXUINames)
	{
		panelp->mToolTipMsg = filename;
	}

	LLViewHandle handle = panelp->getHandle();
	mBuiltPanels[handle] = filename;

	if (factory_map)
	{
		mFactoryStack.pop_front();
	}

	return didPost;
}

//-----------------------------------------------------------------------------
// buildMenu()
//-----------------------------------------------------------------------------
LLMenuGL *LLUICtrlFactory::buildMenu(const LLString &filename, LLView* parentp)
{
	// TomY TODO: Break this function into buildMenu and buildMenuBar
	LLXMLNodePtr root;
	LLMenuGL*    menu;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return NULL;
	}

	// root must be called panel
	if( !root->hasName( "menu_bar" ) && !root->hasName( "menu" ))
	{
		llwarns << "Root node should be named menu bar or menu in : " << filename << llendl;
		return NULL;
	}

	

	if (root->hasName("menu"))
	{
		menu = (LLMenuGL*)LLMenuGL::fromXML(root, parentp, this);
	}
	else
	{
		menu = (LLMenuGL*)LLMenuBarGL::fromXML(root, parentp, this);
	}
	
	if (LLUI::sShowXUINames)
	{
		menu->mToolTipMsg = filename;
	}

    return menu;
}

//-----------------------------------------------------------------------------
// buildMenu()
//-----------------------------------------------------------------------------
LLPieMenu *LLUICtrlFactory::buildPieMenu(const LLString &filename, LLView* parentp)
{

	LLXMLNodePtr root;

	if (!LLUICtrlFactory::getLayeredXMLNode(filename, root))
	{
		return NULL;
	}

	// root must be called panel
	if( !root->hasName( LL_PIE_MENU_TAG ))
	{
		llwarns << "Root node should be named " << LL_PIE_MENU_TAG << " in : " << filename << llendl;
		return NULL;
	}

	LLString name("menu");
	root->getAttributeString("name", name);

	LLPieMenu *menu = new LLPieMenu(name);
	parentp->addChild(menu);
	menu->initXML(root, parentp, this);

	if (LLUI::sShowXUINames)
	{
		menu->mToolTipMsg = filename;
	}

	return menu;
}

//-----------------------------------------------------------------------------
// removePanel()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::removePanel(LLPanel* panelp)
{
	mBuiltPanels.erase(panelp->getHandle());
}

//-----------------------------------------------------------------------------
// removeFloater()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::removeFloater(LLFloater* floaterp)
{
	mBuiltFloaters.erase(floaterp->getHandle());
}

//-----------------------------------------------------------------------------
// rebuild()
//-----------------------------------------------------------------------------
void LLUICtrlFactory::rebuild()
{
	built_panel_t::iterator built_panel_it;
	for (built_panel_it = mBuiltPanels.begin();
		built_panel_it != mBuiltPanels.end();
		++built_panel_it)
		{
			LLString filename = built_panel_it->second;
			LLPanel* panelp = LLPanel::getPanelByHandle(built_panel_it->first);
			if (!panelp)
			{
				continue;
			}
			llinfos << "Rebuilding UI panel " << panelp->getName() 
				<< " from " << filename
				<< llendl;
			BOOL visible = panelp->getVisible();
			panelp->setVisible(FALSE);
			panelp->setFocus(FALSE);
			panelp->deleteAllChildren();

			buildPanel(panelp, filename.c_str(), &panelp->getFactoryMap());
			panelp->setVisible(visible);
		}

	built_floater_t::iterator built_floater_it;
	for (built_floater_it = mBuiltFloaters.begin();
		built_floater_it != mBuiltFloaters.end();
		++built_floater_it)
		{
			LLFloater* floaterp = LLFloater::getFloaterByHandle(built_floater_it->first);
			if (!floaterp)
			{
				continue;
			}
			LLString filename = built_floater_it->second;
			llinfos << "Rebuilding UI floater " << floaterp->getName()
				<< " from " << filename
				<< llendl;
			BOOL visible = floaterp->getVisible();
			floaterp->setVisible(FALSE);
			floaterp->setFocus(FALSE);
			floaterp->deleteAllChildren();

			gFloaterView->removeChild(floaterp);
			buildFloater(floaterp, filename, &floaterp->getFactoryMap());
			floaterp->setVisible(visible);
		}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// static
EWidgetType LLUICtrlFactory::getWidgetType(const LLString& ctrl_type)
{
	U32 ctrl_id;
	for (ctrl_id = 0; ctrl_id < WIDGET_TYPE_COUNT; ctrl_id++)
	{
		if (sUICtrlNames[ctrl_id] == ctrl_type)
		{
			break;
		}
	}
	return (EWidgetType) ctrl_id;
}

LLString LLUICtrlFactory::getWidgetType(EWidgetType ctrl_type)
{
	return sUICtrlNames[ctrl_type];
}

LLView *LLUICtrlFactory::createCtrlWidget(LLPanel *parent, LLXMLNodePtr node)
{
	LLString ctrl_type = node->getName()->mString;
	LLString::toLower(ctrl_type);
	
	creator_list_t::const_iterator it = mCreatorFunctions.find(ctrl_type);
	if (it == mCreatorFunctions.end())
	{
		llwarns << "Unknown control type " << ctrl_type << llendl;
		return NULL;
	}

	LLView *ctrl = (*it->second)(node, parent, this);

	return ctrl;
}

void LLUICtrlFactory::createWidget(LLPanel *parent, LLXMLNodePtr node)
{
	LLView* view = createCtrlWidget(parent, node);

	S32 tab_group = parent->getLastTabGroup();
	node->getAttributeS32("tab_group", tab_group);

	if (view)
	{
		parent->addChild(view, tab_group);
	}
}

//-----------------------------------------------------------------------------
// createFactoryPanel()
//-----------------------------------------------------------------------------
LLPanel* LLUICtrlFactory::createFactoryPanel(LLString name)
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
	return NULL;
}

//-----------------------------------------------------------------------------

//static
BOOL LLUICtrlFactory::getAttributeColor(LLXMLNodePtr node, const LLString& name, LLColor4& color)
{
	LLString colorstring;
	BOOL res = node->getAttributeString(name, colorstring);
	if (res && LLUI::sColorsGroup)
	{
		if (LLUI::sColorsGroup->controlExists(colorstring))
		{
			color.setVec(LLUI::sColorsGroup->getColor(colorstring));
		}
		else
		{
			res = FALSE;
		}
	}
	if (!res)
	{
		res = LLColor4::parseColor(colorstring.c_str(), &color);
	}	
	if (!res)
	{
		res = node->getAttributeColor(name, color);
	}
	return res;
}

//============================================================================

LLButton*	LLUICtrlFactory::getButtonByName(LLPanel* panelp, const LLString& name)
{ 
	return (LLButton*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_BUTTON); 
}

LLCheckBoxCtrl*		LLUICtrlFactory::getCheckBoxByName(LLPanel* panelp, const LLString& name)		
{ 
	return (LLCheckBoxCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_CHECKBOX); 
}

LLComboBox*			LLUICtrlFactory::getComboBoxByName(LLPanel* panelp, const LLString& name)		
{ 
	return (LLComboBox*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_COMBO_BOX); 
}

LLIconCtrl*			LLUICtrlFactory::getIconByName(LLPanel* panelp, const LLString& name)			
{ 
	return (LLIconCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_ICON); 
}

LLLineEditor*		LLUICtrlFactory::getLineEditorByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLLineEditor*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_LINE_EDITOR); 
}

LLNameListCtrl*	LLUICtrlFactory::getNameListByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLNameListCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_NAME_LIST); 
}

LLRadioGroup*		LLUICtrlFactory::getRadioGroupByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLRadioGroup*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_RADIO_GROUP); 
}

LLScrollListCtrl*	LLUICtrlFactory::getScrollListByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLScrollListCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_SCROLL_LIST); 
}

LLSliderCtrl*		LLUICtrlFactory::getSliderByName(LLPanel* panelp, const LLString& name)		
{ 
	return (LLSliderCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_SLIDER); 
}

LLSlider*			LLUICtrlFactory::getSliderBarByName(LLPanel* panelp, const LLString& name)		
{ 
	return (LLSlider*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_SLIDER_BAR); 
}

LLSpinCtrl*			LLUICtrlFactory::getSpinnerByName(LLPanel* panelp, const LLString& name)		
{ 
	return (LLSpinCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_SPINNER); 
}

LLTextBox*			LLUICtrlFactory::getTextBoxByName(LLPanel* panelp, const LLString& name)		
{ 
	return (LLTextBox*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_TEXT_BOX);
}

LLTextEditor*		LLUICtrlFactory::getTextEditorByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLTextEditor*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_TEXT_EDITOR); 
}

LLTabContainerCommon* LLUICtrlFactory::getTabContainerByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLTabContainerCommon*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_TAB_CONTAINER); 
}

LLScrollableContainerView* LLUICtrlFactory::getScrollableContainerByName(LLPanel* panelp, const LLString& name)
{
	return (LLScrollableContainerView*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_SCROLL_CONTAINER); 
}

LLTextureCtrl* LLUICtrlFactory::getTexturePickerByName(LLPanel* panelp, const LLString& name)
{
	return (LLTextureCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_TEXTURE_PICKER);
}

LLPanel* LLUICtrlFactory::getPanelByName(LLPanel* panelp, const LLString& name)	
{ 
	return (LLPanel*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_PANEL); 
}

LLColorSwatchCtrl* LLUICtrlFactory::getColorSwatchByName(LLPanel* panelp, const LLString& name)
{ 
	return (LLColorSwatchCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_COLOR_SWATCH); 
}

LLWebBrowserCtrl* LLUICtrlFactory::getWebBrowserCtrlByName(LLPanel* panelp, const LLString& name)
{ 
	return (LLWebBrowserCtrl*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_WEBBROWSER); 
}

LLMenuItemCallGL* LLUICtrlFactory::getMenuItemCallByName(LLPanel* panelp, const LLString& name)
{ 
	return (LLMenuItemCallGL*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_MENU_ITEM_CALL); 
}

LLScrollingPanelList* LLUICtrlFactory::getScrollingPanelList(LLPanel* panelp, const LLString& name)
{ 
	return (LLScrollingPanelList*)panelp->getCtrlByNameAndType(name, WIDGET_TYPE_SCROLLING_PANEL_LIST); 
}


LLCtrlListInterface* LLUICtrlFactory::getListInterfaceByName(LLPanel* panelp, const LLString& name)
{
	LLView* viewp = panelp->getCtrlByNameAndType(name, WIDGET_TYPE_DONTCARE);
	if (viewp && viewp->isCtrl())
	{
		return ((LLUICtrl*)viewp)->getListInterface();
	}
	return NULL;
}

LLCtrlSelectionInterface* LLUICtrlFactory::getSelectionInterfaceByName(LLPanel* panelp, const LLString& name)
{
	LLView* viewp = panelp->getCtrlByNameAndType(name, WIDGET_TYPE_DONTCARE);
	if (viewp && viewp->isCtrl())
	{
		return ((LLUICtrl*)viewp)->getSelectionInterface();
	}
	return NULL;
}

LLCtrlScrollInterface* LLUICtrlFactory::getScrollInterfaceByName(LLPanel* panelp, const LLString& name)
{
	LLView* viewp = panelp->getCtrlByNameAndType(name, WIDGET_TYPE_DONTCARE);
	if (viewp && viewp->isCtrl())
	{
		return ((LLUICtrl*)viewp)->getScrollInterface();
	}
	return NULL;
}

void LLUICtrlFactory::registerCreator(LLString ctrlname, creator_function_t function)
{
	LLString::toLower(ctrlname);
	mCreatorFunctions[ctrlname] = function;
}


