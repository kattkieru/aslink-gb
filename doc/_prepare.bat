@ECHO OFF
REM -- DOS script for installing the Revised SDCC Linker documentation

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  REM -- make the target directories
  ECHO ### SDCCLinker: making target directories for documentation ###
  CALL :conditionalMkDir %TARGET_DOCUMENTATION_DIR%\figures

  REM -- copy the include and source files to the appropriate directories
  ECHO ### SDCCLinker: copying files for documentation ###
  %CP_PROGRAM% linker_documentation.tex %TARGET_DOCUMENTATION_DIR%
  %CP_PROGRAM% figures\*.mp %TARGET_DOCUMENTATION_DIR%\figures
:ENDIF1

GOTO :EOF

REM ============================================================

:conditionalMkDir
  REM make directory given by %1 when it does not exist
  REM --
  IF NOT EXIST %1\NUL  MKDIR %1 >NUL
  REM RETURN
  GOTO :EOF
