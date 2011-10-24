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

#include "llviewerprecompiledheaders.h"

#include "lltoolbarview.h"

#include "llappviewer.h"
#include "lldir.h"
#include "llxmlnode.h"
#include "lltoolbar.h"
#include "llbutton.h"
#include "lltooldraganddrop.h"
#include "lltransientfloatermgr.h"
#include "llclipboard.h"

#include "llagent.h"  // HACK for destinations guide on startup
#include "llfloaterreg.h"  // HACK for destinations guide on startup
#include "llviewercontrol.h"  // HACK for destinations guide on startup

#include <boost/foreach.hpp>

LLToolBarView* gToolBarView = NULL;

static LLDefaultChildRegistry::Register<LLToolBarView> r("toolbar_view");

void handleLoginToolbarSetup();

bool isToolDragged()
{
	return (LLToolDragAndDrop::getInstance()->getSource() == LLToolDragAndDrop::SOURCE_VIEWER);
}

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
	mToolbarBottom(NULL),
	mDragStarted(false),
	mDragToolbarButton(NULL),
	mToolbarsLoaded(false)
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

	mToolbarLeft->setStartDragCallback(boost::bind(LLToolBarView::startDragTool,_1,_2,_3));
	mToolbarLeft->setHandleDragCallback(boost::bind(LLToolBarView::handleDragTool,_1,_2,_3,_4));
	mToolbarLeft->setHandleDropCallback(boost::bind(LLToolBarView::handleDropTool,_1,_2,_3,_4));
	mToolbarLeft->setButtonAddCallback(boost::bind(LLToolBarView::onToolBarButtonAdded, _1));
	
	mToolbarRight->setStartDragCallback(boost::bind(LLToolBarView::startDragTool,_1,_2,_3));
	mToolbarRight->setHandleDragCallback(boost::bind(LLToolBarView::handleDragTool,_1,_2,_3,_4));
	mToolbarRight->setHandleDropCallback(boost::bind(LLToolBarView::handleDropTool,_1,_2,_3,_4));
	mToolbarRight->setButtonAddCallback(boost::bind(LLToolBarView::onToolBarButtonAdded, _1));
	
	mToolbarBottom->setStartDragCallback(boost::bind(LLToolBarView::startDragTool,_1,_2,_3));
	mToolbarBottom->setHandleDragCallback(boost::bind(LLToolBarView::handleDragTool,_1,_2,_3,_4));
	mToolbarBottom->setHandleDropCallback(boost::bind(LLToolBarView::handleDropTool,_1,_2,_3,_4));
	mToolbarBottom->setButtonAddCallback(boost::bind(LLToolBarView::onToolBarButtonAdded, _1));

	LLAppViewer::instance()->setOnLoginCompletedCallback(boost::bind(&handleLoginToolbarSetup));
	
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
		llwarns	<< "Toolbars creation : the command with id " << command.uuid().asString() << " cannot be found in the command manager" << llendl;
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
		llwarns << "Unable to load toolbars from file: " << toolbar_file << llendl;
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
			LLToolBarEnums::ButtonType button_type = toolbar_set.left_toolbar.button_display_mode;
			mToolbarLeft->setButtonType(button_type);
		}
		BOOST_FOREACH(const LLCommandId::Params& command_name_param, toolbar_set.left_toolbar.commands)
		{
			if (addCommand(LLCommandId(command_name_param), mToolbarLeft) == false)
			{
				llwarns << "Error adding command '" << command_name_param.name() << "' to left toolbar." << llendl;
			}
		}
	}
	if (toolbar_set.right_toolbar.isProvided() && mToolbarRight)
	{
		if (toolbar_set.right_toolbar.button_display_mode.isProvided())
		{
			LLToolBarEnums::ButtonType button_type = toolbar_set.right_toolbar.button_display_mode;
			mToolbarRight->setButtonType(button_type);
		}
		BOOST_FOREACH(const LLCommandId::Params& command_name_param, toolbar_set.right_toolbar.commands)
		{
			if (addCommand(LLCommandId(command_name_param), mToolbarRight) == false)
			{
				llwarns << "Error adding command '" << command_name_param.name() << "' to right toolbar." << llendl;
			}
		}
	}
	if (toolbar_set.bottom_toolbar.isProvided() && mToolbarBottom)
	{
		if (toolbar_set.bottom_toolbar.button_display_mode.isProvided())
		{
			LLToolBarEnums::ButtonType button_type = toolbar_set.bottom_toolbar.button_display_mode;
			mToolbarBottom->setButtonType(button_type);
		}
		BOOST_FOREACH(const LLCommandId::Params& command_name_param, toolbar_set.bottom_toolbar.commands)
		{
			if (addCommand(LLCommandId(command_name_param), mToolbarBottom) == false)
			{
				llwarns << "Error adding command '" << command_name_param.name() << "' to bottom toolbar." << llendl;
			}
		}
	}
	mToolbarsLoaded = true;
	return true;
}

//static
bool LLToolBarView::loadDefaultToolbars()
{
	bool retval = false;

	if (gToolBarView)
	{
		retval = gToolBarView->loadToolbars(true);
		if (retval)
		{
			gToolBarView->saveToolbars();
		}
	}

	return retval;
}

void LLToolBarView::saveToolbars() const
{
	if (!mToolbarsLoaded)
		return;
	
	// Build the parameter tree from the toolbar data
	LLToolBarView::ToolbarSet toolbar_set;
	if (mToolbarLeft)
	{
		toolbar_set.left_toolbar.button_display_mode = mToolbarLeft->getButtonType();
		addToToolset(mToolbarLeft->getCommandsList(),toolbar_set.left_toolbar);
	}
	if (mToolbarRight)
	{
		toolbar_set.right_toolbar.button_display_mode = mToolbarRight->getButtonType();
		addToToolset(mToolbarRight->getCommandsList(),toolbar_set.right_toolbar);
	}
	if (mToolbarBottom)
	{
		toolbar_set.bottom_toolbar.button_display_mode = mToolbarBottom->getButtonType();
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
	LLCommandManager& mgr = LLCommandManager::instance();

	for (command_id_list_t::const_iterator it = command_list.begin();
		 it != command_list.end();
		 ++it)
	{
		LLCommand* command = mgr.getCommand(*it);
		if (command)
		{
			LLCommandId::Params command_name_param;
			command_name_param.name = command->name();
			toolbar.commands.add(command_name_param);
		}
	}
}

void LLToolBarView::onToolBarButtonAdded(LLView* button)
{
	if (button && button->getName() == "speak")
	{
		// Add the "Speak" button as a control view in LLTransientFloaterMgr
		// to prevent hiding the transient IM floater upon pressing "Speak".
		LLTransientFloaterMgr::getInstance()->addControlView(button);
	}
	else if (button && button->getName() == "voice")
	{
		// Add the "Voice controls" button as a control view in LLTransientFloaterMgr
		// to prevent hiding the transient IM floater upon pressing "Voice controls".
		LLTransientFloaterMgr::getInstance()->addControlView(button);
	}
}

void LLToolBarView::draw()
{
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
	
	// Draw drop zones if drop of a tool is active
	if (isToolDragged())
	{
		LLColor4 drop_color = LLUIColorTable::instance().getColor( "ToolbarDropZoneColor" );
		gl_rect_2d(bottom_rect, drop_color, TRUE);
		gl_rect_2d(left_rect, drop_color, TRUE);
		gl_rect_2d(right_rect, drop_color, TRUE);
	}
	
	LLUICtrl::draw();
}


// ----------------------------------------
// Drag and Drop Handling
// ----------------------------------------


void LLToolBarView::startDragTool(S32 x, S32 y, LLToolBarButton* button)
{
	resetDragTool(button);

	// Flag the tool dragging but don't start it yet
	LLToolDragAndDrop::getInstance()->setDragStart( x, y );
}

BOOL LLToolBarView::handleDragTool( S32 x, S32 y, const LLUUID& uuid, LLAssetType::EType type)
{
	if (LLToolDragAndDrop::getInstance()->isOverThreshold( x, y ))
	{
		if (!gToolBarView->mDragStarted)
		{
			// Start the tool dragging:
			
			// First, create the global drag and drop object
			std::vector<EDragAndDropType> types;
			uuid_vec_t cargo_ids;
			types.push_back(DAD_WIDGET);
			cargo_ids.push_back(uuid);
			gClipboard.setSourceObject(uuid,LLAssetType::AT_WIDGET);
			LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_VIEWER;
			LLUUID srcID;
			LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, src, srcID);

			// Second, stop the command if it is in progress and requires stopping!
			LLCommandId command_id = LLCommandId(uuid);
			gToolBarView->mToolbarLeft->stopCommandInProgress(command_id);
			gToolBarView->mToolbarRight->stopCommandInProgress(command_id);
			gToolBarView->mToolbarBottom->stopCommandInProgress(command_id);

			gToolBarView->mDragStarted = true;
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

BOOL LLToolBarView::handleDropTool( void* cargo_data, S32 x, S32 y, LLToolBar* toolbar)
{
	BOOL handled = FALSE;
	LLInventoryItem* inv_item = (LLInventoryItem*)cargo_data;
	
	LLAssetType::EType type = inv_item->getType();
	if (type == LLAssetType::AT_WIDGET)
	{
		handled = TRUE;
		// Get the command from its uuid
		LLCommandManager& mgr = LLCommandManager::instance();
		LLCommandId command_id(inv_item->getUUID());
		LLCommand* command = mgr.getCommand(command_id);
		if (command)
		{
			// Suppress the command from the toolbars (including the one it's dropped in, 
			// this will handle move position).
			bool command_present = gToolBarView->hasCommand(command_id);
			LLToolBar* old_toolbar = NULL;

			if (command_present)
			{
				llassert(gToolBarView->mDragToolbarButton);
				old_toolbar = gToolBarView->mDragToolbarButton->getParentByType<LLToolBar>();
				if (old_toolbar->isReadOnly() && toolbar->isReadOnly())
				{
					// do nothing
				}
				else
				{
					gToolBarView->mToolbarBottom->removeCommand(command_id);
					gToolBarView->mToolbarLeft->removeCommand(command_id);
					gToolBarView->mToolbarRight->removeCommand(command_id);
				}
			}

			// Convert the (x,y) position in rank in toolbar
			if (!toolbar->isReadOnly())
			{
				int new_rank = toolbar->getRankFromPosition(x,y);
				toolbar->addCommand(command_id, new_rank);
			}
			
			// Save the new toolbars configuration
			gToolBarView->saveToolbars();
		}
		else
		{
			llwarns << "Command couldn't be found in command manager" << llendl;
		}
	}

	resetDragTool(NULL);
	return handled;
}

void LLToolBarView::resetDragTool(LLToolBarButton* button)
{
	// Clear the saved command, toolbar and rank
	gToolBarView->mDragStarted = false;
	gToolBarView->mDragToolbarButton = button;
}

void LLToolBarView::setToolBarsVisible(bool visible)
{
	mToolbarBottom->getParent()->setVisible(visible);
	mToolbarLeft->getParent()->setVisible(visible);
	mToolbarRight->getParent()->setVisible(visible);
}

bool LLToolBarView::isModified() const
{
	bool modified = false;

	modified |= mToolbarBottom->isModified();
	modified |= mToolbarLeft->isModified();
	modified |= mToolbarRight->isModified();

	return modified;
}


//
// HACK to bring up destinations guide at startup
//

void handleLoginToolbarSetup()
{
	// Open the destinations guide by default on first login, per Rhett
	if (gSavedSettings.getBOOL("FirstLoginThisInstall") || gAgent.isFirstLogin())
	{
		LLFloaterReg::showInstance("destinations");
	}
}

