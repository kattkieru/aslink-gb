@ECHO OFF
REM -- DOS script for installing the Revised SDCC Linker

CALL _configure.bat

REM -- make the target directories
ECHO ### SDCCLinker: making target directories ###
CALL :conditionalMkDir %TARGET_INCLUDE_DIR%
CALL :conditionalMkDir %TARGET_INCLUDE_DIR%\platform
CALL :conditionalMkDir %TARGET_SOURCE_DIR%
CALL :conditionalMkDir %TARGET_SOURCE_DIR%\platform
CALL :conditionalMkDir %TARGET_OBJECT_DIR%
CALL :conditionalMkDir %TARGET_OBJECT_DIR%\platform
CALL :conditionalMkDir %TARGET_DOCUMENTATION_DIR%

REM -- install files from the relevant subdirectories
PUSHD doc
CALL _prepare.bat
POPD

PUSHD src
CALL _prepare.bat
POPD

GOTO :EOF

REM ============================================================

:conditionalMkDir
  REM make directory given by %1 when it does not exist
  REM --
  IF NOT EXIST %1\NUL  MKDIR %1 >NUL
  REM RETURN
  GOTO :EOF
