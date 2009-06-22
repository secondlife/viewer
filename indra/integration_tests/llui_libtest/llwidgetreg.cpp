/** 
 * @file llwidgetreg.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
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
#include "linden_common.h"

#include "llwidgetreg.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcontainerview.h"
#include "lliconctrl.h"
#include "llmenugl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llprogressbar.h"
#include "llradiogroup.h"
#include "llscrollcontainer.h"
#include "llscrollingpanellist.h"
#include "llscrolllistctrl.h"
#include "llslider.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "llstatview.h"
#include "lltabcontainer.h"
#include "lltextbox.h"
#include "lltexteditor.h"
#include "llflyoutbutton.h"
#include "llsearcheditor.h"
#include "lllayoutstack.h"

void LLWidgetReg::initClass(bool register_widgets)
{
	// Only need to register if the Windows linker has optimized away the
	// references to the object files.
	if (register_widgets)
	{
		LLDefaultWidgetRegistry::Register<LLButton> button("button");
		LLDefaultWidgetRegistry::Register<LLCheckBoxCtrl> check_box("check_box");
		LLDefaultWidgetRegistry::Register<LLComboBox> combo_box("combo_box");
		LLDefaultWidgetRegistry::Register<LLFlyoutButton> flyout_button("flyout_button");
		LLDefaultWidgetRegistry::Register<LLContainerView> container_view("container_view");
		LLDefaultWidgetRegistry::Register<LLIconCtrl> icon("icon");
		LLDefaultWidgetRegistry::Register<LLLineEditor> line_editor("line_editor");
		LLDefaultWidgetRegistry::Register<LLSearchEditor> search_editor("search_editor");
		LLDefaultWidgetRegistry::Register<LLMenuItemSeparatorGL> menu_item_separator("menu_item_separator");
		LLDefaultWidgetRegistry::Register<LLMenuItemCallGL> menu_item_call_gl("menu_item_call");
		LLDefaultWidgetRegistry::Register<LLMenuItemCheckGL> menu_item_check_gl("menu_item_check");
		LLDefaultWidgetRegistry::Register<LLMenuGL> menu("menu");
		LLDefaultWidgetRegistry::Register<LLMenuBarGL> menu_bar("menu_bar");
		LLDefaultWidgetRegistry::Register<LLContextMenu> context_menu("context_menu");
		LLDefaultWidgetRegistry::Register<LLMultiSlider> multi_slider_bar("multi_slider_bar");
		LLDefaultWidgetRegistry::Register<LLMultiSliderCtrl> multi_slider("multi_slider");
		LLDefaultWidgetRegistry::Register<LLPanel> panel("panel", &LLPanel::fromXML);
		LLDefaultWidgetRegistry::Register<LLLayoutStack> layout_stack("layout_stack", &LLLayoutStack::fromXML);
		LLDefaultWidgetRegistry::Register<LLProgressBar> progress_bar("progress_bar");
		LLDefaultWidgetRegistry::Register<LLRadioGroup> radio_group("radio_group");
		LLDefaultWidgetRegistry::Register<LLRadioCtrl> radio_item("radio_item");
		LLDefaultWidgetRegistry::Register<LLScrollContainer> scroll_container("scroll_container");
		LLDefaultWidgetRegistry::Register<LLScrollingPanelList> scrolling_panel_list("scrolling_panel_list");
		LLDefaultWidgetRegistry::Register<LLScrollListCtrl> scroll_list("scroll_list");
		LLDefaultWidgetRegistry::Register<LLSlider> slider_bar("slider_bar");
		LLDefaultWidgetRegistry::Register<LLSliderCtrl> slider("slider");
		LLDefaultWidgetRegistry::Register<LLSpinCtrl> spinner("spinner");
		LLDefaultWidgetRegistry::Register<LLStatBar> stat_bar("stat_bar");
		//LLDefaultWidgetRegistry::Register<LLPlaceHolderPanel> placeholder("placeholder");
		LLDefaultWidgetRegistry::Register<LLTabContainer> tab_container("tab_container");
		LLDefaultWidgetRegistry::Register<LLTextBox> text("text");
		LLDefaultWidgetRegistry::Register<LLTextEditor> simple_text_editor("simple_text_editor");
		LLDefaultWidgetRegistry::Register<LLUICtrl> ui_ctrl("ui_ctrl");
		LLDefaultWidgetRegistry::Register<LLStatView> stat_view("stat_view");
		//LLDefaultWidgetRegistry::Register<LLUICtrlLocate> locate("locate");
		//LLDefaultWidgetRegistry::Register<LLUICtrlLocate> pad("pad");
		LLDefaultWidgetRegistry::Register<LLViewBorder> view_border("view_border");
	}

	// *HACK: Usually this is registered as a viewer text editor
	LLDefaultWidgetRegistry::Register<LLTextEditor> text_editor("text_editor");
}
