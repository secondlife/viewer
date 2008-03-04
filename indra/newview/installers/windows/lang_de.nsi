; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\German.nlf"

; subtitle on license text caption (setup new version or update current one
LangString LicenseSubTitleUpdate ${LANG_GERMAN} " Update"
LangString LicenseSubTitleSetup ${LANG_GERMAN} " Setup"

; description on license page
LangString LicenseDescUpdate ${LANG_GERMAN} "Dieses Paket wird das bereits installierte Second Life mit Version ${VERSION_LONG}. ersetzen."
LangString LicenseDescSetup ${LANG_GERMAN} "Dieses Paket wird Second Life auf Ihrem Computer installieren."
LangString LicenseDescNext ${LANG_GERMAN} "Weiter"

; installation directory text
LangString DirectoryChooseTitle ${LANG_GERMAN} "Installations-Ordner"
LangString DirectoryChooseUpdate ${LANG_GERMAN} "Wählen Sie den Second Life Ordner für dieses Update:"
LangString DirectoryChooseSetup ${LANG_GERMAN} "Pfad in dem Second Life installieren werden soll:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_GERMAN} "Konnte Programm '$INSTPROG' nicht finden. Stilles Update fehlgeschlagen."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_GERMAN} "Second Life starten?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_GERMAN} "Überprüfe alte Version ..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_GERMAN} "Überprüfung der Windows Version ..."
LangString CheckWindowsVersionMB ${LANG_GERMAN} 'Second Life unterstützt nur Windows XP, Windows 2000 und Mac OS X.$\n$\nDer Versuch es auf Windows $R0 zu installieren, könnte zu unvorhersehbaren Abstürzen und Datenverlust führen.$\n$\nTrotzdem installieren?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_GERMAN} "Überprüfung der Installations-Berechtigungen ..."
LangString CheckAdministratorInstMB ${LANG_GERMAN} 'Sie besitzen ungenügende Berechtigungen.$\nSie müssen ein "administrator" sein, um Second Life installieren zu können.'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_GERMAN} "Überprüfung der Entfernungs-Berechtigungen ..."
LangString CheckAdministratorUnInstMB ${LANG_GERMAN} 'Sie besitzen ungenügende Berechtigungen.$\nSie müssen ein "administrator" sein, um Second Life entfernen zu können..'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_GERMAN} "Anscheinend ist Second Life ${VERSION_LONG} bereits installiert.$\n$\nWürden Sie es gerne erneut installieren?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_GERMAN} "Warten auf die Beendigung von Second Life ..."
LangString CloseSecondLifeInstMB ${LANG_GERMAN} "Second Life kann nicht installiert oder ersetzt werden, wenn es bereits läuft.$\n$\nBeenden Sie, was Sie gerade tun und klicken Sie OK, um Second Life zu beenden.$\nKlicken Sie CANCEL, um die Installation abzubrechen."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_GERMAN} "Warten auf die Beendigung von Second Life ..."
LangString CloseSecondLifeUnInstMB ${LANG_GERMAN} "Second Life kann nicht entfernt werden, wenn es bereits läuft.$\n$\nBeenden Sie, was Sie gerade tun und klicken Sie OK, um Second Life zu beenden.$\nKlicken Sie CANCEL, um abzubrechen."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_GERMAN} "Löschung aller Cache Dateien in Dokumente und Einstellungen."

; delete program files
LangString DeleteProgramFilesMB ${LANG_GERMAN} "Es existieren weiterhin Dateien in Ihrem SecondLife Programm Ordner.$\n$\nDies sind möglicherweise Dateien, die sie modifiziert oder bewegt haben:$\n$INSTDIR$\n$\nMöchten Sie diese ebenfalls löschen?"

; uninstall text
LangString UninstallTextMsg ${LANG_GERMAN} "Dies wird Second Life ${VERSION_LONG} von Ihrem System entfernen."
