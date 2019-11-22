;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Second Life setup.nsi
;; Copyright 2004-2015, Linden Research, Inc.
;;
;; This library is free software; you can redistribute it and/or
;; modify it under the terms of the GNU Lesser General Public
;; License as published by the Free Software Foundation;
;; version 2.1 of the License only.
;;
;; This library is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; Lesser General Public License for more details.
;;
;; You should have received a copy of the GNU Lesser General Public
;; License along with this library; if not, write to the Free Software
;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
;;
;; Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
;;
;; NSIS 3 or higher required for Unicode support
;;
;; Author: James Cook, TankMaster Finesmith, Don Kjer, Callum Prentice
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Compiler flags
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Unicode true
SetOverwrite on				# Overwrite files
SetCompress auto			# Compress if saves space
SetCompressor /solid lzma	# Compress whole installer as one block
SetDatablockOptimize off	# Only saves us 0.1%, not worth it
XPStyle on                  # Add an XP manifest to the installer
RequestExecutionLevel admin	# For when we write to Program Files

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Project flags
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

# This placeholder is replaced by viewer_manifest.py
%%VERSION%%

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; - language files - one for each language (or flavor thereof)
;; (these files are in the same place as the nsi template but the python script generates a new nsi file in the 
;; application directory so we have to add a path to these include files)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Ansariel notes: "Under certain circumstances the installer will fall back
;; to the first defined (aka default) language version. So you want to include
;; en-us as first language file."
!include "%%SOURCE%%\installers\windows\lang_en-us.nsi"

# Danish and Polish no longer supported by the viewer itself
##!include "%%SOURCE%%\installers\windows\lang_da.nsi"
!include "%%SOURCE%%\installers\windows\lang_de.nsi"
!include "%%SOURCE%%\installers\windows\lang_es.nsi"
!include "%%SOURCE%%\installers\windows\lang_fr.nsi"
!include "%%SOURCE%%\installers\windows\lang_ja.nsi"
!include "%%SOURCE%%\installers\windows\lang_it.nsi"
##!include "%%SOURCE%%\installers\windows\lang_pl.nsi"
!include "%%SOURCE%%\installers\windows\lang_pt-br.nsi"
!include "%%SOURCE%%\installers\windows\lang_ru.nsi"
!include "%%SOURCE%%\installers\windows\lang_tr.nsi"
!include "%%SOURCE%%\installers\windows\lang_zh.nsi"

# *TODO: Move these into the language files themselves
##LangString LanguageCode ${LANG_DANISH}   "da"
LangString LanguageCode ${LANG_GERMAN}   "de"
LangString LanguageCode ${LANG_ENGLISH}  "en"
LangString LanguageCode ${LANG_SPANISH}  "es"
LangString LanguageCode ${LANG_FRENCH}   "fr"
LangString LanguageCode ${LANG_JAPANESE} "ja"
LangString LanguageCode ${LANG_ITALIAN}  "it"
##LangString LanguageCode ${LANG_POLISH}   "pl"
LangString LanguageCode ${LANG_PORTUGUESEBR} "pt"
LangString LanguageCode ${LANG_RUSSIAN}  "ru"
LangString LanguageCode ${LANG_TURKISH}  "tr"
LangString LanguageCode ${LANG_TRADCHINESE}  "zh"

# This placeholder is replaced by viewer_manifest.py
%%INST_VARS%%

Name ${INSTNAME}

SubCaption 0 $(LicenseSubTitleSetup)	# Override "license agreement" text

!define MUI_ICON   "%%SOURCE%%\installers\windows\install_icon.ico"
!define MUI_UNICON "%%SOURCE%%\installers\windows\uninstall_icon.ico"

BrandingText " "						# Bottom of window text
Icon          "${MUI_ICON}"
UninstallIcon "${MUI_UNICON}"
WindowIcon on							# Show our icon in left corner
BGGradient off							# No big background window
CRCCheck on								# Make sure CRC is OK
InstProgressFlags smooth colored		# New colored smooth look
SetOverwrite on							# Overwrite files by default
AutoCloseWindow true					# After all files install, close window

# Registry key paths, ours and Microsoft's
!define LINDEN_KEY      "SOFTWARE\Linden Research, Inc."
!define INSTNAME_KEY    "${LINDEN_KEY}\${INSTNAME}"
!define MSCURRVER_KEY   "SOFTWARE\Microsoft\Windows\CurrentVersion"
!define MSNTCURRVER_KEY "SOFTWARE\Microsoft\Windows NT\CurrentVersion"
!define MSUNINSTALL_KEY "${MSCURRVER_KEY}\Uninstall\${INSTNAME}"

# from http://nsis.sourceforge.net/Docs/MultiUser/Readme.html
### Highest level permitted for user: Admin for Admin, Standard for Standard
##!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_EXECUTIONLEVEL Admin
!define MULTIUSER_MUI
### Look for /AllUsers or /CurrentUser switches
##!define MULTIUSER_INSTALLMODE_COMMANDLINE
# appended to $PROGRAMFILES, as affected by MULTIUSER_USE_PROGRAMFILES64
!define MULTIUSER_INSTALLMODE_INSTDIR "${INSTNAME}"
# expands to !define MULTIUSER_USE_PROGRAMFILES64 or nothing
%%PROGRAMFILES%%
# should make MultiUser.nsh initialization read existing INSTDIR from registry
## SL-10506: don't
##!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "${INSTNAME_KEY}"
##!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME ""
# Don't set MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY and
# MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME to cause the installer to
# write $MultiUser.InstallMode to the registry, because when the user installs
# multiple viewers with the same channel (same ${INSTNAME}, hence same
# ${INSTNAME_KEY}), the registry entry is overwritten. Instead we'll write a
# little file into the install directory -- see .onInstSuccess and un.onInit.
!include MultiUser.nsh
!include MUI2.nsh
!define MUI_BGCOLOR FFFFFF
!insertmacro MUI_FUNCTION_GUIINIT

UninstallText $(UninstallTextMsg)
DirText $(DirectoryChooseTitle) $(DirectoryChooseSetup)
##!insertmacro MULTIUSER_PAGE_INSTALLMODE
!define MUI_PAGE_CUSTOMFUNCTION_PRE dirPre
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Var INSTNAME
Var INSTEXE
Var VIEWER_EXE
Var INSTSHORTCUT
Var COMMANDLINE         # Command line passed to this installer, set in .onInit
Var SHORTCUT_LANG_PARAM # "--set InstallLanguage de", Passes language to viewer
Var SKIP_DIALOGS        # Set from command line in  .onInit. autoinstall GUI and the defaults.
Var SKIP_AUTORUN		# Skip automatic launch of the viewer after install
Var DO_UNINSTALL_V2     # If non-null, path to a previous Viewer 2 installation that will be uninstalled.

# Function definitions should go before file includes, because calls to
# DLLs like LangDLL trigger an implicit file include, so if that call is at
# the end of this script NSIS has to decompress the whole installer before 
# it can call the DLL function. JC

!include "FileFunc.nsh"     # For GetParameters, GetOptions
!insertmacro GetParameters
!insertmacro GetOptions
!include WinVer.nsh			# For OS and SP detection
!include 'LogicLib.nsh'     # for value comparison
!include "x64.nsh"			# for 64bit detection

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Pre-directory page callback
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function dirPre
    StrCmp $SKIP_DIALOGS "true" 0 +2
	Abort

FunctionEnd    

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Prep Installer Section
;;
;; Note: to add new languages, add a language file include to the list 
;; at the top of this file, add an entry to the menu and then add an 
;; entry to the language ID selector below
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function .onInit
!insertmacro MULTIUSER_INIT

%%ENGAGEREGISTRY%%

# SL-10506: Setting MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY and
# MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME should
# read the current location of the install for this version into INSTDIR.
# However, SL-10506 complains about the resulting behavior, so the logic below
# is adapted from before we introduced MultiUser.nsh.

# if $0 is empty, this is the first time for this viewer name
ReadRegStr $0 SHELL_CONTEXT "${INSTNAME_KEY}" ""

# viewer with this name was installed before
${If} $0 != ""
	# use the value we got from registry as install location
    StrCpy $INSTDIR $0
${EndIf}

Call CheckCPUFlags							# Make sure we have SSE2 support
Call CheckWindowsVersion					# Don't install On unsupported systems
    Push $0
    ${GetParameters} $COMMANDLINE			# Get our command line

    ${GetOptions} $COMMANDLINE "/SKIP_DIALOGS" $0   
    IfErrors +2 0	# If error jump past setting SKIP_DIALOGS
        StrCpy $SKIP_DIALOGS "true"

	${GetOptions} $COMMANDLINE "/SKIP_AUTORUN" $0
	IfErrors +2 0 ; If error jump past setting SKIP_AUTORUN
		StrCpy $SKIP_AUTORUN "true"

    ${GetOptions} $COMMANDLINE "/LANGID=" $0	# /LANGID=1033 implies US English

# If no language (error), then proceed
    IfErrors lbl_configure_default_lang
# No error means we got a language, so use it
    StrCpy $LANGUAGE $0
    Goto lbl_return

lbl_configure_default_lang:
# If we currently have a version of SL installed, default to the language of that install
# Otherwise don't change $LANGUAGE and it will default to the OS UI language.
    ReadRegStr $0 SHELL_CONTEXT "${INSTNAME_KEY}" "InstallerLanguage"
    IfErrors +2 0	# If error skip the copy instruction 
	StrCpy $LANGUAGE $0

# For silent installs, no language prompt, use default
    IfSilent 0 +3
    StrCpy $SKIP_AUTORUN "true"
    Goto lbl_return
    StrCmp $SKIP_DIALOGS "true" lbl_return
  
	Push ""
# Use separate file so labels can be UTF-16 but we can still merge changes into this ASCII file. JC
    !include "%%SOURCE%%\installers\windows\language_menu.nsi"
    
	Push A	# A means auto count languages for the auto count to work the first empty push (Push "") must remain
	LangDLL::LangDialog $(InstallerLanguageTitle) $(SelectInstallerLanguage)
	Pop $0
	StrCmp $0 "cancel" 0 +2
		Abort
    StrCpy $LANGUAGE $0

# Save language in registry		
	WriteRegStr SHELL_CONTEXT "${INSTNAME_KEY}" "InstallerLanguage" $LANGUAGE
lbl_return:
    Pop $0
    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Prep Uninstaller Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.onInit
    # Save $INSTDIR -- it appears to have the correct value before
    # MULTIUSER_UNINIT, but then gets munged by MULTIUSER_UNINIT?!
    Push $INSTDIR
    !insertmacro MULTIUSER_UNINIT
    Pop $INSTDIR

    # Now read InstallMode.txt from $INSTDIR
    Push $0
    ClearErrors
    FileOpen $0 "$INSTDIR\InstallMode.txt" r
    IfErrors skipread
    FileRead $0 $MultiUser.InstallMode
    FileClose $0
skipread:
    Pop $0

%%ENGAGEREGISTRY%%

# Read language from registry and set for uninstaller. Key will be removed on successful uninstall
	ReadRegStr $0 SHELL_CONTEXT "${INSTNAME_KEY}" "InstallerLanguage"
    IfErrors lbl_end
	StrCpy $LANGUAGE $0
lbl_end:

##  MessageBox MB_OK "After restoring:$\n$$INSTDIR = '$INSTDIR'$\n$$MultiUser.InstallMode = '$MultiUser.InstallMode'$\n$$LANGUAGE = '$LANGUAGE'"

    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Checks for CPU valid (must have SSE2 support)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckCPUFlags
    Push $1
    System::Call 'kernel32::IsProcessorFeaturePresent(i) i(10) .r1'
    IntCmp $1 1 OK_SSE2
    MessageBox MB_OKCANCEL $(MissingSSE2) /SD IDOK IDOK OK_SSE2
    Quit

  OK_SSE2:
    Pop $1
    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Make sure this computer meets the minimum system requirements.
;; Currently: Windows Vista SP2
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckWindowsVersion
  ${If} ${AtMostWin2003}
    MessageBox MB_OK $(CheckWindowsVersionMB)
    Quit
  ${EndIf}

  ${If} ${IsWinVista}
  ${AndIfNot} ${IsServicePack} 2
    MessageBox MB_OK $(CheckWindowsVersionMB)
    Quit
  ${EndIf}

  ${If} ${IsWin2008}
  ${AndIfNot} ${IsServicePack} 2
    MessageBox MB_OK $(CheckWindowsVersionMB)
    Quit
  ${EndIf}

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Install Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section ""

# SetShellVarContext is set by MultiUser.nsh initialization.

# Start with some default values.
StrCpy $INSTNAME "${INSTNAME}"
StrCpy $INSTEXE "${INSTEXE}"
StrCpy $VIEWER_EXE "${VIEWER_EXE}"
StrCpy $INSTSHORTCUT "${SHORTCUT}"

Call CheckIfAdministrator		# Make sure the user can install/uninstall
Call CloseSecondLife			# Make sure Second Life not currently running
Call CheckWillUninstallV2		# Check if Second Life is already installed

StrCmp $DO_UNINSTALL_V2 "" PRESERVE_DONE
PRESERVE_DONE:

# Viewer had "SLLauncher" for some time and we was seting "IsHostApp" for viewer, make sure to clean it up
DeleteRegValue HKEY_CLASSES_ROOT "Applications\$VIEWER_EXE" "IsHostApp"
DeleteRegValue HKEY_CLASSES_ROOT "Applications\$VIEWER_EXE" "NoStartPage"
ClearErrors

Call RemoveProgFilesOnInst		# Remove existing files to prevent certain errors when running the new version of the viewer

# This placeholder is replaced by the complete list of all the files in the installer, by viewer_manifest.py
%%INSTALL_FILES%%

# Pass the installer's language to the client to use as a default
StrCpy $SHORTCUT_LANG_PARAM "--set InstallLanguage $(LanguageCode)"

# Shortcuts in start menu
CreateDirectory	"$SMPROGRAMS\$INSTSHORTCUT"
SetOutPath "$INSTDIR"
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\$INSTSHORTCUT.lnk" \
				"$INSTDIR\$VIEWER_EXE" "$SHORTCUT_LANG_PARAM" "$INSTDIR\$VIEWER_EXE"


WriteINIStr		"$SMPROGRAMS\$INSTSHORTCUT\SL Create Account.url" \
				"InternetShortcut" "URL" \
				"http://join.secondlife.com/"
WriteINIStr		"$SMPROGRAMS\$INSTSHORTCUT\SL Your Account.url" \
				"InternetShortcut" "URL" \
				"http://www.secondlife.com/account/"
WriteINIStr		"$SMPROGRAMS\$INSTSHORTCUT\SL Scripting Language Help.url" \
				"InternetShortcut" "URL" \
                "http://wiki.secondlife.com/wiki/LSL_Portal"
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\Uninstall $INSTSHORTCUT.lnk" \
				'"$INSTDIR\uninst.exe"' ''

# Other shortcuts
SetOutPath "$INSTDIR"
CreateShortCut "$DESKTOP\$INSTSHORTCUT.lnk" \
        "$INSTDIR\$VIEWER_EXE" "$SHORTCUT_LANG_PARAM" "$INSTDIR\$VIEWER_EXE"
CreateShortCut "$INSTDIR\$INSTSHORTCUT.lnk" \
        "$INSTDIR\$VIEWER_EXE" "$SHORTCUT_LANG_PARAM" "$INSTDIR\$VIEWER_EXE"
CreateShortCut "$INSTDIR\Uninstall $INSTSHORTCUT.lnk" \
				'"$INSTDIR\uninst.exe"' ''

# Write registry
WriteRegStr SHELL_CONTEXT "${INSTNAME_KEY}" "" "$INSTDIR"
WriteRegStr SHELL_CONTEXT "${INSTNAME_KEY}" "Version" "${VERSION_LONG}"
WriteRegStr SHELL_CONTEXT "${INSTNAME_KEY}" "Shortcut" "$INSTSHORTCUT"
WriteRegStr SHELL_CONTEXT "${INSTNAME_KEY}" "Exe" "$VIEWER_EXE"
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "Publisher" "Linden Research, Inc."
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "URLInfoAbout" "http://secondlife.com/whatis/"
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "URLUpdateInfo" "http://secondlife.com/support/downloads/"
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "HelpLink" "https://support.secondlife.com/contact-support/"
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "DisplayName" "$INSTNAME"
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninst.exe"'
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "DisplayVersion" "${VERSION_LONG}"
WriteRegDWORD SHELL_CONTEXT "${MSUNINSTALL_KEY}" "EstimatedSize" "0x0001D500"		# ~117 MB

# from FS:Ansariel
WriteRegStr SHELL_CONTEXT "${MSUNINSTALL_KEY}" "DisplayIcon" '"$INSTDIR\$VIEWER_EXE"'

# BUG-2707 Disable SEHOP for installed viewer.
WriteRegDWORD SHELL_CONTEXT "${MSNTCURRVER_KEY}\Image File Execution Options\$VIEWER_EXE" "DisableExceptionChainValidation" 1

# Write URL registry info
WriteRegStr HKEY_CLASSES_ROOT "${URLNAME}" "(default)" "URL:Second Life"
WriteRegStr HKEY_CLASSES_ROOT "${URLNAME}" "URL Protocol" ""
WriteRegStr HKEY_CLASSES_ROOT "${URLNAME}\DefaultIcon" "" '"$INSTDIR\$VIEWER_EXE"'

# URL param must be last item passed to viewer, it ignores subsequent params to avoid parameter injection attacks.
# MAINT-8305: On SLURL click, directly invoke the viewer, not the launcher.
WriteRegExpandStr HKEY_CLASSES_ROOT "${URLNAME}\shell\open\command" "" '"$INSTDIR\$VIEWER_EXE" -url "%1"'
WriteRegStr HKEY_CLASSES_ROOT "x-grid-location-info" "(default)" "URL:Second Life"
WriteRegStr HKEY_CLASSES_ROOT "x-grid-location-info" "URL Protocol" ""
WriteRegStr HKEY_CLASSES_ROOT "x-grid-location-info\DefaultIcon" "" '"$INSTDIR\$VIEWER_EXE"'

# URL param must be last item passed to viewer, it ignores subsequent params to avoid parameter injection attacks.
WriteRegExpandStr HKEY_CLASSES_ROOT "x-grid-location-info\shell\open\command" "" '"$INSTDIR\$VIEWER_EXE" -url "%1"'

WriteRegStr HKEY_CLASSES_ROOT "Applications\$INSTEXE" "IsHostApp" ""
##WriteRegStr HKEY_CLASSES_ROOT "Applications\${VIEWER_EXE}" "NoStartPage" ""

# Write out uninstaller
WriteUninstaller "$INSTDIR\uninst.exe"

# Uninstall existing "Second Life Viewer 2" install if needed.
StrCmp $DO_UNINSTALL_V2 "" REMOVE_SLV2_DONE
  ExecWait '"$PROGRAMFILES\SecondLifeViewer2\uninst.exe" /S _?=$PROGRAMFILES\SecondLifeViewer2'
  Delete "$PROGRAMFILES\SecondLifeViewer2\uninst.exe"	# With _? option above, uninst.exe will be left behind.
  RMDir "$PROGRAMFILES\SecondLifeViewer2"	# Will remove only if empty.

REMOVE_SLV2_DONE:

SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Uninstall Section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section Uninstall

# Start with some default values.
StrCpy $INSTNAME "${INSTNAME}"
StrCpy $INSTEXE "${INSTEXE}"
StrCpy $VIEWER_EXE "${VIEWER_EXE}"
StrCpy $INSTSHORTCUT "${SHORTCUT}"

# SetShellVarContext per the mode saved at install time in registry at
# MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY
# MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME
# Couldn't get NSIS to expand $MultiUser.InstallMode into the function name at Call time
${If} $MultiUser.InstallMode == 'AllUsers'
##MessageBox MB_OK "Uninstalling for all users"
  Call un.MultiUser.InstallMode.AllUsers
${Else}
##MessageBox MB_OK "Uninstalling for current user"
  Call un.MultiUser.InstallMode.CurrentUser
${EndIf}

# Make sure we're not running
Call un.CloseSecondLife

# Clean up registry keys and subkeys (these should all be !defines somewhere)
DeleteRegKey SHELL_CONTEXT "${INSTNAME_KEY}"
DeleteRegKey SHELL_CONTEXT "${MSCURRVER_KEY}\Uninstall\$INSTNAME"
# BUG-2707 Remove entry that disabled SEHOP
DeleteRegKey SHELL_CONTEXT "${MSNTCURRVER_KEY}\Image File Execution Options\$VIEWER_EXE"
DeleteRegKey HKEY_CLASSES_ROOT "Applications\$INSTEXE"
DeleteRegKey HKEY_CLASSES_ROOT "Applications\${VIEWER_EXE}"

# Clean up shortcuts
Delete "$SMPROGRAMS\$INSTSHORTCUT\*.*"
RMDir  "$SMPROGRAMS\$INSTSHORTCUT"

Delete "$DESKTOP\$INSTSHORTCUT.lnk"
Delete "$INSTDIR\$INSTSHORTCUT.lnk"
Delete "$INSTDIR\Uninstall $INSTSHORTCUT.lnk"

# Remove the main installation directory
Call un.ProgramFiles

# Clean up cache and log files, but leave them in-place for non AGNI installs.
Call un.UserSettingsFiles

SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Make sure the user can install
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckIfAdministrator
    DetailPrint $(CheckAdministratorInstDP)
    UserInfo::GetAccountType
    Pop $R0
    StrCmp $R0 "Admin" lbl_is_admin
        MessageBox MB_OK $(CheckAdministratorInstMB)
        Quit
lbl_is_admin:
    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Make sure the user can uninstall
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.CheckIfAdministrator
    DetailPrint $(CheckAdministratorUnInstDP)
    UserInfo::GetAccountType
    Pop $R0
    StrCmp $R0 "Admin" lbl_is_admin
        MessageBox MB_OK $(CheckAdministratorUnInstMB)
        Quit
lbl_is_admin:
    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Function CheckWillUninstallV2               
;;
;; If called through auto-update, need to uninstall any existing V2 installation.
;; Don't want to end up with SecondLifeViewer2 and SecondLifeViewer installations
;;  existing side by side with no indication on which to use.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckWillUninstallV2

  StrCpy $DO_UNINSTALL_V2 ""

  StrCmp $SKIP_DIALOGS "true" 0 CHECKV2_DONE
  StrCmp $INSTDIR "$PROGRAMFILES\SecondLifeViewer2" CHECKV2_DONE	# Don't uninstall our own install dir.
  IfFileExists "$PROGRAMFILES\SecondLifeViewer2\uninst.exe" CHECKV2_FOUND CHECKV2_DONE

CHECKV2_FOUND:
  StrCpy $DO_UNINSTALL_V2 "true"

CHECKV2_DONE:

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Close the program, if running. Modifies no variables.
;; Allows user to bail out of install process.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CloseSecondLife
  Push $0
  FindWindow $0 "Second Life" ""
  IntCmp $0 0 DONE
  
  StrCmp $SKIP_DIALOGS "true" CLOSE
    MessageBox MB_OKCANCEL $(CloseSecondLifeInstMB) IDOK CLOSE IDCANCEL CANCEL_INSTALL

  CANCEL_INSTALL:
    Quit

  CLOSE:
    DetailPrint $(CloseSecondLifeInstDP)
    SendMessage $0 16 0 0

  LOOP:
	  FindWindow $0 "Second Life" ""
	  IntCmp $0 0 DONE
	  Sleep 500
	  Goto LOOP

  DONE:
    Pop $0
    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Close the program, if running. Modifies no variables.
;; Allows user to bail out of uninstall process.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.CloseSecondLife
  Push $0
  FindWindow $0 "Second Life" ""
  IntCmp $0 0 DONE
  MessageBox MB_OKCANCEL $(CloseSecondLifeUnInstMB) IDOK CLOSE IDCANCEL CANCEL_UNINSTALL

  CANCEL_UNINSTALL:
    Quit

  CLOSE:
    DetailPrint $(CloseSecondLifeUnInstDP)
    SendMessage $0 16 0 0

  LOOP:
	  FindWindow $0 "Second Life" ""
	  IntCmp $0 0 DONE
	  Sleep 500
	  Goto LOOP

  DONE:
    Pop $0
    Return

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Delete files on install if previous install exists to prevent undesired behavior
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function RemoveProgFilesOnInst

# Remove old SecondLife.exe to invalidate any old shortcuts to it that may be in non-standard locations. See MAINT-3575
Delete "$INSTDIR\$INSTEXE"
Delete "$INSTDIR\$VIEWER_EXE"

# Remove old shader files first so fallbacks will work. See DEV-5663
RMDir /r "$INSTDIR\app_settings\shaders"

# Remove skins folder to clean up files removed during development
RMDir /r "$INSTDIR\skins"

# We are no longer including release notes with the viewer, so remove them.
Delete "$SMPROGRAMS\$INSTSHORTCUT\SL Release Notes.lnk"
Delete "$INSTDIR\releasenotes.txt"

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Delete files in \Users\<User>\AppData\
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.UserSettingsFiles

StrCmp $DO_UNINSTALL_V2 "true" Keep			# Don't remove user's settings files on auto upgrade

# Ask if user wants to keep data files or not
MessageBox MB_YESNO|MB_ICONQUESTION $(RemoveDataFilesMB) IDYES Remove IDNO Keep

Remove:
Push $0
Push $1
Push $2

  DetailPrint "Deleting Second Life data files"

  StrCpy $0 0	# Index number used to iterate via EnumRegKey

  LOOP:
    EnumRegKey $1 SHELL_CONTEXT "${MSNTCURRVER_KEY}\ProfileList" $0
    StrCmp $1 "" DONE               # No more users

    ReadRegStr $2 SHELL_CONTEXT "${MSNTCURRVER_KEY}\ProfileList\$1" "ProfileImagePath" 
    StrCmp $2 "" CONTINUE 0         # "ProfileImagePath" value is missing

# Required since ProfileImagePath is of type REG_EXPAND_SZ
    ExpandEnvStrings $2 $2

# Delete files in \Users\<User>\AppData\Roaming\SecondLife
# Remove all settings files but leave any other .txt files to preserve the chat logs
;    RMDir /r "$2\AppData\Roaming\SecondLife\logs"
    RMDir /r "$2\AppData\Roaming\SecondLife\browser_profile"
    RMDir /r "$2\AppData\Roaming\SecondLife\user_settings"
    Delete  "$2\AppData\Roaming\SecondLife\*.xml"
    Delete  "$2\AppData\Roaming\SecondLife\*.bmp"
    Delete  "$2\AppData\Roaming\SecondLife\search_history.txt"
    Delete  "$2\AppData\Roaming\SecondLife\plugin_cookies.txt"
    Delete  "$2\AppData\Roaming\SecondLife\typed_locations.txt"
# Delete files in \Users\<User>\AppData\Local\SecondLife
    RmDir /r  "$2\AppData\Local\SecondLife"							#Delete the cache folder

  CONTINUE:
    IntOp $0 $0 + 1
    Goto LOOP
  DONE:

Pop $2
Pop $1
Pop $0

# Delete files in ProgramData\Secondlife
Push $0
  ReadRegStr $0 SHELL_CONTEXT "${MSCURRVER_KEY}\Explorer\Shell Folders" "Common AppData"
  StrCmp $0 "" +2
  RMDir /r "$0\SecondLife"
Pop $0

Keep:

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Delete the installed files
;; This deletes the uninstall executable, but it works because it is copied to temp directory before running
;;
;; Note:  You must list all files here, because we only want to delete our files,
;; not things users left in the program directory.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.ProgramFiles

# This placeholder is replaced by the complete list of files to uninstall by viewer_manifest.py
%%DELETE_FILES%%

# our InstallMode.txt
Delete "$INSTDIR\InstallMode.txt"

# Optional/obsolete files.  Delete won't fail if they don't exist.
Delete "$INSTDIR\autorun.bat"
Delete "$INSTDIR\dronesettings.ini"
Delete "$INSTDIR\message_template.msg"
Delete "$INSTDIR\newview.pdb"
Delete "$INSTDIR\newview.map"
Delete "$INSTDIR\SecondLife.pdb"
Delete "$INSTDIR\SecondLife.map"
Delete "$INSTDIR\comm.dat"
Delete "$INSTDIR\*.glsl"
Delete "$INSTDIR\motions\*.lla"
Delete "$INSTDIR\trial\*.html"
Delete "$INSTDIR\newview.exe"
Delete "$INSTDIR\SecondLife.exe"

# MAINT-3099 workaround - prevent these log files, if present, from causing a user alert
Delete "$INSTDIR\VivoxVoiceService-*.log"

# Remove entire help directory
RMDir /r  "$INSTDIR\help"

Delete "$INSTDIR\uninst.exe"
RMDir "$INSTDIR"

IfFileExists "$INSTDIR" FOLDERFOUND NOFOLDER

FOLDERFOUND:
  MessageBox MB_OK $(DeleteProgramFilesMB) /SD IDOK IDOK NOFOLDER

NOFOLDER:

MessageBox MB_YESNO $(DeleteRegistryKeysMB) IDYES DeleteKeys IDNO NoDelete

DeleteKeys:
  DeleteRegKey SHELL_CONTEXT "SOFTWARE\Classes\x-grid-location-info"
  DeleteRegKey SHELL_CONTEXT "SOFTWARE\Classes\secondlife"
  DeleteRegKey HKEY_CLASSES_ROOT "x-grid-location-info"
  DeleteRegKey HKEY_CLASSES_ROOT "secondlife"

NoDelete:

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; After install completes, launch app
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function .onInstSuccess
        Push $0
        FileOpen $0 "$INSTDIR\InstallMode.txt" w
        # No newline -- this is for our use, not for users to read.
        FileWrite $0 "$MultiUser.InstallMode"
        FileClose $0
        Pop $0

        Push $R0
        Push $0
        ;; MAINT-7812: Only write nsis.winstall file with /marker switch
        ${GetParameters} $R0
        ${GetOptionsS} $R0 "/marker" $0
        ;; If no /marker switch, skip to ClearErrors
        IfErrors +4 0
        ;; $EXEDIR is where we find the installer file
        ;; Put a marker file there so VMP will know we're done
        ;; and it can delete the download directory next time.
        ;; http://nsis.sourceforge.net/Write_text_to_a_file
        FileOpen $0 "$EXEDIR\nsis.winstall" w
        FileWrite $0 "NSIS done$\n"
        FileClose $0

        ClearErrors
        Pop $0
        Pop $R0

        Call CheckWindowsServPack		# Warn if not on the latest SP before asking to launch.
        StrCmp $SKIP_AUTORUN "true" +2;
        # Assumes SetOutPath $INSTDIR
        # Run INSTEXE (our updater), passing VIEWER_EXE plus the command-line
        # arguments built into our shortcuts. This gives the updater a chance
        # to verify that the viewer we just installed is appropriate for the
        # running system -- or, if not, to download and install a different
        # viewer. For instance, if a user running 32-bit Windows installs a
        # 64-bit viewer, it cannot run on this system. But since the updater
        # is a 32-bit executable even in the 64-bit viewer package, the
        # updater can detect the problem and adapt accordingly.
        # Once everything is in order, the updater will run the specified
        # viewer with the specified params.
        # Quote the updater executable and the viewer executable because each
        # must be a distinct command-line token, but DO NOT quote the language
        # string because it must decompose into separate command-line tokens.
        Exec '"$INSTDIR\$INSTEXE" precheck "$INSTDIR\$VIEWER_EXE" $SHORTCUT_LANG_PARAM'
# 
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Recommend Upgrading to Service Pack 1 for Windows 7, if not present
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckWindowsServPack
  ${If} ${IsWin7}
  ${AndIfNot} ${IsServicePack} 1
    MessageBox MB_OK $(CheckWindowsServPackMB)
    DetailPrint $(UseLatestServPackDP)
    Return
  ${EndIf}

  ${If} ${IsWin2008R2}
  ${AndIfNot} ${IsServicePack} 1
    MessageBox MB_OK $(CheckWindowsServPackMB)
    DetailPrint $(UseLatestServPackDP)
    Return
  ${EndIf}

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Clobber user files - TEST ONLY
;; This is here for testing, DO NOT USE UNLESS YOU KNOW WHAT YOU ARE TESTING FOR!
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Function ClobberUserFilesTESTONLY

;Push $0
;Push $1
;Push $2
;
;    StrCpy $0 0	# Index number used to iterate via EnumRegKey
;
;  LOOP:
;    EnumRegKey $1 SHELL_CONTEXT "${MSNTCURRVER_KEY}\ProfileList" $0
;    StrCmp $1 "" DONE               # no more users
;
;    ReadRegStr $2 SHELL_CONTEXT "${MSNTCURRVER_KEY}\ProfileList\$1" "ProfileImagePath"
;    StrCmp $2 "" CONTINUE 0         # "ProfileImagePath" value is missing
;
;# Required since ProfileImagePath is of type REG_EXPAND_SZ
;    ExpandEnvStrings $2 $2
;
;    RMDir /r "$2\Application Data\SecondLife\"
;
;  CONTINUE:
;    IntOp $0 $0 + 1
;    Goto LOOP
;  DONE:
;
;Pop $2
;Pop $1
;Pop $0
;
;# Copy files in Documents and Settings\All Users\SecondLife
;Push $0
;    ReadRegStr $0 SHELL_CONTEXT "${MSCURRVER_KEY}\Explorer\Shell Folders" "Common AppData"
;    StrCmp $0 "" +2
;    RMDir /r "$2\Application Data\SecondLife\"
;Pop $0
;
;FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; EOF  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
