; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\German.nlf"

; subtitle on license text caption (setup new version or update current one
LangString LicenseSubTitleUpdate ${LANG_GERMAN} " Update"
LangString LicenseSubTitleSetup ${LANG_GERMAN} " Setup"

; description on license page
LangString LicenseDescUpdate ${LANG_GERMAN} "Dieses Paket wird Second Life auf Version ${VERSION_LONG}.updaten"
LangString LicenseDescSetup ${LANG_GERMAN} "Dieses Paket installiert Second Life auf Ihrem Computer."
LangString LicenseDescNext ${LANG_GERMAN} "Nächster Schritt"

; installation directory text
LangString DirectoryChooseTitle ${LANG_GERMAN} "Installations Ordner"
LangString DirectoryChooseUpdate ${LANG_GERMAN} "Wählen Sie den Second Life Ordner für dieses Update:"
LangString DirectoryChooseSetup ${LANG_GERMAN} "Wählen Sie den Pfad, in den Sie Second Life installieren möchten:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_GERMAN} "Konnte Programm '$INSTPROG' nicht finden. Stilles Update fehlgeschlagen."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_GERMAN} "Second Life jetzt starten?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_GERMAN} "Überprüfe alte Version..."

; check windows version
LangString CheckWindowsVersionDP ${LANG_GERMAN} "Überprüfe Windows Version..."
LangString CheckWindowsVersionMB ${LANG_GERMAN} 'Second Life unterstützt nur Windows XP, Windows 2000 und Mac OS X.$\n$\nDer Versuch es auf Windows $R0 zu installieren, könnte in unvorhersehbaren Abstürtzen und zu Datenverlust führen.$\n$\nTrotzdem installieren?'

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_GERMAN} "Überprüfe nach Genehmigung zur Installation..."
LangString CheckAdministratorInstMB ${LANG_GERMAN} 'Es scheint so, als würden Sie einen "limited" Account verwenden.$\nSie müssen ein"administrator" sein, um Second Life installieren zu können..'

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_GERMAN} "Überprüfe Genehmigung zum Deinstallieren..."
LangString CheckAdministratorUnInstMB ${LANG_GERMAN} 'Es scheint so, als würden Sie einen "limited" Account verwenden.$\nSie müssen ein"administrator" sein, um Second Life installieren zu können..'

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_GERMAN} "Es scheint so, als hätten Sie Second Life ${VERSION_LONG} bereits installiert.$\n$\nWürden Sie es gerne erneut installieren?"

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_GERMAN} "Warte darauf, dass Second Life beendet wird..."
LangString CloseSecondLifeInstMB ${LANG_GERMAN} "Second Life kann nicht installiert werden, wenn es bereits läuft.$\n$\nBeenden Sie, was Sie gerade tun und wählen Sie OK, um Second Life zu beenden oder Continue .$\nSelect CANCEL, um abzubrechen."

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_GERMAN} "Warte darauf, dass Second Life beendet wird..."
LangString CloseSecondLifeUnInstMB ${LANG_GERMAN} "Second Life kann nicht installiert werden, wenn es bereits läuft.$\n$\nBeenden Sie, was Sie gerade tun und wählen Sie OK, um Second Life zu beenden oder Continue .$\nSelect CANCEL, um abzubrechen."

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_GERMAN} "Lösche alle Cache Files in Dokumente und Einstellungen"

; delete program files
LangString DeleteProgramFilesMB ${LANG_GERMAN} "Es bestehen weiterhin Dateien in Ihrem SecondLife Programm Ordner.$\n$\nDies sind möglicherweise Dateien, die sie modifiziert oder bewegt haben:$\n$INSTDIR$\n$\nMöchten Sie diese ebenfalls löschen?"

; uninstall text
LangString UninstallTextMsg ${LANG_GERMAN} "Dies wird Second Life ${VERSION_LONG} von Ihrem System entfernen."