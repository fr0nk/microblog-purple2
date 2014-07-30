; Script based on the Skype4Pidgin, Off-the-Record Messaging and Facebook Chat NSI files


SetCompress auto
SetCompressor lzma

; todo: SetBrandingImage
; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "pidgin-microblog"
;!define PRODUCT_VERSION "0.1.1"
!define PRODUCT_PUBLISHER "Somsak Sriprayoonsakul"
!define PRODUCT_WEB_SITE "http://microblog-purple.googlecode.com/"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "COPYING"
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Run Pidgin"
!define MUI_FINISHPAGE_RUN_FUNCTION "RunPidgin"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

!define SHELLFOLDERS "Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders"
 
; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "${PRODUCT_NAME}.exe"

Var "PidginDir"

ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
    ;Check for pidgin installation
    Call GetPidginInstPath
    
	; Removing deprecated file
	Delete "$PidginDir\protocols\16\laconica.png"
	Delete "$PidginDir\protocols\22\laconica.png"
	Delete "$PidginDir\protocols\48\laconica.png"
	
    SetOverwrite try
	
	; Icon
	SetOutPath "$PidginDir\pixmaps\pidgin"
	File "/oname=protocols\16\twitter.png" "microblog\twitter16.png"
	File "/oname=protocols\22\twitter.png" "microblog\twitter22.png"
	File "/oname=protocols\48\twitter.png" "microblog\twitter48.png"
	File "/oname=protocols\16\identica.png" "microblog\identica16.png"
	File "/oname=protocols\22\identica.png" "microblog\identica22.png"
	File "/oname=protocols\48\identica.png" "microblog\identica48.png"
	File "/oname=protocols\16\statusnet.png" "microblog\statusnet16.png"
	File "/oname=protocols\22\statusnet.png" "microblog\statusnet22.png"
	File "/oname=protocols\48\statusnet.png" "microblog\statusnet48.png"

	;CA Certs
	SetOverwrite try
	SetOutPath "$PidginDir\ca-certs"
	File "certs\EquifaxSecureGlobaleBusinessCA.pem"
	
	; main DLL
    SetOverwrite try
	copy:
		ClearErrors
		;Delete "$PidginDir\plugins\libtwitter.dll"
		Delete "$PidginDir\plugins\liblaconica.dll"
		IfErrors dllbusy
		SetOutPath "$PidginDir\plugins"
		File "microblog\libtwitter.dll"
		File "microblog\libidentica.dll"
		File "microblog\libstatusnet.dll"
		File "twitgin\twitgin.dll"
		;SetOutPath "$PidginDir"
		;File "microblog\mbchprpl.exe"
		writeUninstaller "$PidginDir\pidgin-microblog-uninst.exe"
		WriteRegStr "${PRODUCT_UNINST_ROOT_KEY}" "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}  - Microblogging support for Pidgin"
		WriteRegStr "${PRODUCT_UNINST_ROOT_KEY}" "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
		WriteRegStr "${PRODUCT_UNINST_ROOT_KEY}" "${PRODUCT_UNINST_KEY}" "UninstallString" "$PidginDir\pidgin-microblog-uninst.exe"
		Goto after_copy
	dllbusy:
		MessageBox MB_RETRYCANCEL "DLLs are busy. Please close Pidgin (including tray icon) and try again" IDCANCEL cancel
		Goto copy
	cancel:
		Abort "Installation of pidgin-microblog aborted"
	after_copy:
		;Call FixAccount
		
SectionEnd

Section "Uninstall"
	Call un.GetPidginInstPath
	
	uninstall: 
		; certs
		Delete "$PidginDir\ca-certs\EquifaxSecureGlobaleBusinessCA.pem"
		; icons
		Delete "$PidginDir\protocols\16\twitter.png"
		Delete "$PidginDir\protocols\22\twitter.png"
		Delete "$PidginDir\protocols\48\twitter.png"
		Delete "$PidginDir\protocols\16\identica.png"
		Delete "$PidginDir\protocols\22\identica.png"
		Delete "$PidginDir\protocols\48\identica.png"
		Delete "$PidginDir\protocols\16\statusnet.png"
		Delete "$PidginDir\protocols\22\statusnet.png"
		Delete "$PidginDir\protocols\48\statusnet.png"
		; main DLLs
		Delete "$PidginDir\plugins\libtwitter.dll"
		Delete "$PidginDir\plugins\libidentica.dll"
		Delete "$PidginDir\plugins\libstatusnet.dll"
		Delete "$PidginDir\plugins\twitgin.dll"
		IfErrors dllbusy
		Delete "$PidginDir\pidgin-microblog-uninst.exe"
		DeleteRegKey "${PRODUCT_UNINST_ROOT_KEY}" "${PRODUCT_UNINST_KEY}"
		goto afteruninstall
	dllbusy: 
		MessageBox MB_RETRYCANCEL "DLLs are busy. Please close Pidgin (including tray icon) and try again" IDCANCEL cancel
		Goto uninstall
	cancel:
		Abort "Uninstall of pidgin-microblog aborted"
	afteruninstall:
		
SectionEnd

Function GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	ReadRegStr $0 HKCU "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
		MessageBox MB_OK|MB_ICONINFORMATION "Failed to find Pidgin installation."
		Abort "Failed to find Pidgin installation. Please install Pidgin first."
  cont:
	StrCpy $PidginDir $0
FunctionEnd

Function un.GetPidginInstPath
  Push $0
  ReadRegStr $0 HKLM "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
	ReadRegStr $0 HKCU "Software\pidgin" ""
	IfFileExists "$0\pidgin.exe" cont
		MessageBox MB_OK|MB_ICONINFORMATION "Failed to find Pidgin installation."
		Abort "Failed to find Pidgin installation. Please install Pidgin first."
  cont:
	StrCpy $PidginDir $0
FunctionEnd

Function RunPidgin
	ExecShell "" "$PidginDir\pidgin.exe"
FunctionEnd

Function FixAccount
  ReadRegStr $0 HKCU "${SHELLFOLDERS}" AppData
	StrCmp $0 "" 0 +2
		ReadRegStr $0 HKLM "${SHELLFOLDERS}" "Common AppData"
	StrCmp $0 "" 0 +2
		StrCpy $0 "$WINDIR\Application Data"
	IfFileExists "$0\Roaming\.purple\accounts.xml" cont
  cont: 
	ExecWait "$PidginDir\mbchprpl.exe"
FunctionEnd

