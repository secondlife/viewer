/** 
 * @file llfoldervieweventlistener.h
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
#ifndef LLFOLDERVIEWEVENTLISTENER_H
#define LLFOLDERVIEWEVENTLISTENER_H

#include "lldarray.h"	// *TODO: convert to std::vector
#include "llfoldertype.h"
#include "llfontgl.h"	// just for StyleFlags enum
#include "llinventorytype.h"
#include "llpermissionsflags.h"
#include "llpointer.h"
#include "llwearabletype.h"


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
	virtual BOOL cutToClipboard() const = 0;
	virtual BOOL isClipboardPasteable() const = 0;
	virtual void pasteFromClipboard() = 0;
	virtual void pasteLinkFromClipboard() = 0;
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) = 0;
	virtual BOOL isUpToDate() const = 0;
	virtual BOOL hasChildren() const = 0;
	virtual LLInventoryType::EType getInventoryType() const = 0;
	virtual void performAction(LLInventoryModel* model, std::string action) = 0;
	virtual LLWearableType::EType getWearableType() const = 0;
	
	// This method should be called when a drag begins. returns TRUE
	// if the drag can begin, otherwise FALSE.
	virtual BOOL startDrag(EDragAndDropType* type, LLUUID* id) const = 0;
	
	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. Returns TRUE if a drop is possible/happened,
	// otherwise FALSE.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
							EDragAndDropType cargo_type,
							void* cargo_data,
							std::string& tooltip_msg) = 0;
};

#endif
