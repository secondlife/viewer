/** 
* @file   llavatarfolder.h
* @brief  Header file for llavatarfolder
* @author Gilbert@lindenlab.com
*
* $LicenseInfo:firstyear=2013&license=viewerlgpl$
* Second Life Viewer Source Code
* Copyright (C) 2013, Linden Research, Inc.
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
#ifndef LL_LLPERSONMODELCOMMON_H
#define LL_LLPERSONMODELCOMMON_H

#include "../llui/llfolderviewitem.h"
#include "../llui/llfolderviewmodel.h"

class LLPersonTabModel;
class LLPersonModel;

// Conversation items: we hold a list of those and create an LLFolderViewItem widget for each  
// that we tuck into the mConversationsListPanel. 
class LLPersonModelCommon : public LLFolderViewModelItemCommon
{
public:

	LLPersonModelCommon(std::string name, LLFolderViewModelInterface& root_view_model);
	LLPersonModelCommon(LLFolderViewModelInterface& root_view_model);
	virtual ~LLPersonModelCommon();

	// Stub those things we won't really be using in this conversation context
	virtual const std::string& getName() const { return mName; }
	virtual const std::string& getDisplayName() const { return mName; }
	virtual const std::string& getSearchableName() const { return mName; }
	virtual LLPointer<LLUIImage> getIcon() const { return NULL; }
	virtual LLPointer<LLUIImage> getOpenIcon() const { return getIcon(); }
	virtual LLFontGL::StyleFlags getLabelStyle() const { return LLFontGL::NORMAL; }
	virtual std::string getLabelSuffix() const { return LLStringUtil::null; }
	virtual BOOL isItemRenameable() const { return TRUE; }
	virtual BOOL renameItem(const std::string& new_name) { mName = new_name; return TRUE; }
	virtual BOOL isItemMovable( void ) const { return FALSE; }
	virtual BOOL isItemRemovable( void ) const { return FALSE; }
	virtual BOOL isItemInTrash( void) const { return FALSE; }
	virtual BOOL removeItem() { return FALSE; }
	virtual void removeBatch(std::vector<LLFolderViewModelItem*>& batch) { }
	virtual void move( LLFolderViewModelItem* parent_listener ) { }
	virtual BOOL isItemCopyable() const { return FALSE; }
	virtual BOOL copyToClipboard() const { return FALSE; }
	virtual BOOL cutToClipboard() const { return FALSE; }
	virtual BOOL isClipboardPasteable() const { return FALSE; }
	virtual void pasteFromClipboard() { }
	virtual void pasteLinkFromClipboard() { }
	virtual void buildContextMenu(LLMenuGL& menu, U32 flags) { }
	virtual BOOL isUpToDate() const { return TRUE; }
	virtual bool hasChildren() const { return FALSE; }

	virtual bool potentiallyVisible() { return true; }
	virtual bool filter( LLFolderViewFilter& filter) { return false; }
	virtual bool descendantsPassedFilter(S32 filter_generation = -1) { return true; }
	virtual void setPassedFilter(bool passed, S32 filter_generation, std::string::size_type string_offset = std::string::npos, std::string::size_type string_size = 0) { }
	virtual bool passedFilter(S32 filter_generation = -1) { return true; }

	// The action callbacks
	virtual void performAction(LLInventoryModel* model, std::string action);
	virtual void openItem( void );
	virtual void closeItem( void );
	virtual void previewItem( void );
	virtual void selectItem(void) { } 
	virtual void showProperties(void);

	// This method will be called to determine if a drop can be
	// performed, and will set drop to TRUE if a drop is
	// requested. 
	// Returns TRUE if a drop is possible/happened, FALSE otherwise.
	virtual BOOL dragOrDrop(MASK mask, BOOL drop,
		EDragAndDropType cargo_type,
		void* cargo_data,
		std::string& tooltip_msg) { return FALSE; }

	const LLUUID& getID() {return mID;}
	void postEvent(const std::string& event_type, LLPersonTabModel* session, LLPersonModel* participant);

protected:

	std::string mName;	// Name of the session or the participant
	LLUUID mID;
};	

class LLPersonTabModel : public LLPersonModelCommon
{
public:
	LLPersonTabModel(std::string display_name, LLFolderViewModelInterface& root_view_model);
	LLPersonTabModel(LLFolderViewModelInterface& root_view_model);

	LLPointer<LLUIImage> getIcon() const { return NULL; }
	void addParticipant(LLPersonModel* participant);
	void removeParticipant(LLPersonModel* participant);
	void removeParticipant(const LLUUID& participant_id);
	void clearParticipants();
	LLPersonModel* findParticipant(const LLUUID& person_id);

private:
};

class LLPersonModel : public LLPersonModelCommon
{
public:
	LLPersonModel(std::string display_name, LLFolderViewModelInterface& root_view_model);
	LLPersonModel(LLFolderViewModelInterface& root_view_model);

private:

};


#endif // LL_LLPERSONMODELCOMMON_H

