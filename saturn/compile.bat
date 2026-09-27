:; export SRL_INSTALL_ROOT="../SaturnRingLib"; case "$(uname -s)" in Darwin) export PATH="../SaturnRingLib/Compiler/mac/sh2eb-elf/bin:$PATH";; Linux) export PATH="../SaturnRingLib/Compiler/linux/sh2eb-elf/bin:$PATH";; MINGW*|MSYS*) export PATH="../SaturnRingLib/Compiler/sh2eb-elf/bin:$PATH:../SaturnRingLib/Compiler/msys2/usr/bin"; export TMPDIR=/tmp TMP=/tmp TEMP=/tmp;; esac; if [ "${1:-debug}" = "clean" ]; then make clean; elif [ "${1:-debug}" = "release" ]; then make all; else make all DEBUG=1; fi; exit;
@ECHO Off
SETLOCAL
IF "%~1"=="" (SET "TGT=debug") ELSE (SET "TGT=%~1")
SET "SRL_INSTALL_ROOT=../SaturnRingLib"
SET "CDIR=%~dp0..\SaturnRingLib\Compiler"
SET "PATH=%CDIR%\sh2eb-elf\bin;%CDIR%\msys2\usr\bin;%CDIR%\Other Utilities;%PATH%"
IF /I "%TGT%"=="clean"   GOTO doclean
IF /I "%TGT%"=="release" GOTO dorelease
make all DEBUG=1
GOTO done
:dorelease
make all
GOTO done
:doclean
make clean
GOTO done
:done
ENDLOCAL
