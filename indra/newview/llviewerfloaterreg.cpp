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
#include "llfloaterabout.h"
#include "llfloateractivespeakers.h"
#include "llfloaterauction.h"
#include "llfloateraddlandmark.h"
#include "llfloateravatarinfo.h"
#include "llfloaterbeacons.h"
#include "llfloaterbulkpermission.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterbump.h"
#include "llfloatercamera.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llfloaterdirectory.h"
#include "llfloaterfonttest.h"
#include "llfloatergodtools.h"
#include "llfloaterhtmlcurrency.h"
#include "llfloaterhtmlhelp.h"
#include "llfloaterhud.h"
#include "llfloaterinspect.h"
#include "llfloaterjoystick.h"
#include "llfloaternotificationsconsole.h"
#include "llfloaterlagmeter.h"
#include "llfloaterland.h"
#include "llfloatermap.h"
#include "llfloatermemleak.h"
#include "llfloatermute.h"
#include "llfloaterobjectiminfo.h"
#include "llfloateropenobject.h"
#include "llfloaterperms.h"
#include "llfloaterpreference.h"
#include "llfloaterregioninfo.h"
#include "llfloatersnapshot.h"
#include "llfloatersettingsdebug.h"
#include "llfloatertestlistview.h"
#include "llfloatertopobjects.h"
#include "llfloatertools.h"
#include "llfloateruipreview.h"
#include "llfloaterurldisplay.h"
#include "llfloatervoicedevicesettings.h"
#include "llfloaterworldmap.h"
#include "llfloaterinventory.h"
#include "llmediaremotectrl.h"
#include "llmoveview.h"
#include "llnearbychathistory.h"

#include "llpreviewanim.h"
#include "llpreviewgesture.h"
#include "llpreviewlandmark.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llpreviewsound.h"
#include "llpreviewtexture.h"
#include "llfloaterminiinspector.h"

//class LLLLFloaterObjectIMInfo;

void LLViewerFloaterReg::registerFloaters()
{
	LLFloaterReg::add("about_land", "floater_about_land.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLand>);
	LLFloaterReg::add("active_speakers", "floater_active_speakers.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterActiveSpeakers>);
	LLFloaterReg::add("add_landmark", "floater_add_landmark.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAddLandmark>);
	LLFloaterReg::add("auction", "floater_auction.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAuction>);

	LLFloaterReg::add("beacons", "floater_beacons.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBeacons>);
	LLFloaterReg::add("bulk_perms", "floater_bulk_perms.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBulkPermission>);
	LLFloaterReg::add("build", "floater_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTools>);
	LLFloaterReg::add("build_options", "floater_build_options.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBuildOptions>);
	LLFloaterReg::add("bumps", "floater_bumps.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterBump>);

	LLFloaterReg::add("camera", "floater_camera.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCamera>);
	LLFloaterReg::add("chat", "floater_chat_history.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChat>);
	LLFloaterReg::add("communicate", "floater_chatterbox.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterChatterBox>);
	LLFloaterReg::add("compile_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterCompileQueue>);
	LLFloaterReg::add("contacts", "floater_my_friends.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMyFriends>);	

	LLFloaterReg::add("font_test", "floater_font_test.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterFontTest>);

	LLFloaterReg::add("god_tools", "floater_god_tools.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterGodTools>);

	LLFloaterReg::add("html_currency", "floater_html_simple.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHtmlCurrency>);
	LLFloaterReg::add("hud", "floater_hud.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterHUD>);
	
	LLFloaterReg::add("inventory", "floater_inventory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInventory>);
	LLFloaterReg::add("inspect", "floater_inspect.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterInspect>);
	
	LLFloaterReg::add("lagmeter", "floater_lagmeter.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterLagMeter>);
	
	LLFloaterReg::add("mem_leaking", "floater_mem_leaking.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMemLeak>);
	LLFloaterReg::add("me_profile", "floater_me.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarInfo>);
	LLFloaterReg::add("media_browser", "floater_media_browser.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaBrowser>);	
	LLFloaterReg::add("moveview", "floater_moveview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMove>);
	LLFloaterReg::add("mute", "floater_mute.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMute>);
	LLFloaterReg::add("mini_map", "floater_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMap>);
	LLFloaterReg::add("mini_inspector", "panel_mini_inspector.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMiniInspector>);
	
	LLFloaterReg::add("notifications_console", "floater_notifications_console.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotificationConsole>);
	LLFloaterReg::add("nearby_chat", "floater_nearby_chat.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLNearbyChatHistory>);

	LLFloaterReg::add("openobject", "floater_openobject.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterOpenObject>);

	LLFloaterReg::add("preferences", "floater_preferences.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPreference>);
	LLFloaterReg::add("perm_prefs", "floater_perm_prefs.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPerms>);
	LLFloaterReg::add("preview_url", "floater_preview_url.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterURLDisplay>);
	LLFloaterReg::add("pref_joystick", "floater_joystick.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterJoystick>);
	LLFloaterReg::add("pref_voicedevicesettings", "floater_device_settings.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterVoiceDeviceSettings>);
	LLFloaterReg::add("preview_avatar", "floater_profile.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAvatarInfo>);
	LLFloaterReg::add("preview_anim", "floater_preview_animation.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewAnim>, "preview");
	LLFloaterReg::add("preview_gesture", "floater_preview_gesture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewGesture>, "preview");
	LLFloaterReg::add("preview_landmark", "floater_preview_existing_landmark.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewLandmark>, "preview");
	LLFloaterReg::add("preview_notecard", "floater_preview_notecard.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewNotecard>, "preview");
	LLFloaterReg::add("preview_script", "floater_script_preview.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewLSL>, "preview");
	LLFloaterReg::add("preview_scriptedit", "floater_live_lsleditor.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLLiveLSLEditor>, "preview");
	LLFloaterReg::add("preview_sound", "floater_preview_sound.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewSound>, "preview");
	LLFloaterReg::add("preview_texture", "floater_preview_texture.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLPreviewTexture>, "preview");
	
	LLFloaterReg::add("test_list_view", "floater_test_list_view.xml",&LLFloaterReg::build<LLFloaterTestListView>);
	LLFloaterReg::add("test_widgets", "floater_test_widgets.xml", &LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("top_objects", "floater_top_objects.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterTopObjects>);
	
	LLFloaterReg::add("reset_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterResetQueue>);
	LLFloaterReg::add("region_info", "floater_region_info.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRegionInfo>);
	
	LLFloaterReg::add("settings_debug", "floater_settings_debug.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSettingsDebug>);	
	LLFloaterReg::add("stats", "floater_stats.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloater>);
	LLFloaterReg::add("start_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterRunQueue>);
	LLFloaterReg::add("stop_queue", "floater_script_queue.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterNotRunQueue>);
	LLFloaterReg::add("sl_about", "floater_about.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterAbout>);
	LLFloaterReg::add("snapshot", "floater_snapshot.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterSnapshot>);
	LLFloaterReg::add("search", "floater_directory.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterDirectory>);
	
	LLFloaterReg::add("ui_preview", "floater_ui_preview.xml", &LLFloaterReg::build<LLFloaterUIPreview>);
	
	LLFloaterReg::add("world_map", "floater_world_map.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterWorldMap>);	
	
	LLObjectIMInfo::register_floater();
	// debug use only
	LLFloaterReg::add("media_remote_ctrl", "floater_media_remote.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterMediaRemoteCtrl>);

	// *NOTE: Please keep these alphabetized for easier merges

	LLFloaterReg::registerControlVariables(); // Make sure visibility and rect controls get preserved when saving
}
