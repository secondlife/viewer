/** 
 * @file llfoldervieweventlistener.h
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
#ifndef LLFOLDERVIEWEVENTLISTENER_H
#define LLFOLDERVIEWEVENTLISTENER_H

#include "lldarray.h"	// JAMESDEBUG convert to std::vector
#include "llfoldertype.h"
#include "llfontgl.h"	// just for StyleFlags enum
#include "llinventorytype.h"
#include "llpermissionsflags.h"
#include "llpointer.h"


class LLFolderViewItem;
class LLFolderView;
class LLFontGL;
class LLInventoryModel;
class LLMenuGL;
class LLScrollContainer;
class LLUIImage;
class LLUUID;

// This is an abstract base class that users of the folderview classes
// would use to catch the useful events emitted from the folder
// views.
class LLFolderViewEventListener
{
public:
	virtual ~LLFolderViewEventListener( void ) {}
	virtual const std::string& getName() const = 0;
	virtual const std::string& getDisplayName() const = 0;
	virtual const LLUUID& getUUID() const = 0;
	virtual time_t getCreationDate() const = 0;	// UTC seconds
	virtual PermissionMask getPermissionMask() const = 0;
	virtual LLFolderType::EType getPreferredType() const = 0;
	virtual LLPointer<LLUIImage> getIcon() const = 0;
	virtual LLPointer<LLUIImage> getOpenIcon() const { return getIcon(); }
	virtual LLFontGL::StyleFlags getLabelStyle() const = 0;
	virtual std::string getLabelSuffix() const = 0;
	virtual void openItem( void ) = 0;
	virtual void closeItem( void ) = 0;
	virtual void previewItem( void ) = 0;
	virtual void selectItem(void) = 0;
	virtual void showProperties(void) = 0;
	virtual BOOL isItemRenameable() const = 0;
	virtual BOOL renameItem(const std::string& new_name) = 0;
	virtual BOOL isItemMovable( void ) const = 0;		// Can be moved to another folder
	virtual BOOL isItemRemovable( void ) const = 0;		// Can be destroyed
	virtual BOOL isItemInTrash( void) const { return FALSE; } // TODO: make into pure virtual.
	virtual BOOL removeItem() = 0;
	virtual void removeBatch(LLDynamicArray<LLFolderViewEventListener*>& batch) = 0;
	virtual void move( LLFolderViewEventListener* parent_listener ) = 0;
	virtual BOOL isItemCopyable() const = 0;
	virtual BOOL copyToClipboard() const = 0;
	virtual void cutToClipboard() = 0;
	virtual BOOL isClipboardPasteable() const = 0;
	virtual void pasteFromClipboard() = 0;
	virtual void pasteLinkFromClipboard() = 0;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) = 0;
	virtual BOOL isUpToDate() const = 0;
	virtual BOOL hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLFolderView* folder, LLInventoryModel* model, std::string action) = 0;
	
	// This method should be called when a drag begins. returns TRUE
	// if the drag can begin, otherwise FALSE.
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const = 0;
	
	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. Returns TRUE if a drop is possible/happened,
	// otherwise FALSE.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data) = 0;
};

#endif
