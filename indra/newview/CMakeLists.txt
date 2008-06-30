# -*- cmake -*-

project(viewer)

include(00-Common)
include(Boost)
include(BuildVersion)
include(DirectX)
include(ELFIO)
include(FMOD)
include(FindOpenGL)
include(LLAudio)
include(LLCharacter)
include(LLCommon)
include(LLImage)
include(LLImageJ2COJ)
include(LLInventory)
include(LLMath)
include(LLMedia)
include(LLMessage)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(LScript)
include(Linking)
include(Mozlib)
include(NDOF)
include(TemplateCheck)
include(UI)
include(UnixInstall)
include(LLKDU)
include(ViewerMiscLibs)

if (WINDOWS)
    include(CopyWinLibs)
endif (WINDOWS)

include_directories(
    ${ELFIO_INCLUDE_DIR}
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMEDIA_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLPRIMITIVE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}/lscript_compile
    )

set(viewer_SOURCE_FILES
    llagent.cpp
    llagentdata.cpp
    llagentlanguage.cpp
    llagentpilot.cpp
    llanimstatelabels.cpp
    llappviewer.cpp
    llassetuploadresponders.cpp
    llaudiosourcevo.cpp
    llbbox.cpp
    llbox.cpp
    llcallbacklist.cpp
    llcallingcard.cpp
    llcaphttpsender.cpp
    llchatbar.cpp
    llclassifiedinfo.cpp
    llclassifiedstatsresponder.cpp
    llcloud.cpp
    llcolorscheme.cpp
    llcolorswatch.cpp
    llcommandhandler.cpp
    llcommandlineparser.cpp
    llcompass.cpp
    llcompilequeue.cpp
    llconfirmationmanager.cpp
    llconsole.cpp
    llcontainerview.cpp
    llcurrencyuimanager.cpp
    llcylinder.cpp
    lldebugmessagebox.cpp
    lldebugview.cpp
    lldelayedgestureerror.cpp
    lldirpicker.cpp
    lldrawable.cpp
    lldrawpoolalpha.cpp
    lldrawpoolavatar.cpp
    lldrawpoolbump.cpp
    lldrawpool.cpp
    lldrawpoolground.cpp
    lldrawpoolsimple.cpp
    lldrawpoolsky.cpp
    lldrawpoolterrain.cpp
    lldrawpooltree.cpp
    lldrawpoolwater.cpp
    lldrawpoolwlsky.cpp
    lldriverparam.cpp
    lldynamictexture.cpp
    llemote.cpp
    lleventinfo.cpp
    lleventnotifier.cpp
    lleventpoll.cpp
    llface.cpp
    llfasttimerview.cpp
    llfeaturemanager.cpp
    llfilepicker.cpp
    llfirstuse.cpp
    llflexibleobject.cpp
    llfloaterabout.cpp
    llfloateractivespeakers.cpp
    llfloateranimpreview.cpp
    llfloaterauction.cpp
    llfloateravatarinfo.cpp
    llfloateravatarpicker.cpp
    llfloateravatartextures.cpp
    llfloaterbuildoptions.cpp
    llfloaterbump.cpp
    llfloaterbuycontents.cpp
    llfloaterbuy.cpp
    llfloaterbuycurrency.cpp
    llfloaterbuyland.cpp
    llfloatercamera.cpp
    llfloaterchat.cpp
    llfloaterchatterbox.cpp
    llfloaterclassified.cpp
    llfloaterclothing.cpp
    llfloatercolorpicker.cpp
    llfloatercustomize.cpp
    llfloaterdaycycle.cpp
    llfloaterdirectory.cpp
    llfloatereditui.cpp
    llfloaterenvsettings.cpp
    llfloaterevent.cpp
    llfloaterfriends.cpp
    llfloatergesture.cpp
    llfloatergodtools.cpp
    llfloatergroupinfo.cpp
    llfloatergroupinvite.cpp
    llfloatergroups.cpp
    llfloaterhardwaresettings.cpp
    llfloaterhtml.cpp
    llfloaterhtmlhelp.cpp
    llfloaterhud.cpp
    llfloaterimagepreview.cpp
    llfloaterinspect.cpp
    llfloaterjoystick.cpp
    llfloaterlagmeter.cpp
    llfloaterland.cpp
    llfloaterlandholdings.cpp
    llfloaterlandmark.cpp
    llfloatermap.cpp
    llfloatermute.cpp
    llfloaternamedesc.cpp
    llfloaternewim.cpp
    llfloateropenobject.cpp
    llfloaterparcel.cpp
    llfloaterpermissionsmgr.cpp
    llfloaterpostcard.cpp
    llfloaterpostprocess.cpp
    llfloaterpreference.cpp
    llfloaterproperties.cpp
    llfloaterregioninfo.cpp
    llfloaterreleasemsg.cpp
    llfloaterreporter.cpp
    llfloaterscriptdebug.cpp
    llfloatersellland.cpp
    llfloatersettingsdebug.cpp
    llfloatersnapshot.cpp
    llfloaterstats.cpp
    llfloatertelehub.cpp
    llfloatertest.cpp
    llfloatertools.cpp
    llfloatertopobjects.cpp
    llfloatertos.cpp
    llfloaterurldisplay.cpp
    llfloaterurlentry.cpp
    llfloatervoicedevicesettings.cpp
    llfloaterwater.cpp
    llfloaterwindlight.cpp
    llfloaterworldmap.cpp
    llfolderview.cpp
    llfollowcam.cpp
    llframestats.cpp
    llframestatview.cpp
    llgenepool.cpp
    llgesturemgr.cpp
    llgivemoney.cpp
    llglsandbox.cpp
    llgroupmgr.cpp
    llgroupnotify.cpp
    llhoverview.cpp
    llhudeffectbeam.cpp
    llhudeffect.cpp
    llhudeffectlookat.cpp
    llhudeffectpointat.cpp
    llhudeffecttrail.cpp
    llhudicon.cpp
    llhudmanager.cpp
    llhudobject.cpp
    llhudrender.cpp
    llhudtext.cpp
    llhudview.cpp
    llimpanel.cpp
    llimview.cpp
    llinventoryactions.cpp
    llinventorybridge.cpp
    llinventoryclipboard.cpp
    llinventorymodel.cpp
    llinventoryview.cpp
    lljoystickbutton.cpp
    lllandmarklist.cpp
    lllogchat.cpp
    llmanip.cpp
    llmaniprotate.cpp
    llmanipscale.cpp
    llmaniptranslate.cpp
    llmapresponders.cpp
    llmediaremotectrl.cpp
    llmemoryview.cpp
    llmenucommands.cpp
    llmimetypes.cpp
    llmorphview.cpp
    llmoveview.cpp
    llmutelist.cpp
    llnamebox.cpp
    llnameeditor.cpp
    llnamelistctrl.cpp
    llnetmap.cpp
    llnotify.cpp
    lloverlaybar.cpp
    llpanelaudioprefs.cpp
    llpanelaudiovolume.cpp
    llpanelavatar.cpp
    llpanelclassified.cpp
    llpanelcontents.cpp
    llpaneldebug.cpp
    llpaneldirbrowser.cpp
    llpaneldirclassified.cpp
    llpaneldirevents.cpp
    llpaneldirfind.cpp
    llpaneldirgroups.cpp
    llpaneldirland.cpp
    llpaneldirpeople.cpp
    llpaneldirplaces.cpp
    llpaneldirpopular.cpp
    llpaneldisplay.cpp
    llpanelevent.cpp
    llpanelface.cpp
    llpanelgeneral.cpp
    llpanelgroup.cpp
    llpanelgroupgeneral.cpp
    llpanelgroupinvite.cpp
    llpanelgrouplandmoney.cpp
    llpanelgroupnotices.cpp
    llpanelgrouproles.cpp
    llpanelgroupvoting.cpp
    llpanelinput.cpp
    llpanelinventory.cpp
    llpanelland.cpp
    llpanellandmedia.cpp
    llpanellandobjects.cpp
    llpanellandoptions.cpp
    llpanellogin.cpp
    llpanelmorph.cpp
    llpanelmsgs.cpp
    llpanelnetwork.cpp
    llpanelobject.cpp
    llpanelpermissions.cpp
    llpanelpick.cpp
    llpanelplace.cpp
    llpanelvolume.cpp
    llpanelweb.cpp
    llparcelselection.cpp
    llpatchvertexarray.cpp
    llpolymesh.cpp
    llpolymorph.cpp
    llprefschat.cpp
    llprefsim.cpp
    llprefsvoice.cpp
    llpreviewanim.cpp
    llpreview.cpp
    llpreviewgesture.cpp
    llpreviewlandmark.cpp
    llpreviewnotecard.cpp
    llpreviewscript.cpp
    llpreviewsound.cpp
    llpreviewtexture.cpp
    llprogressview.cpp
    llregionposition.cpp
    llremoteparcelrequest.cpp
    llsavedsettingsglue.cpp
    llselectmgr.cpp
    llsky.cpp
    llspatialpartition.cpp
    llsprite.cpp
    llsrv.cpp
    llstartup.cpp
    llstatbar.cpp
    llstatgraph.cpp
    llstatusbar.cpp
    llstatview.cpp
    llstylemap.cpp
    llsurface.cpp
    llsurfacepatch.cpp
    lltexlayer.cpp
    lltexturecache.cpp
    lltexturectrl.cpp
    lltexturefetch.cpp
    lltextureview.cpp
    lltoolbar.cpp
    lltoolbrush.cpp
    lltoolcomp.cpp
    lltool.cpp
    lltooldraganddrop.cpp
    lltoolface.cpp
    lltoolfocus.cpp
    lltoolgrab.cpp
    lltoolgun.cpp
    lltoolindividual.cpp
    lltoolmgr.cpp
    lltoolmorph.cpp
    lltoolobjpicker.cpp
    lltoolpie.cpp
    lltoolpipette.cpp
    lltoolplacer.cpp
    lltoolselect.cpp
    lltoolselectland.cpp
    lltoolselectrect.cpp
    lltoolview.cpp
    lltracker.cpp
    lltrans.cpp
    lluploaddialog.cpp
    llurl.cpp
    llurldispatcher.cpp
    llurlhistory.cpp
    llurlsimstring.cpp
    llurlwhitelist.cpp
    lluserauth.cpp
    llvectorperfoptions.cpp
    llvelocitybar.cpp
    llviewchildren.cpp
    llviewerassetstorage.cpp
    llvieweraudio.cpp
    llviewercamera.cpp
    llviewercontrol.cpp
    llviewerdisplay.cpp
    llviewergenericmessage.cpp
    llviewergesture.cpp
    llviewerimage.cpp
    llviewerimagelist.cpp
    llviewerinventory.cpp
    llviewerjointattachment.cpp
    llviewerjoint.cpp
    llviewerjointmesh.cpp
    llviewerjointmesh_sse2.cpp
    llviewerjointmesh_sse.cpp
    llviewerjointmesh_vec.cpp
    llviewerjoystick.cpp
    llviewerkeyboard.cpp
    llviewerlayer.cpp
    llviewermedia.cpp
    llviewermenu.cpp
    llviewermenufile.cpp
    llviewermessage.cpp
    llviewernetwork.cpp
    llviewerobject.cpp
    llviewerobjectlist.cpp
    llviewerparcelmedia.cpp
    llviewerparcelmediaautoplay.cpp
    llviewerparcelmgr.cpp
    llviewerparceloverlay.cpp
    llviewerpartsim.cpp
    llviewerpartsource.cpp
    llviewerregion.cpp
    llviewershadermgr.cpp
    llviewerstats.cpp
    llviewertexteditor.cpp
    llviewertextureanim.cpp
    llviewerthrottle.cpp
    llviewervisualparam.cpp
    llviewerwindow.cpp
    llvlcomposition.cpp
    llvlmanager.cpp
    llvoavatar.cpp
    llvocache.cpp
    llvoclouds.cpp
    llvograss.cpp
    llvoground.cpp
    llvoiceclient.cpp
    llvoiceremotectrl.cpp
    llvoicevisualizer.cpp
    llvoinventorylistener.cpp
    llvopartgroup.cpp
    llvosky.cpp
    llvosurfacepatch.cpp
    llvotextbubble.cpp
    llvotree.cpp
    llvovolume.cpp
    llvowater.cpp
    llvowlsky.cpp
    llwatchdog.cpp
    llwaterparammanager.cpp
    llwaterparamset.cpp
    llwearable.cpp
    llwearablelist.cpp
    llweb.cpp
    llwebbrowserctrl.cpp
    llwind.cpp
    llwlanimator.cpp
    llwldaycycle.cpp
    llwlparammanager.cpp
    llwlparamset.cpp
    llworld.cpp
    llworldmap.cpp
    llworldmapview.cpp
    llxmlrpctransaction.cpp
    noise.cpp
    pipeline.cpp
    )

if (LINUX)
  # We can't set these flags for Darwin, because they get passed to
  # the PPC compiler.  Ugh.

  set_source_files_properties(
      llviewerjointmesh_sse.cpp
      PROPERTIES COMPILE_FLAGS "-msse -mfpmath=sse"
      )
  set_source_files_properties(
      llviewerjointmesh_sse2.cpp
      PROPERTIES COMPILE_FLAGS "-msse2 -mfpmath=sse"
      )
endif (LINUX)

set(viewer_HEADER_FILES
    CMakeLists.txt
    ViewerInstall.cmake

    llagent.h
    llagentdata.h
    llagentlanguage.h
    llagentpilot.h
    llanimstatelabels.h
    llappearance.h
    llappviewer.h
    llassetuploadresponders.h
    llaudiosourcevo.h
    llbbox.h
    llbox.h
    llcallbacklist.h
    llcallingcard.h
    llcaphttpsender.h
    llchatbar.h
    llclassifiedinfo.h
    llclassifiedstatsresponder.h
    llcloud.h
    llcolorscheme.h
    llcolorswatch.h
    llcommandhandler.h
    llcommandlineparser.h
    llcompass.h
    llcompilequeue.h
    llconfirmationmanager.h
    llconsole.h
    llcontainerview.h
    llcurrencyuimanager.h
    llcylinder.h
    lldebugmessagebox.h
    lldebugview.h
    lldelayedgestureerror.h
    lldirpicker.h
    lldrawable.h
    lldrawpool.h
    lldrawpoolalpha.h
    lldrawpoolavatar.h
    lldrawpoolbump.h
    lldrawpoolclouds.h
    lldrawpoolground.h
    lldrawpoolsimple.h
    lldrawpoolsky.h
    lldrawpoolterrain.h
    lldrawpooltree.h
    lldrawpoolwater.h
    lldrawpoolwlsky.h
    lldriverparam.h
    lldynamictexture.h
    llemote.h
    lleventinfo.h
    lleventnotifier.h
    lleventpoll.h
    llface.h
    llfasttimerview.h
    llfeaturemanager.h
    llfilepicker.h
    llfirstuse.h
    llflexibleobject.h
    llfloaterabout.h
    llfloateractivespeakers.h
    llfloateranimpreview.h
    llfloaterauction.h
    llfloateravatarinfo.h
    llfloateravatarpicker.h
    llfloateravatartextures.h
    llfloaterbuildoptions.h
    llfloaterbump.h
    llfloaterbuy.h
    llfloaterbuycontents.h
    llfloaterbuycurrency.h
    llfloaterbuyland.h
    llfloatercamera.h
    llfloaterchat.h
    llfloaterchatterbox.h
    llfloaterclassified.h
    llfloaterclothing.h
    llfloatercolorpicker.h
    llfloatercustomize.h
    llfloaterdaycycle.h
    llfloaterdirectory.h
    llfloatereditui.h
    llfloaterenvsettings.h
    llfloaterevent.h
    llfloaterfriends.h
    llfloatergesture.h
    llfloatergodtools.h
    llfloatergroupinfo.h
    llfloatergroupinvite.h
    llfloatergroups.h
    llfloaterhardwaresettings.h
    llfloaterhtml.h
    llfloaterhtmlhelp.h
    llfloaterhud.h
    llfloaterimagepreview.h
    llfloaterinspect.h
    llfloaterjoystick.h
    llfloaterlagmeter.h
    llfloaterland.h
    llfloaterlandholdings.h
    llfloaterlandmark.h
    llfloatermap.h
    llfloatermute.h
    llfloaternamedesc.h
    llfloaternewim.h
    llfloateropenobject.h
    llfloaterparcel.h
    llfloaterpermissionsmgr.h
    llfloaterpostcard.h
    llfloaterpostprocess.h
    llfloaterpreference.h
    llfloaterproperties.h
    llfloaterregioninfo.h
    llfloaterreleasemsg.h
    llfloaterreporter.h
    llfloaterscriptdebug.h
    llfloatersellland.h
    llfloatersettingsdebug.h
    llfloatersnapshot.h
    llfloaterstats.h
    llfloatertelehub.h
    llfloatertest.h
    llfloatertools.h
    llfloatertopobjects.h
    llfloatertos.h
    llfloaterurldisplay.h
    llfloaterurlentry.h
    llfloatervoicedevicesettings.h
    llfloaterwater.h
    llfloaterwindlight.h
    llfloaterworldmap.h
    llfolderview.h
    llfollowcam.h
    llframestats.h
    llframestatview.h
    llgenepool.h
    llgesturemgr.h
    llgivemoney.h
    llgroupmgr.h
    llgroupnotify.h
    llhoverview.h
    llhudeffect.h
    llhudeffectbeam.h
    llhudeffectlookat.h
    llhudeffectpointat.h
    llhudeffecttrail.h
    llhudicon.h
    llhudmanager.h
    llhudobject.h
    llhudrender.h
    llhudtext.h
    llhudview.h
    llimpanel.h
    llimview.h
    llinventorybridge.h
    llinventoryclipboard.h
    llinventorymodel.h
    llinventoryview.h
    lljoystickbutton.h
    lllandmarklist.h
    lllightconstants.h
    lllogchat.h
    llmanip.h
    llmaniprotate.h
    llmanipscale.h
    llmaniptranslate.h
    llmapresponders.h
    llmediaremotectrl.h
    llmemoryview.h
    llmenucommands.h
    llmimetypes.h
    llmorphview.h
    llmoveview.h
    llmutelist.h
    llnamebox.h
    llnameeditor.h
    llnamelistctrl.h
    llnetmap.h
    llnotify.h
    lloverlaybar.h
    llpanelaudioprefs.h
    llpanelaudiovolume.h
    llpanelavatar.h
    llpanelclassified.h
    llpanelcontents.h
    llpaneldebug.h
    llpaneldirbrowser.h
    llpaneldirclassified.h
    llpaneldirevents.h
    llpaneldirfind.h
    llpaneldirgroups.h
    llpaneldirland.h
    llpaneldirpeople.h
    llpaneldirplaces.h
    llpaneldirpopular.h
    llpaneldisplay.h
    llpanelevent.h
    llpanelface.h
    llpanelgeneral.h
    llpanelgroup.h
    llpanelgroupgeneral.h
    llpanelgroupinvite.h
    llpanelgrouplandmoney.h
    llpanelgroupnotices.h
    llpanelgrouproles.h
    llpanelgroupvoting.h
    llpanelinput.h
    llpanelinventory.h
    llpanelland.h
    llpanellandmedia.h
    llpanellandobjects.h
    llpanellandoptions.h
    llpanelLCD.h
    llpanellogin.h
    llpanelmorph.h
    llpanelmsgs.h
    llpanelnetwork.h
    llpanelobject.h
    llpanelpermissions.h
    llpanelpick.h
    llpanelplace.h
    llpanelvolume.h
    llpanelweb.h
    llparcelselection.h
    llpatchvertexarray.h
    llpolymesh.h
    llpolymorph.h
    llprefschat.h
    llprefsim.h
    llprefsvoice.h
    llpreview.h
    llpreviewanim.h
    llpreviewgesture.h
    llpreviewlandmark.h
    llpreviewnotecard.h
    llpreviewscript.h
    llpreviewsound.h
    llpreviewtexture.h
    llprogressview.h
    llregionposition.h
    llremoteparcelrequest.h
    llresourcedata.h
    llsavedsettingsglue.h
    llselectmgr.h
    llsky.h
    llspatialpartition.h
    llsprite.h
    llsrv.h
    llstartup.h
    llstatbar.h
    llstatgraph.h
    llstatusbar.h
    llstatview.h
    llstylemap.h
    llsurface.h
    llsurfacepatch.h
    lltable.h
    lltexlayer.h
    lltexturecache.h
    lltexturectrl.h
    lltexturefetch.h
    lltextureview.h
    lltool.h
    lltoolbar.h
    lltoolbrush.h
    lltoolcomp.h
    lltooldraganddrop.h
    lltoolface.h
    lltoolfocus.h
    lltoolgrab.h
    lltoolgun.h
    lltoolindividual.h
    lltoolmgr.h
    lltoolmorph.h
    lltoolobjpicker.h
    lltoolpie.h
    lltoolpipette.h
    lltoolplacer.h
    lltoolselect.h
    lltoolselectland.h
    lltoolselectrect.h
    lltoolview.h
    lltracker.h
    lltrans.h
    lluiconstants.h
    lluploaddialog.h
    llurl.h
    llurldispatcher.h
    llurlhistory.h
    llurlsimstring.h
    llurlwhitelist.h
    lluserauth.h
    llvectorperfoptions.h
    llvelocitybar.h
    llviewchildren.h
    llviewerassetstorage.h
    llvieweraudio.h
    llviewerbuild.h
    llviewercamera.h
    llviewercontrol.h
    llviewerdisplay.h
    llviewergenericmessage.h
    llviewergesture.h
    llviewerimage.h
    llviewerimagelist.h
    llviewerinventory.h
    llviewerjoint.h
    llviewerjointattachment.h
    llviewerjointmesh.h
    llviewerjoystick.h
    llviewerkeyboard.h
    llviewerlayer.h
    llviewermedia.h
    llviewermenu.h
    llviewermenufile.h
    llviewermessage.h
    llviewernetwork.h
    llviewerobject.h
    llviewerobjectlist.h
    llviewerparcelmedia.h
    llviewerparcelmediaautoplay.h
    llviewerparcelmgr.h
    llviewerparceloverlay.h
    llviewerpartsim.h
    llviewerpartsource.h
    llviewerprecompiledheaders.h
    llviewerregion.h
    llviewershadermgr.h
    llviewerstats.h
    llviewertexteditor.h
    llviewertextureanim.h
    llviewerthrottle.h
    llviewervisualparam.h
    llviewerwindow.h
    llvlcomposition.h
    llvlmanager.h
    llvoavatar.h
    llvocache.h
    llvoclouds.h
    llvograss.h
    llvoground.h
    llvoiceclient.h
    llvoiceremotectrl.h
    llvoicevisualizer.h
    llvoinventorylistener.h
    llvopartgroup.h
    llvosky.h
    llvosurfacepatch.h
    llvotextbubble.h
    llvotree.h
    llvotreenew.h
    llvovolume.h
    llvowater.h
    llvowlsky.h
    llwatchdog.h
    llwaterparammanager.h
    llwaterparamset.h
    llwearable.h
    llwearablelist.h
    llweb.h
    llwebbrowserctrl.h
    llwind.h
    llwindebug.h
    llwlanimator.h
    llwldaycycle.h
    llwlparammanager.h
    llwlparamset.h
    llworld.h
    llworldmap.h
    llworldmapview.h
    llxmlrpctransaction.h
    macmain.h
    noise.h
    pipeline.h
    randgauss.h
    VertexCache.h
    VorbisFramework.h
    )

source_group("CMake Rules" FILES ViewerInstall.cmake)

if (DARWIN)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx.cpp)

  find_library(AGL_LIBRARY AGL)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)

  set(viewer_LIBRARIES
    ${COCOA_LIBRARY}
    ${AGL_LIBRARY}
    ${IOKIT_LIBRARY}
    )

  # Add resource files to the project.
  set(viewer_RESOURCE_FILES
    secondlife.icns
    macview.r
    gpu_table.txt
    Info-SecondLife.plist
    SecondLife.nib/
    # CMake doesn't seem to support Xcode language variants well just yet
    English.lproj/InfoPlist.strings
    English.lproj/language.txt
    German.lproj/language.txt
    Japanese.lproj/language.txt
    Korean.lproj/language.txt
    )
  set_source_files_properties(
    ${viewer_RESOURCE_FILES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
    #MACOSX_PACKAGE_LOCATION Resources #don't do this! this tells cmake to copy the files.
    )
  SOURCE_GROUP("Resources" FILES ${viewer_RESOURCE_FILES})
  list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})
endif (DARWIN)

if (LINUX)
    LIST(APPEND viewer_SOURCE_FILES llappviewerlinux.cpp)
    LIST(APPEND CMAKE_EXE_LINKER_FLAGS -Wl,--as-needed)

    set(viewer_LIBRARIES
        Xinerama
        )
endif (LINUX)

if (WINDOWS)
    list(APPEND viewer_SOURCE_FILES
         llappviewerwin32.cpp
         llwindebug.cpp
         )

    list(APPEND viewer_HEADER_FILES
         llappviewerwin32.h
         llwindebug.h
         )

    # precompiled header configuration
    # llviewerprecompiledheaders.cpp generates
    # the .pch file.
    # All sources added to viewer_SOURCE_FILES 
    # at this point use it.
    set_source_files_properties(llviewerprecompiledheaders.cpp
        PROPERTIES
        COMPILE_FLAGS "/Ycllviewerprecompiledheaders.h"
        )        
    foreach( src_file ${viewer_SOURCE_FILES} )
        set_source_files_properties(
            ${src_file}
            PROPERTIES
            COMPILE_FLAGS "/Yullviewerprecompiledheaders.h"
            )        
    endforeach( src_file ${viewer_SOURCE_FILES} )
    list(APPEND viewer_SOURCE_FILES llviewerprecompiledheaders.cpp)
    
    # Add resource files to the project.
    # viewerRes.rc is the only buildable file, but
    # the rest are all dependencies of it.
    set(viewer_RESOURCE_FILES
        res/arrow.cur
        res/arrowcop.cur
        res/arrowcopmulti.cur
        res/arrowdrag.cur
        res/bitmap2.bmp
        res/circleandline.cur
        res/icon1.ico
        res/install_icon.BMP
        res/install_icon.ico
        res/llarrow.cur
        res/llarrowdrag.cur
        res/llarrowdragmulti.cur
        res/llarrowlocked.cur
        res/llgrablocked.cur
        res/llno.cur
        res/llnolocked.cur
        res/lltoolcamera.cur
        res/lltoolcreate.cur
        res/lltoolfocus.cur
        res/lltoolgrab.cur
        res/lltoolland.cur
        res/lltoolpan.cur
        res/lltoolpipette.cur
        res/lltoolrotate.cur
        res/lltoolscale.cur
        res/lltooltranslate.cur
        res/lltoolzoomin.cur
        res/lltoolzoomout.cur
        res/ll_icon.BMP
        res/ll_icon.ico
        res/loginbackground.bmp
        res/resource.h
        res/toolbuy.cur
        res/toolopen.cur
        res/toolpay.cur
        res/toolpickobject.cur
        res/toolpickobject2.cur
        res/toolpickobject3.cur
        res/toolpipette.cur
        res/toolsit.cur
        res/uninstall_icon.BMP
        res/uninstall_icon.ico
        )

    set_source_files_properties(${viewer_RESOURCE_FILES}
                                PROPERTIES HEADER_FILE_ONLY TRUE)

    set(viewer_RESOURCE_FILES 
        res/viewerRes.rc 
        ${viewer_RESOURCE_FILES}
        )

    SOURCE_GROUP("Resource Files" FILES ${viewer_RESOURCE_FILES})

    list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})

    find_library(DINPUT_LIBRARY dinput8 ${DIRECTX_LIBRARY_DIR})
    find_library(DXGUID_LIBRARY dxguid ${DIRECTX_LIBRARY_DIR})
    mark_as_advanced(
        DINPUT_LIBRARY
        DXGUID_LIBRARY
        )

    set(viewer_LIBRARIES
        advapi32
        comdlg32
        ${DINPUT_LIBRARY}
        ${DXGUID_LIBRARY}
        fmodvc
        gdi32
        kernel32
        odbc32
        odbccp32
        ole32
        oleaut32
        opengl32
        shell32
        user32
        Vfw32
        winspool
        )

    find_library(INTEL_MEMOPS_LIBRARY
                 NAMES ll_intel_memops
                 PATHS
                 optimized ${ARCH_PREBUILT_DIRS_RELEASE}
                 debug ${ARCH_PREBUILT_DIRS_DEBUG}
                 )
    mark_as_advanced(INTEL_MEMOPS_LIBRARY)

    if (INTEL_MEMOPS_LIBRARY)
      list(APPEND viewer_LIBRARIES ${INTEL_MEMOPS_LIBRARY})
    endif (INTEL_MEMOPS_LIBRARY)
endif (WINDOWS)

# Add the xui files. This is handy for searching for xui elements
# from within the IDE
set(viewer_XUI_FILES
    skins/xui/en-us/alerts.xml
    skins/xui/en-us/floater_about_land.xml
    skins/xui/en-us/floater_about.xml
    skins/xui/en-us/floater_active_speakers.xml
    skins/xui/en-us/floater_animation_preview.xml
    skins/xui/en-us/floater_auction.xml
    skins/xui/en-us/floater_audio_volume.xml
    skins/xui/en-us/floater_avatar_picker.xml
    skins/xui/en-us/floater_avatar_textures.xml
    skins/xui/en-us/floater_build_options.xml
    skins/xui/en-us/floater_bumps.xml
    skins/xui/en-us/floater_buy_contents.xml
    skins/xui/en-us/floater_buy_currency.xml
    skins/xui/en-us/floater_buy_land.xml
    skins/xui/en-us/floater_buy_object.xml
    skins/xui/en-us/floater_camera.xml
    skins/xui/en-us/floater_chat_history.xml
    skins/xui/en-us/floater_chatterbox.xml
    skins/xui/en-us/floater_choose_group.xml
    skins/xui/en-us/floater_clothing.xml
    skins/xui/en-us/floater_color_picker.xml
    skins/xui/en-us/floater_critical.xml
    skins/xui/en-us/floater_customize.xml
    skins/xui/en-us/floater_day_cycle_options.xml
    skins/xui/en-us/floater_device_settings.xml
    skins/xui/en-us/floater_directory.xml
    skins/xui/en-us/floater_env_settings.xml
    skins/xui/en-us/floater_gesture.xml
    skins/xui/en-us/floater_god_tools.xml
    skins/xui/en-us/floater_group_info.xml
    skins/xui/en-us/floater_hardware_settings.xml
    skins/xui/en-us/floater_html.xml
    skins/xui/en-us/floater_hud.xml
    skins/xui/en-us/floater_image_preview.xml
    skins/xui/en-us/floater_import.xml
    skins/xui/en-us/floater_im.xml
    skins/xui/en-us/floater_inspect.xml
    skins/xui/en-us/floater_instant_message_ad_hoc.xml
    skins/xui/en-us/floater_instant_message_group.xml
    skins/xui/en-us/floater_instant_message.xml
    skins/xui/en-us/floater_inventory_item_properties.xml
    skins/xui/en-us/floater_inventory_view_finder.xml
    skins/xui/en-us/floater_inventory.xml
    skins/xui/en-us/floater_joystick.xml
    skins/xui/en-us/floater_lagmeter.xml
    skins/xui/en-us/floater_land_holdings.xml
    skins/xui/en-us/floater_landmark_ctrl.xml
    skins/xui/en-us/floater_live_lsleditor.xml
    skins/xui/en-us/floater_lsl_guide.xml
    skins/xui/en-us/floater_media_browser.xml
    skins/xui/en-us/floater_moveview.xml
    skins/xui/en-us/floater_mute_object.xml
    skins/xui/en-us/floater_mute.xml
    skins/xui/en-us/floater_my_friends.xml
    skins/xui/en-us/floater_name_description.xml
    skins/xui/en-us/floater_new_im.xml
    skins/xui/en-us/floater_new_outfit_dialog.xml
    skins/xui/en-us/floater_openobject.xml
    skins/xui/en-us/floater_pay_object.xml
    skins/xui/en-us/floater_pay.xml
    skins/xui/en-us/floater_postcard.xml
    skins/xui/en-us/floater_post_process.xml
    skins/xui/en-us/floater_preferences.xml
    skins/xui/en-us/floater_preview_animation.xml
    skins/xui/en-us/floater_preview_classified.xml
    skins/xui/en-us/floater_preview_embedded_texture.xml
    skins/xui/en-us/floater_preview_event.xml
    skins/xui/en-us/floater_preview_existing_landmark.xml
    skins/xui/en-us/floater_preview_gesture.xml
    skins/xui/en-us/floater_preview_notecard_keep_discard.xml
    skins/xui/en-us/floater_preview_notecard.xml
    skins/xui/en-us/floater_preview_sound.xml
    skins/xui/en-us/floater_preview_texture_keep_discard.xml
    skins/xui/en-us/floater_preview_texture.xml
    skins/xui/en-us/floater_preview_url.xml
    skins/xui/en-us/floater_price_for_listing.xml
    skins/xui/en-us/floater_profile.xml
    skins/xui/en-us/floater_region_info.xml
    skins/xui/en-us/floater_report_abuse.xml
    skins/xui/en-us/floater_report_bug.xml
    skins/xui/en-us/floater_script_debug.xml
    skins/xui/en-us/floater_script_ed_panel.xml
    skins/xui/en-us/floater_script_preview.xml
    skins/xui/en-us/floater_script_queue.xml
    skins/xui/en-us/floater_script_search.xml
    skins/xui/en-us/floater_select_key.xml
    skins/xui/en-us/floater_sell_land.xml
    skins/xui/en-us/floater_settings_debug.xml
    skins/xui/en-us/floater_sim_release_message.xml
    skins/xui/en-us/floater_snapshot.xml
    skins/xui/en-us/floater_sound_preview.xml
    skins/xui/en-us/floater_telehub.xml
    skins/xui/en-us/floater_test.xml
    skins/xui/en-us/floater_texture_ctrl.xml
    skins/xui/en-us/floater_tools.xml
    skins/xui/en-us/floater_top_objects.xml
    skins/xui/en-us/floater_tos.xml
    skins/xui/en-us/floater_url_entry.xml
    skins/xui/en-us/floater_voice_wizard.xml
    skins/xui/en-us/floater_water.xml
    skins/xui/en-us/floater_wearable_save_as.xml
    skins/xui/en-us/floater_windlight_options.xml
    skins/xui/en-us/floater_world_map.xml
    skins/xui/en-us/menu_inventory.xml
    skins/xui/en-us/menu_login.xml
    skins/xui/en-us/menu_pie_attachment.xml
    skins/xui/en-us/menu_pie_avatar.xml
    skins/xui/en-us/menu_pie_land.xml
    skins/xui/en-us/menu_pie_object.xml
    skins/xui/en-us/menu_pie_self.xml
    skins/xui/en-us/menu_slurl.xml
    skins/xui/en-us/menu_viewer.xml
    skins/xui/en-us/mime_types.xml
    skins/xui/en-us/notify.xml
    skins/xui/en-us/panel_audio_device.xml
    skins/xui/en-us/panel_audio.xml
    skins/xui/en-us/panel_avatar_classified.xml
    skins/xui/en-us/panel_avatar_pick.xml
    skins/xui/en-us/panel_avatar.xml
    skins/xui/en-us/panel_bars.xml
    skins/xui/en-us/panel_bg_tab.xml
    skins/xui/en-us/panel_bg_toolbar.xml
    skins/xui/en-us/panel_chat_bar.xml
    skins/xui/en-us/panel_classified.xml
    skins/xui/en-us/panel_event.xml
    skins/xui/en-us/panel_friends.xml
    skins/xui/en-us/panel_group_finder.xml
    skins/xui/en-us/panel_group_general.xml
    skins/xui/en-us/panel_group_invite.xml
    skins/xui/en-us/panel_group_land_money.xml
    skins/xui/en-us/panel_group_notices.xml
    skins/xui/en-us/panel_group_roles.xml
    skins/xui/en-us/panel_groups.xml
    skins/xui/en-us/panel_group_voting.xml
    skins/xui/en-us/panel_group.xml
    skins/xui/en-us/panel_land_covenant.xml
    skins/xui/en-us/panel_login.xml
    skins/xui/en-us/panel_master_volume.xml
    skins/xui/en-us/panel_media_controls.xml
    skins/xui/en-us/panel_media_remote_expanded.xml
    skins/xui/en-us/panel_media_remote.xml
    skins/xui/en-us/panel_overlaybar.xml
    skins/xui/en-us/panel_place_small.xml
    skins/xui/en-us/panel_place.xml
    skins/xui/en-us/panel_preferences_audio.xml
    skins/xui/en-us/panel_preferences_chat.xml
    skins/xui/en-us/panel_preferences_general.xml
    skins/xui/en-us/panel_preferences_graphics1.xml
    skins/xui/en-us/panel_preferences_im.xml
    skins/xui/en-us/panel_preferences_input.xml
    skins/xui/en-us/panel_preferences_network.xml
    skins/xui/en-us/panel_preferences_popups.xml
    skins/xui/en-us/panel_preferences_voice.xml
    skins/xui/en-us/panel_preferences_web.xml
    skins/xui/en-us/panel_region_covenant.xml
    skins/xui/en-us/panel_region_debug.xml
    skins/xui/en-us/panel_region_estate.xml
    skins/xui/en-us/panel_region_general.xml
    skins/xui/en-us/panel_region_terrain.xml
    skins/xui/en-us/panel_region_texture.xml
    skins/xui/en-us/panel_scrolling_param.xml
    skins/xui/en-us/panel_speaker_controls.xml
    skins/xui/en-us/panel_status_bar.xml
    skins/xui/en-us/panel_toolbar.xml
    skins/xui/en-us/panel_top_pick.xml
    skins/xui/en-us/panel_voice_controls.xml
    skins/xui/en-us/panel_voice_enable.xml
    skins/xui/en-us/panel_voice_options.xml
    skins/xui/en-us/panel_voice_remote_expanded.xml
    skins/xui/en-us/panel_voice_remote.xml
    skins/xui/en-us/role_actions.xml
    skins/xui/en-us/strings.xml
    skins/xui/en-us/teleport_strings.xml
    skins/xui/en-us/xui_version.xml
    )

source_group("XUI Files" FILES ${viewer_XUI_FILES})

set_source_files_properties(${viewer_XUI_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_XUI_FILES})

set(viewer_APPSETTINGS_FILES
    app_settings/anim.ini
    app_settings/cmd_line.xml
    app_settings/colors.xml
    app_settings/colors_base.xml
    app_settings/grass.xml
    app_settings/high_graphics.xml
    app_settings/keys.ini
    app_settings/keywords.ini
    app_settings/logcontrol.xml
    app_settings/low_graphics.xml
    app_settings/mid_graphics.xml
    app_settings/settings.xml
    app_settings/settings_crash_behavior.xml
    app_settings/settings_files.xml
    app_settings/settings_per_account.xml
    app_settings/std_bump.ini
    app_settings/trees.xml
    app_settings/ultra_graphics.xml
    app_settings/viewerart.xml
    ${CMAKE_SOURCE_DIR}/../etc/message.xml
    ${CMAKE_SOURCE_DIR}/../scripts/messages/message_template.msg
    )

source_group("App Settings" FILES ${viewer_APPSETTINGS_FILES})

set_source_files_properties(${viewer_APPSETTINGS_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_APPSETTINGS_FILES})

set(viewer_CHARACTER_FILES
    character/attentions.xml
    character/attentionsN.xml
    character/avatar_lad.xml
    character/avatar_skeleton.xml
    character/genepool.xml
    )

source_group("Character File" FILES ${viewer_CHARACTER_FILES})

set_source_files_properties(${viewer_CHARACTER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_CHARACTER_FILES})

if (FMOD)
  set_source_files_properties(llstartup.cpp PROPERTIES COMPILE_FLAGS -DLL_FMOD)

  if (NOT WINDOWS)
    set(fmodwrapper_SOURCE_FILES fmodwrapper.cpp)
    add_library(fmodwrapper SHARED ${fmodwrapper_SOURCE_FILES})
    set(fmodwrapper_needed_LIBRARIES ${FMOD_LIBRARY})
    if (DARWIN)
      list(APPEND fmodwrapper_needed_LIBRARIES ${CARBON_LIBRARY})
      set_target_properties(
        fmodwrapper
        PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_NAME_DIR "@executable_path/../Resources"
        )
    endif (DARWIN)
    set(FMODWRAPPER_LIBRARY fmodwrapper)
    target_link_libraries(fmodwrapper ${fmodwrapper_needed_LIBRARIES})
  endif (NOT WINDOWS)
endif (FMOD)

list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

set_source_files_properties(${viewer_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(secondlife-bin
    WIN32
    MACOSX_BUNDLE
    ${viewer_SOURCE_FILES}
    )
check_message_template(secondlife-bin)

if (LLKDU_LIBRARY)
  add_dependencies(secondlife-bin ${LLKDU_LIBRARY})
endif (LLKDU_LIBRARY)

set(PACKAGE OFF CACHE BOOL
    "Add a package target that builds an installer package.")

if (WINDOWS)
    if(MSVC71)
        set(release_flags "/MAP:Release/secondlife-bin.map /MAPINFO:LINES")
    else(MSVC71)
        set(release_flags "/MAP:Release/secondlife-bin.map")
    endif(MSVC71)
    
    set_target_properties(secondlife-bin
        PROPERTIES
        LINK_FLAGS "/debug /NODEFAULTLIB:MSVCRT /SUBSYSTEM:WINDOWS"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:MSVCRTD"
        LINK_FLAGS_RELEASE ${release_flags}
        )

    # sets the 'working directory' for debugging from visual studio.
    if (NOT UNATTENDED)
        add_custom_command(
            TARGET secondlife-bin PRE_BUILD
            COMMAND ${CMAKE_SOURCE_DIR}/tools/vstool/vstool.exe
            ARGS
              --solution 
              ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln
              --workingdir
              secondlife-bin
              ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Setting the secondlife-bin working directory for debugging."
            )
    endif (NOT UNATTENDED)

    add_custom_command(
        TARGET secondlife-bin PRE_BUILD
        COMMAND ${CMAKE_COMMAND}
        ARGS
          -E
          copy_if_different
          ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/messages/message_template.msg
          ${CMAKE_CURRENT_SOURCE_DIR}/app_settings/message_template.msg
        COMMENT "Copying message_template.msg to the runtime folder."
        )

    add_dependencies(secondlife-bin copy_win_libs)

    if (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)
      add_dependencies(secondlife-bin copy_win_scripts)
    endif (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)
        
    add_custom_command(
        OUTPUT ${CMAKE_CFG_INTDIR}/touched.bat
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --configuration=${CMAKE_CFG_INTDIR}
          --channel=${VIEWER_CHANNEL}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
              --artwork=${ARTWORK_DIR}
              --build=${CMAKE_CURRENT_BINARY_DIR}
              --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
              --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/touched.bat
        DEPENDS secondlife-bin ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        )
    if (PACKAGE)
      add_custom_target(package ALL DEPENDS ${CMAKE_CFG_INTDIR}/touched.bat)
      add_dependencies(package windows-updater windows-crash-logger)
    endif (PACKAGE)
endif (WINDOWS)

target_link_libraries(secondlife-bin
    ${LLAUDIO_LIBRARIES}
    ${LLCHARACTER_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLIMAGEJ2COJ_LIBRARIES}
    ${LLINVENTORY_LIBRARIES}
    ${LLMEDIA_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLPRIMITIVE_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LLUI_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLWINDOW_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LSCRIPT_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${viewer_LIBRARIES}
    ${BOOST_PROGRAM_OPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
    ${BOOST_SIGNALS_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${FMODWRAPPER_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${MOZLIB_LIBRARIES}
    ${SDL_LIBRARY}
    ${SMARTHEAP_LIBRARY}
    ${UI_LIBRARIES}
    ${NDOF_LIBRARY}
    ${QUICKTIME_LIBRARY}
    ${WINDOWS_LIBRARIES}
    ${XMLRPCEPI_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ELFIO_LIBRARIES}
    )

build_version(viewer)

set(ARTWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
    "Path to artwork files.")

set(VIEWER_CHANNEL "Developer" CACHE STRING
    "The name of the channel to use.")

if (LINUX)
  add_custom_command(
      OUTPUT secondlife-stripped-globalsyms
      COMMAND strip
      ARGS --strip-debug -o secondlife-stripped-globalsyms secondlife-bin
      DEPENDS secondlife-bin
      )

  add_custom_command(
      OUTPUT secondlife-stripped
      COMMAND objcopy
      ARGS
        --keep-global-symbols
        ${CMAKE_CURRENT_SOURCE_DIR}/linux_tools/exposed-symbols.txt
        secondlife-stripped-globalsyms
        secondlife-stripped
      DEPENDS
        secondlife-stripped-globalsyms
        ${CMAKE_CURRENT_SOURCE_DIR}/linux_tools/exposed-symbols.txt
      )

  set(product SecondLife-${ARCH}-${viewer_VERSION})

  add_custom_command(
      OUTPUT ${product}.tar.bz2
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --grid=agni
        --channel=${VIEWER_CHANNEL}
        --installer_name=${product}
        --arch=${ARCH}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --artwork=${ARTWORK_DIR}
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
      DEPENDS secondlife-stripped ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )

  if (NOT INSTALL)
    add_custom_target(package ALL DEPENDS ${product}.tar.bz2)
    add_dependencies(package linux-crash-logger-stripped)
  endif (NOT INSTALL)
endif (LINUX)

if (DARWIN)
  set(product "Second Life")
  set_target_properties(
    secondlife-bin
    PROPERTIES
    OUTPUT_NAME "${product}"
    MACOSX_BUNDLE_INFO_STRING "info string - localize me"
    MACOSX_BUNDLE_ICON_FILE "secondlife.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "Second Life"
    MACOSX_BUNDLE_LONG_VERSION_STRING "ververver"
    MACOSX_BUNDLE_BUNDLE_NAME "Second Life"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "asdf"
    MACOSX_BUNDLE_BUNDLE_VERSION "asdf"
    MACOSX_BUNDLE_COPYRIGHT "copyright linden lab 2007 - localize me and run me through a legal wringer"
    )

  add_custom_command(
    TARGET secondlife-bin POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --actions=copy
      --configuration=${CMAKE_CFG_INTDIR}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
      --artwork=${ARTWORK_DIR}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
    DEPENDS secondlife-bin ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
    )

  if (PACKAGE)
    add_custom_target(package
      COMMAND
        ${PYTHON_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --configuration=${CMAKE_CFG_INTDIR}
        --channel=${VIEWER_CHANNEL}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --artwork=${ARTWORK_DIR}
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
      DEPENDS
        secondlife-bin
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )
  endif (PACKAGE)
endif (DARWIN)

if (INSTALL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/ViewerInstall.cmake)
endif (INSTALL)
