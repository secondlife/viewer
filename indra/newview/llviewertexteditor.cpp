/** 
 * @file llviewertexteditor.cpp
 * @brief Text editor widget to let users enter a multi-line document.
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

#include "llviewertexteditor.h"

#include "llagent.h"
#include "llaudioengine.h"
#include "llavataractions.h"
#include "llfloaterreg.h"
#include "llfloatersidepanelcontainer.h"
#include "llfloaterworldmap.h"
#include "llfocusmgr.h"
#include "llinventorybridge.h"
#include "llinventorydefines.h"
#include "llinventorymodel.h"
#include "lllandmark.h"
#include "lllandmarkactions.h"
#include "lllandmarklist.h"
#include "llmemorystream.h"
#include "llmenugl.h"
#include "llnotecard.h"
#include "llnotificationsutil.h"
#include "llpanelplaces.h"
#include "llpreview.h"
#include "llpreviewnotecard.h"
#include "llpreviewtexture.h"
#include "llscrollbar.h"
#include "llscrollcontainer.h"
#include "lltooldraganddrop.h"
#include "lltooltip.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llviewerassettype.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewertexturelist.h"
#include "llviewerwindow.h"

static LLDefaultChildRegistry::Register<LLViewerTextEditor> r("text_editor");

///-----------------------------------------------------------------------
///  Class LLEmbeddedLandmarkCopied
///-----------------------------------------------------------------------
class LLEmbeddedLandmarkCopied: public LLInventoryCallback
{
public:

	LLEmbeddedLandmarkCopied(){}
	void fire(const LLUUID& inv_item)
	{
		showInfo(inv_item);
	}
	static void showInfo(const LLUUID& landmark_inv_id)
	{
		LLSD key;
		key["type"] = "landmark";
		key["id"] = landmark_inv_id;
		LLFloaterSidePanelContainer::showPanel("places", key);
	}
	static void processForeignLandmark(LLLandmark* landmark,
			const LLUUID& object_id, const LLUUID& notecard_inventory_id,
			LLPointer<LLInventoryItem> item_ptr)
	{
		LLVector3d global_pos;
		landmark->getGlobalPos(global_pos);
		LLViewerInventoryItem* agent_landmark =
				LLLandmarkActions::findLandmarkForGlobalPos(global_pos);

		if (agent_landmark)
		{
			showInfo(agent_landmark->getUUID());
		}
		else
		{
			if (item_ptr.isNull())
			{
				// check to prevent a crash. See EXT-8459.
				llwarns << "Passed handle contains a dead inventory item. Most likely notecard has been closed and embedded item was destroyed." << llendl;
			}
			else
			{
				LLInventoryItem* item = item_ptr.get();
				LLPointer<LLEmbeddedLandmarkCopied> cb = new LLEmbeddedLandmarkCopied();
				copy_inventory_from_notecard(get_folder_by_itemtype(item),
											 object_id,
											 notecard_inventory_id,
											 item,
											 gInventoryCallbacks.registerCB(cb));
			}
		}
	}
};
///----------------------------------------------------------------------------
/// Class LLEmbeddedNotecardOpener
///----------------------------------------------------------------------------
class LLEmbeddedNotecardOpener : public LLInventoryCallback
{
	LLViewerTextEditor* mTextEditor;

public:
	LLEmbeddedNotecardOpener()
		: mTextEditor(NULL)
	{
	}

	void setEditor(LLViewerTextEditor* e) {mTextEditor = e;}

	// override
	void fire(const LLUUID& inv_item)
	{
		if(!mTextEditor)
		{
			// The parent text editor may have vanished by now. 
            // In that case just quit.
			return;
		}

		LLInventoryItem* item = gInventory.getItem(inv_item);
		if(!item)
		{
			llwarns << "Item add reported, but not found in inventory!: " << inv_item << llendl;
		}
		else
		{
			if(!gSavedSettings.getBOOL("ShowNewInventory"))
			{
				LLFloaterReg::showInstance("preview_notecard", LLSD(item->getUUID()), TAKE_FOCUS_YES);
			}
		}
	}
};

//
// class LLEmbeddedItemSegment
//

const S32 EMBEDDED_ITEM_LABEL_PADDING = 2;

class LLEmbeddedItemSegment : public LLTextSegment
{
public:
	LLEmbeddedItemSegment(S32 pos, LLUIImagePtr image, LLPointer<LLInventoryItem> inv_item, LLTextEditor& editor)
	:	LLTextSegment(pos, pos + 1),
		mImage(image),
		mLabel(utf8str_to_wstring(inv_item->getName())),
		mItem(inv_item),
		mEditor(editor),
		mHasMouseHover(false)
	{

		mStyle = new LLStyle(LLStyle::Params().font(LLFontGL::getFontSansSerif()));
		mToolTip = inv_item->getName() + '\n' + inv_item->getDescription();
	}

	/*virtual*/ bool getDimensions(S32 first_char, S32 num_chars, S32& width, S32& height) const
	{
		if (num_chars == 0)
		{
			width = 0;
			height = 0;
		}
		else
		{
			width = EMBEDDED_ITEM_LABEL_PADDING + mImage->getWidth() + mStyle->getFont()->getWidth(mLabel.c_str());
			height = llmax(mImage->getHeight(), mStyle->getFont()->getLineHeight());
		}
		return false;
	}

	/*virtual*/ S32				getNumChars(S32 num_pixels, S32 segment_offset, S32 line_offset, S32 max_chars) const 
	{
		// always draw at beginning of line
		if (line_offset == 0)
		{
			return 1;
		}
		else
		{
			S32 width, height;
			getDimensions(mStart, 1, width, height);
			if (width > num_pixels) 
			{
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}
	/*virtual*/ F32				draw(S32 start, S32 end, S32 selection_start, S32 selection_end, const LLRect& draw_rect)
	{
		LLRect image_rect = draw_rect;
		image_rect.mRight = image_rect.mLeft + mImage->getWidth();
		image_rect.mTop = image_rect.mBottom + mImage->getHeight();
		mImage->draw(image_rect);

		LLColor4 color;
		if (mEditor.getReadOnly())
		{
			color = LLUIColorTable::instance().getColor("TextEmbeddedItemReadOnlyColor");
		}
		else
		{
			color = LLUIColorTable::instance().getColor("TextEmbeddedItemColor");
		}

		F32 right_x;
		mStyle->getFont()->render(mLabel, 0, image_rect.mRight + EMBEDDED_ITEM_LABEL_PADDING, draw_rect.mTop, color, LLFontGL::LEFT, LLFontGL::TOP, LLFontGL::UNDERLINE, LLFontGL::NO_SHADOW, mLabel.length(), S32_MAX, &right_x);
		return right_x;
	}
	
	/*virtual*/ bool			canEdit() const { return false; }


	/*virtual*/ BOOL			handleHover(S32 x, S32 y, MASK mask)
	{
		LLUI::getWindow()->setCursor(UI_CURSOR_HAND);
		return TRUE;
	}
	virtual BOOL				handleToolTip(S32 x, S32 y, MASK mask )
	{ 
		if (!mToolTip.empty())
		{
			LLToolTipMgr::instance().show(mToolTip);
			return TRUE;
		}
		return FALSE; 
	}

	/*virtual*/ LLStyleConstSP		getStyle() const { return mStyle; }

private:
	LLUIImagePtr	mImage;
	LLWString		mLabel;
	LLStyleSP		mStyle;
	std::string		mToolTip;
	LLPointer<LLInventoryItem> mItem;
	LLTextEditor&	mEditor;
	bool			mHasMouseHover;

};



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
	
	BOOL	insertEmbeddedItem(LLInventoryItem* item, llwchar* value, bool is_new);
	BOOL	removeEmbeddedItem( llwchar ext_char );

	BOOL	hasEmbeddedItem(llwchar ext_char); // returns TRUE if /this/ editor has an entry for this item
	LLUIImagePtr getItemImage(llwchar ext_char) const;

	void	getEmbeddedItemList( std::vector<LLPointer<LLInventoryItem> >& items );
	void	addItems(const std::vector<LLPointer<LLInventoryItem> >& items);

	llwchar	getEmbeddedCharFromIndex(S32 index);

	void 	removeUnusedChars();
	void	copyUsedCharsToIndexed();
	S32		getIndexFromEmbeddedChar(llwchar wch);

	void	markSaved();
	
	static LLPointer<LLInventoryItem> getEmbeddedItemPtr(llwchar ext_char); // returns pointer to item from static list
	static BOOL getEmbeddedItemSaved(llwchar ext_char); // returns whether item from static list is saved

private:

	struct embedded_info_t
	{
		LLPointer<LLInventoryItem> mItemPtr;
		BOOL mSaved;
	};
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
		wc_emb = LLTextEditor::FIRST_EMBEDDED_CHAR;
	}
	else
	{
		item_map_t::iterator last = sEntries.end();
		--last;
		wc_emb = last->first;
		if (wc_emb >= LLTextEditor::LAST_EMBEDDED_CHAR)
		{
			return FALSE;
		}
		++wc_emb;
	}

	sEntries[wc_emb].mItemPtr = item;
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
LLPointer<LLInventoryItem> LLEmbeddedItems::getEmbeddedItemPtr(llwchar ext_char)
{
	if( ext_char >= LLTextEditor::FIRST_EMBEDDED_CHAR && ext_char <= LLTextEditor::LAST_EMBEDDED_CHAR )
	{
		item_map_t::iterator iter = sEntries.find(ext_char);
		if (iter != sEntries.end())
		{
			return iter->second.mItemPtr;
		}
	}
	return NULL;
}

// static
BOOL LLEmbeddedItems::getEmbeddedItemSaved(llwchar ext_char)
{
	if( ext_char >= LLTextEditor::FIRST_EMBEDDED_CHAR && ext_char <= LLTextEditor::LAST_EMBEDDED_CHAR )
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
		if( wc >= LLTextEditor::FIRST_EMBEDDED_CHAR && wc <= LLTextEditor::LAST_EMBEDDED_CHAR )
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


LLUIImagePtr LLEmbeddedItems::getItemImage(llwchar ext_char) const
{
	LLInventoryItem* item = getEmbeddedItemPtr(ext_char);
	if (item)
	{
		const char* img_name = "";
		switch( item->getType() )
		{
			case LLAssetType::AT_TEXTURE:
				if(item->getInventoryType() == LLInventoryType::IT_SNAPSHOT)
				{
					img_name = "Inv_Snapshot";
				}
				else
				{
					img_name = "Inv_Texture";
				}

				break;
			case LLAssetType::AT_SOUND:			img_name = "Inv_Sound";		break;
			case LLAssetType::AT_CLOTHING:		img_name = "Inv_Clothing";	break;
			case LLAssetType::AT_OBJECT:
				img_name = LLInventoryItemFlags::II_FLAGS_OBJECT_HAS_MULTIPLE_ITEMS & item->getFlags() ?
					"Inv_Object_Multi" : "Inv_Object";
				break;
			case LLAssetType::AT_CALLINGCARD:	img_name = "Inv_CallingCard"; break;
			case LLAssetType::AT_LANDMARK:		img_name = "Inv_Landmark"; 	break;
			case LLAssetType::AT_NOTECARD:		img_name = "Inv_Notecard";	break;
			case LLAssetType::AT_LSL_TEXT:		img_name = "Inv_Script";	break;
			case LLAssetType::AT_BODYPART:		img_name = "Inv_Skin";		break;
			case LLAssetType::AT_ANIMATION:		img_name = "Inv_Animation";	break;
			case LLAssetType::AT_GESTURE:		img_name = "Inv_Gesture";	break;
			case LLAssetType::AT_MESH:          img_name = "Inv_Mesh";	    break;
			default: llassert(0);
		}

		return LLUI::getUIImage(img_name);
	}
	return LLUIImagePtr();
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
		LLPointer<LLInventoryItem> item = getEmbeddedItemPtr(wc);
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

class LLViewerTextEditor::TextCmdInsertEmbeddedItem : public LLTextBase::TextCmd
{
public:
	TextCmdInsertEmbeddedItem( S32 pos, LLInventoryItem* item )
		: TextCmd(pos, FALSE), 
		  mExtCharValue(0)
	{
		mItem = item;
	}

	virtual BOOL execute( LLTextBase* editor, S32* delta )
	{
		LLViewerTextEditor* viewer_editor = (LLViewerTextEditor*)editor;
		// Take this opportunity to remove any unused embedded items from this editor
		viewer_editor->mEmbeddedItemList->removeUnusedChars();
		if(viewer_editor->mEmbeddedItemList->insertEmbeddedItem( mItem, &mExtCharValue, true ) )
		{
			LLWString ws;
			ws.assign(1, mExtCharValue);
			*delta = insert(editor, getPosition(), ws );
			return (*delta != 0);
		}
		return FALSE;
	}
	
	virtual S32 undo( LLTextBase* editor )
	{
		remove(editor, getPosition(), 1);
		return getPosition(); 
	}
	
	virtual S32 redo( LLTextBase* editor )
	{ 
		LLWString ws;
		ws += mExtCharValue;
		insert(editor, getPosition(), ws );
		return getPosition() + 1;
	}
	virtual BOOL hasExtCharValue( llwchar value ) const
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
LLViewerTextEditor::LLViewerTextEditor(const LLViewerTextEditor::Params& p)
:	LLTextEditor(p),
	mDragItemChar(0),
	mDragItemSaved(FALSE),
	mInventoryCallback(new LLEmbeddedNotecardOpener)
{
	mEmbeddedItemList = new LLEmbeddedItems(this);
	mInventoryCallback->setEditor(this);
}

LLViewerTextEditor::~LLViewerTextEditor()
{
	delete mEmbeddedItemList;
	
	
	// The inventory callback may still be in use by gInventoryCallbackManager...
	// so set its reference to this to null.
	mInventoryCallback->setEditor(NULL); 
}

///////////////////////////////////////////////////////////////////
// virtual
void LLViewerTextEditor::makePristine()
{
	mEmbeddedItemList->markSaved();
	LLTextEditor::makePristine();
}

BOOL LLViewerTextEditor::handleMouseDown(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// Let scrollbar have first dibs
	handled = LLView::childrenHandleMouseDown(x, y, mask) != NULL;

	if( !handled)
	{
		if( allowsEmbeddedItems() )
		{
			setCursorAtLocalPos( x, y, FALSE );
			llwchar wc = 0;
			if (mCursorPos < getLength())
			{
				wc = getWText()[mCursorPos];
			}
			LLPointer<LLInventoryItem> item_at_pos = LLEmbeddedItems::getEmbeddedItemPtr(wc);
			if (item_at_pos)
			{
				mDragItem = item_at_pos;
				mDragItemChar = wc;
				mDragItemSaved = LLEmbeddedItems::getEmbeddedItemSaved(wc);
				gFocusMgr.setMouseCapture( this );
				mMouseDownX = x;
				mMouseDownY = y;
				S32 screen_x;
				S32 screen_y;
				localPointToScreen(x, y, &screen_x, &screen_y );
				LLToolDragAndDrop::getInstance()->setDragStart( screen_x, screen_y );

				if (hasTabStop())
				{
					setFocus( TRUE );
				}

				handled = TRUE;
			}
			else
			{
				mDragItem = NULL;
			}
		}

		if (!handled)
		{
			handled = LLTextEditor::handleMouseDown(x, y, mask);
		}
	}

	return handled;
}


BOOL LLViewerTextEditor::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLTextEditor::handleHover(x, y, mask);

	if(hasMouseCapture() && mDragItem)
	{
		S32 screen_x;
		S32 screen_y;
		localPointToScreen(x, y, &screen_x, &screen_y );

		mScroller->autoScroll(x, y);

		if( LLToolDragAndDrop::getInstance()->isOverThreshold( screen_x, screen_y ) )
		{
			LLToolDragAndDrop::getInstance()->beginDrag(
				LLViewerAssetType::lookupDragAndDropType( mDragItem->getType() ),
				mDragItem->getUUID(),
				LLToolDragAndDrop::SOURCE_NOTECARD,
				mPreviewID, mObjectID);
			return LLToolDragAndDrop::getInstance()->handleHover( x, y, mask );
		}
		getWindow()->setCursor(UI_CURSOR_HAND);
		handled = TRUE;
	}

	return handled;
}


BOOL LLViewerTextEditor::handleMouseUp(S32 x, S32 y, MASK mask)
{
	BOOL handled = FALSE;

	if( hasMouseCapture() )
	{
		if (mDragItem)
		{
			// mouse down was on an item
			S32 dx = x - mMouseDownX;
			S32 dy = y - mMouseDownY;
			if (-2 < dx && dx < 2 && -2 < dy && dy < 2)
			{
				if(mDragItemSaved)
				{
					openEmbeddedItem(mDragItem, mDragItemChar);
				}
				else
				{
					showUnsavedAlertDialog(mDragItem);
				}
			}
		}
		mDragItem = NULL;
	}

	handled = LLTextEditor::handleMouseUp(x,y,mask);

	return handled;
}

BOOL LLViewerTextEditor::handleDoubleClick(S32 x, S32 y, MASK mask)
{
	BOOL	handled = FALSE;

	// let scrollbar have first dibs
	handled = LLView::childrenHandleDoubleClick(x, y, mask) != NULL;

	if( !handled)
	{
		if( allowsEmbeddedItems() )
		{
			S32 doc_index = getDocIndexFromLocalCoord(x, y, FALSE);
			llwchar doc_char = getWText()[doc_index];
			if (mEmbeddedItemList->hasEmbeddedItem(doc_char))
			{
				if( openEmbeddedItemAtPos( doc_index ))
				{
					deselect();
					setFocus( FALSE );
					return TRUE;
				}
			}
		}
		handled = LLTextEditor::handleDoubleClick(x, y, mask);
	}
	return handled;
}


// virtual
BOOL LLViewerTextEditor::handleDragAndDrop(S32 x, S32 y, MASK mask,
					  BOOL drop, EDragAndDropType cargo_type, void *cargo_data,
					  EAcceptance *accept,
					  std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	
	LLToolDragAndDrop::ESource source = LLToolDragAndDrop::getInstance()->getSource();
	if (LLToolDragAndDrop::SOURCE_NOTECARD == source)
	{
		// We currently do not handle dragging items from one notecard to another
		// since items in a notecard must be in Inventory to be verified. See DEV-2891.
		return FALSE;
	}
	
	if (getEnabled() && acceptsTextInput())
	{
		switch( cargo_type )
		{
			case DAD_CALLINGCARD:
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
			case DAD_MESH:
			{
				LLInventoryItem *item = (LLInventoryItem *)cargo_data;
				if( item && allowsEmbeddedItems() )
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

							needsReflow();
							
						}
						*accept = ACCEPT_YES_COPY_MULTI;
					}
					else
					{
						*accept = ACCEPT_NO;
						if (tooltip_msg.empty())
						{
							// *TODO: Translate
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

	return handled;
}

void LLViewerTextEditor::setASCIIEmbeddedText(const std::string& instr)
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

void LLViewerTextEditor::setEmbeddedText(const std::string& instr)
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

std::string LLViewerTextEditor::getEmbeddedText()
{
#if 1
	// New version (Version 2)
	mEmbeddedItemList->copyUsedCharsToIndexed();
	LLWString outtextw;
	for (S32 i=0; i<(S32)getWText().size(); i++)
	{
		llwchar wch = getWText()[i];
		if( wch >= FIRST_EMBEDDED_CHAR && wch <= LAST_EMBEDDED_CHAR )
		{
			S32 index = mEmbeddedItemList->getIndexFromEmbeddedChar(wch);
			wch = FIRST_EMBEDDED_CHAR + index;
		}
		outtextw.push_back(wch);
	}
	std::string outtext = wstring_to_utf8str(outtextw);
	return outtext;
#else
	// Old version (Version 1)
	mEmbeddedItemList->copyUsedCharsToIndexed();
	std::string outtext;
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

std::string LLViewerTextEditor::appendTime(bool prepend_newline)
{
	time_t utc_time;
	utc_time = time_corrected();
	std::string timeStr ="[["+ LLTrans::getString("TimeHour")+"]:["
		+LLTrans::getString("TimeMin")+"]] ";

	LLSD substitution;

	substitution["datetime"] = (S32) utc_time;
	LLStringUtil::format (timeStr, substitution);
	appendText(timeStr, prepend_newline, LLStyle::Params().color(LLColor4::grey));
	blockUndo();

	return timeStr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

llwchar LLViewerTextEditor::pasteEmbeddedItem(llwchar ext_char)
{
	if (mEmbeddedItemList->hasEmbeddedItem(ext_char))
	{
		return ext_char; // already exists in my list
	}
	LLInventoryItem* item = LLEmbeddedItems::getEmbeddedItemPtr(ext_char);
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

void LLViewerTextEditor::onValueChange(S32 start, S32 end)
{
	updateSegments();
	updateLinkSegments();
	findEmbeddedItemSegments(start, end);
}

void LLViewerTextEditor::findEmbeddedItemSegments(S32 start, S32 end)
{
	LLWString text = getWText();

	// Start with i just after the first embedded item
	for(S32 idx = start; idx < end; idx++ )
	{
		llwchar embedded_char = text[idx];
		if( embedded_char >= FIRST_EMBEDDED_CHAR 
			&& embedded_char <= LAST_EMBEDDED_CHAR 
			&& mEmbeddedItemList->hasEmbeddedItem(embedded_char) )
		{
			LLInventoryItem* itemp = mEmbeddedItemList->getEmbeddedItemPtr(embedded_char);
			LLUIImagePtr image = mEmbeddedItemList->getItemImage(embedded_char);
			insertSegment(new LLEmbeddedItemSegment(idx, image, itemp, *this));
		}
	}
}

BOOL LLViewerTextEditor::openEmbeddedItemAtPos(S32 pos)
{
	if( pos < getLength())
	{
		llwchar wc = getWText()[pos];
		LLPointer<LLInventoryItem> item = LLEmbeddedItems::getEmbeddedItemPtr( wc );
		if( item )
		{
			BOOL saved = LLEmbeddedItems::getEmbeddedItemSaved( wc );
			if (saved)
			{
				return openEmbeddedItem(item, wc); 
			}
			else
			{
				showUnsavedAlertDialog(item);
			}
		}
	}
	return FALSE;
}


BOOL LLViewerTextEditor::openEmbeddedItem(LLPointer<LLInventoryItem> item, llwchar wc)
{

	switch( item->getType() )
	{
		case LLAssetType::AT_TEXTURE:
			openEmbeddedTexture( item, wc );
			return TRUE;

		case LLAssetType::AT_SOUND:
			openEmbeddedSound( item, wc );
			return TRUE;

		case LLAssetType::AT_NOTECARD:
			openEmbeddedNotecard( item, wc );
			return TRUE;

		case LLAssetType::AT_LANDMARK:
			openEmbeddedLandmark( item, wc );
			return TRUE;

		case LLAssetType::AT_CALLINGCARD:
			openEmbeddedCallingcard( item, wc );
			return TRUE;

		case LLAssetType::AT_LSL_TEXT:
		case LLAssetType::AT_CLOTHING:
		case LLAssetType::AT_OBJECT:
		case LLAssetType::AT_BODYPART:
		case LLAssetType::AT_ANIMATION:
		case LLAssetType::AT_GESTURE:
			showCopyToInvDialog( item, wc );
			return TRUE;
		default:
			return FALSE;
	}

}


void LLViewerTextEditor::openEmbeddedTexture( LLInventoryItem* item, llwchar wc )
{
	// *NOTE:  Just for embedded Texture , we should use getAssetUUID(), 
	// not getUUID(), because LLPreviewTexture pass in AssetUUID into 
	// LLPreview constructor ItemUUID parameter.
	if (!item)
		return;
	LLPreviewTexture* preview = LLFloaterReg::showTypedInstance<LLPreviewTexture>("preview_texture", LLSD(item->getAssetUUID()), TAKE_FOCUS_YES);
	if (preview)
	{
		preview->setAuxItem( item );
		preview->setNotecardInfo(mNotecardInventoryID, mObjectID);
	}
}

void LLViewerTextEditor::openEmbeddedSound( LLInventoryItem* item, llwchar wc )
{
	// Play sound locally
	LLVector3d lpos_global = gAgent.getPositionGlobal();
	const F32 SOUND_GAIN = 1.0f;
	if(gAudiop)
	{
		gAudiop->triggerSound(item->getAssetUUID(), gAgentID, SOUND_GAIN, LLAudioEngine::AUDIO_TYPE_UI, lpos_global);
	}
	showCopyToInvDialog( item, wc );
}


void LLViewerTextEditor::openEmbeddedLandmark( LLPointer<LLInventoryItem> item_ptr, llwchar wc )
{
	if (item_ptr.isNull())
		return;

	LLLandmark* landmark = gLandmarkList.getAsset(item_ptr->getAssetUUID(),
			boost::bind(&LLEmbeddedLandmarkCopied::processForeignLandmark, _1, mObjectID, mNotecardInventoryID, item_ptr));
	if (landmark)
	{
		LLEmbeddedLandmarkCopied::processForeignLandmark(landmark, mObjectID,
				mNotecardInventoryID, item_ptr);
	}
}

void LLViewerTextEditor::openEmbeddedNotecard( LLInventoryItem* item, llwchar wc )
{
	copyInventory(item, gInventoryCallbacks.registerCB(mInventoryCallback));
}

void LLViewerTextEditor::openEmbeddedCallingcard( LLInventoryItem* item, llwchar wc )
{
	if(item && !item->getCreatorUUID().isNull())
	{
		LLAvatarActions::showProfile(item->getCreatorUUID());
	}
}

void LLViewerTextEditor::showUnsavedAlertDialog( LLInventoryItem* item )
{
	LLSD payload;
	payload["item_id"] = item->getUUID();
	payload["notecard_id"] = mNotecardInventoryID;
	LLNotificationsUtil::add( "ConfirmNotecardSave", LLSD(), payload, LLViewerTextEditor::onNotecardDialog);
}

// static
bool LLViewerTextEditor::onNotecardDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if( option == 0 )
	{
		LLPreviewNotecard* preview = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", notification["payload"]["notecard_id"]);;
		if (preview)
		{
			preview->saveItem();
		}
	}
	return false;
}



void LLViewerTextEditor::showCopyToInvDialog( LLInventoryItem* item, llwchar wc )
{
	LLSD payload;
	LLUUID item_id = item->getUUID();
	payload["item_id"] = item_id;
	payload["item_wc"] = LLSD::Integer(wc);
	LLNotificationsUtil::add( "ConfirmItemCopy", LLSD(), payload,
		boost::bind(&LLViewerTextEditor::onCopyToInvDialog, this, _1, _2));
}

bool LLViewerTextEditor::onCopyToInvDialog(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if( 0 == option )
	{
		LLUUID item_id = notification["payload"]["item_id"].asUUID();
		llwchar wc = llwchar(notification["payload"]["item_wc"].asInteger());
		LLInventoryItem* itemp = LLEmbeddedItems::getEmbeddedItemPtr(wc);
		if (itemp)
			copyInventory(itemp);
	}
	return false;
}



// Returns change in number of characters in mWText
S32 LLViewerTextEditor::insertEmbeddedItem( S32 pos, LLInventoryItem* item )
{
	return execute( new TextCmdInsertEmbeddedItem( pos, item ) );
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
		if (allowsEmbeddedItems())
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

void LLViewerTextEditor::copyInventory(const LLInventoryItem* item, U32 callback_id)
{
	copy_inventory_from_notecard(LLUUID::null,  // Don't specify a destination -- let the sim do that
								 mObjectID,
								 mNotecardInventoryID,
								 item,
								 callback_id);
}

bool LLViewerTextEditor::hasEmbeddedInventory()
{
	return ! mEmbeddedItemList->empty();
}

////////////////////////////////////////////////////////////////////////////

BOOL LLViewerTextEditor::importBuffer( const char* buffer, S32 length )
{
	LLMemoryStream str((U8*)buffer, length);
	return importStream(str);
}

BOOL LLViewerTextEditor::exportBuffer( std::string& buffer )
{
	LLNotecard nc(LLNotecard::MAX_SIZE);

	// Get the embedded text and update the item list to just be the used items
	nc.setText(getEmbeddedText());

	// Now get the used items and copy the list to the notecard
	std::vector<LLPointer<LLInventoryItem> > embedded_items;
	mEmbeddedItemList->getEmbeddedItemList(embedded_items);	
	nc.setItems(embedded_items);
	
	std::stringstream out_stream;
	nc.exportStream(out_stream);
	
	buffer = out_stream.str();
	
	return TRUE;
}

