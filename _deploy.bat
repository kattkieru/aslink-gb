@ECHO OFF
REM -- DOS script for deploying the Revised SDCC Linker files

CALL _configure.bat

REM -- deploy the relevant subdirectories

PUSHD doc
CALL _deploy
POPD

PUSHD src
CALL _deploy
POPD
