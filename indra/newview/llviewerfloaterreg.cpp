/** 
 * @file llviewerfloaterreg.cpp
 * @brief LLViewerFloaterReg class registers floaters used in the viewer
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
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

#include "llfloaterreg.h"

#include "llviewerfloaterreg.h"

#include "llcompilequeue.h"
#include "llcallfloater.h"
#include "llfloaterabout.h"
#include "llfloateractivespeakers.h"
#include "llfloateranimpreview.h"
#include "llfloaterauction.h"
#include "llfloateravatarpicker.h"
#include "llfloateravatartextures.h"
#include "llfloaterbeacons.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbuy.h"
#include "llfloaterbuycontents.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterbuyland.h"
#include "llfloaterbulkpermission.h"
#include "llfloaterbump.h"
#include "llfloatercamera.h"
#include "llfloaterdaycycle.h"
#include "llfloatersearch.h"
#include "llfloaterenvsettings.h"
#include "llfloaterfonttest.h"
#include "llfloatergesture.h"
#include "llfloatergodtools.h"
#include "llfloatergroups.h"
#include "llfloaterhardwaresettings.h"
#include "llfloaterhelpbrowser.h"
#include "llfloatermediabrowser.h"
#include "llfloatermediasettings.h"
#include "llfloaterhud.h"
#include "llfloaterimagepreview.h"
#include "llimfloater.h"
#include "llfloaterinspect.h"
#include "llfloaterinventory.h"
#include "llfloaterjoystick.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloaterlandholdings.h"
#include "llfloatermap.h"
#include "llfloatermemleak.h"
#include "llfloaternamedesc.h"
#include "llfloaternearbymedia.h"
#include "llfloaternotificationsconsole.h"
#include "llfloateropenobject.h"
#include "llfloaterpay.h"
#include "llfloaterparcel.h"
#include "llfloaterperms.h"
#include "llfloaterpostcard.h"
#include "llfloaterpostprocess.h"
#include "llfloaterpreference.h"
#include "llfloaterproperties.h"
#include "llfloaterregioninfo.h"
#include "llfloaterreporter.h"
#include "llfloaterscriptdebug.h"
#include "llfloaterscriptlimits.h"
#include "llfloatersellland.h"
#include "llfloatersettingsdebug.h"
#include "llfloatersnapshot.h"
#include "llfloatertelehub.h"
#include "llfloatertestinspectors.h"
#include "llfloatertestlistview.h"
#include "llfloatertools.h"
#include "llfloatertos.h"
#include "llfloatertopobjects.h"
#include "llfloateruipreview.h"
#include "llfloaterurldisplay.h"
#include "llfloatervoicedevicesettings.h"
#include "llfloaterwater.h"
#include "llfloaterwhitelistentry.h"
#include "llfloaterwindlight.h"
#include "llfloaterwindowsize.h"
#include "llfloaterworldmap.h"
#include "llimfloatercontainer.h"
#include "llinspectavatar.h"
#include "llinspectgroup.h"
#include "llinspectobject.h"
#include "llinspectremoteobject.h"
#include "llinspecttoast.h"
#include "llmoveview.h"
#include "llnearbychat.h"
#include "llpanelblockedlist.h"
#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llsyswellwindow.h"
#include "llscriptfloater.h"
// *NOTE: Please add files in alphabetical order to keep merges easy.


void LLViewerFloaterReg::registerFloaters()
{
	// *NOTE: Please keep these alphabetized for easier merges

	LLFloaterAboutUtil::registerFloater();
	LLFloaterReg::add("about_land", "floater_about_land.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLand>);
	LLFloaterReg::add("active_speakers", "floater_active_speakers.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterActiveSpeakers>);
	LLFloaterReg::add("auction", "floater_auction.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAuction>);
	LLFloaterReg::add("avatar_picker", "floater_avatar_picker.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarPicker>);
	LLFloaterReg::add("avatar_textures", "floater_avatar_textures.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarTextures>);

	LLFloaterReg::add("beacons", "floater_beacons.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBeacons>);
	LLFloaterReg::add("bulk_perms", "floater_bulk_perms.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBulkPermission>);
	LLFloaterReg::add("buy_currency", "floater_buy_currency.xml", &LLFloaterBuyCurrency::buildFloater);
	LLFloaterReg::add("buy_land", "floater_buy_land.xml", &LLFloaterBuyLand::buildFloater);
	LLFloaterReg::add("buy_object", "floater_buy_object.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuy>);
	LLFloaterReg::add("buy_object_contents", "floater_buy_contents.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuyContents>);
	LLFloaterReg::add("build", "floater_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTools>);
	LLFloaterReg::add("build_options", "floater_build_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuildOptions>);
	LLFloaterReg::add("bumps", "floater_bumps.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBump>);

	LLFloaterReg::add("camera", "floater_camera.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCamera>);
	//LLFloaterReg::add("chat", "floater_chat_history.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChat>);
	LLFloaterReg::add("nearby_chat", "floater_nearby_chat.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLNearbyChat>);

	LLFloaterReg::add("compile_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCompileQueue>);

	LLFloaterReg::add("env_day_cycle", "floater_day_cycle_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterDayCycle>);
	LLFloaterReg::add("env_post_process", "floater_post_process.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPostProcess>);
	LLFloaterReg::add("env_settings", "floater_env_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterEnvSettings>);
	LLFloaterReg::add("env_water", "floater_water.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWater>);
	LLFloaterReg::add("env_windlight", "floater_windlight_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWindLight>);
	
	LLFloaterReg::add("font_test", "floater_font_test.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFontTest>);

	LLFloaterReg::add("gestures", "floater_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGesture>);
	LLFloaterReg::add("god_tools", "floater_god_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGodTools>);
	LLFloaterReg::add("group_picker", "floater_choose_group.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGroupPicker>);

	LLFloaterReg::add("help_browser", "floater_help_browser.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHelpBrowser>);	
	LLFloaterReg::add("hud", "floater_hud.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHUD>);

	LLFloaterReg::add("impanel", "floater_im_session.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIMFloater>);
	LLFloaterReg::add("im_container", "floater_im_container.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIMFloaterContainer>);
	LLFloaterReg::add("im_well_window", "floater_sys_well.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIMWellWindow>);
	LLFloaterReg::add("incoming_call", "floater_incoming_call.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIncomingCallDialog>);
	LLFloaterReg::add("inventory", "floater_inventory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInventory>);
	LLFloaterReg::add("inspect", "floater_inspect.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInspect>);
	LLInspectAvatarUtil::registerFloater();
	LLInspectGroupUtil::registerFloater();
	LLInspectObjectUtil::registerFloater();
	LLInspectRemoteObjectUtil::registerFloater();
	LLNotificationsUI::registerFloater();
	
	LLFloaterReg::add("lagmeter", "floater_lagmeter.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLagMeter>);
	LLFloaterReg::add("land_holdings", "floater_land_holdings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLandHoldings>);
	
	LLFloaterReg::add("mem_leaking", "floater_mem_leaking.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMemLeak>);
	LLFloaterReg::add("media_browser", "floater_media_browser.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaBrowser>);	
	LLFloaterReg::add("media_settings", "floater_media_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaSettings>);	
	LLFloaterReg::add("message_critical", "floater_critical.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTOS>);
	LLFloaterReg::add("message_tos", "floater_tos.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTOS>);
	LLFloaterReg::add("moveview", "floater_moveview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMove>);
	LLFloaterReg::add("mute_object_by_name", "floater_mute_object.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGetBlockedObjectName>);
	LLFloaterReg::add("mini_map", "floater_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMap>);

	LLFloaterReg::add("nearby_media", "floater_nearby_media.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNearbyMedia>);
	LLFloaterReg::add("notifications_console", "floater_notifications_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationConsole>);
	LLFloaterReg::add("notification_well_window", "floater_sys_well.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLNotificationWellWindow>);

	LLFloaterReg::add("openobject", "floater_openobject.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOpenObject>);
	LLFloaterReg::add("outgoing_call", "floater_outgoing_call.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLOutgoingCallDialog>);
	LLFloaterReg::add("parcel_info", "floater_preview_url.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterParcelInfo>);
	LLFloaterPayUtil::registerFloater();

	LLFloaterReg::add("postcard", "floater_postcard.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPostcard>);
	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	LLFloaterReg::add("prefs_hardware_settings", "floater_hardware_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHardwareSettings>);
	LLFloaterReg::add("perm_prefs", "floater_perm_prefs.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPerms>);
	LLFloaterReg::add("preview_url", "floater_preview_url.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterURLDisplay>);
	LLFloaterReg::add("pref_joystick", "floater_joystick.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterJoystick>);
	LLFloaterReg::add("pref_voicedevicesettings", "floater_device_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterVoiceDeviceSettings>);
	LLFloaterReg::add("preview_anim", "floater_preview_animation.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewAnim>, "preview");
	LLFloaterReg::add("preview_gesture", "floater_preview_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewGesture>, "preview");
	LLFloaterReg::add("preview_notecard", "floater_preview_notecard.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewNotecard>, "preview");
	LLFloaterReg::add("preview_script", "floater_script_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewLSL>, "preview");
	LLFloaterReg::add("preview_scriptedit", "floater_live_lsleditor.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLLiveLSLEditor>, "preview");
	LLFloaterReg::add("preview_sound", "floater_preview_sound.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewSound>, "preview");
	LLFloaterReg::add("preview_texture", "floater_preview_texture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewTexture>, "preview");
	LLFloaterReg::add("properties", "floater_inventory_item_properties.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterProperties>);

	LLFloaterReg::add("telehubs", "floater_telehub.xml",&LLFloaterReg::build<LLFloaterTelehub>);
	LLFloaterReg::add("test_inspectors", "floater_test_inspectors.xml",
		&LLFloaterReg::build<LLFloaterTestInspectors>);
	//LLFloaterReg::add("test_list_view", "floater_test_list_view.xml",&LLFloaterReg::build<LLFloaterTestListView>);
	LLFloaterReg::add("test_textbox", "floater_test_textbox.xml",
		&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("test_text_editor", "floater_test_text_editor.xml",
		&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("test_widgets", "floater_test_widgets.xml",
		&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("top_objects", "floater_top_objects.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTopObjects>);
	
	LLFloaterReg::add("reporter", "floater_report_abuse.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterReporter>);
	LLFloaterReg::add("reset_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterResetQueue>);
	LLFloaterReg::add("region_info", "floater_region_info.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRegionInfo>);
	
	LLFloaterReg::add("script_debug", "floater_script_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptDebug>);
	LLFloaterReg::add("script_debug_output", "floater_script_debug_panel.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptDebugOutput>);
	LLFloaterReg::add("script_floater", "floater_script.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLScriptFloater>);
	LLFloaterReg::add("script_limits", "floater_script_limits.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterScriptLimits>);
	LLFloaterReg::add("sell_land", "floater_sell_land.xml", &LLFloaterSellLand::buildFloater);
	LLFloaterReg::add("settings_debug", "floater_settings_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSettingsDebug>);
	LLFloaterReg::add("stats", "floater_stats.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("start_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRunQueue>);
	LLFloaterReg::add("stop_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotRunQueue>);
	LLFloaterReg::add("snapshot", "floater_snapshot.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSnapshot>);
	LLFloaterReg::add("search", "floater_search.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSearch>);	
	
	LLFloaterUIPreviewUtil::registerFloater();
	LLFloaterReg::add("upload_anim", "floater_animation_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAnimPreview>, "upload");
	LLFloaterReg::add("upload_image", "floater_image_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterImagePreview>, "upload");
	LLFloaterReg::add("upload_sound", "floater_sound_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSoundPreview>, "upload");
	
	LLFloaterReg::add("voice_controls", "floater_voice_controls.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLCallFloater>);
	
	LLFloaterReg::add("whitelist_entry", "floater_whitelist_entry.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWhiteListEntry>);	
	LLFloaterWindowSizeUtil::registerFloater();
	LLFloaterReg::add("world_map", "floater_world_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWorldMap>);	

	// *NOTE: Please keep these alphabetized for easier merges
	
	LLFloaterReg::registerControlVariables(); // Make sure visibility and rect controls get preserved when saving
}
