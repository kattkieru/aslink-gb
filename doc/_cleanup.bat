@ECHO OFF
REM -- DOS makefile for cleaning up the temporary files generated
REM -- for the documentation of the Revised SDCC linker

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  SETLOCAL
  ECHO ### SDCC linker: cleaning up documentation temporary files ###

  PUSHD %DOCUMENTATION_FIGURE_DIRECTORY%
  %RM_PROGRAM% *.mps *.log *.mpx
  POPD

  REM -- remove the interface description TeX file for all modules --
  PUSHD %TARGET_DOCUMENTATION_DIR%
  FOR %%i IN (%SUPPORTING_MODULE_NAME_LIST%) DO %RM_PROGRAM% %%~ni.tex
  POPD
:ENDIF1
