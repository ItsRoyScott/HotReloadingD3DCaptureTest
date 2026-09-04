!include x64.nsh
Name "libjpeg-turbo SDK for Visual C++ 64-bit"
OutFile "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}libjpeg-turbo-3.2.0-vc-x64.exe"
InstallDir "D:\Projects\HotReloadingD3DCapture\SegmentationMap\vcpkg_installed\x64-windows\vcpkg\pkgs\libjpeg-turbo_x64-windows"

SetCompressor bzip2

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "libjpeg-turbo SDK for Visual C++ 64-bit (required)"
!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif
	SectionIn RO
!ifdef GCC
	IfFileExists $SYSDIR/libturbojpeg.dll exists 0
!else
	IfFileExists $SYSDIR/turbojpeg.dll exists 0
!endif
	goto notexists
	exists:
!ifdef GCC
	MessageBox MB_OK "An existing version of the libjpeg-turbo SDK for Visual C++ 64-bit is already installed.  Please uninstall it first."
!else
	MessageBox MB_OK "An existing version of the libjpeg-turbo SDK for Visual C++ 64-bit or the TurboJPEG SDK is already installed.  Please uninstall it first."
!endif
	quit

	notexists:
	SetOutPath $SYSDIR
!ifdef GCC
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libturbojpeg.dll"
!else
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}turbojpeg.dll"
!endif
	SetOutPath $INSTDIR\bin
!ifdef GCC
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libturbojpeg.dll"
!else
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}turbojpeg.dll"
!endif
!ifdef GCC
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libjpeg-62.dll"
!else
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}jpeg62.dll"
!endif
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}cjpeg.exe"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}djpeg.exe"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}jpegtran.exe"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}tjbench.exe"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}rdjpgcom.exe"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}wrjpgcom.exe"
	SetOutPath $INSTDIR\lib
!ifdef GCC
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libturbojpeg.dll.a"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libturbojpeg.a"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libjpeg.dll.a"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\libjpeg.a"
!else
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}turbojpeg.lib"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}turbojpeg-static.lib"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}jpeg.lib"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\${BUILDDIR}jpeg-static.lib"
!endif
	SetOutPath $INSTDIR\lib\pkgconfig
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\pkgscripts\libjpeg.pc"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\pkgscripts\libturbojpeg.pc"
	SetOutPath $INSTDIR\lib\cmake\libjpeg-turbo
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\pkgscripts\libjpeg-turboConfig.cmake"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\pkgscripts\libjpeg-turboConfigVersion.cmake"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\win\libjpeg-turboTargets.cmake"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\win\libjpeg-turboTargets-release.cmake"
!ifdef JNA
	SetOutPath $INSTDIR\classes
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\jna\turbojpeg-jna.jar"
!endif
	SetOutPath $INSTDIR\include
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/x64-windows-rel\jconfig.h"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\jerror.h"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\jmorecfg.h"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\jpeglib.h"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\turbojpeg.h"
	SetOutPath $INSTDIR\doc
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\README.ijg"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\README.md"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\LICENSE.md"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\example.c"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\libjpeg.txt"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\structure.txt"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\usage.txt"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\wizard.txt"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\tjcomp.c"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\tjdecomp.c"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\src\tjtran.c"
	SetOutPath $INSTDIR\doc\jna
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\jna\TJ.java"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\jna\TJComp.java"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\jna\TJDecomp.java"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\jna\TJTran.java"
!ifdef GCC
	SetOutPath $INSTDIR\man\man1
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\cjpeg.1"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\djpeg.1"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\jpegtran.1"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\rdjpgcom.1"
	File "D:/Projects/HotReloadingD3DCapture/SegmentationMap/vcpkg_installed/x64-windows/vcpkg/blds/libjpeg-turbo/src/3.2.0-3cdbbb0a83.clean\doc\wrjpgcom.1"
!endif

	WriteRegStr HKLM "SOFTWARE\libjpeg-turbo64 3.2.0" "Install_Dir" "$INSTDIR"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo64 3.2.0" "DisplayName" "libjpeg-turbo SDK v3.2.0 for Visual C++ 64-bit"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo64 3.2.0" "UninstallString" '"$INSTDIR\uninstall_3.2.0.exe"'
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo64 3.2.0" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo64 3.2.0" "NoRepair" 1
	WriteUninstaller "uninstall_3.2.0.exe"
SectionEnd

Section "Uninstall"
!ifdef WIN64
	${If} ${RunningX64}
	${DisableX64FSRedirection}
	${Endif}
!endif

	SetShellVarContext all

	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\libjpeg-turbo64 3.2.0"
	DeleteRegKey HKLM "SOFTWARE\libjpeg-turbo64 3.2.0"

!ifdef GCC
	Delete $INSTDIR\bin\libjpeg-62.dll
	Delete $INSTDIR\bin\libturbojpeg.dll
	Delete $SYSDIR\libturbojpeg.dll
	Delete $INSTDIR\lib\libturbojpeg.dll.a
	Delete $INSTDIR\lib\libturbojpeg.a
	Delete $INSTDIR\lib\libjpeg.dll.a
	Delete $INSTDIR\lib\libjpeg.a
!else
	Delete $INSTDIR\bin\jpeg62.dll
	Delete $INSTDIR\bin\turbojpeg.dll
	Delete $SYSDIR\turbojpeg.dll
	Delete $INSTDIR\lib\jpeg.lib
	Delete $INSTDIR\lib\jpeg-static.lib
	Delete $INSTDIR\lib\turbojpeg.lib
	Delete $INSTDIR\lib\turbojpeg-static.lib
!endif
	Delete $INSTDIR\lib\pkgconfig\libjpeg.pc
	Delete $INSTDIR\lib\pkgconfig\libturbojpeg.pc
	Delete $INSTDIR\lib\cmake\libjpeg-turbo\libjpeg-turboConfig.cmake
	Delete $INSTDIR\lib\cmake\libjpeg-turbo\libjpeg-turboConfigVersion.cmake
	Delete $INSTDIR\lib\cmake\libjpeg-turbo\libjpeg-turboTargets.cmake
	Delete $INSTDIR\lib\cmake\libjpeg-turbo\libjpeg-turboTargets-release.cmake
!ifdef JNA
	Delete $INSTDIR\classes\turbojpeg-jna.jar
!endif
	Delete $INSTDIR\bin\cjpeg.exe
	Delete $INSTDIR\bin\djpeg.exe
	Delete $INSTDIR\bin\jpegtran.exe
	Delete $INSTDIR\bin\tjbench.exe
	Delete $INSTDIR\bin\rdjpgcom.exe
	Delete $INSTDIR\bin\wrjpgcom.exe
	Delete $INSTDIR\include\jconfig.h
	Delete $INSTDIR\include\jerror.h
	Delete $INSTDIR\include\jmorecfg.h
	Delete $INSTDIR\include\jpeglib.h
	Delete $INSTDIR\include\turbojpeg.h
	Delete $INSTDIR\uninstall_3.2.0.exe
	Delete $INSTDIR\doc\README.ijg
	Delete $INSTDIR\doc\README.md
	Delete $INSTDIR\doc\LICENSE.md
	Delete $INSTDIR\doc\example.c
	Delete $INSTDIR\doc\libjpeg.txt
	Delete $INSTDIR\doc\structure.txt
	Delete $INSTDIR\doc\usage.txt
	Delete $INSTDIR\doc\wizard.txt
	Delete $INSTDIR\doc\tjcomp.c
	Delete $INSTDIR\doc\tjdecomp.c
	Delete $INSTDIR\doc\tjtran.c
	Delete $INSTDIR\doc\jna\TJ.java
	Delete $INSTDIR\doc\jna\TJComp.java
	Delete $INSTDIR\doc\jna\TJDecomp.java
	Delete $INSTDIR\doc\jna\TJTran.java
!ifdef GCC
	Delete $INSTDIR\man\man1\cjpeg.1
	Delete $INSTDIR\man\man1\djpeg.1
	Delete $INSTDIR\man\man1\jpegtran.1
	Delete $INSTDIR\man\man1\rdjpgcom.1
	Delete $INSTDIR\man\man1\wrjpgcom.1
!endif

	RMDir "$INSTDIR\include"
	RMDir "$INSTDIR\lib\pkgconfig"
	RMDir "$INSTDIR\lib\cmake\libjpeg-turbo"
	RMDir "$INSTDIR\lib\cmake"
	RMDir "$INSTDIR\lib"
	RMDir "$INSTDIR\doc\jna"
	RMDir "$INSTDIR\doc"
!ifdef GCC
	RMDir "$INSTDIR\man\man1"
	RMDir "$INSTDIR\man"
!endif
!ifdef JNA
	RMDir "$INSTDIR\classes"
!endif
	RMDir "$INSTDIR\bin"
	RMDir "$INSTDIR"

SectionEnd
