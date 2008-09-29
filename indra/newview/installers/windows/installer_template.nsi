;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; secondlife setup.nsi
;; Copyright 2004-2007, Linden Research, Inc.
;; For info, see http://www.nullsoft.com/free/nsis/
;;
;; NSIS 2.22 or higher required
;; Author: James Cook, Don Kjer, Callum Prentice
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Detect NSIS compiler version
!define "NSIS${NSIS_VERSION}"
!ifdef "NSISv2.02" | "NSISv2.03" | "NSISv2.04" | "NSISv2.05" | "NSISv2.06"
    ;; before 2.07 defaulted lzma to solid (whole file)
    SetCompressor lzma
!else
    ;; after 2.07 required /solid for whole file compression
    SetCompressor /solid lzma
!endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Compiler flags
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
SetOverwrite on				; overwrite files
SetCompress auto			; compress iff saves space
SetDatablockOptimize off	; only saves us 0.1%, not worth it
XPStyle on                  ; add an XP manifest to the installer

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Project flags
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%%VERSION%%

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; - language files - one for each language (or flavor thereof)
;; (these files are in the same place as the nsi template but the python script generates a new nsi file in the 
;; application directory so we have to add a path to these include files)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
!include "%%SOURCE%%\installers\windows\lang_de.nsi"
!include "%%SOURCE%%\installers\windows\lang_en-us.nsi"
!include "%%SOURCE%%\installers\windows\lang_ja.nsi"
!include "%%SOURCE%%\installers\windows\lang_ko.nsi"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tweak for different servers/builds (this placeholder is replaced by viewer_manifest.py)
%%GRID_VARS%%

Name ${INSTNAME}

SubCaption 0 $(LicenseSubTitleSetup)	; override "license agreement" text

BrandingText " "						; bottom of window text
Icon %%SOURCE%%\res\install_icon.ico	; our custom icon
UninstallIcon %%SOURCE%%\res\uninstall_icon.ico    ; our custom icon
WindowIcon on							; show our icon in left corner
BGGradient off							; no big background window
CRCCheck on								; make sure CRC is OK
InstProgressFlags smooth colored		; new colored smooth look
ShowInstDetails nevershow				; no details, no "show" button
SetOverwrite on							; stomp files by default
AutoCloseWindow true					; after all files install, close window

InstallDir "$PROGRAMFILES\${INSTNAME}"
InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\${INSTNAME}" ""
!ifdef UPDATE
DirText $(DirectoryChooseTitle) $(DirectoryChooseUpdate)
!else
DirText $(DirectoryChooseTitle) $(DirectoryChooseSetup)
!endif


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Var INSTPROG
Var INSTEXE
Var INSTFLAGS
Var LANGFLAGS
Var INSTSHORTCUT

;;; Function definitions should go before file includes, because the NSIS package
;;; is a single stream of bytecodes + file data.  So if your function definitions are at
;;; the end of the file it has to decompress the whole thing before it can call a function. JC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PostInstallExe
; This just runs any post installation scripts.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function PostInstallExe
push $0
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "PostInstallExe"
  ;MessageBox MB_OK '$0'
  ExecWait '$0'
pop $0
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; CheckStartupParameters
; Sets INSTFLAGS, INSTPROG, and INSTEXE.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckStartupParams
push $0
push $R0

  ; Look for a registry entry with info about where to update.
  Call GetProgramName
  pop $R0
  StrCpy $INSTPROG "$R0"
  StrCpy $INSTEXE "$R0.exe"

  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" ""
  ; If key doesn't exist, skip install
  IfErrors ABORT
  StrCpy $INSTDIR "$0"

  ; We now have a directory to install to.  Get the startup parameters and shortcut as well.
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Flags"
  IfErrors +2
  StrCpy $INSTFLAGS "$0"
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Shortcut"
  IfErrors +2
  StrCpy $INSTSHORTCUT "$0"
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Exe"
  IfErrors +2
  StrCpy $INSTEXE "$0"
  Goto FINISHED

ABORT:
  MessageBox MB_OK $(CheckStartupParamsMB)
  Quit

FINISHED:
  ;MessageBox MB_OK "INSTPROG: $INSTPROG, INSTEXE: $INSTEXE, INSTFLAGS: $INSTFLAGS"
pop $R0
pop $0
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.CheckStartupParams
push $0
push $R0

  ; Look for a registry entry with info about where to update.
  Call un.GetProgramName
  pop $R0
  StrCpy $INSTPROG "$R0"
  StrCpy $INSTEXE "$R0.exe"

  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" ""
  ; If key doesn't exist, skip install
  IfErrors ABORT
  StrCpy $INSTDIR "$0"

  ; We now have a directory to install to.  Get the startup parameters and shortcut as well.
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Flags"
  IfErrors +2
  StrCpy $INSTFLAGS "$0"
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Shortcut"
  IfErrors +2
  StrCpy $INSTSHORTCUT "$0"
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Exe"
  IfErrors +2
  StrCpy $INSTEXE "$0"
  Goto FINISHED

ABORT:
  MessageBox MB_OK $(CheckStartupParamsMB)
  Quit

FINISHED:
  ;MessageBox MB_OK "INSTPROG: $INSTPROG, INSTEXE: $INSTEXE, INSTFLAGS: $INSTFLAGS"
pop $R0
pop $0
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; After install completes, offer readme file
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function .onInstSuccess
	MessageBox MB_YESNO \
	$(InstSuccesssQuestion) /SD IDYES IDNO NoReadme
		; Assumes SetOutPath $INSTDIR
		Exec '"$INSTDIR\$INSTEXE" $INSTFLAGS'
	NoReadme:
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Remove old NSIS version. Modifies no variables.
; Does NOT delete the LindenWorld directory, or any
; user files in that directory.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function RemoveNSIS
  Push $0
  ; Grab the installation directory of the old version
  DetailPrint $(RemoveOldNSISVersion)
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" ""

  ; If key doesn't exist, skip uninstall
  IfErrors NO_NSIS

  ; Clean up legacy beta shortcuts
  Delete "$SMPROGRAMS\Second Life Beta.lnk"
  Delete "$DESKTOP\Second Life Beta.lnk"
  Delete "$SMPROGRAMS\Second Life.lnk"
  
  ; Clean up old newview.exe file
  Delete "$INSTDIR\newview.exe"

  ; Intentionally don't delete the stuff in
  ; Documents and Settings, so we keep the user's settings

  NO_NSIS:
  Pop $0
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Make sure we're not on Windows 98 / ME
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckWindowsVersion
	DetailPrint "Checking Windows version..."
	Call GetWindowsVersion
	Pop $R0
	; Just get first two characters, ignore 4.0 part of "NT 4.0"
	StrCpy $R0 $R0 2
	; Blacklist certain OS versions
	StrCmp $R0 "95" win_ver_bad
	StrCmp $R0 "98" win_ver_bad
	StrCmp $R0 "ME" win_ver_bad
	StrCmp $R0 "NT" win_ver_bad
	Return
win_ver_bad:
	MessageBox MB_YESNO $(CheckWindowsVersionMB) IDNO win_ver_abort
	Return
win_ver_abort:
	Quit
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Make sure the user can install/uninstall
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckIfAdministrator
		DetailPrint $(CheckAdministratorInstDP)
         UserInfo::GetAccountType
         Pop $R0
         StrCmp $R0 "Admin" is_admin
         MessageBox MB_OK $(CheckAdministratorInstMB)
         Quit
is_admin:
         Return
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.CheckIfAdministrator
	DetailPrint $(CheckAdministratorUnInstDP)
         UserInfo::GetAccountType
         Pop $R0
         StrCmp $R0 "Admin" is_admin
         MessageBox MB_OK $(CheckAdministratorUnInstMB)
         Quit
is_admin:
         Return
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Checks to see if the current version has already been installed (according to the registry).
; If it has, allow user to bail out of install process.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CheckIfAlreadyCurrent
  Push $0
	ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Version"
    StrCmp $0 ${VERSION_LONG} 0 DONE
	MessageBox MB_OKCANCEL $(CheckIfCurrentMB) /SD IDOK IDOK DONE
    Quit

  DONE:
    Pop $0
    Return
FunctionEnd
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Close the program, if running. Modifies no variables.
; Allows user to bail out of install process.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function CloseSecondLife
  Push $0
  FindWindow $0 "Second Life" ""
  IntCmp $0 0 DONE
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
; Delete files in Documents and Settings\<user>\SecondLife\cache
; Delete files in Documents and Settings\All Users\SecondLife\cache
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Function RemoveCacheFiles
;
;; Delete files in Documents and Settings\<user>\SecondLife
;Push $0
;Push $1
;Push $2
;  DetailPrint $(RemoveCacheFilesDP)
;
;  StrCpy $0 0 ; Index number used to iterate via EnumRegKey
;
;  LOOP:
;    EnumRegKey $1 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList" $0
;    StrCmp $1 "" DONE               ; no more users
;
;    ReadRegStr $2 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\$1" "ProfileImagePath" 
;    StrCmp $2 "" CONTINUE 0         ; "ProfileImagePath" value is missing
;
;    ; Required since ProfileImagePath is of type REG_EXPAND_SZ
;    ExpandEnvStrings $2 $2
;
;	; When explicitly uninstalling, everything goes away
;    RMDir /r "$2\Application Data\SecondLife\cache"
;
;  CONTINUE:
;    IntOp $0 $0 + 1
;    Goto LOOP
;  DONE:
;Pop $2
;Pop $1
;Pop $0
;
;; Delete files in Documents and Settings\All Users\SecondLife
;Push $0
;  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" "Common AppData"
;  StrCmp $0 "" +2
;  RMDir /r "$0\SecondLife\cache"
;Pop $0
;
;; Delete filse in C:\Windows\Application Data\SecondLife
;; If the user is running on a pre-NT system, Application Data lives here instead of
;; in Documents and Settings.
;RMDir /r "$WINDIR\Application Data\SecondLife\cache"
;
;FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Delete the installed shader files
;;; Since shaders are in active development, we'll likely need to shuffle them
;;; around a bit from build to build.  This ensures that shaders that we move
;;; or rename in the dev tree don't get left behind in the install.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function RemoveOldShaders

;; Remove old shader files first so fallbacks will work. see DEV-5663
RMDir /r "$INSTDIR\app_settings\shaders\*"

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Delete the installed XUI files
;;; We've changed the directory hierarchy for skins, putting all XUI and texture
;;; files under a specific skin directory, i.e. skins/default/xui/en-us as opposed
;;; to skins/xui/en-us.  Need to clean up the old path when upgrading
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function RemoveOldXUI

;; remove old XUI and texture files
RmDir /r "$INSTDIR\skins\html"
RmDir /r "$INSTDIR\skins\xui"
RmDir /r "$INSTDIR\skins\textures"
Delete "$INSTDIR\skins\*.txt"

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Remove any releasenotes files.
;;; We are no longer including release notes with the viewer. This will delete
;;; any that were left behind by an older installer. Delete will not fail if
;;; the files do not exist
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function RemoveOldReleaseNotes

;; remove releasenotes.txt file from application directory, and the shortcut
;; from the start menu.
Delete "$SMPROGRAMS\$INSTSHORTCUT\SL Release Notes.lnk"
Delete "$INSTDIR\releasenotes.txt"

FunctionEnd


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Delete files in Documents and Settings\<user>\SecondLife
; Delete files in Documents and Settings\All Users\SecondLife
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.DocumentsAndSettingsFolder

; Delete files in Documents and Settings\<user>\SecondLife
Push $0
Push $1
Push $2

  DetailPrint "Deleting files in Documents and Settings folder"

  StrCpy $0 0 ; Index number used to iterate via EnumRegKey

  LOOP:
    EnumRegKey $1 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList" $0
    StrCmp $1 "" DONE               ; no more users

    ReadRegStr $2 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList\$1" "ProfileImagePath" 
    StrCmp $2 "" CONTINUE 0         ; "ProfileImagePath" value is missing

    ; Required since ProfileImagePath is of type REG_EXPAND_SZ
    ExpandEnvStrings $2 $2

	; If uninstalling a normal install remove everything
	; Otherwise (preview/dmz etc) just remove cache
    StrCmp $INSTFLAGS "" RM_ALL RM_CACHE
      RM_ALL:
        RMDir /r "$2\Application Data\SecondLife"
        GoTo CONTINUE
      RM_CACHE:
        RMDir /r "$2\Application Data\SecondLife\Cache"
        Delete "$2\Application Data\SecondLife\user_settings\settings_windlight.xml"

  CONTINUE:
    IntOp $0 $0 + 1
    Goto LOOP
  DONE:

Pop $2
Pop $1
Pop $0

; Delete files in Documents and Settings\All Users\SecondLife
Push $0
  ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" "Common AppData"
  StrCmp $0 "" +2
  RMDir /r "$0\SecondLife"
Pop $0

; Delete filse in C:\Windows\Application Data\SecondLife
; If the user is running on a pre-NT system, Application Data lives here instead of
; in Documents and Settings.
RMDir /r "$WINDIR\Application Data\SecondLife"

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Close the program, if running. Modifies no variables.
; Allows user to bail out of uninstall process.
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
;
;   Delete the stored password for the current Windows user
;   DEV-10821 -- Unauthorised user can gain access to an SL account after a real user has uninstalled
;
Function un.RemovePassword

DetailPrint "Removing Second Life password"

SetShellVarContext current
Delete "$APPDATA\SecondLife\user_settings\password.dat"
SetShellVarContext all

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Delete the installed files
;;; This deletes the uninstall executable, but it works 
;;; because it is copied to temp directory before running
;;;
;;; Note:  You must list all files here, because we only
;;; want to delete our files, not things users left in the
;;; application directories.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.ProgramFiles

;; Remove mozilla file first so recursive directory deletion doesn't get hung up
Delete "$INSTDIR\app_settings\mozilla\components"

;; This placeholder is replaced by the complete list of files to uninstall by viewer_manifest.py
%%DELETE_FILES%%

;; Optional/obsolete files.  Delete won't fail if they don't exist.
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
;; Remove entire help directory
Delete "$INSTDIR\help\Advanced\*"
RMDir  "$INSTDIR\help\Advanced"
Delete "$INSTDIR\help\basics\*"
RMDir  "$INSTDIR\help\basics"
Delete "$INSTDIR\help\Concepts\*"
RMDir  "$INSTDIR\help\Concepts"
Delete "$INSTDIR\help\welcome\*"
RMDir  "$INSTDIR\help\welcome"
Delete "$INSTDIR\help\*"
RMDir  "$INSTDIR\help"

Delete "$INSTDIR\uninst.exe"
RMDir "$INSTDIR"

IfFileExists "$INSTDIR" FOLDERFOUND NOFOLDER

FOLDERFOUND:
  MessageBox MB_YESNO $(DeleteProgramFilesMB) IDNO NOFOLDER
  RMDir /r "$INSTDIR"

NOFOLDER:

FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Uninstall settings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
UninstallText $(UninstallTextMsg)
ShowUninstDetails show

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Uninstall section
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section Uninstall

; Start with some default values.
StrCpy $INSTFLAGS ""
StrCpy $INSTPROG "${INSTNAME}"
StrCpy $INSTEXE "${INSTEXE}"
StrCpy $INSTSHORTCUT "${SHORTCUT}"
Call un.CheckStartupParams              ; Figure out where, what and how to uninstall.
Call un.CheckIfAdministrator		; Make sure the user can install/uninstall

; uninstall for all users (if you change this, change it in the install as well)
SetShellVarContext all			

; Make sure we're not running
Call un.CloseSecondLife

; Clean up registry keys (these should all be !defines somewhere)
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG"
DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$INSTPROG"
DeleteRegKey HKEY_LOCAL_MACHINE "Software\Linden Research, Inc.\Installer Language" 

; Clean up shortcuts
Delete "$SMPROGRAMS\$INSTSHORTCUT\*.*"
RMDir  "$SMPROGRAMS\$INSTSHORTCUT"

Delete "$DESKTOP\$INSTSHORTCUT.lnk"
Delete "$INSTDIR\$INSTSHORTCUT.lnk"
Delete "$INSTDIR\Uninstall $INSTSHORTCUT.lnk"

; Clean up cache and log files.
; Leave them in-place for non AGNI installs.

!ifdef UNINSTALL_SETTINGS
Call un.DocumentsAndSettingsFolder
!endif

; remove stored password on uninstall
Call un.RemovePassword

Call un.ProgramFiles

SectionEnd 				; end of uninstall section

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (From the NSIS wiki, DK)
; GetParameterValue
;
; Usage:
; !insertmacro GetParameterValue "/L=" "1033"
; pop $R0
;
; Returns on top of stack
;
; Example command lines:
; foo.exe /S /L=1033 /D=C:\Program Files\Foo
; or:
; foo.exe /S "/L=1033" /D="C:\Program Files\Foo"
; gpv "/L=" "1033"
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

 !macro GetParameterValue SWITCH DEFAULT
   Push $0
   Push $1
   Push $2
   Push $3
   Push $4

 ;$CMDLINE='"My Setup\Setup.exe" /L=1033 /S'
   Push "$CMDLINE"
   Push '${SWITCH}"'
   !insertmacro StrStr
   Pop $0
   StrCmp "$0" "" gpv_notquoted
 ;$0='/L="1033" /S'
   StrLen $2 "$0"
   Strlen $1 "${SWITCH}"
   IntOp $1 $1 + 1
   StrCpy $0 "$0" $2 $1
 ;$0='1033" /S'
   Push "$0"
   Push '"'
   !insertmacro StrStr
   Pop $1
   StrLen $2 "$0"
   StrLen $3 "$1"
   IntOp $4 $2 - $3
   StrCpy $0 $0 $4 0
   Goto gpv_done

   gpv_notquoted:
   Push "$CMDLINE"
   Push "${SWITCH}"
   !insertmacro StrStr
   Pop $0
   StrCmp "$0" "" gpv_done
 ;$0='/L="1033" /S'
   StrLen $2 "$0"
   Strlen $1 "${SWITCH}"
   StrCpy $0 "$0" $2 $1
 ;$0=1033 /S'
   Push "$0"
   Push ' '
   !insertmacro StrStr
   Pop $1
   StrLen $2 "$0"
   StrLen $3 "$1"
   IntOp $4 $2 - $3
   StrCpy $0 $0 $4 0
   Goto gpv_done

   gpv_done:
   StrCmp "$0" "" 0 +2
   StrCpy $0 "${DEFAULT}"

   Pop $4
   Pop $3
   Pop $2
   Pop $1
   Exch $0
 !macroend

; And I had to modify StrStr a tiny bit.
; Possible upgrade switch the goto's to use ${__LINE__}

!macro STRSTR
  Exch $R1 ; st=haystack,old$R1, $R1=needle
  Exch    ; st=old$R1,haystack
  Exch $R2 ; st=old$R1,old$R2, $R2=haystack
  Push $R3
  Push $R4
  Push $R5
  StrLen $R3 $R1
  StrCpy $R4 0
  ; $R1=needle
  ; $R2=haystack
  ; $R3=len(needle)
  ; $R4=cnt
  ; $R5=tmp
 ;  loop;
    StrCpy $R5 $R2 $R3 $R4
    StrCmp $R5 $R1 +4
    StrCmp $R5 "" +3
    IntOp $R4 $R4 + 1
    Goto -4
 ;  done;
  StrCpy $R1 $R2 "" $R4
  Pop $R5
  Pop $R4
  Pop $R3
  Pop $R2
  Exch $R1
!macroend

Function GetProgramName
  !insertmacro GetParameterValue "/P=" "SecondLife"
FunctionEnd

Function un.GetProgramName
  !insertmacro GetParameterValue "/P=" "SecondLife"
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (From the NSIS documentation, JC)
; GetWindowsVersion
;
; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
; Updated by Joost Verburg
;
; Returns on top of stack
;
; Windows Version (95, 98, ME, NT x.x, 2000, XP, 2003)
; or
; '' (Unknown Windows Version)
;
; Usage:
;   Call GetWindowsVersion
;   Pop $R0
;   ; at this point $R0 is "NT 4.0" or whatnot
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function GetWindowsVersion
 
   Push $R0
   Push $R1
 
   ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

   IfErrors 0 lbl_winnt
   
   ; we are not NT
   ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber
 
   StrCpy $R1 $R0 1
   StrCmp $R1 '4' 0 lbl_error
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '4.0' lbl_win32_95
   StrCmp $R1 '4.9' lbl_win32_ME lbl_win32_98
 
   lbl_win32_95:
     StrCpy $R0 '95'
   Goto lbl_done
 
   lbl_win32_98:
     StrCpy $R0 '98'
   Goto lbl_done
 
   lbl_win32_ME:
     StrCpy $R0 'ME'
   Goto lbl_done
 
   lbl_winnt:
 
   StrCpy $R1 $R0 1
 
   StrCmp $R1 '3' lbl_winnt_x
   StrCmp $R1 '4' lbl_winnt_x
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '5.0' lbl_winnt_2000
   StrCmp $R1 '5.1' lbl_winnt_XP
   StrCmp $R1 '5.2' lbl_winnt_2003 lbl_error
 
   lbl_winnt_x:
     StrCpy $R0 "NT $R0" 6
   Goto lbl_done
 
   lbl_winnt_2000:
     Strcpy $R0 '2000'
   Goto lbl_done
 
   lbl_winnt_XP:
     Strcpy $R0 'XP'
   Goto lbl_done
 
   lbl_winnt_2003:
     Strcpy $R0 '2003'
   Goto lbl_done
 
   lbl_error:
     Strcpy $R0 ''
   lbl_done:
 
   Pop $R1
   Exch $R0
 
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Note: to add new languages, add a language file include to the list 
;;  at the top of this file, add an entry to the menu and then add an 
;;  entry to the language ID selector below
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function .onInit

	; read the language from registry (ok if not there) and set langauge menu
	ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\${INSTNAME}" "InstallerLanguage"
	StrCpy $LANGUAGE $0

	Push ""
	Push ${LANG_ENGLISH}
	Push English
	Push ${LANG_GERMAN}
	Push German
	Push ${LANG_JAPANESE}
	Push Japanese
	Push ${LANG_KOREAN}
	Push Korean
	Push A ; A means auto count languages for the auto count to work the first empty push (Push "") must remain
	LangDLL::LangDialog "Installer Language" "Please select the language of the installer"
	Pop $LANGUAGE
	StrCmp $LANGUAGE "cancel" 0 +2
		Abort

	; save language in registry		
	WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\${INSTNAME}" "InstallerLanguage" $LANGUAGE
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Function un.onInit

	; read language from registry and set for ininstaller
	ReadRegStr $0 HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\${INSTNAME}" "InstallerLanguage"
	StrCpy $LANGUAGE $0

FunctionEnd


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Sections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
Section ""						; (default section)

SetShellVarContext all			; install for all users (if you change this, change it in the uninstall as well)

; Start with some default values.
StrCpy $INSTFLAGS "${INSTFLAGS}"
StrCpy $INSTPROG "${INSTNAME}"
StrCpy $INSTEXE "${INSTEXE}"
StrCpy $INSTSHORTCUT "${SHORTCUT}"

IfSilent +2
Goto NOT_SILENT
  Call CheckStartupParams                 ; Figure out where, what and how to install.
NOT_SILENT:
Call CheckWindowsVersion		; warn if on Windows 98/ME
Call CheckIfAdministrator		; Make sure the user can install/uninstall
Call CheckIfAlreadyCurrent		; Make sure that we haven't already installed this version
Call CloseSecondLife			; Make sure we're not running
Call RemoveNSIS					; Check for old NSIS install to remove

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Don't remove cache files during a regular install, removing the inventory cache on upgrades results in lots of damage to the servers.
;Call RemoveCacheFiles			; Installing over removes potentially corrupted
								; VFS and cache files.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Need to clean out shader files from previous installs to fix DEV-5663
Call RemoveOldShaders

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Need to clean out old XUI files that predate skinning
Call RemoveOldXUI

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Clear out old releasenotes.txt files. These are now on the public wiki.
Call RemoveOldReleaseNotes

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This placeholder is replaced by the complete list of all the files in the installer, by viewer_manifest.py
%%INSTALL_FILES%%

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; If this is a silent update, we don't need to re-create these shortcuts or registry entries.
IfSilent POST_INSTALL

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Shortcuts in start menu
CreateDirectory	"$SMPROGRAMS\$INSTSHORTCUT"
SetOutPath "$INSTDIR"
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\$INSTSHORTCUT.lnk" \
				"$INSTDIR\$INSTEXE" "$INSTFLAGS"

!ifdef MUSEUM
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\$INSTSHORTCUT Museum.lnk" \

				"$INSTDIR\$INSTEXE" "$INSTFLAGS -simple"
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\$INSTSHORTCUT Museum Spanish.lnk" \

				"$INSTDIR\$INSTEXE" "$INSTFLAGS -simple -spanish"
!endif

WriteINIStr		"$SMPROGRAMS\$INSTSHORTCUT\SL Create Trial Account.url" \
				"InternetShortcut" "URL" \
				"http://www.secondlife.com/registration/"
WriteINIStr		"$SMPROGRAMS\$INSTSHORTCUT\SL Your Account.url" \
				"InternetShortcut" "URL" \
				"http://www.secondlife.com/account/"
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\SL Scripting Language Help.lnk" \
				"$INSTDIR\lsl_guide.html"
CreateShortCut	"$SMPROGRAMS\$INSTSHORTCUT\Uninstall $INSTSHORTCUT.lnk" \
				'"$INSTDIR\uninst.exe"' '/P="$INSTPROG"'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Other shortcuts
SetOutPath "$INSTDIR"
CreateShortCut "$DESKTOP\$INSTSHORTCUT.lnk" "$INSTDIR\$INSTEXE" "$INSTFLAGS"
CreateShortCut "$INSTDIR\$INSTSHORTCUT.lnk" "$INSTDIR\$INSTEXE" "$INSTFLAGS"
CreateShortCut "$INSTDIR\Uninstall $INSTSHORTCUT.lnk" \
				'"$INSTDIR\uninst.exe"' '/P="$INSTPROG"'

!ifdef MUSEUM
CreateShortCut "$DESKTOP\$INSTSHORTCUT Museum.lnk" "$INSTDIR\$INSTEXE" "$INSTFLAGS -simple"

CreateShortCut "$DESKTOP\$INSTSHORTCUT Museum Spanish.lnk" "$INSTDIR\$INSTEXE" "$INSTFLAGS -simple -spanish"

CreateShortCut "$INSTDIR\$INSTSHORTCUT Museum.lnk" "$INSTDIR\$INSTEXE" "$INSTFLAGS -simple"

CreateShortCut "$INSTDIR\$INSTSHORTCUT Museum Spanish.lnk" "$INSTDIR\$INSTEXE" "$INSTFLAGS -simple -spanish"

!endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Write registry
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "" "$INSTDIR"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Version" "${VERSION_LONG}"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Flags" "$INSTFLAGS"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Shortcut" "$INSTSHORTCUT"
WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Linden Research, Inc.\$INSTPROG" "Exe" "$INSTEXE"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\$INSTPROG" "DisplayName" "$INSTPROG (remove only)"
WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\$INSTPROG" "UninstallString" '"$INSTDIR\uninst.exe" /P="$INSTPROG"'

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Write URL registry info
WriteRegStr HKEY_CLASSES_ROOT "${URLNAME}" "(default)" "URL:Second Life"
WriteRegStr HKEY_CLASSES_ROOT "${URLNAME}" "URL Protocol" ""
WriteRegStr HKEY_CLASSES_ROOT "${URLNAME}\DefaultIcon" "" '"$INSTDIR\$INSTEXE"'
WriteRegExpandStr HKEY_CLASSES_ROOT "${URLNAME}\shell\open\command" "" '"$INSTDIR\$INSTEXE" $INSTFLAGS -url "%1"'

Goto WRITE_UNINST

POST_INSTALL:
; Run a post-executable script if necessary.
Call PostInstallExe

WRITE_UNINST:
; write out uninstaller
WriteUninstaller "$INSTDIR\uninst.exe"

; end of default section
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; EOF  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
