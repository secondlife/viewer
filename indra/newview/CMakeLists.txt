# -*- cmake -*-

project(viewer)

include(00-Common)
# DON'T move Linking.cmake to its place in the alphabetized list below: it
# sets variables on which the 3p .cmake files depend.
include(Linking)

include(Boost)
include(bugsplat)
include(BuildPackagesInfo)
include(BuildVersion)
include(CMakeCopyIfDifferent)
include(CubemapToEquirectangularJS)
include(DBusGlib)
include(DragDrop)
include(EXPAT)
include(Hunspell)
include(JPEGEncoderBasic)
include(LLAppearance)
include(LLAudio)
include(LLCA)
include(LLCommon)
include(LLCoreHttp)
include(LLImage)
include(LLKDU)
include(LLPhysicsExtensions)
include(LLPrimitive)
include(LLWindow)
include(SDL2)
include(NDOF)
include(NFDE)
include(NVAPI)
include(OPENAL)
include(OpenGL)
include(OpenSSL)
include(OpenXR)
include(PNG)
include(TemplateCheck)
include(TinyEXR)
include(ThreeJS)
include(Tracy)
include(UI)
include(ViewerMiscLibs)
include(ViewerManager)
include(VisualLeakDetector)
include(VulkanGltf)
include(ZLIBNG)
include(LLPrimitive)
include(Lualibs)

if (NOT HAVOK_TPV)
   # When using HAVOK_TPV, the library is precompiled, so no need for this

   # Stub and probably havok lib itself is a hack, autobuild loads a 3p that really is a source tarball
   # which includes a CMakeList.txt and then this whole source tree gets pushed into out build ... :/
   # To make matters worse there is a internal assumption about the structure of the viewers CMake layout,
   # which means we need to duct tape this togther ...

   add_subdirectory(${LLPHYSICSEXTENSIONS_SRC_DIR} llphysicsextensions)

   # Another hack that works with newer cmake versions:
   cmake_policy( SET CMP0079 NEW)
   if( TARGET llphysicsextensionsstub )
      target_link_libraries(llphysicsextensionsstub llcommon llmath)
   endif()
   if( TARGET llphysicsextensions )
      target_link_libraries(llphysicsextensions llrender )
      if (DARWIN)
        target_compile_options( llphysicsextensions  PRIVATE -Wno-unused-local-typedef)
      endif (DARWIN)
   endif()
endif (NOT HAVOK_TPV)

set(viewer_SOURCE_FILES
    gltfscenemanager.cpp
    gltf/asset.cpp
    gltf/accessor.cpp
    gltf/primitive.cpp
    gltf/animation.cpp
    groupchatlistener.cpp
    llaccountingcostmanager.cpp
    llaisapi.cpp
    llagent.cpp
    llagentaccess.cpp
    llagentbenefits.cpp
    llagentcamera.cpp
    llagentdata.cpp
    llagentlanguage.cpp
    llagentlistener.cpp
    llagentpicksinfo.cpp
    llagentpilot.cpp
    llagentui.cpp
    llagentwearables.cpp
    llanimstatelabels.cpp
    llappcorehttp.cpp
    llappearancelistener.cpp
    llappearancemgr.cpp
    llappviewer.cpp
    llappviewerlistener.cpp
    llattachmentsmgr.cpp
    llaudiosourcevo.cpp
    llautoreplace.cpp
    llavataractions.cpp
    llavatariconctrl.cpp
    llavatarlist.cpp
    llavatarlistitem.cpp
    llavatarrenderinfoaccountant.cpp
    llavatarrendernotifier.cpp
    llavatarpropertiesprocessor.cpp
    llblockedlistitem.cpp
    llblocklist.cpp
    llbox.cpp
    llbrowsernotification.cpp
    llbuycurrencyhtml.cpp
    llcallingcard.cpp
    llchannelmanager.cpp
    llchatbar.cpp
    llchathistory.cpp
    llchatitemscontainerctrl.cpp
    llchatmsgbox.cpp
    llchiclet.cpp
    llchicletbar.cpp
    llclassifiedinfo.cpp
    llcofwearables.cpp
    llcolorswatch.cpp
    llcommanddispatcherlistener.cpp
    llcommandhandler.cpp
    llcommandlineparser.cpp
    llcommunicationchannel.cpp
    llcompilequeue.cpp
    llconfirmationmanager.cpp
    llcontrolavatar.cpp
    llconversationlog.cpp
    llconversationloglist.cpp
    llconversationloglistitem.cpp
    llconversationmodel.cpp
    llconversationview.cpp
    llcurrencyuimanager.cpp
    llcylinder.cpp
    lldateutil.cpp
    lldebugmessagebox.cpp
    lldebugview.cpp
    lldeferredsounds.cpp
    lldelayedgestureerror.cpp
    lldirpicker.cpp
    lldonotdisturbnotificationstorage.cpp
    lldndbutton.cpp
    lldrawable.cpp
    lldrawpool.cpp
    lldrawpoolalpha.cpp
    lldrawpoolavatar.cpp
    lldrawpoolbump.cpp
    lldrawpoolmaterials.cpp
    lldrawpoolpbropaque.cpp
    lldrawpoolsimple.cpp
    lldrawpoolsky.cpp
    lldrawpoolterrain.cpp
    lldrawpooltree.cpp
    lldrawpoolwater.cpp
    lldrawpoolwlsky.cpp
    lldynamictexture.cpp
    llemote.cpp
    llenvironment.cpp
    llestateinfomodel.cpp
    lleventnotifier.cpp
    lleventpoll.cpp
    llexpandabletextbox.cpp
    llexperiencelog.cpp
    llexternaleditor.cpp
    llface.cpp
    llfasttimerview.cpp
    llfavoritesbar.cpp
    llfeaturemanager.cpp
    llfetchedgltfmaterial.cpp
    llfilepicker.cpp
    llfilteredwearablelist.cpp
    llfirstuse.cpp
    llflexibleobject.cpp
    llfloater360capture.cpp
    llfloaterabout.cpp
    llfloaterbvhpreview.cpp
    llfloateraddpaymentmethod.cpp
    llfloaterauction.cpp
    llfloaterautoreplacesettings.cpp
    llfloateravatar.cpp
    llfloateravatarpicker.cpp
    llfloateravatarrendersettings.cpp
    llfloateravatartextures.cpp
    llfloaterbanduration.cpp
    llfloaterbeacons.cpp
    llfloaterbigpreview.cpp
    llfloaterbuildoptions.cpp
    llfloaterbulkpermission.cpp
    llfloaterbulkupload.cpp
    llfloaterbump.cpp
    llfloaterbuy.cpp
    llfloaterbuycontents.cpp
    llfloaterbuycurrency.cpp
    llfloaterbuycurrencyhtml.cpp
    llfloaterbuyland.cpp
    llfloatercamera.cpp
    llfloatercamerapresets.cpp
    llfloaterchangeitemthumbnail.cpp
    llfloaterchatvoicevolume.cpp
    llfloaterclassified.cpp
    llfloatercolorpicker.cpp
    llfloaterconversationlog.cpp
    llfloaterconversationpreview.cpp
    llfloatercreatelandmark.cpp
    llfloaterdeleteprefpreset.cpp
    llfloaterdestinations.cpp
    llfloaterdisplayname.cpp
    llfloatereditenvironmentbase.cpp
    llfloatereditextdaycycle.cpp
    llfloateremojipicker.cpp
    llfloaterenvironmentadjust.cpp
    llfloaterevent.cpp
    llfloaterexperiencepicker.cpp
    llfloaterexperienceprofile.cpp
    llfloaterexperiences.cpp
    llfloaterfixedenvironment.cpp
    llfloaterfonttest.cpp
    llfloaterforgetuser.cpp
    llfloatergesture.cpp
    llfloatergltfasseteditor.cpp
    llfloatergodtools.cpp
    llfloatergotoline.cpp
    llfloatergridstatus.cpp
    llfloatergroupbulkban.cpp
    llfloatergroupinvite.cpp
    llfloatergroups.cpp
    llfloaterhandler.cpp
    llfloaterhelpbrowser.cpp
    llfloaterhoverheight.cpp
    llfloaterhowto.cpp
    llfloaterhud.cpp
    llfloaterimagepreview.cpp
    llfloaterimsessiontab.cpp
    llfloaterimsession.cpp
    llfloaterimcontainer.cpp
    llfloaterinspect.cpp
    llfloaterinventorysettings.cpp
    llfloaterinventorythumbnailshelper.cpp
    llfloaterjoystick.cpp
    llfloaterlagmeter.cpp
    llfloaterland.cpp
    llfloaterlandholdings.cpp
    llfloaterlinkreplace.cpp
    llfloaterloadprefpreset.cpp
    llfloaterluadebug.cpp
    llfloaterluascripts.cpp
    llfloatermarketplacelistings.cpp
    llfloatermap.cpp
    llfloatermediasettings.cpp
    llfloatermemleak.cpp
    llfloatermodelpreview.cpp
    llfloatermodeluploadbase.cpp
    llfloatermyscripts.cpp
    llfloatermyenvironment.cpp
    llfloaternamedesc.cpp
    llfloaternewfeaturenotification.cpp
    llfloaternotificationsconsole.cpp
    llfloaternotificationstabbed.cpp
    llfloaterobjectweights.cpp
    llfloateropenobject.cpp
    llfloatersimplesnapshot.cpp
    llfloaterpathfindingcharacters.cpp
    llfloaterpathfindingconsole.cpp
    llfloaterpathfindinglinksets.cpp
    llfloaterpathfindingobjects.cpp
    llfloaterpay.cpp
    llfloaterperformance.cpp
    llfloaterperms.cpp
    llfloaterprofile.cpp
    llfloaterpreference.cpp
    llfloaterpreferencesgraphicsadvanced.cpp
    llfloaterpreferenceviewadvanced.cpp
    llfloaterpreviewtrash.cpp
    llfloaterprofiletexture.cpp
    llfloaterregiondebugconsole.cpp
    llfloaterregioninfo.cpp
    llfloaterreporter.cpp
    llfloaterregionrestarting.cpp
    llfloaterregionrestartschedule.cpp
    llfloatersavecamerapreset.cpp
    llfloatersaveprefpreset.cpp
    llfloatersceneloadstats.cpp
    llfloaterscriptdebug.cpp
    llfloaterscriptedprefs.cpp
    llfloaterscriptlimits.cpp
    llfloatersearch.cpp
    llfloatersellland.cpp
    llfloatersettingscolor.cpp
    llfloatersettingsdebug.cpp
    llfloatersidepanelcontainer.cpp
    llfloatersnapshot.cpp
    llfloatersounddevices.cpp
    llfloaterspellchecksettings.cpp
    llfloatertelehub.cpp
    llfloatertestinspectors.cpp
    llfloatertestlistview.cpp
    llfloatertools.cpp
    llfloatertopobjects.cpp
    llfloatertos.cpp
    llfloatertoybox.cpp
    llfloatertranslationsettings.cpp
    llfloateruipreview.cpp
    llfloaterurlentry.cpp
    llfloatervoiceeffect.cpp
    llfloatervoicevolume.cpp
    llfloaterwebcontent.cpp
    llfloaterwhitelistentry.cpp
    llfloaterwindowsize.cpp
    llfloaterworldmap.cpp
    llfolderviewmodelinventory.cpp
    llfollowcam.cpp
    llfriendcard.cpp
    llflyoutcombobtn.cpp
    llflycam.cpp
    llgesturelistener.cpp
    llgesturemgr.cpp
    llgiveinventory.cpp
    llglsandbox.cpp
    llgltffolderitem.cpp
    llgltffoldermodel.cpp
    llgltfmateriallist.cpp
    llgltfmaterialpreviewmgr.cpp
    llgroupactions.cpp
    llgroupiconctrl.cpp
    llgrouplist.cpp
    llgroupmgr.cpp
    llhasheduniqueid.cpp
    llhints.cpp
    llhttpretrypolicy.cpp
    llhudeffect.cpp
    llhudeffectbeam.cpp
    llhudeffectlookat.cpp
    llhudeffectpointat.cpp
    llhudeffecttrail.cpp
    llhudeffectblob.cpp
    llhudicon.cpp
    llhudmanager.cpp
    llhudnametag.cpp
    llhudobject.cpp
    llhudrender.cpp
    llhudtext.cpp
    llhudview.cpp
    llimagefiltersmanager.cpp
    llimhandler.cpp
    llimprocessing.cpp
    llimview.cpp
    llinspect.cpp
    llinspectavatar.cpp
    llinspectgroup.cpp
    llinspectobject.cpp
    llinspectremoteobject.cpp
    llinspecttexture.cpp
    llinspecttoast.cpp
    llinventorybridge.cpp
    llinventoryfilter.cpp
    llinventoryfunctions.cpp
    llinventorygallery.cpp
    llinventorygallerymenu.cpp
    llinventoryicon.cpp
    llinventoryitemslist.cpp
    llinventorylistener.cpp
    llinventorylistitem.cpp
    llinventorymodel.cpp
    llinventorymodelbackgroundfetch.cpp
    llinventoryobserver.cpp
    llinventorypanel.cpp
    lljoystickbutton.cpp
    llkeyconflict.cpp
    lllandmarkactions.cpp
    lllandmarklist.cpp
    lllegacyatmospherics.cpp
    lllistcontextmenu.cpp
    lllistview.cpp
    lllocalbitmaps.cpp
    lllocalgltfmaterials.cpp
    lllocationhistory.cpp
    lllocationinputctrl.cpp
    lllogchat.cpp
    llloginhandler.cpp
    lllogininstance.cpp
    llluamanager.cpp
    llmachineid.cpp
    llmanip.cpp
    llmaniprotate.cpp
    llmanipscale.cpp
    llmaniptranslate.cpp
    llmarketplacefunctions.cpp
    llmarketplacenotifications.cpp
    llmaterialeditor.cpp
    llmaterialmgr.cpp
    llmediactrl.cpp
    llmediadataclient.cpp
    llmenuoptionpathfindingrebakenavmesh.cpp
    llmeshrepository.cpp
    llmimetypes.cpp
    llmodelpreview.cpp
    llmorphview.cpp
    llmoveview.cpp
    llmutelist.cpp
    llnamebox.cpp
    llnameeditor.cpp
    llnamelistctrl.cpp
    llnavigationbar.cpp
    llfloaterimnearbychat.cpp
    llfloaterimnearbychathandler.cpp
    llfloaterimnearbychatlistener.cpp
    llnetmap.cpp
    llnotificationalerthandler.cpp
    llnotificationgrouphandler.cpp
    llnotificationhandlerutil.cpp
    llnotificationhinthandler.cpp
    llnotificationlistitem.cpp
    llnotificationlistview.cpp
    llnotificationmanager.cpp
    llnotificationofferhandler.cpp
    llnotificationscripthandler.cpp
    llnotificationstorage.cpp
    llnotificationtiphandler.cpp
    lloutfitgallery.cpp
    lloutfitslist.cpp
    lloutfitobserver.cpp
    lloutputmonitorctrl.cpp
    llpanelappearancetab.cpp
    llpanelavatar.cpp
    llpanelavatartag.cpp
    llpanelblockedlist.cpp
    llpanelclassified.cpp
    llpanelcontents.cpp
    llpaneleditsky.cpp
    llpaneleditwater.cpp
    llpaneleditwearable.cpp
    llpanelemojicomplete.cpp
    llpanelenvironment.cpp
    llpanelexperiencelisteditor.cpp
    llpanelexperiencelog.cpp
    llpanelexperiencepicker.cpp
    llpanelexperiences.cpp
    llpanelface.cpp
    llpanelgenerictip.cpp
    llpanelgroup.cpp
    llpanelgroupcreate.cpp
    llpanelgroupbulk.cpp
    llpanelgroupbulkban.cpp
    llpanelgroupexperiences.cpp
    llpanelgroupgeneral.cpp
    llpanelgroupinvite.cpp
    llpanelgrouplandmoney.cpp
    llpanelgroupnotices.cpp
    llpanelgrouproles.cpp
    llpanelhome.cpp
    llpanelland.cpp
    llpanellandaudio.cpp
    llpanellandmarkinfo.cpp
    llpanellandmarks.cpp
    llpanellandmedia.cpp
    llpanellogin.cpp
    llpanelloginlistener.cpp
    llpanelmaininventory.cpp
    llpanelmarketplaceinbox.cpp
    llpanelmarketplaceinboxinventory.cpp
    llpanelmediasettingsgeneral.cpp
    llpanelmediasettingspermissions.cpp
    llpanelmediasettingssecurity.cpp
    llpanelnearbymedia.cpp
    llpanelobject.cpp
    llpanelobjectinventory.cpp
    llpanelonlinestatus.cpp
    llpaneloutfitedit.cpp
    llpaneloutfitsinventory.cpp
    llpanelpeople.cpp
    llpanelpeoplemenus.cpp
    llpanelpermissions.cpp
    llpanelplaceinfo.cpp
    llpanelplaceprofile.cpp
    llpanelplaces.cpp
    llpanelplacestab.cpp
    llpanelpresetscamerapulldown.cpp
    llpanelpresetspulldown.cpp
    llpanelprimmediacontrols.cpp
    llpanelprofile.cpp
    llpanelprofileclassifieds.cpp
    llpanelprofilepicks.cpp
    llpanelsnapshot.cpp
    llpanelsnapshotinventory.cpp
    llpanelsnapshotlocal.cpp
    llpanelsnapshotoptions.cpp
    llpanelsnapshotpostcard.cpp
    llpanelsnapshotprofile.cpp
    llpanelteleporthistory.cpp
    llpaneltiptoast.cpp
    llpanelvoiceeffect.cpp
    llpaneltopinfobar.cpp
    llpanelpulldown.cpp
    llpanelvoicedevicesettings.cpp
    llpanelvolume.cpp
    llpanelvolumepulldown.cpp
    llpanelwearing.cpp
    llparcelselection.cpp
    llparticipantlist.cpp
    llpatchvertexarray.cpp
    llpathfindingcharacter.cpp
    llpathfindingcharacterlist.cpp
    llpathfindinglinkset.cpp
    llpathfindinglinksetlist.cpp
    llpathfindingmanager.cpp
    llpathfindingnavmesh.cpp
    llpathfindingnavmeshstatus.cpp
    llpathfindingnavmeshzone.cpp
    llpathfindingobject.cpp
    llpathfindingobjectlist.cpp
    llpathfindingpathtool.cpp
    llpbrterrainfeatures.cpp
    llpersistentnotificationstorage.cpp
    llphysicsmotion.cpp
    llphysicsshapebuilderutil.cpp
    llpipelinelistener.cpp
    llplacesinventorybridge.cpp
    llplacesinventorypanel.cpp
    llplacesfolderview.cpp
    llpopupview.cpp
    llpostcard.cpp
    llpresetsmanager.cpp
    llpreview.cpp
    llpreviewanim.cpp
    llpreviewgesture.cpp
    llpreviewnotecard.cpp
    llpreviewscript.cpp
    llpreviewsound.cpp
    llpreviewtexture.cpp
    llproductinforequest.cpp
    llprogressview.cpp
    llrecentpeople.cpp
    llreflectionmap.cpp
    llreflectionmapmanager.cpp
    llheroprobemanager.cpp
    llregioninfomodel.cpp
    llregionposition.cpp
    llremoteparcelrequest.cpp
    llsaveoutfitcombobtn.cpp
    llscenemonitor.cpp
    llsceneview.cpp
    llscreenchannel.cpp
    llscripteditor.cpp
    llscriptfloater.cpp
    llscrollingpanelparam.cpp
    llscrollingpanelparambase.cpp
    llsculptidsize.cpp
    llsearchableui.cpp
    llsearchcombobox.cpp
    llsearchhistory.cpp
    llsecapi.cpp
    llsechandler_basic.cpp
    llselectmgr.cpp
    llsetkeybinddialog.cpp
    llsettingspicker.cpp
    llsettingsvo.cpp
    llshareavatarhandler.cpp
    llsidepanelappearance.cpp
    llsidepanelinventory.cpp
    llsidepanelinventorysubpanel.cpp
    llsidepaneliteminfo.cpp
    llsidepaneltaskinfo.cpp
    llsidetraypanelcontainer.cpp
    llskinningutil.cpp
    llsky.cpp
    llslurl.cpp
    llsnapshotlivepreview.cpp
    llspatialpartition.cpp
    llspeakers.cpp
    llspeakingindicatormanager.cpp
    llsplitbutton.cpp
    llsprite.cpp
    llstartup.cpp
    llstartuplistener.cpp
    llstatusbar.cpp
    llstylemap.cpp
    llsurface.cpp
    llsurfacepatch.cpp
    llsyntaxid.cpp
    llsyswellitem.cpp
    llsyswellwindow.cpp
    llteleporthistory.cpp
    llteleporthistorystorage.cpp
    llterrainpaintmap.cpp
    lltexturecache.cpp
    lltexturectrl.cpp
    lltexturefetch.cpp
    lltextureinfo.cpp
    lltextureinfodetails.cpp
    lltexturestats.cpp
    lltextureview.cpp
    llthumbnailctrl.cpp
    lltinygltfhelper.cpp
    lltoast.cpp
    lltoastalertpanel.cpp
    lltoastgroupnotifypanel.cpp
    lltoastimpanel.cpp
    lltoastnotifypanel.cpp
    lltoastpanel.cpp
    lltoastscripttextbox.cpp
    lltoastscriptquestion.cpp
    lltool.cpp
    lltoolbarview.cpp
    lltoolbrush.cpp
    lltoolcomp.cpp
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
    lltracker.cpp
    lltrackpicker.cpp
    lltransientdockablefloater.cpp
    lltransientfloatermgr.cpp
    lltranslate.cpp
    lluiavatar.cpp
    lluilistener.cpp
    lluploaddialog.cpp
    llurl.cpp
    llurldispatcher.cpp
    llurldispatcherlistener.cpp
    llurlfloaterdispatchhandler.cpp
    llurlhistory.cpp
    llurllineeditorctrl.cpp
    llurlwhitelist.cpp
    llversioninfo.cpp
    llviewchildren.cpp
    llviewerassetstats.cpp
    llviewerassetstorage.cpp
    llviewerassettype.cpp
    llviewerassetupload.cpp
    llviewerattachmenu.cpp
    llvieweraudio.cpp
    llviewercamera.cpp
    llviewerchat.cpp
    llviewercontrol.cpp
    llviewercontrollistener.cpp
    llviewerdisplay.cpp
    llviewerdisplayname.cpp
    llviewerfloaterreg.cpp
    llviewerfoldertype.cpp
    llviewergenericmessage.cpp
    llviewergesture.cpp
    llviewerhelp.cpp
    llviewerhelputil.cpp
    llviewerhome.cpp
    llviewerinventory.cpp
    llviewerjoint.cpp
    llviewerjointattachment.cpp
    llviewerjointmesh.cpp
    llviewerjoystick.cpp
    llviewerinput.cpp
    llviewerlayer.cpp
    llviewermedia.cpp
    llviewermedia_streamingaudio.cpp
    llviewermediafocus.cpp
    llviewermenu.cpp
    llviewermenufile.cpp
    llviewermessage.cpp
    llviewernetwork.cpp
    llviewerobject.cpp
    llviewerobjectlist.cpp
    llvieweroctree.cpp
    llviewerparcelaskplay.cpp
    llviewerparcelmedia.cpp
    llviewerparcelmediaautoplay.cpp
    llviewerparcelmgr.cpp
    llviewerparceloverlay.cpp
    llviewerpartsim.cpp
    llviewerpartsource.cpp
    llviewerregion.cpp
    llviewershadermgr.cpp
    llviewerstats.cpp
    llviewerstatsrecorder.cpp
    llviewertexlayer.cpp
    llviewertexteditor.cpp
    llviewertexture.cpp
    llviewertextureanim.cpp
    llviewertexturelist.cpp
    llviewerthrottle.cpp
    llviewerwearable.cpp
    llviewerwindow.cpp
    llviewerwindowlistener.cpp
    llvlcomposition.cpp
    llvlmanager.cpp
    llvoavatar.cpp
    llvoavatarself.cpp
    llvocache.cpp
    llvograss.cpp
    llvoicecallhandler.cpp
    llvoicechannel.cpp
    llvoiceclient.cpp
    llvoicevisualizer.cpp
    llvoicevivox.cpp
    llvoicewebrtc.cpp
    llvoinventorylistener.cpp
    llvopartgroup.cpp
    llvosky.cpp
    llvosurfacepatch.cpp
    llvotree.cpp
    llvovolume.cpp
    llvowater.cpp
    llvowlsky.cpp
    llwatchdog.cpp
    llwearableitemslist.cpp
    llwearablelist.cpp
    llweb.cpp
    llwebprofile.cpp
    llwind.cpp
    llwindowlistener.cpp
    llwlhandlers.cpp
    llworld.cpp
    llworldmap.cpp
    llworldmapmessage.cpp
    llworldmipmap.cpp
    llworldmapview.cpp
    llxmlrpclistener.cpp
    llxmlrpctransaction.cpp
    noise.cpp
    pipeline.cpp
    rlvactions.cpp
    rlvcommon.cpp
    rlvfloaters.cpp
    rlvhelper.cpp
    rlvhandler.cpp
    )

set(VIEWER_BINARY_NAME "secondlife-bin" CACHE STRING
    "The name of the viewer executable to create.")

set(viewer_HEADER_FILES
    CMakeLists.txt
    ViewerInstall.cmake
    gltfscenemanager.h
    groupchatlistener.h
    gltf/asset.h
    gltf/accessor.h
    gltf/buffer_util.h
    gltf/primitive.h
    gltf/animation.h
    llaccountingcost.h
    llaccountingcostmanager.h
    llaisapi.h
    llagent.h
    llagentaccess.h
    llagentbenefits.h
    llagentcamera.h
    llagentdata.h
    llagentlanguage.h
    llagentlistener.h
    llagentpicksinfo.h
    llagentpilot.h
    llagentui.h
    llagentwearables.h
    llanimstatelabels.h
    llappcorehttp.h
    llappearance.h
    llappearancelistener.h
    llappearancemgr.h
    llappviewer.h
    llappviewerlistener.h
    llattachmentsmgr.h
    llaudiosourcevo.h
    llautoreplace.h
    llavataractions.h
    llavatariconctrl.h
    llavatarlist.h
    llavatarlistitem.h
    llavatarpropertiesprocessor.h
    llavatarrenderinfoaccountant.h
    llavatarrendernotifier.h
    llblockedlistitem.h
    llblocklist.h
    llbox.h
    llbuycurrencyhtml.h
    llcallingcard.h
    llcapabilityprovider.h
    llchannelmanager.h
    llchatbar.h
    llchathistory.h
    llchatitemscontainerctrl.h
    llchatmsgbox.h
    llchiclet.h
    llchicletbar.h
    llclassifiedinfo.h
    llcofwearables.h
    llcolorswatch.h
    llcommanddispatcherlistener.h
    llcommandhandler.h
    llcommandlineparser.h
    llcommunicationchannel.h
    llcompilequeue.h
    llconfirmationmanager.h
    llcontrolavatar.h
    llconversationlog.h
    llconversationloglist.h
    llconversationloglistitem.h
    llconversationmodel.h
    llconversationview.h
    llcurrencyuimanager.h
    llcylinder.h
    lldateutil.h
    lldebugmessagebox.h
    lldebugview.h
    lldeferredsounds.h
    lldelayedgestureerror.h
    lldirpicker.h
    lldonotdisturbnotificationstorage.h
    lldndbutton.h
    lldrawable.h
    lldrawpool.h
    lldrawpoolalpha.h
    lldrawpoolavatar.h
    lldrawpoolbump.h
    lldrawpoolmaterials.h
    lldrawpoolpbropaque.h
    lldrawpoolsimple.h
    lldrawpoolsky.h
    lldrawpoolterrain.h
    lldrawpooltree.h
    lldrawpoolwater.h
    lldrawpoolwlsky.h
    lldynamictexture.h
    llemote.h
    llenvironment.h
    llestateinfomodel.h
    lleventnotifier.h
    lleventpoll.h
    llexpandabletextbox.h
    llexperiencelog.h
    llexternaleditor.h
    llface.h
    llfasttimerview.h
    llfavoritesbar.h
    llfeaturemanager.h
    llfetchedgltfmaterial.h
    llfilepicker.h
    llfilteredwearablelist.h
    llfirstuse.h
    llflexibleobject.h
    llfloater360capture.h
    llfloaterabout.h
    llfloaterbvhpreview.h
    llfloateraddpaymentmethod.h
    llfloaterauction.h
    llfloaterautoreplacesettings.h
    llfloateravatar.h
    llfloateravatarpicker.h
    llfloateravatarrendersettings.h
    llfloateravatartextures.h
    llfloaterbanduration.h
    llfloaterbeacons.h
    llfloaterbigpreview.h
    llfloaterbuildoptions.h
    llfloaterbulkpermission.h
    llfloaterbulkupload.h
    llfloaterbump.h
    llfloaterbuy.h
    llfloaterbuycontents.h
    llfloaterbuycurrency.h
    llfloaterbuycurrencyhtml.h
    llfloaterbuyland.h
    llfloatercamerapresets.h
    llfloaterchangeitemthumbnail.h
    llfloatercamera.h
    llfloaterchatvoicevolume.h
    llfloaterclassified.h
    llfloatercolorpicker.h
    llfloaterconversationlog.h
    llfloaterconversationpreview.h
    llfloatercreatelandmark.h
    llfloaterdeleteprefpreset.h
    llfloaterdestinations.h
    llfloaterdisplayname.h
    llfloatereditenvironmentbase.h
    llfloatereditextdaycycle.h
    llfloateremojipicker.h
    llfloaterenvironmentadjust.h
    llfloaterevent.h
    llfloaterexperiencepicker.h
    llfloaterexperienceprofile.h
    llfloaterexperiences.h
    llfloaterfixedenvironment.h
    llfloaterfonttest.h
    llfloaterforgetuser.h
    llfloatergesture.h
    llfloatergltfasseteditor.h
    llfloatergodtools.h
    llfloatergotoline.h
    llfloatergridstatus.h
    llfloatergroupbulkban.h
    llfloatergroupinvite.h
    llfloatergroups.h
    llfloaterhandler.h
    llfloaterhelpbrowser.h
    llfloaterhoverheight.h
    llfloaterhowto.h
    llfloaterhud.h
    llfloaterimagepreview.h
    llfloaterimnearbychat.h
    llfloaterimnearbychathandler.h
    llfloaterimnearbychatlistener.h
    llfloaterimsessiontab.h
    llfloaterimsession.h
    llfloaterimcontainer.h
    llfloaterinspect.h
    llfloaterinventorysettings.h
    llfloaterinventorythumbnailshelper.h
    llfloaterjoystick.h
    llfloaterlagmeter.h
    llfloaterland.h
    llfloaterlandholdings.h
    llfloaterlinkreplace.h
    llfloaterloadprefpreset.h
    llfloaterluadebug.h
    llfloaterluascripts.h
    llfloatermap.h
    llfloatermarketplacelistings.h
    llfloatermediasettings.h
    llfloatermemleak.h
    llfloatermodelpreview.h
    llfloatermodeluploadbase.h
    llfloatermyscripts.h
    llfloatermyenvironment.h
    llfloaternamedesc.h
    llfloaternewfeaturenotification.h
    llfloaternotificationsconsole.h
    llfloaternotificationstabbed.h
    llfloaterobjectweights.h
    llfloateropenobject.h
    llfloatersimplesnapshot.h
    llfloaterpathfindingcharacters.h
    llfloaterpathfindingconsole.h
    llfloaterpathfindinglinksets.h
    llfloaterpathfindingobjects.h
    llfloaterpay.h
    llfloaterperformance.h
    llfloaterperms.h
    llfloaterprofile.h
    llfloaterpreference.h
    llfloaterpreferencesgraphicsadvanced.h
    llfloaterpreferenceviewadvanced.h
    llfloaterpreviewtrash.h
    llfloaterprofiletexture.h
    llfloaterregiondebugconsole.h
    llfloaterregioninfo.h
    llfloaterreporter.h
    llfloaterregionrestarting.h
    llfloaterregionrestartschedule.h
    llfloatersavecamerapreset.h
    llfloatersaveprefpreset.h
    llfloatersceneloadstats.h
    llfloaterscriptdebug.h
    llfloaterscriptedprefs.h
    llfloaterscriptlimits.h
    llfloatersearch.h
    llfloatersellland.h
    llfloatersettingscolor.h
    llfloatersettingsdebug.h
    llfloatersidepanelcontainer.h
    llfloatersnapshot.h
    llfloatersounddevices.h
    llfloaterspellchecksettings.h
    llfloatertelehub.h
    llfloatertestinspectors.h
    llfloatertestlistview.h
    llfloatertools.h
    llfloatertopobjects.h
    llfloatertos.h
    llfloatertoybox.h
    llfloatertranslationsettings.h
    llfloateruipreview.h
    llfloaterurlentry.h
    llfloatervoiceeffect.h
    llfloatervoicevolume.h
    llfloaterwebcontent.h
    llfloaterwhitelistentry.h
    llfloaterwindowsize.h
    llfloaterworldmap.h
    llfolderviewmodelinventory.h
    llfollowcam.h
    llfriendcard.h
    llflyoutcombobtn.h
    llflycam.h
    llgesturelistener.h
    llgesturemgr.h
    llgiveinventory.h
    llgltffolderitem.h
    llgltffoldermodel.h
    llgltfmateriallist.h
    llgltfmaterialpreviewmgr.h
    llgroupactions.h
    llgroupiconctrl.h
    llgrouplist.h
    llgroupmgr.h
    llhasheduniqueid.h
    llhints.h
    llhttpretrypolicy.h
    llhudeffect.h
    llhudeffectbeam.h
    llhudeffectlookat.h
    llhudeffectpointat.h
    llhudeffecttrail.h
    llhudeffectblob.h
    llhudicon.h
    llhudmanager.h
    llhudnametag.h
    llhudobject.h
    llhudrender.h
    llhudtext.h
    llhudview.h
    llimagefiltersmanager.h
    llimprocessing.h
    llimview.h
    llinspect.h
    llinspectavatar.h
    llinspectgroup.h
    llinspectobject.h
    llinspectremoteobject.h
    llinspecttexture.h
    llinspecttoast.h
    llinventorybridge.h
    llinventoryfilter.h
    llinventoryfunctions.h
    llinventorygallery.h
    llinventorygallerymenu.h
    llinventoryicon.h
    llinventoryitemslist.h
    llinventorylistener.h
    llinventorylistitem.h
    llinventorymodel.h
    llinventorymodelbackgroundfetch.h
    llinventoryobserver.h
    llinventorypanel.h
    lljoystickbutton.h
    llkeyconflict.h
    lllandmarkactions.h
    lllandmarklist.h
    lllightconstants.h
    lllistcontextmenu.h
    lllistview.h
    lllocalbitmaps.h
    lllocalgltfmaterials.h
    lllocationhistory.h
    lllocationinputctrl.h
    lllogchat.h
    llloginhandler.h
    lllogininstance.h
    llluamanager.h
    llmachineid.h
    llmanip.h
    llmaniprotate.h
    llmanipscale.h
    llmaniptranslate.h
    llmarketplacefunctions.h
    llmarketplacenotifications.h
    llmaterialeditor.h
    llmaterialmgr.h
    llmediactrl.h
    llmediadataclient.h
    llmenuoptionpathfindingrebakenavmesh.h
    llmeshrepository.h
    llmimetypes.h
    llmodelpreview.h
    llmorphview.h
    llmoveview.h
    llmutelist.h
    llnamebox.h
    llnameeditor.h
    llnamelistctrl.h
    llnavigationbar.h
    llnetmap.h
    llnotificationhandler.h
    llnotificationlistitem.h
    llnotificationlistview.h
    llnotificationmanager.h
    llnotificationstorage.h
    lloutfitgallery.h
    lloutfitslist.h
    lloutfitobserver.h
    lloutputmonitorctrl.h
    llpanelappearancetab.h
    llpanelavatar.h
    llpanelavatartag.h
    llpanelblockedlist.h
    llpanelclassified.h
    llpanelcontents.h
    llpaneleditsky.h
    llpaneleditwater.h
    llpaneleditwearable.h
    llpanelemojicomplete.h
    llpanelenvironment.h
    llpanelexperiencelisteditor.h
    llpanelexperiencelog.h
    llpanelexperiencepicker.h
    llpanelexperiences.h
    llpanelface.h
    llpanelgenerictip.h
    llpanelgroup.h
    llpanelgroupcreate.h
    llpanelgroupbulk.h
    llpanelgroupbulkimpl.h
    llpanelgroupbulkban.h
    llpanelgroupexperiences.h
    llpanelgroupgeneral.h
    llpanelgroupinvite.h
    llpanelgrouplandmoney.h
    llpanelgroupnotices.h
    llpanelgrouproles.h
    llpanelhome.h
    llpanelland.h
    llpanellandaudio.h
    llpanellandmarkinfo.h
    llpanellandmarks.h
    llpanellandmedia.h
    llpanellogin.h
    llpanelloginlistener.h
    llpanelmaininventory.h
    llpanelmarketplaceinbox.h
    llpanelmarketplaceinboxinventory.h
    llpanelmediasettingsgeneral.h
    llpanelmediasettingspermissions.h
    llpanelmediasettingssecurity.h
    llpanelnearbymedia.h
    llpanelobject.h
    llpanelobjectinventory.h
    llpanelonlinestatus.h
    llpaneloutfitedit.h
    llpaneloutfitsinventory.h
    llpanelpeople.h
    llpanelpeoplemenus.h
    llpanelpermissions.h
    llpanelplaceinfo.h
    llpanelplaceprofile.h
    llpanelplaces.h
    llpanelplacestab.h
    llpanelpresetscamerapulldown.h
    llpanelpresetspulldown.h
    llpanelprimmediacontrols.h
    llpanelprofile.h
    llpanelprofileclassifieds.h
    llpanelprofilepicks.h
    llpanelsnapshot.h
    llpanelteleporthistory.h
    llpaneltiptoast.h
    llpanelpulldown.h
    llpanelvoicedevicesettings.h
    llpanelvoiceeffect.h
    llpaneltopinfobar.h
    llpanelvolume.h
    llpanelvolumepulldown.h
    llpanelwearing.h
    llparcelselection.h
    llparticipantlist.h
    llpatchvertexarray.h
    llpathfindingcharacter.h
    llpathfindingcharacterlist.h
    llpathfindinglinkset.h
    llpathfindinglinksetlist.h
    llpathfindingmanager.h
    llpathfindingnavmesh.h
    llpathfindingnavmeshstatus.h
    llpathfindingnavmeshzone.h
    llpathfindingobject.h
    llpathfindingobjectlist.h
    llpathfindingpathtool.h
    llpbrterrainfeatures.h
    llpersistentnotificationstorage.h
    llphysicsmotion.h
    llphysicsshapebuilderutil.h
    llpipelinelistener.h
    llplacesinventorybridge.h
    llplacesinventorypanel.h
    llplacesfolderview.h
    llpopupview.h
    llpostcard.h
    llpresetsmanager.h
    llpreview.h
    llpreviewanim.h
    llpreviewgesture.h
    llpreviewnotecard.h
    llpreviewscript.h
    llpreviewsound.h
    llpreviewtexture.h
    llproductinforequest.h
    llprogressview.h
    llrecentpeople.h
    llreflectionmap.h
    llreflectionmapmanager.h
    llheroprobemanager.h
    llregioninfomodel.h
    llregionposition.h
    llremoteparcelrequest.h
    llresourcedata.h
    llrootview.h
    llsaveoutfitcombobtn.h
    llscenemonitor.h
    llsceneview.h
    llscreenchannel.h
    llscripteditor.h
    llscriptfloater.h
    llscriptruntimeperms.h
    llscrollingpanelparam.h
    llscrollingpanelparambase.h
    llsculptidsize.h
    llsearchableui.h
    llsearchcombobox.h
    llsearchhistory.h
    llsecapi.h
    llsechandler_basic.h
    llselectmgr.h
    llsetkeybinddialog.h
    llsettingspicker.h
    llsettingsvo.h
    llsidepanelappearance.h
    llsidepanelinventory.h
    llsidepanelinventorysubpanel.h
    llsidepaneliteminfo.h
    llsidepaneltaskinfo.h
    llsidetraypanelcontainer.h
    llskinningutil.h
    llsky.h
    llslurl.h
    llsnapshotlivepreview.h
    llsnapshotmodel.h
    llspatialpartition.h
    llspeakers.h
    llspeakingindicatormanager.h
    llsplitbutton.h
    llsprite.h
    llstartup.h
    llstartuplistener.h
    llstatusbar.h
    llstylemap.h
    llsurface.h
    llsurfacepatch.h
    llsyntaxid.h
    llsyswellitem.h
    llsyswellwindow.h
    lltable.h
    llteleporthistory.h
    llteleporthistorystorage.h
    llterrainpaintmap.h
    lltexturecache.h
    lltexturectrl.h
    lltexturefetch.h
    lltextureinfo.h
    lltextureinfodetails.h
    lltexturestats.h
    lltextureview.h
    llthumbnailctrl.h
    lltinygltfhelper.h
    lltoast.h
    lltoastalertpanel.h
    lltoastgroupnotifypanel.h
    lltoastimpanel.h
    lltoastnotifypanel.h
    lltoastpanel.h
    lltoastscripttextbox.h
    lltoastscriptquestion.h
    lltool.h
    lltoolbarview.h
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
    lltracker.h
    lltrackpicker.h
    lltransientdockablefloater.h
    lltransientfloatermgr.h
    lltranslate.h
    lluiconstants.h
    lluiavatar.h
    lluilistener.h
    lluploaddialog.h
    lluploadfloaterobservers.h
    llurl.h
    llurldispatcher.h
    llurldispatcherlistener.h
    llurlfloaterdispatchhandler.h
    llurlhistory.h
    llurllineeditorctrl.h
    llurlwhitelist.h
    llversioninfo.h
    llviewchildren.h
    llviewerassetstats.h
    llviewerassetstorage.h
    llviewerassettype.h
    llviewerassetupload.h
    llviewerattachmenu.h
    llvieweraudio.h
    llviewercamera.h
    llviewerchat.h
    llviewercontrol.h
    llviewercontrollistener.h
    llviewerdisplay.h
    llviewerdisplayname.h
    llviewerfloaterreg.h
    llviewerfoldertype.h
    llviewergenericmessage.h
    llviewergesture.h
    llviewerhelp.h
    llviewerhome.h
    llviewerinventory.h
    llviewerjoint.h
    llviewerjointattachment.h
    llviewerjointmesh.h
    llviewerjoystick.h
    llviewerinput.h
    llviewerlayer.h
    llviewermedia.h
    llviewermediafocus.h
    llviewermediaobserver.h
    llviewermenu.h
    llviewermenufile.h
    llviewermessage.h
    llviewernetwork.h
    llviewerobject.h
    llviewerobjectlist.h
    llvieweroctree.h
    llviewerparcelaskplay.h
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
    llviewerstatsrecorder.h
    llviewertexlayer.h
    llviewertexteditor.h
    llviewertexture.h
    llviewertextureanim.h
    llviewertexturelist.h
    llviewerthrottle.h
    llviewerwearable.h
    llviewerwindow.h
    llviewerwindowlistener.h
    llvlcomposition.h
    llvlmanager.h
    llvoavatar.h
    llvoavatarself.h
    llvocache.h
    llvograss.h
    llvoicechannel.h
    llvoiceclient.h
    llvoicevisualizer.h
    llvoicevivox.h
    llvoicewebrtc.h
    llvoinventorylistener.h
    llvopartgroup.h
    llvosky.h
    llvosurfacepatch.h
    llvotree.h
    llvovolume.h
    llvowater.h
    llvowlsky.h
    llwatchdog.h
    llwearableitemslist.h
    llwearablelist.h
    llweb.h
    llwebprofile.h
    llwind.h
    llwindowlistener.h
    llwlhandlers.h
    llworld.h
    llworldmap.h
    llworldmapmessage.h
    llworldmipmap.h
    llworldmapview.h
    llxmlrpclistener.h
    llxmlrpctransaction.h
    noise.h
    pipeline.h
    rlvdefines.h
    rlvactions.h
    rlvcommon.h
    rlvfloaters.h
    rlvhelper.h
    rlvhandler.h
    roles_constants.h
    VertexCache.h
    VorbisFramework.h
    )

  list(APPEND viewer_SOURCE_FILES llperfstats.cpp)
  list(APPEND viewer_HEADER_FILES llperfstats.h)

source_group("CMake Rules" FILES ViewerInstall.cmake)

#build_data.json creation moved to viewer_manifest.py MAINT-6413
# the viewer_version.txt file created here is for passing to viewer_manifest and autobuild
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt"
           "${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}\n")

set_source_files_properties(
   llversioninfo.cpp tests/llversioninfo_test.cpp 
   PROPERTIES
   COMPILE_DEFINITIONS "${VIEWER_CHANNEL_VERSION_DEFINES}" # see BuildVersion.cmake
   )

if (DARWIN)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx.cpp)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx-objc.mm)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx-objc.h)
  LIST(APPEND viewer_SOURCE_FILES llfilepicker_mac.mm)
  LIST(APPEND viewer_HEADER_FILES llfilepicker_mac.h)

  set_source_files_properties(
    llappviewermacosx-objc.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )

  set_source_files_properties(
    llfilepicker_mac.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )

  # This should be compiled with the viewer.
  LIST(APPEND viewer_SOURCE_FILES llappdelegate-objc.mm)
  set_source_files_properties(
    llappdelegate-objc.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
    COMPILE_DEFINITIONS "${VIEWER_CHANNEL_VERSION_DEFINES}"
    # BugsplatMac is a module, imported with @import. That language feature
    # demands these -f switches.
    # Xcode 10.2 requires that Objective-C++ headers declare nullability of
    # pointer variables. As of 2019-06-26, the BugsplatMac version we're using
    # does not yet do so in its own header files. This -W flag prevents fatal
    # warnings.
    COMPILE_FLAGS "-fmodules -fcxx-modules -Wno-nullability-completeness"
    )

  # Add resource files to the project.
  set(viewer_RESOURCE_FILES
    secondlife.icns
    Info-SecondLife.plist
    SecondLife.xib/
    # CMake doesn't seem to support Xcode language variants well just yet
    English.lproj/InfoPlist.strings
    English.lproj/language.txt
    German.lproj/language.txt
    Japanese.lproj/language.txt
    Korean.lproj/language.txt
    )

  SOURCE_GROUP("Resources" FILES ${viewer_RESOURCE_FILES})
  list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})
endif (DARWIN)

if (LINUX)
    LIST(APPEND viewer_SOURCE_FILES llappviewerlinux.cpp)
    set_source_files_properties(
      llappviewerlinux.cpp
      PROPERTIES
      COMPILE_DEFINITIONS "${VIEWER_CHANNEL_VERSION_DEFINES}"
      )
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

endif (LINUX)

if (WINDOWS)

    list(APPEND viewer_SOURCE_FILES
         llappviewerwin32.cpp
         llwindebug.cpp
         )
    set_source_files_properties(
      llappviewerwin32.cpp
      PROPERTIES
      COMPILE_DEFINITIONS "${VIEWER_CHANNEL_VERSION_DEFINES}"
      )

    list(APPEND viewer_HEADER_FILES
         llappviewerwin32.h
         llwindebug.h
         )

    # Replace the icons with the appropriate ones for the channel
    # ('test' is the default)
    set(ICON_PATH "test")
    set(VIEWER_MACOSX_PHASE "d")
    string(TOLOWER ${VIEWER_CHANNEL} channel_lower)
    if(channel_lower MATCHES "^second life release")
        set(ICON_PATH "release")
        set(VIEWER_MACOSX_PHASE "f")
    elseif(channel_lower MATCHES "^second life beta")
        set(ICON_PATH "beta")
        set(VIEWER_MACOSX_PHASE "b")
    elseif(channel_lower MATCHES "^second life project")
        set(ICON_PATH "project")
        set(VIEWER_MACOSX_PHASE "a")
    endif()
    message(STATUS "Copying icons for ${ICON_PATH}")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/${ICON_PATH}/secondlife.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/ll_icon.ico"
      )
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/${ICON_PATH}/secondlife_256.BMP"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/ll_icon.BMP"
      )
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/${ICON_PATH}/secondlife_256.BMP"
        "${CMAKE_CURRENT_SOURCE_DIR}/res-sdl/ll_icon.BMP"
      )

    # Add resource files to the project.
    # viewerRes.rc is the only buildable file, but
    # the rest are all dependencies of it.
    set(viewer_RESOURCE_FILES
        res/arrow.cur
        res/arrowcop.cur
        res/arrowcopmulti.cur
        res/arrowdrag.cur
        res/circleandline.cur
        res/icon1.ico
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
        res/lltoolpathfinding.cur
        res/lltoolpathfindingpathend.cur
        res/lltoolpathfindingpathendadd.cur
        res/lltoolpathfindingpathstart.cur
        res/lltoolpathfindingpathstartadd.cur
        res/lltoolpipette.cur
        res/lltoolrotate.cur
        res/lltoolscale.cur
        res/lltooltranslate.cur
        res/lltoolzoomin.cur
        res/lltoolzoomout.cur
        res-sdl/ll_icon.BMP
        res/ll_icon.BMP
        res/ll_icon.ico
        res/resource.h
        res/toolpickobject.cur
        res/toolpickobject2.cur
        res/toolpickobject3.cur
        res/toolpipette.cur
        res/toolbuy.cur
        res/toolopen.cur
        res/toolsit.cur
        )

    set_source_files_properties(${viewer_RESOURCE_FILES}
                                PROPERTIES HEADER_FILE_ONLY TRUE)

    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/res/viewerRes.rc
                    ${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
                    )
    set(viewer_RESOURCE_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
        ${viewer_RESOURCE_FILES}
        )

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
      PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/res"
      )

    SOURCE_GROUP("Resource Files" FILES ${viewer_RESOURCE_FILES})

    list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})

    find_library(INTEL_MEMOPS_LIBRARY
                 NAMES ll_intel_memops
                 PATHS
                 optimized ${ARCH_PREBUILT_DIRS_RELEASE}
                 debug ${ARCH_PREBUILT_DIRS_DEBUG}
                 )
    mark_as_advanced(INTEL_MEMOPS_LIBRARY)


    if (INTEL_MEMOPS_LIBRARY)
      add_library( ll::intel_memops INTERFACE IMPORTED )
      target_link_libraries( ll::intel_memops ${INTEL_MEMOPS_LIBRARY} )
    endif (INTEL_MEMOPS_LIBRARY)

    if (ADDRESS_SIZE EQUAL 64)
        # We deliberately omit this from the 32bit build because it declares that
        # the viewer is compatible with Windows 10; we need that to properly detect
        # the Windows version, but doing so causes systems with certain HD video
        # cards to fail because Windows 10 does not support them.  Leaving this out
        # causes those systems to run in a Windows 8 compatibility mode, which works.
        LIST(APPEND viewer_SOURCE_FILES windows.manifest)
    endif (ADDRESS_SIZE EQUAL 64)

endif (WINDOWS)

# Add the xui files. This is handy for searching for xui elements
# from within the IDE.
file(GLOB_RECURSE viewer_XUI_FILES LIST_DIRECTORIES FALSE
    ${CMAKE_CURRENT_SOURCE_DIR}/skins/*.xml)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/skins PREFIX "XUI Files" FILES ${viewer_XUI_FILES})
set_source_files_properties(${viewer_XUI_FILES} 
                            PROPERTIES HEADER_FILE_ONLY TRUE)
list(APPEND viewer_SOURCE_FILES ${viewer_XUI_FILES})

# Add the shader sources
file(GLOB_RECURSE viewer_SHADER_FILES LIST_DIRECTORIES FALSE
    ${CMAKE_CURRENT_SOURCE_DIR}/app_settings/shaders/*.glsl)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/app_settings/shaders PREFIX "Shaders" FILES ${viewer_SHADER_FILES})
set_source_files_properties(${viewer_SHADER_FILES} 
                            PROPERTIES HEADER_FILE_ONLY TRUE)
list(APPEND viewer_SOURCE_FILES ${viewer_SHADER_FILES})

# Add lua script sources
file(GLOB_RECURSE viewer_LUA_SCRIPT_FILES LIST_DIRECTORIES FALSE
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/lua/*.lua)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/scripts/lua PREFIX "LUA Scripts" FILES ${viewer_LUA_SCRIPT_FILES})
set_source_files_properties(${viewer_LUA_SCRIPT_FILES} 
                            PROPERTIES HEADER_FILE_ONLY TRUE)
list(APPEND viewer_SOURCE_FILES ${viewer_LUA_SCRIPT_FILES})

set(viewer_APPSETTINGS_FILES
    app_settings/anim.ini
    app_settings/autoreplace.xml
    app_settings/cmd_line.xml
    app_settings/commands.xml
    app_settings/emoji_groups.xml
    app_settings/foldertypes.xml
    app_settings/grass.xml
    app_settings/ignorable_dialogs.xml
    app_settings/key_bindings.xml
    app_settings/keywords_lsl_default.xml
    app_settings/logcontrol.xml
    app_settings/settings.xml
    app_settings/settings_crash_behavior.xml
    app_settings/settings_files.xml
    app_settings/settings_per_account.xml
    app_settings/std_bump.ini
    app_settings/toolbars.xml
    app_settings/trees.xml
    app_settings/viewerart.xml
    app_settings/gamecontrollerdb.txt
    ${CMAKE_SOURCE_DIR}/../etc/message.xml
    ${CMAKE_SOURCE_DIR}/../scripts/messages/message_template.msg
    packages-info.txt
    featuretable.txt
    featuretable_mac.txt
    featuretable_linux.txt
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

if (WINDOWS)
  file(GLOB viewer_INSTALLER_FILES installers/windows/*.nsi)

  source_group("Installer Files" FILES ${viewer_INSTALLER_FILES})

  set_source_files_properties(${viewer_INSTALLER_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  list(APPEND viewer_SOURCE_FILES ${viewer_INSTALLER_FILES})
endif (WINDOWS)

if (HAVOK OR HAVOK_TPV)
  set(LLSTARTUP_COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS} -DLL_HAVOK")
endif (HAVOK OR HAVOK_TPV)

if( DEFINED LLSTARTUP_COMPILE_FLAGS )
   # progress view disables/enables icons based on available packages
   set_source_files_properties(llprogressview.cpp PROPERTIES COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS}")

   set_source_files_properties(llstartup.cpp PROPERTIES COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS}")
endif()

list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

add_executable(${VIEWER_BINARY_NAME}
    WIN32
    MACOSX_BUNDLE
    ${viewer_SOURCE_FILES}
    )

# add package files
file(GLOB EVENT_HOST_SCRIPT_GLOB_LIST
     ${CMAKE_CURRENT_SOURCE_DIR}/../viewer_components/*.py)
list(APPEND EVENT_HOST_SCRIPTS ${EVENT_HOST_SCRIPT_GLOB_LIST})

set(PACKAGE ON CACHE BOOL
    "Add a package target that builds an installer package.")

if(USE_PRECOMPILED_HEADERS)
  target_precompile_headers( ${VIEWER_BINARY_NAME} PRIVATE llviewerprecompiledheaders.h )
endif(USE_PRECOMPILED_HEADERS)

if (WINDOWS)
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        # *TODO -reenable this once we get server usage sorted out
        LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS /LARGEADDRESSAWARE"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO /LARGEADDRESSAWARE"
        LINK_FLAGS_RELEASE "/FORCE:MULTIPLE /MAP\"secondlife-bin.MAP\" /OPT:REF /LARGEADDRESSAWARE"
        )
    target_compile_options(${VIEWER_BINARY_NAME} PRIVATE /bigobj)

    # If adding a file to viewer_manifest.py in the WindowsManifest.construct() method, be sure to add the dependency
    # here.
    # *NOTE:Mani - This is a crappy hack to have important dependencies for the viewer_manifest copy action
    # be met. I'm looking forward to a source-code split-up project next year that will address this kind of thing.
    # In the meantime, if you have any ideas on how to easily maintain one list, either here or in viewer_manifest.py
    # and have the build deps get tracked *please* tell me about it.
    # nat: https://cmake.org/cmake/help/v3.14/command/file.html
    # "For example, the code
    # file(STRINGS myfile.txt myfile)
    # stores a list in the variable myfile in which each item is a line from the input file."
    # And of course it's straightforward to read a text file in Python.

    set(COPY_INPUT_DEPENDENCIES
      # The following commented dependencies are determined variably at build time. Can't do this here.
      ${CMAKE_SOURCE_DIR}/../etc/message.xml
      ${CMAKE_SOURCE_DIR}/../scripts/messages/message_template.msg
      ${SHARED_LIB_STAGING_DIR}/openjp2.dll
      ${SHARED_LIB_STAGING_DIR}/llwebrtc.dll
      #${SHARED_LIB_STAGING_DIR}/${LL_INTDIR}/SLVoice.exe
      #${SHARED_LIB_STAGING_DIR}/${LL_INTDIR}/libsndfile-1.dll
      #${SHARED_LIB_STAGING_DIR}/${LL_INTDIR}/vivoxoal.dll
      ${AUTOBUILD_INSTALL_DIR}/ca-bundle.crt
      ${CMAKE_CURRENT_SOURCE_DIR}/licenses-win32.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/featuretable.txt
      ${viewer_APPSETTINGS_FILES}
      SLPlugin
      media_plugin_cef
      media_plugin_libvlc
      media_plugin_example
      )

    if (ADDRESS_SIZE EQUAL 64)
       list(APPEND COPY_INPUT_DEPENDENCIES
               ${SHARED_LIB_STAGING_DIR}/vivoxsdk_x64.dll
               ${SHARED_LIB_STAGING_DIR}/ortp_x64.dll
               )
    else (ADDRESS_SIZE EQUAL 64)
       list(APPEND COPY_INPUT_DEPENDENCIES
               ${SHARED_LIB_STAGING_DIR}/vivoxsdk.dll
               ${SHARED_LIB_STAGING_DIR}/ortp.dll
               )
    endif (ADDRESS_SIZE EQUAL 64)

    if (TARGET ll::openal)
      list(APPEND COPY_INPUT_DEPENDENCIES
           ${SHARED_LIB_STAGING_DIR}/OpenAL32.dll
           ${SHARED_LIB_STAGING_DIR}/alut.dll
          )
    endif ()

    add_custom_command(
      OUTPUT  ${CMAKE_CFG_INTDIR}/copy_touched.bat
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --actions=copy
        --arch=${ARCH}
        --artwork=${ARTWORK_DIR}
        "--bugsplat=${BUGSPLAT_DB}"
        "--openal=${USE_OPENAL}"
        "--tracy=${USE_TRACY}"
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --buildtype=$<CONFIG>
        "--channel=${VIEWER_CHANNEL}"
        --configuration=${CMAKE_CFG_INTDIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>
        --grid=${GRID}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --touch=${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/copy_touched.bat
        --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        stage_third_party_libs
        llwebrtc
        ${COPY_INPUT_DEPENDENCIES}
      COMMENT "Performing viewer_manifest copy"
      )

    add_custom_target(copy_w_viewer_manifest ALL DEPENDS ${CMAKE_CFG_INTDIR}/copy_touched.bat)

    add_dependencies(${VIEWER_BINARY_NAME} stage_third_party_libs llcommon llwebrtc copy_w_viewer_manifest)

    # Useful for remote debugging or profiling if set to target a shared drive. for example, brad sets it to "dist/secondlife" to deploy under
    # the build tree (and then I share that directory) or "f:/dist/secondlife" to deploy to a network mounted drive (although this can be slow).
    # this will also create an "unpacked" tarball at build-<platform>/newview/unpacked_<platform>_7_1_11_<build num>.tar
    set(LOCAL_DIST_DIR "" CACHE PATH "Path for 'local' unpacked copy of viewer distribution to be deployed to.")
    set(product SecondLife-${ARCH}-${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION})
    if(IS_DIRECTORY ${LOCAL_DIST_DIR})
        add_custom_command(
            OUTPUT  ${LOCAL_DIST_DIR}/.${product}.copy_touched
            COMMAND ${PYTHON_EXECUTABLE}
            ARGS
            ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
            "--actions=\"copy package unpacked\""
            --arch=${ARCH}
            --artwork=${ARTWORK_DIR}
            "--bugsplat=${BUGSPLAT_DB}"
            "--openal=${USE_OPENAL}"
            "--tracy=${USE_TRACY}"
            --build=${CMAKE_CURRENT_BINARY_DIR}
            --buildtype=$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,${CMAKE_BUILD_TYPE}>
            "--channel=${VIEWER_CHANNEL}"
            --configuration=$<CONFIG>
            --dest=${LOCAL_DIST_DIR}
            --grid=${GRID}
            --source=${CMAKE_CURRENT_SOURCE_DIR}
            --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
            --touch=${LOCAL_DIST_DIR}/.${product}.copy_touched
            DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
            stage_third_party_libs
            llwebrtc
            ${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
            ${COPY_INPUT_DEPENDENCIES}
            ${VIEWER_BINARY_NAME}
            COMMENT "Performing viewer_manifest local dist copy"
            )

        add_custom_target(copy_local_dist_viewer_manifest ALL DEPENDS ${LOCAL_DIST_DIR}/.${product}.copy_touched)
    elseif (NOT LOCAL_DIST_DIR STREQUAL "")
        message(FATAL_ERROR "LOCAL_DIST_DIR ${LOCAL_DIST_DIR} specified but is not valid target directory to copy viewer distribution into.  Please create the directory and try again")
    endif ()  # IS_DIRECTORY ${LOCAL_DIST_DIR}

    if (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)
      add_dependencies(${VIEWER_BINARY_NAME} copy_win_scripts)
    endif (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)

    add_dependencies(${VIEWER_BINARY_NAME} SLPlugin)

    # sets the 'working directory' for debugging from visual studio.
    # Condition for version can be moved to requirements once build agents will be updated (see TOOL-3865)
    if (NOT UNATTENDED)
        set_property(
          TARGET ${VIEWER_BINARY_NAME}
          PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
          )
    endif (NOT UNATTENDED)

    if (PACKAGE)
      add_custom_command(
        OUTPUT ${CMAKE_CFG_INTDIR}/touched.bat
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
              ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
              --arch=${ARCH}
              --artwork=${ARTWORK_DIR}
              "--bugsplat=${BUGSPLAT_DB}"
              "--openal=${USE_OPENAL}"
              "--tracy=${USE_TRACY}"
              --build=${CMAKE_CURRENT_BINARY_DIR}
              --buildtype=$<CONFIG>
              "--channel=${VIEWER_CHANNEL}"
              --configuration=${CMAKE_CFG_INTDIR}
              --dest=${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>
              --grid=${GRID}
              --source=${CMAKE_CURRENT_SOURCE_DIR}
              --touch=${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/touched.bat
              --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
        DEPENDS
            ${VIEWER_BINARY_NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
            ${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
            ${COPY_INPUT_DEPENDENCIES}
        )

      add_custom_target(llpackage ALL DEPENDS
              ${CMAKE_CFG_INTDIR}/touched.bat
        )

    endif (PACKAGE)
elseif (DARWIN)
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        LINK_FLAGS_RELEASE "${LINK_FLAGS_RELEASE} -Xlinker -dead_strip -Xlinker -map -Xlinker ${CMAKE_CURRENT_BINARY_DIR}/${VIEWER_BINARY_NAME}.MAP"
        )
else (WINDOWS)
        # Linux
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        LINK_FLAGS_RELEASE "${LINK_FLAGS_RELEASE} -no-pie -Wl,--Map=${VIEWER_BINARY_NAME}.MAP"
        )
endif (WINDOWS)

# *NOTE: - this list is very sensitive to ordering, test carefully on all
# platforms if you change the relative order of the entries here.
# In particular, cmake 2.6.4 (when building with linux/makefile generators)
# appears to sometimes de-duplicate redundantly listed dependencies improperly.
# To work around this, higher level modules should be listed before the modules
# that they depend upon. -brad
#
# *NOTE:  On mixing system shared libraries and updated static archives.
# We use a number of libraries that have an existence as system libraries,
# internal-use libraries and applications libraries.  The most-referenced
# one of these being libz where you can find four or more versions in play
# at once.  On Linux, libz can be found at link and run time via a number
# of paths:
#     
#      => -lfreetype
#        => libz.so.1 (on install machine, not build)
#      => -lSDL
#        => libz.so.1 (on install machine, not build)
#      => -lgdk-x11-2.0
#        => libz.so.1
#      => -lz
#
# We generally want the newest version of the library to provide all symbol
# resolution.  To that end, when using static archives, the *_PRELOAD_ARCHIVES
# variables, PNG_PRELOAD_ARCHIVES and ZLIBNG_PRELOAD_ARCHIVES, get the archives
# dumped into the target binary and runtime lookup will find the most
# modern version.

target_link_libraries(${VIEWER_BINARY_NAME}
        llaudio
        llcharacter
        llimage
        llinventory
        llmessage
        llplugin
        llprimitive
        llrender
        llui
        llfilesystem
        llwindow
        llxml
        llmath
        llcorehttp
        llcommon
        llmeshoptimizer
        llwebrtc
        lllogin
        llprimitive
        llappearance
        ${LLPHYSICSEXTENSIONS_LIBRARIES}
        ll::bugsplat
        ll::lualibs
        ll::ndof
        ll::tracy
        ll::openxr
        ll::nfde
        )

if( TARGET ll::intel_memops )
   target_link_libraries(${VIEWER_BINARY_NAME} ll::intel_memops )
endif()

if( TARGET ll::nvapi )
   target_link_libraries(${VIEWER_BINARY_NAME} ll::nvapi )
endif()

set(ARTWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
    "Path to artwork files.")

message("Copying fonts")
file(GLOB FONT_FILE_GLOB_LIST
  "${AUTOBUILD_INSTALL_DIR}/fonts/*"
)
file(COPY ${FONT_FILE_GLOB_LIST} DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/fonts")

# Copy over the Emoji/shortcodes mapping XML files (and create dependency
# if they are changed, CMake will run again and copy over new versions)
message("Copying Emoji/shortcode mappings")
set(emoji_mapping_src_folder ${AUTOBUILD_INSTALL_DIR}/xui)
set(emoji_mapping_dst_folder ${CMAKE_CURRENT_SOURCE_DIR}/skins/default/xui)

# Note Turkey is missing from this set (not available in Emoji package yet)
set(country_codes "da;de;en;es;fr;it;ja;pl;pt;ru;zh")
foreach(elem ${country_codes})
   set(emoji_mapping_src_file
      "${emoji_mapping_src_folder}/${elem}/emoji_characters.xml")
   set(emoji_mapping_dst_file
      "${emoji_mapping_dst_folder}/${elem}/emoji_characters.xml")      
   configure_file(${emoji_mapping_src_file} ${emoji_mapping_dst_file} COPYONLY)
endforeach()

if (LINUX)
  set(product SecondLife-${ARCH}-${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION})

  # These are the generated targets that are copied to package/
  set(COPY_INPUT_DEPENDENCIES
    ${VIEWER_BINARY_NAME}
    SLPlugin
    #media_plugin_gstreamer010
    llcommon
    )

  #if (NOT USE_BUGSPLAT)
  #    LIST(APPEND COPY_INPUT_DEPENDENCIES linux-crash-logger)
  #endif (NOT USE_BUGSPLAT)

  add_custom_command(
      OUTPUT ${product}.tar.xz
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --arch=${ARCH}
        --artwork=${ARTWORK_DIR}
        "--bugsplat=${BUGSPLAT_DB}"
        "--openal=${USE_OPENAL}"
        "--tracy=${USE_TRACY}"
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --buildtype=${CMAKE_BUILD_TYPE}
        "--channel=${VIEWER_CHANNEL}"
        --configuration=${CMAKE_CFG_INTDIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
        --grid=${GRID}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
        --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        ${COPY_INPUT_DEPENDENCIES}
      )

  if (PACKAGE)
  endif (PACKAGE)

  add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.copy_touched
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --actions=copy
      --arch=${ARCH}
      --artwork=${ARTWORK_DIR}
      "--bugsplat=${BUGSPLAT_DB}"
      "--openal=${USE_OPENAL}"
      "--tracy=${USE_TRACY}"
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --buildtype=${CMAKE_BUILD_TYPE}
      "--channel=${VIEWER_CHANNEL}"
      --configuration=${CMAKE_CFG_INTDIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
      --grid=${GRID}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
      --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      ${COPY_INPUT_DEPENDENCIES}
    COMMENT "Performing viewer_manifest copy"
    )

  add_custom_target(copy_l_viewer_manifest ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.copy_touched)

  if (PACKAGE)
    add_custom_target(llpackage ALL DEPENDS ${product}.tar.xz)
    # Make sure we don't run two instances of viewer_manifest.py at the same time.
    add_dependencies(llpackage copy_l_viewer_manifest)
    check_message_template(llpackage)
  endif (PACKAGE)
endif (LINUX)

if (DARWIN)
  # These all get set with PROPERTIES. It's not that the property names are
  # magically known to CMake -- it's that these names are referenced in the
  # Info-SecondLife.plist file in the configure_file() directive below.
  set(product "${VIEWER_CHANNEL}")
  set(MACOSX_EXECUTABLE_NAME "${VIEWER_CHANNEL}")
  set(MACOSX_BUNDLE_INFO_STRING "${VIEWER_CHANNEL}")
  set(MACOSX_BUNDLE_ICON_FILE "secondlife.icns")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.secondlife.indra.viewer")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VIEWER_CHANNEL} ${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "SecondLife")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${VIEWER_SHORT_VERSION}${VIEWER_MACOSX_PHASE}${VIEWER_REVISION}")
  set(MACOSX_BUNDLE_COPYRIGHT "Copyright © Linden Research, Inc. 2020")
  set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "SecondLife.nib")
  set(MACOSX_BUNDLE_NSPRINCIPAL_CLASS "LLApplication")

  # https://blog.kitware.com/upcoming-in-cmake-2-8-12-osx-rpath-support/
  set(CMAKE_MACOSX_RPATH 1)
  
  set_target_properties(
    ${VIEWER_BINARY_NAME}
    PROPERTIES
    OUTPUT_NAME "${product}"
    # From Contents/MacOS/SecondLife, look in Contents/Frameworks
    INSTALL_RPATH "@loader_path/../Frameworks"
    # SIGH, as of 2018-05-24 (cmake 3.11.1) the INSTALL_RPATH property simply
    # does not work. Try this:
    LINK_FLAGS "-rpath @loader_path/../Frameworks"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-SecondLife.plist"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
    )

  set(VIEWER_APP_BUNDLE "${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/${product}.app")
  set(VIEWER_APP_EXE "${VIEWER_APP_BUNDLE}/Contents/MacOS/${product}")

  configure_file(
     "${CMAKE_CURRENT_SOURCE_DIR}/Info-SecondLife.plist"
     "${VIEWER_APP_BUNDLE}/Contents/Info.plist"
    )

  add_custom_command(
    TARGET ${VIEWER_BINARY_NAME} POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --actions=copy
      --arch=${ARCH}
      --artwork=${ARTWORK_DIR}
      "--bugsplat=${BUGSPLAT_DB}"
      "--openal=${USE_OPENAL}"
      "--tracy=${USE_TRACY}"
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --buildtype=$<CONFIG>
      --bundleid=${MACOSX_BUNDLE_GUI_IDENTIFIER}
      "--channel=${VIEWER_CHANNEL}"
      --configuration=${CMAKE_CFG_INTDIR}
      --dest=${VIEWER_APP_BUNDLE}
      --grid=${GRID}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
      --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
    DEPENDS
      ${VIEWER_BINARY_NAME}
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
    )

  add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_libvlc media_plugin_cef)

  if (ENABLE_SIGNING)
      set(SIGNING_SETTING "--signature=${SIGNING_IDENTITY}")
  else (ENABLE_SIGNING)
      set(SIGNING_SETTING "")
  endif (ENABLE_SIGNING)

  if (PACKAGE)
      add_custom_target(llpackage ALL DEPENDS ${VIEWER_BINARY_NAME})

      add_custom_command(
        TARGET llpackage POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
              ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
              --arch=${ARCH}
              --artwork=${ARTWORK_DIR}
              "--bugsplat=${BUGSPLAT_DB}"
              "--openal=${USE_OPENAL}"
              "--tracy=${USE_TRACY}"
              --build=${CMAKE_CURRENT_BINARY_DIR}
              --buildtype=$<CONFIG>
              "--channel=${VIEWER_CHANNEL}"
              --configuration=${CMAKE_CFG_INTDIR}
              --dest=${VIEWER_APP_BUNDLE}
              --grid=${GRID}
              --source=${CMAKE_CURRENT_SOURCE_DIR}
              --touch=${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/.${product}.bat
              --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
              ${SIGNING_SETTING}
        DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )
  endif (PACKAGE)
endif (DARWIN)

if (INSTALL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/ViewerInstall.cmake)
endif (INSTALL)

if (PACKAGE AND (RELEASE_CRASH_REPORTING OR NON_RELEASE_CRASH_REPORTING) AND VIEWER_SYMBOL_FILE)
  if (USE_BUGSPLAT)
    # BugSplat symbol-file generation
    if (WINDOWS)
      set(VIEWER_APP_SYMBOLS_ARCHIVE "${SYMBOLS_STAGING_DIR}.sym.tar.xz")
      set_target_properties( ${VIEWER_BINARY_NAME} PROPERTIES PDB_OUTPUT_DIRECTORY "${SYMBOLS_STAGING_DIR}")

      # Just pack up a tarball containing only the .pdb files for the
      # executables.
      add_custom_command(OUTPUT "${VIEWER_APP_SYMBOLS_ARCHIVE}"
        COMMAND "tar"
        ARGS
          "cJf"
          "${VIEWER_CHANNEL}.sym.tar.xz"
          "${VIEWER_CHANNEL}"
        DEPENDS "${VIEWER_BINARY_NAME}" llwebrtc
        WORKING_DIRECTORY "${SYMBOLS_STAGING_DIR}/.."
        COMMENT "Packing viewer PDBs into ${VIEWER_APP_SYMBOLS_ARCHIVE}"
        )
      add_custom_target(generate_symbols DEPENDS "${VIEWER_APP_SYMBOLS_ARCHIVE}")
      add_dependencies(generate_symbols ${VIEWER_BINARY_NAME} llwebrtc)

    endif (WINDOWS)
    if (DARWIN)
      set(VIEWER_APP_XCARCHIVE "${SYMBOLS_STAGING_DIR}.xcarchive.zip")

      # we only need an xcarchive with dSYMs (including the application)
      set_target_properties(${VIEWER_BINARY_NAME}
            PROPERTIES
            XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
            XCODE_ATTRIBUTE_DWARF_DSYM_FOLDER_PATH "${SYMBOLS_STAGING_DIR}/dSYMs")
    
      add_custom_command(OUTPUT "${VIEWER_APP_XCARCHIVE}"
        COMMAND "zip"
        ARGS
          "-r"
          "${VIEWER_APP_XCARCHIVE}"
            "${VIEWER_CHANNEL}"
          WORKING_DIRECTORY "${SYMBOLS_STAGING_DIR}/.."
          DEPENDS "${VIEWER_BINARY_NAME}" llwebrtc
          COMMENT "Generating ${VIEWER_APP_XCARCHIVE} for upload to BugSplat"
        )
      add_custom_target(generate_symbols DEPENDS
        "${VIEWER_APP_XCARCHIVE}"
        )
    endif (DARWIN)
    if (LINUX)
      # TBD
    endif (LINUX)

    add_dependencies(llpackage generate_symbols)
  endif (USE_BUGSPLAT)
endif ()

if (LL_TESTS)
  # To add a viewer unit test, just add the test .cpp file below
  # This creates a separate test project per file listed.
  include(LLAddBuildTest)
  SET(viewer_TEST_SOURCE_FILES
    llagentaccess.cpp
    lldateutil.cpp
#    llmediadataclient.cpp
    lllogininstance.cpp
#    llremoteparcelrequest.cpp
    llviewerhelputil.cpp
    llversioninfo.cpp
#    llvocache.cpp  
    llworldmap.cpp
    llworldmipmap.cpp
  )

  set_source_files_properties(
    llworldmap.cpp
    llworldmipmap.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES 
    tests/llviewertexture_stub.cpp
    #llviewertexturelist.cpp
  )

#  set_source_files_properties(
#    llvocache.cpp
#    PROPERTIES
#    LL_TEST_ADDITIONAL_SOURCE_FILES ../llmessage/lldatapacker.cpp
#    LL_TEST_ADDITIONAL_PROJECTS "llprimitive"
#  )

  set(test_libs
          llcommon
          llfilesystem
          llxml
          llmessage
          llcharacter
          llui
          lllogin
          llplugin
          llappearance
          ll::SDL2
    )

  set_source_files_properties(
    llmediadataclient.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${test_libs}"
  )

  set_source_files_properties(
    llworldmap.cpp
    llworldmipmap.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES 
    tests/llviewertexture_stub.cpp
  )

  set_source_files_properties(
    llmediadataclient.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES llprimitive
  )

  set_source_files_properties(
    lllogininstance.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES llversioninfo.cpp
  )

  set_property( SOURCE
          ${viewer_TEST_SOURCE_FILES}
          PROPERTY
          LL_TEST_ADDITIONAL_LIBRARIES ${test_libs}
  )

  LL_ADD_PROJECT_UNIT_TESTS(${VIEWER_BINARY_NAME} "${viewer_TEST_SOURCE_FILES}")

  #set(TEST_DEBUG on)

  set(test_libs
          llfilesystem
          llmath
          llcommon
          llmessage
          llcorehttp
          llxml
          llui
          llplugin
          llappearance
          lllogin
          llprimitive
          lllogin
          ll::SDL2
          )

  LL_ADD_INTEGRATION_TEST(cppfeatures
    ""
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llluamanager
    "llluamanager.cpp"
    "${test_libs};ll::lualibs"
    )

  LL_ADD_INTEGRATION_TEST(llsechandler_basic
    llsechandler_basic.cpp
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llsecapi
     llsecapi.cpp
    "${test_libs}"
    )

  set(llslurl_test_sources
      llslurl.cpp
      llviewernetwork.cpp
  )

  LL_ADD_INTEGRATION_TEST(llslurl
     "${llslurl_test_sources}"
    "${test_libs}"
    )

  set(llviewercontrollistener_test_sources
    llviewercontrollistener.cpp
    ../llxml/llcontrol.cpp
    ../llxml/llxmltree.cpp
    ../llxml/llxmlparser.cpp
    ../llcommon/commoncontrol.cpp
    )

  LL_ADD_INTEGRATION_TEST(llviewercontrollistener
    "${llviewercontrollistener_test_sources}"
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llviewernetwork
     llviewernetwork.cpp
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llviewerassetstats
    llviewerassetstats.cpp
    "${test_libs}"
    )

# LL_ADD_INTEGRATION_TEST(llhttpretrypolicy "llhttpretrypolicy.cpp" "${test_libs}")

  #ADD_VIEWER_BUILD_TEST(llmemoryview viewer)
  #ADD_VIEWER_BUILD_TEST(llagentaccess viewer)
  #ADD_VIEWER_BUILD_TEST(lltextureinfo viewer)
  #ADD_VIEWER_BUILD_TEST(lltextureinfodetails viewer)


endif (LL_TESTS)

check_message_template(${VIEWER_BINARY_NAME})
