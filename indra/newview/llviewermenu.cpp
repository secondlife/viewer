/** 
 * @file llviewermenu.cpp
 * @brief Builds menus out of items.
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llviewermenu.h" 

// system library includes
#include <iostream>
#include <fstream>
#include <sstream>

// linden library includes
#include "audioengine.h"
#include "indra_constants.h"
#include "llassetstorage.h"
#include "llchat.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llinstantmessage.h"
#include "llpermissionsflags.h"
#include "llrect.h"
#include "llsecondlifeurls.h"
#include "lltransactiontypes.h"
#include "llui.h"
#include "llview.h"
#include "llxfermanager.h"
#include "message.h"
#include "raytrace.h"
#include "llsdserialize.h"
#include "lltimer.h"
#include "llvfile.h"
#include "llvolumemgr.h"
#include "llwindow.h"	// for shell_open()

// newview includes
#include "llagent.h"

#include "llagentpilot.h"
#include "llbox.h"
#include "llcallingcard.h"
#include "llcameraview.h"
#include "llclipboard.h"
#include "llcompilequeue.h"
#include "llconsole.h"
#include "llviewercontrol.h"
#include "lldebugview.h"
#include "lldir.h"
#include "lldrawable.h"
#include "lldrawpoolalpha.h"
#include "lldrawpooltree.h"
#include "llface.h"
#include "llfirstuse.h"
#include "llfloater.h"
#include "llfloaterabout.h"
#include "llfloaterbuycurrency.h"
#include "llfloateravatarinfo.h"
#include "llfloateravatartextures.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbump.h"
#include "llfloaterbuy.h"
#include "llfloaterbuycontents.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloaterchat.h"
#include "llfloatercustomize.h"
#include "llfloaterdirectory.h"
#include "llfloatereditui.h"
#include "llfloaterfriends.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloatergroupinfo.h"
#include "llfloatergroups.h"
#include "llfloaterhtml.h"
#include "llfloaterhtmlhelp.h"
#include "llfloaterhtmlfind.h"
#include "llfloaterinspect.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermute.h"
#include "llfloateropenobject.h"
#include "llfloaterpermissionsmgr.h"
#include "llfloaterpreference.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llfloaterscriptdebug.h"
#include "llfloatertest.h"
#include "llfloatertools.h"
#include "llfloaterworldmap.h"
#include "llframestats.h"
#include "llframestatview.h"
#include "llfasttimerview.h"
#include "llmemoryview.h"
#include "llgivemoney.h"
#include "llgroupmgr.h"
#include "llhoverview.h"
#include "llhudeffecttrail.h"
#include "llhudmanager.h"
#include "llimage.h"
#include "llimagebmp.h"
#include "llimagej2c.h"
#include "llimagetga.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llkeyboard.h"
#include "llpanellogin.h"
#include "llmenucommands.h"
#include "llmenugl.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llmutelist.h"
#include "llnotify.h"
#include "llpanelobject.h"
#include "llparcel.h"
#include "llprimitive.h"
#include "llresmgr.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llstatusbar.h"
#include "llstatview.h"
#include "llstring.h"
#include "llsurfacepatch.h"
#include "llimview.h"
#include "lltextureview.h"
#include "lltool.h"
#include "lltoolbar.h"
#include "lltoolcomp.h"
#include "lltoolfocus.h"
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "lltoolplacer.h"
#include "lltoolselectland.h"
#include "llvieweruictrlfactory.h"
#include "lluploaddialog.h"
#include "lluserauth.h"
#include "lluuid.h"
#include "llvelocitybar.h"
#include "llviewercamera.h"
#include "llviewergenericmessage.h"
#include "llviewergesture.h"
#include "llviewerinventory.h"
#include "llviewermenufile.h"	// init_menu_file()
#include "llviewermessage.h"
#include "llviewernetwork.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvolume.h"
#include "llweb.h"
#include "llworld.h"
#include "llworldmap.h"
#include "object_flags.h"
#include "pipeline.h"
#include "viewer.h"
#include "roles_constants.h"
#include "llviewerjoystick.h"

#include "lltexlayer.h"

void init_client_menu(LLMenuGL* menu);
void init_server_menu(LLMenuGL* menu);

void init_debug_world_menu(LLMenuGL* menu);
void init_debug_rendering_menu(LLMenuGL* menu);
void init_debug_ui_menu(LLMenuGL* menu);
void init_debug_xui_menu(LLMenuGL* menu);
void init_debug_avatar_menu(LLMenuGL* menu);
void init_debug_baked_texture_menu(LLMenuGL* menu);

BOOL enable_land_build(void*);
BOOL enable_object_build(void*);

LLVOAvatar* find_avatar_from_object( LLViewerObject* object );
LLVOAvatar* find_avatar_from_object( const LLUUID& object_id );

void handle_test_load_url(void*);

extern void disconnect_viewer(void *);

//
// Evil hackish imported globals
//
extern BOOL	gRenderLightGlows;
extern BOOL gRenderAvatar;
extern BOOL	gHideSelectedObjects;
extern BOOL gShowOverlayTitle;
extern BOOL gRandomizeFramerate;
extern BOOL gPeriodicSlowFrame;
extern BOOL gOcclusionCull;
extern BOOL gAllowSelectAvatar;

//
// Globals
//

LLMenuBarGL		*gMenuBarView = NULL;
LLViewerMenuHolderGL	*gMenuHolder = NULL;
LLMenuGL		*gPopupMenuView = NULL;

// Pie menus
LLPieMenu	*gPieSelf	= NULL;
LLPieMenu	*gPieAvatar = NULL;
LLPieMenu	*gPieObject = NULL;
LLPieMenu	*gPieAttachment = NULL;
LLPieMenu	*gPieLand	= NULL;

// local constants
const LLString CLIENT_MENU_NAME("Client");
const LLString SERVER_MENU_NAME("Server");

const LLString SAVE_INTO_INVENTORY("Save Object Back to My Inventory");
const LLString SAVE_INTO_TASK_INVENTORY("Save Object Back to Object Contents");

#if LL_WINDOWS
static const char* SOUND_EXTENSIONS = ".wav";
static const char* IMAGE_EXTENSIONS = ".tga .bmp .jpg .jpeg";
static const char* ANIM_EXTENSIONS =  ".bvh";
#ifdef _CORY_TESTING
static const char* GEOMETRY_EXTENSIONS = ".slg";
#endif
static const char* XML_EXTENSIONS = ".xml";
static const char* SLOBJECT_EXTENSIONS = ".slobject";
#endif
static const char* ALL_FILE_EXTENSIONS = "*.*";

LLMenuGL* gAttachSubMenu = NULL;
LLMenuGL* gDetachSubMenu = NULL;
LLMenuGL* gTakeOffClothes = NULL;
LLPieMenu* gPieRate = NULL;
LLPieMenu* gAttachScreenPieMenu = NULL;
LLPieMenu* gAttachPieMenu = NULL;
LLPieMenu* gAttachBodyPartPieMenus[8];
LLPieMenu* gDetachPieMenu = NULL;
LLPieMenu* gDetachScreenPieMenu = NULL;
LLPieMenu* gDetachBodyPartPieMenus[8];

LLMenuItemCallGL* gAFKMenu = NULL;
LLMenuItemCallGL* gBusyMenu = NULL;

typedef LLMemberListener<LLView> view_listener_t;

//
// Local prototypes
//
void handle_leave_group(void *);

// File Menu
const char* upload_pick(void* data);
void handle_upload(void* data);
void handle_upload_object(void* data);
void handle_compress_image(void*);
BOOL enable_save_as(void *);

// Edit menu
void handle_dump_group_info(void *);
void handle_dump_capabilities_info(void *);
void handle_dump_focus(void*);

void handle_region_dump_settings(void*);
void handle_region_dump_temp_asset_data(void*);
void handle_region_clear_temp_asset_data(void*);

// Object pie menu
BOOL sitting_on_selection();

void near_sit_object();
void label_sit_or_stand(LLString& label, void*);
// buy and take alias into the same UI positions, so these
// declarations handle this mess.
BOOL is_selection_buy_not_take();
S32 selection_price();
BOOL enable_take();
void handle_take();
void confirm_take(S32 option, void* data);
BOOL enable_buy(void*); 
void handle_buy(void *);
void handle_buy_object(LLSaleInfo sale_info);
void handle_buy_contents(LLSaleInfo sale_info);
void label_touch(LLString& label, void*);

// Land pie menu
void near_sit_down_point(BOOL success, void *);

// Avatar pie menu
void handle_follow(void *userdata);
void handle_talk_to(void *userdata);

// Debug menu
void show_permissions_control(void*);
void load_url_local_file(const char* file_name);
void toggle_build_options(void* user_data);
#if 0 // Unused
void handle_audio_status_1(void*);
void handle_audio_status_2(void*);
void handle_audio_status_3(void*);
void handle_audio_status_4(void*);
#endif
void reload_ui(void*);
void handle_agent_stop_moving(void*);
void print_packets_lost(void*);
void drop_packet(void*);
void velocity_interpolate( void* data );
void update_fov(S32 increments);
void toggle_wind_audio(void);
void toggle_water_audio(void);
void handle_rebake_textures(void*);
BOOL check_admin_override(void*);
void handle_admin_override_toggle(void*);
#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
void handle_toggle_hacked_godmode(void*);
BOOL check_toggle_hacked_godmode(void*);
#endif

void toggle_glow(void *);
BOOL check_glow(void *);

void toggle_vertex_shaders(void *);
BOOL check_vertex_shaders(void *);

void toggle_cull_small(void *);

void toggle_show_xui_names(void *);
BOOL check_show_xui_names(void *);

// Debug UI
void handle_save_to_xml(void*);
void handle_load_from_xml(void*);

void handle_god_mode(void*);

// God menu
void handle_leave_god_mode(void*);

BOOL is_inventory_visible( void* user_data );
void handle_reset_view();

void disabled_duplicate(void*);
void handle_duplicate_in_place(void*);
void handle_repeat_duplicate(void*);

void handle_export(void*);
void handle_deed_object_to_group(void*);
BOOL enable_deed_object_to_group(void*);
void handle_object_owner_self(void*);
void handle_object_owner_permissive(void*);
void handle_object_lock(void*);
void handle_object_asset_ids(void*);
void force_take_copy(void*);
#ifdef _CORY_TESTING
void force_export_copy(void*);
void force_import_geometry(void*);
#endif

void handle_force_parcel_owner_to_me(void*);
void handle_force_parcel_to_content(void*);
void handle_claim_public_land(void*);

void handle_god_request_havok(void *);
void handle_god_request_avatar_geometry(void *);	// Hack for easy testing of new avatar geometry
void reload_personal_settings_overrides(void *);
void force_breakpoint(void *);
void reload_vertex_shader(void *);
void slow_mo_animations(void *);
void handle_disconnect_viewer(void *);

void handle_stopall(void*);
void handle_hinge(void*);
void handle_ptop(void*);
void handle_lptop(void*);
void handle_wheel(void*);
void handle_dehinge(void*);
BOOL enable_dehinge(void*);
void handle_force_delete(void*);
void print_object_info(void*);
void show_debug_menus();
void toggle_debug_menus(void*);
void toggle_map( void* user_data );
void export_info_callback(LLAssetInfo *info, void **user_data, S32 result);
void export_data_callback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type, void **user_data, S32 result);
void upload_done_callback(const LLUUID& uuid, void* user_data, S32 result);
BOOL menu_check_build_tool( void* user_data );
void handle_reload_settings(void*);
void focus_here(void*);
void dump_select_mgr(void*);
void dump_volume_mgr(void*);
void dump_inventory(void*);
void edit_ui(void*);
void toggle_visibility(void*);
BOOL get_visibility(void*);

// Avatar Pie menu
void request_friendship(const LLUUID& agent_id);

// Tools menu
void handle_force_unlock(void*);
void handle_selected_texture_info(void*);
void handle_dump_image_list(void*);

void handle_fullscreen_debug(void*);
void handle_crash(void*);
void handle_dump_followcam(void*);
void handle_toggle_flycam(void*);
BOOL check_flycam(void*);
void handle_viewer_enable_message_log(void*);
void handle_viewer_disable_message_log(void*);
void handle_send_postcard(void*);
void handle_gestures_old(void*);
void handle_focus(void *);
BOOL enable_buy_land(void*);
void handle_move(void*);
void handle_show_inventory(void*);
void handle_activate(void*);
BOOL enable_activate(void*);

// Help menu
void handle_buy_currency(void*);

void handle_test_male(void *);
void handle_test_female(void *);
void handle_toggle_pg(void*);
void handle_dump_attachments(void *);
void handle_show_overlay_title(void*);
void handle_dump_avatar_local_textures(void*);
void handle_debug_avatar_textures(void*);
void handle_grab_texture(void*);
BOOL enable_grab_texture(void*);
void handle_dump_region_object_cache(void*);

BOOL menu_ui_enabled(void *user_data);
void check_toggle_control( LLUICtrl *, void* user_data );
BOOL menu_check_control( void* user_data);
void menu_toggle_variable( void* user_data );
BOOL menu_check_variable( void* user_data);
BOOL enable_land_selected( void* );
BOOL enable_more_than_one_selected(void* );
BOOL enable_selection_you_own_all(void*);
BOOL enable_selection_you_own_one(void*);
BOOL enable_save_into_inventory(void*);
BOOL enable_save_into_task_inventory(void*);
BOOL enable_not_thirdperson(void*);
BOOL enable_export_selected(void *);
BOOL enable_have_card(void*);
BOOL enable_detach(void*);
BOOL enable_region_owner(void*);


class LLMenuParcelObserver : public LLParcelObserver
{
public:
	LLMenuParcelObserver();
	~LLMenuParcelObserver();
	virtual void changed();
};

static LLMenuParcelObserver* gMenuParcelObserver = NULL;

LLMenuParcelObserver::LLMenuParcelObserver()
{
	gParcelMgr->addObserver(this);
}

LLMenuParcelObserver::~LLMenuParcelObserver()
{
	gParcelMgr->removeObserver(this);
}

void LLMenuParcelObserver::changed()
{
	gMenuHolder->childSetEnabled("Land Buy Pass", LLPanelLandGeneral::enableBuyPass(NULL));
	
	BOOL buyable = enable_buy_land(NULL);
	gMenuHolder->childSetEnabled("Land Buy", buyable);
	gMenuHolder->childSetEnabled("Buy Land...", buyable);
}


//-----------------------------------------------------------------------------
// Menu Construction
//-----------------------------------------------------------------------------

// code required to calculate anything about the menus
void pre_init_menus()
{
	// static information
	LLColor4 color;
	color = gColors.getColor( "MenuDefaultBgColor" );
	LLMenuGL::setDefaultBackgroundColor( color );
	color = gColors.getColor( "MenuItemEnabledColor" );
	LLMenuItemGL::setEnabledColor( color );
	color = gColors.getColor( "MenuItemDisabledColor" );
	LLMenuItemGL::setDisabledColor( color );
	color = gColors.getColor( "MenuItemHighlightBgColor" );
	LLMenuItemGL::setHighlightBGColor( color );
	color = gColors.getColor( "MenuItemHighlightFgColor" );
	LLMenuItemGL::setHighlightFGColor( color );
}

void initialize_menus();

//-----------------------------------------------------------------------------
// Initialize main menus
//
// HOW TO NAME MENUS:
//
// First Letter Of Each Word Is Capitalized, Even At Or And
//
// Items that lead to dialog boxes end in "..."
//
// Break up groups of more than 6 items with separators
//-----------------------------------------------------------------------------
void init_menus()
{
	S32 top = gViewerWindow->getRootView()->getRect().getHeight();
	S32 width = gViewerWindow->getRootView()->getRect().getWidth();

	//
	// Main menu bar
	//
	gMenuHolder = new LLViewerMenuHolderGL();
	gMenuHolder->setRect(LLRect(0, top, width, 0));
	gMenuHolder->setFollowsAll();

	LLMenuGL::sMenuContainer = gMenuHolder;

	// Initialize actions
	initialize_menus();

	///
	/// Popup menu
	///
	/// The popup menu is now populated by the show_context_menu()
	/// method.
	
	gPopupMenuView = new LLMenuGL( "Popup" );
	gPopupMenuView->setVisible( FALSE );
	gMenuHolder->addChild( gPopupMenuView );

	///
	/// Pie menus
	///
	gPieSelf = gUICtrlFactory->buildPieMenu("menu_pie_self.xml", gMenuHolder);

	// TomY TODO: what shall we do about these?
	gDetachScreenPieMenu = (LLPieMenu*)gMenuHolder->getChildByName("Object Detach HUD", true);
	gDetachPieMenu = (LLPieMenu*)gMenuHolder->getChildByName("Object Detach", true);

	if (gAgent.mAccess < SIM_ACCESS_MATURE)
	{
		gMenuHolder->getChildByName("Self Underpants", TRUE)->setVisible(FALSE);
		gMenuHolder->getChildByName("Self Undershirt", TRUE)->setVisible(FALSE);
	}

	gPieAvatar = gUICtrlFactory->buildPieMenu("menu_pie_avatar.xml", gMenuHolder);

	gPieObject = gUICtrlFactory->buildPieMenu("menu_pie_object.xml", gMenuHolder);

	gAttachScreenPieMenu = (LLPieMenu*)gMenuHolder->getChildByName("Object Attach HUD", true);
	gAttachPieMenu = (LLPieMenu*)gMenuHolder->getChildByName("Object Attach", true);
	gPieRate = (LLPieMenu*)gMenuHolder->getChildByName("Rate Menu", true);

	gPieAttachment = gUICtrlFactory->buildPieMenu("menu_pie_attachment.xml", gMenuHolder);

	gPieLand = gUICtrlFactory->buildPieMenu("menu_pie_land.xml", gMenuHolder);

	///
	/// set up the colors
	///
	LLColor4 color;

	LLColor4 pie_color = gColors.getColor("PieMenuBgColor");
	gPieSelf->setBackgroundColor( pie_color );
	gPieAvatar->setBackgroundColor( pie_color );
	gPieObject->setBackgroundColor( pie_color );
	gPieAttachment->setBackgroundColor( pie_color );
	gPieLand->setBackgroundColor( pie_color );

	color = gColors.getColor( "MenuPopupBgColor" );
	gPopupMenuView->setBackgroundColor( color );

	// If we are not in production, use a different color to make it apparent.
	if (gInProductionGrid)
	{
		color = gColors.getColor( "MenuBarBgColor" );
	}
	else
	{
		color = gColors.getColor( "MenuNonProductionBgColor" );
	}
	gMenuBarView = (LLMenuBarGL*)gUICtrlFactory->buildMenu("menu_viewer.xml", gMenuHolder);
	gMenuBarView->setRect(LLRect(0, top, 0, top - MENU_BAR_HEIGHT));
	gMenuBarView->setBackgroundColor( color );

	gMenuHolder->addChild(gMenuBarView);
	
	// menu holder appears on top of menu bar so you can see the menu title
	// flash when an item is triggered (the flash occurs in the holder)
	gViewerWindow->getRootView()->addChild(gMenuHolder);

	gMenuHolder->childSetLabelArg("Upload Image", "[COST]", "10");
	gMenuHolder->childSetLabelArg("Upload Sound", "[COST]", "10");
	gMenuHolder->childSetLabelArg("Upload Animation", "[COST]", "10");
	gMenuHolder->childSetLabelArg("Bulk Upload", "[COST]", "10");

	gAFKMenu = (LLMenuItemCallGL*)gMenuBarView->getChildByName("Set Away", TRUE);
	gBusyMenu = (LLMenuItemCallGL*)gMenuBarView->getChildByName("Set Busy", TRUE);
	gAttachSubMenu = gMenuBarView->getChildMenuByName("Attach Object", TRUE);
	gDetachSubMenu = gMenuBarView->getChildMenuByName("Detach Object", TRUE);

	if (gAgent.mAccess < SIM_ACCESS_MATURE)
	{
		gMenuBarView->getChildByName("Menu Underpants", TRUE)->setVisible(FALSE);
		gMenuBarView->getChildByName("Menu Undershirt", TRUE)->setVisible(FALSE);
	}

	// TomY TODO convert these two
	LLMenuGL*menu;
	menu = new LLMenuGL(CLIENT_MENU_NAME);
	init_client_menu(menu);
	gMenuBarView->appendMenu( menu );
	menu->updateParent(LLMenuGL::sMenuContainer);

	menu = new LLMenuGL(SERVER_MENU_NAME);
	init_server_menu(menu);
	gMenuBarView->appendMenu( menu );
	menu->updateParent(LLMenuGL::sMenuContainer);

	gMenuBarView->createJumpKeys();

	// Let land based option enable when parcel changes
	gMenuParcelObserver = new LLMenuParcelObserver();

	//
	// Debug menu visiblity
	//
	show_debug_menus();
}

void init_client_menu(LLMenuGL* menu)
{
	LLMenuGL* sub_menu = NULL;

	//menu->append(new LLMenuItemCallGL("Permissions Control", &show_permissions_control));

// this is now in the view menu so we don't need it here!
	{
		LLMenuGL* sub = new LLMenuGL("Consoles");
		menu->appendMenu(sub);
		sub->append(new LLMenuItemCheckGL("Frame Console", 
										&toggle_visibility,
										NULL,
										&get_visibility,
										(void*)gDebugView->mFrameStatView,
										'2', MASK_CONTROL|MASK_SHIFT ) );
		sub->append(new LLMenuItemCheckGL("Texture Console", 
										&toggle_visibility,
										NULL,
										&get_visibility,
										(void*)gTextureView,
									   	'3', MASK_CONTROL|MASK_SHIFT ) );
		LLView* debugview = gDebugView->mDebugConsolep;
		sub->append(new LLMenuItemCheckGL("Debug Console", 
										&toggle_visibility,
										NULL,
										&get_visibility,
										debugview,
									   	'4', MASK_CONTROL|MASK_SHIFT ) );
#if 0 // Unused
	{
		LLMenuGL* sub = new LLMenuGL("Audio");
		menu->appendMenu(sub);

		sub->append(new LLMenuItemCallGL("Global Pos", 
					&handle_audio_status_1, NULL, NULL ,'5', MASK_CONTROL|MASK_SHIFT) );
		sub->append(new LLMenuItemCallGL("Cone", 
					&handle_audio_status_2, NULL, NULL ,'6', MASK_CONTROL|MASK_SHIFT) );
		sub->append(new LLMenuItemCallGL("Local Pos", 
					&handle_audio_status_3, NULL, NULL ,'7', MASK_CONTROL|MASK_SHIFT) );
		sub->append(new LLMenuItemCallGL("Duration", 
					&handle_audio_status_4, NULL, NULL ,'8', MASK_CONTROL|MASK_SHIFT) );
		sub->createJumpKeys();
	}
#endif
		sub->append(new LLMenuItemCheckGL("Fast Timers", 
										&toggle_visibility,
										NULL,
										&get_visibility,
										(void*)gDebugView->mFastTimerView,
										'9', MASK_CONTROL|MASK_SHIFT ) );
		sub->append(new LLMenuItemCheckGL("Memory", 
										&toggle_visibility,
										NULL,
										&get_visibility,
										(void*)gDebugView->mMemoryView,
										'0', MASK_CONTROL|MASK_SHIFT ) );
		sub->appendSeparator();
		sub->append(new LLMenuItemCallGL("Region Info to Debug Console", 
			&handle_region_dump_settings, NULL));
		sub->append(new LLMenuItemCallGL("Group Info to Debug Console",
			&handle_dump_group_info, NULL, NULL));
		sub->append(new LLMenuItemCallGL("Capabilities Info to Debug Console",
			&handle_dump_capabilities_info, NULL, NULL));
		sub->createJumpKeys();
	}
	
	// neither of these works particularly well at the moment
	/*menu->append(new LLMenuItemCallGL(  "Reload UI XML",	&reload_ui,	
	  				NULL, NULL, 'R', MASK_ALT | MASK_CONTROL ) );*/
	/*menu->append(new LLMenuItemCallGL("Reload settings/colors", 
					&handle_reload_settings, NULL, NULL));*/
	menu->append(new LLMenuItemCallGL("Reload personal setting overrides", 
		&reload_personal_settings_overrides, NULL, NULL, KEY_F2, MASK_CONTROL|MASK_SHIFT));

	sub_menu = new LLMenuGL("HUD Info");
	{
		sub_menu->append(new LLMenuItemCheckGL("Velocity", 
												&toggle_visibility,
												NULL,
												&get_visibility,
												(void*)gVelocityBar));

		sub_menu->append(new LLMenuItemToggleGL("Camera",	&gDisplayCameraPos ) );
		sub_menu->append(new LLMenuItemToggleGL("Wind", 	&gDisplayWindInfo) );
		sub_menu->append(new LLMenuItemToggleGL("FOV",  	&gDisplayFOV ) );
		sub_menu->createJumpKeys();
	}
	menu->appendMenu(sub_menu);

	menu->appendSeparator();

	menu->append(new LLMenuItemCheckGL( "High-res Snapshot",
										&menu_toggle_control,
										NULL,
										&menu_check_control,
										(void*)"HighResSnapshot"));

	menu->append(new LLMenuItemToggleGL("Quiet Snapshots to Disk",
										&gQuietSnapshot));

	menu->append(new LLMenuItemCheckGL( "Compress Snapshots to Disk",
										&menu_toggle_control,
										NULL,
										&menu_check_control,
										(void*)"CompressSnapshotsToDisk"));

	menu->append(new LLMenuItemCheckGL("Show Mouselook Crosshairs",
									   &menu_toggle_control,
									   NULL,
									   &menu_check_control,
									   (void*)"ShowCrosshairs"));

	menu->append(new LLMenuItemCheckGL("Debug Permissions",
									   &menu_toggle_control,
									   NULL,
									   &menu_check_control,
									   (void*)"DebugPermissions"));
	


#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
	if (!gInProductionGrid)
	{
		menu->append(new LLMenuItemCheckGL("Hacked Godmode",
										   &handle_toggle_hacked_godmode,
										   NULL,
										   &check_toggle_hacked_godmode,
										   (void*)"HackedGodmode"));
	}
#endif

	menu->append(new LLMenuItemCallGL("Clear Group Cache", 
									  LLGroupMgr::debugClearAllGroups));
	menu->appendSeparator();

	sub_menu = new LLMenuGL("Rendering");
	init_debug_rendering_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("World");
	init_debug_world_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("UI");
	init_debug_ui_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("XUI");
	init_debug_xui_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("Character");
	init_debug_avatar_menu(sub_menu);
	menu->appendMenu(sub_menu);

{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Network");

		sub->append(new LLMenuItemCallGL("Enable Message Log",  
			&handle_viewer_enable_message_log,  NULL));
		sub->append(new LLMenuItemCallGL("Disable Message Log", 
			&handle_viewer_disable_message_log, NULL));

		sub->appendSeparator();

		sub->append(new LLMenuItemCheckGL("Velocity Interpolate Objects", 
										  &velocity_interpolate,
										  NULL, 
										  &menu_check_control,
										  (void*)"VelocityInterpolate"));
		sub->append(new LLMenuItemCheckGL("Ping Interpolate Object Positions", 
										  &menu_toggle_control,
										  NULL, 
										  &menu_check_control,
										  (void*)"PingInterpolate"));

		sub->appendSeparator();

		sub->append(new LLMenuItemCallGL("Drop a Packet", 
			&drop_packet, NULL, NULL, 
			'L', MASK_ALT | MASK_CONTROL));

		menu->appendMenu( sub );
		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Recorder");

		sub->append(new LLMenuItemCheckGL("Full Session Logging", &menu_toggle_control, NULL, &menu_check_control, (void*)"StatsSessionTrackFrameStats"));

		sub->append(new LLMenuItemCallGL("Start Logging",	&LLFrameStats::startLogging, NULL));
		sub->append(new LLMenuItemCallGL("Stop Logging",	&LLFrameStats::stopLogging, NULL));
		sub->append(new LLMenuItemCallGL("Log 10 Seconds", &LLFrameStats::timedLogging10, NULL));
		sub->append(new LLMenuItemCallGL("Log 30 Seconds", &LLFrameStats::timedLogging30, NULL));
		sub->append(new LLMenuItemCallGL("Log 60 Seconds", &LLFrameStats::timedLogging60, NULL));
		sub->appendSeparator();
		sub->append(new LLMenuItemCallGL("Start Playback", &LLAgentPilot::startPlayback, NULL));
		sub->append(new LLMenuItemCallGL("Stop Playback",	&LLAgentPilot::stopPlayback, NULL));
		sub->append(new LLMenuItemToggleGL("Loop Playback", &LLAgentPilot::sLoop) );
		sub->append(new LLMenuItemCallGL("Start Record",	&LLAgentPilot::startRecord, NULL));
		sub->append(new LLMenuItemCallGL("Stop Record",	&LLAgentPilot::saveRecord, NULL));

		menu->appendMenu( sub );
		sub->createJumpKeys();
	}

	menu->appendSeparator();

	menu->append(new LLMenuItemToggleGL("Show Updates", 
		&gShowObjectUpdates,
		'U', MASK_ALT | MASK_SHIFT | MASK_CONTROL));
	
	menu->appendSeparator(); 
	
	menu->append(new LLMenuItemCallGL("Compress Image...", 
		&handle_compress_image, NULL, NULL));

	menu->append(new LLMenuItemCheckGL("Limit Select Distance", 
									   &menu_toggle_control,
									   NULL, 
									   &menu_check_control,
									   (void*)"LimitSelectDistance"));

	menu->append(new LLMenuItemToggleGL("Disable Camera Constraints", 
		&LLViewerCamera::sDisableCameraConstraints));

	menu->append(new LLMenuItemCheckGL("Joystick Flycam", 
		&handle_toggle_flycam,NULL,&check_flycam,NULL));
		
	menu->append(new LLMenuItemCheckGL("Mouse Smoothing",
										&menu_toggle_control,
										NULL,
										&menu_check_control,
										(void*) "MouseSmooth"));
	menu->appendSeparator();

	menu->append(new LLMenuItemCheckGL( "Console Window", 
										&menu_toggle_control,
										NULL, 
										&menu_check_control,
										(void*)"ShowConsoleWindow"));

#ifndef LL_RELEASE_FOR_DOWNLOAD
	{
		LLMenuGL* sub = NULL;
		sub = new LLMenuGL("Debugging");
		sub->append(new LLMenuItemCallGL("Force Breakpoint", &force_breakpoint, NULL, NULL, 'B', MASK_CONTROL | MASK_ALT));
		sub->append(new LLMenuItemCallGL("LLError And Crash", &handle_crash));
		sub->createJumpKeys();
		menu->appendMenu(sub);
	}
#endif	

	// TomY Temporary menu item so we can test this floater
	menu->append(new LLMenuItemCheckGL("Clothing...", 
												&handle_clothing,
												NULL,
												NULL,
												NULL));

	menu->append(new LLMenuItemCallGL("Debug Settings", LLFloaterSettingsDebug::show, NULL, NULL));
	menu->append(new LLMenuItemCheckGL("View Admin Options", &handle_admin_override_toggle, NULL, &check_admin_override, NULL, 'V', MASK_CONTROL | MASK_ALT));
	menu->createJumpKeys();
}

void init_debug_world_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCheckGL("Mouse Moves Sun", 
									   &menu_toggle_control,
									   NULL, 
									   &menu_check_control,
									   (void*)"MouseSun", 
									   'M', MASK_CONTROL|MASK_ALT));
	menu->append(new LLMenuItemCheckGL("Sim Sun Override", 
									   &menu_toggle_control,
									   NULL, 
									   &menu_check_control,
									   (void*)"SkyOverrideSimSunPosition"));
	menu->append(new LLMenuItemCallGL("Dump Scripted Camera",
		&handle_dump_followcam, NULL, NULL));
	menu->append(new LLMenuItemCheckGL("Fixed Weather", 
									   &menu_toggle_control,
									   NULL, 
									   &menu_check_control,
									   (void*)"FixedWeather"));
	menu->append(new LLMenuItemCallGL("Dump Region Object Cache",
		&handle_dump_region_object_cache, NULL, NULL));
	menu->createJumpKeys();
}


void handle_export_menus_to_xml(void*)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if(!picker.getSaveFile(LLFilePicker::FFSAVE_XML))
	{
		llwarns << "No file" << llendl;
		return;
	}
 	const char* filename = picker.getFirstFile();

	llofstream out(filename);
	LLXMLNodePtr node = gMenuBarView->getXML();
	node->writeToOstream(out);
	out.close();
}

extern BOOL gDebugClicks;
extern BOOL gDebugWindowProc;
extern BOOL gDebugTextEditorTips;
extern BOOL gDebugSelectMgr;

void init_debug_ui_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCallGL("Editable UI", &edit_ui));
	menu->append(new LLMenuItemToggleGL("Async Keystrokes", &gHandleKeysAsync));
	menu->append(new LLMenuItemCallGL( "Dump SelectMgr", &dump_select_mgr));
	menu->append(new LLMenuItemCallGL( "Dump Inventory", &dump_inventory));
	menu->append(new LLMenuItemCallGL( "Dump Focus Holder", &handle_dump_focus, NULL, NULL, 'F', MASK_ALT | MASK_CONTROL));
	menu->append(new LLMenuItemCallGL( "Dump VolumeMgr",	&dump_volume_mgr, NULL, NULL));
	menu->append(new LLMenuItemCallGL( "Print Selected Object Info",	&print_object_info, NULL, NULL, 'P', MASK_CONTROL|MASK_SHIFT ));
	menu->append(new LLMenuItemCallGL( "Print Agent Info",			&print_agent_nvpairs, NULL, NULL, 'P', MASK_SHIFT ));
	menu->append(new LLMenuItemCallGL( "Print Texture Memory Stats",  &output_statistics, NULL, NULL, 'M', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
	menu->append(new LLMenuItemCheckGL("Double-Click Auto-Pilot", 
		menu_toggle_control, NULL, menu_check_control, 
		(void*)"DoubleClickAutoPilot"));
	menu->appendSeparator();
//	menu->append(new LLMenuItemCallGL( "Print Packets Lost",			&print_packets_lost, NULL, NULL, 'L', MASK_SHIFT ));
	menu->append(new LLMenuItemToggleGL("Debug SelectMgr", &gDebugSelectMgr));
	menu->append(new LLMenuItemToggleGL("Debug Clicks", &gDebugClicks));
	menu->append(new LLMenuItemToggleGL("Debug Views", &LLView::sDebugRects));
	menu->append(new LLMenuItemCheckGL("Show Name Tooltips", toggle_show_xui_names, NULL, check_show_xui_names, NULL));
	menu->append(new LLMenuItemToggleGL("Debug Mouse Events", &LLView::sDebugMouseHandling));
	menu->append(new LLMenuItemToggleGL("Debug Keys", &LLView::sDebugKeys));
	menu->append(new LLMenuItemToggleGL("Debug WindowProc", &gDebugWindowProc));
	menu->append(new LLMenuItemToggleGL("Debug Text Editor Tips", &gDebugTextEditorTips));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Show Time", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowTime"));
	menu->append(new LLMenuItemCheckGL("Show Render Info", menu_toggle_control, NULL, menu_check_control, (void*)"DebugShowRenderInfo"));
	
	menu->createJumpKeys();
}

void init_debug_xui_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCallGL("Floater Test...", LLFloaterTest::show));
	menu->append(new LLMenuItemCallGL("Export Menus to XML...", handle_export_menus_to_xml));
	menu->append(new LLMenuItemCallGL("Edit UI...", LLFloaterEditUI::show));	
	menu->append(new LLMenuItemCallGL("Load from XML...", handle_load_from_xml));
	menu->append(new LLMenuItemCallGL("Save to XML...", handle_save_to_xml));
	menu->append(new LLMenuItemCheckGL("Show XUI Names", toggle_show_xui_names, NULL, check_show_xui_names, NULL));

	//menu->append(new LLMenuItemCallGL("Buy Currency...", handle_buy_currency));
	menu->createJumpKeys();
}

void init_debug_rendering_menu(LLMenuGL* menu)
{
	LLMenuGL* sub_menu = NULL;

	///////////////////////////
	//
	// Debug menu for types/pools
	//
	sub_menu = new LLMenuGL("Types");
	menu->appendMenu(sub_menu);

	sub_menu->append(new LLMenuItemCheckGL("Simple",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_SIMPLE,	'1', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Alpha",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_ALPHA, '2', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Tree",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_TREE, '3', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Character",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_AVATAR, '4', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("SurfacePatch",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_TERRAIN, '5', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Sky",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_SKY, '6', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Water",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_WATER, '7', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Ground",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_GROUND, '8', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Volume",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_VOLUME, '9', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Grass",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_GRASS, '0', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Clouds",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_CLOUDS, '-', MASK_CONTROL|MASK_ALT| MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Particles",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_PARTICLES, '=', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->append(new LLMenuItemCheckGL("Bump",
											&LLPipeline::toggleRenderTypeControl, NULL,
											&LLPipeline::hasRenderTypeControl,
											(void*)LLPipeline::RENDER_TYPE_BUMP, '\\', MASK_CONTROL|MASK_ALT|MASK_SHIFT));
	sub_menu->createJumpKeys();
	sub_menu = new LLMenuGL("Features");
	menu->appendMenu(sub_menu);
	sub_menu->append(new LLMenuItemCheckGL("UI",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_UI, '1', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Selected",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_SELECTED, '2', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Highlighted",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_HIGHLIGHTED, '3', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Dynamic Textures",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_DYNAMIC_TEXTURES, '4', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL( "Foot Shadows", 
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FOOT_SHADOWS, '5', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Fog",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FOG, '6', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Palletized Textures",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_PALETTE, '7', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL("Test FRInfo",
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FR_INFO, '8', MASK_ALT|MASK_CONTROL));
	sub_menu->append(new LLMenuItemCheckGL( "Flexible Objects", 
											&LLPipeline::toggleRenderDebugFeature, NULL,
											&LLPipeline::toggleRenderDebugFeatureControl,
											(void*)LLPipeline::RENDER_DEBUG_FEATURE_FLEXIBLE, '9', MASK_ALT|MASK_CONTROL));
	sub_menu->createJumpKeys();

	/////////////////////////////
	//
	// Debug menu for info displays
	//
	sub_menu = new LLMenuGL("Info Displays");
	menu->appendMenu(sub_menu);

	sub_menu->append(new LLMenuItemCheckGL("Verify",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_VERIFY));
	sub_menu->append(new LLMenuItemCheckGL("BBoxes",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_BBOXES));
	sub_menu->append(new LLMenuItemCheckGL("Points",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_POINTS));
	sub_menu->append(new LLMenuItemCheckGL("Octree",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_OCTREE));
	sub_menu->append(new LLMenuItemCheckGL("Occlusion",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_OCCLUSION));
	sub_menu->append(new LLMenuItemCheckGL("Animated Textures",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_TEXTURE_ANIM));
	sub_menu->append(new LLMenuItemCheckGL("Texture Priority",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_TEXTURE_PRIORITY));
	sub_menu->append(new LLMenuItemCheckGL("Texture Area (sqrt(A))",&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_TEXTURE_AREA));
	sub_menu->append(new LLMenuItemCheckGL("Face Area (sqrt(A))",&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_FACE_AREA));
	sub_menu->append(new LLMenuItemCheckGL("Pick Render",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_PICKING));
	sub_menu->append(new LLMenuItemCheckGL("Particles",	&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_PARTICLES));
	sub_menu->append(new LLMenuItemCheckGL("Composition", &LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_COMPOSITION));
	sub_menu->append(new LLMenuItemCheckGL("ShadowMap", &LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_SHADOW_MAP));
	sub_menu->append(new LLMenuItemCheckGL("LightTrace",&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_LIGHT_TRACE));
	sub_menu->append(new LLMenuItemCheckGL("Glow",&LLPipeline::toggleRenderDebug, NULL,
													&LLPipeline::toggleRenderDebugControl,
													(void*)LLPipeline::RENDER_DEBUG_GLOW));
	
	sub_menu->append(new LLMenuItemCheckGL("Show Depth Buffer",
										   &menu_toggle_control,
										   NULL,
										   &menu_check_control,
										   (void*)"ShowDepthBuffer"));
	sub_menu->append(new LLMenuItemToggleGL("Show Select Buffer", &gDebugSelect));

	sub_menu = new LLMenuGL("Render Tests");

	sub_menu->append(new LLMenuItemCheckGL("Camera Offset", 
										  &menu_toggle_control,
										  NULL, 
										  &menu_check_control,
										  (void*)"CameraOffset"));

	sub_menu->append(new LLMenuItemToggleGL("Randomize Framerate", &gRandomizeFramerate));

	sub_menu->append(new LLMenuItemToggleGL("Periodic Slow Frame", &gPeriodicSlowFrame));
	sub_menu->createJumpKeys();

	menu->appendMenu( sub_menu );

	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Axes", menu_toggle_control, NULL, menu_check_control, (void*)"ShowAxes"));
//	menu->append(new LLMenuItemCheckGL("Cull Small Objects", toggle_cull_small, NULL, menu_check_control, (void*)"RenderCullBySize"));

	menu->appendSeparator();
	menu->append(new LLMenuItemToggleGL("Hide Selected", &gHideSelectedObjects));
	menu->appendSeparator();
	menu->append(new LLMenuItemCheckGL("Tangent Basis", menu_toggle_control, NULL, menu_check_control, (void*)"ShowTangentBasis"));
	menu->append(new LLMenuItemCallGL("Selected Texture Info", handle_selected_texture_info, NULL, NULL, 'T', MASK_CONTROL|MASK_SHIFT|MASK_ALT));
	//menu->append(new LLMenuItemCallGL("Dump Image List", handle_dump_image_list, NULL, NULL, 'I', MASK_CONTROL|MASK_SHIFT));
	
	menu->append(new LLMenuItemToggleGL("Wireframe", &gUseWireframe, 
			'R', MASK_CONTROL|MASK_SHIFT));

	LLMenuItemCheckGL* item;
	item = new LLMenuItemCheckGL("Object-Object Occlusion", menu_toggle_control, NULL, menu_check_control, (void*)"UseOcclusion", 'O', MASK_CONTROL|MASK_SHIFT);
	item->setEnabled(gGLManager.mHasOcclusionQuery);
	menu->append(item);
	
	
	item = new LLMenuItemCheckGL("Animate Textures", menu_toggle_control, NULL, menu_check_control, (void*)"AnimateTextures");
	menu->append(item);
	
	item = new LLMenuItemCheckGL("Disable Textures", menu_toggle_variable, NULL, menu_check_variable, (void*)&LLViewerImage::sDontLoadVolumeTextures);
	menu->append(item);
	
#ifndef LL_RELEASE_FOR_DOWNLOAD
	item = new LLMenuItemCheckGL("HTTP Get Textures", menu_toggle_control, NULL, menu_check_control, (void*)"ImagePipelineUseHTTP");
	menu->append(item);
#endif
	
	item = new LLMenuItemCheckGL("Run Multiple Threads", menu_toggle_control, NULL, menu_check_control, (void*)"RunMultipleThreads");
	menu->append(item);

#ifndef LL_RELEASE_FOR_DOWNLOAD
	item = new LLMenuItemCheckGL("Dynamic Reflections", menu_toggle_control, NULL, menu_check_control, (void*)"RenderDynamicReflections");
	menu->append(item);
#endif
	
	item = new LLMenuItemCheckGL("Cheesy Beacon", menu_toggle_control, NULL, menu_check_control, (void*)"CheesyBeacon");
	menu->append(item);
	
	menu->createJumpKeys();
}

extern BOOL gDebugAvatarRotation;

void init_debug_avatar_menu(LLMenuGL* menu)
{
	LLMenuGL* sub_menu = new LLMenuGL("Grab Baked Texture");
	init_debug_baked_texture_menu(sub_menu);
	menu->appendMenu(sub_menu);

	sub_menu = new LLMenuGL("Character Tests");
	sub_menu->append(new LLMenuItemToggleGL("Go Away/AFK When Idle",
		&gAllowAFK));

	sub_menu->append(new LLMenuItemCallGL("Appearance To XML", 
		&LLVOAvatar::dumpArchetypeXML));

	// HACK for easy testing of avatar geometry
	sub_menu->append(new LLMenuItemCallGL( "Toggle Character Geometry", 
		&handle_god_request_avatar_geometry, &enable_god_customer_service, NULL));

	sub_menu->append(new LLMenuItemCallGL("Test Male", 
		handle_test_male));

	sub_menu->append(new LLMenuItemCallGL("Test Female", 
		handle_test_female));

	sub_menu->append(new LLMenuItemCallGL("Toggle PG", handle_toggle_pg));

	sub_menu->append(new LLMenuItemToggleGL("Allow Select Avatar", &gAllowSelectAvatar));
	sub_menu->createJumpKeys();

	menu->appendMenu(sub_menu);

	menu->append(new LLMenuItemCallGL("Force Params to Default", &LLAgent::clearVisualParams, NULL));
	menu->append(new LLMenuItemCallGL("Reload Vertex Shader", &reload_vertex_shader, NULL));
	menu->append(new LLMenuItemToggleGL("Animation Info", &LLVOAvatar::sShowAnimationDebug));
	menu->append(new LLMenuItemCallGL("Slow Motion Animations", &slow_mo_animations, NULL));
	menu->append(new LLMenuItemToggleGL("Show Look At", &LLHUDEffectLookAt::sDebugLookAt));
	menu->append(new LLMenuItemToggleGL("Show Point At", &LLHUDEffectPointAt::sDebugPointAt));
	menu->append(new LLMenuItemToggleGL("Debug Joint Updates", &LLVOAvatar::sJointDebug));
	menu->append(new LLMenuItemToggleGL("Disable LOD", &LLViewerJoint::sDisableLOD));
	menu->append(new LLMenuItemToggleGL("Debug Character Vis", &LLVOAvatar::sDebugInvisible));
	//menu->append(new LLMenuItemToggleGL("Show Attachment Points", &LLVOAvatar::sShowAttachmentPoints));
	menu->append(new LLMenuItemToggleGL("Show Collision Plane", &LLVOAvatar::sShowFootPlane));
	menu->append(new LLMenuItemToggleGL("Show Collision Skeleton", &LLVOAvatar::sShowCollisionVolumes));
	menu->append(new LLMenuItemToggleGL( "Display Agent Target", &LLAgent::sDebugDisplayTarget));
	menu->append(new LLMenuItemToggleGL( "Debug Rotation", &gDebugAvatarRotation));
	menu->append(new LLMenuItemCallGL("Dump Attachments", handle_dump_attachments));
	menu->append(new LLMenuItemCallGL("Rebake Textures", handle_rebake_textures));
#ifndef LL_RELEASE_FOR_DOWNLOAD
	menu->append(new LLMenuItemCallGL("Debug Avatar Textures", handle_debug_avatar_textures, NULL, NULL, 'A', MASK_SHIFT|MASK_CONTROL|MASK_ALT));
	menu->append(new LLMenuItemCallGL("Dump Local Textures", handle_dump_avatar_local_textures, NULL, NULL, 'M', MASK_SHIFT|MASK_ALT ));	
#endif
	menu->createJumpKeys();
}

void init_debug_baked_texture_menu(LLMenuGL* menu)
{
	menu->append(new LLMenuItemCallGL("Iris", handle_grab_texture, enable_grab_texture, (void*) LLVOAvatar::TEX_EYES_BAKED));
	menu->append(new LLMenuItemCallGL("Head", handle_grab_texture, enable_grab_texture, (void*) LLVOAvatar::TEX_HEAD_BAKED));
	menu->append(new LLMenuItemCallGL("Upper Body", handle_grab_texture, enable_grab_texture, (void*) LLVOAvatar::TEX_UPPER_BAKED));
	menu->append(new LLMenuItemCallGL("Lower Body", handle_grab_texture, enable_grab_texture, (void*) LLVOAvatar::TEX_LOWER_BAKED));
	menu->append(new LLMenuItemCallGL("Skirt", handle_grab_texture, enable_grab_texture, (void*) LLVOAvatar::TEX_SKIRT_BAKED));
	menu->createJumpKeys();
}

void init_server_menu(LLMenuGL* menu)
{
	/*
	{
		// These messages are now trusted. We can write scripts to do
		// this, and the message is unchecked for source.
		LLMenuGL* sub_menu = NULL;
		sub_menu = new LLMenuGL("Sim Logging");

		sub_menu->append(new LLMenuItemCallGL("Turn off llinfos Log", 
			&handle_reduce_llinfo_log, &enable_god_customer_service));

		sub_menu->append(new LLMenuItemCallGL("Normal Logging", 
			&handle_normal_llinfo_log, &enable_god_customer_service));

		sub_menu->appendSeparator();
		sub_menu->append(new LLMenuItemCallGL("Enable Message Log",  
			&handle_sim_enable_message_log,  &enable_god_customer_service));
		sub_menu->append(new LLMenuItemCallGL("Disable Message Log", 
			&handle_sim_disable_message_log, &enable_god_customer_service));

		sub_menu->appendSeparator();

		sub_menu->append(new LLMenuItemCallGL("Fetch Message Log",	
			&handle_sim_fetch_message_log,  &enable_god_customer_service));

		sub_menu->append(new LLMenuItemCallGL("Fetch Log",			
			&handle_sim_fetch_log, &enable_god_customer_service));

		menu->appendMenu( sub_menu );
	}
	*/

	{
		LLMenuGL* sub = new LLMenuGL("Object");
		menu->appendMenu(sub);

		sub->append(new LLMenuItemCallGL( "Take Copy",
										  &force_take_copy, &enable_god_customer_service, NULL,
										  'O', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
#ifdef _CORY_TESTING
		sub->append(new LLMenuItemCallGL( "Export Copy",
										   &force_export_copy, NULL, NULL));
		sub->append(new LLMenuItemCallGL( "Import Geometry",
										   &force_import_geometry, NULL, NULL));
#endif
		//sub->append(new LLMenuItemCallGL( "Force Public", 
		//			&handle_object_owner_none, NULL, NULL));
		//sub->append(new LLMenuItemCallGL( "Force Ownership/Permissive", 
		//			&handle_object_owner_self_and_permissive, NULL, NULL, 'K', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->append(new LLMenuItemCallGL( "Force Owner To Me", 
					&handle_object_owner_self, &enable_god_customer_service));
		sub->append(new LLMenuItemCallGL( "Force Owner Permissive", 
					&handle_object_owner_permissive, &enable_god_customer_service));
		//sub->append(new LLMenuItemCallGL( "Force Totally Permissive", 
		//			&handle_object_permissive));
		sub->append(new LLMenuItemCallGL( "Delete", 
					&handle_force_delete, &enable_god_customer_service, NULL, KEY_DELETE, MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->append(new LLMenuItemCallGL( "Lock", 
					&handle_object_lock, &enable_god_customer_service, NULL, 'L', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->append(new LLMenuItemCallGL( "Get Asset IDs", 
					&handle_object_asset_ids, &enable_god_customer_service, NULL, 'I', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = new LLMenuGL("Parcel");
		menu->appendMenu(sub);

		sub->append(new LLMenuItemCallGL("Owner To Me",
										 &handle_force_parcel_owner_to_me,
										 &enable_god_customer_service, NULL));
		sub->append(new LLMenuItemCallGL("Set to Linden Content",
										 &handle_force_parcel_to_content,
										 &enable_god_customer_service, NULL,
										 'C', MASK_SHIFT | MASK_ALT | MASK_CONTROL));
		sub->appendSeparator();
		sub->append(new LLMenuItemCallGL("Claim Public Land",
										 &handle_claim_public_land, &enable_god_customer_service));

		sub->createJumpKeys();
	}
	{
		LLMenuGL* sub = new LLMenuGL("Region");
		menu->appendMenu(sub);
		sub->append(new LLMenuItemCallGL("Dump Temp Asset Data",
			&handle_region_dump_temp_asset_data,
			&enable_god_customer_service, NULL));
		sub->createJumpKeys();
	}	
	menu->append(new LLMenuItemCallGL( "God Tools...", 
		&LLFloaterGodTools::show, &enable_god_basic, NULL));

	menu->appendSeparator();

	menu->append(new LLMenuItemCallGL("Save Region State", 
		&LLPanelRegionTools::onSaveState, &enable_god_customer_service, NULL));

//	menu->append(new LLMenuItemCallGL("Force Join Group", handle_force_join_group));



	menu->appendSeparator();
//
//	menu->append(new LLMenuItemCallGL( "OverlayTitle",
//		&handle_show_overlay_title, &enable_god_customer_service, NULL));

	menu->append(new LLMenuItemCallGL("Request Admin Status", 
		&handle_god_mode, NULL, NULL, 'G', MASK_ALT | MASK_CONTROL));

	menu->append(new LLMenuItemCallGL("Leave Admin Status", 
		&handle_leave_god_mode, NULL, NULL, 'G', MASK_ALT | MASK_SHIFT | MASK_CONTROL));
	menu->createJumpKeys();
}

//-----------------------------------------------------------------------------
// cleanup_menus()
//-----------------------------------------------------------------------------
void cleanup_menus()
{
	delete gMenuParcelObserver;
	gMenuParcelObserver = NULL;
}

//-----------------------------------------------------------------------------
// Object pie menu
//-----------------------------------------------------------------------------

class LLObjectReportAbuse : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterReporter::showFromObject(gLastHitObjectID);
		return true;
	}
};

// Enabled it you clicked an object
class LLObjectEnableReportAbuse : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !gLastHitObjectID.isNull();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectTouch : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = gObjectList.findObject(gLastHitObjectID);
		if (!object) return true;

		LLMessageSystem	*msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_ObjectGrab);
		msg->nextBlockFast( _PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast( _PREHASH_ObjectData);
		msg->addU32Fast(    _PREHASH_LocalID, object->mLocalID);
		msg->addVector3Fast(_PREHASH_GrabOffset, LLVector3::zero );
		msg->sendMessage( object->getRegion()->getHost());

		// *NOTE: Hope the packets arrive safely and in order or else
		// there will be some problems.
		// *TODO: Just fix this bad assumption.
		msg->newMessageFast(_PREHASH_ObjectDeGrab);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addU32Fast(_PREHASH_LocalID, object->mLocalID);
		msg->sendMessage(object->getRegion()->getHost());

		return true;
	}
};


// One object must have touch sensor
class LLObjectEnableTouch : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* obj = gObjectList.findObject(gLastHitObjectID);
		bool new_value = obj && obj->flagHandleTouch();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		// Update label based on the node touch name if available.
		LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
		if (node && node->mValid && !node->mTouchName.empty())
		{
			gMenuHolder->childSetText("Object Touch", node->mTouchName);
		}
		else
		{
			gMenuHolder->childSetText("Object Touch", userdata["data"].asString());
		}

		return true;
	}
};

void label_touch(LLString& label, void*)
{
	LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
	if (node && node->mValid && !node->mTouchName.empty())
	{
		label.assign(node->mTouchName);
	}
	else
	{
		label.assign("Touch");
	}
}

bool handle_object_open()
{
	LLViewerObject* obj = gObjectList.findObject(gLastHitObjectID);
	if(!obj) return true;

	LLFloaterOpenObject::show();
	return true;
}

class LLObjectOpen : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_object_open();
	}
};

class LLObjectEnableOpen : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Look for contents in root object, which is all the LLFloaterOpenObject
		// understands.
		LLViewerObject* obj = gObjectList.findObject(gLastHitObjectID);
		bool new_value = (obj != NULL);
		if (new_value)
		{
			LLViewerObject* root = obj->getRootEdit();
			if (!root) new_value = false;
			else new_value = root->allowOpen();
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};


class LLViewCheckBuildMode : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gToolMgr->inEdit();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

bool toggle_build_mode()
{
	if (gToolMgr->inEdit())
	{
		// just reset the view, will pull us out of edit mode
		handle_reset_view();
	}
	else
	{
		if (gAgent.getFocusOnAvatar() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(gAgent.getPositionGlobal() + 2.0 * LLVector3d(gAgent.getAtAxis()));
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
		}

		gToolMgr->setCurrentToolset(gBasicToolset);
		gToolMgr->getCurrentToolset()->selectTool( gToolCreate );

		// Could be first use
		LLFirstUse::useBuild();
	}
	return true;
}

class LLViewBuildMode : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return toggle_build_mode();
	}
};


class LLObjectBuild : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getFocusOnAvatar() && !gToolMgr->inEdit() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
			gViewerWindow->moveCursorToCenter();
		}
		else if ( gSavedSettings.getBOOL("EditCameraMovement") )
		{
			gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
			gViewerWindow->moveCursorToCenter();
		}

		gToolMgr->setCurrentToolset(gBasicToolset);
		gToolMgr->getCurrentToolset()->selectTool( gToolCreate );

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLObjectEdit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gParcelMgr->deselectLand();

		if (gAgent.getFocusOnAvatar() && !gToolMgr->inEdit())
		{
			LLObjectSelectionHandle selection = gSelectMgr->getSelection();

			if (selection->getSelectType() == SELECT_TYPE_HUD || !gSavedSettings.getBOOL("EditCameraMovement"))
			{
				// always freeze camera in space, even if camera doesn't move
				// so, for example, follow cam scripts can't affect you when in build mode
				gAgent.setFocusGlobal(gAgent.calcFocusPositionTargetGlobal(), LLUUID::null);
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			}
			else
			{
				gAgent.setFocusOnAvatar(FALSE, ANIMATE);
				// zoom in on object center instead of where we clicked, as we need to see the manipulator handles
				gAgent.setFocusGlobal(gLastHitPosGlobal /*+ gLastHitObjectOffset*/, gLastHitObjectID);
				gAgent.cameraZoomIn(0.666f);
				gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
				gViewerWindow->moveCursorToCenter();
			}
		}

		gFloaterTools->open();		/* Flawfinder: ignore */
	
		gToolMgr->setCurrentToolset(gBasicToolset);
		gFloaterTools->setEditTool( gToolTranslate );

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLObjectInspect : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterInspect::show();
		return true;
	}
};


//---------------------------------------------------------------------------
// Land pie menu
//---------------------------------------------------------------------------
class LLLandBuild : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gParcelMgr->deselectLand();

		if (gAgent.getFocusOnAvatar() && !gToolMgr->inEdit() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
			gAgent.cameraZoomIn(0.666f);
			gAgent.cameraOrbitOver( 30.f * DEG_TO_RAD );
			gViewerWindow->moveCursorToCenter();
		}
		else if ( gSavedSettings.getBOOL("EditCameraMovement")  )
		{
			// otherwise just move focus
			gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
			gViewerWindow->moveCursorToCenter();
		}


		gToolMgr->setCurrentToolset(gBasicToolset);
		gToolMgr->getCurrentToolset()->selectTool( gToolCreate );

		// Could be first use
		LLFirstUse::useBuild();
		return true;
	}
};

class LLLandBuyPass : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLPanelLandGeneral::onClickBuyPass((void *)FALSE);
		return true;
	}
};

class LLLandEnableBuyPass : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLPanelLandGeneral::enableBuyPass(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// BUG: Should really check if CLICK POINT is in a parcel where you can build.
BOOL enable_land_build(void*)
{
	if (gAgent.isGodlike()) return TRUE;
	if (gAgent.inPrelude()) return FALSE;

	BOOL can_build = FALSE;
	LLParcel* agent_parcel = gParcelMgr->getAgentParcel();
	if (agent_parcel)
	{
		can_build = agent_parcel->getAllowModify();
	}
	return can_build;
}

// BUG: Should really check if OBJECT is in a parcel where you can build.
BOOL enable_object_build(void*)
{
	if (gAgent.isGodlike()) return TRUE;
	if (gAgent.inPrelude()) return FALSE;

	BOOL can_build = FALSE;
	LLParcel* agent_parcel = gParcelMgr->getAgentParcel();
	if (agent_parcel)
	{
		can_build = agent_parcel->getAllowModify();
	}
	return can_build;
}

class LLEnableEdit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike() || !gAgent.inPrelude();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLSelfRemoveAllAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLAgent::userRemoveAllAttachments(NULL);
		return true;
	}
};

class LLSelfEnableRemoveAllAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = false;
		if (gAgent.getAvatarObject())
		{
			LLVOAvatar* avatarp = gAgent.getAvatarObject();
			for (LLViewerJointAttachment* attachmentp = avatarp->mAttachmentPoints.getFirstData();
				attachmentp;
				attachmentp = avatarp->mAttachmentPoints.getNextData())
			{
				if (attachmentp->getObject())
				{
					new_value = true;
					break;
				}
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_has_attachments(void*)
{

	return FALSE;
}

//---------------------------------------------------------------------------
// Avatar pie menu
//---------------------------------------------------------------------------
void handle_follow(void *userdata)
{
	// follow a given avatar, ID in gLastHitObjectID
	gAgent.startFollowPilot(gLastHitObjectID);
}

class LLObjectEnableMute : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = gViewerWindow->lastObjectHit();
		bool new_value = (object != NULL);
		if (new_value)
		{
			LLVOAvatar* avatar = find_avatar_from_object(object); 
			if (avatar)
			{
				// It's an avatar
				LLNameValue *lastname = avatar->getNVPair("LastName");
				BOOL is_linden = lastname && !LLString::compareStrings(lastname->getString(), "Linden");
				BOOL is_self = avatar->isSelf();
				new_value = !is_linden && !is_self;
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectMute : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerObject* object = gViewerWindow->lastObjectHit();
		if (!object) return true;
		
		LLUUID id;
		LLString name;
		LLMute::EType type;
		LLVOAvatar* avatar = find_avatar_from_object(object); 
		if (avatar)
		{
			id = avatar->getID();

			LLNameValue *firstname = avatar->getNVPair("FirstName");
			LLNameValue *lastname = avatar->getNVPair("LastName");
			if (firstname && lastname)
			{
				name = firstname->getString();
				name += " ";
				name += lastname->getString();
			}
			
			type = LLMute::AGENT;
		}
		else
		{
			// it's an object
			id = object->getID();

			LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
			if (node)
			{
				name = node->mName;
			}
			
			type = LLMute::OBJECT;
		}
		
		LLMute mute(id, name, type);
		if (gMuteListp->isMuted(mute.mID, mute.mName))
		{
			gMuteListp->remove(mute);
		}
		else
		{
			gMuteListp->add(mute);
			gFloaterMute->show();
		}
		
		return true;
	}
};

bool handle_go_to()
{
	// JAMESDEBUG try simulator autopilot
	std::vector<std::string> strings;
	std::string val;
	val = llformat("%g", gLastHitPosGlobal.mdV[VX]);
	strings.push_back(val);
	val = llformat("%g", gLastHitPosGlobal.mdV[VY]);
	strings.push_back(val);
	val = llformat("%g", gLastHitPosGlobal.mdV[VZ]);
	strings.push_back(val);
	send_generic_message("autopilot", strings);

	gParcelMgr->deselectLand();

	if (gAgent.getAvatarObject() && !gSavedSettings.getBOOL("AutoPilotLocksCamera"))
	{
		gAgent.setFocusGlobal(gAgent.getFocusTargetGlobal(), gAgent.getAvatarObject()->getID());
	}
	else 
	{
		// Snap camera back to behind avatar
		gAgent.setFocusOnAvatar(TRUE, ANIMATE);
	}

	// Could be first use
	LLFirstUse::useGoTo();
	return true;
}

class LLGoToObject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_go_to();
	}
};

//---------------------------------------------------------------------------
// Parcel freeze, eject, etc.
//---------------------------------------------------------------------------
void callback_freeze(S32 option, void* data)
{
	LLUUID* avatar_id = (LLUUID*) data;

	if (0 == option || 1 == option)
	{
		U32 flags = 0x0;
		if (1 == option)
		{
			// unfreeze
			flags |= 0x1;
		}

		LLMessageSystem* msg = gMessageSystem;
		LLViewerObject* avatar = gObjectList.findObject(*avatar_id);

		if (avatar)
		{
			msg->newMessage("FreezeUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID());
			msg->addUUID("SessionID", gAgent.getSessionID());
			msg->nextBlock("Data");
			msg->addUUID("TargetID", *avatar_id );
			msg->addU32("Flags", flags );
			msg->sendReliable( avatar->getRegion()->getHost() );
		}
	}

	delete avatar_id;
	avatar_id = NULL;
}

class LLAvatarFreeze : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( gViewerWindow->lastObjectHit() );
		if( avatar )
		{
			LLUUID* avatar_id = new LLUUID( avatar->getID() );

			gViewerWindow->alertXml("FreezeAvatar",
				callback_freeze, (void*)avatar_id);
			
		}
		return true;
	}
};

class LLAvatarVisibleDebug : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLAvatarEnableDebug : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLAvatarDebug : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( gViewerWindow->lastObjectHit() );
		if( avatar )
		{
			avatar->dumpLocalTextures();
			llinfos << "Dumping temporary asset data to simulator logs for avatar " << avatar->getID() << llendl;
			std::vector<std::string> strings;
			strings.push_back(avatar->getID().asString());
			LLUUID invoice;
			send_generic_message("dumptempassetdata", strings, invoice);
			LLFloaterAvatarTextures::show( avatar->getID() );
		}
		return true;
	}
};

void callback_eject(S32 option, void* data)
{
	LLUUID* avatar_id = (LLUUID*) data;

	if (0 == option || 1 == option)
	{
		LLMessageSystem* msg = gMessageSystem;
		LLViewerObject* avatar = gObjectList.findObject(*avatar_id);

		if (avatar)
		{
			U32 flags = 0x0;
			if (1 == option)
			{
				// eject and add to ban list
				flags |= 0x1;
			}

			msg->newMessage("EjectUser");
			msg->nextBlock("AgentData");
			msg->addUUID("AgentID", gAgent.getID() );
			msg->addUUID("SessionID", gAgent.getSessionID() );
			msg->nextBlock("Data");
			msg->addUUID("TargetID", *avatar_id );
			msg->addU32("Flags", flags );
			msg->sendReliable( avatar->getRegion()->getHost() );
		}
	}

	delete avatar_id;
	avatar_id = NULL;
}

class LLAvatarEject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( gViewerWindow->lastObjectHit() );
		if( avatar )
		{
			LLUUID* avatar_id = new LLUUID( avatar->getID() );
			gViewerWindow->alertXml("EjectAvatar",
					callback_eject, (void*)avatar_id);
			
		}
		return true;
	}
};

class LLAvatarEnableFreezeEject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( gViewerWindow->lastObjectHit() );
		bool new_value = (avatar != NULL);

		if (new_value)
		{
			const LLVector3& pos = avatar->getPositionRegion();
			LLViewerRegion* region = avatar->getRegion();
			new_value = (region != NULL);

			if (new_value)
			{
				new_value = (region->isOwnedSelf(pos) || region->isOwnedGroup(pos));
			}
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLAvatarGiveCard : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		llinfos << "handle_give_card()" << llendl;
		LLViewerObject* dest = gViewerWindow->lastObjectHit();
		if(dest && dest->isAvatar())
		{
			bool found_name = false;
			LLString::format_map_t args;
			LLNameValue* nvfirst = dest->getNVPair("FirstName");
			LLNameValue* nvlast = dest->getNVPair("LastName");
			if(nvfirst && nvlast)
			{
				args["[FIRST]"] = nvfirst->getString();
				args["[LAST]"] = nvlast->getString();
				found_name = true;
			}
			LLViewerRegion* region = dest->getRegion();
			LLHost dest_host;
			if(region)
			{
				dest_host = region->getHost();
			}
			if(found_name && dest_host.isOk())
			{
				LLMessageSystem* msg = gMessageSystem;
				msg->newMessage("OfferCallingCard");
				msg->nextBlockFast(_PREHASH_AgentData);
				msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
				msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
				msg->nextBlockFast(_PREHASH_AgentBlock);
				msg->addUUIDFast(_PREHASH_DestID, dest->getID());
				LLUUID transaction_id;
				transaction_id.generate();
				msg->addUUIDFast(_PREHASH_TransactionID, transaction_id);
				msg->sendReliable(dest_host);
				LLNotifyBox::showXml("OfferedCard", args);
			}
			else
			{
				gViewerWindow->alertXml("CantOfferCallingCard", args);
			}
		}
		return true;
	}
};



void login_done(S32 which, void *user)
{
	llinfos << "Login done " << which << llendl;

	LLPanelLogin::close();
}


void callback_leave_group(S32 option, void *userdata)
{
	if (option == 0)
	{
		LLMessageSystem *msg = gMessageSystem;

		msg->newMessageFast(_PREHASH_LeaveGroupRequest);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->nextBlockFast(_PREHASH_GroupData);
		msg->addUUIDFast(_PREHASH_GroupID, gAgent.mGroupID );
		//msg->sendReliable( gUserServer );
		gAgent.sendReliableMessage();
	}
}

void handle_leave_group(void *)
{
	if (gAgent.getGroupID() != LLUUID::null)
	{
		LLString::format_map_t args;
		args["[GROUP]"] = gAgent.mGroupName;
		gViewerWindow->alertXml("GroupLeaveConfirmMember", args, callback_leave_group);
	}
}

void append_aggregate(LLString& string, const LLAggregatePermissions& ag_perm, PermissionBit bit, const char* txt)
{
	LLAggregatePermissions::EValue val = ag_perm.getValue(bit);
	char buffer[MAX_STRING];		/* Flawfinder: ignore */
	buffer[0] = '\0';
	switch(val)
	{
	case LLAggregatePermissions::AP_NONE:
		snprintf(buffer, MAX_STRING, "* %s None\n", txt);		/* Flawfinder: ignore */
		break;
	case LLAggregatePermissions::AP_SOME:
		snprintf(buffer, MAX_STRING, "* %s Some\n", txt);		/* Flawfinder: ignore */
		break;
	case LLAggregatePermissions::AP_ALL:
		snprintf(buffer, MAX_STRING, "* %s All\n", txt);		/* Flawfinder: ignore */
		break;
	case LLAggregatePermissions::AP_EMPTY:
	default:
		break;
	}
	string.append(buffer);
}

const char* build_extensions_string(LLFilePicker::ELoadFilter filter)
{
	switch(filter)
	{
#if LL_WINDOWS
	case LLFilePicker::FFLOAD_IMAGE:
		return IMAGE_EXTENSIONS;
	case LLFilePicker::FFLOAD_WAV:
		return SOUND_EXTENSIONS;
	case LLFilePicker::FFLOAD_ANIM:
		return ANIM_EXTENSIONS;
	case LLFilePicker::FFLOAD_SLOBJECT:
		return SLOBJECT_EXTENSIONS;
#ifdef _CORY_TESTING
	case LLFilePicker::FFLOAD_GEOMETRY:
		return GEOMETRY_EXTENSIONS;
#endif
	case LLFilePicker::FFLOAD_XML:
	    return XML_EXTENSIONS;
	case LLFilePicker::FFLOAD_ALL:
		return ALL_FILE_EXTENSIONS;
#endif
    default:
	return ALL_FILE_EXTENSIONS;
	}
}


BOOL enable_buy(void*)
{
    // In order to buy, there must only be 1 purchaseable object in
    // the selection manger.
	if(gSelectMgr->getSelection()->getRootObjectCount() != 1) return FALSE;
    LLViewerObject* obj = NULL;
    LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
	if(node)
    {
        obj = node->getObject();
        if(!obj) return FALSE;

		if(node->mSaleInfo.isForSale() && node->mPermissions->getMaskOwner() & PERM_TRANSFER &&
			(node->mPermissions->getMaskOwner() & PERM_COPY || node->mSaleInfo.getSaleType() != LLSaleInfo::FS_COPY))
		{
			if(obj->permAnyOwner()) return TRUE;
		}
    }
	return FALSE;
}

class LLObjectEnableBuy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_buy(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// Note: This will only work if the selected object's data has been
// received by the viewer and cached in the selection manager.
void handle_buy_object(LLSaleInfo sale_info)
{
	if(!gSelectMgr->selectGetAllRootsValid())
	{
		LLNotifyBox::showXml("UnableToBuyWhileDownloading");
		return;
	}

	LLUUID owner_id;
	LLString owner_name;
	BOOL owners_identical = gSelectMgr->selectGetOwner(owner_id, owner_name);
	if (!owners_identical)
	{
		LLNotifyBox::showXml("CannotBuyObjectsFromDifferentOwners");
		return;
	}

	LLPermissions perm;
	BOOL valid = gSelectMgr->selectGetPermissions(perm);
	LLAggregatePermissions ag_perm;
	valid &= gSelectMgr->selectGetAggregatePermissions(ag_perm);
	if(!valid || !sale_info.isForSale() || !perm.allowTransferTo(gAgent.getID()))
	{
		LLNotifyBox::showXml("ObjectNotForSale");
		return;
	}

	if(sale_info.getSalePrice() > gStatusBar->getBalance())
	{
		LLFloaterBuyCurrency::buyCurrency(
			"This object costs", sale_info.getSalePrice());
		return;
	}

	LLFloaterBuy::show(sale_info);
}


void handle_buy_contents(LLSaleInfo sale_info)
{
	LLFloaterBuyContents::show(sale_info);
}

void handle_region_dump_temp_asset_data(void*)
{
	llinfos << "Dumping temporary asset data to simulator logs" << llendl;
	std::vector<std::string> strings;
	LLUUID invoice;
	send_generic_message("dumptempassetdata", strings, invoice);
}

void handle_region_clear_temp_asset_data(void*)
{
	llinfos << "Clearing temporary asset data" << llendl;
	std::vector<std::string> strings;
	LLUUID invoice;
	send_generic_message("cleartempassetdata", strings, invoice);
}

void handle_region_dump_settings(void*)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		llinfos << "Damage:    " << (regionp->getAllowDamage() ? "on" : "off") << llendl;
		llinfos << "Landmark:  " << (regionp->getAllowLandmark() ? "on" : "off") << llendl;
		llinfos << "SetHome:   " << (regionp->getAllowSetHome() ? "on" : "off") << llendl;
		llinfos << "ResetHome: " << (regionp->getResetHomeOnTeleport() ? "on" : "off") << llendl;
		llinfos << "SunFixed:  " << (regionp->getSunFixed() ? "on" : "off") << llendl;
		llinfos << "BlockFly:  " << (regionp->getBlockFly() ? "on" : "off") << llendl;
		llinfos << "AllowP2P:  " << (regionp->getAllowDirectTeleport() ? "on" : "off") << llendl;
		llinfos << "Water:     " << (regionp->getWaterHeight()) << llendl;
	}
}

void handle_dump_group_info(void *)
{
	llinfos << "group   " << gAgent.mGroupName << llendl;
	llinfos << "ID      " << gAgent.mGroupID << llendl;
	llinfos << "powers " << gAgent.mGroupPowers << llendl;
	llinfos << "title   " << gAgent.mGroupTitle << llendl;
	//llinfos << "insig   " << gAgent.mGroupInsigniaID << llendl;
}

void handle_dump_capabilities_info(void *)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->logActiveCapabilities();
	}
}

void handle_dump_region_object_cache(void*)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	if (regionp)
	{
		regionp->dumpCache();
	}
}

void handle_dump_focus(void *)
{
	LLView *view = gFocusMgr.getKeyboardFocus();

	llinfos << "Keyboard focus " << (view ? view->getName() : "(none)") << llendl;
}

class LLSelfStandUp : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
		return true;
	}
};

class LLSelfEnableStandUp : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.getAvatarObject() && gAgent.getAvatarObject()->mIsSitting;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL check_admin_override(void*)
{
	return gAgent.getAdminOverride();
}

void handle_admin_override_toggle(void*)
{
	if(!gAgent.getAdminOverride())
	{
		gAgent.setAdminOverride(TRUE);
		show_debug_menus();
	}
	else gAgent.setAdminOverride(FALSE);
}

void handle_god_mode(void*)
{
	gAgent.requestEnterGodMode();
}

void handle_leave_god_mode(void*)
{
	gAgent.requestLeaveGodMode();
}

void set_god_level(U8 god_level)
{
		U8 old_god_level = gAgent.getGodLevel();
		gAgent.setGodLevel( god_level );
		show_debug_menus();
		gIMView->refresh();
		gParcelMgr->notifyObservers();

		// Some classifieds change visibility on god mode
		LLFloaterDirectory::requestClassifieds();

		// God mode changes sim visibility
		gWorldMap->reset();
		gWorldMap->setCurrentLayer(0);

		// inventory in items may change in god mode
		gObjectList.dirtyAllObjectInventory();

		LLString::format_map_t args;
		if(god_level > GOD_NOT)
		{
			args["[LEVEL]"] = llformat("%d",(S32)god_level);
			if (gInProductionGrid)
			{
				gMenuBarView->setBackgroundColor( gColors.getColor( "MenuBarGodBgColor" ) );
				gStatusBar->setBackgroundColor( gColors.getColor( "MenuBarGodBgColor" ) );
			}
			else
			{
				gMenuBarView->setBackgroundColor( gColors.getColor( "MenuNonProductionGodBgColor" ) );
				gStatusBar->setBackgroundColor( gColors.getColor( "MenuNonProductionGodBgColor" ) );
			}
			LLNotifyBox::showXml("EnteringGodMode", args);
		}
		else
		{
			args["[LEVEL]"] = llformat("%d",(S32)old_god_level);
			if (gInProductionGrid)
			{
				gMenuBarView->setBackgroundColor( gColors.getColor( "MenuBarBgColor" ) );
				gStatusBar->setBackgroundColor( gColors.getColor( "MenuBarBgColor" ) );
			}
			else
			{
				gMenuBarView->setBackgroundColor( gColors.getColor( "MenuNonProductionBgColor" ) );
				gStatusBar->setBackgroundColor( gColors.getColor( "MenuNonProductionBgColor" ) );
			}
			LLNotifyBox::showXml("LeavingGodMode", args);
		}
}

#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
void handle_toggle_hacked_godmode(void*)
{
	gHackGodmode = !gHackGodmode;
	set_god_level(gHackGodmode ? GOD_MAINTENANCE : GOD_NOT);
}

BOOL check_toggle_hacked_godmode(void*)
{
	return gHackGodmode;
}
#endif

void process_grant_godlike_powers(LLMessageSystem* msg, void**)
{
	LLUUID agent_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	LLUUID session_id;
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_SessionID, session_id);
	if((agent_id == gAgent.getID()) && (session_id == gAgent.getSessionID()))
	{
		U8 god_level;
		msg->getU8Fast(_PREHASH_GrantData, _PREHASH_GodLevel, god_level);
		set_god_level(god_level);
	}
	else
	{
		llwarns << "Grant godlike for wrong agent " << agent_id << llendl;
	}
}

void load_url_local_file(const char* file_name)
{
	if( gAgent.cameraMouselook() )
	{
		gAgent.changeCameraToDefault();
	}

#if LL_DARWIN || LL_LINUX
	// MBW -- If the Mac client is in fullscreen mode, it needs to go windowed so the browser will be visible.
	if(gViewerWindow->mWindow->getFullscreen())
	{
		gViewerWindow->toggleFullscreen(TRUE);
	}
#endif

	// JC - system() blocks until IE has launched.
	// spawn() runs asynchronously, but opens a command prompt.
	// ShellExecute() just opens the damn file with the default
	// web browser.
	std::string full_path = "file:///";
	full_path.append(gDirUtilp->getAppRODataDir());
	full_path.append(gDirUtilp->getDirDelimiter());
	full_path.append(file_name);

	LLWeb::loadURL(full_path.c_str());
}

/*
class LLHaveCallingcard : public LLInventoryCollectFunctor
{
public:
	LLHaveCallingcard(const LLUUID& agent_id);
	virtual ~LLHaveCallingcard() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	BOOL isThere() const { return mIsThere;}
protected:
	LLUUID mID;
	BOOL mIsThere;
};

LLHaveCallingcard::LLHaveCallingcard(const LLUUID& agent_id) :
	mID(agent_id),
	mIsThere(FALSE)
{
}

bool LLHaveCallingcard::operator()(LLInventoryCategory* cat,
								   LLInventoryItem* item)
{
	if(item)
	{
		if((item->getType() == LLAssetType::AT_CALLINGCARD)
		   && (item->getCreatorUUID() == mID))
		{
			mIsThere = TRUE;
		}
	}
	return FALSE;
}
*/

BOOL is_agent_friend(const LLUUID& agent_id)
{
	return (LLAvatarTracker::instance().getBuddyInfo(agent_id) != NULL);
}

BOOL is_agent_mappable(const LLUUID& agent_id)
{
	return (is_agent_friend(agent_id) &&
		LLAvatarTracker::instance().getBuddyInfo(agent_id)->isOnline() &&
		LLAvatarTracker::instance().getBuddyInfo(agent_id)->isRightGrantedFrom(LLRelationship::GRANT_MAP_LOCATION)
		);
}

// Enable a menu item when you have someone's card.
/*
BOOL enable_have_card(void *userdata)
{
	LLUUID* avatar_id = (LLUUID *)userdata;
	if (gAgent.isGodlike())
	{
		return TRUE;
	}
	else if(avatar_id)
	{
		return is_agent_friend(*avatar_id);
	}
	else
	{
		return FALSE;
	}
}
*/

// Enable a menu item when you don't have someone's card.
class LLAvatarEnableAddFriend : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(gViewerWindow->lastObjectHit());
		bool new_value = avatar && !is_agent_friend(avatar->getID());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

void request_friendship(const LLUUID& dest_id)
{
	LLViewerObject* dest = gObjectList.findObject(dest_id);
	if(dest && dest->isAvatar())
	{
		LLString fullname;
		LLString::format_map_t args;
		LLNameValue* nvfirst = dest->getNVPair("FirstName");
		LLNameValue* nvlast = dest->getNVPair("LastName");
		if(nvfirst && nvlast)
		{
			args["[FIRST]"] = nvfirst->getString();
			args["[LAST]"] = nvlast->getString();
			fullname = nvfirst->getString();
			fullname += " ";
			fullname += nvlast->getString();
		}
		if (!fullname.empty())
		{
			LLFloaterFriends::requestFriendship(dest_id, fullname);
			LLNotifyBox::showXml("OfferedFriendship", args);
		}
		else
		{
			gViewerWindow->alertXml("CantOfferFriendship");
		}
	}
}


class LLEditEnableCustomizeAvatar : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.getWearablesLoaded();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

bool handle_sit_or_stand()
{
	LLViewerObject *object = gObjectList.findObject(gLastHitNonFloraObjectID);	
	if (!object)
	{
		return true;
	}

	if (sitting_on_selection())
	{
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
		return true;
	}

	// get object selection offset 

	if (object && object->getPCode() == LL_PCODE_VOLUME)
	{
		LLVector3d offset_double = gViewerWindow->lastNonFloraObjectHitOffset();
		LLVector3 offset_single;
		offset_single.setVec(offset_double);
		
		gMessageSystem->newMessageFast(_PREHASH_AgentRequestSit);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_TargetObject);
		gMessageSystem->addUUIDFast(_PREHASH_TargetID, object->mID);
		gMessageSystem->addVector3Fast(_PREHASH_Offset, offset_single);

		object->getRegion()->sendReliableMessage();
	}
	return true;
}

class LLObjectSitOrStand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_sit_or_stand();
	}
};

void near_sit_down_point(BOOL success, void *)
{
	if (success)
	{
		gAgent.setFlying(FALSE);
		gAgent.setControlFlags(AGENT_CONTROL_SIT_ON_GROUND);

		// Might be first sit
		LLFirstUse::useSit();
	}
}

class LLLandSit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.setControlFlags(AGENT_CONTROL_STAND_UP);
		gParcelMgr->deselectLand();

		LLVector3d posGlobal = gLastHitPosGlobal;
		
		LLQuaternion target_rot;
		if (gAgent.getAvatarObject())
		{
			target_rot = gAgent.getAvatarObject()->getRotation();
		}
		else
		{
			target_rot = gAgent.getFrameAgent().getQuaternion();
		}
		gAgent.startAutoPilotGlobal(posGlobal, "Sit", &target_rot, near_sit_down_point, NULL, 0.7f);
		return true;
	}
};

void show_permissions_control(void*)
{
	LLFloaterPermissionsMgr* floaterp = LLFloaterPermissionsMgr::show();
	floaterp->mPermissions->addPermissionsData("foo1", LLUUID::null, 0);
	floaterp->mPermissions->addPermissionsData("foo2", LLUUID::null, 0);
	floaterp->mPermissions->addPermissionsData("foo3", LLUUID::null, 0);
}

#if 0 // Unused (these just modify AudioInfoPage which is not used anywhere in the code
void handle_audio_status_1(void*)
{
	S32 page = gSavedSettings.getS32("AudioInfoPage");
	if (1 == page)
	{
		page = 0;
	}
	else
	{
		page = 1;
	}
	gSavedSettings.setS32("AudioInfoPage", page);	
}

void handle_audio_status_2(void*)
{
	S32 page = gSavedSettings.getS32("AudioInfoPage");
	if (2 == page)
	{
		page = 0;
	}
	else
	{
		page = 2;
	}
	gSavedSettings.setS32("AudioInfoPage", page);	
}

void handle_audio_status_3(void*)
{
	S32 page = gSavedSettings.getS32("AudioInfoPage");
	if (3 == page)
	{
		page = 0;
	}
	else
	{
		page = 3;
	}
	gSavedSettings.setS32("AudioInfoPage", page);	
}

void handle_audio_status_4(void*)
{
	S32 page = gSavedSettings.getS32("AudioInfoPage");
	if (4 == page)
	{
		page = 0;
	}
	else
	{
		page = 4;
	}
	gSavedSettings.setS32("AudioInfoPage", page);	
}
#endif

void reload_ui(void *)
{
	gUICtrlFactory->rebuild();
}

class LLWorldFly : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.toggleFlying();
		return true;
	}
};

void handle_agent_stop_moving(void*)
{
	// stop agent
	gAgent.setControlFlags(AGENT_CONTROL_STOP);

	// cancel autopilot
	gAgent.stopAutoPilot();
}

void print_packets_lost(void*)
{
	gWorldPointer->printPacketsLost();
}


void drop_packet(void*)
{
	gMessageSystem->mPacketRing.dropPackets(1);
}


void velocity_interpolate( void* data )
{
	BOOL toggle = gSavedSettings.getBOOL("VelocityInterpolate");
	LLMessageSystem* msg = gMessageSystem;
	if ( !toggle )
	{
		msg->newMessageFast(_PREHASH_VelocityInterpolateOn);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		llinfos << "Velocity Interpolation On" << llendl;
	}
	else
	{
		msg->newMessageFast(_PREHASH_VelocityInterpolateOff);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gAgent.sendReliableMessage();
		llinfos << "Velocity Interpolation Off" << llendl;
	}
	// BUG this is a hack because of the change in menu behavior.  The
	// old menu system would automatically change a control's value,
	// but the new LLMenuGL system doesn't know what a control
	// is. However, it's easy to distinguish between the two callers
	// because LLMenuGL passes in the name of the user data (the
	// control name) to the callback function, and the user data goes
	// unused in the old menu code. Thus, if data is not null, then we
	// need to swap the value of the control.
	if( data )
	{
		gSavedSettings.setBOOL( static_cast<char*>(data), !toggle );
	}
}


void update_fov(S32 increments)
{
	F32 old_fov = gCamera->getDefaultFOV();
	// for each increment, FoV is 20% bigger
	F32 new_fov = old_fov * pow(1.2f, increments);

	// cap the FoV
	new_fov = llclamp(new_fov, MIN_FIELD_OF_VIEW, MAX_FIELD_OF_VIEW);

	if (new_fov != old_fov)
	{
		LLMessageSystem* msg = gMessageSystem;
		msg->newMessageFast(_PREHASH_AgentFOV);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addU32Fast(_PREHASH_CircuitCode, gMessageSystem->mOurCircuitCode);

		msg->nextBlockFast(_PREHASH_FOVBlock);
		msg->addU32Fast(_PREHASH_GenCounter, 0);
		msg->addF32Fast(_PREHASH_VerticalAngle, new_fov);

		gAgent.sendReliableMessage();

		// force agent to update dirty patches
		gCamera->setDefaultFOV(new_fov);
		gCamera->setView(new_fov);
	}
}

class LLViewZoomOut : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		update_fov(1);
		return true;
	}
};

class LLViewZoomIn : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		update_fov(-1);
		return true;
	}
};

class LLViewZoomDefault : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		F32 old_fov = gCamera->getView();
		// for each increment, FoV is 20% bigger
		F32 new_fov = DEFAULT_FIELD_OF_VIEW;

		if (new_fov != old_fov)
		{
			LLMessageSystem* msg = gMessageSystem;
			msg->newMessageFast(_PREHASH_AgentFOV);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->addU32Fast(_PREHASH_CircuitCode, gMessageSystem->mOurCircuitCode);
			msg->nextBlockFast(_PREHASH_FOVBlock);
			msg->addU32Fast(_PREHASH_GenCounter, 0);
			msg->addF32Fast(_PREHASH_VerticalAngle, new_fov);

			gAgent.sendReliableMessage();

			// force agent to update dirty patches
			gCamera->setDefaultFOV(new_fov);
			gCamera->setView(new_fov);
		}
		return true;
	}
};



void toggle_wind_audio(void)
{
	if (gAudiop)
	{
		gAudiop->enableWind(!(gAudiop->isWindEnabled()));
	}
}


// Callback for enablement
BOOL is_inventory_visible( void* user_data )
{
	LLInventoryView* iv = reinterpret_cast<LLInventoryView*>(user_data);
	if( iv )
	{
		return iv->getVisible();
	}
	return FALSE;
}

void handle_show_newest_map(void*)
{
	LLFloaterWorldMap::show(NULL, FALSE);
}

//-------------------------------------------------------------------
// Help menu functions
//-------------------------------------------------------------------

class LLHelpMOTD : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString::format_map_t args;
		args["[MOTD]"] = gAgent.mMOTD;
		gViewerWindow->alertXml("MOTD", args, NULL, NULL);
		return true;
	}
};

class LLHelpLiveHelp : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// the session_id of a 911 session will always be this agent's session id
		static LLUUID session_id(LLUUID::null);
		if (session_id.isNull())
		{
			session_id.generate();
		}
		gIMView->setFloaterOpen(TRUE);
		LLDynamicArray<LLUUID> members;
		members.put(gAgent.getID());
		gIMView->addSession("Help Request", IM_SESSION_911_START, session_id, members); //xui: translate
		return true;
	}
};

//
// Major mode switching
//
void reset_view_final( BOOL proceed, void* );

void handle_reset_view()
{
	if( (CAMERA_MODE_CUSTOMIZE_AVATAR == gAgent.getCameraMode()) && gFloaterCustomize )
	{
		// Show dialog box if needed.
		gFloaterCustomize->askToSaveAllIfDirty( reset_view_final, NULL );
	}
	else
	{
		reset_view_final( TRUE, NULL );
	}
}

class LLViewResetView : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_reset_view();
		return true;
	}
};

// Note: extra parameters allow this function to be called from dialog.
void reset_view_final( BOOL proceed, void* ) 
{
	if( !proceed )
	{
		return;
	}

	gAgent.changeCameraToDefault();
	
	if (LLViewerJoystick::sOverrideCamera)
	{
		handle_toggle_flycam(NULL);
	}

	gAgent.resetView(!gFloaterTools->getVisible());
	gFloaterTools->close();
	
	gViewerWindow->showCursor();

	// Switch back to basic toolset
	gToolMgr->setCurrentToolset(gBasicToolset);
}

class LLViewLookAtLastChatter : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.lookAtLastChat();
		return true;
	}
};

class LLViewMouselook : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (!gAgent.cameraMouselook())
		{
			gAgent.changeCameraToMouselook();
		}
		else
		{
			gAgent.changeCameraToDefault();
		}
		return true;
	}
};

class LLViewFullscreen : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gViewerWindow->toggleFullscreen(TRUE);
		return true;
	}
};

class LLViewDefaultUISize : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gSavedSettings.setF32("UIScaleFactor", 1.0f);
		gSavedSettings.setBOOL("UIAutoScale", FALSE);	
		gViewerWindow->reshape(gViewerWindow->getWindowDisplayWidth(), gViewerWindow->getWindowDisplayHeight());
		return true;
	}
};

class LLEditDuplicate : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if(gEditMenuHandler)
		{
			gEditMenuHandler->duplicate();
		}
		return true;
	}
};

class LLEditEnableDuplicate : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canDuplicate();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};


void disabled_duplicate(void*)
{
	if (gSelectMgr->getSelection()->getFirstObject())
	{
		LLNotifyBox::showXml("CopyFailed");
	}
}

void handle_duplicate_in_place(void*)
{
	llinfos << "handle_duplicate_in_place" << llendl;

	LLVector3 offset(0.f, 0.f, 0.f);
	gSelectMgr->selectDuplicate(offset, TRUE);
}

void handle_repeat_duplicate(void*)
{
	gSelectMgr->repeatDuplicate();
}

void handle_deed_object_to_group(void*)
{
	LLUUID group_id;
	
	gSelectMgr->selectGetGroup(group_id);
	gSelectMgr->sendOwner(LLUUID::null, group_id, FALSE);
	gViewerStats->incStat(LLViewerStats::ST_RELEASE_COUNT);
}

BOOL enable_deed_object_to_group(void*)
{
	if(gSelectMgr->getSelection()->isEmpty()) return FALSE;
	LLPermissions perm;
	LLUUID group_id;

	if (gSelectMgr->selectGetGroup(group_id) &&
		gAgent.hasPowerInGroup(group_id, GP_OBJECT_DEED) &&
		gSelectMgr->selectGetPermissions(perm) &&
		perm.deedToGroup(gAgent.getID(), group_id))
	{
		return TRUE;
	}
	return FALSE;
}


/*
 * No longer able to support viewer side manipulations in this way
 *
void god_force_inv_owner_permissive(LLViewerObject* object,
									InventoryObjectList* inventory,
									S32 serial_num,
									void*)
{
	typedef std::vector<LLPointer<LLViewerInventoryItem> > item_array_t;
	item_array_t items;

	InventoryObjectList::const_iterator inv_it = inventory->begin();
	InventoryObjectList::const_iterator inv_end = inventory->end();
	for ( ; inv_it != inv_end; ++inv_it)
	{
		if(((*inv_it)->getType() != LLAssetType::AT_CATEGORY)
		   && ((*inv_it)->getType() != LLAssetType::AT_ROOT_CATEGORY))
		{
			LLInventoryObject* obj = *inv_it;
			LLPointer<LLViewerInventoryItem> new_item = new LLViewerInventoryItem((LLViewerInventoryItem*)obj);
			LLPermissions perm(new_item->getPermissions());
			perm.setMaskBase(PERM_ALL);
			perm.setMaskOwner(PERM_ALL);
			new_item->setPermissions(perm);
			items.push_back(new_item);
		}
	}
	item_array_t::iterator end = items.end();
	item_array_t::iterator it;
	for(it = items.begin(); it != end; ++it)
	{
		// since we have the inventory item in the callback, it should not
		// invalidate iteration through the selection manager.
		object->updateInventory((*it), TASK_INVENTORY_ITEM_KEY, false);
	}
}
*/

void handle_object_owner_permissive(void*)
{
	// only send this if they're a god.
	if(gAgent.isGodlike())
	{
		// do the objects.
		gSelectMgr->selectionSetObjectPermissions(PERM_BASE, TRUE, PERM_ALL, TRUE);
		gSelectMgr->selectionSetObjectPermissions(PERM_OWNER, TRUE, PERM_ALL, TRUE);
	}
}

void handle_object_owner_self(void*)
{
	// only send this if they're a god.
	if(gAgent.isGodlike())
	{
		gSelectMgr->sendOwner(gAgent.getID(), gAgent.getGroupID(), TRUE);
	}
}

// Shortcut to set owner permissions to not editable.
void handle_object_lock(void*)
{
	gSelectMgr->selectionSetObjectPermissions(PERM_OWNER, FALSE, PERM_MODIFY);
}

void handle_object_asset_ids(void*)
{
	// only send this if they're a god.
	if (gAgent.isGodlike())
	{
		gSelectMgr->sendGodlikeRequest("objectinfo", "assetids");
	}
}

void handle_force_parcel_owner_to_me(void*)
{
	gParcelMgr->sendParcelGodForceOwner( gAgent.getID() );
}

void handle_force_parcel_to_content(void*)
{
	gParcelMgr->sendParcelGodForceToContent();
}

void handle_claim_public_land(void*)
{
	if (gParcelMgr->getSelectionRegion() != gAgent.getRegion())
	{
		LLNotifyBox::showXml("ClaimPublicLand");
		return;
	}

	LLVector3d west_south_global;
	LLVector3d east_north_global;
	gParcelMgr->getSelection(west_south_global, east_north_global);
	LLVector3 west_south = gAgent.getPosAgentFromGlobal(west_south_global);
	LLVector3 east_north = gAgent.getPosAgentFromGlobal(east_north_global);

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("GodlikeMessage");
	msg->nextBlock("AgentData");
	msg->addUUID("AgentID", gAgent.getID());
	msg->addUUID("SessionID", gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", "claimpublicland");
	msg->addUUID("Invoice", LLUUID::null);
	char buffer[32];		/* Flawfinder: ignore */
	snprintf(buffer, sizeof(buffer), "%f", west_south.mV[VX]);		/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	snprintf(buffer, sizeof(buffer), "%f", west_south.mV[VY]);		/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	snprintf(buffer, sizeof(buffer), "%f", east_north.mV[VX]);		/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	snprintf(buffer, sizeof(buffer), "%f", east_north.mV[VY]);		/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buffer);
	gAgent.sendReliableMessage();
}

void handle_god_request_havok(void *)
{
	if (gAgent.isGodlike())
	{
		gSelectMgr->sendGodlikeRequest("havok", "infoverbose");
	}
}

//void handle_god_request_foo(void *)
//{
//	if (gAgent.isGodlike())
//	{
//		gSelectMgr->sendGodlikeRequest(GOD_WANTS_FOO);
//	}
//}

//void handle_god_request_terrain_save(void *)
//{
//	if (gAgent.isGodlike())
//	{
//		gSelectMgr->sendGodlikeRequest("terrain", "save");
//	}
//}

//void handle_god_request_terrain_load(void *)
//{
//	if (gAgent.isGodlike())
//	{
//		gSelectMgr->sendGodlikeRequest("terrain", "load");
//	}
//}


// HACK for easily testing new avatar geometry
void handle_god_request_avatar_geometry(void *)
{
	if (gAgent.isGodlike())
	{
		gSelectMgr->sendGodlikeRequest("avatar toggle", NULL);
	}
}


void handle_show_overlay_title(void*)
{
	gShowOverlayTitle = !gShowOverlayTitle;
	gSavedSettings.setBOOL("ShowOverlayTitle", gShowOverlayTitle);
}

void derez_objects(EDeRezDestination dest, const LLUUID& dest_id)
{
	if(gAgent.cameraMouselook())
	{
		gAgent.changeCameraToDefault();
	}
	//gInventoryView->setPanelOpen(TRUE);

	LLObjectSelectionHandle selection = gSelectMgr->getSelection();
	LLViewerObject* object = NULL;
	LLSelectNode* node = selection->getFirstRootNode();
	if(!node) return;
	object = node->getObject();
	if(!object) return;
	LLViewerRegion* region = object->getRegion();
	char* error = NULL;

	// Check conditions that we can't deal with, building a list of
	// everything that we'll actually be derezzing.
	LLDynamicArray<LLViewerObject*> derez_objects;
	BOOL can_derez_current;
	for( ; node != NULL; node = selection->getNextRootNode())
	{
		object = node->getObject();
		if(!object || !node->mValid) continue;
		if(object->getRegion() != region)
		{
			// Derez doesn't work at all if the some of the objects
			// are in regions besides the first object selected.

			// ...crosses region boundaries
			error = "AcquireErrorObjectSpan";
			break;
		}
		if (object->isAvatar())
		{
			// ...don't acquire avatars
			continue;
		}

		// If AssetContainers are being sent back, they will appear as 
		// boxes in the owner's inventory.
		if (object->getNVPair("AssetContainer")
			&& dest != DRD_RETURN_TO_OWNER)
		{
			// this object is an asset container, derez its contents, not it
			llwarns << "Attempt to derez deprecated AssetContainer object type not supported." << llendl;
			/*
			object->requestInventory(container_inventory_arrived, 
				(void *)(BOOL)(DRD_TAKE_INTO_AGENT_INVENTORY == dest));
			*/
			continue;
		}
		can_derez_current = FALSE;
		switch(dest)
		{
		case DRD_TAKE_INTO_AGENT_INVENTORY:
		case DRD_TRASH:
			if( (node->mPermissions->allowTransferTo(gAgent.getID()) && object->permModify())
				|| (node->allowOperationOnNode(PERM_OWNER, GP_OBJECT_MANIPULATE)) )
			{
				can_derez_current = TRUE;
			}
			break;

		case DRD_RETURN_TO_OWNER:
			can_derez_current = TRUE;
			break;

		default:
			if((node->mPermissions->allowTransferTo(gAgent.getID())
				&& object->permCopy())
			   || gAgent.isGodlike())
			{
				can_derez_current = TRUE;
			}
			break;
		}
		if(can_derez_current)
		{
			derez_objects.put(object);
		}
	}

	// This constant is based on (1200 - HEADER_SIZE) / 4 bytes per
	// root.  I lopped off a few (33) to provide a bit
	// pad. HEADER_SIZE is currently 67 bytes, most of which is UUIDs.
	// This gives us a maximum of 63500 root objects - which should
	// satisfy anybody.
	const S32 MAX_ROOTS_PER_PACKET = 250;
	const S32 MAX_PACKET_COUNT = 254;
	F32 packets = ceil((F32)derez_objects.count() / (F32)MAX_ROOTS_PER_PACKET);
	if(packets > (F32)MAX_PACKET_COUNT)
	{
		error = "AcquireErrorTooManyObjects";
	}

	if(!error && derez_objects.count() > 0)
	{
		U8 d = (U8)dest;
		LLUUID tid;
		tid.generate();
		U8 packet_count = (U8)packets;
		S32 object_index = 0;
		S32 objects_in_packet = 0;
		LLMessageSystem* msg = gMessageSystem;
		for(U8 packet_number = 0;
			packet_number < packet_count;
			++packet_number)
		{
			msg->newMessageFast(_PREHASH_DeRezObject);
			msg->nextBlockFast(_PREHASH_AgentData);
			msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
			msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
			msg->nextBlockFast(_PREHASH_AgentBlock);
			msg->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());
			msg->addU8Fast(_PREHASH_Destination, d);	
			msg->addUUIDFast(_PREHASH_DestinationID, dest_id);
			msg->addUUIDFast(_PREHASH_TransactionID, tid);
			msg->addU8Fast(_PREHASH_PacketCount, packet_count);
			msg->addU8Fast(_PREHASH_PacketNumber, packet_number);
			objects_in_packet = 0;
			while((object_index < derez_objects.count())
				  && (objects_in_packet++ < MAX_ROOTS_PER_PACKET))

			{
				object = derez_objects.get(object_index++);
				msg->nextBlockFast(_PREHASH_ObjectData);
				msg->addU32Fast(_PREHASH_ObjectLocalID, object->getLocalID());
				// VEFFECT: DerezObject
				LLHUDEffectSpiral* effectp = (LLHUDEffectSpiral*)gHUDManager->createViewerEffect(LLHUDObject::LL_HUD_EFFECT_POINT, TRUE);
				effectp->setPositionGlobal(object->getPositionGlobal());
				effectp->setColor(LLColor4U(gAgent.getEffectColor()));
			}
			msg->sendReliable(region->getHost());
		}
		make_ui_sound("UISndObjectRezOut");

		// Busy count decremented by inventory update, so only increment
		// if will be causing an update.
		if (dest != DRD_RETURN_TO_OWNER)
		{
			gViewerWindow->getWindow()->incBusyCount();
		}
	}
	else if(error)
	{
		gViewerWindow->alertXml(error);
	}
}

class LLToolsTakeCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gSelectMgr->getSelection()->isEmpty()) return true;

		const LLUUID& category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_OBJECT);
		derez_objects(DRD_ACQUIRE_TO_AGENT_INVENTORY, category_id);

		return true;
	}
};


// You can return an object to its owner if it is on your land.
class LLObjectReturn : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gSelectMgr->getSelection()->isEmpty()) return true;
		
		mObjectSelection = gSelectMgr->getEditSelection();

		gViewerWindow->alertXml("ReturnToOwner",
			onReturnToOwner,
			(void*)this);
		return true;
	}

	static void onReturnToOwner(S32 option, void* data)
	{
		LLObjectReturn* object_return = (LLObjectReturn*)data;

		if (0 == option)
		{
			// Ignore category ID for this derez destination.
			derez_objects(DRD_RETURN_TO_OWNER, LLUUID::null);
		}

		// drop reference to current selection
		object_return->mObjectSelection = NULL;
	}

protected:
	LLObjectSelectionHandle mObjectSelection;
};


// Allow return to owner if one or more of the selected items is
// over land you own.
class LLObjectEnableReturn : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
#ifdef HACKED_GODLIKE_VIEWER
		bool new_value = true;
#else
		bool new_value = false;
		if (gAgent.isGodlike())
		{
			new_value = true;
		}
		else
		{
			LLViewerRegion* region = gAgent.getRegion();
			if (region)
			{
				// Estate owners and managers can always return objects.
				if (region->canManageEstate())
				{
					new_value = true;
				}
				else
				{
					LLObjectSelectionHandle selection = gSelectMgr->getSelection();
					LLViewerObject* obj = NULL;
					for(obj = selection->getFirstRootObject();
						obj;
						obj = selection->getNextRootObject())
					{
						if (obj->isOverAgentOwnedLand()
							|| obj->isOverGroupOwnedLand()
							|| obj->permModify())
						{
							new_value = true;
							break;
						}
					}
				}
			}
		}
#endif
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

void force_take_copy(void*)
{
	if (gSelectMgr->getSelection()->isEmpty()) return;
	const LLUUID& category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_OBJECT);
	derez_objects(DRD_FORCE_TO_GOD_INVENTORY, category_id);
}
#ifdef _CORY_TESTING

void force_export_copy(void*)
{
	LLViewerObject* object = NULL;
	LLSelectNode* node = gSelectMgr->getSelection()->getFirstNode();
	if(!node) return;
	object = node->getObject();
	if(!object) return;


	LLString proposed_name;
	proposed_name.append(node->mName);
	proposed_name.append( ".slg" );

	LLViewerRegion* region = object->getRegion();

	// Check conditions that we can't deal with, building a list of
	// everything that we'll actually be derezzing.
	
	std::vector<LLViewerObject*>		export_objects;
	std::vector<std::string>			export_names;
	std::vector<std::string>			export_descriptions;

	S32 object_index = 0;

	for( ; node != NULL; node = gSelectMgr->getSelection()->getNextNode())
	{
		object = node->getObject();
		if(!object || !node->mValid)
		{
			// Clicked cancel
			return;
		}
		if(object->getRegion() != region)
		{
			// Clicked cancel
			return;
		}
		if (object->isAvatar())
		{
			continue;
		}

		if (object->getNVPair("AssetContainer"))
		{
			continue;
		}
		export_objects.push_back(node->getObject());
		export_names.push_back(node->mName);
		export_descriptions.push_back(node->mDescription);
	}

	if (export_objects.empty())
	{
		return;
	}

	// pick a save file
	LLFilePicker& picker = LLFilePicker::instance();
	if (!picker.getSaveFile(LLFilePicker::FFSAVE_GEOMETRY, proposed_name))
	{
		// Clicked cancel
		return;
	}

	// Copy the directory + file name
	char filepath[LL_MAX_PATH];		/* Flawfinder: ignore */
	strncpy(filepath, picker.getFirstFile(), LL_MAX_PATH -1);		/* Flawfinder: ignore */
	filepath[LL_MAX_PATH -1] = '\0';

	apr_file_t* fp = ll_apr_file_open(filepath, LL_APR_W);

	if (!fp)
	{
		return;
	}

	object = export_objects[object_index];
	LLVector3 baseoffset = object->getPositionRegion();

	apr_file_printf(fp, "<?xml version=\"1.0\" encoding=\"US-ASCII\" standalone=\"yes\"?>\n");
	apr_file_printf(fp, "<LindenGeometry>\n");

	while(object_index < export_objects.size())
	{
		apr_file_printf(fp, "<Object\n");
		apr_file_printf(fp, "\tShape='%s'\n", export_names[object_index].c_str());
		apr_file_printf(fp, "\tDescription='%s'\n", export_descriptions[object_index].c_str());

		apr_file_printf(fp, "\tPCode='%d'\n", (U32)object->getPCode());
		apr_file_printf(fp, "\tMaterial='%d'\n", object->getMaterial());
		apr_file_printf(fp, "\tScale='%5f %5f %5f'\n", object->getScale().mV[VX], object->getScale().mV[VY], object->getScale().mV[VZ]);
		LLVector3 delta = object->getPositionRegion() - baseoffset;
		LLQuaternion rot = object->getRotationRegion();
		apr_file_printf(fp, "\tOffset='%5f %5f %5f'\n", delta.mV[VX], delta.mV[VY], delta.mV[VZ]);
		apr_file_printf(fp, "\tOrientation='%5f %5f %5f %5f'\n", rot.mQ[VX], rot.mQ[VY], rot.mQ[VZ], rot.mQ[VS]);
		const LLProfileParams pparams = object->getVolume()->getProfile().mParams;
		apr_file_printf(fp, "\tShapeProfile='%d %f %f %f'\n", pparams.getCurveType(), pparams.getBegin(), pparams.getEnd(), pparams.getHollow());
		const LLPathParams paparams = object->getVolume()->getPath().mParams;
		apr_file_printf(fp, "\tShapePath='%d %f %f %f %f %f %f %f %f %f %f %f %f %f'\n",
								 paparams.getCurveType(), paparams.getBegin(), paparams.getEnd(), paparams.getTwist(), paparams.getTwistBegin(), paparams.getScaleX(), paparams.getScaleY(),
								 paparams.getShearX(), paparams.getShearY(), paparams.getRadiusOffset(), paparams.getTaperX(), paparams.getTaperY(),
								 paparams.getRevolutions(), paparams.getSkew());
		S32 face, numfaces;
		numfaces = object->getNumTEs();
		apr_file_printf(fp, "\tNumberOfFaces='%d'>\n", numfaces);
		for (face = 0; face < numfaces; face++)
		{
			const LLTextureEntry *te = object->getTE(face);
			LLColor4 color = te->getColor();
			apr_file_printf(fp, "\t<Face\n\t\tFaceColor='%d %5f %5f %5f %5f'\n", face, color.mV[VX], color.mV[VY], color.mV[VZ], color.mV[VW]);
			
			char texture[UUID_STR_LENGTH];		/* Flawfinder: ignore */
			LLUUID texid = te->getID();
			texid.toString(texture);
			F32 sx, sy, ox, oy;
			te->getScale(&sx, &sy);
			te->getOffset(&ox, &oy);
			
			apr_file_printf(fp, "\t\tFace='%d %5f %5f %5f %5f %5f %d %s'\n\t/>\n", face, sx, sy, ox, oy, te->getRotation(), te->getBumpShinyFullbright(), texture);
		}
		apr_file_printf(fp, "</Object>\n");
		object = export_objects[++object_index];
	}

	apr_file_printf(fp, "</LindenGeometry>\n");

	fclose(fp);
}

void undo_find_local_contact_point(LLVector3 &contact,
								   const LLVector3& surface_norm, 
								   const LLQuaternion& rot, 
								   const LLVector3& scale  )
{
	LLVector3 local_norm = surface_norm;
	local_norm.rotVec( ~rot );

	LLVector3 v[6]; 
	v[0].mV[VX] = -1.f;
	v[1].mV[VX] = 1.f;
	
	v[2].mV[VY] = -1.f;
	v[3].mV[VY] = 1.f;

	v[4].mV[VZ] = -1.f;
	v[5].mV[VZ] = 1.f;

	contact = v[0];
	F32 cur_val = 0;

	for( S32 i = 0; i < 6; i++ )
	{
		F32 val = v[i] * local_norm;
		if( val < cur_val )
		{
			contact = v[i];
			cur_val = val;
		}
	}

	contact.mV[VX] *= 0.5f * scale.mV[VX];
	contact.mV[VY] *= 0.5f * scale.mV[VY];
	contact.mV[VZ] *= 0.5f * scale.mV[VZ];
	contact.rotVec( rot );
}



void force_import_geometry(void*)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if (!picker.getOpenFile(LLFilePicker::FFLOAD_GEOMETRY))
	{
		llinfos << "Couldn't import objects from file" << llendl;
		return;
	}

	char directory[LL_MAX_PATH];		/* Flawfinder: ignore */
	strncpy(directory, picker.getFirstFile(), LL_MAX_PATH -1);		/* Flawfinder: ignore */
	directory[LL_MAX_PATH -1] = '\0';

	llinfos << "Loading LSG file " << directory << llendl;
	LLXmlTree *xmlparser = new LLXmlTree();
	xmlparser->parseFile(directory, TRUE);
	LLXmlTreeNode	*root = xmlparser->getRoot();
	if( !root )
	{
		return;
	}
	// header
	if( !root->hasName( "LindenGeometry" ) )
	{
		llwarns << "Invalid LindenGeometry file header: " << directory << llendl;
		return;
	}
	// objects
	for (LLXmlTreeNode *child = root->getChildByName( "Object" );
		 child;
		 child = root->getNextNamedChild())
	{
		// get object data
		// *NOTE: This buffer size is hard coded into scanf() below.
		char name[255];		/* Flawfinder: ignore */			// Shape
		char description[255];		/* Flawfinder: ignore */		// Description
		U32	 material;			// Material
		F32  sx, sy, sz;		// Scale
		LLVector3 scale;
		F32  ox, oy, oz;		// Offset
		LLVector3 offset;
		F32  rx, ry, rz, rs;	// Orientation
		LLQuaternion rot;
		U32	 curve;
		F32  begin;
		F32  end;
		F32	 hollow;
		F32	 twist;
		F32	 scx, scy;
		F32  shx, shy;
		F32  twist_begin;
		F32	 radius_offset;
		F32  tx, ty;
		F32	 revolutions;
		F32	 skew;
		S32  faces;
		U32 pcode;
		U32 flags = FLAGS_CREATE_SELECTED;

		LLString attribute;

		S32 count = 0;

		child->getAttributeString("PCode", &attribute);
		pcode = atoi(attribute.c_str());
		child->getAttributeString("Shape", &attribute);
		sscanf(	/* Flawfinder: ignore */
			attribute.c_str(), "%254s", name);
		child->getAttributeString("Description", &attribute);
		sscanf(	/* Flawfinder: ignore */
			attribute.c_str(), "%254s", description);
		child->getAttributeString("Material", &attribute);
		material = atoi(attribute.c_str());
		child->getAttributeString("Scale", &attribute);
		sscanf(attribute.c_str(), "%f %f %f", &sx, &sy, &sz);
		scale.setVec(sx, sy, sz);
		child->getAttributeString("Offset", &attribute);
		sscanf(attribute.c_str(), "%f %f %f", &ox, &oy, &oz);
		offset.setVec(ox, oy, oz);
		child->getAttributeString("Orientation", &attribute);
		sscanf(attribute.c_str(), "%f %f %f %f", &rx, &ry, &rz, &rs);
		rot.mQ[VX] = rx;
		rot.mQ[VY] = ry;
		rot.mQ[VZ] = rz;
		rot.mQ[VS] = rs;

		child->getAttributeString("ShapeProfile", &attribute);
		sscanf(attribute.c_str(), "%d %f %f %f", &curve, &begin, &end, &hollow);
		LLProfileParams pparams(curve, begin, end, hollow);
		child->getAttributeString("ShapePath", &attribute);
		sscanf(attribute.c_str(), "%d %f %f %f %f %f %f %f %f %f %f %f %f %f", 
			&curve, &begin, &end, &twist, &twist_begin, &scx, &scy, &shx, &shy, &radius_offset, &tx, &ty, &revolutions, &skew);
		LLPathParams paparams(curve, begin, end, scx, scy, shx, shy, twist, twist_begin, radius_offset, tx, ty, revolutions, skew);
		child->getAttributeString("NumberOfFaces", &attribute);
		faces = atoi(attribute.c_str());



		gMessageSystem->newMessageFast(_PREHASH_ObjectAdd);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->addUUIDFast(_PREHASH_GroupID, gAgent.getGroupID());

		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU8Fast(_PREHASH_PCode,		pcode);
		gMessageSystem->addU8Fast(_PREHASH_Material,	material);
		gMessageSystem->addU32Fast(_PREHASH_AddFlags,	flags );
		pparams.packMessage(gMessageSystem);
		paparams.packMessage(gMessageSystem);

		LLVector3 forward;
		forward.setVec(3.f, 0.f, 1.f);
		forward = forward * gAgent.getQuat();

		LLVector3 start = gAgent.getPositionAgent() + forward;

		start += offset;

		// offset position to make up for error introduced by placement code
		LLVector3 normal(0.f, 0.f, 1.f);
		LLVector3 delta;

		undo_find_local_contact_point(delta, normal, rot, scale);

		start += delta;

		gMessageSystem->addVector3Fast(_PREHASH_Scale,			scale );
		gMessageSystem->addQuatFast(_PREHASH_Rotation,			rot );
		gMessageSystem->addVector3Fast(_PREHASH_RayStart,		start );
		gMessageSystem->addVector3Fast(_PREHASH_RayEnd,			start );
		gMessageSystem->addBOOLFast(_PREHASH_BypassRaycast,		TRUE );
		gMessageSystem->addBOOLFast(_PREHASH_RayEndIsIntersection,	FALSE );

		U8 state = 0;
		gMessageSystem->addU8Fast(_PREHASH_State, state);
	
		LLUUID ray_target_id;
		gMessageSystem->addUUIDFast(_PREHASH_RayTargetID,			ray_target_id );
	
		/* Setting TE info through ObjectAdd is no longer supported.
		LLPrimitive     temp_primitive;
		temp_primitive.setNumTEs(faces);
		for (LLXmlTreeNode *face = child->getChildByName( "Face" );
			 face;
			 face = child->getNextNamedChild())
		{
			// read the faces
			U32 facenumber;
			LLColor4 color;
			// *NOTE: This buffer size is hard coded into scanf() below.
			char texture[UUID_STR_LENGTH];
			LLUUID texid;
			texid.toString(texture);
			F32 sx, sy, ox, oy, rot;
			U8 bump;
			LLTextureEntry te;

			face->getAttributeString("FaceColor", &attribute);
			sscanf(attribute, "%d %f %f %f %f", &facenumber, &color.mV[VX], &color.mV[VY], &color.mV[VZ], &color.mV[VW]);
			face->getAttributeString("Face", &attribute);
			sscanf(attribute, "%d %f %f %f %f %f %d %36s", &facenumber, &sx, &sy, &ox, &oy, &rot, &bump, texture);
			texid.set(texture);
			te.setColor(color);
			te.setBumpShinyFullbright(bump);
			te.setID(texid);
			te.setRotation(rot);
			te.setOffset(ox, oy);
			te.setScale(sx, sy);

			temp_primitive.setTE(facenumber, te);
		}

		temp_primitive.packTEMessage(gMessageSystem);
		*/
		gMessageSystem->sendReliable(gAgent.getRegionHost());
	}

}
#endif

void handle_take()
{
	// we want to use the folder this was derezzed from if it's
	// available. Otherwise, derez to the normal place.
	if(gSelectMgr->getSelection()->isEmpty()) return;
	LLSelectNode* node = NULL;
	LLViewerObject* object = NULL;
	BOOL you_own_everything = TRUE;

	BOOL locked_but_takeable_object = FALSE;
	LLUUID category_id;
	for(node = gSelectMgr->getSelection()->getFirstRootNode();
		node != NULL;
		node = gSelectMgr->getSelection()->getNextRootNode())
	{
		object = node->getObject();
		if(object)
		{
			if(!object->permYouOwner())
			{
				you_own_everything = FALSE;
			}

			if(!object->permMove())

			{

				locked_but_takeable_object = TRUE;

			}
		}
		if(node->mFolderID.notNull())
		{
			if(category_id.isNull())
			{
				category_id = node->mFolderID;
			}
			else if(category_id != node->mFolderID)
			{
				// we have found two potential destinations. break out
				// now and send to the default location.
				category_id.setNull();
				break;
			}
		}
	}
	if(category_id.notNull())
	{
		// there is an unambiguous destination. See if this agent has
		// such a location and it is not in the trash or library
		if(!gInventory.getCategory(category_id))
		{
			// nope, set to NULL.
			category_id.setNull();
		}
		if(category_id.notNull())
		{
		        // check trash
			LLUUID trash;
			trash = gInventory.findCategoryUUIDForType(LLAssetType::AT_TRASH);
			if(category_id == trash || gInventory.isObjectDescendentOf(category_id, trash))
			{
				category_id.setNull();
			}

			// check library
			if(gInventory.isObjectDescendentOf(category_id, gInventoryLibraryRoot))
			{
				category_id.setNull();
			}

		}
	}
	if(category_id.isNull())
	{
		category_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_OBJECT);
	}
	LLUUID* cat_id = new LLUUID(category_id);
	if(locked_but_takeable_object ||

	   !you_own_everything)
	{
		if(locked_but_takeable_object && you_own_everything)
		{
			gViewerWindow->alertXml("ConfirmObjectTakeLock",
			confirm_take,
			(void*)cat_id);

		}
		else if(!locked_but_takeable_object && !you_own_everything)
		{
			gViewerWindow->alertXml("ConfirmObjectTakeNoOwn",
			confirm_take,
			(void*)cat_id);
		}
		else
		{
			gViewerWindow->alertXml("ConfirmObjectTakeLockNoOwn",
			confirm_take,
			(void*)cat_id);
		}


	}

	else
	{
		confirm_take(0, (void*)cat_id);
	}
}

void confirm_take(S32 option, void* data)
{
	LLUUID* cat_id = (LLUUID*)data;
	if(!cat_id) return;
	if(enable_take() && (option == 0))
	{
		derez_objects(DRD_TAKE_INTO_AGENT_INVENTORY, *cat_id);
	}
	delete cat_id;
}

// You can take an item when it is public and transferrable, or when
// you own it. We err on the side of enabling the item when at least
// one item selected can be copied to inventory.
BOOL enable_take()
{
	if (sitting_on_selection())
	{
		return FALSE;
	}

	LLViewerObject* object = NULL;
	for(LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
		node != NULL;
		node = gSelectMgr->getSelection()->getNextRootNode())
	{
		object = node->getObject();
		if(!object || !node->mValid) continue;
		if (object->isAvatar())
		{
			// ...don't acquire avatars
			continue;
		}

#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!gInProductionGrid && gAgent.isGodlike())
		{
			return TRUE;
		}
# endif
		if((node->mPermissions->allowTransferTo(gAgent.getID())
			&& object->permModify())
		   || (node->mPermissions->getOwner() == gAgent.getID()))
		{
			return TRUE;
		}
#endif
	}
	return FALSE;
}

class LLToolsBuyOrTake : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gSelectMgr->getSelection()->isEmpty())
		{
			return true;
		}

		if (is_selection_buy_not_take())
		{
			S32 total_price = selection_price();

			if (total_price <= gStatusBar->getBalance())
			{
				handle_buy(NULL);
			}
			else
			{
				LLFloaterBuyCurrency::buyCurrency(
					"Buying this costs", total_price);
			}
		}
		else
		{
			handle_take();
		}
		return true;
	}
};

class LLToolsEnableBuyOrTake : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool is_buy = is_selection_buy_not_take();
		bool new_value = is_buy ? enable_buy(NULL) : enable_take();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		// Update label
		LLString label;
		LLString buy_text;
		LLString take_text;
		LLString param = userdata["data"].asString();
		LLString::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			buy_text = param.substr(0, offset);
			take_text = param.substr(offset+1);
		}
		if (is_buy)
		{
			label = buy_text;
		}
		else
		{
			label = take_text;
		}
		gMenuHolder->childSetText("Pie Object Take", label);
		gMenuHolder->childSetText("Menu Object Take", label);

		return true;
	}
};

// This is a small helper function to determine if we have a buy or a
// take in the selection. This method is to help with the aliasing
// problems of putting buy and take in the same pie menu space. After
// a fair amont of discussion, it was determined to prefer buy over
// take. The reasoning follows from the fact that when users walk up
// to buy something, they will click on one or more items. Thus, if
// anything is for sale, it becomes a buy operation, and the server
// will group all of the buy items, and copyable/modifiable items into
// one package and give the end user as much as the permissions will
// allow. If the user wanted to take something, they will select fewer
// and fewer items until only 'takeable' items are left. The one
// exception is if you own everything in the selection that is for
// sale, in this case, you can't buy stuff from yourself, so you can
// take it.
// return value = TRUE if selection is a 'buy'.
//                FALSE if selection is a 'take'
BOOL is_selection_buy_not_take()
{
	LLViewerObject* obj = NULL;
	for(LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
		node != NULL;
		node = gSelectMgr->getSelection()->getNextRootNode())
	{
		obj = node->getObject();
		if(obj && !(obj->permYouOwner()) && (node->mSaleInfo.isForSale()))
		{
			// you do not own the object and it is for sale, thus,
			// it's a buy
			return TRUE;
		}
	}
	return FALSE;
}

S32 selection_price()
{
	LLViewerObject* obj = NULL;
	S32 total_price = 0;
	for(LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
		node != NULL;
		node = gSelectMgr->getSelection()->getNextRootNode())
	{
		obj = node->getObject();
		if(obj && !(obj->permYouOwner()) && (node->mSaleInfo.isForSale()))
		{
			// you do not own the object and it is for sale.
			// Add its price.
			total_price += node->mSaleInfo.getSalePrice();
		}
	}

	return total_price;
}

void callback_show_buy_currency(S32 option, void*)
{
	if (0 == option)
	{
		llinfos << "Loading page " << BUY_CURRENCY_URL << llendl;
		LLWeb::loadURL(BUY_CURRENCY_URL);
	}
}


void show_buy_currency(const char* extra)
{
	// Don't show currency web page for branded clients.

	std::ostringstream mesg;
	if (extra != NULL)
	{	
		mesg << extra << "\n \n";
	}
	mesg << "Go to " << BUY_CURRENCY_URL << "\nfor information on purchasing currency?";

	LLString::format_map_t args;
	if (extra != NULL)
	{
		args["[EXTRA]"] = extra;
	}
	args["[URL]"] = BUY_CURRENCY_URL;
	gViewerWindow->alertXml("PromptGoToCurrencyPage", args, 
		callback_show_buy_currency);
}

void handle_buy_currency(void*)
{
//	LLFloaterBuyCurrency::buyCurrency();
}

void handle_buy(void*)
{
	if (gSelectMgr->getSelection()->isEmpty()) return;

	LLSaleInfo sale_info;
	BOOL valid = gSelectMgr->selectGetSaleInfo(sale_info);
	if (!valid) return;

	if (sale_info.getSaleType() == LLSaleInfo::FS_CONTENTS)
	{
		handle_buy_contents(sale_info);
	}
	else
	{
		handle_buy_object(sale_info);
	}
}

class LLObjectBuy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_buy(NULL);
		return true;
	}
};

BOOL sitting_on_selection()
{
	LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
	if (!node)
	{
		return FALSE;
	}

	if (!node->mValid)
	{
		return FALSE;
	}

	LLViewerObject* root_object = node->getObject();
	if (!root_object)
	{
		return FALSE;
	}

	// Need to determine if avatar is sitting on this object
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (!avatar)
	{
		return FALSE;
	}

	return (avatar->mIsSitting && avatar->getRoot() == root_object);
}

class LLToolsSaveToInventory : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if(enable_save_into_inventory(NULL))
		{
			derez_objects(DRD_SAVE_INTO_AGENT_INVENTORY, LLUUID::null);
		}
		return true;
	}
};

class LLToolsSaveToObjectInventory : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if(gSelectMgr)
		{
			LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
			if(node && (node->mValid) && (!node->mFromTaskID.isNull()))
			{
				// *TODO: check to see if the fromtaskid object exists.
				derez_objects(DRD_SAVE_INTO_TASK_INVENTORY, node->mFromTaskID);
			}
		}
		return true;
	}
};

// Round the position of all root objects to the grid
class LLToolsSnapObjectXY : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		F64 snap_size = (F64)gSavedSettings.getF32("GridResolution");

		LLViewerObject* obj;
		LLObjectSelectionHandle selection = gSelectMgr->getSelection();
		for (obj = selection->getFirstRootObject();
			obj != NULL;
			obj = selection->getNextRootObject())
		{
			if (obj->permModify())
			{
				LLVector3d pos_global = obj->getPositionGlobal();
				F64 round_x = fmod(pos_global.mdV[VX], snap_size);
				if (round_x < snap_size * 0.5)
				{
					// closer to round down
					pos_global.mdV[VX] -= round_x;
				}
				else
				{
					// closer to round up
					pos_global.mdV[VX] -= round_x;
					pos_global.mdV[VX] += snap_size;
				}

				F64 round_y = fmod(pos_global.mdV[VY], snap_size);
				if (round_y < snap_size * 0.5)
				{
					pos_global.mdV[VY] -= round_y;
				}
				else
				{
					pos_global.mdV[VY] -= round_y;
					pos_global.mdV[VY] += snap_size;
				}

				obj->setPositionGlobal(pos_global, FALSE);
			}
		}
		gSelectMgr->sendMultipleUpdate(UPD_POSITION);
		return true;
	}
};

// in order to link, all objects must have the same owner, and the
// agent must have the ability to modify all of the objects. However,
// we're not answering that question with this method. The question
// we're answering is: does the user have a reasonable expectation
// that a link operation should work? If so, return true, false
// otherwise. this allows the handle_link method to more finely check
// the selection and give an error message when the uer has a
// reasonable expectation for the link to work, but it will fail.
class LLToolsEnableLink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = false;
		// check if there are at least 2 objects selected, and that the
		// user can modify at least one of the selected objects.

		// in component mode, can't link
		if (gSavedSettings.getBOOL("SelectLinkedSet"))
		{
			if(gSelectMgr->selectGetAllRootsValid() && gSelectMgr->getSelection()->getRootObjectCount() >= 2)
			{
				LLObjectSelectionHandle selection = gSelectMgr->getSelection();
				for(LLViewerObject* object = selection->getFirstRootObject();
					object != NULL;
					object = selection->getNextRootObject())
				{
					if(object->permModify())
					{
						new_value = true;
						break;
					}
				}
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsLink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if(!gSelectMgr->selectGetAllRootsValid())
		{
			LLNotifyBox::showXml("UnableToLinkWhileDownloading");
			return true;
		}

		S32 object_count = gSelectMgr->getSelection()->getObjectCount();
		if (object_count > MAX_CHILDREN_PER_TASK + 1)
		{
			LLStringBase<char>::format_map_t args;
			args["[COUNT]"] = llformat("%d", object_count);
			int max = MAX_CHILDREN_PER_TASK+1;
			args["[MAX]"] = llformat("%d", max);
			gViewerWindow->alertXml("UnableToLinkObjects", args);
			return true;
		}

		if(gSelectMgr->getSelection()->getRootObjectCount() < 2)
		{
			gViewerWindow->alertXml("CannotLinkIncompleteSet");
			return true;
		}
		if(!gSelectMgr->selectGetRootsModify())
		{
			gViewerWindow->alertXml("CannotLinkModify");
			return true;
		}
		LLUUID owner_id;
		LLString owner_name;
		if(!gSelectMgr->selectGetOwner(owner_id, owner_name))
		{
			// we don't actually care if you're the owner, but novices are
			// the most likely to be stumped by this one, so offer the
			// easiest and most likely solution.
			gViewerWindow->alertXml("CannotLinkDifferentOwners");
			return true;
		}
		gSelectMgr->sendLink();
		return true;
	}
};

class LLToolsEnableUnlink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gSelectMgr->selectGetAllRootsValid() &&
			gSelectMgr->getSelection()->getFirstEditableObject() &&
			!gSelectMgr->getSelection()->getFirstEditableObject()->isAttachment();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsUnlink : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gSelectMgr->sendDelink();
		return true;
	}
};


class LLToolsStopAllAnimations : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatarp = gAgent.getAvatarObject();
		
		if (!avatarp) return true;
		
		LLVOAvatar::AnimSourceIterator anim_it = avatarp->mAnimationSources.begin();
		for (;anim_it != avatarp->mAnimationSources.end(); ++anim_it)
		{
			avatarp->stopMotion( anim_it->second, TRUE );
		}
	
		avatarp->processAnimationStateChanges();
		return true;
	}
};

void handle_hinge(void*)
{
	gSelectMgr->sendHinge(1);
}

void handle_ptop(void*)
{
	gSelectMgr->sendHinge(2);
}

void handle_lptop(void*)
{
	gSelectMgr->sendHinge(3);
}

void handle_wheel(void*)
{
	gSelectMgr->sendHinge(4);
}

void handle_dehinge(void*)
{
	gSelectMgr->sendDehinge();
}

BOOL enable_dehinge(void*)
{
	LLViewerObject* obj = gSelectMgr->getSelection()->getFirstEditableObject();
	return obj && !obj->isAttachment();
}


class LLEditEnableCut : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canCut();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditCut : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler )
		{
			gEditMenuHandler->cut();
		}
		return true;
	}
};

class LLEditEnableCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canCopy();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler )
		{
			gEditMenuHandler->copy();
		}
		return true;
	}
};

class LLEditEnablePaste : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canPaste();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditPaste : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler )
		{
			gEditMenuHandler->paste();
		}
		return true;
	}
};

class LLEditEnableDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canDoDelete();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// If a text field can do a deletion, it gets precedence over deleting
		// an object in the world.
		if( gEditMenuHandler && gEditMenuHandler->canDoDelete())
		{
			gEditMenuHandler->doDelete();
		}

		// and close any pie/context menus when done
		gMenuHolder->hideMenus();

		// When deleting an object we may not actually be done
		// Keep selection so we know what to delete when confirmation is needed about the delete
		gPieObject->hide(TRUE);
		return true;
	}
};

class LLObjectEnableDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = 
#ifdef HACKED_GODLIKE_VIEWER
			TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
			(!gInProductionGrid && gAgent.isGodlike()) ||
# endif
			(gSelectMgr && gSelectMgr->canDoDelete());
#endif
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditSearch : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLFloaterDirectory::toggleFind(NULL);
		return true;
	}
};

class LLObjectDelete : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gSelectMgr)
		{
			gSelectMgr->doDelete();
		}

		// and close any pie/context menus when done
		gMenuHolder->hideMenus();

		// When deleting an object we may not actually be done
		// Keep selection so we know what to delete when confirmation is needed about the delete
		gPieObject->hide(TRUE);
		return true;
	}
};

void handle_force_delete(void*)
{
	gSelectMgr->selectForceDelete();
}

class LLViewEnableLastChatter : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// *TODO: add check that last chatter is in range
		bool new_value = (gAgent.cameraThirdPerson() && gAgent.getLastChatter().notNull());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditEnableDeselect : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canDeselect();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditDeselect : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler )
		{
			gEditMenuHandler->deselect();
		}
		return true;
	}
};

class LLEditEnableSelectAll : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canSelectAll();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};


class LLEditSelectAll : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler )
		{
			gEditMenuHandler->selectAll();
		}
		return true;
	}
};


class LLEditEnableUndo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canUndo();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditUndo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler && gEditMenuHandler->canUndo() )
		{
			gEditMenuHandler->undo();
		}
		return true;
	}
};

class LLEditEnableRedo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gEditMenuHandler && gEditMenuHandler->canRedo();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditRedo : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if( gEditMenuHandler && gEditMenuHandler->canRedo() )
		{
			gEditMenuHandler->redo();
		}
		return true;
	}
};



void print_object_info(void*)
{
	gSelectMgr->selectionDump();
}


void show_debug_menus()
{
	// this can get called at login screen where there is no menu so only toggle it if one exists
	if ( gMenuBarView )
	{
		BOOL debug = gSavedSettings.getBOOL("UseDebugMenus");
		gMenuBarView->setItemVisible(CLIENT_MENU_NAME, debug);
		gMenuBarView->setItemEnabled(CLIENT_MENU_NAME, debug);
		gMenuBarView->setItemVisible(SERVER_MENU_NAME, debug);
		gMenuBarView->setItemEnabled(SERVER_MENU_NAME, debug);
		//gMenuBarView->setItemVisible(LLString("DebugOptions"),	visible);
		//gMenuBarView->setItemVisible(LLString(AVI_TOOLS),	visible);
	};
}

void toggle_debug_menus(void*)
{
	BOOL visible = ! gSavedSettings.getBOOL("UseDebugMenus");
	gSavedSettings.setBOOL("UseDebugMenus", visible);
	show_debug_menus();
}


void toggle_map( void* user_data )
{
	// Toggle the item
	BOOL checked = gSavedSettings.getBOOL( static_cast<char*>(user_data) );
	gSavedSettings.setBOOL( static_cast<char*>(user_data), !checked );
	if (checked)
	{
		gFloaterMap->close();
	}
	else
	{
		gFloaterMap->open();		/* Flawfinder: ignore */	
	}
}


LLUUID gExporterRequestID;
LLString gExportDirectory;

LLUploadDialog *gExportDialog = NULL;

void handle_export_selected( void * )
{
	LLObjectSelectionHandle selection = gSelectMgr->getSelection();
	if (selection->isEmpty())
	{
		return;
	}
	llinfos << "Exporting selected objects:" << llendl;
	LLViewerObject *object = selection->getFirstRootObject();

	gExporterRequestID.generate();
	gExportDirectory = "";

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessageFast(_PREHASH_ObjectExportSelected);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_RequestID, gExporterRequestID);
	msg->addS16Fast(_PREHASH_VolumeDetail, 4);

	for (; object != NULL; object = selection->getNextRootObject())
	{
		msg->nextBlockFast(_PREHASH_ObjectData);
		msg->addUUIDFast(_PREHASH_ObjectID, object->getID());
		llinfos << "Object: " << object->getID() << llendl;
	}
	msg->sendReliable(gAgent.getRegion()->getHost());

	gExportDialog = LLUploadDialog::modalUploadDialog("Exporting selected objects...");
}

BOOL menu_check_build_tool( void* user_data )
{
	S32 index = (intptr_t) user_data;
	return gToolMgr->getCurrentToolset()->isToolSelected( index );
}

void handle_reload_settings(void*)
{
	gSavedSettings.resetToDefaults();
	gSavedSettings.loadFromFile(gSettingsFileName, TRUE);

	llinfos << "Loading colors from colors.xml" << llendl;
	std::string color_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS,"colors.xml");
	gColors.resetToDefaults();
	gColors.loadFromFile(color_file, FALSE, TYPE_COL4U);
}

class LLWorldSetHomeLocation : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// we just send the message and let the server check for failure cases
		// server will echo back a "Home position set." alert if it succeeds
		// and the home location screencapture happens when that alert is recieved
		gAgent.setStartPosition(START_LOCATION_ID_HOME);
		return true;
	}
};

class LLWorldTeleportHome : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gAgent.teleportHome();
		return true;
	}
};

class LLWorldAlwaysRun : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getAlwaysRun())
		{
			gAgent.clearAlwaysRun();
		}
		else
		{
			gAgent.setAlwaysRun();
		}
		LLMessageSystem *msg = gMessageSystem;


		msg->newMessageFast(_PREHASH_SetAlwaysRun);
		msg->nextBlockFast(_PREHASH_AgentData);
		msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		msg->addBOOLFast(_PREHASH_AlwaysRun, gAgent.getAlwaysRun() );
		gAgent.sendReliableMessage();
		return true;
	}
};

class LLWorldCheckAlwaysRun : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.getAlwaysRun();
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldSetAway : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getAFK())
		{
			gAgent.clearAFK();
		}
		else
		{
			gAgent.setAFK();
		}
		return true;
	}
};

class LLWorldSetBusy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getBusy())
		{
			gAgent.clearBusy();
		}
		else
		{
			gAgent.setBusy();
			gViewerWindow->alertXml("BusyModeSet");
		}
		return true;
	}
};


class LLWorldCreateLandmark : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerRegion* agent_region = gAgent.getRegion();
		if(!agent_region)
		{
			llwarns << "No agent region" << llendl;
			return true;
		}
		LLParcel* agent_parcel = gParcelMgr->getAgentParcel();
		if (!agent_parcel)
		{
			llwarns << "No agent parcel" << llendl;
			return true;
		}
		if (!agent_parcel->getAllowLandmark()
			&& !LLViewerParcelMgr::isParcelOwnedByAgent(agent_parcel, GP_LAND_ALLOW_LANDMARK))
		{
			gViewerWindow->alertXml("CannotCreateLandmarkNotOwner");
			return true;
		}

		LLUUID folder_id;
		folder_id = gInventory.findCategoryUUIDForType(LLAssetType::AT_LANDMARK);
		std::string pos_string;
		gAgent.buildLocationString(pos_string);
		
		create_inventory_item(gAgent.getID(), gAgent.getSessionID(),
							  folder_id, LLTransactionID::tnull,
							  pos_string, pos_string, // name, desc
							  LLAssetType::AT_LANDMARK,
							  LLInventoryType::IT_LANDMARK,
							  NOT_WEARABLE, PERM_ALL, 
							  NULL);
		return true;
	}
};

class LLToolsLookAtSelection : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		const F32 PADDING_FACTOR = 2.f;
		BOOL zoom = (userdata.asString() == "zoom");
		if (!gSelectMgr->getSelection()->isEmpty())
		{
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);

			LLBBox selection_bbox = gSelectMgr->getBBoxOfSelection();
			F32 angle_of_view = llmax(0.1f, gCamera->getAspect() > 1.f ? gCamera->getView() * gCamera->getAspect() : gCamera->getView());
			F32 distance = selection_bbox.getExtentLocal().magVec() * PADDING_FACTOR / atan(angle_of_view);

			LLVector3 obj_to_cam = gCamera->getOrigin() - selection_bbox.getCenterAgent();
			obj_to_cam.normVec();

			if (zoom)
			{
				gAgent.setCameraPosAndFocusGlobal(gSelectMgr->getSelectionCenterGlobal() + LLVector3d(obj_to_cam * distance), gSelectMgr->getSelectionCenterGlobal(), gSelectMgr->getSelection()->getFirstObject()->mID );
			}
			else
			{
				gAgent.setFocusGlobal( gSelectMgr->getSelectionCenterGlobal(), gSelectMgr->getSelection()->getFirstObject()->mID );
			}
		}
		return true;
	}
};

class LLAvatarAddFriend : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( gViewerWindow->lastObjectHit() );
		if(avatar && !is_agent_friend(avatar->getID()))
		{
			request_friendship(avatar->getID());
		}
		return true;
	}
};

void complete_give_money(S32 option, void* user_data)
{
	if (option == 0)
	{
		gAgent.clearBusy();
	}

	LLObjectSelectionHandle handle(*(LLObjectSelectionHandle*)user_data);
	delete (LLObjectSelectionHandle*)user_data;

	LLViewerObject* objectp = handle->getPrimaryObject();

	// Show avatar's name if paying attachment
	if (objectp && objectp->isAttachment())
	{
		while (objectp && !objectp->isAvatar())
		{
			objectp = (LLViewerObject*)objectp->getParent();
		}
	}

	if (objectp)
	{
		if (objectp->isAvatar())
		{
			const BOOL is_group = FALSE;
			LLFloaterPay::payDirectly(&give_money,
									  objectp->getID(),
									  is_group);
		}
		else
		{
			LLFloaterPay::payViaObject(&give_money, objectp->getID());
		}
	}
}

bool handle_give_money_dialog()
{
	LLObjectSelectionHandle* handlep = new LLObjectSelectionHandle(gSelectMgr->getSelection());
	if (gAgent.getBusy())
	{
		// warn users of being in busy mode during a transaction
		gViewerWindow->alertXml("BusyModePay", complete_give_money, handlep);
	}
	else
	{
		complete_give_money(1, handlep);
	}
	return true;
}

class LLPayObject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return handle_give_money_dialog();
	}
};

class LLEnablePayObject : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object(gViewerWindow->lastObjectHit());
		bool new_value = (avatar != NULL);
		if (!new_value)
		{
			LLViewerObject* object = gViewerWindow->lastObjectHit();
			if( object )
			{
				LLViewerObject *parent = (LLViewerObject *)object->getParent();
				if((object->flagTakesMoney()) || (parent && parent->flagTakesMoney()))
				{
					new_value = true;
				}
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLObjectEnableSitOrStand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = false;
		LLViewerObject* dest_object = NULL;
		if((dest_object = gObjectList.findObject(gLastHitObjectID)))
		{
			if(dest_object->getPCode() == LL_PCODE_VOLUME)
			{
				new_value = true;
			}
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);

		// Update label
		LLString label;
		LLString sit_text;
		LLString stand_text;
		LLString param = userdata["data"].asString();
		LLString::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			sit_text = param.substr(0, offset);
			stand_text = param.substr(offset+1);
		}
		if (sitting_on_selection())
		{
			label = stand_text;
		}
		else
		{
			LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
			if (node && node->mValid && !node->mSitName.empty())
			{
				label.assign(node->mSitName);
			}
			else
			{
				label = sit_text;
			}
		}
		gMenuHolder->childSetText("Object Sit", label);

		return true;
	}
};

void edit_ui(void*)
{
	LLFloater::setEditModeEnabled(!LLFloater::getEditModeEnabled());
}

void dump_select_mgr(void*)
{
	gSelectMgr->dump();
}

void dump_volume_mgr(void*)
{
	gVolumeMgr->dump();
}

void dump_inventory(void*)
{
	gInventory.dumpInventory();
}

// forcibly unlock an object
void handle_force_unlock(void*)
{
	// First, make it public.
	gSelectMgr->sendOwner(LLUUID::null, LLUUID::null, TRUE);

	// Second, lie to the viewer and mark it editable and unowned
	LLViewerObject* object;
	for (object = gSelectMgr->getSelection()->getFirstObject(); object; object = gSelectMgr->getSelection()->getNextObject() )
	{
		object->mFlags |= FLAGS_OBJECT_MOVE;
		object->mFlags |= FLAGS_OBJECT_MODIFY;
		object->mFlags |= FLAGS_OBJECT_COPY;

		object->mFlags &= ~FLAGS_OBJECT_ANY_OWNER;
		object->mFlags &= ~FLAGS_OBJECT_YOU_OWNER;
	}
}

// Fullscreen debug stuff
void handle_fullscreen_debug(void*)
{
	llinfos << "Width " << gViewerWindow->getWindowWidth() << " Height " << gViewerWindow->getWindowHeight() << llendl;
	llinfos << "mouse_x_from_center(100) " << mouse_x_from_center(100) << " y " << mouse_y_from_center(100) << llendl;
}

void handle_crash(void*)
{
	llerrs << "This is an llerror" << llendl;
}

class LLWorldForceSun : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString tod = userdata.asString();
		LLVector3 sun_direction;
		if (tod == "sunrise")
		{
			sun_direction.setVec(1.0f, 0.f, 0.2f);
		}
		else if (tod == "noon")
		{
			sun_direction.setVec(0.0f, 0.3f, 1.0f);
		}
		else if (tod == "sunset")
		{
			sun_direction.setVec(-1.0f, 0.f, 0.2f);
		}
		else if (tod == "midnight")
		{
			sun_direction.setVec(0.0f, 0.3f, -1.0f);
		}
		else
		{
			gSky.setOverrideSun(FALSE);
			return true;
		}
		sun_direction.normVec();
		gSky.setOverrideSun(TRUE);
		gSky.setSunDirection( sun_direction, LLVector3(0.f, 0.f, 0.f));
		return true;
	}
};

void handle_dump_followcam(void*)
{
	LLFollowCamMgr::dump();
}

BOOL check_flycam(void*)
{
	return LLViewerJoystick::sOverrideCamera;
}

void handle_toggle_flycam(void*)
{
	LLViewerJoystick::sOverrideCamera = !LLViewerJoystick::sOverrideCamera;
	if (LLViewerJoystick::sOverrideCamera)
	{
		LLViewerJoystick::updateCamera(TRUE);
		LLFloaterJoystick::show(NULL);
	}
}

void handle_viewer_enable_message_log(void*)
{
	gMessageSystem->startLogging();
}

void handle_viewer_disable_message_log(void*)
{
	gMessageSystem->stopLogging();
}

// TomY TODO: Move!
class LLShowFloater : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString floater_name = userdata.asString();
		if (floater_name == "gestures")
		{
			LLFloaterGesture::toggleVisibility();
		}
		else if (floater_name == "appearance")
		{
			if (gAgent.getWearablesLoaded())
			{
				gAgent.changeCameraToCustomizeAvatar();
			}
		}
		else if (floater_name == "friends")
		{
			LLFloaterFriends::toggle(NULL);
		}
		else if (floater_name == "preferences")
		{
			LLFloaterPreference::show(NULL);
		}
		else if (floater_name == "toolbar")
		{
			LLToolBar::toggle(NULL);
		}
		else if (floater_name == "chat history")
		{
			LLFloaterChat::toggle(NULL);
		}
		else if (floater_name == "im")
		{
			LLToolBar::onClickIM(NULL);
		}
		else if (floater_name == "inventory")
		{
			LLInventoryView::toggleVisibility(NULL);
		}
		else if (floater_name == "mute list")
		{
			LLFloaterMute::toggle(NULL);
		}
		else if (floater_name == "camera controls")
		{
			LLFloaterCamera::toggle(NULL);
		}
		else if (floater_name == "movement controls")
		{
			LLFloaterMove::show(NULL);
		}
		else if (floater_name == "world map")
		{
			LLFloaterWorldMap::toggle(NULL);
		}
		else if (floater_name == "mini map")
		{
			LLFloaterMap::toggle(NULL);
		}
		else if (floater_name == "stat bar")
		{
			gDebugView->mStatViewp->setVisible(!gDebugView->mStatViewp->getVisible());
		}
		else if (floater_name == "my land")
		{
			LLFloaterLandHoldings::show(NULL);
		}
		else if (floater_name == "about land")
		{
			if (gParcelMgr->selectionEmpty())
			{
				gParcelMgr->selectParcelAt(gAgent.getPositionGlobal());
			}

			LLFloaterLand::show();
		}
		else if (floater_name == "buy land")
		{
			if (gParcelMgr->selectionEmpty())
			{
				gParcelMgr->selectParcelAt(gAgent.getPositionGlobal());
			}
			
			gParcelMgr->startBuyLand();
		}
		else if (floater_name == "about region")
		{
			LLFloaterRegionInfo::show((void *)NULL);
		}
		else if (floater_name == "grid options")
		{
			LLFloaterBuildOptions::show(NULL);
		}
		else if (floater_name == "script errors")
		{
			LLFloaterScriptDebug::show(LLUUID::null);
		}
		else if (floater_name == "help f1")
		{
#if LL_LIBXUL_ENABLED
			gViewerHtmlHelp.show();
#endif
		}
		else if (floater_name == "help in-world")
		{
#if LL_LIBXUL_ENABLED
			LLFloaterHtml::getInstance()->show( "in-world_help" );
#endif
		}
		else if (floater_name == "help additional")
		{
#if LL_LIBXUL_ENABLED
			LLFloaterHtml::getInstance()->show( "additional_help" );
#endif
		}
		else if (floater_name == "complaint reporter")
		{
			// Prevent menu from appearing in screen shot.
			gMenuHolder->hideMenus();
			LLFloaterReporter::showFromMenu(COMPLAINT_REPORT);
		}
		else if (floater_name == "mean events")
		{
			if (!gNoRender)
			{
				LLFloaterBump::show(NULL);
			}
		}
		else if (floater_name == "bug reporter")
		{
			// Prevent menu from appearing in screen shot.
			gMenuHolder->hideMenus();
			LLFloaterReporter::showFromMenu(BUG_REPORT);
		}
		else if (floater_name == "buy currency")
		{
			LLFloaterBuyCurrency::buyCurrency();
		}
		else if (floater_name == "about")
		{
			LLFloaterAbout::show(NULL);
		}
		return true;
	}
};

class LLFloaterVisible : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString control_name = userdata["control"].asString();
		LLString floater_name = userdata["data"].asString();
		bool new_value = false;
		if (floater_name == "friends")
		{
			new_value = LLFloaterFriends::visible(NULL);
		}
		else if (floater_name == "toolbar")
		{
			new_value = LLToolBar::visible(NULL);
		}
		else if (floater_name == "chat history")
		{
			new_value = LLFloaterChat::visible(NULL);
		}
		else if (floater_name == "im")
		{
			new_value = gIMView && gIMView->mTalkFloater && gIMView->mTalkFloater->getVisible();
		}
		else if (floater_name == "mute list")
		{
			new_value = LLFloaterMute::visible(NULL);
		}
		else if (floater_name == "camera controls")
		{
			new_value = LLFloaterCamera::visible(NULL);
		}
		else if (floater_name == "movement controls")
		{
			new_value = LLFloaterMove::visible(NULL);
		}
		else if (floater_name == "stat bar")
		{
			new_value = gDebugView->mStatViewp->getVisible();
		}
		gMenuHolder->findControl(control_name)->setValue(new_value);
		return true;
	}
};

void callback_show_url(S32 option, void* url)
{
	const char* urlp = (const char*)url;
	if (0 == option)
	{
		LLWeb::loadURL(urlp);
	}
	delete urlp;
}

class LLPromptShowURL : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString param = userdata.asString();
		LLString::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			LLString alert = param.substr(0, offset);
			LLString url = param.substr(offset+1);
			char *url_copy = new char[url.size()+1];
			if (url_copy != NULL)
			{
				strcpy(url_copy, url.c_str());		/* Flawfinder: ignore */
			}
			else
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return false;
			}
			gViewerWindow->alertXml(alert, callback_show_url, url_copy);
		}
		else
		{
			llinfos << "PromptShowURL invalid parameters! Expecting \"ALERT,URL\"." << llendl;
		}
		return true;
	}
};

void callback_show_file(S32 option, void* filename)
{
	const char* filenamep = (const char*)filename;
	if (0 == option)
	{
		load_url_local_file(filenamep);
	}
	delete filenamep;
}

class LLPromptShowFile : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString param = userdata.asString();
		LLString::size_type offset = param.find(",");
		if (offset != param.npos)
		{
			LLString alert = param.substr(0, offset);
			LLString file = param.substr(offset+1);
			char *file_copy = new char[file.size()+1];
			if (file_copy != NULL)
			{
				strcpy(file_copy, file.c_str());		/* Flawfinder: ignore */
			}
			else
			{
				llerrs << "Memory Allocation Failed" << llendl;
				return false;
			}
			gViewerWindow->alertXml(alert, callback_show_file, file_copy);
		}
		else
		{
			llinfos << "PromptShowFile invalid parameters! Expecting \"ALERT,FILE\"." << llendl;
		}
		return true;
	}
};

class LLShowAgentProfile : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLUUID agent_id;
		if (userdata.asString() == "agent")
		{
			agent_id = gAgent.getID();
		}
		else if (userdata.asString() == "hit object")
		{
			agent_id = gLastHitObjectID;
		}
		else
		{
			agent_id = userdata.asUUID();
		}

		LLVOAvatar* avatar = find_avatar_from_object(agent_id);
		if (avatar)
		{
			LLFloaterAvatarInfo::showFromAvatar(avatar);
		}
		return true;
	}
};

class LLShowAgentGroups : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLUUID agent_id;
		if (userdata.asString() == "agent")
		{
			agent_id = gAgent.getID();
		}
		else
		{
			agent_id = userdata.asUUID();
		}
		if(agent_id.notNull())
		{
			LLFloaterGroups::show(agent_id, LLFloaterGroups::AGENT_GROUPS);
		}
		return true;
	}
};

void handle_focus(void *)
{
	if (gDisconnected)
	{
		return;
	}

	if (gAgent.getFocusOnAvatar())
	{
		// zoom in if we're looking at the avatar
		gAgent.setFocusOnAvatar(FALSE, ANIMATE);
		gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
		gAgent.cameraZoomIn(0.666f);
	}
	else
	{
		gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
	}

	gViewerWindow->moveCursorToCenter();

	// Switch to camera toolset
//	gToolMgr->setCurrentToolset(gCameraToolset);
	gToolMgr->getCurrentToolset()->selectTool( gToolCamera );
}

class LLLandEdit : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		if (gAgent.getFocusOnAvatar() && gSavedSettings.getBOOL("EditCameraMovement") )
		{
			// zoom in if we're looking at the avatar
			gAgent.setFocusOnAvatar(FALSE, ANIMATE);
			gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);

			gAgent.cameraOrbitOver( F_PI * 0.25f );
			gViewerWindow->moveCursorToCenter();
		}
		else if ( gSavedSettings.getBOOL("EditCameraMovement") )
		{
			gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
			gViewerWindow->moveCursorToCenter();
		}


		gParcelMgr->selectParcelAt( gLastHitPosGlobal );

		gFloaterTools->showMore(TRUE);
		gFloaterView->bringToFront( gFloaterTools );

		// Switch to land edit toolset
		gToolMgr->getCurrentToolset()->selectTool( gToolParcel );
		return true;
	}
};

class LLWorldEnableBuyLand : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gParcelMgr->canAgentBuyParcel(
								gParcelMgr->selectionEmpty()
									? gParcelMgr->getAgentParcel()
									: gParcelMgr->getParcelSelection()->getParcel(),
								false);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_buy_land(void*)
{
	return gParcelMgr->canAgentBuyParcel(
				gParcelMgr->getParcelSelection()->getParcel(), false);
}


void handle_move(void*)
{
	if (gAgent.getFocusOnAvatar())
	{
		// zoom in if we're looking at the avatar
		gAgent.setFocusOnAvatar(FALSE, ANIMATE);
		gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);

		gAgent.cameraZoomIn(0.666f);
	}
	else
	{
		gAgent.setFocusGlobal(gLastHitPosGlobal + gLastHitObjectOffset, gLastHitObjectID);
	}

	gViewerWindow->moveCursorToCenter();

	gToolMgr->setCurrentToolset(gBasicToolset);
	gToolMgr->getCurrentToolset()->selectTool( gToolGrab );
}

class LLObjectAttachToAvatar : public view_listener_t
{
public:
	static void setObjectSelection(LLObjectSelectionHandle selection) { sObjectSelection = selection; }

private:
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		setObjectSelection(gSelectMgr->getSelection());
		LLViewerObject* selectedObject = sObjectSelection->getFirstRootObject();
		if (selectedObject)
		{
			S32 index = userdata.asInteger();
			LLViewerJointAttachment* attachment_point = index > 0 ?
				gAgent.getAvatarObject()->mAttachmentPoints[index] :
				NULL;
			confirm_replace_attachment(0, attachment_point);
		}
		return true;
	}

protected:
	static LLObjectSelectionHandle sObjectSelection;
};

LLObjectSelectionHandle LLObjectAttachToAvatar::sObjectSelection;

void near_attach_object(BOOL success, void *user_data)
{
	if (success)
	{
		LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;
		
		U8 attachment_id;
		if (attachment)
		{
			attachment_id = gAgent.getAvatarObject()->mAttachmentPoints.reverseLookup(attachment);
		}
		else
		{
			// interpret 0 as "default location"
			attachment_id = 0;
		}
		gSelectMgr->sendAttach(attachment_id);
	}		
	LLObjectAttachToAvatar::setObjectSelection(NULL);
}

void confirm_replace_attachment(S32 option, void* user_data)
{
	if (option == 0/*YES*/)
	{
		LLViewerObject* selectedObject = gSelectMgr->getSelection()->getFirstRootObject();
		if (selectedObject)
		{
			const F32 MIN_STOP_DISTANCE = 1.f;	// meters
			const F32 ARM_LENGTH = 0.5f;		// meters
			const F32 SCALE_FUDGE = 1.5f;

			F32 stop_distance = SCALE_FUDGE * selectedObject->getMaxScale() + ARM_LENGTH;
			if (stop_distance < MIN_STOP_DISTANCE)
			{
				stop_distance = MIN_STOP_DISTANCE;
			}

			LLVector3 walkToSpot = selectedObject->getPositionAgent();
			
			// make sure we stop in front of the object
			LLVector3 delta = walkToSpot - gAgent.getPositionAgent();
			delta.normVec();
			delta = delta * 0.5f;
			walkToSpot -= delta;

			gAgent.startAutoPilotGlobal(gAgent.getPosGlobalFromAgent(walkToSpot), "Attach", NULL, near_attach_object, user_data, stop_distance);
			gAgent.clearFocusObject();
		}
	}
}

class LLAttachmentDrop : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Called when the user clicked on an object attached to them
		// and selected "Drop".
		LLViewerObject *object = gViewerWindow->lastObjectHit();
		if (!object)
		{
			llwarns << "handle_drop_attachment() - no object to drop" << llendl;
			return true;
		}

		LLViewerObject *parent = (LLViewerObject*)object->getParent();
		while (parent)
		{
			if(parent->isAvatar())
			{
				break;
			}
			object = parent;
			parent = (LLViewerObject*)parent->getParent();
		}

		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		if (object->isAvatar())
		{
			llwarns << "Trying to detach avatar from avatar." << llendl;
			return true;
		}

		// The sendDropAttachment() method works on the list of selected
		// objects.  Thus we need to clear the list, make sure it only
		// contains the object the user clicked, send the message,
		// then clear the list.
		gSelectMgr->sendDropAttachment();
		return true;
	}
};

// called from avatar pie menu
void handle_detach_from_avatar(void* user_data)
{
	LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;
	
	LLViewerObject* attached_object = attachment->getObject();

	if (attached_object)
	{
		gMessageSystem->newMessage("ObjectDetach");
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID() );
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

		gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
		gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, attached_object->getLocalID());
		gMessageSystem->sendReliable( gAgent.getRegionHost() );
	}
}

void attach_label(LLString& label, void* user_data)
{
	LLViewerJointAttachment* attachmentp = (LLViewerJointAttachment*)user_data;
	if (attachmentp)
	{
		label = attachmentp->getName();
		if (attachmentp->getObject())
		{
			LLViewerInventoryItem* itemp = gInventory.getItem(attachmentp->getItemID());
			if (itemp)
			{
				label += LLString(" (") + itemp->getName() + LLString(")");
			}
		}
	}
}

void detach_label(LLString& label, void* user_data)
{
	LLViewerJointAttachment* attachmentp = (LLViewerJointAttachment*)user_data;
	if (attachmentp)
	{
		label = attachmentp->getName();
		if (attachmentp->getObject())
		{
			LLViewerInventoryItem* itemp = gInventory.getItem(attachmentp->getItemID());
			if (itemp)
			{
				label += LLString(" (") + itemp->getName() + LLString(")");
			}
		}
	}
}


class LLAttachmentDetach : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// Called when the user clicked on an object attached to them
		// and selected "Detach".
		LLViewerObject *object = gViewerWindow->lastObjectHit();
		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		LLViewerObject *parent = (LLViewerObject*)object->getParent();
		while (parent)
		{
			if(parent->isAvatar())
			{
				break;
			}
			object = parent;
			parent = (LLViewerObject*)parent->getParent();
		}

		if (!object)
		{
			llwarns << "handle_detach() - no object to detach" << llendl;
			return true;
		}

		if (object->isAvatar())
		{
			llwarns << "Trying to detach avatar from avatar." << llendl;
			return true;
		}

		// The sendDetach() method works on the list of selected
		// objects.  Thus we need to clear the list, make sure it only
		// contains the object the user clicked, send the message,
		// then clear the list.
		// We use deselectAll to update the simulator's notion of what's
		// selected, and removeAll just to change things locally.
		//RN: I thought it was more useful to detach everything that was selected
		if (gSelectMgr->getSelection()->isAttachment())
		{
			gSelectMgr->sendDetach();
		}
		return true;
	}
};

//Adding an observer for a Jira 2422 and needs to be a fetch observer
//for Jira 3119
class LLWornItemFetchedObserver : public LLInventoryFetchObserver
{
public:
	LLWornItemFetchedObserver() {}
	virtual ~LLWornItemFetchedObserver() {}

protected:
	virtual void done()
	{
		gPieAttachment->buildDrawLabels();
		gInventory.removeObserver(this);
		delete this;
	}
};

// You can only drop items on parcels where you can build.
class LLAttachmentEnableDrop : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLParcel* parcel = gParcelMgr->getAgentParcel();
		BOOL can_build   = gAgent.isGodlike() || (parcel && parcel->getAllowModify());

		//Add an inventory observer to only allow dropping the newly attached item
		//once it exists in your inventory.  Look at Jira 2422.
		//-jwolk

		// A bug occurs when you wear/drop an item before it actively is added to your inventory
		// if this is the case (you're on a slow sim, etc.) a copy of the object,
		// well, a newly created object with the same properties, is placed
		// in your inventory.  Therefore, we disable the drop option until the
		// item is in your inventory

		LLViewerObject*              object         = gViewerWindow->lastObjectHit();
		LLViewerJointAttachment*     attachment_pt  = NULL;
		LLInventoryItem*             item           = NULL;

		if ( object )
		{
    		S32 attachmentID  = ATTACHMENT_ID_FROM_STATE(object->getState());
  				attachment_pt = gAgent.getAvatarObject()->mAttachmentPoints.getIfThere(attachmentID);

			if ( attachment_pt )
			{
				// make sure item is in your inventory (it could be a delayed attach message being sent from the sim)
				// so check to see if the item is in the inventory already
				item = gInventory.getItem(attachment_pt->getItemID());
				
				if ( !item )
				{
					// Item does not exist, make an observer to enable the pie menu 
					// when the item finishes fetching worst case scenario 
					// if a fetch is already out there (being sent from a slow sim)
					// we refetch and there are 2 fetches
					LLWornItemFetchedObserver* wornItemFetched = new LLWornItemFetchedObserver();
					LLInventoryFetchObserver::item_ref_t items; //add item to the inventory item to be fetched

					items.push_back(attachment_pt->getItemID());
				
					wornItemFetched->fetchItems(items);
					gInventory.addObserver(wornItemFetched);
				}
			}
		}
		
		//now check to make sure that the item is actually in the inventory before we enable dropping it
		bool new_value = enable_detach(NULL) && can_build && item;

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_detach(void*)
{
	LLViewerObject* object = gViewerWindow->lastObjectHit();
	if (!object) return FALSE;
	if (!object->isAttachment()) return FALSE;

	// Find the avatar who owns this attachment
	LLViewerObject* avatar = object;
	while (avatar)
	{
		// ...if it's you, good to detach
		if (avatar->getID() == gAgent.getID())
		{
			return TRUE;
		}

		avatar = (LLViewerObject*)avatar->getParent();
	}

	return FALSE;
}

class LLAttachmentEnableDetach : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_detach(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// Used to tell if the selected object can be attached to your avatar.
BOOL object_selected_and_point_valid(void *user_data)
{
	//LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;
	if (gSelectMgr == NULL) return FALSE;

	LLObjectSelectionHandle selection = gSelectMgr->getSelection();
	for (LLViewerObject *object = selection->getFirstRootObject(); object; object = selection->getNextRootObject())
	{
		for (U32 child_num = 0; child_num < object->mChildList.size(); child_num++ )
		{
			if (object->mChildList[child_num]->isAvatar())
			{
				return FALSE;
			}
		}
	}

	return (selection->getRootObjectCount() == 1) && 
		(selection->getFirstRootObject()->getPCode() == LL_PCODE_VOLUME) && 
		selection->getFirstRootObject()->permYouOwner() &&
		!((LLViewerObject*)selection->getFirstRootObject()->getRoot())->isAvatar() && 
		(selection->getFirstRootObject()->getNVPair("AssetContainer") == NULL);
}

// Also for seeing if object can be attached.  See above.
class LLObjectEnableWear : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		return object_selected_and_point_valid(NULL);
	}
};


BOOL object_attached(void *user_data)
{
	LLViewerJointAttachment *attachment = (LLViewerJointAttachment *)user_data;

	return attachment->getObject() != NULL;
}

class LLAvatarSendIM : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLVOAvatar* avatar = find_avatar_from_object( gViewerWindow->lastObjectHit() );
		if(avatar)
		{
			LLString name("IM");
			LLNameValue *first = avatar->getNVPair("FirstName");
			LLNameValue *last = avatar->getNVPair("LastName");
			if (first && last)
			{
				name.assign( first->getString() );
				name.append(" ");
				name.append( last->getString() );
			}

			gIMView->setFloaterOpen(TRUE);
			//EInstantMessage type = have_agent_callingcard(gLastHitObjectID)
			//	? IM_SESSION_ADD : IM_SESSION_CARDLESS_START;
			gIMView->addSession(name,
								IM_NOTHING_SPECIAL,
								avatar->getID());
		}
		return true;
	}
};


void handle_activate(void*)
{
}

BOOL enable_activate(void*)
{
	return FALSE;
}

class LLToolsSelectedScriptAction : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString action = userdata.asString();
		LLFloaterScriptQueue *queue = NULL;
		if (action == "compile")
		{
			queue = LLFloaterCompileQueue::create();
		}
		else if (action == "reset")
		{
			queue = LLFloaterResetQueue::create();
		}
		else if (action == "start")
		{
			queue = LLFloaterRunQueue::create();
		}
		else if (action == "stop")
		{
			queue = LLFloaterNotRunQueue::create();
		}
		if (!queue) return true;

		BOOL scripted = FALSE;
		BOOL modifiable = FALSE;

		for(LLViewerObject* obj = gSelectMgr->getSelection()->getFirstObject();
			obj;
			obj = gSelectMgr->getSelection()->getNextObject())
		{
			scripted = obj->flagScripted();
			modifiable = obj->permModify();

			if( scripted &&  modifiable )
				queue->addObject(obj->getID());
			else
				break;
		}

		if(!queue->start())
		{
			if ( ! scripted )
			{
				gViewerWindow->alertXml("CannotRecompileSelectObjectsNoScripts");
			}
			else if ( ! modifiable )
			{
				gViewerWindow->alertXml("CannotRecompileSelectObjectsNoPermission");
			}
		}
		return true;
	}
};

void handle_reset_selection(void*)
{
	LLFloaterResetQueue* queue = LLFloaterResetQueue::create();

	BOOL scripted = FALSE;
	BOOL modifiable = FALSE;

	for(LLViewerObject* obj = gSelectMgr->getSelection()->getFirstObject();
		obj;
		obj = gSelectMgr->getSelection()->getNextObject())
	{
		scripted = obj->flagScripted();
		modifiable = obj->permModify();

		if( scripted &&  modifiable )
			queue->addObject(obj->getID());
		else
			break;
	}

	if(!queue->start())
	{
		if ( ! scripted )
		{
			gViewerWindow->alertXml("CannotResetSelectObjectsNoScripts");
		}
		else if ( ! modifiable )
		{
			gViewerWindow->alertXml("CannotResetSelectObjectsNoPermission");
		}
	}
}

void handle_set_run_selection(void*)
{
	LLFloaterRunQueue* queue = LLFloaterRunQueue::create();

	BOOL scripted = FALSE;
	BOOL modifiable = FALSE;

	for(LLViewerObject* obj = gSelectMgr->getSelection()->getFirstObject();
		obj;
		obj = gSelectMgr->getSelection()->getNextObject())
	{
		scripted = obj->flagScripted();
		modifiable = obj->permModify();

		if( scripted &&  modifiable )
			queue->addObject(obj->getID());
		else
			break;
	}

	if(!queue->start())
	{
		if ( ! scripted )
		{
			gViewerWindow->alertXml("CannotSetRunningSelectObjectsNoScripts");
		}
		else if ( ! modifiable )
		{
			gViewerWindow->alertXml("CannotSerRunningSelectObjectsNoPermission");
		}
	}
}

void handle_set_not_run_selection(void*)
{
	LLFloaterNotRunQueue* queue = LLFloaterNotRunQueue::create();

	BOOL scripted = FALSE;
	BOOL modifiable = FALSE;

	for(LLViewerObject* obj = gSelectMgr->getSelection()->getFirstObject();
		obj;
		obj = gSelectMgr->getSelection()->getNextObject())
	{
		scripted = obj->flagScripted();
		modifiable = obj->permModify();

		if( scripted &&  modifiable )
			queue->addObject(obj->getID());
		else
			break;
	}

	if(!queue->start())
	{
		if ( ! scripted )
		{
			gViewerWindow->alertXml("CannotSetRunningNotSelectObjectsNoScripts");
		}
		else if ( ! modifiable )
		{
			gViewerWindow->alertXml("CannotSerRunningNotSelectObjectsNoPermission");
		}
	}
}

void handle_selected_texture_info(void*)
{
	LLSelectNode* node = NULL;
	for (node = gSelectMgr->getSelection()->getFirstNode(); node != NULL; node = gSelectMgr->getSelection()->getNextNode())
	{
		if (!node->mValid) continue;

		std::string msg;
		msg.assign("Texture info for: ");
		msg.append(node->mName);
		LLChat chat(msg);
		LLFloaterChat::addChat(chat);

		U8 te_count = node->getObject()->getNumTEs();
		// map from texture ID to list of faces using it
		typedef std::map< LLUUID, std::vector<U8> > map_t;
		map_t faces_per_texture;
		for (U8 i = 0; i < te_count; i++)
		{
			if (!node->isTESelected(i)) continue;

			LLViewerImage* img = node->getObject()->getTEImage(i);
			LLUUID image_id = img->getID();
			faces_per_texture[image_id].push_back(i);
		}
		// Per-texture, dump which faces are using it.
		map_t::iterator it;
		for (it = faces_per_texture.begin(); it != faces_per_texture.end(); ++it)
		{
			LLUUID image_id = it->first;
			U8 te = it->second[0];
			LLViewerImage* img = node->getObject()->getTEImage(te);
			S32 height = img->getHeight();
			S32 width = img->getWidth();
			S32 components = img->getComponents();
			std::string image_id_string;
			if (gAgent.isGodlike())
			{
				image_id_string = image_id.asString() + " ";
			}
			msg = llformat("%s%dx%d %s on face ",
								image_id_string.c_str(),
								width,
								height,
								(components == 4 ? "alpha" : "opaque"));
			for (U8 i = 0; i < it->second.size(); ++i)
			{
				msg.append( llformat("%d ", (S32)(it->second[i])));
			}
			LLChat chat(msg);
			LLFloaterChat::addChat(chat);
		}
	}
}

void handle_dump_image_list(void*)
{
	gImageList.dump();
}

void handle_test_male(void*)
{
	wear_outfit_by_name("Male Shape & Outfit");
	//gGestureList.requestResetFromServer( TRUE );
}

void handle_test_female(void*)
{
	wear_outfit_by_name("Female Shape & Outfit");
	//gGestureList.requestResetFromServer( FALSE );
}

void handle_toggle_pg(void*)
{
	if (gAgent.mAccess < SIM_ACCESS_MATURE)
	{
		gAgent.mAccess = SIM_ACCESS_MATURE;
	}
	else
	{
		gAgent.mAccess = SIM_ACCESS_PG;
	}

	LLFloaterWorldMap::reloadIcons(NULL);

	llinfos << "Access set to " << (S32)gAgent.mAccess << llendl;
}

void handle_dump_attachments(void*)
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( !avatar )
	{
		llinfos << "NO AVATAR" << llendl;
		return;
	}

	for( LLViewerJointAttachment* attachment = avatar->mAttachmentPoints.getFirstData();
		attachment;
		attachment = avatar->mAttachmentPoints.getNextData() )
	{
		S32 key = avatar->mAttachmentPoints.getCurrentKeyWithoutIncrement();
		BOOL visible = (attachment->getObject() != NULL &&
						attachment->getObject()->mDrawable.notNull() && 
						!attachment->getObject()->mDrawable->isRenderType(0));
		LLVector3 pos;
		if (visible) pos = attachment->getObject()->mDrawable->getPosition();
		llinfos << "ATTACHMENT " << key << ": item_id=" << attachment->getItemID()
				<< (attachment->getObject() ? " present " : " absent ")
				<< (visible ? "visible " : "invisible ")
				<<  " at " << pos
				<< " and " << (visible ? attachment->getObject()->getPosition() : LLVector3::zero)
				<< llendl;
	}
}

//---------------------------------------------------------------------
// Callbacks for enabling/disabling items
//---------------------------------------------------------------------

BOOL menu_ui_enabled(void *user_data)
{
	BOOL high_res = gSavedSettings.getBOOL( "HighResSnapshot" );
	return !high_res;
}

// TomY TODO DEPRECATE & REMOVE
void menu_toggle_control( void* user_data )
{
        BOOL checked = gSavedSettings.getBOOL( static_cast<char*>(user_data) );
        if (LLString(static_cast<char*>(user_data)) == "HighResSnapshot" && !checked)
        {
                // High Res Snapshot active, must uncheck RenderUIInSnapshot
                gSavedSettings.setBOOL( "RenderUIInSnapshot", FALSE );
        }
        gSavedSettings.setBOOL( static_cast<char*>(user_data), !checked );
}


// these are used in the gl menus to set control values.
class LLToggleControl : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString control_name = userdata.asString();
		BOOL checked = gSavedSettings.getBOOL( control_name );
		if (control_name == "HighResSnapshot" && !checked)
		{
			// High Res Snapshot active, must uncheck RenderUIInSnapshot
			gSavedSettings.setBOOL( "RenderUIInSnapshot", FALSE );
		}
		gSavedSettings.setBOOL( control_name, !checked );
		return true;
	}
};

// As above, but can be a callback from a LLCheckboxCtrl
void check_toggle_control( LLUICtrl *, void* user_data )
{
	BOOL checked = gSavedSettings.getBOOL( static_cast<char*>(user_data) );
	gSavedSettings.setBOOL( static_cast<char*>(user_data), !checked );
}

BOOL menu_check_control( void* user_data)
{
	return gSavedSettings.getBOOL((char*)user_data);
}

// 
void menu_toggle_variable( void* user_data )
{
	BOOL checked = *(BOOL*)user_data;
	*(BOOL*)user_data = !checked;
}

BOOL menu_check_variable( void* user_data)
{
	return *(BOOL*)user_data;
}


BOOL enable_land_selected( void* )
{
	return gParcelMgr && !(gParcelMgr->selectionEmpty());
}

class LLSomethingSelected : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = !(gSelectMgr->getSelection()->isEmpty());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLSomethingSelectedNoHUD : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLObjectSelectionHandle selection = gSelectMgr->getSelection();
		bool new_value = !(selection->isEmpty()) && !(selection->getSelectType() == SELECT_TYPE_HUD);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_more_than_one_selected(void* )
{
	return (gSelectMgr->getSelection()->getObjectCount() > 1);
}

class LLEditableSelected : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = (gSelectMgr->getSelection()->getFirstEditableObject() != NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsEnableTakeCopy : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = false;
		if (gSelectMgr)
		{
			new_value = true;
#ifndef HACKED_GODLIKE_VIEWER
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
			if (gInProductionGrid || !gAgent.isGodlike())
# endif
			{
				LLObjectSelectionHandle selection = gSelectMgr->getSelection();
				LLViewerObject* obj = selection->getFirstRootObject();
				if(obj)
				{
					for( ; obj; obj = selection->getNextRootObject())
					{
						if(!(obj->permCopy()) || obj->isAttachment())
						{
							new_value = false;
						}
					}
				}
			}
#endif // HACKED_GODLIKE_VIEWER
		}

		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_selection_you_own_all(void*)
{
	LLViewerObject *obj;
	if (gSelectMgr)
	{
		LLObjectSelectionHandle selection = gSelectMgr->getSelection();
		for (obj = selection->getFirstRootObject(); obj; obj = selection->getNextRootObject())
		{
			if (!obj->permYouOwner())
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL enable_selection_you_own_one(void*)
{
	if (gSelectMgr)
	{
		LLObjectSelectionHandle selection = gSelectMgr->getSelection();
		LLViewerObject *obj;
		for (obj = selection->getFirstRootObject(); obj; obj = selection->getNextRootObject())
		{
			if (obj->permYouOwner())
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

class LLHasAsset : public LLInventoryCollectFunctor
{
public:
	LLHasAsset(const LLUUID& id) : mAssetID(id), mHasAsset(FALSE) {}
	virtual ~LLHasAsset() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item);
	BOOL hasAsset() const { return mHasAsset; }

protected:
	LLUUID mAssetID;
	BOOL mHasAsset;
};

bool LLHasAsset::operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
{
	if(item && item->getAssetUUID() == mAssetID)
	{
		mHasAsset = TRUE;
	}
	return FALSE;
}

BOOL enable_save_into_inventory(void*)
{
	if(gSelectMgr)
	{
		// find the last root
		LLSelectNode* last_node = NULL;
		for(LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
			node != NULL;
			node = gSelectMgr->getSelection()->getNextRootNode())
		{
			last_node = node;
		}

#ifdef HACKED_GODLIKE_VIEWER
		return TRUE;
#else
# ifdef TOGGLE_HACKED_GODLIKE_VIEWER
		if (!gInProductionGrid && gAgent.isGodlike())
		{
			return TRUE;
		}
# endif
		// check all pre-req's for save into inventory.
		if(last_node && last_node->mValid && !last_node->mItemID.isNull()
		   && (last_node->mPermissions->getOwner() == gAgent.getID())
		   && (gInventory.getItem(last_node->mItemID) != NULL))
		{
			LLViewerObject* obj = last_node->getObject();
			if( obj && !obj->isAttachment() )
			{
				return TRUE;
			}
		}
#endif
	}
	return FALSE;
}

class LLToolsEnableSaveToInventory : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_save_into_inventory(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_save_into_task_inventory(void*)
{
	if(gSelectMgr)
	{
		LLSelectNode* node = gSelectMgr->getSelection()->getFirstRootNode();
		if(node && (node->mValid) && (!node->mFromTaskID.isNull()))
		{
			// *TODO: check to see if the fromtaskid object exists.
			LLViewerObject* obj = node->getObject();
			if( obj && !obj->isAttachment() )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

class LLToolsEnableSaveToObjectInventory : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = enable_save_into_task_inventory(NULL);
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

BOOL enable_not_thirdperson(void*)
{
	return !gAgent.cameraThirdPerson();
}


BOOL enable_export_selected(void *)
{
	if (gSelectMgr->getSelection()->isEmpty())
	{
		return FALSE;
	}
	if (!gExporterRequestID.isNull())
	{
		return FALSE;
	}
	if (!LLUploadDialog::modalUploadIsFinished())
	{
		return FALSE;
	}
	return TRUE;
}

class LLViewEnableMouselook : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// You can't go directly from customize avatar to mouselook.
		// TODO: write code with appropriate dialogs to handle this transition.
		bool new_value = (CAMERA_MODE_CUSTOMIZE_AVATAR != gAgent.getCameraMode() && !gSavedSettings.getBOOL("FreezeTime"));
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLToolsEnableToolNotPie : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = ( gToolMgr->getBaseTool() != gToolPie );
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldEnableCreateLandmark : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike() || 
			(gAgent.getRegion() && gAgent.getRegion()->getAllowLandmark());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldEnableSetHomeLocation : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = gAgent.isGodlike() || 
			(gAgent.getRegion() && gAgent.getRegion()->getAllowSetHome());
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLWorldEnableTeleportHome : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLViewerRegion* regionp = gAgent.getRegion();
		bool agent_on_prelude = (regionp && regionp->isPrelude());
		bool enable_teleport_home = gAgent.isGodlike() || !agent_on_prelude;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(enable_teleport_home);
		return true;
	}
};

BOOL enable_region_owner(void*)
{
	if(gAgent.getRegion() && gAgent.getRegion()->getOwner() == gAgent.getID())
		return TRUE;
	return enable_god_customer_service(NULL);
}

BOOL enable_god_full(void*)
{
	return gAgent.getGodLevel() >= GOD_FULL;
}

BOOL enable_god_liaison(void*)
{
	return gAgent.getGodLevel() >= GOD_LIAISON;
}

BOOL enable_god_customer_service(void*)
{
	return gAgent.getGodLevel() >= GOD_CUSTOMER_SERVICE;
}

BOOL enable_god_basic(void*)
{
	return gAgent.getGodLevel() > GOD_NOT;
}

#if 0 // 1.9.2
void toggle_vertex_shaders(void *)
{
	BOOL use_shaders = gPipeline.getUseVertexShaders();
	gPipeline.setUseVertexShaders(use_shaders);
}

BOOL check_vertex_shaders(void *)
{
	return gPipeline.getUseVertexShaders();
}
#endif

void toggle_show_xui_names(void *)
{
	BOOL showXUINames = gSavedSettings.getBOOL("ShowXUINames");
	
	showXUINames = !showXUINames;
	gSavedSettings.setBOOL("ShowXUINames", showXUINames);
}

BOOL check_show_xui_names(void *)
{
	return gSavedSettings.getBOOL("ShowXUINames");
}



void toggle_cull_small(void *)
{
//	gPipeline.mCullBySize = !gPipeline.mCullBySize;
//
//	gSavedSettings.setBOOL("RenderCullBySize", gPipeline.mCullBySize);
}

class LLToolsSelectOnlyMyObjects : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		BOOL cur_val = gSavedSettings.getBOOL("SelectOwnedOnly");

		gSavedSettings.setBOOL("SelectOwnedOnly", ! cur_val );

		return true;
	}
};

class LLToolsSelectOnlyMovableObjects : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		BOOL cur_val = gSavedSettings.getBOOL("SelectMovableOnly");

		gSavedSettings.setBOOL("SelectMovableOnly", ! cur_val );

		return true;
	}
};

class LLToolsSelectBySurrounding : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLSelectMgr::sRectSelectInclusive = !LLSelectMgr::sRectSelectInclusive;

		gSavedSettings.setBOOL("RectangleSelectInclusive", LLSelectMgr::sRectSelectInclusive);
		return true;
	}
};

class LLToolsShowHiddenSelection : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// TomY TODO Merge these
		LLSelectMgr::sRenderHiddenSelections = !LLSelectMgr::sRenderHiddenSelections;

		gSavedSettings.setBOOL("RenderHiddenSelections", LLSelectMgr::sRenderHiddenSelections);
		return true;
	}
};

class LLToolsShowSelectionLightRadius : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		// TomY TODO merge these
		LLSelectMgr::sRenderLightRadius = !LLSelectMgr::sRenderLightRadius;

		gSavedSettings.setBOOL("RenderLightRadius", LLSelectMgr::sRenderLightRadius);
		return true;
	}
};

void reload_personal_settings_overrides(void *)
{
	llinfos << "Loading overrides from " << gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,"overrides.xml") << llendl;
	
	gSavedSettings.loadFromFile(gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT,"overrides.xml"));
}

void force_breakpoint(void *)
{
#if LL_WINDOWS // Forcing a breakpoint
	DebugBreak();
#endif
}

void reload_vertex_shader(void *)
{
	//THIS WOULD BE AN AWESOME PLACE TO RELOAD SHADERS... just a thought	- DaveP
}

void slow_mo_animations(void*)
{
	static BOOL slow_mo = FALSE;
	if (slow_mo)
	{
		gAgent.getAvatarObject()->setAnimTimeFactor(1.f);
		slow_mo = FALSE;
	}
	else
	{
		gAgent.getAvatarObject()->setAnimTimeFactor(0.2f);
		slow_mo = TRUE;
	}
}

void handle_dump_avatar_local_textures(void*)
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if( avatar )
	{
		avatar->dumpLocalTextures();
	}
}

void handle_debug_avatar_textures(void*)
{
	LLFloaterAvatarTextures::show(gLastHitObjectID);
}

void handle_grab_texture(void* data)
{
	LLVOAvatar::ETextureIndex index = (LLVOAvatar::ETextureIndex)((intptr_t)data);
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if ( avatar )
	{
		const LLUUID& asset_id = avatar->grabLocalTexture(index);
		llinfos << "Adding baked texture " << asset_id << " to inventory." << llendl;
		LLAssetType::EType asset_type = LLAssetType::AT_TEXTURE;
		LLInventoryType::EType inv_type = LLInventoryType::IT_TEXTURE;
		LLUUID folder_id(gInventory.findCategoryUUIDForType(asset_type));
		if(folder_id.notNull())
		{
			LLString name = "Baked ";
			switch (index)
			{
			case LLVOAvatar::TEX_EYES_BAKED:
				name.append("Iris");
				break;
			case LLVOAvatar::TEX_HEAD_BAKED:
				name.append("Head");
				break;
			case LLVOAvatar::TEX_UPPER_BAKED:
				name.append("Upper Body");
				break;
			case LLVOAvatar::TEX_LOWER_BAKED:
				name.append("Lower Body");
				break;
			case LLVOAvatar::TEX_SKIRT_BAKED:
				name.append("Skirt");
				break;
			default:
				name.append("Unknown");
				break;
			}
			name.append(" Texture");

			LLUUID item_id;
			item_id.generate();
			LLPermissions perm;
			perm.init(gAgentID,
					  gAgentID,
					  LLUUID::null,
					  LLUUID::null);
			U32 next_owner_perm = PERM_MOVE | PERM_TRANSFER;
			perm.initMasks(PERM_ALL,
						   PERM_ALL,
						   PERM_NONE,
						   PERM_NONE,
						   next_owner_perm);
			S32 creation_date_now = time_corrected();
			LLPointer<LLViewerInventoryItem> item
				= new LLViewerInventoryItem(item_id,
											folder_id,
											perm,
											asset_id,
											asset_type,
											inv_type,
											name,
											"",
											LLSaleInfo::DEFAULT,
											LLInventoryItem::II_FLAGS_NONE,
											creation_date_now);

			item->updateServer(TRUE);
			gInventory.updateItem(item);
			gInventory.notifyObservers();

			LLInventoryView* view = LLInventoryView::getActiveInventory();

			// Show the preview panel for textures to let
			// user know that the image is now in inventory.
			if(view)
			{
				LLUICtrl* focus_ctrl = gFocusMgr.getKeyboardFocus();
				LLFocusMgr::FocusLostCallback callback = gFocusMgr.getFocusCallback();

				view->getPanel()->setSelection(item_id, TAKE_FOCUS_NO);
				view->getPanel()->openSelected();
				//LLInventoryView::dumpSelectionInformation((void*)view);
				// restore keyboard focus
				gFocusMgr.setKeyboardFocus(focus_ctrl, callback);
			}
		}
		else
		{
			llwarns << "Can't find a folder to put it in" << llendl;
		}
	}
}

BOOL enable_grab_texture(void* data)
{
	LLVOAvatar::ETextureIndex index = (LLVOAvatar::ETextureIndex)((intptr_t)data);
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if ( avatar )
	{
		return avatar->canGrabLocalTexture(index);
	}
	return FALSE;
}

// Returns a pointer to the avatar give the UUID of the avatar OR of an attachment the avatar is wearing.
// Returns NULL on failure.
LLVOAvatar* find_avatar_from_object( LLViewerObject* object )
{
	if (object)
	{
		if( object->isAttachment() )
		{
			do
			{
				object = (LLViewerObject*) object->getParent();
			}
			while( object && !object->isAvatar() );
		}
		else
		if( !object->isAvatar() )
		{
			object = NULL;
		}
	}

	return (LLVOAvatar*) object;
}


// Returns a pointer to the avatar give the UUID of the avatar OR of an attachment the avatar is wearing.
// Returns NULL on failure.
LLVOAvatar* find_avatar_from_object( const LLUUID& object_id )
{
	return find_avatar_from_object( gObjectList.findObject(object_id) );
}


void handle_disconnect_viewer(void *)
{
	char message[2048];		/* Flawfinder: ignore */
	message[0] = '\0';

	snprintf(message, sizeof(message), "Testing viewer disconnect");		/* Flawfinder: ignore */

	do_disconnect(message);
}


class LLToolsUseSelectionForGrid : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		gSelectMgr->clearGridObjects();
		LLObjectSelectionHandle selection = gSelectMgr->getSelection();
		for (LLViewerObject* objectp = selection->getFirstRootObject();
			objectp;
			objectp = selection->getNextRootObject())
			{
				gSelectMgr->addGridObject(objectp);
			}
		gSelectMgr->setGridMode(GRID_MODE_REF_OBJECT);
		if (gFloaterTools)
		{
			gFloaterTools->mComboGridMode->setCurrentByIndex((S32)GRID_MODE_REF_OBJECT);
		}
		return true;
	}
};

void handle_test_load_url(void*)
{
	LLWeb::loadURL("");
	LLWeb::loadURL("hacker://www.google.com/");
	LLWeb::loadURL("http");
	LLWeb::loadURL("http://www.google.com/");
}

//
// LLViewerMenuHolderGL
//

LLViewerMenuHolderGL::LLViewerMenuHolderGL() : LLMenuHolderGL()
{
}

BOOL LLViewerMenuHolderGL::hideMenus()
{
	BOOL handled = LLMenuHolderGL::hideMenus();

	// drop pie menu selection
	mParcelSelection = NULL;
	mObjectSelection = NULL;

	gMenuBarView->clearHoverItem();
	gMenuBarView->resetMenuTrigger();

	return handled;
}

void LLViewerMenuHolderGL::setParcelSelection(LLHandle<LLParcelSelection> selection) 
{ 
	mParcelSelection = selection; 
}

void LLViewerMenuHolderGL::setObjectSelection(LLHandle<LLObjectSelection> selection) 
{ 
	mObjectSelection = selection; 
}


const LLRect LLViewerMenuHolderGL::getMenuRect() const
{
	return LLRect(0, mRect.getHeight() - MENU_BAR_HEIGHT, mRect.getWidth(), STATUS_BAR_HEIGHT);
}

void handle_save_to_xml(void*)
{
	LLFloater* frontmost = gFloaterView->getFrontmost();
	if (!frontmost)
	{
        gViewerWindow->alertXml("NoFrontmostFloater");
		return;
	}

	LLString default_name = "floater_";
	default_name += frontmost->getTitle();
	default_name += ".xml";

	LLString::toLower(default_name);
	LLString::replaceChar(default_name, ' ', '_');
	LLString::replaceChar(default_name, '/', '_');
	LLString::replaceChar(default_name, ':', '_');
	LLString::replaceChar(default_name, '"', '_');

	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getSaveFile(LLFilePicker::FFSAVE_XML, default_name.c_str()))
	{
		LLString filename = picker.getFirstFile();
		gUICtrlFactory->saveToXML(frontmost, filename);
	}
}

void handle_load_from_xml(void*)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if (picker.getOpenFile(LLFilePicker::FFLOAD_XML))
	{
		LLString filename = picker.getFirstFile();
		LLFloater* floater = new LLFloater("sample_floater");
		gUICtrlFactory->buildFloater(floater, filename);
	}
}

void handle_rebake_textures(void*)
{
	LLVOAvatar* avatar = gAgent.getAvatarObject();
	if (!avatar) return;

	// Slam pending upload count to "unstick" things
	bool slam_for_debug = true;
	avatar->forceBakeAllTextures(slam_for_debug);
}

void toggle_visibility(void* user_data)
{
	LLView* viewp = (LLView*)user_data;
	viewp->setVisible(!viewp->getVisible());
}

BOOL get_visibility(void* user_data)
{
	LLView* viewp = (LLView*)user_data;
	return viewp->getVisible();
}

// TomY TODO: Get rid of these?
class LLViewShowHoverTips : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLHoverView::sShowHoverTips = !LLHoverView::sShowHoverTips;
		return true;
	}
};

class LLViewCheckShowHoverTips : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLHoverView::sShowHoverTips;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

// TomY TODO: Get rid of these?
class LLViewHighlightTransparent : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLDrawPoolAlpha::sShowDebugAlpha = !LLDrawPoolAlpha::sShowDebugAlpha;
		return true;
	}
};

class LLViewCheckHighlightTransparent : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLDrawPoolAlpha::sShowDebugAlpha;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewToggleBeacon : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString beacon = userdata.asString();
		if (beacon == "scripts")
		{
			LLPipeline::toggleRenderScriptedBeacons(NULL);
		}
		else if (beacon == "physical")
		{
			LLPipeline::toggleRenderPhysicalBeacons(NULL);
		}
		else if (beacon == "sounds")
		{
			LLPipeline::toggleRenderSoundBeacons(NULL);
		}
		else if (beacon == "particles")
		{
			LLPipeline::toggleRenderParticleBeacons(NULL);
		}
		return true;
	}
};

class LLViewCheckBeaconEnabled : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString beacon = userdata["data"].asString();
		bool new_value = false;
		if (beacon == "scripts")
		{
			new_value = LLPipeline::getRenderScriptedBeacons(NULL);
		}
		else if (beacon == "physical")
		{
			new_value = LLPipeline::getRenderPhysicalBeacons(NULL);
		}
		else if (beacon == "sounds")
		{
			new_value = LLPipeline::getRenderSoundBeacons(NULL);
		}
		else if (beacon == "particles")
		{
			new_value = LLPipeline::getRenderParticleBeacons(NULL);
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewToggleRenderType : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString type = userdata.asString();
		if (type == "particles")
		{
			LLPipeline::toggleRenderType(LLPipeline::RENDER_TYPE_PARTICLES);
		}
		return true;
	}
};

class LLViewCheckRenderType : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString type = userdata["data"].asString();
		bool new_value = false;
		if (type == "particles")
		{
			new_value = LLPipeline::toggleRenderTypeControlNegated((void *)(S32)LLPipeline::RENDER_TYPE_PARTICLES);
		}
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLViewShowHUDAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLPipeline::sShowHUDAttachments = !LLPipeline::sShowHUDAttachments;
		return true;
	}
};

class LLViewCheckHUDAttachments : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		bool new_value = LLPipeline::sShowHUDAttachments;
		gMenuHolder->findControl(userdata["control"].asString())->setValue(new_value);
		return true;
	}
};

class LLEditEnableTakeOff : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString control_name = userdata["control"].asString();
		LLString clothing = userdata["data"].asString();
		bool new_value = false;
		if (clothing == "shirt")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_SHIRT);
		}
		if (clothing == "pants")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_PANTS);
		}
		if (clothing == "shoes")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_SHOES);
		}
		if (clothing == "socks")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_SOCKS);
		}
		if (clothing == "jacket")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_JACKET);
		}
		if (clothing == "gloves")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_GLOVES);
		}
		if (clothing == "undershirt")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_UNDERSHIRT);
		}
		if (clothing == "underpants")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_UNDERPANTS);
		}
		if (clothing == "skirt")
		{
			new_value = LLAgent::selfHasWearable((void *)WT_SKIRT);
		}
		gMenuHolder->findControl(control_name)->setValue(new_value);
		return true;
	}
};

class LLEditTakeOff : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString clothing = userdata.asString();
		if (clothing == "shirt")
		{
			LLAgent::userRemoveWearable((void*)WT_SHIRT);
		}
		else if (clothing == "pants")
		{
			LLAgent::userRemoveWearable((void*)WT_PANTS);
		}
		else if (clothing == "shoes")
		{
			LLAgent::userRemoveWearable((void*)WT_SHOES);
		}
		else if (clothing == "socks")
		{
			LLAgent::userRemoveWearable((void*)WT_SOCKS);
		}
		else if (clothing == "jacket")
		{
			LLAgent::userRemoveWearable((void*)WT_JACKET);
		}
		else if (clothing == "gloves")
		{
			LLAgent::userRemoveWearable((void*)WT_GLOVES);
		}
		else if (clothing == "undershirt")
		{
			LLAgent::userRemoveWearable((void*)WT_UNDERSHIRT);
		}
		else if (clothing == "underpants")
		{
			LLAgent::userRemoveWearable((void*)WT_UNDERPANTS);
		}
		else if (clothing == "skirt")
		{
			LLAgent::userRemoveWearable((void*)WT_SKIRT);
		}
		else if (clothing == "all")
		{
			LLAgent::userRemoveAllClothes(NULL);
		}
		return true;
	}
};

class LLWorldChat : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_chat(NULL);
		return true;
	}
};

class LLWorldStartGesture : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		handle_slash_key(NULL);
		return true;
	}
};

class LLToolsSelectTool : public view_listener_t
{
	bool handleEvent(LLPointer<LLEvent> event, const LLSD& userdata)
	{
		LLString tool_name = userdata.asString();
		if (tool_name == "focus")
		{
			gToolMgr->getCurrentToolset()->selectToolByIndex(1);
		}
		else if (tool_name == "move")
		{
			gToolMgr->getCurrentToolset()->selectToolByIndex(2);
		}
		else if (tool_name == "edit")
		{
			gToolMgr->getCurrentToolset()->selectToolByIndex(3);
		}
		else if (tool_name == "create")
		{
			gToolMgr->getCurrentToolset()->selectToolByIndex(4);
		}
		else if (tool_name == "land")
		{
			gToolMgr->getCurrentToolset()->selectToolByIndex(5);
		}
		return true;
	}
};

void initialize_menus()
{
	// File menu
	init_menu_file();

	// Edit menu
	(new LLEditUndo())->registerListener(gMenuHolder, "Edit.Undo");
	(new LLEditRedo())->registerListener(gMenuHolder, "Edit.Redo");
	(new LLEditCut())->registerListener(gMenuHolder, "Edit.Cut");
	(new LLEditCopy())->registerListener(gMenuHolder, "Edit.Copy");
	(new LLEditPaste())->registerListener(gMenuHolder, "Edit.Paste");
	(new LLEditDelete())->registerListener(gMenuHolder, "Edit.Delete");
	(new LLEditSearch())->registerListener(gMenuHolder, "Edit.Search");
	(new LLEditSelectAll())->registerListener(gMenuHolder, "Edit.SelectAll");
	(new LLEditDeselect())->registerListener(gMenuHolder, "Edit.Deselect");
	(new LLEditDuplicate())->registerListener(gMenuHolder, "Edit.Duplicate");
	(new LLEditTakeOff())->registerListener(gMenuHolder, "Edit.TakeOff");

	(new LLEditEnableUndo())->registerListener(gMenuHolder, "Edit.EnableUndo");
	(new LLEditEnableRedo())->registerListener(gMenuHolder, "Edit.EnableRedo");
	(new LLEditEnableCut())->registerListener(gMenuHolder, "Edit.EnableCut");
	(new LLEditEnableCopy())->registerListener(gMenuHolder, "Edit.EnableCopy");
	(new LLEditEnablePaste())->registerListener(gMenuHolder, "Edit.EnablePaste");
	(new LLEditEnableDelete())->registerListener(gMenuHolder, "Edit.EnableDelete");
	(new LLEditEnableSelectAll())->registerListener(gMenuHolder, "Edit.EnableSelectAll");
	(new LLEditEnableDeselect())->registerListener(gMenuHolder, "Edit.EnableDeselect");
	(new LLEditEnableDuplicate())->registerListener(gMenuHolder, "Edit.EnableDuplicate");
	(new LLEditEnableTakeOff())->registerListener(gMenuHolder, "Edit.EnableTakeOff");
	(new LLEditEnableCustomizeAvatar())->registerListener(gMenuHolder, "Edit.EnableCustomizeAvatar");

	// View menu
	(new LLViewMouselook())->registerListener(gMenuHolder, "View.Mouselook");
	(new LLViewBuildMode())->registerListener(gMenuHolder, "View.BuildMode");
	(new LLViewResetView())->registerListener(gMenuHolder, "View.ResetView");
	(new LLViewLookAtLastChatter())->registerListener(gMenuHolder, "View.LookAtLastChatter");
	(new LLViewShowHoverTips())->registerListener(gMenuHolder, "View.ShowHoverTips");
	(new LLViewHighlightTransparent())->registerListener(gMenuHolder, "View.HighlightTransparent");
	(new LLViewToggleBeacon())->registerListener(gMenuHolder, "View.ToggleBeacon");
	(new LLViewToggleRenderType())->registerListener(gMenuHolder, "View.ToggleRenderType");
	(new LLViewShowHUDAttachments())->registerListener(gMenuHolder, "View.ShowHUDAttachments");
	(new LLViewZoomOut())->registerListener(gMenuHolder, "View.ZoomOut");
	(new LLViewZoomIn())->registerListener(gMenuHolder, "View.ZoomIn");
	(new LLViewZoomDefault())->registerListener(gMenuHolder, "View.ZoomDefault");
	(new LLViewFullscreen())->registerListener(gMenuHolder, "View.Fullscreen");
	(new LLViewDefaultUISize())->registerListener(gMenuHolder, "View.DefaultUISize");

	(new LLViewEnableMouselook())->registerListener(gMenuHolder, "View.EnableMouselook");
	(new LLViewEnableLastChatter())->registerListener(gMenuHolder, "View.EnableLastChatter");

	(new LLViewCheckBuildMode())->registerListener(gMenuHolder, "View.CheckBuildMode");
	(new LLViewCheckShowHoverTips())->registerListener(gMenuHolder, "View.CheckShowHoverTips");
	(new LLViewCheckHighlightTransparent())->registerListener(gMenuHolder, "View.CheckHighlightTransparent");
	(new LLViewCheckBeaconEnabled())->registerListener(gMenuHolder, "View.CheckBeaconEnabled");
	(new LLViewCheckRenderType())->registerListener(gMenuHolder, "View.CheckRenderType");
	(new LLViewCheckHUDAttachments())->registerListener(gMenuHolder, "View.CheckHUDAttachments");

	// World menu
	(new LLWorldChat())->registerListener(gMenuHolder, "World.Chat");
	(new LLWorldStartGesture())->registerListener(gMenuHolder, "World.StartGesture");
	(new LLWorldAlwaysRun())->registerListener(gMenuHolder, "World.AlwaysRun");
	(new LLWorldFly())->registerListener(gMenuHolder, "World.Fly");
	(new LLWorldCreateLandmark())->registerListener(gMenuHolder, "World.CreateLandmark");
	(new LLWorldSetHomeLocation())->registerListener(gMenuHolder, "World.SetHomeLocation");
	(new LLWorldTeleportHome())->registerListener(gMenuHolder, "World.TeleportHome");
	(new LLWorldSetAway())->registerListener(gMenuHolder, "World.SetAway");
	(new LLWorldSetBusy())->registerListener(gMenuHolder, "World.SetBusy");

	(new LLWorldEnableCreateLandmark())->registerListener(gMenuHolder, "World.EnableCreateLandmark");
	(new LLWorldEnableSetHomeLocation())->registerListener(gMenuHolder, "World.EnableSetHomeLocation");
	(new LLWorldEnableTeleportHome())->registerListener(gMenuHolder, "World.EnableTeleportHome");
	(new LLWorldEnableBuyLand())->registerListener(gMenuHolder, "World.EnableBuyLand");

	(new LLWorldCheckAlwaysRun())->registerListener(gMenuHolder, "World.CheckAlwaysRun");

	(new LLWorldForceSun())->registerListener(gMenuHolder, "World.ForceSun");

	// Tools menu
	(new LLToolsSelectTool())->registerListener(gMenuHolder, "Tools.SelectTool");
	(new LLToolsSelectOnlyMyObjects())->registerListener(gMenuHolder, "Tools.SelectOnlyMyObjects");
	(new LLToolsSelectOnlyMovableObjects())->registerListener(gMenuHolder, "Tools.SelectOnlyMovableObjects");
	(new LLToolsSelectBySurrounding())->registerListener(gMenuHolder, "Tools.SelectBySurrounding");
	(new LLToolsShowHiddenSelection())->registerListener(gMenuHolder, "Tools.ShowHiddenSelection");
	(new LLToolsShowSelectionLightRadius())->registerListener(gMenuHolder, "Tools.ShowSelectionLightRadius");
	(new LLToolsSnapObjectXY())->registerListener(gMenuHolder, "Tools.SnapObjectXY");
	(new LLToolsUseSelectionForGrid())->registerListener(gMenuHolder, "Tools.UseSelectionForGrid");
	(new LLToolsLink())->registerListener(gMenuHolder, "Tools.Link");
	(new LLToolsUnlink())->registerListener(gMenuHolder, "Tools.Unlink");
	(new LLToolsStopAllAnimations())->registerListener(gMenuHolder, "Tools.StopAllAnimations");
	(new LLToolsLookAtSelection())->registerListener(gMenuHolder, "Tools.LookAtSelection");
	(new LLToolsBuyOrTake())->registerListener(gMenuHolder, "Tools.BuyOrTake");
	(new LLToolsTakeCopy())->registerListener(gMenuHolder, "Tools.TakeCopy");
	(new LLToolsSaveToInventory())->registerListener(gMenuHolder, "Tools.SaveToInventory");
	(new LLToolsSaveToObjectInventory())->registerListener(gMenuHolder, "Tools.SaveToObjectInventory");
	(new LLToolsSelectedScriptAction())->registerListener(gMenuHolder, "Tools.SelectedScriptAction");

	(new LLToolsEnableToolNotPie())->registerListener(gMenuHolder, "Tools.EnableToolNotPie");
	(new LLToolsEnableLink())->registerListener(gMenuHolder, "Tools.EnableLink");
	(new LLToolsEnableUnlink())->registerListener(gMenuHolder, "Tools.EnableUnlink");
	(new LLToolsEnableBuyOrTake())->registerListener(gMenuHolder, "Tools.EnableBuyOrTake");
	(new LLToolsEnableTakeCopy())->registerListener(gMenuHolder, "Tools.EnableTakeCopy");
	(new LLToolsEnableSaveToInventory())->registerListener(gMenuHolder, "Tools.SaveToInventory");
	(new LLToolsEnableSaveToObjectInventory())->registerListener(gMenuHolder, "Tools.SaveToObjectInventory");

	/*(new LLToolsVisibleBuyObject())->registerListener(gMenuHolder, "Tools.VisibleBuyObject");
	(new LLToolsVisibleTakeObject())->registerListener(gMenuHolder, "Tools.VisibleTakeObject");*/

	// Help menu
	(new LLHelpLiveHelp())->registerListener(gMenuHolder, "Help.LiveHelp");
	(new LLHelpMOTD())->registerListener(gMenuHolder, "Help.MOTD");

	// Self pie menu
	(new LLSelfStandUp())->registerListener(gMenuHolder, "Self.StandUp");
	(new LLSelfRemoveAllAttachments())->registerListener(gMenuHolder, "Self.RemoveAllAttachments");

	(new LLSelfEnableStandUp())->registerListener(gMenuHolder, "Self.EnableStandUp");
	(new LLSelfEnableRemoveAllAttachments())->registerListener(gMenuHolder, "Self.EnableRemoveAllAttachments");

	 // Avatar pie menu
	(new LLObjectMute())->registerListener(gMenuHolder, "Avatar.Mute");
	(new LLAvatarAddFriend())->registerListener(gMenuHolder, "Avatar.AddFriend");
	(new LLAvatarFreeze())->registerListener(gMenuHolder, "Avatar.Freeze");
	(new LLAvatarDebug())->registerListener(gMenuHolder, "Avatar.Debug");
	(new LLAvatarVisibleDebug())->registerListener(gMenuHolder, "Avatar.VisibleDebug");
	(new LLAvatarEnableDebug())->registerListener(gMenuHolder, "Avatar.EnableDebug");
	(new LLAvatarGiveCard())->registerListener(gMenuHolder, "Avatar.GiveCard");
	(new LLAvatarEject())->registerListener(gMenuHolder, "Avatar.Eject");
	(new LLAvatarSendIM())->registerListener(gMenuHolder, "Avatar.SendIM");
	
	(new LLObjectEnableMute())->registerListener(gMenuHolder, "Avatar.EnableMute");
	(new LLAvatarEnableAddFriend())->registerListener(gMenuHolder, "Avatar.EnableAddFriend");
	(new LLAvatarEnableFreezeEject())->registerListener(gMenuHolder, "Avatar.EnableFreezeEject");

	// Object pie menu
	(new LLObjectOpen())->registerListener(gMenuHolder, "Object.Open");
	(new LLObjectBuild())->registerListener(gMenuHolder, "Object.Build");
	(new LLObjectTouch())->registerListener(gMenuHolder, "Object.Touch");
	(new LLObjectSitOrStand())->registerListener(gMenuHolder, "Object.SitOrStand");
	(new LLObjectDelete())->registerListener(gMenuHolder, "Object.Delete");
	(new LLObjectAttachToAvatar())->registerListener(gMenuHolder, "Object.AttachToAvatar");
	(new LLObjectReturn())->registerListener(gMenuHolder, "Object.Return");
	(new LLObjectReportAbuse())->registerListener(gMenuHolder, "Object.ReportAbuse");
	(new LLObjectMute())->registerListener(gMenuHolder, "Object.Mute");
	(new LLObjectBuy())->registerListener(gMenuHolder, "Object.Buy");
	(new LLObjectEdit())->registerListener(gMenuHolder, "Object.Edit");
	(new LLObjectInspect())->registerListener(gMenuHolder, "Object.Inspect");

	(new LLObjectEnableOpen())->registerListener(gMenuHolder, "Object.EnableOpen");
	(new LLObjectEnableTouch())->registerListener(gMenuHolder, "Object.EnableTouch");
	(new LLObjectEnableSitOrStand())->registerListener(gMenuHolder, "Object.EnableSitOrStand");
	(new LLObjectEnableDelete())->registerListener(gMenuHolder, "Object.EnableDelete");
	(new LLObjectEnableWear())->registerListener(gMenuHolder, "Object.EnableWear");
	(new LLObjectEnableReturn())->registerListener(gMenuHolder, "Object.EnableReturn");
	(new LLObjectEnableReportAbuse())->registerListener(gMenuHolder, "Object.EnableReportAbuse");
	(new LLObjectEnableMute())->registerListener(gMenuHolder, "Object.EnableMute");
	(new LLObjectEnableBuy())->registerListener(gMenuHolder, "Object.EnableBuy");

	/*(new LLObjectVisibleTouch())->registerListener(gMenuHolder, "Object.VisibleTouch");
	(new LLObjectVisibleCustomTouch())->registerListener(gMenuHolder, "Object.VisibleCustomTouch");
	(new LLObjectVisibleStandUp())->registerListener(gMenuHolder, "Object.VisibleStandUp");
	(new LLObjectVisibleSitHere())->registerListener(gMenuHolder, "Object.VisibleSitHere");
	(new LLObjectVisibleCustomSit())->registerListener(gMenuHolder, "Object.VisibleCustomSit");*/

	// Attachment pie menu
	(new LLAttachmentDrop())->registerListener(gMenuHolder, "Attachment.Drop");
	(new LLAttachmentDetach())->registerListener(gMenuHolder, "Attachment.Detach");

	(new LLAttachmentEnableDrop())->registerListener(gMenuHolder, "Attachment.EnableDrop");
	(new LLAttachmentEnableDetach())->registerListener(gMenuHolder, "Attachment.EnableDetach");

	// Land pie menu
	(new LLLandBuild())->registerListener(gMenuHolder, "Land.Build");
	(new LLLandSit())->registerListener(gMenuHolder, "Land.Sit");
	(new LLLandBuyPass())->registerListener(gMenuHolder, "Land.BuyPass");
	(new LLLandEdit())->registerListener(gMenuHolder, "Land.Edit");

	(new LLLandEnableBuyPass())->registerListener(gMenuHolder, "Land.EnableBuyPass");

	// Generic actions
	(new LLShowFloater())->registerListener(gMenuHolder, "ShowFloater");
	(new LLPromptShowURL())->registerListener(gMenuHolder, "PromptShowURL");
	(new LLPromptShowFile())->registerListener(gMenuHolder, "PromptShowFile");
	(new LLShowAgentProfile())->registerListener(gMenuHolder, "ShowAgentProfile");
	(new LLShowAgentGroups())->registerListener(gMenuHolder, "ShowAgentGroups");
	(new LLToggleControl())->registerListener(gMenuHolder, "ToggleControl");

	(new LLGoToObject())->registerListener(gMenuHolder, "GoToObject");
	(new LLPayObject())->registerListener(gMenuHolder, "PayObject");

	(new LLEnablePayObject())->registerListener(gMenuHolder, "EnablePayObject");
	(new LLEnableEdit())->registerListener(gMenuHolder, "EnableEdit");

	(new LLFloaterVisible())->registerListener(gMenuHolder, "FloaterVisible");
	(new LLSomethingSelected())->registerListener(gMenuHolder, "SomethingSelected");
	(new LLSomethingSelectedNoHUD())->registerListener(gMenuHolder, "SomethingSelectedNoHUD");
	(new LLEditableSelected())->registerListener(gMenuHolder, "EditableSelected");
}
