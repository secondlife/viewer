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

#include <boost/foreach.hpp>

LLToolBarView* gToolBarView = NULL;

static LLDefaultChildRegistry::Register<LLToolBarView> r("toolbar_view");

LLToolBarView::Toolbar::Toolbar()
:	commands("command")
{}

LLToolBarView::ToolbarSet::ToolbarSet()
:	left_toolbar("left_toolbar"),
	right_toolbar("right_toolbar"),
	bottom_toolbar("bottom_toolbar")
{}

bool LLToolBarView::load()
{	
	LLToolBarView::ToolbarSet toolbar_set;

	// Load the default toolbars.xml file
	// *TODO : pick up the user's toolbar setting if existing
	std::string toolbar_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "toolbars.xml");

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
	
	// Add commands to each toolbar
	// *TODO: factorize that code : tricky with Blocks though, simple lexical approach fails
	LLCommandManager& mgr = LLCommandManager::instance();

	if (toolbar_set.left_toolbar.isProvided() && mToolbarLeft)
	{
		BOOST_FOREACH(LLCommandId::Params& command, toolbar_set.left_toolbar.commands)
		{
			LLCommandId* commandId = new LLCommandId(command);
			if (mgr.getCommand(*commandId))
			{
				mToolbarLeft->addCommand(*commandId);
			}
			else 
			{
				llwarns	<< "Toolbars creation : the command " << commandId->name() << " cannot be found in the command manager" << llendl;
			}
		}
	}
	if (toolbar_set.right_toolbar.isProvided() && mToolbarRight)
	{
		BOOST_FOREACH(LLCommandId::Params& command, toolbar_set.right_toolbar.commands)
		{
			LLCommandId* commandId = new LLCommandId(command);
			if (mgr.getCommand(*commandId))
			{
				mToolbarRight->addCommand(*commandId);
			}
			else 
			{
				llwarns	<< "Toolbars creation : the command " << commandId->name() << " cannot be found in the command manager" << llendl;
			}
		}
	}
	if (toolbar_set.bottom_toolbar.isProvided() && mToolbarBottom)
	{
		BOOST_FOREACH(LLCommandId::Params& command, toolbar_set.bottom_toolbar.commands)
		{
			LLCommandId* commandId = new LLCommandId(command);
			if (mgr.getCommand(*commandId))
			{
				mToolbarBottom->addCommand(*commandId);
			}
			else 
			{
				llwarns	<< "Toolbars creation : the command " << commandId->name() << " cannot be found in the command manager" << llendl;
			}
		}
	}
	return true;
}

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
}

BOOL LLToolBarView::postBuild()
{
	mToolbarLeft   = getChild<LLToolBar>("toolbar_left");
	mToolbarRight  = getChild<LLToolBar>("toolbar_right");
	mToolbarBottom = getChild<LLToolBar>("toolbar_bottom");

	// Load the toolbars from the settings
	load();
	
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

void LLToolBarView::draw()
{
	static bool debug_print = true;
	static S32 old_width = 0;
	static S32 old_height = 0;

//	LLPanel* sizer_left = getChild<LLPanel>("sizer_left");
	
	LLRect bottom_rect, left_rect, right_rect;

	if (mToolbarBottom) mToolbarBottom->localRectToOtherView(mToolbarBottom->getLocalRect(), &bottom_rect, this);
	if (mToolbarLeft)   mToolbarLeft->localRectToOtherView(mToolbarLeft->getLocalRect(), &left_rect, this);
	if (mToolbarRight)  mToolbarRight->localRectToOtherView(mToolbarRight->getLocalRect(), &right_rect, this);
	
	
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
	gl_rect_2d(bottom_rect, back_color_hori, TRUE);
	gl_rect_2d(left_rect, back_color_vert, TRUE);
	gl_rect_2d(right_rect, back_color_vert, TRUE);
	
	LLUICtrl::draw();
}
