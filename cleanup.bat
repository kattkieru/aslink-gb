@ECHO OFF
REM -- DOS script for cleaning up the documentation and executable
REM -- files for the Revised SDCC Linker

CALL _configure.bat

REM -- cleanup the relevant subdirectories
PUSHD doc
CALL _cleanup.bat
POPD

PUSHD src
CALL _cleanup.bat
POPD
