# -*- cmake -*-

project(llui)

include(00-Common)
include(LLAudio)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLWindow)
include(LLVFS)
include(LLXML)

include_directories(
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )

set(llui_SOURCE_FILES
    llalertdialog.cpp
    llbutton.cpp
    llcheckboxctrl.cpp
    llclipboard.cpp
    llcombobox.cpp
    llctrlselectioninterface.cpp
    lldraghandle.cpp
    lleditmenuhandler.cpp
    llfloater.cpp
    llfocusmgr.cpp
    llfunctorregistry.cpp
    lliconctrl.cpp
    llkeywords.cpp
    lllineeditor.cpp
    llmenugl.cpp
    llmodaldialog.cpp
    llmultislider.cpp
    llmultisliderctrl.cpp
    llnotifications.cpp
    llpanel.cpp
    llprogressbar.cpp
    llradiogroup.cpp
    llresizebar.cpp
    llresizehandle.cpp
    llresmgr.cpp
    llrootview.cpp
    llscrollbar.cpp
    llscrollcontainer.cpp
    llscrollingpanellist.cpp
    llscrolllistctrl.cpp
    llslider.cpp
    llsliderctrl.cpp
    llspinctrl.cpp
    llstyle.cpp
    lltabcontainer.cpp
    lltabcontainervertical.cpp
    lltextbox.cpp
    lltexteditor.cpp
    lltextparser.cpp
    llui.cpp
    lluictrl.cpp
    lluictrlfactory.cpp
    lluistring.cpp
    lluitrans.cpp
    llundo.cpp
    llviewborder.cpp
    llview.cpp
    llviewquery.cpp
    )
    
set(llui_HEADER_FILES
    CMakeLists.txt

    llalertdialog.h
    llbutton.h
    llcallbackmap.h
    llcheckboxctrl.h
    llclipboard.h
    llcombobox.h
    llctrlselectioninterface.h
    lldraghandle.h
    lleditmenuhandler.h
    llfloater.h
    llfocusmgr.h
    llfunctorregistry.h
    llhtmlhelp.h
    lliconctrl.h
    llkeywords.h
    lllineeditor.h
    llmemberlistener.h
    llmenugl.h
    llmodaldialog.h
    llmultisliderctrl.h
    llmultislider.h
    llnotifications.h
    llpanel.h
    llprogressbar.h
    llradiogroup.h
    llresizebar.h
    llresizehandle.h
    llresmgr.h
    llrootview.h
    llscrollbar.h
    llscrollcontainer.h
    llscrollingpanellist.h
    llscrolllistctrl.h
    llsliderctrl.h
    llslider.h
    llspinctrl.h
    llstyle.h
    lltabcontainer.h
    lltabcontainervertical.h
    lltextbox.h
    lltexteditor.h
    lltextparser.h
    lluiconstants.h
    lluictrlfactory.h
    lluictrl.h
    lluifwd.h
    llui.h
    lluistring.h
    lluitrans.h
    lluixmltags.h
    llundo.h
    llviewborder.h
    llview.h
    llviewquery.h
    )

set_source_files_properties(${llui_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llui_SOURCE_FILES ${llui_HEADER_FILES})

add_library (llui ${llui_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llui
    llrender
    llwindow
    llimage
    llvfs       # ugh, just for LLDir
    llxml
    llcommon    # must be after llimage, llwindow, llrender
    llmath
    )
