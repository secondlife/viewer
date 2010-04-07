/** 
 * @file llviewermenu.h
 * @brief Builds menus out of objects
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#ifndef LL_LLVIEWERMENU_H
#define LL_LLVIEWERMENU_H

#include "llmenugl.h"
#include "llsafehandle.h"

class LLMessageSystem;
class LLSD;
class LLUICtrl;
class LLView;
class LLParcelSelection;
class LLObjectSelection;
class LLSelectNode;

void init_menus();
void cleanup_menus();

void show_debug_menus(); // checks for if menus should be shown first.
void toggle_debug_menus(void*);
void show_context_menu( S32 x, S32 y, MASK mask );
void show_build_mode_context_menu(S32 x, S32 y, MASK mask);
void show_navbar_context_menu(LLView* ctrl, S32 x, S32 y);
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

BOOL is_agent_mappable(const LLUUID& agent_id);

void confirm_replace_attachment(S32 option, void* user_data);
void handle_detach_from_avatar(const LLSD& user_data);
void attach_label(std::string& label, const LLSD&);
void detach_label(std::string& label, const LLSD&);
void handle_detach(void*);
BOOL enable_god_full(void* user_data);
BOOL enable_god_liaison(void* user_data);
BOOL enable_god_basic(void* user_data);
void set_underclothes_menu_options();

void exchange_callingcard(const LLUUID& dest_id);

void handle_gestures(void*);
void handle_sit_down(void*);
void handle_object_build(void*);
void handle_object_touch();
bool enable_object_open();
void handle_object_open();

// Buy either contents or object itself
void handle_buy();
void handle_take_copy();
void handle_look_at_selection(const LLSD& param);
void handle_zoom_to_object(LLUUID object_id);

void handle_buy_land();

// Takes avatar UUID, or if no UUID passed, uses last selected object
void handle_avatar_freeze(const LLSD& avatar_id);

// Takes avatar UUID, or if no UUID passed, uses last selected object
void handle_avatar_eject(const LLSD& avatar_id);

bool enable_freeze_eject(const LLSD& avatar_id);

// Can anyone take a free copy of the object?
// *TODO: Move to separate file
bool anyone_copy_selection(LLSelectNode* nodep);

// Is this selected object for sale?
// *TODO: Move to separate file
bool for_sale_selection(LLSelectNode* nodep);

void handle_save_snapshot(void *);
void handle_toggle_flycam();

void handle_object_sit_or_stand();
void handle_give_money_dialog();
bool enable_pay_object();
bool enable_buy_object();
bool handle_go_to();

// Export to XML or Collada
void handle_export_selected( void * );

class LLViewerMenuHolderGL : public LLMenuHolderGL
{
public:
	struct Params : public LLInitParam::Block<Params, LLMenuHolderGL::Params>
	{};

	LLViewerMenuHolderGL(const Params& p);

	virtual BOOL hideMenus();
	
	void setParcelSelection(LLSafeHandle<LLParcelSelection> selection);
	void setObjectSelection(LLSafeHandle<LLObjectSelection> selection);

	virtual const LLRect getMenuRect() const;

protected:
	LLSafeHandle<LLParcelSelection> mParcelSelection;
	LLSafeHandle<LLObjectSelection> mObjectSelection;
};

extern const std::string SAVE_INTO_INVENTORY;

extern LLMenuBarGL*		gMenuBarView;
//extern LLView*			gMenuBarHolder;
extern LLMenuGL*		gEditMenu;
extern LLMenuGL*		gPopupMenuView;
extern LLViewerMenuHolderGL*	gMenuHolder;
extern LLMenuBarGL*		gLoginMenuBarView;

// Context menus in 3D scene
extern LLContextMenu		*gMenuAvatarSelf;
extern LLContextMenu		*gMenuAvatarOther;
extern LLContextMenu		*gMenuObject;
extern LLContextMenu		*gMenuAttachmentSelf;
extern LLContextMenu		*gMenuAttachmentOther;
extern LLContextMenu		*gMenuLand;

// Needed to build menus when attachment site list available
extern LLMenuGL* gAttachSubMenu;
extern LLMenuGL* gDetachSubMenu;
extern LLMenuGL* gTakeOffClothes;
extern LLContextMenu* gAttachScreenPieMenu;
extern LLContextMenu* gDetachScreenPieMenu;
extern LLContextMenu* gAttachPieMenu;
extern LLContextMenu* gDetachPieMenu;
extern LLContextMenu* gAttachBodyPartPieMenus[8];
extern LLContextMenu* gDetachBodyPartPieMenus[8];

extern LLMenuItemCallGL* gAFKMenu;
extern LLMenuItemCallGL* gBusyMenu;
extern LLMenuItemCallGL* gMutePieMenu;
extern LLMenuItemCallGL* gMuteObjectPieMenu;
extern LLMenuItemCallGL* gBuyPassPieMenu;

#endif
