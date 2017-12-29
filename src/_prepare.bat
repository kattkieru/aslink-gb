@ECHO OFF
REM -- DOS script for installing the Revised SDCC Linker sources

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  REM -- copy the include and source files to the appropriate directories
  ECHO ### SDCCLinker: copying source files ###
  %CP_PROGRAM% *.h %TARGET_INCLUDE_DIR%
  %CP_PROGRAM% *.c %TARGET_SOURCE_DIR%
  %CP_PROGRAM% platform\*.h %TARGET_INCLUDE_DIR%\platform
  %CP_PROGRAM% platform\*.c %TARGET_SOURCE_DIR%\platform
:ENDIF1
