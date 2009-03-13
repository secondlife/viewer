/** 
 * @file llviewertexteditor.h
 * @brief Text editor widget to let users enter a multi-line document//
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#ifndef LL_VIEWERTEXTEDITOR_H
#define LL_VIEWERTEXTEDITOR_H

#include "lltexteditor.h"


//
// Classes
//
class LLViewerTextEditor : public LLTextEditor
{
	
public:
	LLViewerTextEditor(const std::string& name,
					   const LLRect& rect,
					   S32 max_length,
					   const std::string& default_text = std::string(),
					   const LLFontGL* glfont = NULL,
					   BOOL allow_embedded_items = FALSE);

	virtual ~LLViewerTextEditor();

	virtual void makePristine();
	
	static LLView* fromXML(LLXMLNodePtr node, LLView *parent, LLUICtrlFactory *factory);

	// mousehandler overrides
	virtual BOOL	handleMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
	virtual BOOL	handleRightMouseDown(S32 x, S32 y, MASK mask);
	virtual BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual BOOL	handleDoubleClick(S32 x, S32 y, MASK mask );

	virtual BOOL	handleToolTip(S32 x, S32 y, std::string& msg, LLRect* sticky_rect);
	virtual BOOL	handleDragAndDrop(S32 x, S32 y, MASK mask,
										BOOL drop, EDragAndDropType cargo_type, 
										void *cargo_data, EAcceptance *accept, std::string& tooltip_msg);

  	const class LLInventoryItem* getDragItem() const { return mDragItem; }
	virtual BOOL 	importBuffer(const char* buffer, S32 length);
	virtual bool	importStream(std::istream& str);
	virtual BOOL 	exportBuffer(std::string& buffer);
	void setNotecardInfo(const LLUUID& notecard_item_id, const LLUUID& object_id)
	{
		mNotecardInventoryID = notecard_item_id;
		mObjectID = object_id;
	}
	
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
	virtual llwchar	pasteEmbeddedItem(llwchar ext_char);
	virtual void	bindEmbeddedChars(const LLFontGL* font) const;
	virtual void	unbindEmbeddedChars(const LLFontGL* font) const;

	BOOL			getEmbeddedItemToolTipAtPos(S32 pos, LLWString &wmsg) const;
	BOOL			openEmbeddedItemAtPos( S32 pos );
	BOOL			openEmbeddedItem(LLInventoryItem* item);

	S32				insertEmbeddedItem(S32 pos, LLInventoryItem* item);

	void			openEmbeddedTexture( LLInventoryItem* item );
	void			openEmbeddedSound( LLInventoryItem* item );
	void			openEmbeddedLandmark( LLInventoryItem* item );
	void			openEmbeddedNotecard( LLInventoryItem* item);
	void			showCopyToInvDialog( LLInventoryItem* item );
	void			showUnsavedAlertDialog( LLInventoryItem* item );

	bool			onCopyToInvDialog(const LLSD& notification, const LLSD& response );
	static bool		onNotecardDialog(const LLSD& notification, const LLSD& response );
	
	LLPointer<LLInventoryItem> mDragItem;
	BOOL mDragItemSaved;
	class LLEmbeddedItems* mEmbeddedItemList;

	LLUUID mObjectID;
	LLUUID mNotecardInventoryID;

	LLPointer<class LLEmbeddedNotecardOpener> mInventoryCallback;

	// *TODO: Add right click menus for SLURLs
	//LLViewHandle mPopupMenuHandle;

	//
	// Inner classes
	//

	class LLTextCmdInsertEmbeddedItem;
};

#endif  // LL_VIEWERTEXTEDITOR_H
