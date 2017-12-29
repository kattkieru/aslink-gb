@ECHO OFF
REM -- DOS makefile for cleaning up the temporary files generated
REM -- for the executable of the Revised SDCC linker

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  SETLOCAL
  ECHO ### SDCC linker: cleaning up executable temporary files ###

  REM -- remove the object files for all modules --
  PUSHD %TARGET_OBJECT_DIR%
  FOR %%i IN (%MODULE_NAME_LIST%) DO %RM_PROGRAM% %%i.obj
  POPD
:ENDIF1
