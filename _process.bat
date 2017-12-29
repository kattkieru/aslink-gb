@ECHO OFF
REM -- DOS script for making the documentation and executable files

CALL _configure.bat

REM -- make the relevant subdirectories
PUSHD doc
CALL _process.bat
POPD

PUSHD src
CALL _process.bat
POPD
