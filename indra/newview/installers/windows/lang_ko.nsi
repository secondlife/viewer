; First is default
LoadLanguageFile "${NSISDIR}\Contrib\Language files\Korean.nlf"

; subtitle on license text caption
LangString LicenseSubTitleUpdate ${LANG_KOREAN} "업데이트"
LangString LicenseSubTitleSetup ${LANG_KOREAN} " 설치하기"

; description on license page
LangString LicenseDescUpdate ${LANG_KOREAN} "이 팩키지는 세컨드라이프를 버전${VERSION_LONG}.으로 업데이트 합니다. "
LangString LicenseDescSetup ${LANG_KOREAN} "이 팩키지는 세컨드라이프를 컴퓨터에 설치합니다."
LangString LicenseDescNext ${LANG_KOREAN} "다음"

; installation directory text
LangString DirectoryChooseTitle ${LANG_KOREAN} "설치 디렉토리"
LangString DirectoryChooseUpdate ${LANG_KOREAN} "세컨드라이프를 업데이트할 디렉토리를 선택하세요. "
LangString DirectoryChooseSetup ${LANG_KOREAN} "세컨드라이프를 설치할 디렉토리를 선택하세요:"

; CheckStartupParams message box
LangString CheckStartupParamsMB ${LANG_KOREAN} " ‘$INSTPROG’ 프로그램을 찾지 못했습니다. 자동 업데이트에 실패했습니다."

; installation success dialog
LangString InstSuccesssQuestion ${LANG_KOREAN} "세컨드라이프를 시작하겠습니까?"

; remove old NSIS version
LangString RemoveOldNSISVersion ${LANG_KOREAN} "이전 버전을 찾고 있습니다… "

; check windows version
LangString CheckWindowsVersionDP ${LANG_KOREAN} "윈도우 버전을 확인하고 있습니다."
LangString CheckWindowsVersionMB ${LANG_KOREAN} "세컨드라이프는 윈도우 XP, 윈도우 2000, 그리고 맥 OS X를 지원합니다. 윈도우 $R0에 설치를 시도하면 오작동과 데이터 분실이 일어날 수 있습니다. 계속 설치하겠습니까? "

; checkifadministrator function (install)
LangString CheckAdministratorInstDP ${LANG_KOREAN} "설치 권한을 확인 중입니다... "
LangString CheckAdministratorInstMB ${LANG_KOREAN} "현재 ‘손님’계정을 사용 중입니다. 세컨드라이프를 설치하기 위해선 ‘운영자” 계정을 사용해야 합니다."

; checkifadministrator function (uninstall)
LangString CheckAdministratorUnInstDP ${LANG_KOREAN} "제거 권한을 확인 중입니다. "
LangString CheckAdministratorUnInstMB ${LANG_KOREAN} " 현재 ‘손님’계정을 사용 중입니다. 세컨드라이프를 제거하기 위해선 ‘운영자” 계정을 사용해야 합니다. "

; checkifalreadycurrent
LangString CheckIfCurrentMB ${LANG_KOREAN} "세컨드라이프 버전 ${VERSION_LONG}이 이미 설치되어 있습니다. 다시 설치하시겠습니까? "

; closesecondlife function (install)
LangString CloseSecondLifeInstDP ${LANG_KOREAN} "세컨드라이프를 종료할 때 까지 대기 중… "
LangString CloseSecondLifeInstMB ${LANG_KOREAN} "세컨드라이프가 이미 작동 중일 경우 설치를 계속 할 수 없습니다. 현재 작업을 멈추고 ‘확인’을 눌러 세컨드라이프를 종료한 다음 진행하기 바랍니다. 설치를 취소하려면 ‘취소’를 누르세요." 

; closesecondlife function (uninstall)
LangString CloseSecondLifeUnInstDP ${LANG_KOREAN} "세컨드라이프를 종료할 때 까지 대기 중…”"
LangString CloseSecondLifeUnInstMB ${LANG_KOREAN} " 세컨드라이프가 이미 작동 중일 경우 제거를 계속 할 수 없습니다. 현재 작업을 멈추고 ‘확인’을 눌러 세컨드라이프를 종료한 다음 진행하기 바랍니다. 설치를 취하려면 ‘취소’를 누르세요. "

; removecachefiles
LangString RemoveCacheFilesDP ${LANG_KOREAN} " Documents and Settings 폴더 내의 캐시 파일들을 지웁니다."

; delete program files
LangString DeleteProgramFilesMB ${LANG_KOREAN} "세컨드라이프 프로그램 디렉토리에 아직 파일들이 남아 있습니다. 이 파일들은 사용자가 만든 것들이거나$\n$INSTDIR$\n$\n로 이동한 파일들일 수 있습니다. 이 파일들을 제거하겠습니까?" 

; uninstall text
LangString UninstallTextMsg ${LANG_KOREAN} "세컨드라이프${VERSION_LONG}을 시스템에서 제거합니다."
