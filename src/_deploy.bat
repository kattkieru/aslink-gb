@ECHO OFF
REM -- DOS script for deploying the Revised SDCC Linker executable

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  ECHO ### SDCCLinker: deploying executable ###
  SET executableFile=%TARGET_OBJECT_DIR%\revSdccLink.exe
  %CP_PROGRAM% %executableFile% %TARGET_EXECUTABLE%
:ENDIF1
