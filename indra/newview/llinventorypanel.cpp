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
#include "llinventorypanel.h"

#include <utility> // for std::pair<>

#include "llagent.h"
#include "llagentwearables.h"
#include "llappearancemgr.h"
#include "llfloaterinventory.h"
#include "llfloaterreg.h"
#include "llimfloater.h"
#include "llimview.h"
#include "llinventorybridge.h"
#include "llsidepanelinventory.h"
#include "llsidetray.h"
#include "llscrollcontainer.h"
#include "llviewerfoldertype.h"
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
	virtual void changed(U32 mask) 
	{
		mIP->modelChanged(mask);
	}
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
	mViewsInitialized(false),
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
	
	if (mStartFolderString != "")
	{
		mBuildDefaultHierarchy = false;
	}
}

BOOL LLInventoryPanel::postBuild()
{
	LLMemType mt(LLMemType::MTYPE_INVENTORY_POST_BUILD);

	mCommitCallbackRegistrar.pushScope(); // registered as a widget; need to push callback scope ourselves
	
	// Create root folder
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
	
	// Scroller
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
		addChild(mScroller);
		mScroller->addChild(mFolders);
		mFolders->setScrollContainer(mScroller);
	}

	// Set up the callbacks from the inventory we're viewing, and then build everything.
	mInventoryObserver = new LLInventoryPanelObserver(this);
	mInventory->addObserver(mInventoryObserver);

	// Build view of inventory if we need default full hierarchy and inventory ready,
	// otherwise wait for idle callback.
	if (mBuildDefaultHierarchy && mInventory->isInventoryUsable() && !mViewsInitialized)
	{
		initializeViews();
	}
	gIdleCallbacks.addFunction(onIdle, (void*)this);

	if (mSortOrderSetting != INHERIT_SORT_ORDER)
	{
		setSortOrder(gSavedSettings.getU32(mSortOrderSetting));
	}
	else
	{
		setSortOrder(gSavedSettings.getU32(DEFAULT_SORT_ORDER));
	}
	mFolders->setSortOrder(getFilter()->getSortOrder());

	return TRUE;
}

LLInventoryPanel::~LLInventoryPanel()
{
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

void LLInventoryPanel::draw()
{
	// Select the desired item (in case it wasn't loaded when the selection was requested)
	mFolders->updateSelection();
	LLPanel::draw();
}

LLInventoryFilter* LLInventoryPanel::getFilter()
{
	if (mFolders) 
	{
		return mFolders->getFilter();
	}
	return NULL;
}

void LLInventoryPanel::setFilterTypes(U64 types, LLInventoryFilter::EFilterType filter_type)
{
	if (filter_type == LLInventoryFilter::FILTERTYPE_OBJECT)
		getFilter()->setFilterObjectTypes(types);
	if (filter_type == LLInventoryFilter::FILTERTYPE_CATEGORY)
		getFilter()->setFilterCategoryTypes(types);
}

void LLInventoryPanel::setFilterPermMask(PermissionMask filter_perm_mask)
{
	getFilter()->setFilterPermissions(filter_perm_mask);
}

void LLInventoryPanel::setFilterSubString(const std::string& string)
{
	getFilter()->setFilterSubString(string);
}

void LLInventoryPanel::setSortOrder(U32 order)
{
	getFilter()->setSortOrder(order);
	if (getFilter()->isModified())
	{
		mFolders->setSortOrder(order);
		// try to keep selection onscreen, even if it wasn't to start with
		mFolders->scrollToShowSelection();
	}
}

void LLInventoryPanel::setSinceLogoff(BOOL sl)
{
	getFilter()->setDateRangeLastLogoff(sl);
}

void LLInventoryPanel::setHoursAgo(U32 hours)
{
	getFilter()->setHoursAgo(hours);
}

void LLInventoryPanel::setShowFolderState(LLInventoryFilter::EFolderShow show)
{
	getFilter()->setShowFolderState(show);
}

LLInventoryFilter::EFolderShow LLInventoryPanel::getShowFolderState()
{
	return getFilter()->getShowFolderState();
}

void LLInventoryPanel::modelChanged(U32 mask)
{
	static LLFastTimer::DeclareTimer FTM_REFRESH("Inventory Refresh");
	LLFastTimer t2(FTM_REFRESH);

	bool handled = false;

	if (!mViewsInitialized) return;
	
	const LLInventoryModel* model = getModel();
	if (!model) return;

	const LLInventoryModel::changed_items_t& changed_items = model->getChangedIDs();
	if (changed_items.empty()) return;

	for (LLInventoryModel::changed_items_t::const_iterator items_iter = changed_items.begin();
		 items_iter != changed_items.end();
		 ++items_iter)
	{
		const LLUUID& item_id = (*items_iter);
		const LLInventoryObject* model_item = model->getObject(item_id);
		LLFolderViewItem* view_item = mFolders->getItemByID(item_id);
		LLFolderViewFolder* view_folder = mFolders->getFolderByID(item_id);

		//////////////////////////////
		// LABEL Operation
		// Empty out the display name for relabel.
		if (mask & LLInventoryObserver::LABEL)
		{
			handled = true;
			if (view_item)
			{
				// Request refresh on this item (also flags for filtering)
				LLInvFVBridge* bridge = (LLInvFVBridge*)view_item->getListener();
				if(bridge)
				{	// Clear the display name first, so it gets properly re-built during refresh()
					bridge->clearDisplayName();
				}
				view_item->refresh();
			}
		}

		//////////////////////////////
		// REBUILD Operation
		// Destroy and regenerate the UI.
		if (mask & LLInventoryObserver::REBUILD)
		{
			handled = true;
			if (model_item && view_item)
			{
				view_item->destroyView();
			}
			buildNewViews(item_id);
		}

		//////////////////////////////
		// INTERNAL Operation
		// This could be anything.  For now, just refresh the item.
		if (mask & LLInventoryObserver::INTERNAL)
		{
			if (view_item)
			{
				view_item->refresh();
			}
		}

		//////////////////////////////
		// SORT Operation
		// Sort the folder.
		if (mask & LLInventoryObserver::SORT)
		{
			if (view_folder)
			{
				view_folder->requestSort();
			}
		}	

		// We don't typically care which of these masks the item is actually flagged with, since the masks
		// may not be accurate (e.g. in the main inventory panel, I move an item from My Inventory into
		// Landmarks; this is a STRUCTURE change for that panel but is an ADD change for the Landmarks
		// panel).  What's relevant is that the item and UI are probably out of sync and thus need to be
		// resynchronized.
		if (mask & (LLInventoryObserver::STRUCTURE |
					LLInventoryObserver::ADD |
					LLInventoryObserver::REMOVE))
		{
			handled = true;

			//////////////////////////////
			// ADD Operation
			// Item exists in memory but a UI element hasn't been created for it.
			if (model_item && !view_item)
			{
				// Add the UI element for this item.
				buildNewViews(item_id);
				// Select any newly created object that has the auto rename at top of folder root set.
				if(mFolders->getRoot()->needsAutoRename())
				{
					setSelection(item_id, FALSE);
				}
			}

			//////////////////////////////
			// STRUCTURE Operation
			// This item already exists in both memory and UI.  It was probably reparented.
			if (model_item && view_item)
			{
				// Don't process the item if it's hanging from the root, since its
				// model_item's parent will be NULL.
				if (view_item->getRoot() != view_item->getParent())
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
			}
			
			//////////////////////////////
			// REMOVE Operation
			// This item has been removed from memory, but its associated UI element still exists.
			if (!model_item && view_item)
			{
				// Remove the item's UI.
				view_item->destroyView();
			}
		}
	}
}

// static
void LLInventoryPanel::onIdle(void *userdata)
{
	if (!gInventory.isInventoryUsable())
		return;

	LLInventoryPanel *self = (LLInventoryPanel*)userdata;
	// Inventory just initialized, do complete build
	if (!self->mViewsInitialized)
	{
		self->initializeViews();
	}
	if (self->mViewsInitialized)
	{
		gIdleCallbacks.deleteFunction(onIdle, (void*)self);
	}
}

void LLInventoryPanel::initializeViews()
{
	if (!gInventory.isInventoryUsable()) return;

	// Determine the root folder in case specified, and
	// build the views starting with that folder.
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

	mViewsInitialized = true;
	openStartFolderOrMyInventory();
}

void LLInventoryPanel::rebuildViewsFor(const LLUUID& id)
{
	// Destroy the old view for this ID so we can rebuild it.
	LLFolderViewItem* old_view = mFolders->getItemByID(id);
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
		{
			parent_folder = mFolders;
		}
		else if ((mStartFolderID != LLUUID::null) && (!gInventory.isObjectDescendentOf(id, mStartFolderID)))
		{
			// This item exists outside the inventory's hierarchy, so don't add it.
			return;
		}
		
		if (objectp->getType() <= LLAssetType::AT_NONE ||
			objectp->getType() >= LLAssetType::AT_COUNT)
		{
			llwarns << "LLInventoryPanel::buildNewViews called with invalid objectp->mType : "
					<< ((S32) objectp->getType()) << " name " << objectp->getName() << " UUID " << objectp->getUUID() 
					<< llendl;
			return;
		}
		
		if ((objectp->getType() == LLAssetType::AT_CATEGORY) &&
			(objectp->getActualType() != LLAssetType::AT_LINK_FOLDER))
		{
			LLInvFVBridge* new_listener = mInvFVBridgeBuilder->createBridge(objectp->getType(),
																			objectp->getType(),
																			LLInventoryType::IT_CATEGORY,
																			this,
																			objectp->getUUID());
			
			if (new_listener)
			{
				LLFolderViewFolder::Params params;
				params.name = new_listener->getDisplayName();
				params.icon = new_listener->getIcon();
				params.icon_open = new_listener->getOpenIcon();
				params.root = mFolders;
				params.listener = new_listener;
				params.tool_tip = params.name;
				LLFolderViewFolder* folderp = LLUICtrlFactory::create<LLFolderViewFolder>(params);
				folderp->setItemSortOrder(mFolders->getSortOrder());
				itemp = folderp;

				// Hide the root folder, so we can show the contents of a folder flat
				// but still have the parent folder present for listener-related operations.
				if (id == mStartFolderID)
				{
					folderp->setHidden(TRUE);
				}
			}
		}
		else 
		{
			// Build new view for item.
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
				params.name = new_listener->getDisplayName();
				params.icon = new_listener->getIcon();
				params.icon_open = new_listener->getOpenIcon();
				params.creation_date = new_listener->getCreationDate();
				params.root = mFolders;
				params.listener = new_listener;
				params.rect = LLRect (0, 0, 0, 0);
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
			for (LLViewerInventoryCategory::cat_array_t::const_iterator cat_iter = categories->begin();
				 cat_iter != categories->end();
				 ++cat_iter)
			{
				const LLViewerInventoryCategory* cat = (*cat_iter);
				buildNewViews(cat->getUUID());
			}
		}
		
		if(items)
		{
			for (LLViewerInventoryItem::item_array_t::const_iterator item_iter = items->begin();
				 item_iter != items->end();
				 ++item_iter)
			{
				const LLViewerInventoryItem* item = (*item_iter);
				buildNewViews(item->getUUID());
			}
		}
		mInventory->unlockDirectDescendentArrays(id);
	}
}

// bit of a hack to make sure the inventory is open.
void LLInventoryPanel::openStartFolderOrMyInventory()
{
	if (mStartFolderString != "")
	{
		mFolders->openFolder(mStartFolderString);
	}
	else
	{
		// Find My Inventory folder and open it up by name
		for (LLView *child = mFolders->getFirstChild(); child; child = mFolders->findNextSibling(child))
		{
			LLFolderViewFolder *fchild = dynamic_cast<LLFolderViewFolder*>(child);
			if (fchild && fchild->getListener() &&
				(fchild->getListener()->getUUID() == gInventory.getRootFolderID()))
			{
				const std::string& child_name = child->getName();
				mFolders->openFolder(child_name);
				break;
			}
		}
	}
}

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

	// If folder view is empty the (x, y) point won't be in its rect
	// so the handler must be called explicitly.
	if (!mFolders->hasVisibleChildren())
	{
		handled = mFolders->handleDragAndDrop(x, y, mask, drop, cargo_type, cargo_data, accept, tooltip_msg);
	}

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

BOOL LLInventoryPanel::getSinceLogoff()
{
	return getFilter()->isSinceLogoff();
}

// DEBUG ONLY
// static 
void LLInventoryPanel::dumpSelectionInformation(void* user_data)
{
	LLInventoryPanel* iv = (LLInventoryPanel*)user_data;
	iv->mFolders->dumpSelectionInformation();
}

BOOL is_inventorysp_active()
{
	if (!LLSideTray::getInstance()->isPanelActive("sidepanel_inventory")) return FALSE;
	LLSidepanelInventory *inventorySP = dynamic_cast<LLSidepanelInventory *>(LLSideTray::getInstance()->getPanel("sidepanel_inventory"));
	if (!inventorySP) return FALSE; 
	return inventorySP->isMainInventoryPanelActive();
}

// static
LLInventoryPanel* LLInventoryPanel::getActiveInventoryPanel(BOOL auto_open)
{
	// A. If the inventory side panel is open, use that preferably.
	if (is_inventorysp_active())
	{
		LLSidepanelInventory *inventorySP = dynamic_cast<LLSidepanelInventory *>(LLSideTray::getInstance()->getPanel("sidepanel_inventory"));
		if (inventorySP)
		{
			return inventorySP->getActivePanel();
		}
	}
	
	// B. Iterate through the inventory floaters and return whichever is on top.
	LLFloaterReg::const_instance_list_t& inst_list = LLFloaterReg::getFloaterList("inventory");
	S32 z_min = S32_MAX;
	LLInventoryPanel* res = NULL;
	for (LLFloaterReg::const_instance_list_t::const_iterator iter = inst_list.begin(); iter != inst_list.end(); ++iter)
	{
		LLFloaterInventory* iv = dynamic_cast<LLFloaterInventory*>(*iter);
		if (iv && iv->getVisible())
		{
			S32 z_order = gFloaterView->getZOrder(iv);
			if (z_order < z_min)
			{
				res = iv->getPanel();
				z_min = z_order;
			}
		}
	}
	if (res) return res;
		
	// C. If no panels are open and we don't want to force open a panel, then just abort out.
	if (!auto_open) return NULL;
	
	// D. Open the inventory side panel and use that.
    LLSD key;
	LLSidepanelInventory *sidepanel_inventory =
		dynamic_cast<LLSidepanelInventory *>(LLSideTray::getInstance()->showPanel("sidepanel_inventory", key));
	if (sidepanel_inventory)
	{
		return sidepanel_inventory->getActivePanel();
	}

	return NULL;
}
