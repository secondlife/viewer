# -*- cmake -*-

project(llui)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLWindow)
include(LLVFS)
include(LLXML)
include(LLXUIXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LLXUIXML_INCLUDE_DIRS}
    )

set(llui_SOURCE_FILES
    llaccordionctrl.cpp
    llaccordionctrltab.cpp
    llbutton.cpp
    llcheckboxctrl.cpp
    llclipboard.cpp
    llcombobox.cpp
    llconsole.cpp
    llcontainerview.cpp
    llctrlselectioninterface.cpp
    lldockablefloater.cpp
    lldockcontrol.cpp
    lldraghandle.cpp
    lleditmenuhandler.cpp
    llf32uictrl.cpp
    llfiltereditor.cpp
    llflatlistview.cpp
    llfloater.cpp
    llfloaterreg.cpp
    llfloaterreglistener.cpp
    llflyoutbutton.cpp 
    llfocusmgr.cpp
    llfunctorregistry.cpp
    lliconctrl.cpp
    llkeywords.cpp
    lllayoutstack.cpp
    lllineeditor.cpp
    llloadingindicator.cpp
    lllocalcliprect.cpp
    llmenubutton.cpp
    llmenugl.cpp
    llmodaldialog.cpp
    llmultifloater.cpp 
    llmultislider.cpp
    llmultisliderctrl.cpp
    llnotifications.cpp
    llnotificationslistener.cpp
    llnotificationsutil.cpp
    llpanel.cpp
    llprogressbar.cpp
    llradiogroup.cpp
    llresizebar.cpp
    llresizehandle.cpp
    llresmgr.cpp
    llrngwriter.cpp
    llscrollbar.cpp
    llscrollcontainer.cpp
    llscrollingpanellist.cpp
    llscrolllistcell.cpp
    llscrolllistcolumn.cpp
    llscrolllistctrl.cpp
    llscrolllistitem.cpp
    llsdparam.cpp
    llsearcheditor.cpp
    llslider.cpp
    llsliderctrl.cpp
    llspinctrl.cpp
    llstatbar.cpp
    llstatgraph.cpp
    llstatview.cpp
    llstyle.cpp
    lltabcontainer.cpp
    lltextbase.cpp
    lltextbox.cpp
    lltexteditor.cpp
    lltextparser.cpp
    lltextutil.cpp
    lltextvalidate.cpp
    lltransutil.cpp
    lltoggleablemenu.cpp
    lltooltip.cpp
    llui.cpp
    lluicolortable.cpp
    lluictrl.cpp
    lluictrlfactory.cpp
    lluiimage.cpp
    lluistring.cpp
    llundo.cpp
    llurlaction.cpp
    llurlentry.cpp
    llurlmatch.cpp
    llurlregistry.cpp
    llviewborder.cpp
    llviewmodel.cpp
    llview.cpp
    llviewquery.cpp
    )
    
set(llui_HEADER_FILES
    CMakeLists.txt

    llaccordionctrl.h
    llaccordionctrltab.h
    llbutton.h
    llcallbackmap.h
    llcheckboxctrl.h
    llclipboard.h
    llcombobox.h
    llconsole.h
    llcontainerview.h
    llctrlselectioninterface.h
    lldraghandle.h
    lldockablefloater.h
    lldockcontrol.h
    lleditmenuhandler.h
    llf32uictrl.h
    llfiltereditor.h 
    llflatlistview.h
    llfloater.h
    llfloaterreg.h
    llfloaterreglistener.h
    llflyoutbutton.h 
    llfocusmgr.h
    llfunctorregistry.h
    llhandle.h
    llhelp.h
    lliconctrl.h
    llkeywords.h
    lllayoutstack.h
    lllazyvalue.h
    lllineeditor.h
    llloadingindicator.h
    lllocalcliprect.h
    llmenubutton.h
    llmenugl.h
    llmodaldialog.h
    llmultifloater.h 
    llmultisliderctrl.h
    llmultislider.h
    llnotificationptr.h
    llnotifications.h
    llnotificationslistener.h
    llnotificationsutil.h
    llnotificationtemplate.h
	llnotificationvisibilityrule.h
    llpanel.h
    llprogressbar.h
    llradiogroup.h
    llresizebar.h
    llresizehandle.h
    llresmgr.h
    llrngwriter.h
    llsearcheditor.h 
    llscrollbar.h
    llscrollcontainer.h
    llscrollingpanellist.h
    llscrolllistcell.h
    llscrolllistcolumn.h
    llscrolllistctrl.h
    llscrolllistitem.h
    llsdparam.h
    llsliderctrl.h
    llslider.h
    llspinctrl.h
    llstatbar.h
    llstatgraph.h
    llstatview.h
    llstyle.h
    lltabcontainer.h
    lltextbase.h
    lltextbox.h
    lltexteditor.h
    lltextparser.h
    lltextutil.h
    lltextvalidate.h
    lltoggleablemenu.h
    lltooltip.h
    lltransutil.h
    lluicolortable.h
    lluiconstants.h
    lluictrlfactory.h
    lluictrl.h
    lluifwd.h
    llui.h
    lluiimage.h
    lluistring.h
    llundo.h
    llurlaction.h
    llurlentry.h
    llurlmatch.h
    llurlregistry.h
    llviewborder.h
    llviewmodel.h
    llview.h
    llviewquery.h
    )

set_source_files_properties(${llui_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

SET(llurlentry_TEST_DEPENDENCIES
    llurlmatch.cpp
    llurlregistry.cpp
    )

set_source_files_properties(llurlentry.cpp
    PROPERTIES LL_TEST_ADDITIONAL_SOURCE_FILES
    "${llurlentry_TEST_DEPENDENCIES}"
    )

list(APPEND llui_SOURCE_FILES ${llui_HEADER_FILES})

add_library (llui ${llui_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llui
    ${LLMESSAGE_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    ${LLWINDOW_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLVFS_LIBRARIES}    # ugh, just for LLDir
    ${LLXUIXML_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES} # must be after llimage, llwindow, llrender
    )

# Add tests
if (LL_TESTS)
	include(LLAddBuildTest)
	SET(llui_TEST_SOURCE_FILES
		llurlmatch.cpp
		llurlentry.cpp
		)
	LL_ADD_PROJECT_UNIT_TESTS(llui "${llui_TEST_SOURCE_FILES}")
endif (LL_TESTS)