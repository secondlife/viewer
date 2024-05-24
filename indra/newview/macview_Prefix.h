/**
 * @file macview_Prefix.h
 * @brief Prefix header for all source files of the 'newview' target in the 'newview' project.
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
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

// MBW -- This doesn't work.  There are some conflicts between things in Carbon.h and some of the linden source.
//#include <Carbon/Carbon.h>

////////////////// From llagent.cpp
#include "llpreprocessor.h"
#include "stdtypes.h"
#include "stdenums.h"

#include "llagent.h"

#include "llcoordframe.h"
#include "indra_constants.h"
#include "llmath.h"
#include "llcriticaldamp.h"
#include "llfocusmgr.h"
#include "llparcel.h"
#include "llpermissions.h"
#include "llregionhandle.h"
#include "m3math.h"
#include "m4math.h"
#include "message.h"
#include "qmath.h"
#include "v3math.h"
#include "v4math.h"
#include "vmath.h"
//#include "llteleportflags.h"

#include "llbox.h"
#include "llbutton.h"
#include "llconsole.h"
#include "lldrawable.h"
#include "llfirstuse.h"
#include "llfloater.h"
#include "llfloaterbuildoptions.h"
#include "llfloaterchat.h"
#include "llfloatergroups.h"
#include "llfloaterworldmap.h"
#include "llfloatermute.h"
#include "llconversation.h"
#include "llfloatertools.h"
#include "llhudeffectlookat.h"
#include "llhudmanager.h"
#include "lljoystickbutton.h"
#include "llmenugl.h"
#include "llmorphview.h"
#include "llmoveview.h"
#include "llselectmgr.h"
#include "llsky.h"
#include "llrendersphere.h"
#include "llstatusbar.h"
#include "lltalkview.h"
#include "lltool.h"
#include "lltoolfocus.h"
#include "lltoolcomp.h"     // for gToolGun
#include "lltoolgrab.h"
#include "lltoolmgr.h"
#include "lltoolpie.h"
#include "llui.h"           // for make_ui_sound
#include "llviewercamera.h"
#include "llviewermenu.h"
#include "llviewerobjectlist.h"
#include "llviewerparcelmgr.h"
#include "llviewerparceloverlay.h"
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llvoavatar.h"
#include "llvoground.h"
#include "llvosky.h"
#include "llworld.h"
#include "pipeline.h"

/////////////////// From llfloater.cpp
#include "llbutton.h"
#include "lldraghandle.h"
#include "llfocusmgr.h"
#include "llresizebar.h"
#include "llresizehandle.h"
#include "llresmgr.h"
#include "llui.h"
#include "llviewborder.h"
#include "lluictrlfactory.h"


/////////////////// From lldrawpool.cpp
#include "llface.h"
#include "llcontrol.h"
#include "pipeline.h"

#include "llviewerobjectlist.h" // For debug listing.

//extern LLPipeline gPipeline;

#include "lldrawpoolsimple.h"
#include "lldrawpoolalpha.h"
#include "lldrawpoolavatar.h"
#include "lldrawpooltree.h"
#include "lldrawpoolterrain.h"
#include "lldrawpoolsky.h"
#include "lldrawpoolwater.h"
#include "lldrawpoolground.h"
#include "lldrawpoolbump.h"

/////////////////// From llface.cpp
#include "llgl.h"
#include "llviewerimage.h"
#include "llsky.h"
#include "llvosky.h"
#include "llcontrol.h"
#include "v3color.h"
#include "pipeline.h"
#include "llvolume.h"
#include "llviewercamera.h"
#include "lllightconstants.h"

#include "llvovolume.h"
#include "m3math.h"
#include "lldrawpoolbump.h"



/////////////////// From llpanel.cpp
#include "llpanel.h"

#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "lltimer.h"

#include "llmenugl.h"
#include "llstatusbar.h"
#include "llui.h"
#include "llkeyboard.h"
#include "llviewerwindow.h"
#include "llcontrol.h"
#include "lluictrl.h"
#include "lluictrlfactory.h"
#include "llviewborder.h"
#include "llviewerimagelist.h"
#include "llbutton.h"
#include "llfocusmgr.h"



/////////////////// From llvovolume.cpp
#include "llvovolume.h"
#include "llviewerimagelist.h"

#include "llcontrol.h"

#include "object_flags.h"

#include "material_codes.h"
#include "llagent.h"
#include "llworld.h"
#include "llviewerregion.h"
#include "llprimitive.h"
#include "llvolume.h"
#include "lldrawable.h"
#include "llface.h"
#include "llvolumemgr.h"
#include "llsky.h"

#include "pipeline.h"
#include "llmaterialtable.h"
#include "message.h"
#include "llviewertextureanim.h"
#include "llviewercamera.h"
#include "lldrawpoolbump.h"


/////////////////// From llagentpilot.cpp
#include "llagentpilot.h"
#include "llagent.h"
#include "llframestats.h"
#include "viewer.h"
#include "llcontrol.h"


/////////////////// From llloginview.cpp
#include "llloginview.h"

#include "indra_constants.h"        // for key and mask constants
#include "llfontgl.h"
#include "v4color.h"
#include "llwindow_impl.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcontrol.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "llui.h"
//#include "lluiconstants.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"           // for handle_preferences()
#include "llviewerwindow.h"         // to link into child list
#include "llfocusmgr.h"
#include "llmd5.h"
#include "llversion.h"
#include "viewer.h"

