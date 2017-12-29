@ECHO OFF
REM -- DOS makefile for generating documentation for Revised SDCC linker

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  SETLOCAL
  ECHO ### SDCC linker: processing documentation ###

  SET awkFile=%TOOLS_DIR%\cIncludeToLatex.awk

  PUSHD %DOCUMENTATION_FIGURE_DIRECTORY%
  ECHO === preparing documentation figures with MetaPost ===
  %METAPOST% -quiet linkerFigures
  POPD

  REM -- prepare the interface description for all modules --
  FOR %%i IN (%SUPPORTING_MODULE_NAME_LIST%) DO CALL :makeTexFromInclude %%i

  PUSHD %TARGET_DOCUMENTATION_DIR%
  ECHO === preparing documentation with LaTeX ===
  REM -- run LaTeX twice for correct labels --
  %PDFLATEX% -quiet linker_documentation
  %PDFLATEX% -quiet linker_documentation
  POPD

GOTO :EOF

REM ============================================================

:makeTexFromInclude
  REM make TeX file from include file given as %1
  REM   %1 include file path name relative to %TARGET_INCLUDE_DIR%
  REM --

  ECHO === converting interface of module %1 ===
  SET includeFile=%TARGET_INCLUDE_DIR%\%1.h
  SET texFile=%TARGET_DOCUMENTATION_DIR%\%~n1.tex
  %GAWK_PROGRAM% -f %awkFile% %includeFile% >%texFile%

  REM RETURN
  GOTO :EOF
