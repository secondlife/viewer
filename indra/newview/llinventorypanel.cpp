/* 
 * @file llinventorypanel.cpp
 * @brief Implementation of the inventory panel and associated stuff.
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

#include "llviewerprecompiledheaders.h"

#include <utility> // for std::pair<>

#include "llinventorypanel.h"

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llscrollcontainer.h"
#include "llviewerfoldertype.h"
#include "llimfloater.h"
#include "llvoavatarself.h"

static LLDefaultChildRegistry::Register<LLInventoryPanel> r("inventory_panel");

const std::string LLInventoryPanel::DEFAULT_SORT_ORDER = std::string("InventorySortOrder");
const std::string LLInventoryPanel::RECENTITEMS_SORT_ORDER = std::string("RecentItemsSortOrder");
const std::string LLInventoryPanel::INHERIT_SORT_ORDER = std::string("");
static const LLInventoryFVBridgeBuilder INVENTORY_BRIDGE_BUILDER;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLInventoryPanelObserver
//
// Bridge to support knowing when the inventory has changed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLInventoryPanelObserver : public LLInventoryObserver
{
public:
	LLInventoryPanelObserver(LLInventoryPanel* ip) : mIP(ip) {}
	virtual ~LLInventoryPanelObserver() {}
	virtual void changed(U32 mask);
protected:
	LLInventoryPanel* mIP;
};

LLInventoryPanel::LLInventoryPanel(const LLInventoryPanel::Params& p) :	
	LLPanel(p),
	mInventoryObserver(NULL),
	mFolders(NULL),
	mScroller(NULL),
	mSortOrderSetting(p.sort_order_setting),
	mInventory(p.inventory),
	mAllowMultiSelect(p.allow_multi_select),
	mHasInventoryConnection(false),
	mStartFolderString(p.start_folder),	
	mBuildDefaultHierarchy(true),
	mInvFVBridgeBuilder(NULL)
{
	mInvFVBridgeBuilder = &INVENTORY_BRIDGE_BUILDER;

	// contex menu callbacks
	mCommitCallbackRegistrar.add("Inventory.DoToSelected", boost::bind(&LLInventoryPanel::doToSelected, this, _2));
	mCommitCallbackRegistrar.add("Inventory.EmptyTrash", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyTrash", LLFolderType::FT_TRASH));
	mCommitCallbackRegistrar.add("Inventory.EmptyLostAndFound", boost::bind(&LLInventoryModel::emptyFolderType, &gInventory, "ConfirmEmptyLostAndFound", LLFolderType::FT_LOST_AND_FOUND));
	mCommitCallbackRegistrar.add("Inventory.DoCreate", boost::bind(&LLInventoryPanel::doCreate, this, _2));
	mCommitCallbackRegistrar.add("Inventory.AttachObject", boost::bind(&LLInventoryPanel::attachObject, this, _2));
	mCommitCallbackRegistrar.add("Inventory.BeginIMSession", boost::bind(&LLInventoryPanel::beginIMSession, this));
	
	setBackgroundColor(LLUIColorTable::instance().getColor("InventoryBackgroundColor"));
	setBackgroundVisible(TRUE);
	setBackgroundOpaque(TRUE);
}

BOOL LLInventoryPanel::postBuild()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_POST_BUILD);

	mCommitCallbackRegistrar.pushScope(); // registered as a widget; need to push callback scope ourselves
	
	// create root folder
	{
		LLRect folder_rect(0,
						   0,
						   getRect().getWidth(),
						   0);
		LLFolderView::Params p;
		p.name = getName();
		p.rect = folder_rect;
		p.parent_panel = this;
		p.tool_tip = p.name;
		mFolders = LLUICtrlFactory::create<LLFolderView>(p);
		mFolders->setAllowMultiSelect(mAllowMultiSelect);
	}

	mCommitCallbackRegistrar.popScope();
	
	mFolders->setCallbackRegistrar(&mCommitCallbackRegistrar);
	
	// scroller
	{
		LLRect scroller_view_rect = getRect();
		scroller_view_rect.translate(-scroller_view_rect.mLeft, -scroller_view_rect.mBottom);
		LLScrollContainer::Params p;
		p.name("Inventory Scroller");
		p.rect(scroller_view_rect);
		p.follows.flags(FOLLOWS_ALL);
		p.reserve_scroll_corner(true);
		p.tab_stop(true);
		mScroller = LLUICtrlFactory::create<LLScrollContainer>(p);
	}
	addChild(mScroller);
	mScroller->addChild(mFolders);
	
	mFolders->setScrollContainer(mScroller);

	// set up the callbacks from the inventory we're viewing, and then
	// build everything.
	mInventoryObserver = new LLInventoryPanelObserver(this);
	mInventory->addObserver(mInventoryObserver);

	// build view of inventory if we need default full hierarchy and inventory ready, otherwise wait for modelChanged() callback
	if (mBuildDefaultHierarchy && mInventory->isInventoryUsable() && !mHasInventoryConnection)
	{
		rebuildViews();
		mHasInventoryConnection = true;
		defaultOpenInventory();
	}

	if (mSortOrderSetting != INHERIT_SORT_ORDER)
	{
		setSortOrder(gSavedSettings.getU32(mSortOrderSetting));
	}
	else
	{
		setSortOrder(gSavedSettings.getU32(DEFAULT_SORT_ORDER));
	}
	mFolders->setSortOrder(mFolders->getFilter()->getSortOrder());

	return TRUE;
}

LLInventoryPanel::~LLInventoryPanel()
{
	// should this be a global setting?
	if (mFolders)
	{
		U32 sort_order = mFolders->getSortOrder();
		if (mSortOrderSetting != INHERIT_SORT_ORDER)
		{
			gSavedSettings.setU32(mSortOrderSetting, sort_order);
		}
	}

	// LLView destructor will take care of the sub-views.
	mInventory->removeObserver(mInventoryObserver);
	delete mInventoryObserver;
	mScroller = NULL;
}

LLMemType mt(LLMemType::MTYPE_INVENTORY_FROM_XML); // ! BUG ! Should this be removed?
void LLInventoryPanel::draw()
{
	// select the desired item (in case it wasn't loaded when the selection was requested)
	mFolders->updateSelection();
	LLPanel::draw();
}

LLInventoryFilter* LLInventoryPanel::getFilter()
{
	if (mFolders) return mFolders->getFilter();
	return NULL;
}

void LLInventoryPanel::setFilterTypes(U64 filter_types, BOOL filter_for_categories)
{
	mFolders->getFilter()->setFilterTypes(filter_types, filter_for_categories);
}	

void LLInventoryPanel::setFilterPermMask(PermissionMask filter_perm_mask)
{
	mFolders->getFilter()->setFilterPermissions(filter_perm_mask);
}

void LLInventoryPanel::setFilterSubString(const std::string& string)
{
	mFolders->getFilter()->setFilterSubString(string);
}

void LLInventoryPanel::setSortOrder(U32 order)
{
	mFolders->getFilter()->setSortOrder(order);
	if (mFolders->getFilter()->isModified())
	{
		mFolders->setSortOrder(order);
		// try to keep selection onscreen, even if it wasn't to start with
		mFolders->scrollToShowSelection();
	}
}

void LLInventoryPanel::setSinceLogoff(BOOL sl)
{
	mFolders->getFilter()->setDateRangeLastLogoff(sl);
}

void LLInventoryPanel::setHoursAgo(U32 hours)
{
	mFolders->getFilter()->setHoursAgo(hours);
}

void LLInventoryPanel::setShowFolderState(LLInventoryFilter::EFolderShow show)
{
	mFolders->getFilter()->setShowFolderState(show);
}

LLInventoryFilter::EFolderShow LLInventoryPanel::getShowFolderState()
{
	return mFolders->getFilter()->getShowFolderState();
}

static LLFastTimer::DeclareTimer FTM_REFRESH("Inventory Refresh");

void LLInventoryPanel::modelChanged(U32 mask)
{
	LLFastTimer t2(FTM_REFRESH);

	bool handled = false;

	// inventory just initialized, do complete build
	if ((mask & LLInventoryObserver::ADD) && gInventory.getChangedIDs().empty() && !mHasInventoryConnection)
	{
		rebuildViews();
		mHasInventoryConnection = true;
		defaultOpenInventory();
		return;
	}

	if (mask & LLInventoryObserver::LABEL)
	{
		handled = true;
		// label change - empty out the display name for each object
		// in this change set.
		const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();
		std::set<LLUUID>::const_iterator id_it = changed_items.begin();
		std::set<LLUUID>::const_iterator id_end = changed_items.end();
		LLFolderViewItem* view = NULL;
		LLInvFVBridge* bridge = NULL;
		for (;id_it != id_end; ++id_it)
		{
			view = mFolders->getItemByID(*id_it);
			if(view)
			{
				// request refresh on this item (also flags for filtering)
				bridge = (LLInvFVBridge*)view->getListener();
				if(bridge)
				{	// Clear the display name first, so it gets properly re-built during refresh()
					bridge->clearDisplayName();
				}
				view->refresh();
			}
		}
	}

	// We don't really care which of these masks the item is actually flagged with, since the masks
	// may not be accurate (e.g. in the main inventory panel, I move an item from My Inventory into
	// Landmarks; this is a STRUCTURE change for that panel but is an ADD change for the Landmarks
	// panel).  What's relevant is that the item and UI are probably out of sync and thus need to be
	// resynchronized.
	if (mask & (LLInventoryObserver::STRUCTURE |
				LLInventoryObserver::ADD |
				LLInventoryObserver::REMOVE))
	{
		handled = true;
		// Record which folders are open by uuid.
		LLInventoryModel* model = getModel();
		if (model)
		{
			const std::set<LLUUID>& changed_items = gInventory.getChangedIDs();

			std::set<LLUUID>::const_iterator id_it = changed_items.begin();
			std::set<LLUUID>::const_iterator id_end = changed_items.end();
			for (;id_it != id_end; ++id_it)
			{
				// sync view with model
				LLInventoryObject* model_item = model->getObject(*id_it);
				LLFolderViewItem* view_item = mFolders->getItemByID(*id_it);

				// Item exists in memory but a UI element hasn't been created for it.
				if (model_item && !view_item)
				{
					// Add the UI element for this item.
					buildNewViews(*id_it);
					// Select any newly created object that has the auto rename at top of folder root set.
					if(mFolders->getRoot()->needsAutoRename())
					{
						setSelection(*id_it, FALSE);
					}
				}

				// This item already exists in both memory and UI.  It was probably moved
				// around in the panel's directory structure (i.e. reparented).
				if (model_item && view_item)
				{
					LLFolderViewFolder* new_parent = (LLFolderViewFolder*)mFolders->getItemByID(model_item->getParentUUID());

					// Item has been moved.
					if (view_item->getParentFolder() != new_parent)
					{
						if (new_parent != NULL)
						{
							// Item is to be moved and we found its new parent in the panel's directory, so move the item's UI.
							view_item->getParentFolder()->extractItem(view_item);
							view_item->addToFolder(new_parent, mFolders);
						}
						else 
						{
							// Item is to be moved outside the panel's directory (e.g. moved to trash for a panel that 
							// doesn't include trash).  Just remove the item's UI.
							view_item->destroyView();
						}
					}
				}

				// This item has been removed from memory, but its associated UI element still exists.
				if (!model_item && view_item)
				{
					// Remove the item's UI.
					view_item->destroyView();
				}
			}
		}
	}

	if (!handled)
	{
		// it's a small change that only requires a refresh.
		// *TODO: figure out a more efficient way to do the refresh
		// since it is expensive on large inventories
		mFolders->refresh();
	}
}


void LLInventoryPanel::rebuildViews()
{
	// Determine the root folder and rebuild the views starting
	// at that folder.
	const LLFolderType::EType preferred_type = LLViewerFolderType::lookupTypeFromNewCategoryName(mStartFolderString);

	if ("LIBRARY" == mStartFolderString)
	{
		mStartFolderID = gInventory.getLibraryRootFolderID();
	}
	else
	{
		mStartFolderID = (preferred_type != LLFolderType::FT_NONE ? gInventory.findCategoryUUIDForType(preferred_type) : LLUUID::null);
	}
	
	rebuildViewsFor(mStartFolderID);
}

void LLInventoryPanel::rebuildViewsFor(const LLUUID& id)
{
	LLFolderViewItem* old_view = NULL;

	// get old LLFolderViewItem
	old_view = mFolders->getItemByID(id);
	if (old_view && id.notNull())
	{
		old_view->destroyView();
	}

	buildNewViews(id);
}

void LLInventoryPanel::buildNewViews(const LLUUID& id)
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_BUILD_NEW_VIEWS);
	LLFolderViewItem* itemp = NULL;
	LLInventoryObject* objectp = gInventory.getObject(id);
	if (objectp)
	{
		const LLUUID &parent_id = objectp->getParentUUID();
		LLFolderViewFolder* parent_folder = (LLFolderViewFolder*)mFolders->getItemByID(parent_id);
		if (id == mStartFolderID)
			parent_folder = mFolders;
		
		if (!parent_folder)
		{
			// This item exists outside the inventory's hierarchy, so don't add it.
			return;
		}
		
		if (objectp->getType() <= LLAssetType::AT_NONE ||
			objectp->getType() >= LLAssetType::AT_COUNT)
		{
			llwarns << "LLInventoryPanel::buildNewViews called with invalid objectp->mType : " << 
				((S32) objectp->getType()) << " name " << objectp->getName() << " UUID " << objectp->getUUID() << llendl;
			return;
		}
		
		if (objectp->getType() == LLAssetType::AT_CATEGORY &&
			objectp->getActualType() != LLAssetType::AT_LINK_FOLDER) 
		{
			LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(objectp->getType(),
																			objectp->getType(),
																			LLInventoryType::IT_CATEGORY,
																			this,
																			objectp->getUUID());
			
			if (new_listener)
			{
				LLFolderViewFolder::Params p;
				p.name = new_listener->getDisplayName();
				p.icon = new_listener->getIcon();
				p.root = mFolders;
				p.listener = new_listener;
				p.tool_tip = p.name;
				LLFolderViewFolder* folderp = LLUICtrlFactory::create<LLFolderViewFolder>(p);
				folderp->setItemSortOrder(mFolders->getSortOrder());
				itemp = folderp;

				// Hide the root folder, so we can show the contents of a folder
				// flat but still have the parent folder present for listener-related
				// operations.
				if (id == mStartFolderID)
				{
					folderp->setDontShowInHierarchy(TRUE);
				}
			}
		}
		else 
		{
			// Build new view for item
			LLInventoryItem* item = (LLInventoryItem*)objectp;
			LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(item->getType(),
																			item->getActualType(),
																			item->getInventoryType(),
																			this,
																			item->getUUID(),
																			item->getFlags());

			if (new_listener)
			{
				LLFolderViewItem::Params params;
				params.name(new_listener->getDisplayName());
				params.icon(new_listener->getIcon());
				params.creation_date(new_listener->getCreationDate());
				params.root(mFolders);
				params.listener(new_listener);
				params.rect(LLRect (0, 0, 0, 0));
				params.tool_tip = params.name;
				itemp = LLUICtrlFactory::create<LLFolderViewItem> (params);
			}
		}

		if (itemp)
		{
			itemp->addToFolder(parent_folder, mFolders);
		}
	}

	// If this is a folder, add the children of the folder and recursively add any 
	// child folders.
	if ((id == mStartFolderID) ||
		(objectp && objectp->getType() == LLAssetType::AT_CATEGORY))
	{
		LLViewerInventoryCategory::cat_array_t* categories;
		LLViewerInventoryItem::item_array_t* items;

		mInventory->lockDirectDescendentArrays(id, categories, items);
		if(categories)
		{
			S32 count = categories->count();
			for(S32 i = 0; i < count; ++i)
			{
				LLInventoryCategory* cat = categories->get(i);
				buildNewViews(cat->getUUID());
			}
		}
		if(items)
		{
			S32 count = items->count();
			for(S32 i = 0; i < count; ++i)
			{
				LLInventoryItem* item = items->get(i);
				buildNewViews(item->getUUID());
			}
		}
		mInventory->unlockDirectDescendentArrays(id);
	}
}

// bit of a hack to make sure the inventory is open.
void LLInventoryPanel::defaultOpenInventory()
{
	const LLFolderType::EType preferred_type = LLViewerFolderType::lookupTypeFromNewCategoryName(mStartFolderString);
	if (preferred_type != LLFolderType::FT_NONE)
	{
		const std::string& top_level_folder_name = LLViewerFolderType::lookupNewCategoryName(preferred_type);
		mFolders->openFolder(top_level_folder_name);
	}
	else
	{
		// Get the first child (it should be "My Inventory") and
		// open it up by name (just to make sure the first child is actually a folder).
		LLView* first_child = mFolders->getFirstChild();
		const std::string& first_child_name = first_child->getName();
		mFolders->openFolder(first_child_name);
	}
}

struct LLConfirmPurgeData
{
	LLUUID mID;
	LLInventoryModel* mModel;
};

class LLIsNotWorn : public LLInventoryCollectFunctor
{
public:
	LLIsNotWorn() {}
	virtual ~LLIsNotWorn() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		return !gAgentWearables.isWearingItem(item->getUUID());
	}
};

class LLOpenFolderByID : public LLFolderViewFunctor
{
public:
	LLOpenFolderByID(const LLUUID& id) : mID(id) {}
	virtual ~LLOpenFolderByID() {}
	virtual void doFolder(LLFolderViewFolder* folder)
		{
			if (folder->getListener() && folder->getListener()->getUUID() == mID) folder->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_UP);
		}
	virtual void doItem(LLFolderViewItem* item) {}
protected:
	const LLUUID& mID;
};


void LLInventoryPanel::openSelected()
{
	LLFolderViewItem* folder_item = mFolders->getCurSelectedItem();
	if(!folder_item) return;
	LLInvFVBridge* bridge = (LLInvFVBridge*)folder_item->getListener();
	if(!bridge) return;
	bridge->openItem();
}

BOOL LLInventoryPanel::handleHover(S32 x, S32 y, MASK mask)
{
	BOOL handled = LLView::handleHover(x, y, mask);
	if(handled)
	{
		ECursorType cursor = getWindow()->getCursor();
		if (LLInventoryModel::backgroundFetchActive() && cursor == UI_CURSOR_ARROW)
		{
			// replace arrow cursor with arrow and hourglass cursor
			getWindow()->setCursor(UI_CURSOR_WORKING);
		}
	}
	else
	{
		getWindow()->setCursor(UI_CURSOR_ARROW);
	}
	return TRUE;
}

BOOL LLInventoryPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg)
{

	BOOL handled = LLPanel::handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);

	if (handled)
	{
		mFolders->setDragAndDropThisFrame();
	}

	return handled;
}

void LLInventoryPanel::onFocusLost()
{
	// inventory no longer handles cut/copy/paste/delete
	if (LLEditMenuHandler::gEditMenuHandler == mFolders)
	{
		LLEditMenuHandler::gEditMenuHandler = NULL;
	}

	LLPanel::onFocusLost();
}

void LLInventoryPanel::onFocusReceived()
{
	// inventory now handles cut/copy/paste/delete
	LLEditMenuHandler::gEditMenuHandler = mFolders;

	LLPanel::onFocusReceived();
}


void LLInventoryPanel::openAllFolders()
{
	mFolders->setOpenArrangeRecursively(TRUE, LLFolderViewFolder::RECURSE_DOWN);
	mFolders->arrangeAll();
}

void LLInventoryPanel::openDefaultFolderForType(LLFolderType::EType type)
{
	LLUUID category_id = mInventory->findCategoryUUIDForType(type);
	LLOpenFolderByID opener(category_id);
	mFolders->applyFunctorRecursively(opener);
}

void LLInventoryPanel::setSelection(const LLUUID& obj_id, BOOL take_keyboard_focus)
{
	// Don't select objects in COF (e.g. to prevent refocus when items are worn).
	const LLInventoryObject *obj = gInventory.getObject(obj_id);
	if (obj && obj->getParentUUID() == LLAppearanceManager::instance().getCOF())
	{
		return;
	}
	mFolders->setSelectionByID(obj_id, take_keyboard_focus);
}

void LLInventoryPanel::clearSelection()
{
	mFolders->clearSelection();
}

void LLInventoryPanel::onSelectionChange(const std::deque<LLFolderViewItem*>& items, BOOL user_action)
{
	LLFolderView* fv = getRootFolder();
	if (fv->needsAutoRename()) // auto-selecting a new user-created asset and preparing to rename
	{
		fv->setNeedsAutoRename(FALSE);
		if (items.size()) // new asset is visible and selected
		{
			fv->startRenamingSelectedItem();
		}
	}
	// Seraph - Put determineFolderType in here for ensemble typing?
}

//----------------------------------------------------------------------------

void LLInventoryPanel::doToSelected(const LLSD& userdata)
{
	mFolders->doToSelected(&gInventory, userdata);
}

void LLInventoryPanel::doCreate(const LLSD& userdata)
{
	menu_create_inventory_item(mFolders, LLFolderBridge::sSelf, userdata);
}

bool LLInventoryPanel::beginIMSession()
{
	std::set<LLUUID> selected_items;
	mFolders->getSelectionList(selected_items);

	std::string name;
	static int session_num = 1;

	LLDynamicArray<LLUUID> members;
	EInstantMessage type = IM_SESSION_CONFERENCE_START;

	std::set<LLUUID>::const_iterator iter;
	for (iter = selected_items.begin(); iter != selected_items.end(); iter++)
	{

		LLUUID item = *iter;
		LLFolderViewItem* folder_item = mFolders->getItemByID(item);
			
		if(folder_item) 
		{
			LLFolderViewEventListener* fve_listener = folder_item->getListener();
			if (fve_listener && (fve_listener->getInventoryType() == LLInventoryType::IT_CATEGORY))
			{

				LLFolderBridge* bridge = (LLFolderBridge*)folder_item->getListener();
				if(!bridge) return true;
				LLViewerInventoryCategory* cat = bridge->getCategory();
				if(!cat) return true;
				name = cat->getName();
				LLUniqueBuddyCollector is_buddy;
				LLInventoryModel::cat_array_t cat_array;
				LLInventoryModel::item_array_t item_array;
				gInventory.collectDescendentsIf(bridge->getUUID(),
												cat_array,
												item_array,
												LLInventoryModel::EXCLUDE_TRASH,
												is_buddy);
				S32 count = item_array.count();
				if(count > 0)
				{
					LLFloaterReg::showInstance("communicate");
					// create the session
					LLAvatarTracker& at = LLAvatarTracker::instance();
					LLUUID id;
					for(S32 i = 0; i < count; ++i)
					{
						id = item_array.get(i)->getCreatorUUID();
						if(at.isBuddyOnline(id))
						{
							members.put(id);
						}
					}
				}
			}
			else
			{
				LLFolderViewItem* folder_item = mFolders->getItemByID(item);
				if(!folder_item) return true;
				LLInvFVBridge* listenerp = (LLInvFVBridge*)folder_item->getListener();

				if (listenerp->getInventoryType() == LLInventoryType::IT_CALLINGCARD)
				{
					LLInventoryItem* inv_item = gInventory.getItem(listenerp->getUUID());

					if (inv_item)
					{
						LLAvatarTracker& at = LLAvatarTracker::instance();
						LLUUID id = inv_item->getCreatorUUID();

						if(at.isBuddyOnline(id))
						{
							members.put(id);
						}
					}
				} //if IT_CALLINGCARD
			} //if !IT_CATEGORY
		}
	} //for selected_items	

	// the session_id is randomly generated UUID which will be replaced later
	// with a server side generated number

	if (name.empty())
	{
		name = llformat("Session %d", session_num++);
	}

	LLUUID session_id = gIMMgr->addSession(name, type, members[0], members);
	if (session_id != LLUUID::null)
	{
		LLIMFloater::show(session_id);
	}
		
	return true;
}

bool LLInventoryPanel::attachObject(const LLSD& userdata)
{
	std::set<LLUUID> selected_items;
	mFolders->getSelectionList(selected_items);

	std::string joint_name = userdata.asString();
	LLVOAvatar *avatarp = static_cast<LLVOAvatar*>(gAgent.getAvatarObject());
	LLViewerJointAttachment* attachmentp = NULL;
	for (LLVOAvatar::attachment_map_t::iterator iter = avatarp->mAttachmentPoints.begin(); 
		 iter != avatarp->mAttachmentPoints.end(); )
	{
		LLVOAvatar::attachment_map_t::iterator curiter = iter++;
		LLViewerJointAttachment* attachment = curiter->second;
		if (attachment->getName() == joint_name)
		{
			attachmentp = attachment;
			break;
		}
	}
	if (attachmentp == NULL)
	{
		return true;
	}

	for (std::set<LLUUID>::const_iterator set_iter = selected_items.begin(); 
		 set_iter != selected_items.end(); 
		 ++set_iter)
	{
		const LLUUID &id = *set_iter;
		LLViewerInventoryItem* item = (LLViewerInventoryItem*)gInventory.getItem(id);
		if(item && gInventory.isObjectDescendentOf(id, gInventory.getRootFolderID()))
		{
			rez_attachment(item, attachmentp);
		}
		else if(item && item->isComplete())
		{
			// must be in library. copy it to our inventory and put it on.
			LLPointer<LLInventoryCallback> cb = new RezAttachmentCallback(attachmentp);
			copy_inventory_item(gAgent.getID(),
								item->getPermissions().getOwner(),
								item->getUUID(),
								LLUUID::null,
								std::string(),
								cb);
		}
	}
	gFocusMgr.setKeyboardFocus(NULL);

	return true;
}


//----------------------------------------------------------------------------

// static DEBUG ONLY:
void LLInventoryPanel::dumpSelectionInformation(void* user_data)
{
	LLInventoryPanel* iv = (LLInventoryPanel*)user_data;
	iv->mFolders->dumpSelectionInformation();
}

BOOL LLInventoryPanel::getSinceLogoff()
{
	return mFolders->getFilter()->isSinceLogoff();
}

void example_param_block_usage()
{
	LLInventoryPanel::Params param_block;
	param_block.name(std::string("inventory"));

	param_block.sort_order_setting(LLInventoryPanel::RECENTITEMS_SORT_ORDER);
	param_block.allow_multi_select(true);
	param_block.filter(LLInventoryPanel::Filter()
			.sort_order(1)
			.types(0xffff0000));
	param_block.inventory(&gInventory);
	param_block.has_border(true);

	LLUICtrlFactory::create<LLInventoryPanel>(param_block);

	param_block = LLInventoryPanel::Params();
	param_block.name(std::string("inventory"));

	//LLSD param_block_sd;
	//param_block_sd["sort_order_setting"] = LLInventoryPanel::RECENTITEMS_SORT_ORDER;
	//param_block_sd["allow_multi_select"] = true;
	//param_block_sd["filter"]["sort_order"] = 1;
	//param_block_sd["filter"]["types"] = (S32)0xffff0000;
	//param_block_sd["has_border"] = true;

	//LLInitParam::LLSDParser(param_block_sd).parse(param_block);

	LLUICtrlFactory::create<LLInventoryPanel>(param_block);
}

// +=================================================+
// |        LLInventoryPanelObserver                 |
// +=================================================+
void LLInventoryPanelObserver::changed(U32 mask)
{
	mIP->modelChanged(mask);
}
