/** 
 * @file LLAccordionCtrlTab.cpp
 * @brief Collapsible control implementation
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "lluictrl.h"

#include "llaccordionctrltab.h"

#include "lltextbox.h"

static const std::string DD_BUTTON_NAME = "dd_button";
static const std::string DD_TEXTBOX_NAME = "dd_textbox";
static const std::string DD_HEADER_NAME = "dd_header";

static const S32 HEADER_HEIGHT = 20;
static const S32 HEADER_IMAGE_LEFT_OFFSET = 5;
static const S32 HEADER_TEXT_LEFT_OFFSET = 30;
static const F32 AUTO_OPEN_TIME = 1.f;

static LLDefaultChildRegistry::Register<LLAccordionCtrlTab> t1("accordion_tab");

class LLAccordionCtrlTab::LLAccordionCtrlTabHeader : public LLUICtrl
{
public:
	friend class LLUICtrlFactory;

	struct Params : public LLInitParam::Block<Params, LLAccordionCtrlTab::Params>
	{
		Params();
	};

	LLAccordionCtrlTabHeader(const LLAccordionCtrlTabHeader::Params& p);
	
	virtual ~LLAccordionCtrlTabHeader();

	virtual void draw();

	virtual void reshape(S32 width, S32 height, BOOL called_from_parent = TRUE);

	virtual BOOL postBuild();

	void	setTitle(const std::string& title);

	virtual void onMouseEnter(S32 x, S32 y, MASK mask);
	virtual void onMouseLeave(S32 x, S32 y, MASK mask);
	virtual BOOL handleKey(KEY key, MASK mask, BOOL called_from_parent);
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
private:

	LLTextBox* mHeaderTextbox;

	// Overlay images (arrows)
	LLPointer<LLUIImage> mImageCollapsed;
	LLPointer<LLUIImage> mImageExpanded;
	LLPointer<LLUIImage> mImageCollapsedPressed;
	LLPointer<LLUIImage> mImageExpandedPressed;

	// Background images
	LLPointer<LLUIImage> mImageHeader;
	LLPointer<LLUIImage> mImageHeaderOver;
	LLPointer<LLUIImage> mImageHeaderPressed;
	LLPointer<LLUIImage> mImageHeaderFocused;

	LLUIColor mHeaderBGColor;

	bool mNeedsHighlight;

	LLFrameTimer mAutoOpenTimer;
};

LLAccordionCtrlTab::LLAccordionCtrlTabHeader::Params::Params()
{
}

LLAccordionCtrlTab::LLAccordionCtrlTabHeader::LLAccordionCtrlTabHeader(
	const LLAccordionCtrlTabHeader::Params& p)
: LLUICtrl(p)
, mHeaderBGColor(p.header_bg_color())
,mNeedsHighlight(false),
	mImageCollapsed(p.header_collapse_img),
	mImageCollapsedPressed(p.header_collapse_img_pressed),
	mImageExpanded(p.header_expand_img),
	mImageExpandedPressed(p.header_expand_img_pressed),
	mImageHeader(p.header_image),
	mImageHeaderOver(p.header_image_over),
	mImageHeaderPressed(p.header_image_pressed),
	mImageHeaderFocused(p.header_image_focused)
{
	LLTextBox::Params textboxParams;
	textboxParams.name(DD_TEXTBOX_NAME);
	textboxParams.initial_value(p.title());
	textboxParams.text_color(p.header_text_color());
	textboxParams.follows.flags(FOLLOWS_NONE);
	textboxParams.font( p.font() );
	textboxParams.font_shadow(LLFontGL::NO_SHADOW);
	textboxParams.use_ellipses = true;
	textboxParams.bg_visible = false;
	textboxParams.mouse_opaque = false;
	mHeaderTextbox = LLUICtrlFactory::create<LLTextBox>(textboxParams);
	addChild(mHeaderTextbox);
}

LLAccordionCtrlTab::LLAccordionCtrlTabHeader::~LLAccordionCtrlTabHeader()
{
}

BOOL LLAccordionCtrlTab::LLAccordionCtrlTabHeader::postBuild()
{
	return TRUE;
}

void	LLAccordionCtrlTab::LLAccordionCtrlTabHeader::setTitle(const std::string& title)
{
	if(mHeaderTextbox)
		mHeaderTextbox->setText(title);
}

void LLAccordionCtrlTab::LLAccordionCtrlTabHeader::draw()
{
	S32 width = getRect().getWidth();
	S32 height = getRect().getHeight();

	gl_rect_2d(0,0,width - 1 ,height - 1,mHeaderBGColor.get(),true);

	LLAccordionCtrlTab* parent = dynamic_cast<LLAccordionCtrlTab*>(getParent());
	bool collapsible = (parent && parent->getCollapsible());
	bool expanded = (parent && parent->getDisplayChildren());

	// Handle overlay images, if needed
	// Only show green "focus" background image if the accordion is open,
	// because the user's mental model of focus is that it goes away after
	// the accordion is closed.
	if (getParent()->hasFocus()
		/*&& !(collapsible && !expanded)*/ // WHY??
		)
	{
		mImageHeaderFocused->draw(0,0,width,height);
	}
	else
	{
		mImageHeader->draw(0,0,width,height);
	}

	if(mNeedsHighlight)
	{
		mImageHeaderOver->draw(0,0,width,height);
	}
	

	if(collapsible)
	{
		LLPointer<LLUIImage> overlay_image;
		if(expanded)
		{
			overlay_image = mImageExpanded;
		}
		else
		{
			overlay_image = mImageCollapsed;
		}
		overlay_image->draw(HEADER_IMAGE_LEFT_OFFSET,
							(height - overlay_image->getHeight()) / 2);
	}
	
	LLUICtrl::draw();
}

void LLAccordionCtrlTab::LLAccordionCtrlTabHeader::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	S32 header_height = mHeaderTextbox->getTextPixelHeight();

	LLRect textboxRect(HEADER_TEXT_LEFT_OFFSET,(height+header_height)/2 ,width,(height-header_height)/2);
	mHeaderTextbox->reshape(textboxRect.getWidth(), textboxRect.getHeight());
	mHeaderTextbox->setRect(textboxRect);
}

void LLAccordionCtrlTab::LLAccordionCtrlTabHeader::onMouseEnter(S32 x, S32 y, MASK mask)
{
	LLUICtrl::onMouseEnter(x, y, mask);
	mNeedsHighlight = true;
}
void LLAccordionCtrlTab::LLAccordionCtrlTabHeader::onMouseLeave(S32 x, S32 y, MASK mask)
{
	LLUICtrl::onMouseLeave(x, y, mask);
	mNeedsHighlight = false;
	mAutoOpenTimer.stop();
}
BOOL LLAccordionCtrlTab::LLAccordionCtrlTabHeader::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	if ( ( key == KEY_LEFT || key == KEY_RIGHT) && mask == MASK_NONE)
	{
		return getParent()->handleKey(key, mask, called_from_parent);
	}
	return LLUICtrl::handleKey(key, mask, called_from_parent);
}
BOOL LLAccordionCtrlTab::LLAccordionCtrlTabHeader::handleDragAndDrop(S32 x, S32 y, MASK mask,
																	 BOOL drop,
																	 EDragAndDropType cargo_type,
																	 void* cargo_data,
																	 EAcceptance* accept,
																	 std::string& tooltip_msg)
{
	LLAccordionCtrlTab* parent = dynamic_cast<LLAccordionCtrlTab*>(getParent());

	if ( parent && !parent->getDisplayChildren() && parent->getCollapsible() && parent->canOpenClose() )
	{
		if (mAutoOpenTimer.getStarted())
		{
			if (mAutoOpenTimer.getElapsedTimeF32() > AUTO_OPEN_TIME)
			{
				parent->changeOpenClose(false);
				mAutoOpenTimer.stop();
				return TRUE;
			}
		}
		else
			mAutoOpenTimer.start();
	}

	return LLUICtrl::handleDragAndDrop(x, y, mask, drop, cargo_type,
									   cargo_data, accept, tooltip_msg);
}
LLAccordionCtrlTab::Params::Params()
	: title("title")
	,display_children("expanded", true)
	,header_height("header_height", HEADER_HEIGHT),
	min_width("min_width", 0),
	min_height("min_height", 0)
	,collapsible("collapsible", true)
	,header_bg_color("header_bg_color")
	,dropdown_bg_color("dropdown_bg_color")
	,header_visible("header_visible",true)
	,padding_left("padding_left",2)
	,padding_right("padding_right",2)
	,padding_top("padding_top",2)
	,padding_bottom("padding_bottom",2)
	,header_expand_img("header_expand_img")
	,header_expand_img_pressed("header_expand_img_pressed")
	,header_collapse_img("header_collapse_img")
	,header_collapse_img_pressed("header_collapse_img_pressed")
	,header_image("header_image")
	,header_image_over("header_image_over")
	,header_image_pressed("header_image_pressed")
	,header_image_focused("header_image_focused")
	,header_text_color("header_text_color")
{
	mouse_opaque(false);
}

LLAccordionCtrlTab::LLAccordionCtrlTab(const LLAccordionCtrlTab::Params&p)
	: LLUICtrl(p)
	,mDisplayChildren(p.display_children)
	,mCollapsible(p.collapsible)
	,mExpandedHeight(0)
	,mDropdownBGColor(p.dropdown_bg_color())
	,mHeaderVisible(p.header_visible)
	,mPaddingLeft(p.padding_left)
	,mPaddingRight(p.padding_right)
	,mPaddingTop(p.padding_top)
	,mPaddingBottom(p.padding_bottom)
	,mCanOpenClose(true)
{
	mStoredOpenCloseState = false;
	mWasStateStored = false;
	
	mDropdownBGColor = LLColor4::white;
	LLAccordionCtrlTabHeader::Params headerParams;
	headerParams.name(DD_HEADER_NAME);
	headerParams.title(p.title);
	mHeader = LLUICtrlFactory::create<LLAccordionCtrlTabHeader>(headerParams);
	addChild(mHeader, 1);

	reshape(100, 200,FALSE);
}

LLAccordionCtrlTab::~LLAccordionCtrlTab()
{
}


void LLAccordionCtrlTab::setDisplayChildren(bool display)
{
	mDisplayChildren = display;
	LLRect rect = getRect();

	rect.mBottom = rect.mTop - (getDisplayChildren() ? 
		mExpandedHeight : HEADER_HEIGHT);
	setRect(rect);

	for(child_list_const_iter_t it = getChildList()->begin();
		getChildList()->end() != it; ++it)
	{
		LLView* child = *it;
		if(DD_HEADER_NAME == child->getName())
			continue;

		child->setVisible(getDisplayChildren());
	}
}

void LLAccordionCtrlTab::reshape(S32 width, S32 height, BOOL called_from_parent /* = TRUE */)
{
	LLRect headerRect;

	LLUICtrl::reshape(width, height, TRUE);

	headerRect.setLeftTopAndSize(
		0,height,width,HEADER_HEIGHT);
	mHeader->setRect(headerRect);
	mHeader->reshape(headerRect.getWidth(), headerRect.getHeight());

	for(child_list_const_iter_t it = getChildList()->begin(); 
		getChildList()->end() != it; ++it)
	{
		LLView* child = *it;
		if(DD_HEADER_NAME == child->getName())
			continue;
		if(!child->getVisible())
			continue;

		LLRect childRect = child->getRect();
		S32 childWidth = width - getPaddingLeft() - getPaddingRight();
		S32 childHeight = height - getHeaderHeight() - getPaddingTop() - getPaddingBottom();

		child->reshape(childWidth,childHeight);
		
		childRect.setLeftTopAndSize(
			getPaddingLeft(),
			childHeight + getPaddingBottom(),
			childWidth, 
			childHeight);

		child->setRect(childRect);
		
		break;//suppose that there is only one panel
	}

}

void LLAccordionCtrlTab::changeOpenClose(bool is_open)
{
	if(is_open)
		mExpandedHeight = getRect().getHeight();

	setDisplayChildren(!is_open);
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
	if (mCommitSignal)
	{
		(*mCommitSignal)(this, getDisplayChildren());
	}
}

BOOL LLAccordionCtrlTab::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if(mCollapsible && mHeaderVisible && mCanOpenClose)
	{
		if(y >= (getRect().getHeight() - HEADER_HEIGHT) )
		{
			LLAccordionCtrlTabHeader* header = getChild<LLAccordionCtrlTabHeader>(DD_HEADER_NAME);
			header->setFocus(true);
			changeOpenClose(getDisplayChildren());

			//reset stored state
			mWasStateStored = false;
			return TRUE;
		}
	}
	return LLUICtrl::handleMouseDown(x,y,mask);
}

BOOL LLAccordionCtrlTab::handleMouseUp(S32 x, S32 y, MASK mask)
{
	return LLUICtrl::handleMouseUp(x,y,mask);
}

boost::signals2::connection LLAccordionCtrlTab::setDropDownStateChangedCallback(commit_callback_t cb)
{
	return setCommitCallback(cb);
}

bool LLAccordionCtrlTab::addChild(LLView* child, S32 tab_group)
{
	if(DD_HEADER_NAME != child->getName())
	{
		reshape(child->getRect().getWidth() , child->getRect().getHeight() + HEADER_HEIGHT );
		mExpandedHeight = getRect().getHeight();
	}

	bool res = LLUICtrl::addChild(child, tab_group);

	if(DD_HEADER_NAME != child->getName())
	{
		if(!mCollapsible)
			setDisplayChildren(true);
		else
			setDisplayChildren(getDisplayChildren());	
	}

	return res;
}

void LLAccordionCtrlTab::setAccordionView(LLView* panel)
{
	addChild(panel,0);
}


LLView*	LLAccordionCtrlTab::getAccordionView()
{
	for(child_list_const_iter_t it = getChildList()->begin(); 
		getChildList()->end() != it; ++it)
	{
		LLView* child = *it;
		if(DD_HEADER_NAME == child->getName())
			continue;
		if(!child->getVisible())
			continue;
		return child;
	}
	return NULL;
}


S32 LLAccordionCtrlTab::getHeaderHeight()
{
	return mHeaderVisible?HEADER_HEIGHT:0; 
}

void LLAccordionCtrlTab::setHeaderVisible(bool value) 
{
	if(mHeaderVisible == value)
		return;
	mHeaderVisible = value;
	if(mHeader)
		mHeader->setVisible(value);
	reshape(getRect().getWidth(), getRect().getHeight(), FALSE);
};

//vurtual
BOOL LLAccordionCtrlTab::postBuild()
{
	mHeader->setVisible(mHeaderVisible);
	return LLUICtrl::postBuild();
}
bool	LLAccordionCtrlTab::notifyChildren	(const LLSD& info)
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "store_state")
		{
			storeOpenCloseState();
			return true;
		}
		if(str_action == "restore_state")
		{
			restoreOpenCloseState();
			return true;
		}
	}	
	return LLUICtrl::notifyChildren(info);
}

S32	LLAccordionCtrlTab::notifyParent(const LLSD& info)
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "size_changes")
		{
			//
			S32 height = info["height"];
			height = llmax(height,10) + HEADER_HEIGHT + getPaddingTop() + getPaddingBottom();
			
			mExpandedHeight = height;
			
			if(isExpanded())
			{
				LLRect panel_rect = getRect();
				panel_rect.setLeftTopAndSize( panel_rect.mLeft, panel_rect.mTop, panel_rect.getWidth(), height);
				reshape(getRect().getWidth(),height);
				setRect(panel_rect);
			}
			
			//LLAccordionCtrl should rearrange accodion tab if one of accordion change its size
			getParent()->notifyParent(info);
			return 1;
		}
		else if(str_action == "select_prev") 
		{
			showAndFocusHeader();
			return 1;
		}
	}
	return LLUICtrl::notifyParent(info);
}

S32 LLAccordionCtrlTab::notify(const LLSD& info)
{
	if(info.has("action"))
	{
		std::string str_action = info["action"];
		if(str_action == "select_first")
		{
			showAndFocusHeader();
			return 1;
		}
		else if( str_action == "select_last" )
		{
			if(getDisplayChildren() == false)
			{
				showAndFocusHeader();
			}
			else
			{
				LLView* view = getAccordionView();
				if(view)
					view->notify(LLSD().with("action","select_last"));
			}
		}
	}
	return 0;
}

BOOL LLAccordionCtrlTab::handleKey(KEY key, MASK mask, BOOL called_from_parent)
{
	LLAccordionCtrlTabHeader* header = getChild<LLAccordionCtrlTabHeader>(DD_HEADER_NAME);	
	if( !header->hasFocus() )
		return LLUICtrl::handleKey(key, mask, called_from_parent);

	if ( (key == KEY_RETURN )&& mask == MASK_NONE)
	{
		changeOpenClose(getDisplayChildren());
		return TRUE;
	}

	if ( (key == KEY_ADD || key == KEY_RIGHT)&& mask == MASK_NONE)
	{
		if(getDisplayChildren() == false)
		{
			changeOpenClose(getDisplayChildren());
			return TRUE;
		}
	}
	if ( (key == KEY_SUBTRACT || key == KEY_LEFT)&& mask == MASK_NONE)
	{
		if(getDisplayChildren() == true)
		{
			changeOpenClose(getDisplayChildren());
			return TRUE;
		}
	}

	if ( key == KEY_DOWN && mask == MASK_NONE)
	{
		//if collapsed go to the next accordion
		if(getDisplayChildren() == false)
			//we processing notifyParent so let call parent directly
			getParent()->notifyParent(LLSD().with("action","select_next"));
		else
		{
			getAccordionView()->notify(LLSD().with("action","select_first"));
		}
		return TRUE;
	}

	if ( key == KEY_UP && mask == MASK_NONE)
	{
		//go to the previous accordion

		//we processing notifyParent so let call parent directly
		getParent()->notifyParent(LLSD().with("action","select_prev"));
		return TRUE;
	}

	return LLUICtrl::handleKey(key, mask, called_from_parent);
}

void LLAccordionCtrlTab::showAndFocusHeader()
{
	LLAccordionCtrlTabHeader* header = getChild<LLAccordionCtrlTabHeader>(DD_HEADER_NAME);	
	header->setFocus(true);

	LLRect screen_rc;
	LLRect selected_rc = header->getRect();
	localRectToScreen(selected_rc, &screen_rc);
	notifyParent(LLSD().with("scrollToShowRect",screen_rc.getValue()));

}
void    LLAccordionCtrlTab::storeOpenCloseState()
{
	if(mWasStateStored)
		return;
	mStoredOpenCloseState = getDisplayChildren();
	mWasStateStored = true;
}
void   LLAccordionCtrlTab::restoreOpenCloseState()
{
	if(!mWasStateStored)
		return;
	if(getDisplayChildren() != mStoredOpenCloseState)
	{
		changeOpenClose(getDisplayChildren());
	}
	mWasStateStored = false;
}
