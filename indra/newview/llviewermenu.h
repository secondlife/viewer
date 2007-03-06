/** 
 * @file llviewermenu.h
 * @brief Builds menus out of objects
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLVIEWERMENU_H
#define LL_LLVIEWERMENU_H

#include "llassetstorage.h"
#include "llinventory.h"
#include "llmenugl.h"

//newview includes
#include "llfilepicker.h"

class LLUICtrl;
class LLView;
class LLParcelSelection;
class LLObjectSelection;

struct LLResourceData
{
	LLAssetInfo mAssetInfo;
	LLAssetType::EType mPreferredLocation;
	LLInventoryType::EType mInventoryType;
	U32 mNextOwnerPerm;
	void *mUserData;
};

void pre_init_menus();
void init_menus();
void cleanup_menus();

void show_debug_menus(); // checks for if menus should be shown first.
void show_context_menu( S32 x, S32 y, MASK mask );
void show_build_mode_context_menu(S32 x, S32 y, MASK mask);
void load_url_local_file(const char *file_name);
BOOL enable_save_into_inventory(void*);
void handle_reset_view();
void handle_cut(void*);
void handle_copy(void*);
void handle_paste(void*);
void handle_delete(void*);
void handle_redo(void*);
void handle_undo(void*);
void handle_select_all(void*);
void handle_deselect(void*);
void handle_delete_object();
void handle_duplicate(void*);
void handle_duplicate_in_place(void*);
BOOL enable_not_have_card(void *userdata);
void process_grant_godlike_powers(LLMessageSystem* msg, void**);

BOOL enable_cut(void*);
BOOL enable_copy(void*);
BOOL enable_paste(void*);
BOOL enable_select_all(void*);
BOOL enable_deselect(void*);
BOOL enable_undo(void*);
BOOL enable_redo(void*);

// returns TRUE if we have a friend relationship with agent_id
BOOL is_agent_friend(const LLUUID& agent_id);
BOOL is_agent_mappable(const LLUUID& agent_id);

void menu_toggle_control( void* user_data );
void check_toggle_control( LLUICtrl *, void* user_data );
void confirm_replace_attachment(S32 option, void* user_data);
void handle_detach_from_avatar(void* user_data);
void attach_label(LLString& label, void* user_data);
void detach_label(LLString& label, void* user_data);
BOOL object_selected_and_point_valid(void* user_data);
BOOL object_attached(void* user_data);
void handle_detach(void*);
BOOL enable_god_full(void* user_data);
BOOL enable_god_liaison(void* user_data);
BOOL enable_god_customer_service(void* user_data);
BOOL enable_god_basic(void* user_data);
void handle_show_newest_map(void*);

void exchange_callingcard(const LLUUID& dest_id);

void handle_gestures(void*);
void handle_sit_down(void*);
bool toggle_build_mode();
void handle_object_build(void*);
void handle_save_snapshot(void *);

bool handle_sit_or_stand();
bool handle_give_money_dialog();
bool handle_object_open();
bool handle_go_to();

void upload_new_resource(const LLString& src_filename, std::string name,
						 std::string desc, S32 compression_info,
						 LLAssetType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perm = PERM_NONE,
						 const LLString& display_name = LLString::null,
						 LLAssetStorage::LLStoreAssetCallback callback = NULL,
						 void *userdata = NULL);

void upload_new_resource(const LLTransactionID &tid, LLAssetType::EType type,
						 std::string name,
						 std::string desc, S32 compression_info,
						 LLAssetType::EType destination_folder_type,
						 LLInventoryType::EType inv_type,
						 U32 next_owner_perm = PERM_NONE,
						 const LLString& display_name = LLString::null,
						 LLAssetStorage::LLStoreAssetCallback callback = NULL,
						 void *userdata = NULL);

// Export to XML or Collada
void handle_export_selected( void * );

//Retrieve a list of valid extensions for a given file "type"
const char* build_extensions_string(LLFilePicker::ELoadFilter filter);

// Pass in an empty string and this function will build a string that
// describes buyer permissions.
class LLSaleInfo;
class LLPermissions;

class LLViewerMenuHolderGL : public LLMenuHolderGL
{
public:
	LLViewerMenuHolderGL();

	virtual BOOL hideMenus();
	
	void setParcelSelection(LLHandle<LLParcelSelection> selection);
	void setObjectSelection(LLHandle<LLObjectSelection> selection);

	virtual const LLRect getMenuRect() const;

protected:
	LLHandle<LLParcelSelection> mParcelSelection;
	LLHandle<LLObjectSelection> mObjectSelection;
};

extern const LLString SAVE_INTO_INVENTORY;

extern LLMenuBarGL*		gMenuBarView;
//extern LLView*			gMenuBarHolder;
extern LLMenuGL*		gPopupMenuView;
extern LLViewerMenuHolderGL*	gMenuHolder;

// Pie menus
extern LLPieMenu	*gPieSelf;
extern LLPieMenu	*gPieAvatar;
extern LLPieMenu	*gPieObject;
extern LLPieMenu	*gPieAttachment;
extern LLPieMenu	*gPieLand;
extern LLPieMenu*	gPieRate;

// Pie menus
extern LLPieMenu	*gPieSelfSimple;
extern LLPieMenu	*gPieAvatarSimple;
extern LLPieMenu	*gPieObjectSimple;
extern LLPieMenu	*gPieAttachmentSimple;
extern LLPieMenu	*gPieLandSimple;

// Needed to build menus when attachment site list available
extern LLMenuGL* gAttachSubMenu;
extern LLMenuGL* gDetachSubMenu;
extern LLMenuGL* gTakeOffClothes;
extern LLPieMenu* gAttachScreenPieMenu;
extern LLPieMenu* gDetachScreenPieMenu;
extern LLPieMenu* gAttachPieMenu;
extern LLPieMenu* gDetachPieMenu;
extern LLPieMenu* gAttachBodyPartPieMenus[8];
extern LLPieMenu* gDetachBodyPartPieMenus[8];

extern LLMenuItemCallGL* gAFKMenu;
extern LLMenuItemCallGL* gBusyMenu;
extern LLMenuItemCallGL* gMutePieMenu;
extern LLMenuItemCallGL* gMuteObjectPieMenu;
extern LLMenuItemCallGL* gBuyPassPieMenu;

#endif
