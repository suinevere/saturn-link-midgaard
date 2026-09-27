:; export SRL_INSTALL_ROOT="../SaturnRingLib"; case "$(uname -s)" in Darwin) export PATH="../SaturnRingLib/Compiler/mac/sh2eb-elf/bin:$PATH";; Linux) export PATH="../SaturnRingLib/Compiler/linux/sh2eb-elf/bin:$PATH";; MINGW*|MSYS*) export PATH="../SaturnRingLib/Compiler/sh2eb-elf/bin:$PATH:../SaturnRingLib/Compiler/msys2/usr/bin"; export TMPDIR=/tmp TMP=/tmp TEMP=/tmp;; esac; if [ "$1" = "clean" ]; then make clean NETBIN=1; rm -f BuildDrop/coffeemud.netbin; exit; fi; if [ "$1" = "debug" ]; then make all NETBIN=1 LDFILE=./sgl-netbin.linker DEBUG=1; else make all NETBIN=1 LDFILE=./sgl-netbin.linker; fi; sh ./package-netbin.sh; exit;
@ECHO Off
SETLOCAL
SET "SRL_INSTALL_ROOT=../SaturnRingLib"
SET "CDIR=%~dp0..\SaturnRingLib\Compiler"
SET "PATH=%CDIR%\sh2eb-elf\bin;%CDIR%\msys2\usr\bin;%CDIR%\Other Utilities;%PATH%"
IF /I "%~1"=="clean" (make clean NETBIN=1 & DEL /Q BuildDrop\coffeemud.netbin 2>NUL & GOTO done)
IF /I "%~1"=="debug" (make all NETBIN=1 LDFILE=./sgl-netbin.linker DEBUG=1 & GOTO package)
make all NETBIN=1 LDFILE=./sgl-netbin.linker
:package
IF ERRORLEVEL 1 GOTO done
sh package-netbin.sh
:done
ENDLOCAL
