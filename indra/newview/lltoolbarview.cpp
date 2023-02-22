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
#include "llbutton.h"
#include "llclipboard.h"
#include "lldir.h"
#include "lldockablefloater.h"
#include "lldockcontrol.h"
#include "llimview.h"
#include "lltransientfloatermgr.h"
#include "lltoolbar.h"
#include "lltooldraganddrop.h"
#include "llxmlnode.h"

#include "llagent.h"  // HACK for destinations guide on startup
#include "llfloaterreg.h"  // HACK for destinations guide on startup
#include "llviewercontrol.h"  // HACK for destinations guide on startup
#include "llinventorymodel.h" // HACK to disable starter avatars button for NUX

#include <boost/foreach.hpp>

LLToolBarView* gToolBarView = NULL;

static LLDefaultChildRegistry::Register<LLToolBarView> r("toolbar_view");

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
	mDragStarted(false),
	mShowToolbars(true),
	mDragToolbarButton(NULL),
	mDragItem(NULL),
	mToolbarsLoaded(false),
	mBottomToolbarPanel(NULL)
{
	for (S32 i = 0; i < LLToolBarEnums::TOOLBAR_COUNT; i++)
	{
		mToolbars[i] = NULL;
	}
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
	mToolbars[LLToolBarEnums::TOOLBAR_LEFT] = getChild<LLToolBar>("toolbar_left");
	mToolbars[LLToolBarEnums::TOOLBAR_LEFT]->getCenterLayoutPanel()->setLocationId(LLToolBarEnums::TOOLBAR_LEFT);

	mToolbars[LLToolBarEnums::TOOLBAR_RIGHT] = getChild<LLToolBar>("toolbar_right");
	mToolbars[LLToolBarEnums::TOOLBAR_RIGHT]->getCenterLayoutPanel()->setLocationId(LLToolBarEnums::TOOLBAR_RIGHT);

	mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM] = getChild<LLToolBar>("toolbar_bottom");
	mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM]->getCenterLayoutPanel()->setLocationId(LLToolBarEnums::TOOLBAR_BOTTOM);

	mBottomToolbarPanel = getChild<LLView>("bottom_toolbar_panel");

	for (int i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
	{
		mToolbars[i]->setStartDragCallback(boost::bind(LLToolBarView::startDragTool,_1,_2,_3));
		mToolbars[i]->setHandleDragCallback(boost::bind(LLToolBarView::handleDragTool,_1,_2,_3,_4));
		mToolbars[i]->setHandleDropCallback(boost::bind(LLToolBarView::handleDropTool,_1,_2,_3,_4));
		mToolbars[i]->setButtonAddCallback(boost::bind(LLToolBarView::onToolBarButtonAdded,_1));
		mToolbars[i]->setButtonRemoveCallback(boost::bind(LLToolBarView::onToolBarButtonRemoved,_1));
	}
	
	return TRUE;
}

S32 LLToolBarView::hasCommand(const LLCommandId& commandId) const
{
	S32 command_location = LLToolBarEnums::TOOLBAR_NONE;

	for (S32 loc = LLToolBarEnums::TOOLBAR_FIRST; loc <= LLToolBarEnums::TOOLBAR_LAST; loc++)
	{
		if (mToolbars[loc]->hasCommand(commandId))
		{
			command_location = loc;
			break;
		}
	}

	return command_location;
}

S32 LLToolBarView::addCommand(const LLCommandId& commandId, LLToolBarEnums::EToolBarLocation toolbar, int rank)
{
	int old_rank;
	removeCommand(commandId, old_rank);

	S32 command_location = mToolbars[toolbar]->addCommand(commandId, rank);

	return command_location;
}

S32 LLToolBarView::removeCommand(const LLCommandId& commandId, int& rank)
{
	S32 command_location = hasCommand(commandId);
	rank = LLToolBar::RANK_NONE;

	if (command_location != LLToolBarEnums::TOOLBAR_NONE)
	{
		rank = mToolbars[command_location]->removeCommand(commandId);
	}

	return command_location;
}

S32 LLToolBarView::enableCommand(const LLCommandId& commandId, bool enabled)
{
	S32 command_location = hasCommand(commandId);

	if (command_location != LLToolBarEnums::TOOLBAR_NONE)
	{
		mToolbars[command_location]->enableCommand(commandId, enabled);
	}

	return command_location;
}

S32 LLToolBarView::stopCommandInProgress(const LLCommandId& commandId)
{
	S32 command_location = hasCommand(commandId);

	if (command_location != LLToolBarEnums::TOOLBAR_NONE)
	{
		mToolbars[command_location]->stopCommandInProgress(commandId);
	}

	return command_location;
}

S32 LLToolBarView::flashCommand(const LLCommandId& commandId, bool flash, bool force_flashing/* = false */)
{
	S32 command_location = hasCommand(commandId);

	if (command_location != LLToolBarEnums::TOOLBAR_NONE)
	{
		mToolbars[command_location]->flashCommand(commandId, flash, force_flashing);
	}

	return command_location;
}

bool LLToolBarView::addCommandInternal(const LLCommandId& command, LLToolBar* toolbar)
{
	LLCommandManager& mgr = LLCommandManager::instance();
	if (mgr.getCommand(command))
	{
		toolbar->addCommand(command);
	}
	else 
	{
		LL_WARNS()	<< "Toolbars creation : the command with id " << command.uuid().asString() << " cannot be found in the command manager" << LL_ENDL;
		return false;
	}
	return true;
}

bool LLToolBarView::loadToolbars(bool force_default)
{
	LLToolBarView::ToolbarSet toolbar_set;
	bool err = false;
	
	// Load the toolbars.xml file
	std::string toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "toolbars.xml");
	if (force_default)
	{
		toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "toolbars.xml");
	}
	else if (!gDirUtilp->fileExists(toolbar_file)) 
	{
		LL_WARNS() << "User toolbars def not found -> use default" << LL_ENDL;
		toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "toolbars.xml");
	}
	
	LLXMLNodePtr root;
	if(!LLXMLNode::parseFile(toolbar_file, root, NULL))
	{
		LL_WARNS() << "Unable to load toolbars from file: " << toolbar_file << LL_ENDL;
		err = true;
	}
	
	if (!err && !root->hasName("toolbars"))
	{
		LL_WARNS() << toolbar_file << " is not a valid toolbars definition file" << LL_ENDL;
		err = true;
	}
	
	// Parse the toolbar settings
	LLXUIParser parser;
	if (!err)
	{
	    parser.readXUI(root, toolbar_set, toolbar_file);
	}

	if (!err && !toolbar_set.validateBlock())
	{
		LL_WARNS() << "Unable to validate toolbars from file: " << toolbar_file << LL_ENDL;
		err = true;
	}
	
	if (err)
	{
		if (force_default)
		{
			LL_ERRS() << "Unable to load toolbars from default file : " << toolbar_file << LL_ENDL;
		    return false;
	    }

		// Try to load the default toolbars
		return loadToolbars(true);
	}
	
	// Clear the toolbars now before adding the loaded commands and settings
	for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
	{
		if (mToolbars[i])
		{
			mToolbars[i]->clearCommandsList();
		}
	}
	
	// Add commands to each toolbar
	if (toolbar_set.left_toolbar.isProvided() && mToolbars[LLToolBarEnums::TOOLBAR_LEFT])
	{
		if (toolbar_set.left_toolbar.button_display_mode.isProvided())
		{
			LLToolBarEnums::ButtonType button_type = toolbar_set.left_toolbar.button_display_mode;
			mToolbars[LLToolBarEnums::TOOLBAR_LEFT]->setButtonType(button_type);
		}
		BOOST_FOREACH(const LLCommandId::Params& command_params, toolbar_set.left_toolbar.commands)
		{
			if (!addCommandInternal(LLCommandId(command_params), mToolbars[LLToolBarEnums::TOOLBAR_LEFT]))
			{
				LL_WARNS() << "Error adding command '" << command_params.name() << "' to left toolbar." << LL_ENDL;
			}
		}
	}
	if (toolbar_set.right_toolbar.isProvided() && mToolbars[LLToolBarEnums::TOOLBAR_RIGHT])
	{
		if (toolbar_set.right_toolbar.button_display_mode.isProvided())
		{
			LLToolBarEnums::ButtonType button_type = toolbar_set.right_toolbar.button_display_mode;
			mToolbars[LLToolBarEnums::TOOLBAR_RIGHT]->setButtonType(button_type);
		}
		BOOST_FOREACH(const LLCommandId::Params& command_params, toolbar_set.right_toolbar.commands)
		{
			if (!addCommandInternal(LLCommandId(command_params), mToolbars[LLToolBarEnums::TOOLBAR_RIGHT]))
			{
				LL_WARNS() << "Error adding command '" << command_params.name() << "' to right toolbar." << LL_ENDL;
			}
		}
	}
	if (toolbar_set.bottom_toolbar.isProvided() && mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM])
	{
		if (toolbar_set.bottom_toolbar.button_display_mode.isProvided())
		{
			LLToolBarEnums::ButtonType button_type = toolbar_set.bottom_toolbar.button_display_mode;
			mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM]->setButtonType(button_type);
		}
		BOOST_FOREACH(const LLCommandId::Params& command_params, toolbar_set.bottom_toolbar.commands)
		{
			if (!addCommandInternal(LLCommandId(command_params), mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM]))
			{
				LL_WARNS() << "Error adding command '" << command_params.name() << "' to bottom toolbar." << LL_ENDL;
			}
		}
	}

    // SL-18581: Don't show the starter avatar toolbar button for NUX users
    LLViewerInventoryCategory* my_outfits_cat = gInventory.getCategory(gInventory.findCategoryUUIDForType(LLFolderType::FT_MY_OUTFITS));
    if (gAgent.isFirstLogin()
        && my_outfits_cat != NULL
        && my_outfits_cat->getDescendentCount() > 0)
    {
        for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
        {
            if (mToolbars[i])
            {
                mToolbars[i]->removeCommand(LLCommandId("avatar"));
            }
        }
    }

	mToolbarsLoaded = true;
	return true;
}

bool LLToolBarView::clearToolbars()
{
	for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
	{
		if (mToolbars[i])
		{
			mToolbars[i]->clearCommandsList();
		}
	}

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

//static
bool LLToolBarView::clearAllToolbars()
{
	bool retval = false;

	if (gToolBarView)
	{
		retval = gToolBarView->clearToolbars();
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
	if (mToolbars[LLToolBarEnums::TOOLBAR_LEFT])
	{
		toolbar_set.left_toolbar.button_display_mode = mToolbars[LLToolBarEnums::TOOLBAR_LEFT]->getButtonType();
		addToToolset(mToolbars[LLToolBarEnums::TOOLBAR_LEFT]->getCommandsList(), toolbar_set.left_toolbar);
	}
	if (mToolbars[LLToolBarEnums::TOOLBAR_RIGHT])
	{
		toolbar_set.right_toolbar.button_display_mode = mToolbars[LLToolBarEnums::TOOLBAR_RIGHT]->getButtonType();
		addToToolset(mToolbars[LLToolBarEnums::TOOLBAR_RIGHT]->getCommandsList(), toolbar_set.right_toolbar);
	}
	if (mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM])
	{
		toolbar_set.bottom_toolbar.button_display_mode = mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM]->getButtonType();
		addToToolset(mToolbars[LLToolBarEnums::TOOLBAR_BOTTOM]->getCommandsList(), toolbar_set.bottom_toolbar);
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
	llassert(button);
	
	if (button->getName() == "speak")
	{
		// Add the "Speak" button as a control view in LLTransientFloaterMgr
		// to prevent hiding the transient IM floater upon pressing "Speak".
		LLTransientFloaterMgr::getInstance()->addControlView(button);
		
		// Redock incoming and/or outgoing call windows, if applicable
		
		LLFloater* incoming_floater = LLFloaterReg::getLastFloaterInGroup("incoming_call");
		LLFloater* outgoing_floater = LLFloaterReg::getLastFloaterInGroup("outgoing_call");
		
		if (incoming_floater && incoming_floater->isShown())
		{
			LLCallDialog* incoming = dynamic_cast<LLCallDialog *>(incoming_floater);
			llassert(incoming);
			
			LLDockControl* dock_control = incoming->getDockControl();
			if (dock_control->getDock() == NULL)
			{
				incoming->dockToToolbarButton("speak");
			}
		}
		
		if (outgoing_floater && outgoing_floater->isShown())
		{
			LLCallDialog* outgoing = dynamic_cast<LLCallDialog *>(outgoing_floater);
			llassert(outgoing);
			
			LLDockControl* dock_control = outgoing->getDockControl();
			if (dock_control->getDock() == NULL)
			{
				outgoing->dockToToolbarButton("speak");
			}
		}
	}
	else if (button->getName() == "voice")
	{
		// Add the "Voice controls" button as a control view in LLTransientFloaterMgr
		// to prevent hiding the transient IM floater upon pressing "Voice controls".
		LLTransientFloaterMgr::getInstance()->addControlView(button);
	}
}

void LLToolBarView::onToolBarButtonRemoved(LLView* button)
{
	llassert(button);

	if (button->getName() == "speak")
	{
		LLTransientFloaterMgr::getInstance()->removeControlView(button);
		
		// Undock incoming and/or outgoing call windows
		
		LLFloater* incoming_floater = LLFloaterReg::getLastFloaterInGroup("incoming_call");
		LLFloater* outgoing_floater = LLFloaterReg::getLastFloaterInGroup("outgoing_call");
		
		if (incoming_floater && incoming_floater->isShown())
		{
			LLDockableFloater* incoming = dynamic_cast<LLDockableFloater *>(incoming_floater);
			llassert(incoming);

			LLDockControl* dock_control = incoming->getDockControl();
			dock_control->setDock(NULL);
		}
		
		if (outgoing_floater && outgoing_floater->isShown())
		{
			LLDockableFloater* outgoing = dynamic_cast<LLDockableFloater *>(outgoing_floater);
			llassert(outgoing);

			LLDockControl* dock_control = outgoing->getDockControl();
			dock_control->setDock(NULL);
		}
	}
	else if (button->getName() == "voice")
	{
		LLTransientFloaterMgr::getInstance()->removeControlView(button);
	}
}

void LLToolBarView::draw()
{
	LLRect toolbar_rects[LLToolBarEnums::TOOLBAR_COUNT];
	
	for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
	{
		if (mToolbars[i])
		{
			LLView::EOrientation orientation = LLToolBarEnums::getOrientation(mToolbars[i]->getSideType());

			if (orientation == LLLayoutStack::HORIZONTAL)
			{
				mToolbars[i]->getParent()->reshape(mToolbars[i]->getParent()->getRect().getWidth(), mToolbars[i]->getRect().getHeight());
			}
			else
			{
				mToolbars[i]->getParent()->reshape(mToolbars[i]->getRect().getWidth(), mToolbars[i]->getParent()->getRect().getHeight());
			}

			mToolbars[i]->localRectToOtherView(mToolbars[i]->getLocalRect(), &toolbar_rects[i], this);
		}
	}
	
	for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
	{
		mToolbars[i]->getParent()->setVisible(mShowToolbars 
											&& (mToolbars[i]->hasButtons() 
											|| isToolDragged()));
	}

	// Draw drop zones if drop of a tool is active
	if (isToolDragged())
	{
		LLColor4 drop_color = LLUIColorTable::instance().getColor( "ToolbarDropZoneColor" );

		for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
		{
			gl_rect_2d(toolbar_rects[i], drop_color, TRUE);
		}
	}
	
	LLUICtrl::draw();
}


// ----------------------------------------
// Drag and Drop Handling
// ----------------------------------------


void LLToolBarView::startDragTool(S32 x, S32 y, LLToolBarButton* toolbarButton)
{
	resetDragTool(toolbarButton);

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
			LLToolDragAndDrop::ESource src = LLToolDragAndDrop::SOURCE_VIEWER;
			LLUUID srcID;
			LLToolDragAndDrop::getInstance()->beginMultiDrag(types, cargo_ids, src, srcID);

			// Second, stop the command if it is in progress and requires stopping!
			LLCommandId command_id = LLCommandId(uuid);
			gToolBarView->stopCommandInProgress(command_id);

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
	LLInventoryObject* inv_item = static_cast<LLInventoryObject*>(cargo_data);
	
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
			S32 old_toolbar_loc = gToolBarView->hasCommand(command_id);
			LLToolBar* old_toolbar = NULL;

			if (old_toolbar_loc != LLToolBarEnums::TOOLBAR_NONE)
			{
				llassert(gToolBarView->mDragToolbarButton);
				old_toolbar = gToolBarView->mDragToolbarButton->getParentByType<LLToolBar>();
				if (old_toolbar->isReadOnly() && toolbar->isReadOnly())
				{
					// do nothing
				}
				else
				{
					int old_rank = LLToolBar::RANK_NONE;
					gToolBarView->removeCommand(command_id, old_rank);
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
			LL_WARNS() << "Command couldn't be found in command manager" << LL_ENDL;
		}
	}

	resetDragTool(NULL);
	return handled;
}

void LLToolBarView::resetDragTool(LLToolBarButton* toolbarButton)
{
	// Clear the saved command, toolbar and rank
	gToolBarView->mDragStarted = false;
	gToolBarView->mDragToolbarButton = toolbarButton;
}

// Provide a handle on a free standing inventory item containing references to the tool.
// This might be used by Drag and Drop to move around references to tool items.
LLInventoryObject* LLToolBarView::getDragItem()
{
	if (mDragToolbarButton)
	{
		LLUUID item_uuid = mDragToolbarButton->getCommandId().uuid();
		mDragItem = new LLInventoryObject (item_uuid, LLUUID::null, LLAssetType::AT_WIDGET, "");
	}
	return mDragItem;
}

void LLToolBarView::setToolBarsVisible(bool visible)
{
	mShowToolbars = visible;
}

bool LLToolBarView::isModified() const
{
	bool modified = false;

	for (S32 i = LLToolBarEnums::TOOLBAR_FIRST; i <= LLToolBarEnums::TOOLBAR_LAST; i++)
	{
		modified |= mToolbars[i]->isModified();
	}

	return modified;
}


