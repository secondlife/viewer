/** 
 * @file lltoolview.cpp
 * @brief A UI contains for tool palette tools
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#include "llviewerprecompiledheaders.h"

#include "lltoolview.h"

#include "llfontgl.h"
#include "llrect.h"

#include "llagent.h"
#include "llbutton.h"
#include "llpanel.h"
#include "lltool.h"
#include "lltoolmgr.h"
#include "lltextbox.h"
#include "llresmgr.h"


LLToolContainer::LLToolContainer(LLToolView* parent)
:   mParent(parent),
    mButton(NULL),
    mPanel(NULL),
    mTool(NULL)
{ }


LLToolContainer::~LLToolContainer()
{
    // mParent is a pointer to the tool view
    // mButton is owned by the tool view
    // mPanel is owned by the tool view
    delete mTool;
    mTool = NULL;
}


LLToolView::LLToolView(const std::string& name, const LLRect& rect)
:   mButtonCount(0)
{
    LLView::init(LLView::Params().name(name).rect(rect).mouse_opaque(true));
}


LLToolView::~LLToolView()
{
    for_each(mContainList.begin(), mContainList.end(), DeletePointer());
    mContainList.clear();
}

//*TODO:translate?
// void LLToolView::addTool(const std::string& icon_off, const std::string& icon_on, LLPanel* panel, LLTool* tool, LLView* hoverView, const char* label)
// {
//  llassert(tool);

//  LLToolContainer* contain = new LLToolContainer(this);

//  LLRect btn_rect = getButtonRect(mButtonCount);

//  contain->mButton = new LLButton("ToolBtn",
//      btn_rect, 
//      icon_off,
//      icon_on, 
//      "",
//      &LLToolView::onClickToolButton,
//      contain,
//      LLFontGL::getFontSansSerif());

//  contain->mPanel = panel;
//  contain->mTool = tool;

//  addChild(contain->mButton);
//  mButtonCount++;

//  const S32 LABEL_TOP_SPACING = 0;
//  const LLFontGL* font = LLResMgr::getInstance()->getRes( LLFONT_SANSSERIF_SMALL );
//  S32 label_width = font->getWidth( label );
//  LLRect label_rect;
//  label_rect.setLeftTopAndSize( 
//      btn_rect.mLeft + btn_rect.getWidth() / 2 - label_width / 2,
//      btn_rect.mBottom - LABEL_TOP_SPACING,
//      label_width, 
//      llfloor(font->getLineHeight()));
//  addChild( new LLTextBox( "tool label", label_rect, label, font ) );

//  // Can optionally ignore panel
//  if (contain->mPanel)
//  {
//      contain->mPanel->setBackgroundVisible( FALSE );
//      contain->mPanel->setBorderVisible( FALSE );
//      addChild(contain->mPanel);
//  }

//  mContainList.push_back(contain);
// }


LLRect LLToolView::getButtonRect(S32 button_index)
{
    const S32 HPAD = 7;
    const S32 VPAD = 7;
    const S32 TOOL_SIZE = 32;
    const S32 HORIZ_SPACING = TOOL_SIZE + 5;
    const S32 VERT_SPACING = TOOL_SIZE + 14;

    S32 tools_per_row = getRect().getWidth() / HORIZ_SPACING;

    S32 row = button_index / tools_per_row;
    S32 column = button_index % tools_per_row; 

    // Build the rectangle, recalling the origin is at lower left
    // and we want the icons to build down from the top.
    LLRect rect;
    rect.setLeftTopAndSize( HPAD + (column * HORIZ_SPACING),
                            -VPAD + getRect().getHeight() - (row * VERT_SPACING),
                            TOOL_SIZE,
                            TOOL_SIZE
                            );

    return rect;
}


void LLToolView::draw()
{
    // turn off highlighting for all containers 
    // and hide all option panels except for the selected one.
    LLTool* selected = LLToolMgr::getInstance()->getCurrentToolset()->getSelectedTool();
    for (contain_list_t::iterator iter = mContainList.begin();
         iter != mContainList.end(); ++iter)
    {
        LLToolContainer* contain = *iter;
        BOOL state = (contain->mTool == selected);
        contain->mButton->setToggleState( state );
        if (contain->mPanel)
        {
            contain->mPanel->setVisible( state );
        }
    }

    // Draw children normally
    LLView::draw();
}

// protected
LLToolContainer* LLToolView::findToolContainer( LLTool *tool )
{
    // Find the container for this tool
    llassert( tool );
    for (contain_list_t::iterator iter = mContainList.begin();
         iter != mContainList.end(); ++iter)
    {
        LLToolContainer* contain = *iter;
        if( contain->mTool == tool )
        {
            return contain;
        }
    }
    LL_ERRS() << "LLToolView::findToolContainer - tool not found" << LL_ENDL;
    return NULL;
}

// static
void LLToolView::onClickToolButton(void* userdata)
{
    LLToolContainer* clicked = (LLToolContainer*) userdata;

    // Switch to this one
    LLToolMgr::getInstance()->getCurrentToolset()->selectTool( clicked->mTool );
}



