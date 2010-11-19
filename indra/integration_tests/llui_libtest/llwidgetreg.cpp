/** 
 * @file llwidgetreg.cpp
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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
#include "linden_common.h"

#include "llwidgetreg.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llcontainerview.h"
#include "lliconctrl.h"
#include "llloadingindicator.h"
#include "llmenubutton.h"
#include "llmenugl.h"
#include "llmultislider.h"
#include "llmultisliderctrl.h"
#include "llprogressbar.h"
#include "llradiogroup.h"
#include "llsearcheditor.h"
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
#include "llfiltereditor.h"
#include "lllayoutstack.h"

void LLWidgetReg::initClass(bool register_widgets)
{
	// Only need to register if the Windows linker has optimized away the
	// references to the object files.
	if (register_widgets)
	{
		LLDefaultChildRegistry::Register<LLButton> button("button");
		LLDefaultChildRegistry::Register<LLMenuButton> menu_button("menu_button");
		LLDefaultChildRegistry::Register<LLCheckBoxCtrl> check_box("check_box");
		LLDefaultChildRegistry::Register<LLComboBox> combo_box("combo_box");
		LLDefaultChildRegistry::Register<LLFilterEditor> filter_editor("filter_editor");
		LLDefaultChildRegistry::Register<LLFlyoutButton> flyout_button("flyout_button");
		LLDefaultChildRegistry::Register<LLContainerView> container_view("container_view");
		LLDefaultChildRegistry::Register<LLIconCtrl> icon("icon");
		LLDefaultChildRegistry::Register<LLLoadingIndicator> loading_indicator("loading_indicator");
		LLDefaultChildRegistry::Register<LLLineEditor> line_editor("line_editor");
		LLDefaultChildRegistry::Register<LLMenuItemSeparatorGL> menu_item_separator("menu_item_separator");
		LLDefaultChildRegistry::Register<LLMenuItemCallGL> menu_item_call_gl("menu_item_call");
		LLDefaultChildRegistry::Register<LLMenuItemCheckGL> menu_item_check_gl("menu_item_check");
		LLDefaultChildRegistry::Register<LLMenuGL> menu("menu");
		LLDefaultChildRegistry::Register<LLMenuBarGL> menu_bar("menu_bar");
		LLDefaultChildRegistry::Register<LLContextMenu> context_menu("context_menu");
		LLDefaultChildRegistry::Register<LLMultiSlider> multi_slider_bar("multi_slider_bar");
		LLDefaultChildRegistry::Register<LLMultiSliderCtrl> multi_slider("multi_slider");
		LLDefaultChildRegistry::Register<LLPanel> panel("panel", &LLPanel::fromXML);
		LLDefaultChildRegistry::Register<LLLayoutStack> layout_stack("layout_stack");
		LLDefaultChildRegistry::Register<LLProgressBar> progress_bar("progress_bar");
		LLDefaultChildRegistry::Register<LLRadioGroup> radio_group("radio_group");
		LLDefaultChildRegistry::Register<LLSearchEditor> search_editor("search_editor");
		LLDefaultChildRegistry::Register<LLScrollContainer> scroll_container("scroll_container");
		LLDefaultChildRegistry::Register<LLScrollingPanelList> scrolling_panel_list("scrolling_panel_list");
		LLDefaultChildRegistry::Register<LLScrollListCtrl> scroll_list("scroll_list");
		LLDefaultChildRegistry::Register<LLSlider> slider_bar("slider_bar");
		LLDefaultChildRegistry::Register<LLSliderCtrl> slider("slider");
		LLDefaultChildRegistry::Register<LLSpinCtrl> spinner("spinner");
		LLDefaultChildRegistry::Register<LLStatBar> stat_bar("stat_bar");
		//LLDefaultChildRegistry::Register<LLPlaceHolderPanel> placeholder("placeholder");
		LLDefaultChildRegistry::Register<LLTabContainer> tab_container("tab_container");
		LLDefaultChildRegistry::Register<LLTextBox> text("text");
		LLDefaultChildRegistry::Register<LLTextEditor> simple_text_editor("simple_text_editor");
		LLDefaultChildRegistry::Register<LLUICtrl> ui_ctrl("ui_ctrl");
		LLDefaultChildRegistry::Register<LLStatView> stat_view("stat_view");
		//LLDefaultChildRegistry::Register<LLUICtrlLocate> locate("locate");
		//LLDefaultChildRegistry::Register<LLUICtrlLocate> pad("pad");
		LLDefaultChildRegistry::Register<LLViewBorder> view_border("view_border");
	}

	// *HACK: Usually this is registered as a viewer text editor
	LLDefaultChildRegistry::Register<LLTextEditor> text_editor("text_editor");
}
