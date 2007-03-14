/** 
 * @file llviewertexteditor.cpp
 * @brief Text editor widget to let users enter a a multi-line ASCII document.
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llfocusmgr.h"
#include "audioengine.h"
#include "llagent.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"

#include "llviewertexteditor.h"

#include "llfloaterchat.h"
#include "llfloaterworldmap.h"
#include "llnotify.h"
#include "llpreview.h"
#include "llpreviewtexture.h"
#include "llpreviewnotecard.h"
#include "llpreviewlandmark.h"
#include "llscrollbar.h"
#include "lltooldraganddrop.h"
#include "llviewercontrol.h"
#include "llviewerimagelist.h"
#include "llviewerwindow.h"
#include "llviewerinventory.h"
#include "llnotecard.h"
#include "llmemorystream.h"

extern BOOL gPacificDaylightTime;

////////////////////////////////////////////////////////////
// LLEmbeddedItems
//
// Embedded items are stored as:
// * A global map of llwchar to LLInventoryItem
// ** This is unique for each item embedded in any notecard
//    to support copy/paste across notecards
// * A per-notecard set of embeded llwchars for easy removal
//   from the global list
// * A per-notecard vector of embedded lwchars for mapping from
//   old style 0x80 + item format notechards

class LLEmbeddedItems
{
public:
	LLEmbeddedItems(const LLViewerTextEditor* editor);
	~LLEmbeddedItems();
	void clear();

	// return true if there are no embedded items.
	bool empty();
	
	void	bindEmbeddedChars(const LLFontGL* font);
	void	unbindEmbeddedChars(const LLFontGL* font);

	BOOL	insertEmbeddedItem(LLInventoryItem* item, llwchar* value, bool is_new);
	BOOL	removeEmbeddedItem( llwchar ext_char );

	BOOL	hasEmbeddedItem(llwchar ext_char); // returns TRUE if /this/ editor has an entry for this item

	void	getEmbeddedItemList( std::vector<LLPointer<LLInventoryItem> >& items );
	void	addItems(const std::vector<LLPointer<LLInventoryItem> >& items);

	llwchar	getEmbeddedCharFromIndex(S32 index);

	void 	removeUnusedChars();
	void	copyUsedCharsToIndexed();
	S32		getIndexFromEmbeddedChar(llwchar wch);

	void	markSaved();
	
	static LLInventoryItem* getEmbeddedItem(llwchar ext_char); // returns item from static list
	static BOOL getEmbeddedItemSaved(llwchar ext_char); // returns whether item from static list is saved

	struct embedded_info_t
	{
		LLPointer<LLInventoryItem> mItem;
		BOOL mSaved;
	};
private:
	typedef std::map<llwchar, embedded_info_t > item_map_t;
	static item_map_t sEntries;
	static std::stack<llwchar> sFreeEntries;

	std::set<llwchar> mEmbeddedUsedChars;	 // list of used llwchars
	std::vector<llwchar> mEmbeddedIndexedChars; // index -> wchar for 0x80 + index format
	const LLViewerTextEditor* mEditor;
};

//statics
LLEmbeddedItems::item_map_t LLEmbeddedItems::sEntries;
std::stack<llwchar> LLEmbeddedItems::sFreeEntries;

LLEmbeddedItems::LLEmbeddedItems(const LLViewerTextEditor* editor)
	: mEditor(editor)
{
}

LLEmbeddedItems::~LLEmbeddedItems()
{
	clear();
}

void LLEmbeddedItems::clear()
{
	// Remove entries for this editor from static list
	for (std::set<llwchar>::iterator iter = mEmbeddedUsedChars.begin();
		 iter != mEmbeddedUsedChars.end();)
	{
		std::set<llwchar>::iterator nextiter = iter++;
		removeEmbeddedItem(*nextiter);
	}
	mEmbeddedUsedChars.clear();
	mEmbeddedIndexedChars.clear();
}

bool LLEmbeddedItems::empty()
{
	removeUnusedChars();
	return mEmbeddedUsedChars.empty();
}

// Inserts a new unique entry
BOOL LLEmbeddedItems::insertEmbeddedItem( LLInventoryItem* item, llwchar* ext_char, bool is_new)
{
	// Now insert a new one
	llwchar wc_emb;
	if (!sFreeEntries.empty())
	{
		wc_emb = sFreeEntries.top();
		sFreeEntries.pop();
	}
	else if (sEntries.empty())
	{
		wc_emb = FIRST_EMBEDDED_CHAR;
	}
	else
	{
		item_map_t::iterator last = sEntries.end();
		--last;
		wc_emb = last->first;
		if (wc_emb >= LAST_EMBEDDED_CHAR)
		{
			return FALSE;
		}
		++wc_emb;
	}

	sEntries[wc_emb].mItem = item;
	sEntries[wc_emb].mSaved = is_new ? FALSE : TRUE;
	*ext_char = wc_emb;
	mEmbeddedUsedChars.insert(wc_emb);
	return TRUE;
}

// Removes an entry (all entries are unique)
BOOL LLEmbeddedItems::removeEmbeddedItem( llwchar ext_char )
{
	mEmbeddedUsedChars.erase(ext_char);
	item_map_t::iterator iter = sEntries.find(ext_char);
	if (iter != sEntries.end())
	{
		sEntries.erase(ext_char);
		sFreeEntries.push(ext_char);
		return TRUE;
	}
	return FALSE;
}
	
// static
LLInventoryItem* LLEmbeddedItems::getEmbeddedItem(llwchar ext_char)
{
	if( ext_char >= FIRST_EMBEDDED_CHAR && ext_char <= LAST_EMBEDDED_CHAR )
	{
		item_map_t::iterator iter = sEntries.find(ext_char);
		if (iter != sEntries.end())
		{
			return iter->second.mItem;
		}
	}
	return NULL;
}

// static
BOOL LLEmbeddedItems::getEmbeddedItemSaved(llwchar ext_char)
{
	if( ext_char >= FIRST_EMBEDDED_CHAR && ext_char <= LAST_EMBEDDED_CHAR )
	{
		item_map_t::iterator iter = sEntries.find(ext_char);
		if (iter != sEntries.end())
		{
			return iter->second.mSaved;
		}
	}
	return FALSE;
}

llwchar	LLEmbeddedItems::getEmbeddedCharFromIndex(S32 index)
{
	if (index >= (S32)mEmbeddedIndexedChars.size())
	{
		llwarns << "No item for embedded char " << index << " using LL_UNKNOWN_CHAR" << llendl;
		return LL_UNKNOWN_CHAR;
	}
	return mEmbeddedIndexedChars[index];
}

void LLEmbeddedItems::removeUnusedChars()
{
	std::set<llwchar> used = mEmbeddedUsedChars;
	const LLWString& wtext = mEditor->getWText();
	for (S32 i=0; i<(S32)wtext.size(); i++)
	{
		llwchar wc = wtext[i];
		if( wc >= FIRST_EMBEDDED_CHAR && wc <= LAST_EMBEDDED_CHAR )
		{
			used.erase(wc);
		}
	}
	// Remove chars not actually used
	for (std::set<llwchar>::iterator iter = used.begin();
		 iter != used.end(); ++iter)
	{
		removeEmbeddedItem(*iter);
	}
}

void LLEmbeddedItems::copyUsedCharsToIndexed()
{
	// Prune unused items
	removeUnusedChars();

	// Copy all used llwchars to mEmbeddedIndexedChars
	mEmbeddedIndexedChars.clear();
	for (std::set<llwchar>::iterator iter = mEmbeddedUsedChars.begin();
		 iter != mEmbeddedUsedChars.end(); ++iter)
	{
		mEmbeddedIndexedChars.push_back(*iter);
	}
}

S32 LLEmbeddedItems::getIndexFromEmbeddedChar(llwchar wch)
{
	S32 idx = 0;
	for (std::vector<llwchar>::iterator iter = mEmbeddedIndexedChars.begin();
		 iter != mEmbeddedIndexedChars.end(); ++iter)
	{
		if (wch == *iter)
			break;
		++idx;
	}
	if (idx < (S32)mEmbeddedIndexedChars.size())
	{
		return idx;
	}
	else
	{
		llwarns << "Embedded char " << wch << " not found, using 0" << llendl;
		return 0;
	}
}

BOOL LLEmbeddedItems::hasEmbeddedItem(llwchar ext_char)
{
	std::set<llwchar>::iterator iter = mEmbeddedUsedChars.find(ext_char);
	if (iter != mEmbeddedUsedChars.end())
	{
		return TRUE;
	}
	return FALSE;
}

void LLEmbeddedItems::bindEmbeddedChars( const LLFontGL* font )
{
	if( sEntries.empty() )
	{
		return; 
	}

	for (std::set<llwchar>::iterator iter1 = mEmbeddedUsedChars.begin(); iter1 != mEmbeddedUsedChars.end(); ++iter1)
	{
		llwchar wch = *iter1;
		item_map_t::iterator iter2 = sEntries.find(wch);
		if (iter2 == sEntries.end())
		{
			continue;
		}
		LLInventoryItem* item = iter2->second.mItem;
		if (!item)
		{
			continue;
		}
		const char* img_name;
		switch( item->getType() )
		{
		  case LLAssetType::AT_TEXTURE:
			if(item->getInventoryType() == LLInventoryType::IT_SNAPSHOT)
			{
				img_name = "inv_item_snapshot.tga";
			}
			else
			{
				img_name = "inv_item_texture.tga";
			}

			break;
		  case LLAssetType::AT_SOUND:			img_name = "inv_item_sound.tga";	break;
		  case LLAssetType::AT_LANDMARK:		
			if (item->getFlags() & LLInventoryItem::II_FLAGS_LANDMARK_VISITED)
			{
				img_name = "inv_item_landmark_visited.tga";	
			}
			else
			{
				img_name = "inv_item_landmark.tga";	
			}
			break;
		  case LLAssetType::AT_CLOTHING:		img_name = "inv_item_clothing.tga";	break;
		  case LLAssetType::AT_OBJECT:			img_name = "inv_item_object.tga";	break;
		  case LLAssetType::AT_NOTECARD:		img_name = "inv_item_notecard.tga";	break;
		  case LLAssetType::AT_LSL_TEXT:		img_name = "inv_item_script.tga";	break;
		  case LLAssetType::AT_BODYPART:		img_name = "inv_item_bodypart.tga";	break;
		  case LLAssetType::AT_ANIMATION:		img_name = "inv_item_animation.tga";break;
		  case LLAssetType::AT_GESTURE:			img_name = "inv_item_gesture.tga";	break;
		  default: llassert(0); continue;
		}

		LLViewerImage* image = gImageList.getImage(LLUUID(gViewerArt.getString(img_name)), MIPMAP_FALSE, TRUE);

		((LLFontGL*)font)->addEmbeddedChar( wch, image, item->getName() );
	}
}

void LLEmbeddedItems::unbindEmbeddedChars( const LLFontGL* font )
{
	if( sEntries.empty() )
	{
		return; 
	}

	for (std::set<llwchar>::iterator iter1 = mEmbeddedUsedChars.begin(); iter1 != mEmbeddedUsedChars.end(); ++iter1)
	{
		((LLFontGL*)font)->removeEmbeddedChar(*iter1);
	}
}

void LLEmbeddedItems::addItems(const std::vector<LLPointer<LLInventoryItem> >& items)
{
	for (std::vector<LLPointer<LLInventoryItem> >::const_iterator iter = items.begin();
		 iter != items.end(); ++iter)
	{
		LLInventoryItem* item = *iter;
		if (item)
		{
			llwchar wc;
			if (!insertEmbeddedItem( item, &wc, false ))
			{
				break;
			}
			mEmbeddedIndexedChars.push_back(wc);
		}
	}
}

void LLEmbeddedItems::getEmbeddedItemList( std::vector<LLPointer<LLInventoryItem> >& items )
{
	for (std::set<llwchar>::iterator iter = mEmbeddedUsedChars.begin(); iter != mEmbeddedUsedChars.end(); ++iter)
	{
		llwchar wc = *iter;
		LLPointer<LLInventoryItem> item = getEmbeddedItem(wc);
		if (item)
		{
			items.push_back(item);
		}
	}
}

void LLEmbeddedItems::markSaved()
{
	for (std::set<llwchar>::iterator iter = mEmbeddedUsedChars.begin(); iter != mEmbeddedUsedChars.end(); ++iter)
	{
		llwchar wc = *iter;
		sEntries[wc].mSaved = TRUE;
	}
}

///////////////////////////////////////////////////////////////////

class LLTextCmdInsertEmbeddedItem : public LLTextCmd
{
public:
	LLTextCmdInsertEmbeddedItem( S32 pos, LLInventoryItem* item )
		: LLTextCmd(pos, FALSE), 
		  mExtCharValue(0)
	{
		mItem = item;
	}

	virtual BOOL execute( LLTextEditor* editor, S32* delta )
	{
		LLViewerTextEditor* viewer_editor = (LLViewerTextEditor*)editor;
		// Take this opportunity to remove any unused embedded items from this editor
		viewer_editor->mEmbeddedItemList->removeUnusedChars();
		if(viewer_editor->mEmbeddedItemList->insertEmbeddedItem( mItem, &mExtCharValue, true ) )
		{
			LLWString ws;
			ws.assign(1, mExtCharValue);
			*delta = insert(editor, mPos, ws );
			return (*delta != 0);
		}
		return FALSE;
	}
	
	virtual S32 undo( LLTextEditor* editor )
	{
		remove(editor, mPos, 1);
		return mPos; 
	}
	
	virtual S32 redo( LLTextEditor* editor )
	{ 
		LLWString ws;
		ws += mExtCharValue;
		insert(editor, mPos, ws );
		return mPos + 1;
	}
	virtual BOOL hasExtCharValue( llwchar value )
	{
		return (value == mExtCharValue);
	}

private:
	LLPointer<LLInventoryItem> mItem;
	llwchar mExtCharValue;
};

struct LLNotecardCopyInfo
{
	LLNotecardCopyInfo(LLViewerTextEditor *ed, LLInventoryItem *item)
		: mTextEd(ed)
	{
		mItem = item;
	}

	LLViewerTextEditor* mTextEd;
	// need to make this be a copy (not a * here) because it isn't stable.
	// I wish we had passed LLPointers all the way down, but we didn't
	LLPointer<LLInventoryItem> mItem;
};

//----------------------------------------------------------------------------

//
// Member functions
//

LLViewerTextEditor::LLViewerTextEditor(const LLString& name, 
									   const LLRect& rect, 
									   S32 max_length, 
									   const LLString& default_text, 
									   const LLFontGL* font,
									   BOOL allow_embedded_items)
	: LLTextEditor(name, rect, max_length, default_text, font, allow_embedded_items),
	  mDragItemSaved(FALSE)
{
	mEmbeddedItemList = new LLEmbeddedItems(this);
}

LLViewerTextEditor::~LLViewerTextEditor()
{
	delete mEmbeddedItemList;
}

///////////////////////////////////////////////////////////////////
// virtual
void LLViewerTextEditor::makePristine()
{
	mEmbeddedItemList->markSaved();
	LLTextEditor::makePristine();
}

///////////////////////////////////////////////////////////////////

BOOL LLViewerTextEditor::handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect_screen)
{
	if (pointInView(x, y) && getVisible())
	{
		for (child_list_const_iter_t child_iter = getChildList()->begin();
			 child_iter != getChildList()->end(); ++child_iter)
		{
			LLView *viewp = *child_iter;
			S32 local_x = x - viewp->getRect().mLeft;
			S32 local_y = y - viewp->getRect().mBottom;
			if( viewp->handleToolTip(local_x, local_y, msg, sticky_rect_screen ) )
			{
				return TRUE;
			}
		}

		if( mSegments.empty() )
		{
			return TRUE;
		}

		LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
		if( cur_segment )
		{
			BOOL has_tool_tip = FALSE;
			if( cur_segment->getStyle().getIsEmbeddedItem() )
			{
				LLWString wtip;
				has_tool_tip = getEmbeddedItemToolTipAtPos(cur_segment->getStart(), wtip);
				msg = wstring_to_utf8str(wtip);
			}
			else
			{
				has_tool_tip = cur_segment->getToolTip( msg );
			}
			if( has_tool_tip )
			{
				// Just use a slop area around the cursor
				// Convert rect local to screen coordinates
				S32 SLOP = 8;
				localPointToScreen( 
					x - SLOP, y - SLOP, 
					&(sticky_rect_screen->mLeft), &(sticky_rect_screen->mBottom) );
				sticky_rect_screen->mRight = sticky_rect_screen->mLeft + 2 * SLOP;
				sticky_rect_screen->mTop = sticky_rect_screen->mBottom + 2 * SLOP;
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL LLViewerTextEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// Let scrollbar have first dibs
	handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;

	// enable I Agree checkbox if the user scrolled through entire text
	BOOL was_scrolled_to_bottom = (mScrollbar->getDocPos() == mScrollbar->getDocPosMax());
	if (mOnScrollEndCallback && was_scrolled_to_bottom)
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}

	if( !handled && mTakesNonScrollClicks)
	{
		if (!(mask & MASK_SHIFT))
		{
			deselect();
		}

		BOOL start_select = TRUE;
		if( mAllowEmbeddedItems )
		{
			setCursorAtLocalPos( x, y, FALSE );
			llwchar wc = 0;
			if (mCursorPos < getLength())
			{
				wc = mWText[mCursorPos];
			}
			LLInventoryItem* item_at_pos = LLEmbeddedItems::getEmbeddedItem(wc);
			if (item_at_pos)
			{
				mDragItem = item_at_pos;
				mDragItemSaved = LLEmbeddedItems::getEmbeddedItemSaved(wc);
				gFocusMgr.setMouseCapture( this, NULL );
				mMouseDownX = x;
				mMouseDownY = y;
				S32 screen_x;
				S32 screen_y;
				localPointToScreen(x, y, &screen_x, &screen_y );
				gToolDragAndDrop->setDragStart( screen_x, screen_y );

				start_select = FALSE;
			}
			else
			{
				mDragItem = NULL;
			}
		}

		if( start_select )
		{
			// If we're not scrolling (handled by child), then we're selecting
			if (mask & MASK_SHIFT)
			{
				S32 old_cursor_pos = mCursorPos;
				setCursorAtLocalPos( x, y, TRUE );

				if (hasSelection())
				{
					/* Mac-like behavior - extend selection towards the cursor
					if (mCursorPos < mSelectionStart
						&& mCursorPos < mSelectionEnd)
					{
						// ...left of selection
						mSelectionStart = llmax(mSelectionStart, mSelectionEnd);
						mSelectionEnd = mCursorPos;
					}
					else if (mCursorPos > mSelectionStart
						&& mCursorPos > mSelectionEnd)
					{
						// ...right of selection
						mSelectionStart = llmin(mSelectionStart, mSelectionEnd);
						mSelectionEnd = mCursorPos;
					}
					else
					{
						mSelectionEnd = mCursorPos;
					}
					*/
					// Windows behavior
					mSelectionEnd = mCursorPos;
				}
				else
				{
					mSelectionStart = old_cursor_pos;
					mSelectionEnd = mCursorPos;
				}
				// assume we're starting a drag select
				mIsSelecting = TRUE;
			}
			else
			{
				setCursorAtLocalPos( x, y, TRUE );
				startSelection();
			}
			gFocusMgr.setMouseCapture( this, &LLTextEditor::onMouseCaptureLost );
		}

		handled = TRUE;
	}

	if (mTakesFocus)
	{
		setFocus( TRUE );
		handled = TRUE;
	}

	// Delay cursor flashing
	mKeystrokeTimer.reset();

	return handled;
}


BOOL LLViewerTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if (!mDragItem)
	{
		// leave hover segment active during drag and drop
		mHoverSegment = NULL;
	}
	if( getVisible() )
	{
		if(gFocusMgr.getMouseCapture() == this )
		{
			if( mIsSelecting ) 
			{
				if (x != mLastSelectionX || y != mLastSelectionY)
				{
					mLastSelectionX = x;
					mLastSelectionY = y;
				}

				if( y > mTextRect.mTop )
				{
					mScrollbar->setDocPos( mScrollbar->getDocPos() - 1 );
				}
				else
				if( y < mTextRect.mBottom )
				{
					mScrollbar->setDocPos( mScrollbar->getDocPos() + 1 );
				}

				setCursorAtLocalPos( x, y, TRUE );
				mSelectionEnd = mCursorPos;
				
				updateScrollFromCursor();
				getWindow()->setCursor(UI_CURSOR_IBEAM);
			}
			else if( mDragItem )
			{
				S32 screen_x;
				S32 screen_y;
				localPointToScreen(x, y, &screen_x, &screen_y );
				if( gToolDragAndDrop->isOverThreshold( screen_x, screen_y ) )
				{
					gToolDragAndDrop->beginDrag(
						LLAssetType::lookupDragAndDropType( mDragItem->getType() ),
						mDragItem->getUUID(),
						LLToolDragAndDrop::SOURCE_NOTECARD,
						mSourceID, mObjectID);

					return gToolDragAndDrop->handleHover( x, y, mask );
				}
				getWindow()->setCursor(UI_CURSOR_HAND);
			}

			lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (active)" << llendl;		
			handled = TRUE;
		}

		if( !handled )
		{
			// Pass to children
			handled = LLView::childrenHandleHover(x, y, mask) != NULL;
		}

		if( handled )
		{
			// Delay cursor flashing
			mKeystrokeTimer.reset();
		}
	
		// Opaque
		if( !handled && mTakesNonScrollClicks)
		{
			// Check to see if we're over an HTML-style link
			if( !mSegments.empty() )
			{
				LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
				if( cur_segment )
				{
					if(cur_segment->getStyle().isLink())
					{
						lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over link, inactive)" << llendl;		
						getWindow()->setCursor(UI_CURSOR_HAND);
						handled = TRUE;
					}
					else
					if(cur_segment->getStyle().getIsEmbeddedItem())
					{
						lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (over embedded item, inactive)" << llendl;		
						getWindow()->setCursor(UI_CURSOR_HAND);
						//getWindow()->setCursor(UI_CURSOR_ARROW);
						handled = TRUE;
					}
					mHoverSegment = cur_segment;
				}
			}

			if( !handled )
			{
				lldebugst(LLERR_USER_INPUT) << "hover handled by " << getName() << " (inactive)" << llendl;		
				if (!mScrollbar->getVisible() || x < mRect.getWidth() - SCROLLBAR_SIZE)
				{
					getWindow()->setCursor(UI_CURSOR_IBEAM);
				}
				else
				{
					getWindow()->setCursor(UI_CURSOR_ARROW);
				}
				handled = TRUE;
			}
		}
	}

	return handled;
}


BOOL LLViewerTextEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// let scrollbar have first dibs
	handled = LLView::childrenHandleMouseUp(x, y, mask) != NULL;

	// enable I Agree checkbox if the user scrolled through entire text
	BOOL was_scrolled_to_bottom = (mScrollbar->getDocPos() == mScrollbar->getDocPosMax());
	if (mOnScrollEndCallback && was_scrolled_to_bottom)
	{
		mOnScrollEndCallback(mOnScrollEndData);
	}

	if( !handled && mTakesNonScrollClicks)
	{
		if( mIsSelecting )
		{
			// Finish selection
			if( y > mTextRect.mTop )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() - 1 );
			}
			else
			if( y < mTextRect.mBottom )
			{
				mScrollbar->setDocPos( mScrollbar->getDocPos() + 1 );
			}
			
			setCursorAtLocalPos( x, y, TRUE );
			endSelection();

			updateScrollFromCursor();
		}
		
		if( !hasSelection() )
		{
			handleMouseUpOverSegment( x, y, mask );
		}

		handled = TRUE;
	}

	// Delay cursor flashing
	mKeystrokeTimer.reset();

	if( gFocusMgr.getMouseCapture() == this  )
	{
		if (mDragItem)
		{
			// mouse down was on an item
			S32 dx = x - mMouseDownX;
			S32 dy = y - mMouseDownY;
			if (-2 < dx && dx < 2 && -2 < dy && dy < 2)
			{
				openEmbeddedItem(mDragItem, mDragItemSaved);
			}
		}
		mDragItem = NULL;
		gFocusMgr.setMouseCapture( NULL, NULL );
		handled = TRUE;
	}

	return handled;
}


BOOL LLViewerTextEditor::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// let scrollbar have first dibs
	handled = LLView::childrenHandleDoubleClick(x, y, mask) != NULL;

	if( !handled && mTakesNonScrollClicks)
	{
		if( mAllowEmbeddedItems )
		{
			LLTextSegment* cur_segment = getSegmentAtLocalPos( x, y );
			if( cur_segment && cur_segment->getStyle().getIsEmbeddedItem() )
			{
				if( openEmbeddedItemAtPos( cur_segment->getStart() ) )
				{
					deselect();
					setFocus( FALSE );
					return TRUE;
				}
			}
		}

		if (mTakesFocus)
		{
			setFocus( TRUE );
		}
		
		setCursorAtLocalPos( x, y, FALSE );
		deselect();

		const LLWString &text = getWText();
		
		if( isPartOfWord( text[mCursorPos] ) )
		{
			// Select word the cursor is over
			while ((mCursorPos > 0) && isPartOfWord(text[mCursorPos-1]))
			{
				mCursorPos--;
			}
			startSelection();

			while ((mCursorPos < (S32)text.length()) && isPartOfWord( text[mCursorPos] ) )
			{
				mCursorPos++;
			}
		
			mSelectionEnd = mCursorPos;
		}
		else if ((mCursorPos < (S32)text.length()) && !iswspace( text[mCursorPos]) )
		{
			// Select the character the cursor is over
			startSelection();
			mCursorPos++;
			mSelectionEnd = mCursorPos;
		}

		// We don't want handleMouseUp() to "finish" the selection (and thereby
		// set mSelectionEnd to where the mouse is), so we finish the selection here.
		mIsSelecting = FALSE;  

		// delay cursor flashing
		mKeystrokeTimer.reset();

		handled = TRUE;
	}
	return handled;
}


// Allow calling cards to be dropped onto text fields.  Append the name and
// a carriage return.
// virtual
BOOL LLViewerTextEditor::handleDragAndDrop(S32 x, S32 y, MASK mask,
					  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
					  EAcceptance *accept,
					  LLString& tooltip_msg)
{
	BOOL handled = FALSE;

	if (mTakesNonScrollClicks)
	{
		if (getEnabled() && !mReadOnly)
		{
			switch( cargo_type )
			{
			case DAD_CALLINGCARD:
				if(mAcceptCallingCardNames)
				{
					if (drop)
					{
						LLInventoryItem *item = (LLInventoryItem *)cargo_data;
						LLString name = item->getName();
						appendText(name, true, true);
					}
					*accept = ACCEPT_YES_COPY_SINGLE;
				}
				else
				{
					*accept = ACCEPT_NO;
				}
				break;

			case DAD_TEXTURE:
			case DAD_SOUND:
			case DAD_LANDMARK:
			case DAD_SCRIPT:
			case DAD_CLOTHING:
			case DAD_OBJECT:
			case DAD_NOTECARD:
			case DAD_BODYPART:
			case DAD_ANIMATION:
			case DAD_GESTURE:
				{
					LLInventoryItem *item = (LLInventoryItem *)cargo_data;
					if( mAllowEmbeddedItems )
					{
						U32 mask_next = item->getPermissions().getMaskNextOwner();
						if((mask_next & PERM_ITEM_UNRESTRICTED) == PERM_ITEM_UNRESTRICTED)
						{
							if( drop )
							{
								deselect();
								S32 old_cursor = mCursorPos;
								setCursorAtLocalPos( x, y, TRUE );
								S32 insert_pos = mCursorPos;
								setCursorPos(old_cursor);
								BOOL inserted = insertEmbeddedItem( insert_pos, item );
								if( inserted && (old_cursor > mCursorPos) )
								{
									setCursorPos(mCursorPos + 1);
								}

								updateLineStartList();
							}
							*accept = ACCEPT_YES_COPY_MULTI;
						}
						else
						{
							*accept = ACCEPT_NO;
							if (tooltip_msg.empty())
							{
								tooltip_msg.assign("Only items with unrestricted\n"
													"'next owner' permissions \n"
													"can be attached to notecards.");
							}
						}
					}
					else
					{
						*accept = ACCEPT_NO;
					}
					break;
				}

			default:
				*accept = ACCEPT_NO;
				break;
			}
		}
		else
		{
			// Not enabled
			*accept = ACCEPT_NO;
		}

		handled = TRUE;
		lldebugst(LLERR_USER_INPUT) << "dragAndDrop handled by LLViewerTextEditor " << getName() << llendl;
	}

	return handled;
}

void LLViewerTextEditor::setASCIIEmbeddedText(const LLString& instr)
{
	LLWString wtext;
	const U8* buffer = (U8*)(instr.c_str());
	while (*buffer)
	{
		llwchar wch;
		U8 c = *buffer++;
		if (c >= 0x80)
		{
			S32 index = (S32)(c - 0x80);
			wch = mEmbeddedItemList->getEmbeddedCharFromIndex(index);
		}
		else
		{
			wch = (llwchar)c;
		}
		wtext.push_back(wch);
	}
	setWText(wtext);
}

void LLViewerTextEditor::setEmbeddedText(const LLString& instr)
{
	LLWString wtext = utf8str_to_wstring(instr);
	for (S32 i=0; i<(S32)wtext.size(); i++)
	{
		llwchar wch = wtext[i];
		if( wch >= FIRST_EMBEDDED_CHAR && wch <= LAST_EMBEDDED_CHAR )
		{
			S32 index = wch - FIRST_EMBEDDED_CHAR;
			wtext[i] = mEmbeddedItemList->getEmbeddedCharFromIndex(index);
		}
	}
	setWText(wtext);
}

LLString LLViewerTextEditor::getEmbeddedText()
{
#if 1
	// New version (Version 2)
	mEmbeddedItemList->copyUsedCharsToIndexed();
	LLWString outtextw;
	for (S32 i=0; i<(S32)mWText.size(); i++)
	{
		llwchar wch = mWText[i];
		if( wch >= FIRST_EMBEDDED_CHAR && wch <= LAST_EMBEDDED_CHAR )
		{
			S32 index = mEmbeddedItemList->getIndexFromEmbeddedChar(wch);
			wch = FIRST_EMBEDDED_CHAR + index;
		}
		outtextw.push_back(wch);
	}
	LLString outtext = wstring_to_utf8str(outtextw);
	return outtext;
#else
	// Old version (Version 1)
	mEmbeddedItemList->copyUsedCharsToIndexed();
	LLString outtext;
	for (S32 i=0; i<(S32)mWText.size(); i++)
	{
		llwchar wch = mWText[i];
		if( wch >= FIRST_EMBEDDED_CHAR && wch <= LAST_EMBEDDED_CHAR )
		{
			S32 index = mEmbeddedItemList->getIndexFromEmbeddedChar(wch);
			wch = 0x80 | index % 128;
		}
		else if (wch >= 0x80)
		{
			wch = LL_UNKNOWN_CHAR;
		}
		outtext.push_back((U8)wch);
	}
	return outtext;
#endif
}

LLString LLViewerTextEditor::appendTime(bool prepend_newline)
{
	U32 utc_time;
	utc_time = time_corrected();

	// There's only one internal tm buffer.
	struct tm* timep;

	// Convert to Pacific, based on server's opinion of whether
	// it's daylight savings time there.
	timep = utc_to_pacific_time(utc_time, gPacificDaylightTime);

	LLString text = llformat("[%d:%02d]  ", timep->tm_hour, timep->tm_min);
	appendColoredText(text, false, prepend_newline, LLColor4::grey);

	return text;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

llwchar LLViewerTextEditor::pasteEmbeddedItem(llwchar ext_char)
{
	if (mEmbeddedItemList->hasEmbeddedItem(ext_char))
	{
		return ext_char; // already exists in my list
	}
	LLInventoryItem* item = LLEmbeddedItems::getEmbeddedItem(ext_char);
	if (item)
	{
		// Add item to my list and return new llwchar associated with it
		llwchar new_wc;
		if (mEmbeddedItemList->insertEmbeddedItem( item, &new_wc, true ))
		{
			return new_wc;
		}
	}
	return LL_UNKNOWN_CHAR; // item not found or list full
}

void LLViewerTextEditor::bindEmbeddedChars(const LLFontGL* font)
{
	mEmbeddedItemList->bindEmbeddedChars( font );
}

void LLViewerTextEditor::unbindEmbeddedChars(const LLFontGL* font)
{
	mEmbeddedItemList->unbindEmbeddedChars( font );
}

BOOL LLViewerTextEditor::getEmbeddedItemToolTipAtPos(S32 pos, LLWString &msg)
{
	if (pos < getLength())
	{
		LLInventoryItem* item = LLEmbeddedItems::getEmbeddedItem(mWText[pos]);
		if( item )
		{
			msg = utf8str_to_wstring(item->getName());
			msg += '\n';
			msg += utf8str_to_wstring(item->getDescription());
			return TRUE;
		}
	}
	return FALSE;
}


BOOL LLViewerTextEditor::openEmbeddedItemAtPos(S32 pos)
{
	if( pos < getLength())
	{
		LLInventoryItem* item = LLEmbeddedItems::getEmbeddedItem( mWText[pos] );
		if( item )
		{
			BOOL saved = LLEmbeddedItems::getEmbeddedItemSaved( mWText[pos] );
			return openEmbeddedItem(item, saved);
		}
	}
	return FALSE;
}


BOOL LLViewerTextEditor::openEmbeddedItem(LLInventoryItem* item, BOOL saved)
{
	switch( item->getType() )
	{
	case LLAssetType::AT_TEXTURE:
		openEmbeddedTexture( item );
		return TRUE;

	case LLAssetType::AT_SOUND:
		openEmbeddedSound( item );
		return TRUE;
	
	case LLAssetType::AT_NOTECARD:
	    openEmbeddedNotecard( item, saved );
		return TRUE;

	case LLAssetType::AT_LANDMARK:
		showLandmarkDialog( item );
		return TRUE;
		
	case LLAssetType::AT_LSL_TEXT:
	case LLAssetType::AT_CLOTHING:
	case LLAssetType::AT_OBJECT:
	case LLAssetType::AT_BODYPART:
	case LLAssetType::AT_ANIMATION:
	case LLAssetType::AT_GESTURE:
		showCopyToInvDialog( item );
		return TRUE;	
	default:
		return FALSE;
	}
}


void LLViewerTextEditor::openEmbeddedTexture( LLInventoryItem* item )
{
	// See if we can bring an existing preview to the front
	if( !LLPreview::show( item->getUUID() ) )
	{
		// There isn't one, so make a new preview
		if(item)
		{
			S32 left, top;
			gFloaterView->getNewFloaterPosition(&left, &top);
			LLRect rect = gSavedSettings.getRect("PreviewTextureRect");
			rect.translate( left - rect.mLeft, top - rect.mTop );

			LLPreviewTexture* preview = new LLPreviewTexture("preview texture",
				rect,
				item->getName(),
				item->getAssetUUID(),
				TRUE);
			preview->setAuxItem( item );
			preview->setNotecardInfo(mNotecardInventoryID, mObjectID);
		}
	}
}

void LLViewerTextEditor::openEmbeddedSound( LLInventoryItem* item )
{
	// Play sound locally
	LLVector3d lpos_global = gAgent.getPositionGlobal();
	const F32 SOUND_GAIN = 1.0f;
	if(gAudiop)
	{
		gAudiop->triggerSound(
			item->getAssetUUID(), gAgentID, SOUND_GAIN, lpos_global);
	}
	showCopyToInvDialog( item );
}

/*
void LLViewerTextEditor::openEmbeddedLandmark( LLInventoryItem* item )
{
	// See if we can bring an existing preview to the front
	if( !LLPreview::show( item->getUUID() ) )
	{
		// There isn't one, so make a new preview
		S32 left, top;
		gFloaterView->getNewFloaterPosition(&left, &top);
		LLRect rect = gSavedSettings.getRect("PreviewLandmarkRect");
		rect.translate( left - rect.mLeft, top - rect.mTop );

		LLPreviewLandmark* preview = new LLPreviewLandmark(
			"preview landmark",
			rect,
			item->getName(),
			item->getUUID());
		preview->setAuxItem( item );
		preview->addCopyToInvButton();
		preview->open();
	}
}*/

void LLViewerTextEditor::openEmbeddedNotecard( LLInventoryItem* item, BOOL saved )
{
	if (saved)
	{
		// Copy to inventory
		copyInventory(item);
	}
	else
	{
		LLNotecardCopyInfo *info = new LLNotecardCopyInfo(this, item);
		gViewerWindow->alertXml("ConfirmNotecardSave",		
								LLViewerTextEditor::onNotecardDialog, (void*)info);
	}
}

// static
void LLViewerTextEditor::onNotecardDialog( S32 option, void* userdata )
{
	LLNotecardCopyInfo *info = (LLNotecardCopyInfo *)userdata;
	if( option == 0 )
	{
		// itemptr is deleted by LLPreview::save
		LLPointer<LLInventoryItem>* itemptr = new LLPointer<LLInventoryItem>(info->mItem);
		LLPreview::save( info->mTextEd->mNotecardInventoryID, itemptr);
	}
}


void LLViewerTextEditor::showLandmarkDialog( LLInventoryItem* item )
{
	LLNotecardCopyInfo *info = new LLNotecardCopyInfo(this, item);
	gViewerWindow->alertXml("ConfirmLandmarkCopy",		
		LLViewerTextEditor::onLandmarkDialog, (void*)info);
}

// static
void LLViewerTextEditor::onLandmarkDialog( S32 option, void* userdata )
{
	LLNotecardCopyInfo *info = (LLNotecardCopyInfo *)userdata;
	if( option == 0 )
	{
		// Copy to inventory
		info->mTextEd->copyInventory(info->mItem);
		/*
		 * XXXPAM
		 *
		 * Yes, this is broken.  We don't show the map yet.
		 *
		LLInventoryItem* orig_item = (LLInventoryItem*)userdata;

		// Copy to inventory
		LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem;
		cloneInventoryItemToViewer(orig_item, new_item);
		U32 flags = new_item->getFlags();
		flags &= ~LLInventoryItem::II_FLAGS_LANDMARK_VISITED;
		new_item->setFlags(flags);
		new_item->updateServer(TRUE);
		gInventory.updateItem(new_item);
		gInventory.notifyObservers();

		LLInventoryView* view = LLInventoryView::getActiveInventory();
		if(view)
		{
			view->getPanel()->setSelection(new_item->getUUID(), TAKE_FOCUS_NO);
		}

		if( (0 == option) && gFloaterWorldMap )
		{
			// Note: there's a minor race condition here.
			// If the user immediately tries to teleport to the landmark, the dataserver may
			// not yet know that the user has the landmark in his inventory and so may 
			// disallow the teleport.  However, the user will need to be pretty fast to make
			// this happen, and, if it does, they haven't lost anything.  Once the dataserver
			// knows about the new item, the user will be able to teleport to it successfully.
			gFloaterWorldMap->trackLandmark(new_item->getUUID());
			LLFloaterWorldMap::show(NULL, TRUE);
		}*/
	}
	delete info;
}


void LLViewerTextEditor::showCopyToInvDialog( LLInventoryItem* item )
{
	LLNotecardCopyInfo *info = new LLNotecardCopyInfo(this, item);
	gViewerWindow->alertXml( "ConfirmItemCopy",
		LLViewerTextEditor::onCopyToInvDialog, (void*)info);
}

// static
void LLViewerTextEditor::onCopyToInvDialog( S32 option, void* userdata )
{
	LLNotecardCopyInfo *info = (LLNotecardCopyInfo *)userdata;
	if( 0 == option )
	{
		info->mTextEd->copyInventory(info->mItem);
	}
	delete info;
}



// Returns change in number of characters in mWText
S32 LLViewerTextEditor::insertEmbeddedItem( S32 pos, LLInventoryItem* item )
{
	return execute( new LLTextCmdInsertEmbeddedItem( pos, item ) );
}

bool LLViewerTextEditor::importStream(std::istream& str)
{
	LLNotecard nc(LLNotecard::MAX_SIZE);
	bool success = nc.importStream(str);
	if (success)
	{
		mEmbeddedItemList->clear();
		const std::vector<LLPointer<LLInventoryItem> >& items = nc.getItems();
		mEmbeddedItemList->addItems(items);
		// Actually set the text
		if (mAllowEmbeddedItems)
		{
			if (nc.getVersion() == 1)
				setASCIIEmbeddedText( nc.getText() );
			else
				setEmbeddedText( nc.getText() );
		}
		else
		{
			setText( nc.getText() );
		}
	}
	return success;
}

void LLViewerTextEditor::copyInventory(LLInventoryItem* item)
{
	copy_inventory_from_notecard(mObjectID,
								 mNotecardInventoryID,
								 item);
}

bool LLViewerTextEditor::hasEmbeddedInventory()
{
	return (!(mEmbeddedItemList->empty()));
}

////////////////////////////////////////////////////////////////////////////

BOOL LLViewerTextEditor::importBuffer( const LLString& buffer )
{
	LLMemoryStream str((U8*)buffer.c_str(), buffer.length());
	return importStream(str);
}

BOOL LLViewerTextEditor::exportBuffer( LLString& buffer )
{
	LLNotecard nc(LLNotecard::MAX_SIZE);

	std::vector<LLPointer<LLInventoryItem> > embedded_items;
	mEmbeddedItemList->getEmbeddedItemList(embedded_items);
	
	nc.setItems(embedded_items);
	nc.setText(getEmbeddedText());

	std::stringstream out_stream;
	nc.exportStream(out_stream);
	
	buffer = out_stream.str();
	
	return TRUE;
}

LLView* LLViewerTextEditor::fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory)
{
	LLString name("text_editor");
	node->getAttributeString("name", name);

	LLRect rect;
	createRect(node, rect, parent, LLRect());

	U32 max_text_length = 255;
	node->getAttributeU32("max_length", max_text_length);

	BOOL allow_embedded_items = FALSE;
	node->getAttributeBOOL("embedded_items", allow_embedded_items);

	LLFontGL* font = LLView::selectFont(node);

	LLString text = node->getValue();

	if (text.size() > max_text_length)
	{
		// Erase everything from max_text_length on.
		text.erase(max_text_length);
	}

	LLViewerTextEditor* text_editor = new LLViewerTextEditor(name, 
								rect,
								max_text_length,
								text,
								font,
								allow_embedded_items);

	BOOL ignore_tabs = text_editor->mTabToNextField;
	node->getAttributeBOOL("ignore_tab", ignore_tabs);

	text_editor->setTabToNextField(ignore_tabs);


	text_editor->setTextEditorParameters(node);

	BOOL hide_scrollbar = FALSE;
	node->getAttributeBOOL("hide_scrollbar",hide_scrollbar);
	text_editor->setHideScrollbarForShortDocs(hide_scrollbar);

	text_editor->initFromXML(node, parent);

	return text_editor;
}
