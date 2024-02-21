/** 
 * @file llviewertexteditor.h
 * @brief Text editor widget to let users enter a multi-line document//
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

#ifndef LL_VIEWERTEXTEDITOR_H
#define LL_VIEWERTEXTEDITOR_H

#include "lltexteditor.h"

//
// Classes
//
class LLViewerTextEditor : public LLTextEditor
{
public:
	struct Params : public LLInitParam::Block<Params, LLTextEditor::Params>
	{};

protected:
	LLViewerTextEditor(const Params&);
	friend class LLUICtrlFactory;

public:
	virtual ~LLViewerTextEditor();

	virtual void makePristine();

	/*virtual*/ void onVisibilityChange( bool new_visibility );
	
	// mousehandler overrides
	virtual bool	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual bool	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual bool	handleHover(S32 x, S32 y, MASK mask);
	virtual bool	handleDoubleClick(S32 x, S32 y, MASK mask );

	virtual bool	handleDragAndDrop(S32 x, S32 y, MASK mask,
										bool drop, EDragAndDropType cargo_type, 
										void *cargo_data, EAcceptance *accept, std::string& tooltip_msg);

  	const class LLInventoryItem* getDragItem() const { return mDragItem; }
	virtual bool 	importBuffer(const char* buffer, S32 length);
	virtual bool	importStream(std::istream& str);
	virtual bool 	exportBuffer(std::string& buffer);
	virtual void	onValueChange(S32 start, S32 end);

	void setNotecardInfo(const LLUUID& notecard_item_id, const LLUUID& object_id, const LLUUID& preview_id)
	{
		mNotecardInventoryID = notecard_item_id;
		mObjectID = object_id;
		mPreviewID = preview_id;
	}
	void setNotecardObjectID(const LLUUID& object_id){ mObjectID = object_id;}

	void setASCIIEmbeddedText(const std::string& instr);
	void setEmbeddedText(const std::string& instr);
	std::string getEmbeddedText();
	
	// Appends Second Life time, small font, grey.
	// If this starts a line, you need to prepend a newline.
	std::string appendTime(bool prepend_newline);

	void copyInventory(const LLInventoryItem* item, U32 callback_id = 0);

	// returns true if there is embedded inventory.
	// *HACK: This is only useful because the notecard verifier may
	// change the asset if there is embedded inventory. This mechanism
	// should be changed to get a different asset id from the verifier
	// rather than checking if a re-load is necessary. Phoenix 2007-02-27
	bool hasEmbeddedInventory();

private:
	// Embedded object operations
	void findEmbeddedItemSegments(S32 start, S32 end);
	virtual llwchar	pasteEmbeddedItem(llwchar ext_char);

	bool			openEmbeddedItemAtPos( S32 pos );
	bool			openEmbeddedItem(LLPointer<LLInventoryItem> item, llwchar wc);

	S32				insertEmbeddedItem(S32 pos, LLInventoryItem* item);

	// *NOTE: most of openEmbeddedXXX methods except openEmbeddedLandmark take pointer to LLInventoryItem.
	// Be sure they don't bind it to callback function to avoid situation when it gets invalid when
	// callback is trigged after text editor is closed. See EXT-8459.
	void			openEmbeddedTexture( LLInventoryItem* item, llwchar wc );
	void			openEmbeddedSound( LLInventoryItem* item, llwchar wc );
	void			openEmbeddedLandmark( LLPointer<LLInventoryItem> item_ptr, llwchar wc );
	void			openEmbeddedCallingcard( LLInventoryItem* item, llwchar wc);
	void			openEmbeddedSetting(LLInventoryItem* item, llwchar wc);
    void			openEmbeddedGLTFMaterial(LLInventoryItem* item, llwchar wc);
	void			showCopyToInvDialog( LLInventoryItem* item, llwchar wc );
	void			showUnsavedAlertDialog( LLInventoryItem* item );

	bool			onCopyToInvDialog(const LLSD& notification, const LLSD& response );
	static bool		onNotecardDialog(const LLSD& notification, const LLSD& response );
	
	LLPointer<LLInventoryItem> mDragItem;
	LLTextSegment* mDragSegment;
	llwchar mDragItemChar;
	bool mDragItemSaved;
	class LLEmbeddedItems* mEmbeddedItemList;

	LLUUID mObjectID;
	LLUUID mNotecardInventoryID;
	LLUUID mPreviewID;

	LLPointer<class LLEmbeddedNotecardOpener> mInventoryCallback;

	//
	// Inner classes
	//

	class TextCmdInsertEmbeddedItem;
};

#endif  // LL_VIEWERTEXTEDITOR_H
