@ECHO OFF
REM -- DOS script for deploying the Revised SDCC Linker documentation

CALL ..\_configure.bat
CALL _localconfigure.bat

IF DEFINED TARGET_INCLUDE_DIR GOTO :ELSE1
  ECHO ### ERROR: configuration file not adapted - please edit it first ###
  GOTO :ENDIF1
:ELSE1
  ECHO ### SDCCLinker: deploying documentation ###
  SET pdfFile=%TARGET_DOCUMENTATION_DIR%\linker_documentation.pdf
  %CP_PROGRAM% %pdfFile% %TARGET_DOCUMENTATION%
:ENDIF1
