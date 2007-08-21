/** 
 * @file llviewertexteditor.h
 * @brief Text editor widget to let users enter a a multi-line document//
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_VIEWERTEXTEDITOR_H
#define LL_VIEWERTEXTEDITOR_H

#include "lltexteditor.h"

class LLInventoryItem;
class LLEmbeddedItems;
class LLEmbeddedNotecardOpener;

//
// Classes
//
class LLViewerTextEditor : public LLTextEditor
{
	friend class LLEmbeddedItems;
	friend class LLTextCmdInsertEmbeddedItem;
	
public:
	LLViewerTextEditor(const LLString& name,
					   const LLRect& rect,
					   S32 max_length,
					   const LLString& default_text = "",
					   const LLFontGL* glfont = NULL,
					   BOOL allow_embedded_items = FALSE);

	virtual ~LLViewerTextEditor();

	virtual void makePristine();
	
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	// mousehandler overrides
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask );

	virtual BOOL	handleToolTip(S32 x, S32 y, LLString& msg, LLRect* sticky_rect);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
										BOOL drop, EDragAndDropType cargo_type, 
										void *cargo_data, EAcceptance *accept, LLString& tooltip_msg);

  	const LLInventoryItem* getDragItem() { return mDragItem; }
	virtual BOOL 	importBuffer(const LLString& buffer);
	virtual bool	importStream(std::istream& str);
	virtual BOOL 	exportBuffer(LLString& buffer);
	void setNotecardInfo(const LLUUID& notecard_item_id, const LLUUID& object_id)
	{
		mNotecardInventoryID = notecard_item_id;
		mObjectID = object_id;
	}
	
	void setASCIIEmbeddedText(const LLString& instr);
	void setEmbeddedText(const LLString& instr);
	LLString getEmbeddedText();
	
	LLString appendTime(bool prepend_newline);
		// Appends Second Life time, small font, grey
		// If this starts a line, you need to prepend a newline.

	void copyInventory(const LLInventoryItem* item, U32 callback_id = 0);

	// returns true if there is embedded inventory.
	// *HACK: This is only useful because the notecard verifier may
	// change the asset if there is embedded inventory. This mechanism
	// should be changed to get a different asset id from the verifier
	// rather than checking if a re-load is necessary. Phoenix 2007-02-27
	bool hasEmbeddedInventory();

protected:
	// Embedded object operations
	virtual llwchar	pasteEmbeddedItem(llwchar ext_char);
	virtual void	bindEmbeddedChars(const LLFontGL* font);
	virtual void	unbindEmbeddedChars(const LLFontGL* font);

	BOOL			getEmbeddedItemToolTipAtPos(S32 pos, LLWString &wmsg);
	BOOL			openEmbeddedItemAtPos( S32 pos );
	BOOL			openEmbeddedItem(LLInventoryItem* item, BOOL saved);

	S32				insertEmbeddedItem(S32 pos, LLInventoryItem* item);

	void			openEmbeddedTexture( LLInventoryItem* item );
	void			openEmbeddedSound( LLInventoryItem* item );
	void			openEmbeddedLandmark( LLInventoryItem* item );
	void			openEmbeddedNotecard( LLInventoryItem* item, BOOL saved );
	void			showCopyToInvDialog( LLInventoryItem* item );

	static void		onCopyToInvDialog( S32 option, void* userdata );
	static void		onNotecardDialog( S32 option, void* userdata );
	
protected:
	LLPointer<LLInventoryItem> mDragItem;
	BOOL mDragItemSaved;
	LLEmbeddedItems* mEmbeddedItemList;

	LLUUID mObjectID;
	LLUUID mNotecardInventoryID;

	LLPointer<LLEmbeddedNotecardOpener> mInventoryCallback;
};

#endif  // LL_VIEWERTEXTEDITOR_H
