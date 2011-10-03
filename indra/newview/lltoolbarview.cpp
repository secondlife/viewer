/** 
 * @file lltoolbarview.cpp
 * @author Merov Linden
 * @brief User customizable toolbar class
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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

#include "linden_common.h"

#include "lltoolbarview.h"

#include "lldir.h"
#include "llxmlnode.h"
#include "lltoolbar.h"
#include "llbutton.h"
#include "lltooldraganddrop.h"
#include "llclipboard.h"

#include <boost/foreach.hpp>

LLToolBarView* gToolBarView = NULL;

static LLDefaultChildRegistry::Register<LLToolBarView> r("toolbar_view");
bool LLToolBarView::sDragStarted = false;

LLToolBarView::Toolbar::Toolbar()
:	button_display_mode("button_display_mode"),
	commands("command")
{}

LLToolBarView::ToolbarSet::ToolbarSet()
:	left_toolbar("left_toolbar"),
	right_toolbar("right_toolbar"),
	bottom_toolbar("bottom_toolbar")
{}


LLToolBarView::LLToolBarView(const LLToolBarView::Params& p)
:	LLUICtrl(p),
	mToolbarLeft(NULL),
	mToolbarRight(NULL),
	mToolbarBottom(NULL)
{
}

void LLToolBarView::initFromParams(const LLToolBarView::Params& p)
{
	// Initialize the base object
	LLUICtrl::initFromParams(p);
}

LLToolBarView::~LLToolBarView()
{
	saveToolbars();
}

BOOL LLToolBarView::postBuild()
{
	mToolbarLeft   = getChild<LLToolBar>("toolbar_left");
	mToolbarRight  = getChild<LLToolBar>("toolbar_right");
	mToolbarBottom = getChild<LLToolBar>("toolbar_bottom");

	mToolbarLeft->setStartDragCallback(boost::bind(LLToolBarView::startDragItem,_1,_2,_3));
	mToolbarLeft->setHandleDragCallback(boost::bind(LLToolBarView::handleDragItem,_1,_2,_3,_4));
	mToolbarLeft->setHandleDropCallback(boost::bind(LLToolBarView::handleDrop,_1,_2,_3));
	
	mToolbarRight->setStartDragCallback(boost::bind(LLToolBarView::startDragItem,_1,_2,_3));
	mToolbarRight->setHandleDragCallback(boost::bind(LLToolBarView::handleDragItem,_1,_2,_3,_4));
	mToolbarRight->setHandleDropCallback(boost::bind(LLToolBarView::handleDrop,_1,_2,_3));
	
	mToolbarBottom->setStartDragCallback(boost::bind(LLToolBarView::startDragItem,_1,_2,_3));
	mToolbarBottom->setHandleDragCallback(boost::bind(LLToolBarView::handleDragItem,_1,_2,_3,_4));
	mToolbarBottom->setHandleDropCallback(boost::bind(LLToolBarView::handleDrop,_1,_2,_3));
	
	return TRUE;
}

bool LLToolBarView::hasCommand(const LLCommandId& commandId) const
{
	bool has_command = false;
	if (mToolbarLeft && !has_command)
	{
		has_command = mToolbarLeft->hasCommand(commandId);
	}
	if (mToolbarRight && !has_command)
	{
		has_command = mToolbarRight->hasCommand(commandId);
	}
	if (mToolbarBottom && !has_command)
	{
		has_command = mToolbarBottom->hasCommand(commandId);
	}
	return has_command;
}

bool LLToolBarView::addCommand(const LLCommandId& command, LLToolBar* toolbar)
{
	LLCommandManager& mgr = LLCommandManager::instance();
	if (mgr.getCommand(command))
	{
		toolbar->addCommand(command);
	}
	else 
	{
		llwarns	<< "Toolbars creation : the command " << command.name() << " cannot be found in the command manager" << llendl;
		return false;
	}
	return true;
}

bool LLToolBarView::loadToolbars(bool force_default)
{	
	LLToolBarView::ToolbarSet toolbar_set;
	
	// Load the toolbars.xml file
	std::string toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "toolbars.xml");
	if (force_default)
	{
		toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "toolbars.xml");
	}
	else if (!gDirUtilp->fileExists(toolbar_file)) 
	{
		llwarns << "User toolbars def not found -> use default" << llendl;
		toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "toolbars.xml");
	}
	
	LLXMLNodePtr root;
	if(!LLXMLNode::parseFile(toolbar_file, root, NULL))
	{
		llerrs << "Unable to load toolbars from file: " << toolbar_file << llendl;
		return false;
	}
	if(!root->hasName("toolbars"))
	{
		llwarns << toolbar_file << " is not a valid toolbars definition file" << llendl;
		return false;
	}
	
	// Parse the toolbar settings
	LLXUIParser parser;
	parser.readXUI(root, toolbar_set, toolbar_file);
	if (!toolbar_set.validateBlock())
	{
		llerrs << "Unable to validate toolbars from file: " << toolbar_file << llendl;
		return false;
	}
	
	// Clear the toolbars now before adding the loaded commands and settings
	if (mToolbarLeft)
	{
		mToolbarLeft->clearCommandsList();
	}
	if (mToolbarRight)
	{
		mToolbarRight->clearCommandsList();
	}
	if (mToolbarBottom)
	{
		mToolbarBottom->clearCommandsList();
	}
	
	// Add commands to each toolbar
	if (toolbar_set.left_toolbar.isProvided() && mToolbarLeft)
	{
		if (toolbar_set.left_toolbar.button_display_mode.isProvided())
		{
			U32 button_type = toolbar_set.left_toolbar.button_display_mode;
			mToolbarLeft->setButtonType((LLToolBarEnums::ButtonType)(button_type));
		}
		BOOST_FOREACH(LLCommandId::Params& command, toolbar_set.left_toolbar.commands)
		{
			addCommand(LLCommandId(command),mToolbarLeft);
		}
	}
	if (toolbar_set.right_toolbar.isProvided() && mToolbarRight)
	{
		if (toolbar_set.right_toolbar.button_display_mode.isProvided())
		{
			U32 button_type = toolbar_set.right_toolbar.button_display_mode;
			mToolbarRight->setButtonType((LLToolBarEnums::ButtonType)(button_type));
		}
		BOOST_FOREACH(LLCommandId::Params& command, toolbar_set.right_toolbar.commands)
		{
			addCommand(LLCommandId(command),mToolbarRight);
		}
	}
	if (toolbar_set.bottom_toolbar.isProvided() && mToolbarBottom)
	{
		if (toolbar_set.bottom_toolbar.button_display_mode.isProvided())
		{
			U32 button_type = toolbar_set.bottom_toolbar.button_display_mode;
			mToolbarBottom->setButtonType((LLToolBarEnums::ButtonType)(button_type));
		}
		BOOST_FOREACH(LLCommandId::Params& command, toolbar_set.bottom_toolbar.commands)
		{
			addCommand(LLCommandId(command),mToolbarBottom);
		}
	}
	return true;
}

void LLToolBarView::saveToolbars() const
{
	// Build the parameter tree from the toolbar data
	LLToolBarView::ToolbarSet toolbar_set;
	if (mToolbarLeft)
	{
		toolbar_set.left_toolbar.button_display_mode = (int)(mToolbarLeft->getButtonType());
		addToToolset(mToolbarLeft->getCommandsList(),toolbar_set.left_toolbar);
	}
	if (mToolbarRight)
	{
		toolbar_set.right_toolbar.button_display_mode = (int)(mToolbarRight->getButtonType());
		addToToolset(mToolbarRight->getCommandsList(),toolbar_set.right_toolbar);
	}
	if (mToolbarBottom)
	{
		toolbar_set.bottom_toolbar.button_display_mode = (int)(mToolbarBottom->getButtonType());
		addToToolset(mToolbarBottom->getCommandsList(),toolbar_set.bottom_toolbar);
	}
	
	// Serialize the parameter tree
	LLXMLNodePtr output_node = new LLXMLNode("toolbars", false);
	LLXUIParser parser;
	parser.writeXUI(output_node, toolbar_set);
	
	// Write the resulting XML to file
	if(!output_node->isNull())
	{
		const std::string& filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "toolbars.xml");
		LLFILE *fp = LLFile::fopen(filename, "w");
		if (fp != NULL)
		{
			LLXMLNode::writeHeaderToFile(fp);
			output_node->writeToFile(fp);
			fclose(fp);
		}
	}
}

// Enumerate the commands in command_list and add them as Params to the toolbar
void LLToolBarView::addToToolset(command_id_list_t& command_list, Toolbar& toolbar) const
{
	for (command_id_list_t::const_iterator it = command_list.begin();
		 it != command_list.end();
		 ++it)
	{
		LLCommandId::Params command;
		command.name = it->name();		
		toolbar.commands.add(command);
	}
}

void LLToolBarView::draw()
{
	static bool debug_print = true;
	static S32 old_width = 0;
	static S32 old_height = 0;

	//LLPanel* sizer_left = getChild<LLPanel>("sizer_left");
	
	LLRect bottom_rect, left_rect, right_rect;

	if (mToolbarBottom) 
	{
		mToolbarBottom->getParent()->reshape(mToolbarBottom->getParent()->getRect().getWidth(), mToolbarBottom->getRect().getHeight());
		mToolbarBottom->localRectToOtherView(mToolbarBottom->getLocalRect(), &bottom_rect, this);
	}
	if (mToolbarLeft)   
	{
		mToolbarLeft->getParent()->reshape(mToolbarLeft->getRect().getWidth(), mToolbarLeft->getParent()->getRect().getHeight());
		mToolbarLeft->localRectToOtherView(mToolbarLeft->getLocalRect(), &left_rect, this);
	}
	if (mToolbarRight)  
	{
		mToolbarRight->getParent()->reshape(mToolbarRight->getRect().getWidth(), mToolbarRight->getParent()->getRect().getHeight());
		mToolbarRight->localRectToOtherView(mToolbarRight->getLocalRect(), &right_rect, this);
	}
	
	if ((old_width != getRect().getWidth()) || (old_height != getRect().getHeight()))
		debug_print = true;
	if (debug_print)
	{
		LLRect ctrl_rect = getRect();
		llinfos << "Merov debug : draw control rect = " << ctrl_rect.mLeft << ", " << ctrl_rect.mTop << ", " << ctrl_rect.mRight << ", " << ctrl_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw bottom  rect = " << bottom_rect.mLeft << ", " << bottom_rect.mTop << ", " << bottom_rect.mRight << ", " << bottom_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw left    rect = " << left_rect.mLeft << ", " << left_rect.mTop << ", " << left_rect.mRight << ", " << left_rect.mBottom << llendl; 
		llinfos << "Merov debug : draw right   rect = " << right_rect.mLeft << ", " << right_rect.mTop << ", " << right_rect.mRight << ", " << right_rect.mBottom << llendl; 
		old_width = ctrl_rect.getWidth();
		old_height = ctrl_rect.getHeight();
		debug_print = false;
	}
	// Debug draw
	LLColor4 back_color = LLColor4::blue;
	LLColor4 back_color_vert = LLColor4::red;
	LLColor4 back_color_hori = LLColor4::yellow;
	back_color[VALPHA] = 0.5f;
	back_color_hori[VALPHA] = 0.5f;
	back_color_vert[VALPHA] = 0.5f;
	//gl_rect_2d(getLocalRect(), back_color, TRUE);
	//gl_rect_2d(bottom_rect, back_color_hori, TRUE);
	//gl_rect_2d(left_rect, back_color_vert, TRUE);
	//gl_rect_2d(right_rect, back_color_vert, TRUE);
	
	LLUICtrl::draw();
}


// ----------------------------------------
// Drag and Drop hacks (under construction)
// ----------------------------------------


void LLToolBarView::startDragItem( S32 x, S32 y, const LLUUID& uuid)
{
	llinfos << "Merov debug: startDragItem() : x = " << x << ", y = " << y << llendl;
	LLToolDragAndDrop::getInstance()->setDragStart( x, y );
	sDragStarted = false;
}

BOOL LLToolBarView::handleDragItem( S32 x, S32 y, const LLUUID& uuid, LLAssetType::EType type)
{
//	llinfos << "Merov debug: handleDragItem() : x = " << x << ", y = " << y << ", uuid = " << uuid << llendl;
	if (LLToolDragAndDrop::getInstance()->isOverThreshold( x, y ))
	{
		if (!sDragStarted)
		{
			std::vector<EDragAndDropType> types;
			uuid_vec_t cargo_ids;
			types.push_back(DAD_WIDGET);
			cargo_ids.push_back(uuid);
			gClipboard.setSourceObject(uuid,LLAssetType::AT_WIDGET);
			LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_VIEWER;
			LLUUID srcID;
			llinfos << "Merov debug: handleDragItem() :  beginMultiDrag()" << llendl;
			LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, src, srcID);
			sDragStarted = true;
			return TRUE;
		}
		else
		{
			MASK mask = 0;
			return LLToolDragAndDrop::getInstance()->handleHover( x, y, mask );
		}
	}
	return FALSE;
}

BOOL LLToolBarView::handleDrop( EDragAndDropType cargo_type, void* cargo_data, const LLUUID& toolbar_id)
{
	LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
	llinfos << "Merov debug : handleDrop. Drop " << inv_item->getUUID() << " named " << inv_item->getName() << " of type " << inv_item->getType() << " to toolbar " << toolbar_id << " under cargo type " << cargo_type << llendl;
		
	LLAssetType::EType type = inv_item->getType();
	if (type == LLAssetType::AT_WIDGET)
	{
		llinfos << "Merov debug : handleDrop. Drop source is a widget -> that's where we'll get code in..." << llendl;
		// Find out if he command is in one of the toolbar
		// If it is, pull it out of the toolbar
		// Now insert it in the toolbar in the correct spot...
	}
	else
	{
		llinfos << "Merov debug : handleDrop. Drop source is not a widget -> nothing to do" << llendl;
	}
	
	return TRUE;
}


