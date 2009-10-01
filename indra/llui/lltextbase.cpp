/** 
 * @file lltextbase.cpp
 * @author Martin Reddy
 * @brief The base class of text box/editor, providing Url handling support
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

#include "lltextbase.h"
#include "llstl.h"
#include "llview.h"
#include "llwindow.h"
#include "llmenugl.h"
#include "lltooltip.h"
#include "lluictrl.h"
#include "llurlaction.h"
#include "llurlregistry.h"

#include <boost/bind.hpp>

// global state for all text fields
LLUIColor LLTextBase::mLinkColor = LLColor4::blue;

bool LLTextBase::compare_segment_end::operator()(const LLTextSegmentPtr& a, const LLTextSegmentPtr& b) const
{
	return a->getEnd() < b->getEnd();
}

//
// LLTextSegment
//

LLTextSegment::~LLTextSegment()
{}

S32	LLTextSegment::getWidth(S32 first_char, S32 num_chars) const { return 0; }
S32	LLTextSegment::getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const { return 0; }
S32	LLTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const { return 0; }
void LLTextSegment::updateLayout(const LLTextBase& editor) {}
F32	LLTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect) { return draw_rect.mLeft; }
S32	LLTextSegment::getMaxHeight() const { return 0; }
bool LLTextSegment::canEdit() const { return false; }
void LLTextSegment::unlinkFromDocument(LLTextBase*) {}
void LLTextSegment::linkToDocument(LLTextBase*) {}
void LLTextSegment::setHasMouseHover(bool hover) {}
const LLColor4& LLTextSegment::getColor() const { return LLColor4::white; }
void LLTextSegment::setColor(const LLColor4 &color) {}
const LLStyleSP LLTextSegment::getStyle() const {static LLStyleSP sp(new LLStyle()); return sp; }
void LLTextSegment::setStyle(const LLStyleSP &style) {}
void LLTextSegment::setToken( LLKeywordToken* token ) {}
LLKeywordToken*	LLTextSegment::getToken() const { return NULL; }
BOOL LLTextSegment::getToolTip( std::string& msg ) const { return FALSE; }
void LLTextSegment::setToolTip( const std::string &msg ) {}
void LLTextSegment::dump() const {}


//
// LLNormalTextSegment
//

LLNormalTextSegment::LLNormalTextSegment( const LLStyleSP& style, S32 start, S32 end, LLTextBase& editor ) 
:	LLTextSegment(start, end),
	mStyle( style ),
	mToken(NULL),
	mHasMouseHover(false),
	mEditor(editor)
{
	mMaxHeight = llceil(mStyle->getFont()->getLineHeight());
}

LLNormalTextSegment::LLNormalTextSegment( const LLColor4& color, S32 start, S32 end, LLTextBase& editor, BOOL is_visible) 
:	LLTextSegment(start, end),
	mToken(NULL),
	mHasMouseHover(false),
	mEditor(editor)
{
	mStyle = new LLStyle(LLStyle::Params().visible(is_visible).color(color));

	mMaxHeight = llceil(mStyle->getFont()->getLineHeight());
}

F32 LLNormalTextSegment::draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
{
	if( end - start > 0 )
	{
		if ( mStyle->isImage() && (start >= 0) && (end <= mEnd - mStart))
		{
			LLUIImagePtr image = mStyle->getImage();
			S32 style_image_height = image->getHeight();
			S32 style_image_width = image->getWidth();
			image->draw(draw_rect.mLeft, draw_rect.mTop-style_image_height, 
				style_image_width, style_image_height);
		}

		return drawClippedSegment( getStart() + start, getStart() + end, selection_start, selection_end, draw_rect.mLeft, draw_rect.mBottom);
	}
	return draw_rect.mLeft;
}

// Draws a single text segment, reversing the color for selection if needed.
F32 LLNormalTextSegment::drawClippedSegment(S32 seg_start, S32 seg_end, S32 selection_start, S32 selection_end, F32 x, F32 y)
{
	const LLWString &text = mEditor.getWText();

	F32 right_x = x;
	if (!mStyle->isVisible())
	{
		return right_x;
	}

	const LLFontGL* font = mStyle->getFont();

	LLColor4 color = mStyle->getColor();

	font = mStyle->getFont();

  	if( selection_start > seg_start )
	{
		// Draw normally
		S32 start = seg_start;
		S32 end = llmin( selection_start, seg_end );
		S32 length =  end - start;
		font->render(text, start, x, y, color, LLFontGL::LEFT, LLFontGL::BOTTOM, 0, LLFontGL::NO_SHADOW, length, S32_MAX, &right_x, mEditor.allowsEmbeddedItems());
	}
	x = right_x;
	
	if( (selection_start < seg_end) && (selection_end > seg_start) )
	{
		// Draw reversed
		S32 start = llmax( selection_start, seg_start );
		S32 end = llmin( selection_end, seg_end );
		S32 length = end - start;

		font->render(text, start, x, y,
					 LLColor4( 1.f - color.mV[0], 1.f - color.mV[1], 1.f - color.mV[2], 1.f ),
					 LLFontGL::LEFT, LLFontGL::BOTTOM, 0, LLFontGL::NO_SHADOW, length, S32_MAX, &right_x, mEditor.allowsEmbeddedItems());
	}
	x = right_x;
	if( selection_end < seg_end )
	{
		// Draw normally
		S32 start = llmax( selection_end, seg_start );
		S32 end = seg_end;
		S32 length = end - start;
		font->render(text, start, x, y, color, LLFontGL::LEFT, LLFontGL::BOTTOM, 0, LLFontGL::NO_SHADOW, length, S32_MAX, &right_x, mEditor.allowsEmbeddedItems());
	}
	return right_x;
}

S32	LLNormalTextSegment::getMaxHeight() const	
{ 
	return mMaxHeight; 
}

BOOL LLNormalTextSegment::getToolTip(std::string& msg) const
{
	// do we have a tooltip for a loaded keyword (for script editor)?
	if (mToken && !mToken->getToolTip().empty())
	{
		const LLWString& wmsg = mToken->getToolTip();
		msg = wstring_to_utf8str(wmsg);
		return TRUE;
	}
	// or do we have an explicitly set tooltip (e.g., for Urls)
	if (! mTooltip.empty())
	{
		msg = mTooltip;
		return TRUE;
	}
	return FALSE;
}

void LLNormalTextSegment::setToolTip(const std::string& tooltip)
{
	// we cannot replace a keyword tooltip that's loaded from a file
	if (mToken)
	{
		llwarns << "LLTextSegment::setToolTip: cannot replace keyword tooltip." << llendl;
		return;
	}
	mTooltip = tooltip;
}

S32	LLNormalTextSegment::getWidth(S32 first_char, S32 num_chars) const
{
	LLWString text = mEditor.getWText();
	return mStyle->getFont()->getWidth(text.c_str(), mStart + first_char, num_chars);
}

S32	LLNormalTextSegment::getOffset(S32 segment_local_x_coord, S32 start_offset, S32 num_chars, bool round) const
{
	LLWString text = mEditor.getWText();
	return mStyle->getFont()->charFromPixelOffset(text.c_str(), mStart + start_offset,
											   (F32)segment_local_x_coord,
											   F32_MAX,
											   num_chars,
											   round);
}

S32	LLNormalTextSegment::getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const
{
	LLWString text = mEditor.getWText();
	S32 num_chars = mStyle->getFont()->maxDrawableChars(text.c_str() + segment_offset + mStart, 
												(F32)num_pixels,
												max_chars, 
												mEditor.getWordWrap());

	if (num_chars == 0 
		&& line_offset == 0 
		&& max_chars > 0)
	{
		// If at the beginning of a line, and a single character won't fit, draw it anyway
		num_chars = 1;
	}
	if (mStart + segment_offset + num_chars == mEditor.getLength())
	{
		// include terminating NULL
		num_chars++;
	}
	return num_chars;
}

void LLNormalTextSegment::dump() const
{
	llinfos << "Segment [" << 
//			mColor.mV[VX] << ", " <<
//			mColor.mV[VY] << ", " <<
//			mColor.mV[VZ] << "]\t[" <<
		mStart << ", " <<
		getEnd() << "]" <<
		llendl;
}

//////////////////////////////////////////////////////////////////////////
//
// LLTextBase
//

LLTextBase::LLTextBase(const LLUICtrl::Params &p) :
	mHoverSegment(NULL),
	mDefaultFont(p.font),
	mParseHTML(TRUE),
	mPopupMenu(NULL)
{
}

LLTextBase::~LLTextBase()
{
	clearSegments();
}

void LLTextBase::clearSegments()
{
	setHoverSegment(NULL);
	mSegments.clear();
}

void LLTextBase::setHoverSegment(LLTextSegmentPtr segment)
{
	if (mHoverSegment)
	{
		mHoverSegment->setHasMouseHover(false);
	}
	if (segment)
	{
		segment->setHasMouseHover(true);
	}
	mHoverSegment = segment;
}

void LLTextBase::getSegmentAndOffset( S32 startpos, segment_set_t::const_iterator* seg_iter, S32* offsetp ) const
{
	*seg_iter = getSegIterContaining(startpos);
	if (*seg_iter == mSegments.end())
	{
		*offsetp = 0;
	}
	else
	{
		*offsetp = startpos - (**seg_iter)->getStart();
	}
}

void LLTextBase::getSegmentAndOffset( S32 startpos, segment_set_t::iterator* seg_iter, S32* offsetp )
{
	*seg_iter = getSegIterContaining(startpos);
	if (*seg_iter == mSegments.end())
	{
		*offsetp = 0;
	}
	else
	{
		*offsetp = startpos - (**seg_iter)->getStart();
	}
}

LLTextBase::segment_set_t::iterator LLTextBase::getSegIterContaining(S32 index)
{
	segment_set_t::iterator it = mSegments.upper_bound(new LLIndexSegment(index));
	return it;
}

LLTextBase::segment_set_t::const_iterator LLTextBase::getSegIterContaining(S32 index) const
{
	LLTextBase::segment_set_t::const_iterator it =  mSegments.upper_bound(new LLIndexSegment(index));
	return it;
}

// Finds the text segment (if any) at the give local screen position
LLTextSegmentPtr LLTextBase::getSegmentAtLocalPos( S32 x, S32 y )
{
	// Find the cursor position at the requested local screen position
	S32 offset = getDocIndexFromLocalCoord( x, y, FALSE );
	segment_set_t::iterator seg_iter = getSegIterContaining(offset);
	if (seg_iter != mSegments.end())
	{
		return *seg_iter;
	}
	else
	{
		return LLTextSegmentPtr();
	}
}

BOOL LLTextBase::handleHoverOverUrl(S32 x, S32 y)
{
	setHoverSegment(NULL);

	// Check to see if we're over an HTML-style link
	LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
	if (cur_segment)
	{
		setHoverSegment(cur_segment);

		LLStyleSP style =  cur_segment->getStyle();
		if (style && style->isLink())
		{
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLTextBase::handleMouseUpOverUrl(S32 x, S32 y)
{
	if (mParseHTML && mHoverSegment)
	{
		LLStyleSP style = mHoverSegment->getStyle();
		if (style && style->isLink())
		{
			LLUrlAction::clickAction(style->getLinkHREF());
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLTextBase::handleRightMouseDownOverUrl(LLView *view, S32 x, S32 y)
{
	// pop up a context menu for any Url under the cursor
	const LLTextSegment* cur_segment = getSegmentAtLocalPos(x, y);
	if (cur_segment && cur_segment->getStyle() && cur_segment->getStyle()->isLink())
	{
		delete mPopupMenu;
		mPopupMenu = createUrlContextMenu(cur_segment->getStyle()->getLinkHREF());
		if (mPopupMenu)
		{
			mPopupMenu->show(x, y);
			LLMenuGL::showPopup(view, mPopupMenu, x, y);
			return TRUE;
		}
	}

	return FALSE;
}

BOOL LLTextBase::handleToolTipForUrl(LLView *view, S32 x, S32 y, std::string& msg, LLRect& sticky_rect_screen)
{
	std::string tooltip_msg;
	const LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
	if (cur_segment && cur_segment->getToolTip( tooltip_msg ) && view)
	{
		// Use a slop area around the cursor
		const S32 SLOP = 8;
		// Convert rect local to screen coordinates
		view->localPointToScreen(x - SLOP, y - SLOP, &(sticky_rect_screen.mLeft),
								 &(sticky_rect_screen.mBottom));
		sticky_rect_screen.mRight = sticky_rect_screen.mLeft + 2 * SLOP;
		sticky_rect_screen.mTop = sticky_rect_screen.mBottom + 2 * SLOP;

		LLToolTipMgr::instance().show(LLToolTipParams()
			.message(tooltip_msg)
			.sticky_rect(sticky_rect_screen));
		return TRUE;
	}
	return FALSE;
}

LLContextMenu *LLTextBase::createUrlContextMenu(const std::string &in_url)
{
	// work out the XUI menu file to use for this url
	LLUrlMatch match;
	std::string url = in_url;
	if (! LLUrlRegistry::instance().findUrl(url, match))
	{
		return NULL;
	}
	
	std::string xui_file = match.getMenuName();
	if (xui_file.empty())
	{
		return NULL;
	}

	// set up the callbacks for all of the potential menu items, N.B. we
	// don't use const ref strings in callbacks in case url goes out of scope
	LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registrar;
	registrar.add("Url.Open", boost::bind(&LLUrlAction::openURL, url));
	registrar.add("Url.OpenInternal", boost::bind(&LLUrlAction::openURLInternal, url));
	registrar.add("Url.OpenExternal", boost::bind(&LLUrlAction::openURLExternal, url));
	registrar.add("Url.Execute", boost::bind(&LLUrlAction::executeSLURL, url));
	registrar.add("Url.Teleport", boost::bind(&LLUrlAction::teleportToLocation, url));
	registrar.add("Url.CopyLabel", boost::bind(&LLUrlAction::copyLabelToClipboard, url));
	registrar.add("Url.CopyUrl", boost::bind(&LLUrlAction::copyURLToClipboard, url));

	// create and return the context menu from the XUI file
	return LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(xui_file, LLMenuGL::sMenuContainer,
																		 LLMenuHolderGL::child_registry_t::instance());	
}
