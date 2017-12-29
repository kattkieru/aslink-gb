@ECHO OFF
REM -- DOS makefile for compiling revised SDCC linker

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  SETLOCAL
  ECHO ### SDCC linker: compiling ###

  SET fileNameList=main area banking codeoutput codesequence error file
  SET fileNameList=%fileNameList% globdefs integermap library list
  SET fileNameList=%fileNameList% listingupdater module map multimap
  SET fileNameList=%fileNameList% noicemapfile parser scanner set string
  SET fileNameList=%fileNameList% stringlist stringtable symbol target
  SET fileNameList=%fileNameList% typedescriptor

  FOR %%i IN (%fileNameList%) DO CALL :compile %%i

  CALL :extendedCompile mapfile
  CALL :compile platform\gameboy

  ECHO ### SDCC linker: linking ###

  FOR %%i IN (%MODULE_NAME_LIST%) DO SET objectList=!objectList! %TARGET_OBJECT_DIR%\%%i.obj
  %CLINKER% %CLINKFLAGS% /OUT:revSdccLink.exe %objectList%
  %MV_PROGRAM% revSdccLink.* %TARGET_OBJECT_DIR%

REM DEL *.obj

GOTO :EOF
REM ============================================================
REM ============================================================

:compile
  REM compile C-file %1 with CCOMPFLAGS
  REM --
  SET FLAGS=%CCOMPFLAGS% %TARGET_SOURCE_DIR%\%1.c
  SET FLAGS=%FLAGS% %CCOMP_OUTFLAG%%TARGET_OBJECT_DIR%\%1.obj
  ECHO === compiling %1.c ===
  REM ECHO %CCOMP% %FLAGS%
  %CCOMP% %FLAGS%

  REM RETURN
  GOTO :EOF

REM ====================
:extendedCompile
  REM compile C-file %1 with EXTENDED_CCOMPFLAGS
  REM --
  SET FLAGS=%EXTENDED_CCOMPFLAGS% %TARGET_SOURCE_DIR%\%1.c
  SET FLAGS=%FLAGS% %CCOMP_OUTFLAG%%TARGET_OBJECT_DIR%\%1.obj
  ECHO === compiling %1.c ===
  %CCOMP% %FLAGS%

  REM RETURN
  GOTO :EOF
